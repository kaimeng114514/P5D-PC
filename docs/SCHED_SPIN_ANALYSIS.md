# P5D 静态重编译 harness 调度空转根因分析（只读）

> 分析对象
> - harness 源码：`C:\dev\tmp\recomp_split\recomp_harness.cpp`（当前 2026-10-28 14:04 版，3470 行；下文行号以此版为准）
> - 现场日志：`out100m_682a2.txt`（100M 块，116MB）、`out30m_682a2.txt`（30M 块）、`err100m_682a2.txt`
> - 反汇编：`C:\dev\p5d_build\p5d_true.elf`（capstone 5.0.7 / Thumb）
> - 旧运行时：`C:\dev\p5dsrc\src\loader\eboot_probe.cpp`、`C:\dev\p5dsrc\src\arm\thumb_interpreter.cpp`、旧运行日志 `C:\dev\p5d_build\run.log`
> - 已有研究：`docs/OLD_RUNTIME_FIBER_HLE.md`
>
> 约束：本文件是本次唯一写出的文件；未修改 harness、生成器、其它 docs；不含游戏数据。

---

## 0. 结论摘要（先看这里）

1. **空转不是"主线程在正常等事件"，而是"guest 的等待者与生产者被 harness 的调度模型永久隔开"**。
   当前 100M 块里，guest 可见进度在 **total≈1,891,302 块（约 n=1760）之后完全冻结**：
   `[CNT] 81198BEA=749 811E5CDA=749 811E5928=749 8120683C=0` 在 30M 与 100M 日志中逐字相同，
   帧计数 `[FRAMEDUMP] ... flags=00000002 t4=0 t8=0 ctr=0`（20000 次采样全为 0）。
2. **第一层根因（P0，最关键）：fiber 运行期间 guest 线程被结构性禁止运行**。
   `hle_thr_pump()` 的入口门控 `if (g_cur_thr || g_in_fiber_exec || g_cur_fiber) return;`
   （`recomp_harness.cpp:2255`）意味着：
   - 嵌套 boot 阶段（`P5D_NESTED_UNTIL=1500000`，`g_in_fiber_exec=1`）线程泵被关闭；
   - `[NESTED-STOP]` 之后主循环恢复 fiber 上下文（`g_cur_fiber≠0`），线程泵**永久**关闭。
   而 app 初始化本来就是"fiber 发起异步任务 + Vita worker 线程完成 + 事件/信号量唤醒"的结构
   （旧运行日志里有 0x6006~0x6010 共 10 条线程、`[SEM]/[EVT]` 唤醒、CPK 加载状态机一路推进）。
   新 harness 在嵌套 boot 时就死在 `0x8127BE36/FUN_8127be36`（每隔一迭代 `sceKernelSetEventFlag(0x60016000,1)`，
   日志 100M 行 11301 起大量 `[EVT] set obj=60016000 ... lr=8127BE56`）——**这就是 1.5M 块被耗光的那个自旋**。
3. **第二层根因（P0）：fiber 管理链表被 harness 人为写坏**。
   `fiber_load()` 首次装载时把 `[fiber+0x80] |= 1`（`recomp_harness.cpp:1189-1193`，`[FACT]`）。
   而 guest 的 `0x812067A0` 正是用 bit0 判断"该 fiber 已完成、应摘链"：一旦置 1，
   当前 fiber 会被**摘链并标记 finished（`+0x80` 变成 0x3）**，另一个 fiber 再调 `0x812067A0` 时
   `target = prev ?: 链表尾` 退化为**自己** → 分支 `cmp r3,r0; beq 8120682A` 直接跳过 `sceFiberSwitch`
   → **永不切回对方**。日志实测：`[GSELF] ... f1=848367C0/st=00000003 f2=84846860/st=00000001`
   （A 已 finished、B 仍在链上），且 `[FSW]` 只有 A→B 方向、没有 B→A 方向。
4. 第 3~5 层是补丁本身制造的二次破坏：`[SCHEDRUN]` 强制重置 B 并从入口重跑
   （`recomp_harness.cpp:2886-2908`）→ 被 `[YMAIN]`（DelayThread 每 64 次 unwind，`:1157`）打断
   → harness **伪造成 B 已完成**（`:2902` `[fb+0x80]|=2`，日志 `[SCHEDRUN] ... done bit2-set`）
   → 主线程 join 退出、`main` 返回（`[MAINRET] n=1760`）→ 主循环用错误现场恢复 A
   （`[682A-SKIP] sp=A0DFFFF0 [sp+4]=81206BEC`，A 的 scheduler 帧被 trampoline 帧顶替）
   → `DelayThread` 在非嵌套 fiber 上下文里是**纯 no-op**（`:1155-1167`）→ B 的等待循环变成纯忙等
   → 主循环每 2000 块 pump 一次 A 的 lifecycle 轮、pump 的 switch 请求又被丢弃
   → 形成 100M 块、每秒数千万块的"空转"。
5. 一句话：**"boot 完成后调度空转"的真正状态是：主循环在反复执行
   `GetSelf(812B1B70) → DelayThread(812B2A50) → GetThreadId(812B27B0) → 退出码调度器(812067A0)
   → canary 尾声(8120682A/81206832)` 的 B 等待循环，同时 pump 每隔 2000 块跑一次 A 的
   0x81198BE8 轮；两条 guest 控制流都被 harness 的 fake 调度卡在原处，frame counter 从未被写。**

---

## 1. 证据基线

### 1.1 100M / 30M 日志的硬指标

| 指标 | 100M（`out100m_682a2.txt`） | 30M（`out30m_682a2.txt`） | 含义 |
|---|---|---|---|
| `done:` 行 | `blocks=100000000 pc=812067E8 last_block=812067DC r0=00000000 r9=00000003` | `blocks=30000000 pc=812067E8 ...` | 主循环空转 |
| `[CNT]` | `81198BEA=749 811E5CDA=749 811E5928=749 8120683C=0 81206860=0 FSLOT=749` | **完全相同** | fiber_exec 内 guest 进度冻结 |
| `[CNT2]` | `mgr=781 67B0=0 67E8=781 680E=781 681C=781 6820=781 6822=749` | **完全相同** | 调度器 count 冻结 |
| `[FRAMEDUMP]` | 20000 条全部 `flags=00000002 t4=0 t8=0 ctr=0` | 同上 | 帧循环从未进入 |
| `[WD] loop=` | 54 条；`loop=0` 的 `cnt` 冻结在 1,891,301（`total=1,891,302`），之后只有 `loop=2/3` | 同样形态 | 嵌套 fiber_exec 不再运行 |
| `[682A-SKIP]` | 1 次：`sp=A0DFFFF0 [sp+4]=81206BEC canary=812B4150 lr=81198BEE fib=848367C0` | — | A 的 scheduler 帧错位 |
| `[SCHEDRUN]` | 2 次（`#1 run`、`ret=... done bit2-set`） | — | 伪完成 |
| `[MAINRET]` | 1 次 `n=1760 -> resume fiber=848367C0 pc=8120682A` | — | 主线程被放行 |
| `[GETSELF-DUMMY]` | `thread-level -> 8497D820`（5 次相关打印） | — | GetSelf 线程级返回假对象 |
| `[NESTED-STOP]` | `total=1500000 pc=8127BE36 cur=84846860` | — | 混合 boot 截断点 |
| `err` 尾部 | `[COUNT] present=0 begin_pass=0 draw=0` | — | Vulkan 后端无任何帧提交 |
| `[TOPBLK]`（100M） | `812B1B70 x6100707, 812B2A50 x6049957, 812067A0 x3050727, 812067DC x3050727, 812067E8 x3050726, 81207632 x3049978, 811E5ECE x3049978, 811E5CDA x749, 8120683C x0` | — | 稳态自旋块构成 |
| `[HLECOUNT]`（100M） | total=15,607,211；`812B1B70 x6151916`（GetSelf）、`812B2A50 x6051302`（DelayThread）、`812B27B0 x3050584`（GetThreadId）、`812B1BA0 x50748`（Switch）、`812B2AC0 x50748`（CheckCallback） | — | 每 2 块一次 GetSelf、每 2 块一次 DelayThread |

### 1.2 关键时间线（100M 日志行号）

```
106340 [NESTED-STOP] total=1500000 pc=8127BE36 cur=84846860
106354 [SCHEDRUN] #1 run fiber=84846860 entry=81198C0B arg=82EFFF28
       [SCHEDRUN3] ctx reset fiber=84846860 spy=A0D80000 entry=81198C0A
107850 [SCHEDRUN] fiber=84846860 ret=00000000 done bit2-set
107854 [FIBHLE] 最近 40 条 HLE：
         812B2A50@8120766A / 812B1B70@812067B6 / 812B27B0@8120765C /
         812B1B70@81207648 / 812B2A50@81207612  —— 五条一循环
107893 [MAINRET] main returned at n=1760 -> resume fiber=848367C0 pc=8120682A
107895 [FIBLIST] [0]=848367C0 entry=81198BE9 ctxpc=8120682A sp=A0DFFFD8
                [1]=84846860 entry=81198C0B ctxpc=8120766A sp=A0D7FD90
107947 [682A-SKIP] sp=A0DFFFF0 [sp+4]=81206BEC canary=812B4150 lr=81198BEE fib=848367C0
107948 [MSW] sync switch -> 84846860 pc=811E5EE8 r0=812B4150
107985 [PUMPSW] stop pump pass: switch_to=84846860 flag_return=0 ppc=812B1BA0 resume=8120682A
...（之后每 pass 都一样：PMP2B 37 块、PUMPSW、A 的 ctx 固定保存为 pc=8120682A）
100M 末尾 [PCRING] 81206806 8120680E 81206810 8120681C 81206820 8120682A 81206832
        81207664 812B2A50 8120766A 81207672 811E5EE8 811E5EC6 811E5ECE ...
```

**两个交替的上下文：**

- **主循环（loop=2）** 反复执行 fiber **B=0x84846860**（entry=0x81198C0B）的"等待/让出"循环：
  `811E5EC6 → 811E5ECE → 8120760C(DelayThread(30)) → 811E5ED4 → 81207632 → GetSelf → GetThreadId → 812067A0`
  `→ 812067B6/812067C8/812067DC/812067E8 → 8120680E/8120681C/81206820/8120682A/81206832`
  `→ 81207664 → DelayThread(30) → ...`
- **pump（loop=3）** 反复执行 fiber **A=0x848367C0**（entry=0x81198BE9）的 lifecycle 轮：
  `8120682A → 81206832 → 81198BF6 → 81198BEA → 811E5CDA → 811E5CE8 → 811E5928(FIOS 队列回调)`
  `→ 81207344/812B2830/81207350 → 811E5962 → 811E5970 → 811E5AEC → ... → 81198BEE → 81198BF2`
  `→ 812067A0 → 81206826 → sceFiberSwitch(B) → PUMPSW 打断 → 丢弃请求 → 恢复 B`

也就是说：**A 每 2000 个主循环块被 harness "借"出来跑一轮，跑完一轮的 sceFiberSwitch(B) 请求
永远在 pump 返回时被丢掉**（因为它恢复 f1=B 后，主循环 dispatcher 看到 `_t == g_cur_fiber`
就不再切，见 `recomp_harness.cpp:3340-3356`）。

### 1.3 旧运行时同一位置发生了什么（`C:\dev\p5d_build\run.log`）

```
220 [NATIVE] fiberInit obj=0xA014D000 entry=0x81198BE9 arg=0x0 stackbase=0xA014D0A0
223 [NATIVE] fiberInit obj=0xA015E000 entry=0x81198C0B arg=0xA5FFFF28 stackbase=0xA015E0A0
226 [NATIVE] fiberRun ENTER fiber=0xA014D000 NEW argOnRunTo=0x0
237 [NATIVE] fiberSwitch -> NEW fiber=0xA015E000 pc=0x81206BDC from=0xA014D000
    lr=0x8120682A a1=0x0 a2=0x0 | tsp=0xA016E0A0
264 [TRAMP-IN] r0(fiber)=0xA015E000 sp=0xA016E0A0
...  B 的 init 真正跑起来：创建 0x6006~0x6010 线程、LwMutex、事件、信号量 ...
929~4190 [FIBCHK]（CriFs 读取状态机）/ [SEMA] / [EVT] WAKE / CPK 加载 / PREAD 文件读
26643 [ALIVE] epoch=1000 frame=0 | 6009:alive=1 schedcnt=1 | 6008:alive=1 schedcnt=1
26750 [GTIME-STAT] calls=600 wall=11.443s
```

旧运行时的关键差别：**A 只 switch 出去一次就长期挂起，B 的 init 不靠 fiber 轮转，而靠
Vita worker 线程 + 事件/信号量完成异步 IO**（旧日志：`[SEMA] Signal uid=0x24`、
`[EVT] WAKE uid=0x1E thr=0x600A`、`[YIELD] thr=0x6007 us=5000 lr=0x81207613`、
CPK loader 状态 `CPKST state=4/16/17`、`PREAD` 逐段读）。这与本 harness "fiber/线程互斥"的
模型是两套东西。

---

## 2. 空转的确切循环路径与判断条件

### 2.1 涉及的 guest 函数（从 ELF 反汇编还原，均为 Thumb）

#### (a) A 的 lifecycle loop：`0x81198BE8`

```
81198BE8: push {r4,lr}
81198BEA: bl 0x811E5CDA          ; 调用的回调 = [0x81513DEC]（见 (b)）
81198BEE: blx 0x812B2AC0         ; sceKernelCheckCallback（旧运行时/本 harness 均 noop，返 0）
81198BF2: bl 0x812067A0          ; fiber 管理器步进（见 (c)）
81198BF6: ldr r0,[0x816CC630]    ; app flags
81198BFE: ands r0,r0,#2          ; APP_RUNNING 位
81198C04: bne 0x81198BEA         ; 若 running 则继续循环
81198C06: movs r0,#1 ; pop {r4,pc}
```
> `[0x816CC630]=0x00000002` 是**正常的 APP_RUNNING 状态**，不是死循环原因；它本来就该让
> `0x81198BE8` 一直循环（真机同逻辑）。判断它有没有推进要看循环体里的副作用（帧计数、队列）。

#### (b) 每轮回调：`0x811E5CDA`

```
811E5CDA: push {r4,lr}
811E5CDC: movw r0,#0x3dec ; movt r0,#0x8151     ; r0 = 0x81513DEC
811E5CE4: ldr r0,[r0]
811E5CE6: cbz r0, 811E5CEA
811E5CE8: blx r0                                ; -> 0x811E5929（0x81513DEC 表项 = FUN_811e5928）
811E5CEA: pop {r4,pc}
```
`0x81513DD4` 起的 8 项回调表在日志 `[TBL]` 中为：
`811E5D83 811E5E77 811E5E87 811E5EB9 811E5EC3 811E5EF3 811E5929 811E5F25`；
其中 `[0x81513DEC] = 0x811E5929`，即 **FIOS/异步请求队列处理器 `FUN_811e5928`**。

#### (c) 调度器步进：`0x812067A0`（核心！）

```
812067A0: push {r4,r5,lr}; sub sp,#0xc
812067A4: r5 = 0x812B414C           ; 全局 stack canary 地址
812067AC: r0=[r5]; str r0,[sp,#4]   ; 把 canary 存到本帧 [sp+4]
812067B0: add r0,sp,#0 ; blx sceFiberGetSelf(out=sp)   ; *sp = 当前 fiber 对象（或 0）
812067B6: r3=[sp]                   ; r3 = self
812067B8: r2=[r3+0x94]              ; r2 = self->prev
812067BC: r0=[r3+0x80]              ; r0 = self->state
812067C0: r1 = r0 & 1               ; bit0 = "已完成/待摘链"标志
812067C4: r0 = r2                   ; r0 = prev
812067C6: beq 0x8120680E            ; bit0==0 → 不摘链，直接选下一个
        ; ---- bit0==1 时：把 self 从 ready 链表中摘掉并置 finished ----
812067C8: r1=[r3+0x98]              ; r1 = self->next
812067CC: cbz r1, 0x812067DC        ; next==0 → self 是链尾
812067CE: [r1+0x94]=r2              ; next->prev = self->prev
812067D2: r2=[r3+0x94]; r1=[r3+0x98]; b 0x812067E8
812067DC: [0x816CBC08+0xA1C]=r2     ; 全局尾指针 = self->prev
812067E8: cbz r2, 0x812067F4
812067EA: [r2+0x98]=r1              ; prev->next = self->next
812067F4: [0x816CBC08+0xA18]=r1     ; 否则全局头指针 = self->next
81206806: r1 |= 2 ; [r3+0x80]=r1    ; self->state |= 2（finished）
        ; ---- 选择切换目标 ----
8120680E: cbnz r0, 0x8120681C       ; prev != 0 → 目标 = prev
81206810: r0 = [0x816CBC08+0xA1C]   ; 否则目标 = 全局链表尾
8120681C: cmp r3,r0
8120681E: beq 0x8120682A            ; 目标 == self → 不切换，直接返回！
81206820: cbz r0, 0x8120682A        ; 目标 == 0 → 不切换
81206822: r1=0; r2=0; blx sceFiberSwitch(r0, 0, 0)   ; 切到另一个 fiber
        ; ---- 被切回来后的返回路径 ----
8120682A: ldr r1,[sp,#4]            ; 读回 canary
8120682C: r0=[r5]; cmp r0,r1
81206830: bne 0x81206836            ; 不等 → __stack_chk_fail(0x812B2810)（真机直接 abort）
81206832: add sp,#0xc; pop {r4,r5,pc}   ; 正常返回到 A 的 0x81198BF6 或 B 的 0x81198CF6
```
要点：
- bit0 是"该 fiber 已完成、要从 ready 链摘除"的标记，**不是**"active/开始运行"。
  游戏创建 fiber 的包装 `0x81206700` 明确写 `[fiber+0x80]=0`。
- 切换目标 = `self->prev`，否则 = 全局链表尾 `[0x816CBC08+0xA1C]`。
- 目标等于自己时**不切换**（本条即"没有别的 fiber 可跑"时的返回路径）。
- 返回前做 canary 校验；**只有正常"切出去再切回来"的同一帧才会通过**。
  harness 在错误现场恢复时必然触发 `0x81206836 __stack_chk_fail`（harness 用 `[682A-SKIP]`
  把它换成 `pc=lr`，等于跳过整个 scheduler 尾声和返回到调用者的正常路径）。

#### (d) B 的等待循环：`0x811E5EC2`（48 字节）

```
811E5EC6: ldrsh r0,[r4,#2]      ; node->state（r4 = 传入的 job/node）
811E5ECA: cmp r0,#6
811E5ECC: beq 0x811E5EF0        ; state==6 → 返回（完成）
811E5ECE: r0=0x1e; bl 0x8120760C ; sceKernelDelayThread(30)
811E5ED4: r0=[0x816CC630]; ands r0,#2; beq 0x811E5EEA
811E5EE4: bl 0x81207632         ; 帧等待/调度步进（见 (e)）
811E5EE8: b 0x811E5EC6          ; 循环
811E5EEA: bl 0x811E5928         ; 若 app 非 running：自己处理 FIOS 队列
811E5EEE: b 0x811E5EC6
```
B（0x84846860，entry 0x81198C0B）在 app 初始化某一步提交了一个异步 job 后，就停在这个
`state != 6` 的轮询里。日志里 B 的 pump/主循环轨迹每 37 块回到这个循环一次（`[PMP2B]`）。

#### (e) `0x81207632`（72 字节）与 `0x8120760C`（8 字节）

```
81207632: 保存 canary
81207644: GetSelf(&sp); r0=[sp]
81207648: cbz r0, 0x81207664      ; 线程级（self==0）→ 只 DelayThread
8120764C: r4 = [0x816CBC08+0xA20] ; 保存的"帧/主线程 id"
81207658: GetThreadId()
8120765C: cmp r4,r0
8120765E: bne 0x81207664          ; id 不同 → 不跑调度器
81207660: bl 0x812067A0            ; 否则 fiber 管理器步进（可能切走）
81207664: r0=0x1e; blx sceKernelDelayThread
```

```
8120760C: push {r4,lr}; blx 0x812B2A50(DelayThread); pop {r4,pc}
```

### 2.2 空转循环的逐块形态（与日志 PCRING 对齐）

**B 侧（主循环 dispatch，loop=2，fib=0x84846860）：**

| 块 | 做什么 | 判断/结果 |
|---|---|---|
| `811E5EC6` | `r0 = node->state` (ldrsh) | 整个稳态从未观测到 6（日志 31 个 pump pass 里该分支都没走通），`beq` 永不成立 |
| `811E5ECE` | DelayThread(30) | harness `case 0x812B2A50`：`g_in_fex==0` 时**什么都不做**（`:1155-1167` 的 3 个分支都要求 `g_in_fex==1`/`g_in_fex>0`），只把 r0 置 0；不计时间 |
| `811E5ED4` | `[0x816CC630]&2` | =2 → 跳 811E5EE4（正常） |
| `811E5EE4/81207632` | GetSelf、GetThreadId、可能 812067A0 | GetSelf 返回 B（因为 `g_in_fiber_exec==1`，`:828`） |
| `8120765C` | `[0x816CCBC08+0xA20] == GetThreadId()` | GetThreadId 恒 0x100（`:699`）；guest 此前把自己的 GetThreadId 结果写进该全局，故相等 → 会进调度器 |
| `812067B0` | GetSelf(&sp) | = B |
| `812067B8-C6` | 读 `[B+0x80]=3`（bit0=1！） | **走"摘链 + finished"分支** |
| `812067DC` | B->next == 0 → 全局尾 = B->prev | B 是最后剩余项 |
| `812067F4` | 全局头 = B->next | 链表清空 |
| `81206810/1C` | `prev==0` → 目标 = 全局尾 | 尾 == B（或 0） |
| `8120681E` | `目标 == self` | **beq 0x8120682A：不切换** |
| `8120682A` | canary 检查 | 同一帧，通过 |
| `81206832` | pop 返回 | 回到 `811E5ED4` 的下一轮 → 循环 |
| `81207664` | DelayThread(30) | 又一次 no-op |

**结论：B 的 `state==6` 条件与 `sceFiberSwitch` 都永远不会发生，B 只是每 ~40 块写一次
`[B+0x80]|=2` 并"原地返回"，因此主循环无限重复。**

**A 侧（pump，loop=3，fib=0x848367C0）：**

- pump 触发条件：主循环 `(pc==0x811E5CDA || n%2000==0) && !g_pumping2 && fibers>1 && g_pump_cool==0`
  （`:3105`）；
- pump 把 `g_cur_fiber` 置为 f2（此处为 A），从 A 保存的 ctx（`pc=0x8120682A, sp=A0DFFFD8`）开始跑；
- A 依次执行 scheduler 尾声（`8120682A/6832`）→ 返回到 `81198BF6` → 循环体：
  `81198BEA → 811E5CDA → 811E5CE8 → 811E5928`（FIOS 队列，本轮可能只推进一个状态）
  → `81198BEE CheckCallback` → `81198BF2 812067A0`；
- 在 `812067A0` 里 A 的 `[A+0x80]=3`（bit0=1）→ 摘链 + finished → 目标 = `A->prev = B`
  （A 首次被摘链时残留下的 prev，日志 `[FSW] r0=84846860 ... fib=848367C0`）→
  `blx sceFiberSwitch(B)`；
- harness HLE `case 0x812B1BA0`（`:742-809`）：因为 `g_in_fiber_exec==1`，走 defer 分支
  `if (fb != g_cur_fiber) g_fiber_switch_to = fb;`（`:782`），返回 0；
- pump 在 `:3198-3209` 看到 `g_fiber_switch_to` → `s.r[15] = nx(0x8120682A)` → **break**；
- pump 结尾把 f2(=A) 的现场存回 `g_fctx[A]`（`:3217-3220`，`pc=0x8120682A`）；
- 恢复 f1(=B) 的现场（`:3226`）、`g_cur_fiber=B`（`:3230`）；
- 主循环 dispatcher（`:3340-3356`）看到 `g_fiber_switch_to=B`，但 `_t == g_cur_fiber` →
  **不切换、只清标志**。A 的"切到 B"请求被静默丢弃。
- 每 2000 块重来一次。**A 的一轮实际已执行但结果被丢弃，guest 看到 A 停在同一处。**

### 2.3 为什么永不推进——机理链

- **A1（fiber 状态位被写坏）**：`fiber_load()` 置 `[fiber+0x80] bit0=1`（`:1189-1193`，
  日志 `[FACT] fiber=... state|=1 -> 00000001`）。guest `0x812067A0` 把 bit0 当"已完成"，
  第一次经过调度器时就把当前 fiber **摘链 + finished**（`[GSELF] f1 st=00000003`）。
  之后的切换目标只能从 `prev/链表尾` 里选，一旦对方也被摘/标记，目标退化为 self → **不切换**。
- **A2（B 永远切不出去）**：A 被摘链后，链上只剩/没有 B；B 调 `0x812067A0` 时
  `prev=0 → 目标=链表尾(=B 或 0)` → `beq 8120682A`/`cbz` 直接跳过 `sceFiberSwitch`。
  所以 **B 的等待循环没有"让出"能力**；A 只能靠 harness pump 每 2000 块强行借出。
- **A3（线程被饿死）**：B 等待的 node 状态 6 需要异步任务完成。旧运行时由 worker 线程完成
  （6 条 job 线程 + 定时线程 + CPK loader 线程，旧日志 `[SEM]/[EVT]/[ALIVE]`）。
  新 harness 中这 6 条线程早在 boot 早期就创建并阻塞在信号量上（100M 日志 372-476 行：
  `[THR] create tid=40011000..40016000 entry=811E68C9`、`[SEM] wait ... have=0`），
  而 `hle_thr_pump` 被 `g_in_fiber_exec/g_cur_fiber` 门控（`:2255`）**永久无法运行**。
  最直接的证据就是 1.5M 块的嵌套 boot 死在 `0x8127BE36` 的
  `sceKernelSetEventFlag(0x60016000,1)` 自旋（日志 11301-11575 行起大量
  `[EVT] set obj=60016000 bits=1 lr=8127BE56`）；这是一段"kick 线程 + 等完成"的循环。
- **A4（异步等待节点的生产者/消费者错位）**：A 的 `0x811E5928` 在本 harness 里确实把
  GFDPSP2PRESET.GSC 打开并读完了（`[FIOS] OPEN ... size=62100`、`[CONSUME] FHRead calls=1
  bytes=62100`），但 B 的 `node->state==6` 依然观测不到——稳态每个 pump pass 的
  `[PMP2B]` 尾都是 `811E5970 → 811E5AEC`（扫描到队列尾/空），说明队列侧已无活可做；
  而该 6 状态由 guest 的 job/线程侧推进，正好落在 A3 被饿死的那一半。
  → 需要加探针确认（见 §4 P0-0），但"线程侧不运行"是必要条件。
- **A5（DelayThread 不真让出）**：`case 0x812B2A50`（`:1155-1167`）只有在
  `g_in_fex>0 && g_in_fiber_exec` 且 `g_fiber_list.size()>1` 时才设 `g_fiber_switch_to`；
  非嵌套/主循环上下文（本稳态的 B）里它**只返回 0**。旧实现是
  `n_thread_yield`（`eboot_probe.cpp:7441-7462`）：保存 `svc+2`、登记 delay 冷却、
  `sched_pick` 后真正切走。B 的等待循环因此从"阻塞让出"变成"纯忙等"。
- **A6（补丁制造的伪完成/伪现场）**：
  1. `[SCHEDRUN]`（`:2886-2908`）在 `0x812068D8` 处强行把 B 从入口 `0x81198C0B` 重置重跑，
     64 次 DelayThread 后触发 `[YMAIN]` unwind（`:1157` + `:1262-1263`），
     随后**无条件把 B 标成 finished**（`:2902` `[fb+0x80]|=2`）。B 实际没完成。
  2. 主线程 join（`0x8120683C`：检查目标 `[obj+0x80]&2`）看到 bit2 后退出，`main()` 返回
     （`[MAINRET] n=1760`）。
  3. `MAINRET` 路径（`:3379-3406`）直接恢复 `g_fiber_list[0]=A` 的现场 `pc=8120682A`
     ——但 A 的保存现场其实是"主循环/pump 合成的"，其 `sp` 与 `[sp+4]` 不匹配
     （`[682A-SKIP] [sp+4]=81206BEC`，0x81206BEC 是 trampoline 里 `blx entry` 的返回地址，
     说明 A 的帧早被 trampoline 帧覆盖）。`fix682a`（`:422-433`）把错误吞掉：强行
     `pc=lr=0x81198BEE`，**跳过 0x812067A0 的摘链/切换/尾声**，使 A 的"yield"没有真实语义。
  4. 之后 A 的现场被固定写成 `pc=8120682A, sp=A0DFFFD8`，每个 pump pass 都重复"跑一轮、
     到 switch 被打断、恢复 B"，即 100M 的稳态。
- **A7（GetSelf 线程级假对象）**：`case 0x812B1B70` 在 `g_in_fiber_exec==0` 时返回
  惰性分配的 dummy（`:826-840`，日志 `[GETSELF-DUMMY] thread-level -> 8497D820`）。
  旧运行时返回 0（`eboot_probe.cpp:6932-6938`）。这使主线程 join 循环
  `0x8120683C` 走了"有当前 fiber"的错误分支（`[SPOLL] lr=8497D820`），
  会对 dummy 做摘链/switch 计算，污染真实 ready 链表。

### 2.4 回答：是"主线程在等事件"还是"fiber 调度错误"？

**两者叠加，主因是调度器（fiber+线程）错误，表现为主线程在等一个永远等不到的事件。**

- 从 guest 语义看：B 停在 `0x811E5EC2` 等待 job/node `state==6`，是**合法的异步等待**；
  A 的 `0x81198BE8` 循环有 `[0x816CC630]&2` 就继续跑，也是**正常的 app 主循环**。
- 但从执行语义看：能完成该 job 的线程被 harness 结构性禁止运行（A3），
  把控制权交回 A/B 的 fiber 切换又因 bit0/链表/丢弃请求而失效（A1/A2/A6），
  DelayThread 又不让出（A5）。所以**不是"等事件"本身有问题，而是"该事件的完成者永远得不到 CPU"**。

---

## 3. 与旧运行时的对比

### 3.1 架构差异

| 维度 | 旧运行时（`C:\dev\p5dsrc\src`，能到 title/菜单） | 新 harness（当前） |
|---|---|---|
| 执行器 | **单一** `IArmExecutor`，fiber/线程/主线程共享同一份 r[16]+APSR（`thumb_interpreter.cpp:967-978`） | 一个 `RecState s` 在 main loop/fiber_exec/pump/thread pump 之间手工 memcpy，多套保存点 |
| 切换原语 | HLE 内部**同步**完成：保存 `g_cur_fiber` 现场（pc=`svc+2`、r0=0）→ 装载 target → `context_switch`（`eboot_probe.cpp:6965-6995`，`thumb_interpreter.cpp:967-970`） | HLE 内**不切换**，置 `g_fiber_switch_to/g_switch_pending`，由外层 dispatcher/pump 各自消费（`recomp_harness.cpp:706-809`、`:1273-1318`、`:3198-3209`、`:3340-3356`） |
| 嵌套执行 | 不存在（切换不展开 C++ 栈） | `fiber_exec_to_completion()` 递归块循环（`:1235-2035`），HLE Run/Switch 里嵌套调用（`:720-739`、`:803-807`），且带 1.5M 块截断（`:1257-1261`） |
| guest 线程 | 统一 `sched_pick` 轮转（`eboot_probe.cpp:7044-7060, 7142, 7441-7462`），`g_thread`/`g_main`/`g_fiber` 三态（`OLD_RUNTIME_FIBER_HLE.md §2.2`） | 独立 `g_thr` + `hle_thr_pump`（`:2254`），但与 fiber 执行**互斥**（`:2255`），主循环只每 512 块调用一次（`:2509`） |
| DelayThread | `n_thread_yield`：保存 `svc+2`、cooldown、`sched_pick` 切走（`eboot_probe.cpp:7441-7462`） | `case 0x812B2A50`：嵌套内"轮转设 switch"，非嵌套内 **no-op**（`:1155-1167`） |
| GetSelf | 返回 `g_cur_fiber`；宿主线程返回 0，**绝不惰性创建**（`eboot_probe.cpp:6932-6938`） | `g_in_fiber_exec==0` 时返回惰性 dummy（`:826-840`） |
| fiber 栈 | `fobj + malloc_size`（guest 自己分配的 0xA0+stackSize 区块；旧日志 stackbase=A014D0A0，栈顶 A016E0A0） | 另造 region B 8MB 栈 `0xA0E00000 - idx*0x800000`（`:1205-1209`），忽略 guest 对象布局 |
| fiber 状态位 | 从不改 guest 的 `[fobj+0x80]`；bit0/bit1 完全由 guest 的 `0x812067A0`/trampoline 维护 | 首次装载就 `[fiber+0x80]|=1`（`:1189-1193`） |
| 主线程→fiber | `n_fiber_run` 保存线程现场（pc=`svc+2`）、置 `g_in_fiber_world`、`g_cur_fiber=target`（`eboot_probe.cpp:6997-7016`） | 嵌套执行（`fiber_exec`）；纯 deferral 分支（`:717-719`）**不保存线程现场**，dispatcher 消费时 `g_cur_fiber==0` 也没有保存分支（`:3340-3356`） |

### 3.2 旧运行时能推进的关键语义（可直接照抄）

1. **切换 = 保存/装载寄存器 + 强制下一条 PC**，发生在 HLE 内部，不切换 C++ 栈：
   `eboot_probe.cpp:6965-6995`（n_fiber_switch：`pc.g_fiber[prev]=ex->get_reg`；`pc.regs[15]=(pc&~1)+2`；
   `pc.regs[0]=0`；`context_switch(target)`；`g_cur_fiber=target`；最后 `g_fiber_pump()`）；
   `thumb_interpreter.cpp:967-978`（context_switch 置 `forced_npc`，boundary 版不 force）；
   `thumb_interpreter.cpp:11007`（每条指令后采 forced_npc）；
   `thumb_interpreter.cpp:11714-11717`（**切换后不得用 native 返回值覆盖新现场 r0**）。
2. **线程/fiber 同级调度**：`sched_save` 按 `g_cur_thread/g_in_fiber_world/g_cur_fiber` 选择保存对象
   （`eboot_probe.cpp:7044-7060`；`g_cur_thread==0` 且 fiber 世界 → 存 `g_fiber[g_cur_fiber]`；
   线程 → 存 `g_thread[thid]`），`sched_enter` 反方向恢复（`:7051-7060`），`sched_pick`
   轮转所有 runnable 线程并支持 `target==0`（`:7142`，`OLD_RUNTIME_FIBER_HLE.md §2.2`）。
3. **DelayThread 真让出**：`n_thread_yield`（`eboot_probe.cpp:7441-7462`）：保存 `svc+2`、
   设置 `g_delay_until[prev]` 冷却、`sched_pick`、`sched_enter`。
4. **CheckCallback 是 noop**：`eboot_probe.cpp:431` 的 `n_noop`，绑定于 `:6411`。
5. **fiber 首次启动**用 guest 自己的分配区：`fiber_setup`（`eboot_probe.cpp:6940-6963`）：
   `c.regs[13]=fobj+malloc_size`，`c.regs[15]=0x81206BDDu`（复用 guest trampoline），`c.regs[0]=fobj`。
6. **getSelf 线程级 = 0**（`eboot_probe.cpp:6932-6938`）。
7. **旧主循环**：`eboot_probe.cpp:8300-8348`：`mx->run(ROUND=20M)` + `STOP_QUANTUM` 抢占
   + `sched_pick/sched_enter_boundary`；没有 `fiber_exec`。

### 3.3 旧运行日志的直接对照（`run.log`）

- line 220/223：两条 fiber 的 entry 与新 harness 完全一致（`0x81198BE9`、`0x81198C0B`）。
- line 226/237/264：`fiberRun(A)` → A 第一次 `0x812067A0` 就切到 B（`lr=8120682A`）
  ——新 harness 日志同样的 `[FSW] r0=84846860 ... lr=8120682A`，**这一步两边一致**；
  差别在之后：旧运行时 B 的 init 用线程跑完 IO，新 harness B 在 1.5M 块内死在
  `0x8127BE36` 的 kick/等待自旋。
- line 929~4190：CriFs/CPK 状态机与 `PREAD` 文件读持续推进（`[FIBCHK] reqState 2→3→7` 等）；
- line 26643：`[ALIVE] epoch=1000 ... 6008/6009 alive schedcnt=1`——线程确实在被调度；
- line 26750：`[GTIME-STAT] calls=600` 说明帧计时/更新回调在跑（当时 frame 仍是 0，但系统在动）。

---

## 4. 修复方案（按优先级；行号=当前 `recomp_harness.cpp`）

> 建议顺序：**P0-0（加探针确认）→ P0-A（线程调度）→ P0-B（fiber 状态位）→ P0-C（DelayThread）
> → P0-D（单层切换/去掉伪完成）→ P1**。每步都可单独跑 2M 验证，不要一次全上。

### P0-0　先加 3 个探针，把 B 等待的节点看清楚（不改行为，零风险）

目标：确认 B 的 `r4` 指向的 node、`node->state`、以及 1.5M 自旋循环到底在等什么。
在 `fiber_exec_to_completion` / 主循环共用的 HLE 前面加（或直接加在 HLE 的默认分支前）：

```cpp
// 探针1：B 的等待节点
if (((s->r[15] & ~1u) == 0x811E5EC6u) && g_dbg_node_n++ < 40) {
  uint32_t nd = s->r[4], st = 0, fl = 0;
  uint8_t* q = GP(nd);
  if (q) { std::memcpy(&fl, q, 2); uint16_t h; std::memcpy(&h, q + 2, 2); st = h; }
  printf("[NODE] nd=%08X state=%u flags=%04X lr=%08X fib=%08X\n", nd, st, fl, s->r[14], g_cur_fiber);
}
// 探针2：0x8127BE36 自旋循环（1.5M 块的真正消耗者）
if (((s->r[15] & ~1u) == 0x8127BE36u) && g_dbg_be36_n++ < 40) {
  printf("[BE36] r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X lr=%08X sp=%08X fib=%08X\n",
         s->r[0], s->r[1], s->r[2], s->r[3], s->r[4], s->r[14], s->r[13], g_cur_fiber);
}
// 探针3：SetEventFlag 目标/是否有等待者
// 放在 case 0x812B2AE0 里：打印 obj/bits/是否有 g_thr 在 wait_obj==obj
```
验证：`P5D_NESTED_UNTIL=100000000`（等于不截断）跑 2M：
- 若 `[WD] loop=0` 的 total 一直涨、`[BE36]` 大量出现、`[EVT] set 60016000` 持续 →
  证实"线程饿死导致 boot 卡在 kick/等待自旋"（P0-A 是主根因）；
- 若 `[NODE]` 的 nd/state 显示 state 已是 6 而 B 仍不退出，则另查寄存器/分支条件；
- 若 state 从 2→3→4→5→7→8 跳过 6，则说明等待条件与生产者约定不匹配（要单独修 0x811E5928
  状态机接线，见 P2）。

### P0-A（最高优先）　在 fiber 世界内允许 guest worker 线程运行

**现状**：`hle_thr_pump()` 开头 `if (g_cur_thr || g_in_fiber_exec || g_cur_fiber) return;`
（`:2255`），只有主线程阶段（`g_cur_fiber==0 且不在嵌套 fiber`）才会跑线程；
fiber 一运行，6 条 job 线程 + loader 线程全部饿死。旧运行时是统一调度器（§3.2-2/3）。

**最小 patch（先行版，仅放开"借时间片跑线程"，不改 fiber 切换逻辑）：**

```cpp
// 1) 新增：是否有可运行的 guest 线程
static bool thr_has_ready() {
  for (auto& kv : g_thr) if (kv.second.alive && kv.second.ready) return true;
  return false;
}

// 2) 改 hle_thr_pump 门控（原 :2255）：
static void hle_thr_pump(RecState* s, int quantum) {
  if (g_cur_thr) return;                                  // 只禁止重入
  // P0-A: 旧运行时线程/fiber 同级。允许在 fiber 现场借时间片；
  //        本函数内部本就 save/restore 整个 RecState，返回时 s 恢复原样。
}

// 3) 在主循环 pump/loop=2 的块边界与 fiber_exec 的安全点调用：
//    a. 主循环已有 :2509 的调用，去掉门控后即可生效；
//    b. fiber_exec_to_completion 的 while 循环里（:1256，每轮块调度前）加：
if (g_thr_allowed_in_fiber && thr_has_ready() && (g_blkcnt_total & 0x3FFu) == 0u)
  hle_thr_pump(s, 400);
//    c. DelayThread HLE 末尾（see P0-C）也可再补一次。

// 4) 环境开关（默认先 0，A/B 对照）：
static int g_thr_allowed_in_fiber = 0;  // 由 P5D_THR_IN_FIBER=1 打开
```
同时把 HLE `sceKernelGetThreadId`（`:699`，见 P2）改为 `return g_cur_thr ? g_cur_thr : 0x4001u;`，
否则 thread 内部所有 GetThreadId 都是 0x100，会让 0x81207632 / LwMutex owner 判定失真。

**注意**：打开后要保证线程里不会再走 `g_yield_to_main`（那是 fiber_exec 的退出请求），
所以在 P0-A 同时把 DelayThread/CheckCallback 里的 `g_yield_to_main` 分支限制为
`g_cur_thr==0`（本报告 P0-C 的 patch 已包含）。

**预期效果**：1.5M 块处的 `0x8127BE36` kick/等待自旋能收到 worker 的完成；B 的
`node->state==6` 能变成真；B 继续 init；`[FRAMEDUMP]` 的 `t4/t8/ctr` 出现非 0；
`[GXM-BEGIN]/[GXM-PRESENT]` 开始出现；`err` 里 `[COUNT] present/beg in/draw` 非 0。

**2M 验证**（`P5D_THR_IN_FIBER=1 P5D_NESTED_UNTIL=100000000`）：
1. `[WD] loop=0` 的 `total` 突破 1.5M（否则 boot 仍在自旋）；
2. 出现 `[SEM] signal` / `[EVT]` 唤醒后的线程继续执行（新增探针或看 `g_loop_cnt[1]` 增长）；
3. `[FRAMEDUMP]` 最后 3 条至少一条 `t4!=0`（帧计时被写）。
**30M 验证**：`[CNT] 81198BEA` 或 `811E5CDA` 明显 > 749；`[GXM-CTX]`/`[GXM-BEGIN]` 出现；
`[FRAMEDUMP]` 连续行 `ctr` 递增；`err` 的 `[COUNT]` 非 0。

### P0-B　不要写 guest 的 `[fiber+0x80]` bit0（fiber 状态位）

**现状**：`fiber_load()` 首次装载时 `if (!(v&1)) { v|=1; ... }`（`:1189-1193`，日志 `[FACT]`）。
**机理**：guest `0x812067A0` 用 bit0 决定"该 fiber 已完成、从 ready 链表摘除并置 finished"；
置 1 后当前 fiber 被摘链（`[GSELF] f1 st=00000003`），另一个 fiber 的目标退化为自己
→ 永不 `sceFiberSwitch`（§2.3 A1/A2）。

**patch（删除该块，保留旧行为开关）：**

```cpp
static void fiber_load(RecState* s, uint32_t fiber, uint32_t entry, uint32_t arg) {
  // P0-B: 真实运行时 [fiber+0x80] 由 guest/trampoline 维护，HLE 不得代写。
  //       原 [FACT] 置 bit0 会让 0x812067A0 把当前 fiber 摘链并标 finished，
  //       导致另一个 fiber 的目标退化为 self、永不切换（见本报告 §2）。
  static int g_active_hack = -1;
  if (g_active_hack < 0) { const char* e = getenv("P5D_FIBER_ACTIVE_HACK");
    g_active_hack = (e && atoi(e) != 0) ? 1 : 0; }
  if (g_active_hack) { /* 旧行为（仅供回退/对照） */
    uint8_t* q = GP(fiber + 0x80u);
    if (q) { uint32_t v; std::memcpy(&v,q,4); if (!(v&1)) { v|=1; std::memcpy(q,&v,4); } }
  }
  ...
```

**预期效果**：A 第一次 `0x812067A0` 不再摘链，`prev=B` → 切 B；B 的
`0x812067A0` 看到 `prev==0 → 目标=链表尾=A` → 真正 `sceFiberSwitch(A)`；
guest 的 A↔B 乒乓恢复，A 的 FIOS 队列回调能被 B 的等待循环驱动（不再只靠 pump 每 2000 块）。

**2M 验证**：
- `[GSELF]`：`f1=.../st` 不再是 `00000003`（应为 0 或完成后的 2）；
- `[FSW]` 出现**反方向**：`... fib=84846860`（B 作为当前 fiber 请求切 A）；
- `[PUMPSW]` 数量下降/`switch_to` 出现 848367C0；
- `[CNT] 811E5CDA`/`81198BEA` > 749（若 P0-A 也生效则更高）。

### P0-C　DelayThread 变成真让出（并移除 YMAIN 64 次 hack）

**现状**：`case 0x812B2A50u`（`:1155-1167`）在 `g_in_fex==0`（主循环/pump 里的 fiber 上下文）
完全 no-op；`g_in_fex==1` 时又用 `&0x3F` 每 64 次设 `g_yield_to_main`（这是 `[SCHEDRUN]`
被打断、boot 被强行拽回的直接原因）。
旧实现 `eboot_probe.cpp:7441-7462`：保存 `svc+2`、冷却、`sched_pick` 切走。

**patch（示意）：**

```cpp
case 0x812B2A50u: {
  r0 = 0;
  if (g_cur_thr) break;                      // 线程自己的 DelayThread 交给 thread 调度
  if (g_delay_n++ < 8) printf("[DELAY] usec=%u lr=%08X fib=%08X\n", s->r[0], s->r[14], g_cur_fiber);
  // 1) 借时间片给 guest worker 线程（P0-A 的核心：DelayThread 在旧运行时=sched_yield）
  if (g_thr_allowed_in_fiber && thr_has_ready()) hle_thr_pump(s, 200);
  // 2) fiber 之间的切换仍然交给 guest 自己的 0x812067A0（不要在这里自造 round-robin，
  //    否则会重演当前 harness 的"harness 替 guest 调度"错误）
  // 3) 删除 g_yield_to_main / &0x3F 这段（只保留给旧路径的开关 P5D_LEGACY_YMAIN）
  break; }
```
并在 `fiber_exec_to_completion` 里保留一个兼容开关：`if (g_legacy_ymain && g_yield_to_main && g_in_fex==1) ...`
（默认 0），避免旧 boot 流程回退时依赖它。

**预期效果**：B 的等待循环不再"原地空转 37 块"，而是真正切给 A/线程；
`[PMP2] pass` 的块数从 37 降到个位数并伴随有效进度；`g_blkcnt_total` 继续增长而不是冻结。

**2M 验证**：`[WD]` 中 `loop=0` 继续增长（说明 fiber_exec 在跑）；`[YDELAY]` 出现
`from=84846860 to=...`；`[FRAMEDUMP]` 的 `pc` 不再长期停在 `812067DC/81207648`。

### P0-D　统一切换原语 + 去掉"伪完成/错误现场恢复"

**现状**：
- Run/Switch/ReturnToThread 的切换语义分散在 HLE、`fiber_exec`、pump、main dispatcher 四处
  （`:706-809`、`:1273-1318`、`:3198-3209`、`:3340-3356`），互相丢请求（§2.2 A 侧第 6 步）。
- `[SCHEDRUN]`（`:2886-2908`）强制重置 B + 伪置 finished；`[MAINRET]`（`:3379-3406`）用
  不匹配的现场恢复 A；`fix682a`（`:422-433`）把错误吞掉。
- 纯 deferral（`nested_allowed()==false`，`:307`）时，`sceFiberRun` 只置
  `g_switch_pending`（`:717-719`），但 dispatcher 消费切换时没有"`g_cur_fiber==0` 保存主线程现场"
  的分支（`:3342-3343`），返回线程时会丢现场（旧实现见 `eboot_probe.cpp:6997-7016`）。

**patch 要点（按顺序做）：**

1. **dispatcher 切换点补主线程现场保存**（`:3340-3356`）：
```cpp
if (g_switch_pending || g_fiber_switch_to) {
  uint32_t _t = g_switch_pending ? g_switch_target : g_fiber_switch_to;
  g_switch_pending = 0; g_fiber_switch_to = 0;
  if (_t && _t != g_cur_fiber) {
    if (g_cur_fiber) {                       // 已有
      FCtx& _c = g_fctx[g_cur_fiber];
      std::memcpy(_c.r, s.r, sizeof(s.r)); _c.apsr = s.apsr; _c.r[15] = nxt;
    } else {                                 // P0-D: 宿主线程 → 保存 g_thread_regs
      std::memcpy(g_thread_regs, s.r, sizeof(s.r)); g_thread_apsr = s.apsr;
      g_thread_regs[15] = nxt;               // Run 的返回点 = svc 下一条
      g_in_fiber_world = 1;                  // 新状态（见 P1-A）
    }
    auto _it = g_fctx.find(_t);
    ...
  }
}
```
2. **ReturnToThread 在 dispatcher 侧落地**：`fiber_switch` 收到 `g_fiber_flag_return`（`:810`）
   时，把"回线程"也变成 pending（`g_return_to_thread=1`），dispatcher 在块边界恢复
   `g_thread_regs`、`g_cur_fiber=0`、`g_in_fiber_world=0`。
3. **删除/禁用 `[SCHEDRUN]`**：至少不置 `[fb+0x80]|=2`、不 force-reset ctx
   （删 `:2894-2899`，`:2902`）；改成只打印一次诊断（`[SCHEDRUN-BLOCKED]`）。
4. **`[MAINRET]`/`[RESTART]` 保持"最后兜底"，但在正常路径不应触发**：加断言/日志
   `if (g_cur_fiber && s.r[15]==0) printf("[MAINRET-UNEXPECTED] ...")`，修好后应消失。
5. **`fix682a` 从"静默改 pc"降级为断言**：改成 `printf("[FATAL-682A] ..."); std::abort();`
   （或只在 `P5D_ALLOW_682A=1` 时执行旧逻辑）。它掩盖了"fiber 现场混装"这一必须暴露的 bug。
6. 之后把 `P5D_NESTED_UNTIL` 默认置 0（纯 deferral），主线程调 `sceFiberRun` 走
   P0-D 的 dispatcher 路径，彻底删除 `fiber_exec` 嵌套执行（`:720-739`、`:803-807`、
   `:846-872` 的 GetSelf pump、`:3105-3234` 的 pump）。这一步做完，`[NESTED-STOP]`、
   `[SCHEDRUN]`、`[MAINRET]`、`[PMP2]`、`[682A-SKIP]` 都应从日志里消失。

**预期效果**：boot 全程在纯块级调度下推进；没有任何"混合上下文"；guest 的
`0x812067A0` 乒乓与 `0x811E5EC2` 等待都由 guest 自己驱动；帧循环
（`0x81198C8E..0x81198D08`，含 `0x81198CE4` 帧计数 `[0x816CC630+0xC]++`）能被进入。

**2M 验证**：`[NESTED-STOP]/[SCHEDRUN]/[MAINRET]/[PMP2]/[682A-SKIP]` 全部为 0 条；
`[WD] loop=0` 持续增长；`[FRAMEDUMP]` 出现 `ctr>0` 或 `t4!=0`。

### P1-A　`sceFiberGetSelf` 在线程级返回 0（对齐旧运行时）

**现状**：`:822-840`，`g_in_fiber_exec==0` 时返回惰性 dummy（`[GETSELF-DUMMY]`）。
**旧语义**：`eboot_probe.cpp:6932-6938` 返回 `g_cur_fiber`（宿主线程为 0，绝不建假对象）。

```cpp
case 0x812B1B70u: {
  uint32_t f = g_cur_fiber;     // fiber 现场 = 真实对象；主线程 = 0
  if (!f && g_in_fiber_world && g_cur_fiber) f = g_cur_fiber;  // 线程0=当前fiber（旧 §2.2）
  if (s->r[0]) { uint8_t* p = GP(s->r[0]); if (p) std::memcpy(p, &f, 4); }
  r0 = 0; break; }
```
并删除 dummy 分配与 `:846-872` 的嵌套 pump。
**注意**：`0x812067A0` 的调用者从线程级进入时 GetSelf 会是 0（guest 会 crash）；旧运行时该函数
只会被 fiber 代码调用。若日志出现线程级调用，加保护日志而不是返回假 fiber。
**验证**：`[GETSELF-DUMMY]` 消失；`[SPOLL] lr=8497D820` 消失；主线程 join 进入
`0x8120683C` 的 `self==0` 分支（`:812068E4` 只 DelayThread，不再对 fiber 链表动手脚）。

### P1-B　fiber 栈沿用 guest 自己的分配区（对齐 `fiber_setup`）

**现状**：`:1205-1209` 前两行判断 `f88/arg` 在 `0x82E00000..0x83000000` 才用，否则**无视**
后面的 `f13 = 0xA0E00000 - idx*0x800000` 覆盖一切——另造 8MB 栈。
**旧语义**：`eboot_probe.cpp:6940-6963`：`c.regs[13] = fobj + malloc_size`（旧日志
`fiberInit obj=0xA014D000 ... stackbase=0xA014D0A0`，栈顶 `0xA016E0A0` = obj+0x100A0）。
**patch 思路**：在 HLE malloc（`case 0x812B22A0`，`:650-655`）记录 `g_malloc_size[ret]=n`；
`fiber_load` 里改用

```cpp
uint32_t f8c = 0;                       // guest 包装写的 [fiber+0x8c] = fiber+0xA0（栈底）
if (off + 0x90 <= IMG_SIZE) std::memcpy(&f8c, gp_img+off+0x8c, 4);
uint32_t alloc = g_malloc_size.count(fiber) ? g_malloc_size[fiber] : 0x100A0u;
if (f8c && f8c >= 0x84600000u && f8c < 0x8C600000u) f13 = fiber + alloc;   // 栈顶
else f13 = 0xA0E00000u - idx0 * 0x800000u;                                  // 兜底
if (f13 < 0x84600000u || f13 >= 0x8C600000u) f13 = 0xA0E00000u - idx0*0x800000u;
```
**为什么重要**：`0x811E5928` 等函数会在栈顶附近做 canary/局部数组；真实运行时 SP 在
guest 分配的 64KB 区内。当前 8MB region-B 栈 + pump/主循环混用，是 `[682A-SKIP]`
（`[sp+4]=81206BEC`，trampoline 返回地址串进 scheduler 帧）的土壤之一。
**验证**：`[FL2] sp=` 应为 `fiber+0x100A0` 附近而不再是 `0xA0E00000`；`[682A-SKIP]` 消失；
2M 内不再出现 `[FLOAD-BAD]`/`[FSAVE-BAD]`。

### P2　清理项（不阻塞主目标，但应补）

1. **LwMutex Create 语义**（`:685`）：旧 `eboot_probe.cpp:6922-6929` 是"工作区 0x20 字节清零 + 返回 0"，
   当前 `r0 = hle_malloc(64)`。游戏包装 `0x81207308` 会丢弃返回值、自己 malloc work，
   所以**返回指针本身不致命**，但 work 区必须清零（guest 的 lock 自旋会读它）：
```cpp
case 0x812B2910u: { if (s->r[0]) { uint8_t* p = GP(s->r[0]); if (p) std::memset(p, 0, 0x20); } r0 = 0; break; }
```
   Lock/Unlock（`:686-687`）保持 `r0=0`; 并可在 HLE 里维护 `g_lwm_hold[g_cur_thr]` 做
   "持锁期间不切换"（旧 `eboot_probe.cpp:7127-7132/8428-8431`，见 `OLD_RUNTIME_FIBER_HLE.md §3.1`）。
2. **GetThreadId**（`:699`）：`return g_cur_thr ? g_cur_thr : 0x4001u;`
   （旧 `eboot_probe.cpp:6385-6403`，文档引用）。否则所有线程/fiber 都是 0x100，
   会破坏 `0x81207632` 的 owner 门控与 LwMutex owner 判定。
3. **CheckCallback 保持 noop**（`:1134-1146` 默认 `g_cb_yield=0` 已正确，**不要**设
   `P5D_CB_YIELD=1`；旧运行时即 noop，见 `eboot_probe.cpp:431/6411`）。
4. **`[CANARY-CLOBBER] A0DFFFDC` 是良性噪声**：那是 A 的栈顶在 `0x811E5928` 的
   `push.w {..sl..}` 中保存 r10 覆盖了刚 pop 掉的 scheduler 帧 canary 槽
   （`0x811E5928` prologue 9 个 push 寄存器中 sl 恰落在 A0DFFFDC），不是新增破坏；
   修好调度后此打印自然消失或可关闭，不要按"栈被破坏"处理。
5. **`err` 里的 `[COUNT] present=0 begin_pass=0 draw=0`** 是最终判据：修复后应出现
   非 0 的 present/begin/draw。

---

## 5. 验证流程（2M / 30M / 100M）

### 5.1 命令（PowerShell，避免 stderr 被当二进制）

```powershell
cd C:\dev\tmp\recomp_split
# 基线（可选）
.\recomp_harness.exe 0x81198464 2000000 > out2m_base.txt 2> err2m_base.txt

# 单补丁 A/B：
$env:P5D_NESTED_UNTIL = ''        # 或 100000000（不截断）
$env:P5D_THR_IN_FIBER = '1'
.\recomp_harness.exe 0x81198464 2000000 > out2m_fix.txt 2> err2m_fix.txt
$env:P5D_NESTED_UNTIL = '1500000'

# 30M
.\recomp_harness.exe 0x81198464 30000000 > out30m_fix.txt 2> err30m_fix.txt
```

### 5.2 判据表

| 检查点 | 基线（当前 100M） | 2M 期望 | 30M 期望 |
|---|---|---|---|
| `[CNT] 81198BEA / 811E5CDA / 811E5928` | 749 / 749 / 749 | > 749 且继续涨 | 远大于 749，单调增 |
| `[CNT2] mgr` | 781 | > 781 | 单调增 |
| `[FRAMEDUMP]` | `flags=2 t4=0 t8=0 ctr=0` | 至少一条 `t4!=0` 或 `ctr>0` | `ctr` 递增（帧循环 `81198CE4` 在跑） |
| `[GXM-BEGIN]/[GXM-ENDSCENE]/[GXM-PRESENT]` | 0 条 | 可能 0 | 应 > 0 |
| `err` `[COUNT]` | `present=0 begin_pass=0 draw=0` | 0 可接受 | `present>0` |
| `[NESTED-STOP]` | 1 条 | 0 条（P0-D 后） | 0 条 |
| `[SCHEDRUN] / [MAINRET] / [682A-SKIP]` | 2 / 1 / 1 | 0 / 0 / 0 | 0 / 0 / 0 |
| `[PMP2B] pass blocks` | 恒 37 | 变小/消失 | — |
| `[WD] loop=0 total` | 冻结 1,891,302 | 持续增长 | 持续增长 |
| `[FSW]` 方向 | 只有 `fib=848367C0` | 出现 `fib=84846860` | 双向 |
| 具体探针 `[NODE] state` | — | state 会到 6 并继续 | — |

### 5.3 快速定位（2M 就够的分支判断）

- **2M 仍冻结在 1.5M 的 0x8127BE36** → 线程仍未跑（P0-A 没生效/门控没放开）。
- **2M 过了 1.5M 但 `ctr` 仍 0** → 线程在跑（`[SEM]/[EVT]` 有变化）但 fiber 乒乓/等待仍断
  → 检查 P0-B 的 `[GSELF]` 状态与 `[FSW]` 方向。
- **2M 出现 `[682A-SKIP]` 或 `pc=8120682A sp != A0DFFFD8` 的现场** → 切换原语仍有混装
  （P0-D 未完成）。
- **2M 有 frame 迹象而 30M 又回到 loop2/3 空转** → 检查 worker 线程是否又全体 block
  （`[SEM] wait ... have=0`），即 P0-A 的线程泵调用点/频率不足。

---

## 6. 风险与落地顺序建议

1. **P0-0 先做**（只加打印），用 1 次 2M run 就能把"1.5M 到底在等什么"钉死，避免后面的 patch 盲改。
2. **P0-A 单独上、单独验证**：这是唯一能让 boot 越过 1.5M 自旋的改动；先不放 P0-B/D。
3. **P0-B 单独上**：看 `[GSELF]/[FSW]` 方向是否恢复；此时可以保留 nested boot 观察 ping-pong。
4. **P0-C + P0-D 一起上**：去掉伪完成/错误恢复，改为纯 deferral；这一步改动最大，
   建议先 `P5D_NESTED_UNTIL=0`（默认值就是 0，不设环境变量即可）在 2M 验证切换路径。
5. **P1/P2 最后**：P1-A（GetSelf=0）与 P1-B（栈）会改善稳定性；P2 是正确性收尾。
6. 每一步都保留 `P5D_FIBER_ACTIVE_HACK`、`P5D_LEGACY_YMAIN`、`P5D_THR_IN_FIBER` 等回退开关，
   便于 30M/100M 对照。

---

## 附录 A　关键地址表（guest VA）

| 地址 | 函数/块 | 作用 |
|---|---|---|
| `0x81198BE8` | FUN_81198be8（A 的 lifecycle loop） | 每帧：回调 → CheckCallback → `0x812067A0` → 测 `[0x816CC630]&2` |
| `0x81198C0A` | FUN_81198c0a（B 的 init + 帧环） | init → `0x81198C8E..0x81198D08` 帧环（`0x81198CE4` 帧计数++） |
| `0x811E5CDA` | FUN_811e5cda | 调 `[0x81513DEC]`（=FIOS 队列处理器） |
| `0x811E5928` | FUN_811e5928 | 异步请求状态机（OPEN/READ/CLOSE/Op 系列） |
| `0x811E5D82` | FUN_811e5d82 | 创建异步请求节点并入队 |
| `0x811E5EC2` | FUN_811e5ec2 | 等待节点 `+2 == 6`；DelayThread/调度循环 |
| `0x812067A0` | FUN_812067a0 | fiber 管理器：GetSelf → 摘链/finished → 选 prev/尾 → Switch → canary |
| `0x8120683C` | FUN_8120683c | join/等待目标 fiber `[obj+0x80]&2` |
| `0x81206BDC` | FUN_81206bdc | fiber trampoline：`blx [fobj+0x84]`，返回后摘链+ReturnToThread |
| `0x81206700` | FUN_81206700 | fiber 创建包装：malloc(0xA0+stack)、写 +0x84/+0x88/+0x8c、Initialize |
| `0x8120760C` | FUN_8120760c | DelayThread(30) 包装 |
| `0x81207614` | FUN_81207614 | 比较 `[0x816CBC08+0xA20]` 与 GetThreadId |
| `0x81207632` | FUN_81207632 | GetSelf → (tid 匹配) `0x812067A0` → DelayThread(30) |
| `0x8127BE36/0x8127BE58/0x8127C1D2` | FUN_8127be36 / be58 / c1d2 | boot 末期 CPK/异步 kick-等待循环（1.5M 块消耗处） |
| `0x8128F6B8` | CriFs 读取状态机 | 旧运行时 `[FIBCHK]` 处，不是 fiber 切换点 |
| `HLE 0x812B1B70/1B60/1B80/1BA0/1BB0` | GetSelf/ReturnToThread/Run/Switch/Initialize | 见正文 |
| `HLE 0x812B2A50/2AC0/2910/2830/28A0/27B0` | DelayThread/CheckCallback/Create-LwMutex/Lock/Unlock/GetThreadId | 见正文 |
| 全局 `0x816CBC08+0xA18/+0xA1C` | fiber ready 链表头/尾 | guest 管理 |
| `0x816CBC08+0xA20`（=0x816CC628） | 帧/主线程 id | `0x81207632` 门控 |
| `0x816CC630` | app flags；`+4/+8` 时间；`+0xC` 帧计数 | `[FRAMEDUMP]` 就是这 16 字节 |
| `0x812B414C` | 全局 stack canary 值地址 | `[682A-SKIP]` 校验对象 |
| `0x81513DEC` | 每帧回调表项（=0x811E5929） | A 的轮询回调 |

## 附录 B　当前 harness 行号索引（2026-10-28 14:04 版）

| 位置 | 内容 |
|---|---|
| `:307` | `nested_allowed()`：`g_fiber_nested || (g_nested_until && total<cut)` |
| `:422-433` | `fix682a()`：canary 失配时 `*ppc = lr`（掩盖错误） |
| `:685-687` | LwMutex Create/Lock/Unlock 桩 |
| `:699` | GetThreadId 恒 0x100 |
| `:706-741` | HLE `sceFiberRun`（嵌套执行 + 纯 deferral 分支） |
| `:742-809` | HLE `sceFiberSwitch`（同上） |
| `:810` | HLE `sceFiberReturnToThread` |
| `:811-821` | `_sceFiberInitializeImpl`（维护 `g_fiber_list`） |
| `:822-886` | HLE `sceFiberGetSelf` + dummy + 嵌套 pump（`:846-872`） |
| `:1134-1146` | HLE CheckCallback（默认 noop） |
| `:1155-1167` | HLE DelayThread（非嵌套时 no-op） |
| `:1183-1188` / `:1189-1228` | fiber_save / fiber_load（`:1190-1193` bit0 hack） |
| `:1235-2035` | `fiber_exec_to_completion`（`:1257-1261` 1.5M 截断；`:1262-1263` YMAIN unwind；`:1273-1318` 切换；`:2033` fiber_save） |
| `:2254-2255` | `hle_thr_pump` 互斥门控 |
| `:2508-2509` | 主循环每 512 块的线程泵调用 |
| `:2886-2908` | `[SCHEDRUN]` 强制重跑 + 伪 finished |
| `:3105-3234` | 主循环内的 fiber2 pump（`:3198-3209` 遇 switch break；`:3217-3220` 存 f2；`:3226-3230` 恢复 f1） |
| `:3340-3356` | dispatcher 消费 switch（缺 `g_cur_fiber==0` 的线程现场保存） |
| `:3379-3415` | `nxt==0` 处理 / `[MAINRET]` / RESTART |
| `:2425-2436` / `:2450-2457` | 两个 watchdog（`[WD] loop=` / `[WDRING]+[WDCTX]`） |
| `:2458-2463` | `P5D_CB_YIELD / P5D_FIBER_NESTED / P5D_NESTED_UNTIL / P5D_BOOT_THUNK / P5D_HACKS` 环境变量 |

## 附录 C　旧运行时行号索引（供照抄）

| 位置 | 内容 |
|---|---|
| `eboot_probe.cpp:431` | `n_noop`（CheckCallback 等） |
| `eboot_probe.cpp:6411` | CheckCallback 绑定到 noop |
| `eboot_probe.cpp:6932-6938` | `n_fiber_self`：返回 `g_cur_fiber`（宿主线程 0） |
| `eboot_probe.cpp:6940-6963` | `fiber_setup`：栈=`fobj+malloc_size`、pc=`0x81206BDD`、r0=fobj、argOnRunTo 回写 |
| `eboot_probe.cpp:6965-6995` | `n_fiber_switch`：保存 prev（pc=svc+2、r0=0）→ setup → context_switch → `g_cur_fiber=target` → `g_fiber_pump()` |
| `eboot_probe.cpp:6997-7016` | `n_fiber_run`：保存线程现场、进入 fiber 世界 |
| `eboot_probe.cpp:7018-7035` | `n_fiber_return`：回填 argOnReturn、恢复线程现场 |
| `eboot_probe.cpp:7044-7060` | `sched_save` / `sched_enter`（主线程/fiber/worker 三态） |
| `eboot_probe.cpp:7142` | `sched_pick`（统一轮转） |
| `eboot_probe.cpp:7294-7299` | `g_fiber_pump`（切完后 pump worker） |
| `eboot_probe.cpp:7441-7462` | `n_thread_yield`（DelayThread 真让出 + 冷却） |
| `eboot_probe.cpp:8300-8348` | 旧主循环（20M 指令/轮 + 量子抢占） |
| `thumb_interpreter.cpp:967-978` | `context_switch` / `context_switch_boundary` |
| `thumb_interpreter.cpp:11007` | 每条指令后应用 `forced_npc` |
| `thumb_interpreter.cpp:11714-11717` | SVC 返回后不得用 native 返回值覆盖切换后的 r0 |

---

## 附录 D　本次分析未做的事 / 边界

- 未修改任何源码/批处理/harness；除本文件外无写入。
- 未运行 harness（避免与 Lead 正在进行的 100M run 冲突/超出只读约束）；所有结论来自
  日志、ELF 反汇编与旧源码/旧日志。P0 各 patch 的建议行号已按当前源码核对。
- `0x811E5EC2` 等待的 node `state==6` 的生产者到底在哪个线程/回调，建议按 P0-0 的探针
  在 2M run 中确认后，再决定是否需要单独修 `0x811E5928` 的状态机接线。

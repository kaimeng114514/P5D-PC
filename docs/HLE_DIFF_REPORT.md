# HLE 差异审计报告：旧运行时（可跑到 title/menu） vs 当前静态 AOT harness

> 任务：`task-5`（HLE 差异审计，只读分析 + 探针工具）
> 审计对象：`C:\dev\tmp\recomp_split\recomp_harness.cpp`（当前静态 AOT harness 的 HLE 层）
> 对照对象：`C:\dev\p5dsrc\src\loader\eboot_probe.cpp`（旧运行时 HLE，曾实际跑到 P5D title/menu）
> 配套工具：`C:\dev\p5dsrc\tools\hle_probe.py`（本任务产出，可直接跑当前 run 日志）
> 约束：本报告只读分析，未修改 harness / 生成器 / 其它 docs。
>
> **行号与快照约定（重要）**
> * harness 行号基于 `recomp_harness.cpp` **2026-10-28 14:14 版本（约 257,376 字节 / 3,474 行；写作截止时最新为 14:14:56 / 257,657 字节，见 §0.4）**。该文件正被并行编辑修复，**报告写作期间已发生 4+ 次改动**（14:04 → 14:13 → 14:14 → 14:14:56），因此行号可能继续漂移；14:14:56 起 GetThreadId case 由 1 行扩为多行，其后行号约 +3~5；**桩地址（`case 0x812Bxxxxu`）是稳定锚点**，行号对不上时请以 `case` 地址搜索。§0.4 给出了写作时的“修复进展快照”，请以它为准判断某条差异是否已被修复。
> * 旧运行时行号基于当前工作区 `src/loader/eboot_probe.cpp`（8,591 行，未被并行修改）。
> * 旧运行时调用次数取自 `_nidcall_h1164.txt`（该次运行 195 个导入、合计 4,721,644 次调用）。
> * 当前 run 证据取自 `out100m_682a2.txt`（13:58，116 MB，UTF-16LE，1,378,734 行）；14:13-14:14 的并行实验证据取自 `out30m_thry.txt`、`out30m_p0b.txt`、`err30m_p0b.txt`。
> * 换算规则（正文表格若仍是写作早期的 14:04 基线行号）：hle_call 区（旧 `:603-:1172`）→ **+2**；`fiber_load` 区（旧 `:1188-:1233`）→ **+4**；`hle_thr_pump` 及其后（旧 `:2250+`）→ **+3**。§9.1 与附录 A 已直接按 14:14 版本列出行号。

---

## 0. 结论摘要（先看这里）

### 0.1 当前 harness 的客观症状（100M blocks run）

用 `hle_probe.py` 对该日志统计得到（命令见 §10）：

| 证据 | 数值 | 解释 |
|---|---|---|
| `[HLECOUNT]` | 15,607,211 次 HLE 调用 / 86 个唯一桩 | 运行结束正常 |
| HLE 最热桩 | `0x812B1B70` sceFiberGetSelf ×6,151,916；`0x812B2A50` DelayThread ×6,051,302；`0x812B27B0` GetThreadId ×3,050,584 | 三者占 98%+，全部在 fiber/线程等待环里 |
| **未实现桩** | **`0x812B2A80` DelayThreadCB ×72,518**（harness 无 case → 默认 `r0=0`，不切换、不延时） | 与 `0x812B2AE0` SetEventFlag ×72,518 **1:1 配对**，调用点分别为 `lr=8127C0F8` / `lr=8127BE56` —— 正是 CriFs 调度等待环 |
| `[EVT]` | `set obj=60016000 bits=1 thr=0 lr=8127BE56` 重复 72,518 次 | 事件早已置位，但等待方仍不前进 |
| `[THR]` | 10 个 worker 线程 create+start；`[SEM] wait ... have=0` 至少 6 条（打印上限），全部阻塞 | 100M blocks 内 **`0x812B2AD0` SignalSema 调用次数 = 0**（`[HLECOUNT]` 无该桩） |
| `[FRAMEDUMP]` | `n=100000000 flags=00000002 t4=0 t8=0 ctr=0` | 帧计数/帧序始终 0：帧循环从未推进 |
| `[CONSUME]` | `FHRead calls=1 bytes=62100 FIOS-opens=12289` | `g_fh_id` 初值 0x3000 ⇒ 实际只成功打开 **1** 个文件、只读 **1** 次 |
| `[FLOAD]/[FIBINIT]` | 两个 fiber（loader 0x848367C0 / 0x84846860）互相切换 | loader fiber 与 CriFs 等待环 ping-pong |
| `[WD]`（早期） | `blk=812B2A80 fib=84846860 fex=1 ex=1` | 卡点在 DelayThreadCB，且位于 fiber_exec 内 |

### 0.2 根因判断（HLE 语义视角）

旧运行时能跑通的关键是 **“DelayThread/CheckCallback 一旦被调用就真正把 CPU 让给别的线程”**：
`n_thread_yield`（`eboot_probe.cpp:7441-7462`）保存现场、把恢复 PC 推到 `svc+2`、设置
`g_delay_until` 冷却、调用 `sched_pick` 在所有 worker/主线程间切换；fiber 切换末尾还会调用
`g_fiber_pump`（`:7294-7298`）把 worker 线程泵起来。

当前 harness 把“让出”实现成了：
1. `0x812B2A80` DelayThreadCB **根本没有 case**（`recomp_harness.cpp:1171` 的 `default: r0=0`）；
2. 即使 `0x812B2A50` DelayThread 有 case（`:1157-1170`），它只在 fiber 之间 round-robin，**从不把 worker 线程放进来**；
3. `hle_thr_pump()` 14:14 版首行是 `if (g_cur_thr || g_in_fiber_exec) return;`（`:2257-2258`；18.4228 已放松 `g_cur_fiber` 条件，但 nested fiber 阶段仍拒绝），而 MSW 派发器切入 fiber 后 `g_cur_fiber` 一直不清零（`:3357`）→ nested 阶段 **worker 线程仍然不会被调度**。

结果：guest 的 CriFs/线程等待环（`0x8127BE36` / `0x8127C0F8`）持续“让出但没人可切”，
worker 的 sema 永远等不到 signal，loader fiber 永远等不到 worker 完成 → 帧循环不启动。
这与旧运行时注释里记录过的历史根因完全一致（`eboot_probe.cpp:7565-7567`：
“DelayThread 的 CB/200 同族 + 标准 sched_yield 此前全部漏绑 … 主线程在 CriFs 调度等待循环
(0x8127BE36→桩) 里‘让出’却不切给 worker … 调度器空转 327 万次”）。

此外还有一个 harness **自造语义**（旧运行时 HLE 完全没有）：
`fiber_load()` 首次装载时把 guest 的 `[fiber+0x80] |= 1`（`:1195-1196`，日志 `[FACT]`；18.4229 起已开关化默认关），
`[SCHEDRUN]` 收尾又把 `|=2` 当成“完成”（`:2906`）。而 guest 的 fiber 管理器 `0x812067A0`
正是用 bit0 判断“该 fiber 已完成、应摘链并标 finished”——置 1 后当前 fiber 被摘链
（`[GSELF] f1 st=00000003`），另一个 fiber 的目标退化为 self，**两个 fiber 永不互切**。
旧运行时 `n_fiber_init`/`fiber_setup` 只维护寄存器上下文，**从不写 guest 的 fiber 对象字段**
（`eboot_probe.cpp:6940-6962`）。一并修复才能恢复 guest 自己的 A↔B 乒乓。
（该机理与并行产出的 `docs/SCHED_SPIN_ANALYSIS.md` §2.3 / P0-B 独立得出相同结论。）

### 0.3 优先级清单（P0 > P1 > P2）

| 优先级 | 差异 | 一句话 |
|---|---|---|
| **P0-1** | fiber 世界内 worker pump 未闭环 + `0x812B2A80` DelayThreadCB 缺失 | 当前空转的直接原因；旧 `g_fiber_pump`/`n_thread_yield` 未完整移植。14:14 快照：pump 门控已部分放松、DelayThread 加了 `[THRYIELD]`，但 nested boot 阶段不触发（`out30m_thry` CNT 不变），DelayThreadCB 仍缺 |
| **P0-1b** | harness 自造 fiber 状态位：`SCHEDRUN` 仍写 `bit2` 伪完成并 reset ctx；`fiber_load` 的 bit0 写入已开关化（默认关） | 旧运行时 HLE 从不写 guest fiber 字段；该位使 guest `0x812067A0` 摘链标 finished → A/B 永不互切 |
| **P0-2** | `0x812B27B0` GetThreadId 恒返回 `0x100`（18.4228b 回退） | 旧实现返回真实线程 id；**但单独改为 0x4001 已被实测否决**（guest 由 tid 算指针 0x4003 → BADMEM，见 §0.4），需与 CreateThread id 空间联合设计 |
| **P0-3** | LwMutex 三件套语义（Create 返回 malloc 指针；Lock/Unlock 不写 work、不登记持锁） | 旧实现明确注释这是“加载停摆真凶”（`eboot_probe.cpp:7095-7107`） |
| P1-1 | `0x812B27C0` CreateSema / `0x812B2890` CreateEventFlag 用错参数（用 r1 attr 当 init，应用 r2） | 初始计数/位模可能错 |
| P1-2 | WaitSema / WaitEventFlag 不满足条件时“谎报成功”，AOT 下恢复后不重放 SVC | 旧实现 yield+重放；worker 唤醒后会跳过等待 |
| P1-3 | `0x812B1EA0` GetCurrentTick、`0x812B28E0` GetProcessTimeWide、time() 缺失 | 旧实现维护递增虚拟时钟（ts_advance）；时钟恒 0 会破坏超时/帧时长 |
| P1-4 | `0x812B29E0` GetMemBlockBase 写 32 字节（应为 4 字节 base） | 33 次调用，可能覆盖 guest 栈变量 |
| P1-5 | `0x812B18A0` AppUtilDrmOpen 缺失（应返 ENOENT）；AppMgr ReceiveSystemEvent/GetAppState 语义相反 | 旧实现有明确修复记录 |
| P1-6 | Display：`0x812B1810` 被误当 GetFrameBuf、`0x812B1800` SetFrameBuf 缺失、`0x812B1820` 语义错位 | 帧节拍/帧通知 |
| P1-7 | 别名桩缺失：LockLwMutexCB `0x812B2800`、WaitEventFlagCB `0x812B2930`、WaitThreadEnd `0x812B2920` | 旧运行时前者高频（877,998 次） |
| P1-8 | 文件：sceIoOpen/Pread/Lseek/Getstat 全缺；Fios 基本已移植 | 旧运行时经 sceIo 读 2.69 GB |
| P2 | Fiber GetSelf 线程级返回假对象、Run 缺重入保护；strcmp/strncmp/strstr/memcmp 缺失；sprintf 缺 %p/%f；touch/audio/ctrl 细节；Fios OPEN-MISS 策略；GXM RT 大小公式 | 现阶段不阻塞，但语义偏差会放大后续问题 |

---

### 0.4 写作时的“修复进展快照”（截止 14:14:56，供 Lead 避免重复劳动）

> 报告写作期间 harness 已被并行修复推进。以下为写作时点的实际状态（以 `case` 地址为准）：

| 项 | 状态（14:14） | 证据/说明 |
|---|---|---|
| `0x812B27B0` GetThreadId | **三次尝试**：0x100 常量 → 18.4228 `g_cur_thr ?: 0x4001`（BADMEM）→ 18.4228b 回退 0x100 → **18.4228c（写作截止时最新）：返回 `hle_malloc(0x80)` 的 TLS-like guest 对象指针**（`[TIDPTR]`） | 0x4001 实验触发 **BADMEM guest va=0x4003 block=0x811E5EC6**（`err30m_p0b.txt`）⇒ guest 把 tid 当指针基址用（`tid+2` 被解引用）。18.4228c 改成返回 guest 指针是合理方向，但需验证 guest 是否只做相等比较/是否写入该 TLS 对象；**旧运行时的 thread id 语义不能直接照搬** |
| `hle_thr_pump` 门控 | **部分放松**（18.4228）：`if (g_cur_thr || g_in_fiber_exec) return;`，主循环世界可 pump | 但仍**拒绝 `g_in_fiber_exec`**（nested boot fiber 阶段 1.5M 自旋仍不 pump） |
| DelayThread 借时间片 | **已加实验版**：`g_in_fex==0 && !g_in_fiber_exec` 时 `hle_thr_pump(s, 20000)`（`[THRYIELD]`） | 30M 实验 `out30m_thry.txt`：`[CNT] 81198BEA=749` 与基线完全相同 → 该分支在 nested boot 阶段**不触发**，未解决问题 |
| fiber `[fiber+0x80]` bit0 写入 | **已加开关默认关**（18.4229 P0-B）：`if (g_fact_bit)`，`P5D_FACTBIT=1` 恢复旧行为 | 30M 实验 `out30m_p0b.txt` 与 GetThreadId=0x4001 同时启用时崩溃在 0x4003（见上），尚未单独 A/B |
| `[SCHEDRUN]` 伪完成 `bit2` | **仍在**（`v |= 2u`），`[SCHEDRUN3]` 仍 reset ctx 从入口重跑 | 与 sched 分析 P0-B/P0-D 一致，仍是 A/B 乒乓被破坏的来源之一 |
| DelayThreadCB `0x812B2A80` | **仍缺 case**（默认 0） | 本报告 P0-1 的核心 HLE 差异；100M run 实际调用 72,518 次 |
| LwMutex / Sema / EventFlag / 时间 / Display / 文件 | **仍为原状**（见 §2） | 本报告 §2/§3 的 P0-3、P1 清单仍然有效 |

> 结论：Lead 并行修复已完成 GetThreadId 的尝试（需回退重设计）、pump 门控部分放松、fiber bit0 写入开关化；
> **仍待处理**：DelayThreadCB 缺失、nested fiber 阶段线程饿死（P0-A 未闭环）、[SCHEDRUN] 伪完成、
> LwMutex 三件套、Sema/EventFlag 参数与重放、时间/Display/文件等（详见 §2/§3/§4）。

---

## 1. 审计方法与证据来源

### 1.1 “桩地址 → NID → 函数名”映射

* `docs/eboot_imports_with_nid.json`：guest 链接桩地址（0x812B11A0–0x812B2AF0，共 410 个）→ `module` / `func_nid`。本报告用其把 harness `case 0x812Bxxxxu` 反查到 NID。
* `src/loader/nid_import_names.h`：`func_nid` → `模块名/函数名`。
* 抽查校验（与已知实证一致）：`0x812B27B0=0x0FB972F9=GetThreadId`、`0x812B2900=0xC5C11EE7=CreateThread`、`0x812B2940=0xF08DE149=StartThread`、`0x812B1B70=0x414D8CA5=FiberGetSelf`、`0x812B2A50=0x4B675D05=DelayThread`、`0x812B2A80=0x9C0180E1=DelayThreadCB`、`0x812B2AE0=0xEC94DFF7=SetEventFlag`、`0x812B1810=0x05F27764=DisplayWaitVblankStartMultiCB`、`0x812B1800=0x7A410B64=DisplaySetFrameBuf`。

### 1.2 harness HLE 层

* `hle_call()` 入口 `recomp_harness.cpp:605`；`switch (stub_va)` `:650`；`default: r0 = 0` `:1171`；返回 LR `:1175`（等价 `bx lr`）。
* HLE 证据打印：`[HLE]` 前 6000 次调用 `:607`；`[HLECOUNT]` 结束时全量 TOP40 `:3454`；`[WD]` 看门狗 `:2428`。
* 线程调度：`hle_thr_pump()` `:2257`，主循环调用点 `:2512`；fiber 同步切换派发器 `:3343-3360`。
* 共 **69 个 case**（69 个唯一桩），其中纯常量返回 19 个、有实现 46 个、其它 4 个。

### 1.3 旧运行时 HLE

* 旧运行时把每个 NID 绑到 `n_*` lambda（`bind_by_nid`），关键实现均在 `src/loader/eboot_probe.cpp`；
  `thumb_interpreter.cpp` 是解释器本体（`context_switch`/`force_npc`/SVC 返回 r0 保护），本报告只在必需处引用。
* 旧运行时导入调用计数：`_nidcall_h1164.txt`（195 个导入、4,721,644 次；其中非 GXM 4,214,818 次）。
* 旧运行时日志：`C:\dev\p5d_build\run.log`（其尾部的 `[GTIME-STAT] guestTick=12.999s/600` 证明旧运行时有递增的 guest 时钟）。

### 1.4 覆盖差异总览（按旧运行时调用次数加权）

| 统计 | 数值 |
|---|---|
| 旧运行时非 GXM 调用总数 | 4,214,818 |
| harness 有 case 的桩覆盖到的次数 | 3,162,215（75.0%） |
| harness **缺 case**（默认返回 0）的桩覆盖次数 | **1,052,603（25.0%）**，其中旧运行时 `impl=Y` 的 1,052,516 |
| harness case 中“纯常量返回” | 19 / 69 |

非 GXM 分模块覆盖（`covered/missing`，按旧调用次数）：

| 模块 | 旧调用 | 已覆盖 | 缺失 | 缺失且旧实现=Y |
|---|---|---|---|---|
| SceLibKernel | 3,012,742 | 2,106,176 | **906,566** | 906,562（LockLwMutexCB 877,998 + GetProcessTimeWide 21,413 + WaitEventFlagCB 7,104 + …） |
| SceLibc | 964,617 | 941,824 | **22,793** | 22,793（strcmp 22,505 + memcmp/strncpy/strncat/vsprintf…） |
| SceThreadmgr | 92,204 | 89,126 | 3,078 | 3,067（DelayThreadCB 3,067） |
| SceAudio | 58,128 | 0 | **58,128** | AudioOutOutput 36,441 + GetAdopt 21,686 |
| SceNgs | 36,994 | 0 | 36,994 | 36,987 |
| SceRtcUser | 10,089 | 0 | **10,089** | GetCurrentTick 10,089 |
| SceTouch | 6,182 | 0 | 6,182 | TouchRead 6,178 |
| SceFiber | 12,361 | 12,361 | 0 | 全部有 case（但行为有偏差） |
| SceCtrl | 6,181 | 6,181 | 0 | 0（但 0x812B1830 映射错位） |
| SceDisplay | 3,356 | 3,356 | 0 | 0（但 0x812B1810 语义错） |
| SceAppMgrUser | 3,090 | 3,090 | 0 | 0（但语义与旧实现相反） |
| SceAppUtil | 123 | 1 | 122 | 60（DrmOpen 59） |
| SceSysmem | 91 | 91 | 0 | 0（但 GetMemBlockBase 写越界） |
| SceNpManager | 3,091 | 0 | 3,091 | NpCheckCallback（旧实现同 noop=0，行为等价） |
| SceSysmodule | 19 | 8 | 11 | 11 |
| mceFios2 | 1 | 1 | 0 | 0（实现基本对齐） |

> 注：`covered` 只表示“有 case”，不代表语义正确（例：19 个纯常量 case 全部计入 covered）。

---

## 2. 逐项差异表（任务关注面）

**列说明**：`stub` 为 guest 链接桩地址；`harness` 列引用 `recomp_harness.cpp` 当前版本行号；`旧运行时` 列引用 `src/loader/eboot_probe.cpp` 行号。

### 2.1 调度 / 线程

| NID / 名字 | stub | harness 当前行为 | 旧运行时行为（文件:行） | 差异风险 | 优先级 |
|---|---|---|---|---|---|
| `0x0FB972F9` sceKernelGetThreadId | 0x812B27B0 | 14:14:56 版（18.4228c，实验性）：返回 `hle_malloc(0x80)` 的 TLS-like guest 对象指针（`[TIDPTR]`）；更早版本依次为 `0x100` 常量 / 0x4001（18.4228b 回退，见 §0.4） | `return g_cur_thread ? g_cur_thread : 0x4001u;` 主线程 0x4001、worker 0x6001+（`:6385-6403`，bind `:6410`） | guest 在 `0x81207614/0x8120765C` 用 `GetThreadId()==DAT_816CC628` 判主线程；常量让所有上下文“都是主线程”；guest 的 LwMutex owner 判定也失效（旧注释 `:6391-6400` 明确记录“恒返 1 导致所有线程都认为自己是锁 owner”） | **P0** |
| `0xC5C11EE7` sceKernelCreateThread | 0x812B2900 | 分配 HThread、tid=`g_next_tid+=0x1000`（0x40011000 起）、记录 entry；返回 tid（`:1037-1043`） | 用 r3 的 stackSize 分配独立栈、tid=++s_thid（0x6001 起）、记录 TCreate（`:7393-7415`，bind `:8267`） | harness 忽略 stackSize（StartThread 时固定 0x40000，`:1049`）；旧运行时按请求大小分配。一般够用，但栈尺寸语义不一致 | P2 |
| `0xF08DE149` sceKernelStartThread | 0x812B2940 | 建 ctx：r0=arglen、r1=argp（把参数块拷到持久堆，`:1044-1063`）、r14=0xFFFFFFF0，ready=1 | 同样构造 `entry(arglen, argp)`、参数块冻结拷贝、r14=EXIT_SENTINEL（`:7417-7440`，bind `:8266`） | 语义基本对齐（这是旧实现里已成熟的部分） | — |
| `0xDDB395A9` sceKernelWaitThreadEnd | 0x812B2920 | **无 case → default r0=0 立即“成功”** | 旧运行时也未绑定（`impl=N`，本次调用 1 次，直接走默认 0） | 若 guest 用它同步回收线程，立即返回会提前继续；与旧行为相同，但真机语义应为阻塞到线程结束 | P2 |
| `0x1D17DECF` ksceKernelExitDeleteThread / `0x1BBDE3D9` DeleteThread / `0x0C8A38E1` ExitThread | 0x812B2A20 / 2A10 / 2AF0 | 全部无 case → 0（不退出、不删除、不切换） | 旧运行时也 `impl=N`（未绑定）；线程结束靠 worker 入口返回触发 `STOP_SENTINEL`（`:8438-8442`） | guest 调 ExitThread 后当前实现会**继续往下跑**（相当于 return 0 不退出）；旧运行时同样如此但没有暴露问题。可做 `r0=0` + 标记线程 dead | P2 |
| `0xE53E41F6` sceKernelCheckCallback | 0x812B2AC0 | 默认 `r0=0` 纯 noop（`:1134-1146`，仅 `P5D_CB_YIELD=1` 时才有自造让出） | `n_noop`（`:431`，bind `:6411`） | **已对齐**（旧运行时就是 noop）；不要再打开自作主张的 yield | — |
| `0x4B675D05` ksceKernelDelayThread | 0x812B2A50 | `r0=0`；每 64 次（且 `g_in_fex==1`）置 `g_yield_to_main`（`:1155-1167`）；否则仅当 `g_in_fiber_exec && g_fiber_list>1` 时置 fiber 切换目标；**绝不调度 worker 线程** | `n_thread_yield`：`g_sched_active=true`、保存现场、PC=svc+2、`g_delay_until[prev]=g_sched_epoch+nAlive+1` 冷却、`sched_pick/sched_enter` 在所有线程间切换（`:7441-7462`，bind `:7564`） | 让出语义不足：worker 线程拿不到 CPU；且没有虚拟延时冷却（空转）。当前热桩 6,051,302 次 | **P0**（与 P0-1 同修） |
| `0x9C0180E1` ksceKernelDelayThreadCB | 0x812B2A80 | **无 case → default 0**，完全不切换/不延时（`:1168` 附近） | 与 DelayThread 同绑 `n_thread_yield`（`:7568`；`:7565-7567` 注释记录“CB/200 同族 + sched_yield 全部漏绑 → 调度器空转 327 万次”的修复） | **当前 run 实测 72,518 次、调用点 `lr=8127C0F8`（CriFs 等待环），与 SetEventFlag@8127BE56 1:1 配对** → 主线程/loader 在“让出”但 CPU 空转 | **P0-1** |
| `0x3EFD3165` GetTimerTimeWide / `0x48091E0C` StartTimer / `0x869E9F20` StopTimer / `0xAB1E42C4` DeleteTimer / `0x2255B2A5` CreateTimer | 2A30 / 2A40 / 2A70 / 2A90 / 27D0 | 全部无 case → 0 | 旧运行时也全部未绑定（`impl=N`，各 4/4/0/0/2 次） | 旧运行时能跑通说明不阻塞；若后续代码用 timer 超时，返回 0 可能造成立即超时 | P2 |
| （架构）fiber 世界内调度 worker | — | `hle_thr_pump()` 首行现为 `if (g_cur_thr || g_in_fiber_exec) return;`（18.4228 放松了 `g_cur_fiber` 条件），但 **nested boot fiber（`g_in_fiber_exec=1`）阶段仍拒绝 pump**；MSW 派发器切入 fiber 后 `g_cur_fiber=_t` 且不再清 0（`:3357`） | 旧运行时每次 `n_fiber_switch` 末尾调 `g_fiber_pump`（`:6991`，实现 `:7294-7298`）：`sched_save` 当前 fiber → `sched_pick(0)` → `sched_enter` 到 worker；worker 让出时经 `target==0` 回到当前 fiber | **worker 线程永远跑不到**（当前 run 10 个 worker 全部停在 `[SEM] wait have=0`，100M blocks 内 0 次 SignalSema） | **P0-1** |

### 2.2 LwMutex（旧运行时调用量第一梯队）

| NID / 名字 | stub | harness 当前行为 | 旧运行时行为 | 差异风险 | 优先级 |
|---|---|---|---|---|---|
| `0xDA6EC8EF` sceKernelCreateLwMutex | 0x812B2910 | `r0 = hle_malloc(64)`（返回堆指针作为“句柄”）（`:685`） | 返回 **0**，并把 guest 传入的 work 区 0x20 字节清零（`:6926-6929`，bind `:8265`）；注释明确：“成功必须返回 SCE_OK(=0)，返回非 0 会被游戏当失败、导致对象成员里的 work 指针留空，后续锁包装 ldr r0,[r0] 取到 0 而解引用空指针” | 返回类型完全错误；guest 可能把堆指针存进结构，或按失败路径清理 | **P0-3** |
| `0x46E7BE7B` sceKernelLockLwMutex | 0x812B2830 | 纯 `r0=0`（`:686`），不登记持锁、不写 work | `g_lwm_hold[g_cur_thread]++` 登记临界区；`lwm_mark` 回写 `work+4=lockCount`、`work+8=holder`（旧注释 `:7095-7124`：guest 的锁包装 `0x81207344` 是 try-lock，读不到 holder 就会**无限自旋**）；`g_lwm_nopre` 默认持锁期间禁止量子抢占 | guest 永远读不到“我就是持有者” → 自旋；临界区可被 `hle_thr_pump` 在块边界打断 → 链表半写（旧注释 `:7078-7090` 记录过 appJobQueue BADMEM） | **P0-3** |
| `0x3148C6B6` sceKernelLockLwMutexCB | 0x812B2800 | **无 case → 0** | 与 LockLwMutex 同实现（`:7575`）；旧运行时调用 877,998 次（最高频之一） | 一旦 guest 走 CB 版本，持锁语义完全丢失 | **P1**（若 guest 走此路径则为 P0） |
| `0x91FA6614` sceKernelUnlockLwMutex | 0x812B28A0 | 纯 `r0=0`（`:687`） | `lwm_mark(work,cnt,release)` 递减 work 计数、归零清 holder；`g_lwm_hold--`（`:7493-7508`，bind `:7573`）。旧运行时调用 1,005,642 次（第一） | work 计数永远 0；持锁登记不存在 → 与 Lock 同因 | **P0-3** |
| `0x244E76D2` sceKernelDeleteLwMutex | 0x812B27E0 | 无 case → 0 | `n_noop`（`:7576`） | 行为等价 | — |
| （架构）持锁禁止抢占 | — | `hle_thr_pump` 每 0x1FF 主循环块无条件 pump（`:2509`），不检查任何锁持有状态 | 主循环 `if(_holding) nxt=prev;`（`:8426-8432`），持锁期间原地续跑；量子边界 `context_switch` 不重置 | guest 临界区的非原子链表插入可能被 worker 线程切换打断 → 数据竞争/后续 BADMEM | **P1**（与 P0-3 一起） |

### 2.3 信号量（Sema）

| NID / 名字 | stub | harness 当前行为 | 旧运行时行为 | 差异风险 | 优先级 |
|---|---|---|---|---|---|
| `0x1BD67366` sceKernelCreateSema | 0x812B27C0 | `uint32_t cnt = s->r[1];`（取 **attr**）作为初始计数并返回 uid（`:1064-1069`） | 取 **r2（initCount）**：`g_sema[u]=init`（`:6563`，bind `:8268`） | 若 attr≠init（常见 attr=0、init>0），worker 一创建就等不到；此次 run 打印 `init=0` 恰好掩盖了问题 | **P1-1** |
| `0x0C7B834B` sceKernelWaitSema | 0x812B27A0 | 线程上：have≥need 则扣减返回 0，否则置 `g_thr_blocked` 并**照常返回 0**；主线程上：在 HLE 内自旋 pump，最后返回 0（`:1070-1090`） | 不足时登记 `g_wait_sema[thread]` + `sched_yield()` 并**返回 0 让 SVC 重放**（恢复后重新判定）；足够则扣减（`:7351-7391`，bind `:7581`） | AOT 下 `hle_call` 返回 LR，**恢复后不会重放 wait**：线程被唤醒后继续执行“已拿到信号量”的路径 → 计数与真实占用不符；主线程路径在未满足时也返回成功 | **P1-2** |
| `0xE6B761D1` sceKernelSignalSema | 0x812B2AD0 | `g_sema[obj] += 1`（忽略 r1 cnt）并唤醒等待者（`:1091-1096`） | `cnt=r1?r1:1` 累加；带 FIFO 过户/轮转唤醒（`:6565-6627`，bind `:7582`）；旧调用 60,009 次，当前 run **0 次** | 不计 cnt 会造成计数偏少/偏多；唤醒策略简化一般可接受 | **P1** |
| `0xDB32948A` sceKernelDeleteSema | 0x812B2AB0 | 常量 0（`:1120`） | 未绑定（默认 0） | 行为等价 | — |
| `0x595D3FA6` sceKernelGetSemaInfo | 0x812B2850 | 无 case | 未绑定 | 调试用，低 | P3 |

### 2.4 事件标志（EventFlag）

| NID / 名字 | stub | harness 当前行为 | 旧运行时行为 | 差异风险 | 优先级 |
|---|---|---|---|---|---|
| `0x8516D040` sceKernelCreateEventFlag | 0x812B2890 | `uint32_t attr = s->r[1]; ... g_eflag[obj]=attr;`（把 **attr** 当初始位模，且完全没有用 r2 initPattern）（`:1097-1102`） | `g_evt[u] = initPat`，initPat=r2；attr 只打印（`:6527-6538`，bind `:7577`） | 初始位模错误 → 首个 wait 立即阻塞/误判 | **P1-1** |
| `0x83C0E2AF` sceKernelWaitEventFlag | 0x812B2880 | 只做 **AND** 判定（忽略 mode bit0=OR）；`resultp` 写回 **bits**（真机应写回 cur）；成功清位用 `mode==1` 当 CLEARALL、`mode==2/3` 当 CLEAR（模式位含义错）；不满足且主线程时 **直接返回 0**（假成功）（`:1103-1112`） | `orMode=(mode&1)!=0`；成功时 `wr32mem(resultp,cur)`；`mode&4` CLEARALL、`mode&2` 只清等待位；不满足则登记等待 + `sched_yield` + 返回 0 让 SVC 重放（`:7301-7348`，bind `:7579`） | 位模/返回模式全错，guest 事件状态机可能永久自旋或提前继续 | **P1-2** |
| `0xE737B1DF` sceKernelWaitEventFlagCB | 0x812B2930 | **无 case → 0** | 与 WaitEventFlag 同实现（`:7580`）；旧调用 7,104 次 | CB 版等待形同假成功 | **P1-7** |
| `0xEC94DFF7` sceKernelSetEventFlag | 0x812B2AE0 | `g_eflag[obj] |= bits` 并唤醒满足者（`:1113-1119`） | OR 置位 + 把满足条件的等待者设为下一次调度首选（`:6541-6559`，bind `:7578`）；旧调用 15,938 次 | 置位语义对；唤醒/优先级简化可接受 | — |
| `0x5840162C` sceKernelDeleteEventFlag | 0x812B2A60 | 无 case → 0 | 未绑定 | 低 | P3 |

### 2.5 Fiber（SceFiber）

| NID / 名字 | stub | harness 当前行为 | 旧运行时行为 | 差异风险 | 优先级 |
|---|---|---|---|---|---|
| `0x7DF23243` sceFiberRun | 0x812B1B80 | 非嵌套时 `fiber_exec_to_completion()` 递归执行（`:706-741`）；嵌套时置 `g_switch_pending`（`:720`）；**不校验 `g_cur_fiber!=0`**、不返回错误码 | 只允许从宿主线程进入；保存线程现场（PC=svc+2）；`g_in_fiber_world=true`；已在 fiber 内调用返回 `0x80590005`（`:6997-7015`，bind `:7561`） | 非法重入会静默改变状态 | P2 |
| `0xE4283144` sceFiberSwitch | 0x812B1BA0 | 内层调用置 `g_fiber_switch_to`/`g_switch_pending`，派发器用 `nxt` 保存现场（`:742-809`）；自切换=空操作 | 保存 `g_cur_fiber` 现场、**PC=svc+2**、**r0=0**；装载 target 后 `g_cur_fiber=target`、再 `g_fiber_pump()`（`:6965-6993`，bind `:7560`） | AOT 的“保存 nxt”与旧“svc+2”等价，但缺少切换后的 worker pump（见 2.1 架构行） | P0/P1 关联 |
| `0x3B42921F` sceFiberReturnToThread | 0x812B1B60 | 置 `g_fiber_flag_return=1`、r0=0（`:810`）；**只有 fiber_exec 循环消费该 flag**（`:1270`），主派发器路径不消费 | 保存当前 fiber（PC=svc+2、r0=0、argOnRun），回填 argOnReturn，恢复宿主线程现场（`:7018-7033`，bind `:7562`） | 在非 nested 路径调用返回线程会“卡住不回” | P2 |
| `0xF24A298C` _sceFiberInitializeImpl | 0x812B1BB0 | 记录 fiber 列表/entry/arg，返回 0（`:811-821`） | 只登记“未启动”上下文，真正 SP/PC 在第一次 Run/Switch 构造（`:6956-6962`，bind `:7559`） | 基本对齐 | — |
| **（harness 自造）fiber 状态位 `[fiber+0x80]`** | — | `fiber_load()` 的 `bit0=1` 写入 18.4229 起默认关（`if (g_fact_bit)`，`P5D_FACTBIT=1` 恢复）；**`[SCHEDRUN]` 收尾写 `bit2` 当“完成”与 reset ctx 从入口重跑仍在** | 旧 `n_fiber_init` 只登记未启动 ctx、`fiber_setup` 只构造 r0/sp/pc（`:6940-6962`），**HLE 从不写 guest 的 fiber 对象字段**；完成位由 guest 的 `0x812067A0`/trampoline 自己维护 | guest 用 bit0 判“已完成→摘链”，置 1 后当前 fiber 被摘链并标 finished，另一 fiber 目标退化为 self → **A/B 永不互切**（`[FSW]` 只有单方向）。详见 `docs/SCHED_SPIN_ANALYSIS.md` §2.3/P0-B | **P0-1b** |
| `0x414D8CA5` sceFiberGetSelf | 0x812B1B70 | fiber 内返回当前 fiber；**线程级返回堆上 dummy 对象**（避免 NULL 解引用，`:822-886`）；并内嵌“pump fiber2”等自造行为 | **宿主线程上返回 0**，绝不惰性创建（`:6932-6937`，bind `:7558`） | 当前 run 该桩 6,151,916 次（最热），dummy 语义会让 guest 的“无当前 fiber”分支走不到；P0-1 修好后应恢复返回 0 | P1（对齐 sched 分析的 P1-A） |
| `0xE160F844` sceFiberFinalize | 0x812B1B90 | 无 case → 0 | `n_noop`（`:7563`） | 等价 | — |

### 2.6 时间 / RTC（重点：所有“返回 0 的时钟桩”）

| NID / 名字 | stub | harness 当前行为 | 旧运行时行为 | 差异风险 | 优先级 |
|---|---|---|---|---|---|
| `0x23F79274` sceRtcGetCurrentTick | 0x812B1EA0 | **无 case → 返回 0，且不写 8 字节输出** | `g_fake_tick=ts_advance()`，写 lo/hi 到 *out，返回 lo（`:6921`，bind `:7653`）；`ts_advance` 默认**按调用次数**推进（每次 +1000000/60 µs ≈ 16.67ms，`P5D_TIMESCALE` 可调），或 wall-clock 模式（`:6515-6524`） | guest 的 RTC/帧时长恒 0 → 动画/时间轴/超时逻辑退化；当前 run 该桩被调用 1 次（`lr=8127C262`）后不再调用（因为卡在更早的环） | **P1-3** |
| `0xB110C123` sceKernelGetProcessTimeWide | 0x812B28E0 | **无 case → 返回 0，且不写 64 位输出** | 同样 `ts_advance()` 写 64 位并返回 lo（`:6629-6650`，bind `:7583`）；旧调用 21,413 次，是 guest 的“引擎时间/超时”来源 | 当前 run 调用 1 次（`lr=8127C238`，CriFs 定时等待包装 `0x8127C232` 用它算 `now+1000µs` 截止）——0 时钟会让超时判定退化 | **P1-3** |
| `0xDAE8D60F` time() | 0x812B2390 | 无 case → 0，不写输出 | `n_time` 返回真实 `time(nullptr)` 并写 *out（`:6404-6408`，bind `:6412`） | 低（初始化期调用 4 次） | P2 |
| `0xC082CA03` clock() | （见映射；旧运行时 bind `:7642`） | 无 case → 0 | `n_clock` 返回解释器指令数/1000（`:6857-6858`） | 低 | P2 |
| `0x1282C436/0xCD89F464/0xCDDD25FE` Rtc Convert/SetTick/NetworkTick | 0x812B1870/1880/1890 | 全部无 case → 0 | 旧运行时未绑定（本次 0 调用） | 低 | P3 |
| （语义关联）DelayThread 作为帧节拍 | 0x812B2A50/2A80 | 见 2.1：无“虚拟时间推进/延时冷却” | `g_delay_until[prev]=g_sched_epoch+nAlive+1`（`:7455-7457`） | 空转的另一半原因 | P0-1 |

> 注：任务提到的 “GetSystemTime” 在 P5D 的导入表（本 stub 区 410 项）中**不存在**；实际导入的时间函数就是上表这些 + `time()/clock()`。

### 2.7 AppMgr / AppUtil

| NID / 名字 | stub | harness 当前行为 | 旧运行时行为 | 差异风险 | 优先级 |
|---|---|---|---|---|---|
| `0x10B5765F` sceAppMgrReceiveSystemEvent | 0x812B11A0 | 常量 `r0=0`（“success”）（`:696`） | **无事件必须返回 EAGAIN `0x80010001`**（`n_eagain` `:431-434`，bind `:7679`）；注释：返 SCE_OK(0) 会让事件循环空转，实测该 NID 被调 1.27 亿次卡死主循环 | guest 误以为每轮都有系统事件 → 事件轮询空转 | **P1-5** |
| `0x5E86319A` _sceAppMgrGetAppState | 0x812B11B0 | 把 `1` 写进 `*r0` 并返回 0（`:689-695`，18.4086 为过 pre-registration 栅栏而写非零） | 把 `r0` 指向的事件缓冲按 **r1 字节数 memset 0** 后返回 0（`:435-441`，bind `:7680`）；调用方把 `*(u32*)r0` 当“待处理事件数”循环 ReceiveSystemEvent | 当前写 1 会让 caller 认为有 1 个待处理事件；旧注释：不清零会读到栈残留（实测 0x1388=5000）导致每轮空转 5000 次 | **P1-5** |
| `0xDAFFE671` sceAppUtilInit | 0x812B1910 | 常量 0（`:688`） | `n_noop`（`:7665`） | 等价 | — |
| `0x2DB7BE3B` sceAppUtilDrmOpen | 0x812B18A0 | **无 case → 0**（“DLC 存在”） | 离线无 DLC 返回 **ENOENT `0x80010002`**（`n_addcont_absent` `:7669-7672`，bind `:7673`）；旧注释：默认 0 会让 59 次 DLC 全部误判存在、cbA→state6 重试 | DLC/存档路径行为错误 | **P1-5** |
| `0x5DFB9CA0` sceAppUtilSystemParamGetInt | 0x812B18B0 | 无 case → 0（不写 out 参数） | `n_misc_ok` 返回 0（`:1972`，bind `:7852`） | 等价（旧也未写 out） | — |
| 0x6A140498 DrmClose / 0x607647BA SaveDataDataSave / 0x93F0D89F / 0x98630136 Slot* | 18D0/18C0/18F0/1900 | 无 case → 0 | 旧运行时未绑定（impl=N） | 基本等价 | P3 |

### 2.8 Display / 帧通知

| NID / 名字 | stub | harness 当前行为 | 旧运行时行为 | 差异风险 | 优先级 |
|---|---|---|---|---|---|
| `0x7A410B64` sceDisplaySetFrameBuf | 0x812B1800 | **无 case → 0** | 解析 `SceDisplayFrameBuf{size,pBase,pitch,pixelformat,width,height}` 并记录翻页参数（`:1549-1560`，bind `:7657`） | 翻页语义缺失；真机由 display 回调在 vsync 调用它 | **P1-6** |
| `0x05F27764` sceDisplayWaitVblankStartMultiCB | 0x812B1810 | 被**当成 GetFrameBuf**：把 `g_fb_addr` 写进 `*r0` 并返回 0（`:964-967`） | **绑 `n_thread_yield` 做帧节拍让出**（`:7654`）；旧运行时调用 3,356 次 | 语义完全错位：vblank 不等待/不让出 → 帧循环变忙等；且写的是 guest 的 `count` 参数指向（若为小整数会触发 harness 的 `[HLE-BADMEM]` stderr）。**注意：这是旧的错误映射（历史上把 0x812B1810 当 GetFrameBuf），必须纠正** | **P1-6** |
| `0x7D9864A8` sceDisplayWaitSetFrameBufMulti | 0x812B1820 | 被当成 SetFrameBuf：`g_fb_addr = s->r[0]; r0=0`（`:968-970`） | `n_display_ok` 恒 0（`:1561`，bind `:7658`） | 返回值等价，但写入了错误的全局；真实 SetFrameBuf 的解析缺失 | P2 |
| `0xEC5C26B5` sceGxmDisplayQueueAddEntry（帧通知上游） | 0x812B1720 | `gxmvk_present_now()` + 计数（`:975`）；**不调用 guest 的 displayQueueCallback** | 旧 `n_gxm_display_entry` 做 present，并可（`P5D_DISPCALL=1`）嵌套调用 guest display 回调 `cb(cbd)`（`:1120-1160`、`:1501-1540`）；回调里做 SetFrameBuf 翻页 + WaitSetFrameBufMulti 回收。旧注释 `:1502-1506` 记录：不调用回调时，影片播放器拿不到“帧已上屏”事件、序列状态机走不下去 | 帧通知缺失 → 后续“frame loop 已跑但 guest 状态机不推进” | P2（当前 run 未到该阶段；GXM 层另有专门任务） |
| `0xB0F1E4EC` sceGxmInitialize | 0x812B1570 | **无 case**（只在 harness 头部注释出现） | 读出 `SceGxmInitializeParams` 的 `displayQueueCallback / callbackDataSize / maxPending`（`:1565-1572`，bind `:7707`） | 回调地址丢失 → display 回调无从调用 | P2（GXM 任务范围，但影响帧通知） |

### 2.9 文件系统（Fios / sceIo）

| NID / 名字 | stub | harness 当前行为 | 旧运行时行为 | 差异风险 | 优先级 |
|---|---|---|---|---|---|
| `0x774C2C05` sceFiosInitialize / `0x2904B539` Terminate / `0xBF699BD4` FHOpen / `0xB09AFBDF` FHRead / `0xC5C26581` FHGetSize / `0xA88EDCA8` FHClose / `0x45182328` FHCloseSync / `0x1B9A575E` OpIsDone / `0x2AC79DFC` OpWait / `0x3904F205` OpCancel / `0xBF099E16` OpGetError / `0xE4EA92FA` OpDelete | 19B0 / 1970 / 19F0 / 19D0 / 1A00 / 19C0 / 19A0 / 1960 / 1980 / 1990 / 19E0 / 1A10 | 同步立即完成模型：挂 CPK、按成员名读取、fh 从 0x3001、op 从 0x5001；`OpIsDone=1`；`FHRead` 返回读取字节数（大小取 `[sp]`）（`:1001-1036`） | 与 harness 结构相同（harness 由旧实现移植）：`fios_ensure_cpk`、`cpk_read_any`、`OpIsDone=1` 等（`:6107-6147`，binds `:8274-8285`） | **基本对齐**。唯一语义差异：OPEN-MISS 时 harness 返回“有效空句柄”继续（`:1010-1015`，18.3798 记录旧行为返回 0xFFFFFFFF 导致异步 op 永远失败重试），旧实现只在成功时写 fh、miss 保持 0xFFFFFFFF（`:6119-6124`）。这是有意的行为改进，保留但需在日志中跟踪 MISS 数 | — |
| `0x6C60AC61` sceIoOpen / `0x52315AD7` sceIoPread / `0x99BA173E` sceIoLseek / `0xBCA5B623` sceIoGetstat / `0x632980D7` sceClibMemset | 2870 / 2840 / 28B0 / 28F0 / 2860 | **全部无 case → 默认 0**（Open 返回 fd=0、Pread 返回 0 字节） | 旧运行时用宿主文件真读：Open/Pread/Lseek/Getstat 均实现（`:8287-8295`，binds `:8287-8295`；`n_io_pread` `:6255-6304`）；旧统计 Pread 42 次、Lseek 2 次 | 若 guest 走 sceIo 路径，所有读取返回 0/空，loader 状态机卡死（旧注释 `:8288-8289`：“漏绑 = 读到 0 字节，子对象状态机卡死 state2”）。**当前 100M run 未见调用**（未到该路径或全走 Fios），但必须补 | **P1-8** |
| `0x632980D7` sceClibMemset | 0x812B2860 | 无 case | 未绑定 | 等价 | — |

### 2.10 其它“返回常量”桩（Sysmem / libc / ctrl / touch / audio / np / sysmodule）

| NID / 名字 | stub | harness 当前行为 | 旧运行时行为 | 差异风险 | 优先级 |
|---|---|---|---|---|---|
| `0xB8EF5818` sceKernelGetMemBlockBase | 0x812B29E0 | 向 `r1` 指向处写 **8 个 word（32 字节）**：`{base,size,size,0,...}`（`:896-906`） | 只写 **4 字节 base VA** 到 `*basep`（`:6024-6028`，bind `:8270`） | 若 guest 传入 4 字节局部变量，后 28 字节覆盖栈/相邻字段 → 极难排查的越界写。当前 run 33 次调用 | **P1-4** |
| `0xB9D5EBDE` sceKernelAllocMemBlock | 0x812B29F0 | `hle_malloc(size+0x100)`、uid 从 0x101 起、记录 `{addr,size}`（`:887-895`） | CDRAM（type 高字节 0x09/0x0D）**必须 1MB 对齐**，其它 64KB；块尾 guard；注释记录不对齐会让 CRI 视频解码器静默失败（`:5994-6022`，bind `:8269`） | 当前 33 次分配；若含 CDRAM 解码块，可能因未对齐导致后续解码器失败 | P2（视频路径） |
| `0xA91E15EE` sceKernelFreeMemBlock | 0x812B29D0 | 无 case → 0（不回收） | 从 uid→va map 删除（`:6029`） | 泄漏，长期运行才会暴露 | P2 |
| `0xA9C3CED6` sceCtrlPeekBufferPositive | 0x812B1840 | 注入 `0x4008`（CROSS+START）前 200 次，写入 `SceCtrlData`（`:919-926`） | `n_ctrl_peek` 实现（`:1827`，bind `:7817`） | 行为可用；注意 `0x812B1830`（SetSamplingMode）与它共用同一 case，`r0` 是 port 小整数 → `GP(port)` 失败会向 stderr 打印 `[HLE-BADMEM]`（噪声） | P2（清理噪声） |
| `0xA497B150` sceCtrlSetSamplingMode | 0x812B1830 | 与 Peek 共用 case（`:919`） | `n_ctrl_ok` 返回 0（`:1656`，bind `:7816`） | 映射错位：应单独 `r0=0` | P2 |
| `0x169A1D58` sceTouchRead | 0x812B1930 | 无 case → 0 | 按本次 `nBuf` 清零 `SceTouchData` 并返回 **1**（`:452-458`，binds `:7683-7684`）；注释：返回 0 会触发 `vita_ll_touch.c:177` 断言 | 一旦走到触摸输入即失败 | P1（输入路径） |
| `0x10A2CA25` TouchGetPanelInfo / `0x1B9C5D14` TouchSetSamplingState | 1920 / 1940 | 无 case → 0 | `n_noop`（`:7681-7682`） | 等价 | — |
| `0x02DB3F5F` sceAudioOutOutput | 0x812B1EB0 | 无 case → 0（立即返回） | 首次让出一个时间片、重放即通过（`:7466-7470`，bind `:7659`）；用于把音频线程节拍化，避免饿死 IO/主线程 | 音频线程会全速自旋抢 CPU（旧注释：曾 93% SVC） | P2（音频线程目前未跑起来） |
| `0x12FB1767` sceAudioOutGetAdopt | 0x812B1EC0 | 无 case → 0 | `n_noop`（`:7660`） | 等价 | — |
| `0x3B0AE9A9` sceNpCheckCallback | 0x812B1BD0 | 无 case → 0 | `n_misc_ok` 返回 0（`:8249`） | 等价（旧调用 3,089 次） | — |
| `0x53099B7A` sceSysmoduleIsLoaded | 0x812B1E90 | 无 case → 0（=未加载） | `n_noop` 返回 0（`:7664`） | 等价 | — |
| `0x1B58FA3B` strcmp / `0xE4299DCB` strncmp / `0x0D5200CB` strstr / `0x7747F6D7` memcmp / `0x9F87712D` strncpy / `0xFBA69BC2` strncat | 2200 / 23B0 / 21D0 / 2290 / 2310 / 2410 | 全部无 case → **返回 0 = “相等/NULL”** | 旧运行时逐字实现 strcmp/strstr 等（`:6421-6430`）；旧注释：漏绑 strstr 使扩展名匹配恒假、GFD 加载请求无法投递；strncmp 漏绑导致 CRIFSMEM 前缀误判 | **语义毒性极高**：`strcmp()==0` 表示所有路径/键相等，会导致缓存命中错乱、路径分支走错。当前日志命中 strncpy ×22、strncat ×24（低样本，但后续会放大） | P2（若后续路径依赖则升 P1） |
| `0x7449B359` sprintf | 0x812B2280 | 自写 formatter：支持 `%s/%d/%i/%u/%x/%X/%c/%%`，**不支持 `%p/%f/%g/%e` 和精度**（`:927-963`） | 旧 `n_sprintf` 支持 `%p/%f/%g/%e`、宽度/补零/精度（`:6655-6683`，bind `:7584`） | 用 `%p`/浮点的字符串会出现字面 `%p`；当前 7 次调用、风险低 | P2 |
| `0x9F87712D` strncpy 等见上 | — | — | — | — | — |

---

## 3. P0 详细修复建议（可直接粘贴的 case 代码）

> 下列代码均按 harness 现有风格（`RecState* s`、`GP()`、`hle_malloc()`、`r0`）书写；
> 行号/函数名为当前版本，合并时以 `case 0x812Bxxxxu` 定位。

### P0-1 DelayThreadCB 补齐 + fiber 世界内 worker pump（旧 `n_thread_yield` + `g_fiber_pump` 移植）

**第一步：补齐别名 case（最小改动，立即消除 72,518 次隐性空转）**

```cpp
// recomp_harness.cpp 现有 case 0x812B2A50u（约 :1155）:
case 0x812B2A50u:   // ksceKernelDelayThread
case 0x812B2A80u: { // 18.4227: ksceKernelDelayThreadCB —— 旧运行时与 DelayThread 同绑
                    // n_thread_yield（eboot_probe.cpp:7568），此处必须同语义
    r0 = 0;
    // (a) 虚拟延时冷却（完整做法见旧 g_delay_until，eboot_probe.cpp:7455-7457；
    //     需要先引入 §P1-3 的 g_fake_tick 虚拟时钟。最小改动可先省略这一行）
    /* g_delay_until[g_cur_thr] = g_fake_tick + (s->r[0] ? s->r[0] : 1u); */
    // (b) 在 fiber 世界内请求“回主循环跑 worker”，而不是只在 fiber 之间轮转
    if (g_in_fiber_exec && !g_thr.empty()) g_yield_to_main = 1;
    // (c) 原有 fiber round-robin 逻辑保留（下面原样保留 :1157-1166）
    ...
    break; }
```

> 注意：(b) 依赖下一步让 `hle_thr_pump` 能在 fiber 世界里运行；否则 `g_yield_to_main`
> 只是让 fiber 提前返回主循环，worker 仍被 `hle_thr_pump` 首行挡住。

**第二步：移植 `g_fiber_pump`（旧运行时 `eboot_probe.cpp:7294-7298`，调用点 `:6991`）**

```cpp
// 新增：在 fiber 世界里运行就绪 worker 线程（旧 g_fiber_pump 的等价物）
static int g_fiber_pump_cool = 0;
static void fiber_pump_threads(RecState* s) {
  if (g_thr.empty()) return;
  if (!g_in_fiber_exec || !g_cur_fiber) return;
  if (--g_fiber_pump_cool > 0) return;          // 冷却，避免每个 DelayThread 都 pump
  g_fiber_pump_cool = 8;                        // 可按实测调大/调小
  // 旧语义：sched_save(ex)（当前现场即 fiber 现场）→ sched_pick(0) → sched_enter(worker)；
  // AOT 等价：把当前 s(已经是 fiber 现场) 交给 hle_thr_pump，由其保存/恢复。
  const uint32_t saved_fib = g_cur_fiber;
  const int saved_fex = g_in_fiber_exec;
  g_cur_fiber = 0; g_in_fiber_exec = 0;         // 允许 hle_thr_pump 进入（:2255 门控）
  hle_thr_pump(s, 400);                         // 内部会保存/恢复 *s，因此 fiber 现场不丢
  g_cur_fiber = saved_fib; g_in_fiber_exec = saved_fex;
}

// 在 hle_call 的 DelayThread/DelayThreadCB case 中（以及 CheckCallback 打开 yield 时）调用：
case 0x812B2A50u: case 0x812B2A80u: {
    r0 = 0;
    if (g_in_fiber_exec) fiber_pump_threads(s);   // 让 worker 真正拿到 CPU
    ...
}
```

**第三步：不要写 guest 的 fiber 状态位（对齐旧运行时 `eboot_probe.cpp:6940-6962`）**

```cpp
static void fiber_load(RecState* s, uint32_t fiber, uint32_t entry, uint32_t arg) {
  // 旧运行时 HLE 只维护 r0..r15/APSR；[fiber+0x80] 的 active/finished 位由 guest 的
  // fiber 管理器（0x812067A0）与启动 trampoline 维护，HLE 不得代写。
  // 原 :1189-1193 的 `[fiber+0x80] |= 1`（[FACT]）会让 guest 把当前 fiber 摘链并标
  // finished，另一个 fiber 的切换目标退化为 self → A/B 永不互切。
  // 同理删除 [SCHEDRUN] 收尾 :2903 的 `v |= 2` 伪完成写，以及 :2886-2908 的
  // “reset ctx + 从入口重跑”逻辑（应改回正常的“保存/装载现场”）。
  // 调试期可用 P5D_FIBER_ACTIVE_HACK=1 保留旧行为做 A/B。
  ...
}
```
（旧运行时实证：`n_fiber_init` 只 `g_fiber[fobj].started=false; regs清零`，`fiber_setup`
只写 r[13]/r[15]/r[0]/r[1] 与 `argOnRun`，见 `eboot_probe.cpp:6940-6962`。）

**更彻底的做法（推荐，也是旧运行时 P0-2 的等价物）**
把 `hle_call()` 改成“切换后不得写 r0”的契约：任何 HLE 内发生调度/切换时，
返回**新的 guest PC**（旧 `thumb_interpreter.cpp:11705-11718` 的 `if(!force_npc) r[0]=__svcret;`
与 AOT 的 `s->r[0]=r0` 等价规则）。当前 harness 无嵌套切换时 `return s->r[14]`（`:1172`）是正确的；
但 `g_yield_to_main` 由 `fiber_exec_to_completion` 消费（`:1262-1263`）后，外层调度器
（`:3340-3357`）必须**清 `g_cur_fiber` 或把 fiber 现场存回 `g_fctx`**，否则：
* `hle_thr_pump` 永远 early-return（`:2255`）；
* 下一次 fiber 切换会把“已返回的主线程寄存器”当成 fiber 现场保存（旧运行时手工保证
  `save(prev) → setup(target) → load(target) → owner=target → pump` 的顺序，见 `:6965-6993`）。

**验证**：跑 30M blocks 后应出现 `[THR]` worker 真正执行（`[SEM]` 出现 WAKE、
`[HLECOUNT]` 出现 `0x812B2AD0 SignalSema`），`[FRAMEDUMP] ctr` 开始增长。

### P0-2 GetThreadId 的 id 语义（旧 `eboot_probe.cpp:6385-6403`）—— 需与 CreateThread id 空间联合设计

```cpp
// 当前 :699
// case 0x812B27B0u: r0 = 0x100u; break;   // 删掉常量
case 0x812B27B0u:                                                    // sceKernelGetThreadId
    r0 = g_cur_thr ? g_cur_thr : 0x4001u;   // 主线程 0x4001 / worker 用真实 tid
    break;
```

**实测注意（18.4228b，必读）**：直接返回 0x4001 已被 Lead 实验否决——
`err30m_p0b.txt` 出现 `[BADMEM] guest va=0x00004003 unmapped block=0x811E5EC6`，
即 guest 把线程 id 当成了某个对象/指针链的一环（`tid+2=0x4003` 被解引用）。
因此**不能只改 GetThreadId 的返回值**，必须先回答：

1. guest 在 `0x8120764C` 读取的 `[0x816CBC08+0xA20]`（旧运行时的 `[GETTID-FENCE] owner`）
   到底由谁写入、写入的是 id 还是对象指针；
2. `0x811E5EC6` 附近的 `[tid+2]` 或 `tid 作为句柄` 的指令来自哪里（反汇编 0x811E5E80-0x811E5F00）；
3. `CreateThread` 返回的 tid 空间（当前 0x40011000 起）与旧运行时（0x6001 起）、以及
   sema/eflag uid 空间（0x5001/0x6001xxxx）是否需要统一成“小整数句柄表”。

**建议的中间方案**（先不动返回值，只补日志）：在 `case 0x812B27B0` 记录 `lr` 与返回值，
同时记录 `[0x816CBC08+0xA20]`，跑 2M 找出 `0x4003` 的算式与上下文，再做联合修改。
（旧运行时主线程 0x4001 / worker 0x6001+ 是已验证可用的 id 值，但前提是旧运行时的
`[0x816CBC08+0xA20]` 与 `CreateThread`/`GetThreadId` 使用同一套 id 约定。）

**18.4228c 已做的实验**：返回 `hle_malloc(0x80)` 的 TLS-like guest 对象指针（日志 `[TIDPTR]`）。
这是目前最合理的假设（guest 把返回值当指针基址），但必须验证：
(1) 所有调用点是否只把它当不透明句柄（除了 `0x8120764C` 的相等比较）；
(2) 不同线程是否应拿到**不同**对象（旧语义 worker tid 不同 → TLS 也应不同）；
(3) 与 `CreateThread` 返回值是否需要配对（当前 CreateThread 返回 0x40011000 起的 id）。

### P0-3 LwMutex 三件套（旧 `eboot_probe.cpp:6926-6929, 7117-7124, 7493-7557`）

```cpp
// 需要新增的状态（放在 g_sema/g_eflag 附近，:235 左右）
static int g_lwm_hold[64] = {0};        // 或用 std::map<uint32_t,int>，键 = 线程 id
// work 布局（旧逆向结论）：attr@0 / lockCount@4 / holder@8

case 0x812B2910u: {                     // sceKernelCreateLwMutex —— 旧 :6926-6929
    uint32_t work = s->r[0];
    if (work) { uint8_t* p = GP(work); if (p) std::memset(p, 0, 0x20); }
    r0 = 0;                             // 必须 0；绝不能返回 hle_malloc(64)！
    break; }
case 0x812B2830u:                       // sceKernelLockLwMutex
case 0x812B2800u: {                     // sceKernelLockLwMutexCB —— 旧 :7574-7575 同实现
    uint32_t work = s->r[0], cnt = s->r[1] ? s->r[1] : 1u;
    auto rd32w = [](uint32_t va, uint32_t dflt) -> uint32_t {
        uint8_t* p = GP(va); uint32_t v = dflt;
        if (p) std::memcpy(&v, p, 4); return v; };
    if (work) {
        uint32_t lc = rd32w(work + 4u, 0u) + cnt;      // lockCount += cnt
        if (uint8_t* p = GP(work + 4u)) std::memcpy(p, &lc, 4);
        uint32_t holder = g_cur_thr ? g_cur_thr : 0x4001u;
        if (uint8_t* p = GP(work + 8u)) std::memcpy(p, &holder, 4);
    }
    g_lwm_hold[g_cur_thr & 63u]++;                      // 登记“本线程在临界区”
    r0 = 0; break; }
case 0x812B28A0u: {                     // sceKernelUnlockLwMutex —— 旧 :7493-7508
    uint32_t work = s->r[0], cnt = s->r[1] ? s->r[1] : 1u;
    if (work) {
        uint32_t lc = 0; uint8_t* p = GP(work + 4u);
        if (p) std::memcpy(&lc, p, 4);
        lc = (lc > cnt) ? (lc - cnt) : 0u;
        if (p) std::memcpy(p, &lc, 4);
        // 归零即释放：旧语义 holder 在 lockCount 归零时清 0（eboot_probe.cpp:7123）
        if (uint8_t* q = GP(work + 8u)) {
            uint32_t z = lc ? (g_cur_thr ? g_cur_thr : 0x4001u) : 0u;
            std::memcpy(q, &z, 4);
        }
    }
    if (g_lwm_hold[g_cur_thr & 63u] > 0) --g_lwm_hold[g_cur_thr & 63u];
    r0 = 0; break; }
```

**配套**：持锁期间禁止抢占（旧 `:7125-7132, 8426-8432`）。AOT 的抢占点是主循环每
0x1FF 块的 `hle_thr_pump()`（`:2509`）；建议在主循环 pump 前加：

```cpp
// 主循环 :2509 附近
if ((n & 0x1FF) == 0 && !g_thr.empty()) {
    bool busy_lock = (g_lwm_hold[(g_cur_thr ? g_cur_thr : 0x4001u) & 63u] > 0);
    if (!busy_lock) hle_thr_pump(&s, 400);   // 持锁时原地续跑，避免切开 guest 临界区
}
```
（注：若 P0-1 的 `fiber_pump_threads` 也做 pump，需要在同一处统一检查 `busy_lock`。）

---

## 4. P1 修复建议（代码要点）

### P1-1 CreateSema / CreateEventFlag 用 r2（init）

```cpp
// :1064  当前: uint32_t cnt = s->r[1];   → 应为 r2
case 0x812B27C0u: {                                   // sceKernelCreateSema(name,attr,init,max,opt)
    uint32_t initCnt = s->r[2];                       // 旧 eboot_probe.cpp:6563
    uint32_t obj = (g_next_sema += 0x1000u);
    if (obj) g_sema[obj] = initCnt;
    r0 = obj; break; }

// :1097  当前: uint32_t attr = s->r[1]; g_eflag[obj]=attr; → 应为 r2
case 0x812B2890u: {                                   // sceKernelCreateEventFlag(name,attr,initPat,opt)
    uint32_t initPat = s->r[2];                       // 旧 eboot_probe.cpp:6527
    uint32_t obj = (g_next_eflag += 0x1000u);
    if (obj) g_eflag[obj] = initPat;
    r0 = obj; break; }
```

### P1-2 WaitSema / WaitEventFlag：AOT 下“重放”契约

旧运行时靠“PC 停在 SVC → 恢复后重放”实现等待；AOT 的 `hle_call` 返回 LR（`:1172`）会跳过 SVC。
最小可用改法：**等待不满足时，让派发器回到同一个 stub 地址重放**（并在 worker 上置 blocked）。

```cpp
// 新增：hle_call 返回“重放桩地址”而不是 LR 的通道
static uint32_t g_hle_replay_va = 0;      // 非 0 时，派发器把 next-pc 设回该桩

// hle_call 末尾（:1170 附近）：
if (g_hle_replay_va) {
    uint32_t rp = g_hle_replay_va; g_hle_replay_va = 0;
    return rp;                            // 让 block 返回该地址 → 派发器重新进入同一 stub
}
s->r[0] = r0;
return s->r[14];

// WaitSema（:1070）/ WaitEventFlag（:1103）不满足分支：
if (have < need) {
    if (g_cur_thr) {                      // worker：阻塞，恢复点=stub 本身（重放）
        g_thr_blocked = 1; g_thr_wait_obj = obj; g_thr_wait_bits = 0;
        g_hle_replay_va = 0x812B27A0u;    // ← 修 WaitSema 时用对应桩地址
    } else {                              // 主线程：先 pump 一次 worker，再重放
        hle_thr_pump(s, 400);
        g_hle_replay_va = 0x812B27A0u;
    }
    r0 = 0; break;
}
```

> `hle_thr_pump` 保存 `t.ctx.r[15] = pc`（`:2418`）时会自动把“重放桩地址”存进线程上下文，
> 唤醒后即重新执行 wait（真机语义）。若不想改返回值契约，退而求其次：在 `hle_thr_pump`
> 唤醒时显式把 `t.ctx.r[15]` 设回 wait 桩地址（war 待对象记录在 HThread 里）。

WaitEventFlag 同时修正：
```cpp
bool orMode = (mode & 1u) != 0;
bool ok = orMode ? ((cur & bits) != 0u) : ((cur & bits) == bits);
if (ok) {
    if (s->r[3]) { uint8_t* p = GP(s->r[3]); if (p) std::memcpy(p, &cur, 4); } // 写 cur，不是 bits
    if (mode & 4u) g_eflag[obj] = 0;                    // WAITCLEARALL
    else if (mode & 2u) g_eflag[obj] = cur & ~bits;      // WAITCLEAR
    r0 = 0;
} else { /* 同上：blocked + replay */ }
case 0x812B2930u:  // WaitEventFlagCB —— 与 0x812B2880 合并（旧 :7580）
```

### P1-3 时间桩（旧 `ts_advance` / `n_rtc_tick` / `n_get_processtime_wide`）

```cpp
// 文件级静态（与旧运行时同模型，默认 CALLCOUNT 模式：每次调用推进 1e6/60 µs）
static uint64_t g_fake_tick = 0x100000ull;
static uint64_t ts_advance() {
    g_fake_tick += (uint64_t)(1000000.0 / 60.0);   // 旧 eboot_probe.cpp:6521
    return g_fake_tick;
}

case 0x812B1EA0u: {                                  // sceRtcGetCurrentTick(U64* out)
    uint64_t t = ts_advance();
    if (s->r[0]) { uint8_t* p = GP(s->r[0]); if (p) std::memcpy(p, &t, 8); }
    r0 = (uint32_t)t; break; }
case 0x812B28E0u: {                                  // sceKernelGetProcessTimeWide(void)
    uint64_t t = ts_advance();
    r0 = (uint32_t)t;
    // 旧实现还会把 64 位写 *out（outp=r0），PSV 真机无参数；如需兼容可判断 s->r[0] 是否为有效 guest 指针
    break; }
case 0x812B2390u: {                                  // time(time_t* out)
    uint32_t t = (uint32_t)::time(nullptr);
    if (s->r[0]) { uint8_t* p = GP(s->r[0]); if (p) std::memcpy(p, &t, 4); }
    r0 = t; break; }
```
（`GetProcessTimeWide` 的“写 out 指针”在旧实现里实际存在：`:6629-6631`；若担心真机无参数，
可只在 `GP(s->r[0])` 有效时写，避免误写。）

### P1-4 GetMemBlockBase 只写 4 字节

```cpp
// 当前 :896-906 写了 w[8]（32 字节）
case 0x812B29E0u: {                                  // sceKernelGetMemBlockBase(uid, void** base)
    auto it = g_memblks.find(s->r[0]);
    uint32_t base = (it == g_memblks.end()) ? 0u : it->second.first;
    if (s->r[1]) { uint8_t* p = GP(s->r[1]); if (p) std::memcpy(p, &base, 4); }  // 旧 :6024-6028
    r0 = 0; break; }
```

### P1-5 AppMgr / AppUtil

```cpp
case 0x812B11A0u: r0 = 0x80010001u; break;           // ReceiveSystemEvent: 无事件 EAGAIN（旧 :431-434）
case 0x812B11B0u: {                                  // GetAppState(out, size)
    uint32_t p = s->r[0], sz = s->r[1] ? s->r[1] : 0x80u;
    if (p) { uint8_t* q = GP(p); if (q) std::memset(q, 0, sz); }   // 旧 :435-441
    r0 = 0; break; }
case 0x812B18A0u: r0 = 0x80010002u; break;           // AppUtilDrmOpen: 离线无 DLC → ENOENT（旧 :7669-7672）
```

### P1-6 Display 语义纠正

```cpp
case 0x812B1800u: {                                  // sceDisplaySetFrameBuf(fb,pitch,fmt,sync)
    if (s->r[0]) g_fb_addr = s->r[0];                // 旧 :1549-1560 解析 SceDisplayFrameBuf
    r0 = 0; break; }
case 0x812B1810u:                                    // sceDisplayWaitVblankStartMultiCB
    // 旧运行时绑 n_thread_yield（eboot_probe.cpp:7654）：这里等价为“帧节拍让出”
    r0 = 0; g_vblank_pending = 1; break;             // 由主循环/DelayThread 统一做让出与虚拟时间推进
case 0x812B1820u: r0 = 0; break;                     // sceDisplayWaitSetFrameBufMulti（旧 n_display_ok :1561）
```
> `[HLE-BADMEM]` 噪声来源：`0x812B1830`（SetSamplingMode，r0=port）当前与 Peek 共用 case 会
> `GP(port)` 失败。拆开：`case 0x812B1830u: r0 = 0; break;`。

### P1-7 别名补齐一览（直接加 `case` 别名即可）

```cpp
case 0x812B2830u: case 0x812B2800u:   // LockLwMutex / LockLwMutexCB（旧 :7574-7575）
case 0x812B2880u: case 0x812B2930u:   // WaitEventFlag / WaitEventFlagCB（旧 :7579-7580）
case 0x812B2A50u: case 0x812B2A80u:   // DelayThread / DelayThreadCB（旧 :7564-7568）
```
`WaitThreadEnd`（0x812B2920）需要新增：若目标 tid 仍 alive/ready，则 block+replay；否则返回 0。

### P1-8 sceIo* 最小实现

旧运行时用宿主文件/CPK 真读（`:6255-6304`、`:8287-8295`）。AOT 版最小实现：
`Open(path)` 返回内部 fd（map 保存路径/偏移）→ `Pread(fd,dst,size,seekPtr)` 按偏移从
`cpk_read_any()` 或宿主文件读入 → 返回读取字节数；`Lseek` 改偏移；`Getstat` 不存在返回
`0x80010002`（ENOENT）、存在返回 0。若 guest 当前只走 Fios，可延后但**不要用 0 冒充成功**。

---

## 5. P2 / P3 清单

| 项 | 现状 | 建议 |
|---|---|---|
| Fiber GetSelf 线程级 dummy 对象 | `:822-886`：`g_dummy_getself`（`[GETSELF-DUMMY]`） | 旧运行时线程级返回 0；P0-1 修好后改回 0（旧 `:6932-6937`），先观察 `0x812067A0` 的“无当前 fiber”路径 |
| Fiber Run 重入 | `:706` 不校验 `g_cur_fiber` | 旧实现已在 fiber 内调用返回 `0x80590005`（`:7000`）；补上并在非法时打印 |
| Fiber Switch 目标校验 | `:742` 有 `[FSWSAN]` 判断（地址区间） | 保留；补 `target` 未初始化时按旧 `fiber_setup` 构造 |
| strcmp/strncmp/strstr/memcmp/strncpy/strncat | 无 case → 全部 0 | 逐字实现（旧 `:6421-6430`）；strcmp 至少实现“返回 -1/0/1” |
| sprintf 格式化 | `:927-963` 缺 `%p/%f/%g/%e`、精度 | 补 `%p`（最常用）与浮点；或直接借用旧 `n_sprintf`（`:6655-6683`） |
| Fios OPEN-MISS 返回空句柄 | `:1010-1015` 有意的改进 | 保留；但在 `[FIOS] OPEN-MISS` 日志里同时统计 MISS 总数，防“空文件”掩盖内容缺失 |
| free() 直接返回 0 | `:656` | 旧实现真回收（`:6415`）；长期运行的内存增长需注意，可先记录分配表做泄漏观测 |
| GetRenderTargetMemSize | 固定 `0x33D000`（`:907-917`） | 旧实现按 params 公式计算（`:6035-6050`）；GXM 任务范围，保留固定值但记录 params 以便核对 |
| Ctrl SetSamplingMode | 与 Peek 共用 case（`:919-926`） | 拆开为 `r0=0`，消除 stderr `[HLE-BADMEM]` 噪声 |
| Touch / Audio / Np | 见 §2.9-2.10 | touch 至少实现“清 nBuf 段并返回 1”；audio 可用 n_thread_yield 等价让出 |

---

## 6. “返回 0 但旧实现返回真实值 / 句柄 / 副作用”的桩清单（任务重点）

> 这是本次审计的“反模式清单”：凡旧实现有真实返回或输出参数的桩，当前 harness 若走
> `default: r0=0` 或常量 case，都会造成 guest 状态错误。按危害排序：

| # | stub | NID / 名字 | harness 返回（当前） | 旧运行时返回/写出 | 后果 |
|---|---|---|---|---|---|
| 1 | 0x812B27B0 | GetThreadId | `0x100` 常量 | `g_cur_thread ?: 0x4001` | 主线程/worker 身份混淆，锁 owner 判定失效 |
| 2 | 0x812B2A80 | DelayThreadCB | 0（无 case） | 让出 + 虚拟延时 + 调度 worker | CriFs 等待环空转（当前 72,518 次） |
| 3 | 0x812B2830/2800/28A0 | Lock/UnlockLwMutex(CB) | 0 常量（不写 work） | 写 `work+4/+8`、维护持锁计数 | guest try-lock 自旋；临界区被切开 |
| 4 | 0x812B2910 | CreateLwMutex | `hle_malloc(64)` 指针 | **0** + 清 work 0x20 字节 | 返回类型错误；work 指针语义丢失 |
| 5 | 0x812B2910 | CreateSema/EFlag 初始值 | 用 r1(attr) 而非 r2(init) | 用 r2 | 初始计数/位模错误 |
| 6 | 0x812B27A0/2880/2930 | WaitSema/WaitEventFlag(CB) | 不满足也返回 0（假成功） | 阻塞 + 重放 SVC | 唤醒后跳过等待 → 计数/位模不一致 |
| 7 | 0x812B2AD0 | SignalSema | 固定 +1 | `+ (r1?:1)` + 过户/轮转唤醒 | 计数偏差 |
| 8 | 0x812B1EA0/28E0 | GetCurrentTick/GetProcessTimeWide | 0，且不写 out | 递增 64 位虚拟时钟并写 out | 时钟恒 0；超时/帧时长退化 |
| 9 | 0x812B29E0 | GetMemBlockBase | 0（但写了 32 字节） | 0，只写 4 字节 base | **越界写** guest 栈/结构 |
| 10 | 0x812B18A0 | AppUtilDrmOpen | 0（= 存在） | `0x80010002` ENOENT | 59 个 DLC 误判存在 → state6 重试 |
| 11 | 0x812B11A0 | AppMgrReceiveSystemEvent | 0（= 有事件） | `0x80010001` EAGAIN | 事件轮询空转（旧实测 1.27 亿次） |
| 12 | 0x812B11B0 | AppMgrGetAppState | 写 1 到 *out | 按 r1 字节数清零 | caller 误判有 1 个待处理事件 |
| 13 | 0x812B1800 | DisplaySetFrameBuf | 0（无 case） | 解析 SceDisplayFrameBuf 翻页参数 | 翻页/帧通知链缺失 |
| 14 | 0x812B1810 | WaitVblankStartMultiCB | 按 GetFrameBuf 写 *r0 | 帧节拍让出 | 语义错位、忙等、可能写坏参数 |
| 15 | 0x812B1930 | TouchRead | 0 | 清 SceTouchData、返回 1 | guest 断言（vita_ll_touch.c:177） |
| 16 | 0x812B2A50 | DelayThread | 0（只做 fiber 轮转） | 全线程调度 + 延时冷却 | 让出无效 |
| 17 | 0x812B1B70 | FiberGetSelf | 线程级返回 dummy 指针 | 线程级返回 **0** | “无当前 fiber”分支不可达 |
| 18 | 0x812B2200 等 | strcmp/strncmp/strstr/memcmp | 0（= 相等/NULL） | 逐字比较 | 路径/资源匹配错误 |
| 19 | 0x812B2A60 / 0x812B27E0 | DeleteEventFlag / DeleteLwMutex | 0（无 case） | 删除/清理（旧 DeleteLwMutex=noop） | 句柄泄漏，影响低 |
| 20 | 0x812B2840/2870/28B0/28F0 | IoPread/IoOpen/IoLseek/IoGetstat | 0（= 读到 0 字节/成功） | 宿主文件真读 | loader 状态机卡死（旧注释实证） |

---

## 7. 落地顺序与验证方法

1. **先 P0-1（最小版：DelayThreadCB 别名 + `fiber_pump_threads`）** → 30M blocks：
   期望 `[THR]` 出现 worker 执行日志、`[HLECOUNT]` 出现 `0x812B2AD0 SignalSema`、
   `[FRAMEDUMP] ctr` 从 0 开始增长；用 `hle_probe.py` 观察 `812B2A80` 的 kind 变为“有实现”。
2. **P0-2 GetThreadId** → 观察 `[WD]/[HLECOUNT]` 中 `0x812B27B0` 仍高频但
   worker 线程内的返回值不再等于主线程 id；可用 `[HLE]` 行 a0/lr 对照 guest 的
   `DAT_816CC628` 判定路径是否改善。
3. **P0-3 LwMutex** → 观察 `[HLECOUNT]` 中 Lock/Unlock 常量 case 变为 impl；
   若 guest 有 try-lock 自旋，`0x812B2830` 次数应显著下降。
4. **P1-1/2 同步原语** → `[SEM] WAKE` / `[EVT] WAKE` 出现，worker 不再“假成功”。
5. **P1-3 时间** → 在 loader 通过后检查宿主日志里 `GetCurrentTick/GetProcessTimeWide`
   返回值递增（可在 case 内加 8 次打印）。
6. **P1-4/5/6** → 消除 `[HLE] malloc OOM`/`[HLE-BADMEM]` 噪声、AppMgr 轮询不再刷屏、Display 路径出现 `[DISPLAY]`。
7. **回归**：每次改动用 `hle_probe.py` 对比 `[HLECOUNT]` 的 unique/stub 分布与
   `[FRAMEDUMP]` 帧计数；A/B 时只改一个 case（旧运行时的“默认关开关”风格）。

工具命令：

```powershell
py -3 tools\hle_probe.py C:\dev\tmp\recomp_split\out100m_682a2.txt
py -3 tools\hle_probe.py out30m_682a2.txt --top 15 --min-count 10 --json probe.json
```

---

## 8. 附录 A：harness `hle_call` 全部 case 表（69 个，14:14 版本行号）

> 分类为 `hle_probe.py` 对 `switch` 块的启发式判定：`常量 0xN` = 只返回常量；
> `有实现` = 有实际副作用/读写；`其它` = 需要人工复核（如 FHGetSize 的三元表达式、共享块的第二个标签）。

| stub | NID | 模块/函数 | harness 行 | 分类 |
|---|---|---|---|---|
| 0x812B11A0 | 10B5765F | SceAppMgrUser/sceAppMgrReceiveSystemEvent | 698 | 常量 0x0 |
| 0x812B11B0 | 5E86319A | SceAppMgrUser/_sceAppMgrGetAppState | 691 | 有实现 |
| 0x812B1270 | 1997DC17 | SceGxm/sceGxmProgramParameterGetCategory | 1133 | 常量 0x1 |
| 0x812B12A0 | 277794C4 | SceGxm/sceGxmProgramFindParameterByName | 1128 | 有实现 |
| 0x812B1340 | 4811AECB | SceGxm/sceGxmTextureInitLinear | 989 | 有实现 |
| 0x812B13D0 | 6A6013E1 | SceGxm/sceGxmSyncObjectCreate | 985 | 有实现 |
| 0x812B1470 | 8734FF4E | SceGxm/sceGxmBeginScene | 975 | 有实现 |
| 0x812B1530 | A949A803 | SceGxm/sceGxmShaderPatcherGetProgramFromId | 1123 | 有实现 |
| 0x812B1580 | B291C959 | SceGxm/sceGxmGetRenderTargetMemSize | 909 | 有实现 |
| 0x812B15E0 | B98C5B0D | SceGxm/sceGxmDisplayQueueFinish | 978 | 有实现 |
| 0x812B1640 | C61E34FC | SceGxm/sceGxmMapMemory | 920 | 常量 0x0 |
| 0x812B1660 | CA9D41D1 | SceGxm/sceGxmDepthStencilSurfaceInit | 995 | 有实现 |
| 0x812B16F0 | E84CE5B4 | SceGxm/sceGxmCreateContext | 973 | 有实现 |
| 0x812B1720 | EC5C26B5 | SceGxm/sceGxmDisplayQueueAddEntry | 977 | 有实现 |
| 0x812B1730 | ED0F6E25 | SceGxm/sceGxmColorSurfaceInit | 979 | 有实现 |
| 0x812B17F0 | FE300E2F | SceGxm/sceGxmEndScene | 976 | 有实现 |
| 0x812B1810 | 05F27764 | SceDisplay/sceDisplayWaitVblankStartMultiCB | 966 | 有实现 |
| 0x812B1820 | 7D9864A8 | SceDisplay/sceDisplayWaitSetFrameBufMulti | 970 | 有实现 |
| 0x812B1830 | A497B150 | SceCtrl/sceCtrlSetSamplingMode | 921 | 其它 |
| 0x812B1840 | A9C3CED6 | SceCtrl/sceCtrlPeekBufferPositive | 922 | 有实现 |
| 0x812B1860 | 79A0160A | SceSysmodule/sceSysmoduleLoadModule | 700 | 常量 0x0 |
| 0x812B1910 | DAFFE671 | SceAppUtil/sceAppUtilInit | 690 | 常量 0x0 |
| 0x812B1960 | 1B9A575E | mceFios2/sceFiosOpIsDone | 1034 | 常量 0x1 |
| 0x812B1970 | 2904B539 | mceFios2/sceFiosTerminate | 1004 | 常量 0x0 |
| 0x812B1980 | 2AC79DFC | mceFios2/sceFiosOpWait | 1035 | 常量 0x0 |
| 0x812B1990 | 3904F205 | mceFios2/sceFiosOpCancel | 1036 | 常量 0x0 |
| 0x812B19A0 | 45182328 | mceFios2/sceFiosFHCloseSync | 1033 | 其它 |
| 0x812B19B0 | 774C2C05 | mceFios2/sceFiosInitialize | 1003 | 有实现 |
| 0x812B19C0 | A88EDCA8 | mceFios2/sceFiosFHClose | 1033 | 其它 |
| 0x812B19D0 | B09AFBDF | mceFios2/sceFiosFHRead | 1021 | 有实现 |
| 0x812B19E0 | BF099E16 | mceFios2/sceFiosOpGetError | 1037 | 常量 0x0 |
| 0x812B19F0 | BF699BD4 | mceFios2/sceFiosFHOpen | 1005 | 有实现 |
| 0x812B1A00 | C5C26581 | mceFios2/sceFiosFHGetSize | 1032 | 其它 |
| 0x812B1A10 | E4EA92FA | mceFios2/sceFiosOpDelete | 1038 | 常量 0x0 |
| 0x812B1B60 | 3B42921F | SceFiber/sceFiberReturnToThread | 812 | 有实现 |
| 0x812B1B70 | 414D8CA5 | SceFiber/sceFiberGetSelf | 824 | 有实现 |
| 0x812B1B80 | 7DF23243 | SceFiber/sceFiberRun | 708 | 有实现 |
| 0x812B1BA0 | E4283144 | SceFiber/sceFiberSwitch | 744 | 有实现 |
| 0x812B1BB0 | F24A298C | SceFiber/_sceFiberInitializeImpl | 813 | 有实现 |
| 0x812B2210 | 20FE0FFF | SceLibc/abort | 699 | 常量 0x0 |
| 0x812B2250 | 5B9BB802 | SceLibc/free | 658 | 常量 0x0 |
| 0x812B2260 | 6DC1F0D8 | SceLibc/memset | 672 | 有实现 |
| 0x812B2270 | 7205BFDB | SceLibc/memcpy | 667 | 有实现 |
| 0x812B2280 | 7449B359 | SceLibc/sprintf | 929 | 有实现 |
| 0x812B22A0 | 775A0CB2 | SceLibc/malloc | 652 | 有实现 |
| 0x812B22C0 | 826BBBAF | SceLibc/exit | 702 | 有实现 |
| 0x812B22D0 | 85B924B7 | SceLibc/strcpy | 681 | 有实现 |
| 0x812B22E0 | 8AECC873 | SceLibc/strlen | 677 | 有实现 |
| 0x812B2340 | A9363E6B | SceLibc/memalign | 659 | 有实现 |
| 0x812B2360 | BA2EF93E | SceLibc/__at_quick_exit | 686 | 常量 0x0 |
| 0x812B2370 | BFE02B3A | SceLibc/__cxa_set_dso_handle_main | 651 | 常量 0x0 |
| 0x812B23E0 | E7EC3D0B | SceLibc/calloc | 1149 | 有实现 |
| 0x812B27A0 | 0C7B834B | SceLibKernel/sceKernelWaitSema | 1072 | 有实现 |
| 0x812B27B0 | 0FB972F9 | SceLibKernel/sceKernelGetThreadId | 701 | 常量 0x100 |
| 0x812B27C0 | 1BD67366 | SceLibKernel/sceKernelCreateSema | 1066 | 有实现 |
| 0x812B2830 | 46E7BE7B | SceLibKernel/sceKernelLockLwMutex | 688 | 常量 0x0 |
| 0x812B2880 | 83C0E2AF | SceLibKernel/sceKernelWaitEventFlag | 1105 | 有实现 |
| 0x812B2890 | 8516D040 | SceLibKernel/sceKernelCreateEventFlag | 1099 | 有实现 |
| 0x812B28A0 | 91FA6614 | SceLibKernel/sceKernelUnlockLwMutex | 689 | 常量 0x0 |
| 0x812B2900 | C5C11EE7 | SceLibKernel/sceKernelCreateThread | 1039 | 有实现 |
| 0x812B2910 | DA6EC8EF | SceLibKernel/sceKernelCreateLwMutex | 687 | 有实现 |
| 0x812B2940 | F08DE149 | SceLibKernel/sceKernelStartThread | 1046 | 有实现 |
| 0x812B29E0 | B8EF5818 | SceSysmem/sceKernelGetMemBlockBase | 898 | 有实现 |
| 0x812B29F0 | B9D5EBDE | SceSysmem/sceKernelAllocMemBlock | 889 | 有实现 |
| 0x812B2A50 | 4B675D05 | SceThreadmgr/ksceKernelDelayThread | 1157 | 有实现 |
| 0x812B2AB0 | DB32948A | SceThreadmgr/sceKernelDeleteSema | 1122 | 常量 0x0 |
| 0x812B2AC0 | E53E41F6 | SceThreadmgr/ksceKernelCheckCallback | 1136 | 有实现 |
| 0x812B2AD0 | E6B761D1 | SceThreadmgr/sceKernelSignalSema | 1093 | 有实现 |
| 0x812B2AE0 | EC94DFF7 | SceThreadmgr/sceKernelSetEventFlag | 1115 | 有实现 |

**harness 没有 case 的桩（本报告关注面，含旧运行时调用次数）**

| stub | NID | 名字 | 旧调用次数 | 备注 |
|---|---|---|---|---|
| 0x812B2800 | 3148C6B6 | LockLwMutexCB | 877,998 | 与 Lock 同实现即可 |
| 0x812B2A80 | 9C0180E1 | DelayThreadCB | 3,067（当前 run 72,518） | **P0-1** |
| 0x812B2930 | E737B1DF | WaitEventFlagCB | 7,104 | 与 WaitEventFlag 同实现 |
| 0x812B2920 | DDB395A9 | WaitThreadEnd | 1 | block+replay 或检查 alive |
| 0x812B1EA0 | 23F79274 | RtcGetCurrentTick | 10,089 | 虚拟时钟 |
| 0x812B28E0 | B110C123 | GetProcessTimeWide | 21,413 | 虚拟时钟 |
| 0x812B18A0 | 2DB7BE3B | AppUtilDrmOpen | 59 | ENOENT |
| 0x812B1800 | 7A410B64 | DisplaySetFrameBuf | 0（旧日志未见） | 翻页 |
| 0x812B1930 | 169A1D58 | TouchRead | 6,178 | 返回 1 |
| 0x812B1EB0 | 02DB3F5F | AudioOutOutput | 36,441 | 首次让出 |
| 0x812B2840 | 52315AD7 | sceIoPread | 42 | 真读 |
| 0x812B2870 | 6C60AC61 | sceIoOpen | 2 | 真读 |
| 0x812B28B0 | 99BA173E | sceIoLseek | 2 | — |
| 0x812B28F0 | BCA5B623 | sceIoGetstat | 1 | ENOENT |
| 0x812B27E0 | 244E76D2 | DeleteLwMutex | 0 | 旧 n_noop |
| 0x812B2A60 | 5840162C | DeleteEventFlag | 0 | — |
| 0x812B1B90 | E160F844 | FiberFinalize | 0 | 旧 n_noop |
| 0x812B29D0 | A91E15EE | FreeMemBlock | 0 | 回收 |
| 0x812B2200 | 1B58FA3B | strcmp | 22,505 | 逐字实现 |
| 0x812B23B0 | E4299DCB | strncmp | 124 | 逐字实现 |
| 0x812B21D0 | 0D5200CB | strstr | 100 | 逐字实现 |
| 0x812B2290 | 7747F6D7 | memcmp | 21 | 逐字实现 |
| 0x812B2310 | 9F87712D | strncpy | 20 | 逐字实现 |
| 0x812B2410 | FBA69BC2 | strncat | 2 | 逐字实现 |
| 0x812B2390 | DAE8D60F | time | 2 | 真实时间 |
| 0x812B2550 | 6DEA815A | powf | 1 | 旧运行时已实现（`:7588`） |

---

## 9. 证据索引

### 9.1 harness（`C:\dev\tmp\recomp_split\recomp_harness.cpp`，2026-10-28 14:14 版）

| 主题 | 行（14:14 版本） |
|---|---|
| HLE 入口 / 返回契约 | `:605`、`:1173-1175` |
| [HLE] / [HLECOUNT] / [WD] 打印 | `:607`、`:3454`、`:2428` |
| switch 起点 / default | `:650`、`:1171` |
| GetThreadId 常量（18.4228b 回退点） | `:701` |
| LwMutex Create/Lock/Unlock | `:687-689` |
| AppMgr / AppUtil | `:691-698`、`:690` |
| Fiber Run/Switch/Return/Init/GetSelf | `:708-888` |
| 线程创建/启动 | `:1039-1065` |
| Sema（Create/Wait/Signal/Delete） | `:1066-1097`、`:1122` |
| EventFlag（Create/Wait/Set） | `:1099-1121` |
| CheckCallback | `:1136-1148` |
| DelayThread（+18.4228 `[THRYIELD]`） | `:1157-1170` |
| Fios 的 12 个 case（Init/Open/Read/GetSize/Close/Op*） | `:1003-1038` |
| hle_thr_pump 门控（18.4228 放松） / 主循环 pump | `:2257-2258`、`:2512` |
| fiber 同步切换派发（MSW） | `:3343-3360`（`g_cur_fiber=_t` 在 `:3357`） |
| `fiber_load` / bit0 写入（18.4229 开关） | `:1192`、`:1195-1196` |
| `[SCHEDRUN]` 伪完成 bit2 / ctx reset | `:2906`、`:2889-2911` |
| GetMemBlockBase / AllocMemBlock | `:898-908`、`:889-897` |
| Display 两 case | `:966-972` |
| Ctrl 两 case | `:921-928` |

### 9.2 旧运行时（`C:\dev\p5dsrc\src\loader\eboot_probe.cpp`）

| 主题 | 行 |
|---|---|
| n_noop / CheckCallback | `:431`、bind `:6411` |
| n_eagain / n_appmgr_getstate | `:431-441`、binds `:7679-7680` |
| n_touch_read | `:452-458` |
| n_rtc_tick / ts_advance | `:6921`、`:6515-6524` |
| n_display_setfb / n_display_ok | `:1549-1561` |
| n_gxm_initialize（读 displayQueueCallback） | `:1565-1572` |
| n_gxm_display_entry（present + 可选 guest 回调） | `:1120-1160`、`:1501-1540` |
| n_alloc_memblock / n_get_memblock_base / n_free_memblock | `:5994-6029` |
| n_fios_* | `:6107-6147` |
| n_io_pread / n_io_read / n_io_lseek | `:6255-6339` |
| n_gettid | `:6385-6403`、bind `:6410` |
| n_time / n_clock | `:6404-6408`、`:6857-6858` |
| n_create_eventflag / n_set_eventflag / n_create_sema / n_signal_sema | `:6527-6627` |
| n_get_processtime_wide | `:6629-6650` |
| n_sprintf | `:6655-6683` |
| n_create_lwmutex | `:6926-6929`、bind `:8265` |
| n_fiber_self / fiber_setup / n_fiber_init / n_fiber_switch / n_fiber_run / n_fiber_return | `:6930-7033`、binds `:7558-7562` |
| sched_save / sched_enter / sched_enter_boundary | `:7044-7077` |
| lwm_mark / g_lwm_hold / g_lwm_nopre | `:7117-7132` |
| sched_pick / runnable / sched_yield / g_fiber_pump | `:7142-7299` |
| n_wait_eventflag | `:7301-7348`、binds `:7579-7580` |
| n_wait_sema | `:7351-7391`、bind `:7581` |
| n_create_thread / n_start_thread | `:7393-7440`、binds `:8266-8267` |
| n_thread_yield | `:7441-7462`、binds `:7564-7572` |
| n_audioout_output | `:7466-7470`、bind `:7659` |
| n_unlock_lwmutex / n_lock_lwmutex | `:7493-7557`、binds `:7573-7575` |
| RTC/Display/AppUtil/AppMgr binds | `:7653-7684` |
| n_addcont_absent（DrmOpen ENOENT） | `:7669-7673` |
| 线程层 binds / fios binds / io binds | `:8265-8295` |
| 主循环 / STOP_QUANTUM / 持锁不抢占 | `:8334-8448`、`:8426-8432` |
| SVC 层 r0 保护 / context_switch | `thumb_interpreter.cpp:967-978`、`:11705-11718` |

### 9.3 数据文件

| 文件 | 用途 |
|---|---|
| `_nidcall_h1164.txt` | 旧运行时 NID 调用次数（195 imports / 4.72M calls） |
| `docs/eboot_imports_with_nid.json` | 410 个链接桩 → module/func_nid |
| `src/loader/nid_import_names.h` | func_nid → 模块/函数名 |
| `_gxm_table.csv` | 100 个 GXM 导入的 NID/名字/stub/旧原生处理器 |
| `docs/OLD_RUNTIME_FIBER_HLE.md` | 旧 fiber/线程模型的既有详细分析（本报告 2.5/2.1 的补充） |
| `docs/SCHED_SPIN_ANALYSIS.md` | 并行产出的“调度空转根因”分析；其 P0-A/P0-B/P0-C 与本报告 P0-1/P0-1b 相互印证 |
| `C:\dev\tmp\recomp_split\out100m_682a2.txt` | 当前 run 证据（UTF-16，1.38M 行） |
| `C:\dev\tmp\recomp_split\out30m_682a2.txt` | 当前 run 证据（30M blocks 版本，便于快速比对） |
| `C:\dev\p5d_build\run.log` | 旧运行时日志（含 `[GTIME-STAT]` 虚拟时钟证据） |
| `out30m_thry.txt` / `err30m_thry.txt` | 18.4228 `[THRYIELD]` 实验：CNT=749 不变 → nested 阶段未触发 |
| `out30m_p0b.txt` / `err30m_p0b.txt` | 18.4229 P0-B + GetThreadId=0x4001 实验：BADMEM guest va=0x4003 block=0x811E5EC6 |

---

## 10. 复现命令

```powershell
# 1) HLE 统计与可疑桩（本任务工具）
cd C:\dev\p5dsrc
py -3 tools\hle_probe.py C:\dev\tmp\recomp_split\out100m_682a2.txt
py -3 tools\hle_probe.py C:\dev\tmp\recomp_split\out30m_682a2.txt --top 15 --min-count 10

# 2) 只跑 harness（验证单点修复）
cd C:\dev\tmp\recomp_split
.\recomp_harness.exe 0x81198464 30000000 | Out-File -Encoding utf8 out30m_check.txt
py -3 C:\dev\p5dsrc\tools\hle_probe.py out30m_check.txt --top 15

# 3) 对照旧运行时 HLE 语义（源码行）
#    C:\dev\p5dsrc\src\loader\eboot_probe.cpp 的 n_* 实现与 bind_by_nid 列表
```

---

*报告结束。本报告只写 `docs/HLE_DIFF_REPORT.md` 与 `tools/hle_probe.py` 两个文件，未修改 harness / 生成器 / batch / 其它 docs。*

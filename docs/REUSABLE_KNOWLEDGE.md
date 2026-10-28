# P5D-PC 可复用知识清单（当前静态重编译主路径尚未使用）

> 生成于 2026-10-28（M726），方法：对 `docs/P5D_MASTER.md`（3,355 个 §18 章节 / 2,731 条 ANTI-FORGETTING 记录）、
> 299 份 `_archive` 文档、配套报告（HLE_DIFF_REPORT / SCHED_SPIN_ANALYSIS / OLD_RUNTIME_FIBER_HLE / GXM_ABI_TEST_REPORT /
> P5R_ANDROID_REFERENCE / RELEASE_COMPLIANCE）、旧运行时源码（`src/loader/eboot_probe.cpp` 711 KB、
> `src/arm/thumb_interpreter.cpp` 990 KB、`src/gxm/vulkan_backend.cpp` 399 KB）与历史产物做了一次全面挖掘。
> 本文只收录「已经做过/已经验证、但当前静态重编译路线还没接上」的知识；每一项都给出证据路径。

---

## 0. 一句话结论

**这个项目历史上已经两次做到「原版代码画出真实画面」：**

1. **旧运行时（AOT 块执行 + 解释器 fallback + 自研 Vulkan 后端）**：
   - M37 首次 `sceGxmDisplayQueueAddEntry` 渲染首帧（§18.3578）；
   - M48 到达 **S1_TITLE**（388 帧，§18.3589）；
   - M53 标题影片 `c_p5d001.usm` **真实画面可见**（红笔刷 + P5D 标题插画，§18.3594）；
   - M68 离开 TITLE 进入 UI 内容帧（§18.3609）；
   - **M72 抓到原版 P5D 主菜单 S2_MENU**：DANCING!/TUTORIAL/COMMU/COLLECTION/CONFIG/SYSTEM + 中文说明（§18.3613）；
   - M78 `P5D_PAD_SCRIPT` 精确输入把主菜单推进到 SAVE 界面（§18.3619）；
   - M80 进入 S3_SELECT 候选（黄绿底 + 角色插画，§18.3621）；
   - 阻塞点：`appGxModel.c:809` 双重释放断言（§18.3622）、NGS 音频 `VoiceLockParams` 异常（§18.3589）。
2. **原生重写路线（route ②，PORT M1–M18）**：`p5d_play.exe` / `p5d_ui_shell_demo.exe` 等已用**真实游戏资产**渲染
   S4 对局 HUD、判定、音符、主菜单/选曲界面（证据：`C:\dev\p5d_build\host_p5menu.png`、`host_p5title.png`、
   `host_p5d_select.png`、`host_p5ui.png`、`p5d_select_real.png`、`title_shot*.png`、`renders\port_m11_*`）。

而从 M87 起切换到**静态重编译分支**（当前 M725）后，渲染/HLE 丰富度回到初始化阶段——**旧路线的渲染器与 HLE 层
是可以直接搬过来的最大一笔资产**。另外，旧路线可执行文件仍在且能启动：
`C:\dev\p5d_build\eboot_probe_aot.exe`（1.7 MB，2026-10-25；冒烟 12 s 生成 2.1 MB 日志，挂载 data.cpk 10,555 项，
进入 DEBUG MENU 状态机），可作为**行为金标准（oracle）**与截图来源。

---

## A. 调度 / 启动 / 线程 / 内存（静态 harness 直接可修）

| # | 可复用知识 | 关键细节 | 证据 | 状态 |
|---|---|---|---|---|
| A1 | **Display Queue 回调从未分发** | `sceGxmInitialize` 参数：`displayQueueCallback=0x811EE7C3`、`cbDataSize=8`、`paramBufSize=0x1000000`、swapchain 960×544。guest 调 `DisplayQueueAddEntry` 3089 次，但回调体（内部 `blx 0x812B1800 SetFrameBuf` + `blx 0x812B1820 WaitSetFrameBufMulti`）从未执行 → `[SETFB]=0`，影片播放器永远收不到 presented 事件 | MASTER §ARCH_14 §2.5/§4.2；HLE_DIFF_REPORT §2.8；旧实现 `eboot_probe.cpp:1120-1160,1501-1540` | ❌ 未实现 |
| A2 | **CreateLwMutex 语义仍然错** | 正确：返回 0（SCE_OK）+ `memset(work,0,0x20)`；work 布局 attr@0 / lockCount@4 / holder@8；Lock/Unlock 必须写 +4/+8。现在 harness 返回 `hle_malloc(64)` 指针且不写 holder，guest try-lock 循环 `0x81207344` 永远自旋 | OLD_RUNTIME_FIBER_HLE §3.1；HLE_DIFF_REPORT §2.2；harness 旧行 884 | ❌ |
| A3 | **缺失的 HLE case 清单** | `0x812B2A80` DelayThreadCB（CriFs 等待循环 lr=0x8127C0F8，100M 块 72,518 次）、`0x812B1EA0` RtcGetCurrentTick（旧 10,089）、`0x812B28E0` GetProcessTimeWide（21,413）、`0x812B18A0` AppUtilDrmOpen（59 次；必须返回 ENOENT 0x80010002，否则 DLC 被当存在→state6 重试）、`0x812B1EB0` AudioOutOutput（36,441；缺失→音频线程自旋）、`0x812B2920` WaitThreadEnd、`0x812B2A20/2A10/2AF0` exit/delete、`0x812B2A30/2A40/2A70/2A90` timers、`0x812B2550` powf、`0x812B1FB0` NgsVoiceGetInfo、`0x812B2060` NgsVoiceGetStateData | HLE_DIFF_REPORT §8；旧日志调用计数 | ❌ 逐个 S |
| A4 | **两段式 VA→文件映射（权威）** | SCE 容器 0x1000 头，ELF@0x0A0，phdr@0x0E0（4×32B）：PH0 文件 0x10E0 / VA 0x81000000 / filesz 0x321810；PH1 文件 0x3228F0 / VA 0x81325000 / filesz 0x21BA74 / memsz 0x3AA4A0（BSS 至 0x816CF4A0）。代码段 `file=VA-0x81000000+0x10E0`；数据段 `file=VA-0x81325000+0x3228F0`；单一线性公式在数据段偏 0x3800，会把 BSS 当代码 | MASTER §18.1180/§18.1182、§8.1 | 工具需按两段式 |
| A5 | **CDRAM memblock 必须 1 MiB 对齐** | `type>>24 ∈ {0x09,0x0D}` → 对齐 0x100000，其它 0x10000，另加 0x10000 保护；错误串 0x81320804、`"Can't create Video decoder."`。harness 的 `0x812B29F0` 只是 `hle_malloc(size+0x100)`，**不对齐** → 视频解码被静默禁用 | MASTER §h145k72~k79；eboot_probe.cpp:5994-6022 | ❌ S |
| A6 | **内存需求基线** | guest memblock 需求 458.5 MB / 107 次分配（最大 58MB、50MB×3、15MB×4）；我们已加 96MB 0xA2000000 窗口，保持余量即可 | MASTER M705 §18.4250 | ✅ 已做 |
| A7 | **语言 stub 决定资源集** | `0x812B18B0` 返回 -1 → `DAT_81670354=1` 恒成立 ⇒ 只认 `c_` 资源集（`data/C/`、`c_title_p5.bin`、`c_mainmenu_p5.bin`）；资源层必须按 `c_` 名字解析 | MASTER §4BZ.1 | 资源层 |
| A8 | **Fios 基本是死的，CRI 用 sceIo 直读 CPK** | mceFios2 12 个导入中只有 `sceFiosInitialize` 被调用 1 次；42/42 次 `sceIoPread` 全部来自 guest CriFs `lr=0x812951FB`，CriFs 自己解析 `@UTF` 算物理偏移。**不要再投入 Fios 异步语义** | MASTER §ARCH_14 §2.2/§2.3.1 | ✅ 方向正确 |
| A9 | **sceIoPread ABI 细节** | r0=fd、r1=dst、r2=size、r3=seekVa（42 次调用全为 0）；真正 64 位偏移在 `[sp]:[sp+4]`；返回字节数，负数=错误（调用者 `cmp r0,#0; bge`）。返回 0 字节会让加载状态机停在 state2 | MASTER §ARCH_14 §3.4；已在 M725 实现，保留错误符号约定 | ✅ 已做 |
| A10 | **CPK / @UTF / CRILAYLA 规格** | `"CPK "`；@UTF 大端（table/base/columns，5B 列描述，类型编码 f32/str/blob…）；TOC 行 DirName/FileName/FileOffset/FileSize/ExtractSize/ID；物理偏移 = FileOffset+min(TocOffset,ContentOffset)；CRILAYLA LE 头，前 0x100 字节直接复制，LZ **逆序**解码，ExtractSize=uncomp+0x100 | MASTER §ARCH_14 §4.1 | CpkReader 部分 |
| A11 | **Fios 健康度指标** | `[CONSUME] FHRead calls=1 bytes=62100` vs `FHOpen=12,289` 说明路径解析/miss 路径坏了；一行日志即可判断 | HLE_DIFF_REPORT §0.1 | 建议加日志 |
| A12 | **帧驱动器与帧计数** | `0x81198BE8`: `bl 0x811E5CDA`（Fios/async 队列处理）→ `CheckCallback` → `0x812067A0` → 读 `[0x816CC630]&2` 循环；帧循环 `0x81198C8E..0x81198D08` 自增 `[0x816CC630+0xC]`（+4/+8 是时间字段） | SCHED_SPIN_ANALYSIS §2.1 | 仪表 |
| A13 | **Fiber 管理器 `0x812067A0` 契约** | `self[0x80]&1` → 摘链（prev@0x94/next@0x98，头尾在 `[0x816CBC08+0xA18/A1C]`），state|=2；target=prev 否则 tail；target==self/0 → 不切换直接返回；否则 `sceFiberSwitch`。HLE 绝不能写 `[fiber+0x80]` | SCHED_SPIN_ANALYSIS §2.1(c) | bit0 已关 |
| A14 | **旧运行时切 fiber 的顺序** | 保存 cur_fiber、`pc=(svc&~1)+2`、r0=0、apsr；装载目标；owner=target；pump；SVC 层 `if(!force_npc) r[0]=__svcret`。顺序错 → 上下文混淆（`f2_pc=0x81207344,r0=2`） | OLD_RUNTIME_FIBER_HLE §1.3/§1.5；thumb_interpreter.cpp:967-978/11006/11705 | [UNCERTAIN] |
| A15 | **Fiber 对象布局与栈** | +0x80 state / +0x84 entry(Thumb) / +0x88 argOnRun / +0x8C stackbase=fobj+0xA0 / +0x94 prev / +0x98 next；wrapper `0x81206700` malloc `0xA0+stack`，trampoline `0x81206BDC`，栈顶=fobj+malloc_size | OLD_RUNTIME_FIBER_HLE §1.2；SCHED_SPIN P1-B | [UNCERTAIN] |
| A16 | **三态调度 + replay 规则** | 阻塞等待 = 让出并 **replay SVC**（pc 不变）；主动让出/DelayThread = pc=svc+2 不 replay；fiber switch = svc+2 + r0=0；quantum 抢占 = boundary 且重置 quantum | OLD_RUNTIME_FIBER_HLE §2.2/§2.3；MASTER §5ZI.1/5ZI.2 | 部分 |
| A17 | **EventFlag 丢唤醒（混合模式）** | 同 uid 同时被 mode=2(CLEAR) 与 mode=5(OR+CLEARALL) 等待时，先到者 `g_evt&~bits` 或 =0 导致后者 replay 见 0；修复 wake 只把 `[EVT] WAKE 0→56`，**不改变验收指标**（瓶颈是供给不是唤醒） | MASTER h841/h842/h843 | 别把它当主线 |
| A18 | **Job pool 规格（1/4/1 worker）** | 池对象：sema@0/capSema@4/doneEvt@8/lwmutex@0xC/state@0x10/ring@0x14/cap@0x18/cons@0x1C/prod@0x20/workers@0x24/nWorkers@0x28/pending@0x2C；record 32B `[fn@0,args@4..,pool@0x1C]`；表 `[0x8169E080+idx*4]`；路径 A `0x811E63A8` 信号后阻塞在 evtflag@+8，路径 B `0x811E64F0` 直接返回；worker `0x811E68C8`；state==2 退出 | MASTER §5ZH、§2Q | 部分 |
| A19 | **21 线程清单 = 存活度 oracle** | 入口：`0x811E68C9` pool workers、`0x8120767D` guest threads、`0x8127C1D3` CriFs、`0x811539F3` GXM send、`0x8105C24F/0x8105C07B`；CriFs 等 evt 0x1A/1B/1E/20/22/23；worker 等 sema 0x11/14/17；thread 0x600E 占 34.3% picks | MASTER §5ZI.4、h840 | 建议仪表 |
| A20 | **GetMemBlockBase 只写 4 字节** | 旧运行时只写 base 4B；harness 写 32B 会踩邻近 guest 栈/结构体（极难排查） | HLE_DIFF_REPORT §2.10；eboot_probe.cpp:6024-6028 | ❌ S |
| A21 | **AppMgr/AppUtil 返回码** | `ReceiveSystemEvent` 无事件时返回 `0x80010001`(EAGAIN)（返回 0 导致 1.27e8 次自旋）；`GetAppState` `memset(out,0,r1)`；DrmOpen 返回 ENOENT | HLE_DIFF_REPORT §2.7 | 部分 |
| A22 | **时钟不能每次调用前进** | 旧 `n_rtc_tick` 每次 +16,667 µs，而 guest 每帧调 ~3.2 次（10,089/3,060）→ guest 时间快 3.2×；应改用 wall clock 或 `[0x816CC630+0xC]` | MASTER §ARCH_14 §2.7 | ❌ |
| A23 | **断言桩策略** | `0x8115F7E0` = `movs r0,#1; bx lr`（决策桩）；`0x8115F9CA` 固定崩在 0x8115F9D6；ignore 路径会把断言变成 NULL 传播（0x8117BB42→0x8117BB80→0x81181CFA BADMEM 0x94）。**正确做法=abort-in-place 定位真因** | OLD_RUNTIME_FIBER_HLE §4。 | harness 现为 ignore |
| A24 | **Title async done-bit 无人写** | `0x81193A6A` 只有 `async[0]&2` 才返回 `async[+8]`；子状态 0x81193A90..0x81193DBC **没有任何写 bit1**，唯一写是 `0x81193A80 orrs #0x10000`（bit16）⇒ state7 永久等待 | MASTER §ARCH_23 §2.3 | [UNCERTAIN] |
| A25 | **Boot 顺序与 2000 帧长杆** | module_start 0x81198464 → CRT 0x811982BC → 0x811845C8 → init 0x8118464C（18 调用）→ initDataRead 9 步：0x8118484F、sound、gamedata、msgwindow、dancemain、**mainmenu（F29→F2208，长杆）**、mainmenubg、network、dlccheck | MASTER §4BZ.2、§ARCH_23 §2.4 | 排障地图 |
| A26 | **Task-node 分发契约** | `0x811DB1F0`：node[0]==2 否则跳过；node[4]&1 跳过；r1=node[0x28]；返回 -1 → 0x811DAD8C 摘链；0 保留；否则写回；next=node[0x4C]。异步对象 +0x00 bit1=done、+0x08 ret、+0x04 step、+0x0C/0x10/0x14 handles；构造 0x81193A00 | MASTER §ARCH_23 §2.1/§2.3 | 仪表契约 |
| A27 | **App-state 表正确基址（修正）** | 行 = `0x813B01B4 + state*0x50`；enter@+0x44、update@+0x48、leave@+0x4C，共 7 行；id：0 DEBUG MENU、1 initread、2 FREE MODE、3 COMMU、4 TUTORIAL、**5 MAIN MENU**、**6 TITLE**。`_APPSTATES.txt` 扫的是指针单元 `0x813B01AC` → 全是垃圾；ARCH_16 §4.2 的 tag 列还错位一行 | ARCH_16 §0/§4.2；ARCH_22 §2.1 | ❌ 直接用 |

---

## B. 渲染 / GXM / 着色器 / 网格 / UI

| # | 可复用知识 | 关键细节 | 证据 | 状态 |
|---|---|---|---|---|
| B1 | **最小可行帧 = 12 个 program** | 每帧约 3 draw：①全屏 clear 三角（vert `A71B28370132FF67` + frag `26F80E96E57EB001`，stride 12）②③两张全屏 compose quad（vert `ABE1E100963724B0` + frag `5D73281E76A107B2`，stride 16）；再 histogram（`4A3A4C8DFD6083F6`/`E9728942A6857EC0` → `AA9A2000/AE000000` 224×68 str224）、adaptive（`8D301D508EDD0DF5`/`BD212AB1360EF2F7` → `AA9E6000/AE200000`）、tonemap（`EC5D3D7E8BC3DADA`/`5BE8035296882A66` → `A0E1D000` 960×544 str1024）。**只要 clear+compose 就能出第一张非黑帧** | ARCH_4 §2.3/§4.2；ARCH_21 §6.4 | ❌ |
| B2 | **RT/帧缓冲几何（8 面）** | backbuffer 960×544 **stride 1024** @ A0F00000/A1300000/A1700000；224×68 str224 @ AE000000/AE200000；movie 960×544 **str960** @ AEB00000/AEE00000/AF100000（从未绘制）。MemSize：960×544 s3 msaa2→0x33D000、1088×1088 s1→0xD1000、224×68→0x43000、960×544 s1→0x80000 | ARCH_4 §3.2；ARCH_10 §2.6 | 部分 |
| B3 | **帧边界 = DisplayQueueAddEntry；三缓冲轮换** | 3089 entries = 3089 帧；cd 顺序 A1300000→A1700000→A0F00000；每帧 update→BeginScene→draws→EndScene→present(copy to swapchain)→DisplayEntry | ARCH_4 §4.1；ARCH_1 §4.4 | vblank 部分 |
| B4 | **BeginScene 参数语义 + 深度被 guest 清零** | r2=RenderTarget（旧日志写 color0 是错的）、r3=ValidRegion(NULL)、colorSurface=第7参、depth=第8参；`0x811EA3D0 movs r3,#0` 主动丢弃深度面（0x816CBD64）；flag 0x4=FRAGMENT_TRANSFER_SYNC 前 2160 场景有效 | ARCH_10 §2.6；ARCH_21 §4.6 | 部分 |
| B5 | **图元枚举 + 索引公式 + app→GXM 映射** | POINTS=1, LINE_STRIP=2, TRIANGLES=3, TRIANGLE_STRIP=4, LINES=5；索引数 n / 2n-2 / n(n%3==0) / 3n-6 / n；strip→list (i,i+1,i+2)；`PRIM_MAP[5]={3,4,5,1,2}`（app 0⇒TRIANGLES） | MASTER §4F.14.1 | ❌ S |
| B6 | **2D sprite 顶点 + 12 参 draw + 索引顺序** | app 侧 24B `[x,y,z,alpha,u,v]`；预处理后 16B；line 12B；索引 `{0,1,2,0,2,3}`；参数 (x,y,z,w,h,uvOffU,uvOffV,rotDeg,alpha,uvRect,tex,beginFrame)；屏幕缩放 `0x81540A98/A9C` | MASTER §4F.5/§4F.14.2 | ❌ M |
| B7 | **内容层 program 无矩阵** | `172A9E736C7DC35E`：attrs position/color/texcoord0，单一 uniform **`screen`(float4)** + sampler；顶点是**屏幕像素坐标**；`6952EF6C507D515E` 只有 colorMap | ARCH_21 §6.4/§6.5 | ❌ S |
| B8 | **GXP header/参数布局 + program census** | magic `GXP\0`；+0x14 flags bit0=frag；+0x24 param_count；+0x28 params_off；entry 16B（name_off@0、cat/type/cc/cont@4、semantic@6、array@8、res_idx@0xC）。Census：注册 1631、蒙皮 3D 681、静态 3D 213、lit 48，**实际只用 12 个** | ARCH_11 §3.1/§3.2；ARCH_21 §7.1 | 解析已实现 |
| B9 | **shader/接口清单已提取** | 4480 programs（file/offset/stage/size/names）、22212 接口行、142 GXP 文件→hash/家族/指令数/sampler/KILL、142→115 去重、111 按 ISA | shader_index.tsv / shader_interfaces.tsv / _SHADER_CLASSIFY.txt / _SHADER_DEDUP.txt / _GXP_NAMEMAP.txt | 离线可查 |
| B10 | **旧 Vulkan 后端可整体复用** | `src/gxm/vulkan_backend.cpp`（399 KB，真管线 + FUBO 修复 + 绿块链路测试）已在仓库中 forked 为 `engine/gxm/vulkan_backend.cpp`（409 KB）；旧运行时共 167 个 SceGxm API 绑定 | ARCH_66；仓库 engine/gxm | 链接未接 |
| B11 | **GXM 被丢弃的状态（带调用计数）** | no-op：SetCullMode 10262、SetFrontDepthFunc 18428、SetFrontDepthWriteEnable 18454、SetForceStoreMode 4084、min/mag filter 8229、U/VAddrMode 285、mipFilter 88、lodBias 1；`TextureGetMipmapCount` 硬编码 1（279）；blend 信息在 CreateFragmentProgram 时已捕获 | ARCH_10 §2.1/§2.7 | ❌ |
| B12 | **黑屏根因已解（历史）** | ① `PX32UNPACK` 把 float 当 U32（960.0f→NDC 2392063，裁剪成一角）；② `vert_0015` 双重投影（自带 screen/SA + PX2NDC）；③ 片元 UBO 选错（frag_0084 拿到 reserve 块 (0,0,0,1)）→ 改用 `g_last_su_ub` 后不再黑（M61/M62 §18.3602/§18.3603） | ARCH_4 §5.6；MASTER §18.3602-03 | 历史已修 |
| B13 | **已知未解决渲染 bug** | RT 采样成 1024×544 vs pass 目标 960×544（get_rt 比较 w/h 不比 stride → 6.67% 横向误差 + 64 列未初始化）；`IDXFIX` 只对 cnt∈[3,8] 生成 identity indices | ARCH_4 §5.4.3 T2；ARCH_4 §5.5 | 未解决 |
| B14 | **Vulkan 后端架构与语义文档** | `_archive/docs/vulkan_backend_architecture.md`、`RENDER_SEMANTICS.md`（SSAA 3×、px+0.5、bbox-area test、背景 16/16/16、行向量矩阵、相机 +Z） | _archive/docs | 规格 |
| B15 | **4331 个预翻译 SPIR-V 可用** | `C:\dev\p5d_build\spv_all\*.spv`（gxp_frag_0000.. 等） | 目录实测 | 可直接用 |

---

## C. 场景 / 状态机 / 序列

| # | 可复用知识 | 关键细节 | 证据 |
|---|---|---|---|
| C1 | **app-state 表（修正版）** | 见 A27；state 5=mainMenu（update `0x810E8678`）、state 6=titleSequence（update `0x8119807C`） | ARCH_16 §4.2 |
| C2 | **titleSequence 9 状态机（修正 TBH 目标）** | step `0x81198172`、tbh `0x81198184`；正确目标 `0x8119819E/E8/FA/218/228/23A/248/26E/A8`（ARCH_6 的表整体偏 4）；state4=进影片（`0x811580CC(1)` + `0x811581DC` 设 \|0x40000）；state5=等 `find('movie play')`；state6=建异步任务 `0x81193A00`；state7=轮询 `async[0]&2` | ARCH_23 §2.1–2.3 |
| C3 | **S0_LOGO 不是场景模块** | 是 3 张 TMX 图集（ATLUS / P-STUDIO / CRIWARE），gate `0x81163170`→`0x81163FA4`→`0x811CA9F0` 测 `[obj+0x28]&0x80000000`；LOGO-READY h1164 F2595、LOGO-END F2838/F2967；`[TMXDONE #1..3]`；播 `movie/c_p5d000.usm`（与 p5d001 存疑） | ARCH_23 §7.1；ARCH_1 §4.8 |
| C4 | **S0→S1 路径** | state table 1(initDataRead) → state 6 TITLE → titleSequence 0..7；state4/5 在 `find('movie play')` 前等待 | ARCH_23 §2.2/§7.1 |
| C5 | **主菜单背景 P5（最便宜可见画面）** | 9 函数：create `0x810F6898`、update `0x810F6DDC`、draw `0x810F6C8E`=`FUN_81160B7E(0,0xC40000FF,x,y,w,h,0)`；draw-task id 10；ready `*obj>1`；name `"main menu bg p5"` prio 200 ⇒ S2 第一帧就是一个暗红全屏矩形 | MASTER §4F.11 |
| C6 | **Draw-layer/OT 表 + layer id** | 表 `[0x81555420]`，0x1C B/项，每帧由 `mainDraw 0x81185308` 重建；5 层/frame id 0x0B/0x0E/0x19/0x19/0x43；模块 id 10=mainmenu bg、0x1A=titleDraw、0x20/0x41=init、0x39=helpwindow、0x43=debug；`[0x81555438] bit31`=已重建；flush `0x8116046E(0..6)` | ARCH_22 §2.6；MASTER §4F.12 |
| C7 | **命名序列注册表 + hash 公式** | `h=Σ(int8)name[i]<<(i&7)`；145 个名字；`title sequence 0x7935@0x8130B140`(create 0x811980F4)、`title 0x0CC6@0x8130ACB4`(0x81193A62)、`titleDraw 0x62BD@0x8130AD68`、`logoTMX 0x2B3E@0x8130ABFC`、`logoDrawProc 0x72A6@0x8130AC40`、`movie play 0x640E@0x81306538`、`mainmenu bg 0x7233@0x812FF4A4`、`MusicSelectMain 0x9A2E@0x81305094`、`DanceSystem 0x7138@0x812F1884`、`result 0x1BF0@0x812F1FE0`、`RESULT_LINE 0x53C9@0x812F1D18`、`mns 0x0315@0x81303404` | ARCH_30 §3.1/§6.1 |
| C8 | **场景图 = 3 条全局链 + 运行时 vtable** | 列表 idx `0x816CC630`；头 `0x816CD468`(content)、`0x816CE50C`(overlay)、`0x816CD498`(3D/post)；tick `0x811CBE9A`；content pass `0x811E8AAE`；sprite 快路径 `0x811EBB94` 由 `gfx+0x919` 门控；node 0x50B（+0 vtIdx、+0x0C next、+0x20 type 4/5/-1）；16 list slot、0x30B/registry | ARCH_21 §2/§4 |
| C9 | **模块 VA 区间（排探针用）** | title 0x81193356–0x811980FE；mainmenu 0x810E6324–0x810F839E；music_select 0x81133190–0x81150D5A（52 状态 TBH `0x81147A40`）；mns 0x811199F6–0x81133190；dance 0x810394DE–0x81051xxx；dance_ui/result 0x8105119A–0x81057FCC；movie 0x81153A18–0x811593F8；appGraphics 0x81003E7E–0x810158D8 | ARCH_25 §2.2 |
| C10 | **M1–M18 已验证的场景级 UI 规格** | S4 对局 HUD/判定/音符用真实图集与布局（dance.spr6 + dansing_p5.lbn + dance_ui_p5.bin）；MNS 音符语义（data&0xFF=id、bits8..15=hold 长度）；两段判定（LONG/HOLD）与原版数值对齐（song 208 EASY 176 notes=158+18）；判定窗口表 `_JUDGE_WINDOW.txt`（base 0x813A8014/18，4×4 级，单位 1/100 beat）；music 表 101×56B（[2]/[3]=长度、[5]=BPM） | MASTER §18.3544-3558；_JUDGE_WINDOW.txt；§18.16/§18.17 |

---

## D. UI / 资产 / 文本 / 输入 / 存档

| # | 可复用知识 | 关键细节 | 证据 |
|---|---|---|---|
| D1 | **SPR6 图集格式 + 53 个 UI 容器** | `SPR6`；+0x08 size、+0x10 sprCount、+0x14 tex table 0x20、+0x18 0x4C、+0x70 8B record offsets、+0x188 sprite records（Shift-JIS 名）；容器：dance_ui_p5 18 tex/144 pan、mainmenu_p5 6/45、result_ui_p5 2/45、c_backlog_p5 1/7 | ARCH_22 §4.7；_UI_SCREENS.txt |
| D2 | **菜单 panel 用纹理坐标而非 960×544** | panel 36B/个；mainmenu 45 panels @0x390、sprites 96B @0x150；例 (1,1,249,318)，max x+w=513、y+h=1011 vs atlas 512×1024；dance max 480/483 vs 1024×1024 | _SPR6_COORDSPACE.txt；_MAINMENU_PANELS.txt；_SPR6_PANELS.txt |
| D3 | **UI 元素表（按屏）** | mainmenu 28、dance 50、result 27、music_select 3 条 x/y/w/h | _UI_ELEMENTS.txt |
| D4 | **输入真实字段（不要读错）** | 用 `appPad_8116ef02`(+0x18 边沿) 与 `appPad_8116ef98`(+0x1c 持续)；`appPad_8116f02e` 读 +0x24 每帧被清⇒恒 0 | HANDOFF §0.0c；CONTROL_SEMANTICS.md |
| D5 | **PAD_SCRIPT / PAD_AUTOTAP / PADSHOT 工具语义** | `P5D_PAD_SCRIPT="4000:CROSS,4150:CROSS,4300:DOWN"` 帧驱动注入；PADSHOT 按键时刻即时抓帧；AUTOTAP 每 3 游戏秒轮换（会扰动状态机，谨慎） | eboot_probe.cpp:1879-1925；MASTER §18.3618-3621 |
| D6 | **金标准帧** | `docs/_evidence_title.bmp`、`docs/_evidence_mainmenu.bmp`：**960×544 24bpp BMP（1,566,774 B）**，逐像素 diff 目标；另有 `p5d_build\host_p5menu.png` 等 20+ 张历史渲染证据 | 实测文件头 |
| D7 | **存档/双释放断言背景** | 存档写入支持已实现（M83/M84），但 `appGxModel.c:809` 对象双重释放断言仍是 S3 前后崩点（0x8115F9D6）；正确路径=修对象生命周期，不是跳过断言 | MASTER §18.3622-3624 |
| D8 | **消息/对话数据** | 5548 条 MSG、3329 MSG ID、226 个 .bf；`_bf_dialogue.tsv`、`_MSG_TEXT_INDEX.txt`；.bf opcode 36 fn ptr @0x813AFEA4 + handler 表 | _MSG_ID_TABLE.txt；_BF_OPCODES.txt；_esm_handler_table.tsv |
| D9 | **资源路径映射** | `code_resource_map.tsv`：字符串 VA→资源路径（107 literal + 60 %-template），如 0x812F1258→`dance/table/stage/stage_format_%03d.bin` | code_resource_map.tsv |
| D10 | **CPK/ARC 清单（离线即可查）** | `data_cpk_filelist.txt`（10,559 条 + 偏移/大小）、`arc_entries.tsv`（135 容器 / 5176+495 条目）；`FIRE1_0.ep`@0x78000 extract 0xd4 | 两个 tsv |
| D11 | **字体** | FNT0/FCT0 Huffman；cmap 用**高字节**（11/11 验证）；旧运行时 `appFontUnitList.c:381` 断言、glyph 裁剪 N1b 是历史坑 | ARCH_201-211/257-258；ARCH_23 §7.1 |
| D12 | **`_STUB_REPLACE.txt`：672 个桩分 A/B/C 类** | A 类 406 个（全部 0x812Bxxxx=CRIWARE）应映射原生 CRI 库；B/C 类 266 个**从未被调用**可安全 noop——避免盲目实现 | _STUB_LIST_672.txt / _STUB_REPLACE.txt |

---

## E. 音频 / 视频 / 输入 / 存档 / 系统

> 本节 P0 项来自子系统专项挖掘：其中多数是 **`initDataRead` 的硬门**或「便宜但致命」的语义；
> 旧运行时大多已实现，当前静态 harness 缺失。注意：这些是**当前 boot/frame 空转之后的下一个层次**，
> 不是 M722-M725 卡点的直接解法，但一旦帧/任务流恢复，它们就是到达 LOGO/TITLE 的必经之门。

### E0. 启动硬门（到 LOGO/TITLE 前必须通过）

| # | 知识 | 具体细节 | 证据 | 状态 |
|---|---|---|---|---|
| E0-1 | **奖杯 SetupDialog 状态必须返回 2 (FINISHED)** | NID 0xC3A59547 / stub `0x812B1B40` → **2**；Init `0x9E2C02C9` / GetResult `0xE37069D5` / Term `0xA81082DD` → 0；netTrophyProc state3 `cmp r0,#2` → state4（6 份旧日志一致）。**initDataRead step0 硬门**，返回 0 会冻结 boot 在 title 之前 | ARCH_9 §4.1；ARCH_13 §5.2；eboot_probe.cpp:442-446 | ❌ NEW 无 case |
| E0-2 | **_FDclass 用 ARM RTABI 码（不是 glibc）** | NID 0xBD8EF217 / stub `0x812B2670`：**1=NaN、2=INF、4=ZERO、8=subnormal、16=normal**。`appSound.c:1003 assert(_FDclass(x)!=FP_INFINITE)`，对 +0.0f 应返回 **4**。旧文档「返回 2」是 glibc 误读。**initDataRead step1 硬门**：错误码 ⇒ 5 个 appSound player 全 assert（0x81306138/3C/48/6C/A4；init 0x8115478E/ready 0x8115487A/update 0x81171476/step 0x811848B7）⇒ 没有 title/LOGO | P5D_MASTER 161075-161099；ARCH_17 §4.4-4.5；ARCH_26 §4.4 | ❌ NEW 缺失 |
| E0-3 | **AppMgr 事件语义** | ReceiveSystemEvent `0x10B5765F/0x812B11A0`：无事件 → **EAGAIN 0x80010001**；GetAppState `0x5E86319A/0x812B11B0` → `memset(out,0,r1)`（caller 把 *out 当 pending 事件数）。当前 harness 返回 0（=有事件）+写 1（=1 个 pending）⇒ 事件循环语义错误；旧 0x1388 残留曾导致 1.27 亿次自旋 | ARCH_9 §2.1；HLE_DIFF §2.7 | ❌ NEW 错误 |
| E0-4 | **AppUtil DrmOpen / SystemParamGetInt** | DrmOpen `0x2DB7BE3B/0x812B18A0` 必须返回 **ENOENT 0x80010002**（59 次 "ADDCONT%s" 循环 0x81058A80；返回 0 会把全部 DLC 当存在 → cbA→state6 重试）；DrmClose=0 正确。SystemParamGetInt `0x5DFB9CA0/0x812B18B0` 必须写日期（paramId=1，调用点 0x811845FC；日期门 0x81670354/58 共 178 引用，不写=栈垃圾） | ARCH_9 §2.3；HLE_DIFF §2.7 | ❌ NEW 无 case |
| E0-5 | **NGS out-param：绝不返回 0 而不写 out** | RackGetVoiceHandle `0xFE1A98E9/0x812B2110` 必须写**非零** voice handle（57 次；caller 立刻 VoicePlay(*out) @0x8125C640）；RackInit 0x0A92E4EC；PatchGetInfo 0x98703DBC；VoiceGetInfo `0x5551410D/0x812B1FB0` 必须 zero/fill（caller 位测 *out&0xF/0x10）；VoiceGetStateData `0xC9B8C0B4` out=0x18B；两个 NeedSize `0x6CE8B36F/0x477318C0` 必须写 size。同类错误已造成 LockParams BADMEM | ARCH_12 §3.6/§5.5/§6.4 | ❌ NEW none |
| E0-6 | **AudioOutOutput 必须让出（哪怕静音）** | NID 0x02DB3F5F / `0x812B1EB0`，旧日志 36,441 次（~11.9/帧）。立即返回会让音频线程自旋（旧运行时 93% SVC、sched 18,271）。不需要真 PCM，但必须 yield。音频泵 0x8125BC8C（thid 0x600E）：4KB buf、OpenPort(type0,len=*(0x8153FA00),48000,mode1)、loop SystemUpdate→GetStateData→SetVolume→Output | ARCH_12 §5.2；eboot_probe.cpp:7463-7470；HLE_DIFF §2.10 | ❌ NEW 无 case |

### E1. 视频（S0 logo 不是视频；首个 USM 是 title stage4）

| # | 知识 | 具体细节 | 证据 |
|---|---|---|---|
| E1-1 | **术语纠正** | 「S0_LOGO」= titleSequence **stage 2-3 的 LOGO 三图集**，由 `logo_81193344` / `FUN_811933B8` 画 LOGO/PRESS/copyright sprites（`title/c_title_p5.bin`），**不是 USM**；首个 USM 是 stage4 `appMovie_811580CC(1)` → `movie/c_p5d000.usm` | ARCH_23 §7.1；ARCH_5 §2.1；P5D_MASTER 2782-2922 |
| E1-2 | **CDRAM memblock 必须 1 MiB 对齐** | `type>>24 ∈ {0x09,0x0D}` → align 0x100000，其它 0x10000；实例 `type=0x0D808060 size=0xC00000`。失败链 VavcPsp2 0x812A9210/0x812A92B8 → E12041901M @0x812A9140 → "Can't create Video decoder." 0x812A3C40 → criMvPly 分配失败 → 黑 title | eboot_probe.cpp:5994-6022；HLE_DIFF §2.10 |
| E1-3 | **DecodeAvailableSize = w·h·3/2** | NID 0x441673E3 / `0x812B2140` → 960×544 时 783360 (0xBF400)；caller 0x812AA180 与 ES size 比较。返回 0 ⇒「0 bytes available」⇒ 解码循环不进；0x800000 又大 10.7×。QueryMem 0x97E95EDB = ((w·h·3/2)·ref) align 0x40000（ref=3→0x200000） | eboot_probe.cpp:8178-8193；ARCH_5 §5.1 |
| E1-4 | **SceVideodecUser 9-NID + 结构体布局** | InitLib 0xF1AF65A3；Create 0xE82BB69B（handle 写 ctrl+0）；Decode 0xD6190A06(ctrl, Au{pts u64@0,dts@8,es@0x10,size@0x14}, ArrayPicture{numOut@0,numElm@4,pPic@8})；Picture size@0/pitch@8/W@0xC/H@0x10/pPicture[0]@0x3C/[1]@0x40/horiz@0x14/vert@0x18/info numUnitsInTick@0x44/timeScale@0x48/fixed@0x4C/pts@0x5C/0x60/pixelType 0x20=NV12 packed；Flush/Stop/Delete 有 `is_stopped` 生命周期 | eboot_probe.cpp:7853-8249；ARCH_5 §2.5/§3.7 |
| E1-5 | **CRI timeScale=90000 / numUnitsInTick=1** | AU PTS 每帧 +3003（0xBBB,0x1776…）⇒ 3003/90000 = 1/29.97s。若写 30 ⇒ 100.1 s/帧 ⇒ beat 回调 0x8129C6D8 只过 1/1600 ⇒ player+0x50C 永不为 1 ⇒ movie quad 永不提交。**解码成功 ≠ 可见**，这是差别所在 | eboot_probe.cpp:8088-8116；ARCH_5 §2.4 |
| E1-6 | **USM 事实（不要写 demuxer）** | guest 静态链接 CRI（无 CriMana/SceCri 导入）⇒ 只需 SceVideodecUser + timing。c_p5d000.usm：960×544/29.97fps/3084 帧/102.9s/54.4MB（OP，title stage4）；c_p5d001.usm：900 帧/30s/8.0MB（title bg，无 @SFA）。c_p5d001 容器：CRID + 3 @UTF（DIR_STREAM 2 行 / VIDEO_HDRINFO / STREAMINFO 30 行）+ 41 @SFV，无 CRILAYLA，H.264 | ARCH_5 §4.3；P5D_MASTER 21195-21235 |

### E2. 音频 / 音乐

| # | 知识 | 具体细节 | 证据 |
|---|---|---|---|
| E2-1 | **判定时钟 = appSound playingTime，不是 BeatSync** | `0x810456BC` 返回 `[danceMusic+0x1C]`；producer 0x8104566C → 0x811717D6(appSound.c:856) → 0x81172DF0 → **0x81187620** 查全局 **0x8167B48**（记录 0x60B：+0 active/+8 key/+0xC key2）→ 秒；负值夹 0（"invalid playingTime!"）。返回 0 ⇒ 所有 note 钉在 t=0。（writer 未知） | ARCH_26 §3.3/§8.3；P5D_MASTER 293632-293654 |
| E2-2 | **NGS 播放位置（引擎音乐时钟）** | 0x81275000：GetInfo bit-test；GetStateData(voice,0,buf,0x18) → **buf+8=samples played**；`(samples/2 × clock[+8]) ÷ 48000` 累加 64 位到 voice+0x28；handle 在 obj+0x38，state +0xD/+0x11，reset +0x80；48000 也在 0x8125C2A8/0x8125BCC2。仅 NGS 同步的 dance/BGM 需要 | ARCH_12 §3.4 |
| E2-3 | **音乐资产** | 28 个 AWB/ATRAC9 + 已解 28 个 WAV（48kHz/2ch/16bit，~1.1GB）；`sound/music/music%03d.acb/.awb`（201-225、301-303）；`sound/dance/dcommon.acb/.awb`；`bgm/p5d_bgm.awb` 91.5MB；`preview.awb` 61.6MB；AWB=AFS2+RIFF/WAVE+ATRAC9 192kbps；旧 Android 用 AAudio 播过。[U] AFS2 index/GUID 未解 | ARCH_27 §2；ARCH_17 §5.5；ARCH_392 §2 |
| E2-4 | **NGS 是 M48 阻塞点** | `VoiceLockParams reqSize=0x1015CEC` 异常 → worker BADMEM；reqSize 序列新旧一致，AOT 特异性来自解释器内 NGS-loop-fix hook 被绕过（修法：含 hook pc 的块强制走解释器）。NGS reqSize 不是 size，guest 长度链把代码地址 0x8123DF61 当长度 | MASTER §18.3589-18.3592 |

### E3. 输入

| # | 知识 | 具体细节 | 证据 |
|---|---|---|---|
| E3-1 | **SceCtrl 契约 + 引擎全局** | SceCtrlData 0x20B：ts u64@0、buttons u32@8、lx@0xC/ly@0xD/rx@0xE/ry@0xF；peek 返回 0；engine 只读 +8/+0xC..+0xF。按键位：SELECT 1, L3 2, R3 4, START 8, UP 0x10, RIGHT 0x20, DOWN 0x40, LEFT 0x80, L2 0x100, R2 0x200, L1 0x400, R1 0x800, TRI 0x1000, CIRC 0x2000, CROSS 0x4000, SQUARE 0x8000。engine globals：prev 0x816CDFB0+4p、new +2、down +4、lx 0x816CDAAC+2p、rx 0x816CDAA8；repeat timer 0x81680148+4p（0.4667s→0.1s）；2 ports、2 peeks/帧 | ARCH_18 §2.2-2.3/§4.2 |
| E3-2 | **appTouch 生产链** | appTouch 0x8117339C-0x81174AB2；token 表 100×36B、2 appLists、singleton 0x813AFCDC；设备 0x81174624-0x81174A50（arrayA{base@0,u16 n@4,8B float 坐标[U]}、arrayB{u16 n@6,20B 记录}）；生产 `FUN_81174CD4`(call 0x81174CE6) → thunk 0x81003B72 → FUN_81003CCC → enqueue 0x8101603C；job fn 0x81003D89 开头 sceTouchRead。空 `[0x81539754]+0xC` 是症状不是根因 | P5D_MASTER 2U.13-2U.16；§18.4268 |
| E3-3 | **PAD_SCRIPT / PADSHOT 语义** | `P5D_PAD_SCRIPT="4000:CROSS,4150:CROSS,4300:DOWN"` 帧驱动注入；PADSHOT 按键时刻立即抓帧；AUTOTAP 每 3 游戏秒轮换（会扰动状态机，慎用） | eboot_probe.cpp:1879-1925；§18.3618-3621 |

### E4. 存档 / DLC / 网络 / 对话框

| # | 知识 | 具体细节 | 证据 |
|---|---|---|---|
| E4-1 | **存档只有一个文件** | 只 `savedata0:system.bin`；host `savedata0:` → `<root>/savedata/`；SlotGetParam `0x93F0D89F/0x812B18F0` 必须填 SceAppUtilSaveDataSlotParam（0x34C）：status@0、title@4（32×u16）、subTitle@0x44、detail@0xC4、iconPath@0x2C4、userParam@0x304、sizeKB@0x308、modifiedTime@0x30C；DataSave `0x607647BA`(slot,files,fileNum,mountPoint,requiredSizeKB*) 第 5 参是 out；SlotSetParam `0x98630136`。写模式：绝不把 `flags&3==1` 当写（SCE_O_RDONLY=1），只有 savedata0:+WRONLY/RDWR 才写（M85 曾把 2GB vita.cpk 截成 0） | ARCH_9 §2.3/§3/§4.2；HANDOFF |
| E4-2 | **网络：init 安全，输出是陷阱** | netInit 0x81058CA0（SysmoduleLoad(1,4,0x15)、NetInit、NetCtlInit、SslInit(0x4B000)、HttpInit(0x14000)、NpInit、NpRegisterServiceStateCallback 0x810591F1）全部 ≥0 即可；但 GetNpId 0x3C94B4B4（10 站点）、GetContentRatingFlag 0xAF0073B2、GetChatRestrictionFlag 0x60C575B1 必须写 out，否则 0x0A/0x64 分支读栈垃圾；NpCheckCallback 0x3B0AE9A9 保持 0；NetCheckDialogGetStatus 0x8027292A → 2。**不要做真网络** | ARCH_13 §4.1/§4.2/§5.3/§7-Q6 |
| E4-3 | **CommonDialog：14/18 从未被调用** | 只有 4 个 sceNpTrophySetupDialog* 被调用；sceSaveDataDialog*×4、sceMsgDialog*×3、sceNetCheckDialog*×4、CommonDialogUpdate 0x90530F2F 在所有日志中 0 次。不用投资 | ARCH_9 §2.4/§6.1；ARCH_13 §7-Q3 |
| E4-4 | **Sysmodule/Power/GameUpdate 已正确** | LoadModule 0x79A0160A/0x812B1860 与 IsLoaded 0x53099B7A/0x812B1E90 → 0 正确有效；模块 ID 0x0B NGS、4 HTTPS、0x15 NP、0x1E NP_SCORE、0x24 NP_ACTIVITY、0x25 NP_TROPHY、0x2C NP_TUS（另有未初始化 0x813B1F04 带来的 0x5555 假 ID）；Power 0x3CE187B6 + GameUpdate×3 从未调用 | ARCH_9 §2.2/§2.5-2.6；ARCH_13 §4.1 |

### E5. 本节负结果（不要重复）

1. `_FDclass`「必须返回 2」是 glibc 误读；RTABI 码才是对的（+0.0f→4）。
2. `SceAvcdecPicture.info.pts`：写入回读正常（[PTSWR]），但 CRI `rec+0x28` 仍 0 ⇒ CRI 不从 info.pts 取 PTS，不要追。
3. 宿主假解码/假影片层（P5D_MOVIE_FS/PRELOAD/TEXSRC、P5D_FORCE_MOVEND/…）是已验证的绕过手段，但**不符合「原版代码自己跑」的目标**。
4. 不要为判定实现 BeatSync 回调（判定循环不读它）。
5. 不要写 USM demuxer / CriMana 替代（CRI 静态链接在 eboot 内）。
6. 不要用公平/yield 注入修 lwmutex 自旋（0x811E68FE）：死锁仍在且慢 2.4×。
7. 不要盲目 memset 0x140 清触摸报告（只清 16+min(nBuf,8)·0x20；越界会清掉 appJobQueue slot → appJobQueue.c:194 断言）。
8. `0x812B1810` 身份有冲突 [U]：ARCH_19 说是 ARM veneer（音频线程）；导入表/HLE_DIFF 绑定 0x05F27764 `sceDisplayWaitVblankStartMultiCB`——依赖前先核实。


## F. AOT / 语义 / 性能（旧分支已验证的坑）

| # | 知识 | 细节 | 证据 |
|---|---|---|---|
| F1 | **已修复的关键 AOT bug 列表** | LDM/STM 基址覆盖（M33）、push/pop 寄存器顺序（M101）、pop-pc terminator 映射（M118）、2 操作数高寄存器 add/sub 丢弃（M119）、MOVW 高 16 位未清（M122）、shift if/elif 链断裂（M126）、块末尾非终结指令被丢（M137）、单指令 add/sub-sp 块（M133b）、嵌套 call_guest stale sync_to_fallback 覆盖实时状态（M46）、条件分支/IT/quantum/forced_npc 语义（M24-M28）| §18.3573/3642/3659/3660/3663/3667/3678/3674/3587/3568 |
| F2 | **差分验证器可直接用** | `tools/aot_difftest.py`（golden cases、production blocks、unicorn oracle）；AOT 块级验证器 2552/2552 全绿（M28） | tests/aot_difftest_* ; §18.3569 |
| F3 | **性能：run(N) 批量 fallback** | 旧 M36：120s 执行 1.5–1.7 亿 fallback 指令（15–50× 提速）；解释器逐条固定开销是主瓶颈 | §18.3577 |
| F4 | **静态重编译生成器现状** | 26 批 × 500 函数、601,976 行 C++、obj 42.4MB、并行 4 编译 262s；全 CFG 87,871 块 / 924,838 指令 / 10,591 间接跳转；Ghidra C 参考 `C:\dev\p5d_build\true_seg0_final.c`（12,917 函数/669,563 行，函数名 <源文件>_<VA>，含 `//! src:` 注释——查函数实现比汇编快几十倍） | §18.3628-3634；§18.3629 A |
| F5 | **VFP/NEON 语义** | 27 个 VFP 数学 wrapper 已在解释器实现，可移植到时生成器；NEON 仍是最大缺口 | §18.3629 E；M92 |
| F6 | **真实设备 dump 校验工具** | `tools/realdev.py` 按双基址（code/data）校准读 vitacheat dump；设备 eboot 与 PC eboot 有 4.916% 字节差异，地址不可跨镜像直接信 | START_HERE §5.5b |

---

## G. 明确不要重复的坑（负结果清单）

1. **0x1B 队列 / 线程 0x6009 是红鲱鱼**：三个 job body 都解析到 `0x811Dxxxx/0x811Bxxxx` 通用 I/O helper，与 movie/GXM 无关（MASTER h960）。
2. **修 eventflag 唤醒不是解**：`[EVT] WAKE 0→56` 而验收指标不动（h843）——瓶颈在供给不在唤醒。
3. **「跳过断言继续跑」被证伪**：只会把局部断言变成 NULL 传播（0x8117BB42→0x8117BB80→BADMEM 0x94）。
4. **「在 HLE 里加资源解析」是伪需求**：游戏只开 data.cpk/vita.cpk，解包是 guest 侧 CriFs。
5. **扫描/扰动 HLE 返回值是死路**：40 候选 sweep 结果 BEYOND=0。
6. **真实阻塞 LwMutex 更糟**：P5D_LWMX_BLOCK=1 更早崩（267.8M insn）；FAIR 不修死锁且 2.4× 慢。
7. **GetThreadId 单独返回 0x4001 会 BADMEM 0x4003**（guest 把它当指针基址），需要 id/TLS 联合设计。
8. **周期性 pump fiber2 会改变游戏行为**（驱动到 0x81555E98 main-return）。
9. **每帧/固定堆地址 watchpoint 不可靠**（堆布局每次变），要从结构入口当前地址或代码点布防。
10. **文档/资产陷阱**：`_APPSTATES.txt` 基址错（见 A27）；titleSequence TBH（ARCH_6）偏 4；SEG1 单线性公式偏 0x3800；BeginScene 日志标签 color0/color1 实为 RT/ValidRegion；`vita.cpk files=278` 是旧值（现 531）；`progress.md` 过期且编码损坏；`_UI_LAYOUT.txt` 是负结果。

---

## H. 建议的下一步（按 ROI 排序）

1. **复活旧运行时作为金标准（S，1–2 小时）**：`eboot_probe_aot.exe` 已可启动并挂载 data.cpk；用 `P5D_TIMESCALE=8` 长跑复现 TITLE/MENU/SELECT，用现成截图工具重抓金标准帧（旧证据：`p5d_build\host_p5*.png`、`docs\_evidence_*.bmp`）。它同时是静态 harness 的 HLE 行为 oracle。
2. **把旧 HLE 子系统移植进静态 harness（L，最高长期收益）**：
   a. display queue 回调→SetFrameBuf（A1，S）；
   b. LwMutex 语义（A2，S）+ 缺失 stub（A3，S×N）+ GetMemBlockBase 4B（A20）+ AppMgr 返回码（A21）；
   c. 把 `engine/gxm/vulkan_backend.cpp` 接到静态 harness 的 GXM 调用面（B10，L）。
3. **启动硬门补齐（S，到达 LOGO/TITLE 的必经门）**：E0-1 奖杯状态→2、E0-2 _FDclass RTABI、E0-3 AppMgr EAGAIN/清零、
   E0-4 DrmOpen ENOENT、E0-6 AudioOutOutput 让出。这些是 `initDataRead` step0/step1 的硬门（旧运行时已实现、我们缺）。
4. **首张非黑帧（M，验证闭环）**：用 B1 的最小 12-program + B2 RT 几何 + B3 帧边界，配合 D6 金标准 BMP 做像素 diff。
5. **场景链（M）**：A27 app-state 表 + C2 titleSequence + C3 S0 logo gate 探针 + C5 主菜单背景，先拿到 S0→S1→S2 三段可见帧。
6. **输入/存档（M）**：移植 D4/D5 的 PAD_SCRIPT/PADSHOT 与存档写入（D7），推进 S2→S3→S4。
7. **音频/视频（L）**：E1/E2/E3/E4，视频先假解码、音频先让出，最后再上真解码。

---

## I. 许可证

本项目采用 **MIT**（见 [LICENSE](../LICENSE)）：源代码、AOT 生成器、工具与研究文档均为 MIT；
第三方依赖（Capstone BSD-3-Clause、Vulkan-Headers Apache-2.0、MinGW/GCC runtime）见 [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)；
游戏本体/素材版权归 ATLUS / SEGA，不在 MIT 范围，且本仓库不包含任何游戏数据。

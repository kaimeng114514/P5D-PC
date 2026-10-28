# p5r-android 参考研究（Persona 5 Royal Nintendo Switch → Android 原生移植）

> 参考仓库：https://github.com/Brandon04Gonzalez/p5r-android
> 上游运行时：SwitchGamesRecomp / Super Mario Bros. Wonder Recomp（BelmanteGu，MIT）
> 许可证：MIT。仅用于方法学习，不复制游戏代码/资源/密钥。
> 本文件由 P5D 移植项目在 2026-10-28 整理，用于对标我们的 AOT + HLE + GXM→Vulkan 路线。

## 0. 一句话结论

这个仓库和我们的目标架构几乎同构，只是平台从 **PSV(ARMv7/Thumb, GXM)** 换成 **Switch(ARM64, NVN)**，
游戏从 **P5D** 换成 **P5R**（同一个 Atlus GFD 引擎族）：

- CPU：**静态重编译**（AArch64→C++，Python 生成器）——用于 PC 调试版；
- 系统：**HLE**（Horizon 内核 + nn::* 服务按 **名字** 替换）；
- GPU：**在图形 API 层**（NVN）实现到 Vulkan，而不是模拟 GPU 命令流；
- 着色器：**全部 AOT 预翻译为 SPIR-V**（Ryujinx 的 Maxwell 翻译器离线跑），运行时只查表；
- 手机版最终走 **NCE**（原版 ARM64 代码直接在手机 CPU 上执行 + 指令补丁），以规避分发重编译代码的版权问题。

## 1. 仓库结构（关键目录）

| 路径 | 内容 |
|---|---|
| `recomp/` | AArch64→C++ 静态重编译器：`nso.py`(NSO/段/重定位)、`ehframe.py`(函数边界)、`analysis.py`(种子/跳表/PLT)、`emit*.py`(标量/FP/NEON/VLDST 生成)、`__main__.py`(驱动) |
| `runtime/` | 共享运行时：`nce/`(原生执行)、`nvn/`(NVN→Vulkan)、`hle_*.cpp`(fs/audio/hid/movie/net/pl/vi)、`kernel.cpp`(Horizon 内核语义)、`ipc.cpp`(HIPC/CMIF)、`platform_*.cpp`(Win/Linux/Mac/Android)、`vk_gen.cpp`(生成的 Vulkan 引导)、`vk_present.cpp`/`vk_swapchain.cpp` |
| `games/p5r/` | P5R 专用：`game.cpp`(引擎钩子/画质/帧率)、`import/`(资产导入：CPK/GSC 着色器提取与打包)、`tools/`(`cpk.py`、`gsc_shaders.py`、`prepare_data.ps1` 等) |
| `tools/maxwell/` | Maxwell 着色器工具：BNSH/FSHA/SARC/BAHS 容器解析、BNTX 纹理、`spirv_harness/`(调 Ryujinx 翻译器) |
| `docs/` | `RECOMP.md`、`RUNTIME.md`、`NVN.md`、`FRAMEWORK.md`、`nvn/SHADERS.md`、`nvn/TRANSLATOR.md`、`nvn/DEBUG*.md`、`nvn/ANDROID-PORT.md` 等 |

## 2. CPU：静态重编译器（`docs/RECOMP.md`）

- 入口：`python -m recomp <module.nso> <out dir>`；Hades 主模块 1.61M 指令约 1 分钟生成 C++。
- 加载：NSO 段（LZ4）、MOD0、`.dynamic`、`.dynsym`、RELA/JMPREL、init/fini。
- 函数边界：`.eh_frame_hdr` 的 FDE → 10,824 个函数，覆盖 96.3% text、无重叠。
- 分析：种子（FDE、BL 目标、指向 text 的 RELATIVE 重定位、init_array、导出、ADR 代码指针）、PLT→导入名、跳转表（两种形态：clang 压缩表、table-relative ldrsw），不动点把越界分支目标提升为函数入口。
- 代码生成：每个 guest 函数一个 C++ 函数；函数内分支→`goto`，函数外→尾调用，`BL` 直接调用，`BLR`/未知 `BR`→`nx::call` 查函数表；跳转表→`switch`。
- Guest 内存即宿主内存：模块映射在固定基址 `NX_BASE`（默认 0x40_0000_0000），ADR/ADRP/字面量直接生成为 `NX_BASE+offset`，两端小端所以无需字节交换。
- 上下文 `nx::CPU`：x0–x30+SP、v0–v31、NZCV、FPCR/FPSR、TPIDR、独占监视器。
- 独占：`LDXR/LDAXR` 记录(地址,值)，`STXR` 做比较交换；`LDAR/STLR` acquire/release；向量写标量清高位。
- 浮点：宿主 SSE 默认与 FPCR 默认一致；整数转换按 AArch64 饱和；`FMAXNM/FMINNM` 按 IEEE maxNum；`FMADD` 用 `std::fma`。
- **差分测试（非常值得学习）**：`tests/difftest.py` 按“指令形态”（助记符×操作数种类×排列×索引/移位/扩展×寻址模式）抽样，随机状态分别跑原生代码和 **Unicorn**，比较 x0–x30/SP/v0–v31/NZCV/内存窗口。主模块 950 形态、1800 测试、43,200 次运行、**0 不一致**；并抓出过：
  - post-index 结构化 ld/st（Capstone 丢立即数）→ 从传输大小推导；
  - `FMAXNM/FMINNM` 的 sNaN；
  - 64 位排列的 `BIC/ORR` 向量立即数上半未清。
- 已知缺口：分支/调用/独占不测；FPCR 舍入模式未处理；性能上寄存器都在上下文里，后续再缓存。

## 3. 运行时 / HLE（`docs/RUNTIME.md`）

- 模块：`main` 与 `sdk` 两个 NSO 一起重编译、链接成一个可执行文件（sdk 必须重编译，因为 main 内联了 libc++/musl 的对象布局）。`subsdk`（图形驱动）不加载，导出由宿主实现。
- 启动：按 rtld 的 `_start` 逆向顺序：映射模块+清 bss → 应用重定位（`RELATIVE`=基址+加数；`GLOB_DAT/JUMP_SLOT/ABS64` **按名字**查：先查 **HLE 注册表**，再查模块导出，最后给记日志的桩）→ 建主线程(TLS 页、16MiB 栈) → 调 `nn::init::Start`，之后全是重编译的 SDK 代码跑 libc 初始化/静态构造/nnMain。
- 地址空间固定分区：模块 0x40_0000_0000、堆 0x50_...、别名区 0x58_...、栈 0x68_...、内核分配 0x6A_...、HLE thunk 0x6F_...。`svcMapMemory` 用复制模拟别名。
- 内核：每条 guest 线程一个宿主线程 + 独立 `nx::CPU`；ArbitrateLock/Unlock、WaitProcessWideKey/Signal、WaitForAddress/SignalToAddress、事件、`svcQueryMemory`/`MapMemory`/`GetSystemTick`（19.2MHz）等。
- 服务：HIPC/CMIF 解析 TLS 请求（句柄/PID/描述符/域），分发到 Service 对象；`sm:` 已实现；未知服务/命令“记日志并成功返回”，用来发现哪些才是必需的。
- 间接调用：`nx::call(target)` 用稠密表（text offset/4 → 函数）查模块，落空则是 HLE thunk。
- 重要工程点：**HLE 按名字注册**，比按地址 switch 更容易扩展和维护；SDK 的 PLT 调用也可按名字替换。

## 4. GPU：NVN→Vulkan（`docs/NVN.md` + `runtime/src/nvn/`）

- **刻意选择在 NVN API 层切**，不实现 nvhost/Maxwell 命令处理器：
  - NVN 对象带着语义到达（格式/大小/用途/状态），不需要解析命令流；
  - SDK 导出一个入口 `nvnBootstrapLoader`，游戏再用 `nvnDeviceGetProcAddress` 按名字拿其余函数（和 Vulkan 加载器同形）——我们把 `nvnBootstrapLoader` 做 HLE，返回自己的函数，SDK 的 NVN 驱动永不运行。
- 命令缓冲：记录到 **我们自己的命令列表**，`nvnQueueSubmitCommands` 时回放到 Vulkan；同一句柄可多次提交。
- 显存/池是核心：游戏直接把数据写进池再传 **GPU 地址**；做法是宿主地址分配器 + 地址→(池,偏移)→(VkBuffer,偏移) 映射；PC/Mac 用 `VK_EXT_external_memory_host` 直接把 guest 池导入 Vulkan 内存（零拷贝），Android 用影子缓冲+脏页跟踪（后来 Mac 路径本身也走拷贝，所以 Android 也复用）。
- 同步：timeline semaphore / fence；窗口：`nvnWindowAcquireTexture`/`nvnQueuePresentTexture` 走已有 present pass。
- 纹理：NVN→VkFormat 表；ASTC 原生（Android），PC 用 compute 解码；**block-linear（Maxwell tiled）**在 upload 时 untiled；纹理/采样器池（描述符表，NVNtextureHandle=纹理id|采样器id）→ bindless 描述符数组；RT/depth/MSAA resolve/copy/blit。
- 管线：动态状态尽量用 `VK_EXT_extended_dynamic_state` 1–3/`vertex_input_dynamic_state`，其余走 pipeline cache；着色器全部预先已知，可提前建管线。
- 约定：深度范围 −1..1 vs 0..1、窗口原点、viewport 翻转；缺 `depthClipControl` 时用 SPIR-V patch（z=(z+w)/2，和 Ryujinx 一样）。
- 调试设施：draw trace、帧 dump、按开关二分（`docs/nvn/SWITCHES.md`、`DEBUG.md` 数十条编号缺陷记录：症状/原因/修复/开关）。

## 5. 着色器 AOT（`docs/nvn/SHADERS.md`、`TRANSLATOR.md`）

- Switch 游戏带的是 **Maxwell 机器码**（代码块 + control section 带绑定信息），全部在 ExeFS/RomFS 里 → **可以提前翻译**，和 CPU 代码一样。
- 工具链：`tools/maxwell/containers.py` 解析 SARC/FSHA/BNSH/VFXB/BAHS（含 `_RLT` 重定位表、`_STR` 字符串池、ResDic 字典、反射表）；`--extract` 输出每 stage 的 `.code.bin`/`.ctrl.bin`+manifest（含 code 的 SHA-1 前 16 位作 cache key）。
- 翻译：**先用 Ryujinx 的 Maxwell→SPIR-V 翻译器（MIT）离线跑**（`tools/maxwell/spirv_harness`，C# NativeAOT 成 `libspirvharness.so`），Wonder 12,717 个不同块全部出 SPIR-V；长期再自研。
- 结果：APK 里不带着色器源码，运行时按 hash 查 SPIR-V；管线缓存落盘。
- 验证：单着色器编译+回放对比；整帧与参考截图比对。

## 6. P5R 专用引擎知识（`games/p5r/game.cpp`，对我们 P5D 最有用）

- P5R 走 **NVN**，渲染是 **Atlus GFD 引擎**；着色器在 `CPK/ALL_USEU.CPK` 里的 **`BASE/GFDNXHHPRESET*.GSC`** 着色器缓存中（`tools/gsc_shaders.py` 提取）。
- GSC 提取（`games/p5r/import/p5r_shaders.cpp` 的完整实现）：
  - CriWare CPK：`CPK ` 头 → `TOC ` @UTF 表 → 文件偏移（`ContentOffset` 基准），可选 CRILAYLA 解压；
  - 每个 `*.GSC` 里找 **HNSH** 记录（magic `"HNSHV002"`），得到 code/control 偏移；
  - code 块：NVN 头 `0x12345678`(graphics)/`0x12345679`(compute)，graphics 在 +0x30 有 SPH，compute 从 +0x100 开始指令；control 区 0x880：`+0x704` code block size、`+0x6f8` program length、`+0x708` program start、`+0x700/+0x6fc` 常量数据偏移/大小、`+0x748` 计算工作组、`+0x7f9` 采样器数、`+0x804` 每采样器绑定槽……
  - 也扫描 ExeFS 里的 code block/control 配对（有些游戏把 shader 放执行文件里）。
  - 输出 manifest.tsv（stage/code_off/code_size/ctrl_off/program_start/const/code_sha1_16/code_file/ctrl_file）。
- 运行时把 `glslc*`（NVN 的运行时 GLSL 编译器）全部桩掉——因为着色器全部预编译，永不运行时编译。
- 引擎结构（P5R main 模块偏移，P5D 会有不同偏移但布局相似，值得对照）：
  - 目标帧率 `[[main+0x2209f50]+0x84]`（Switch=30），读取函数 main+f73730，设置函数 main+f73740；
  - 帧间隔 setter `main+1048ee0`（vsyncs per frame 1..4 存 obj+0x30），present `main+1025c9c` 交给 `nvnWindowSetPresentInterval`；
  - 引擎帧循环 `main+fe75ac` 用 19.2MHz system tick 计时，clamp 到 frame timer 的 `[[main+0x220a9c0]+0x34]` 秒（0.05），存到 `[[main+0x2209f50]+0x50]`（+0x48 是帧起点）；`main+f730c0` 把它当秒交给游戏 update；
  - **任务链**：`[[main+0x2209f50]+0x2c40]` 是 update 列表，next 在 +0x80，任务名在 +0x18、长度低半在 +0x20；名字有 `TITLE_DRAW`、`MOVIE_DRAW`、`event_proc`、`battle`、`BTL_*_EVENT`、`General_<field>_<n>`、`EventSkip`、`D00_*_START` 等 —— 用任务链可以判断当前场景/阶段，比纯 PC 轨迹有用得多；
  - 帧率/节奏分析：游戏每帧步进 1/target；低帧率下游戏变慢（音画不同步）→ 用“跟随设备”的 paced target，并只在标题/事件场景启用；某些脚本场景只允许 cap、cap/2、cap/3 的步长；
  - 其他：每模型剔除 job（mesh+0x20 bit14、mesh bounds+0x90、矩阵 main+a92c80、视锥平面 [main+0x22cd268] 6×32B、遮挡用 GPU 画到 pitch-linear R32F 的深度金字塔需回读线性 RT）；线性 RT write-back 暂停（电影时）；HUD 1080 目标等。
- 工程细节：把引擎断言用的 `fputs` 包一层记日志（SDK stdio 不落地）；P5R 的 HLE 还包括 fputs/glslc 等。

## 7. 对我们的 P5D 项目意味着什么（可执行建议）

### 7.1 已有资产直接对标
- P5D（PSV）同样是 **Atlus GFD 引擎族**，本机已有 **`C:\dev\p5d_build\GFDPSP2.GSC`**（4.09 MB，头 `GFS0 ... GSC0`）——即 Vita 版（PSP2）的 GFD 着色器缓存，和 P5R 的 `GFD...GSC` 同类。
- 既有工作已经产出 **`C:\dev\p5d_build\spv_all`：4332 个 `.gxp.spv`、83.9 MB**（另有 `spv_all_new`、`spv_2ph` 两版），以及 `rt_gxp/` 运行时抓取的 12 个 GXP 样本；这正符合“**所有游戏着色器 AOT 预编译为 SPIR-V**”的目标路线，可直接复用/重建索引。
- 下一步建议：写一个 **GSC0 解析器**（仿 `gsc_shaders.py`，但 P5D 的 GSC0 里嵌的应是 Vita GXP 而不是 NVN 代码块），把每个 GXP 程序按 **SHA-1 前 16 位**命名、导出 manifest（program id/参数表/类别），并和 `spv_all` 的 4332 个 .spv 对齐；运行时 GXM HLE 的 `CreateShaderProgram/GetProgramFromId/FindParameterByName` 只做 **查表**，永不运行时编译。
- 注意 P5D 的 GXM 与 Switch 的 NVN 不同：P5D 是 GLES2 风格 API（GXM），着色器是 GXP（SceGxmProgram），翻译器需要 GXP→SPIR-V（我们既有 spv_all 说明此链路之前已跑通，需找回对应脚本：`C:\dev\tmp` 下 `_p5d_gxp*.py`、`spv_all` 生成器；`p5d_native`/`out` 目录可能是旧工具链）。

### 7.2 架构层面的采纳
1. **HLE 按名字/导入 NID 注册**：Vita ELF 的导入表（stub 名/NID）可以直接映射到 HLE 函数表；比现在 0x812Bxxxx 地址 switch 更好维护、更好补。
2. **AOT 差分测试（Unicorn）**：为我们 ARM32/Thumb 生成器建同等测试（按指令形态抽样 + 随机状态 + Unicorn 对比）。本轮 M643/M650 抓到的 post-index writeback 丢失、寄存器移位 `&0x1F` 回卷、缺 `setf_nz` 等问题，这类测试本可提前全部捕获。建议优先级高。
3. **GPU 在 API 层接**：确认我们 GXM→Vulkan 也该实现“GXM 对象 + 命令记录 + 提交时回放”，而不是解析 SceGxm 命令流；对象字段尽量保持游戏自己分配的布局。
4. **Guest 内存即宿主内存 + GPU 地址映射**：GXM 也是传 guest GPU 地址（`SceGxmTexture`/surface 数据在 guest 内存），照搬“地址→(池,偏移)→VkBuffer”的表。
5. **任务链探针**：在 P5D 里找引擎 update 任务列表（P5R 是 g+0x2c40），打印任务名判断场景/阶段——比 [FRAMEDUMP] PC 采样更直观，也能快速确认是否进入 TITLE_DRAW 等。
6. **着色器数据包 + pipeline cache**：若做移动端，SPIR-V 打成一个索引文件（成千上万小文件在 Android 外置存储很慢），管线缓存落盘。
7. **帧率/节奏**：P5D 原版 30fps 固定步长；等渲染通了再考虑高刷/跟随设备节奏（可借鉴其 paced_target/scene 检测思路，但 P5D 与 P5R 任务名不同）。
8. **移动端策略选择**：p5r-android 手机版最终用 **NCE（原版代码直接执行）** 来规避分发重编译代码的版权问题；我们的目标明确要求“静态重编译 + 单 exe”，先保持；未来若要公开手机版，需在“只发布引擎/运行时 + 用户自己的数据”与“用户本地重编译”之间做同样的决策。

### 7.3 可参考的工程习惯
- 文档分层：`PROGRESS.md`（按时间的新进展+截图）、`docs/nvn/DEBUG.md`（编号缺陷：症状/原因/修复/开关）、计划文档（阶段/退出标准/工作量）；与我们 `P5D_MASTER.md` 的里程碑/关键词锚点风格互补。
- 差分测试 + 单指令形态覆盖是 AOT 质量的关键手段。
- 每加一个 HLE 都记日志并“失败也继续”，用日志决定下一个要补什么（未知服务 log-and-succeed 策略）。

## 8. 来源

- 仓库主页：https://github.com/Brandon04Gonzalez/p5r-android
- 文件树/单文件（经 jsDelivr 镜像）：`docs/RECOMP.md`、`docs/RUNTIME.md`、`docs/FRAMEWORK.md`、`docs/NVN.md`、`docs/nvn/SHADERS.md`、`docs/nvn/ANDROID-PORT.md`、`games/p5r/game.cpp`、`games/p5r/import/p5r_shaders.cpp`
- 上游：SwitchGamesRecomp / Super Mario Bros. Wonder Recomp（BelmanteGu）
- Ryujinx Maxwell 着色器翻译器（MIT）

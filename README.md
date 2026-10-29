# P5D-PC — P5D（PCSH10109）PC 原生重编译移植

把 PS Vita 版《Persona 5: Dancing in Starlight》（P5D，PCSH10109）的**原版游戏二进制原样加载**，
在 PC（后续可扩展到手机）上由我们实现的本地兼容层（HLE）驱动运行。
CPU 采用 **AOT 静态重编译**（原版 ARM/Thumb 代码一次性翻译为 x86-64 原生执行）+ 解释器 fallback；
系统 API（文件/存档、内存、线程、GXM→Vulkan、音频、输入、视频解码）全部由 HLE 接管；
最终由**原版游戏代码自己绘制**画面，目标串联 `S0_LOGO → S1_TITLE → S2_MENU → S3_SELECT → S4_GAME → RESULT` 全流程。

> 本仓库**不包含任何游戏文件、游戏密钥、固件或由游戏数据翻译出的二进制产物**。
> 要运行本项目，玩家必须使用**自己合法持有的正版游戏**从自己的游戏机提取数据（详见 [docs/RELEASE_COMPLIANCE.md](docs/RELEASE_COMPLIANCE.md)）。

---

## 项目进度总览（2026-10-29，诚实估计：整体约 30–35%）

| 目标模块 | 完成度 | 现状（证据见 `docs/P5D_MASTER.md`） |
|---|---|---|
| ARM/Thumb → x86-64 **AOT 静态重编译** | **85–90%** | 12,921 函数 / 约 22 万基本块已可翻译、编译、原生执行；不靠逐指令运行时模拟，语义仍在用差分测试打磨 |
| 原版 eboot/SELF **原样加载** | ~85% | ELF 段映射、导入表、NID 解析可用；30M guest 块执行 `exit=0` |
| **HLE 系统 API 接管** | 50–60% | 内存/线程/fiber/文件骨架工作；导入 API 仍有大量返回 0 的桩；12 个 Fios API 已映射 |
| **GXM → Vulkan** | **25–30%** | 真实桥接层（`gxm_abi.o` + 旧后端 `gxm_sce.o`/`gxm_vk.o`）已链接；游戏真实 GXM 初始化序列 **60 次调用全部 ret=0**、Vulkan 后端初始化成功、shader patcher 注册循环已有 **2550 次真实 ABI 调用**；但 BeginScene/EndScene/Present 仍为 0 次，尚未画出第一帧 |
| 资源 / 文件系统 | ~30% | CPK 挂载与读取代码在；guest 虚拟文件注册表可建立；**payload（type 0→0xC）加载通路缺失**（当前阻塞点） |
| 线程 / fiber | 60–65% | 配对模式 `exit=0`、无漂移；仍依赖多个 env 开关，默认模式处于平台期 |
| 音频（SceNgs） | 3–5% | 未开始 |
| 输入（SceCtrl/SceTouch） | ~10% | 无实际事件注入 |
| 视频解码 | 0% | 未开始 |
| 场景链 S0→S4→RESULT | **0/7** | 尚未到达第一个 BeginScene |
| 单 exe 打包 | ~40% | 仓库/许可/发布审计/CI 在；未产出单文件成品 |
| 移动端 | 0% | 未开始 |

**一句话**：最难的「原版二进制 + AOT 引擎」骨架已经立住并能跑真实 GXM 初始化；但渲染还差最后一脚，场景链尚未开始。

## 本项目与 Vita3K 模拟器的区别（简表）

> 完整说明见 **[docs/COMPARISON_VITA3K.md](docs/COMPARISON_VITA3K.md)**。
> 一句话：**Vita3K 是"模拟整台 Vita"的通用模拟器；本项目是"把 P5D 这一款游戏的原版二进制静态重编译成 PC 原生代码"的移植。**

| 维度 | 本项目（P5D 静态重编译移植） | Vita3K（通用 PS Vita 模拟器） |
|---|---|---|
| 本质/目标 | 单个游戏的**静态重编译移植**，原版代码原生绘制 | 通用**系统模拟器**，按兼容性列表运行大量游戏 |
| CPU | 启动前一次性 AOT 翻译为 x86-64 原生代码，执行路径无运行时 JIT；解释器仅 fallback | 运行时动态重编译（JIT）+ 解释器 |
| 系统层 | 按 NID 的 **HLE 兼容层**，只实现本作实际用到的 API；不需要固件 | 模拟 Vita 内核/服务/模块，通常需要玩家自备固件 |
| GPU | 在 **GXM API 层**映射到 Vulkan，不模拟 GPU 命令流 | 模拟 SceGxm/GPU 到宿主 OpenGL/Vulkan |
| 着色器 | 玩家数据中的 GSC/GXP **离线一次性翻译为 SPIR-V** 并缓存 | 运行时翻译着色器并缓存 |
| 交付 | 单一引擎可执行文件 + 玩家自备游戏数据 + 本地缓存 | 模拟器 + 玩家自备固件/游戏 |
| 成熟度 | 实验性，尚未进入渲染帧循环（见下方状态） | 多年社区项目，兼容性广 |

两者共性：都需要玩家自备正版游戏数据，都要重新实现 Sce 系统 API，都要把 GXM 翻译到宿主 GPU。

## 当前状态（2026-10-29，持续更新）

| 模块 | 状态 |
|---|---|
| ARM/Thumb → x86-64 AOT 重编译 | 12,921 个函数可翻译、编译、原生运行；语义仍在用差分测试逐项打磨 |
| 原版 eboot/SELF 加载（ELF 段映射、导入表、NID 解析） | 可加载、可执行，HLE 按 NID 接管 |
| 文件系统（CPK 挂载、Fios/IO 模拟） | 可挂载 `data.cpk` / `vita.cpk`；资源数据通路仍在打通中 |
| 内存 / 线程 / fiber / 同步原语 HLE | 协作式模型可跑通；实验路径支持一 guest 线程 = 一宿主线程 |
| GXM → Vulkan | 真实桥接层已集成：`gxm_abi.o` 把 100/100 个导入 GXM stub 映射为 guest ABI 包装，旧原生后端（`engine/gxm/`，5,694 行 Vulkan 实现 + 167 个 SceGxm API）执行真实初始化；**尚未到达 BeginScene/绘制循环** |
| 音频（SceNgs）/ 输入（SceCtrl、SceTouch）/ 视频解码 | 桩/部分实现，待正式接入 |
| 场景流程 S0→RESULT | 0/7，尚未进入帧循环 |

当前调试进度（里程碑持续记录于 `docs/P5D_MASTER.md`，最新为 §18.4326 / M781）：

- **真实 GXM 初始化已跑通**：`[VKMARK] Vulkan` 后端初始化；`sceGxmInitialize / MapMemory / CreateContext / CreateRenderTarget / ColorSurfaceInit / SyncObjectCreate / TextureInitLinear` 等 60 次调用全部 `ret=0`；随后 shader patcher 注册循环出现 2550 次真实 ABI 调用（`ShaderPatcherCreateFragmentProgram`、`ProgramFindParameterByName` 等）。
- **当前唯一阻塞：guest 虚拟文件系统的资源负载通路**。游戏向自己的 VFS 请求 `GFDPSP2PRESET.GSC`（GFD shader preset 容器）；VFS 自动注册了该名字（`FUN_81181A3C`，条目 `type=0`、`size=-1`，仅名字与哈希），但**没有子系统把它升级为 `type=0xC`（带数据指针 `+0x94` / 尺寸 `+0x88`）**；应消费该记录的 worker `0x811830E2` 从未被调用；于是 GFD 工厂拿到零尺寸流，创建 type-3 零边界对象，`FUN_811DE912` 返回 -1，`byteswap` 尺寸 -1 ⇒ `0x3FFFFFFF` 次迭代卡死。
- **已精确定位的证据链**：注册插入块 `0x81181BB2`（`wr32_watch` 归因）；auto-register 路径 `0x81182EC6`；worker 语义（从条目 `+0x94/+0x88` 填流对象后置 `state=3`）已反汇编确认；对象 `state=0→3` 的写入绕过了 `wr16/wr32` 监视（正在用字节级 `wr8` 监视收网）。
- **零崩溃基线保持**：默认配置 30M guest 块 `exit=0, BADMEM=0, GXM=12, FSW=16, FRAMEDUMP=6000`；`kb_audit / verify_doc_integrity / run_all_regressions` 全绿，`release_audit` PASS。

近期里程碑（详细证据与历史纠正在 `docs/P5D_MASTER.md`）：
- M772–M773：定位巨型 byteswap 根因 = `FUN_811DE912` 返回 -1 被当作 byteswap 尺寸；type-3 零边界来自 GFD 工厂参数全零。
- M774–M775：Fios 句柄 watch 证明 "done 但未填数据指针/尺寸"；更正 `FHOpen=0`（此前 12288 为 `g_fh_id` 初值）。
- M776–M777：锁定缺失资源名 `GFDPSP2PRESET.GSC`；解码应填数据的 worker `0x811830E2` 语义。
- M778–M780：解码虚拟文件注册函数 `FUN_81181A3C` 与 auto-register 路径；用写监视精确抓到插入块 `0x81181BB2`。
- M781：对象级写监视证明 `state=3` 写入绕过 `wr16/wr32`，worker 从未被调用——当前正在用字节级写监视收网。

## 仓库结构

```
docs/
  P5D_MASTER.md              权威研究/开发记录（约 33 MB；全部里程碑、证据、结论与历史纠正）
  P5D_DIGEST.md              精简摘要
  P5D_OPERATION_CHECKLIST.md 操作/验证清单
  P5R_ANDROID_REFERENCE.md   同族引擎移植项目（P5R Android）参考研究
  RELEASE_COMPLIANCE.md      发布合规策略（红线/交付形态/本地缓存）
  _MD_INDEX.md               文档索引
  REUSABLE_KNOWLEDGE.md      可复用知识清单（历史成果/工具/规格/负结果，按 ROI 排序）
tools/
  release_audit.py           发布树合规扫描（禁止游戏文件/游戏衍生数据）
  kb_audit.py                知识库锚点审计（每个里程碑必须追加锚点）
  kb_lookup.py               知识库检索
  kb_ledger_audit.py         里程碑账本审计
  verify_doc_integrity.ps1   文档完整性校验
  run_kb_audit.ps1 / run_all_regressions.ps1   一键校验
engine/
  recompiler/_gen_split.py   ARM/Thumb → C++ AOT 重编译生成器
  recompiler/_gen_batch2.py  指令发射后端（含移位/写回等语义修正）
  runtime/recomp_harness.cpp 静态重编译运行时 + HLE（内存映射、线程/fiber、GXM/文件/Pad 等）
  runtime/gxm_abi.cpp        自动生成的 GXM ABI 桥接层（100/100 导入 stub）
  runtime/gxm_vulkan.h       显示/帧路径占位实现（真实后端在 gxm/）
  gxm/                       GXM → Vulkan 后端（sce_gxm、vulkan_backend、texture_format）
  mem/                       虚拟地址空间注册/翻译（客机 VA → 宿主指针）
  common/                    公共类型与日志
```

## 设计路线

1. **原版二进制一字不改**：直接读取玩家提供的 eboot/SELF（不修改、不重打包）。
2. **静态重编译**：启动前一次性把 ARM/Thumb 全量翻译为 x86-64（本机生成并缓存），执行路径不做运行时代码生成；解释器仅作 fallback。
3. **HLE 只替换系统边界**：游戏代码原样执行，文件/内存/线程/图形/音频/输入等系统 API 由兼容层实现。
4. **GXM→Vulkan 在 API 层**：不模拟 GPU 命令流，把 SceGxm 调用映射为 Vulkan 调用；着色器由玩家数据中的 GSC/GXP **在玩家机器上一次性翻译**为 SPIR-V 并缓存。
5. **发布形态**：`P5D_Engine.exe`（引擎，无游戏数据） + 玩家自备游戏数据 + 本地缓存（`cache/recomp`、`cache/shaders`、`cache/pipelines`）。

## 构建与运行（开发中）

当前仓库为**开发中间状态**，尚未提供开箱即用的构建脚本；后续会补充：

- 编译 GXM→Vulkan 后端与运行时（MinGW g++ / MSVC + Vulkan SDK）
- AOT 生成器用法（输入：玩家自备 eboot；输出：本地代码缓存）
- 首次运行 "Prepare" 流程（本地 AOT + 着色器翻译）

## 合规与免责声明

- 本仓库只包含**我们自己的引擎/工具源代码**与**研究记录**；不包含也不接受任何游戏文件、密钥、固件、由游戏数据翻译得到的二进制（如 SPIR-V 缓存）或游戏画面素材。
- 本项目与 Atlus / SEGA / Sony 无关；仅用于**互操作性研究与个人备份游玩**。请使用自己合法持有的正版游戏。
- 许可证：**MIT**（宽松许可，见 [LICENSE](LICENSE)）。本仓库中的源代码、AOT 生成器、工具与研究文档均为 MIT；第三方组件许可见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
- MIT 仅覆盖本仓库中我们原创的内容；游戏本体与游戏素材版权归 ATLUS / SEGA，**不在 MIT 授权范围内**，且本仓库不包含任何游戏数据。

## 相关记录

- 权威开发记录（含所有里程碑与结论）：[docs/P5D_MASTER.md](docs/P5D_MASTER.md)
- 与 Vita3K 模拟器的区别：[docs/COMPARISON_VITA3K.md](docs/COMPARISON_VITA3K.md)
- 发布合规：[docs/RELEASE_COMPLIANCE.md](docs/RELEASE_COMPLIANCE.md)

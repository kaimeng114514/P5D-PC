# P5D-PC — P5D（PCSH10109）PC 原生重编译移植

把 PS Vita 版《Persona 5: Dancing in Starlight》（P5D，PCSH10109）的**原版游戏二进制原样加载**，
在 PC（后续可扩展到手机）上由我们实现的本地兼容层（HLE）驱动运行。
CPU 采用 **AOT 静态重编译**（原版 ARM/Thumb 代码一次性翻译为 x86-64 原生执行）+ 解释器 fallback；
系统 API（文件/存档、内存、线程、GXM→Vulkan、音频、输入、视频解码）全部由 HLE 接管；
最终由**原版游戏代码自己绘制**画面，目标串联 `S0_LOGO → S1_TITLE → S2_MENU → S3_SELECT → S4_GAME → RESULT` 全流程。

> 本仓库**不包含任何游戏文件、游戏密钥、固件或由游戏数据翻译出的二进制产物**。
> 要运行本项目，玩家必须使用**自己合法持有的正版游戏**从自己的游戏机提取数据（详见 [docs/RELEASE_COMPLIANCE.md](docs/RELEASE_COMPLIANCE.md)）。

---

## 当前状态（2026-10-30，持续更新；整体约 50–55%）

| 模块 | 完成度 | 当前状态 |
|---|---|---|
| ARM/Thumb → x86-64 **AOT 静态重编译** | 90–95% | 共 **229,791 个基本块 / 32 个宿主批次**已翻译并原生运行；`[NOFN]=0`。M864 通过 `batch_031`（补充 `extra6_funcs.json`：`{va:0x8118285A,size:0x174}`）修复了 IDA 函数边界截断造成的缺失块 `0x811829CC`（直接导致游戏自己的虚拟文件 worker 线程死亡） |
| 原版 eboot/SELF **原样加载** | 85–90% | ELF 段映射、导入表、NID 解析可用；默认 30M guest 块稳定 `exit=0`；**完整 boot 链已打通**：S0_LOGO → 启动影片（约 100 秒虚拟时间）→ 应用世界在 n≈61M 处恢复运行 |
| **HLE 系统 API 接管** | 65–70% | 内存/线程/fiber/信号量/事件标志/消息/文件/GXM 骨架工作；M868 修复 `sceKernelCreateLwMutex` 语义（原实现返回堆指针而非错误码 0，导致 CRI 初始化静默失败）；新增 `[CRILOG]` 探针接管 CRI 日志函数，可直接看到游戏内部初始化错误 |
| **GXM → Vulkan** | 40–50% | Vulkan 后端初始化成功；`CreateContext` / `CreateRenderTarget` / ShaderPatcher / 程序创建已通；**BeginScene/EndScene/Present 仍为 0**（尚未产生可见帧） |
| 资源 / 文件系统 | 60–65% | 已挂载 `data.cpk`（10,555 文件）+ `vita.cpk`（531 文件）；25 个 title/staff/sound 资源名已解析；两个 GLOBAL_RESOURCE 加载器对象已到达 state `0xC`；但 appResourceList 的 stream 对象因 binder 未初始化停在 state `0x13` |
| 线程 / fiber / 调度 | 75–80% | 游戏已进入自己的每帧调度循环（GFD 主循环 + appProcess 调度器 + app 帧更新）；帧计数 ctr 已推进到 681+；虚拟文件 worker 线程（`4001B000`）已修复存活，`FUN_81182742` 虚拟文件处理器累计运行 **729 万次以上** |
| 音频（SceNgs / CRI Atom） | 5–10% | CRI Atom 初始化已真正进入；当前因 **ACF 配置文件缺失 + 工作缓冲不足** 中止（见下方阻塞） |
| 输入（SceCtrl / SceTouch） | ~10% | Pad 骨架可用，游戏 side 尚未接入完整输入流 |
| 视频解码 | 5–10% | 启动 USM 影片的 guest 解码循环已跑通（Huffman 解码器 `FUN_811DF91A`，约 5,000 万块 ≈ 100 秒虚拟时间），真实解码调用 12 次 |
| 场景链 S0→S4→RESULT | S0 ✅ / S1 加载中 / S2–S4 0/7 | S0_LOGO 完整通过；S1_TITLE 资源加载被上方 binder 卡点阻塞；S2_MENU / S3_SELECT / S4_GAME / RESULT 尚未到达 |
| 单 exe 打包 / 移动端 | ~40% / 0% | 仓库/许可证/审计/CI 在跑，尚未产出单文件成品；移动端未开始 |

### 最近关键里程碑（M844–M868）

- **M844–M847**：显示回调桥接（`p5d_hle_set_display_cb` / display queue）；`mb_811EB930` 显示提交空指针守卫（手工块）
- **M848–M850**：启动影片快速结束（`call_guest_cb2` + 影片完成回调），boot 不再卡在影片播放
- **M852–M859**：修正探针判定错误（嵌套 fiber 执行中 `g_blkcnt_total` 冻结 → 全部迟采样改用 `g_main_n`）；**关键突破**：之前"boot 后就停住"其实是启动 dummy-USM 影片造成的约 100 秒虚拟时间挂起，应用世界在 **n≈60,916,231** 正式恢复运行
- **M860–M866**：恢复后一切都在运行（GFD 主循环、app 帧循环、sequence 观察者、appResourceList 泵、initDataRead 步骤），唯一卡点是**异步文件读取不完成**；随后完整解码了虚拟文件层：处理器 `FUN_81182742`（24 项状态跳转表 `0x81182764`）、读取器 `FUN_811823FE`（stream 字段 `+0x88 大小/+0x94 缓冲/+0x9C binder 句柄/+0xA0 完成标志`，状态 `0x10→0x11→0x13/0xC`）
- **M863**：根因定位——游戏自己的虚拟文件 worker 线程 `4001B000` 在第一次循环后死亡；`[THREXIT] badblock tid=4001B000 pc=811829CC` 指出缺失的基本块
- **M864**：用补充批次 `batch_031` 补上 `0x811829CC`（处理器函数尾部，IDA 边界 `FUN_81182742 size=280` 截断）→ **worker 存活，虚拟文件处理器运行 729 万次以上，stream 状态开始推进（0→0x10）**
- **M867**：确认 binder 模块从未初始化（`DAT_8153FEF4=0` 全程；初始化点 `FUN_81284960` @0x81284CDC，唯一调用者链 `FUN_8128399E = criFs_Initialize`）
- **M868**：**修复重大 HLE 语义 bug**：`sceKernelCreateLwMutex` 原返回 `hle_malloc(64)` 指针，而 API 返回错误码（0=成功）→ CRI 初始化报 `E10062288:sceKernelCreateLwMutex function has failed.` 并中止；修复后 CRI Atom/FS 初始化真正向前推进，`[CRILOG]` 探针暴露出下一批精确阻塞

### 当前阻塞（明确、可执行）

- **CRI Atom 初始化失败**（`E2009072703:Insufficient buffer size.` 来自 `FUN_81224160 = criAtom_Initialize`；`E2010052671:Can not open ACF file.`）
- → `criFs_Initialize`（`FUN_8128399E`）未被调用 → **CRI FS binder 模块未初始化**（`DAT_8153FEF4=0`）
- → 资源文件大小查询被拒绝（`W2008121610:The binder module is not initialized.`）→ 虚拟文件 stream 停在错误态 `0x13`
- → `global_resource_810995D4()` 忙位不清 → `init gamedata` 不结束 → **boot page1 不结束 → 标题页不出现**

下一步任务顺序：
1. 定位 ACF 文件（`*.acf`）在 CPK 中的名字，让文件 HLE 返回 guest 期望的路径；修复 criAtom "工作缓冲不足"（调用方传入大小 vs 需求）
2. criAtom 初始化完成 → `criFs_Initialize` 执行 → binder 初始化 → 文件大小可查
3. stream 状态进入 `0x11` → 异步读完成回调 `LAB_811829E2` → `0xC` → 资源节点完成 → `init gamedata` → **boot page1 结束**
4. 之后进入标题页 → 下一个大关是 **GXM 真实渲染管线**（BeginScene / EndScene / Present / 绘制调用）

## 仓库结构

```
docs/
  P5D_MASTER.md              权威研究/开发记录（全部里程碑、证据、结论与历史纠正）
  P5D_DIGEST.md              精简摘要
  P5D_OPERATION_CHECKLIST.md 操作/验证清单
  REUSABLE_KNOWLEDGE.md      可复用知识清单（历史成果/工具/规格/负结果，按 ROI 排序）
  COMPARISON_VITA3K.md       与 Vita3K 模拟器的区别
  GXM_ABI_TEST_REPORT.md     GXM ABI 桥接测试报告
  HLE_DIFF_REPORT.md         HLE 与真实系统的差异报告
  SCHED_SPIN_ANALYSIS.md     调度/自旋循环分析
  P5R_ANDROID_REFERENCE.md   同族引擎移植项目（P5R Android）参考研究
  RELEASE_COMPLIANCE.md      发布合规策略（红线/交付形态/本地缓存）
  _MD_INDEX.md               文档索引
tools/
  release_audit.py           发布树合规扫描（禁止游戏文件/游戏衍生数据）
  kb_audit.py                知识库锚点审计（每个里程碑必须追加锚点）
  kb_ledger_audit.py         里程碑账本审计
  kb_lookup.py               知识库检索
  verify_doc_integrity.ps1   文档完整性校验
  run_kb_audit.ps1 / run_all_regressions.ps1   一键校验
  aot_difftest.py            AOT 差分测试
  diag.py / hle_probe.py     诊断与 HLE 探测工具
engine/
  recompiler/_gen_split.py   ARM/Thumb → C++ AOT 重编译生成器
  recompiler/_gen_batch2.py  指令发射后端（含移位/写回等语义修正）
  runtime/recomp_harness.cpp 静态重编译运行时 + HLE（内存映射、线程/fiber、GXM/文件/Pad 等）
  runtime/gxm_abi.cpp        GXM ABI 桥接层（导入 stub + 指针翻译）
  runtime/cpk_reader.h       CPK 虚拟文件系统 + CRILAYLA 解压（header-only）
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

## 验证与审计

```powershell
py tools/release_audit.py .        # 发布树合规（PASS 才可提交）
powershell -File tools/run_kb_audit.ps1
powershell -File tools/verify_doc_integrity.ps1
powershell -File tools/run_all_regressions.ps1
```

每个里程碑要求：权威结论写入 `docs/P5D_MASTER.md` 的「ANTI-FORGETTING ENTRY」+ `tools/kb_audit.py` 追加锚点，
以上四项检查全绿后才会提交；历史记录只追加、不删改。

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

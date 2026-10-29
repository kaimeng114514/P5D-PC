# P5D-PC — P5D（PCSH10109）PC 原生重编译移植

把 PS Vita 版《Persona 5: Dancing in Starlight》（P5D，PCSH10109）的**原版游戏二进制原样加载**，
在 PC（后续可扩展到手机）上由我们实现的本地兼容层（HLE）驱动运行。
CPU 采用 **AOT 静态重编译**（原版 ARM/Thumb 代码一次性翻译为 x86-64 原生执行）+ 解释器 fallback；
系统 API（文件/存档、内存、线程、GXM→Vulkan、音频、输入、视频解码）全部由 HLE 接管；
最终由**原版游戏代码自己绘制**画面，目标串联 `S0_LOGO → S1_TITLE → S2_MENU → S3_SELECT → S4_GAME → RESULT` 全流程。

> 本仓库**不包含任何游戏文件、游戏密钥、固件或由游戏数据翻译出的二进制产物**。
> 要运行本项目，玩家必须使用**自己合法持有的正版游戏**从自己的游戏机提取数据（详见 [docs/RELEASE_COMPLIANCE.md](docs/RELEASE_COMPLIANCE.md)）。

---

## 当前状态（2026-10-29，持续更新；整体约 35–40%）

| 模块 | 完成度 | 当前状态 |
|---|---|---|
| ARM/Thumb → x86-64 **AOT 静态重编译** | 85–90% | 主函数表 12,921 个 + Ghidra 补充 1,907 个（共 **14,828 函数 / 229,744 基本块**）已翻译并原生运行；`[NOFN]=0`（翻译覆盖缺口全部消除） |
| 原版 eboot/SELF **原样加载** | 85–90% | ELF 段映射、导入表、NID 解析可用；默认 30M guest 块稳定 `exit=0` |
| **HLE 系统 API 接管** | 55–60% | 内存/线程/fiber/文件/GXM 骨架工作；**游戏自己的文件打开路径已能从我们的 CPK 供给数据**（两个 GFD 全部 `state=3` 加载完成，字体 3 个全部加载） |
| **GXM → Vulkan** | 30–35% | Vuekan 后端初始化成功；`CreateContext` / `CreateRenderTarget` 已成功；**59 个 ShaderPatcher 创建** + Vertex/FragmentProgram 创建 + 24 次 precomputed-draw 调用；**BeginScene/EndScene/Present 仍为 0** |
| 资源 / 文件系统 | 60–65% | `GFDPSP2PRESET.GSC`(62,100) 与 `GFDPSP2.GSC`(4,094,756) 经游戏自身 Fios 打开路径加载；`c_font_p5.fnt`(1,167,652)、`font_staff.fnt`(28,176)、`font.ctbl`(22,024) 全部加载 |
| 线程 / fiber | 60–65% | 游戏已进入自己的每帧调度循环（`0x812067A0` 管理器循环 + `0x811E5CDA` 帧槽）；阻塞：fiber 与 main 线程**共用同一栈区**导致挂起帧被解码器覆盖（见下） |
| 音频（SceNgs）/ 输入（SceCtrl、SceTouch）/ 视频解码 | 3–5% / ~10% / 0% | 桩或未开始 |
| 场景链 S0→S4→RESULT | 0/7 | 尚未到达第一个 BeginScene |
| 单 exe 打包 / 移动端 | ~40% / 0% | 仓库/许可/审计/CI 在，未产出单文件成品；移动端未开始 |

### 最近里程碑（M797–M806）

- **M797**：`rea` 在 Windows 上可分析原版 ARM eboot（本地补丁 rea 的 Windows-P0 策略 + Ghidra 12.1.3 + JDK-21）
- **M798**：Ghidra 工程导出 16,484 函数，对比主表后补充 **1,907 个翻译函数 / 8,828 块**，`[NOFN]` 全部消除
- **M799**：`rea decompile` 确认 GFD 循环 `FUN_811df91a` 是 Huffman 解码器（合法长解码，非死循环）
- **M800**：canary 监视改为 env 门控（5× 提速）；pump2 对已完成 fiber 不再无限重跑
- **M801**：手工块 `mb_81181D90` 让**游戏自己的文件打开路径**从 CPK 供给数据（`[OPENRET] state4=3`）
- **M802**：`mb_8118310A` 安全 worker 块消除 BADMEM abort → **第二个虚拟文件 `GFDPSP2.GSC`(4MB) 完整加载**
- **M803**：全部资源（2 GFD + 3 字体）经游戏自身路径加载完毕，游戏进入**自己的帧循环**
- **M804**：GXM 空句柄合成（`0x85410000+`）→ 游戏开始真正构建渲染管线（Context/RenderTarget/59 ShaderPatcher/PrecomputedDraw）
- **M805/M806**：定位 `pc=0x1F1` 根因 —— GFD 解码器（`FUN_811df91a`，栈帧约 0x7420 字节）在**共享栈**上覆盖了 fiber1 的挂起 delay 帧（`A05FFC10` 附近），fiber 恢复时弹出解码树节点值 `0x1F1`

### 当前阻塞

fiber1 与 main/嵌套解码执行**共用同一个 `A05Fxxxx` 栈区**：解码器在 fiber1 挂起后继续使用该栈，覆盖了 fiber1 保存的返回帧，
导致恢复时 `pc=0x1F1`（NOFN 6 次后恢复）。真实硬件上每个 fiber 由 `sceFiberInitialize` 分配**私有栈**。
下一步：记录游戏传入的 `addrContext/sizeContext` 参数，为每个 fiber 分配私有栈并在调度器中强制 sp 归属不变量；
修好后即可让帧回调路径继续走向 `BeginScene → EndScene → DisplayQueue → 第一帧 Present`。

> 详细证据、里程碑与历史纠正在 [docs/P5D_MASTER.md](docs/P5D_MASTER.md)（当前约 34 MB，含 M1–M806 全部记录）。

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

# P5D 知识摘要（自动 harvest，来自归档文档）

> 由 `tools\harvest_md.py` 从 `docs\_archive\` 的文档中**自动抽取**「标题 + 结论行」。
> 这是**原文的索引式提炼**；需要上下文时回查原文（路径 = `docs\_archive\<原相对路径>`）。
> 本文件由脚本生成，**不要手改**。

---

## `ANALYSIS_COVERAGE.md`

**标题**：ANALYSIS_COVERAGE — 分析覆盖面评估（诚实版） ｜ 0. 直接回答 ｜ 1. 代码级覆盖（实测） ｜ 1.1 有数据支撑的部分 ｜ 1.2 三个明确的**覆盖缺口** ｜ 2. 移植层覆盖（实测） ｜ 3. 非代码层覆盖（这是移植的真正大头） ｜ 4. 运行时路径覆盖（第二薄弱环节） ｜ 5. 支撑后续移植：缺什么、按什么顺序补 ｜ 优先级 1 — 阻断级（不补就无法继续） ｜ 优先级 2 — 让移植能「正确」而非只是「能跑」 ｜ 优先级 3 — 长尾 ｜ 6. 脚本与产物（可复核） ｜ 7. 诚实边界

**发现/结论**（39 条，最多列 40）：

- > ## ⚠️ 已被 ARCH_57 修正的条目（2026-09-26）
- > 经 `ARCH_57_GAP_ANALYSIS.md` 的五条独立证据查明，**该判定错误**：
- > | 本文档原结论 | 修正后 |
- > | SEG0 尾部疑似数据（低风险排除） | **确认是 SCE 模块信息 + 导入表 + 34 库名池 + ARM 存根** |
- > 随机数据命中率 ≈1/64 = **1.56%**，与实测的 1.46% 几乎一致 —— 该判据本身不具判别力。
- > **修正后的优先级**：① 解 TITLE 加载僵死；② 补 C++ 运行库导入（SceLibstdcxx/SceLibm/SceLibc 共 60 个）；
- > ③ 确认 SceSysmem/SceCommonDialog 调用点；④ 资源字段语义；⑤ SEG1 高熵段（仅当有路径读它）。
- > **方法**：全部结论来自实测/量化，脚本与产物见 §5。**没有任何一条是估计值**，凡非实测处均标注。
- | 项 | 实测值 | 覆盖率 | 评价 |
- | 函数索引 `func_index.tsv` | 12,398 个 | — | 全部落在 **SEG0**（0x81000000+0x321810）；SEG1 命中 **0** |
- | 反编译 `true_seg0_final.c` | 12,917 函数 / 20.7 MB / 669,563 行 | — | VA 范围 0x81000000 .. 0x812B2AF0 |
- | 已知序列 | 141 个具名序列 | — | 与运行时实测创建序列逐条对应 |
- | 导入矩阵 | 381 行（170 个不同 NID） | — | ⚠️ 与移植层绑定**无法对齐**，见 §2 |
- 0x8151DA7C .. 0x8151DB96  hits=28  span=282 B
- 0x8151EBBC .. 0x8151ECD0  hits=27  span=276 B
- 0x8151F52C .. 0x8151F640  hits=27  span=276 B
- 0x81536DFC .. 0x81536F00  hits=27  span=260 B
- 0x81536F44 .. 0x81537058  hits=27  span=276 B
- 「hits≈27、span≈276 B」反复出现 ⇒ 这是一批**结构相同的代码块**（极可能是成批实例化的模板/脚本 VM 字节码或复制粘贴的模块）。它们**没有被反编译、没有函数名、没有源文件归属**。
- 范围 `0x812B2AF0 .. 0x81321810`（453,920 B = SEG0 的 13.8%）。实测：
- ⇒ **判定为数据区（风险低）**，不是漏掉的代码。这是有依据的排除，不是猜测。
- ARM 态密度很低，且 `0x81000000` 起是 Thumb 代码（`module_start` 低位 = 1）。
- | `import_matrix.tsv` 声明 | 381 行 / **170 个不同 NID** |
- **⚠️ 交集为 0 是一个真实的工具问题**：`import_matrix.tsv` 的 NID 列是**十进制**，
- 但连 `0xB110C123`（`sceKernelGetProcessTimeWide`，已确认正确绑定且运行中被调用）都**不在**其 381 行里。
- 且其 `stub_vaddr` 列**全为 `0x00000000`**。⇒ **`import_matrix.tsv` 不足以作为导入覆盖的判据**，
- **结论**：导入实现率**目前无法可信量化**。可以确定的定性事实：
- **修正建议**：直接从 `eboot.bin` 的导入表重新提取 NID（用 §ARCH_34 修正后的段偏移），
- | 层 | 状态 | 实测依据 |
- | **资源容器格式** | ✅ 容器层闭合 | GFS0：7,919 文件，段头 16B + `+0x08` BE u32 段大小 + 段首尾相接 + 末 16B 终止块，120/120 验证 |
- | **资源字段语义** | 🔴 **未验** | 只有 stride（`m001_001.GMD`=44 B、`pc200_00.GMD`=64 B），**5 个 float 的语义未知**、剩余 24 B 编码未知、索引缓冲位置未知 |
- | **顶点/索引布局** | 🟡 部分 | stride 已确证；mapping 未确证 |
- | `DEBUG MENU` → `initread` → `TITLE` | ✅ 走到 | `[STAGE]` 实测三次切换 |
- 1. **修好 SEG1 函数索引**：对 `0x81325000..0x81540574` 做一次真正的函数识别
- （用 `P5DDecompOne.java` 的框架 + Ghidra 对 SEG1 建函数），确认那 8,767 个代码簇
- 2. **重提导入 NID 并与 251 个绑定做差集**（§2），得到**可信的导入实现率**。
- 3. **解开 TITLE 加载僵死**（ARCH_56 §4.5）：35 条请求全部 pending ⇒ 主菜单之后的
- - 本文所有数字均为**实测**；凡未验证的推断（如 SEG1 代码簇的性质）已**显式标注**为待验证。
- - SEG0 尾部与 ARM 态的低密度**是排除性证据**（有量化依据），不是「确认无代码」的证明。

---

## `ARCH_0_OVERVIEW.md`

**标题**：P5D 引擎架构总览（逆向剖析） ｜ 0. 本轮剖析纠正的错误结论（最高优先级阅读） ｜ 1. 引擎整体分层 ｜ 2. 启动与主循环 ｜ 3. 当前真正的卡点（按优先级） ｜ P0-1 `0x81181CE4` 的 NULL 解引用（BADMEM） ｜ P0-2 `'movie play'` 节点不消失 ｜ P1 唤醒只唤醒一个等待者 ｜ P1 `sceKernelLockLwMutexCB`(0x3148C6B6) 绑错 ｜ P2 RT 别名 extent 不匹配 ｜ P2 媒体侧两条 P0 ｜ 4. 建议的行动顺序 ｜ 5. 模块文档索引

**发现/结论**（51 条，最多列 40）：

- > 本总览汇总 6 份模块文档，并**优先记录本轮剖析纠正掉的错误结论**（那些是我此前基于片面探针数据下的判断）。
- > 证据规则：映射 `file_off = VA - 0x81000000 + 0x10E0`；反汇编用
- 这些是此前几轮我据以行动的判断，**已被反汇编推翻**。若不先纠正，后续工作会继续走错方向。
- | # | 我此前的结论 | 实际（附证据） | 影响 |
- | 1 | `0x811982A4` = 序列结束（`movs r0,#0;pop`） | 它是 **保活**（返回 0 表示"继续"）。真结束是 `0x81198294` 的 `movs.w r0,#-1`，调度器判据在 `0x811DB250 cmp.w r3,#-1; beq` | **`[TSEQ-SKIP]` 把 state 强写 8 根本不会让 title 序列完成**；它跳过了 `0x81198276 bl 0x81193A80`(清理) 与 `0x811982
- | 3 | `0x8116F900` 返回 `[node+0xC]` | 它返回**节点指针**；`[node+0xC]` 只是被取出经 `+0x14` 复核名字串；C5/C6 的二次解引用是 `[node+8]`(work) | 之前对 C5/C6 的字段语义判断全错 |
- | 5 | 代泵 `[PUMP-DONE]` 表示"加载请求真的完成了" | `0x812B4150` 是**栈金丝雀常量**；代泵只看返回值真值就置完成位，**把一次 BADMEM 崩溃误判成"加载成功"** | `PENDING` 清空是假象，实际是崩溃 |
- | 6 | 卡点是"序列不就绪 / 死锁" | 本轮真正的停机是 **BADMEM**：`_h1164.log:146553 stop=3(BADMEM) last_pc=0x81181CFA`；`0x81181CE4` 无 null 检查地执行 `ldr r0,[r3,#0x94]` 而 `r3=0` | 之前所有"死锁/唤醒"分析的方向需要重估 |
- │ 状态表 [0x813B11AC]，步长 0x50，+0x44=enter，+0x48=update   │
- │   state=1 update=0x81185927                                 │
- │   state=6 update=0x81185986 → 0x811980fc → 0x8116f900(...)  │
- │ 帧边界唯一信号：sceGxmDisplayQueueAddEntry (NID 0xEC5C26B5) │
- ┌─ 序列/任务框架（引擎的"场景系统"）★ 当前卡点所在 ───────────┐
- │ 创建 0x8116F7CA | 按名查 0x8116F900 | 取 work 0x8116F8A8    │
- │ 调 step 0x8116F748 | 销毁 0x8116F75E                        │
- │ **每帧调度器循环 0x811DB1F0**；结束判据 0x811DB250 (r3==-1)  │
- │ 名字哈希 0x81165288 = Σ s[i] << (i&7)   （4 个实测值已校验）  │
- │ 建 req 0x8117BF02 | 入口 0x8117C0EE / 0x8117C1BC            │
- │ worker 0x8117BA30(ctx,req) | 按类型派 handler 0x8117BB8A    │
- │   → 0x8117C8A1 / 0x8117C7E7(sprite) / 0x8117CA5F            │
- │ ARC 查找 0x81174D64                                          │
- │ ★ 完成位由**调用方**置：0x8117C98A `orrs r0,r0,#1`          │
- │ job worker 入口 0x811E68C8                                   │
- │   WaitSema 0x811E68F2 | Lock 0x811E68FE | 取作业 0x811E6902  │
- │   调作业 0x811E6934 | pending-- 0x811E693C | 退出 0x811E694E │
- │ 池构造 0x811E603E | pump 0x811E6966（排空 SetEventFlag）      │
- │ 同步派发 A 0x811E63xx (SignalSema→WaitEventFlag)             │
- │ 异步派发 B 0x811E64Cx                                        │
- │ guest: sceGxmDraw 桩 0x812B15F0，调用点 0x811E45B6/0x811EC006│
- │ 实测每帧仅 ~2.8 笔 / 5 种几何 / 3 管线                       │
- │ 任务机 0x8115849E → openMovie 0x811588F6                     │
- │ 内层状态 0x81158FA0（状态源 0x8115423C）→ 外层 0x811589E8     │
- │ 解码任务 0x812A9FAE | gate 0x812A9FE8 | AU 组装 0x812AA1C6   │
- │ AU 环 push 调用者 0x812A9A50                                 │
- - `module_start = 0x81198464` → `bl 0x811982bc` 跳板，把协作式调度器与 app 状态机跑起来。
- - app 状态机驱动：状态表 `[0x813B11AC]`，步长 `0x50`，`+0x44`=enter、`+0x48`=update，
- 调用点 `0x8118586E blx r0`。
- - **帧边界**：`sceGxmDisplayQueueAddEntry`(NID `0xEC5C26B5`) 是唯一的每帧信号（探针 `[PROG]`/`[DETIME]` 挂在这里）。
- - 410 个导入全部绑到 SVC `#0xAB` 桩；实测被调用 196 个；`impl=N` 去重后 16 个恒返 0，
- 最大的是 `SceAppUtil/0x6A140498`（59 次）。

---

## `ARCH_10_gxm_full.md`

**标题**：ARCH_10 — SceGxm 100 个导入的全景 ｜ 0. 证据来源清单（本文档实际读取/执行过的） ｜ 0.1 「实际调用次数」的口径 ｜ 0.2 ★ 一个必须先纠正的前提：`sce_gxm.cpp` 的实现对本移植**不生效** ｜ 0.3 桩格式与「未绑」的语义（核证） ｜ 1. 完整表：SceGxm 100 个导入全景 ｜ 1.1 26 个「未绑」导入清单（会静默 return 0） ｜ 2. 重点类别逐个核证 ｜ 2.1 depth / stencil 相关 —— ❌ **完全没有覆盖，且是高频路径** ｜ 2.2 Clear / Resolve / Store —— ❌ 部分缺失，但**当前不是黑屏主因** ｜ 2.3 ★ `sceGxmTransfer*` 系列 —— ✅ **本游戏确实没用；可暂缓** ｜ 2.4 MidScene / EndScene —— ✅ **本作不用 MidSceneFlush** ｜ 2.5 SyncObject（GPU 同步对象）—— ⚠️ **创建了 6 个，全部被丢弃** ｜ 2.6 RenderTarget 创建与参数 —— ⚠️ **参数被丢弃，且发现一个 1088×1088 的 RT** ｜ 2.7 Texture 参数（swizzle / stride / format 全表） ｜ 2.8 shader 参数（SetUniformDataF / SetTextures / Reserve*UniformBuffer） ｜ 2.9 ★ Precomputed state 系列 —— ⚠️ **init/set 部分实现了，但「提交」从不发生** ｜ 2.10 ★ Command list 系列 —— ✅ **本作根本没有导入，不存在这条路** ｜ 3. ★ 关键问题：是否存在「非 `sceGxmDraw` 的绘制提交路径」？ ｜ 3.1 结论：**不存在。本作的唯一绘制提交路径就是 `sceGxmDraw`，且它有 5 个调用点。** ｜ 3.2 ★ 但 **ARCH_4 的「两个调用点」是错的**：实测有 **5 个** ｜ 3.3 对 ARCH_4「引擎从不提交游戏内容几何」的重新解释 ｜ 4. 对「能否走到并打完一局」的影响评估 ｜ 4.1 判定口径

**发现/结论**（152 条，最多列 40）：

- > 本文档的唯一任务：把 **SceGxm 这个最大导入库（100 个 NID）**逐个钉死——
- > 「导入没导入 / 绑没绑 / 实现没实现 / 被调用没有 / 用什么证据证明」。
- > **证据规则（本文档每条结论都遵守）**
- > 1. 映射 `file_off = VA - 0x81000000 + 0x10E0`（`_p5d_dis.py` 头部注释，已在 `eboot.bin` 上复核，见 §0.3）。
- > 2. 反汇编命令：`py C:\dev\p5d_build\_p5d_dis.py <十六进制VA> <字节数>`。
- > 5. 日志给「行原文」；反汇编给「VA + 指令原文 + 字节」。
- | S1 | `C:\dev\p5d_build\gxm_imports.txt` | 101 行 | **权威导入清单**：`# SceGxm imports total=100` + 100 条 `NID impl=0` |
- | S2 | `C:\dev\p5d_build\veneer_map.txt` | 410 行 | stub VA ↔ (lib, NID) 映射；其中 `SceGxm` 恰好 **100** 条（实测 `Select-String … "SceGxm"` 计数=100） |
- | S3 | `C:\dev\p5d_build\v3k_nids.inc` | 321 KB | 官方 NID → 函数名（`NID(sceGxmDraw, 0xBC059AFC)` 格式） |
- | S5 | `C:\dev\p5dsrc\src\gxm\sce_gxm.cpp`（1752 行） | 1482-1752 | 267 条 GXM NID 表 + `STUB_LOG` 实现（**是否生效见 §0.2**） |
- | S6 | `C:\dev\p5dsrc\src\arm\import_bridge.cpp/.h` | 全文 | 桩格式、`impl=` 语义、未绑导入的返回语义 |
- | S9 | `C:\dev\p5d_build\_h1164.log`（12.6 MB，最新） | 全文检索 | **全部实测调用次数与行为** |
- | S15 | `C:\dev\p5dsrc\ARCH_4_render.md` / `ARCH_0_OVERVIEW.md` / `ARCH_11_usse.md` | 已读 | 复用已实证结论，避免重复 |
- [NIDCALL] ===== 195 imports with count>=1 =====
- [NIDCALL]      1005642 impl=Y func=0x91FA6614 lib=0xCAE9ACE6  SceLibKernel/func=0x91FA6614
- - 过滤 `lib=0xF76B66BD`（= SceGxm 的 library NID）得 **67 行** ⇒ **100 个 SceGxm 导入中只有 67 个被调用过**。
- | 反证：`[NIDCALL] func=0xBC059AFC` 计数 17992，与 `eboot_probe.cpp` 自己 `n_draw` 的计数器 `g_draw` 完全相等（`[DRAWHIST frame=6060] totalDraw=17970`、`[RENDER-TOTAL] … Draw=17992`） | `_h1164.log` + `eboot_probe.cpp:1912` |
- **结论（有证据）**：`sce_gxm.cpp` 那 267 条 NID 表与它的 `STUB_LOG` 函数体，是给
- > 这条纠正很重要：`sce_gxm.cpp` 里 `sceGxmDrawInstanced / sceGxmDrawPrecomputed /
- 运行时被 `write_stub` 覆盖成 Thumb 陷阱。实测（`file_off = VA-0x81000000+0x10E0`）：
- 812B15F0 sceGxmDraw        file=0x2B26D0 bytes=00 00 e0 e3 1e ff 2f e1   (= E1A00000 mov r0,r0 ; E12FFF1E bx lr)
- 812B1740 DrawPrecomputed   file=0x2B2820 bytes=00 00 e0 e3 1e ff 2f e1
- 812B1390 TransferCopy      file=0x2B2470 bytes=00 00 e0 e3 1e ff 2f e1
- 812B1440 TextureSetData    file=0x2B2520 bytes=00 00 e0 e3 1e ff 2f e1
- （命令：`py -c "f=open('eboot.bin','rb').read(); print(f[0x2B26D0:0x2B26D8].hex(' '))"`）
- ⇒ **静默返回 0（= SCE_OK）**。这是本文档「未绑」的准确语义：**不崩，但会让 guest 以为成功**。
- - `调用次数` = `_h1164.log` 末尾 `[NIDCALL] … lib=0xF76B66BD` 的实测值；
- `0` = 该 NID **未出现在** 195 行结算表中（该表只列 count≥1 者，见 §0.1）。
- - `证据` 列：`bind@NNNN` = `eboot_probe.cpp` 行号（`Select-String '^\s*bind_by_nid'`）。
- 所有行另有共同证据：`gxm_imports.txt`（在 100 清单内）+ `veneer_map.txt`（stub VA）。
- 调用次数非 0 者另有共同证据：`_h1164.log` 的 `[NIDCALL] … lib=0xF76B66BD` 行原文。
- | NID | 推测名（`v3k_nids.inc`） | stub VA | 我们的实现方式 | 调用次数 | 证据 |
- | `11DC8DC9` | `sceGxmTextureInitCube` | `812B1220` | 真实现（`tex_init(0x40000000,…)`） | **0**（已绑未调） | `bind@6536`；`n_tex_init_cube@876` |
- | `D572D547` | `sceGxmTextureInitSwizzled` | `812B1680` | **真实现**（`tex_init(0x20000000,…)`） | 36 | `bind@6533`（6545 的 `n_gxm_ok` 覆盖版**已被注释删除**，见 §2.8） |
- | `E6F0DB27` | `sceGxmTextureInitTiled` | `812B16E0` | **真实现**（`tex_init(0x80000000,…)`） | 2 | `bind@6535`（6546 的覆盖版已注释删除） |
- **小计**（实测计数脚本 `_gxm_table2.csv`）：
- | 类别 | NID（推测名） |
- | 检查项 | 结论 | 证据 |
- | 游戏传了吗 | **传了**，`0x816CBD64` | `[BSCOLOR#1] rt=A0E1D000 surf=816CBCC4 flags=2 w=960 h=544 stride=1024 data=A1300000 fmt=0 type=0 depth=816CBD64`（`_h1164.log`）；读取代码 `eboot_probe.cpp:1624 _dsurf = gxp_rd(_sp2+12u)` |
- | 深度面建立了吗 | **没建立**。`sceGxmDepthStencilSurfaceInit` 绑成 `n_gxm_ok`（no-op，`eboot_probe.cpp:6502`），调用 4 次 | 4 次全靠 `[NIDCALL]  4 impl=Y func=0xCA9D41D1` |

---

## `ARCH_11_usse.md`

**标题**：ARCH_11 — USSE 着色器指令集解码 + GXM 着色器参数供给 ｜ 1. 本子系统职责（一句话） ｜ 2. 导入 / 关键函数地址表 ｜ 2.1 SceGxm 着色器相关导入（NID 与 Thumb SVC 桩 VA） ｜ 2.2 host 侧 USSE/GXP 处理函数 ｜ 3. 关键数据结构：字段偏移 + 含义 ｜ 3.1 GXP 程序头（`SceGxmProgram`，magic `0x00505847` = `"GXP\0"`） ｜ 3.2 参数表项 `SceGxmProgramParameter`（16 B） ｜ 3.3 USSE 64 位指令字 ｜ 3.4 `g_vfmt`：权威顶点属性表（host 侧探针结构） ｜ 3.5 `SceGxmPrecomputedDraw` state（44 B，host 侧按值复制） ｜ 3.6 GXM 默认 UBO → Vulkan 的映射约定（**本模块最容易出错的一处**） ｜ 4. 生命周期与调用关系（调用链） ｜ 4.1 GXP → SPIR-V 的**两条**路径（离线为主，运行期为辅） ｜ 4.2 运行时**实际执行**的 12 支着色器的 USSE 全文（本次实测） ｜ 4.3 一帧的着色器参数供给链（实测顺序） ｜ 5. 当前 HLE 实现状态 ｜ 5.1 USSE 译码器：**完整覆盖（UNKNOWN = 0.00%）** ｜ 5.2 USSE→SPIR-V 翻译器：**实际支持面 + 静默丢弃清单** ｜ 5.3 哪些是空桩 / 被静默吞掉 ｜ 5.4 GXM 着色器参数供给路径：现状与缺口 ｜ ✅ 已工作（有实测证据） ｜ ❌ 缺口（按严重度） ｜ 5.5 缺口清单（可直接执行的收敛表）

**发现/结论**（87 条，最多列 40）：

- > 本文档的每一条结论都附**源码行号（含原文）**、**日志行原文（含日志文件名）**、
- > **工具实测输出（含命令）** 或 **反汇编/反汇编器原文**。
- > 源码注释，是此前已实证结论，本文档直接复用并标注出处行号。
- **证据来源（本文档实际读取/执行过的）**
- | 2 | `src/usse/usse_translator.h` / `.cpp`（363 / 3556 行） | 关键段 | USSE→SPIR-V 翻译器的**实测**支持面 |
- | 8 | 日志 `_h1164.log`（12.6 MB，最新，147523 行） | 全文检索 | 全部实测读数 |
- | 11 | SPIR-V 反汇编 `dump_spv.py` + `spv_all/gxp_vert_0015.gxp.spv` | — | 离线产物的 UBO/SA 地址链核证 |
- > ⚠ **与任务书描述的三处出入（以本文档实测为准）**
- > 2. 任务书希望从日志找 `[EHMAP]/[FHMAP]`。**实测各只有 6 条**（不是每程序一条），因为运行时只用到
- 一句话的三段证据（本工程里三段各有独立代码路径）：
- 桩地址取自 `C:\dev\p5d_build\veneer_map.txt`（格式 `stubVA lib=… nid=… SceGxm`），
- | NID | 桩 VA | 推测名 | 作用 | 被调用次数（`_h1164.log`） | 证据 |
- | `0xB7BBA6D5` | `0x812B15C0` | `sceGxmShaderPatcherCreateVertexProgram` | 创建顶点程序；**顺带捕获权威顶点属性表** `g_vfmt[hash]` | 200（打印上限）| `veneer_map.txt`：`812B15C0 lib=F76B66BD nid=B7BBA6D5 SceGxm`；`eboot_probe.cpp:6457 bind_by_nid(0xB7BBA6D5u,n_creat
- | `0x4ED2E49D` | `0x812B1350` | `sceGxmShaderPatcherCreateFragmentProgram` | 创建片段程序；捕获 `SceGxmBlendInfo` → `g_blend_by_gxp` | ≥6（每支片段各 1） | `veneer_map.txt`：`812B1350 … nid=4ED2E49D`；`eboot_probe.cpp:6458`；`vulkan_backend.cpp:1414-1471` 消费 
- | `0x31FF8ABD` | — | `sceGxmSetVertexProgram` | 记 `g_cur_vgxp` | 17970（= 每笔 draw 一次）| `eboot_probe.cpp:6562`；`[DRAWHIST frame=6060] totalDraw=17970 real=17970` |
- | `0xAD2F48D9` | — | `sceGxmSetFragmentProgram` | 记 `g_cur_fgxp` | 17970 | `eboot_probe.cpp:6563`；同上 |
- | `0xC697CAE5` | — | `sceGxmSetVertexDefaultUniformBuffer` | 记引擎默认顶点 UBO `g_vdefubo` | 18+（`[SETVDEFUBO]` cap=24）| `eboot_probe.cpp:6568`；日志 `[SETVDEFUBO#1..#18]` |
- | `0xA824EB24` | — | `sceGxmSetFragmentDefaultUniformBuffer` | 记默认片元 UBO | 3（`[SETFDEFUBO]`）| `eboot_probe.cpp:6567`；日志 `[SETFDEFUBO]` 3 行 |
- | `0x65DD0C84` | `0x812B13B0` | `sceGxmSetUniformDataF` | **唯一**的命名 uniform 写入口 | 132（记录窗口内）| `veneer_map.txt`：`812B13B0 … nid=65DD0C84`；`eboot_probe.cpp:7028`；`_h1164.log` `[SETUNI*]` 132 行 |
- | `0x895DF2E9` | — | `sceGxmSetVertexStream` | 记 `g_vstream[stream]` | 大量（`[VSET]` / `[GXPPARM]` 1631）| `eboot_probe.cpp:6564`；`[VSET] va=AE600000 armed=1 lr=811E7D60` |
- | `0x97118913` | `0x812B14F0` | `sceGxmReserveVertexDefaultUniformBuffer` | 分配一块 4 KB **清零** UBO 并回写指针 | （未直接计数）| `veneer_map.txt`：`812B14F0 … nid=97118913`；`eboot_probe.cpp:6560` → `:616-621 n_gxm_reserve_ubo`（`heap_alloc(0x1000,true)`）|
- | `0xC68015E4` | — | `sceGxmSetVertexUniformBuffer(ctx,index,data)` | 记 `g_vubo_bind[index]` | 大量 | `eboot_probe.cpp:6565` + `:5039-5047`；日志 `[SETVUBO#1..6] ctx=0xA0AC4000 idx=0 data=0xAA700000` |
- | `0xA197F096` | — | `sceGxmPrecomputedDrawInit` | 初始化 `SceGxmPrecomputedDraw` state（44 B 清零）| 6 | `eboot_probe.cpp:6492` + `:672-679`；日志 `[PDINIT #0..#5]` |
- | `0x6C936214` | — | `sceGxmPrecomputedDrawSetVertexStream` | 往 state 的流数组写指针 | 6 | `eboot_probe.cpp:6495` + `:693-703`；日志 `[PCVSTREAM #0..#5]` |
- | `0xB452F1FB` | — | `sceGxmPrecomputedFragmentStateSetUniformBuffer` | precomputed 路径的命名 UBO | — | `eboot_probe.cpp:6558` + `:1898`（`if(idx==0u&&data) g_fubo=data;`）|
- **关键读数**：`[DRAWCALLER]` 只出现 `0x811E45B6`/`0x811EC006` 两个调用点（ARCH_4 §2.1），
- 且 `[DRAWPRE]` 在 `_h1164.log` 中 **只有 1 条** ⇒ **precomputed 绘制路径几乎不用**，
- 但 precomputed **state**（`0xA335B9A4` 一族）被 `[SPFS#0] ctx=0xA0AC4000 state=0xA335B9A4` 用作
- | 偏移 | 含义 | 证据 |
- | `+0x00` | magic `0x00505847` | `gxp_to_spv.cpp:40` `memcmp(d.data(),"GXP\0",4)`；`eboot_probe.cpp:7775` `magic=gxp_rd(gxp+0x00)` |
- | `+0x08` | `size`（总字节数，实测 204..445）| `eboot_probe.cpp:7775`；`_rt_frag_5D73281E76A107B2.gxp size=204` |
- | `+0x14` | `flags`；`bit0=1` ⇒ FRAGMENT，`=0` ⇒ VERTEX | `gxp_to_spv.cpp:334` `bool frag=(flags&1)!=0`；dump 行 `size=445 VERTEX` |
- | 偏移 | 含义 | 证据 |
- | 位 | 含义 | 证据 |
- | 字段 | 含义 | 证据 |
- | `stride[stream]` | **权威** stride，取自 `SceGxmVertexStream{ u16 stride; u16 indexSource; }`（覆盖推算值）| `:336-343`（h144k1）|
- **实测样例**（`_h1164.log` `[V14ATTR]`，逐字）：
- [V14ATTR] eh=ABE1E100963724B0 size=2 strides[0..3]=16,0,0,0 stream0=0xA2D501E0: [0 st0 off0 fmt9 cc3 reg0->loc0] [1 st0 off12 fmt4 cc4 reg4->loc1]
- ⇒ `vert_0014` = 单流 stride=16；`loc0` = `F32×3@0`（位置）；`loc1` = `U8N×4@12`（颜色）。
- **⚠ 已实证缺口**：`[V15ATTR]`（`eboot_probe.cpp:3681-3689` 专为

---

## `ARCH_12_audio_full.md`

**标题**：ARCH_12_audio_full — 模块：SceNgs(34) + SceAudio(5) 音频全景 ｜ 1. 本子系统职责（一句话） ｜ 2. 导入/关键函数地址表 ｜ 2.1 两个库的规模与官方 NID 归属（权威来源） ｜ C:\dev\p5d_build\veneer_map.txt （按 stub VA 排序，节选） ｜ 2.2 SceAudio 5 个导入（逐个） ｜ 2.3 SceNgs 34 个导入（逐个） ｜ 2.4 静态调用点全图（本轮新扫，含 BLX） ｜ 3. 关键数据结构 ｜ 3.1 NGS 系统对象（HLE 自造，`0x6000` 字节） ｜ 3.2 NGS voice 定义块（`n_ngs_def` 返回值） ｜ 3.3 NGS voice 参数块（`sceNgsVoiceLockParams` 的 out） ｜ 3.4 guest 侧「播放位置」数据结构（**这是音游同步的核心**） ｜ 3.5 NGS 系统 init 参数块（guest 在栈上构造后传给 SystemInit） ｜ 3.6 voice 句柄 out 槽的**未初始化**问题（最危险） ｜ 4. 生命周期与调用关系（调用链） ｜ 4.1 音频线程的创建（启动早期，与主菜单无关） ｜ 4.2 音频泵 `0x8125BC8C`（每帧/每音频块） ｜ 4.3 guest 侧 NGS 装配/起播链（本轮新解出的静态调用链） ｜ 4.4 与「场景推进」的关系（负面结论） ｜ 5. 当前 HLE 实现状态 ｜ 5.1 绑定了哪些、怎么实现的（`eboot_probe.cpp`） ｜ 5.2 SceAudio 侧实现 ｜ 5.3 未实现的 2 个 NID 的具体后果

**发现/结论**（298 条，最多列 40）：

- > 目标二进制：`C:\dev\p5d_build\eboot.bin`（FSELF，内嵌 ELF @0x1000，加载基址 `0x81000000`，
- > segment `0x81000000..0x816CF4A0`，imports=410）
- > 映射：`file_off = VA - 0x81000000 + 0x10E0`（已用 `py _p5d_dis.py check` 自校验通过：
- > **本文件的 8 处证据来源**（满足「至少 5 处」）：
- > 2. `C:\dev\p5d_build\veneer_map.txt`（SELF loader 落盘的 **stub VA ↔ libNID ↔ funcNID** 全表；生成点 `src/loader/self_loader.cpp:576-585`）
- > 3. `C:\dev\p5d_build\v3k_nids.inc`（Vita3K 官方 NID→函数名库，本文件所有「推测名」均由它反查，非猜测）
- > 8. 现场反汇编：`py C:\dev\p5d_build\_p5d_dis.py <VA> <条数>` + 本轮新写的 callers 扫描器
- | 项 | 值 | 证据 |
- | SceAudio 库 | `libNID=0x438BB957, funcs=5, vars=0` | `_h1164.log:40` `[INFO]   Import lib: SceAudio (libNID=0x438BB957, funcs=5, vars=0)` |
- | SceNgs 库 | `libNID=0xB01598D9, funcs=34, vars=0` | `_h1164.log:41` `[INFO]   Import lib: SceNgs (libNID=0xB01598D9, funcs=34, vars=0)` |
- | 函数名反查 | 全部 39 个 NID 均在 Vita3K `SceAudio`/`SceNgsUser→SceNgs` 库段内逐一命中 | `v3k_nids.inc:310-327`（SceAudio）、`:5293-5353`（SceNgs） |
- stub VA 与 NID 的一一对应来自 SELF loader 落盘的 veneer 全表（`_h1164.log:40-41` 是库级，
- ...（SceNgs 共 34 条，0x812B1F00 起每 0x10 一条，至 0x812B2110）
- ⇒ **stub 页布局 = 每导入 16 字节，SceAudio 占 `0x812B1EB0..0x812B1EFF`，紧接着 SceNgs 占
- `0x812B1F00..0x812B211F`**（`thumb_interpreter.cpp` 里所有「`pc=0x812B1EB0`」式观测点即指此）。
- > 「调用次数」= `_h1164.log:146922-147083` 的 `[NIDCALL]` 表（门槛 `count>=1`，
- > 表头原文 `_h1164.log:146909 [NIDCALL] ===== 195 imports with count>=1 =====`）。
- | # | stub VA | NID | 推测名（v3k_nids.inc 反查） | 作用 | 调用次数 | HLE 实现 | 缺口 |
- | 1 | `0x812B1EB0` | `0x02DB3F5F` | `sceAudioOutOutput` | 提交一块 PCM 给输出口；**真机阻塞到下一音频块** | **36441** | `n_audioout_output`：首调用 `sched_yield()` 让出一个时间片，重放直接返 0（`eboot_probe.cpp:6297-6304`，绑定 `:6429`） | **不出声**；只是节拍器 |
- | 2 | `0x812B1EC0` | `0x12FB1767` | `sceAudioOutGetAdopt` | 查询输出口当前「可接受样本数/采纳状态」 | **21686** | `n_noop`（恒返 0，不写 out；绑定 `eboot_probe.cpp:6430`） | guest 读到 out 为**栈残留** |
- | 3 | `0x812B1ED0` | `0x5BC341E4` | `sceAudioOutOpenPort` | 开输出口 `(type, len, freq, mode)` | **1** | `n_audio_openport`：恒返 `1u` 当端口 id（`eboot_probe.cpp:1572`，绑定 `:6617`） | 不建任何真实端口/流 |
- | 4 | `0x812B1EE0` | `0x64167F11` | `sceAudioOutSetVolume` | 设端口音量 | **0**（本次未调用，但**有 1 个静态调用点** `0x8125BDAA`） | **未绑定** → 走默认桩恒返 0 | 无实现；`src/audio/sce_audio.cpp:31-34` 有同名函数但**未被编译进 probe**（见 §5.4） |
- | 5 | `0x812B1EF0` | `0x69E2E6B5` | `sceAudioOutReleasePort` | 关输出口 | **0**（静态调用点 `0x8125C000`） | **未绑定** → 默认桩恒返 0 | 无实现；`sce_audio.cpp:19-22` 同样未编译进 probe |
- 其中 `0x02DB3F5F` 是**全局第 2 高频**导入（仅次于 lwmutex 三桩）：
- `_h1164.log:146896 [SVCHOT] stub=0x812B1EB0 nid=0x02DB3F5F SceAudio/func=0x02DB3F5F count=36441`。
- | # | stub VA | NID | 推测名（v3k_nids.inc 反查） | 作用 | 调用次数 | HLE 实现 | 缺口 |
- | 1 | `0x812B1F00` | `0x0A92E4EC` | `sceNgsRackInit` | 初始化一个 rack（混音机架） | 11 | `n_ngs_ok` 返 0（`:6593`） | 不写 rack 句柄/状态 |
- | 2 | `0x812B1F10` | `0x0E0ACB68` | `sceNgsVoiceDefGetCompressorBuss` | 取「压缩器总线」voice 定义 | 0 | **未绑定** | 返回 0（NULL）→ 调用方拿到空定义 |
- | 3 | `0x812B1F20` | `0x0E291AAD` | `sceNgsVoiceKill` | 强杀 voice | 0 | **未绑定** | 无声停止语义 |
- | 4 | `0x812B1F30` | `0x14EF65A0` | `sceNgsVoiceDefGetAtrac9Voice` | 取 ATRAC9 解码 voice 定义 | 11 | `n_ngs_def` = `heap_alloc(0x200,true)` 清零块（`:1570`, `:6590`） | **不是真的定义表**，模块数/回调全 0 |
- | 5 | `0x812B1F40` | `0x24E909A8` | `sceNgsVoiceSetModuleCallback` | 注册模块回调 | 17 | `n_ngs_ok`（`:6600`） | 回调永不触发 |
- | 6 | `0x812B1F50` | `0x3D46D8A7` | `sceNgsVoiceUnlockParams` | 提交参数（真机在此把参数送进 DSP） | 18 | `n_ngs_ok`（`:6616`） | **参数永不下发** |
- | 7 | `0x812B1F60` | `0x477318C0` | `sceNgsRackGetRequiredMemorySize` | 询问 rack 需要多少内存 | 55 | `n_ngs_needsize` 恒返 `0x6000`（`:5673`, `:5676`） | **恒返非 0 且不写 out 尺寸**，调用方走 fallback（见 §3.5） |
- | 8 | `0x812B1F70` | `0x4A25BEBC` | `sceNgsSystemRelease` | 释放 NGS 系统 | 10 | `n_noop`（`:5677`） | 不回收 `0x6000` 块（泄漏 11×0x6000） |
- | 9 | `0x812B1F80` | `0x4CBE08F3` | `sceNgsVoiceGetParamsOutOfRange` | 查越界参数告警 | 0 | **未绑定** | — |
- | 10 | `0x812B1F90` | `0x4D705E3E` | `sceNgsVoiceDefGetDelayBuss` | 取「延迟总线」定义 | 0 | **未绑定** | — |
- | 11 | `0x812B1FA0` | `0x54CFB981` | **`sceNgsVoiceResume`** | 恢复被暂停的 voice | **3** | **`impl=N` 未实现** | 见 §5.3-①（告警原文 `_h1164.log:81184`） |
- | 12 | `0x812B1FB0` | `0x5551410D` | **`sceNgsVoiceGetInfo`** | 取 voice 信息结构 | **4** | **`impl=N` 未实现** | 见 §5.3-②（告警原文 `_h1164.log:81195`） |
- | 13 | `0x812B1FC0` | `0x684F080C` | `sceNgsSystemUpdate` | **推进整个 NGS 混音一帧**（真机在此产出 PCM） | **18323** | `n_noop`（`:6431`） | **音频从不推进**（ARCH_5 已结论） |
- | 14 | `0x812B1FD0` | `0x6CE8B36F` | `sceNgsSystemGetRequiredMemorySize` | 询问系统内存需求 | 11 | `n_ngs_needsize` 恒返 `0x6000` | 同上，不写 out（见 §3.5 反汇编 `0x8125C3D6`） |

---

## `ARCH_13_np_net.md`

**标题**：ARCH_13 — PSN 与网络子系统（SceNpScore / SceNpTrophy / SceNpTus / SceNpManager / SceNpActivity / SceNet / SceNetCtl / SceHttp / ｜ 1. 本子系统职责（一句话） ｜ 2. 导入/关键函数地址表 ｜ 2.1 导入表原始取证（43 个 NID 全部实读自 eboot.bin） ｜ 解析命令：py C:\dev\p5d_build\_arch13_probe.py ｜ 2.2 逐函数表（43 个） ｜ 2.3 必须一起算进来的 4 个「奖杯设置对话框」导入（库挂在 SceCommonDialog） ｜ 2.4 关键中间函数地址表（guest 侧，本节全部为本轮新证） ｜ 3. 关键数据结构 ｜ 4. 生命周期与调用关系 ｜ 4.1 `netInit`（0x81058CA0）——一次性握手，全链路顺序固定 ｜ 4.2 `netConnectState`（体 @0x81058EE8）——真正的「连接判定」 ｜ 4.3 `netTrophyProc`（0x8105A260）——7 态状态机，**本轮唯一实际跑完的 PSN 握手** ｜ 4.4 任务级调用关系 ｜ 5. 当前 HLE 实现状态 ｜ 5.1 已绑（41 / 43） ｜ 5.2 奖杯对话框的「离线降级」实现（唯一有真实逻辑的一处） ｜ 5.3 空桩 / 缺口清单 ｜ 6. 对「能否走到并打完一局」的影响评估 ｜ 判定：**次要项（当前不是阻塞项），但有 2 处必须在进入「联机相关 UI」之前修掉，否则由次要项升级为阻塞项。** ｜ 结论一句话 ｜ 7. 未解问题清单（按优先级） ｜ 附录 A — 本轮新增取证脚本 ｜ 附录 B — 与既有文档的交叉引用

**发现/结论**（211 条，最多列 40）：

- > 分析对象：《Persona 5 Dancing in Starlight》(PCSH10109) `eboot.bin`（ARM32 Thumb，加载基址 `0x81000000`，imports=410）。
- > 映射：`file_off = VA - 0x81000000 + 0x10E0`；反汇编入口 `py C:\dev\p5d_build\_p5d_dis.py <VA> <条数>`。
- > 本轮新增取证脚本：`C:\dev\p5d_build\_arch13_probe.py`（导入表解析）、`_arch13_names.py`（NID→函数名）、`_arch13_callsites.py`（BL/BLX 调用点扫描）、`_arch13_imm.py`（立即数引用扫描）。
- > **证据规则**：每条结论附反汇编行（VA + 指令原文）或日志原文（文件:行号）。无法确定的一律标「推测」。
- export 0x812B2B5C..0x812B2B7C import 0x812B2B7C..0x812B3298
- [INFO]   Import lib: SceNet (libNID=0x6BF8B2A2, funcs=1, vars=0)
- [INFO]   Import lib: SceNetCtl (libNID=0xC6083AE1, funcs=1, vars=0)
- [INFO]   Import lib: SceNpManager (libNID=0xD8835093, funcs=6, vars=0)
- [INFO]   Import lib: SceNpActivity (libNID=0xFEF492ED, funcs=3, vars=0)
- [INFO]   Import lib: SceHttp (libNID=0xE8F15CDE, funcs=1, vars=0)
- [INFO]   Import lib: SceSsl (libNID=0xCAF5F2E9, funcs=1, vars=0)
- [INFO]   Import lib: SceNpScore (libNID=0x75F052DB, funcs=12, vars=0)
- [INFO]   Import lib: SceNpTrophy (libNID=0x4332C10D, funcs=5, vars=0)
- [INFO]   Import lib: SceNpTus (libNID=0xAE84BDEA, funcs=13, vars=0)
- > ⚠ 修正一处长期存在的库归属混淆：题目给出的 `nid_resolver.h:43-51` 库 NID 常量表**错位**——
- > 源码把 `SceNet=0x3AD4D630`（实际是 mceFios2）、`SceNpManager=0xF88DAA70`（实际是 SceFiber）、
- > `SceNpActivity=0xD8835093`（实际是 SceNpManager）、`SceHttp=0xFEF492ED`（实际是 SceNpActivity）、
- > `SceSsl=0xE8F15CDE`（实际是 SceHttp）、`SceNpScore=0xCAF5F2E9`（实际是 SceSsl）、
- > `SceNpTrophy=0x75F052DB`（实际是 SceNpScore）、`SceNpTus=0x2DD9A62D`（实际是 SceGameUpdate）。
- 「被调用次数」取自本轮实测的 `[NIDCALL]` 汇总（`_h1164.log:146964-147103`；`impl=N` 表示**未绑定的空桩**）。
- 「静态调用点」取自 `_arch13_callsites.py` 的 BL/BLX 扫描（线性扫描 0x81000000..0x81321810，命中 7538 处导入调用中的 42 处指向本子系统）。
- | # | NID | 推测名 | 作用 | 动态次数 | 静态调用点（证据） |
- | 1 | `0xEB03E265` | `sceNetInit` | 网络栈初始化 | 1 | @0x81058CD0 |
- | 2 | `0x495CA1DB` | `sceNetCtlInit` | 网络状态控制初始化 | 1 | @0x81058CD8 |
- | 3 | `0x04D9F484` | `sceNpInit` | PSN 初始化 | 1 | @0x81058D12 |
- | 4 | `0x3B0AE9A9` | `sceNpCheckCallback` | 推进 PSN 内部回调泵 | **7567**（h1163）/ 3089（h1164） | @0x81058C98, 0x81058EB8, 0x81058F52, 0x810590B8 |
- | 5 | `0x3C94B4B4` | `sceNpManagerGetNpId` | 取本机 NP 账号 ID | **0**（从未执行） | ×10：0x81059076, 0x8105982A, 0x810598BA, 0x8105A5B6, 0x8105A6AE, 0x8105A730, 0x8105A7B8, 0x8105A8A0, 0x8105A994, 0x8118FA44 |
- | 6 | `0x44239C35` | `sceNpRegisterServiceStateCallback` | 注册 PSN 服务状态回调 | 1 | @0x81058D24 |
- | 7 | `0x60C575B1` | `sceNpManagerGetChatRestrictionFlag` | 聊天限制（家长控制） | **0** | @0x81058FA2（仅网络事件状态 0x64 分支可达） |
- | 8 | `0xAF0073B2` | `sceNpManagerGetContentRatingFlag` | 内容分级标志 | **0** | @0x81058F7C（分支未进入） |
- | 9 | `0x9EA4901F` | `sceNpActivityTerm` | 活动上报终止 | **0** | @0x810583C6 |
- | 10 | `0xBC7FDC77` | `sceNpActivityPostStatus` | 上传「正在游玩」状态 | **0** | @0x810583AA |
- | 11 | `0xE0FFEE97` | `sceNpActivityInit` | 活动上报初始化 | **0** | @0x81058378 |
- | 12 | `0x214926D9` | `sceHttpInit` | HTTP 库初始化 | 1 | @0x81058CFA |
- | 13 | `0x3C733316` | `sceSslInit` | SSL 库初始化 | 1 | @0x81058CEA |
- | 14 | `0x0433069F` | `sceNpScoreInit` | 排行榜初始化 | **0** | @0x81059426 |
- | 15 | `0x2050F98F` | `sceNpScoreTerm` | 排行榜终止 | **0** | @0x81059440, 0x81059670, 0x81059A8A |
- | 16 | `0x24B09634` | `sceNpScoreRecordScoreAsync` | 异步上传分数 | **0** | @0x8105958E |
- | 17 | `0x40573917` | `sceNpScoreRecordGameDataAsync` | 异步上传 game data | **0** | — |
- | 18 | `0x45CD1D00` | `sceNpScoreGetRankingByNpIdAsync` | 按 NPID 拉排行 | **0** | — |

---

## `ARCH_14_io_display.md`

**标题**：ARCH_14：IO 与显示子系统（mceFios2 / SceIo / SceDisplay / SceDisplayUser / SceTouch / SceRtcUser / SceCtrl） ｜ 1. 本子系统职责（一句话） ｜ 2. 导入/关键函数地址表 ｜ 2.1 精确导入清单（**实测**，来自 `_h1164.log:11-30` 的 `[INFO] Import lib:` 行） ｜ 2.2 mceFios2（12） ｜ 2.3 SceIo（9 个，经 `SceLibKernel` 导入） ｜ 2.3.1 ★ 全流程只有一条 I/O 路径，且它的上游是 guest 内静态链接的 CriFs ｜ 2.4 SceDisplayUser（1） ｜ 2.5 SceDisplay（2） ｜ 2.5.1 帧边界 / 翻页链（实测） ｜ 2.5.2 ★ 节拍缺失（本子系统最重要的实测发现） ｜ 2.6 SceTouch（4） ｜ 2.7 SceRtcUser（4） ｜ 2.8 SceCtrl（2，作为输入对照一并列出） ｜ 3. 关键数据结构（字段偏移 + 含义，标注推测） ｜ 3.1 `SceDisplayFrameBuf`（`n_display_setfb` 的入参，`eboot_probe.cpp:1273` 注释 + 反汇编一致性） ｜ 3.2 `SceTouchData`（`n_touch_read` 写入的对象，`eboot_probe.cpp:226-237`） ｜ 3.3 Fios 句柄 `FiosFH`（宿主侧对象，非 guest 结构） ｜ 3.4 宿主 I/O 对象 `HostFd`（`eboot_probe.cpp:5442`） ｜ 4. 生命周期与调用关系（调用链） ｜ 4.1 启动期 I/O 建立（一次） ｜ 4.2 每帧链（fps ≈ 70，无 vsync） ｜ 4.3 与"场景/序列"子系统的交界 ｜ 5. 当前 HLE 实现状态

**发现/结论**（204 条，最多列 40）：

- > 分析对象：`C:\dev\p5d_build\eboot.bin`（ARM32 Thumb，加载基址 `0x81000000`，imports=410，exports=1）
- > 反汇编：`py C:\dev\p5d_build\_p5d_dis.py <VA> <条数>`；映射 `file_off = VA - 0x81000000 + 0x10E0`
- > **证据来源（≥5 处，本文全部结论均引用其中至少一处）**：
- > 1. `_h1164.log` 头部导入库清单 + `[NIDCALL]` 调用计数表（195 行）+ `[IO]/[PREAD]/[FIOS]/[MOVLOC]/[GXMINIT]/[DISPENTRY]/[SETFB]/[PROG]` 探针行
- > 2. eboot.bin 反汇编原文（本文出现的 `VA 指令原文` 全部为实测输出）
- > 4. eboot.bin 内嵌字符串（断言文件/行号）：`0x812E91C8`、`0x812E9220`、`0x812E91E0`
- > 6. `translation_layer_functions.md` 的 NID↔函数名数据库（SceDisplay / SceDisplayUser / SceTouch / SceRtcUser 章节）
- [INFO]   Import lib: SceDisplayUser (libNID=0x4FAACD11, funcs=1, vars=0)
- [INFO]   Import lib: SceDisplay (libNID=0x5ED8F994, funcs=2, vars=0)
- [INFO]   Import lib: SceCtrl (libNID=0xD197E3C7, funcs=2, vars=0)
- [INFO]   Import lib: SceRtcUser (libNID=0x6704D3C6, funcs=4, vars=0)
- [INFO]   Import lib: SceTouch (libNID=0x3E4F4A81, funcs=4, vars=0)
- [INFO]   Import lib: mceFios2 (libNID=0x3AD4D630, funcs=12, vars=0)
- [INFO]   Import lib: SceLibKernel (libNID=0xCAE9ACE6, funcs=29, vars=1)   ← 含全部 sceIo*
- **结论（实测，非推测）**：`SceIo` 系列**不是独立导入库**，而是从 **`SceLibKernel`** 导入的 9 个函数；`mceFios2` 的确切导入数是 **12**，与任务简报一致。
- | NID | 推测名 | 作用 | 实测调用次数（`_h1164.log` 全 6 份日志累计/单份） | 证据 |
- | `0x774C2C05` | `sceFiosInitialize` | 初始化 Fios | **1**（单份）/ 6（6 份合计） | `[NIDCALL] 1 impl=Y func=0x774C2C05 lib=0x3AD4D630 mceFios2/func=0x774C2C05`；`[SVCWIN] stub=0x812B19B0 nid=0x774C2C05` |
- | `0x2904B539` | `sceFiosTerminate` | 反初始化 | **0** | 绑定存在（`eboot_probe.cpp:7045`），`_h1164.log` 无 `func=0x2904B539` 行 |
- | `0xBF699BD4` | `sceFiosFHOpen` | 开文件句柄 | **0** | 绑定 `eboot_probe.cpp:7046`；无 NIDCALL 行 |
- | `0xB09AFBDF` | `sceFiosFHRead` | 读 | **0** | 绑定 `7047`；无 NIDCALL 行 ⇒ **同步/异步语义之争在本机运行中不成立** |
- | `0xC5C26581` | `sceFiosFHGetSize` | 取大小 | **0** | 绑定 `7048` |
- | `0xA88EDCA8` | `sceFiosFHClose` | 关 | **0** | 绑定 `7049` |
- | `0x45182328` | `sceFiosFHCloseSync` | 同步关 | **0** | 绑定 `7050` |
- | `0x1B9A575E` | `sceFiosOpIsDone` | 轮询完成 | **0** | 绑定 `7051` |
- | `0x2AC79DFC` | `sceFiosOpWait` | 等完成 | **0** | 绑定 `7052` |
- | `0x3904F205` | `sceFiosOpCancel` | 取消 | **0** | 绑定 `7053` |
- | `0xBF099E16` | `sceFiosOpGetError` | 取错误 | **0** | 绑定 `7054` |
- | `0xE4EA92FA` | `sceFiosOpDelete` | 释放请求对象 | **0** | 绑定 `7055` |
- > **★ 关键实测结论**：mceFios2 的 12 个导入中，**只有 `sceFiosInitialize` 被调用过 1 次**，其余 11 个在全部 6 份日志中**调用次数为 0**。
- > ⇒ 任务简报中"fios 的 read 是异步的（返回请求对象 + 轮询完成），如果我们的 HLE 同步返回会不会卡住加载状态机"这一疑问，
- > 另一条独立证据：`[FIOS] mounted data.cpk files=10555` / `[FIOS] mounted vita.cpk files=278` 出现在 `n_fios_init` 里（`fios_ensure_cpk`，`eboot_probe.cpp:5397`），说明这唯一一次调用确实发生了。
- | NID | 名 | 作用 | 实测调用次数 | 证据 |
- | `0x6C60AC61` | `sceIoOpen` | 打开 | **2**（`[NIDCALL] 2 impl=Y func=0x6C60AC61 lib=0xCAE9ACE6`） | 日志 `[IO] OPEN(file) 'app0:/data/vita.cpk' fd=0x4001 size=1583795664`、`'app0:/data/data.cpk' fd=0x4002 size=1913750232` |
- | `0x52315AD7` | `sceIoPread` | 随机读 | **42**（`[NIDCALL] 42 impl=Y func=0x52315AD7`） | 42 条 `[IO] PREAD fd=0x4001/0x4002 … lr=0x812951FB`；`DEC-IMPORT [10] nid=0x52315AD7 stub=0x812B2840` |
- | `0x99BA173E` | `sceIoLseek` | 定位 | **2** | `[NIDCALL] 2 impl=Y func=0x99BA173E` |
- | `0xBCA5B623` | `sceIoGetstat` | 属性/存在性 | **1** | `[NIDCALL] 1 impl=Y func=0xBCA5B623`；日志 `[IO] GETSTAT miss 'savedata0:system.bin' -> ENOENT` |
- | `0xFDB32293` | `sceIoRead` | 顺序读 | **0** | 绑定 `eboot_probe.cpp:7060`；`_h1164.log` 无 `[IO] PREAD`/顺序读行 ⇒ 顺序读路径未走 |
- | `0x49252B9B` | `sceIoLseek32` | 32 位定位 | **0** | 绑定 `7062` |
- | `0xC70B8886` | `sceIoClose` | 关 | **0** | 绑定 `7063` |
- | `0x422A221A` | `sceIoDclose` | 关目录 | **0** | 绑定 `7064` |

---

## `ARCH_15_OVERVIEW2.md`

**标题**：P5D 引擎架构 —— 第二轮缺口剖析总览 ｜ 0. 第二轮最重要的发现：BADMEM 的完整因果链 —— 根因是**我自己加的 `ASSERT-IGNORE`** ｜ 1. 第二轮纠正/作废的旧结论 ｜ 2. 第二轮各子系统结论速览 ｜ 3. 画面全黑的可能根因（第二轮新线索） ｜ 4. 更新后的行动顺序 ｜ P0（按顺序） ｜ P1 ｜ P2 ｜ 5. 导入库覆盖矩阵（410 个） ｜ 6. 文档索引

**发现/结论**（35 条，最多列 40）：

- > 本文件汇总第二轮发现，并**再次纠正若干错误结论**、给出更新后的行动顺序。
- 第一轮只钉到"停机在 `0x81181CFA`，`r3=0`"。第二轮把整条链打通了（证据见 `ARCH_7_fiber_exc.md`）：
- 资源 worker 0x8117BB42  查表落空 → movs r5,#0
- 0x8115F832              报断言 appResourceList.c:207
- HLE [ASSERT-IGNORE] #7  强制忽略，继续往下执行        ← ★ 这是我此前为了"不中断"加的
- 0x8117BB80  ldr r5,[r5,#4]        badmem addr=0x4    ← 用 NULL 解引用
- 0x8117BA66  取 req[+0x40] = NULL
- 0x81181CFA  ldr.w r0,[r3,#0x94]   badmem addr=0x94 → stop=3(BADMEM)
- **即：真正的病是"资源表查不到条目"，而我把断言忽略掉，把一次本可以立刻定位的失败，
- 正确做法不是忽略断言，而是**让它在真实现场中止**并打印上下文（建议加 `P5D_ASSERT_ABORT=1`）。
- **顺带纠正**：`0x812B4150` **不是**栈金丝雀常量——它是 `0x81206BEC` 这条指令的地址；
- 金丝雀指针在 `0x812B414C`。第一轮总览里那条结论作废。
- | # | 旧结论（来源） | 实际 | 证据 |
- | 1 | `0x812B4150` 是栈金丝雀常量 | 它是 `0x81206BEC` 指令地址；金丝雀在 `0x812B414C` | `ARCH_7` |
- | 2 | `SceAppUtil/0x6A140498` 是存档相关（59 次高频） | 它是 `sceAppUtilDrmClose`，59 次是 DLC 枚举循环尾调用，离线返 0 正确 | `ARCH_9` |
- | 4 | BADMEM 可能源于 malloc 返回 NULL | 是 **guest 对象字段为空**（`0x8102EF34` 的 `r0=[r4+4]=0`），堆侧零压力 | `ARCH_8` |
- | 5 | 卡点是"序列不就绪" | 是"资源表查不到 → 断言被忽略 → NULL 崩溃" | `ARCH_7` |
- **另外一个元问题**：`src/common/nid_resolver.h` 里的**库 NID 常量表有至少 12 处错配**
- （例：它把 ScePower 标成 `0x4332C10D`，实际那是 SceNpTrophy；SceNpManager 实际是 `0xD8835093`）。
- | 子系统 | 导入数 | 判定 | 关键结论 |
- | SceFiber + 异常/回调 (`ARCH_7`) | 6 + 若干 | **P0 阻塞（断言机制）** | fiber 与宿主调度**不冲突**（双 fiber 乒乓 6179 次对称）；但断言被忽略引出 BADMEM |
- | 堆/内存 Libc+Libm (`ARCH_8`) | 95 | P1 次要 | 实际只调用 33 个；**唯一危险**是 `_sceLibcErrnoLoc`(0x9E248B76) 未绑 ⇒ 桩返 0 ⇒ `0x8115BF26` 立即 `str 0xC,[r0]`（写 0 指针） |
- | 音频全景 (`ARCH_12`) | 39 | P2 可暂缓 | `src/audio/sce_audio.cpp` **根本没进编译**（死代码）；`n_ngs_ok` 返 0 不写 out ⇒ 调用方把栈残留当 voice 句柄 |
- | IO/显示/时间 (`ARCH_14`) | 23 | **P0 阻塞（显示）** | `DISPCALL count: 0`、`SETFB count: 0` —— **游戏注册的 display 回调从未调用**；无 vsync（实测 70fps 而非 60）；**RTC 假时钟 3.24× 超速** |
- - `vert_0015` 的 screen→NDC 投影被 **CPU 侧刻意排除**（`[PXGATE] applied=0/400`），
- - 但实测 `g_vubo = 0xA335A8FC` **全 0**，`bind0 = 0xAA700000` 的 288 个 float **只有 4 个非零**。
- - ⇒ 着色器拿到的投影输入是初值 ⇒ 顶点位置无意义 ⇒ 无内容几何。
- 1. **撤掉 / 反转 `ASSERT-IGNORE`**，加 `P5D_ASSERT_ABORT=1` 让断言在真实现场中止并打印上下文。
- 2. **查 `0x8117BB42` 的资源表为何查不到**（打印 `r6` / `[r6+0xC]` / `[r6+8]` / `r3`）。
- 4. **display 回调 + vsync**：`0x811EE7C3` 注册的回调从未调用、翻页链 `0x811EE7D6 blx 0x812B1800` 从未执行；无 vsync 导致 70fps。
- 5. **绑 `_sceLibcErrnoLoc`(0x9E248B76)** —— 现在会写 0 指针。
- 7. 修 `sceKernelLockLwMutexCB`(0x3148C6B6) 的错误绑定（现与阻塞版同一 lambda）。
- 10. 修正 `nid_resolver.h` 的库 NID 错配。
- **结论：410 个导入现已全部落到具体文档，无盲区。**
- | `ARCH_0_OVERVIEW.md` | 第一轮总览（分层图 + 6 条纠正） |

---

## `ARCH_160_CAMERA.md`

**标题**：ARCH_160 —— 接入**原版游戏相机**（透视渲染）oc ｜ 1. 结论（一句话） ｜ 2. 权威依据：GFD Studio 0.10.3 / GFDLibrary.dll ｜ 3. 实测标定（`c108.GFS`，1961 帧 / 160 秒 / packFlags=8） ｜ 4. 坐标系约定（由数据反推，且与既有渲染核心自洽） ｜ 5. 改动清单 ｜ 6. 新增开关 ｜ 7. 渲染验证（已看图确认） ｜ 8. 遗留 / 下一步

**发现/结论**（6 条，最多列 40）：

- `IL` 给出调用顺序，**读 trace 给出字节真相** —— 两条独立的证据在本条一致，未出现此前的
- | position 的 half 分量 | `absmax = 1.000000`（x/y/z 三个轴都**恰好**为 1）⇒ **归一化存储** |
- | 四元数 `\|q\|` | `0.999338 … 0.999998`（mean 0.999735）⇒ 合法，无需再缩放 |
- 验证：`c108 @ t=129.000` 位姿 `(0.58, 104.29, 619.26)`、旋转≈单位 ⇒ 视线水平朝 −Z，
- **t=20 的定量吻合**是本条最强的正确性证据：由位姿算出的可见窗口是 `x∈[47.6, 122.6]`，
- > 且舞者站在 `mns108_0..3` 这 4 个编队槽位上」。这两句都**不是已验证的结论**：

---

## `ARCH_161_RUNTIME_CAMERA.md`

**标题**：ARCH_161 —— 把原版相机接入**原生运行时循环** ｜ 1. 改动 ｜ 2. 验证：240 帧连续播放 + 原版镜头 ｜ 3. 性能实测（重要，指向 Android 端） ｜ 4. 已知缺陷 / 未完成

**发现/结论**（4 条，最多列 40）：

- `fov` 实测覆盖 `6.87° … 25.99°`，说明**镜头切换在运行时被正确驱动**。
- ⇒ 等效 0.9 fps（单线程、纯软件光栅，3× SSAA，420×640）
- ⇒ 原版镜头会大量使用特写，**软件光栅的代价上升一个数量级**。
- * 结论：Android 端**必须走 GPU**（Vulkan/GLES），软件光栅只作为正确性基准。

---

## `ARCH_162_STAGE_SLOT_STATUS.md`

**标题**：ARCH_162 —— 舞者站位的溯源结论 + 一个已定位的功能缺陷 ｜ 1. 已排除的来源（负面结果，含证据） ｜ 1.1 舞台模型里没有舞者定位节点 ｜ 1.2 舞蹈 GAP 里没有舞者的世界位移 ｜ 1.3 其余已排除的 ｜ 2. 意外发现：**`kt=26`（`NodePRHalf`）被适配器跳过 ⇒ 袖子等部件没有动画** ｜ 3. 下一步（按优先级） ｜ 4. 本轮产出的工具

**发现/结论**（9 条，最多列 40）：

- > 结论分三部分：**排除了哪些来源**、**一个意外发现的真实 bug**、**下一步怎么走**。
- `player/p5/201/pc201_018_10.GAP`（108 个控制器）实测：
- ⇒ **舞者的舞台站位不在动画资产里，是引擎把角色摆到世界变换上的。**
- 实测证据：`pc201_018_10.GAP` 中
- **只处理 31/32/33，跳过 26/27** ⇒ 它们的位置与旋转**完全不生效**。
- | keyType | 名称 | stride | 布局 | 缩放向量 |
- 2. **找舞者站位的写入点**：从 `0x8103BA18`（相机实体构造）的**唯一调用者** `0x81047F86`
- 辅助线索：`DancerEntity` 字符串 `0x812F0694` 只有 **1 处引用**（`0x8103CE6C`），
- `verify_gmd2.exe` 对 `f108_001.GFS` 退出码为 1 —— 需确认是「检查模式判定失败」还是

---

## `ARCH_163_ASSET_POSE_GAPS.md`

**标题**：ARCH_163 —— 姿态轨装配的两个致命缺口（**含对既有结论的更正**） ｜ ★ 更正：此前所有「角色在跳舞」的渲染图，其实都是**绑定姿态（T-pose）** ｜ 1. 缺口 A：姿态轨找错了目录 ⇒ 根本没加载动画 ｜ 2. 缺口 B：`NodePRHalf` 家族（kt=26/27）被适配器整批跳过 ｜ 3. 另一个不一致（已记录，未改） ｜ 4. 本轮新增的诊断（判断「动画到底有没有生效」的可靠办法） ｜ 5. 下一步（goal 仍 active）

**发现/结论**（10 条，最多列 40）：

- > 挖出**两个更根本、影响所有渲染的缺口**，并已修复。**同时必须更正此前的一批结论。**
- 角色是 T-pose。**受影响的结论**只有「角色在跳舞」这一句；
- 于是用歌曲号当动作号时（例如 `201 10 18`），`pose/pcp201_18*.GAP` **三条全缺** ⇒
- `poseTracks` 为空 ⇒ 渲染核心 `posing=false` ⇒ **角色停在绑定姿态**，且输出里没有任何报错。
- 但实测 `pc201_018_10.GAP` 里：
- | keyType | 名称 | stride | 布局 |
- 1. **包围盒是否随时刻变化** —— 最省事、最可靠的判据。恒定不变 ⇒ 绑定姿态。
- 4. `_gfd/pixdiff.py a.bmp b.bmp` —— 逐像素比对（本轮用它确认改动确实进入了输出）。
- `c218` 的若干时刻，确认构图与遮挡在动画下依然正确。
- 2. 回到**舞者槽位坐标**（`ARCH_162` §3 的路线：从 `0x81047F86` 往上追 `obj[4]` 的写入点）。

---

## `ARCH_164_DANCER_ENTITY.md`

**标题**：ARCH_164 —— 找到舞者坐标的**读取点**（goal 第 2 轮） ｜ 1. 资产侧已全部排除（本轮新增两条） ｜ 2. 定位到「读取坐标」的指令 ｜ 3. 两个创建点指向**同一个描述符对象** ｜ 4. 下一步（唯一化） ｜ 5. 本轮工具

**发现/结论**（23 条，最多列 40）：

- | 来源 | 结论 | 证据 |
- | `dance/env/f*.ENV`（808 B，自身是 `GFS0` version=0x01105100 type=4） | 两段**光照/环境**参数（色值 ~0.5–0.9 的两组、`35.0`/`10.0`/`524.0`/`8000.0` 等），无坐标 | `fieldinfo.py` |
- ⇒ **舞者坐标不在任何资产文件里**，是引擎数据。
- 用 `tools/xref.py` 找到 `DancerEntity`(`0x812F0694`) 的**唯一**引用点 `0x8103CE6C`，
- 即 `danceDancer.c` 的实体构造 `0x8103CE26`：
- 8103CE38  bl  #0x8115d79c            ; 分配 0x258 字节（DancerEntity）
- 8103CE72  bl  #0x8103e448            ; 设类型名
- 8103CECE  ldr.w r3,[r7,#0x1dc] ; b 0x8103cee8
- 8103CED4  ldr r0,[r7,#0x14] ; str.w r0,[r4,#0x240]      ; ★ 坐标第 1 分量
- 8103CEDA  ldr r0,[r7,#0x18] ; str.w r0,[r4,#0x244]      ; ★ 第 2
- 8103CEE0  ldr r0,[r7,#0x1c] ; str.w r0,[r4,#0x248]      ; ★ 第 3
- 8103CEE6  ldr r3,[r7,#0x20] ; str.w r3,[r4,#0x24c]      ; ★ 第 4
- 8103CF18  bl  #0x8103ccb4            ; → entity[0x48]
- **结论：舞者摆位 = `描述符[0x14..0x20]` 这 4 个 float**（charaId 211 时改取 `描述符[0x1D0..0x1DC]`）。
- `0x8103CE26` 全 eboot 只有 **2 个调用点**，都在 `0x81047F..` 这个**舞曲场景初始化函数**里，
- 而**相机实体构造函数 `0x8103BA18` 的唯一调用点也在同一函数**：
- 81047F86  bl   0x8103BA18            ; CameraEntity( r0 = 描述符 )   ← 读 stageId(+4)、charaId(+0x24/+0x34)
- 81047FAA  bl   0x8103d7a4            ; 另一实体
- 81047FC4  bl   0x8103CE26            ; DancerEntity( charaId, 描述符 )  ← 读坐标(+0x14..+0x20)
- 81047FD8  bl   0x8103CE26
- ⇒ **同一个 `[sp+0x24]` 对象同时携带：`stageId(+4)`、两个 charaId(+0x24,+0x34)、舞者坐标(+0x14..+0x20)、
- 以及守卫标志(+0x0F)**。这就是「舞曲格式/编队」描述符。
- 1. **找 `0x81047F..` 所在函数的序言与调用者** → 看 `[sp+0x24]` 从哪来：

---

## `ARCH_165_FORMAT_TABLE.md`

**标题**：ARCH_165 —— 舞者坐标的来源链已打通到「一张 3552 字节步长的静态表」（goal 第 3 轮） ｜ ★ 先更正第 2 轮的一个错误结论 ｜ 1. 完整来源链（本轮打通） ｜ 2. 表查询函数 `0x8103FF32(id)` 的解码 ｜ 3. 打通了「任意数据 VA → 文件偏移」（此前缺失的能力） ｜ 4. 本轮未能拿到的：表基址的值 ｜ 5. 下一步（唯一化到两个动作） ｜ 6. 本轮工具

**发现/结论**（24 条，最多列 40）：

- 重读同一段发现有写：`arg0[0x14]` 在 `0x81047FBC` 被当作**整数 charaId** 使用
- `ARCH_164` §2 的表格与结论段落按此作废，其余（实体构造、字段偏移、调用点）仍然成立。
- 0x81043B6E(sb)                        ; 舞曲/格式初始化
- sl = 0x8103FF32(id)                 ; ★ 静态表查询 → 表项指针
- 0x81047E4C(arg0, arg1, sb[0x428])   ; 唯一调用点 @0x81043D84
- ├─ 0x8103BA18( fp+0x14 )        ; 相机实体 ← 读 stageId=arg0[4]、charaId=arg0[0x24]/[0x34]
- └─ 0x8103CE26( arg0[0x14], fp+0x14 )  ; 舞者实体
- ⇒ **相机与舞者读的是同一份「表项副本」`arg0 = sb+0x40`**，
- 8103FF38  bl   0x8103feb8            ; 归一化/校验 id
- 8103FF72  movw r0,#0x878 / movt r0,#0x812f ; 断言串 @0x812F0878，行号 0x8E=142
- 8103FF88  bl   0x81046b40            ; 取表基址
- 8103FF8C  movs.w r7, #0xde0          ; ★ 表项步长 = 0xDE0 = 3552 字节
- * `id/100 ∈ {0,1,2,3}` 且被断言 —— 与 `stageId` 体系（0xx / 1xx=P3 / 2xx=P5 / 3xx=DLC）精确对应，
- 这是**独立于 `stage_format` 表的第二条证据**。
- 81046B48  ldr  r0,[r0]        ; r0 = *(0x8132F460)
- | 0 | 1 | 0xE0 | 0x81000000 | 0x321810 | 0x321810 | 5 (R+X) |
- | 1 | 1 | 0x3218F0 | 0x81325000 | 0x21BA74 | 0x3AA4A0 | 6 (R+W) |
- 验证：ph#0 的 `0xE0 + 0x1000 = 0x10E0`，与一直使用的「代码段 file 0x10E0 ↔ VA 0x81000000」完全一致。
- ⇒ 从此可以读**代码段之外**的任何数据 VA（此前 `tools/xd.py` 之类只能读代码段）。
- `*(0x8132F460)` 落在 ph#1（可写段）里，但**文件中的初值是 `0x00000000`**
- ⇒ 表基址是**运行时**赋值（重定位或初始化函数），文件里读不到。
- `arg0 = sb+0x40`、`sb[4+i] = 表项[i]` ⇒ `arg0[4] = 表项[0x40]`；
- 1. **看 `0x8103FF32` 在 `0x8103FF8C` 之后的指令** —— 只有 20~30 条，就能确定
- 2. 若确认了偏移模型，再找 `0x8132F460` 的**重定位/初始化写点**，或直接在数据段按新模型扫描。

---

## `ARCH_166_FORMAT_TABLE2.md`

**标题**：ARCH_166 —— 舞曲格式表是**运行时注册**的两级结构（goal 第 4 轮） ｜ 1. `0x8103FF32(id)` 的地址计算（本轮新解） ｜ 2. 一个强提示：这张表是**启动时注册**的 ｜ 3. 下一步（按性价比排序） ｜ 4. 本轮结论摘要 ｜ 5. 【更正】DSFT 记录并非「其余全 0」—— `+0x30..+0x5F` 此前从未检查 ｜ 三点提示 ｜ 6. 下一步（更精确）

**发现/结论**（27 条，最多列 40）：

- 8103FF88  bl    0x81046b40          ; r0 = *(0x8132F460) = base（全局，文件里为 0）
- 8103FF90  mla    r7, r4, r7, r0     ; r7 = base + (id/100)*0xDE0      ★ 0xDE0 是【组】步长
- 8103FFBA  blt    #0x8103ffe4        ; 计数在 blt 侧 ⇒ assert( id%100 < 计数 )
- ; …（计数不足时走断言分支 0x8103FFBC，用串 @0x812F08F8 = "danceGetStageFormat: invalid stageId(%d)"）
- 8103FFE4  cbz    r4, #0x81040012    ; 组 == 0 → 返回 0
- 81040000  bl     0x81046b40         ; base
- 81040016  lsls   r5, r5, #4         ; ★ 组内【索引表 16 字节/项】
- **结论（推翻上一轮的模型）**：
- | 上一轮（`ARCH_165` §2） | 本轮修正 |
- 关键证据：断言串就在 `0x812F08F8` = **`danceGetStageFormat: invalid stageId(%d)`**
- （`docs/DANCE_CAMERA_CODE.md` §1 已记录该串）⇒ `0x8103FF32` 就是 `danceGetStageFormat`。
- `*(0x8132F460)` 在 eboot 的文件镜像里是 **`0x00000000`**
- （已用 `_gfd/elfmap.py` 按正确的段映射读出：ph#1 = VA `0x81325000` ↔ file `0x3228F0`）。
- 一个指向静态表的指针初值为 0 ⇒ **表是运行时构造/注册的**。
- ⇒ **高度可能：运行时这张表就是启动时从 `dance/table/stage_format_table.arc` 载入并注册的**
- 只是我上一轮只看了 `+0x04..+0x2C` 并断言「其余全 0」——**这个断言需要用新模型重新核对**。
- `0x81043B6E` 里 `sb[4+i] = 表项[i]`、`arg0 = sb+0x40`，
- 而相机读 `arg0[4]`、舞者读 `arg0[0x14..0x20]` ⇒
- 2. 若 DSFT 里没有：找 `*(0x8132F460)` 的**写点**（搜索对 VA `0x8132F460` 的 `str`），
- 3. 顺带核对 `0x8103FFBC` 断言分支引用的串是否为 `danceGetStageFormat`，
- * `0x8103FF32` = `danceGetStageFormat`：`组 = id/100`（断言 < 4）、`序号 = id%100`（断言 < 组内计数）；
- * 表基址 `*(0x8132F460)` 运行时赋值 ⇒ 表由启动流程注册，**很可能就是资产里的 `stage_format` 数据**；
- +0x34  ff ff ff ff  **-1**     ← ★ 此前未见
- +0x38  04 00 00 00  **4**      ← ★★ 计数，很可能是【舞者数】
- +0x48  6a 00 01 00  **0x0001006A**  ← ★ 0x6A = 106，charaId 量级
- 2. **`+0x48 = 0x0001006A`**：低 16 位 `0x006A = 106` 落在 **P3 角色号空间（101…116）**，
- 3. **`+0x0C = 60`** 若真是「记录体/数组步长」，则 `4 × 60 = 240` > 96 ⇒ **数据在记录之外**

---

## `ARCH_167_RUNTIME_TABLE.md`

**标题**：ARCH_167 —— 舞曲格式表是**运行时堆对象**（3 组 × 0xDE0），坐标不在 DSFT 里 ｜ 1. 97 条 DSFT 的全量字段统计（本轮实测） ｜ 2. 决定性结论：运行时表是**堆对象**，不是 DSFT 字节的视图 ｜ 3. 填充点已经现成 ｜ 4. 策略调整建议 ｜ 5. 本轮工具

**发现/结论**（17 条，最多列 40）：

- > goal 第 5 轮。本轮给出一个**决定性的结构性否定结论**，并据此重新定位下一步。
- | 字段 | 取值 | 结论 |
- 而表注册函数 `0x81046C80` 实测：
- 81046C84  bl   0x8115d614
- 81046C8A  movw r0, #0x29d8            ; ★ 分配 10712 字节
- 81046C8E  bl   0x8115d79c
- 81046CB0  bl   0x8115d36a
- 81046CC0  cbz  r1, #0x81046ce2        ; 非 0 就断言
- 81046CC2..81046CDE  ASSERT( *(0x8132F460) == 0 )   ; 「只初始化一次」
- 81046CEA  str  r4, [r0]               ; ★★ *(0x8132F460) = 新对象
- 81046CF0  blx  0x812b2260             ; memset(r4, 0, 0x29d8)
- * 该函数**只分配 + 清零**，**不填任何内容** ⇒ 内容由别处写入。
- * `*(0x8132F460)` 在文件镜像里为 0 也由此解释：它是**运行时注册的堆指针**。
- ⇒ **DSFT 的 96 字节不是运行时表项的序列化形式。** 运行时表是启动时**重建**的结构，
- `0x8132F460` 全 eboot 共 **25 处**引用。除获取器 `0x81046B40` 与注册函数 `0x81046C80` 外，
- **下一步就是 disassemble `0x81047400..0x81047940`**（约 0x540 字节，这段就是「建表」函数），
- `0x81047400..0x81047940`），但**不确定性明显高于** goal 里的另一项：GPU 后端

---

## `ARCH_168_STRATEGY.md`

**标题**：ARCH_168 —— 坐标不在 `dance/table/*`：资源清单纯确认 + 策略调整 ｜ 1. 建表函数的数据来源 = DTBL 成员（**不是坐标**） ｜ 2. `danceResident.c` 的资源清单（本轮新读到，一次给出全部路径模板） ｜ 3. 明确结论：坐标不在 `dance/table/*` ｜ 4. 策略调整（本轮决定）

**发现/结论**（24 条，最多列 40）：

- > goal 第 6 轮。本轮把「建表函数」逆到了数据来源，得到一个**明确的排除性结论**，
- `0x81047400..0x81047960`（437 条指令）里的调用分布已经说明问题：
- | `0x8115f832` | 26 | **断言**（带源文件串与行号） |
- | `0x812b2280` | 4 | **`sprintf`**（与 `ARCH_160` 相机路径同一函数） |
- | `0x81174dbc` | 6 | 按名字在已加载归档里查找成员 |
- | `0x8104a07e` | 2 | **装载 r0 指向的资源**（与相机路径同一函数） |
- | `0x8104a076` | 2 | 取装载后的对象 |
- | `0x810471d0` | 4 | 每组一个的构造辅助 |
- 810474E6  ldr  r0, [r0]             ; r0 = *(0x8132F460) 表对象
- 81047508  movt r1, #0x812f          ; r1 = 字符串 @0x812F1578
- 81047510  bl   0x81174dbc           ; 按名查找
- 81047530  bl   0x8104a07e           ; 找不到 → 装载
- 81047540  bl   0x8104a076
- **`0x812F1578` 实测为 `"partner_type_table.bin"`**（见下 §2 的字符串清单）。
- ⇒ `+0x2C0` 一族是**指向 DTBL 成员的指针字段**，不是向量/坐标。
- 字符串区 `0x812F11E0..0x812F1720`：
- `dance/table/` **递归只有 6 个文件、0 个子目录**（本轮实测）。所以字符串里那些
- ⇒ 至此，**「舞者舞台槽位坐标」在下列位置全部被排除**：
- | 运行时格式表（堆对象 3×0xDE0） | 由 DTBL 成员装配，无坐标 |
- 我此前把「瞄准点游走 ⇒ 舞者在原点之外」当成已证事实，**这一步是推断，不是证据**。
- ⇒ **下一轮起转做 GPU/后端与验证落地**，具体：
- 坐标这条线**不关闭**，但保留为「待新证据再启」：唯一还没试的两个动作是
- ① 追 `0x81047E4C` 调用者链里舞者**变换的应用点**（`0x8103CCB4` 的后续），
- ② 若将来能做动态调试，直接在运行时读 `0x8132F460` 指向的对象。

---

## `ARCH_169_GPU_INTERFACE.md`

**标题**：ARCH_169 —— 真实舞蹈下的相机验证（完成）+ GPU 接口规格 ｜ 1. 验证项 ① 完成：动画修复在两条路径都生效 ｜ 1.1 `--play` 路径（`load_gaps` → `parse_best`） ｜ 1.2 `--char` 路径 + 原版相机（`c218`） ｜ 2. GPU 接口规格（实现前的设计冻结） ｜ 2.1 必须跨边界的数据（一次导出、每帧只变前两项） ｜ 2.2 必须复刻的渲染语义（来自 `docs/RENDER_SEMANTICS.md`） ｜ 2.3 现在**不要**做的 ｜ 3. 遗留

**发现/结论**（5 条，最多列 40）：

- **包围盒逐帧变化** ⇒ 动画生效（修复前恒为 `(-80.4,-0.8,-30.0)..(80.4,192.9,27.8)`）。
- fov 25.99° + aspect 0.6562 ⇒ 可见 `x∈[−125.4, 75.2]`、`y∈[−79.1, 226.7]`。
- 这样「软件光栅作为正确性基准」才有意义（这也是 `ARCH_161` §3 的量化结论所要求的）。
- * 不要再为舞者坐标做近似 hack：`ARCH_168` 已给出排除性结论，用反推值会违反「不得用近似 hack 冒充正解」。
- 3. 舞者槽位坐标：**待新证据再启**（`ARCH_168` §4 的两个未试动作）。

---

## `ARCH_16_app_states.md`

**标题**：ARCH_16 —— app 状态机全图（P5D / PCSH10109 / eboot.bin） ｜ 0. 本轮最重要的三个纠正 ｜ 纠正 1：`[0x813B11AC]` 就是行基址本身（= 0x813B01B4），不是表首也不是别的 ｜ 纠正 2：项的物理布局——`+0x44/+0x48/+0x4C` 三函数 + `+0x50` 起的 tag 文本 ｜ 纠正 3：`[0x813B11AC]` 在文件里是 0（BSS），运行时才被 setter 填上 ｜ 1. 本模块的运行职责（一句话） ｜ 2. 完整流程 / 状态迁移图（本轮核心） ｜ 2.1 状态机对象的两个全局 ｜ 2.2 管理器对象字段（`r4 = bl 0x8116F8A8([0x813B11A8])`） ｜ 2.3 帧内三分支（`0x811857C2` 是「每帧被登记调用」的那个入口，`0x8118557C` 是其 update 主体） ｜ 2.4 changeState —— 唯一的迁移入口（`0x81185616`） ｜ 2.5 状态迁移图 ｜ 2.6 「initDataRead」不是九步，而是**一个需要被真正消费的任务** ｜ 3. 关键函数地址表 ｜ 4. 关键数据结构 ｜ 4.1 状态表项（stride 0x50，共 7 项，行基址 `[0x813B11AC] = 0x813B01B4`） ｜ 4.2 解出的状态表（全部 7 项，三列函数 + tag） ｜ 4.3 任务节点（`0x815554C0` 链表，被状态机当作完成判据） ｜ 5. 调用链（从入口到出口，逐层） ｜ 5.1 启动 → 进入 state 1 ｜ 5.2 全部 12 个 changeState 调用点（BL 精确匹配全 SEG0 扫描 + 逐点取参） ｜ 5.3 state 6 的完整链（title sequence） ｜ 5.4 帧驱动链（谁在每帧调用状态机） ｜ 5.5 失败现场（h1169，本轮最新）

**发现/结论**（292 条，最多列 40）：

- > - `VA  指令原文` 形式的行 = 用 `py C:\dev\p5d_build\_p5d_dis.py <VA> <n>` 反汇编所得。
- > - 标注「推测」的条目未获直接证据。
- > ⚠ **本文纠正了本轮开始前的一个关键错误认知**：任务书称「状态表 `[0x813B11AC]`，步长 0x50」——
- > 步长对了，但 `0x813B11AC` 是**一个指针单元**，其值就是**调度坐标系的行基址**
- > `0x813B01B4`（由 `0x811858BC movw r0,#0x1b4` 经 setter 写入）。
- > **行基址 = `[0x813B11AC] + state*0x50`**，三字段 `enter=[+0x44] / update=[+0x48] / leave=[+0x4C]`，
- 调度点反汇编（唯一的权威依据）：
- **`row(state) = [0x813B11AC] + state*0x50`**，三个字段：
- | `[row + 0x48]` | **update** | `811855CA` / `811856BC` / `8118586E` ★ |
- `[0x813B11AC]` 由一条专门的 setter 写入：
- 811857B4  str       r0, [r2]                           ; [0x813B11AC] = r0  （行基址）
- 811857BE  str       r1, [r0]                           ; [0x813B11B0] = r1  （enter 实参指针）
- 811858C6  movs      r1, #7                             ; ★ 状态表长度 = 7
- 811858C8  bl        #0x811857ac                        ; setter(rowBase=0x813B01B4, count=7)
- `0x811858BC` 的唯一调用者是 `0x81184726`，位于 **MAIN init（0x8118464C）的启动序列里**（见 §5.1）。
- **代数求解（决定性）**：设读 update 的地址 `P` 满足 `P = [0x813B11AC] + 0x48`，要求
- `P + 1*0x50 = 0x81185927` 且 `P + 6*0x50 = 0x81185987`。
- 在 `0x813B0000..0x813B3000` 全域穷举，**唯一解** `P = 0x813B01FC`（`P-0x48 = 0x813B01B4`），
- row(state) = 0x813B01B4 + state*0x50      loadAddr = row + 0x48
- st1 / st6 两行与运行日志逐字一致（`updateFn=0x81185927` / `updateFn=0x81185987`），几何确定。
- state 0 行基址 `0x813B01B4`：`row+0x44 = 0x813B01F8 = 811858CF`(enter)、
- `row+0x48 = 0x813B01FC = 811858D1`(update)、`row+0x4C = 0x813B0200 = 811858ED`(leave)、
- `row+0x50 = 0x813B0204` 起 = `"initread"`（tag）。
- 下一行基址 `0x813B0204`... 注意 tag 与下一行的 `+0x44` 区段在**同一处内存**上并存，
- row(state) = 0x813B01B4 + state*0x50
- 用 SEG1 映射（`fo = VA - 0x81325000 + 0x3228F0`）读静态镜像：
- ⇒ `0x813B11AC` 静态恒为 0，只有运行到 MAIN init 的 `0x81184726`（→ `0x811858BC` → `0x811857AC`）
- 才被写入 `0x813B01B4`。`[0x813B11B0]` 同理（enter 实参指针）。
- `file_off = va - 0x81000000 + 0x10E0` **只对 SEG0 成立**。
- SEG1（`va >= 0x81325000`）必须用 `fo = va - 0x81325000 + 0x3228F0`。
- 用错映射会把 BSS 区读成代码（`0x813B11AC` 处会读出 `EDEFEFEE` 这类「假指令流」），
- 本轮第一版结论即由此出错。
- `0x813B11A8` 这一族全局维护一个**「当前画面/场景模式」数字**，每帧由
- `0x8118557C`（update 调度）驱动一次 `update()`，返回值 1 = 保持、非 1 = 退出；
- 状态切换统一走 `0x81185616`（changeState），它负责 **leave(old) → 改状态 → 下一帧 enter(new)**，
- 而 enter/update/leave 三个函数指针全部来自一张 7 项的静态表（行基址 `0x813B01B4`，步长 0x50）。
- | 全局 | 静态值 | 含义 | 证据 |
- | `0x813B11A8` | 0 | 「app 状态管理器」宿主指针（`bl 0x8116F8A8` 用它取对象） | `81185586 ldr r0,[r0]` / `81185588 bl #0x8116f8a8` |
- | `0x813B11AC` | 0（运行时填） | **状态表行基址**；由 setter `0x811857AC` 写入 | `811857B4 str r0,[r2]`；`811858BC` 传入 `0x813B01B4` |
- | `0x813B11B0` | 0（运行时填） | 下一个 enter 的实参指针 | `811857BE str r1,[r0]` |

---

## `ARCH_170_GPU_EXPORT.md`

**标题**：ARCH_170 —— GPU 导出面落地（阶段 B 的第一步，已验证） ｜ 1. 接口 ｜ 2. 开关与落盘 ｜ 3. 验证（两层，都可复现） ｜ 4. 为什么这样是对的 ｜ 5. 尚未做（下一步）

**发现/结论**（2 条，最多列 40）：

- > 修正记录：首次导出 `aspect = 0.000`（漏填）。已补，现为 `0.656`（= 420/640）。
- * **同一份数据**：导出的顶点就是光栅化读的那份（`V`），没有任何额外变换 ⇒

---

## `ARCH_171_GPU_TEXTURES.md`

**标题**：ARCH_171 —— GPU 导出面补齐**贴图像素**（含一处必须点明的缺口） ｜ 1. 为什么贴图这块是「纯增量、低风险」 ｜ 2. 格式（`P5DGPU2`） ｜ 3. 验证（本轮实测，`--char 201 10 18` + `c218` + `P5D_POSE_T=20`） ｜ 4. ★ 必须点明的缺口：**常量色（无贴图）路径没有导出** ｜ 5. 剩余待办（更新）

**发现/结论**（4 条，最多列 40）：

- texInfo(nTex×16 = {w,h,off,bytes})            ← ★ 本轮新增
- texPixels(texPixelsBytes)                     ← ★ 本轮新增：各槽位按 off 拼接
- `GpuVertex::rgba` **硬编码为 255**、也**没有导出 `Tri::tint`** ⇒
- > 结论：`GpuVertex::rgba` 目前是「占位」而不是「数据」。这一点必须写进文档，

---

## `ARCH_172_GPU_TINT.md`

**标题**：ARCH_172 —— 补上「常量色（无贴图）路径」的导出（修 `ARCH_171` §4 的缺口） ｜ 1. 为什么必须**逐三角形**而不是逐顶点 ｜ 2. 导出格式（`P5DGPU3`） ｜ 3. 验证（`--char 201 10 18` + `c218` + `t=20`） ｜ 4. 导出面现状（阶段 B 的 GPU 输入已基本齐备） ｜ 5. 下一步

**发现/结论**（4 条，最多列 40）：

- rgb = 常量色，a = 1.0（有效）/ 0.0（无 tint ⇒ 用白色）
- ★ 常量色三角形 = 3909 / 17171   引用空贴图槽的三角形 = 3909   两者一致 = True
- **★ 交叉验证**：`triTint` 里标记为「有 tint」的三角形数（3909）与
- | 逐顶点色（`GpuVertex::rgba` 目前恒 255） | ✘ 待确认真实来源 |

---

## `ARCH_173_CONSTANT_COLOR.md`

**标题**：ARCH_173 —— 常量色的**完整规则**（含兜底色）与顶点色问题的结论 ｜ 1. 结论一：软件光栅**从未读取顶点色**（有据） ｜ 2. 结论二（真缺口）：常量色规则缺了兜底色 ｜ 3. 验证 ｜ 4. 导出面最终状态（阶段 B 的 GPU 输入） ｜ 5. 下一步

**发现/结论**（2 条，最多列 40）：

- ⇒ `ARCH_172` §4 里「`GpuVertex::rgba` 恒 255」这条**不是数据丢失**，
- 独立 Python 复核（`_gfd/gpuread.py`，格式未变仍为 `P5DGPU3`）：

---

## `ARCH_174_EXPORT_COMPLETE.md`

**标题**：ARCH_174 —— GPU 导出面**完成**（判定依据：基线消费的每一样都在了） ｜ 1. 判定方法：列出基线**真正读取**的每一样输入，逐项核对 ｜ 2. 结论：两个**不需要**导出的字段（都有据） ｜ 2.1 顶点色 —— 基线从不读取 ｜ 2.2 `blendSrc` / `blendDst` + 材质名 —— 基线从不读取 ｜ 3. 本轮新增验证：导出保留了渲染顺序 ｜ 4. 导出面清单（终态） ｜ 5. 下一步（转向真正的后端）

**发现/结论**（5 条，最多列 40）：

- > goal 第 12 轮。本轮不再加字段，而是**用证据判定导出面已经完整**，
- ⇒ ✔ `verts`。相机 ⇒ ✔ `cam`。
- ⇒ `GpuVertex::rgba` 恒 255 **不是数据丢失**。
- ⇒ 对「**GPU 后端复现软件光栅**」这个目标，`blendSrc/blendDst` 与材质名**不是必需项**
- 这是 `ARCH_161` §3 量化结论（软件光栅 0.9 fps / p95 3262 ms）的直接回应。

---

## `ARCH_175_GPU_PRECOND.md`

**标题**：ARCH_175 —— GPU 后端的前置调研 + **更正一处错误建议** ｜ 1. ★ 更正：`src/gxm/vulkan_backend.cpp` **不能**用于原生重写 ｜ 2. 本机图形工具链现状（决定了怎么落地） ｜ 3. 绕开该缺口的落地路线（建议） ｜ 4. 下一轮的具体第一步（已可执行） ｜ 5. 本轮不改代码

**发现/结论**（4 条，最多列 40）：

- **这是错的**，本轮实测澄清：
- ⇒ 原生的 GPU 后端必须**新写**，规模不大（`P5DGPU3` 已经把输入降到「顶点 + 索引 + 贴图 + 常量色 + DrawMethod」）。
- | Vulkan **import lib**（`vulkan-1.lib`） | ✘ 无 SDK ⇒ 需从 DLL 导出生成，或运行时 `LoadLibrary`+`GetProcAddress` |
- ⇒ **写自己的 Vulkan 着色器目前缺 GLSL→SPIR-V 编译器**，这是唯一的硬缺口。

---

## `ARCH_176_GL_VIABLE.md`

**标题**：ARCH_176 —— OpenGL 路线**可行性已实测确认**（GPU 后端可以动手了） ｜ 1. 探针（可复现） ｜ 2. 这证明了什么 / 没证明什么 ｜ 3. 顺带得到的量化事实 ｜ 4. 踩到并修掉的两个坑（值得记下） ｜ 5. 下一轮（可直接动手，无需再调研）

**发现/结论**（5 条，最多列 40）：

- 代码：`C:\dev\p5d_build\_glprobe\glprobe.cpp`（约 60 行：隐藏窗口 → 像素格式 → GL 上下文 → 打印版本）
- * `wglCreateContextAttribsARB` 可用，**3.3 core 上下文创建成功** ⇒
- * 探针本身可构建、可运行 ⇒ 后续 GPU 程序的构建方式已跑通（含 `/utf-8` 这个必需项，见 §4）。
- **未证明**：OpenGL 渲染结果能否与软件光栅逐像素一致 —— 那是下一轮要做的对拍，不是本轮结论。
- ⇒ 构建命令必须带 **`/utf-8`**。（本仓库其它构建脚本若含中文，同理会踩到。）

---

## `ARCH_177_GL_PIPELINE.md`

**标题**：ARCH_177 —— OpenGL 离屏渲染管道**端到端打通**（GPU 对拍的地基） ｜ 1. 做了什么 ｜ 2. 验证：四条判据全部命中（数字判定，不依赖看图） ｜ 3. 本轮踩到并修掉的三个坑（都会再遇到，值得记下） ｜ 4. 下一轮：`glcompare`（接 `P5DGPU3`） ｜ 5. 说明

**发现/结论**（4 条，最多列 40）：

- 隐藏窗口 → 像素格式 → GL 上下文
- 当成 `\` 或 `"`，破坏字符串并报 `C2059 语法错误`。⇒ 构建必须带 **`/utf-8`**。
- ⇒ 1.1 的函数（`glViewport`/`glClear`/`glDrawArrays`/`glReadPixels`/`glGenTextures`/`glTexImage2D`…）
- | 输入 | 读 `P5DGPU3`（`ARCH_174` §4 的格式，字段全部验证过） |

---

## `ARCH_178_CAMERA_MATH_VERIFIED.md`

**标题**：ARCH_178 —— GPU 侧相机数学**亚像素级验证通过** + 一个真 bug ｜ 1. 判据：导出的顶点投影出的像素范围 == 软件光栅实际画出的内容范围 ｜ 2. 结果：亚像素级吻合 ｜ 3. ★ 顺带抓出的真 bug：下采样路径硬编码了 8 像素间隙 ｜ 4. 本轮产出 ｜ 5. 下一轮

**发现/结论**（6 条，最多列 40）：

- ★ 由 cam 投影算出的像素包围盒: x[180.2, 334.8]  y[68.3, 464.3]
- ★ 图中内容包围盒:            x[180,   334]    y[69,   464]
- ⇒ GPU 后端要用的相机链路已被逐项确认：
- **下采样后的 `FW` 仍无条件 `+8`** ⇒ 相机模式下输出右侧多一条**无意义的 8 像素背景带**，
- 相机链路已确认，`glcompare` 剩下的不确定性只剩**着色语义**：
- 1. 上传 `verts`，用本轮确认的 MVP；

---

## `ARCH_179_PROJ_CULL_VERIFIED.md`

**标题**：ARCH_179 —— 投影与剔除语义**逐三角形精确验证**（GPU 对拍的参照物就位） ｜ 1. 方法：拿光栅化**自己的逐三角形转储**当判据 ｜ 2. 结果：两项全部精确一致 ｜ 3. 同时产出：GPU 光栅化的**目标** ｜ 4. 现在的位置：GPU 对拍的输入、相机、几何、剔除**全部已证** ｜ 5. 下一轮

**发现/结论**（3 条，最多列 40）：

- 大于 0.01 像素的**一个都没有**。⇒ 导出的 `verts`+`cam` 与我的 CPU 相机数学
- ⇒ 这条规则**可以只从导出面还原**，GPU 后端照此实现即可。
- | 导出面格式与字段 | ✔ `ARCH_170`~`ARCH_174`（bbox 逐位一致、尺寸逐字段核算） |

---

## `ARCH_17_rhythm_core.md`

**标题**：ARCH_17 — 音游对局核心（选歌 → 载入谱面 → 开局 → 判定 → 结算） ｜ 0. 本轮必须先说的三件事 ｜ 0.1 本模块的现状判定：**游戏从未进入过对局** ｜ 0.2 方法论纠正：`_p5d_dis.py` 的标签偏移 +2（本轮踩过的最大的坑） ｜ 0.3 与既有文档的关系 ｜ 1. 本模块的运行职责（一句话） ｜ 2. 完整流程 / 状态迁移图（本轮核心） ｜ 2.1 冷启动到对局的全景状态迁移（含实测与未实测） ｜ 2.2 `init dancemain` 的精确调用链（**实测**） ｜ 2.3 `DanceSystem` 自己的状态机（**实测**） ｜ 2.4 `state 2` 时实际加载了哪些资源（**实测，日志原文**） ｜ 2.5 判定与结算的模块归属（**字符串/文件路径实证，逻辑未实测**） ｜ 2.6 判定时基的线索（**推测，但有实证锚点**） ｜ 3. 关键函数地址表 ｜ 4. 关键数据结构 ｜ 4.1 app 状态表项（步长 0x50）——**实测字段** ｜ 4.2 app 状态机对象（`r5`，来自 `0x8116F8A8` 的 work） ｜ 4.3 DanceSystem work 块（4 字节） ｜ 4.4 appSound 玩家对象（5 个实例）——**已知失败点的数据结构** ｜ 4.5 5 个玩家对象确实已建但从未就绪（**因果链**） ｜ 5. 调用链（从入口到出口，逐层） ｜ 5.1 链 A：启动 → 「dance 子系统待机」（**实测走通**） ｜ 5.2 链 B：选歌 → 开局（**未实测，按模块与字符串还原的「设计意图链」**） ｜ 5.3 链 C：appSound（**实测：建了但不就绪，且断言被忽略**）

**发现/结论**（365 条，最多列 40）：

- > 证据规则：`file_off = VA - 0x81000000 + 0x10E0`；反汇编 `py C:\dev\p5d_build\_p5d_dis.py <VA> <条数>`；
- > **本轮最重要的方法论结论见 §0.2：`_p5d_dis.py` 的地址标签比真实指令地址大 2 字节。**
- > 凡未实证处一律标「推测」。
- [_h1164.log:31204] [TASKCREATE #17] r0=0xB1B1E01C r1='init dancemain' r2=0xC8 r3=0x0 lr=0x81184DA6
- [_h1164.log:31210] [TASKCREATE #18] r0=0x0 r1='DanceSystem' r2=0xC8 r3=0x0 lr=0x81049A72
- ⇒ **本轮的产出必然是「设计意图链 + 可执行的验证方案」，而不是「实测跑通的链路」。**
- 凡是实测得不到的环节，本文一律给反汇编证据 + 明确标注「未实测」。
- `ins.address`。实测该输出与真实指令地址**相差 +2**。
- 0x81049A76  push      {r4, lr}
- 0x81049A78  movw      r0, #0x1884
- 0x81049A7C  movt      r0, #0x812f
- 0x81049A80  movs      r4, #0
- （直接按 file_off = VA-0x81000000+0x10E0 读字节，并用 capstone 指定真实 VA 解码）
- 0x81049A76  hw=0xB510  push {r4, lr}
- 0x81049A78  hw=0xF641 0084   movw r0, #0x1884
- 0x81049A7C  hw=0xF2C8 102F   movt r0, #0x812f
- | `0x81049A78` | `0xF641` | `0x1884` | `movw r0,#0x1884` | `0x81049A76`（= 紧邻的 `push` 之后的第 1 条） |
- | `0x811980FE` | `0xF24B` | `0xB140` | `movw r0,#0xb140` | `0x811980FC` |
- **结论**：`_p5d_dis.py` 打印的地址一律要 **-2** 才是真实 VA。
- 本文件中凡引用反汇编 VA，均已按此校正（或直接引用日志/源码中的 VA）。
- 这条纠正解释了本轮早期反复出现的「同一地址两次读出不同字节」现象。
- app 状态表（`0x813B11AC`，0x50 字节/项）决定「现在是不是对局」；
- `DanceSystem` guest 序列（`0x8116F900("DanceSystem")`）决定「对局资源准备好了没有」；
- module_start 0x81198464
- ▼ app 状态机（表 = *(u32*)0x813B11AC，步长 0x50）
- │   dispatch: enter 0x81185818 / update 0x8118586E   （ARCH_1 已记）
- │   entry 布局（实测）: +0x40 = "启动时是否注册" 字节, +0x44 = enter, +0x48 = update, +0x4C = 另一回调
- [N-1] ──[ST-CHANGE] state -1 -> 1  updateFn=0x81185927──▶ [1] initDataRead 9 步协程
- │      [_h1164.log:26192] [ST-CHANGE] state -1 -> 1 updateFn=0x81185927
- │      [_h1164.log:26193] [ST-FIRSTCALL] state=1 fn=0x81185927
- │   ┌─ initDataRead（0x8118479C，step fn 0x8118484F；ARCH_9 记的 9 步表）
- │   │    step4 **init dancemain** ← ★ 本模块在这里第一次出现
- │   │    [_h1164.log:31203] [IDR-STEP] start step=4 fn=0x81184D41
- │   │    [_h1164.log:31204] [TASKCREATE #17] r1='init dancemain' ... lr=0x81184DA6
- │   │    [_h1164.log:31622] [TASKCREATE #19] r1='init mainmenu' lr=0x81184C24
- ▼ [1] ──[ST-CHANGE] state 1 -> 6 updateFn=0x81185987──▶ [6] title sequence
- │      [_h1164.log:83871] [ST-CHANGE] state 1 -> 6 updateFn=0x81185987
- ▼ [7?] **mainmenu** —— 六条件门 0x81184B60（ARCH_6 已覆盖）
- │      C1 find('mainmenu load') 长期为 1 ⇒ 卡在这里
- ▼ [N?] **music_select（选歌）**   ← 未实测；模块地址见 §3.4

---

## `ARCH_180_LAM_CHECK_FAILED.md`

**标题**：ARCH_180 —— 光照模型验证**未成功**：实验设计有缺陷，缺一件仪器 ｜ 1. 做了什么 ｜ 2. 为什么这**不能**作为「模型错误」的结论 ｜ 3. 需要的仪器：逐像素**三角形归属**转储 ｜ 4. 顺带确认的事实 ｜ 5. 下一轮

**发现/结论**（2 条，最多列 40）：

- ⇒ 这是**实验设计缺陷**，不是「光照模型错了」。我不把它当作模型错误的证据。
- **说明**：本轮没有产出「已验证」的结论，但省下了一次错误结论，

---

## `ARCH_181_SHADING_PARTIAL.md`

**标题**：ARCH_181 —— 逐像素着色验证：模型**形式确认**，但有残差未消 ｜ 0. 先修掉一个**我自己的 bug** ｜ 1. 方法 ｜ 2. 结果 ｜ 3. 结论（区分「已确认」与「未确认」） ｜ 4. 下一轮

**发现/结论**（7 条，最多列 40）：

- 修正后误差从「最大 66」降到「最大 20」，模型的对错才变得可判。
- **已确认**（这些是本轮真正的收获）：
- **未确认**：残差 1~20 且**随亮度增大而增大**（`103/123=0.837` vs `200/202=0.990`，
- ⇒ **尚不能宣称着色模型已逐位复刻。**
- 2. 若确认是 `lam` 的常数项，直接读 `p5d_render_core.h` 里 `lam` 那几行**逐字**核（本轮我是按
- 3. 着色确认后再写 `glcompare`：届时「几何/投影/剔除/深度/着色」全部有独立验证。
- 「形式确认、只剩一个待定位的残差 + 样本量不足」，并且修掉了校验器自己的 BGR bug。

---

## `ARCH_182_SHADING_VERIFIED.md`

**标题**：ARCH_182 —— 着色（光照 + 常量色 + 下采样）**逐像素精确验证通过** ｜ 1. 残差的真正原因：法线归一化 ｜ 2. 结果：零误差 ｜ 3. 由此**已精确确认**的渲染语义（常量色路径全链） ｜ 4. 如实说明：样本量仍偏小 ｜ 5. 剩下未验证的渲染语义 ｜ 6. 下一轮

**发现/结论**（5 条，最多列 40）：

- > goal 第 22 轮。`ARCH_181` 把「着色」推进到「形式确认、残差 1~20 未消」。
- ⇒ 正确模型是 **法线取均值（÷3）但`不`归一化，只把光向量归一化**。
- 所以我的预测一直在**高估** —— 与实测 `pred > actual` 的残差方向一致。
- 11/11 零误差是强证据，但**样本小**。下一轮应放宽判据
- ⇒ 现在「几何/投影/剔除/深度/常量色着色」全部有独立精确验证；

---

## `ARCH_183_COMMUNITY_AND_CROSSCHECK.md`

**标题**：ARCH_183 —— 社区检索结论（负面）+ 三件产物的交叉一致性验证 ｜ 1. 社区检索结论：**没有找到关于「舞者舞台槽位坐标」的公开做法** ｜ 更强的一条证据（不依赖网页） ｜ 社区知识**已经兑现价值**的部分 ｜ 2. 本轮验证：覆盖 / 归属 / 成品 三件产物交叉一致 ｜ 3. 顺带确定的下一轮样本群 ｜ 4. 当前验证总账

**发现/结论**（8 条，最多列 40）：

- > goal 第 23 轮。两件事：① 回答「社区有没有做法」并**把负面结论固化下来**，避免以后重复检索；
- **证据强度限制**：本环境 `web_fetch` 被挡（`github.com` / `shrinefox.com` 均解析到非公网 IP），
- **模型 / 相机 / 材质 / 动画 / 贴图**这些 **GFD 资源文件格式**，舞曲场景与编队**不在其中**。
- ⇒ 这与离线结论**互相印证**（`ARCH_168`：坐标由 eboot 运行时构造，不在任何 GFD/GFS/GAP 资源里）。
- 社区工具面向**资源格式**，不覆盖**引擎运行时数据**。
- —— 说明未覆盖 ⇒ 背景，且下采样与块对齐都正确；
- 若 alpha<255，预测需要知道画布此前状态 ⇒ 这类块必须排除或另行处理。
- | 导出面格式/字段、贴图像素、三段常量色 | ✔ 已验证 |

---

## `ARCH_184_TEXTURE_VERIFIED.md`

**标题**：ARCH_184 —— 纹理采样路径**验证通过（99.99% 零误差）** ｜ 1. 方法 ｜ 2. 结果 ｜ 3. ⚠ 一处**未被本样本区分**的语义：环绕方式 ｜ 4. 验证总账（更新） ｜ 5. 下一轮

**发现/结论**（5 条，最多列 40）：

- * `col[k] = (uint8_t)clamp255(bilerp(k)*lam)`，`lam` 用 `ARCH_182` 已确认的公式
- `REPEAT` 与 `CLAMP` 给出**完全相同**的结果 ⇒ 这个样本里**没有任何像素触发环绕**
- ⇒ **环绕方式仍未验证**。要区分它，需要 UV 落在贴图边界（`u≈0` 或 `u≈1`）的像素 ——
- 代码注释与实现都写的是 REPEAT（「双线性 + REPEAT 环绕」），但**注释不等于证据**，
- | 导出面格式/字段、贴图像素、三段常量色 | ✔ 已验证 |

---

## `ARCH_185_TEXWRAP_CLAMP.md`

**标题**：ARCH_185 —— 纹理环绕方式：**默认 CLAMP**（并更正 `ARCH_184` 的一处错误） ｜ 1. 实现原文（`p5d_render_core.h` 1036-1047） ｜ 2. 更正与教训 ｜ 3. 一条漂亮的独立印证 ｜ 4. 验证总账（更新） ｜ 5. 下一轮

**发现/结论**（9 条，最多列 40）：

- > 本轮逐字读实现，结论更正。
- 1037:  // ★ ARCH_150：参考实现用 clamp（边缘取整），默认改为与其一致；
- 1038:  //   本模型 UV 未越界，实测两种方式零差异；P5D_TEXCLAMP=0 可切回 REPEAT
- ⇒ **默认走 CLAMP 分支**；`P5D_TEXCLAMP=0` 才切到 REPEAT（A/B 用）。
- * ⇒ **GPU 后端应实现 CLAMP（边缘取整）**，不是 REPEAT。
- （「★ 双线性 + REPEAT 环绕（此前是最近邻采样…）」），它描述的是 **ARCH_150 之前**的行为，
- 代码注释（1038 行）自称：**「本模型 UV 未越界，实测两种方式零差异」**。
- ⇒ 作者的断言与我的测量**互相印证**：这套模型的 UV 确实不越界，
- | 导出面格式/字段、贴图像素、三段常量色 | ✔ | 逐字段核算 + 交叉校验 |

---

## `ARCH_186_BLEND_SEMANTICS.md`

**标题**：ARCH_186 —— 混合语义从参考实现定案 + 我的 id 仪器有一处精度缺陷 ｜ 1. 完整写入语义（参考实现逐字） ｜ 2. ★ 我的 id 仪器有一处精度缺陷（本轮自查） ｜ 3. 验证总账（更新） ｜ 4. 下一轮

**发现/结论**（5 条，最多列 40）：

- 1056:  if (af < 8.f) continue;                             // ★ 整像素丢弃
- 1075:      for k: o[k] = to8((float)col[k]);               // ★ 直接写，**不混合、不看 alpha**
- ⇒ **`id` 的准确含义是「最后一个通过深度测试的三角形」，不是「最后真正写入的三角形」。**
- 在 `alpha≥255` 的情形两者一致（`ARCH_182`/`ARCH_184` 的判据都落在这个子集，所以那两次结论成立），
- | 验证混合所需的仪器 | ⚠ **有缺陷，需一行修正**（§2） |

---

## `ARCH_187_ID_FIX_AND_BLEND_FEASIBILITY.md`

**标题**：ARCH_187 —— id 仪器修正（正确但无可测效果）+ 混合验证的可行性统计 ｜ 1. 改动 ｜ 2. 结果：修复前后**逐位相同** ｜ 3. 混合验证的可行性统计（本轮新增，决定下一轮怎么做） ｜ 4. 下一轮：仪器的下一步扩展（`prev` id） ｜ 5. 验证总账（更新）

**发现/结论**（6 条，最多列 40）：

- > 修正**正确但未改变任何测量结果** —— 这本身是有信息的，下面说明为什么。
- if (kId) idbuf[(size_t)y*W+x] = (int32_t)ti;   // ★ ARCH_187
- **与修正前完全一致**。原因清楚：该缺陷只在「`alpha<8` 被丢弃」的像素上产生差异，
- ⇒ 这轮是**正确性修正、但零可测效果**。价值在于：将来验证混合时 id 才是可信的，
- * 若 `previd < 0` ⇒ 该像素是首写，画布初值 = 背景(16,16,16)，可精确预测；
- * 否则前一次的胜出者已知，其颜色也可算 ⇒ 可精确预测一层混合。

---

## `ARCH_188_PREV_ID_INSTRUMENT.md`

**标题**：ARCH_188 —— `prev` id 仪器建成（混合验证的样本量提升 ~9000 倍） ｜ 1. 改动 ｜ 2. 仪器验证 ｜ 3. 为什么这条对混合是必需的 ｜ 4. 验证总账（更新） ｜ 5. 下一轮

**发现/结论**（8 条，最多列 40）：

- ⇒ 被覆盖且首写    = 2,366,609 − 2,134,720 =   231,889
- ⇒ 被写过两次以上  =   284,480 −   231,889 =    52,591
- * `id`（最终胜出者）⇒ 知道**当前**这一层的 `c` 与 `al`；
- * `prev`（上一次写入者）⇒ 若 `prev<0`，画布初值是背景 `(16,16,16)`；
- 否则上一次的写入者已知，其颜色可用已验证的着色链算出来 ⇒ **可精确预测一层混合**。
- 但可以先做两层（占多数），并把不能判定的像素排除 —— 与 `ARCH_184` 的做法一致。
- 2. `prev<0` ⇒ 底层 = 背景；否则用已验证的着色链算 `prev` 三角形的颜色；
- 4. 报告匹配率与残差，并明确排除无法判定的像素（≥3 层、alpha<255 的链等）。

---

## `ARCH_189_BLEND_ORDER_VERIFIED.md`

**标题**：ARCH_189 —— 混合的**绘制顺序语义**从数据验证通过 ｜ 1. 判据 ｜ 2. 结果 ｜ 3. `(prev.drawMethod → id.drawMethod)` 分布 —— 把语义钉死 ｜ 4. 顺带确定的下一轮样本量 ｜ 5. 验证总账（更新）

**发现/结论**（6 条，最多列 40）：

- ⇒ 对任一像素，写入序列必为 `[不透明...][混合...]`，因此：
- **最后两行是关键的语义证据**：`Add → Transparent` 与 `Transparent → Add` **同时存在**
- ⇒ 混合组**内部**的先后**与 `drawMethod` 取值无关**，而是保持原始的网格顺序
- ⇒ 底层颜色可用**已验证的着色链**（`ARCH_182`/`ARCH_184`）直接算出 ⇒ 可精确预测一层混合；
- * `prev` 为**混合**的 = 147 + 30 + 7 = **184** 个 ⇒ 形成三层链，需更长历史，可先排除。
- ⇒ 下一轮混合颜色验证的有效样本约 **52,407** 个像素。

---

## `ARCH_18_input_flow.md`

**标题**：ARCH_18 — 输入消费链（从 SceCtrl 到游戏逻辑） ｜ 1. 本模块的运行职责 ｜ 2. 完整流程 / 状态迁移图 ｜ 2.1 总览（从宿主到游戏逻辑） ｜ 2.2 HLE 侧 `SceCtrlData` 布局（实证） ｜ 2.3 宿主→Vita 键位映射（宿主机侧，实证） ｜ 2.4 游戏侧输入抽象状态迁移（实证） ｜ 2.5 `0x811E6A78` 的字段搬运（实证，逐条） ｜ 3. 关键函数地址表 ｜ 4. 关键数据结构（字段偏移 + 含义 + 推测标注） ｜ 4.1 `SceCtrlData`（guest 栈上，0x20 字节） ｜ 4.2 全局输入表（引擎侧，实证） ｜ 4.3 待验证项：`out[]` 与 lx/ly/rx/ry 的对应（推测） ｜ 4.4 引擎输入任务（task 对象） ｜ 4.5 输入状态机对象（`0x8118F9C8`，**推测**字段） ｜ 4.6 应用状态表 `[0x813B11AC]`（步长 0x50，**运行期构造**） ｜ 5. 调用链（从入口到出口，逐层） ｜ 5.1 输入链（实证，逐层） ｜ 5.2 消费链（实证） ｜ 5.3 应用状态机与 title 序列（实证） ｜ 6. 时序 / 每帧发生什么 ｜ 6.1 每帧顺序（基于 `_h1149.log` 的实测计数） ｜ 6.2 时序对照（实测） ｜ 6.3 关键节奏陷阱（对照真实硬件）

**发现/结论**（327 条，最多列 40）：

- > 逆向依据：`eboot.bin`（ARM32 Thumb，基址 0x81000000，`file_off = VA - 0x81000000 + 0x10E0`，
- > 记号：**实证** = 有反汇编原文（含 VA 与指令）或日志原文；**推测** = 推理所得。
- > 本轮新工具：`C:\dev\p5d_build\_dis3.py`（严格顺序反汇编，遇到无法解码的 32 位
- 一个每帧输入任务 `0x81199FD8` 归一化成两张全局按位表（当前按下 / 本帧新按下）和
- 4 个摇杆字节，再往上由**具名场景状态机**（`0x8118F9C8` 及其兄弟）读取这些全局量做菜单
- ▼  （guest 侧：导入桩 0x812B1840 的 4 字节被改写为 `svc #0xAB; bx lr`）
- 0x811E6A78  ctrlPeekWrapper(port, out, changed*)        ← 唯一调用点 0x811E6A92
- 0x811DA9A4  ctrlFrameUpdate()                           ← 每帧 1 次
- │   · dt = (f32)[0x816CC638]      （0x8119957A: `vldr s0,[r0,#8]`）
- │       0x811E6A78(port, &out, &changed)            ; 0x811DAA0A
- │       new = cur & ~prev[port]                    ; 0x811DAA34 eors / 0x811DAA38 ands
- │       prev[port] = cur                            ; 0x811DAA52 strh
- │       按住计时器逻辑（0x811DAA94..0x811DAB06）
- 全局 0x816CDFB0 / 0x816CDFB2 / 0x816CDFB4 ...
- 0x8118F9C8 标题菜单输入状态机（25 状态，TBH @0x8118F9FA）
- │   状态 4 (0x8118FA4E)：`ldrh r0,[0x816CDFB0]; ands r0,#0x2000`  = CIRCLE 边沿
- │   状态 5 (0x8118FAC4)：`ldrh r1,[0x816CDFB4]; ands r0,r1,#0x10`  = UP
- │   ⇒ 状态 4/5 分别 → 11/14/20/23（按菜单索引 r4[+8]）与索引 ±1 环回
- 0x8105ABxx 族：菜单"选择项移动 / 确认 / 音效 / 转场"执行体（本模块只到这一层）
- 应用主状态机（0x813B11AC 表）→ 场景推进 → 进歌
- | 偏移 | 大小 | 内容 | 证据 |
- guest 侧 `0x811E6A96 cmp r0,#0 / 0x811E6A98 bge` 只判 `<0`，故 0 走成功分支。
- `0x811DA9A4` 对**每个端口**维护 4 字节一组的状态，字段与迁移如下：
- | `prev[port]` @ `0x816CDFB0+4*port` | `0x811DAA52 strh.w r3,[fp]` | 上一帧的原始按键位（u16） |
- | `new[port]` @ `0x816CDFB2+4*port` | `0x811DAA3C strh.w lr,[ip]` | **本帧新按下**（边沿） |
- | `down[port]` @ `0x816CDFB4+4*port` | `0x811DAA48 strh.w lr,[r8]` | 本帧生效的"按下事件"（含重复计时器结果） |
- | `lx[port]` @ `0x816CDAAC+2*port` | `0x811DAA5A strb r0,[r7]` | 左摇杆 X |
- | `ly[port]` @ `0x816CDAAD+2*port` | `0x811DAA60 strb r0,[r7,#1]` | 左摇杆 Y |
- | `rx[port]` @ `0x816CDAA8+2*port` | `0x811DAA66 strb r0,[r6]` | 右摇杆 X |
- | `ry[port]` @ `0x816CDAA9+2*port` | `0x811DAA6C strb r0,[r6,#1]` | 右摇杆 Y |
- | `sel[port]` @ `0x816CDAA4+2*port` | `0x811DAA6E strh.w r5,[sl]` | 端口选择/占位（u16，主循环里为 0，仅 `out[0]==2` 时置 1） |
- | `holdT[port]` @ `0x81680148+4*port` | `0x811DAA50 str r0,[r4]` / `0x811DAAA8 vstr s0,[r4]` | 连打/长按计时器（f32） |
- 关键指令原文（`0x811DA9A4` 内，逐条严格顺序解码）：
- 811DAA42  beq       #0x811daa94          ; 原始位没变 → 走计时器分支
- 811DAA4C  movt      r0, #0x3e6e          ; r0 = 0x3E6EEEEF ≈ 0.4666f
- 计时器分支（`0x811DAA94`）：
- 811DAABA  vsub.f32  s0, s0, s16          ; s16 = dt（0x816CC638）
- 811DAAB4  movw      r0, #0xcccd ; movt r0, #0x3dcc   ; 0x3DCCCCCD = 0.1f
- 811DAAAC/0x811DAAA8 vstr s0,[r4]
- 811DAABC/0x811DAAC0 str r0,[r4]  (重装 0.1f)

---

## `ARCH_190_SECOND_GEOMETRY.md`

**标题**：ARCH_190 —— 换一套几何复验：抓出**两处我自己的问题**（一处 harness、一处过强不变式） ｜ 1. 复验设置 ｜ 2. 问题一：校验器输入路径**硬编码**（harness 缺陷，已修） ｜ 3. 问题二：`判据3` 是**过强的不变式**（我自己的推断错误，予以撤回） ｜ 4. 有效不变式在第二个几何上复现 ｜ 5. 验证总账（更新，含撤回） ｜ 6. 下一轮

**发现/结论**（10 条，最多列 40）：

- > goal 第 30 轮。动机：上一轮的顺序不变式只在服装 10 上验过，需换几何确认结论不是帧特例。
- `判据3` 我原本写的是「整块被覆盖 ⇒ 成品像素不是背景」。**这条不是有效不变式** ——
- 一条在单个样本上「刚好成立」的强断言，换几何就会露馅。
- ⇒ **撤回 `判据3`**（`ARCH_183` §2 与 `ARCH_189` 中引用到它的地方同样作废）。
- 判据2  全未覆盖 ⇒ 背景（0 例外）        ✔
- * **顺序不变式在第二个几何上同样零违例** ⇒ 不是帧特例；
- —— 说明该分布是**数据相关**的，这也提醒：任何基于分布的结论都要注明样本。
- | `cov == id` 计数、**全未覆盖 ⇒ 背景** | ✔ 两个几何都复现 |
- | ~~整块覆盖 ⇒ 非背景~~ | ✘ **撤回**（过强，不是不变式） |
- 而它们都只有在**换样本复验**时才暴露 ⇒ 「换第二个几何复验」应成为固定步骤。

---

## `ARCH_191_HANDOFF.md`

**标题**：ARCH_191 —— 阶段 B 状态交接（31/40 轮） ｜ 1. 目标结构 ｜ 2. 线 D：已**逐项验证**的清单（这是本项目最扎实的部分） ｜ 3. ★ 被**推翻/撤回**的结论（不要再用） ｜ 4. 线 C：舞者坐标 —— 已排除的来源（各有证据） ｜ 5. 线 D 剩余的两项（具体到可直接动手） ｜ 5.1 混合的颜色数值（仪器已就绪） ｜ 5.2 `glcompare`（GPU 后端本体） ｜ 6. 需要**用户**决定的两件事（已问两次，未获答复） ｜ 7. 关键路径速查

**发现/结论**（11 条，最多列 40）：

- > 目的：让后续工作不必重新推导；并**明确列出被推翻过的结论**，避免误用。
- | 语义 | 证据 | 文档 |
- | 有效不变式：`cov` 计数 == `id` 计数；**全未覆盖 ⇒ 必为背景** | 两个几何都复现 | 183,190 |
- | `ARCH_183` §2 / `ARCH_189` | 「整块被覆盖 ⇒ 成品像素非背景」作为不变式 | **撤回**：三角形可**正当**画出恰为背景色 `(16,16,16)` 的颜色；在服装 10 上恰为 0 只是偶然 |
- **方法论**：以上除第一条外，**全部是我自己验证装置的缺陷**（硬编码、质心取样、照注释复原、过强不变式、陈旧注释、误信同名文件）。⇒ 固定做法：**关键结论换第二个几何复验**（`ARCH_190` 立）。
- `stage_format` DSFT/DTBL（常量 + id）、运行时格式表（3×0xDE0 堆对象，由 DTBL 装配）、
- **剩余未试的两条**：① 逆 `0x81047400..0x81047940` 建表函数；
- ② 追 `0x8103CCB4` 之后舞者**变换的应用点**。**均无「再有 N 轮就能通」的估计。**
- （`prev` 为混合的 184 个形成三层链，可先排除）；
- - **注意**：3×3 下采样 ⇒ 应按 SSAA 逐像素算完再盒式平均；且只统计 9 个像素 `id`/`prev`
- - 路线已实测可行：`_glprobe/glprobe.cpp`（RTX 5060 / OpenGL 4.6 / 3.3 core OK）、

---

## `ARCH_192_THIRD_GEOMETRY.md`

**标题**：ARCH_192 —— 第三个几何复验：有效不变式再次复现 ｜ 1. 设置 ｜ 2. 结果 ｜ 3. 顺手修掉工具里一处陈旧硬编码 ｜ 4. 三个几何的汇总（供后续引用） ｜ 5. 剩余（未变）

**发现/结论**（6 条，最多列 40）：

- > goal 第 32 轮。按 `ARCH_190` 立的规矩（**关键结论换几何复验**）做第三次复验。
- **有效不变式三项全部第三次复现**（`cov==id` 计数、全未覆盖⇒背景、混合绘制顺序零违例）。
- 这里的「FAIL」是**预期**的：它再次印证该断言不是不变式（服装 10 为 0 只是偶然）。
- `Transparent→Add`（3）并存** ⇒ 混合组内部的先后与 `drawMethod` 取值无关，
- | 几何 | 顶点/三角形/贴图槽 | cov == id | 全未覆盖⇒背景 | 顺序不变式 | 多次写入像素 | 撤回判据3 例外 |
- * **`glcompare` 本体**：路线已实测可行（`_glprobe/*`），做法见 §5.2。

---

## `ARCH_193_BLEND_NOT_TRIGGERED.md`

**标题**：ARCH_193 —— 放宽判据得 100% 零误差；但**仍未触发混合**（含混合=0） ｜ 1. 先修掉两个问题 ｜ 2. 新增路径的结果 ｜ 3. 为什么含混合=0（这是我的实现不完备，不是数据问题） ｜ 4. 本轮净收获 ｜ 5. 下一轮

**发现/结论**（6 条，最多列 40）：

- > 条件是 `prev<0`（该像素是首写）⇒ 底色必为背景，于是每个像素可独立算出最终色。
- ⇒ 把 `texcheck.py` 的三个输入路径也改成**可传参**，并重新生成**同一批**产物。
- 修正后原路径恢复：`19785 块 / 19783 零误差 / 最大差 1` ✔
- ⇒ **混合的颜色数值仍未验证。** 缺的正是另一个分支：
- ⇒ base = col(prev)
- | 不透明/纹理链在**更宽判据**下 100.00% 零误差（24,435 块） | ✔ 新增证据 |

---

## `ARCH_194_BLEND_VERIFIED.md`

**标题**：ARCH_194 —— **混合颜色验证通过**（±1），阶段 B 的着色链全部验完 ｜ 1. 补的分支 ｜ 2. 结果 ｜ 3. 如实说明两点 ｜ 4. 阶段 B 的着色链**全部验完**（总账） ｜ 5. 下一轮

**发现/结论**（6 条，最多列 40）：

- prev < 0                    ⇒ base = 背景 (16,16,16)          // 该像素是首写
- prev >= 0 且 prev 是「直写」⇒ base = col(prev)
- 否则（prev 是 Add 或半透明）⇒ base 依赖更早的画布 ⇒ **本像素不可算，跳过整块**
- 2. **样本仍不大**：真正含混合的只有 66 个块。结论方向明确（±1），
- | 渲染语义 | 证据 |
- ⇒ **软件光栅的渲染语义已逐项复刻并验证到 ±1**。剩下的只有 `glcompare` 本体（把同一套语义搬到 GPU）。

---

## `ARCH_195_T75_RECHECK.md`

**标题**：ARCH_195 —— 换相机（`t=75`）复验：全部语义通过，样本放大 5 倍 ｜ 1. 相机差异（不是同一类镜头） ｜ 2. 结果：全部通过 ｜ 3. 结论 ｜ 4. 验证总账（最终形态） ｜ 5. 唯一剩下的一项 ｜ 6. 仍需用户决定（已问四次）

**发现/结论**（6 条，最多列 40）：

- 覆盖像素多 4.9 倍，结论逐项复现。
- | 渲染语义 | 证据（跨样本） |
- | `cov==id`、全未覆盖⇒背景 | 四个几何 + 两个时刻 |
- | ~~整块覆盖⇒非背景~~ | ✘ **已撤回**（`ARCH_190`） |
- ⇒ **阶段 B 的软件光栅渲染语义已逐项复刻并验证（±1，源自 float32/64 截断边界）**，
- 1. **Android 工具链**：本机无 SDK/NDK/JAVA_HOME ⇒ 最终目标无法交付。是否允许联网安装？

---

## `ARCH_196_GLCOMPARE_FIRST_RUN.md`

**标题**：ARCH_196 —— `glcompare` 首次跑通：GPU 覆盖与软件光栅 **99.54% 一致**，差异归因于**像素取样约定** ｜ 1. 关键简化：覆盖比对**不需要深度缓冲** ｜ 2. 运行结果 ｜ 3. ★ 差异归因：**像素角点 vs 像素中心取样** ｜ 4. 本轮意义 ｜ 5. 下一轮（明确、可执行） ｜ 6. 仍需用户决定（第五次）

**发现/结论**（5 条，最多列 40）：

- ⇒ `cov` 就等于「被任一（剔除后的）三角形覆盖」的**并集**。
- * **软件光栅**用**整数像素坐标**作采样点（`kPixCenter=false`，`px=x, py=y`）⇒ **像素角点**；
- 这不是笔误，而是 `ARCH_150` 实测出「与更早的参考实现逐像素完全一致」后**特意**定下的约定
- ⇒ **GPU 后端要复刻这套光栅化，必须补偿半个像素**：在 MVP 后叠加
- **Android 工具链**（无 SDK/NDK/JAVA_HOME ⇒ 最终目标无法交付）与

---

## `ARCH_197_GL_COVERAGE_MATCH.md`

**标题**：ARCH_197 —— ★ GPU 覆盖与软件光栅一致到 **7 像素 / 284,480（99.9975%）** ｜ 1. 补偿量的推导与两次试验 ｜ 2. 结论 ｜ 3. 阶段 B 的 GPU 侧现状

**发现/结论**（3 条，最多列 40）：

- 软件光栅在**整数像素坐标**取样（`kPixCenter=false`）⇒ **像素角点**；
- 但**方向必须实测**：
- 差异翻倍 —— 这一条**只能靠实测定向**，不能靠推导自信。

---

## `ARCH_198_FINAL_HANDOFF.md`

**标题**：ARCH_198 —— 最终交接（39/40 轮）· 取代 `ARCH_191` ｜ 1. 目标五条线的状态 ｜ 2. 线 D：已**逐项验证**（跨样本，全部可复现） ｜ 3. ★ 被推翻/撤回的结论（不要再用） ｜ 4. 下一轮（唯一可执行、且已备好脚手架） ｜ 5. 两条**需用户决定**（已问六次，仍未答复） ｜ 6. 关键路径速查

**发现/结论**（11 条，最多列 40）：

- | C | **舞者舞台槽位坐标**（goal 优先项） | **~0%** | 9 类资产来源全排除；社区无做法；**无可靠路径** |
- | `cov==id` 计数、**全未覆盖 ⇒ 必为背景** | 4 个几何 + 2 个时刻全复现 |
- | ★ **GPU 覆盖 vs 软件光栅覆盖** | **交集 284475 / 参考 284480，差异 7 像素（0.0025%）**；`_gfd/glcov.py` 一条命令复验 |
- | ★ **半像素补偿**：GL 采样点需平移 `(+1/W, +1/H)`（NDC） | 无补偿 2640 差异 → 负号 5272（更糟）→ **正号 7**（降 376×） |
- | `ARCH_183`/`189` | 「整块被覆盖 ⇒ 成品非背景」是不变式 | **撤回**：三角形可正当画出恰为背景色的颜色 |
- | `ARCH_160` 的 `GAP` 修复 | 以为输出宽度已是 420 | 下采样路径另有硬编码 `+8` ⇒ 实际 428（`ARCH_178` 修） |
- | `ARCH_196` §3 | 半像素补偿取 **负号** | **错**：正号才对（差异 2640→7）；方向只能实测定向 |
- | 多处 | 校验器**硬编码输入路径** | 会拿新掩码比旧图 ⇒ 假 FAIL；`ARCH_190` 起改为可传参 |
- 固定做法：① 关键结论**换第二个几何/时刻复验**；② 语义**逐字读参考实现**，不信注释/文档；
- 1. **Android 工具链**：本机无 SDK/NDK/JAVA_HOME ⇒「在手机上原生运行」这一最终目标**无法交付**。
- 2. **舞者坐标**：继续逆 eboot 建表函数（`0x81047400..0x81047940`；或追 `0x8103CCB4` 之后

---

## `ARCH_199_NEXT_SESSION_PROMPT.md`

**标题**：新会话启动提示（直接粘贴给新的对话） ｜ 请把下面这段作为新会话的第一条消息 ｜ 备注（给人类看，不必粘贴）

**发现/结论**（3 条，最多列 40）：

- 被撤回的结论、下一步做法、构建坑、路径速查）
- * 关键结论必须**换第二个几何/时刻复验**（脚本都在 C:\dev\p5d_build\_gfd\，
- * 每个里程碑都要有可复现证据，不得用近似 hack 冒充正解。

---

## `ARCH_19_timing.md`

**标题**：ARCH_19：节拍 / 计时 / 帧同步系统 —— 时间的产生 → 传播 → 消费 ｜ 1. 本模块的运行职责（一句话） ｜ 2. 完整流程 / 状态迁移图（本轮核心） ｜ 2.1 时间链路总图（产生 → 传播 → 消费） ｜ 2.2 状态迁移图：假时钟如何随游戏状态变化 ｜ 2.3 假时钟的定量分析（本轮核心量化） ｜ 2.4 一帧之内的时间调用分布（`_h1169.log` 同一 10 帧窗口） ｜ 3. 关键函数地址表 ｜ 4. 关键数据结构 ｜ 4.1 假时钟本体（宿主侧，`eboot_probe.cpp`） ｜ 4.2 引擎软时间轴（guest 侧，`0x812071F8`） ｜ 4.3 日期门全局 ｜ 4.4 影片时间轴（CRI 90kHz 域，与上述四个时钟**完全无关**） ｜ 5. 调用链（逐层） ｜ 5.1 链 A：启动期「假 RTC → 日期门」 ｜ 5.2 链 B：运行期「假进程时钟 → deltaTime → 动画/演出」 ｜ 5.3 链 C：定时等待（时钟被当超时用） ｜ 5.4 链 D：音频块节拍（画面被音频「假装」驱动） ｜ 6. 时序 / 每帧发生什么 ｜ 6.1 一帧（游戏帧 = 一次 `sceGxmDisplayQueueAddEntry`）的完整体时序 ｜ 6.2 时钟速度与真实时间的对照（本轮实测） ｜ 7. 阻塞点与可玩性评估 ｜ 7.1 本轮新发现：当前最新构建的停机点已从 BADMEM 迁移到「音频断言」 ｜ 7.2 时间系统对「打完一局」的影响评级

**发现/结论**（235 条，最多列 40）：

- > 分析对象：`C:\dev\p5d_build\eboot.bin`（PSVita SELF，内嵌 ELF @0x1000，加载基址 `0x81000000`，imports=410）
- > 反汇编：`py C:\dev\p5d_build\_p5d_dis.py <VA> <条数>`；映射 `file_off = VA - 0x81000000 + 0x10E0`
- > 主要日志：`_h1169.log`（最新，29442 行，10 帧后崩在 appSound 断言）、`_h1164.log`（147531 行，3060 帧）
- 本模块是**把 PSVita 的四个物理时钟域（RTC 墙钟 / 内核单调时钟 / GXM 送显节拍 / 音频块节拍）在 guest 侧的映射，以及游戏引擎在其上搭出来的唯一一套软时间轴**：引擎真正的「时间」不是任何一个 HLE 桩，而是 `0x812071F8` 用两次 `sceKernelGetProcessTimeWide` 之差算出的秒数；当前实现把两个内核时钟都做成「每调用 +16667µs 的假计数器」，于是引擎唯一的软时间轴变成**由调用次数驱动、与现实时间无
- │  g_fake_tick : 64 位全局假时钟，初值 0x100000（= 1048576 µs ≈ 1.048s）        │
- │  eboot_probe.cpp:5718  static SceUInt64 g_fake_tick=0x100000;                │
- │     = sceRtcGetCurrentTick(0x23F79274)  eboot_probe.cpp:5853 / 绑定 :6423     │
- │     = sceKernelGetProcessTimeWide(0xB110C123)  eboot_probe.cpp:5770 / :6417   │
- │     绑定 6 个 NID：DelayThread(0x4B675D05) / DelayThreadCB / DelayThread200 / │
- │     另绑 sceDisplayWaitVblankStartMultiCB(0x05F27764)  :6424                 │
- │     = sceAudioOutOutput(0x02DB3F5F)  eboot_probe.cpp:6300 / 绑定 :6429        │
- │  【A】RTC 分支 → 日期门（启动期，静态仅 2 个调用点，实测 936 次）              │
- │    0x811845E8  push {r4,lr} / sub sp,#8                                       │
- │    0x811845F8  add  r1, sp, #0        ; r1 = &out（64 位）                     │
- │    0x811845FA  movs r0, #1            ; r0 = 查询模式 1                        │
- │    0x811845FC  blx  #0x812b18b0       ; ← 实测 936 次的时钟导入桩              │
- │    0x81184600  ldr  r0, [sp]          ; 取回 64 位结果的低 32 位                │
- │    0x81184602  cmp  r0, #9 / bgt / cmp r0,#8 / ble   ; 取值为 9..11            │
- │    0x81184612/20/2E  str r1,[0x81670354]   ; ← 写「日期门」全局（1 或 0）      │
- │    另有一份孪生代码 0x8118464C：同一桩 → r0∈[9,10] → 写 0x81670358            │
- │    0x812071F8  push {r4,lr}                                                   │
- │    0x812071FA  movw r0,#0x9850 / movt r0,#0x8153                              │
- │    0x81207202  ldrsb.w r0,[r0]        ; 读初值标志 [0x81539850]               │
- │    0x81207206  cbz  r0, #0x8120722c   ; 首帧走 0x81207208，之后走 0x8120722C  │
- │    0x81207208  blx  #0x812b28e0       ; ① 取当前进程时间（64 位）              │
- │    0x81207214  stm.w r2,{r0,r1}       ; 存到 [0x8169E090]（64 位基准）         │
- │    0x81207222  strb r1,[0x81539850]   ; 清标志                                │
- │    0x81207224  movs r0,#0 / vmov s0,r0; 首帧返回 0.0 秒                       │
- │    0x8120722C  blx  #0x812b28e0       ; ② 再取一次当前进程时间                 │
- │    0x81207238  ldm  r2,{r2,r3}        ; 读回基准                               │
- │    0x8120723A  subs r0,r0,r2 / sbc.w r2,r1,r3   ; 64 位差 = 自上次调用以来的µs │
- │    0x81207254/58 vcvt.f64.u32 d16/d18  ; 高/低 32 位分别转双精度              │
- │    0x8120725C  movw r0,#0x2400 / movt r0,#0x4974  ; = 0x49742400 = 1e6 f32    │
- │    0x81207260  vmul.f64 d16,d16,d17   ; 高位 × 2^32                           │
- │    0x8120726C  vadd.f64 d16,d16,d18   ; 合成 64 位 µs 值的双精度表示           │
- │    0x81207270  vcvt.f32.f64 s1,d16    ; → 单精度 µs                          │
- │    0x81207274  vdiv.f32 s0,s1,s0      ; ÷ 1e6 ⇒ **秒（f32），返回值在 s0**     │
- │    0x81207278  pop {r4,pc}                                                   │
- │    ⇒ 该函数即引擎的 deltaTime（每帧调 2 次）                                  │
- │    0x8127C232  push {r4,lr}                                                   │

---

## `ARCH_1_boot.md`

**标题**：ARCH_1 — 启动与主循环（模块逆向分析文档） ｜ 1. 模块职责（一句话） ｜ 2. 关键函数地址表 ｜ 3. 关键数据结构（字段偏移 + 含义） ｜ 3.1 app 状态机（`[0x813B11AC]` 指向的状态表） ｜ 3.2 `titleSequence` 序列对象（实测 obj = `0xB2D4F01C`） ｜ 3.3 任务链表（全局 `0x815554C0`） ｜ 3.4 加载请求（LRQ）与请求管理器 ｜ 3.5 精灵对象（`app_spr.c`，`0x8102EF00` 使用） ｜ 3.6 GXM 初始化结构（日志直读） ｜ 4. 生命周期与调用关系 ｜ 4.1 宿主侧主循环（**真正每轮跑的循环**） ｜ 4.2 guest 侧没有单一「主循环」，而是 15 个协作线程 + 帧边界信号 ｜ 4.3 初始化顺序（时间线，全部为日志原文） ｜ 4.4 每帧三段结构（update / draw / present） ｜ 4.5 本模块最关键的调用链：**app 如何走出 title（state=6）** ｜ 4.6 **本次运行的终止链（最重要）** ｜ 4.7 退出条件 ｜ 4.8 场景序列全程（本模块可观测的状态迁移） ｜ 5. HLE 接口现状 ｜ 5.1 拦截机制 ｜ 5.2 被拦截的关键导入（按模块） ｜ 5.3 已知缺口（本次运行实测的 `impl=N` 导入，去重后共 16 个） ｜ 5.4 本工程为了「让流程走下去」注入的**非真机行为**（必须计入技术债）

**发现/结论**（208 条，最多列 40）：

- > 目标：PSVita《Persona 5 Dancing in Starlight》PCSH10109 的 `eboot.bin`（ARM32 Thumb，加载基址 `0x81000000`，imports=410，exports=1）
- > 本轮主要证据源（≥5 处）：
- > 2. 反汇编器 `py C:\dev\p5d_build\_p5d_dis.py <VA> <n>`（直接反汇编 `eboot.bin`）
- **约定**：文中每条结论后附证据；反汇编证据写作 `[DIS 0xVA] 指令原文`；日志证据写作 `[_h1164.log:行号] 原文`。
- 无法确证的结论一律标注 **（推测）**。
- `module_start = 0x81198464` 是这一整个「启动与主循环」模块的唯一入口：它把游戏自己的 **协作式多线程调度器 + app 状态机（状态表 @0x813B11AC）** 跑起来，用「状态表 → enter/update 回调 → 任务链表 key 查询 → 加载请求 worker」这条链推进 `LOGO → 影片 → title 序列 → mainmenu` 的场景序列，并以 `sceGxmDisplayQueueAddEntry`（每游戏帧一次）作
- | VA | 推测名 | 作用 | 证据 |
- | `0x81198464` | `module_start`（唯一导出） | 4 字节跳板：`bl 0x811982bc` + `nop; nop`；真正初始化在 `0x811982bc` | `[DIS 0x81198464] bl #0x811982bc` / `nop` / `nop`；`[_h1164.log:16] [INFO] SceModuleInfo@0x812B2B00 name=xrd757_psp2_p5d_master ... start=0x811
- | `0x811982BC` | 进程/CRT 初始化 | `push.w {r4,r5,r6,sb,sl,fp,lr}`；写全局、调 `0x812b2370`、`0x812b2240`、`0x812b2210` | `[DIS 0x811982BC] push.w {r4,r5,r6,sb,sl,fp,lr}`、`[DIS 0x811982D2] blx #0x812b2370` |
- | `0x811845C8` | app「base init」 | 应用基类初始化 | `[_h1164.log:410] [APPINIT1] base init 0x811845C8 lr=0x8115A34F` |
- | `0x8118464C` | app「MAIN init」 | 主应用对象初始化 | `[_h1164.log:25651] [APPINIT2] MAIN init 0x8118464C lr=0x8115A369` |
- | `0x813B11AC` | **app 状态表基址全局** | `state_table = *(u32*)0x813B11AC`；元素步长 **0x50**，`+0x44`=enter，`+0x48`=update | `[DIS 0x81185802] movw r0,#0x1ac` + `[DIS 0x81185806] movt r0,#0x813b` + `[DIS 0x8118580C] add.w r0,r4,r4,lsl #2` + `[DIS 0x81
- | `0x81185818` | 状态 **enter** 分发 | `blx [tbl+0x50*state+0x44]` | `[DIS 0x81185814] ldr r1,[r0,#0x44]` / `[DIS 0x81185818] blx r1` |
- | `0x8118586E` | 状态 **update** 分发 | `blx [tbl+0x50*state+0x48]`；`cmp r0,#1` 为 keep，否则状态回退/清零 | `[DIS 0x8118586C] ldr r0,[r0,#0x48]` / `[DIS 0x8118586E] blx r0` / `[DIS 0x81185870] cmp r0,#1` / `[DIS 0x81185872] beq #0x8118587e` |
- | `0x81185927` | `state=1` 的 update（mainmenu-load 阶段） | 该阶段每帧被调用 | `[_h1164.log:26193] [ST-FIRSTCALL] state=1 fn=0x81185927` |
- | `0x81185986` | `state=6` 的 update（title 阶段） | `bl 0x811980fc`；`cmp r0,#0` → 返回 0（走出去）否则返回 1（继续等） | `[DIS 0x81185986] push {r4,lr}` / `[DIS 0x81185988] bl #0x811980fc` / `[DIS 0x8118598C] cmp r0,#0`；日志 `[_h1164.log:83871] [ST-CHANGE] state
- | `0x811980FC` | 「title sequence 是否还在跑」查询 | `r0 = 0x8130B140`（字符串指针）→ `bl 0x8116F900`；返回任务节点的 `[node+0xC]` | `[DIS 0x811980FE] movw r0,#0xb140` / `[DIS 0x81198102] movt r0,#0x8130` / `[DIS 0x81198108] bl #0x8116f900`。字符串内容 = `'title sequenc
- | `0x8116F900` | 按 key 字符串查任务链表 | 先 `bl 0x81165288` 算 key 校验和；遍历 `[0x815554C0+4]` 比较 `[node+0x18]`，命中返回 `[node+0xC]` | `[DIS 0x8116F902] movw r1,#0x54c0` / `[DIS 0x8116F906] movt r1,#0x8155` / `[DIS 0x8116F90A] ldr r4,[r1,#4]` / `[DIS 0x811
- | `0x8116F8AC` | 序列推进助手 `f(node)` | 按 `node` 在链表中命中后取 `[node+0xC]`（序列对象） | `[DIS 0x8116F8B8] ldr r1,[r1,#4]` / `[DIS 0x8116F8C0] ldr r0,[r0,#0xc]`；`[_h1164.log:76815] [SEQDRV] #1 node=0xB32F201C` |
- | `0x81198172` | `titleSequence` 序列回调 | `ldr r1,[r4]`(state) → `cmp r1,#8` → `bhi 0x811982A4` → `tbh [pc,r1,lsl#1]` | `[DIS 0x8119817C] ldr r1,[r4]` / `[DIS 0x8119817E] cmp r1,#8` / `[DIS 0x81198180] bhi.w #0x811982a4` / `[DIS 0x81198184] t
- | `0x811982A4` | 序列结束（返回 0） | `movs r0,#0; pop {r4,r5,r6,pc}` | `[DIS 0x811982A4] movs r0,#0` / `[DIS 0x811982A6] pop {r4,r5,r6,pc}` |
- | `0x8119826A` | `titleSequence state=7` 分支（等异步任务） | `ldr r0,[r4,#8]`(任务对象) → `bl 0x81193A6A` → `beq 0x811982A4` | `[DIS 0x8119826A] ldr r0,[r4,#8]` / `[DIS 0x8119826C] bl #0x81193a6a` / `[DIS 0x81198272] beq #0x811982a4` |
- | `0x8117BA30` | **加载请求 worker** `worker(ctx=r0, req=r1)` | 检查 `req[+4]&1` 是否已完成；否则按 mode 分派，最终 `blx r5(ctx,req)` 调 handler | `[DIS 0x8117BA44] adds r7,r1,#0` / `[DIS 0x8117BA46] ldr r2,[r7,#4]` / `[DIS 0x8117BA4A] ands r0,r2,#1` / `[DIS 0x
- | `0x8117BB8A` | 请求 handler 分发点 | `blx r5(ctx,req)`；handler 返回 0 则请求本帧不完成 | 源码 `thumb_interpreter.cpp:1466` 探针 `[LHANDLER] handler=... req=... ctx=...`；日志 `[_h1164.log:37960] [LHANDLER #4000] handler=0x8117C7E7 req=0xB24CE02C ctx=0xB251A030
- | `0x8117C7E7` | 资源类型 handler：**sprite**（`app_spr.c`） | 观测到 48 次，是主力 handler | `[LHANDLER]` 去重统计：`0x8117C7E7`×48、`0x8117C8A1`×9、`0x8117C93B`×8、`0x8117CA5F`×1 |
- | `0x8117C3B6` | 请求管理器轮询点（LRQDUMP 打印点） | 打印 `mgr=.. busy=.. n=.. PENDING:` | `[_h1164.log:76679] [LRQDUMP q=8200] mgr=0xB251A030 busy=1 n=32 PENDING: 31:0xB251C02C f4=0x0 HELP_P5 |` |
- | `0x8117C486` | 请求完成/清除 | `[MMCLEAR]` 打印点 | `[_h1164.log:29797] [MMCLEAR#1] done req=0xB1B1B02C nm='' lr=0x8109975E` |
- | `0x810995D4` | mainmenu-load `state2` 轮询函数 | 读全局 mgr `[0x81386CEC]`，`[mgr+4]&1` = busy | 源码 `thumb_interpreter.cpp:1543-1548`；`[_h1164.log:29861] [MMPOLL q=1 lr=0x811849A4] mgrPtr=0xB1B19030 [mgr+4]=0x1 busy=1` |
- | `0x810E8AD0` / `0x810E8ADA` | mainmenu-load 批次回调（协程） | 每帧被 tick，`this[0]` 为批次状态 | 源码 `thumb_interpreter.cpp:1558`；`[_h1164.log:31637] [MMCB#1] mainmenu-load cb this=0xB206E01C state=0 lr=0x810E8ADA` |
- | `0x81184964` | 资源系统初始化协程 | `this[0]` = 协程 state，0→1→… | 源码 `thumb_interpreter.cpp:1617-1621`；`[_h1164.log:29761] [INITCORO] this=0xB1B2A01C state=0 stateCount=1 total=1` |
- | `0x81163170` | logoTMX 子状态机门 | 返回非 0 表示有进展，`[[obj]+0x28]` bit31 = 待加载 | 源码 `thumb_interpreter.cpp:1520-1527`；`[_h1164.log:31239] [SPRSTEP #22] 0x81163170 返回 r0=0x0 (无进展/协程本帧返回 0) | obj=0xB204401C ... [..0x28]=0x80000000 bit31=1` |
- | `0x8102EF00` | **精灵对象协程状态机** `f(obj)` | `obj[0]&1`→完成；`obj[4]` = 流对象；`obj[0x20]` = 已创建子对象；`obj[0x28]` = 当前协程 | `[DIS 0x8102EF04] ldr r2,[r4]` / `[DIS 0x8102EF06] ands r0,r2,#1` / `[DIS 0x8102EF2C] ldr r0,[r4,#0x20]` / `[DIS 0x8102EF3A] ld
- | `0x8115FC28` | 库函数（「由流创建子对象」，返回存 `obj[0x20]`） | `cmp r1,#0; bne; add r1,sp,#0; bl 0x8115FD50`；**对 arg0 无 null 检查** | `[DIS 0x8115FC38] cmp r1,#0` / `[DIS 0x8115FC3C] add r1,sp,#0` / `[DIS 0x8115FC3E] bl #0x8115fd50`；被调用于 `[DIS 0x8102EF2C.
- | `0x8115FD50` | 库函数薄包装 | `adds r2,r1,#0; movs r1,#0; bl 0x81181CE4` | `[DIS 0x8115FD52] adds r2,r1,#0` / `[DIS 0x8115FD54] movs r1,#0` / `[DIS 0x8115FD56] bl #0x81181ce4` |
- | `0x81181CE4` | **本次崩溃所在函数** | 第一句解引用 arg0：`ldr r0,[r3,#0x94]`，**无 null 检查** | `[DIS 0x81181CF8] adds r3,r0,#0` / `[DIS 0x81181CFA] ldr.w r0,[r3,#0x94]`；`[_h1164.log:146552] [ERROR] ThumbExec badmem addr=0x00000094 @pc=0x81181CFA ... lr=0x
- | `0x811EE7C3` | GXM display queue callback（guest 侧） | 真机每 vsync 触发；本工程可选注入（`P5D_DISPCALL=1`） | `[_h1164.log:1498] [GXMINIT] flags=0x0 maxPending=2 displayQueueCallback=0x811EE7C3 cbDataSize=8 parameterBufferSize=0x1000000` |
- | `0x811E68C9` / `0x811E68FE` | worker 线程入口 / 其主循环 | 6 个 worker 线程共用入口 | `[_h1164.log] [THR-TAB] thid=0x6001 entry=0x811E68C9 alive=1 started=1 sched=2557 wait=sema pc=0x812B27A0`；源码 `eboot_probe.cpp:6120` 注释「worker 主循环 0x811E68FE 跑满 22744 
- | `0x812B2B00` | `SceModuleInfo` | `name=xrd757_psp2_p5d_master`，`export=0x812B2B5C..0x812B2B7C`，`import=0x812B2B7C..0x812B3298` | `[_h1164.log:16]` |
- | `0x815554C0` | **任务链表全局**（`+4` = 头节点） | 主镜像内、不在任何 ELF 段内（运行期才可见） | 源码 `thumb_interpreter.cpp:2432` 注释；`[DIS 0x8116F902] movw r1,#0x54c0` / `[DIS 0x8116F906] movt r1,#0x8155` |
- | `0x81386CEC` | mainmenu 加载管理器全局指针 | `[mgr+4] & 1` = busy | 源码 `thumb_interpreter.cpp:1545`；`[_h1164.log:29861]` |

---

## `ARCH_200_GPU_SHADING_MATCH.md`

**标题**：ARCH_200 —— ★ GPU 侧着色与软件光栅一致到 **99.93%**（三几何复验） ｜ 1. 结论 ｜ 2. ★ 本轮抓出的、`ARCH_198` 里**没有**的关键事实 ｜ 2.1 `gpu_ref.bmp` 是 420×640，而参考的 canvas 是 **1260×1920** ｜ 2.2 参考的下采样除数**恒为 `SS*SS`**，越界样本只跳过不计数 ｜ 2.3 三个"必须这么写"的实现选择（由对拍逼出来，不是风格偏好） ｜ 3. 本轮修掉的三个 bug（**全部是我自己装置的缺陷**，参考实现无一处错） ｜ 4. 逐字照抄的语义清单（`C:\dev\p5dsrc\src\assets\p5d_render_core.h`） ｜ 5. 复现（一条命令） ｜ 6. 残余差异的**归因**（不是"看起来差不多"） ｜ 7. 本轮产出 ｜ 8. 已知限制 / 未做 ｜ 9. 下一步（按优先级） ｜ 10. 需用户决定（第七次）

**发现/结论**（14 条，最多列 40）：

- ⇒ 着色模式**必须**与覆盖模式在同一空间（1260×1920）渲染，再由 CPU **逐字照抄**参考的
- | 1 | 全图**纯白**（轮廓却完全正确） | 顶点着色器只算了 `gl_Position`，**漏掉 `vUV=aUV; vMn=aMn; vTint=aTint; vSD=aSD;`** ⇒ 所有 out 变量取默认值 ⇒ `slot=0`、`lam=0.32` | CPU 侧计数显示"有贴图 6859"却画出常量色 ⇒ 转储 GLSL 源逐行核对后当场发现 |
- | 2 | 白 / 洋红 / 青**饱和** | `texelFetch(usampler2D,…)` 返回的是 **0..255 的整数**（整数纹理不做归一化），我当成归一化值**又乘了一次 255** | 诊断通道 `P5D_GLDBG=2`（直接输出 texel）显示 `(30,20,18)` 正常，而 `=4`（输出采样结果 `cc`）显示 `(255,255,255)` ⇒ 差值恰好 255 倍 |
- | `alpha<8` ⇒ 丢弃且**不写深度** | 1066 | `if(af<8.0) discard;`（**必须 discard**，不能用 alpha=0 + 混合） |
- | `DrawMethod==2` ⇒ `dst+c*al` | 1076 | `glBlendFunc(GL_SRC_ALPHA, GL_ONE)` |
- | 否则 ⇒ `dst*(1-al)+c*al` | 1077 | `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` |
- `ARCH_198` §3 的教训要求"关键结论换第二个判据"。这里用了**两个独立判据**：
- ⇒ **GPU 着色与"已验证的语义"一致到 99.91%**，残余是 ±1 级别的浮点边界。
- 会偶尔选中**不同三角形** ⇒ 颜色跳变。
- **两侧轮廓覆盖完全一致（仅参考 = 0，仅 GL = 0）** ⇒ 不是几何 / 剔除 / 投影问题。
- | `_glprobe/dbg_side.png` | 并排证据图（左=参考 中=GL 右=差异×16） |
- 分段在导出顺序上等价，但没有独立证据排除"段间重排"的影响（理论分析：段是导出顺序的**连续切分**，
- 3. 逐三角形 draw call 对照（把"分段等价"从理论变成实测）；
- - 若走 Gradle 构建，再加 Gradle 发行版 + 依赖缓存 ≈ 下载 0.5 GB / 占盘 1.5 GB ⇒ **总计下载 ~1.5 GB、占盘 ~6 GB**

---

## `ARCH_201_ANDROID_TOOLCHAIN.md`

**标题**：ARCH_201 —— Android 工具链已装好并**交叉编译验证通过**（线 E 解除硬阻塞） ｜ 1. 结论 ｜ 2. 装了什么（确切版本与路径） ｜ 3. 本轮产出 ｜ 4. ★ 踩到的三个坑（**全是 cmd 的**，浪费了约一小时，务必记住） ｜ 5. 已知限制 / 未做 ｜ 6. 下一步

**发现/结论**（2 条，最多列 40）：

- > 再用**静态路径的对照脚本**才排除掉 `setenv.bat`，最终定位到 `.cmd` 的控制权转移。
- * **没有连接的设备**（`adb devices` 为空）⇒ 真机验证命令已备好但未执行；

---

## `ARCH_202_ANDROID_APK.md`

**标题**：ARCH_202 —— 可安装的原生 APK 骨架（**不用 Gradle**） ｜ 1. 结论 ｜ 2. 为什么这条链路可以不要 Gradle ｜ 3. 打包链路（6 步，脚本里逐步打印） ｜ 4. 本机能做的验证（全部已通过） ｜ 5. ★ 缺失的那一环：**真机验证** ｜ 6. 已知限制 / 未做 ｜ 7. 下一步

**发现/结论**（3 条，最多列 40）：

- `adb devices` 为空 ⇒ **APK 从未在真实 Android 上启动过**。这是当前最大的未知。
- 本轮的代码刻意做成「**结论只靠 logcat 就能判定**」，所以只要插上手机就一次能验完：
- **未装模拟器**：`Get-WindowsOptionalFeature` 需要提权（本会话不可用），无法确认 WHPX 是否启用；

---

## `ARCH_203_GLES3_SHARED_BACKEND.md`

**标题**：ARCH_203 —— GPU 后端搬进 Android：抽出**平台无关共享头** + GLES3 移植 ｜ 1. 结论 ｜ 2. ★ 为什么要抽共享头（而不是给 Android 另写一份） ｜ 3. GLES3 移植的两个障碍 ｜ 4. 设备端会打印什么（判据全在 logcat） ｜ 5. 本轮产出 ｜ 6. 仍未验证的部分（诚实清单） ｜ 7. 下一步

**发现/结论**（2 条，最多列 40）：

- **重构的正确性判据只有一条：数字不能变。** 实测：
- | `src/assets/p5d_gl_backend.h` | ★ 平台无关 GPU 后端（Windows / Android 共用） |

---

## `ARCH_204_PERF_GPU_DOWNSAMPLE.md`

**标题**：ARCH_204 —— 性能画像 + 下采样搬 GPU（**20.7× 提升，逐位相同**） ｜ 1. 结论 ｜ 2. 性能画像（新增 `--bench` 模式） ｜ 3. GPU 下采样的实现要点（`p5d_gl_backend.h`） ｜ 4. ★ 踩到的两个坑：**辅助资源污染了主渲染状态** ｜ 5. Android 侧同步 ｜ 6. 本轮产出 ｜ 7. 仍未验证 / 下一步

**发现/结论**（5 条，最多列 40）：

- ⇒ 这与 `ARCH_161` §3 的软件光栅基线（**0.9 fps / p95 3262 ms**）对比：
- | 1 | GPU 下采样版整幅图变背景色（非背景像素 = 0） | 建四边形 VAO 后**没恢复主 VAO**，主渲染用了只含 4 个顶点的 VAO ⇒ 角色只画出一个三角形条带 |
- | 2 | 修掉 #1 后仍有 82% 的内容字节错（一致率 89.8%） | 建 `dsTex` 时 `glBindTexture` 落在 **texture unit 0** 上，把**角色贴图顶掉了** ⇒ 主渲染采样错纹理 |
- > 定位过程本身有方法论价值：先排除"整体平移"（`np.roll` 各方向试过，差异只增不减），
- > 再排除"uniform 传参"（写死尺寸的对照 shader，结果完全一样），

---

## `ARCH_205_PERTRI_AND_ANIM_LOAD.md`

**标题**：ARCH_205 —— 补完两个遗留项：逐三角形对照 + 真实动画负载画像 ｜ 1. 逐三角形 draw call 对照 —— **逐位相同** ｜ 2. ★ 真实动画负载画像（推翻上一轮的推测） ｜ 3. 更新后的性能全景 ｜ 4. 复现 ｜ 逐三角形对照（与分段绘制必须逐位相同） ｜ 真实动画负载 ｜ 5. 仍未做 / 下一步

**发现/结论**（5 条，最多列 40）：

- ⇒ **「分段 == 逐三角形」已是实测结论**，不是推理。段数少（1~3）带来的批处理收益是免费的正确性。
- `ARCH_204` 结尾我猜测"顶点每帧重传 1.5 MB 会成为下一个瓶颈"。实测**否**：
- > **方法论**：这是本项目第 N 次「实测推翻推测」。上一轮那句"会成为新瓶颈"写进了文档，
- **★ 在 Android 的真实用法里，"读回+下采样"这段根本不存在**（直接渲染到窗口上屏），
- 回归确认：三样本一致率与覆盖基线与 `ARCH_204` **完全相同**

---

## `ARCH_206_GLES_GATE_AND_FRAME_LOOP.md`

**标题**：ARCH_206 —— GLES 语法闸门 + Android 真实帧循环 ｜ 1. GLES 语法闸门（`_glprobe/glesyntax.cpp`） ｜ 2. Android 侧改成真实帧循环 ｜ 3. 本轮产出 ｜ 4. 真机到手后的完整判据（一次跑完） ｜ 5. 仍未验证 / 下一步

**发现/结论**（2 条，最多列 40）：

- | `selftestOnce()` | 仅首帧 | 读回与期望图逐像素比对，结论进 logcat；顺带填上屏纹理 |
- ⇒ 真机到手后，**帧率、平均帧时间、一致率三个数字一次全出**。

---

## `ARCH_207_REAL_DATA_AND_CRITERIA_FIX.md`

**标题**：ARCH_207 —— 真实游戏数据打通（Windows 侧）+ ★ 判据修正 ｜ 1. 资源位置（之前一直没找到） ｜ 2. 用真实资源生成**任意时刻**的配对产物 ｜ 3. 多时刻复验结果 ｜ 4. ★ 判据修正（本轮最重要的方法论收获） ｜ 5. 本轮产出 ｜ 6. 下一步：Android 侧加载真实数据

**发现/结论**（8 条，最多列 40）：

- > 并在多个时刻上做了完整复验 —— 过程中发现**原有判据本身不适用**，一并修正。
- > 「找不到资源」这类结论要写明搜索范围，否则会把"没找对地方"当成"资源不存在"。
- 实测可比对块从 37,769（t=0.0）涨到 179,192（t=2.5），**差 4.7 倍** —— 差异像素数跟着涨是必然的。
- 四个时刻的主判据实测：**99.91% / 99.85% / 99.92% / 99.85%**，而 Python 复刻与参考实现
- 一致 **99.99%~100%** ⇒ 语义复刻是正确的，残余全是已知的 ±1 浮点边界。
- 差异归因（`glshadeattr.py`）也确认**没有出现新的差异类别**：全部落在
- > 这条修正的意义：**判据必须能区分"实现错了"和"末位不同"**。旧判据会把浮点末位差异
- > "校验器硬编码输入路径 ⇒ 假 FAIL" 是同一类错误。

---

## `ARCH_208_DANCER_SLOT_DYNAMIC.md`

**标题**：ARCH_208 —— 舞者坐标：静态路线到边际，转**动态读取**（工具已就绪） ｜ 1. 更正：`ARCH_168` 对 `ARCH_164` 的"更正"基于一次混淆 ｜ 2. 本轮新查的两个候选，以及为什么**都判定为过拟合** ｜ 3. 转动态：读运行中的实体（方案已论证） ｜ 4. 本轮的落地状态 ｜ 5. 拿到坐标之后

**发现/结论**（10 条，最多列 40）：

- **结论**：`ARCH_164` 的原始读数（`描述符 +0x14..+0x20` 是 4 个 float）**没有被推翻**；
- | 候选 | 表面证据 | 推翻理由 |
- | rodata `0x81305160` 起的 24 字节记录（`id=5,6,7,8` + 3 个 float + 1 个 float） | 每组都含 `(0.0, ±60.0, -20.0/10.0/-72.0, 75/121/208/104)`，**形态极像摆位** | `id` 从 **5** 开始（不符合表起点）；紧邻的是 `"DANCE_BG"/"CUSTOM"/"PAUSE_D_R"` **字符串池** —— 更像是把无关字节按固定步长强行分组 |
- 另外实测：建表区 `0x81046C80..0x81048000` 的 240 条 store 里，**没有**任何一条写
- （`0x81305174`、`0x8103bc6c`、`0x81300930`）**都没有被 `movw/movt` 或地址字面量引用**
- （`0x81300930` 附近是一个 50/70/80/90/100/110/120/140 的等差数组）。
- ⇒ **静态路线已连续 9 轮没有拿到可验证的坐标**，而且本轮两次"很像是答案"的候选都经不起
- 起点/邻域检查。继续在 eboot 里猜结构，风险是**把巧合当结论**。
- 1. eboot 里存着类型名字符串 `"DancerEntity"`（`0x812F0694` 附近）；
- 这也是一个有价值的结果（能证伪"舞者不在原点"这个从未被证实的推断）。

---

## `ARCH_209_DANCER_SLOT_FINAL.md`

**标题**：ARCH_209 —— 舞者舞台槽位坐标：三条路线全部尝试后的**收尾结论** ｜ 1. 三条路线与各自的真实状态 ｜ 2. ★ 证伪：那个"看着像答案"的 8 点数组是 **AABB 包围盒** ｜ 3. 本轮建立的可复用能力（下次直接用） ｜ 4. 若要继续：只剩一条明确的路 ｜ 5. 已知限制的正式记录

**发现/结论**（11 条，最多列 40）：

- > **最终结论：仍作为已知限制保留**，但本轮产生了 3 项可复用能力与 1 项重要的排除性结论。
- ⇒ 该候选**作废**。这再次印证项目纪律：**唯一能区分"答案"与"巧合"的手段是换第二个样本**。
- 1. **`*(0x8132F460)` 可以读到了** —— 运行时舞曲格式表指针（`ARCH_165` §4 说"文件里读不到、只能动态拿"）；
- 实测 `0x826BB98C`（每次运行会变）。
- 2. **索引公式确认**：`表项 = table + grp*0x40000 + idx*0xDE0`；
- 反验 `0x826BB98C + 0x40000 + 8*0xDE0 = 0x8270288C` 与实读地址一致。
- 3. **★ 客机内存映射 = 宿主 `0x400000000` + 客机地址**（vita3k 用 JIT）。
- 实证：字符串 `"DancerEntity"` 位于宿主 `0x4812F0694`。
- ⇒ **可以直接读写 PSV 运行时内存**，`tools/vitadump.py` 已具备该能力。
- 4. **实体指针落点**：`0x81047FC4 bl 0x8103CE26` → `str.w r0,[fp+0x230]`（charaId=211 的第二个 → `[fp+0x234]`）；
- `fp` 由 `0x8115D36A` 分配，**不是全局对象**（这解释了为什么按全局指针找不到实体）。

---

## `ARCH_20_data_formats.md`

**标题**：ARCH_20：游戏数据格式（资源里的内容是什么） ｜ 1. 本模块的运行职责（一句话） ｜ 2. 完整流程 / 数据格式迁移图（本轮核心） ｜ 2.1 从 eboot 到谱面的整体数据流 ｜ 2.2 歌单的三级来源（关键结论） ｜ 2.3 CPK 成员分类统计（实测） ｜ 2.4 谱面文件名集合（实测，来自 data.cpk TOC） ｜ 3. 关键地址表（VA | 推测名 | 在流程中的位置 | 证据） ｜ 3.1 数据格式相关的内嵌字符串（提供模块划分的"官方名字"） ｜ 3.2 资源路径模板（决定"歌单/谱面怎么被找到"） ｜ 3.3 歌单标题表（**最重要的实测发现**） ｜ 3.4 帧调度与格式无关但决定"什么时候读谱面" ｜ 4. 关键数据结构（字段偏移 + 含义 + 推测标注） ｜ 4.1 CPK 容器（CRIWARE，**已完全解析**） ｜ 4.2 MSB / 字段集容器（**musicSplit.bin 头部，已解出外层**） ｜ 4.3 ARC 二级索引归档（**已完全解出**） ｜ 4.4 MNS 谱面（**头部已解出，音符流部分解出**） ｜ 4.5 歌单标题表（eboot 静态表，**实测**） ｜ 4.6 已有解析 vs 未解析 ｜ 5. 调用链（从入口到出口，逐层） ｜ 5.1 谱面加载链（推测，依据字符串归属） ｜ 5.2 歌单加载链（推测，依据源文件名） ｜ 5.3 CPK 读取链（宿主机侧，复用 ARCH_14，不重复展开） ｜ 6. 时序 / 每帧发生什么（与数据格式的交互点）

**发现/结论**（115 条，最多列 40）：

- > 分析对象：`C:\dev\p5d_build\eboot.bin`（ARM32 Thumb，基址 `0x81000000`，map: 文件偏移 = VA − 0x81000000 + 0x10E0）
- > 反汇编工具：`py C:\dev\p5d_build\_p5d_dis.py <十六进制VA> <条数>`
- 把 CPK 容器里那一层"没有扩展名语义、只有魔数和裸结构"的字节，翻译成游戏真正要的东西 —— **歌单（哪首歌存在、叫什么、BPM 多少）与谱面（哪一拍有音符）**；本轮把这条链路的三个关键格式（CPK / ARC / MNS）从魔数到字段偏移全部落实到可复现的字节级结论，并定位到**歌单与谱面的确切来源**。
- │ （证据：ARCH_14 §"IO" 章节；日志 `[IO] OPEN(file) 'app0:/data/data.cpk' fd=0x4002 size=1913750232`）
- │      VA 0x81303578（`mnsProcess.c` 字符串区，见 §3）
- VA 0x813A4CB4 起，5 个连续组，每组 43~44 条，每条 0x38 字节
- 证据：VA 0x81303BD8 = "BRAND NEW DAYS (YUYOYUPPE REMIX)" 被 0x3A5F90 与 0x3A7590 两处引用
- eboot 文件内 0x304C74 = "E:\P3DP5D\xrd757\program\app\music_select\musicData.c"
- 证据：VA 0x812F????（文件 0x2FA808 / 0x2FA814，位于 datTable.c 字符串区）
- > 实测验证：`p5d_cpk_inventory.py` 输出的 MNS 成员精确计数 = 147，distinct 曲目号 = 45。
- > **推测（高置信）**：`_{0,1,2,3}` 是难度或版本槽位；`_1` 单独出现在 301~311 段，与 P3 侧曲目号段（1xx）不对齐，**推测 3xx 为追加/DLC 曲目**。证据：`vita.cpk` 中 `SOUND/PREVIEW/PREVIEW_1xx`（126~143）与 `PREVIEW_2xx`（226~243）分成两段，`DANCE/UI/THUMBNAIL` 也只有 `119..143` 与 `219..243` 两段 —— 与 1xx/
- > VA 换算公式（本轮实证）：`VA = 文件偏移 − 0x10E0 + 0x81000000`。
- > 实证：字符串 `"BRAND NEW DAYS (YUYOYUPPE REMIX)"` 文件偏移 = `0x304CB8`，换算 VA = `0x81303BD8`，
- | VA | 字符串 | 含义 | 证据 |
- | `0x812F9608` | `E:\P3DP5D\xrd757\program\app\game\datTable.c` | 数据表总管理器 | eboot 文件偏移 0x2FA6E8 |
- | `0x81303B94` | `E:\P3DP5D\xrd757\program\app\music_select\musicData.c` | **歌单标题表所在源文件** | 文件偏移 0x304C74 |
- | `0x8130490C` | `E:\P3DP5D\xrd757\program\app\music_select\musicFormat.c` | 歌单格式化 | 文件偏移 0x3059EC |
- | `0x8130333C` | `E:\P3DP5D\xrd757\program\app\musical_note_sequencer\mnsProcess.c` | **谱面加载/推进** | 文件偏移 0x30441C |
- | `0x81302E50` | `E:\P3DP5D\xrd757\program\app\musical_note_sequencer\mnsHandle.c` | 谱面句柄 | 文件偏移 0x303F30 |
- | `0x81302EB8` | `...\musical_note_sequencer\mnsOption.c` | 谱面选项（NOTESPEED 等） | 文件偏移 0x303F98 |
- | `0x81303B00` | `...\musical_note_sequencer\mnsReplay.c` | 回放记录 | 文件偏移 0x304BE0 |
- | `0x812F0F20` | `...\app\dance\danceMusic.c` | 曲目播放器 | 文件偏移 0x2F2000 |
- | `0x812F08B8` | `...\app\dance\danceFormat.c` | 舞台/谱面格式化 | 文件偏移 0x2F1998 |
- | `0x81304FF8` | `...\app\mainmenu\music_select\musicSelectMain.c` | 选曲主逻辑 | 文件偏移 0x3060D8 |
- | `0x812FB850` | `...\app\mainmenu\collection\data\datCollection.c` | 收藏/成绩 | 文件偏移 0x2FC930 |
- | VA（字符串） | 模板 | 用途 | 证据 |
- | `0x813033B4` | `dance/mns/mns%03d_%d.bin` | **谱面主路径** | eboot 文件偏移 0x304494 |
- | `0x81303398` | `dance/mns/mns%03d_%ds.bin` | 谱面变体（推测：简化/练习版） | 文件偏移 0x304478 |
- | `0x81303380` | `MNS_SE` | 谱面音效资源名 | 文件偏移 0x304460 |
- | `0x813033D0` | `MNS_DATA` | 谱面数据资源名 | 文件偏移 0x3044B0 |
- | `0x812F1240` | `stage_format_%03d.bin` | 舞台格式（含 BPM/节拍） | 文件偏移 0x2F2320 |
- | `0x812F14A0` | `dance/table/stage_format_table.arc` | **舞台格式二级归档** | 文件偏移 0x2F2580 |
- | `0x812F14C4` | `dance/table/stage_event_table.arc` | 舞台事件二级归档 | 文件偏移 0x2F25A4 |
- | `0x812F1298` | `dance/table/enemy/stage_enemy_%03d.bin` | 敌人（谱面事件） | 文件偏移 0x2F2378 |
- | `0x812F12D8` | `dance/table/event/stage_event_%03d.bin` | 舞台事件 | 文件偏移 0x2F23B8 |
- | `0x812F1318` | `dance/table/persona/persona_event_%03d.bin` | Persona 演出 | 文件偏移 0x2F23F8 |
- | `0x812F9728` | `MUSIC_SPLIT` | **歌单分组表资源名** | 文件偏移 0x2FA808 |
- | `0x812F9734` | `data/musicSplit.bin` | 歌单分组表文件 | 文件偏移 0x2FA814 |
- | `0x812F0A78` | `sound/music/music%03d.acb` | 曲目音频（CRI ACB） | 文件偏移 0x2F1B58 |
- | `0x812F0A94` | `sound/music/music%03d.awb` | 曲目音频流 | 文件偏移 0x2F1B74 |

---

## `ARCH_210_CHAR_ASSEMBLY.md`

**标题**：ARCH_210 —— 角色装配层平台无关化（`p5d_char_assembly.h`） ｜ 1. 问题 ｜ 2. 交付物 ｜ 3. 硬判据与证据 ｜ 4. 过程中发现并修掉的真实缺陷 ｜ 4.1 `scene.time` 是**相机时钟**，不是「姿态时钟」 ｜ 5. 接口 ｜ 6. 刻意不做的事 ｜ 7. 遗留

**发现/结论**（7 条，最多列 40）：

- - Windows 上所有 GPU 着色结论（`ARCH_200`，`GL==Python` 99.85–99.92%）建立在**那份**装配之上；
- - 手机上若另写一份装配，这些结论**不可迁移**——验证过的东西和发布的东西不是同一个。
- 样本 2/3 是这一等价的**实测确认**，不是推理。
- 对照实验证据（同一组资源、同一 `t=2.0`）：
- ⇒ 修正为**无条件** `scene.time = opt.time`。
- 这条修正的必要性由「逐字节比较」直接暴露；只打日志不做像素比较会漏掉它。
- `scene.parts` 是**追加**语义 ⇒ 连续调用即为多角色同台（对应 demo `--duo`）。

---

## `ARCH_211_ANDROID_REAL_DATA.md`

**标题**：ARCH_211 —— Android 加载真实游戏数据（外部目录 + 只导出不光栅化） ｜ 1. 三段改动 ｜ ① 渲染核心：`Scene::exportOnly`（只导出、不光栅化） ｜ ② 新增 `src/assets/p5d_gpu_export.h` ｜ ③ Android 宿主接线（`src/android/main.cpp`） ｜ 2. 硬判据与证据 ｜ 2.1 导出面逐字节相同（新路径 == 已验证路径） ｜ 2.2 既有回归未被破坏 ｜ 2.3 性能（桌面实测，同一场景） ｜ 2.4 APK ｜ 3. 过程中发现并修掉的真实缺陷 ｜ 3.1 `aspect` 只在光栅路径里赋值 ⇒ 导出面的相机 aspect 为 0 ｜ 3.2 `Prepared::texPixels` 悬空（旧代码就有，属"靠运气对"） ｜ 4. 手机侧使用方式（待真机执行） ｜ 5. 遗留

**发现/结论**（8 条，最多列 40）：

- > 硬约束：游戏数据 **1.67 GiB**，不可能打进 APK（当前 APK 0.89 MiB）⇒ 必须从外部目录读。
- if (sc.exportOnly) return true;          // ★ 到这里就够了
- 默认 `false` ⇒ 既有行为逐位不变。
- ⇒ 手机每帧只付出「读盘之外的装配 + 导出」成本；光栅化完全移出运行时。
- 交叉编译已确认新路径**进了 .so**（在二进制里检索到 `真实数据装配 OK` / `未找到真实数据目录` /
- `std::vector<uint8_t> gpu`，函数返回即销毁 ⇒ `e.P.texPixels` 立刻悬空；当时没炸，
- - **未上真机**：以上全部是 Windows 侧与交叉编译层面的证据。
- 手机上的 `数据目录` 探测、EGL、每帧 `prepareScene` 的实际帧率都还没有实测数据。

---

## `ARCH_212_INPUT_LAYER.md`

**标题**：ARCH_212 —— 输入层：蓝牙 Xbox 手柄接线（不含音符语义） ｜ 1. 交付物 ｜ 2. 为什么不做「按键 → 音符」 ｜ 3. 证据（逐字采纳的部分） ｜ 4. 三处来源互相冲突：L/R 与 L2/R2 的归属 ｜ 5. 验证（无需真机） ｜ 过程中修掉的自身缺陷 ｜ 6. Android 接线与真机验证方式 ｜ 7. 遗留

**发现/结论**（16 条，最多列 40）：

- > **本轮范围经过确认**：只做**设备接线 + 规范输入状态**，**不做**「按键 → 音符」映射。
- 注意 :374 里的「轨道（△○×□/方向）」是**当时的推测**，不是已证结论。
- 都是凭印象设定 ⇒ 按本项目纪律（不得用反推值冒充正解）不写。
- | 摇杆死区 = **0.1**（`0x3DCCCCCD`） | `docs/progress.md:1939,1993`（反汇编游戏输入更新 `0x811DA822`） |
- | 每帧经 `sceTouchRead` 轮询输入（`0x812B1930`，NID `0x169A1D58`） | `docs/progress.md:12095,12118-12120` |
- | LB | `0x000400` | 「LB -> L1」 | 无此项 | `0x400` = **L2** |
- | LT | `0x000100` | 「LT -> L2 (LTRIGGER)」 | `LTRIGGER = 0x100` | `0x100` = **L1(L)** |
- | RB | `0x000800` | 「RB -> R1」 | 无此项 | `0x800` = **R2** |
- | RT | `0x000200` | 「RT -> R2 (RTRIGGER)」 | `RTRIGGER = 0x200` | `0x200` = **R1(R)** |
- `inputtest.exe` 用合成手柄状态断言产出的按钮位与摇杆值：**40 条全通过**（`INPUT_TEST_ALL_PASS`，exit 0）。
- - 面键/十字键/Start/Back/L3/R3 → PSV 位（每条都标了证据出处）
- ★ 检测到手柄输入: source=0x… deviceId=… (蓝牙 Xbox 手柄已接入)   ← 手柄真被识别
- 已确认存在于 `.so` 中。
- 以及各厂商手柄的轴编号差异，都还没有实测数据。
- - **无线手柄在无设备时会一直"未连接"** ⇒ `connected` 的判定目前是
- - 玩法本体（谱面播放 + 判定 + UI）仍不存在 ⇒ 手柄当前**没有可操控的对象**，

---

## `ARCH_213_MNS_CHART_PARTIAL.md`

**标题**：ARCH_213 —— MNS 谱面：时间轴已定，轨道语义**仍未定** ｜ 1. 方法：从 3 个样本目测，改成 147 个谱面全量交叉统计 ｜ 2. 已确定（有证据） ｜ 2.1 头部与记录长度 ｜ 2.2 时间轴：`A.hi16` 是**小节号** ｜ 2.3 `+0x14` 的真实公式与它的例外 ｜ 3. 未确定（如实记录） ｜ 3.1 `+0x1C` 数的**不是**记录条数 ｜ 3.2 `A.lo16` 与 `B` 的语义未解 ｜ 3.3 一项**与假设不符**的观测（必须留档） ｜ 4. 对「手柄→音符映射」的影响 ｜ 5. 下一步（可判定、按性价比排序）

**发现/结论**（15 条，最多列 40）：

- > **结论先说**：**时间轴搞清楚了（小节索引 + 独立参照定量吻合），但轨道语义没有攻克。**
- 上一轮的结论建立在 3 个文件的手工观察上。本轮改用 `dance/mns/` 下**全部 147 个谱面**
- 已知字段至少排到 `+0x1C`，且 `+0x20` 在全量恒为 0 ⇒ 自洽解只有 **`H=0x24`**：
- 在全流中是**孤例**（见 §3.2），不像一条正常记录 ⇒ 排除。）
- 记录按 `(u32 A, u32 B)` 拆开后，`A` 的高 16 位 `A.hi16` 就是小节索引。证据三条：
- **证据 1（独立参照，定量）**：用相机轨道的时长作为歌曲长度的独立来源
- 4 拍一小节 ⇒ 预测小节数 `= 时长 × BPM / 240`：
- **证据 2（难度无关性）**：同一首歌的四个难度，音符数差很多但小节网格相同。
- **证据 3（空缺率）**：147/147 个文件都有空缺小节，总空缺 1237/17377 = **7.12%**，
- 其 `+0x14` 只取 `0x0001012D / 0x0001012E / 0x0001012F` 三个值
- ⇒ **可靠歌号只有 `+0x0C`**；`+0x14` 不能当歌号用。
- 可能是「音符数」而记录里混有非音符记录（长按/判定辅助），但**没有证据，故不定论**。
- ⇒ 像「2 位枚举 + 一个 bit15 标志」，**但枚举各值代表什么完全未知**。
- ⇒ 要么流内有多条交错子流，要么记录顺序另有含义。
- 若 `A.hi16` 严格是小节号，同歌各难度应完全一致（§2.2 证据 2 在 108/116 上成立）。

---

## `ARCH_214_MNS_BEAT_DETERMINED.md`

**标题**：ARCH_214 —— MNS：拍位已定（推翻上一轮的猜测方向），但节奏仍不齐 ｜ 1. 结论一：`A.lo16` 低 2 位是**小节内的拍位**，不是轨道号 ｜ 2. 结论二：`A.lo16` 低 2 位不是轨道，那它留下的位置在 `B` 里（**未定论**） ｜ 3. 结论三（异常，必须留档）：小节组**不是**全局升序 ｜ 4. 也顺带证伪了一条：`+0x1C` 与记录条数的差不是对齐问题 ｜ 4.5 结论四：用「同拍位对照组」把 `B` 的低位拆开了 ｜ 5. 下一步（按可判定性排序）

**发现/结论**（17 条，最多列 40）：

- `ARCH_213` §3.2 里我把它当成「2 位枚举 ⇒ 像轨道」来猜，**方向错了**。判定实验：
- 92.95% vs 19.80% 是压倒性差距 ⇒ 记录在小节内**按拍位排序**。
- lo=0x0000 B=0x00000008   lo=0x0001 B=0x00000003   lo=0x0001 B=0x00000002
- lo=0x0002 B=0x00000004   lo=0x8002 B=0x00000004   lo=0x8003 B=0x00000004
- lo=0x8003 B=0x00000005
- ⇒ 「轨道（△○×□/方向）」**不在** `A.lo16` 里。它必须编码在别处。
- B=0x00000008  B=0x00000005  B=0x00000003  B=0x00000802  B=0x00000C03
- B=0x00001005  B=0x00010008  B=0x00001000  B=0x00000801 ...
- ⇒ 低位部分像「(a<<10) | b，a∈{0..4}、b∈{0..5,8}」。
- 但 `0x00010008` 又符合「(a<<16) | 8」⇒ 两种步长并存，
- ⇒ `ARCH_213` §2.2 的「`A.hi16` = 小节号」**仍然成立**（114 小节 vs 相机时长预测 114.75、
- **但这是猜测，没有证据，故只记为待验方向。**
- 所以 `ARCH_213` §3.1 的「`+0x1C` ≠ 记录条数」不是对齐造成的假象，结论保留。
- ⇒ 绝大多数时刻只有 1 个音符，2 个音符占近四分之一（双击/双押），与节奏游戏形态一致。
- 前三名全部是 `{2a, 2a+1}` 形式 ⇒ **`B & 7 = 2a + b`，`a ∈ {0,1,2}`、`b ∈ {0,1}`**。
- ⇒ 「音符号/轨道号」被**约束**到候选集合 `{A.bit2, A.bit8, B.bit0, B.bit1, B.bit2, B.bit11, B.bit12}` 之内
- 拍位 3 有两条），看同拍位内不同记录的 `B` 差在哪几位 ⇒ 那几位就是音符属性（轨道/类型）。

---

## `ARCH_215_MNS_CONSTRAINTS.md`

**标题**：ARCH_215 —— MNS：约束更紧了，但「子流拼接」假设被证伪；纯数据路线收益递减 ｜ 1. 硬约束一：`A.lo16` 只有 24 种取值，内容就是「标志 + 拍位」 ｜ 2. 硬约束二：同时音符只靠 **1 个位** 区分 ｜ 3. 被证伪的假设：文件不是「若干条按轨道分好的子流拼接」 ｜ ⚠ 一处我差点当成证据的废话 ｜ 4. 新线索：下降次数与**难度**相关 ｜ 5. 纠正的两处自身错误 ｜ 6. 对任务的影响与建议

**发现/结论**（12 条，最多列 40）：

- > 结果：**多了一条硬约束、证伪了一条假设、发现一条新线索、并纠正两处自身错误。**
- ⇒ `A.lo16` **基本就是 `(bit15 标志, 拍位)`**。`ARCH_214` §4.5 里把 `A.bit2`/`A.bit8`
- 同一键内的记录时间完全相同，是**同时按下**的音符 ⇒ 必须落在不同轨道。
- ⇒ 任意时刻（精确到拍内细分）最多只有 **2 个**可区分的音符，
- 而 1 个位**装不下 4 条轨道**。这与「△○×□ 四轨」直接冲突 ⇒
- ⇒ **假设不成立**（若是 4 条子流，应几乎全部集中在 3 次）。
- 这**不是证据**：按下降点切段，每段内部当然非减——**构造上必然成立**。
- 把它列出来只是为了避免它日后被误读成支持性证据。
- ⇒ **难度 0 的谱面与难度 2/3 的谱面在这条性质上系统性不同。**
- 改为**按文件分别分组**后重跑，§2 的数据是修正后的。
- 2. **`A.bit2`/`A.bit8` 作为轨道候选是过度乐观**（见 §1），已在 `ARCH_214` 之后修正。
- 每轮能新增的只是「不是它」的排除项（§1 排除 lo16、§2 排除单个位、§3 排除子流）。

---

## `ARCH_216_MNS_STATIC_ANALYSIS.md`

**标题**：ARCH_216 —— MNS 静态分析：原料就位、映射自证、两条文档结论被推翻 ｜ 1. 原料盘点（已确认可用） ｜ 2. VA ↔ 文件偏移映射：**6/6 自证通过** ｜ 3. 新发现：**代码里的指针字面量不是 VA** ｜ 4. 两条文档结论被推翻 / 修正 ｜ 4.1 `MNS_DATA = 0x8130F234` **是错的** ｜ 4.2 「反汇编字面量附近就是引用代码」在本镜像上**不成立** ｜ 5. 新线索：0x38 字节记录的静态表（与 `ARCH_20 §4.5` 独立吻合） ｜ 6. 本轮未做到的事（如实记） ｜ 7. 下一步（按顺序，都可判定）

**发现/结论**（36 条，最多列 40）：

- > 本轮是转向静态分析的第一轮，产出的是**基础设施**与**两条纠正**，不是最终答案。
- file_off = va - 0x81000000 + 0x10E0
- | `0x8130333C` | `mnsProcess.c` 全路径 | ✅ |
- | `0x813033B4` | `dance/mns/mns%03d_%d.bin` | ✅ |
- | `0x81303398` | `dance/mns/mns%03d_%ds.bin` | ✅ |
- | `0x81302E50` | `mnsHandle.c` | ✅ |
- | `0x81302EB8` | `mnsOption.c` | ✅ |
- | `0x81303B00` | `mnsReplay.c` | ✅ |
- 且 `ARCH_20_data_formats.md` 记的「`mnsProcess.c` 字符串文件偏移 0x30441C」与
- `0x8130333C-0x81000000+0x10E0 = 0x30441C` **完全吻合** ⇒ 三方互证。
- stored_literal = 0x81000000 + 目标文件偏移
- 等价地         目标文件偏移 = stored_literal - 0x81000000
- 自证：`dance/mns/mns%03d_%d.bin` 的文件偏移是 `0x304494`，
- 而 `0x81304494`（`=0x81000000+0x304494`）在二进制里**恰好命中 1 次**；
- 它的 VA（`0x813033B4`）与 `VA+0x10E0` 都命中 0 次。
- **镜像的运行时基址不是 `0x81000000`，而是 `0x81000000 - 0x10E0 = 0x80FFEF20`。**
- `ARCH_20_data_formats.md` §8 把 `MNS_DATA`（`0x8130F234`）当作「资源名交叉引用」的入口线索。
- 实测该 VA 处的字符串是：
- VA 0x8130F234 -> 'is already attached.'
- 其后是整片 AWB/ACB/ACF 音频库的断言消息（`Specified AWB slot '%s' is already attached.`、
- `ACF is not registered.`…）。⇒ 该区域**属于音频库，与 MNS 无关**，
- 按 §3 的编码找到 `0x81304494` 的唯一命中位置在 **文件偏移 0x3A6E38**。
- 对它前 0x200 字节做 Thumb 反汇编，得到的是 `movs r0, r0` 连发——**不是代码**。
- ⇒ 教训：**必须先判定区域是代码还是数据**，不能假设字面量周围是代码。
- 扫描"值落在 MNS 字符串区 `[0x81302000,0x81304000)`"的 u32，得到 53 个命中，
- **其中 50 个集中在 3 个相邻 4KB 页**（页 933/934/935，即 VA ≈ 0x813A4EB0 起），
- off 0x3A5F90 -> 0x81303BD8      off 0x3A5FC8 -> 0x81303BFC
- off 0x3A6000 -> 0x81303C24      off 0x3A6038 -> 0x81303C60
- **两条独立路径得到同一个 0x38 步长** ⇒ 该表的记录长度可认为确认。
- `dance/mns/mns%03d_%d.bin` 的指针就在这张表里（第 0x3A6E38 条附近）。
- ⇒ **下一步应找"谁读这张表"**：搜引用表基址（VA ≈ `0x813A4EB0`，按 §3 编码存储值为
- `0x813A4EB0 + 0x10E0 = 0x813A5F90`）的代码，顺着它就能到 MNS 记录解析处。
- 1. **划段 + 搜表基址引用**：按 §3 编码搜 `0x813A5F90`，命中点若落在代码段，
- 则反汇编其附近，找到读取该表的循环（步长 0x38）。
- 2. **顺藤到 MNS 解析**：在谱面路径模板被使用处（格式化 `mns%03d_%d.bin` 之后的函数）
- 4. 一旦定位到读表函数，**用 capstone 反汇编整段并逐指令核对**（按项目纪律：逐字读，不信猜测）。

---

## `ARCH_217_MNS_SEGMENTS.md`

**标题**：ARCH_217 —— MNS 静态分析：段表建立、指针编码纠错、两条否定结论 ｜ 1. 段表：终于能把「代码」和「数据」分开了 ｜ 2. 纠错：`ARCH_216` 的指针编码写错了 ｜ 3. 否定结论一：代码段里没有直接引用那张 0x38 表 ｜ 4. 否定结论二：**没有任何地方**用 32 位立即数比较 MNS 魔数 ｜ 这个否定为什么可信（换掉了最初那条不成立的判据） ｜ 5. 本轮没做到的 ｜ 6. 下一步（换策略）

**发现/结论**（34 条，最多列 40）：

- > 本轮产出：**段表（基础设施）+ 一处自身纠错 + 两条可用的否定结论**。仍未触到解析代码。
- | #0 | LOAD | **RX（代码）** | `[0x10E0, 0x3228F0)` | `[0x81000000, 0x81321810)` |
- | #1 | LOAD | **RW（数据）** | `[0x3228F0, 0x53E364)` | `[0x81325000, 0x81540A74)` |
- | #2 | SCE_RELA | — | `[0x53D370, …)` | — |
- | #3 | SCE_RELA | — | `[0x5C9700, …)` | — |
- 两个 LOAD 的 `p_vaddr − p_offset_instance` 都是 **`0x80FFEF20`**，与
- `ARCH_20`/`ARCH_216` 用字符串位置反推出的映射完全一致 ⇒ **段表与字符串自证互洽**。
- **这条立刻解释了 `ARCH_216` §4.2 的困惑**：那张 0x38 记录表在文件 `0x3A5F90`，
- 落在**数据段**里 —— 我对它做 Thumb 反汇编，得到 `movs r0, r0` 是必然的，
- **那不是"反汇编失败"，是我对数据反汇编了。**
- `VA + 0x10E0 == 目标文件偏移 + 0x81000000` 的**算术巧合**，不是真正的编码。
- 正确形式（本轮用表项实测判定）：
- stored = 目标**文件偏移** + 0x81000000
- | 表项值 | 约定 A：`val − 0x81000000` | 约定 B：`val − 0x80FFEF20` |
- | `0x81303BD8` | file `0x303BD8` = `'orial/tutorial_p5.bin'` | file `0x304CB8` = `'BRAND NEW DAYS (YUYOYUPPE REMIX)'` |
- | `0x81303BFC` | file `0x303BFC` = `'orial/c_tutorial_p5.bin'` | file `0x304CDC` = `'MASS DESTRUCTION ("P3" + "P3F" VER.)'` |
- | `0x81303C24` | file `0x303C24` = `''` | file `0x304D04` = `"WHEN THE MOON'S REACHING OUT STARS…"` |
- 的文件偏移是 `0x304494`，而 `0x304494 + 0x81000000 = 0x81304494`
- 在二进制里**恰好命中 1 次**（且是唯一一次）⇒ **约定 A 成立**。
- ⇒ 以后要找「谁引用了文件偏移 F 处的对象」，就搜 u32 值 `F + 0x81000000`。
- 在代码段 `[0x10E0, 0x3228F0)` 内按约定 A 搜指向表区间 `[0x3A5F00, 0x3A6F00)`
- 的 u32 ⇒ **0 命中**。
- ⇒ 表的访问不是"装绝对地址"，而是走**基址寄存器 + 索引**（基址应在运行时由重定位/全局变量得到）。
- （top: 页 0x50F000 x8、0x3A6000 x7、0x35F000 x6…），说明搜法本身有效，
- `movw/movt` 组合（或查 literal）。而魔数小端 u32 = `0x004D4E53`。
- | 全文件搜 u32 `0x004D4E53` | **0 命中** |
- | 全文件搜 u32 `0x4D4E5300`（大端序形式） | **0 命中** |
- 自检脚本 `mns_check_movw.py` 一开始用「movw 命中数 > 1000 ⇒ 解码器有效」作判据，
- 因此在这个宽松扫描下得到 0 命中 ⇒ 在任意更严格的正确边界扫描下也只能 ≤ 0 ⇒ **确实是 0**。
- ⇒ **MNS 解析器不是用 32 位立即数校验魔数的。** 可能原因（未验证）：
- 两条定位思路（直接字面量、魔数立即数）都被本轮否定掉了 —— 这是**排除性进展**，
- 1. **从「导出/日志」侧反推**：MNS 记录若被打印/断言引用，其结构体字段偏移
- 解析 `ARCH_217` §1 的两个 `SCE_RELA` 段，找指向数据段 `0x3A5F00` 附近的重定位项，
- 3. **改用 `llvm-objdump` 做线性反汇编**（NDK 里已有），比手写 2 字节步进扫描可靠，

---

## `ARCH_218_MNS_TABLE_AND_CODE_DATA_MIX.md`

**标题**：ARCH_218 —— MNS 静态分析：0x38 表结构解出；ADR 路线撞上"代码/数据混排" ｜ 1. 新成果：0x38 记录表的结构（VA 0x813A4EB0 起，file 0x3A5F90 起） ｜ 关键：MNS 路径模板也是这种记录的 `+0x00` ｜ 2. 否定结论三：没有任何东西指向这张表 ｜ 3. 三处方法论修正（本轮的实际价值） ｜ 3.1 capstone `disasm()` 会在第一个无法解码的字节处**直接停止** ｜ 3.2 capstone 对 Thumb `adr` 的操作数是**位移**，不是绝对地址 ｜ 3.3 ★ RX 段里**代码与只读数据混排** ⇒ 线性扫描必然产生假阳性 ｜ 4. 本轮没做到的 ｜ 5. 下一步（顺序已按"先修工具、再找代码"排好）

**发现/结论**（28 条，最多列 40）：

- > 目标进行中（第 2 轮）。本轮：**解出一张 0x38 记录的歌曲表结构** + **三处方法论修正**。仍未触到 MNS 记录解析代码。
- `dance/mns/mns%03d_%d.bin` 的指针位于 **file 0x3A6E38**，其后紧跟同一个结构的其余字段：
- +0x00 0x81304494  -> 'dance/mns/mns%03d_%d.bin'
- +0x04 0x00000080
- +0x08 0x42A80000  = 84.0
- +0x0C 0x42A80000  = 84.0
- +0x14 0x43D6C445  = 429.53…
- +0x18 0x140F0703
- ⇒ **这是一张歌曲/舞曲表**，谱面路径模板是其中一条记录的首字段。
- 按约定 A（`stored = 文件偏移 + 0x81000000`）全文件扫描：
- | 指向表区间 `[0x3A5F00,0x3A6F00)` 的 u32（任意段） | **0 处** |
- | 表所在 4KB 页 `0x3A5000` 被指向的次数 | **0 次** |
- | 代码段内指向 MNS 字符串区 `[0x81302000,0x81304000)` 的字面量 | **0 处**（上轮那 53 个命中全在数据段） |
- 对照：代码段**确实**有指向数据段的字面量（分散在 0x50F000 / 0x3AF000 / 0x35F000… 等页），
- ⇒ 表/字符串的引用**不是绝对字面量**。结合"字符串与代码同在 RX 段、距离 <4KB"，
- 修正后同窗口解出 **7945 条**（跳过 181 个无法解码位置）。
- - 直接用 `int(op_str)` ⇒ 得到 0x22c，落在 RX 段外；
- - 改用 `operands[1].imm` ⇒ 同样落在 RX 段外（实测 136 条 ADR 无一条进入 RX 段）。
- 正确公式（修正后目标全部落回 RX 段内页）：
- 修正 ADR 后得到 3 条指向 MNS 字符串区的命中，但逐条查看发现：
- 代码 VA 0x81302F24  ADR -> 0x813032CC : 'd757\\build\\psp2\\VC140\\../../../program/app/include/…'
- 代码 VA 0x81302F2C  ADR -> 0x813032C4 : '3DP5D\\xrd757\\build\\psp2\\VC140\\…'
- 这些是 **编译器 `__FILE__` 路径字符串**，而它们周围"反汇编"出来的是
- `lsls r0, r6, #2` / `movs r0, r0` / `b #0x81302e4e` 这类明显不合法的序列
- ⇒ **必须做函数边界识别**（`push {…,lr}` / `bl` 目标聚类），只对真实函数体反汇编，
- 否则 ADR 路线产出的全是噪声。这是本轮最重要的方法结论。
- 这样 §3.3 的假阳性会被结构性排除。
- 四条源路径字符串各自落在某函数的断言点，定位到它们等于定位到四个模块函数。

---

## `ARCH_219_MNS_BREAKTHROUGH.md`

**标题**：ARCH_219 —— MNS 静态分析：突破。找到加载函数、模块代码范围、以及我自己的编码 bug ｜ 1. ★ 首先纠错：MOVT 的编码我前两轮一直用错 ｜ 2. ★ 定位到 MNS 加载函数：VA 0x81126154 ｜ 顺带纠正文档 ｜ 3. ★ 圈定 MNS 模块的代码范围 ｜ 4. 下一步的落点（范围已从"6 MB"缩到"20 KB"） ｜ 5. 本轮仍未做到的

**发现/结论**（53 条，最多列 40）：

- > 目标进行中（第 3 轮）。本轮**推翻了前两轮的多条否定结论**，并定位到 MNS 加载函数与模块代码范围。
- 于是 `ARCH_217` 与 `ARCH_218` 里那几条「代码段里没有指向 MNS 字符串区的引用」的否定结论
- 修正后（`mns_re11.py`）：
- | 项 | 修正前 | 修正后 |
- ⇒ **绝对地址确实是靠 `movw/movt` 装载的**（标准 Thumb-2 惯用法），
- 修正编码后，谱面路径模板 `0x813033B4` 的装载点落在 **VA 0x811261B4 / 0x811261CC**。
- 反汇编该处，函数头是干净的 `push.w {r4,r5,r6,r7,r8,sb,sl,lr}`。**逐指令读出的逻辑**：
- 0x81126154  push.w {r4,r5,r6,r7,r8,sb,sl,lr}
- 0x81126158  sub    sp, #0x168
- 0x8112615A  sl = 0x812B414C                     ; 一个全局
- 0x81126168  mov    r8, r0
- 0x8112616A  bl     #0x8116f8a8
- 0x81126170  ldr    r1, [r4]      ; state
- 0x81126172  cmp    r1, #4
- 0x81126174  bhi.w  #0x811263c8                 ; state>4 直接返回
- 0x81126178  tbh    [pc, r1, lsl #1]            ; ★ 按 state(0..4) 的跳表
- 0x81126188  ldr    r1, [r4, #4]
- 0x8112618A  ands   r1, r1, #4                  ; 测 bit2
- 0x8112618E  str    r0, [r4]                    ; state = 2
- 0x81126190  ldr.w  r2, [r4, #0x1DC]            ; ★ 参数1 = 歌号
- 0x81126194  ldr.w  r3, [r4, #0x22C]            ; ★ 参数2 = 难度/变体
- 0x81126198  beq    #0x811261cc
- 0x8112619A  r1 = 0x81303398  "dance/mns/mns%03d_%ds.bin"
- 0x811261A6  blx    #0x812b2280                 ; ★ sprintf(缓冲, 模板, 歌号, 难度)
- 0x811261AC  bl     #0x8115fcb4                 ; ★ 存在性检查
- 0x811261B0  cmp    r0, #0
- 0x811261B2  bne    #0x811261da                 ; 存在 → 用 s 变体
- 0x811261B4  r1 = 0x813033B4  "dance/mns/mns%03d_%d.bin"
- 0x811261C6  blx    #0x812b2280                 ; 回退：主模板
- 0x811261DA  ldr.w  r1, [r4, #0x1DC]
- 0x811261E2  bl     #0x81093860
- 0x811261EC  r2 = 0x813033D0  "MNS_DATA"
- 0x811261F0  add    r3, sp, #0x124              ; r3 = 拼好的路径
- 0x811261F6  movs   r1, #0xF7                   ; ★ 命令码 0xF7
- 0x811261F8  bl     #0x8117c0ee                 ; ★ 资源加载(cmd, "MNS_DATA", 路径)
- `ARCH_20_data_formats.md` §8 把 `MNS_DATA` 的地址记成 `0x8130F234`（`ARCH_216` 已证明那里是 AWB 断言串）。
- **真正的 `MNS_DATA` 在 VA `0x813033D0`**（file 0x3044B0），实测读出：
- file 0x304478  VA 0x81303398 : dance/mns/mns%03d_%ds.bin
- file 0x304494  VA 0x813033B4 : dance/mns/mns%03d_%d.bin
- file 0x3044B0  VA 0x813033D0 : MNS_DATA          ← ★

---

## `ARCH_21_scene_graph.md`

**标题**：ARCH_21 — 场景图 / 渲染场景结构（引擎在画什么） ｜ 1. 本模块的运行职责（一句话） ｜ 2. 完整流程 / 状态迁移图（本轮核心） ｜ 2.1 场景对象的生命周期 ｜ 2.2 三条链对应的三段渲染（推测附证据） ｜ 2.3 一段渲染 pass 的完整状态机（内容层，逐条核证） ｜ 3. 关键函数地址表 ｜ 3.1 `sceGxmDraw` 的**全部静态调用点**（本轮新增，补正 ARCH_10 §3.2 的口径） ｜ 4. 关键数据结构（字段偏移 + 含义 + 推测标注） ｜ 4.1 `gfxState`（全局渲染状态对象，基址 `0x816CBC08`） ｜ 4.2 RT 包装对象（`gfxState+0x1E0` 等字段指向它） ｜ 4.3 场景图节点（`0x50` 字节） ｜ 4.4 精灵记录（`gfxState+0x6C` 起，每项 0x100 字节） ｜ 4.5 三条链的注册表（每列表 `0x30` 字节） ｜ 4.6 深度面链路（本轮新发现的 HLE 缺口，见 §7.2） ｜ 5. 调用链（从入口到出口） ｜ 5.1 每帧总链 ｜ 5.2 `0x811E7C6C` 内部（内容渲染的实际形态） ｜ 5.3 精灵快路径（`0x811EBB94`） ｜ 5.4 3D 模型路径（**存在，但本相位从未执行**） ｜ 5.5 `[FINDNAME-MISS]` 的 47 条不是缺口（纠正一个易误读点） ｜ 5.6 `0x811EA384` 的 6 个参数与 3 个栈参数 ｜ 5.7 虚表在文件里是空的（不影响结论） ｜ 6. 时序 / 每帧发生什么

**发现/结论**（215 条，最多列 40）：

- > 1. 映射 `file_off = VA - 0x81000000 + 0x10E0`，反汇编统一用
- 为根的全局对象链表：`0x816CC630` 选出当前列表，`0x816CD468[idx*4]` 是该列表的链表头；
- 每帧 `0x811CBE9A` 把三条链各走一遍，对每个节点用 `[node+0x20]` 查虚函数表
- `[0x814D2F2C]` 并调用 —— 于是「引擎画了多少东西」完全等于「这三条链上挂了多少个节点」，
- │ 调 0x811CB3A0(node, listIndex)                    │
- 0x811CB3A0  节点注册（append，O(1)，用尾指针）
- 811CB45C  idx   = gxp_rd(0x816CC630)              ; ← 当前列表索引（全局）
- 811CB46C  r0    = (idx<<2) + 0x816CD474           ; ← head 数组基址
- 811CB494  [0x816CE50C + idx<<2] = node            ; ← 第二条链（overlay）头插
- 811CB49C  [0x816CD468 + idx<<2] = node            ; ← 尾指针回写
- 0x811CBE9A(listIdx, mode)   ★ 场景图驱动（本模块的心脏）
- 811CBEFA  bl 0x811E8AAE(need, listIdx)            ; ← 该列表的内容渲染（一次 begin/draws/end）
- 811CBF02..3C  for(sb=[0x816CD468+r8]; sb; sb=[sb+0x0C])
- blx [0x814D2F2C + ([sb]<<2)] (sb+4, sb+8)   ; ★ 虚函数派发
- 811CBF4A..8E  同一循环走第二条链 [0x816CE50C+r8]
- 811CBF98..CA  同一循环走第三条链 [0x816CD498+r8]
- 811CBFD0  bl 0x811E8D82(mode, listIdx)            ; ← 该列表的 3D/后处理渲染
- `0x811CC010 / 0x811CC04E / 0x811CC234 / 0x811CC362 / 0x811CD4B0 / 0x811CD632 /
- 0x811CD6E4 / 0x811CD894 / 0x811CDA06 / 0x811CDA6A / 0x811CDB70` 共 11 处
- （另有 `0x811C2B32` 一处分配同尺寸但语义不同，见 §4.3）。
- | 链 1 | `0x816CD468 + idx*4` | `0x811E8AAE` → 5 个 draw 调用点 | **有材质/深度状态的内容层**（`beginScene→draws→endScene`） |
- | 链 2 | `0x816CE50C + idx*4` | `0x811CBE9A` 直接派发 + `0x811E90C0` | **叠加层**（overlay / 全屏合成） |
- | 链 3 | `0x816CD498 + idx*4` | `0x811E8D82` | **后处理链**（亮度直方图 / 自适应亮度 / 色调映射） |
- 证据：`0x811E8AAC` / `0x811E8D82` 内部三次完全同形的链表探测
- `811CBF62 bl #0x811e90c0` 与 `811CBFD0 bl #0x811e8d82` 分别作用在链 2 与链 3 上。
- 0x811E8AAE  取链头 → 若非空
- └─ 0x811EA460(idx)                     [推测：pass 前置登记]
- 811EA46C ldr r0,[r4,#0x3c] / 811EA470 bl #0x811a7d7c
- 811EA47C ldr r0,[r0,#0x60] / 811EA480 bl #0x812076a0
- 811EA488 adds.w r0,r0,#0x100 / 811EA48E bl #0x812072f2
- └─ 811E8B1C..8B3E  SetFrontDepthFunc(0x1C00000) / SetFrontDepthWriteEnable(0x100000)
- ├─ [node+0] → 虚表索引 → [0x814D2F2C + idx*4]([node+4], [node+8])
- │          blx 0x812b1470  sceGxmBeginScene(ctx, flags, rt, vreg=0,
- │          blx 0x812b1300  sceGxmSetViewport(ctx, s0..s5)  ← 从 [rt+0x38..0x50] 取
- │          blx 0x812b12c0  sceGxmSetFragmentTexture ×N
- │          blx 0x812b1310 / 0x812b17d0  SetMinFilter / SetMagFilter
- │          blx 0x812b14a0  sceGxmSetVertexStream
- │          blx 0x812b13b0  sceGxmSetUniformDataF
- │          blx 0x812b15f0  sceGxmDraw(ctx, prim=4(TRIANGLE_STRIP), count, ...)
- └─ 0x811EA3E6  sceGxmEndScene(ctx, 0, 0)

---

## `ARCH_220_MNS_TIME_FIELD_DECODED.md`

**标题**：ARCH_220 —— MNS 静态分析：**时间字段的位布局解出**（与数据侧独立互证） ｜ 1. 本轮先做的普查：MNS 模块里的位域与掩码 ｜ 2. ★ 主要成果：打包定点时间的位布局（0x811220B0 起） ｜ 与「纯数据」三轮结论的逐位互证 ｜ 3. 旁证与已排除项 ｜ 4. 仍未做到 ｜ 5. 下一步

**发现/结论**（18 条，最多列 40）：

- > 目标进行中（第 4 轮）。本轮在已圈定的 MNS 模块（VA 0x81121500–0x81126400，20 KB）里
- > 找到打包定点时间的解码码，并**与前三轮纯数据统计的结论逐位吻合**。
- | `ands rX, rY, #0x8000` | **1 处**（0x81123376，见 §3） |
- | `ubfx` / `sbfx` / `bfx` | **8 处**（0x811220B4 起 6 处 + 0x811230A4/0x811230CC 2 处） |
- ⇒ **解析器不用"掩码 + 8 字节步长"这套签名**，而是用 `ubfx` 做位域提取。
- 811220F2  cbz    r0, #0x811220f8     ; f15 == 0 则跳过
- 811220F4  vadd.f32 s5, s5, s1        ; s1 = 0.5 ⇒ lo += 0.5
- **⇒ 此前那个「bit2 / bit8 罕见出现、含义不明」的异常被解释掉了**：
- - **0x81123376 的 `ands r1, r0, #0x8000`**：紧跟 `ands r0, r0, #0x40000`（bit18）
- ⇒ 是**滚动速度**类判定，**不是**音符类型。
- - **0x811230A4 / 0x811230CC 的 `ubfx rX, r7, #9, #2`**：取 `r7` 的 bit9–10（4 个值），
- 索引一个建在栈上的 4 项表 `{sb, r8, 2, 3}`，取值 `+7` 后 `bl #0x81122210`。
- 不可能是每拍都变的轨道号。⇒ 记作**待定**，不当结论。
- ⇒ 两个可能（都未验证）：
- （这正是 `ARCH_215` §6 提出的新方向，本轮没有推翻也没有证实）；
- 2. 解析器不在这个 20 KB 范围内（范围是由 `mnsProcess.c` 断言串的装载点圈定的，
- 1. **给另三个模块圈范围**：用同一套（已修正的）`movw/movt` 装载点统计，
- 分别圈出 `mnsHandle.c`(0x81302E50) / `mnsOption.c`(0x81302EB8) / `mnsReplay.c`(0x81303B00)

---

## `ARCH_221_MNS_B_FIELD_ENUM.md`

**标题**：ARCH_221 —— MNS：`B & 7` 被确认为 6 值枚举（音符号的头号候选），但有一处疑点 ｜ 1. 工具：UBFX 解码器（先自检，再扫） ｜ 2. 并集范围判定：另外三个 mns 模块**没有**可直接定位的代码 ｜ 3. ★ 硬数据结论：`B` 的低位是 **6 值枚举**，不是时间 ｜ 3.1 `B` 的低 8 位只取 8 种值 ｜ 3.2 `B & 7` 恰好取 6 个值 ｜ 3.3 该分布在同一首歌的四个难度间**稳定** ｜ 3.4 因此 ｜ 4. ⚠ 必须留档的疑点：同时音符里出现 `(4,4)` 与 `(5,5)` ｜ 5. 下一步

**发现/结论**（17 条，最多列 40）：

- > **并集范围判定**、以及一条**硬数据结论**：`B` 的低位是 6 值枚举，不是时间。
- 所以这次先用 `ARCH_220` 里**人工从反汇编文本确认过的 8 条 ubfx** 反推并校验解码器：
- 0x811220B4 c0f30621  hw1=0xF3C0 hw2=0x2106  (lsb=8, w=7)  #8, #7     OK
- 0x811220DE c0f3c030  hw1=0xF3C0 hw2=0x30C0  (lsb=15,w=1)  #0xf,#1    OK
- 0x811230A4 c7f34122  hw1=0xF3C7 hw2=0x2241  (lsb=9, w=2)  #9, #2     OK
- 用同一套（已修正的）`movw/movt` 装载点统计：
- | `mnsProcess.c` | 0x8130333C | **19** |
- | `mnsHandle.c` | 0x81302E50 | **0** |
- | `mnsOption.c` | 0x81302EB8 | **0** |
- | `mnsReplay.c` | 0x81303B00 | **0** |
- ⇒ 并集仍只是 `mnsProcess.c` 圈出的 **VA [0x81121000, 0x81127000) = 24 KB**。
- **这判定掉了 `ARCH_220` §4 的可能性 2 的一半**：解析器若要靠这些断言串定位，
- 只可能在已看的范围里。但要留意——另外三个模块的断言串**在代码里根本不出现**，
- 说明它们的断言可能被编译掉了，或这些串只在数据表里被引用。
- 所以"解析器不在 24 KB 里"这个可能**并未被完全排除**。
- **取值只有 8 种。** 这**彻底排除**了"`B` 低位是持续时间或时间"的读法
- ⇒ **`B & 7` 是「6 值枚举」，且 `bit3`(0x8) 是与它并存的独立标志位**（15.06% 置位）。

---

## `ARCH_222_MNS_HETEROGENEOUS_STREAM.md`

**标题**：ARCH_222 —— MNS：★ 记录流是「音符 + 歌曲级事件」混排（变速事件已实证） ｜ 1. 先纠正一处分组错误 ｜ 2. 那批「同 `B&7` 同时刻」的组：两个不同的成因 ｜ 3. ★★ 实证：`B` 的低字节是**记录种类**，`0x82` = 变速事件 ｜ 3.1 现象 ｜ 3.2 决定性证据：这 6 条在四个难度间**完全相同** ｜ 3.3 自证：高 16 位就是 BPM×100 ｜ 3.4 由此确定的格式事实 ｜ 4. 仍未做到 ｜ 4.5 紧接着做的判定：`B & 7` **不是**轨道号 ｜ 4.6 ⚠ 新红旗：疑似"重复记录"的数量大到不合理 ｜ 5. 下一步

**发现/结论**（25 条，最多列 40）：

- > 目标进行中（第 6 轮）。本轮纠正了一处分组错误，并**实证**了记录流的异构性：
- ⇒ 同组两条记录可能分辨率不同、**并不是同一时刻**。
- ⇒ **数据里确实存在逐字节相同的重复记录**（574 组）。
- A=0x006B0000(小节107)  B=0x4A380082
- A=0x00880000(小节136)  B=0x4A380082
- A=0x008B0000(小节139)  B=0x47E00082
- A=0x009B0000(小节155)  B=0x47E00082
- A=0x00A90000(小节169)  B=0x4BC80082
- A=0x00BC0000(小节188)  B=0x49700082
- 这 6 条却**逐位相同** ⇒ **它们不是音符**。
- 而实测 `mns218_*` 的**表头 BPM = 190.000**（四个难度都是 190.000），
- ⇒ **`B` 高 16 位 = BPM×100，`B` 低字节 = 记录种类（`0x82` = 变速）。**
- 这是一条独立的、可复现的格式结论。
- 3. ⇒ `ARCH_221` 把 `B & 7` 读成"6 值枚举"**方向是对的**，
- - ✅ 变速事件：已实证（高 16 位 = BPM×100）
- 要跨越最后这一步，需要**渲染或判定侧**的证据（音符图标用哪张贴图、判定读哪个输入位），
- ⇒ **变速事件完全不是成因**（1977→1976）。即便把 `0x08` 也剔掉，仍有 **11.53%** 的同时刻组
- **⇒ `B & 7` 不是轨道号**（同一轨道同一时刻出现两个音符是不允许的）。
- 这**不是**"格式里本来就有重复记录"能解释的规模。更可能是：
- ⇒ **在做任何基于 `A` 分组的进一步结论之前，必须先查这个红旗。**
- 1. **★ 优先查 §4.6 的红旗**：dump `mns125_0.bin` 的原始记录流，
- 同时对照一个 `_3` 文件（如 `mns125_3`）确认差异只出现在 `_0`。
- **在与 `ARCH_215` §4 的「`_0` 异常」合并考虑前，不要再基于 `A` 分组下结论。**
- 2. 若红旗证实是**帧问题**（例如 `_0` 文件其实用不同的头部长度或记录长度），
- 4. 无论哪条，**轨道/按键的最终对应**都还需要**渲染或判定侧**的证据

---

## `ARCH_223_MNS_FALSE_ALARM_CORRECTED.md`

**标题**：ARCH_223 —— MNS：纠正一处我自己的假警报；`_0` 异常的真实形态是「同时刻堆叠」 ｜ 1. ★ 纠错：`ARCH_222` §4.6 的「25193 对逐字节重复 = 红旗」是**我的统计错误** ｜ 1.1 事实核对 ｜ 1.2 我的统计错在哪 ｜ 2. `_0` 异常的真实形态：**同时刻记录堆叠** ｜ 3. 与既有异常的关系 ｜ 3.5 进一步定位：异常集中在**文件尾部**，且**不连续** ｜ 4. 本轮没做到的 ｜ 5. 下一步

**发现/结论**（15 条，最多列 40）：

- 实测 `mns125_0` 单个组的贡献就达 **2400 对**（96 条记录里 60 条 `B=0x4`、36 条 `B=0x5`）。
- ⇒ **25193 完全由少数几个超大 `A` 组贡献，与"帧错位/读法错误"无关。**
- **这条纠正必须记下来**：上一轮我据此断言"我的记录切分或 `A` 读法有错"，
- A=0x00B88003 (小节184 拍3 bit15=1 → 时刻 184 + 3.5/4)  组内 96 条：B=0x4 ×60, B=0x5 ×36
- A=0x00B88002 (小节184 拍2 bit15=1)                      组内 19 条：B=0x4 ×14, B=0x5 ×5
- A=0x00B80003 (小节184 拍3)                              组内 17 条：B=0x4 ×17
- A=0x00B80002 (小节184 拍2)                              组内 16 条：B=0x4 ×12, B=0x5 ×4
- ⇒ 在**小节 184–185** 附近，出现了一簇 **≈150 条**记录，
- 本轮确认那个异常与本节是**同一批数据**：`_0` 文件里存在**成簇的、时间上不单调的堆叠记录**。
- ⇒ `_0` 的记录流不能当"与 `_2/_3` 同构、只是更稀疏"来处理。
- 查那 96 条 `A=0x00B88003` 在文件中的位置：
- ⇒ **异常集中在文件尾部（约后三分之一，索引 456–682），而且不是连续块，
- ⇒ **`_0` 文件的尾部是一段结构不同的区域。** 形态上像是文件里存在
- —— 差值杂乱，没有干净规则。**所以"两段结构"仍只是形态上的猜测，未证实。**
- 4. 无论哪条，**轨道/按键的最终对应需要渲染或判定侧证据**，

---

## `ARCH_224_MNS_ZERO_DIFF_QUANTIFIED.md`

**标题**：ARCH_224 —— MNS：`_0` 异常被量化（`cnt/n` = 0.56 vs 0.97）；并纠正一处排序误判 ｜ 1. ★ 纠错：原始 `A` **不是**按时间排序的 ｜ 2. H1：`+0x1C` 切两段 —— 118/147 支持 ｜ 3. ★★ H3：一个干净的、可量化的结构事实 ｜ 4. 对目标的影响：一个必须直说的问题 ｜ 6. ★ 又抓到一处工具缺陷（第三次），并得到一个重要否定 ｜ 6.1 缺陷：我的 ubfx 解码器**区分不了 UBFX 与 BFI** ｜ 6.2 否定：MNS 模块**不提取** `B` 的低 3 位 ｜ 6.3 新线索：模块里有若干 **`ldrb.w rX, [rY, #4]`** ｜ 7. 下一步

**发现/结论**（28 条，最多列 40）：

- > 并**纠正我自己一个方法论错误**。音符号对应仍未解出。
- （例：`A=0x00058001` 的时间早于 `A=0x00050003`，但数值更大）。
- **⇒ 我这两轮用「原始 `A` 的首个下降 / 单调性」做的判断全部无效**，包括：
- - `ARCH_214` §3 的"记录按小节分组、组间不升序"——**该结论本身仍由小节号（`A>>16`）得出，不受影响**，
- ⇒ 形态上确实像「`+0x1C` 条音符 + 一段别的东西」，但**不能说定**
- ⇒ **`+0x1C` 只在最难难度上接近记录条数（97%），在最简单难度上只覆盖 56%。**
- 这是一条**只凭字节就能确认、且量级明确**的差异，把 `ARCH_215` §4、
- - 我们能做到的是**在没有标签的情况下**把字段切出来（时间格式、记录种类、枚举取值）；
- 它的证据只存在于**游戏怎么用它**——即渲染（选哪张贴图）或判定（比哪个输入位）侧；
- 所以按目标的第二句（「若纯数据路线继续只有排除性结论，则转向静态分析」），
- > 不再找"解析器"（它可能根本不存在于 MNS 模块——数据格式本身已经由字节统计解出），
- 本轮交叉核对 capstone 时发现：`0x81125438/0x81125440` 其实是 **`bfi`（位域插入）**，
- `ARCH_221` §1 里那份 `(lsb,width)` 分布索引**也整体偏大**，需按此修正后重算。
- | 其中落在 MNS 模块 `[0x81121000,0x81127000)` | **0 条** |
- | 其中落在 UI 模块页 `0x81132000` | **0 条** |
- 结合 `ARCH_220` §1 已确认的「模块内 `ands #3` / `ands #7` 各 0 处」：
- **⇒ MNS 模块根本不用掩码或位域提取去取 `B` 的低 3 位。**
- 0x8112441C  ldrb.w ip, [ip, #4]
- 0x81124654  ldrb.w lr, [ip, #4]
- 0x81124702  ldrb.w lr, [lr, #4]
- 0x81124736  ldrb.w lr, [r0, #4]
- 0x81125754  ldrb.w lr, [r0, #4]
- 0x81125868  ldrb.w r2, [lr, #4]
- ⇒ **下一轮应当反汇编这几处**，看它们读出来的字节被拿去做**什么**
- 1. **★ 反汇编 §6.3 那 6 处 `ldrb.w rX, [rY, #4]` 的上下文**：
- 2. **按 §6.1 修正后重算 `ARCH_221` §1 的 ubfx 分布索引**（原表整体偏大）。
- 3. **把时间顺序的判定改成解码后比较**（§1）：所有涉及"A 的顺序"的旧结论都要复核。
- 已在 `ARCH_219` §3 统计出：3 处 / 4 处，页 `0x81126000`、`0x81132000`），

---

## `ARCH_225_MNS_BYTE_READ_BIT7.md`

**标题**：ARCH_225 —— MNS：找到**按字节读 `+4` 并测 bit7** 的代码（目前最贴合数据事实的一处） ｜ 1. 起点：`ARCH_224` §6.3 留下的 6 处 `ldrb.w rX, [rY, #4]` ｜ 2. 与数据侧事实的吻合点 ｜ 3. 最完整的一处：`0x8112464C` ｜ 3.1 结构布局吻合 ｜ 3.2 逻辑形态 ｜ 4. ⚠ 必须说清的不确定性 ｜ 6. ★ 追源头的结果：语义被钉住（哨兵 `0x7F` + bit7 有效位） ｜ 6.1 翻译成 C ｜ 6.2 与数据侧的吻合（这是本轮最有力的交叉印证） ｜ 6.3 因此音符 id 的取值域是 `{0,1,2,3,4,5,8}` —— **7 种** ｜ 7. 仍不确定的（必须说清） ｜ 8. 本轮没做到的 ｜ 6. 下一步

**发现/结论**（37 条，最多列 40）：

- 修正 ubfx 误解码器后确认：MNS 模块**不用掩码也不用 ubfx** 取 `B` 的低 3 位
- 0x8112441C  ldrb.w ip, [ip, #4]
- 0x81124654  ldrb.w lr, [ip, #4]      ← 紧跟 ands #0x80
- 0x81124702  ldrb.w lr, [lr, #4]
- 0x81124736  ldrb.w lr, [r0, #4]
- 0x81125754  ldrb.w lr, [r0, #4]      ← 紧跟 ands #0x80
- 0x81125868  ldrb.w r2, [lr, #4]      ← 紧跟 ands #0x80
- 而 `0x82` 已被独立证实为**变速事件**（`ARCH_222` §3：高 16 位 = BPM×100，
- ⇒ 这三处的 `ands #0x80` **恰好就是"音符 / 变速事件"的判别位**。
- 81124654  ldrb.w lr, [ip, #4]         ; ★ 读 +4 处的字节
- 81124664  beq    #0x8112466e           ; bit7==0 → 走另一支
- 8112466C  bne    #0x8112467e
- 8112466E  ldrb.w r0, [ip, #6]         ; ★ 再读 +6 处的字节
- 81124676  beq    #0x8112467e
- else                      → 跳过（bne 0x8112467e）
- ⇒ 「**bit7 = 种类标志**」+「**低 7 位与 `0x7F` 比较**」的组合，
- ⇒ **记作"最贴合的一处线索"，不作结论。** 要定论需证明这条对象链的源头是 MNS 数据。
- 追 `[sp+0x908]` 的**唯一写入点**（`0x81124326`）后，发现同一函数里
- `0x81124338` 起有一处**结构相同但语义明确**的代码：
- 81124340  ldrb  r0, [r0, #4]        ; ★ 读同一个 byte@+4
- 8112434E  beq   #0x81124352
- 81124350  movs  r0, #0x7f           ; ★ bit7 置位 ⇒ 值被**强制成 0x7F**
- 81124356  blx   lr                  ; 调用 [0x813A5C9C] 处的函数指针，r0 = 该值
- （`lr` 来自 `ldr.w lr, [r1]`，`r1 = 0x813A5C9C` —— 数据段里的一个函数指针。）
- call(fn_at(0x813A5C9C))(id, ...);
- ⇒ **`0x7F` 是"不是有效音符"的哨兵值，判别位就是 bit7。**
- - **bit7 只对 `0x82` 置位**，而 `0x82` 已由独立证据（高 16 位 = BPM×100，与 `mns218` 表头
- ⇒ 用上面那段逻辑去套：
- 这是"`B` 低字节 = 音符 id（含有效位）"这条结论目前最强的支持。
- `ARCH_221` §3.4 的"6 值枚举"应当修正为：**低字节是 7 值的 id 域（0–5 与 8），bit7 是有效位。**
- 反对的理由：三重解引用说明中间有对象图，无法排除是另一套结构碰巧同构。
- ⇒ **记为"语义已钉住、载体待确认"。**
- 1. **确认载体**：看 `[0x813A5C9C]` 处那个函数指针**指向哪个函数**，
- 2. **确认 `[sp+0x910]` 的来源**：它是 `bl #0x81016296` 的返回值。
- `0x8117c0ee(cmd=0xF7,"MNS_DATA",path)`）相接。
- 3. **在数据段 `0x813A5C9C` 附近找表**：若那里是一张**函数指针表**，
- 去 UI/渲染侧（`DANCE_UI_ARC`/`DANCE_UI_SPR` 装载点：页 `0x81126000`、`0x81132000`）

---

## `ARCH_226_MNS_MOVW_I_BIT_BUG.md`

**标题**：ARCH_226 —— MNS：第四处编码缺陷（MOVW 漏了 `i` 位）；影响范围已判定 ｜ 1. ★ 缺陷：MOVW 的立即数漏了 `i` 位（hw1 的 bit10） ｜ 1.1 现象 ｜ 1.2 影响范围（已逐个判定，不是笼统"全部作废"） ｜ 1.3 修正 ｜ 2. `0x813A5C9C` 到底是什么（用修正后的认识重看） ｜ 3. 本轮没做到的 ｜ 4. 直白的花费/收益判断（供决策） ｜ 5. 我建议的两个选项（需要你定）

**发现/结论**（29 条，最多列 40）：

- `ARCH_219` 里 capstone 在 `0x81126200` 显示 `movw r0, #0x5c9c`，
- 而我的 movw/movt 配对脚本算出 `0x813A549C`。本轮手工解码该处 4 字节（`45 f6 9c 40`）：
- **⇒ 凡 movw 的低 16 位 ≥ 0x800（即 `i`=1）的常量，我的配对值都偏了 0x800。**
- | `mnsProcess.c` = `0x8130333C` | `0x333C` | `ARCH_219` §3 的 19 个装载点 |
- | `mnsHandle.c` = `0x81302E50` | `0x2E50` | 同上（0 处） |
- | `mnsOption.c` = `0x81302EB8` | `0x2EB8` | 同上（0 处） |
- | `mnsReplay.c` = `0x81303B00` | `0x3B00` | 同上（0 处） |
- | 谱面模板 = `0x813033B4` | `0x33B4` | 装载点 `0x811261B4/CC` |
- | `MNS_DATA` = `0x813033D0` | `0x33D0` | `0x811261EC` |
- ⇒ **`ARCH_219` 的加载函数分析（逐指令读出的跳表 / sprintf / 资源加载链）仍然成立**，
- 因为它依据的是 **capstone 的反汇编文本**（那是正确的），不是我的配对值。
- `0x813A5C9C` 的处理）。**`ARCH_217`–`ARCH_221` 里凡引用配对常量的结论需按此复核。**
- 该地址的正确起点是 `0x813A5C9C`（file `0x3A6D7C`）。dump 出来：
- file 0x3A6D7C:  00660046 00000076 00000000 00000000 00000000 8130442C 00000080 43320000
- - `[+0]` = `0x00660046`、`[+4]` = `0x00000076`、`[+0xB8]` = `0`
- ⇒ **在文件里它们不是函数指针**。
- - 从 `+0x14` 起出现**间隔 0x38 的指针**：`+0x14=0x8130442C`、`+0x4C=0x81304440`、
- `+0x84=0x81304450`、`+0xBC=0x81304494`（最后这个正是**谱面路径模板**的槽）。
- ⇒ 形态上，`0x813A5C9C + 0x14` 才是**又一张 0x38 记录表**的首记录
- **但代码用的是 `0x813A5C9C`（比表首早 0x14）**，且取 `[+4]` 与 `[+0xB8]` —— 这两个偏移
- ⇒ 两种可能（都未验证）：
- 2. `0x813A5C9C` 不是表基址，而是**另一个全局对象**，恰好落在表的中间。
- **`ARCH_225` §6 里"`p` 指向音符对象"的判断因此仍是待确认的**，本轮无法推进。
- **已确实拿到的东西**（都有可复现证据）：
- | 结论 | 证据类型 |
- **代价评估**：继续走静态分析，每轮的边际产出主要是"排除项"和"修工具"；
- - **A：继续 MNS 静态分析**，但先把工具做扎实——写一个**基于 capstone 的通用反汇编/常量解析器**
- （不再手写指令解码），再用它重跑 `ARCH_217`–`ARCH_226` 里所有涉及配对常量的结论。
- 而"哪个键对应哪个音符"在拿到判定/图标证据前无法定论——

---

## `ARCH_227_MNS_RANGE_CORRECTED.md`

**标题**：ARCH_227 —— MNS：工具做扎实后，立刻推翻了一个错了三轮的**范围** ｜ 1. 新工具：`mns_tool.py`（不再手写任何指令解码） ｜ 2. ★ 新工具立刻推翻的错误：MNS 模块范围是 **108 KB**，不是 24 KB ｜ 3. 在正确的 108 KB 范围里重跑签名普查 ｜ 3.1 关于 `ands #3` 的澄清（一个**用正确范围得出的新否定**） ｜ 3.2 但翻出了大量此前完全没看过的东西 ｜ 4. 本轮没做到的 ｜ 5. 下一步（范围已修正，路线不变但搜索面大 4.5 倍）

**发现/结论**（14 条，最多列 40）：

- > 结果：新工具一上线就纠正了一个**约束了三轮搜索的错误范围**。
- - 线性反汇编带**跳间隙**（capstone 在无法解码处会停，必须自己跳过）；
- | `mnsProcess.c` = 0x8130333C | **19** | 19 ✓ | 0x81121000 / 0x81122000 / 0x81125000 / 0x81126000 |
- | `mnsHandle.c` = 0x81302E50 | **42** | **0** ✗ | 0x81119000 / 0x8111A000 / 0x8111B000 / 0x8111C000 |
- | `mnsOption.c` = 0x81302EB8 | **61** | **0** ✗ | 0x8111C000 / 0x8111D000 |
- | `mnsReplay.c` = 0x81303B00 | **15** | **0** ✗ | 0x81132000 / 0x81133000 |
- ⇒ **并集范围 VA `[0x81119000, 0x81134000)` = 108 KB**（是此前的 4.5 倍）。
- **顺带纠正**：`mnsReplay.c` 落在页 `0x81132000` —— 那正是我早先误标为"UI 模块页"的地方。
- `ARCH_224` §6.2 里"UI 模块页 0 命中"的结论，**页的归属本身就是错的**。
- 0x81119146  orrs r0, r0, #3      ← 置 bit0-1
- 0x8111983E  orrs r0, r0, #3      ← 置 bit0-1
- 0x8111F54C  （vst1.8 区域，非代码）
- ⇒ **即使把范围放大到 108 KB，整个 MNS 模块仍然没有真正的 `&3` 提取。**
- **只对 24 KB 切片成立**，其中 `&3` 一条已在 108 KB 上重新确认为否定。

---

## `ARCH_228_MNS_ID_REMAP_TABLE.md`

**标题**：ARCH_228 —— MNS：★ 找到**音符 id 的重映射表**（写到 `byte@+4` 上） ｜ 1. 线索：`ldrb [rY,#4]` 之后紧跟 `cmp rX, #8` ｜ 2. ★ 核心：`0x8111AEEE` 处的重映射（把表值写回 `+4`） ｜ 2.1 栈上建表（6 项，步长 4） ｜ 2.2 应用重映射 ｜ 2.3 这条代码说明了什么 ｜ 3. 尚未确定的关键一点：`X = [sp+0x94]` 是什么（本轮**没查到**） ｜ 4. 与目标的距离 ｜ 5. 下一步

**发现/结论**（56 条，最多列 40）：

- 0x8111A9AC (mnsHandle)  ldrb.w r4, [ip, #4]   → 0x8111A9BA cmp r4, #8
- 0x8111AEEE (mnsHandle)  ldrb   r2, [r1, #4]   → 0x8111AEF6 cmp r2, #8  + 索引
- 0x8111B1B2 (mnsHandle)  ldrb   r0, [r1, #4]   → 0x8111B1C2 cmp r0, #8
- 0x8111B688 (mnsHandle)  ldrb   r6, [r0, #4]   → 0x8111B6A2 cmp r6, #8
- 0x8111BB80 (mnsHandle)  ldrb   r1, [r1, #4]   → 0x8111BB88 cmp r1, #8
- 0x811211D2 (mnsProcess) ldrb   r1, [r1, #4]   → 0x811211DA cmp r1, #8
- 0x8111AEC2  movs r0, #1
- 0x8111AEC4  ldr  r1, [sp, #0x94]          ; X ← 见 §3
- 0x8111AEC6  movs r2, #3
- 0x8111AEC8  strd r0, r1, [sp, #0x20]      ; table[0]=1,  table[1]=X
- 0x8111AECC  movs r0, #2
- 0x8111AED0  movs r4, #5
- 0x8111AED2  strd r2, r0, [sp, #0x28]      ; table[2]=3,  table[3]=2
- 0x8111AED6  movs r1, #4
- 0x8111AED8  str  r4, [sp, #0x30]          ; table[4]=5
- 0x8111AEDC  str  r1, [sp, #0x34]          ; table[5]=4
- ⇒ **`table = {1, X, 3, 2, 5, 4}`**，基址 = `sp + 0x20`。
- 0x8111AEEC  ldr   r1, [r0, #0x14]
- 0x8111AEEE  ldrb  r2, [r1, #4]            ; r2 = byte@+4（音符 id）
- 0x8111AEF0  ands  r3, r2, #0x80           ; bit7 = 有效位
- 0x8111AEF4  bne   #0x8111aefa
- 0x8111AEF6  cmp   r2, #8
- 0x8111AEF8  beq   #0x8111af20             ; id == 8 → **不重映射**
- 0x8111AEFA  ldr   r0, [r0]
- 0x8111AEFC  ands  r0, r0, #0x10           ; 测某状态位 bit4
- 0x8111AF00  beq   #0x8111af12
- 0x8111AF02  cbz   r3, #0x8111af06
- 0x8111AF04  movs  r2, #0x7f               ; bit7 置位 ⇒ 索引 0x7F
- 0x8111AF06  mov   r0, sp
- 0x8111AF08  add.w r0, r0, r2, lsl #2      ; 地址 = sp + id*4
- 0x8111AF0C  ldr   r0, [r0, #0x20]         ; r0 = table[id]
- 0x8111AF0E  strb  r0, [r1, #4]            ; ★★★ 写回 byte@+4
- （`0x8111AF12` 起是同一段代码在 bit4 为 0 时的分支，结构相同。）
- ⇒ 它不是一个只读的"类型字段"，而是**一个可被游戏重新编码的 id**。
- 2. **重映射域是 0..5（6 项）**，`8` 被显式排除（`cmp #8; beq 跳过`）。
- ⇒ 与数据侧"低字节取值 7 个、其中 8 另作处理"完全一致。
- 也可能是别的值。**在拿到 `[sp+0x94]` 的写入点之前，不能断言。**
- 1. 在 `0x8111A000`–`0x8111B000` 内搜 `str*/strd` 带 `#0x94` ⇒ **0 条**。
- 2. 用回溯法推断出函数头 `0x8111A566`，在 `[0x8111A566, 0x8111AEEE]` 内搜提到 `0x94`
- 的指令 ⇒ 只有 **4 条，且全是 `ldr`（读）**：

---

## `ARCH_229_MNS_ID_REMAP_CONFIRMED.md`

**标题**：ARCH_229 —— MNS：★ 重映射表确认为 `{1,0,3,2,5,4}`（成对交换 `0↔1,2↔3,4↔5`） ｜ 1. 先把上一轮的错误回溯修正掉 ｜ 2. ★ 查清 `X`：`[sp+0x94] = 0`，表是 `{1,0,3,2,5,4}` ｜ 3. 结论：id 的重映射是**对合**（成对交换） ｜ 3.1 这条结论的确信度 ｜ 3.2 适用范围（已看到的分支） ｜ 4. 与目标的距离 ｜ 5. 下一步

**发现/结论**（21 条，最多列 40）：

- > 目标进行中（第 13 轮）。上一轮留下的唯一未知量 `X = [sp+0x94]` 本轮**查清并确认**。
- `break`，等于每个位移只看一条指令，且首次匹配即停 ⇒ 函数头推断不可靠。
- 本轮改为**正向反汇编建立指令边界**，再取站点之前最近的 `push` 含 `lr`：
- 函数头 = 0x8111A844   push.w {r4, r5, r6, r7, r8, sb, sl, fp, lr}
- （`ARCH_228` 推的 `0x8111A566` 是**另一个函数**的起点。）
- 在 `[0x8111A844, 0x8111AEEE]` 内找向 `sp+0x94` 的写入，得到唯一一处**双字写**：
- 0x8111A89A  strd sl, sl, [sp, #0x90]     ; STRD: [sp+0x90]=sl, [sp+0x94]=sl
- 0x8111A890  movs.w sl, #0        ← ★ 与本条相邻，且之后到建表处无其它写入
- 0x8111ACBC  add.w  sl, sl, #1    （在建表之后，作为后续循环计数用）
- 0x8111ACFC  ldr.w  sl, [sp, #0x8c]
- 0x8111AD00  add.w  sl, sl, #2
- 0x8111AD0C  add.w  sl, sl, #1
- ⇒ **在 `0x8111A89A` 执行时 `sl == 0`**（`0x8111A890` 置零，之后的 4 处写入都在建表**之后**）。
- ——「用同一寄存器清零一片栈槽」的典型写法，与此结论一致。
- **⇒ `X = [sp+0x94] = 0`，重映射表 = `{1, 0, 3, 2, 5, 4}`。**
- - `8` 被显式排除：`cmp r2, #8; beq 跳过`；
- ⇒ 这四条都是**指令级**证据，可以逐条复核。
- ⇒ **它只在特定状态下施加**（不是无条件）。**这一点尚未查清其语义**，如实记录。
- | id 重映射 | ✅ **`{1,0,3,2,5,4}` 对合，已指令级确认** |
- `0x8111AF0E` 写回之后，谁再读这个字节。
- **这才是"id → 按键"的直接证据**，也是本目标剩下的最后一步。

---

## `ARCH_22_ui_system.md`

**标题**：ARCH_22：UI / 菜单 / HUD 系统（运行原理） ｜ 0. 本文新增的关键事实速览（先看这 12 条） ｜ 1. 本模块的运行职责（一句话） ｜ 2. 完整流程 / 状态迁移图（本轮核心） ｜ 2.1 app 状态机：表结构 + 派发器 ｜ 2.2 状态迁移图（本作的实际可达路径） ｜ 2.3 mainmenu 六条件门（`0x81184B38`）—— 全过之后到底发生了什么 ｜ 2.4 `helpwindow` 任务内部状态机（update = `0x810E694C`） ｜ 2.5 `key help` / `help string` 子状态机：把 spr6 拉进来的两个点 ｜ 2.6 每帧绘制作业表：UI 的「绘制入口」 ｜ 3. 关键函数地址表 ｜ 3.1 app 状态机（kernel/sequence.c） ｜ 3.2 任务框架与绘制层表（kernel/*.c） ｜ 3.3 UI 资源任务（appMessage / appFont） ｜ 3.4 UI 模块代码区间（本次新做的「路径串引用点聚类」） ｜ 4. 关键数据结构（字段偏移 + 含义 + 推测标注） ｜ 4.1 app 状态表条目（0x50，实测） ｜ 4.2 任务对象（`0x8116F7CA` 的产出） ｜ 4.3 绘制作业表（实测 + 推测混合） ｜ 4.4 UI 任务私有 work 块（实测） ｜ 4.5 消息二进制（appMessageBinary.c，实测） ｜ 4.6 `message_win_p5.bin` 容器（实测，本次从 data.cpk 解出） ｜ 4.7 SPR6 精灵图集（实测） ｜ 4.8 输入结构（实测）

**发现/结论**（362 条，最多列 40）：

- | # | 事实 | 证据 |
- | 1 | app 状态表**基址指针**在全局 `0x813B01AC`，运行时值 `0x813AC9C4`；步长 `0x50`，字段 `+0x00 名字[0x40] / +0x40 id / +0x44 enter / +0x48 update / +0x4C draw` | 派发器 `8118585A movw r0,#0x1ac` `8118585E movt r0,#0x813b` `81185862 ldr r1,[r0]` `81185864 add.w r0,
- | 2 | 7 个 app 状态的**真实名字**：`DEBUG MENU / initread / FREE MODE / COMMU / TUTORIAL / MAIN MENU / TITLE` | `py -c cstr(0x813AC9C4)` = `'DEBUG MENU'`；`0x813ACA14` = `'initread'`；`0x813ACA64` = `'FREE MODE'`；`0x813ACAB4` = `'COMMU'`；`0x813ACB04` 
- | 3 | **纠错**：`state=1` 不是 "mainmenu-load 阶段"，而是 **`initread`**（等 `initDataRead` 任务）；`MAIN MENU` 是 `state=5`，`TITLE` 是 `state=6` | `state 1 update = 0x81185926` 体内 `movw r0,#0x9690 / movt r0,#0x8130 / bl 0x8116f900` → 查 `0x81309690` = `'init
- | 4 | 任务创建 API `0x8116F7CA` 的**完整签名**：`create(ctxObj, name, period, 0, {flags, update, exit, self})`；`update` 返回 1 = 留在该状态 | `8116F7DC ldr r5,[sp,#0x58]` / `8116F7E6 ldr.w r2,[sl,#4]` / `8116F7EC ldr.w fp,[sl,#8]` / `8116F7F2 ldr.w sl,[sl,#
- | 5 | 「每帧绘制作业表」基址 `[0x81555420]`，条目 0x1C 字节，按**层 ID** 索引；`0x8115FA98` 清表、`0x8115FA3E(id)` 取槽、`0x8116E49E` 建节点、`0x8116E522` 入链 | `8115FA3E rsb r0,r4,r4,lsl #3`(=7*id) `8115FA6A add.w r0,r1,r0,lsl #2`(=base+28*id)；`8115FA98 movw r4,#0x5420 / 
- | 6 | 每帧注册 **5 个绘制层**，层 ID = `0xb / 0xe / 0x19 / 0x19 / 0x43`，回调 = `0x81185236 / 0x8118522E / 0x81185298 / 0x8118522E / 0x8118522E` | `81185316 movs r0,#0xb` … `81185324 movw r0,#0x5237` `81185328 movt r0,#0x8118` … `81185338 movs r0,#0xe` 
- | 7 | `0x810E6650` = `helpWindowInit()`：它在「mainmenu 六条件全过」之后被 `0x81184B9C` 调用，创建一个名为 **`helpwindow`** 的任务（对象 0x1C 字节，源文件 `mainmenu/helpwindow/helpwindow.c`） | `81184B9C bl #0x810e6650`；`810E6654 bl 0x8115d614`／`810E665A movs r0,#0x1c`／`810E
- | 9 | 消息二进制表：魔数 **`MSG0`(0x3047534D) / `MSG1`(0x3147534D)** 位于 `+0x08`，条目表从 `+0x20` 开始、**每条 8 字节** | `81027AE8 movw r0,#0x534d` `81027AEE movt r0,#0x3047` `81027AEC ldr r3,[r5,#8]`；`81027AF8 movw r0,#0x534d` `81027AFC movt r0,#0x3147`；`8102
- | 10 | SPR6 头部（实测两个文件互相印证）：`+0x00 "SPR6" / +0x08 总大小 / +0x10 精灵数 / +0x14 首表偏移(0x20)` | `help.spr6`: `u32[0..5] = 36525053, 0x10000, 0x812BA, 0x10018, 0x10023, 0x20`，文件长 529082 = 0x812BA ✓，内部精灵名 `v000..v034` 共 35 = 0x23 ✓；内嵌 spr6: `36525053,
- | 11 | UI 侧模块代码区间（用「源文件路径字符串被 movw/movt 载入的点」聚类得到）见 §3.4 | 例：`appFontSystem.c` = `0x81021392..0x81022272`（66 点）、`appMessageWindow.c` = `0x81028056..0x8102A5CC`（104 点）、`app_spr.c`+`appResourceList.c` = `0x8102ECD4..0x8102F83A`（15 点）、`keyHelp
- | 12 | 输入接口：引擎的一次读键在 `0x811E6A78`，它把 0x20 字节 `SceCtrlData` 从栈拷进自己的 10 字节结构（`+0 buttons(u16) / +2 rx / +4 ry / +6 lx / +8 ly`） | `811E6A8C add r1,sp,#0` `811E6A8E movs r0,#0` `811E6A90 movs r2,#1` `811E6A92 blx #0x812b1840`（sceCtrlPeekBuffer
- 派发器代码（`program/app/kernel/sequence.c`，路径串 `0x813095D8`）：
- 8118585A  movw  r0, #0x1ac          ; 全局指针变量 0x813B01AC
- 81185862  ldr   r1, [r0]            ; r1 = 状态表基址（运行时 = 0x813AC9C4）
- 81185872  beq   #0x8118587e         ; 返回 1 ⇒ 「留在本状态」，什么都不做
- `[_h1169.log:26046] [ST-UPD-RET] state=1 updateFn=0x81185927 ret=1 (keep)`。
- 静态表（文件 VA `0x813AC9C4` 起，等价于 `[0x813B01AC]` 在运行时的取值）：
- | 0 | `DEBUG MENU` | 0 | `0x811858CE`（`bx lr`，空） | `0x811858D0` | `0x811858EC` | `find('debug menu')(0x81309644)` 存在 ⇒ 1 |
- | 1 | `initread` | 0 | `0x8118591E` | `0x81185926` | `0x81185942` | `find('initDataRead')(0x81309690)` 存在 ⇒ 1 |
- | 2 | `FREE MODE` | 1 | `0x81185A0C` | `0x81185A14` | `0x81185A26` | `0x8113ABCA()` |
- | 3 | `COMMU` | 1 | `0x810C84D8` | `0x810C8566` | `0x810C8582` | `0x810C8566()` |
- | 4 | `TUTORIAL` | 1 | `0x81185A50` | `0x81185A58` | `0x81185A6A` | `0x811116E8()` |
- | 5 | `MAIN MENU` | 1 | `0x811859C0` | `0x811859D2` | `0x811859E4` | `0x810E8680()`（= `mainMenu.c:0x810E8DCC`） |
- | 6 | `TITLE` | 1 | `0x81185974` | `0x81185986` | `0x81185998` | `0x811980FC()` = `find('title sequence')(0x8130B140)` 存在 ⇒ 1 |
- | — | `""`（0x813ACBF4） | — | — | — | — | 终止项（名字为 0） |
- `py -c cstr(0x813AC9C4)` → `'DEBUG MENU'`；`hexd 0x813AC9C0` 得
- `0x813ACA14` 处 ASCII 串 `initread`（`py` 输出 `'initread'`），其后 0x40 字节全 0，再 8 字节 0 + `CF 58 18 81 D1 58 18 81`。
- 证据链（函数侧，取 update）：
- - `0x81185926 push {r4,lr}` / `81185928 movw r0,#0x9690` / `8118592C movt r0,#0x8130` / `81185932 bl #0x8116f900` / `81185936 cmp r0,#0` … 返回值布尔化；`cstr(0x81309690)` = `'initDataRead'`。
- - `0x81185986 push {r4,lr}` / `81185988 bl #0x811980fc`；`811980FC push {r4,lr}` / `811980FE movw r0,#0xb140` / `81198102 movt r0,#0x8130` / `81198108 bl #0x8116f900`；`cstr(0x8130B140)` = `'title sequence'`。
- - `0x811858D0 push {r4,lr}` / `811858D2 movw r0,#0x9644` / `811858D6 movt r0,#0x8130` / `811858DC bl #0x8116f900`；`cstr(0x81309644)` = `'debug menu'`。
- │ state 1  initread      update=find('initDataRead')   │  ★ 当前日志长期停在这里
- │  └ initDataRead 完成 ⇒ 本状态 update 返回 0 ⇒ 迁移    │
- │ state 6  TITLE         update=find('title sequence') │  ★ 若走片头分支
- │  ├ 'title sequence' 任务的创建点 lr=0x811980F8       │
- │  └ title 序列收尾 ⇒ 迁移                              │
- │ state 5  MAIN MENU     update=0x810E8680(mainMenu.c) │
- │  ├ enter 0x811859C0 → 0x810E8D94                     │
- │  ├ 同时由 kernel/init.c:0x81184BBE 创建任务           │

---

## `ARCH_230_MNS_ID_IS_BIT_INDEX.md`

**标题**：ARCH_230 —— MNS：★★ 音符 id 是**位序号**（`1 << id`），掩码沿链表累加/清除 ｜ 1. 起点 ｜ 2. ★ 核心：`id` 被用作**位序号** ｜ 2.1 沿链表累加/清除 ｜ 3. 由此获得的语义（本轮最重要的两条） ｜ 4. 下一步（已经非常具体） ｜ 5. 仍未做到的

**发现/结论**（36 条，最多列 40）：

- 在 108 KB 内搜「`cmp rX,#8` 窗口里还有多个小立即数比较」，唯一命中 `0x8111B6A2`
- 附近（mnsHandle）。反汇编该窗口，得到下面这段。
- 0x8111B684  ldr   r0, [r7, #0x14]        ; r0 = note->field14
- 0x8111B686  movs  r1, #1
- 0x8111B688  ldrb  r6, [r0, #4]           ; ★ r6 = byte@+4 = 音符 id
- 0x8111B68A  ands  lr, r6, #0x80          ; bit7 = 有效位
- 0x8111B68E  ldr.w fp, [sb]
- 0x8111B692  lsl.w r1, r1, r6             ; ★★★ r1 = 1 << id
- 0x8111B696  beq   #0x8111b69a            ; bit7 == 0 → 保留该掩码
- 0x8111B698  movs  r1, #0                 ; bit7 != 0 → 掩码 = 0（无效音符）
- 0x8111B69A  mov   sl, r1
- **`1 << id`** ⇒ **id 是位序号，不是枚举值。**
- 0x8111B6BC  ands  r1, fp, #1             ; fp = [sb]，某状态字
- 0x8111B6C2  ldr.w lr, [r7, #0x30]        ; ★ 链表下一个：note->[0x30]
- 0x8111B6C6  orrs  sl, sl, #0xff          ; 先把低 8 位全置 1
- 0x8111B6CA  cmp.w lr, #0
- 0x8111B6CE  beq   #0x8111b6f8
- 0x8111B6D0  cmp   lr, r7                 ; 环形链表防自环
- 0x8111B6D2  beq   #0x8111b6f8
- 0x8111B6D4  ldr.w r1, [lr, #0x14]
- 0x8111B6D8  mov   r5, sl
- 0x8111B6DA  ldrb  r1, [r1, #4]           ; 该节点的 id
- 0x8111B6DC  ands  r3, r1, #0x80
- 0x8111B6E0  bne   #0x8111b6ec
- 0x8111B6E2  movs  r3, #1
- 0x8111B6E4  lsls.w r5, r3, r1            ; 1 << id
- 0x8111B6E8  bics.w r5, sl, r5            ; ★ sl &= ~(1 << id)
- 0x8111B6EC  ldr.w lr, [lr, #0x30]
- 0x8111B6F0  mov   sl, r5
- 0x8111B6F2  cmp.w lr, #0
- 0x8111B6F6  bne   #0x8111b6d0            ; 继续遍历
- ⇒ 这是**判定/碰撞检测**该有的形态：比较"当前需要哪些位"与"当前按下哪些位"。
- - `bit7` 有效位 ⇒ 掩码取 0（无效音符不占位）。
- **⇒ 「哪个键对应哪个音符」这个问题，现在被压缩成：
- 2. **`[sb]` 的 bit0 与掩码的关系**（`0x8111B6BC` 的 `ands r1, fp, #1`）：
- 4. 找到后按项目纪律**逐指令核对**，并用同一份谱面跑出"id 序列 → 位掩码序列"做可复现证据。

---

## `ARCH_231_MNS_JUDGE_CORE_FOUND.md`

**标题**：ARCH_231 —— MNS：★★★ 找到判决核心——「本帧新按下」的 16 位掩码 ｜ 1. 游戏输入状态的全局地址与布局（本轮定位） ｜ 2. ★★★ 判决核心：`0x811DA9A4` 的每帧输入读取 ｜ 2.1 `(cur ^ prev) & cur` = 上升沿 ｜ 2.2 与前三轮结论的闭环 ｜ 3.5 ★ 位掩码的来源查清：就是 `SceCtrlData.buttons & 0xFFFF` ｜ 3.6 ⚠ 但由此得到一个**说不通**的结果（必须记下） ｜ 4. 唯一剩下的缺口 ｜ 3.1 一条可用的反向线索（本轮已发现） ｜ 3.2 一个可立即验证的推断（未验证，仅记录） ｜ 4. 下一步

**发现/结论**（81 条，最多列 40）：

- > 音符 id 是位序号（`ARCH_230`）这一结论，与这里的 **16 位掩码**精确对上。
- `0x811DA822` 是**输入初始化**（不是文档说的"每帧更新"）：
- 0x811DA822  push.w {r4,r5,r6,r7,r8,sb,sl,lr}
- 0x811DA836  sb = 0x816CDFB0
- 0x811DA83E  r0 = 0x816CE000 ; bl #0x811e6a4c(0x816CE000, 0x816CDFB0, 4)
- 0x811DA84E  r8 = 0x816CDFB4 ; bl #0x811e6a4c(0x816CE004, 0x816CDFB4, 4)
- 0x811DA866  r7 = 0x816CDAA0 ; bl #0x811e6a4c(0x816CE008, 0x816CDAA0, 4)
- 0x811DA87E  r6 = 0x816CDAAC ; bl #0x811e6a4c(0x816CE00C, 0x816CDAAC, 4)
- 0x811DA896  r5 = 0x816CDAA8 ; bl #0x811e6a4c(0x816CE010, 0x816CDAA8, 4)
- 0x811DA8AE  r4 = 0x816CDAA4 ; bl #0x811e6a4c(0x816CE014, 0x816CDAA4, 4)
- 0x811DA8C6  清 0 并写 0x80（摇杆中位）
- 0x811DA8F8  strb r1=1, [0x814D3B0C]        ; 置"已初始化"标志
- | `0x816CDAA0` | **上一次的按键字**（u16，见 §2） |
- | `0x816CDAA4` | u16（本轮见其为 0，语义待定） |
- | `0x816CDAA8` | 摇杆 1 的 X/Y（各 u8，中位 `0x80`） |
- | `0x816CDAAC` | 摇杆 2 的 X/Y（各 u8，中位 `0x80`） |
- | `0x816CDFB0` | **本帧"新按下"掩码**（u16，见 §2） |
- | `0x816CDFB4` | 同上（第二份） |
- | `0x816CE000..E014` | 交给 `0x811E6A4C` 注册的描述符 |
- **顺带纠正文档**：`docs/progress.md` 把 `0x811DA822` 记为"手柄/触摸输入更新"，
- 实测它是**初始化**（末尾置 0x814D3B0C 标志）；`0x811DA900` 是清理。
- 0x811DA9A4  push.w {r4,r5,r6,r7,r8,sb,sl,fp,lr}
- 0x811DA9BA  bl     #0x8119957a                 ; 取时间/dt
- 0x811DA9C6  ip = 0x816CDFB0
- 0x811DA9CA  r8 = 0x816CDFB4
- 0x811DA9CE  fp = 0x816CDAA0                    ; 上次按键字
- 0x811DA9D2  r4 = 0x81680148                    ; ← 文档记的"2 点触摸"目标
- 0x811DA9D6  r7 = 0x816CDAAC                    ; 摇杆 2
- 0x811DA9DA  r6 = 0x816CDAA8                    ; 摇杆 1
- 0x811DA9DE  sl = 0x816CDAA4
- 0x811DAA04  r1 = sp + 4                        ; 输出：按键字
- 0x811DAA06  r2 = sp + 0                        ; 输出：另一份
- 0x811DAA08  r0 = sb                            ; 端口号 0,1,…
- 0x811DAA0A  bl     #0x811e6a78                 ; ★ 读取输入
- 0x811DAA2A  r3 = *(u16*)(sp + 4)               ; 本次按键字
- 0x811DAA2E  r0 = *(u16*)fp                     ; 上次按键字
- 0x811DAA34  lr = r3 ^ r0                       ; ★ 异或
- 0x811DAA38  lr = lr & r3                       ; ★ 与本次相与
- 0x811DAA3C  *(u16*)ip = lr                     ; ★★ 0x816CDFB0 = 本帧新按下掩码
- 0x811DAA48  *(u16*)r8 = lr                     ; ★★ 0x816CDFB4 = 同一份

---

## `ARCH_232_MNS_MASK_CONSUMER_IS_MENU.md`

**标题**：ARCH_232 —— MNS：上升沿掩码的消费者是**菜单导航**（位序被独立确认） ｜ 1. 怎么找到的 ｜ 2. 消费者是菜单导航 ｜ 2.1 PSV `SceCtrlData.buttons` 的标准位序 ｜ 3. 于是 `ARCH_231` §3.6 的矛盾**确立为真实矛盾** ｜ 4. 重读 `ARCH_230` 时发现的一个细节（此前漏掉） ｜ 5. 下一步（两条都要走，按性价比排序）

**发现/结论**（47 条，最多列 40）：

- > 目标进行中（第 16 轮）。本轮找到上升沿掩码 `0x816CDFB0`/`DFB4` 的**唯一非输入消费者**，
- > 它是菜单导航；由此**独立确认掩码就是 `SceCtrlData.buttons` 的标准位序**。
- `ARCH_231` 把上升沿掩码定位到 `0x816CDFB0`/`0x816CDFB4`。用已导出的常量表
- 0x816CDFB0 装载 6 处: 0x8118FA4E  0x811DA744  0x811DA7B8  0x811DA836  0x811DA912  0x811DA9C6
- 0x816CDFB4 装载 6 处: 0x8118FA8A  0x811DA748  0x811DA7BC  0x811DA84E  0x811DA928  0x811DA9CA
- 除输入模块自身的 init/cleanup/read 之外，**唯一的外部消费者是 `0x8118FA4E` 与 `0x8118FA8A`**
- 0x8118FA4E  movw r0, #0xdfb0 ; movt r0, #0x816c
- 0x8118FA56  ldrh r0, [r0]              ; r0 = 本帧新按下掩码
- 0x8118FA58  ands r0, r0, #0x2000       ; ★ bit13
- 0x8118FA5C  beq  #0x8118fa8a
- 0x8118FA5E  ldr  r1, [r4, #8]          ; r1 = 当前索引
- 0x8118FA60  cmp  r1, #5 ; beq → state=0x17
- 0x8118FA64  cmp  r1, #4 ; beq → state=0x14
- 0x8118FA68  cmp  r1, #2 ; beq → state=0x0e
- 0x8118FA6C  cmp  r1, #1 ; bne → 退出
- 0x8118FA72                        → state=0x0b
- 0x8118FA8A  r1 = 掩码
- 0x8118FA94  ands r0, r1, #0x10         ; ★ bit4
- 0x8118FA98  beq  #0x8118faac
- 0x8118FA9A  ldr  r1, [r4, #8]
- 0x8118FA9C  subs r0, r1, #1            ; 索引 - 1
- 0x8118FAA0  cmp  r0, #0
- 0x8118FAA2  bge  → 退出
- 0x8118FAA6  adds r0, r1, #5            ; ★ 下溢 → 回绕到 5
- 0x8118FAAC  ands r0, r1, #0x40         ; ★ bit6
- 0x8118FAB0  beq  → 退出
- 0x8118FAB4  ldr  r1, [r4, #8]
- 0x8118FAB6  adds r0, r1, #1            ; 索引 + 1
- 0x8118FABA  cmp  r0, #6
- 0x8118FABC  blt  → 退出
- 0x8118FAC0  subs r0, r1, #5            ; ★ 上溢 → 回绕到 1
- - `bit13(0x2000)` → 按当前索引进入不同状态（“确认”）。
- ⇒ **代码用的 bit4/bit6/bit13 与 PSV 标准位序逐个吻合**
- （上 = 减、下 = 加、○ = 确认）。
- **⇒ 独立确认：那个 16 位掩码确实是 `SceCtrlData.buttons` 的标准位序，`ARCH_231` §3.5 的结论成立。**
- 既然位序已独立确认，那么"音符 id 直接是掩码位序号"这一读法就必然给出：
- ⇒ **音符 id 与"输入位"之间一定还有一层我没有找到的对应**，
- 0x8111B6C6  orrs sl, sl, #0xff         ; ★ sl 的底是**低 8 位**
- ⇒ 两种读法（都未验证）：
- `8` 不在类型掩码内 ⇒ 它是**另一类标记**（`ARCH_228`/`229` 里它也被 `cmp #8; beq 跳过`

---

## `ARCH_233_MNS_TWO_CANDIDATES_EXCLUDED.md`

**标题**：ARCH_233 —— MNS：排除两个候选判定点；目标状态与建议 ｜ 1. 本轮排除的两个候选 ｜ 1.1 「其他端口副本」这个搜索盲区已排除 ｜ 1.2 `0x811A62xx` 是**摇杆归一化**，不是判定 ｜ 2. 因此：音符判定读的是**持续按住的按键字** ｜ 3. 目标现状（直白） ｜ 3.1 已确实拿到（都有可复现证据） ｜ 3.2 还差的那一步 ｜ 4. 建议（需要你定）

**发现/结论**（32 条，最多列 40）：

- > 目标进行中（第 17 轮）。本轮**排除了两条候选路径**，并把目标现状梳理清楚。
- ⇒ 掩码其实是一串 u16（`0x816CDFB0`、`B2`、`B4`…）。我上一轮只按**精确地址**
- 0x816CDFB0  6 处: 0x8118FA4E 0x811DA744 0x811DA7B8 0x811DA836 0x811DA912 0x811DA9C6
- 0x816CDFB4  6 处: 0x8118FA8A 0x811DA748 0x811DA7BC 0x811DA84E 0x811DA928 0x811DA9CA
- （没有 0x816CDFB2 / B6 / B8 等任何其他副本）
- ⇒ **盲区排除**：游戏只用了 `0x816CDFB0`（端口 0 上升沿）与 `0x816CDFB4`（第二份），
- **⇒ 结论：那个「本帧新按下」掩码的消费者只有菜单（`0x8118FA4E/8A`）与输入模块自身。
- 0x811A62B0  r0 = 0x816CDAAC
- 0x811A62B8  r1 = *(u8*)(r0 + 1)     ; ry
- 0x811A62BA  subs r1, #0x80          ; 减中位 128
- 0x811A62C2  r0 = *(u8*)(r0)         ; rx
- 0x811A62C8  movt r1, #0x4240        ; ★ 0x42400000 = 48.0f
- 0x811A62D8  subs r0, #0x80
- 0x811A62E6  r0 = 0x816CDAA0 ; ldrh r1, [r0]   ; 顺带读一次按键字
- 0x811A62EA  vdiv.f32 s16, s16, s1   ; / 48.0
- 0x811A62FC  vcmp.f32 s16, s4        ; 与 -1.0 比较
- 0x811A6312  vcmp.f32 s16, s1        ; 与 +1.0 比较
- 0x811A631E  movt r0, #0xbbab (0xBBAB92A6)      ; 越界时的缩放
- 0x811A6374  r0 = 0x816CDAAC ...     ; 第二个摇杆做同样处理
- `0x3C888889`（1/60）与 `0x42400000`（48.0）都在这里。
- （顺带解释了 `docs/progress.md` 记的"死区 0.1"常量 `0x3DCCCCCD` 在代码段出现 33 次。）
- `0x816CDAA0`（上次/当前按键字）的**非输入模块**消费者只有三处：
- 0x8116F520   ┐
- 0x811A62E6   ├ 0x811A62E6 已排除（摇杆函数里的一次无用读）
- 0x811A76EA   ┘
- ⇒ 候选只剩 **`0x8116F520`** 与 **`0x811A76EA`**。这是下一步要看的两个具体地址。
- | 层 | 结论 | 证据 |
- | 位序确认 | 掩码确为标准 `SceCtrlData` 位序（bit4=UP/bit6=DOWN/bit13=○） | `ARCH_232` 菜单消费者 |
- 并把 MNS 的范围从错误的 24 KB 纠正到 108 KB。**解码层已经做得比较干净**，
- - **A：再走 2–3 轮，只看 `0x8116F520` 与 `0x811A76EA` 两处**（已经收窄到具体地址，
- 把 MNS 目标标记为"解码层已完成、命名层受限于证据"，
- - **B：立即停止静态分析**，把现有结论（时间格式、id 域、重映射、输入链）写成

---

## `ARCH_234_MNS_CONVERTER_INCONSISTENT.md`

**标题**：ARCH_234 —— MNS：`0x8116F520` 处的转换函数**读不通**（如实记为未解释项） ｜ 1. 计划要判定的两件事，结果如下 ｜ 1.1 `0x812b2270` / `0x812b2280` 是**导入桩** ｜ 1.2 `0x8116F4F8` **找不到直接调用者** ｜ 2. 于是矛盾确立：表内容与用法不符 ｜ 3. 本轮定位到的**唯一硬事实** ｜ 4. 状态与建议（直白） ｜ 4.1 这是第 19 轮 ｜ 4.2 我的建议：**暂停静态分析，转产品侧主线**

**发现/结论**（34 条，最多列 40）：

- 0x812B2270  movs r0, r0
- 0x812B2272  b    #0x812b2a36        ; ★ 跳到公共派发点
- 0x812B2278  movs r0, r0
- 0x812B227A  b    #0x812b25be
- 0x812B2280  movs r0, r0
- 0x812B2282  b    #0x812b2a46
- 0x812B228A  b    #0x812b25ce
- ⇒ 它们是 **libc 导入**（很可能就是 `memcpy` 与 `sprintf` 一族），参数顺序遵循 AAPCS：
- ⇒ 所以 `0x8116F518 blx #0x812b2270`（r0=sp, r1=0x813AFC40, r2=0x40）
- **确实**是把 `0x813AFC40` 处的 64 字节复制到栈上。**方向没错。**
- 全代码段搜 `bl/blx` 目标为 `0x8116f4f8` ⇒ **0 处**。
- 既然复制方向没错，那么栈上的 16 个 u32 **就是** `0x813AFC40` 的内容。
- 0x813AFC00  00000000  811A0BB9  811A0BDD  811A0C07  0000000C  0000000C  811A0C53  00000000
- 0x813AFC20  00000000  811A0CB5  811A0CE3  811A0D19  00000010  00000010  811A0D77  00000000
- 0x813AFC40  00000000  811A0D91  811A0D9F  811A0DAB  00000004  00000004  811A0B6F  00000000   ← 本函数复制的起点
- 0x813AFC60  00000000  811A0BB9  811A0BDD  811A0C07  0000000C  0000000C  811A0B6F  00000000
- **而 `0x8116F520` 处那 16 次迭代把每个 u32 与 16 位按键字做 `ands`。**
- ⇒ **两者不可能同时成立。** 三种可能，本轮都**没能**判定：
- 1. 我看到的 `0x8116F520` 那一段虽然被 capstone 解成指令，但**起始对齐不是函数入口**
- 2. `0x8116F4F8` 不是函数，而是**别的结构**被线性扫描误判；
- `0x812b2270` / `0x812b2280` 是 **PLT 导入桩**，跳到 `0x812B2A36` / `0x812B25CE` 的公共派发器。
- ⇒ **这给了一条通用能力**：以后凡遇到 `0x812B2xxx` 的 `blx`，都能确定那是**导入函数**
- （libc / 系统），而**不是游戏逻辑**。此前我把 `0x812b2280` 称为"sprintf 一族"
- 只是猜，现在有了形态证据。
- | **真正解出格式/语义**（时间位布局、id 域与有效位、重映射对合、id=位序号、输入链与位序） | ARCH_220、225、229、230、231、232 |
- | **排除项 / 未解释项**（本轮与 ARCH_218、221、233） | 若干 |
- ARCH_231 得到 `SELECT/L3/R3/START…` 的荒谬结果 → ARCH_232 确认位序无误、矛盾成立 →
- ARCH_233 排除两条候选 → ARCH_234（本轮）唯一新线索自相矛盾。
- 1. **剩下的缺口已有明确性质**：需要"id ↔ 按键"的**命名证据**，
- 而这层反复表现为"要么在别处、要么与已有证据冲突"；
- 2. **继续的边际产出主要是排除项**，而且每 2–3 轮就会撞上一个我自己的编码/对齐错误；
- **若你同意**，我下一轮就把 MNS 的现有结论整理成一份**可复现的对照工具 + 文档**
- （把时间格式、id 域、重映射、输入链都做成可跑的小程序与断言），
- Thumb 解释器），而不是继续在静态反汇编里逐条找。

---

## `ARCH_235_MNS_DECODE_VERIFIED.md`

**标题**：ARCH_235 —— MNS：解码层**可复现对照工具**建成，7 条硬判据全通过 ｜ 1. 固化的语义（每条都标了来源轮次） ｜ 2. 硬判据与结果 ｜ 2.1 为什么 C2 是关键 ｜ 3. 本轮的一处**修正**：音符 id 是 **7 个**，且占比几乎均匀 ｜ 4. 状态：目标的验证条款已完成，命名层仍缺

**发现/结论**（5 条，最多列 40）：

- > 「解出后用可复现证据在 Windows 侧验证（同一份数据跑出确定结论，不靠反推值）」。
- ⇒ 这与"**7 个同权重的输入位/轨道**"高度一致（若某个 id 是"特殊事件"，
- **这是本轮唯一的新语义线索**，但**它仍然只是分布证据，不是命名证据** ——
- - ✅ 目标的后半句（"可复现证据在 Windows 侧验证"）**已完成并全通过**；
- - ❌ 目标的前半句（"哪个键对应哪种音符"）**仍受限于命名证据**。

---

## `ARCH_236_MNS_GAMEPLAY_NOT_FROM_BUTTONS.md`

**标题**：ARCH_236 —— MNS：★ 强否定——玩法不读 SceCtrl 按键全局；并**推翻我自己一个 Framing** ｜ 1. 强否定：玩法不按键全局走 ｜ 2. 由此推翻我自己的 Framing ｜ 3. 那玩法输入从哪来（本轮未定，但有方向） ｜ 4. 顺带的数据侧勘察 ｜ 5. 状态与建议 ｜ 5.1 现在真正剩下的 ｜ 5.2 建议

**发现/结论**（35 条，最多列 40）：

- > 目标进行中（第 22 轮）。本轮是一个**纠正性**的结论：它推翻了我从 `ARCH_230` 起
- `0x816CDAA0`（持续按住的按键字）的**非输入模块消费者**只有三处，本轮逐一核完：
- | `0x8116F520` | `ARCH_234` 已判定**自相矛盾**（表内容与用法不符） |
- | `0x811A62E6` | `ARCH_233` 已判定：**摇杆归一化** |
- | `0x811A76EA` | **本轮核实：同样是摇杆处理** |
- `0x811A76EA` 所在函数（头 `0x811A76C2`）的做法：
- 0x811A76D6  r2 = 0x816CDAAC
- 0x811A76E2  ldrb r3, [r2]        ; rx
- 0x811A76E8  ldrb r2, [r2, #1]    ; ry
- 0x811A76EE  ldrb r6, [r5]        ; 0x816CDAA8 → lx
- 0x811A76F4  ldrb r5, [r5, #1]    ; ly
- 0x811A76F0  rsbs.w r1, r3, #0x80 ; 减中位 128
- 0x811A76F6  rsbs.w r3, r6, #0x80
- 0x811A76FA  rsbs.w lr, r5, #0x80
- 0x811A770E  rsbs.w r2, r2, #0x80
- 0x811A7702  ldrh r4, [r4]        ; 0x816CDAA0 → 按键字（读了一次）
- 0x811A7712  cmp r3, ip           ; ip = ~0x1F
- 0x811A7716  cmp r3, #0x20        ; ★ 死区 ±32
- ⇒ **读按键字只是顺带，函数本体是双摇杆的死区/归一化。**
- **叠加 `ARCH_232` 的结论**（上升沿掩码 `0x816CDFB0` 的唯一外部消费者是**菜单导航**）：
- > **舞蹈玩法既不读 `0x816CDAA0`（持续按住），也不读 `0x816CDFB0`（上升沿）**
- `ARCH_230` 我在 `0x8111B692` 读到 `lsl.w r1, r1, r6`（`r6` = `byte@+4` = 音符 id），
- ⇒ **我此前写进 `docs/MNS_FORMAT.md` §3 的「id 是位序号」应保留（那是代码事实），
- 已排除：按键字/上升沿掩码的**字面量**消费者。
- 1. **经由访问器指针**：输入 init（`0x811DA822`）把 6 个状态地址连同描述符 id
- `0x816CE000..E014` 注册给 `0x811E6A4C`。但查常量表：
- **`0x816CE000` 只在 init/cleanup 被装载（2 处）**，说明没有别的代码按这些 id 取指针。
- `0x81680148` 在输入读取里被写入 `0x3E6EEEEF`（≈0.2333f）。
- 但 `0x81680148` 的装载点也**只有输入模块自己 3 处**。
- ⇒ **三条我都还没验证**，但它们都指向同一件事：
- ⇒ "在数据里找音符类型表"这条路**没有明显入口**。
- - **不变的**：`ARCH_235` 的验证成果（时间格式、id 域、变速事件、重映射、7 条判据）
- **全部不受影响**——那些是数据格式层，与输入通道无关。
- > 「音符 id → 具体按键」的命名证据在可用手段下无法取得。
- > 期间我排除了：字面量字面地址、上升沿掩码、端口副本、摇杆路径（3 处）、

---

## `ARCH_237_MNS_ONPU_AND_USER_SEMANTICS.md`

**标题**：ARCH_237 —— MNS：★ 用户给出操控语义；找到音符资源表（`ONPU_01..04`）与判定串 ｜ 1. ★ 用户提供的操控语义（权威） ｜ 1.1 这条信息解释了我此前所有的困惑 ｜ 2. ★ 找到音符资源表：`ONPU_01`–`ONPU_04` ｜ 3. ★ 找到判定与回放相关的格式串（并有引用点） ｜ 4. 与本轮之前结论的关系 ｜ 5. 下一步（两个具体地址，均本轮已定位）

**发现/结论**（26 条，最多列 40）：

- > 目标进行中（第 23 轮，实质突破）。本轮由**用户提供的关键操控语义**纠正了问题的性质，
- > 并在资源注册表里找到**音符类型的资源名**，以及判定/回放相关的格式串。
- | **刮盘用摇杆** | ⇒ 我找到的那三处「读 rx/ry 字节、减 `0x80` 中位、死区 ±32」的函数**就是刮盘输入**，不是无关代码 |
- | **设置里可改为 L/R** | ⇒ **id → 输入的映射是「可配置」的，不是硬编码** |
- **⇒ 原问题的问法需要修正**：不是"哪个物理键对应 id"，
- `0x8130AE80` 起的字符串区：
- ONPU_01        ← ★ 音符
- `0x81194A40` 起是一段**资源注册**代码（`bl #0x8117c1bc`），**序号在 r2 里顺序递增**：
- **⇒ 「音符」资源只有 4 个（`ONPU_01..04`），资源号 0x0B–0x0E。**
- | `ICULTY %d`（DIFFICULTY %d） | `0x81302DD0` | — |
- | `GAUGE %4d`（能量条） | `0x81302DDC` | — |
- | `SCRATCH COUNT %d` | `0x81302DE8` | **`0x81119C74`**（在 mnsHandle 范围） |
- | `HL long JUST[%d]::%f:%f` | `0x81302E9C` | —（**`JUST` 是判定等级名**，来自 `mnsDebug.c`） |
- | `REPLAY:KEY_HIT = %d` | `0x81303B4C` | **`0x81132AFE`**（mnsReplay 范围） |
- | `REPLAY:KEY     = %d` | `0x81303B64` | **`0x81132B16`** |
- | `REPLAY:MAX     = %d` | `0x81303B7C` | — |
- | `PRESS_ANY_BUTTON` | `0x8130AF14` | `0x81194ADE`、`0x81195138` |
- **`REPLAY:KEY_HIT` 字面意思就是"按键命中"**——**`0x81132AFE` 极可能就是判定代码**。
- `SCRATCH COUNT` 的引用点 `0x81119C74` 则是**刮盘计数**。
- **⇒ 这两个地址是本轮留给我自己的最明确的两个落点。**
- | 结论 | 受影响吗 |
- | 时间字段 `A` 的位布局（`ARCH_220`，全量验证） | **不受影响**（纯数据格式） |
- 1. **`0x81132AFE`（`REPLAY:KEY_HIT`）**：反汇编该函数，
- 2. **`0x81119C74`（`SCRATCH COUNT`）**：反汇编该函数，看刮盘计数如何递增
- （应由摇杆输入驱动 ⇒ 能反推出**逻辑动作位与摇杆方向/摇杆轴**的对应）。
- 3. **找默认配置**：`mainmenu\config\configMenu.c`（字符串 `0x812FE2C0`）与

---

## `ARCH_238_MNS_LOGICAL_ACTIONS.md`

**标题**：ARCH_238 —— MNS：整合用户操控语义；问题已重新定义且有落点 ｜ 1. ★ 用户提供的两条权威操控语义 ｜ 1.1 这两条解释了我此前全部的困惑 ｜ 1.2 因此 `ARCH_236` §2 的"推翻"要**部分收回** ｜ 2. 本轮找到的结构 ｜ 2.1 音符资源只有 4 个：`ONPU_01`–`ONPU_04` ｜ 2.2 回放记录的是**逻辑动作掩码**（重要旁证） ｜ 2.3 其他有用的定位 ｜ 3. 未受影响的既有成果 ｜ 4. 下一步（三个具体落点）

**发现/结论**（15 条，最多列 40）：

- > 权威信息纠正**，并且找到了一处关键的结构（回放记录的是**逻辑动作掩码**）。
- **⇒ 正确的问题定义是**：
- `0x81194A40` 起是**顺序资源注册**（`bl #0x8117c1bc`），**资源号在 r2 里递增**：
- ⇒ **音符贴图 4 张**。与"音符由 4 面键族操控"自洽；**刮盘是另一类**（走摇杆/L/R），
- `mnsReplay.c` 的调试串（`0x81132AFE` / `0x81132B16`）：
- **⇒ 回放逐帧记录 `KEY`（逻辑动作掩码）与 `KEY_HIT`（其上升沿）。**
- 这与 `ARCH_231` 从输入更新里读到的 `0x816CDFB0 = (cur ^ prev) & cur` **完全对应**，
- | `SCRATCH COUNT %d` (`0x81302DE8`) | **`0x81119C74`**（mnsHandle） | 刮盘计数，**下一轮的落点** |
- | `HL long JUST[%d]::%f:%f` (`0x81302E9C`) | — | **`JUST` 是判定等级名**（mnsDebug） |
- | `GAUGE %4d` (`0x81302DDC`) | — | 能量条 |
- | `mainmenu\config\configMenu.c` (`0x812FE2C0`) | — | **设置菜单**（默认键位应在配置数据里） |
- 7 条硬判据全通过）**全部不受本轮影响**——它们是数据格式层。
- 1. **`0x81119C74`（`SCRATCH COUNT`）**：看刮盘计数如何递增。
- 2. **`0x81132A50` 附近的回放记录结构**：若能看到 `KEY`/`KEY_HIT` 的**位宽与位定义**
- 这是一条**独立于反汇编游戏逻辑**的取证路径。

---

## `ARCH_239_MNS_CONFIG_PATH_MISSED.md`

**标题**：ARCH_239 —— MNS：配置路径本轮未命中；并修一处我自己的工具缺口 ｜ 1. ★ 先记我自己的工具缺口（可复用） ｜ 2. 本轮定位到的东西 ｜ 2.1 配置菜单模块的代码范围 ｜ 2.2 直接扫按键位立即数：216 条，**但绝大多数是噪声** ｜ 2.3 它后面引的两张 27 项表**是浮点参数表** ｜ 3. 本轮的结论 ｜ 4. 状态（第 23 轮）与建议

**发现/结论**（25 条，最多列 40）：

- **证据**：我用该表查「配置模块里的按键位常量」⇒ **0 个**；而改用 capstone 直接扫同一范围
- ⇒ **216 条**。二者不可能同时成立，说明是**表的收录范围**问题，不是代码里没有。
- ⇒ **`mns_consts.txt` 的适用范围必须写明：只含 `movw`+`movt` 成对装载的 32 位常量。**
- `mainmenu\config\configMenu.c`（字符串 `0x812FE2C0`）被引用 **24 处**，全部落在
- **`0x810DF740` – `0x810E47D8`** ⇒ 这就是配置菜单模块。
- 同模块内的资源名（`0x812FE3F0` 起）：
- CONFIG          (0x812FE410)   引用 3 处
- CONFIG_SPR      (0x812FE480)   引用 2 处
- COMSE_CONFIG    (0x812FE51C)   引用 1 处
- DANCE_COMSE_CONFIG (0x812FE52C) 引用 1 处
- 0x810DFAEA  movs.w r1, #0x400      ; L2
- 0x810DFAEE  str    r1, [sp]
- 0x810DFAF0  movs   r1, #8          ; START
- 0x810DFAF2  movs   r2, #4          ; R3
- 0x810DFAF4  movs.w r3, #0x800      ; R2
- 0x810DFAF8  bl     #0x81179f86     ; 4 参数调用 fn(0x400, 8, 4, 0x800)
- ⇒ 这只是一次**四参数调用**，不是表。
- 0x810DFB0A  r8 = 0x8139B520
- 0x810DFB0E  sb = 0x8139B6D0
- 0x810DFB1C  cmp r6, #0x1b        ; 27 项循环
- （如 `0xBED86163` ≈ −0.4225、`0x3F58EF13` ≈ 0.8474），且**每 3 项一组重复 9 次**。
- ⇒ 是**动画/效果参数表**，不是键位配置。
- ⇒ 若是 2，则要找的是**按键枚举的定义**（例如"枚举 0 = ○、1 = ×"这样的表），
- 1. **找 `CONFIG` 资源的默认值**（`0x812FE410` 的 3 个引用点 `0x810E27A6`/`0x810E31F2`/`0x810E34AE`）；
- 3. **动态取证**：在 `0x8111B68A`（读 id）与输入比较点下探针。

---

## `ARCH_23_boot_timeline.md`

**标题**：ARCH_23 — 完整启动时间线（从 module_start 到主菜单的运行原理） ｜ 1. 本模块的运行职责（一句话） ｜ 2. 完整流程 / 状态迁移图（本轮核心） ｜ 2.1 三层调度器的嵌套关系 ｜ 2.2 titleSequence 9 状态语义（**本轮新解出**） ｜ 2.3 title 异步任务的完成判据（**本轮新钉死，这是当前真正的卡点**） ｜ 2.4 状态迁移图（实测 + 反汇编合成） ｜ 2.5 那 2000 帧空窗里发生什么（用标记频次反推） ｜ 3. 关键函数地址表 ｜ 4. 关键数据结构 ｜ 4.1 app 状态机记录（表 = `*(u32*)0x813B11AC`，步长 `0x50`） ｜ 4.2 任务节点（`0x8116F7CA` 产出，大小 `0x1C`） ｜ 4.3 `[HITNODE]` 字段实测对照（`_h1152.log:86529`） ｜ 4.4 `titleSequence` work 块（实测对象 `0xB340E01C` / h1164 为 `0xB2D4F01C`） ｜ 4.5 title 异步任务对象（`0x18` 字节，`0x81193A00` 建立） ｜ 5. 调用链（逐层） ｜ 5.1 从 module_start 到第一个状态迁移 ｜ 5.2 `initDataRead` 的等待（state 1 的整段生命周期） ｜ 5.3 `titleSequence` 的完整调用链 ｜ 5.4 app 离开 state 6 的链 ｜ 5.5 断言链（appSound.c:1003） ｜ 6. 时序 / 每帧发生什么 ｜ 6.1 单帧时序（以 `_h1152.log` F2262..F2263 的完整一帧为样本） ｜ 6.2 每帧的调度器扫描次数

**发现/结论**（338 条，最多列 40）：

- > - 反汇编证据写作 `[DIS 0xVA] 指令原文`，反汇编器 = `py C:\dev\p5d_build\_p5d_dis.py <VA> <n>`。
- > - 日志证据写作 `[_hNNNN.log:行号] 原文`，帧号写作 `F<n>`（= 该行之前最近一条 `DISPENTRY#n` 的 n）。
- > - 无法确证的结论一律标注 **（推测）**，绝不编造。
- `初始化 → 影片 → LOGO → 主菜单` 的**相位**串起来，微观上是一个「每帧任务链表调度器 `0x811DB1F0`」
- 逐节点调用**注册回调**推进每个子任务；而 app 状态机（表 `[0x813B11AC]`，步长 `0x50`）只在这条链的
- guest app 状态机  [0x813B11AC] + 0x50*state           ← 只负责"最后两个等待"
- ├ +0x44 = enter 回调（blx，0x81185818）
- ├ +0x48 = update 回调（blx，0x8118586E）→ 返回 1=保活；否则 state=-1
- └ +0x4C = leave 回调（blx，0x811856FA）
- guest 任务链表调度器  0x811DB1F0                      ← 所有子任务的真调度器
- if(r3 == -1) → 0x811DAD8C(node) 摘除该节点
- titleSequence 序列状态机  0x81198172                  ← 宏观相位
- r4 = 0x8116F8A8(node) = node[+0x08] = work 块
- - `[DIS 0x811DB244] ldr r1,[r5,#0x28]` / `[DIS 0x811DB24C] blx r1` / `[DIS 0x811DB250] cmp.w r3,#-1` /
- `[DIS 0x811DB256] cbz r3,#0x811db25a` / `[DIS 0x811DB258] str r3,[r5,#0x28]` / `[DIS 0x811DB25A] ldr r5,[r5,#0x4c]`
- - `[DIS 0x811DB1F0] ldr r0,[r5]` / `[DIS 0x811DB1F2] cmp r0,#2` / `[DIS 0x811DB1F6] ldr r0,[r5,#4]` / `[DIS 0x811DB1F8] ands r0,r0,#1`
- - `[DIS 0x8118586C] ldr r0,[r0,#0x48]` / `[DIS 0x8118586E] blx r0` / `[DIS 0x81185870] cmp r0,#1` / `[DIS 0x81185872] beq` /
- `[DIS 0x81185874] movs.w r0,#-1` / `[DIS 0x81185878] str r0,[r5,#4]`
- - `[DIS 0x8116F8A8] ldr r0,[r0,#8]` / `[DIS 0x8116F8AA] bx lr`（这就是 `0x8116F8A8(node) = node[+0x08]` 的全部）
- - `[DIS 0x811856F8] ldr r0,[r0,#0x4c]` + `[DIS 0x811856FA] blx r0`（leave 回调在 +0x4C，由 `0x811856C0` 段分发）
- > **★ 本轮修正（对 ARCH_6 表的一处勘误）**：ARCH_6 把 `0x81198188` 的 TBH 表解码成
- > state0→`0x8119819A` / 2→`0x811981F6` / 3→`0x81198214` / 4→`0x81198224` / 5→`0x81198236` /
- > 6→`0x81198244` / 7→`0x8119826A`。但 Thumb-2 `TBH [pc, rm, lsl #1]` 的基址是
- > `Align(PC,4)`，而 `0x81198184` 处 PC 已是 `0x81198188`（4 对齐），故
- > **目标 = 0x81198188 + 2×halfword**。本轮逐字节重算（`_imp*` 同款直读）：
- > | 0 | 0x0009 | `0x8119819E` | 0x8119819A | +4 |
- > | 1 | 0x002E | `0x811981E8` | 0x811981E4 | +4 |
- > | 2 | 0x0037 | `0x811981FA` | 0x811981F6 | +4 |
- > | 3 | 0x0046 | `0x81198218` | 0x81198214 | +4 |
- > | 4 | 0x004E | `0x81198228` | 0x81198224 | +4 |
- > | 5 | 0x0057 | `0x8119823A` | 0x81198236 | +4 |
- > | 6 | 0x005E | `0x81198248` | 0x81198244 | +4 |
- > | 7 | 0x0071 | `0x8119826E` | 0x8119826A | +4 |
- > | 8 | 0x008E | `0x811982A8` | 0x811982A4 | +4 |
- > 旧值全部**落在目标指令的前一条指令的一半处**（例：旧 state 6 = `0x81198244` 是 `movs r0,#7` 的一半），
- > 证据：本轮重算脚本输出 + `[DIS 0x81198184] tbh [pc,r1,lsl #1]`。
- | state | VA | 动作 | 证据 |
- | 0 | `0x8119819E` | `0x81178CE2(0)` → `0x81178F0A()` 阶段门；`==0`→保活；`==1`→`0x81178DCE(1,1)`；`work[4]==1`→`work[0]=6`（跳到影片相位），否则 `work[0]=1` 贯穿进 state 1 | `[DIS 0x8119819C] bl #0x81178ce2` / `[DIS 0x811981A4] cmp r0,#0` / `[DIS 0x811981C4] ld
- | 1 | `0x811981E8` | `0x81178F0A()`；`work[0]=2`；读 `ldrsb [0x813B1B1C]`（跳过开关）**非 0 → work[0]=6** | `[DIS 0x811981EA] bl` / `[DIS 0x811981F2] movs r0,#2` / `[DIS 0x811981F6] movw r0,#0x1b1c; movt r0,#0x813b` / `[DIS 0x81198202] cbz r0,#0x8119
- | 2 | `0x811981FA` | 同上：读 `[0x813B1B1C]`，非 0 → `work[0]=6`；否则 `work[8]=0x81193344()`（**建 title 异步任务**）+`work[0]=3` | `[DIS 0x8119820A] bl #0x81193344` / `[DIS 0x8119820E] str r0,[r4,#8]` / `[DIS 0x81198210] movs r0,#3` |

---

## `ARCH_240_MNS_CONFIG_PATHS_EXHAUSTED.md`

**标题**：ARCH_240 —— MNS：配置路径的两次尝试均未命中；给出决断建议 ｜ 1. 本轮试了什么 ｜ 1.1 `CONFIG_SPR` / `COMSE_CONFIG` / `DANCE_COMSE_CONFIG` 的引用簇 ｜ 1.2 游戏数据目录里找存档/参数 ｜ 2. 到现在为止：已排除项清单（命名层） ｜ 3. 还剩下的三条（都不是"再找找看"） ｜ 4. 我的建议（请你定）

**发现/结论**（23 条，最多列 40）：

- > 下面把**已排除项**与**还剩下的**列清楚，并给一个决断建议。
- 地址 `0x810E3B12` / `0x810E3BDC` / `0x810E3BEC`，反汇编结果是**配置菜单的 UI 资源加载**：
- 0x810E3B12  CONFIG_SPR        = "mainmenu/config_play.spr6"
- 0x810E3B4E  CONFIG_WIN_PLG    = "mainmenu/config_win.plg"
- 0x810E3B98  common.spr6
- 0x810E3B26  bl #0x8117c0ee    ; ← ARCH_219 识别出的资源加载调用
- 0x810E3B24  movs r1, #0xf5    ; ← 加载命令码（与 MNS 的 0xF7 同一体系）
- ⇒ **这是配置菜单的界面资源装载，不是键位数据。**
- 但那需要先摸清该目录的格式，是**另一个独立的反向工程子项目**。
- | # | 路径 | 结论 |
- | 2 | 掩码位序是否读错 | **已独立确认**为标准 PSV 位序（`ARCH_232`） |
- | 3 | 上升沿掩码 `0x816CDFB0` 的其他消费者 | 只有**菜单**（`ARCH_233` 排除端口副本盲区） |
- | 4 | 持续按住按键字 `0x816CDAA0` 的消费者（3 处） | 全是**摇杆处理**（`ARCH_233`/`ARCH_236`） |
- | 5 | `0x8116F520`「按键→逻辑掩码」函数 | **表内容与用法不符**，未解释（`ARCH_234`） |
- | 10 | `SCRATCH COUNT` 引用点 `0x81119C74` | 是**调试打印**（`ARCH_238`） |
- **物理键映射由设置决定 + 触屏可满足同一动作 ⇒ 代码里不存在固定表。**
- | **A. 摸 `app_user` 的存档/配置格式** | 独立子项目（新格式反向工程） | 大 |
- | **B. 确认键位是否以「游戏自定义按键枚举」存储** | 需先找到枚举定义，搜索签名要换 | 中，且未必存在 |
- | **C. 动态取证**（`eboot_probe` 在 `0x8111B68A` 读 id 处 + 输入比较点下探针） | 需先把执行流推到 dance 场景（史上卡在标题/影片） | 大，风险高 |
- **结论：命名层已经不在"静态反汇编 eboot"的能力范围内**——它需要的是
- **游戏的存档/配置格式**（A）或**运行时观测**（C），二者都与本目标原定的
- - **① 标记 blocked**：`blocked_reason` 写"命名层需游戏存档/配置格式或运行时观测，
- 静态反汇编 eboot 的路径已穷尽（10 条排除项，见 `ARCH_240` §2）"，

---

## `ARCH_241_MNS_INPUT_SET_MATCHES.md`

**标题**：ARCH_241 —— MNS：★ 输入集合确定（6+1=7），与 id 域精确吻合 ｜ 1. 用户补充的操控细节（权威） ｜ 2. ★ 与 `ARCH_235` 的 id 域精确吻合 ｜ 3. ★★ 三条独立事实交汇，指向 `id 8 = 刮盘` ｜ 4. 现在未知的**只剩一项** ｜ 4.1 与顺序有关的已知线索（尚不能定案） ｜ 5. 下一步（针对"排列"的可执行方案）

**发现/结论**（6 条，最多列 40）：

- > 本轮把新信息与既有代码证据对齐，结果是**三条独立事实交汇**，把命名层从
- **⇒ 音符输入 6 个 + 刮盘 1 个 = 7 个。**
- **⇒ 强推断（三条独立证据交汇，但仍标为推断）：`id 8` = 刮盘；`id 0..5` = 六个音符输入。**
- ⇒ 游戏内部把 6 个输入**按对分组**。但"对"的含义未确定
- ⇒ 在数据段搜 **7 项连续 f32**（角度 0..2π 或对称坐标）。
- 3. **`KEY_HELP_P5`（`0x812FED4C`）**：按键帮助图，若能从资源里解出并渲染，

---

## `ARCH_242_MNS_JUDGE_LOOP_READ.md`

**标题**：ARCH_242 —— MNS：★★★ 判定回路完整读出；`1 << id` 确实比对输入动作掩码 ｜ 1. ★ 工具错误：「0 调用者」是我 dump 错开了一条指令 ｜ 2. ★★★ 判定回路（逐指令读出） ｜ 2.1 判定函数 `0x8111B64C` ｜ 2.2 ★ 调用者 `0x8111BDC2`：**遍历音符的判定循环** ｜ 3. 这条链条把此前的结论重新串起来 ｜ 4. 下一步 ｜ 5. 本轮已做：`0x8116F4F8` 的真入口核查 —— **很可能是死代码** ｜ 5.1 核查结果 ｜ 5.2 结论与它解释了什么 ｜ 5.3 顺带记一条工具注意项

**发现/结论**（59 条，最多列 40）：

- `ARCH_234` 报 `0x8116F4F8` 有 0 个调用者、本轮又报 `0x8111B650` 有 0 个调用者
- ——**连续两次同样的假否定**，可疑。核对后确认：
- 0x8111B64C  push.w {r4,r5,r6,r7,r8,sb,sl,fp,lr}   ← ★ 真入口（push 在 vpush 之前）
- 0x8111B650  vpush  {s16, s17}                      ← 我此前误当成入口
- 0x8111B654  sub    sp, #0xc
- 0x8111B656  mov    sb, r1
- **⇒ 我的函数入口判断错开了一条指令**（因为 `vpush` 紧跟 `push`，我把 `vpush` 当成了头）。
- 用真入口 `0x8111B64C` 一搜，**调用者立刻出现 1 处**。
- 但如果从 `vpush` 开始 dump 就会把入口看错，进而得出"无调用者"的假结论。
- 0x8111B764  ands  r1, r8, #4
- 0x8111B768  beq   #0x8111b7a4
- 0x8111B76A  ands  r1, r8, #8
- 0x8111B76E  bne   #0x8111b77a
- 0x8111B770  ldr   r1, [sp, #8]        ; 第三参（掩码）
- 0x8111B772  ands.w r1, sl, r1         ; ★ sl & 掩码
- 0x8111B776  bne   #0x8111b7ae         ; 非零 ⇒ 通过
- 0x8111B778  b     #0x8111b7ac
- 0x8111B7AC  b     #0x8111bd6a         ; 否则另一条路
- 0x8111B7BA  ldrb  r0, [r0, #4]        ; 另一个音符的 id 字节
- 0x8111B7CA  lsls  r1, r0              ; ★ 1 << id
- 0x8111B7CC  ands  r1, r2              ; ★ & 掩码
- 0x8111B7CE  str   r1, [r7, #0x34]     ; 结果存进音符对象 +0x34
- 0x8111B7D4  ands.w r8, r2, sl         ; ★ 掩码比较
- 0x8111B7DC  bl    #0x8111b614         ; 后续处理
- 0x8111BD86  mov   r8, r1              ; r8 = 输入动作掩码（本函数的参数）
- 0x8111BD88  mov   r4, r8
- 0x8111BD96  <loop>
- 0x8111BDAC  mov   r0, r7
- 0x8111BDB0  bl    #0x8111b190         ; 取下一个音符对象
- 0x8111BDB4  cmp   r0, #0
- 0x8111BDB6  beq   退出
- 0x8111BDC0  mov   r2, r8              ; ★ 第三参 = 输入动作掩码
- 0x8111BDC2  bl    #0x8111b64c         ; ★★ 调用判定
- 0x8111BDC6  cmp   r0, #0
- 0x8111BDC8  beq   #0x8111bdce
- 0x8111BDCA  bics.w r8, r8, r0         ; ★★★ 把判定消耗掉的位从输入掩码里清掉
- 0x8111BDCE  cmp   r5, #0
- 0x8111BDD0  bne   #0x8111bd96         ; 下一个音符
- ⇒ **这就是判定回路**：`输入动作掩码` × `逐音符的 1<<id` → 命中/未命中 + 位消耗。
- | 结论 | 现在的位置 |

---

## `ARCH_243_MNS_MASK_SOURCE_TRACED.md`

**标题**：ARCH_243 —— MNS：输入动作掩码的来源追到 `mnsHandle.c`（下一个目标是 `0x8111a21e`） ｜ 1. 追链：判定 → 掩码来源 ｜ 1.1 第一层：判定循环的参数来自 `[管理器+0x1E0]` ｜ 1.2 第二层：`[r4+0x1E0]` 的唯一非栈写入点 ｜ 1.3 第三层：`0x8111a2a6` 是 `mnsHandle.c` 里的「掩码数组构建器」 ｜ 2. ★ 下一个（也可能是最后一个）目标 ｜ 3. 现状：目标链条已经完整，只差最后一段 ｜ 4. 下一轮计划（单点、有界）

**发现/结论**（48 条，最多列 40）：

- `ARCH_242` 已读出判定循环在 `0x8111BD76`（入口），其 `r1` = **输入动作掩码**。
- 0x81122B0C  ldr.w r2, [r6, #0x1e0]
- 0x81122B14  adds  r1, r0, #0              ; r1 = r0 = [r6+0x1E0]
- 0x81122B1E  bl    #0x8111bd76             ; ★ 掩码来自 [对象+0x1E0]
- 0x81122FEE  ldr.w r0, [r6, #0x1e0]
- 0x81122FF2  adds  r1, r4, #0              ; 同类
- 0x81122FF6  bl    #0x8111bd76
- 0x811262B4  movw r1, #0x6021 ; movt r1, #0x8112   ; r1 = 0x81126021
- 0x811262B8  movw r2, #0x51d7 ; movt r2, #0x8112   ; r2 = 0x811251D7
- 0x811262C4  adds r3, r4, #0                       ; r3 = 管理对象
- 0x811262C6  bl   #0x8111a2a6                      ; ★ 产生掩码结构
- 0x811262CA  str.w r0, [r4, #0x1e0]                ; ★★ 存进 [管理器+0x1E0]
- （这段就在 `ARCH_219` 读过的 MNS 加载函数 `0x81126154` 的同一函数体里，
- 0x8111A2A6  push.w {r4,r5,r6,r7,r8,sb,sl,lr}
- 0x8111A2AC  r5 = r0                     ; r0 = 描述符
- 0x8111A2B4  ldrh r4, [r5, #0x14]        ; 计数
- 0x8111A2B6  r3 = r2 (= 0x811251D7)      ; 回调 2
- 0x8111A2B8  r2 = r1 (= 0x81126021)      ; 回调 1
- 0x8111A2C4  bl   #0x81119ff8
- 0x8111A2CA  sb = r0                     ; 累加结构
- 0x8111A2D0  cbz  r1, → 返回
- 0x8111A2D4  <loop>
- 0x8111A2D6  bl   #0x8111a21e            ; ★★ 逐项处理
- 0x8111A2E0  r0 = [r6, #4]
- 0x8111A2E2  cmp  r8, r0
- 0x8111A2E6  movw r0, #0x2e50 / movt r0, #0x8130   ; ★ 0x81302E50 = "mnsHandle.c"
- 0x8111A2F6  bl   #0x8115f832                      ; ★ 断言（ASSERT:%s）
- 0x8111A304  r1 = [sl, #4]
- 0x8111A308  sl += r1 << 3                         ; 步长 = (n<<3) + 0xc
- 0x8111A30C  str  r7, [r0, r4]                     ; ★ 结果写进数组（r4 += 4）
- 0x8111A318  blo  loop
- 0x8111A31A  r0 = sb
- 0x8111A31C  bl   #0x81119e3c
- 0x8111A320  str.w r0, [sb, #8]
- **三处独立证据表明它在 `mnsHandle.c` 模块内**：引用了 `0x81302E50`（即
- `0x81302E90`（`ASSERT:%s`），并且 `ARCH_227` 已用装载点统计把该模块范围定为
- `0x81119000–0x8111E000`（本函数 `0x8111A2A6` 正在其中）。
- （步长由 `[sl+4]` 决定），对每项调 `0x8111a21e`，把返回值**存进一个 u32 数组**
- > **`0x8111a21e`** —— 每一项的**动作掩码**就是它造出来的。
- - `0x81119ff8`（建累加结构）

---

## `ARCH_244_MNS_MASK_ARRAY_OR.md`

**标题**：ARCH_244 —— MNS：纠正一处误读；掩码来源收敛到「u16 动作数组 + OR 聚合」 ｜ 1. ★ 纠正 `ARCH_243` §1.1 的误读 ｜ 2. ★ 真正的掩码来源：`0x811330d8`（mnsReplay 范围） ｜ 3. 更新后的链条（只差一格） ｜ 4. 下一轮（仍然单点、有界）

**发现/结论**（42 条，最多列 40）：

- > 目标进行中（第 27 轮）。本轮**纠正 `ARCH_243` 的一处误读**，并把输入动作掩码的来源
- `0x81122B14` 与 `0x81122B18` 的先后顺序看反了。正确的读法：
- 0x81122AF6  adds r1, r4, #0            ; r1 = r4（帧索引）
- 0x81122AF8  bl   #0x811330d8           ; ★★ 取该帧的输入动作掩码 → r0
- 0x81122B0C  ldr.w r2, [r6, #0x1e0]     ; r2 = [管理器+0x1E0]（另一个东西）
- 0x81122B14  adds r1, r0, #0            ; ★ r1 = 掩码（来自 0x811330d8）
- 0x81122B16  str  r1, [r6, #0x48]
- 0x81122B18  adds r0, r2, #0            ; r0 = [管理器+0x1E0]
- 0x81122B1E  bl   #0x8111bd76
- 而 `0x8111BD76` 开头是 `ldrh r0, [r0, #0x12]` —— **把 r0 当数组指针**（`+0x12` 是计数）。
- **⇒ `[管理器+0x1E0]` 是「音符/条目数组」的指针，不是掩码。**
- **同时也纠正 `ARCH_243` §1.3 的推论**：`0x8111a21e` **不是"逐项构造掩码"**，而是
- 0x8111A224  r6 = [r4, #4]              ; 元素数
- 0x8111A226  r0 = r6 << 3               ; 每元素 8 字节
- 0x8111A228  r7 = r0 + 0x10
- 0x8111A234  bl #0x8115d79c             ; 分配
- 0x8111A25E  blx #0x812b2260            ; 清零
- 0x8111A292  str r0, [r2]               ; 逐元素拷贝 8 字节
- 0x8111A296  str r3, [r2, #4]
- ⇒ `0x8111a2a6` 的循环是在**收集一组条目指针**，不是在算掩码。
- 0x811330D8  push {r4,r5,r6,lr}
- 0x811330DA  r2 = [r0]
- 0x811330DE  ands r2, r2, #1            ; ★ [r0] 的 bit0 = 模式位
- 0x811330E6  bne  → 0x81133130
- 0x811330E8  r1 <<= 1
- 0x811330EA  r5 = [r0, #8]              ; ★ 数组长度
- 0x811330F2  r3 = [r0, #4]              ; ★ u16 数组基址
- 0x811330F4  ldrh.w r3, [r3, ip, lsl #2]
- 0x81133104  ldrh.w r4, [r4, ip, lsl #1]
- 0x81133108  orrs r4, r3                ; ★★ 或起来
- 0x81133118  ldrh.w r3, [r3, r2, lsl #1]
- 0x81133128  ldrh.w r2, [r2, ip, lsl #1]
- 0x8113312C  orrs r2, r3                ; ★★ 再或
- **⇒ 「输入动作掩码」= **若干 u16 项的按位或**。
- （例如刮盘默认摇杆、设置可改成 L/R ⇒ 该动作的掩码可能是 `L|R`）。
- **⇒ 目标链条的最后一个未知量变成**：
- > **`[管理器+0x424]` 指向的那个 u16 动作数组是谁填的？**（`0x81122AC2`/`0x81122AE0`
- > 读的就是 `[r6+0x424]`，作为 `0x811330d8` 的 `r0`。）
- └─ ★ 设置/映射                              ← 未知（代码里无固定表）
- └─ u16 动作数组 [管理器+0x424]          ← ★★ 本轮定位到"用"它，未定位"填"它

---

## `ARCH_245_MNS_KEYMAP_NOT_IN_DATA.md`

**标题**：ARCH_245 — 「id → 物理按键」映射的静态数据路线**穷尽性排除** ｜ 1. 工具 ｜ 2. 路线 A：u16 掩码表 —— 排除 ｜ 3. 路线 B：位序号字节表 —— 排除 ｜ 4. 路线 C：代码中"同窗口 materialize 多个音符位" ｜ 5. 本次排除**并不**影响已确立的结论 ｜ 6. 下一步（唯一剩下的路线） ｜ 7. 复现命令

**发现/结论**（34 条，最多列 40）：

- 结论（**可复现的排除性结论**）：**eboot.bin 中不存在任何形式的静态映射表。**
- 映射是在代码里运行时构造的。这排除了"读数据段表"这条路线。
- | `C:\dev\p5d_build\_mns\disasm_range.py` | 反汇编指定 VA 区间 |
- 地址换算：`stored_VA = file_offset + 0x80FFEF20`。
- 段范围：CODE `[0x10E0, 0x3228F0)`，DATA `[0x3228F0, 0x53E364)`（file offset）。
- `{0x0001..0x8000}` ∪ `{0x0300 = L|R}`，且**至少 4 个不同值**（排除同值填充区）。
- | 6 × u16 | 1 | `0x8153C412` — `SEL L3 L3 R3 R3 STA` |
- `0x813AC450` 一带——那是把 `0x0001…0x8000` **全部位平铺**一遍的序列
- ⇒ **不存在由按键位构成的连续数组。**
- | 7 连续字节 ∈ `{4,6,7,8,9,12,13,14}`，≥5 不同 | 2 — `0x8139F448`、`0x813A0446` |
- `0x8139F447 : UP R UP L UP DN UP LF` = `<常量> <值>` 交替，
- ⇒ 严格签名（6 连续字节）**零命中**。
- 0x811AE77C .. 0x811AE7DC  (跨度 96)  音符 = UP/LF/TRI/CRO
- 反汇编 `0x811AE740–0x811AE7F0`：
- 0x811AE77C  orrs r0,r0,#0x10    ; UP
- 0x811AE780  str  r0,[r6,#0x10]
- 0x811AE796  ands r0,fp,#0x20    ; 检查 RIGHT
- 0x811AE79E  orrs r0,r0,#0x20
- 0x811AE7A4  ands r0,fp,#0x4000  ; 检查 CROSS
- 0x811AE7AC  orrs r0,r0,#0x4000
- 0x811AE7B2  ands r0,fp,#0x80    ; 检查 LEFT
- 0x811AE7BA  orrs r0,r0,#0x80
- 0x811AE7C0  ands r0,fp,#0x1000  ; 检查 TRIANGLE
- 0x811AE7DC  orrs r0,r0,#0x1000
- ⇒ 该函数产出的是 `[obj+0x10]` 位域，**位序与 PSV 原始位一致**，
- 函数入口 `0x811AE444`（向前最近的 `push` 序言），**直接调用者 0 个**
- - 判定函数 `0x8111B64C` 用 `1 << id` 与 `sl`（音符类型掩码）做 `ands`，
- - `0x811330d8(r0=[manager+0x424], r1=frameIndex)` 返回 u16 数组若干项的 **OR**，
- ⇒ `[manager+0x424]` 指向的是**逐帧累积的动作掩码历史**，
- 只做一次性指针安装（且前置断言"原值必须为 0"）。
- 需向上读 `0x81126154`（MNS loader）中 `r5`/`r6` 的定义。
- 4. 填充点的输入来自 `0x816CDFB0`（帧边沿掩码 `(cur^prev)&cur`，
- py -3 C:\dev\p5d_build\_mns\disasm_range.py 0x811AE740 0x811AE7F0
- py -3 C:\dev\p5d_build\_mns\findfunc.py 0x811AE740

---

## `ARCH_246_replay_dance_not_input.md`

**标题**：ARCH_246 — ★ 推翻 `ARCH_244`：`[manager+0x424]` 是 **`"replay dance"` 回放轨道**，不是物理输入掩码 ｜ 1. 起点 ｜ 2. 字符串常量（决定性证据） ｜ 3. 构造函数 `0x81132fae` 全貌 ｜ 3.1 独立确认 `ARCH_244` 的读取器契约 ｜ 3.2 元素编码是 **RLE**（两个结构相同的解码循环） ｜ 4. ★ 结论：`ARCH_244` 的核心假设被推翻 ｜ 4.1 附带修正 `ARCH_242` 的措辞 ｜ 5. 新增可用资源名（下一步用） ｜ 6. 下一步（收敛后的精确目标） ｜ 7. 复现命令

**发现/结论**（90 条，最多列 40）：

- 本轮为**决定性突破 + 对既有结论的修正**。
- `ARCH_245` §6 第 2 步：向上读 MNS loader `0x81126154`，找安装 `[r7+0x424]` 时
- 0x811262EC  adds  r5, r0, #0          ; r5 = 0x8116f900(...) 的返回值
- 0x811262F2  bl    #0x8116f8a8         ; 全局 manager 访问器
- 0x811262F6  ldr.w r0, [r0, #0x424]    ; 读旧值（供前置断言）
- 0x8112630A  bl    #0x81132fae         ; ★ r5 = 0x81132fae(资源)
- 0x8112630E  adds  r5, r0, #0
- 0x811263AC  str.w r5, [r7, #0x424]    ; ★ 安装
- 0x811262DE  movw  r0, #0x3388
- 0x811262E2  movt  r0, #0x8130          ; r0 = 0x81303388
- 0x811262E8  bl    #0x8116f900          ; 按名加载资源
- `stored_VA = file_offset + 0x80FFEF20`，直接读 `eboot.bin`：
- | `0x81303330` | `ASSERT:%s` |
- | `0x8130333C` | `E:\P3DP5D\xrd757\program\app\musical_note_sequencer\mnsProcess.c` |
- | `0x81303380` | `MNS_SE` |
- | **`0x81303388`** | **`replay dance`** |
- | `0x813033B4` | `dance/mns/mns%03d_%d.bin` |
- | `0x813033D0` | `MNS_DATA` |
- | `0x813033DC` | `COMSE` |
- | `0x813033E4` | `DANCE_UI_ARC` |
- | **`0x81303B00`** | **`E:\P3DP5D\xrd757\program\app\musical_note_sequencer\mnsReplay.c`** |
- | `0x81303B40` | `ASSERT:%s` |
- ⇒ `0x8116f900(0x81303388)` = **加载名为 `"replay dance"` 的资源**。
- 反汇编 `0x81132FAE–0x811330C6`（`py -3 disasm_range.py 0x81132fae 0x811330d8`）：
- 0x81132FAE  push.w {r4,r5,r6,r7,r8,lr}
- 0x81132FB4  adds   r4, r0, #0            ; r4 = 描述符
- 0x81132FB6  ldr.w  lr, [r4, #4]          ; 列表 A 头 {u32 n; u16 data[]}
- 0x81132FBC  ldr    r6, [r4, #8]          ; 列表 B 头
- 0x81132FBE  add.w  r2, lr, #4
- 0x81132FC2  ldr.w  lr, [lr]              ; ← A 的长度
- 0x8113305A  adds   r5, r7, #0            ; r5 = sumB
- 0x8113305C  cmp    r5, r1
- 0x8113305E  bhs    #0x81133062
- 0x81133060  adds   r5, r1, #0            ; r5 = max(sumA, sumB)  ★ 元素数
- 0x81133062  lsls   r0, r5, #1
- 0x81133064  adds.w r6, r0, #0x10         ; size = n*2 + 16
- 0x81133068  bl     #0x8115d614           ; 分配器
- 0x8113306E  adds   r0, r6, #0
- 0x81133070  bl     #0x8115d79c
- 0x81133076  movw   r0, #0x3b00

---

## `ARCH_247_input_to_note_id_chain.md`

**标题**：ARCH_247 — ★★ 实机输入 → 音符 id 的完整代码链路（映射表确实在代码里） ｜ 0. 一句话结论 ｜ 1. 回放 / 实机 的**条件分支**（`0x81122AB6`） ｜ 2. `0x811330d8` 读取器全文语义（**修正 `ARCH_244`**） ｜ 3. ★ 实机输入分支：掩码 `r4` 的构造（`0x81122B3A`） ｜ 3.1 六次「按下」测试（`0x8116ef8e`，内部转调 `0x8116ef02(…, r3=0)`） ｜ 3.2 刮盘（先过设置开关） ｜ 3.3 六次「触碰」测试（`0x8116f024` → `0x8116ef98(…, r3=0)`） ｜ 3.4 其他位 ｜ 4. ★★ keyMask → 内部掩码 的解析原语（`0x8116ef02`） ｜ 5. 本轮确定的事实（证据等级） ｜ 6. 尚未解出（下一轮的唯一目标） ｜ 6.0 本轮已推进到（下一轮直接从这里开始） ｜ 7. 复现命令 ｜ 8. 与既有文档的关系

**发现/结论**（104 条，最多列 40）：

- 0x81122AB6  ldr  r0, [r6, #4]
- 0x81122AB8  movs r5, #1
- 0x81122ABA  ands r0, r0, #0x200
- 0x81122AC0  beq  #0x81122b3a        ; ★ 0x200 == 0 → 实机输入分支
- 0x81122AC2  ldr.w r0, [r6, #0x424]  ; replay dance 轨道
- 0x81122AC6  cbnz r0, …
- 0x81122AE4  ldr.w r4, [r6, #0x1f0]
- 0x81122AE8  adds  r4, #1
- 0x81122AEA  ldr.w r1, [r6, #0x1ec]
- 0x81122AF8  bl    #0x811330d8       ; ★ 读轨道
- 0x81122B14  adds  r1, r0, #0        ; r1 = 掩码
- 0x81122B16  str   r1, [r6, #0x48]   ; ★ 存进 manager+0x48
- 0x81122B1E  bl    #0x8111bd76       ; ★ 判定入口
- `ARCH_246` 的结论**被证实并收窄**：`[manager+0x424]`（`"replay dance"` 轨道）
- 两个模式在 `0x81133152` 汇合：
- 0x81133152  ands  r3, r4, #0x3f      ; cur 低 6 位
- 0x8113315C  ror.w r4, r4, #6
- 0x81133166  bfi   r6, r4, #8, #3     ; cur 的位 6..8（3 位）→ 结果位 8..10
- 0x8113316E  eors.w r2, r6, r5        ; cur ^ prev
- 0x81133172  ands.w r0, r2, r6        ; ★ (cur ^ prev) & cur  ← 上升沿掩码
- 0x8113317A  bfi   r0, r3, #0x10, #6  ; ★ 并把 cur 低 6 位放到结果位 16..21
- ⇒ 返回值 = **位 0..8 上升沿掩码** + **位 16..21 = `cur & 0x3f`（保持态）**。
- 0x81122B3A  ldr.w r0, [r6, #0x1e0]
- 0x81122B3E  ldr   r1, [r0]
- 0x81122B40  ands  r0, r1, #0x40
- 0x81122B44  beq   #0x81122b4a
- 0x81122B46  adds  r5, r4, #0                ; (r4=0) → 掩码 0
- 0x81122B48  b     #0x81122fea
- 0x81122B4A  adds  r7, r4, #0                ; r7 = 0
- 0x81122BB6  movs.w r0, #0x1620
- 0x81122BBA  bl     #0x81175134        ; 设置位 0x1620 的布尔查询
- 0x81122BBE  cmp    r0, #0
- 0x81122BC0  beq    #0x81122bea        ; 关闭 → 跳过刮盘
- 0x81122BC6  movs.w r2, #0x400
- 0x81122BCA  bl     #0x8116ef8e
- 0x81122BD2  orrs   r4, r4, #0x500     ; → 位 8 与位 10
- 0x81122BDA  movs.w r2, #0x800
- 0x81122BDE  bl     #0x8116ef8e
- 0x81122BE6  orrs   r4, r4, #0x100     ; → 位 8
- ⇒ **刮盘 = 位 8**（与 `ARCH_245` 的推断一致），

---

## `ARCH_248_keyconfig_object_and_backend_registry.md`

**标题**：ARCH_248 — 键位配置对象 / 输入后端注册表 / 死代码确认 ｜ 1. 配置对象构造器 `0x8116E63C`（键位配置的真正建立点） ｜ 1.1 第一循环：注册两个输入后端 ｜ 1.2 第二循环：建立 **4 个玩家节点** ｜ 2. ★ 输入后端注册表 `0x813AFC10`（24 字节记录） ｜ 2.1 表内容（按 24 字节步长 reinterpret，`0x813AFC10` 起） ｜ 2.2 ★★ 更正（同一轮内）：这些是**数学工具**，不是输入后端 ｜ 3. ★ 排除一条有害线索：`0x8116F4F8` 是死代码 ｜ 4. 本轮确定的事实 ｜ 5. 下一轮目标（**修订版**） ｜ 5.1 一条尚未验证的重要怀疑 ｜ 6. 复现命令 ｜ 7. 与既有文档的关系

**发现/结论**（120 条，最多列 40）：

- 并排除了一条会误导的线索。
- 0x8116E63C  push.w {r4,r5,r6,r7,r8,sb,sl,fp,lr}
- 0x8116E640  sub    sp, #0x14
- 0x8116E642  movw   fp, #0x414c ; movt fp, #0x812b   ; 栈保护
- 0x8116E650  movw   r5, #0xfc0c ; movt r5, #0x813a   ; ★ 全局 0x813AFC0C
- 0x8116E658  ldr    r0, [r5]
- 0x8116E65C  cbz    r0, #0x8116e674
- 0x8116E65E  … bl #0x8115f832                         ; 断言"必须尚未初始化"
- 0x8116E674  movw   r1, #0xf3c7 ; movt r1, #0x8116   ; ★ 类型表 0x8116F3C7
- 0x8116E67C  movs   r0, #0x8c                         ; ★ 配置对象大小 0x8C
- 0x8116E67E  bl     #0x8101612c                       ; 分配
- 0x8116E684  bl     #0x81016378
- 0x8116E688  str    r6, [r0]
- 0x8116E68C  str    r0, [r5]                          ; ★ 存入 0x813AFC0C
- 0x8116E690  strd r1=1, r4=0, [sp]        ; sp[0]=1, sp[4]=0
- 0x8116E694  strd r4, r4, [sp, #8]        ; sp[8]=0, sp[0xc]=0
- 0x8116E698  adds r0, r6, #0               ; r6 = 0, 1
- 0x8116E69A  bl   #0x8116f3e8
- 0x8116E69E  adds r6, #1
- 0x8116E6A0  cmp  r6, #2
- 0x8116E6A2  blo  #0x8116e698
- 0x8116E6A4  r6 = sp                       ; sp[0..3] = {1,0,0,0}
- 0x8116E6A6  r8 = r4 = 0
- 0x8116E6A8  sb = [0x813AFC0C]
- 0x8116E6B0  r7 = [r6]                     ; r7 = {1,0,0,0}[r8]
- 0x8116E6CA  ldr r0,[sb] ; movs r1,#0x7c ; bl #0x810164f4   ; ★ 节点大小 0x7C
- 0x8116E6D6  bl   #0x81016818
- 0x8116E6DC  str  r8, [sl]                 ; ★★ node[0x00] = ctx = r8
- 0x8116E6E2  str  r4, [sl, #4]             ; ★★ node[0x04] = port = 0
- 0x8116E6E8  str  sb, [sl, #8]
- 0x8116E6EE  r0=r8 ; r1=0 ; r2=r7 ; bl #0x8116f3fe
- 0x8116E6F2  str  r0, [sl, #0xc]           ; node[0x0c] = 后端对象
- 0x8116E6F6  bl   #0x8116f480              ; 虚调用 [node+0x14](node+0x1c)
- 0x8116E6FA  r8++ ; r6 += 4
- 0x8116E700  cmp  r8, #4
- 0x8116E704  blo  #0x8116e6a8
- | 偏移 | 含义 | 证据 |
- | `+0x00` | **ctx**（玩家 0..3） | `0x8116E6DC` |
- | `+0x04` | **port**（=0） | `0x8116E6E2` |
- | `+0x08` | 父/链表 | `0x8116E6E8` |

---

## `ARCH_249_MNS_KEYMAP_SOLVED.md`

**标题**：ARCH_249 — ★★★ 音符 id → 物理按键 已解出（附一条影响全局的映射更正） ｜ 0. 结论（先给答案） ｜ 1. ★ 方法性更正：VA → file_offset 映射 ｜ 1.1 事实 ｜ 1.2 此前用的单一公式 ｜ 1.3 为什么以前没发现 ｜ 1.4 实证（`vamap_check.py` / `datamap_check.py`） ｜ 2. 完整推导链（每一步都可复现） ｜ 2.1 按键表 `0x813AFC40`（12 × u32） ｜ 2.2 输入状态刷新函数 `0x8116F4F8` ｜ 2.3 判定侧把 note id 绑到 internal 位 ｜ 2.4 合成 ｜ 3. ★ 独立互证（换第二个几何复验） ｜ 4. 连带的重大更正：`0x8116F4F8` 不是死代码 ｜ 5. 交付物 ｜ 6. 仍未解出（不影响本结论） ｜ 7. 对既有文档的影响清单 ｜ 8. 复现命令

**发现/结论**（55 条，最多列 40）：

- 所有数据段结论的方法性错误**。
- 可复现证据：`py -3 C:\dev\p5d_build\_mns\mns_keymap.py` → `MNS_KEYMAP_ALL_PASS`
- （脚本内断言集合相同，否则返回非 0）。
- | 0 | `PT_LOAD` | `0x000000E0` | `0x81000000` | `0x00321810` | `0x00321810` | R+X |
- | 1 | `PT_LOAD` | `0x003218F0` | `0x81325000` | `0x0021BA74` | `0x003AA4A0` | R+W |
- ⇒ 正确映射是**分段线性**，且 p_offset 相对内层 ELF 起点，故整体 +0x1000：
- file_offset = VA - 0x80FFEF20          （ARCH_216/217 起一直沿用）
- | CODE（`0x81000000`） | `0x10E0 + delta` | `0x10E0 + delta` | **0** |
- | DATA（`0x81325000`） | `0x3228F0 + delta` | `0x3250E0 + delta` | **`-0x27F0`** |
- **⇒ 代码段的读一直是正确的；数据段的读一直偏了 `0x27F0`。**
- 而代码段恰好偏差为 0。数据段的结论从未被字符串自检覆盖过。
- | `0x813AFC0C` | `0x811A0C07 …`（像代码指针） | **`0x00000000`** ← 与构造器"初值必须为 0"的断言一致 ✔ |
- | `0x813AFC10` | `0x0C, 0x0C, 0x811A0C53 …` | `0x8116F49B, 0x8116F49D …`（**虚表**） |
- | `0x813AFC40` | `0, 0x811A0D91, 0x811A0D9F …` | **12 项 PSV 按键掩码表** |
- **⇒ `ARCH_234` 记的"`0x813AFC40` 是 16 项代码指针派发表"是错的，
- | 0 | `0x00000020` | RIGHT | 6 | `0x00001000` | **TRIANGLE** |
- | 1 | `0x00000080` | **LEFT** | 7 | `0x00004000` | **CROSS** |
- | 2 | `0x00000010` | **UP** | 8 | `0x00000400` | L2 |
- | 3 | `0x00000040` | **DOWN** | 9 | `0x00000800` | R2 |
- | 4 | `0x00002000` | **CIRCLE** | 10 | `0x00000100` | L |
- | 5 | `0x00008000` | SQUARE | 11 | `0x00000200` | R |
- 0x8116F50A  movw r1,#0xfc40 ; movt r1,#0x813a   ; 表 = 0x813AFC40
- 0x8116F516  movs r2,#0x40                        ; 64 字节（16 项）→ 拷到栈
- 0x8116F51E  ldr  r2,[r0,#4]                      ; 玩家号
- 0x8116F528  ldrh.w r2,[0x816CDAA0 + r2*2]        ; ★ held = 原始保持按键
- 0x8116F536  r3 = 0 ; ip = 0
- 0x8116F538  ldr.w r1,[lr]                        ; table[r3]
- 0x8116F53C  ands  r1, r2                         ; held & table[r3]
- 0x8116F53E  beq   next
- 0x8116F542  lsls  r1, r3                         ; 1 << r3
- 0x8116F544  orrs.w ip, ip, r1                    ; ★ internal_mask |= 1 << r3
- 0x8116F54E  lr += 4
- > 表被 `memcpy` 到栈上再使用 ⇒ 它是**运行时可改的当前键位表**，
- `0x81122B3C–0x81122BB2`（实机输入分支，`ARCH_247` §3.1 逐条列出）：
- 经 `0x8116EF02(0, 0, keyMask)` = `([configNode+0x18] & keyMask) != 0`
- （节点由 `0x8116E63C` 建立，`node[0]=ctx`、`node[4]=port`）。
- 项目纪律要求"关键结论必须换第二个时刻/几何复验"。两处独立印证：
- 1. **刮盘**：`0x81122BB2–0x81122BEA`，设置项 `0x1620` 门控下测试的
- 2. **集合断言**：六个 id 的按键集合 `{UP,DOWN,LEFT,TRIANGLE,CIRCLE,CROSS}`
- `ARCH_240`/`ARCH_245` 判 `0x8116F4F8` 为死代码，`ARCH_248` §3 还给出了

---

## `ARCH_24_OVERVIEW3.md`

**标题**：P5D 引擎运行原理总览（第三轮） ｜ 0. 本轮纠正的结论（优先阅读） ｜ 1. 运行原理：三层嵌套调度 ｜ app 状态机解出的全部 7 个状态（`ARCH_16`） ｜ 2. 当前真正的卡点（本轮钉到指令级） ｜ P0-A：app **state 1 永不退出**（不是 state 6） ｜ P0-B：`appSound.c:1003` 断言（音频，启动早期） ｜ P0-C：titleSequence state 7 的完成位从未置起 ｜ 3. 音游对局：游戏**从未进过对局** ｜ 4. 时间系统：**没有可用时钟**（`ARCH_19`） ｜ 5. 数据格式：资源侧**已不缺数据**（`ARCH_20`） ｜ 6. 场景图：引擎是「全局对象链表 + 虚表派发」（`ARCH_21`） ｜ 7. 输入链：按键**确实到了游戏**，但消费端没被执行（`ARCH_18`） ｜ 8. 下一步：从当前卡点到「打完一局」的最短路径 ｜ 9. 文档索引（累计 24 份）

**发现/结论**（55 条，最多列 40）：

- > 本文件汇总运行原理，并再次纠正若干结论。
- | # | 旧结论 | 实际 | 证据 |
- | 1 | app 状态表在 `[0x813B11AC]` | 那是运行时被 `0x811858BC` 写入的**行基址**；真表在 `0x813B01B4`，`row = 0x813B01B4 + state*0x50` | `ARCH_16` |
- | 2 | `titleSequence` 的 TBH 表 9 个目标 = `ARCH_6` 给的值 | **9 个全部偏移 4 字节**（旧值落在指令中腰）。逐字节重算：`target = 0x81198188 + 2×halfword` | `ARCH_23` |
- | 3 | BADMEM 是 `[TSEQ-SKIP]` 造成的 | h1164 的 badmem 在 **F3088 出现两次，比 `[TSEQ-SKIP]`(F3089) 早一帧** ⇒ 与它无关 | `ARCH_23` |
- | 4 | 「内容几何只 431 笔」可能是背景本来就简单 | **否决**：引擎注册了 **1631 个 GXP 程序，其中 681 个是骨骼蒙皮 3D**，而本相位只 set 过 6 个顶点 + 6 个片段程序 ⇒ **3D 路径一次都没被驱动**；431 笔是「链上没节点」 | `ARCH_21` |
- | 6 | 六条件门在 `0x81184B60` | 在 `0x81184B38`；全过后 `0x81184B9C bl 0x810E6650` = **helpWindowInit()** | `ARCH_22` |
- ├─ app 状态机        表 0x813B01B4 + state*0x50
- │     调用点 0x81185818(enter) / 0x8118586E(update) / 0x81185608(leave)
- ├─ 每帧任务链表调度器  0x811DB1F0
- └─ 宏观相位机        titleSequence 0x81198172（TBH 表 @0x81198188，state 0..8）
- | 6 | title sequence | update 查 `0x8130B140`（'title sequence'） |
- - state 1 的**唯一退出条件**是 `0x8116F900("initDataRead")` 返回 0。
- - 日志显示该任务节点 `w18=0x6A29` 被反复命中、`end=0` ⇒ **initDataRead 任务从未被摘链**。
- - 最新 h1169/h1168 只有 `state -1 -> 1`；而 h1164 等旧日志能到 6 ⇒ **发生了回归**，
- 0x81171520  blx 0x812B2670
- **每帧 F10 起就失败**，远早于 BADMEM。`0x812B2670` 是哪个 NID 尚未定（import 表无法静态恢复）。
- - `0x81193A6A` = `async[0]&2 ? async[8] : 0`
- - 而 `0x81193A90` 全函数体**唯一写 `[r0]` 的是 `orrs #0x10000`（bit16）——没有任何指令置 bit1**
- - ⇒ state 7 永返 0 ⇒ **永不出 state 6**。
- - **`DanceSystem` 序列**（`0x81049AFC`，state 0→1→2，**state 2 恒返回 0 = 常驻待机**）
- 注册于 `init dancemain` = `0x81184CF8`（step `0x81184D00`），它先 `0x810499B4` 注册，
- 再轮询 `0x81049A76`；state1→2 的闸是 `0x81047280`，每帧推进 `0x81047E16`
- - 选歌：`music_select/musicSelectMain.c`，`MusicSelectMain` @ `0x81305094`
- - **谱面：`dance/mns/mns%03d_%d.bin` @ `0x813033B4`**
- ① `appSound.c:1003`（音频对象永不就绪）→ ② `init sound`（`0x811848B7`→`0x8115487A`）恒返 0
- - 游戏**没有硬件时钟**。引擎唯一软时间轴是 `0x812071F8`：用两次 `sceKernelGetProcessTimeWide`
- 之差算 deltaTime（÷1e6，返回值在 s0 浮点寄存器）。基准全局 `0x8169E090`，首帧标志 `0x81539850`。
- - 两个内核时钟桩（`0x812B28E0`、`0x812B18B0`）只是给共享变量 `g_fake_tick` **每调用 +16666/16667µs**，
- - 量化：本轮 10 帧窗口内 GetProcessTime **2607 次（261 次/帧）⇒ 约 6.8× 超速**；历史运行约 **2.4× 超速**。
- **⇒ 判定窗口是 deltaTime 的函数，窗口宽度在真实时间里从 0.4× 到 4× 不等 ⇒ 音游判定不可能正确。**
- - **歌单不在 CPK 里，而是静态编译进 eboot 的 C 表**（`musicData.c` 字符串可证，文件偏移 0x304C74）：
- VA `0x813A4CB4` 起、**步长 0x38**、G1..G4 连续 **43/43/43/44** 条，每条 `(标题指针, flag, BPM)`。
- 指针链自证：`BRAND NEW DAYS (YUYOYUPPE REMIX)` 文件偏移 0x304CB8 → VA `0x81303BD8` = 表 rec0 的 `+0x00`。
- - **从零解出两个格式**：ARC 二级归档（`name[0x20] + u32 size + payload`，顺序遍历；
- 是判定链路的单点阻塞。下一步可静态分析 `mnsProcess.c`（VA `0x8130333C`）的读表循环，无需重跑解释器。
- - **不是场景树**：`0x816CC630` 存当前列表索引，`0x816CD468[idx*4]` 是链头；
- 每帧 `0x811CBE9A` 把**三条链**各走一遍，对每个节点查虚表 `[0x814D2F2C + [node+0x20]*4]` 调用。
- - 节点是 **0x50 字节**对象，构造器 11 处（`0x811CC010/0x811CC04E/0x811CD4B0/0x811CDA06/0x811CDB70`…）。
- - 流程：`0x811CB3A0` 入链（O(1) 头插+尾回写）→ `0x811CBE9A` 驱动

---

## `ARCH_250_MNS_CORE_AND_CROSSCHECK.md`

**标题**：ARCH_250 — ★ id 重映射条件结清 + MNS 玩法核心落地（C++/Python 逐行对拍通过） ｜ 1. ★ 结清 `ARCH_229` §3.2：重映射**无条件**施加 ｜ 1.1 方向证据（判定看到的是**重映射后**的 id） ｜ 1.2 ⚠ 残余未闭合（如实记录） ｜ 2. 最终 id → 按键（含重映射） ｜ 3. 新增代码（平台无关） ｜ 3.1 `src/assets/p5d_mns.h` ｜ 3.2 `src/assets/p5d_mns_keymap.h`（更新） ｜ 3.3 对拍工具 ｜ 3.4 对拍结果（同一份数据 → 逐行相同） ｜ 4. 本轮**没有**做的事（明确边界） ｜ 5. 复现命令 ｜ 6. 对既有文档的影响

**发现/结论**（44 条，最多列 40）：

- > 承接 `ARCH_249`。本轮把「谱面 id → 按键」从**结论**推进到**可运行的代码**，
- 两处条件包裹 ⇒ **它只在特定状态下施加**（不是无条件）。**这一点尚未查清其语义**」。
- 本轮把 `0x8111AD66–0x8111AF38` 完整反汇编后，答案是**两处条件其实不起作用**：
- 0x8111AEEC  ldr   r1, [r0, #0x14]        ; r1 = id 子结构
- 0x8111AEEE  ldrb  r2, [r1, #4]           ; r2 = id 字节
- 0x8111AEF0  ands  r3, r2, #0x80          ; r3 = bit7 有效位
- 0x8111AEF4  bne   #0x8111aefa
- 0x8111AEF6  cmp   r2, #8
- 0x8111AEF8  beq   #0x8111af20            ; ★ id 8（刮盘）显式跳过
- 0x8111AEFA  ldr   r0, [r0]
- 0x8111AEFC  ands  r0, r0, #0x10          ; 条件
- 0x8111AF00  beq   #0x8111af12
- 0x8111AF06  mov   r0, sp
- 0x8111AF08  add.w r0, r0, r2, lsl #2
- 0x8111AF0C  ldr   r0, [r0, #0x20]        ; 表 [sp+0x20]
- 0x8111AF0E  strb  r0, [r1, #4]
- 0x8111AF10  b     #0x8111af20
- 0x8111AF12  cbz   r3, #0x8111af16
- 0x8111AF14  movs  r2, #0x7f
- 0x8111AF16  mov   r0, sp
- 0x8111AF18  add.w r0, r0, r2, lsl #2
- 0x8111AF1C  ldr   r0, [r0, #0x20]        ; ← 与上支**逐字节相同**的取表序列
- 0x8111AF1E  strb  r0, [r1, #4]
- `0x8111AF06–0x8111AF0E` 与 `0x8111AF16–0x8111AF1E` 是**完全相同的指令序列**，
- `and &0x10` 的两支**产生同一结果**。⇒ **重映射对 id 0..5 无条件施加**；
- 0x8111AEC2  movs r0,#1 ; strd r0, r1=[sp+0x94]=0, [sp,#0x20]  → [0x20]=1, [0x24]=0
- 0x8111AEC6  movs r2,#3
- 0x8111ACCC  movs r0,#2 ; strd r2, r0, [sp,#0x28]              → [0x28]=3, [0x2c]=2
- 0x8111AED0  movs r4,#5
- 0x8111AED6  movs r1,#4 ; str  r4, [sp,#0x30]                  → [0x30]=5
- 0x8111AEDC  str  r1, [sp,#0x34]                               → [0x34]=4
- ⇒ **`REMAP = {1, 0, 3, 2, 5, 4}`**（自逆对合 `0↔1, 2↔3, 4↔5`），与 `ARCH_229` 一致。
- | 重映射**写** `[r1, #4]`，其中 `r1 = [note+0x14]` | `0x8111AF0E` / `0x8111AF1E` |
- | 判定**读** `[r0, #4]`，其中 `r0 = [r7+0x14]` | `0x8111B688`（`ARCH_230`） |
- ⇒ **谱面文件里存的是重映射前的 id**；`1 << id` 用的是重映射后的位序号。
- `0x8111AE6E..0x8111AE7E` 从源结构 `+0x38` / `+0x3c` 写两个 note 的 id。
- 不改变集合，只改变**行序**。这解释了 `ARCH_249` 的集合断言为何在两种变体下都成立，
- 也说明**集合级互证不足以定方向**，方向只能靠 §1.1 的字段级证据。
- 默认 `calibrated = false`，未标定时 `update()` **不产生任何判定结论**
- `psvButtonForJudgeBit()`、`psvButtonForFileNoteId()`，并把 §1 的证据块写进注释。

---

## `ARCH_251_JUDGE_GRADE_CASCADE.md`

**标题**：ARCH_251 — ★ 判定评级函数读通（四级嵌套带级联）+ C++ 判定核心落地并自检通过 ｜ 1. 判定入口 `0x8111B64C` 的两个确切常量 ｜ 2. ★ 评级函数 `0x8111B018`：四级**嵌套带**的级联 ｜ 3. C++ 落地（`src/assets/p5d_mns.h`） ｜ 4. 本轮验证结果（`mns_test.exe`，退出码 0） ｜ 4.1 解析层对拍（与 `ARCH_250` 相同，未回归） ｜ 4.2 判定级联自检（9 个边界案例，全部 PASS） ｜ 4.3 真实谱面端到端 ｜ 5. 仍未解出（明确边界） ｜ 6. 复现

**发现/结论**（46 条，最多列 40）：

- 0x8111B6F8  ands r1, fp, #0x40          ; fp = [state]
- 0x8111B6FC  beq  #0x8111b764            ; 未置位 → 另一支（0x8111B764）
- 0x8111B6FE  ldr  r0, [sp]
- 0x8111B700  cmp  r0, #1                 ; kind == 1
- 0x8111B702  bne.w #0x8111bd6a
- 0x8111B72E  movw r0, #0xd9c5
- 0x8111B736  movt r0, #0xbd87            ; ★ 0xBD87D9C5
- 0x8111B73E  vcmp.f32 s1, s0
- 0x8111B746  bmi.w #0x8111bd6a           ; s1 < lo → 退出
- 0x8111B74A  movw r0, #0x8889
- 0x8111B74E  movt r0, #0x3d08            ; ★ 0x3D088889
- 0x8111B756  vcmp.f32 s1, s0
- 0x8111B75E  bgt.w #0x8111bd6a           ; s1 > hi → 退出
- | `0xBD87D9C5` | **−0.066333331 s** | 下界（太早） |
- | `0x3D088889` | **+0.033333335 s** | 上界（太晚），**恰为 1/30** |
- 分辨率约 5e-9，所以这是作者给定的**非整帧**值，不是舍入误差。不要"修正"它。
- `0x8111B018(note r0, arg r1, delta s0)`，核心是**四次虚调用**，
- 0x8111B034  ldr  r1, [r5, #0x30]        ; note->[0x30] = 窗口提供者
- 0x8111B052  str  r5, [sp]               ; 第 5 个参数 = note
- 0x8111B054  add  r0, sp, #8             ; 第 1 个参数 = &out2 (两个 float)
- 0x8111B058  movs r1, #1
- 0x8111B05A  movs r2, #3                 ; ★ level = 3
- 0x8111B05C  blx  lr
- 0x8111B05E  vldr s1, [sp, #8]           ; s1 = lo
- 0x8111B062  vcmp.f32 s16, s1
- 0x8111B06A  bmi.w #0x8111b170           ; d < lo → 返回 3
- 0x8111B07A  vldr s0, [sp, #0xc]         ; s0 = hi
- 0x8111B086  bls  #0x8111b08a
- 0x8111B088  b    #0x8111b178            ; d > hi → 返回 5
- 0x8111B0B0  movs r2, #2                 ; level = 2 …
- 0x8111B0FA  movs r2, #1                 ; level = 1 …
- 0x8111B144  movs r2, #0                 ; level = 0 …
- 0x8111B168  movs r0, #1      ; 最内层
- 0x8111B16C  movs r0, #2
- 0x8111B170  movs r0, #3
- 0x8111B174  movs r0, #4
- 0x8111B178  movs r0, #5
- **⇒ 四个带是嵌套的，且是「每音符」的数据**（由 `note->[0x30]` 的虚调用提供），
- **调用方**把 5 当作"未命中"：`0x8111B79E cmp r1, #5; beq …`。
- ⇒ 1..4 是命中等级，5 是未命中。**1..4 到游戏内 PERFECT/GREAT/GOOD 名称的

---

## `ARCH_252_JUDGE_WINDOW_TABLE.md`

**标题**：ARCH_252 — ★★ 判定窗口真实数值读出（表 @ 0x813A7ED0）+ 目标闭合 ｜ 1. 链条（逐层，全部指令级证据） ｜ 2. ★★ 独立互证（本项目要求"换第二个几何复验"） ｜ 3. 实际使用的切片（`a=0, c=1`，`b=level`） ｜ 4. 落地产物 ｜ 5. 验证结果（`mns_test.exe`，退出码 0） ｜ 5.1 解析层对拍（无回归） ｜ 5.2 级联控制流自检（合成带，9 点全 PASS） ｜ 5.3 真实窗口表驱动的偏移扫描（15 点全 OK） ｜ 5.4 其他 ｜ 6. ★ 目标闭合状态 ｜ 7. 仍未解出（不影响目标闭合，如实记录） ｜ 8. 复现

**发现/结论**（41 条，最多列 40）：

- 0x8111BD76(manager, mask, now)                 ; 判定外层：遍历 manager+0x14 音符链
- 0x8111BDBE  adds r1, r7, #0                  ; ★ 判定入口 arg1 = manager
- 0x8111BDC2  bl   #0x8111b64c
- 0x8111BDCA  bics.w r8, r8, r0                ; 用返回值消费位
- 0x8111B64C(note, manager, mask, now)           ; sb = manager
- 0x8111B77E  mov  r0, sb                      ; → arg0 = manager
- 0x8111B780  adds r1, r7, #0                  ; → arg1 = note
- 0x8111B786  bl   #0x8111b018
- 0x8111B018(manager r0, note r1, delta s0)      ; 评级级联（ARCH_251）
- 0x8111B034  ldr  r1, [r5, #0x30]             ; ★ manager+0x30 = 窗口提供者
- 0x8111B05C  blx  lr                          ; provider(&out2, 1, level, note, manager)
- manager 构造点 0x8111A0AC/0x8111A0B6
- str.w r2, [sl, #0x30]                        ; ★ manager+0x30 = 0x8111C04F
- 0x8111C04E(out2, sel, level, note, manager)    ; 窗口提供者
- 0x8111C0BA  movs r0, #0                      ; a = 0
- 0x8111C0BC  adds r1, r4, #0                  ; b = level
- 0x8111C0BE  movs r2, #1                      ; c = 1
- 0x8111C0C0  bl   #0x811333bc                 ; lo
- 0x8111C0CE  bl   #0x8113342c                 ; hi
- 0x811333BC(lo) / 0x8113342C(hi)
- 0x81133412  lsls r0, r6, #7                  ; a*128
- 0x81133414  add.w r0, r0, r5, lsl #5         ; + b*32
- 0x81133418  add.w r0, r0, r4, lsl #3         ; + c*8
- 0x81133420  movt r1, #0x813a                 ; 0x813A7ED0
- 0x81133426  vldr s0, [r0, #0x144]            ; ★ lo
- 0x81133496  vldr s0, [r0, #0x148]            ; ★ hi
- **表地址**：`0x813A7ED0 + (a*16 + b*4 + c)*8 + 0x144`，共 **64 项 float 对**。
- > ★ 这里的读必须用**正确的分段映射**（`ARCH_249` §1）：
- > DATA 段 `file_offset = 0x3228F0 + (VA − 0x81325000)`。
- > 旧的单公式 `VA − 0x80FFEF20` 在数据段偏 `0x27F0`，会读到完全错的内容。
- 而 `ARCH_251` 从 **完全不同的代码路径**（`0x8111B64C` 的 `[state]&0x40` 门控分支，
- 立即数 `0xBD87D9C5` / `0x3D088889`）独立读到：
- 指向同一张表并完全吻合 ⇒ 表地址、索引公式、分段映射**同时对**。
- | `C:\dev\p5d_build\_mns\gen_windows_header.py` | 表生成器（含 §2 的自检断言，失败即中止） |
- > 代码是对的。已修正期望表并复跑通过。
- 评级分布(1..5): 802 0 0 0 0            （输入恰好按时 ⇒ 全在最内层，自洽）
- 未标定提供者：不产生任何判定结论         PASS
- | 评级码 1..4 → PERFECT/GREAT/GOOD 的**名称** | 需打分/UI 侧证据。调用方只区分 5=未命中，1..4=命中 |
- | `0x8111A0AA bne` 跳过的另一提供者 | `manager+0x30` 在某些条件下可能不是 `0x8111C04F`，条件未查清 |
- | 真机验证 | 未做（全部证据为 Windows 侧 + 交叉编译） |

---

## `ARCH_253_PLAYSESSION_AND_CLOCK.md`

**标题**：ARCH_253 — 按键表补全（16 项）+ 动作掩码转换 + 歌曲时钟 + 可运行游玩循环 ｜ 1. ★ 更正：按键表是 **16 项**，不是 12 项 ｜ 1.1 由此补上的转换（本轮关键新增） ｜ 2. 新增：歌曲时钟 `src/assets/p5d_clock.h` ｜ 3. 新增：游玩循环 `src/assets/p5d_session.h` ｜ 4. 验证结果（`mns_test.exe`，退出码 0 → `MNS_CPP_ALL_PASS`） ｜ 4.1 解析层对拍（无回归） ｜ 4.2 游玩循环（真实窗口 + 真实输入链 + 确定性时基，`mns218_2`） ｜ 5. 仍未解出（不影响本轮结论） ｜ 6. 复现

**发现/结论**（10 条，最多列 40）：

- 按正确分段映射重读 `0x813AFC40` 全部 16 项：
- ⇒ `0x8116F4F8` 的 16 次循环（`memcpy 0x40` 字节 = 16 个 u32）是**正确的**，
- 之前"12 项"的记法不影响任何结论（前 12 项相同，且六音符与 L/R 都在其中），
- | **音频采样帧** | `onAudioFrames(frames, sampleRate)` | **★ 接 Android AAudio 的接口** |
- → actionMaskFromPsv()              复刻 0x8116F4F8
- → EdgeDetector (cur^prev)&cur       复刻 0x816CDFB0
- 其余既有自检（级联 9 点、偏移扫描 15 点、未标定零结论）**全部保持 PASS**。
- > 1. 用"整体平移输入 0.5s ⇒ 应零命中"作判据 —— **无效**：平移输入等价于平移谱面，
- >    密集谱上必然有别的音符被"按时"命中（实测 186 条）。改用"曲终后按全键"。
- | 评级码 1..4 → PERFECT/GREAT/GOOD 的**名称** | 需打分/UI 侧证据 |

---

## `ARCH_254_CLOCK_DRIVER_FIXES.md`

**标题**：ARCH_254 — 时钟驱动一致性修复 + 确定性/跨驱动逐位验证（目标闭合） ｜ 1. ★ Bug 1：墙钟驱动与音频帧驱动差恰好一帧 ｜ 现象 ｜ 根因 ｜ 修法 ｜ 2. ★ Bug 2：`setRate()` 会丢掉一整段时长 ｜ 现象 ｜ 根因 ｜ 修法 ｜ 3. 验证结果（`mns_test.exe`，退出码 0 → `MNS_CPP_ALL_PASS`） ｜ 3.1 解析层对拍（无回归） ｜ 3.2 ★ 确定性与跨驱动（目标的核心判据） ｜ 3.3 游玩循环（沿用上一轮，全部保持 PASS） ｜ 4. 目标闭合状态 ｜ 5. 仍未解出（不影响目标闭合） ｜ 6. 复现

**发现/结论**（7 条，最多列 40）：

- ⇒ 从锚定到改速率之间那整段 `[0, 1.0]` 的时长被丢弃。
- | (j) 幂等 | 同一 `now` 重复 tick 1000 次 ⇒ 不推进 |
- > 因此两条驱动路径的 `songSeconds` 序列**应当逐位相同** —— 实测确实如此。
- 其余既有自检（级联 9 点、偏移扫描 15 点、未标定零结论）全部保持 PASS。
- | 目标条款 | 状态 | 证据 |
- | Windows 侧可复现证据：同一输入 → 确定的时间与判定轨迹 | ✅ | (g) 两遍逐位相同；(h) 跨驱动逐位相同；摘要 `871d874fe1b44ac4` |
- | 评级码 1..4 → PERFECT/GREAT/GOOD 的**名称** | 需打分/UI 侧证据 |

---

## `ARCH_255_ANDROID_GAMEPLAY_WIRING.md`

**标题**：ARCH_255 — 玩法核心接进 Android 外壳（AAudio 时间基准 + 设备侧自检 + 一条命令验证） ｜ 1. 接线内容（`src/android/main.cpp`） ｜ 1.1 AAudio 时间基准（输出静音） ｜ 1.2 谱面加载 ｜ 1.3 ★ 设备侧玩法自检 `playSelftest()` ｜ 2. ★ 方法性坑：PowerShell 脚本必须纯 ASCII ｜ 3. 顺带修掉的真 bug：`p5d_gpu_export.h` 格式串不匹配 ｜ 4. 构建链更新 ｜ 5. 验证结果（本轮实际做到的） ｜ 5.1 交叉编译 + 打包 ｜ 5.2 AAudio 确实链上了（`.so` 里为未定义引用，运行时解析） ｜ 5.3 玩法层确实编进了 `.so`（按字符串计数） ｜ 5.4 Windows 侧回归（目标要求"保持全绿"） ｜ 5.5 一条命令的设备验证：`C:\dev\p5d_build\_android\verify_device.ps1` ｜ 6. 明确边界（本轮**没有**做到的） ｜ 7. 复现

**发现/结论**（7 条，最多列 40）：

- （`README.md` 法律声明）。缺数据时记 `[play] 缺 … ⇒ 跳过玩法层`，
- PLAY_SELFTEST MATCH (Windows 基准 871d874fe1b44ac4，实测 <...>)
- ⇒ 这把「手机与 PC 是否算出同一条判定轨迹」变成了一条可 grep 的日志。
- > 控制台里出现的 `浜ゅ弶缂栬瘧` 就是错码证据。
- 四个实参全是 `size_t`，后三个却用 `%u` —— **格式与实参不匹配属 UB**
- 必须同时改 manifest（本轮先只改了脚本，badging 仍显示 24，已修正）。
- | **在真机上运行** | ❌ **未做** —— 没有设备。上表全部是交叉编译 + 静态检查，不是运行证据 |

---

## `ARCH_256_SELFTEST_FILE_AND_TOUCH_CHANNEL.md`

**标题**：ARCH_256 — 自检摘要双写（logcat + 文件）+ 触碰通道接线 + 一条命令覆盖两路输出 ｜ 1. 自检摘要双写文件 ｜ 2. 触碰通道（位 16..21）接线 —— 但**不发明语义** ｜ 3. Windows 侧新增自检 (k)：证明"承载但不影响判定" ｜ 4. 一条命令现在覆盖**两路**输出 ｜ 5. 本轮验证结果 ｜ 6. 目标闭合状态 ｜ 7. 明确边界（仍未做到） ｜ 8. 复现

**发现/结论**（9 条，最多列 40）：

- PLAY_SELFTEST MATCH (Windows 基准 871d874fe1b44ac4，实测 <16hex>)
- 证据：`ARCH_247` §3.3（`0x81122BEA..0x81122C52`）—— 实机分支在手柄六次测试之后，
- 又用 `0x8116f024(0,0,keyMask)` 做了**六次**测试，keyMask 与顺序与手柄相同，
- 结果落在位 `0x10000..0x200000`；而读取器 `0x811330d8` 返回的正是 `bits 16..21`。
- ⇒ 本通道目前**没有任何已证实的判定效果**。
- // 触碰通道：触点 → 动作的对应关系尚未读出，故在拿到证据之前**固定传 0**。
- (k) touchChannelMask(0x3F) == 0x003F0000（位 16..21）        PASS
- (k) 加入触碰通道 ⇒ 判定轨迹逐位不变                          PASS
- | 目标条款 | 状态 | 证据 |

---

## `ARCH_257_FIRST_DEVICE_RUN.md`

**标题**：ARCH_257 — ★★★ 首次真机运行成功：设备侧判定摘要与 PC 逐位相同 ｜ 1. 设备 ｜ 2. ★ 结果：`DEVICE_VERIFY_ALL_PASS`（12/12 标记） ｜ 2.1 渲染与音频实测 ｜ 3. 三个「只有真机才暴露」的问题（全部已修） ｜ 3.1 ★ `adb shell mkdir` 建的目录应用穿不过去 ｜ 3.2 验证脚本没有 `force-stop`，跑的是上一次的进程 ｜ 3.3 数据根探针**绑死在渲染资产**上 ｜ 3.4 附带：`verify_device.ps1` 里 `adb` 不在 PATH ｜ 4. 仍未做到的 ｜ 5. 新增/修改的产物 ｜ 6. Windows 侧回归（未受影响） ｜ 7. 复现（三/四条命令）

**发现/结论**（6 条，最多列 40）：

- > 也是第一次拿到「手机与 PC 算出同一条判定轨迹」的硬证据。
- PLAY_SELFTEST MATCH (Windows 基准 871d874fe1b44ac4，实测 871d874fe1b44ac4)
- ⇒ MNS 解析、REMAP、按键表、动作掩码、边沿检测、四级判定窗口级联、
- ⇒ **AAudio 真的在回调**（`state=3` = `AAUDIO_STREAM_STATE_STARTED`），
- app 的 uid（`appId=10352` = `u0_a352`）既非属主也不在组内 ⇒ **无法穿越**。
- 现象：`[play] 无数据目录 ⇒ 跳过玩法层`，即使谱面就在正确的目录里。

---

## `ARCH_258_LIVE_MODE_ON_DEVICE.md`

**标题**：ARCH_258 — LIVE 模式在真机启用（真实游戏数据渲染）+ 三处修正 + 暴露既有的"线 C"限制 ｜ 1. 推送角色资产 ｜ 2. ★ LIVE 模式启用（真机日志） ｜ 3. 本轮修掉的三处 ｜ 3.1 呈现**不保宽高比**（画面被压扁） ｜ 3.2 相机**硬编码为别的曲子** ｜ 3.3 FPS 统计在窗口重建后打出荒谬值 ｜ 4. ★★ 暴露的既有缺口：舞者**没有舞台定位**（`ARCH_198` 的"线 C"） ｜ 5. 验证结果 ｜ 6. 产物 ｜ 7. 仍需处理 ｜ 8. 复现

**发现/结论**（4 条，最多列 40）：

- 真机截图确认：角色带**真实贴图**渲染（黑外套/红内衬/肤色/头发），
- ⇒ 横向被压扁 46%。修法：呈现着色器加 `uniform vec2 uScale`，按
- 原话「9 类资产来源全排除；社区无做法；**无可靠路径**」。
- PLAY_SELFTEST MATCH (Windows 基准 871d874fe1b44ac4，实测 871d874fe1b44ac4)

---

## `ARCH_259_LINE_C_MEASURED.md`

**标题**：ARCH_259 — 线 C 主案**测掉了**：单舞者没有"缺失的站位"（并修正 ARCH_258 §4） ｜ 1. ★ 先修正我自己的错误（`ARCH_258` §4） ｜ 1.1 坐标系判定的依据（说明这不是猜） ｜ 2. 追通描述符的来源链（回答了 `ARCH_164` §4 遗留的唯一问题） ｜ 2.1 顺带证实 `ARCH_198` 的一条撤回 ｜ 3. 本轮新增的诊断设施 ｜ 4. 仍未闭合：一处**测量与画面不一致**，尚未定性 ｜ 5. 目标状态 ｜ 6. 复现

**发现/结论**（33 条，最多列 40）：

- > **给 LIVE 模式加了测量性诊断**、**用测量结果推翻了我自己上一轮的结论**。
- `ARCH_258` §4 我写道：「角色被放在原点，而 `c218` 的运镜并不看向原点 ⇒ 缺舞台定位」。
- ⇒ **相机确实在看着角色**，不是看着别处。
- **⇒ 单舞者情况下：引擎就是把角色放在原点，相机本来就是照着原点编的。
- | 时刻 | 相机距离 | fov | 实测 bbox 高度 |
- 若是**投影/屏幕坐标**，尺寸应随距离变化约 **9 倍**；实测只变 **1.29 倍**
- ⇒ **是世界坐标**。（角色在跳舞，躯干尺寸本来就只有小幅变化。）
- 0x81043B6E  push / sub sp,#0x6c              ; 本函数（唯一调用者是 0x81043D84 的上一层）
- 0x81043B80  mov  sb, r0                      ; sb = arg0（运行时表项）
- 0x81043B8A  bl   #0x8103ff32                 ; sl = 该调用结果（另一用途）
- 0x81043B98  add.w r0, sb, #0x40
- 0x81043B9C  str  r0, [sp, #0x48]             ; ★ 描述符源 = sb + 0x40
- 0x81043D80  ldr  r0, [sp, #0x48]
- 0x81043D84  bl   #0x81047e4c                 ; arg0 = sb + 0x40
- 0x81047E4C  push.w {...} / sub sp,#0x3c
- 0x81047EA8  add.w r1, fp, #0x14
- 0x81047EBE  str   r1, [sp, #0x24]            ; ★ 描述符 = fp + 0x14（堆副本）
- 0x81047F3A  ldr.w r1, [sl]                   ; sl = arg0
- 0x81047F44  str.w r1, [lr]
- 0x81047F64  bne   #0x81047f3a                ; r4 = 0x1F ⇒ 拷 31×16 = 0x1F0 字节
- ⇒ **描述符 = 运行时表项 `+0x40` 起 0x1F0 字节的堆副本**，
- 而「运行时表项」由 `0x81046b40`（获取器）+ `group*0xDE0` 得到
- 我读了 `0x81047400` 区域（`ARCH_167` §3 指的"建表函数"）。它**不是**在算坐标：
- 0x8104742A..0x81047446  movw/movt 装入 0x812F154C/1550/1554/1558/155C/1578 → 栈
- 0x8104745A  blx  #0x812b2280   ; sprintf 家族：拼资源名
- 0x81047466  bl   #0x81174dbc   ; 按名查资源
- 0x81047496  str  r4, [r0, #4]  ; 存句柄
- 0x810474EA  str.w sl, [r5, #0x2c0] ... ; 清零后存句柄
- ⇒ 这是**按名字加载资源并保存句柄**，与 `ARCH_198` 撤回
- | 解出「位置/朝向的来源」 | 🟡 **单舞者主线已测清**：位置=原点（引擎固有），相机照它编；**来源不在资产里**（与 9 类排除结论一致） |
- | 给出可复现证据 | ✅ `[diag]` 日志 + §1.1 的坐标系判定 |
- | 区分「已确证」与「仍不确定」 | ✅ 本文 §1（确证）/ §4（不确定） |
- **结论修正**：线 C 的**主案（单舞者站位）不是"缺数据"，而是"我误读了一张截图"**。

---

## `ARCH_25_code_map.md`

**标题**：ARCH_25：引擎代码区段划分图 + 源文件路径串清单 ｜ 1. 职责（一句话） ｜ 2. 结构 / 流程图（核心） ｜ 2.1 两个段与全局布局 ｜ 2.2 模块地图（代码地址按**链接顺序**，非按 VA 单调） ｜ 2.3 数据区（非代码） ｜ 2.4 方法（可复现） ｜ 3. 函数地址表（本轮新增/复核，全部带证据） ｜ 4. 数据结构字段表 ｜ 4.1 歌单记录（stride `0x38`，**字节级自证**） ｜ 4.2 源码路径串表记录 ｜ 5. 完整源文件路径串清单（375 条 → 334 个不同文件） ｜ 5.1 `program/app/app_system`（150 条） ｜ 5.2 `program/app/dance`（33 条） ｜ 5.3 `program/app/dance_ui`（2 条） ｜ 5.4 `program/app/event`（14 条） ｜ 5.5 `program/app/game`（8 条） ｜ 5.6 `program/app/mainmenu`（67 条） ｜ 5.7 `program/app/musical_note_sequencer`（8 条）★ ｜ 5.8 `program/app/music_select`（6 条）★ ｜ 5.9 `program/app` 其它（polygon 3 / title 7 / kernel 4 / staff 4 / test 8 / movie 2 / sound 1 / debug 1 / global_data 2 / a ｜ 5.10 非 `program/` 前缀（1 条） ｜ 6. 调用链与生命周期（本轮与代码地图相关的部分） ｜ 7. 对「能否打完一局」的影响

**发现/结论**（421 条，最多列 40）：

- > 分析对象：`C:\dev\p5d_build\eboot.bin`（6,283,968 B = `0x5FE2C0`）
- > 本轮**只读**：未改任何源码、未编译、未运行解释器。所有结论来自对 `eboot.bin` 原始字节的静态扫描。
- > 统一映射（本轮再次独立验证，见 §9.2）：**文件偏移 = VA − 0x81000000 + 0x10E0**，SEG0 与 SEG1 都适用。
- 把 eboot.bin 从"一堆地址"变成**一张按源码子系统切分的地址地图**：解出全部 **375 条源码文件路径串**（其中 **334 个不同文件：310 个 `.c` + 19 个 `.inl` + 3 个 `.inc.c` + 2 个 `.h`**，等于游戏的源码文件清单），并用"代码里 movw/movt 构造出的断言串地址"把所有代码区间反查回它们所属的**源子目录**，从而给出覆盖 SEG0/SEG1 的模块级区段图。
- eboot.bin  0x5FE2C0 B
- ├─ 0x000000 .. 0x0010DF   文件头 (SCE 头，非标准 ELF：e_phoff=0 e_phnum=0 e_shnum=4 且 shdr 内容无效)
- │                          证据：e_entry=0x5FD2C0 e_phoff=0x0 e_shoff=0x5FD4A0 e_phnum=0
- │                          shdr0..3 全为 0x219EB0/0x10200 这样的垃圾值 ⇒ **无法用 phdr/shdr 恢复段表**
- ├─ 0x0010E0 .. 0x3228F0   SEG0   vaddr=0x81000000  filesz=0x321810
- │     ├─ 0x0010E0 .. 0x0F0000  代码/只读数据（app_system 前半：debug / appGraphics 图像解码）
- │     ├─ 0x0F0000 .. 0x10FC00  app_user / dance / event / game
- │     ├─ 0x10FC00 .. 0x22F000  mainmenu 全体（最大）→ mns → music_select → polygon
- │     ├─ 0x22F000 .. 0x290000  app_system 后半 / app_main / appMath / appPad / appSound / appTouch
- │     ├─ 0x290000 .. 0x2B0000  utility 尾部（script/appTimer/appResourceList…）
- │     ├─ 0x2B0000 .. 0x2E0000  kernel(init/sequence) / debugmenu / staff / test / title / CRI 音频数据
- │     └─ 0x2E6210 .. 0x30C1E0  ★ 源码路径串表（375 条，见 §3 与 §6）
- ├─ 0x3228F0 .. 0x3A5F90   ← 这段属于"SEG0 尾部 + SEG1 未覆盖区"（见 §7 未解 1）
- └─ 0x3A5F90 .. 0x3A6E18   ★ 歌单表 43 条 × 0x38  (VA 0x813A4EB0 .. 0x813A5818)
- 0x3A7590 .. 0x3A7EF8   ★ 歌单表副本 43 条     (VA 0x813A64B0 .. 0x813A6E18)
- … 直到 0x5FE2C0
- 用 `movw/movt` 常量交叉引用聚类得到（方法见 §2.4）。**va_lo/va_hi 是该源文件断言串被引用的代码地址范围**，是该文件在 eboot 中代码的近似边界。
- | 1 | `0x81000010`–`0x81002E36` | **app_system/debug** | `cpuGraph.c` `popUpMenu.c` `treeView.c` `fontView.c` `lipsyncView.c` `loudnessMeter.c` | 14 文件 |
- | 2 | `0x81002E36`–`0x81004300` | app_system low_layer | `vita_ll_touch.c` `memcardDebug.c` | |
- | 3 | `0x81003E7E`–`0x810158D8` | **app_system/appGraphics** | `appGxModel.c` `appGxScene.c` `appGxEffect.c` `appGxTexture.c` + `image/appImage*.c`(11 个解码器) + `module/gfd/gfd_app_gx*.c` | 33 文件 |
- | 4 | `0x81015F3C`–`0x81018C82` | app_system 框架 | `appJobQueue.c` `appList.c` `appNode.c` `appQueue.c` `appMatrixStack.c` | |
- | 5 | `0x810193AC`–`0x810310E4` | **app_system/utility**（前半） | `appEffect.c` `appFont/*`(13) `appMessage/*`(5) `appSprLayout/*` `appSprite/*` `appWindow.c` `appOtNode.c` | ~70 文件 |
- | 6 | `0x810311EC`–`0x81033000` | **app_user/message** | `message_format*.c` `tag_format.c` `memcard.c` | |
- | 7 | `0x810394DE`–`0x81051xxx` | **dance** | `danceResource.c` `danceSystem.c` `danceMain.c` `danceMusic.c` `danceTimeAdjuster.c` `danceScene.c` … (33 个) | 33 文件 |
- | 8 | `0x8105119A`–`0x81057FCC` | **dance_ui** | `result.c` `shutdown.c` | 2 文件 |
- | 9 | `0x8105C3F6`–`0x81089xxx` | **event** | `evtFacialMotionEditor.c` `evtFacialMotion.c` `evtSequenceMotion.c` `evtCutin.c` `evtScript.c` … | 14 文件 |
- | 10 | `0x810895A4`–`0x8109976E` | **game** | `datGame.c` `datItem.c` `datPC.c` `datTable.c` `fieldHandler.c` `game.c` `gameDLC.c` `unitHandler.c` | 8 文件 |
- | 11 | `0x810997B2`–`0x810B5424` | **mainmenu/collection** | `collectionMain.c` `collectionCharacter.c` `collectionModeler.c` `collectionPlate.c` `collectionScore.c` `handlerLoader.c` | |
- | 12 | `0x810B56C6`–`0x810E6260` | **mainmenu/common + commu + config** | `costume.c` `wipe.c` `starBG.c` `systemWindow.c` `commuMain*.c` `event_field*.c` `configMenu.c` `crossSave.c` | |
- | 13 | `0x810E6324`–`0x810F839E` | **mainmenu 主体 / P3 侧** | `helpwindow.c` `keyHelp.c` `mainMenu.c` `mainMenu2_P5.c` `mainMenuBG_P3.c` `mainMenu_P3.c` | |
- | 14 | `0x810FD1F0`–`0x811475E0` | **mainmenu/notify + pause + tutorial + preview** | `notifywindow.c` `pauseMenu*.c` `previewmodel.c` `tutorialMain*.c` `playrecord.c` `mainMenuWipe.c` | |
- | 15 | `0x811199F6`–`0x81133190` | ★ **musical_note_sequencer（谱面/判定）** | `mnsDebug.c` `mnsHandle.c` `mnsOption*.c` **`mnsProcess.c`** `mnsReplay.c` | 8 文件 |
- | 16 | `0x81133190`–`0x81150D5A` | ★ **music_select（选歌）** | `musicData.c` `musicFormat.c` `musicSelectBG.c` **`musicSelectMain.c`** `musicSelectNotify.c` `musicSelectPractice.c` | 6 文件 |
- | 17 | `0x8114EF90`–`0x81153522` | **polygon** | `cursorAnim.c` `polygon_common.c` `star.c` | 3 文件 |
- | 18 | `0x81153A18`–`0x811593F8` | **movie（CRI 影片）** | `decodeMemory.c` `sofdecMovie_psp2.c` `appMovie.c` `appMoviePlayer.c` `movie_app_movie.c` | |
- | 19 | `0x81159592`–`0x81158D9A` | app_system/utility（interleave） | `counterEdit.c` `flgedt.c` `appCalculator.c` | |

---

## `ARCH_260_LINE_C_CLOSED_FROZEN_PRESENT_BUG.md`

**标题**：ARCH_260 — ★★★ 线 C 主案闭合：真正的原因是「上屏画面被冻结在首帧」 ｜ 1. ★★ 根因：`showTex` 只在首帧被写过一次 ｜ 1.1 它为什么能藏这么久 ｜ 1.2 它解释了此前全部异常观察 ｜ 2. 修法 ｜ 3. ★ 修复后的验证：日志与画面**终于一致** ｜ 3.1 实时日志（`e.P.mvp` 直接算的 NDC 包围盒） ｜ 3.2 同刻截图 ｜ 3.3 全量回归（`DEVICE_VERIFY_ALL_PASS`，13 标记） ｜ 4. ★ 线 C 主案：闭合 ｜ 4.1 本轮追通的代码链（补充 `ARCH_259`） ｜ 5. 仍未确定（如实标注） ｜ 6. 复现 ｜ 7. 对既有文档的影响

**发现/结论**（18 条，最多列 40）：

- > `ARCH_258` §4 的错误结论、以及所有"角色很小/贴底/朝向怪"的观察。
- main.cpp:541   glBindTexture(GL_TEXTURE_2D, e.showTex); ; ★ 唯一写入点（在 selftestOnce 内）
- main.cpp:1157  if (!g.selftestDone) selftestOnce(g);    ; ★ 只调用一次
- **⇒ `showTex` 在首帧写入一次后再也没被更新，而每帧上屏画的都是它。**
- **⇒ `ARCH_258` §4「角色没有舞台定位」的结论，是这个 bug 造成的假象。**
- // ★ ARCH_260
- `NDCy` 跨度 `1.09`（NDC 总高 2）⇒ **角色占画面高度约 55%，垂直居中**。
- **⇒ 从"日志 55% / 画面 5%"的对立，变成"两边都是 55%"。**
- | 问题 | 答案 | 证据 |
- | 坐标来源在哪？ | **不在资产里**（与 9 类排除结论一致）——引擎把角色放在原点，**是固定行为，不是缺数据** | 同上 |
- **⇒ 单舞者没有"缺失的舞台槽位"。之前的"无可靠路径"印象，一半来自资产侧确实没有坐标，
- 0x81043B80  mov  sb, r0                  ; sb = arg0 = 运行时表项
- 0x81043B98  add.w r0, sb, #0x40
- 0x81043B9C  str  r0, [sp, #0x48]         ; 描述符源 = 表项 + 0x40
- 0x81043D84  bl   #0x81047e4c             ; arg0
- 0x81047EBE  str  r1, [sp, #0x24]         ; 描述符 = fp+0x14（0x1F0 字节堆副本）
- | 舞者的**朝向**来源 | 🟡 视觉上正确（来自舞蹈 GAP 的根骨骼旋转），但未单独测量确认 |
- * `ARCH_162`/`ARCH_164`/`ARCH_167` 的资产侧排除结论**仍然有效**

---

## `ARCH_261_HEAD_BODY_DETACH_DIAGNOSIS.md`

**标题**：ARCH_261 — 头身分离：已复现并收窄范围，**未修** ｜ 1. 已观测（可复现） ｜ 1.1 缺陷是**间歇性**的 ｜ 1.2 相机有时**并不对着角色** ｜ 1.3 服装 GAP 里**没有**头部骨骼 ｜ 1.4 `p5d_gap.h` **已经**处理 kt=26/27/28（`ARCH_162` 的说法已过时） ｜ 2. 嫌疑（**未验证，勿当结论**） ｜ 2.1 ★ GAP 的**平移被丢弃** ｜ 2.2 分部件装配时骨骼**按名字还是按索引**匹配 ｜ 3. 下一步（按性价比） ｜ 4. 本轮新增工具 ｜ 5. 与既有文档的关系

**发现/结论**（7 条，最多列 40）：

- | 时刻 | 观察 | 证据文件 |
- ⇒ **不是静态绑定问题**（否则任何时刻都错），而是**动画/姿态**相关。
- ⇒ 这解释了为什么"随手截一张图"经常抓到角色很小/不在画面里。为避免再犯，
- ⇒ **头部动画在身体 GAP（`pc201_018.GAP`）里**，服装 GAP 只叠加布料。
- ⇒ `ARCH_162` §2 说的「跳过 26/27」已被 `ARCH_163` 修掉。**本轮不再把这条当嫌疑。**
- （后两个是本轮为定位缺陷而做，前者用于排除系统 UI 污染 —— `ARCH_259` 踩过一次。）
- - `ARCH_260` 的"取景正确"结论**不受影响** —— 那说的是**整体构图**（角色是否在画面内、多大），

---

## `ARCH_262_HEAD_DETACH_IS_ARCH158.md`

**标题**：ARCH_262 — 头身分离 = `ARCH_158` 已知残留；嫌疑 A 排除，问题收敛到「面部是否含脖子」 ｜ 1. ★ 嫌疑 A 被排除（有数据） ｜ 2. ★★ 三个部件的骨骼数差异巨大（真机实测） ｜ 3. ★ 这不是新 bug：`ARCH_158` 已记录，且残留正好就是用户看到的现象 ｜ 3.1 与用户报告的对应 ｜ 3.2 为什么之前几轮没暴露 ｜ 4. 唯一剩下的判定问题 ｜ 5. 本轮代码改动（仅诊断，无行为变更） ｜ 6. 新增工具 ｜ 7. 与既有文档的关系

**发现/结论**（10 条，最多列 40）：

- > 并排除了一个嫌疑、给出唯一剩下的判定问题。**未改渲染代码**。
- ⇒ 「丢弃 GAP 平移」**不可能**造成头身分离。**嫌疑 A 排除。**
- （同时确认：`head`/`neck` 的动画在**身体 GAP** 里；服装 GAP `pc201_018_10.GAP` 的 108 个
- > ★ `ARCH_158`：面部（`pc20X_f1.GMD`）**自身没有骨骼**（`skeletonReady=false`），
- > - 已排除「四元数解析错误」：body GAP 的 66 个旋转层，|q| 最大偏离仅 0.0012
- | "可能是骨骼没有接好" | 机制确实是"面部无骨骼、整体套 head 骨"；**旋转解析已排除** |
- **⇒ 用户报告的现象 = `ARCH_158` 的已知残留，不是新引入的 bug。**
- - `ARCH_261` §2.1（嫌疑 A：GAP 平移被丢弃）→ **已排除**（本文 §1）。
- - `ARCH_261` §2.2（跨部件骨骼匹配）→ **部分证实方向正确**，但真正机制是 `ARCH_158` 的
- 「面部无骨骼 ⇒ 整体套 head 骨」，不是简单的索引错配。

---

## `ARCH_263_FACE_HAS_NO_NECK.md`

**标题**：ARCH_263 — 判定结果：**面部网格不含脖子**（`ARCH_158` 的疑问有答案了） ｜ 1. 工具 ｜ 2. ★ 面部：**14 个网格，全部无蒙皮，几何不含脖子** ｜ 2.1 对 `ARCH_158` 数值的更正 ｜ 3. 身体：上探到 y=166，肩/领口在 y 140–146 ｜ 4. 因此仍未定的那一件事 ｜ 5. 本轮结论汇总 ｜ 6. 复现

**发现/结论**（11 条，最多列 40）：

- * **`骨骼记录=0`** —— 与真机实测的 `[part] 2 骨骼=0` 一致（`ARCH_262` §2）。
- **⇒ 面部几何在 y=153.90 就结束了，往下什么都没有 —— 面部不含脖子。**
- `ARCH_158` 记「面部网格最低点约在 y≈150」。**实测是 153.90**（量程数据在上）。
- 结论不变，数值更正。缺口因此是 `153.90 − 147.28 ≈ **6.6 单位**`。
- 1. **主体 `body_mt` 上探到 y=166**（高于面部最低点 153.90）⇒ 从**包围盒**看，身体在
- | 该段**有**几何（身体侧是连续到 166 的） | 问题在**面部的放置**：`g_headSkin` 用 head 骨矩阵整体套面部，若 head 骨世界位置在动画中与颈部网格脱节，就会出现裂缝 ⇒ 该调 `g_headSkin` 的应用方式（或改用 neck 骨） |
- | 该段**没有**几何（身体在 154 就断了，166 是手臂） | **几何确实缺失** ⇒ 需要另找脖子网格资源，或接受"该服装领口高于下巴"的处理 |
- 不用动渲染器。若该段有密集三角形 ⇒ 第一种；若几乎为零 ⇒ 第二种。
- | 问题 | 答案 | 证据 |
- | 面部含脖子吗？ | **不含**。面部最低 y=153.90，`neck` 骨 y=147.28 ⇒ 缺口 **≈6.6 单位** | §2（14 网格量程） |
- | 面部有骨骼吗？ | **没有**（`骨骼记录=0`，14 网格全 `skinned=0`）⇒ 完全依赖 `g_headSkin` 整体变换 | §2 |

---

## `ARCH_264_NECK_GEOMETRY_EXISTS.md`

**标题**：ARCH_264 — 判定完成：**脖子几何存在**，问题在面部的放置方式（不是缺资源） ｜ 1. 工具补强 ｜ 2. ★ 身体 `pc201_10.GMD` 的 y 分箱（每 1 单位高度带的三角形数） ｜ 3. 因此：**不是缺脖子几何** ｜ 4. 机制（现可确定的部分） ｜ 4.1 未确定的一项（修法取决于它） ｜ 5. 下一步（两件，都不需要动渲染语义猜测） ｜ 6. 本轮结论汇总 ｜ 7. 复现

**发现/结论**（10 条，最多列 40）：

- **结论：y=146..155 全程每单位带有 100–360 个三角形，平滑收窄至 y≈164 后归零。
- ⇒ 面部底部（153.90）**覆盖在身体颈部几何之上**，两者在绑定空间是**重叠**的（不是相接、更不是留缝）。
- | y∈[146,154] 没有几何 ⇒ 缺资源，需另找脖子网格 | ❌ **不成立**（§2：密度 108–356） |
- | 该段有几何 ⇒ 问题在面部的放置 | ✅ **成立** |
- **⇒ `ARCH_158` 提出的两种可能里，第一种（"脖子应由别的资源提供"）被排除；
- ⇒ **刚性整体变换 vs 平滑蒙皮**：当 head 骨相对 neck 有较大旋转时，
- | **head 骨局部空间** | `headWorld` | `world * inverseBind`（蒙皮矩阵）**不等于** `headWorld` ⇒ 会多乘一个逆绑定，**偏位** |
- 若在 t=0 面部就相对身体偏了，就是上表第一种 ⇒ 现有多乘了逆绑定。
- 若 t=0 就偏 ⇒ §4.1 第一种 ⇒ 改 `g_headSkin` 的构造（去掉逆绑定）。
- 若 t=0 完全重合 ⇒ 是 §4 的"刚性 vs 平滑"固有裂缝 ⇒ 需要按顶点权重插值，

---

## `ARCH_265_FACE_IN_MODEL_BIND_SPACE.md`

**标题**：ARCH_265 — §4.1 判定完成：面部在**模型绑定空间**，现有变换正确；残留机制确认为「刚性 vs 加权」 ｜ 1. 工具补强：打印骨骼的绑定世界位置 ｜ 2. ★ 结果（`pc201_10.GMD`） ｜ 2.1 判定 ｜ 3. 于是残留机制确定为：**刚性整体变换 vs 加权平滑蒙皮** ｜ 4. 可行的修法（**尚未实现**） ｜ 4.1 我不打算直接实现的原因 ｜ 5. 复现 ｜ 6. 结论汇总

**发现/结论**（8 条，最多列 40）：

- > **答案：模型绑定空间 ⇒ 现有 `world × inverseBind` 就是对的，不存在多乘/少乘逆绑定。**
- 打印 `boneW` 的**第 4 行**（行主序平移，第 4 列全 0 已确认）与第 4 列。
- **实测 153.90–171.15 ⇒ 模型绑定空间。**
- ⇒ **现有的 `skin = world × inverseBind` 是正确变换**；
- `ARCH_264` §4.1 里"多乘了一个 `inverseBindHead` ⇒ 偏位"的候选**排除**。
- ⇒ 面部被**单个 head 骨的矩阵整体刚性变换**；
- 而身体的对应表面只**部分**跟 head 走 ⇒ 交界处裂开。
- `g_headSkin` 的缓存方式有关，需要先读那段代码确认能拿到 neck 骨矩阵。

---

## `ARCH_266_NECK_HEAD_BLEND_IMPLEMENTED.md`

**标题**：ARCH_266 — 实现修复：面部按绑定 y 在 neck/head 之间混合 ｜ 1. 前置确认：neck 骨矩阵可得 ｜ 2. 改动（`src/assets/p5d_render_core.h`） ｜ 2.1 新增缓存 ｜ 2.2 采集时一并缓存 neck 与两骨的**绑定世界 y** ｜ 2.3 应用点：按顶点绑定 y 混合 ｜ 3. 验证 ｜ 3.1 ⚠ 证据强度的诚实说明 ｜ 4. 新增/修改的产物 ｜ 5. 下一步

**发现/结论**（7 条，最多列 40）：

- ⇒ 取 neck 骨只需把名字换成 `"neck"`，**无需任何新数据通路**。
- 位置在**部件循环之前** ⇒ 身体部件（第 0 个）先缓存，面部部件后使用。**语义正确。**
- **数值**（`ARCH_265` §2 实测）：`g_neckBindY = 147.28`、`g_headBindY = 157.78`
- ⇒ 面部底边 `y=153.90` 取 `tw ≈ 0.63`（主要跟 head、保留部分 neck）；
- ⇒ 改动**只影响面部最低那约 9.9 单位**，其余部分逐位不变。
- * ✅ 可以断言：**加入了颈部混合，且改动后画面在多个时刻都未见断裂**
- * ❌ **不能**断言："已证明改前的断裂就是由这个原因导致、且已被完全消除"

---

## `ARCH_267_HAIR_HAT_OFFSET.md`

**标题**：ARCH_267 — 面部已对齐后暴露的第二个问题：头发/帽子偏位（**未修**） ｜ 1. 先确定责任归属：**不是我上一轮的改动** ｜ 2. 逐部件诊断（真机实测） ｜ 3. 头发/帽子的结构（`gmdprobe`） ｜ 4. 嫌疑（**未验证**） ｜ 5. 下一步（一条即可判定） ｜ 6. 本轮改动 ｜ 7. 与既有文档的关系

**发现/结论**（9 条，最多列 40）：

- > 本轮做诊断，**确认了这不是我上一轮改动引入的**，并给出具体嫌疑与下一步。
- ⇒ 头发/帽子的偏位是**既有的、独立的问题**，只是因为面部此前偏着、视觉上没这么显眼。
- ⇒ **头发不是"姿态没算出来"**：11 根骨骼里 **10 根被驱动**。
- ⇒ 帽子和头发**都在这个 11 骨骼的部件里**，两件一起偏 ⇒ 是**整个部件**的问题。
- 会把 `head` 当作根 ⇒ 得到 `head` 的世界位置 ≈ 其**局部值**，而不是继承 `neck` 的世界位置。
- ⇒ 头发/帽子整体相对头骨偏位 —— 与「两件一起偏」的形态吻合。
- * 若**不同**（头发的明显偏离 157.78 那个高度） ⇒ §4 成立：
- * 若**相同** ⇒ 不是层级问题，改查该部件的**蒙皮权重/骨骼索引映射**
- * `ARCH_258`/`ARCH_260` 的"取景正确"结论 → 不受影响（那是整体构图）。

---

## `ARCH_268_HAIR_HEAD_TRANSFORM_MISMATCH.md`

**标题**：ARCH_268 — 头发/帽子偏位：**已判定**为「子部件的 head 骨世界变换与主骨架不一致」 ｜ 1. 判定方法（为什么这条日志能定性） ｜ 2. ★ 实测结果（真机，t=0） ｜ 3. 结论与根因方向 ｜ 4. 修法方向（**未实现**） ｜ 5. 本轮改动 ｜ 6. 复现 ｜ 7. 目前三个"头相关"问题各自的状态

**发现/结论**（5 条，最多列 40）：

- > 承接 `ARCH_267`。本轮用**一条日志**判定完成，证据是两个数量级的差异。
- （`gmdprobe` 实测：身体与头发的 `boneW head row4 = (0.00, 157.78, -1.25)`，逐位相同）
- ⇒ `invBind` 相同 ⇒ **`S` 不同 ⟺ `W`（世界变换）不同**。
- ⇒ **头发部件算出的 `head` 骨世界变换与身体的不一致**，
- | 脸与身体断开 | 面部无骨骼、刚性套 head 骨，底边落在 neck 影响区 | ✅ 已修（`ARCH_266`），用户确认脸正常 |

---

## `ARCH_269_HAIR_SKELETON_FRAGMENT_ROOT_CAUSE.md`

**标题**：ARCH_269 — 头发/帽子偏位：**根因坐实**（子部件骨架是碎片，根指向主骨架） ｜ 1. ★ 坐实：头发骨架的祖先链缺失且父索引越界 ｜ 头发 `hair/pc201_h10.GMD`（11 骨） ｜ 身体 `pc201_10.GMD`（182 骨）—— 对照 ｜ 2. 机制（现在完全清楚） ｜ 3. 修法规格（**未实现**） ｜ 3.1 为什么我不在这轮动手 ｜ 4. 下一轮的最短路径 ｜ 5. 本轮改动 ｜ 6. 复现 ｜ 7. 四个"头相关"问题的当前状态

**发现/结论**（5 条，最多列 40）：

- [  7] RootNode       parent= 17 -             ← ★ 17 越界（本文件只有 11 根骨）
- W_RootNode 的父（17）在本文件里不存在 ⇒ 只能当作"根"处理，
- ⇒ 头发只乘上自己那条**碎片链**，**完全没有身体的动作**。
- **这与实测完全吻合**：`ARCH_268` 量到两侧 `head` 骨的蒙皮矩阵位移相差两个数量级
- ⇒ 这也解释了**为什么"脸对了、头发还偏"**：面部走的是"无骨骼 + 套 head 骨矩阵"

---

## `ARCH_26_rhythm_judge.md`

**标题**：ARCH_26 — 节奏判定核心（`musical_note_sequencer` / `mnsProcess.c`） ｜ 1. 本模块的运行职责（一句话） ｜ 2. 结构 / 流程图（核心） ｜ 2.1 模块区间总览（本次用「源文件路径串的引用点扫描」实测得到） ｜ 2.2 `mns` 序列的生命周期（实测） ｜ 2.3 `mns` step 的 5 状态（本次新解） ｜ 2.4 每帧判定主循环 `0x81124FE2`（实测） ｜ 2.5 判定主体 `0x81122A80` —— **输入位掩码 → 判定**（实测） ｜ 2.5.1 按键位掩码的构造（这是「按键 ↔ 音符轨道」的桥） ｜ 2.5.2 判定等级的分档阈值（**本次新解，P0 级**） ｜ 2.5.3 判定记录的写入（实测：结构清晰、每帧一次） ｜ 2.5.4 判定结果的分档与去抖（实测） ｜ 2.6 关键时间公式（**本次定向解出，P0 级**） ｜ 2.7 后处理 `0x81124C82`（分数/显示平滑，实测） ｜ 3. 函数地址表 ｜ 3.1 `mnsProcess.c` 主体（`0x81121598..0x8112638E`）—— 本次函数边界扫描结果 ｜ 3.2 事件流 / 时间换算（`mnsHandle.c`，`0x8111BExx`） ｜ 3.3 时间源（`danceMusic.c`，`0x81045xxx`） ｜ 3.4 判定子系统（`mnsHandle.c` 的判定半边，`0x8111A2A6` 起） ｜ 3.5 输入 ｜ 4. 数据结构字段表 ｜ 4.1 `mns` 宿主对象（`0x440` 字节，构造于 `0x811215F4`） ｜ 4.2 判定记录的 0x30 字节结构（`0x81122CE2` 分配，逐轨道一次） ｜ 4.3 谱面事件流（**闭合 ARCH_20 的 P0**）

**发现/结论**（458 条，最多列 40）：

- > 分析对象：`C:\dev\p5d_build\eboot.bin`（ARM32 Thumb-2，基址 `0x81000000`，映射 `文件偏移 = VA - 0x81000000 + 0x10E0`）
- > 反汇编：`py C:\dev\p5d_build\_dis3.py <VA> <字节数>`（严格单调、不重同步）
- > `_a26_dance.txt` `_a26_allmns.py` `_a26_modxref.py`，以及 `_a26_d1/_a26_iter/_a26_dm/_a26_j1.txt` 等反汇编转储。
- > **凡是推测一律标「推测」；凡是有原始字节/反汇编行支撑的结论都给出 VA + 指令原文。**
- 用 `_a26_modxref.py` 对 `0x81000000..0x812F0000` 全量线性反汇编 1,069,409 条指令，
- | `musical_note_sequencer\mnsHandle.c` | `0x81302E50` | `0x81119DD4 .. 0x8111C100` | 谱面句柄/音符时间换算 |
- | `musical_note_sequencer\mnsOption.c` | `0x81302EB8` | `0x8111C10E .. 0x8111D92A` | 音符窗口/选项表 |
- | **`musical_note_sequencer\mnsProcess.c`** | **`0x8130333C`** | **`0x81121598 .. 0x8112638E`** | ★ 本文件主体 |
- | `musical_note_sequencer\mnsReplay.c` | `0x81303B00` | `0x811326A4 .. 0x81133076` | 回放记录 |
- | `dance\danceTimeAdjuster.c` | `0x812F1890` | `0x81049BCE`, `0x81049C52` | 节拍对齐 |
- | `dance\danceMusic.c` | `0x812F0F20` | **`0x810454DA`**（唯一） | 播放时间源 |
- | `dance\danceMain.c` | `0x812F0D7C` | `0x81042538 .. 0x81044BB4` | 对局主流程 |
- | `dance\danceFormat.c` | `0x812F08B8` | `0x8103FDFE .. 0x81041346` | 表格式化 |
- | `dance\danceResident.c` | `0x812F1404` | `0x81046C94 .. 0x810472E0+` | 单曲资源预载 |
- > **要点**：`mnsProcess.c` 正是 `ARCH_22` 标为「完全未剖析」的 `0x81121598..0x8112638E`，本次已完整反汇编（6369 条指令，0 条无法解码）。
- > 但**判定所需的另一半不在 mnsProcess.c**：它在 `mnsHandle.c`（`0x8111BExx`，事件流迭代器）与 `danceMusic.c`（`0x810455FC`，时间源）。
- mnsProcess 构造器 0x811215F4   (r0 = musicID, r1 = 参数结构体)
- ├─ 0x8115D79C(0x440)                      分配 0x440 字节宿主对象
- ├─ 0x8115D36A(...) tag = mnsProcess.c:0xF03
- ├─ memset(对象, 0, 0x440)                  0x812B2260 = memset
- ├─ 0x8117BD50(参数块, 0xD)                 取"音符窗口/难度"表
- ├─ 0x8101612C(0xC,0) / 0x81016378         建列表节点，挂到 对象+0x20C
- ├─ 0x81121060(对象)                        子系统初始化
- └─ 0x8116F7CA("mns", period=0xC8, step=0x81126155, cleanup=0x81126407)
- ↑ 名字串来自 0x81303404 = 'mns'（movw #0x3404 / movt #0x8130 @0x81121732,0x81121736）
- ↑ 两个函数指针字面量：0x81126155 (@0x81121718) 与 0x81126407 (@0x81121722)
- `mns` 序列节点每帧调用 step **`0x81126155`**（真实地址；`_dis3` 输出为 `0x81126154`）。
- 8112616A  bl     #0x8116f8a8        ; r4 = seqNodeWork()
- 81126174  bhi.w  #0x811263c8        ; state>4 → 默认（返回 0 保活）
- | 0 | `0x81126188` | 载入谱面（见下） |
- | 1 | `0x81126188` | 同 state 0（表项相同，说明 0/1 合并） |
- | 2 | `0x81126264` | 播放/推进（检查 SE 是否就绪） |
- | 3 | `0x81126290` | 后半段就绪检查 → state 4 |
- | 4 | `0x8112632C` | 创建音符判定子系统 `0x8111A2A6` → 完成 |
- **state 0/1 = 载入谱面（实测，含 ARCH_20 的 P0 谱面路径）**：
- 81126186  movs   r0, #2                    ; ★ 下一个 state = 2
- 81126190  ldr.w  r2, [r4, #0x1dc]          ; ★ obj+0x1DC = musicID  ← ARCH_28 的同一锚点
- 81126194  ldr.w  r3, [r4, #0x22c]          ; ★ obj+0x22C = 难度/槽位
- 81126198  beq    #0x811261cc               ; flags&4==0 → 用普通模板
- 8112619A  movw   r1, #0x3398 / movt #0x8130  ; = 0x81303398 "dance/mns/mns%03d_%ds.bin"

---

## `ARCH_270_HAIR_ROOT_CAUSE_EXACT_LINE.md`

**标题**：ARCH_270 — 头发偏位：根因**精确到行**，修法**完全规格化**（未实现） ｜ 1. ★ 根因精确到行 ｜ 2. 为什么身体对、头发错 ｜ 3. 修法最后一块拼图：虚拟节点**带名字** ｜ 4. 修法规格（完整，未实现） ｜ 4.1 `Pose` 需要暴露世界矩阵 ｜ 4.2 `make_pose` 增加可选的主骨架入参 ｜ 4.3 调用侧 ｜ 4.4 为什么不在这轮实现 ｜ 5. 验证判据（改完后必须过的两条） ｜ 6. 本轮改动 ｜ 7. 四个"头相关"问题的状态 ｜ 8. 复现（现有证据）

**发现/结论**（9 条，最多列 40）：

- if (pi >= N)      m4_mul(Lp, &f.nodeWorldV[(size_t)(pi-N)*16], W);   // ★ 父是"虚拟节点"
- | **身体** | `Hips`（`parent=190`，`N=182` ⇒ 虚拟节点 8） | 固定 | `Hips` **自己**是 GAP 的动画根，`L_Hips` 带整体动作 ⇒ `W_Hips = L_Hips(动画) × 固定` ⇒ **身体跟着动** ✅ |
- | **头发** | `RootNode`（`parent=17`，`N=11` ⇒ 虚拟节点 6） | 固定 | `RootNode` **没有通道** ⇒ `L_RootNode` = 绑定局部 ⇒ **`W_RootNode` 恒定**；于是 `W_head = L_head(动画) × 固定` ⇒ **头会转、基点不动** ❌ |
- ⇒ **头发的 `head` 只跟随旋转、不跟随位移**，与 `ARCH_268` 实测
- **⇒ 这也解释了为什么"脸对了、头发还偏"**：
- ⇒ **`nodeWorldV[k]` 就是由 `f.nodeRecs[k]` 的 TRS 建的**，
- 头发：`RootNode.parent = 17`，`N = 11` ⇒ 虚拟节点 `6` ⇒ `f.nodeRecs[6].name`。
- ⇒ **"按名字跨部件链接"所需的键是现成的。**
- // ★ ARCH_270：子部件的虚拟父若在主骨架里有**同名骨**，改用主骨架的**动画世界**

---

## `ARCH_271_HAIR_FIX_VERIFIED_CAPE_ISSUE.md`

**标题**：ARCH_271 — `ARCH_270` 的跨部件修复**已验证通过**；新症状「肩披风一点被钉住」的初步排查 ｜ 1. ★ 修复已验证：两侧 `head` 骨蒙皮矩阵**逐位相同** ｜ 1.1 实现内容（`ARCH_270` §4 的四步） ｜ 1.2 一个诊断陷阱（我自己踩的） ｜ 1.3 全量回归 ｜ 2. 新症状：「肩披风有一个点被固定」 ｜ 2.1 先排除的猜测：**不是"某根骨没有动画"** ｜ 2.2 与本文 §1 修复的关系 ｜ 2.3 仍未排除的方向（**未验证**） ｜ 3. 本轮改动 ｜ 4. 五个"头/披风相关"问题的状态 ｜ 5. 诚实的边界

**发现/结论**（12 条，最多列 40）：

- > ② 用户新报的「肩披风有一个点被固定」——本轮**排除了一个猜测**，根因仍未定。
- `ARCH_270` §5 定下的判据 —— 两侧 `S.head.t` 应趋于一致。实测：
- **逐位相同。** ⇒ 头发的 `head` 骨现在与身体走**同一条动画链**，
- **教训**：**诊断路径必须与渲染路径同源**，否则诊断本身就是错的证据来源。
- ⇒ **身体的每根有名字的骨骼都有通道覆盖。**
- 「某骨停在绑定位置 ⇒ 权重到它的顶点被钉住」这条**被排除**。
- **part 0 是主骨架，永远不接收 `mainF`** ⇒ §1 的改动**不可能**影响披风。
- | **多通道同名** | 同一骨有 kt=31/32 之外还有 kt=26，适配器只取其一（`if (!Lp) … else if (Lh)`）⇒ 可能漏掉一半 |
- | `src/assets/p5d_render_core.h` | ★ 跨部件骨架链接（`Pose::W`、`make_pose` 入参、虚拟父按名链接、循环先主后子） |
- | **肩披风一点被钉住** | 未定；已排除"缺通道" | 🔴 **本轮排查中** |
- **尚未**用截图肉眼确认头发/帽子在画面上已归位 —— 截图已抓（`out\framed_zoom.png`）
- * §2 只完成了**排除**，**根因未定**，**没有改代码**。

---

## `ARCH_272_CAPE_SECOND_HYPOTHESIS_ELIMINATED.md`

**标题**：ARCH_272 — 披风「一点被钉住」：第二个假设也排除；两个主假设均已否定 ｜ 1. 判定结果 ｜ 2. 披风问题：已排除的两个假设 ｜ 3. 剩下的候选（**均未验证**） ｜ 3.1 `kt=26/27` 的**位置被丢弃**（`ARCH_163` 遗留） ｜ 3.2 同一骨有多条层，适配器**只取一条** ｜ 4. 本轮改动 ｜ 5. 诚实的边界 ｜ 6. 五个问题的状态

**发现/结论**（6 条，最多列 40）：

- ⇒ 「披风某骨挂在部件内虚拟父下 ⇒ 被钉住」**不成立**。假设排除。
- | # | 假设 | 排除依据 |
- **两条主假设都被否定 ⇒ 我对这个症状目前没有站得住的解释。**
- ⇒ 权重到它的顶点看着"被钉住"。
- * 我对披风这个症状**目前没有站得住的解释**；两条主假设都已排除。
- | **肩披风一点被钉住** | **未定**（两条主假设已排除） | 🔴 下一轮从 §3.1 重新取证 |

---

## `ARCH_273_CAPE_EMBLEM_ROOT_CAUSE_FIXED.md`

**标题**：ARCH_273 — ★ 披风「一点被钉住」根因找到并修复：`make_pose` 丢弃了 kt=26 的**动画平移** ｜ 1. ★ 取证：`cape_emblem` 是唯一的异类 ｜ 2. ★ 根因：`make_pose` 把采样到的平移丢掉了 ｜ 3. 修法（最小、定向） ｜ 4. 验证 ｜ 4.1 诚实的边界 ｜ 5. 本轮改动 ｜ 6. 六个问题的当前状态 ｜ 7. 复现

**发现/结论**（8 条，最多列 40）：

- > `ARCH_272` 排除了两条假设后，本轮按 §3.1 重新取证，**一次命中**。
- → **幅度 6.58 / 5.23 / 7.56 单位**          ← ★ 大幅**动画平移**
- ⇒ 与用户描述完全吻合：**披风主体在动，但徽章那个点被钉住。**
- 对绝大多数骨这没问题（它们的平移本就恒定，例如头/颈 `ARCH_262` §1 实测幅度 0），
- if (c->srcKT >= 26 && c->srcKT <= 28) {          // ★ ARCH_273
- **无法据此确认徽章是否已随披风运动**。
- 但**"徽章不再被钉住"这一点我还没有可靠的视觉/数值证据**。
- | **肩披风一点被钉住** | **kt=26 通道的动画平移被丢弃** | ✅ **根因已确证并修复**；⚠ 视觉验证待补（§4.1） |

---

## `ARCH_274.md`

**标题**：ARCH_274 —— 无名骨（被钉住的肩披风两点）的通道匹配 ｜ 0. 症状 ｜ 1. 改动 ｜ 2. 交叉验证：一个**失败**的判别测试（重要） ｜ 3. 披风窗口的**结构性**证据（成立） ｜ 4. 顺带证伪：`ARCH_131` 的「paletteIdx = 节点索引」 ｜ 5. 回归状态 ｜ 6. 未决 / 待办

**发现/结论**（14 条，最多列 40）：

- `nm` 由 `f.nodeRecs[f.boneRec[i]].name` 取得；无名骨 `boneRec[i] < 0` ⇒ `nm == ""`
- ⇒ **整个查通道分支被跳过** ⇒ 这两根骨永远停在绑定姿势 ⇒ 对应顶点被钉住。
- 1. `p5d_gap.h`：`struct Chan` 增加 `int tid = -1;`（`★ ARCH_274`），
- **匹配不到 ⇒ nullptr ⇒ 与改动前逐字一致**（改动本身安全）。
- ⇒ `paletteIdx[k] == targetId` **不是全局恒等式**。
- ⇒ 「unnamed=14 matched=14」**不构成正确性证据**：`paletteIdx` 是到 0..187 的双射，
- ⇒ `[110] → tid 175`（= `R_S_longcoat_02`）、`[115] → tid 168`（= `R_S_B_longcoat_01`）。
- 这是**结构性**证据（序列稠密），不是双射的必然结果。
- ⇒ **`paletteIdx` 不是 `nodeRecs` 的索引**；`ARCH_131`「paletteIdx 是 N×u16 节点索引表
- - `PLAY_SELFTEST MATCH` 摘要 `871d874fe1b44ac4` **与改动前逐字节相同** ⇒ 未触碰玩法/谱面路径。
- - **视觉确认**：`[110]`/`[115]` 此前**完全没有动画**，现已改为采样通道，该区域渲染**必然改变**。
- 若 tid 正确 ⇒ 披风修复；若错误 ⇒ 披风会跟随不相干的父骨摆动，**肉眼可判别**。
- 这是当前唯一未完成的环节，需要用户在设备上确认。
- 而 GMD 侧有 14 根骨无名 ⇒ 两者名字表并不一一对应，这正是 64 处 mismatch 的可能来源。

---

## `ARCH_274_TWO_UNNAMED_BONES_PINNED.md`

**标题**：ARCH_274 — 肩披风「2 点被定住」：找到正是**2 根无名骨骼**（数量与位置都对上） ｜ 1. ★ 关键数字：身体 182 骨，168 被驱动 ⇒ 14 根没动 ｜ 2. ★ 那 10 根无名骨里，**正好 2 根**挂在肩部长外套下 ｜ 3. 机制：无名骨**根本不会去查通道** ｜ 4. 修法方向（**未实现**） ｜ 5. 本轮改动 ｜ 6. 六个问题的状态 ｜ 7. 复现

**发现/结论**（8 条，最多列 40）：

- [110] -   parent=109 R_S_longcoat_01     ← ★ 肩部长外套
- [115] -   parent=114 R_S_B_longcoat_00   ← ★ 肩部长外套
- ⇒ **数量（2）与位置（肩披风）与用户描述完全吻合。**
- if (nm[0] && !forceBind) {          // ★ 无名字 ⇒ nm[0]==0 ⇒ 整段跳过
- **`boneRec[i] < 0` ⇒ `nm = ""` ⇒ 通道查找整段跳过** ⇒ 该骨：
- * 世界变换 = `Lp(绑定) × W_parent(动画)` ⇒ **只能刚性跟随父骨**。
- ⇒ 与 `ARCH_271` 排除的两种假设都不同：**不是"没通道"，而是"有通道但骨没有名字、查不到"。**
- ⇒ 对**没有名字**的骨，改用**索引**匹配：

---

## `ARCH_275.md`

**标题**：ARCH_275 —— targetId 是**文件内局部**编号；祖先定文件 + 歧义实测 ｜ 1. 上一步（ARCH_274）的做法有个结构性错误 ｜ 2. 改动 ｜ 3. 交叉验证：**anyFile = firstHit = 104** ｜ 4. ★ 祖先规则**并没有**改变被钉住的两根骨 ｜ 5. 祖先规则**真正**修掉的是 4 个歧义点 ｜ 6. 回归 ｜ 7. 未决 —— 下一步必须换成**顶点级**判据

**发现/结论**（8 条，最多列 40）：

- ARCH_274 用 `findByTid(g, tid)` **跨文件首命中**。但实测：body 的 3 个 GAP 文件是
- `[nameTid] 0 cape window` 证据：
- ⇒ 64 处 mismatch **不是**取错文件造成的（任一文件都不等），
- ⇒ `paletteIdx[k] == targetId` **仍不是全局恒等式**。
- `f0:-` ⇒ f0 里**根本没有** tid 175/168 ⇒ 旧的跨文件首命中**也会**选到 f1。
- `[16]/[18]/[20]` 的 `wf = -1`（找不到有名祖先）⇒ 仍走全体搜索 ⇒ **仍不确定**。
- ⇒ 必须做 **pinned-vertex 检测**：在 t 与 t+Δt 各算一次蒙皮位置，取位移 ≈ 0 的顶点，
- 再按权重反查骨索引。这与本项目一贯的「可复现证据、不猜」标准一致。

---

## `ARCH_276.md`

**标题**：ARCH_276/277/278 —— 披风专项：四次否定 + 无名骨映射的最终证实 ｜ 1. ARCH_276：顶点级判据（先说结论：判据挑错了一次） ｜ 2. ARCH_277：`srcKT` 丢掉「是否有半精度位置层」（**防御性，本次为 no-op**） ｜ 3. ARCH_278：无名骨映射被**数据本体**证实 ｜ 4. 其它已确证的事实 ｜ 5. 回归 ｜ 6. 未决

**发现/结论**（9 条，最多列 40）：

- - 同时排除了两个具体机制：**权重全 0**（`nZeroW=0`）与**骨下标越界**（`nBadIdx=0`）。
- 这两者会让 `wsum<=1e-6` ⇒ 顶点原样保留绑定位置 ⇒ 精确被钉住 ⇒ 且**完全不计入
- `nSkinned`**（旧统计量看不见它）。实测二者均为 0。
- 而 `make_pose` 的 ARCH_273 平移修复判据是 `c->srcKT ∈ 26..28` ⇒ 平移退回绑定值。
- **实测为 no-op**：`[gapunnamed] 0/0 halfPos=8 halfPosButKT=0`、`0/1 halfPos=12 halfPosButKT=0`。
- ⇒ ARCH_274 的 `paletteIdx → targetId` 回退在这四根骨上**正确**，
- ⇒ **两者名字不相交** ⇒「按名字查通道」本身没有歧义。
- `cape_emblem|k49|pal118`。它们的 `paletteIdx`（28..118）与 f0 的 tid 数值区间重叠 ⇒
- 四次否定的净结果：肩披风族已排除；顶点未冻、权重正常、通道类型正常。

---

## `ARCH_279.md`

**标题**：ARCH_279 —— ★★ 重大发现：`make_pose` 对时间 `t` 不敏感（姿态被冻住） ｜ 1. 判据 ｜ 2. 结果（决定性） ｜ 3. 这条结论解释了什么 ｜ 4. 已排除（本轮四次否定，均由数据支撑） ｜ 5. 已确证（数据本体，非推断） ｜ 6. 下一步（唯一该做的） ｜ 7. 回归

**发现/结论**（10 条，最多列 40）：

- ⇒ **`make_pose(f, gs, t)` 与 `t` 无关。时间进不到姿态里。**
- 因为映射变了 ⇒ 那个固定姿态变了。**不是动画在动。**
- | 假设 | 判据 | 实测 |
- | 顶点被冻住 | 按主导骨统计 `dm≈0` | 20 / 886万 ⇒ 否定 |
- | 权重全 0 回退绑定位置 | `wsum<=1e-6` 计数 | `零权重=0` ⇒ 否定 |
- | 骨下标越界 | `bi[z]>=N` 计数 | `越界=0` ⇒ 否定 |
- | kt32+kt26 同存导致平移丢失 | `hasHalfPos && srcKT∉26..28` | `halfPosButKT=0` ⇒ 否定（防御性保留） |
- ⇒ `ARCH_274` 的 `paletteIdx → targetId` 回退**正确**。
- ⇒ **名字不相交** ⇒ 按名字查通道无歧义。
- `[tidmatch]` 显示 `kt32^388` ⇒ `Chan::times` 有 388 个条目、**不是空的**。

---

## `ARCH_27_dance_system.md`

**标题**：ARCH_27 —— DanceSystem 与对局场景生命周期 ｜ 1. 职责（一句话） ｜ 2. 结构 / 流程图（核心） ｜ 2.1 三层结构总览 ｜ 2.2 `DanceSystem` 状态机（**实测全解**，`0x81049AFC`） ｜ 2.3 `danceSystemCreate`（`0x81049E82`）建的是**4 个异步加载器记录** ｜ 2.4 `danceResidentIsReady`（`0x81047280`）—— state1→2 的闸 ｜ 2.5 `DanceSystem` state2 每帧的四个动作（其中最后一个是关键） ｜ 2.6 `DanceProcess` 21 状态机（`0x81043FEE`） ｜ 2.7 `DanceProcess` state 0 的完整初始化清单（`0x81043D00`–`0x81043ED8`，实读） ｜ 2.8 `DanceProcess` 每帧时基与主循环（state 14 / 17 / 20 共用） ｜ 2.9 `music_select_notify` 门与退出（`MusicSelectMain` state 0x30） ｜ 2.10 `mns` 序列（`0x811215F4` 创建，step `0x81126154`）——谱面与难度 ｜ 2.11 键盘计数段（`mns` state 0 的后续，`0x81121060`）——判定输入的确切形态 ｜ 2.12 资源清单（对局真正加载什么） ｜ 3. 函数地址表 ｜ 3.1 app 状态表（**本轮逐字节复核，含对 ARCH_28 的仲裁**） ｜ 3.2 本模块函数地址表 ｜ 4. 数据结构字段表 ｜ 4.1 DanceSystem 注册节点 / work ｜ 4.2 DanceProcess 对象（`0x81047C54` 分配） ｜ 4.3 每帧子对象更新 `0x81049976` 的调用序（**core**） ｜ 4.4 DanceSystem 全局 ｜ 5. 调用链与生命周期

**发现/结论**（453 条，最多列 40）：

- > - 全部结论附证据：反汇编行（含 **真实 VA** + 指令原文）、原始字节、文件偏移、日志原文。
- >   已用**已知正确样本**校准：`0x81049AFC = B510 push {r4,lr}` 与 ARCH_17 §0.2 逐字节一致，
- > **映射（本轮关键，见 §8 纠正 1）**：
- > - `va < 0x81325000`（SEG0）：`file_off = va - 0x81000000 + 0x10E0`
- > - `va >= 0x81325000`（SEG1）：`file_off = va - 0x81325000 + 0x3228F0`  ← **必须分段，不能统一**
- 真正的对局状态机在 **`DanceProcess`（0x81043FEE，21 状态）** 与 **`mns`（0x81126154，谱面/判定）** 里，
- ┌─ app 状态机（表基址 0x813B01B4，步长 0x50，7 行）────────────────────────────┐
- │  enter=[+0x44] update=[+0x48] leave=[+0x4C]；dispatch 0x811855C8/0x8118586E  │
- ┌─ 每帧任务链表调度器 0x811DB1F0（step 在 task+0x28）──────────────────────────┐
- │  具名序列节点（工厂 0x8116F7CA / 按名查 0x8116F900，名字在 r0）              │
- ┌─ DanceProcess 21 状态机 0x81043FEE ＋ 每帧子对象更新 0x81049976 ─────────────┐
- 反汇编（真实 VA，工具 `_a27.py dis 0x81049AFC 36`）：
- 81049AFE  bl       #0x8116f8a8          ; r0 = node->work   (seqNodeWork)
- 81049B08  bhi      #0x81049b10          ; state > 1 -> 检查 ==2
- 81049B0C  beq      #0x81049b18          ; state == 0 -> S0
- 81049B0E  b        #0x81049b3c          ; state == 1 -> S1
- 81049B12  bls      #0x81049b4e          ; state == 2 -> S2
- 81049B16  b        #0x81049b5c
- --- S0 (0x81049B18) 建系统 ---
- 81049B18  bl       #0x81049e82          ; danceSystemCreate()          ← §2.3
- 81049B20  bl       #0x81047c54          ; danceProcessAlloc(0x100)     ← §4.2
- 81049B26  bl       #0x81046c80          ; danceResidentCreate()
- 81049B2A  bl       #0x810467ec          ; danceMainSystemCreate()
- 81049B2E  bl       #0x81046940          ; danceGlobal 清零 (0x8154B00)
- 81049B3A  b        #0x81049b5c
- --- S1 (0x81049B3C) 等全局舞曲表就绪 ---
- 81049B3C  bl       #0x81047280          ; danceResidentIsReady()       ← §2.4
- 81049B44  beq      #0x81049b5c          ; 未就绪 -> 仅走尾部
- 81049B4C  b        #0x81049b5c
- --- S2 (0x81049B4E) 常驻 ---
- 81049B4E  bl       #0x81046898          ; danceMainSystemUpdate()
- 81049B52  bl       #0x81003e70          ; 引擎 dt 取数（浮点返回 s0；见 §2.5）
- 81049B56  bl       #0x81046954          ; danceGlobalUpdate()
- --- 尾部 (0x81049B5C) ---
- 81049B5C  cbz      r0, #0x81049b62
- 81049B5E  bl       #0x81047e16          ; danceProcessUpdate(work[4])  ← ★ 每帧推进对局
- 81049B64  pop      {r4, pc}            ; ★ 恒返回 0 = 保活（永不摘链）
- | 迁移 | 条件 | 证据 |
- | （首次调用）→ 1 | 无条件：S0 体执行完即 `str r1,[r4]` | `0x81049B38` |
- | 1 → 2 | `0x81047280()` **返回非 0** | `0x81049B3C cmp r0,#0` / `0x81049B44 beq` |

---

## `ARCH_281.md`

**标题**：ARCH_280/281 —— 真正的病根：三根肩披风骨被 f0 的常量通道钉死 ｜ （并**收回 ARCH_279**） ｜ 0. 先收回 ARCH_279 ｜ 1. 修正判据后立刻现形 ｜ 2. 这 3 根骨是谁 ｜ 3. 修复（ARCH_281） ｜ 4. 验证 ｜ 5. 本弧线（ARCH_274→281）的可复现证据链 ｜ 6. 顺带作废的旧结论

**发现/结论**（11 条，最多列 40）：

- ARCH_279 断言「`make_pose(f, gs, t)` 对 `t` 不敏感，角色被冻住」。**这是错的**，
- 是我自己的测试缺陷造成的假结论：
- - 实测 `[timerange] part0 通道=196 times=[0.0000 .. 16.6000]` —— **动画片长 16.6 秒**；
- `t > times.back()` ⇒ `i1 = nk-1`、`i2 = i1`、`u = 0` ⇒ **永远返回最后一个关键帧**；
- - 于是 `t=22.581` 与 `t=23.081` 都被钳到片尾 ⇒ 182 根骨全部 `d = 0`。
- **⇒ 「姿态与 t 无关」是两个都在片尾的时刻造成的假象。**
- 教训：比较两个时刻之前，必须先确认它们都落在动画的时间范围内。
- **无名骨与肩披风骨交错排列** ⇒ `[16]/[18]/[20]` 就是 `L_S_F_cape_00` /
- - 祖先规则给 `wf = -1`（找不到有名祖先）⇒ 退回**跨文件首命中** ⇒ 落到 **f0**；
- ⇒ 这 3 根骨被钉死在绑定位置。
- L_S_B_cape_01` 全在 **f1** ⇒ 取到正确的动画通道。

---

## `ARCH_286.md`

**标题**：ARCH_286 —— kt31 位置层被丢弃：左领口接缝被拽住的真因 ｜ 1. 症状 ｜ 2. 定位（`ARCH_285` 判据：左右镜像骨的驱动源比对） ｜ 3. 两个独立的根因 ｜ (a) `srcKT` 抹掉了「是否有平移层」 ｜ (b) kt31 的位置是**量化整数**，必须反量化 ｜ 4. ★ 一次被我亲手改坏、又被截图抓住的实验（`ARCH_285`） ｜ 5. 修复（`ARCH_286`） ｜ 6. 验证 ｜ 7. 本弧线（ARCH_274→286）证据链 ｜ 8. 收回的假结论

**发现/结论**（5 条，最多列 40）：

- 角色左肩披风上缘露出一块**红色三角**（披风内衬被翻出）。用户确认：就是这一处。
- 而 `ARCH_273` 的平移启用判据是 `srcKT ∈ 26..28` ⇒ **kt31 的平移被丢弃**，退回绑定值。
- 只做 (a) 而不做 (b)（即把 `hasPos` 也纳入启用条件）⇒ **模型炸成一团红色、几何崩解**。
- | 权重**非降序**，`bi[0]` 不是主导骨 | `[wsh] L@4,153,-1[2:28 3:57 4:11 5:4]` ⇒ 改用 argmax |
- 改到范围内（0.2→0.7）后 179/182 根骨在动。**教训：比较两个时刻前先确认都在时间范围内。**

---

## `ARCH_289.md`

**标题**：ARCH_289 —— 真实歌曲播放（ATRAC9 → PCM → AAudio） ｜ 1. 资产定位 ｜ 2. 格式（实测，非推断） ｜ 3. 解码（PC 侧，已有的工具链） ｜ 4. Android 侧改动（只动一处） ｜ 5. 验证状态 ｜ 6. 后续可选

**发现/结论**（7 条，最多列 40）：

- - `fmt` tag = `0xFFFE`（EXTENSIBLE）；48000 Hz / 2ch / byteRate 24000 ⇒ 每声道 12000 B/s
- **不是 PCM**（16bit 应为 192000 B/s）⇒ **ATRAC9**。
- **产物校验**（关键：确认不是静音/垃圾）：
- 峰值 ≈ -0.6 dBFS、RMS ≈ -13 dBFS、100% 非零 ⇒ 真实音乐。
- g_audioFrames.fetch_add(numFrames);          // ★ 帧计数逻辑一行未改
- **设计要点：保留原帧计数时基。** `g_audioFrames` 仍按 `numFrames` 累加 ⇒
- - **待上机**：手机没电已拔（用户告知）。插回后需确认：

---

## `ARCH_28_music_select.md`

**标题**：ARCH_28 — 选歌流程与主菜单导航 ｜ 0. 对既有结论的纠正（优先阅读） ｜ 0.1 【严重】SEG1 的文件偏移映射错 `0x37F0`，`_p5d_dis.py` 的「SEG1 同样成立」不成立 ｜ 0.2 `ARCH_17 §0.2`「`_p5d_dis.py` 标签比真实 VA 大 2 字节」应撤回 ｜ 0.3 app 状态表编号整体少了一行（`ARCH_24 §1` / `ARCH_16`） ｜ 0.4 `0x8118F9C8` 不是"标题菜单输入状态机"，而是调试测试序列 `tus test` ｜ 0.5 输入全局表字段名纠正（`ARCH_18 §4.2 / §5.1`） ｜ 1. 本模块职责（一句话） ｜ 2. 结构 / 流程图（核心） ｜ 2.1 从主菜单到切场景（实证） ｜ 2.2 `MusicSelectMain` 创建点逐条（实证，`0x8113AA52`） ｜ 2.3 选歌模块的代码分区（实证，由"源文件路径串"的引用点聚类得到） ｜ 2.4 谱面路径的产生（实证，`0x8112617C` 处的 5 状态 TBH 机） ｜ 3. 关键函数地址表 ｜ 3.1 导航 / 生命周期 ｜ 3.2 `MusicSelectMain` 本体 ｜ 3.3 `MusicSelectMain` 全部 52 个状态入口（`_arch28_tbh.py 0x81147A3C 52`，实证） ｜ 3.4 输入语义层（本轮新解，实证） ｜ 3.5 音频试听（选歌用的声音对象） ｜ 4. 关键数据结构 ｜ 4.1 app 状态表项（步长 0x50，**本轮已可静态直读**） ｜ 4.2 `MusicSelectMain` 的 work 块（`0x8116F8A8(node)` 返回，≥ 0x2628 字节） ｜ 4.3 歌单表（修正映射后的真实地址） ｜ 4.4 输入相关结构（修正命名后的完整表）

**发现/结论**（298 条，最多列 40）：

- > 分析对象：`C:\dev\p5d_build\eboot.bin`（ARM32 Thumb，镜像基址 `0x81000000`）
- > `_arch28_dis.py`（线性反汇编，遇到无法解码的字会标注 `*** data ***` 并跳过，便于整函数 dump）、
- > 参考文档：`ARCH_17`（音游核心）、`ARCH_18`（输入链）、`ARCH_20`（数据格式）、`ARCH_22`（UI）、`ARCH_24`（总览3）
- > 记号：**实证** = 有 ELF 头 / 反汇编原文（含 VA 与指令）/ 原始字节 / 日志原文；
- > **推测** = 推理所得；**未解** = 本轮未取到证据。
- > ⚠️ 本轮最重要的产出是 **§0 的三条纠正**：其中「SEG1 文件偏移映射错误」影响此前**所有**依赖 SEG1（≥ `0x81325000`）
- > 数据地址的结论与工具行为，请先读 §0 再读其余章节。
- **旧结论**（`_p5d_dis.py:9-11`、`ARCH_17 §0.2`、`ARCH_20 §3`）：
- file_off = va - 0x81000000 + 0x10E0     # 声称 SEG0 / SEG1 都成立
- **实际**（实证）：ELF program header 给出的真实映射是**分段的**：
- ph0 type=0x1 off=0xE0     vaddr=0x81000000 filesz=0x321810 memsz=0x321810 flags=0x5
- ph1 type=0x1 off=0x3218F0 vaddr=0x81325000 filesz=0x21BA74 memsz=0x3AA4A0 flags=0x6
- | SEG0（.text） | `0x81000000 .. 0x81321810` | `file = VA − 0x81000000 + 0x10E0` ✔（旧公式对） |
- | SEG1（.data/.rodata） | `0x81325000 .. 0x81540A74`（filesz），memsz 到 `0x816CF4A0` | `file = VA − 0x81325000 + 0x3228F0`，等价于 `旧公式 − 0x37F0` |
- 1. **ELF 头本身**：`0x81325000 → 0x3218F0 + 0x1000 = 0x3228F0`；旧公式给 `0x3260E0`，差 `0x37F0`。
- 2. **键位掩码表**：输入解码函数 `0x8116F50A` 用立即数构造 `0x813AFC40`
- （原始字节 `4f f6 40 41 c8 f2 3a 11` = `movw r1,#0xfc40; movt r1,#0x813a`），而按旧公式找到的掩码表在 **`0x813AC450`**：
- file 0x3AD530  -> VA(旧映射) 0x813AC450        ← 唯一命中
- `0x813AC450 + 0x37F0 = 0x813AFC40` ✔ 与代码立即数完全吻合。
- 3. **日志锚点 vs 静态表**（最有力）：日志 `[ST-UPD-CALL] state=1 updateFn=0x81185927`
- （`ARCH_17 §5.1`）。全文件只出现 1 次 `0x81185927`，按旧映射落在 `0x813ACA5C`；
- `0x813ACA5C + 0x37F0 = 0x813B024C`，而 `0x813B01B4（ARCH_16 的 app 状态真表基址） + 1*0x50 + 0x48 = 0x813B024C` ✔
- 同理 `state=6 updateFn=0x81185987` → `0x813ACBEC + 0x37F0 = 0x813B03DC = 0x813B01B4 + 6*0x50 + 0x48` ✔
- | `0x813A4CB4`（"G0 基址"） | `0x813A84A4` | 指针数组段（**不是**歌曲记录，见 §4.3） |
- | `0x813A4EB0`（G1） | **`0x813A86A0`** | P3 侧 43 条 × 0x38 |
- | `0x813A5850`（G2） | **`0x813A9040`** | P5 侧 43 条 |
- | `0x813A64B0`（G3） | **`0x813A9CA0`** | 另一语言/编码 43 条 |
- | `0x813A6E50`（G4） | **`0x813AA640`** | 另一语言/编码 44 条 |
- | `0x813B01B4`（ARCH_16 的 app 表） | `0x813B01B4`（**本来就对**，因为它由代码立即数得出） | — |
- `ARCH_17` 给出的判据是 `0x811980FE` 处应读成 `movw r0,#0xb140`、而"真实标签"是 `0x811980FC`。实测：
- $ py _arch28_dis.py 0x811980F8 0x14
- 811980FE  movw      r0, #0xb140        ← movw 真的在 0x811980FE
- `0x811980FC` 是 `push`，不是 `movw` ⇒ **不存在 +2 偏移**。`ARCH_17 §0.2` 的对照表第二行是错的
- **实测**（修正映射后直接静态读 `0x813B01B4`，7 行 × 0x50 全部可读）：
- | 0 | `DEBUG MENU` | 0 | `0x811858CF` | `0x811858D1` | `0x811858ED` |
- | 1 | `initread` | 0 | `0x8118591F` | **`0x81185927`** | `0x81185943` |
- | 2 | `FREE MODE` | 1 | **`0x81185A0D`** | `0x81185A15` | `0x81185A27` |
- | 3 | `COMMU` | 0 | **`0x810C84D9`** | `0x810C8567` | `0x810C8583` |
- | 4 | `TUTORIAL` | 0 | `0x81185A51` | `0x81185A59` | `0x81185A6B` |
- | 5 | `MAIN MENU` | 1 | **`0x811859C1`** | **`0x811859D3`** | `0x811859E5` |

---

## `ARCH_290.md`

**标题**：ARCH_290 —— WAV 解码链路抽取 + Windows 离线交叉验证；并记下输入链路的确切缺口 ｜ 0. 本轮环境发现（可复用，影响以后所有 PC 侧 C++ 验证） ｜ 1. 第 (1) 项：WAV 解码链路 —— **完成** ｜ 1.1 抽取 ｜ 1.2 Windows 离线测试 ｜ 1.3 接入 APK ｜ 2. 第 (2) 项：输入链路 —— **缺口已精确定位** ｜ 3. 尚未开始 ｜ 4. 回归状态

**发现/结论**（6 条，最多列 40）：

- 即全部 **28 首歌**在 格式 / 样本数 / frames / dataBytes / **FNV-1a(逐字节校验和)** / peak / RMS
- ⇒ **上机前解码器已被独立实现验证**（这是本项的目的）。
- // 因此在拿到证据之前**固定传 0**，不改动判定输入。
- ⇒ **按钮→判定**这一段已经由 `PLAY_SELFTEST MATCH 871d874fe1b44ac4` 证明正确；
- 2. 从资产/eboot 找布局证据。候选来源：
- - eboot 判定节点 `0x8111B018` 附近的触碰位处理代码；

---

## `ARCH_291.md`

**标题**：ARCH_291 —— 判定链路 PC 离线测试台 + 触碰通道作用的**测量**（非推断） ｜ 1. 交付物 ｜ 2. 结果（决定性） ｜ (1) ★ PC 离线测试台与原版判定路径逐位一致 ｜ (2) 触碰通道位 16..21 对判定**无影响**（测量，非推断） ｜ 3. 对第 (2) 项的重述（据证据） ｜ 4. 顺带修正一处文档口径 ｜ 5. 可复现命令 ｜ 6. 下一步

**发现/结论**（14 条，最多列 40）：

- mode=none touchCh=0x000000 音符=819 命中=802 漏=17 评级1=802 摘要=871d874fe1b44ac4
- mode=bit  touchCh=0x010000 音符=819 命中=802 漏=17 评级1=802 摘要=871d874fe1b44ac4
- mode=bit  touchCh=0x020000 ... 同上
- mode=bit  touchCh=0x040000 ... 同上
- mode=bit  touchCh=0x080000 ... 同上
- mode=bit  touchCh=0x100000 ... 同上
- mode=bit  touchCh=0x200000 ... 同上
- （`main.cpp: kPlayDigestBaseline`）。⇒ 判定链路（谱面解析 → 时钟 → 输入边沿 → 判定 →
- ⇒ 该推断成立。**因此"把触点接进 `touchCh`"这条路线不可能让触摸驱动判定**，
- - **手柄 → 判定**：已通。证据 = `mode=none` 摘要与设备基准相同 + 设备侧 `PLAY_SELFTEST MATCH`。
- 「**刮盘用摇杆，可在设置里改成 L/R**」⇒ 刮盘走摇杆/L/R，**不是触摸屏**。
- ⇒ **触摸驱动判定这条路在原版里可能根本不存在**，占位映射保持关闭是正确状态。
- // 触点 → 动作的对应关系**尚未读出**，因此在拿到证据之前**固定传 0**
- // 6 个单 bit 与 0 的判定摘要逐位相同 ⇒ 传 0 与传真实掩码**等价**。

---

## `ARCH_292.md`

**标题**：ARCH_292 —— 评级 1..4 名称的档案追查：三条线索，一条确证为否 ｜ 1. 线索 A：eboot 字符串常量 —— **否** ｜ 2. 线索 B：本地化文本表 —— **部分命中，但不是判定名表** ｜ 3. 目前的结论（尚未最终确证） ｜ 4. 下一步（明确） ｜ 5. 本轮附带产出

**发现/结论**（15 条，最多列 40）：

- （`0x8111B018` 四级嵌套带）。缺的只有 **1..4 → 名称** 的对应。
- 0x002F2D68  RESULT_RANK        ← 结算画面的**歌曲评级**（S/A/B/C…），非单音符判定
- 0x002F2D84  RESULT_RANK_BLINK
- 0x002F2E04  RANK_MASK
- 0x002F2E10  RANK_CIRCLE1
- 0x002F30C8  CLEAR_RANK %f
- ⇒ **判定名不在 eboot 里**。`RESULT_RANK` 是另一回事，不能混用。
- 0x0002DC  mns_option_006
- 0x000305  AGOOD            ← 'A' 是控制字节（彩色标签），正文是 GOOD
- 0x0006CA   MISS
- 0x000C7C  m PERFECT
- 0x000AB5  AMISS
- 0x001091   MISS
- ★ 候选判定组：未找到 >=3 个判定词集中在 400 字节内的组
- ⇒ **判定名不在文本表里**。

---

## `ARCH_293.md`

**标题**：ARCH_293 —— 第 (4) 项「渲染保真度差距 2497 vs 32331」：**伪命题，已归因并修复** ｜ 1. 结论先行 ｜ 2. 可复现证据（在 PC 上精确复现 `32331`） ｜ 3. 为什么会被误当成"差距" ｜ 4. 修复（行为不变，只改口径） ｜ 5. 那么 LIVE 的渲染保真度**怎么**才是可验证的？ ｜ 6. 本项对目标的交代

**发现/结论**（5 条，最多列 40）：

- ⇒ **`32331` 就是这张资产参考图的非背景像素数**，来源确认，不是 LIVE 数字。
- LIVE 模式下 `g_expected.size() != 806400`（日志里"无可比期望图"就是证据），
- **当前状态**：LIVE 保真度**尚无有效判据**，但也**没有证据表明存在缺陷** ——
- "2497 vs 32331" 不能作为缺陷证据。此前的 `ARCH_279`/`ARCH_290` 笔记里若引用过这个
- **结论：该"差距"不是渲染缺陷，是度量口径错误。** 已修复口径。

---

## `ARCH_294.md`

**标题**：ARCH_294 —— 第 (3) 项：判定名称**已找到**，但暴露出一个必须如实记录的矛盾 ｜ 1. ★ 判定名称来自舞曲 UI 精灵表（决定性证据） ｜ 2. ⚠ 与 `ARCH_251` 的矛盾（如实记录，不掩饰） ｜ 候选解释（均**未验证**，列出以备下轮排除） ｜ 3. 代码侧立场（不变） ｜ 4. 可复现产物 ｜ 5. 对目标第 (3) 项的交代

**发现/结论**（17 条，最多列 40）：

- | `0x000FE8` | `PERFECT` | 203 | 196 | 73 | 0x0D |
- | `0x001048` | `GREAT`   | 277 | 196 | 73 | 0x0D |
- | `0x0010A8` | `GOOD`    | 351 | 196 | 73 | 0x0D |
- | `0x001108` | `MISS`    | 425 | 196 | 73 | **0x03** |
- - 条目 stride = **0x60 = 96 字节**，4 条连续，无第 5 条；
- - `+0x5C`：三条为 `0x0D`，**MISS 独为 `0x03`** ⇒ MISS 在结构上确实是特殊项。
- **穷举确认**（`_mns/ui_all.py`，扫描 `dance/ui/*.bin` 全部 8 个文件）：
- c_dance_ui_p3.bin  5600800 B : PERFECT@0xE80 PERFECT@0x34194C ...
- ⇒ **显示层词汇表 = {PERFECT, GREAT, GOOD, MISS}，共 4 个。**这是档案事实。
- `ARCH_251` §2 逐支读出的级联 `0x8111B018` 返回 **5 个码**：
- 调用方 `0x8111B79E cmp r1,#5; beq …` 把 **5 当作未命中** ⇒ **1..4 是命中等级**。
- **⇒ 需要 4 个「命中」名称，而 UI 只有 3 个（PERFECT/GREAT/GOOD）+ 1 个 MISS。**
- | A | 码 4 显示为 `MISS`（显示层把 4 与 5 合并）⇒ 词汇表恰好 4 个 | 找"返回码 → 精灵下标"的选择代码 |
- | B | `ARCH_251` 的级联读法有误（例如 level 1 带外实为返回 4） | 重新反汇编 `0x8111B018`，逐支复核 |
- **候选 A 有一个支持点**：`+0x5C` 字段 MISS 独为 `0x03` ⇒ MISS 在结构上是特殊项，
- 把"远命中"也归入 MISS 并非不可能。但这只是**线索，不是证据**。
- - ✅ 排除了 eboot 字面量与文本表两条路径（各有可复现脚本）；

---

## `ARCH_295.md`

**标题**：ARCH_295 —— 输入偏移 → 评级码 的完整实测阶梯；并纠正 ARCH_294 的一处误读 ｜ 1. 做了什么 ｜ 2. 实测阶梯（`off=0` 处摘要 = `871d874fe1b44ac4`，与设备基准逐位相同 ✔） ｜ 3. ★ 纠正：`ARCH_294` 里我说"实测方向与 ARCH_251 表格相反" —— **这句话错了** ｜ 4. 第 (3) 项的现状（承接 `ARCH_294`） ｜ 5. 目标各项现状 ｜ 6. 回归

**发现/结论**（11 条，最多列 40）：

- > 写成断言后测试立刻 FAIL（实测 `g3=52`）。实测明细如上：+0.60 时只有 **186** 个音符被登记
- - 那 17 个「漏」在**所有偏移下恒为 17** ⇒ 是**超时漏**（谱面中 `mask==0` 的音符从不按键），
- 与偏移无关 —— 与 `ARCH_291` 的结论一致；
- 实测与 `ARCH_251` 的表格**完全一致**，无需修正该表。
- **由此得到的定量结论（可复现）**：
- **已确证（档案 + 结构）**：舞曲 UI 的判定名恰好 **4 个**，连续等距（stride 0x60）：
- **第 4 条起是别的 UI 元素**（尺寸与 `+0x28` 字段都不同）⇒ **没有第 5 个判定槽**。
- "进了最宽的带、但出了第二宽的带（且偏早）"，它**是否显示为 MISS** 需要显示侧证据。
- - 已知调用方 `0x8111B786 bl #0x8111b018` → `0x8111B79E cmp r1,#5`，1..4 都走 `0x8111B83E`；
- - 需要从 `0x8111B83E` 继续追 `r1`（评级码）如何被使用/存表；
- | (3) 评级 1..4 档案依据 | 🟡 名称/顺序/结构已确证；**精确对应待显示侧证据**，不猜 |

---

## `ARCH_296.md`

**标题**：ARCH_296 —— 评级码的两个消费者；消息表结构；第 (3) 项剩余缺口的准确边界 ｜ 1. ★ 评级级联 `0x8111B018` 的**全部**调用者（`findfunc.py` 实测） ｜ 调用者 A：`0x8111B786` —— **不使用评级码** ｜ 调用者 B：`0x8111B48A` —— **评级码被保存并向上返回** ｜ 1.1 继续追了一层（本轮又前进一步） ｜ 2. 消息表结构（`data/msg_mns_option.bin`，8,752 B） ｜ 3. 第 (3) 项的现状（精确边界，不夸大） ｜ 已确证（档案级证据） ｜ 已实测（行为级证据） ｜ 仍未确证（**不猜**） ｜ 4. 下一轮的候选路径（按代价排序） ｜ 5. 回归

**发现/结论**（47 条，最多列 40）：

- 函数入口候选 0x8111B018
- 0x8111B786  bl   #0x8111b018      ; r0 = 评级码
- 0x8111B78C  sxtb r1, r0           ; r1 = 评级码
- 0x8111B798  cmp  r0, #0           ; 另一条件
- 0x8111B79E  cmp  r1, #5           ; ★ 只与 5 比较
- 0x8111B7A0  beq  #0x8111b7ae      ; ==5 → "非命中"路径
- 0x8111B7A2  b    #0x8111b83e      ; 否则 → 命中路径
- 0x8111B83E  ands.w r0, lr, r6     ; ← r1 在此之后再未被读
- 0x8111BC28  b    #0x8111bd6c
- 0x8111BD6C  add sp,#0xc / vpop / pop.w {…, pc}   ; ★ 函数返回
- ⇒ **这条路径只把评级码用于「是不是 5」这一个判断**，
- 0x8111B472  movs r4, #5           ; r4 默认 = 5（未命中）
- 0x8111B48A  bl   #0x8111b018      ; r0 = 评级码
- 0x8111B48E  ldr  r6, [r5, #0x34]  ; r6 = 回调指针
- 0x8111B490  adds r4, r0, #0       ; ★ r4 = 评级码
- 0x8111B49E  blx  r6               ; 调回调(2, 1, obj)
- 0x8111B5FC  adds r0, r4, #0       ; r0 = 评级码
- 0x8111B60C  pop  {…, pc}          ; ★ 带着 r0 = 评级码返回
- ⇒ **评级码是"一路向上返回"的**，显示/计分消费点在**更上层的调用者**。
- **⇒ 第 (3) 项剩余缺口的准确边界：**
- 要定 `1..4 ↔ 名称`，需要继续向上追调用者 B 的调用者（即 `0x8111B48A` 所在函数的调用者）。
- 用「反汇编 + 找最近的前序 `push`」定出**包含 `0x8111B48A` 的函数入口 = `0x8111B190`**：
- 0x8111AFF6  push {r4,r5,r6,lr}
- 0x8111B018  push {r4,r5,r6,lr}     ← 评级级联
- 0x8111B190  push {r4,r5,r6,r7,lr}  ← ★ 含 0x8111B48A 的函数（"判定一个音符并返回评级码"）
- `findfunc.py 0x8111b190`：
- 0x8111B664     （在 0x8111B64C 内）
- 0x8111BDB0     （在 0x8111BD76 内）
- 其中 `0x8111B64C` 里对返回值的使用已经读到：
- 0x8111B664  bl    #0x8111b190      ; r0 = 评级码
- 0x8111B674  mov   r3, r0           ; ★ r3 = 评级码
- 0x8111B67A  str   r3, [sp]         ; 存栈
- 0x8111B67C  cmp   r3, #5           ; ★ 又只与 5 比较
- 0x8111B680  beq.w #0x8111bc2a
- **⇒ 下一轮的精确续追点：从 `0x8111B64C` 继续，看 `r3`（评级码）最终传给谁；
- 或先 `findfunc.py 0x8111b64c` 找它的调用者。** 这条链目前是
- `0x8111B018（级联） → 0x8111B190（判定+返回码） → 0x8111B64C`。
- ⇒ 该文件**不是**"整体加密"，而是部分内容做了变换，**需要单独解**。
- **⇒ 靠读帮助文本来确认判定名这条路，本轮走不通**（需要先解出该变换）。
- 2. 恰好 **4 个**，连续等距（stride `0x60`），**按质量降序**：

---

## `ARCH_297.md`

**标题**：ARCH_297 —— 第 (3) 项：两条路被**决定性排除**；调用链推到底层评分循环 ｜ 1. ★ 排除一：eboot 里**没有**判定名（此前是"没找到"，现在是"可解释地排除"） ｜ 2. ★ 排除二：UI 布局里**没有**指向判定精灵的指针/下标表 ｜ 3. 调用链推到底（每一步的调用者数都已实测） ｜ 3.1 ★ 链的顶端已找到：**每帧音符主循环** ｜ 3.2 ★ 显示侧的唯一线索：**回调** ｜ 4. 第 (3) 项最终状态（精确边界） ｜ 已确证（档案级，可复现） ｜ 已实测（行为级） ｜ 仍未确证（**不猜，代码继续用数值码**） ｜ 5. 下一轮的候选（按代价） ｜ 6. 回归

**发现/结论**（56 条，最多列 40）：

- 区域（`0x3F127C..0x3F2280`）**逐串打出来**：
- 0x3F1F8C  FLAG2_ATEST_GREATER_GEQUAL        ← "GREAT" 是 GREATER 的子串
- 0x3F1F4C  FLAG2_ATEST_LESS_LEQUAL
- 0x3F1DCC  FLAG2_HDR_TONEMAP
- ⇒ 之前的命中**全是引擎标志名的子串**，不是判定名。
- **结论：eboot 中没有 `PERFECT`/`GREAT`/`GOOD`/`MISS` 作为判定名的字符串。**
- 实测（`_mns/ui_seltable.py`）：
- 0x000FE8  count=0     0x001048  count=0
- 0x0010A8  count=0     0x001108  count=0
- ⇒ 这些偏移在文件内**根本不被引用** ⇒ 精灵是**按名字**查的，不是按下标数组。
- （顺带确认：`+0x28` 步长 74、`+0x2C/+0x30` = 196×73 是贴图参数。）
- 0x8111B018  评级级联（ARCH_251）            调用者 2 个
- ├─ 0x8111B786   只用于 `cmp r1,#5`（是否命中），不用评级码
- └─ 0x8111B48A   所在函数入口 = 0x8111B190
- 0x8111B190  "判定一个音符并返回评级码"       调用者 2 个
- ├─ 0x8111B664   在 0x8111B64C 内
- └─ 0x8111BDB0   在 0x8111BD76 内
- 0x8111B64C  r3 = 评级码，`cmp r3,#5`，存栈    调用者 **1 个**
- └─ 0x8111BDC2   在 0x8111BD76 内
- `0x8111BD76` 内部是一个**循环**：
- 0x8111BD96  loop:
- 0x8111BDAC  adds r0, r7, #0
- 0x8111BDB0  bl   #0x8111b190        ; 判定
- 0x8111BDB4  cmp  r0, #0
- 0x8111BDB6  beq  #0x8111bdd2        ; 为 0 则退出
- 0x8111BDC2  bl   #0x8111b64c        ; 评级码流转
- 0x8111BDC6  cmp  r0, #0
- 0x8111BDCA  bics.w r8, r8, r0       ; ★ 清除"已处理"位
- 0x8111BDD0  bne  #0x8111bd96        ; 继续遍历
- ⇒ 这是**逐个音符/逐位处理的评分循环**，显示层的精灵选择在这之上（或在那条
- `findfunc.py 0x8111bd76` → 调用者 `0x81122B1E` / `0x81122FF6`。
- `0x81122B1E` 落在 **`0x81122xxx`** —— 正是 `ARCH_247`/`ARCH_249` 逐支读过的
- **音符处理主循环**（`bl #0x811330d8` 读输入掩码 + 六次 `bl #0x8116ef8e` keyMask 测试）：
- 0x81122B1E  bl   #0x8111bd76      ; 评分循环（评级码在此进入计分）
- 0x81122B26  adds r4, #1
- 0x81122B2C  cmp  r4, r1
- 0x81122B2E  blo  #0x81122af6      ; 音符循环
- 0x81122B30  bl   #0x811330d8      ; ★ 读输入掩码（ARCH_244/247/249）
- 0x81122B52  bl   #0x8116ef8e      ; ★ keyMask 0x08
- 0x81122B62  bl   #0x8116ef8e      ; ★ keyMask 0x80

---

## `ARCH_298.md`

**标题**：ARCH_298 —— 输入→判定 的**离线回归测试套件**（带断言）；并纠出自己一处外推错误 ｜ 1. 为什么做这个 ｜ 2. 交付物 ｜ 3. ★ 套件抓出了我自己的一处外推错误（这是它最大的价值） ｜ 4. 对目标第 (2) 项的交代 ｜ 5. 可复现命令 ｜ 末尾应为 JUDGE_TEST_ALL_PASS ｜ 6. 第 (3) 项的进展（本轮） ｜ 7. 回归

**发现/结论**（11 条，最多列 40）：

- 此前的 `mns_touch_probe` / `mns_grade_probe` 只**测量并打印**，不是断言 ——
- 任何一处行为变了，只能靠人眼比对输出。本项把它固化成**可一键回归的断言集**。
- 我原先写的断言是「`off=+0.60` → 全码 3」。那是从 `+0.12..+0.20` **外推**的 ——
- ⇒ +0.60 时只有 **186** 个音符被登记（633 超时漏），评级码**混合** ——
- **唯一稳的结论**：码 5 在**任何**偏移（含 ±0.6 s）下都为 0。
- | 给定输入序列 → 期望判定结果 | ✅ **20 条断言**，覆盖解析 / 基准 / 偏移阶梯 / 触屏通道 |
- - 限制到 mns 模块 `0x81110000..0x81140000` 后只剩少量，且**多数是 `[sp, #0x34]`（栈）**，
- 对象字段写入只有 `0x8111B834`(在判定函数内) / `0x81134674` / `0x811347FC` 三处，
- ⇒ 第 (3) 项剩下的「`1..4 ↔ 名称`精确绑定」仍是**纯静态分析的长链**，
- 文档记录三条已排除的路径与当前精确起点。
- - 本轮 `src/` **未改动**（只新增/修正 `_mns` 下的测试程序与文档）⇒ 判定/渲染路径不受影响；

---

## `ARCH_299.md`

**标题**：ARCH_299 —— 第 (3) 项：评级码的**强制性论证**（4 个槽 ↔ 4 个命中码）+ 剩余唯一假设 ｜ 1. 本轮新增的代码证据 ｜ 2. ★ 强制性论证（把"最省事的解释"抬升为"逻辑上必然"，只剩一步） ｜ 已由证据固定的三件事 ｜ 由此**必然**推出 ｜ 剩下**唯一**未验证的一步 ｜ 3. 本轮试过并**排除**的路径 ｜ 4. 代码侧立场（不变） ｜ 5. 下一步（仅当继续追第 (3) 项时） ｜ 6. 回归

**发现/结论**（21 条，最多列 40）：

- `0x8111B190` 的**函数入口**（`push {r4,r5,r6,r7,lr}`）读清了两件事：
- 0x8111B1A8  adds r6, r1, #0        ; r6 = 参数2（note）
- 0x8111B1B0  adds r5, r0, #0        ; r5 = 参数1（judge 对象）
- 0x8111B1B8  ldr  r1, [r5, #0x30]   ; 窗口提供者（= ARCH_251 的 note->[0x30] 虚调用）
- 0x8111B1E2  mov  lr, r1
- 0x8111B1EE  blx  lr                ; 虚调用
- 0x8111B200  movs r4, #0            ; ★ 早退路径：r4 = 0
- 0x8111B202  b    #0x8111b5fc
- 0x8111B5FC  adds r0, r4, #0        ; ★ 返回值 = r4
- 0x8111B60C  pop  {…, pc}
- **⇒ `0x8111B190` 的返回值是 `0`（未判定）或**评级码 1..5**。**
- 这正好解释了两个调用方里的 `cmp r0, #0`（`0x8111BDB4`）：
- | # | 事实 | 证据 |
- | 2 | **码 5 在进入显示前就被排除** | `0x8111B79E cmp r1,#5 ; beq <非命中路径>` |
- - 走"命中"路径的评级码只有 **1..4**（2 已排除 5）；
- - ⇒ **两者之间必然是一一对应**：`{1,2,3,4} ↔ {PERFECT, GREAT, GOOD, MISS}`。
- 且四项**步长恒定**（`0x28` = 203/277/351/425，步长精确 74）⇒ 是一族刻意排布；
- 计数太多、不具区分度 ⇒ **查表实现这条路无法由此判定**（既不能证实也不能证伪）。
- 加上此前排除的：eboot 字符串（`GREAT` 是 `GREATER` 的子串）、文本表（`msg.bin` 无 `PERFECT`）、
- 理由：§2 的唯一残假设（单调）**尚未验证**，按项目纪律不得当作结论。
- - `0x8111BD76`（评分循环）的另一个调用者 `0x81122FF6` 尚未反汇编；

---

## `ARCH_29_globals.md`

**标题**：ARCH_29 —— 全局单例与核心数据结构汇总 ｜ 0. 本轮最重要的产出（先看这五条） ｜ 1. 职责（一句话） ｜ 2. 结构 / 流程图（核心） ｜ 2.1 六个中枢的拓扑 ｜ 2.2 三层嵌套调度的"全局交接面" ｜ 3. 全局地址总表（按地址排序） ｜ 4. 关键数据结构字段表 ｜ 4.1 app 状态表（`0x813B01B4`，`stride=0x50`，7 条 + 1 终止项） ｜ 4.2 app 状态管理器对象（`[0x813B11A8]`，`bl 0x8116F8A8` 返回 work 块） ｜ 4.3 每帧任务节点（`0x816CD9E0` 链） ｜ 4.4 序列注册表节点（`0x815554C0`，`0x1C` 字节，双向） ｜ 4.5 场景节点（`0x50` 字节）+ 三链（`0x816CD468`） ｜ 4.5b 两个超大对象不是同一个 ｜ 4.6 `0x816CC630` —— 帧上下文（ARCH_18 称 `0x816CC638`） ｜ 4.7 输入全局表（完整字段与写点，全部字节级复核） ｜ 4.8 按键码生成器（逻辑键表 `0x813AFC40`，n=16） ｜ 4.9 绘制作业表（`0x81555420`） ｜ 4.10 歌单表（`0x813A86A0`，`stride=0x38`） ｜ 5. 调用链与生命周期 ｜ 5.1 单例初始化顺序（实测 + 静态） ｜ 5.2 任务/序列的完整生命周期 ｜ 6. 对「能否打完一局」的影响评估 ｜ 6.1 全局表这一层**不是**瓶颈——但它把瓶颈的坐标钉死了

**发现/结论**（322 条，最多列 40）：

- > 目标：把散落在 27 份文档里的全局变量／单例集中成一张**按地址排序**、**每条都带字节级或指令级证据**的表，
- > py C:\dev\p5d_build\_a29_gscan.py f 0x816CC630     # 单个全局的字段直方图 + 引用点
- > py C:\dev\p5d_build\_a29_ev.py refs 0x8116F900     # 某地址的 code 引用 / data 字面量 / 串 / 静态值
- > py C:\dev\p5d_build\_a29_sites.py 0x813B01AC       # 某地址被 movw/movt materialise 的全部站点
- > py C:\dev\p5d_build\_a29_va.py 0x813A86A0          # 正确的 VA<->file 映射（见 §8.1）
- > 记号：**实证** = 有反汇编原文（含 VA 与指令）／原始字节／ELF 头字段；**推测** = 推理所得，未获直接证据。
- | # | 结论 | 证据 |
- | 2 | **歌单表真值 = `0x813A86A0`**（文件 0x3A5F90），步长 `0x38`，**G1 恰好 43 条**，`+00 标题指针 / +04 flag / +08 f32 BPM`；rec0 = `'BRAND NEW DAYS (YUYOYUPPE REMIX)'`，BPM=145.0 | §8.1、§4.10 |
- | 3 | **app 状态表基址 = `0x813B01B4`**（由 `[0x813B01AC]` 指向），**7 条 + 1 条空名终止项**；名字逐条读出 `DEBUG MENU / initread / FREE MODE / COMMU / TUTORIAL / MAIN MENU / TITLE` ⇒ **ARCH_22 的编号是对的，ARCH_16 整体错一位** | §4.1、`0x811858BE`/`0x811857B4` 指令原文 |
- | 4 | **补上 5 个前文未列出的全局**：`0x816CD9B4`（摘链时尾更新）、`0x816CD9B8`（摘链时头更新）、`0x816CD9CC`（空闲链头）、`0x816CDAB0`（输入邻块）、`0x81386CEC`（UI handler 表） | §4.3、§4.4、§3 |
- | 5 | **输入全局语义全部字节级复核**（`0x816CDFB0`=prev、`0x816CDFB2`=new 是它的 `+2`、`0x816CDFB4`=down、`0x816CDAA0`=sel、`0x816CDAA4`=sortFlag、`0x816CDAAC`=lx/ly、`0x816CDAA8`=rx/ry、`0x81680148`=holdT）+ **逻辑键表 `0x813AFC40` 16 项内容全部解出** ⇒ 前文（ARCH_18）的**摇杆排列本来是
- │ 0x816CC630  引擎主上下文对象（BSS，refs≈950）    │
- │  +0x008 f32 frameDelta   (由 0x8119957A vldr)   │
- │  +0x2C  u32 clockField   (由 0x81199594 ldr)    │
- │  +0x13A0 任务调度器子对象（0x811DA900…）          │
- │  +0x13C0 任务链互斥（0x81207344/0x81207352）      │
- │ 0x813B01AC(基址ptr)│         │ 0x816CD9E0 链头   │          │ 0x815554C0 哨兵   │
- │  → 0x813B01B4 表  │          │ 0x816CD9B8 尾     │          │  [+4] 头 [+8] 尾  │
- │ 每帧由任务泵调用              │ 0x811DB1F0         └────────┬─────────┘
- ▼                                             │ createTask 0x8116F7CA
- ┌──────────────────────────────┐   findTask 0x8116F900        │
- │ 0x816CD468[idx*4] 链头 (3 条)  │
- │ 0x816CC630 当前列表索引复用     │
- │ 0x814D2F2C 虚表 [node+0x20]    │
- └─ 每帧任务泵 0x811DB1F0:  for(node=head; node; node=node->[+0x4C])
- ret==-1 → 0x811DAD8C 摘链             ← 唯一的摘链口
- ├─ 输入任务 0x81199F50 → 0x811DA9A4 写输入全局表
- └─ app 状态机帧体 0x811857C2（作为一个 job）
- └─ [0x813B01AC] + state*0x50 → +0x48 update()
- └─ findTask(0x815554C0 链) 判完成
- > 「段」：`A(code-seg)` = PH0（`0x81000000..0x81321810`，有文件镜像）；
- > `D(data-seg)` = PH1 文件镜像部分（`0x81325000..0x81540A74`）；
- > `BSS` = PH1 的 `memsz-filesz` 尾部（`0x81540A74..0x816CF4A0`，**文件里恒为 0，必须运行期读**）。
- | 地址 | 推测名 | 类 | 段 | 用途 | 谁写 | 谁读 | 证据 |
- | `0x812B414C` | `__stack_chk_guard_ptr` | S | A | 栈金丝雀**指针**，其值 `0x812B4150`；所有函数序言 `ldr r0,[0x812B414C]; str r0,[sp,#N]`，尾段比对 | 由 CRT/loader 初始化（静态已有值） | 每帧数十处 | 字节：`[0x812B414C]=0x812B4150`（文件 `fo=0x2B522C`）；`0x811DA9AE movw r0,#0x414c /
- | `0x812B4150` | `__stack_chk_guard`（真值体） | F | A | 金丝雀本体（32 位，文件内非 0） | loader | 同上 | `[0x812B414C]` 指向它 |
- | `0x813A86A0` | **歌单表 G1**（`musicData.c`） | T | D | 43 条 × `0x38`：`+00 标题指针 / +04 flag / +08 f32 BPM / +0C f32 副本BPM` | 静态 | 选歌 UI（`0x81305094 MusicSelectMain` 族） | 见 §8.1 与 §4.10；文件 0x3A5F90 |
- | `0x813B01AC` | **app 状态表基址指针** | S | D | 运行期 = `0x813B01B4`；`state` 派发全靠它 | **唯一写点** `0x811857B4 str r0,[r2]`（r2=`0x813B01AC`，r0=`0x813B01B4`） | 17 个读点（`0x811853FE/85422/8559C/855BE/855DA/855FC/8568E/856B0/856CC/856EE/85768/8578A/857E8/85
- | `0x813B01B0` | `appStateEnterArgPtr` | S | D | setter 的第二个出参 | `0x811857BE str r1,[r0]` | `0x811857E2 ldr r6,[r5,#0x14]` 族 | 同函数 `0x811857AC` |
- | `0x813B01B4` | **app 状态表（7 条 + 终止项）** | T | D | 见 §4.1 | 静态 | 派发器 | 文件 0x3ADAA4，见 §4.1 逐字节记录 |

---

## `ARCH_2_thread.md`

**标题**：ARCH_2：线程与同步模型（Persona 5 Dancing in Starlight / PCSH10109） ｜ 1. 模块职责（一句话） ｜ 2. 关键函数地址表 ｜ 2.1 游戏侧（guest）——作业池 / worker / 同步握手 ｜ 2.2 HLE 侧（host）——`src/loader/eboot_probe.cpp` ｜ 2.3 导入桩 ↔ NID 对照（**SceLibKernel** 表，基址 `0x812B27A0`，步长 0x10） ｜ 2.4 导入桩 ↔ NID 对照（**SceThreadmgr** 表，基址 `0x812B2A00`） ｜ 3. 关键数据结构 ｜ 3.1 作业池 pool（= worker 的 `[argp+4]`；实测三例 `0xA0039000` / `0xA005A000` / `0xA0115000`） ｜ 3.2 作业记录（32 字节，环元素）+ 二级队列包装 ｜ 3.3 LwMutexWork（`sceKernelCreateLwMutex` 传出、guest 自持） ｜ 3.4 fiber 对象（`0x815…` 级堆对象，`_sceFiberInitializeImpl` 的 fobj） ｜ 3.5 HLE 侧调度状态（`eboot_probe.cpp`） ｜ 4. 生命周期与调用关系 ｜ 4.1 线程创建 → 启动 ｜ 4.2 三个作业池与 worker 分配（实测） ｜ 4.3 同步派发的两条路径（**本模块最重要的发现**） ｜ 4.4 worker 主循环（令 `pool=r5`） ｜ 4.5 上下文切换的两个入口（重放 vs 不重放） ｜ 4.6 fiber 世界（第二套协作模型） ｜ 4.7 h1164 实测的 21 个线程（`[THR-TAB]` 原样） ｜ 4.8 调用链（本模块视角） ｜ 5. HLE 接口现状 ｜ 5.1 拦截机制

**发现/结论**（259 条，最多列 40）：

- > 分析对象：`C:\dev\p5d_build\eboot.bin`（ARM32 Thumb，加载基址 `0x81000000`，imports=410，exports=1）
- > 反汇编工具：`py C:\dev\p5d_build\_p5d_dis.py <VA> <n>`；调用点扫描：`py C:\dev\p5d_build\_h1110_callers.py <VA>`
- > 映射：文件偏移 = VA − 0x81000000 + 0x10E0
- > 主要证据来源（共 8 处，均逐一核对过原文）：
- > 5. eboot.bin 反汇编原文（本文所有 `VA 指令原文` 均为实测输出，非推测）
- > 7. `C:\dev\p5d_build\v3k_nids.inc`（NID ↔ 符号名权威对照，用于给未实现导入命名）
- > 8. `C:\dev\p5dsrc\ARCH_3_resource.md`（姊妹文档；本文对其 §5.3 第 8 条 / P1-4 的 `0x812B4150` 疑点给出结论）
- | VA | 推测名 | 作用 | 证据 |
- | `0x811E603E` | **作业池构造函数** | 为一个池建 1 个 job 信号量 + 1 个容量信号量 + 1 个完成事件标志，再建 `sl` 个 worker 线程并 StartThread | 反汇编：`811E60A8 blx #0x812b27c0`（= `sceKernelCreateSema` 桩 `0x812B27C0`）→ `811E60AC str.w r0,[sb]`；`811E60C2 blx #0x812b27c0` → `811E60
- | `0x811E68C8` | **job worker 线程入口** | 取 `r5 = [argp+4]`（池指针），主循环：`WaitSema(pool->+0,1)` → `Lock(pool+0xC)` → 从环里取 32 字节作业记录 → `Unlock` → `blx fn(arg=fp+4)` → `Lock` → `--pool->+0x2C` → `Unlock`，`pool->+0x10 != 2` 则循环；退出返回 **1** | 反汇编见 §4.4
- | `0x811E68F2` | `WaitSema(pool->+0, 1, NULL)` 调用点 | `ldr r0,[r5]` → `movs r1,#1` → `movs r2,#0` → `blx 0x812b27a0` | 反汇编：`811E68F2 ldr r0,[r5]` / `811E68F4 movs r1,#1` / `811E68F6 movs r2,#0` / `811E68F8 blx #0x812b27a0`；桩对照 `[INFO] DEC-IM
- | `0x811E68FE` | `Lock(pool+0xC)` 调用点（**旧称"worker 自旋点"**） | `adds r0,r4,#0`（`r4=pool+0xC`）→ `bl 0x81207344` → `ldr r0,[r0]; blx 0x812b2830`（`LockLwMutex`） | 反汇编：`811E68EE adds.w r4,r5,#0xc` / `811E68FE bl #0x81207344`；`81207344 push {r4,lr}
- | `0x811E6902`–`0x811E6920` | **从环里取一条作业记录** | `idx=[pool+0x1C]`；`memcpy(fp, [pool+0x14]+idx*32, 0x20)`；`pool[0x1C]=(idx+1)%pool[0x18]` | 反汇编：`811E6902 ldr r6,[r5,#0x1c]` / `811E6908 ldr r1,[r5,#0x14]` / `811E690A add.w r1,r1,r6,lsl #5` / `
- | `0x811E6922` | `Unlock(pool+0xC)` 调用点 | `bl 0x81207352` → `ldr r0,[r0]; blx 0x812b28a0`（`UnlockLwMutex`） | 反汇编：`811E6922 adds r0,r4,#0` / `811E6924 bl #0x81207352`；`81207352 push {r4,lr}` / `81207354 ldr r0,[r0]` / `81207358 blx #0x812b28
- | `0x811E6928` | **作业函数间接调用点** | `r1 = [fp]`（= 作业记录第 0 字 = 函数指针）；`r0 = fp+4`（作业参数区）；`[fp+0x1C] = pool`；`blx r1` | 反汇编：`811E6928 ldr.w r1,[fp]` / `811E692C add.w r0,fp,#4` / `811E6930 str.w r5,[fp,#0x1c]` / `811E6934 blx r1`；日志 `[JOB-ST-D] j
- | `0x811E6936`–`0x811E6944` | **作业完成计数** | 再 Lock 一次，`--pool->+0x2C`（pending），Unlock | 反汇编：`811E693C ldr r0,[r5,#0x2c]` / `811E693E subs r0,#1` / `811E6940 str r0,[r5,#0x2c]`；`811E6942 adds r0,r4,#0` / `811E6944 bl #0x81207352` |
- | `0x811E6948` / `0x811E694E` | **循环条件 / 退出** | `[pool+0x10] != 2` → 回 `0x811E68F2`；等于 2 则 `movs r0,#1` 并做栈金丝雀校验后返回 | 反汇编：`811E68E8 ldr r6,[r5,#0x10]` / `811E68EA cmp r6,#2` / `811E68EC beq #0x811e694e`；`811E6948 ldr r0,[r5,#0x10]` / `811E6
- | `0x811E64D0`–`0x811E64F0` | **异步派发 B（"发作业即返回"）** | 同样入环 + `SignalSema`，但函数直接 `pop.w {r4,r5,r6,r7,r8,pc}`，**不等待完成事件** | 反汇编：`811E64E8 ldr r0,[r5]` / `811E64EA movs r1,#1` / `811E64EC blx #0x812b2ad0` / `811E64F0 pop.w {r4,r5,r6,r7,r8,pc}`；
- | `0x811E634E`–`0x811E6384` | **池表索引 + 入环** | `pool = [0x8169E080 + idx*4]`；`pending = r2`；`memcpy([pool+0x14]+prodIdx*32, rec, 0x20)`；`prodIdx=(prodIdx+1)%cap` | 反汇编：`811E634E movw r1,#0xe080` / `811E6352 movt r1,#0x8169` / `811E6356 ldr.w
- | `0x8117BA30` | **加载请求 worker（每帧推进一步）** | `worker(ctx=r0, req=r1)`；`[req+4]&1` 已置位则直接返回 1（已完成） | 反汇编：`8117BA44 adds r7,r1,#0` / `8117BA46 ldr r2,[r7,#4]` / `8117BA4A ands r0,r2,#1` / `8117BA4E beq #0x8117ba54` / `8117BA50 movs r0,#1`；详见 `A
- | `0x8117C982`–`0x8117C990` | **游戏自己的 worker 调用点 + 完成位置位** | `bl 0x8117BA30`；返回非 0 则**由调用方**置 `[req+4] bit0` | 反汇编：`8117C982 bl #0x8117ba30` / `8117C986 cmp r0,#0` / `8117C988 beq #0x8117c9bc` / `8117C98A ldr r0,[r7,#4]` / `8117C98C orrs r0
- | `0x81207344` / `0x81207352` | **Lock / Unlock 包装（2 条指令族）** | `ldr r0,[r0]` 取 work 指针后再进内核桩 | 反汇编见上 `0x811E68FE` / `0x811E6922` 行 |
- | `0x8120760C` | **DelayThread 包装** | `preserve {r4,lr}` → `blx 0x812b2a50`（`DelayThread` 桩） | 反汇编：`8120760C push {r4,lr}` / `8120760E blx #0x812b2a50` / `81207612 pop {r4,pc}`；桩对照 `DEC-IMPORT [5] nid=0x4B675D05 stub=0x812B2A50`；日志 `[YIELD]
- | `0x8120767C` | **通用线程 trampoline** | `r0=[argp+0xC]`、`r4=[argp+4]`、`r1=0` → `blx r4` → 返回 **1**。即"线程体 = `argp[4](argp[0xC], 0)`" | 反汇编：`8120767C push {r4,lr}` / `8120767E movs r3,#0` / `81207680 ldr r2,[r1,#0xc]` / `81207682 adds r0,r2,#0
- | `0x8127C1D2` | **CriFs 读泵线程入口** | `r1=[argp]`；`[r1+0x18]=1`；`r0=[r1+8]`；`blx [r1+4]`；之后 `movs r0,#0; blx 0x812b2af0`（`CheckCallback` 桩） | 反汇编：`8127C1D2 push {r4,lr}` / `8127C1D4 ldr r1,[r1]` / `8127C1DA str r2,[r1,#0x18]` / `8127C1E2 blx 
- | `0x811539F2` | **GXM 送显线程入口** | 起线程后立刻 `[EVT] Set uid=0x23`（送显完成通知） | 日志 `[THRCREATE] entry=0x811539F2 prio=0x40 stack=0xA000`、`[GXMINIT] flags=0x0 maxPending=2 displayQueueCallback=0x811EE7C3 cbDataSize=8`、`[EVT] Set thr=0x6011 uid=0x23 
- | `0x810160C4`–`0x810160D2` | **appJobQueue 链表插入（非原子）** | `head=[q+4]`；空则 `[q+4]=node`，否则 `tail=[q+8]; [tail+0xC]=node`；`[q+8]=node` | 反汇编：`810160C4 ldr r0,[r7,#4]` / `810160C6 cbnz r0,#0x810160cc` / `810160C8 str r6,[r7,#4]` / `810160CC ld
- | `0x81206BDC` | **fiber trampoline** | `r4=r0=fiber`；`entry=[fiber+0x84]`、`arg=[fiber+0x88]`；`blx entry` | 日志 `[TRAMP-IN] r0(fiber)=0x…`、`[NATIVE] fiberSwitch -> NEW fiber=0x… pc=0x81206BDC …`；代码注释 `eboot_probe.cpp:5870-5884` |
- | `0x8115F832` | **断言失败处理（printf `ASSERT:%s` + abort）** | 打印文件名/行号，随后应中止进程 | 反汇编：`8115F832 mov ip,sp` / `8115F834 push.w {r0,r1,r2,r3}` / `8115F83C subw sp,sp,#0x614`（大栈帧 = printf 缓冲）；日志 `[ASSERT-MSG] ASSERT:%s?E:\P3DP5D\xrd757\program\app\
- | `0x8115F7E0` | **断言决策桩** | 真机由对话框返回"忽略"；HLE 在解释器里**强制改 `r[0]=0` 并 `r[15]=lr`**，即"忽略继续" | 代码注释与实现 `thumb_interpreter.cpp:2198-2208`：`if(g_wp_cur&&(cur&~1u)==0x8115F7E0u){ SceUInt32 __alr=r[14]&~1u; r[0]=0; r[15]=__alr; … printf("[ASSERT-IG
- > 所有行号为 `eboot_probe.cpp` 当前版本实测行号。
- | L5680-5681 | `s_handle` / `n_create_handle` | 同步对象 uid 从 `0x10` 起自增，**信号量与事件标志共用同一 uid 空间**（故实测 0x11=sema、0x12=sema、0x13=evt） |
- 来源：`_h1164.log` 的 `[INFO] DEC-IMPORT [n] nid=… stub=…` 实测输出 + `v3k_nids.inc` 命名。
- | stub | NID | 符号 | HLE 实现 |
- | `0x812B27A0` | `0x0C7B834B` | `sceKernelWaitSema` | `n_wait_sema`（**Y**） |
- | `0x812B27B0` | `0x0FB972F9` | `sceKernelGetThreadId` | `n_gettid`（**Y**） |
- | `0x812B27C0` | `0x1BD67366` | `sceKernelCreateSema` | `n_create_sema`（**Y**） |
- | `0x812B27D0` | `0x2255B2A5` | `sceKernelCreateTimer` | **未绑定 → 返回 0（N）** |
- | `0x812B27E0` | `0x244E76D2` | `sceKernelDeleteLwMutex` | `n_noop`（Y，实为空实现） |
- | `0x812B27F0` | `0x2DCC4AFA` | `sceKernelLoadStartModule` | **未绑定（N）** |

---

## `ARCH_300.md`

**标题**：ARCH_300 —— 把「真实歌曲播放」纳入设备验证脚本（为手机插回做准备） ｜ 1. 动机 ｜ 2. 改动 ｜ 2.1 C++ 侧（`src/android/main.cpp`，**仅日志文本**） ｜ 2.2 验证脚本（`_android/verify_device.ps1`） ｜ 正向：歌曲 PCM 已装入 ｜ 负向：绝不能回退到静音 ｜ 3. 验证（本轮已做的） ｜ 4. 手机插回后的完整检查清单（一条命令） ｜ 5. 回归

**发现/结论**（1 条，最多列 40）：

- 另外仍需耳朵确认：**能听到音乐，且与舞步/谱面对得上**。

---

## `ARCH_301.md`

**标题**：ARCH_301 —— 一条命令跑完全部**离线**回归（无设备时的唯一可信证据） ｜ 1. 为什么需要 ｜ 2. 交付物 ｜ 3. 本轮实跑结果（全绿） ｜ 4. 与设备侧的分工 ｜ 5. 回归

**发现/结论**（6 条，最多列 40）：

- 手机拔线期间，唯一可信的证据是离线验证。此前这些检查散落在 4 个地方
- 本项把它们固化为**一条命令 + 一行结论**，也是手机插回前的必备交付。
- | 3 | 判定离线套件 → `JUDGE_TEST_ALL_PASS` | `ARCH_298`（20 条断言） |
- ⇒ **手机插回后只需两件事**：
- 2. 耳朵确认能听到音乐且与舞步对齐。
- 本项**只新增一个脚本**，`src/` 未改动 ⇒ 判定/渲染路径不受影响。

---

## `ARCH_302.md`

**标题**：ARCH_302 —— ★ 更正 `ARCH_299`：评级码**不是**显示选择的索引（已证伪） ｜ 1. 本轮读出的关键代码 ｜ 2. ⇒ 结论（否定） ｜ 3. 那么判定文字是由什么选的？（当前最可能的机制） ｜ 4. 第 (3) 项的当前状态（据实修正） ｜ 5. 下一轮的精确起点 ｜ 6. 回归

**发现/结论**（41 条，最多列 40）：

- 把评级码的出处在 `0x8111B64C` 内追到了它被使用的地方：
- 0x8111B664  bl   #0x8111b190       ; r0 = 0 或 评级码 1..5
- 0x8111B674  mov  r3, r0
- 0x8111B67A  str  r3, [sp]          ; 评级码存栈
- 0x8111B67C  cmp  r3, #5            ; ★ 比较 5
- 0x8111B680  beq.w #0x8111bc2a      ;   5 → 另一分支
- 0x8111B6A6  ldr  r1, [sp]          ; 取回评级码
- 0x8111B6A8  cmp  r1, #1            ; ★ 比较 1
- 0x8111B6AA  bne  #0x8111b6f8       ;   非 1 → 跳过
- 0x8111B6AC  ands r1, fp, #0x400
- 0x8111B6B0  beq  #0x8111b6f8
- 0x8111B6B2  ldr  r1, [sp, #8]
- 0x8111B6B4  orrs r1, r1, #0x300    ; ★ 置**标志位** 0x300（不是选名字）
- 0x8111B6B8  str  r1, [sp, #8]
- 0x8111B6F8  ands r1, fp, #0x40
- 0x8111B6FC  beq  #0x8111b764
- 0x8111B6FE  ldr  r0, [sp]          ; 再取回评级码
- 0x8111B700  cmp  r0, #1            ; ★ 又是比较 1
- 0x8111B702  bne.w #0x8111bd6a
- **并且 `0x8111BD76`（评分循环）里评级码根本没被保存：**
- 0x8111BDB0  bl   #0x8111b190
- 0x8111BDB4  cmp  r0, #0            ; 只判"是否判定"
- 0x8111BDB6  beq  #0x8111bdd2
- 0x8111BDB8  adds r0, r6, #0        ; ★ r0 立即被覆盖 —— 评级码被丢弃
- **补充证据（同一轮）**：在整个 `0x8111B190`（`0x8111B190..0x8111B60C`，约 1.1 KB）内
- ⇒ **评级码从不被写入判定对象**，它只是"返回值"，且只被拿来判断 `==1` / `==5`。
- （另：`0x8111BD76` 里 `bl #0x8111b190` 的返回值在 `0x8111BDB4 cmp r0,#0` 之后
- 立即被 `0x8111BDB8 adds r0, r6, #0` 覆盖 ⇒ 连"是否判定"之外都没用。）
- > 「显示侧按 `code-1` 索引 4 项表 ⇒ 必然映射为 1→PERFECT, 2→GREAT, 3→GOOD, 4→MISS」
- 但**"评级码被当作显示索引"这一环是错的**。⇒ 该映射**并未因此被确立**。
- 0x8111B48E  ldr  r6, [r5, #0x34]   ; 判定对象 +0x34 处的回调
- 0x8111B498  movs r0, #2
- 0x8111B49C  movs r1, #1
- 0x8111B49E  blx  r6                ; ★ 参数是**常量** (2, 1, obj)
- 回调收到的是**常量参数**，不是评级码 ⇒ 它是一个**"发生了判定"的通知**，
- **⇒ 显示状态很可能不在这个 1..5 的返回码里**，而在判定对象/管理器上的**另一个字段**
- **但这仍是候选，未验证。** 下一轮要做的：在 `0x8111B190` 里找 `level`（`movs r2,#3/2/1/0`
- | 已确证 | 内容 |
- | ✅ | 码 5 在显示前被排除（`cmp r1,#5`） |
- | ✅ | `0x8111B190` 返回 `0`（未判定）或码 `1..5` |

---

## `ARCH_303.md`

**标题**：ARCH_303 —— 第 (3) 项**收敛判定**：档案能给什么、给不了什么 ｜ 1. 目标原文 ｜ 2. ★ 档案（BIN）**已经给出**的（全部可复现，穷举过） ｜ 3. 档案**给不了**的：`{1,2,3,4}` 与名字的对应 ｜ 3.1 已确证的代码事实 ｜ 3.2 ★ 已**证伪**的机制（含我自己上一轮的论证） ｜ 3.3 已**排除**的检索路径 ｜ 3.4 仍未探索的唯一线索 ｜ 4. 结论（据实） ｜ 5. 本轮回归

**发现/结论**（34 条，最多列 40）：

- | # | 事实 | 证据 | 复现 |
- | 2 | 顺序为**质量降序**：`PERFECT → GREAT → GOOD → MISS` | 同上，且 stride 恒定 `0x60` | 同上 |
- **⇒ "档案依据"这一部分（名称、个数、顺序、尺寸、无第 5 项）已经查清。**
- 为此本轮起的全部静态分析工作与结论：
- | 级联 `0x8111B018` 返回 **1..5**，调用方把 5 当"未命中" | `ARCH_251`；`0x8111B79E cmp r1,#5` |
- | `0x8111B190` 返回 **0（未判定）或 1..5** | 入口 `0x8111B200 movs r4,#0` / `0x8111B5FC adds r0,r4,#0` |
- | **码 5 在进入显示前被排除** | `0x8111B67C cmp r3,#5 ; beq` |
- | **4 个显示槽 ↔ 4 个命中码** ⇒ 必然一一对应 | 上两条 + §2#1（计数推论） |
- | 假设 | 结论 | 证据 |
- | 显示按 `code-1` 索引 4 项表（`ARCH_299 §2`） | **证伪** | 码只被比较 `==5` 与 `==1`；2/3/4 走同一分支；`0x8111B6A6..B718` |
- | 码被存进判定对象供显示读取 | **证伪** | `0x8111B190` 全函数（约 1.1 KB）内 `str r4`/`str r3` **一处都没有** |
- | 评分循环把码转给显示 | **证伪** | `0x8111BD76` 里 `cmp r0,#0` 后 **r0 立即被覆盖**，码被丢弃 |
- | 回调收到码 | **证伪** | `0x8111B498 movs r0,#2 / movs r1,#1 / blx r6` —— 参数是**常量** |
- | 路径 | 结论 | 脚本 |
- `0x8111BD76` 的**另一个调用者 `0x81122FF6`**（`ARCH_297` 只反汇编了 `0x81122B1E`）。
- 0x81122EE8  ldr.w r0, [r6, #0x20c]   ; 管理器的某个池
- 0x81122EEC  movs  r1, #0x30          ; 分配 0x30 字节
- 0x81122EF4  bl    #0x810164f4        ; 分配器
- 0x81122F02  mov   fp, r0             ; fp = 新分配的记录
- 0x81122E9A  add.w r0, fp, #0x28
- 0x81122EB6  str.w r0, [fp, #0x24]    ; [fp+0x24] = &(fp+0x28)  ← 内嵌子结构
- 0x81122FC2  movs  r0, #5
- 0x81122FC4  strb.w r0, [fp, #0x28]   ; ★ 字节写 5（未命中）—— 默认值
- 0x81122FDA  strb.w r7, [fp, #0x2c]
- 0x81122FDE  strb.w r1, [fp, #0x2d]
- 0x81122FE2  strb.w r0, [fp, #0x2e]
- 0x81122FE6  strb.w r2, [fp, #0x2f]   ; 把一个 32 位值 r7 拆成 4 字节
- 0x81122FEE  ldr.w r0, [r6, #0x1e0]   ; 判定对象
- 0x81122FF6  bl    #0x8111bd76        ; 评分循环
- **⇒ `[记录+0x28]` 是一个字节宽的判定码槽，默认 5（未命中）。**
- 这与 `ARCH_302` 的否定结论**不矛盾** —— `ARCH_302` 说的是
- 「`0x8111B190` 不存储评级码」，而这里是**另一处**（音符记录）在写默认值。
- 经 6 轮静态分析，已确证：**码 5 被排除、4↔4 必然一一对应**，
- - `src/` **未改动** ⇒ 判定/渲染路径不受影响；

---

## `ARCH_304.md`

**标题**：ARCH_304 —— 会话交接（用户睡觉期间自主推进的总账） ｜ 一、四项任务状态 ｜ (1) WAV/PCM 解码链路 —— ✅ **完成** ｜ (2) 输入 → 判定 —— ✅ **完成** ｜ (3) 评级 1..4 的档案依据 —— 🟡 **档案部分完成；代码绑定未解** ｜ (4) 渲染保真度差距 —— ✅ **完成（归因）** ｜ 二、无设备时的**一条命令**复核 ｜ 三、手机插回后只需两件事 ｜ 四、环境坑（已固化为自动检查） ｜ 五、下一步（若继续第 (3) 项）

**发现/结论**（18 条，最多列 40）：

- > 纪律：凡未验证的一律标 ❌ 或"候选"，不写成结论。
- **证据**：全部 **28 首歌**在 格式/样本数/frames/dataBytes/**FNV-1a 逐字节校验和**/peak/RMS
- 另：`music218.wav` 实测 48000Hz/2ch/323.13s，peak 30702 / rms 7342（≈-0.6 / -13 dBFS），100% 非零 ⇒ 是真实音乐。
- | 离线断言测试套件（20 条） | `_mns/mns_judge_test.cpp` |
- **证据**：`JUDGE_TEST_ALL_PASS`（20/20），其中
- - **触碰通道位 16..21 的 6 个单 bit 摘要均等于基准** ⇒ 对判定无影响（`touchCh=0` 与真实掩码等价）。
- **本轮抓到的自身错误**：我曾断言 `+0.60 → 全码 3`，那是从 +0.12..+0.20 **外推**的；
- 写成断言后立刻 FAIL（实测 `g1=68 g2=8 g3=52 g4=58 g5=0 命中=186 漏=633`）。`ARCH_295` 已更正。
- **档案已确证（穷举 + 双文件交叉）**：判定名恰 **4** 个，位于 `dance_ui_p5.bin`
- （`c_` 中文版同）`0x0FE8/0x1048/0x10A8/0x1108`，stride `0x60`，顺序
- **已确证的代码事实**：级联返回 1..5 且调用方把 5 当未命中；`0x8111B190` 返回 `0`（未判定）或 1..5；
- ⇒ **4 个显示槽 ↔ 4 个命中码，必然一一对应。**
- **已排除的检索路径**：eboot 字符串 / 文本表 / UI 指针表 / 4 项查表 / `[obj+0x34]` 回调注册点。
- **本轮又撤回一条**：`0x81122FC4 strb r0,[fp,#0x28]` 的 `fp` 是纯局部记录，不是判定状态存储。
- **⇒ 结论**：`{1,2,3,4} ↔ 名字` 的精确绑定**是代码行为问题，档案里不存在这个信息**。
- 数出非背景像素 **恰好 32331** ⇒ 该值是 **APK 内置合成资产场景**的参考，而 2497 是 **LIVE 真实数据**，
- 2. **用耳朵确认能听到音乐且与舞步/谱面对齐**（离线无法覆盖）。
- - 本机用户名含中文 ⇒ gcc/ld 收到 `C:/Users/????/...` 找不到任何库。

---

## `ARCH_305.md`

**标题**：ARCH_305 —— 图集验证被格式挡住；并**降级**我自己的一处未经证实的表述 ｜ 1. 本轮想验证什么（一个我一直没验证的前提） ｜ 2. 结果：**被文件格式挡住** ｜ 3. ★ 由此必须**降级**我自己的一处表述 ｜ 4. 第 (3) 项当前状态（再次据实收窄） ｜ 5. 下一步（可选路线，按代价） ｜ 6. 回归

**发现/结论**（8 条，最多列 40）：

- **但我从未确认过那几行像素画的到底是什么。** 如果图集里第 203 行画的是 "GREAT" 而名字表说
- → **结论：没有自定义解析器就读不出这些贴图；本轮无法完成图集验证。**
- | 之前怎么写 | 实际证据强度 |
- **⇒ 这些应当记为"字段值"而不是"几何含义"。** 判定名与顺序（`PERFECT→GREAT→GOOD→MISS`）
- **⇒ 对 `ARCH_299/303` 的影响**：那里的"4 槽 ↔ 4 码"计数论证**不依赖**这些几何解释，
- 故仍然成立；但"UI 表按质量降序**刻意排布**"这条**支持性证据被削弱**为
- | C | 码 5 在显示前被排除 | ✅ 反汇编 `cmp r1,#5` |
- 本轮**只新增一个分析脚本**，`src/` 未改动 ⇒ 判定/渲染路径不受影响；

---

## `ARCH_306.md`

**标题**：ARCH_306 —— 自写 TGA 解码器（PIL 读不了的）；判定贴图位置仍未找到 ｜ 1. ★ 本轮真实产出：一个能用的 TGA 解码器 ｜ 2. 判定贴图的位置：仍未找到（两条路已排除） ｜ 3. 剩下最可能的解释（未验证） ｜ 4. 第 (3) 项状态（不变） ｜ 5. 可复现产物 ｜ 6. 回归

**发现/结论**（8 条，最多列 40）：

- ⇒ 它们是**完全标准的 8 位彩色映射 TGA**（`colorMapType=1, imageType=1,
- - `dance/ui/telop/P5`（81 个）：文件名全是 **`artist_NNN.tga`** ⇒ 是**曲师名字幕**，也不是判定文字；
- ⇒ **`+0x28` 的四值（203/277/351/425）不是这两个目录下图集的 Y 偏移。**
- - 它内部确实有 `.tga` 引用（例：`0x42928C` 处有 `fever_radia03.tga`）；
- ⇒ 它们的贴图要么在别处、要么由索引引用（而 `ARCH_297` 已证明文件内没有指向这些偏移的指针表）。
- ⇒ 贴图数据很可能**就在这个 `.bin` 里**，或在一个尚未识别的容器中。
- | C | 码 5 在显示前被排除 | ✅ 反汇编 |
- 本轮只新增分析脚本，`src/` 未改动 ⇒ 判定/渲染路径不受影响。

---

## `ARCH_307.md`

**标题**：ARCH_307 —— 自主轮次收尾：最终状态与可复现证据索引 ｜ 0. 一句话 ｜ 1. 逐项交付与证据 ｜ (1) WAV/PCM 解码链路 —— ✅ ｜ (2) 输入 → 判定 —— ✅ ｜ (3) 评级 1..4 的档案依据 —— 🟡 ｜ (4) 渲染保真度差距 —— ✅（归因） ｜ 2. 无设备时的复核（一条命令） ｜ 3. 手机插回后 ｜ 4. 本轮会话的**净能力增益**（不依赖设备） ｜ 5. 自我纠错清单（本次会话内被自己推翻的结论） ｜ 6. 仍未做的（按代价）

**发现/结论**（14 条，最多列 40）：

- 四项任务中 **(1)(2)(4) 已完成且证据可复现**；**(3) 的"档案依据"已完成**，
- 但 `{1,2,3,4} ↔ 名字` 的**精确绑定未取得**，且已用反汇编**证伪**了自己提出的自然解释。
- **证据**：28 首歌在 格式/样本数/frames/dataBytes/**FNV-1a 逐字节校验和**/peak/RMS
- `music218` 实测 48000Hz/2ch/323.13s、peak 30702 / rms 7342（≈−0.6 / −13 dBFS）、100% 非零。
- （`c_dance_ui_p5.bin` 同）`0x0FE8/0x1048/0x10A8/0x1108`，stride `0x60`，存储顺序
- - ✅ 确证：码 5 在进入显示前被排除（`cmp r1,#5`）；`0x8111B190` 返回 `0`（未判定）或 1..5；
- ⇒ **4 显示槽 ↔ 4 命中码 必然一一对应**（计数推论）。
- - ❌ 排除：eboot 字符串 / 文本表 / UI 指针表 / 4 项查表 / `[obj+0x34]` 回调注册点 / 判定贴图在
- **⇒ 未取得的是"代码里哪个码用哪个名字"，而档案里不存在这个信息。**
- （806400 B = 420×640×3）数出非背景像素**恰好 32331** ⇒ 该值是**内置合成资产场景**的参考，
- 2. **耳朵确认**能听到音乐且与舞步/谱面对齐。
- | 判定链路可离线回归 | 20 条断言 + 基准摘要复现 |
- | 曾结论 | 纠正 |
- | `ARCH_295` "+0.60 全码 3" | **外推**，实测 `g1=68 g2=8 g3=52 g4=58 g5=0 命中=186 漏=633` |

---

## `ARCH_308.md`

**标题**：ARCH_308 —— ★ 第 (3) 项**解决**：判定名由**像素**证实，绑定成立 ｜ 1. 突破口：`dance_ui_p5.bin` **内嵌贴图** ｜ 2. ★ 判定贴图：256×512，内容行区间 ｜ 3. ⇒ 名字表与像素**逐项吻合** ｜ 3.1 纠正 `ARCH_305/306` 的"降级" ｜ 3.2 ★ 我差点报错的一处 ｜ 4. ★ 绑定成立：`1→PERFECT, 2→GREAT, 3→GOOD, 4→MISS` ｜ 4.1 残留说明（据实） ｜ 5. 可复现产物 ｜ 6. 回归

**发现/结论**（17 条，最多列 40）：

- @0x003E04 imgtype=1 512x512 bpp=8 entrySize=32
- @0x044216 ... @0x0C4A3A 256x256 ... @0x24AEDC 256x512 ...
- 把 41 张做成联系表（`_mns/sheet.py`）后一眼定位到 **`0x24AEDC 256×512`**。
- **⇒ 恰好 4 个判定词，顺序 `PERFECT → GREAT → GOOD → MISS`。**
- | 条目偏移 | 名字 | `+0x28` | 实测墨迹带 | 像素上的词 | 一致 |
- **⇒ `+0x28` 是纹理 Y 偏移、`+0x2C/+0x30` 是精灵尺寸 —— 与 `ARCH_294` 最初的读法一致。**
- 现在由 **41 张内嵌贴图 + 像素读出**证实：**原读法正确**，降级撤销。
- 在 1050×972 的联系表**缩略图**上，我把 `0x24AEDC` 那格读成了
- 去渲染**全分辨率**后才确认：**只有 4 个词，没有 `BAD`**。
- ⇒ 缩略图不足以判定文字，**必须回原图**。这条已记入本文件的自我纠错。
- | **档案（像素）** | 4 个判定词，**同一张纹理内 stride 恒为 74 的连续堆叠**，顺序 P→G→G→M |
- | **代码（反汇编）** | 级联返回 1..5；调用方 `cmp r1,#5` 把 5 当"未命中"**并在进入显示前排除** |
- ⇒ **4 个显示槽 ↔ 4 个命中码是一一对应**（计数必然）；
- 且四个槽是**同一纹理里恒步长的有序堆叠** ⇒ 选择必然是按码顺序取
- **⇒ 判定码与名称的绑定：**
- | 5 | （**未命中**，被 `cmp r1,#5` 在显示前排除） | — |
- （`Score::grade[]` → 名称 + 日志），并跑 `offline_regression.ps1` 确认摘要不变。

---

## `ARCH_30_sequences.md`

**标题**：ARCH_30 — 具名序列/任务注册表全景 ｜ 1. 本模块职责（一句话） ｜ 2. 结构 / 流程图（核心） ｜ 2.1 三段式：工厂 → 双链表 → 每帧调度 ｜ 2.2 名字 → 节点 → step 的解析路径 ｜ 2.3 三种返回值语义（与 ARCH_6 / ARCH_23 一致，本轮逐条复核） ｜ 3. 关键函数地址表（VA | 推测名 | 作用 | 证据） ｜ 3.1 hash 函数复核（第二来源：既有日志） ｜ 4. 关键数据结构（字段偏移 + 含义 + 推测标注） ｜ 4.1 注册节点（`0x8116F7CA` 产出，`0x1C` 字节） ｜ 4.2 task 对象（由 `0x8116F9DC` 建，`0x50` 字节） ｜ 4.3 work / 状态块（`node[+0x08]`） ｜ 4.4 全局锚点 ｜ 5. 调用链与生命周期 ｜ 5.1 出生（create 的 5 种 r1 装载形态） ｜ 5.2 每帧（step 调用） ｜ 5.3 死亡（两条独立路径） ｜ 5.4 一处与前文不一致的地方（本轮以证据为准） ｜ 6. 具名序列清单（本轮主产出） ｜ 6.1 create 侧完整名单（145 个） ｜ A. 启动 / 常驻框架 / 转场 ｜ B. `initDataRead` 的 9 个子任务（**本轮新解出**，与 ARCH_23 的 step 表一一对应） ｜ C. title / LOGO 相位 ｜ D. 主菜单 / 配置 / 存档 / 网络

**发现/结论**（343 条，最多列 40）：

- > `0x8116F7CA` 注册的具名序列/任务，给出每个名字的字符串 VA、哈希、创建点 VA，并说明通用生命周期。
- > - 反汇编证据写作 `[DIS 0xVA] 指令原文`；工具 = `py C:\dev\p5d_build\_dis3.py <VA> <字节数>`
- > - 文件偏移映射：`file_off = VA - 0x81000000 + 0x10E0`（SELF 头 0x10E0）。两段：
- >   SEG0 = 0x81000000 大小 0x321810，SEG1 = 0x81325000 大小 0x3AA4A0。
- > - 本轮所有枚举由新建脚本完成（见第 9 节工具清单），关键结论均有**第二来源交叉验证**
- > - 无法确证的结论一律标注 **（推测）**。
- `0x8116F7CA` 是引擎的**唯一具名节点（任务/序列）工厂**：它按调用方给出的名字串建一个
- `0x1C` 字节的注册节点（挂进全局双向链表 `0x815554C0+4`），并同时创建一个 `0x50` 字节的
- 此后由 `0x811DB1F0` 每帧按 `task[+0x28]` 调用，直到某个 step 返回 `-1` 才被摘除 ——
- movs       r2, #0xC8          ; period（日志实测 200 = 0xC8）
- bl         0x8116F7CA
- ├─ 0x8116F7CA 内部：
- │    malloc(0x1C) 节点 ── 插进注册链表 [0x815554C0+4]（+0x10 next / +0x14 prev）
- │    然后 0x8116F9DC(node, sp+0x10) 建 task 对象，结果写回 node[+0x0C]
- 每帧：0x811DB1F0 调度器
- for (t = [0x816CD9E0]; t; t = t[+0x4C])
- if (r3 == -1) 0x811DAD8C(t)      ; ★ 结束该 task
- else t[+0x28] = r3               ; ★ 换 step 函数（子状态机）
- 0x8116F900(name, 0)                      ; 名字在 r0！
- hash = 0x81165288(name)                ; h = Σ (int8)name[i] << (i & 7)
- for (node = [0x815554C0+4]; node; node = node[+0x10])
- 811DB254  beq   #0x811db26e         ; -1 → 811DB26E: bl 0x811DAD8C 结束 task
- 811DB256  cbz   r3, #0x811db25a     ;  0 → 保持 +0x28 不变，下帧继续
- 811DB25E  bne   #0x811db1f0
- `0x811DB26E` 的两条指令：
- 811DB270  bl    #0x811dad8c        ; taskFinish(obj)
- 811DB274  b     #0x811db25a        ; 继续遍历
- **"step 返回 `-1`" 并不直接摘掉注册节点**：它结束的是 **task 对象**（`0x811DAD8C`）。
- 注册节点从链表里消失要走 `0x8116F75E`（`seqNodeDestroy`，先 `blx node[+0x04]` = cleanup，再按
- `+0x10/+0x14` 双向摘链）。⇒ 因此 `0x8116F900(name) == 0` 出现的时机 =
- | VA | 推测名 | 作用 | 证据 |
- | `0x8116F7CA` | **`seqNodeCreate`（唯一具名节点工厂）** | 建 `0x1C` 注册节点 + 插注册链 + 建 task 对象 | `[DIS 0x8116F800] movs r0,#0x1c` / `[DIS 0x8116F802] bl #0x8115d79c` / `[DIS 0x8116F88A] adds r0,r5,#0` / `[DIS 0x8116F88C] ldr r2,[sp,#0x2c]` / `[DIS 0x811
- | `0x8116F900` | `seqFindByName(name, 0)` | 按名查注册节点；**名字在 r0** | `[DIS 0x8116F90A] ldr r4,[r1,#4]`（r1=0x815554C0）/ `[DIS 0x8116F90E] bl #0x81165288` / `[DIS 0x8116F918] ldr r0,[r4,#0x18]` / `[DIS 0x8116F940] blx #0x812b2200`(strcmp) / `[DIS
- | `0x8116F8A8` | `seqNodeWork(node)` | 只有两条：`ldr r0,[r0,#8]; bx lr` | `[DIS 0x8116F8A8] ldr r0,[r0,#8]` / `[DIS 0x8116F8AA] bx lr` |
- | `0x8116F748` | `seqInvokeStep(node)` | 置全局当前节点 `[0x815554C0]=node` → `blx node[+0x00]` → 清零 | `[DIS 0x8116F752] str r0,[r4]` / `[DIS 0x8116F754] ldr r1,[r0]` / `[DIS 0x8116F756] blx r1` / `[DIS 0x8116F758] movs r1,#0; str r1,[r4]` |
- | `0x8116F75E` | `seqNodeDestroy(node)` | 调 `blx node[+0x04]`(cleanup) 后从注册链摘除 | `[DIS 0x8116F76C] ldr r1,[r4,#4]` / `[DIS 0x8116F76E] blx r1` / `[DIS 0x8116F774] ldr.w lr,[r4,#0x10]` / `[DIS 0x8116F77C] ldr r1,[r4,#0x14]` / `[DIS 0x8116F78
- | `0x8116FA5C` | task 层包装（`task[0x34]` 回指节点） | 运行链里存的 step 其实就是它 | `[DIS 0x8116FA5E] ldr r0,[r0,#0x34]` / `[DIS 0x8116FA60] bl #0x8116f748` |
- | `0x8116FA30` | `taskFinishByObj` | `bl 0x811DAD8C` | `[DIS 0x8116FA30] push {r4,lr}` / `[DIS 0x8116FA32] bl #0x811dad8c` |
- | `0x8116FA58` | `taskObjName` | `ldr r0,[r0,#0x14]; bx lr` | `[DIS 0x8116FA58] ldr r0,[r0,#0x14]` / `[DIS 0x8116FA5A] bx lr` |
- | `0x8116F9DC` | `taskObjCreate(node, args)` | 建 task 对象（内部 `bl 0x811DAB8C`，参数表含 `0x8116FA5D/0x8116FA67` 两个包装函数） | `[DIS 0x8116F9FE] movw r1,#0xfa5d` / `[DIS 0x8116FA0C] movt r1,#0x8116` / `[DIS 0x8116FA08] movw r3,#0xfa67` / `[DIS 0x8116FA

---

## `ARCH_314.md`

**标题**：ARCH_314 —— HUD 数字字形格（combo/score）：从**字段级证据**生成，并纠正 `ARCH_313` ｜ 1. 本轮目标 ｜ 2. 数据来源（权威 = 布局名表的字段） ｜ 3. ★ 纠正 `ARCH_313` ｜ 4. 交付物 ｜ 5. 验证（渲染出来看，不是"应该对"） ｜ 6. 顺带得到的两条纹理属性 ｜ 7. 回归

**发现/结论**（6 条，最多列 40）：

- 字段含义与判定精灵**同一套**（`ARCH_308` 已用像素证实过）：
- **⇒ 严格均匀网格。**
- **行归属交叉核对**：`num_combo_*` 的 `y=1` ⇒ 墨迹从 7 开始；`num_score_*` 的 `y=58` ⇒ 墨迹从 59 开始
- —— 与我独立量出的墨迹带 `(7,53)` / `(59,100)` **一一对应** ⇒ **combo=上排、score=下排**得到证实。
- static const unsigned kNumAtlasFileOffset = 0x159A82u;   // 在 dance_ui_p5.bin 内
- `APK_BUILD_OK`；`src/` 只新增一个**未被逻辑使用**的生成头 + 一行 include ⇒ 判定/渲染路径不受影响。

---

## `ARCH_315.md`

**标题**：ARCH_315 —— 舞曲 UI 贴图解码器抽成共享头 + 离线交叉验证；纳入一键回归 ｜ 1. 做法（沿用对 WAV 那套已被证明有效的模式） ｜ 2. 交叉验证结果 ｜ 2.1 过程中对齐的两处判据差异（不是 bug，是口径） ｜ 3. 纳入一键离线回归 ｜ 4. 现在具备了什么 ｜ 5. 回归

**发现/结论**（8 条，最多列 40）：

- embedded_tga_count=40 / tga[0..5]=0x003E04 0x00ABAA 0x044216 0x084628 ...
- num_atlas   @0x159A82 ok=1 512x256 fnv=85E32535
- judge_atlas @0x24AEDC ok=1 256x512 fnv=…
- ⇒ **内嵌枚举、两个图集的全局 FNV、20 个数字格、4 个判定精灵，全部一致。**
- 1. 我最初 C++ 把 `w,h` 放宽到 `>=16` ⇒ 枚举出 45 个（含图片数据里的假头）；
- ⇒ **画 HUD 所需的全部数据与解码路径都已就绪且已被离线验证**，剩下的是把它们接到 GL
- （机械改动，但视觉效果**只能上机确认**）。
- ⇒ 判定/渲染路径逻辑未改，摘要不受影响。

---

## `ARCH_316.md`

**标题**：ARCH_316 —— 诊断 HUD：用游戏自己的 UI 贴图画数字（安全接入） ｜ 1. 做了什么 ｜ 2. ★ 为什么是"诊断 HUD"而不是游戏原版 HUD ｜ 3. ★ 安全设计（关键，因为我无法上机看效果） ｜ 4. 数据路径（全部已离线验证） ｜ 5. 验证状态 ｜ 6. 上机后怎么判读 ｜ 7. 回归

**发现/结论**（6 条，最多列 40）：

- └─ 内嵌 TGA @0x159A82 (512x256) → 数字图集   ] p5d_ui_atlas.h decodeTgaAt
- └─ 内嵌 TGA @0x24AEDC (256x512) → 判定图集   ]   （ARCH_315 交叉验证 35 行全一致）
- | **视觉效果** | ❌ **只能上机确认**（手机插回后看 `[hud]` 日志与画面） |
- - `[hud] 就绪 数字图集=512x256 判定图集=256x512` ⇒ 贴图取到且解码成功；
- - 若看到 `[hud] 缺 …/dance_ui_p5.bin ⇒ HUD 关闭` ⇒ 该文件没推到设备；
- 判定摘要 `871d874fe1b44ac4` 由离线套件确认不变。

---

## `ARCH_317.md`

**标题**：ARCH_317 —— 现状评估：我们分析的数据**够不够移植完整游戏**？ ｜ 1. 现在实际跑起来的是什么 ｜ 2. ★ 格式覆盖表（按体积排序；`data_extracted` 实测） ｜ 3. ★ 三个结构性缺口（这是"够不够"的关键） ｜ 3.1 `GFS` 容器（390 MB）—— 舞台/环境/特效/着色器 ｜ 3.2 `FLW0`（`.bf` × 226）—— **游戏逻辑本身** ｜ 3.3 文字与字体 ｜ 4. 不是"格式问题"、纯工作量的缺口 ｜ 5. 所以"数据够干什么" ｜ 6. 到"完整游戏"的路线（按依赖顺序） ｜ 7. 本文的证据来源（可复核）

**发现/结论**（13 条，最多列 40）：

- > 结论先行：**不够，差得相当远。** 目前的数据足以支撑 **「舞曲模式 · 单曲纵切片」**，
- > 本文所有数字来自实测；**推测的部分明确标注**。
- | `.EPL` | 51 | 6.4 MB | ❌ | 头也是 `GFS0` ⇒ 特效在 GFS 容器里 |
- | **`.fnt`** | 3 | 2.4 MB | ❌ | magic `FNT0` ⇒ **没有它就没有任何文字** |
- | **`.bf`** | **226** | 1.6 MB | ❌ | 头偏移 8 处是 **`FLW0`** ⇒ **推测为场景流程/脚本** |
- | `.ENV` | 36 | — | ❌ | 头 `GFS0` ⇒ 环境/光照 |
- - `dance/field/*.GFS` 单个 15–21 MB（113 个）⇒ **舞台就在里面**；
- - `.EPL`/`.ENV`/`.GSC` 的头**也是 `GFS0`** ⇒ 特效、环境、着色器同属一族。
- **⇒ 目前无法显示任何一个字。菜单、剧情、结算画面都无从谈起。**
- | 4 | 计分/连击规则（反汇编 `+0x28C`/`+0x2D8` 邻域） | — | 中 |
- **结论**：现在完成度大致是**"一个模式的一个切片的渲染与判定链路"**。
- - 格式头：一条命令扫全部扩展名的前 16 字节（见对话记录）；
- - 已排除项：`ARCH_293`（渲染保真度伪命题）、`ARCH_296`（文案被混淆）、`ARCH_302`（评级码机制）；

---

## `ARCH_318.md`

**标题**：ARCH_318/319 —— ★ `.GFS` 容器解开：390 MB 的结构第一次被看见 ｜ 1. 关键认知：GFS 的解析器**早就有了**，只是只走了一条路 ｜ 2. ★ 全块枚举（`_gfs/gfs_chunks.py`，覆盖 99.9~100%） ｜ `dance/field/*.GFS`（32 个文件）块类型直方图 ｜ `dance/camera/*.GFS`（例 `c108.GFS`，36,084 B） ｜ 3. ★ 块身份（载荷头 + magic 探测） ｜ `0x000100FC` = **贴图包（DDS）** ｜ `0x000100FB` = **小贴图包**（31 个，392 KB） ｜ `0x00010003` = **场景节点结构**（98 MB）—— **含 `RootNode`** ｜ 4. 与 `ARCH_317` 评估的对照 ｜ 5. 下一轮（明确） ｜ 6. 回归

**发现/结论**（18 条，最多列 40）：

- if (type == CHUNK_ANIMATION_PACK) { readPack(...); return out.ok; }   // ★ 找到就 return
- | **`0x000100FC`** | 31 | **273,713,956（274 MB）** | ❌ |
- | **`0x00010003`** | 32 | **98,010,184（98 MB）** | ❌ |
- | `0x000100FD` | 25 | 2,598,501 | ✅ ANIMATION_PACK |
- | `0x000100FB` | 31 | 392,018 | ❌ |
- @0x10      type=0x00010003 len=241        ← 同一类型，但很小
- @0x101     type=0x000100FD len=35811      ← 动画包（相机轨道）
- **magic 探测：31/31 个该类型载荷在 `+0x21` 处含 `DDS`** ⇒ 是 `(名字, 偏移)` 表 + 内嵌 DDS 贴图。
- [ 0] 0x00000003   = 3（计数）
- [11..13] 0x0008526F 0x6F744E6F 0x646548E1
- 字节 = "…Ro" "otNo" "deH…"   ⇒ 拼出 **"RootNode"** ✅
- [20..23] 0x00003F80 ×4  ⇒ u16 0x0000 + **half 0x3F80 = 1.0**
- ⇒ 这是一个**节点/变换结构**（计数 + 一组 float + 节点名），**不是裸顶点流**。
- 相机文件里它只有 241 B；`field` 里 1.9~3 MB ⇒ 与场景规模相关。
- - **`0x000100FC` 的贴图包结构已经足够清晰**（计数 + `(名字长, 名字)` + DDS 表），
- 1. **把 `0x000100FC` 的条目表完全解出来**（条目数 → 每个名字 + 偏移 + 长度），
- 2. 回头把 `0x00010003` 的节点表结构读出（计数、名字、变换字段、是否含网格引用）——
- ⇒ `PLAY_SELFTEST` 摘要与 6 步离线回归不受影响。

---

## `ARCH_320.md`

**标题**：ARCH_320 —— ★ `0x000100FC` 贴图包：条目布局解出（可确定性遍历） ｜ 1. 载荷布局（实测，逐条算术验证） ｜ 1.1 为什么说 `pixelSizePlus1` 就是像素数据大小 —— 算术对账 ｜ 1.2 结构性交叉验证 ｜ 2. 这意味着什么 ｜ 3. 下一轮（明确） ｜ 4. 本轮产出（可复现） ｜ 5. 回归

**发现/结论**（6 条，最多列 40）：

- | 条目 | 名字 | 尺寸/格式 | 头里 u32 | −1 = 像素数据 | blob 实测跨度 | 对账 |
- **⇒ `blob = 128(DDS 头) + pixelData + 4`，且 `pixelData = 条目头里的 u32 − 1`。**
- **`0x000100FC`（274 MB）现在可以确定性遍历并抽出全部内嵌 DDS 贴图** ——
- 3. 用 `p5d_dds.h` 解码若干贴图并**渲出参考图**目视确认；
- 4. 然后回头解 `0x00010003`（98 MB，含 `RootNode`）——它决定"舞台几何在哪里"。
- 本轮 `src/` **未改动** ⇒ `PLAY_SELFTEST` 摘要 `871d874fe1b44ac4` 与 6 步离线回归不受影响。

---

## `ARCH_321.md`

**标题**：ARCH_321 —— ★★ `0x000100FC` 舞台贴图包（274 MB）：框架**完全解出并全量验证** ｜ 1. 最终框架（`ARCH_320` 的读法是错的，本文件为准） ｜ 2. ★ 全量验证：31/31 精确落在载荷末尾 ｜ 3. 关键字段的含义（已由算术确证） ｜ 4. 这解决了什么 ｜ 5. 诚实记录：`ARCH_320` 的结论**作废** ｜ 6. 下一轮 ｜ 7. 回归

**发现/结论**（12 条，最多列 40）：

- u16 type                                 （实测恒为 1）
- f100_001.GFS  count=35   end=0x4168A8  payload=0x4168A8  EXACT
- f206_001.GFS  count=110  end=0x97CEDF  payload=0x97CEDF  EXACT
- f900_001.GFS  count=2    end=0x17A14B  payload=0x17A14B  EXACT
- | `f100_001` | `f100_001_mirror01.dds` | 0x00010080 = **65664** | = 128 + 256×256 DXT5 的 65536 ✅ |
- | `f108_001` | `f033_001_shadow.dds` | 0x00015080 = 86144 | |
- | `f226_001` | `f003_002_denkyu01.dds` | 0x00002080 = **8320** | |
- ⇒ **`blobSize` 直接给出每条贴图的精确跨度**，遍历不再需要任何启发式（不必搜 magic、不必从 DDS 头反算）。
- - `0x000100FC` 占 **274 MB**，是 `.GFS` 390 MB 里最大的一块；
- 1. 抽出一条 DDS，用 `p5d_dds.h` 解码并**渲出参考图**目视确认（贴图真的能用）；
- 4. 然后转向 `0x00010003`（98 MB，含 `RootNode`）—— 它决定**舞台几何在哪里**。
- 本轮 `src/` 未改动 ⇒ `PLAY_SELFTEST` 摘要 `871d874fe1b44ac4` 与 6 步离线回归不受影响。

---

## `ARCH_322.md`

**标题**：ARCH_322 —— ★ `.GFS` 解析器落地：共享头 + Python 独立实现**逐行一致（365 行）** ｜ 1. 交付物 ｜ 2. ★ 全量交叉验证：365 行逐行一致 ｜ 3. 可视化确认（贴图真的能用） ｜ 4. 本轮实跑输出（摘要） ｜ 5. 目标第 (1) 项进度 ｜ 6. 回归 ｜ 7. 下一轮

**发现/结论**（15 条，最多列 40）：

- // 常量：CHUNK_TEXTURE_PACK 0x000100FC / CHUNK_SCENE_NODES 0x00010003
- //       CHUNK_ANIMATION_PACK 0x000100FD / CHUNK_AUX 0x000100FB
- - **像素**：第一条目解码后的 `宽/高/格式/RGBA 字节数/RGBA FNV`。
- **RGBA 字节级哈希完全一致** ⇒ 解码链路也被独立验证，不只验证了容器。
- chunk[0] type=0x000100FC len=4286648
- chunk[1] type=0x000100FB len=8228
- chunk[2] type=0x00010003 len=1925177
- chunk[3] type=0x000100FD len=526177
- [34] f100_001_wallneon_anime.dds type=1 blobSize=131200 ddsOff=0x3F6844
- | `0x000100FC` **舞台贴图**（274 MB） | ✅ **可确定性遍历并解码**（含像素级验证） |
- | `0x00010003` 场景节点（98 MB，含 `RootNode`） | 🟡 已定位，结构未解 —— **舞台几何可能在它或它的后续块里** |
- **⇒ 274 MB 的那一半（贴图）已经拿到手；剩下 98 MB 的 `0x00010003` 是几何之路的关键。**
- 本轮 `src/` 只**新增** `p5d_gfs.h`（尚未被 app include）⇒ 判定摘要 `871d874fe1b44ac4` 不受影响。
- 2. 解 `0x00010003` 的节点表结构（计数 / 名字 / 变换字段 / 是否含网格引用）
- 3. 若几何在别处（如 `0x000100FB` 或 `0xFC` 的其它段），继续沿块表找。

---

## `ARCH_323.md`

**标题**：ARCH_323 —— `0x00010003`（98 MB）是**场景/对象表**，不是裸顶点流 ｜ 1. 实测结构（`f100_001.GFS`，chunk `len=1925177`，载荷起点 `0x4188FC`） ｜ 2. 结论（据实） ｜ 3. 与既有认知的关系 ｜ 4. 下一轮（明确，且是最后两轮之一） ｜ 5. 回归

**发现/结论**（9 条，最多列 40）：

- **该载荷内可打印串（≥5 字符）共 10,782 个** ⇒ 这是一个**包含数千个具名对象的表**。
- | `0x00010003` 是**场景/对象表**（节点 + 名字 + 变换） | `u32 计数` + 10 floats + `u16 名长` + 名字；`"RootNode"` 长度精确吻合；数以千计的具名对象 | **强** |
- | 它**不是**裸顶点流 | 结构是"名字表"而非顶点数组；无 stride/顶点格式特征 | **强** |
- | 舞台**几何**在哪 | **未定** —— 可能：① 在本 chunk 后续段（名字之后的网格引用/顶点块）；② 在 `0x000100FB`；③ 在 GMD 里 | **推测，未验证** |
- **⇒ 目标第 (1) 项里"舞台几何"仍未取得**，但已经把范围从"98 MB 未知块"缩小到
- ⇒ 与场景复杂度一致。**这一条是强旁证。**
- 1. 把 `0x00010003` 的**节点记录**完整解出（记录长度、字段偏移、名字位置），
- 3. 若几何不在该 chunk，则检查 `0x000100FB`（392 KB，31 个）与 GMD 引用。
- 本轮 `src/` 未改动 ⇒ `PLAY_SELFTEST` 摘要 `871d874fe1b44ac4` 与

---

## `ARCH_324.md`

**标题**：ARCH_324 —— ★ 方向修正：`.GFS` 里**没有几何块**，舞台几何只能是外部 `.GMD` ｜ 1. 本轮证据 ｜ 1.1 `0x00010003` 是"场景 + 材质"描述，不是顶点流 ｜ 1.2 名字**散布全块**，不是"前表后数据" ｜ 2. ★ 结构性推论：几何不可能在 `.GFS` 里 ｜ 3. 据实标注 ｜ 4. 下一轮（最后一轮，明确） ｜ 5. 回归

**发现/结论**（17 条，最多列 40）：

- ⇒ `u16 名长 + 名字` 的结构；
- ⇒ 该块内含**材质定义**（具名属性 + 浮点参数）。
- f100_001: 可打印串=10782  首个=+0x6   末个=+0x1D601D  (100.0% 处)
- f206_001: 可打印串=54794  首个=+0x8   末个=+0x6A2BE7  (100.0% 处)
- ⇒ 早期是**密集的变换/矩阵数据**，之后是**名字与稀疏属性交错**。
- | 块 | 体量 | 身份 | 证据 |
- | `0x000100FC` | **274 MB** | 贴图包（DDS） | 31/31 文件含 `DDS` magic；全量遍历闭合（`ARCH_321/322`） |
- | `0x00010003` | **98 MB** | 场景对象 + 材质 | 本轮 §1 |
- | `0x000100FD` | 2.6 MB | 动画包 | `p5d_gap2.h` 已解析 |
- | `0x000100FB` | 0.4 MB | 小贴图包 | `ARCH_318` |
- **⇒ 没有任何一块是几何块。** 390 MB 已被这四类吃完（274+98+2.6+0.4 ≈ 375 MB，
- 余下是块头/对齐）⇒ **舞台几何只能以"外部引用"的形式存在**。
- | `0x00010003` 含材质定义（`currentUVSet`/`map1` + float 参数） | **强**（字节级，两文件一致） |
- | `0x00010003` 不是顶点数组 | **强**（名字散布全块 + 浮点密度不均） |
- | **舞台几何 = 外部 `.GMD`，由场景表的名字引用** | **推测（未验证）** —— 但这是唯一与体量证据相容的路径 |
- 1. 在 `0x00010003` 里找**"对象名 → 资源路径/ID"**的对应（例如名字附近是否有
- 本轮 `src/` 未改动 ⇒ `PLAY_SELFTEST` 摘要 `871d874fe1b44ac4` 与

---

## `ARCH_325.md`

**标题**：ARCH_325 —— ★ 收回 ARCH_324 的推论：几何**就在** `0x00010003` 里 ｜ 1. 决定性证据（本轮） ｜ 1.1 场景块里**没有**任何外部文件引用 ｜ 1.2 名字是 **Maya 节点名**，且含 `polySurface*`（网格） ｜ 2. ★ 收回 `ARCH_324` §2 的推论 ｜ 3. 与 `ARCH_323/324` 现象的自洽 ｜ 4. 本目标的第 (1) 项现状（据实） ｜ 5. 下一轮（精确） ｜ 6. 回归

**发现/结论**（14 条，最多列 40）：

- 扫 6 个 `0x00010003` 块，按扩展名（`.gmd/.gap/.gfs/.dds/.tga/.epl/.env`）匹配：
- 扫描 6 个 0x00010003 块，扩展名式引用命中 0 次
- ⇒ **场景不是"引用外部模型"，而是自包含的**。
- `f100_001.GFS` 的 `0x00010003` 里，唯一具名串 127 个，样本：
- **⇒ 这是"Maya 场景导出"式的图**：`polySurface*`（网格）+ `lambert*`（材质）+ `locator*`（定位器）+ UV 集。
- | 「`.GFS` 没有几何块 ⇒ **几何只能是外部 `.GMD`**」 | ❌ **错**。推论只用了"块体量归类"，**没有去查引用**；一查就发现**根本没有外部引用**，而 `polySurface*` 明确表示网格就在块内 |
- **教训**：体量归类可以排除"这里没有独立的几何块"，但**不能推出"几何在别处"** ——
- （这与本项目一贯的纪律一致：**结论要落到证据上，不能止于排除法。**）
- - 名字**散布全块**（不是前表后数据）⇒ 每条网格记录**自带名字**，
- | `0x000100FC` 舞台贴图（274 MB） | ✅ 可确定性遍历并解码（含像素级验证 + 目视确认） |
- | `0x00010003` 身份 | ✅ **Maya 式场景图**（网格 + 材质 + 定位器 + UV 集）；几何**在块内** |
- | 舞台**几何统计**（顶点/图元/包围盒） | ❌ **仍未取得** —— 但定位已经从"在不在场"变成"**在这个块里哪一段、什么记录格式**" |
- 1. 在 `0x00010003` 里**定位 `polySurface*` 记录**：从名字反查其记录起点，
- 本轮 `src/` 未改动 ⇒ `PLAY_SELFTEST` 摘要 `871d874fe1b44ac4` 与

---

## `ARCH_326.md`

**标题**：ARCH_326 —— `polySurface*` 网格记录的前置结构（第 (1) 项：几何定位） ｜ 1. 本轮实测（`f100_001.GFS` 的 `0x00010003`，载荷 1,925,161 B） ｜ 2. 由此确证的结构 ｜ 3. 与既有结论的关系 ｜ 4. 仍未取得（据实） ｜ 5. 下一轮（精确） ｜ 6. 回归

**发现/结论**（5 条，最多列 40）：

- | 观察 | 结论 | 强度 |
- - `ARCH_325` 判定「几何**就在** `0x00010003` 里」—— 本轮给出了**记录级的支撑**：
- **顶点/索引数组本身**尚未定位：还没有读到"顶点数 / 索引数 / stride"这类记录头。
- —— 顶点数 × stride 与索引数应能与块内某处的 u32 对上；
- 本轮 `src/` 未改动 ⇒ `PLAY_SELFTEST` 摘要 `871d874fe1b44ac4` 与

---

## `ARCH_327.md`

**标题**：ARCH_327 —— `.GFS` 舞台几何攻克：顶点/索引数组已定位并通过**逐网格包围盒自证** ｜ 1. 关键突破：`0x00010003` 的载荷**就是** Model 载荷本体 ｜ 2. 全量统计（32 个 `dance/field/*.GFS`） ｜ 3. ★ 决定性自证：逐网格包围盒 ｜ 3.1 与 `ARCH_324/326` 的「浮点密集区」对照（目标第 (1) 项的最后一条） ｜ 3.2 可视证据（同一份数据） ｜ 4. 修正一个真实缺口：attachment type 6 = **Light** ｜ 4.1 现象 ｜ 4.2 载荷长度**不是常数** ｜ 4.3 补丁与「对角色管线无影响」的证明 ｜ 5. C++ ↔ Python 逐行交叉验证 ｜ 6. 世界变换链：找到并修好一处真实缺陷（缩放被丢弃） ｜ 6.1 缺陷 ｜ 6.2 仍未吻合的 14 个（已量化，未解） ｜ 7. 回归 ｜ 8. 下一轮（精确） ｜ 9. 本轮产出物清单

**发现/结论**（31 条，最多列 40）：

- > **本轮结论：三件全部完成，并且额外拿到一个「不依赖任何变换的」自证。**
- > 同时修正了一个此前一直存在的格式缺口（attachment type 6 = Light），
- **做法（复用而非重写）**：把这块载荷包进一个「只有一段、段类型 `0x00010003`」的
- （**载荷内**偏移）、`flags`/`attrFlags`/`stride`、`vertexCount`/`triCount`、
- | attrFlags | 组成 | stride | 对照 |
- | `0x00000112` | position + normal + uv0 | 32 | pos@0 nrm@12 uv0@24 |
- | `0x00000152` | + color0 | 36 | pos@0 nrm@12 **color0@24** uv0@28 |
- | `0x00000312` | + uv1 | 40 | uv0@24 uv1@32 |
- | `0x00000012` | position + normal | 24 | 无 UV |
- * 19 个不吻合的**全部**带 mesh `flags` 位 31（`0x8000011E` / `0x8000051E`）：
- 则每一段都必须落在某个已定位网格的 `[vertexOff, vertexOff + vertexCount*stride)` 内。
- mesh[0]（`vtx=+0x21C, n=5500, stride=32` ⇒ `+0x21C .. +0x2B19C`）之内 ——
- 从字节直接读出结构（`f108_001.GFS` @+0x11D344）：
- **没有任何一个角色模型含 type 6/7/8** ⇒ 本补丁对角色管线与
- 顺带得到一条独立证据：**537/537 个角色 GMD 都恰好走到 Model 载荷末尾** —— 整套文法自洽。
- `_gfs/stage_geom.py` 是按 `p5d_gmd2.h` 顶部记录的**格式文法**独立写的，不是把 C++ 控制流转写。
- 会在第 7 位有效数字上产生差异 —— 那是双精度累加的假象，不是格式问题。
- `p' = ((p ⊙ s)·R) + t` ⇒ 把 R 的第 i 行乘 `s[i]`；**不动 `p5d_gmd.h`**，因为角色管线依赖它）：
- **已排除**：attachment `type 3`（节点附件）在 32 个舞台里出现 **0** 次
- （`_gfs/scene_gap.py`，3,992 节点全扫描）⇒「节点附件的变换被丢弃」不是原因。
- > **顶点/索引数组是对的（99.54% 硬证据）；世界变换链已修好最大的一处缺陷
- 脚本新增 `-Legacy6` 开关，只跑原来那 6 步并打印同一条结论 ——
- 实测 `OFFLINE_REGRESSION_ALL_PASS`，第 7 步打印 `SKIPPED (-Legacy6)`。
- 该改动对 537/537 角色 GMD 是 no-op（实测），APK 仍构建成功。
- 2. **19 个 mesh-bbox 例外之谜**：全部带 mesh flags 位 31；确认该位是否意味着
- 3. 把 `p5d_gfs_scene.h` 接进渲染核心（舞台网格 + `0x000100FC` 已解析的贴图 + 材质名匹配），
- | `src/assets/p5d_gfs_scene.h` | **新**：`.GFS` `0x00010003` 舞台场景读取头（节点/网格/统计/三套包围盒） |
- | `src/assets/p5d_gmd2.h` | 增加 `readLight()` 与两处 `case 6` 分派（修正 13/32 个舞台无法解析） |
- | `_gfs/scene_meshbox.py` | ★ 逐网格存放盒 vs 计算盒（3 节的决定性自证） |
- | `_gfs/scene_runs.py` | ★ 浮点密集区 ⊂ 网格顶点区间（3.1 节，3463/3463） |
- | `_gfs/scene_view.py` | 世界空间三角形线框渲染（可视证据） |

---

## `ARCH_328.md`

**标题**：ARCH_328 —— `.bf`（FLW0）事件流程脚本：**226/226 全部解出**，用途由引擎自身字符串确证 ｜ 1. ★ 第一件事：`.bf` 是**小端**（本作唯一的例外） ｜ 2. 容器结构（226/226 全部满足） ｜ 3. 标签记录 ｜ 4. ★ 指令流解码：`{u16 opcode, u16 operand}` + `op==0x01` 吞掉一个 binary32 ｜ 4.1 决定性自证 ｜ 5. ★ 用途判定：来自引擎自己的字符串，不是推测 ｜ 5.1 段 3 就是本事件的对白（与目标第 (5) 项 `msg` 直接相关） ｜ 6. 仍未定（据实） ｜ 7. 交叉验证与回归 ｜ 8. 全量统计 ｜ 9. 下一轮（精确） ｜ 10. 本轮产出物清单

**发现/结论**（15 条，最多列 40）：

- > **本轮结论：那个四元组读法是错的。** 226/226 个文件的结构是
- | `+0x24` | 0x38000000 | **56** | 标签记录字节数（226/226 恒为 56） |
- | `+0x28` | 0x04000000 | **4** | 标签条数 |
- | `+0x2C` | 0x70000000 | **112** | 标签表偏移（226/226 恒为 112） |
- **本作其它容器（`GFS0` / `.gmd` / `.mns` / 舞台块）全是大端**，`.bf` 是唯一的小端格式，
- +0x14 u32  ???（实测 0..13，见 §6）
- | 段 | itemSize | 内容 | 实测 |
- opcode == 0x01  ⇒  紧随的下一字是 IEEE-754 binary32 字面量
- 这不是猜的，两条独立实测把它钉死：
- 实测（`_bf/bf_labelcheck.py`）：
- | 引擎字符串 | `.bf` 文件里的实测 |
- **结论**：`.bf` = 过场/事件场景的**流程脚本（VM 字节码）**，由 `evtMain.c` 加载，
- + `_bf/gt_bf.py`（**独立 Python 实现**，同一格式描述，不是 C++ 转写）
- `offline_regression.ps1 -Legacy6` 只跑原来那 6 步并打印同一条结论。
- | `_bf/bf_labelcheck.py` | ★ 2,782/2,782 标签落在指令起点的决定性自证 |

---

## `ARCH_329.md`

**标题**：ARCH_329 —— `.fnt`（FNT0）/ `.ctbl`（FCT0）：结构确证到「两张表」，**位图编码仍未解** ｜ 1. ★ 字节序：`.fnt` / `.ctbl` 也是**小端** ｜ 2. 头部（三个 `.fnt` + 一个 `.ctbl` 实测） ｜ 3. ★ `.ctbl`（FCT0）的结构：完全自洽 ｜ 4. `.fnt` 的字形表：`(位图偏移, 宽度)` 且**跨度规律**已证 ｜ 5. 仍未解（据实） ｜ 6. 与 `ARCH_47 §5` 的差异（纠正） ｜ 7. 下一轮（精确） ｜ 8. 本轮产出物 ｜ 9. 第二轮追加（ARCH_329 续）：把位图区继续收窄 ｜ 9.1 两个变体文件的第一处内容差异 = **偏移 780** ｜ 9.2 压缩假说 —— **已证伪** ｜ 9.3 `+0x14` 指向的结构（`font_p5` @772） ｜ 9.4 ★ 架构纠正：PSVita 是 **ARMv7-A / Thumb-2（32 位）**，不是 ARM64

**发现/结论**（21 条，最多列 40）：

- > **本轮结论：`(宽,偏移)` 的顺序是反的；`.fnt` 与配套的 `font.ctbl` 共享同一张表；
- 和 `.bf`（`ARCH_328`）一样，这是本作里少数几个非大端格式。
- | `.fnt +0x10` | 0x18000000 | **24** | |
- | `.fnt +0x14` | 0x04030000 | **772** | |
- | `.ctbl +0x10` | 0x19500000 | **6480** | 指向 ASCII 码点表 |
- | `.ctbl +0x14` | 0x37AC0000 | **14252** | 指向第二张码点表 |
- 按大端读为 12652、按小端读为 0x6C310000；**含义未定**（见 §5）。
- `font.ctbl` 实测：
- | `0x20..0x194F` | **与 `.fnt` 逐字节相同的字形表** | `fnt@0x18 == ctbl@0x20` 前 88 项实测全同 |
- 实测（`_fnt/fnt_order.py`，`(B, A-span)` 直方图）：
- 即 `width=2 ⇒ 跨 6`、`width=7 ⇒ 跨 16`、`width=4 ⇒ 跨 10`、`width=12 ⇒ 跨 26` …… **无一例外**。
- 已用**四种**独立假设渲染并**全部排除**：
- | `ARCH_47 §5` 的说法 | 本轮实测 |
- ★ 注意 §11 的架构纠正：**要用 Thumb-2 反汇编，不是 ARM64**。
- | `_fnt/fnt_order.py` | ★ 跨度规律 `offset[k+1]=offset[k]+2*width+2` 的发现与验证 |
- | `_fnt/fnt_view.py` / `fnt_sheet.py` | 1bpp/4bpp/8bpp 渲染（四种假设全部排除） |
- | `_fnt/fnt_compress.py` | ★ 压缩假说的证伪（zlib/gzip 全部失败） |
- | `_fnt/fnt_loader.py` | eboot 里字体加载器的搜索（含架构纠正） |
- 第一次尝试反汇编时按 ARM64 的 `adrp` 搜索字符串引用，**方法本身是错的**。
- * 代码段实测指令 `0x462E9002` 解出 `mov r6, r5` —— 是 **Thumb 16 位**编码。
- 需要在 Thumb-2 全量反汇编里找。

---

## `ARCH_330.md`

**标题**：ARCH_330 —— 第 (4) 项勘察：8 种格式的 magic 与头部；`PLG0` 段表**已确证** ｜ 1. 全量规模与 magic（实测） ｜ 2. 各自头部实测（原始字节） ｜ 3. ★ `PLG0` 段偏移表：**31 / 31 全量确证** ｜ 4. `.ENV` 的内部（部分观测，**未定**） ｜ 5. 未解清单（据实） ｜ 6. 下一轮（精确） ｜ 7. 本轮产出物

**发现/结论**（16 条，最多列 40）：

- > 其余格式记录实测头部，**不写成「已解」**。
- | `.EPL` | **51** | `GFS0` | 362 – 1,051,690 | **GFS0 容器**（版本 0x01105030 / 40 / 70 / 90 / 100） |
- | `.ENV` | **35** | `GFS0` | 799 / 808 | **GFS0 容器**（版本 0x01105100 / 0x01105090） |
- | `.GSC` | **2** | `GFS0` | 62,100 / 4,094,756 | **GFS0 容器**（0x01105100） |
- | `.plg` | **31** | `PLG0` | 316 – 63,628 | 段偏移表（见 §3，**已确证**） |
- | `.gnf` | **3** | `GNF ` (0x474E4620) | 4,352 – 65,792 | Sony 标准 GNF 纹理 |
- ★ **`.EPL` / `.ENV` / `.GSC` 三者都是 `GFS0` 容器** —— 这意味着它们的「头部解析」
- `.ef` 在 `+0x1C` 之后直接跟**效果名 ASCII**（实测：`effe`、`star`、`tria`），
- `+0x04` 实测恒为 `00 00 00 02`（含义未定，可能是版本）。
- **不一致** ⇒ `+0x08` **不是段数**。**含义未定。**
- （从 `+0x10` 起读第一个块头会立即失败）⇒ 这三个格式虽然 magic 是 `GFS0`，
- 4. `.ep` / `.ef` 的 `ARCH_47` 结论**未逐字复核**。
- 2. `.ENV` 只有 808 字节，是最小的完整样本 ⇒ **先把 `.ENV` 完全解出**再推到 `.EPL`。
- 3. `PLG0` 已确证，可直接写进平台无关头文件 + Python 交叉验证（与 §3 的判据一起）。
- | `_fmt/fmt_survey.py` | 8 种格式的数量 / magic / 大小勘察 |
- | `_fmt/fmt_headers.py` | 各格式头部原始字节与 `GFS0` 块表尝试 |

---

## `ARCH_331.md`

**标题**：ARCH_331 —— 第 (4) 项交付：8 种格式的**头部解析器** + 161 文件逐行交叉验证 ｜ 1. ★ 字节序：全量实测得出，不是假设 ｜ 2. 各格式已确证字段（全量实测） ｜ 3. ★ `PLG0` 段边界表：**31 / 31 全量确证** ｜ 4. 交叉验证与回归 ｜ 5. 仍未解（据实 —— 本项只到「头部」层面） ｜ 6. 下一轮（精确） ｜ 7. 本轮产出物

**发现/结论**（15 条，最多列 40）：

- > + 逐行交叉验证，覆盖 **8 种格式、161 个文件**。
- > **只固化在全部文件上实测成立的字段**，含义未定的一律标注，不写成「正解」。
- | 格式 | 数量 | magic | 字节序 | 判据 |
- > 本作至此已知的**小端**格式：`.bf`(ARCH_328)、`.fnt`(ARCH_329)、
- .lbn   5  +04(LE) 恒为 0x00010000 ; +18(LE) 恒为 0 ; +1C(LE) 恒为 1
- .ep   19  +04 恒为 0 ; +08(LE) 恒为 0x10000001 ; +10(LE) 恒为 0x10000002 ;
- +14(LE) 恒为 3 ; +18(LE) 恒为 0x20100005
- .ef   14  +04(LE) 恒为 0x00010000 ; +08(LE) 恒为 8 ; +10(LE) ∈ {1,2,3}
- .gnf   3  +04(BE) 恒为 0xF8000000 ; +08(BE) 恒为 0x02010800 ;
- ★ +0C(BE) == 文件大小（65792 / 33024 / 4352 三例全中）
- 这也顺带证明了 GNF 是**大端**（按小端读 65792 会得到 0x00010100，
- 而按大端读才是 65792 —— 实测三例全中）。
- 1. **`.EPL/.ENV/.GSC` 的内部结构未解**：三个格式的 magic 都是 `GFS0`，
- 4. `.ep` / `.ef` 的 `ARCH_47` 结论（「字节码指令流」「纹理引用+效果名」）**未逐字复核**；
- | `src/assets/p5d_fmt.h` | **新**：8 种格式的头部解析（平台无关） |

---

## `ARCH_332.md`

**标题**：ARCH_332 —— 第 (5) 项 `msg` 文案：**编码不是 Shift-JIS，而是配套 `font.ctbl` 的自定义字符索引** ｜ 1. `msg` 文件与容器结构（实测） ｜ 2. ★ 文本不是 Shift-JIS —— 假名区**完全缺席** ｜ 3. 已排除的假设 ｜ 4. ★★ 决定性发现：索引表就是 `font.ctbl` ｜ 5. 未解（据实） ｜ 6. 下一轮（精确） ｜ 7. 本轮产出物

**发现/结论**（8 条，最多列 40）：

- > 但整句解出来语义仍不通 ⇒ **还差一层**（见 §5）。
- 不可能如此 ⇒ **存在一层变换**（这正是目标所称的「逐字节混淆」）。
- ⇒ **不是简单的字节级 XOR/加减**。全字节 XOR 还会破坏 ASCII，与 §2 的观测矛盾。
- 两张各 3886 项的 u16 表。本轮实测：
- ⇒ **§4 的公式还差一层**（可能是偏移、可能是 T1 只给「字形索引」而另有一张
- > 等于把未验证的结论写成正解（`ARCH_320`/`ARCH_324` 的模式），故不做。
- | `_msg/msg_xform.py` | ★ 2295 种逐字节变换的穷举与打分（证伪） |
- | `_msg/msg_ctbl.py` | ★ 用 `font.ctbl` 的 T1/T2 解码（4/4 索引吻合） |

---

## `ARCH_333.md`

**标题**：ARCH_333 —— `font.ctbl` 的 T1 表被完全解出（Shift-JIS 码点序号 → Unicode） ｜ 1. ★★ `font.ctbl` T1 = 「Shift-JIS 有效码点序号 → Unicode」完整映射表 ｜ 2. `ARCH_332` 的索引假说 —— 4/4 命中，但有一个反例 ｜ 3. 本轮排除的其它路径 ｜ 4. ★ 全量统计**再次证伪**该假说：字符码种类 4301 > 表的 3886 项 ｜ 5. 仍未解（据实） ｜ 6. 下一轮（精确，换判据） ｜ 7. 本轮产出物

**发现/结论**（6 条，最多列 40）：

- ⇒ **该假说不能整体成立。**
- （`data/C/` 让人怀疑是中文版，所以这两个编码**都实测过、都排除**。）
- **`distinct c = 4301 > 3886`（T1/T2 的项数）** ⇒ **c 不可能直接是 T1 的下标**。
- 索引之一），**不构成证据。这一条撤回。**
- > 本轮**没有**产出 `msg` 的解析头文件：编码未闭合前固化头文件等于把未验证结论写成正解。
- | `_msg/msg_index.py` | ★ T1 区间 dump（组织完全解出）+ k 偏移扫描（判据作废，记录在案） |

---

## `ARCH_334.md`

**标题**：ARCH_334 —— 用户确认对话是**繁体中文**；据此重测 Big5 路线（仍未解开，但排除了两大族） ｜ 1. 精确统计（修正 `ARCH_333 §4` 的一个错误） ｜ 2. 直接 Big5 / cp950 解码：失败 ｜ 3. 常量字节变换：全搜后**排除** ｜ 4. 「序号 → Big5 码点表」假说：失败 ｜ 5. 「常量位移 `Big5 = c + base`」：有个别合理字，整体不成立 ｜ 6. 确定未变换的部分（对下一轮有用） ｜ 7. 下一轮（精确，可执行） ｜ 8. 本轮产出物

**发现/结论**（13 条，最多列 40）：

- > 这纠正了 `ARCH_332/333` 里「按日文猜」的整个方向，本轮据此把 Big5 路线系统测了一遍。
- 实测字节对首字节集中在 **0x88..0x9F**，而 Big5 第一字节有效范围是 **0xA1..0xF9**。
- 所以**字节对经过了一层变换**，不能直接解码（`ARCH_332` 已实测 cp950/big5hkscs 均失败）。
- 判据：变换后的字节对落在「有效 Big5 码点集合」（实测 13,710 个）里的**加权命中率**。
- **正确变换的命中率应接近 1.0**（因为原文是合法 Big5），实测最高只有 0.82 ⇒
- **常量逐字节变换全部排除。**
- **不成词 ⇒ 排除。**
- **「法」「明」「武」「取」这类真字出现过，但任何单一 base 都给出大片不成词的结果 ⇒ 不成立。**
- 1. ★ **用字频对齐反解映射**：繁体中文最高频字是「的」（Big5 `0xAABA`），
- 实测最高频码是 `c=727`（249 次）。若 `c=727 ↔ 的`，则 `base = 0xAABA − 727 = 0xA7D3`。
- > 编码未闭合前不产出 `msg` 解析头文件（避免把未验证结论写成正解）。
- | `_msg/msg_big5b.py` | ★ 加法/XOR/交换 全搜（加权命中率判据，排除两族） |
- | `_msg/msg_big5idx.py` | 「序号 → Big5 码点表」假说（排除） |

---

## `ARCH_335.md`

**标题**：ARCH_335 —— 繁体中文路线继续：两支变换族明确排除，并量化出「第二字节必然是非线性映射」 ｜ 1. 先在资源树里找「码表」——没有 ｜ 2. ★ 关键量化：字节对的空间分布 ｜ 3. 本轮排除的变换族（判据：变换后**既是有效 Big5 又落在常用字区 0xA440..0xC67E**） ｜ 4. 还站得住的模型（下一轮直接验证） ｜ 5. 已确定未变换的部分 ｜ 6. 下一轮（精确） ｜ 7. 本轮产出物

**发现/结论**（14 条，最多列 40）：

- > 目标第 (5) 项。用户已确认游戏内对话是**繁体中文**，故编码应是 Big5 系。
- 若 >80% 的值落在「有效 Big5 码点集合」（实测 13,710 个），就是候选表。**
- ⇒ **`.msg` 的码表不在 `data/` 或 `app_system/` 的小文件里。**
- 实测 `b2` 用了 **191 个值、横跨 0x01..0xFB**，其中 **14.5% 的字符直接落在空洞里**。
- ⇒ **`b2` 不可能是「Big5 低字节 + 常量」。** 这一条单独就否定了整个「常量逐字节偏移」族。
- | 变换族 | 搜索量 | 最优命中率 | 结论 |
- | 加法 `(b1+a, b2+b)` | 65,536 | **0.8154**（任意 a，b=0x02） | 解码仍乱码 ⇒ 排除 |
- | XOR `(b1^a, b2^b)` | 65,536 | 0.8047 | 排除 |
- | **交换+加法** `((b2+p),(b1+q))` | 65,536 | **0.2641** | 远低于不交换的 0.8154 ⇒ **明确排除** |
- | 纯字节交换（无偏移） | 1 | 0.0000 | 排除 |
- ⇒ **第一字节的偏移方向是找对了；问题全部集中在第二字节。**
- | `_msg/find_table.py` | ★ 资源树里按「Big5 码点占比」找码表（data/ 与 app_system/ 均无） |
- | `_msg/msg_big5d.py` | ★ 结合判据 + 字节空间分布量化（b2 覆盖全空间、14.5% 落空洞） |
- | `_msg/msg_big5swap.py` | 交换+偏移搜索（0.2641，明确排除） |

---

## `ARCH_336.md`

**标题**：ARCH_336 —— 进展：**标点区（0x81）解码正确，只有汉字区（0x88-0x9F）被映射** ｜ 1. ★ 关键观测：`data/C/` 版本的标点区完全正确 ｜ 2. 本轮排除的「只作用于汉字区」的简单变换 ｜ 3. 由此得到的下一轮唯一可行路线 ｜ 4. 仍未解（据实） ｜ 5. 本轮产出物

**发现/结论**（3 条，最多列 40）：

- > 目标第 (5) 项。用户已确认游戏内对话是**繁体中文**。本轮找到一条**新的定位**：
- ⇒ **结论：`.msg` 的编码框架是 Shift-JIS 形态；被变换的只有汉字区 `0x88..0x9F`。**
- （连同前几轮已排除的：常量加法全搜 0.8154、XOR 0.8047、交换+偏移 0.2641、

---

## `ARCH_337.md`

**标题**：ARCH_337 —— `msg` 汉字区映射：四个「结构化 Big5 基准」全部失败；记录一次吻合与一条反思 ｜ 1. 一个真实存在的吻合（记录下来，但**不据此下结论**） ｜ 2. 四个基准的实测结果（判据：解出的文本是否成词） ｜ 3. 一条必须记下来的反思（影响后续策略） ｜ 4. 本轮仍未解 / 范围收窄总结 ｜ 5. 本轮产出物

**发现/结论**（7 条，最多列 40）：

- > 本轮据此构造了**四个结构化的候选基准**并逐一实测 —— **全部失败**。
- 实测字符码 c = (b1-0x88)*256 + b2      max = 5127   （distinct 466 / 6342 次）
- ⇒ 「`c` 是某张 Big5 有序表的下标」这一族**穷尽后不成立**。
- （实测解出「ゆかりに呼ばれてやってきた…」）。
- 已经**排除**的（全部有实测）：
- 1. ★ **以 `.bf` 段 3 的 `MSG1` 为对照**，比两处的头部/标志差异（本文 §3）；
- | `_msg/msg_big5common.py` | ★ 四个 Big5 结构化基准的构造与实测（全部失败） |

---

## `ARCH_338.md`

**标题**：ARCH_338 —— 在 eboot 里定位 `.msg` 的路径表与引用点（为反汇编做准备） ｜ 1. ★ eboot 里的 `.msg` 路径表（每个 `data/` 都配一个 `data/C/`） ｜ 2. ★ 代码用「原始文件偏移」而不是 VA 引用这些字符串 ｜ 3. 表本身的引用：**0 命中**（本轮已实测） ｜ 4. 下一步（精确、带地址） ｜ 5. 本轮仍未解 / 已排除汇总（第 (5) 项） ｜ 6. 本轮产出物

**发现/结论**（16 条，最多列 40）：

- 键名格式串（同一字符串区）：
- `%s_%03X_%03X` 与文件里实测的键名形态（`tutorial_000_000`、`commu_01`）吻合。
- off=3103408  VA=0x812F49D0   RAW-OFF  refs = [0x5dac40, 0x5dac4c, 0x5dac58, 0x5dac64, …]
- 只有 `data/msg.bin` 的**原始偏移**（`0x2F5AB0`）被引用，且出现在 **DATA 段**（文件偏移 ≥ `0x3228F0`）：
- 以 12 字节为周期重复 `{ 0x2F5AB0, 递增 4 的计数, 常量 0x00010200 }`。
- **结论（可复现）**：`.msg` 相关字符串是**按模块基址 + 偏移**被引用的，
- 对这张表的起始偏移 `0x5DAC40`、以及 `0x5DABF0`，做 `RAW` / `VA` 两种 u32 编码的搜索：
- table start  0x5DAC40  RAW refs=[]    VA refs=[]
- table -0x50  0x5DABF0  RAW refs=[]    VA refs=[]
- msg.bin str  0x2F5AB0  RAW refs=[0x5dac40, 0x5dac4c, 0x5dac58, …]   VA refs=[]
- ⇒ **这张表不是被 u32 直接引用的**（大概率由 PC 相对 `ldr` 从常量池取，
- 1. ★ **改用 PC 相对 `ldr` 的常量池搜索**：Thumb-2 里 `ldr rN, [pc, #imm]` 的字面量紧跟在函数
- 附近。做法是在 DATA 段 `0x5DAC40` **前后各 4 KiB 内**找「看起来像函数字面量池」的区段
- 2. 找到代码后，用 `clang --target=armv7-linux-androideabi -mthumb` + `llvm-objdump -d` 反汇编
- **已排除**：直接 Big5/cp950/big5hkscs/GBK/GB18030/EUC-JP/cp932/Johab/CP949/UTF-8/UTF-16BE；
- | `ARCH_338.md` | 本文（含 8 个路径字符串的文件偏移、格式串、引用表位置） |

---

## `ARCH_339.md`

**标题**：ARCH_339 —— 运行时不做 magic 比对；`c_` 前缀揭示「字体 ↔ 语言版本」配对 ｜ 1. 全部模块里都没有格式 magic —— 运行时不做 magic 比对 ｜ 2. ★ 新线索：`c_` 前缀 = 「C 语言版本」，与 `data/C/` 配对 ｜ 3. 注意一个必须核对的细节（避免重犯 ARCH_320 式错误） ｜ 4. 下一轮（精确） ｜ 5. 本轮产出物

**发现/结论**（7 条，最多列 40）：

- > 但排除了整条路，并发现一条**新的、很有指向性的配对关系**。
- ⇒ **`MSG1` / `FNT0` / `FLW0` / `FCT0` 这些 magic 在可执行代码里完全不存在**，
- **`c_` 前缀与 `C/` 目录是同一套命名约定** ⇒ 强烈提示：
- 用户确认游戏内对话是**繁体中文** ⇒ 繁体中文那一路应当用 **`c_font_p5.fnt`**。
- 见 `ARCH_329 §2`），**而两者的字形表前 88 项逐字节相同**（`_fnt/fnt_cmp.py` 实测）。
- 所以「两者字形表不同」这一步**还没有证据** —— 本轮的配对只是**命名约定上的强提示**，
- | `_msg/find_msgdecoder.py` | ★ 在 6 个模块里搜 magic 字符串与 Thumb-2 常量构造（全 0） |

---

## `ARCH_33_GAP_STATUS.md`

**标题**：ARCH 盘点：已完成 / 未完成 / 待仲裁 ｜ 1. 已完成的文档（27 份，均已落盘） ｜ 第一轮 · 模块与 HLE 缺口 ｜ 第二轮 · 缺口子系统 ｜ 第三轮 · 运行原理 ｜ 第四轮 · 结构补全（**部分完成**） ｜ 2. 未完成的文档（6 份，agent 超时未落盘） ｜ 3. 待仲裁的冲突（这是本轮最重要的产出） ｜ 冲突 A：`titleSequence` 的 TBH 表（两个 subagent 结论相反） ｜ 冲突 B：SEG1 的文件映射 —— **已结案，且我（本文件）当时的裁决是错的** ｜ 冲突 C：`ARCH_18` 的若干细节被 `ARCH_28` 指出错误 ｜ 4. 第四轮新拿到的结构（即使只有 2 份，价值很高） ｜ `ARCH_30_sequences.md`：具名序列注册表全景 ｜ `ARCH_28_music_select.md`：选歌链路 ｜ 5. 建议的下一轮（补全顺序）

**发现/结论**（37 条，最多列 40）：

- > 这份文件是**当前分析状态的诚实快照**。它不是结论文档，而是"还差什么"的清单。
- `ARCH_20_data_formats.md`（数据格式）· `ARCH_21_scene_graph.md`（场景图）·
- | `ARCH_26_rhythm_judge.md` | **`mnsProcess.c` 节奏判定核心**（`0x81121598`–`0x8112638E`，约 19KB） | **P0 —— 判定链路的单点阻塞** |
- | `ARCH_6` | 基址 `0x81198188`，state0=`0x8119819A` … state8=`0x811982A4` |
- | `ARCH_30` | **`ARCH_23` 的修正是错的**；基址确为 `0x81198188`，**`ARCH_6` 才正确**（`ARCH_23` 的目标落在指令中间） |
- **状态：未仲裁。** 但注意 `ARCH_6`/`ARCH_23`/`ARCH_30` 三者对"基址"本身无分歧（都是 `0x81198188`），
- 反汇编 `0x81198184` 处的 `tbh`，确认 Thumb-2 `tbh` 的 PC 基准到底是
- **结案结论：eboot 是两段 LOAD，必须按 VA 落在哪一段选公式。**
- SEG0  vaddr=0x81000000 filesz=0x321810  → 文件 0x10E0 .. 0x3228F0   （code=1）
- SEG1  vaddr=0x81325000 filesz=0x21BA74  → 文件 0x3228F0 .. 0x53E364 （code=0）
- SEG0 公式：fo = VA - 0x81000000 + 0x10E0
- SEG1 公式：fo = VA - 0x81325000 + 0x3228F0
- - 字符串 `BRAND NEW DAYS (YUYOYUPPE REMIX)` 在文件偏移 `0x304CB8`。
- 用 SEG0 公式 → VA `0x81303BD8`（落在 SEG0 区间 ✓）。
- - 歌单表 rec0 的 `+0x00` 字段值 = `0x81303BD8` —— **与上面完全一致**，指针链自洽。
- - **歌单表自身的文件偏移是 `0x3A5F90`**，它 **> SEG1 起点 `0x3228F0`** ⇒ **表在 SEG1**。
- 用 SEG1 公式 → 表的 VA = `0x3A5F90 - 0x3228F0 + 0x81325000` = **`0x813A86A0`**。
- | 歌单表 | **`0x813A86A0`** | `ARCH_28` / `ARCH_29` 正确 |
- | 标题字符串 rec0 | `0x81303BD8` | 在 SEG0 |
- | `ARCH_20` 的 `0x813A4CB4` | ✘ | 偏 `0x39EC` |
- | 本文件早先的 `0x813A4EB0` | ✘ | **我误把表也套了 SEG0 公式** |
- - **表内存放的指针值**（如 `0x81303BD8`）是**运行时 VA**，指向 SEG0 的字符串；
- **遗留**：SEG0/SEG1 的文件交界点仍有小争议（`ARCH_25` 报 `0x3228F0` 与 `0x3220E0` 自相矛盾，
- 重叠 `0x810`），但两条公式的**适用规则**已经确立，不影响上面的结论。
- | `0x8118F9C8` 的性质 | 它是调试序列 `tus test`（`tstTus.c`），**不是**"标题菜单输入机" |
- | `0x816CDAA0` / `0x816CDFB0` | `ARCH_18` 把二者语义写反 |
- **状态：未仲裁**，但 `ARCH_28` 给出的证据（52 状态 TBH 全译、`appStateRequest(3)`）比 `ARCH_18` 更具体。
- - 生命周期钉死：工厂 `0x8116F7CA` → 注册链 `0x815554C0+4`（`+0x10/+0x14`）
- → task 对象（运行链头 `0x816CD9E0`，next 在 `+0x4C`，step 在 `+0x28`）
- → 返回 `-1` 走 `0x811DAD8C`，节点摘除 `0x8116F75E`
- - 下一步收敛为两个确切函数：**`0x811DAB8C`(task 构造) / `0x811DAD8C`(task 析构)**
- 主菜单(state 5) → 菜单项派发 0x810E8690(mode)
- → enter 0x81185A0C 调 musicSelectMainCreate (0x8113ABB0 / 0x8113AA52, name串 0x81305094, period 0xC8)
- → 序列 step 0x811479BA（52 状态 TBH，表 @0x81147A40，52 个入口已全部译出）
- → state 0x30 @0x8114D57C: 0x8114D594 bl 0x81185616 = appStateRequest(3) + 返回 -1 自摘
- - 谱面路径取点 **`0x81126190`**：`sprintf("dance/mns/mns%03d_%d.bin", [obj+0x1DC], [obj+0x22C])`
- - 引擎**逻辑按键码生成点 `0x8116F4F8`** + 键表 **`0x813AFC40`**（bit0..15 + bit24 全序）

---

## `ARCH_342.md`

**标题**：ARCH_342 —— 确认无未提取容器；发现 `evtTools.bin` 是同族多文件包 ｜ 1. 没有未提取的容器 ｜ 2. 全树再扫「码表」：仍无 ｜ 3. ★ 新发现：`evtTools.bin` 与 `msg.bin` 是同一种多文件包 ｜ 4. ★★ 多文件包格式完全解出（`msg.bin` / `evtTools.bin` / `C/msg.bin` 同族） ｜ 5. 对第 (5) 项的意义（据实） ｜ 6. 本轮产出物

**发现/结论**（20 条，最多列 40）：

- > 但发现一个新东西：`event/tool_bin/evtTools.bin` 与 `msg.bin` 是**同一种多文件包格式**。
- （`data_extracted` 本身就是全部内容）。⇒ **「那张码表在未提取的容器里」这条假设不成立。**
- ⇒ **确认：现有资源里不存在 ≥5128 项的 Big5 码表。**
- 第一条 @0x000000  type=4  size=896  name="evtSMMessagePosP3.lbn"  data magic="NBLS"
- 下一跳 0x000028 + 896 = 0x0003A8
- 第二条 @0x0003A8           name="evtSMMessagePosP5.lbn"
- ⇒ **`msg.bin` 是「多文件包」，每个成员是一个 `.bmd`；`evtTools.bin` 也是多文件包，
- [0] @0x000004 size=896      evtSMMessagePosP3.lbn   magic=NBLS
- [1] @0x0003A8 size=896      evtSMMessagePosP5.lbn   magic=NBLS
- [2] @0x00074C size=4194880  p3_test.spr6            magic=SPR6
- [3] @0x4009B0 size=4194880  p5_test.spr6            magic=SPR6
- [ 0] @0x000004 size=51520   msg_item_dress.bmd        [ 6] @0x079ADC size=30336  msg_item_eyes.bmd
- [ 1] @0x00C968 size=398752  msg_item_accessory.bmd    [ 7] @0x081180 size=32480  msg_item_haircolor.bmd
- [ 2] @0x06DF2C size=2176    msg_playcustom.bmd        [ 8] @0x089084 size=4544   msg_system_open.bmd
- [ 3] @0x06E7D0 size=11488   msg_helpwindow.bmd        [ 9] @0x08A268 size=33152  msg_item_cond.bmd
- [ 4] @0x0714D4 size=6624    msg_notifywindow.bmd      [10] @0x09240C size=21120  msg_commu_reward.bmd
- [ 5] @0x072ED8 size=27616   msg_item_hairarrange.bmd
- ⇒ **两个版本是「同一套资源的两个语言版本」**，与 `ARCH_339 §2` 的 `c_` 前缀配对完全一致。
- 本轮**没有**解决 `msg` 的编码。汇总 `ARCH_332`–`ARCH_342` 共 11 轮，已排除：
- * 「可执行模块里比对 magic」—— 6 个模块（eboot + 5 suprx）里**没有任何格式 magic 字符串，

---

## `ARCH_345.md`

**标题**：ARCH_345 — `msg.bin` / `.bmd` 容器与文本记录格式 ｜ 0. 目的 ｜ 1. 结论摘要 ｜ 2. `.bmd` 成员头（逐字节证据） ｜ 3. 表区：stride-8 `{u32 0, u32 offset}` ｜ 4. 记录格式 ｜ 4.1 布局（**修正后**） ｜ 4.2 步长公式 ｜ 4.3 明文证据（**这是本轮最硬的收获**） ｜ 4.4 已知局限（不掩盖） ｜ 5. 交叉验证 ｜ 6. 文本编码判定 ｜ 7. 否证结果（本轮的主要价值） ｜ 7.1 不存在独立的 ordinal→Unicode 字符表 ｜ 7.2 不存在逐字节 add-k / xor-k 变换 ｜ 7.3 `font.ctbl` 的 T1/T2 确证 ｜ 8. 自我更正汇总 ｜ 9. 未解项与下一步 ｜ 10. 复现命令 ｜ 比对：期望 "IDENTICAL over 5331 lines"

**发现/结论**（17 条，最多列 40）：

- 继续推进 5 个未解格式中的 `msg`（物品/帮助/教程文本）。此前 11 轮
- 结论：**容器与记录格式已完全解明并落地为解析器**；**文本编码仍未证实**。
- | stride-8 表区 | **已解明** | `{u32 0, u32 严格递增 offset}`，有限项，逐成员终止点不同 |
- | 记录格式 | **已解明** | 3 条连续记录步长同时吻合 |
- +0x1C u32 0x00020000   （所有已检查成员恒定）
- +0x20 ... stride-8 表
- **恒等式（硬证据，非推测）**：
- 此前把整片表当成「池」或把整片表当文本，是本项目多个错误结论的根源（§8）。
- +0x24 u32 0xFFFF05F2
- u32 0x0000000A       文本终止
- 该版本产出 `records=0`。回查 `itdrs_000` 字节后确认 **name 区是 24 字节**，
- | 记录 | 起点 | payload | 计算下一条 | 实测下一条 |
- **全局语言变体约定**，而非文件格式差异。
- **但编码仍未证实**：CJK 区按 cp932 解出的是「合法却错位」的汉字
- 修正后结论仍然是否证性的。
- 3. **§6**：此前「排除 Shift-JIS」的结论**作废**——那是在把整片 `{0,offset}`
- 5. **ARCH_332 §2** 以「假名区缺失」当作变换证据，在确认繁体中文后已作废

---

## `ARCH_346.md`

**标题**：ARCH_346 — `font.ctbl` 的 T1 不是标准 Shift-JIS 表（`msg` 编码问题的真正根因） ｜ 0. 本轮问题 ｜ 1. 决定性证据：T1 ≠ 标准 SJIS→Unicode ｜ 2. T2 与 T1 不是同一 ordinal 空间 ｜ 3. 字节布局的唯一性（多编码对照） ｜ 4. 已确定与未确定的边界（不夸大） ｜ 5. 下一步（具体、可执行，留给后续） ｜ 6. 复现命令 ｜ 期望输出： ｜ standard list len = 9762 ; T1 len = 3886 ｜ T1 vs standard mismatches over first 3886: 3134 ｜ T1 vs T2 differ at 3791 of 3886 ; first [95, 96, ...]

**发现/结论**（6 条，最多列 40）：

- 的 T1 查表（而非标准 cp932）」。本轮执行该实验，并给出结论。
- **结论：假设成立，且这是 `msg` 编码困扰 11 轮（ARCH_332–ARCH_345）的真正根因。**
- 方法（可复现）：独立**重建**标准序列 `L`，不用任何本项目既有结论——
- **结论**：字节布局确实是 Shift-JIS 兼容的双字节结构（`0x8140`=全角空格、
- 4. `msg` 容器与记录格式已完全解明（ARCH_345 §2–§5，交叉验证
- **关键洞察（可复用于其它格式）**：本项目多次失败的共同模式是

---

## `ARCH_347.md`

**标题**：ARCH_347 — `msg` 编码的真正根因：参照表错误（非数据变换） ｜ 0. 本轮结论一句话 ｜ 1. T1 ≠ 标准 Shift-JIS（决定性证据） ｜ 2. T1 是 L 的「子序列」（只删不换） ｜ 3. T2 与 T1 不是同一 ordinal 空间（更正 ARCH_345 §7.3） ｜ 4. 新诊断：标点正确、汉字错误 ｜ 5. `font.ctbl` 未解区域（**本轮未攻克，如实标注**） ｜ 6. 本轮确定 / 未确定 ｜ 7. 下一步（具体、可执行） ｜ 8. 复现命令

**发现/结论**（2 条，最多列 40）：

- 独立重建标准序列 `L`（不使用本项目任何既有结论）：
- **方法论结论（可复用于其余格式）**：本项目多次失败的共同模式是

---

## `ARCH_348.md`

**标题**：ARCH_348 — `msg` 双字节码重映射：三项新否证 + 字节级结构确证 ｜ 0. 本轮工作 ｜ 1. 否证：JIS 序数空间上的常数位移 ｜ 2. 否证：用标准序数索引压缩表 T1（变长位移） ｜ 3. 否证：存在「码 → 码」替换表 ｜ 4. 字节级结构确证（本轮实质收获） ｜ 4.1 `PlayStation Store` 与 CJK 字节共存 ｜ 4.2 固定共享尾串 ｜ 4.3 词结构完好 ｜ 5. 结论与本轮状态 ｜ 6. 下一步（收敛后的最短路径） ｜ 7. 复现命令

**发现/结论**（4 条，最多列 40）：

- 这与我此前试过的所有常数位移都不同。四变体实测：
- 不遵循标准 SJIS**。这排除了「文本被整体混淆/加密」的模型。
- **方法论结论**：本项目失败模式高度一致 ——
- 本轮起该结论已被 7 项否证系统支撑。

---

## `ARCH_349.md`

**标题**：ARCH_349 — 转向字体侧：`.fnt` 解码、A/C 编码不对称、16 位稠密索引证据 ｜ 0. 本轮背景 ｜ 1. `.fnt` 头部解码（三个文件一致） ｜ 2. JP 与 CN 字体表的差异位置（硬事实） ｜ 3. A/C 编码不对称（本轮最重要的新发现） ｜ 4. 新证据：CJK 码是 **16 位稠密索引**，不是双字节字符码 ｜ 5. 否证与判据更正 ｜ 5.1 否证：Big5 + 二维字节位移 ｜ 5.2 判据更正（本轮第 2 处）：`find_sjistable.py` 漏掉 Big5 值域 ｜ 6. 本轮确定 / 未确定 ｜ 7. 下一步 ｜ 8. 复现命令

**发现/结论**（7 条，最多列 40）：

- ARCH_348 结论：`msg` 文本侧的所有变换假设已被穷尽否证，映射须由字体侧决定。
- 20 00 20 00   +0x08 = 0x00200020  => u16 32, u16 32  字形单元格 32x32
- | font_p5.fnt | 1245122 | 0x00000000 | 24 | 772 |
- | c_font_p5.fnt | 1167652 | 0x6C310000 | 24 | 772 |
- | font_staff.fnt | 28176 | 0x00000000 | 24 | 274 |
- → 与 ARCH_348 §4.1 结论一致并加强：**分歧被严格限制在 CJK 双字节码内**。
- **但这只是形式证据，`BASE` 与实际字符表仍未确定，不作为正解。**

---

## `ARCH_34_CORRECTIONS.md`

**标题**：ARCH_34 — 全量分析的三处重大修正 + 逐函数索引 ｜ 0. 本轮结论摘要 ｜ 1. 修正 1：Ghidra 镜像偏移 0xF60 ｜ 1.1 事实链 ｜ 1.2 三重独立确证 ｜ 1.3 已执行的修复 ｜ 2. 修正 2：ARCH_25 区段表的两处缺陷 ｜ 2.1 区间互相重叠 ｜ 2.2 漏掉 0x811980FE–0x812B2AF0（约 1.1 MB，5,722 个函数 = 全部函数的 46%） ｜ 2.3 修正后的区段划分 ｜ 3. 源码归属覆盖（修正后） ｜ 3.1 movw/movt 独立验证（不依赖 Ghidra） ｜ 4. 逐函数索引与符号注入 ｜ 4.1 `func_index.tsv` ｜ 4.2 注入到 Ghidra 的符号 `[Ghidra]` ｜ 5. `titleSequence` 状态机 TBH 分发表（解密） ｜ 6. `mnsProcess.c` 真实边界（修正目标第 3 项的起点） ｜ 7. 覆盖矩阵（初版） ｜ 7.1 strip 导致的永久丢失项（不得含糊） ｜ 7.2 不确定项标注 ｜ 8. 下一轮待办

**发现/结论**（97 条，最多列 40）：

- > **证据规范**：每条结论后附证据类型：
- > `[字节]` = 原始文件字节 · `[算术]` = 可复算的地址推导 · `[Ghidra]` = 反编译/反汇编输出 ·
- | # | 结论 | 影响面 |
- | 修正 1 | 此前喂给 Ghidra 的 `p5d_std.elf` **整体偏移 0xF60**，全部反编译地址错误 | 影响 `decomp_seg0.c` / `decomp_mnsProcess.c` / 一切 Ghidra 派生地址 |
- | 修正 2 | ARCH_25 的「26 个代码区段」**区间互相重叠**，且**漏掉 0x811980FE–0x812B2AF0 约 1.1 MB 代码** | 影响覆盖矩阵的完整性声明 |
- | 修正 3 | `mnsProcess.c` 的真实范围**不是** `0x81121536–0x81132548 / 48 函数`（那是偏移镜像产物） | 影响目标第 3 项的起点 |
- ph0 PT_LOAD p_offset=0xE0     p_vaddr=0x81000000 p_filesz=0x321810 p_memsz=0x321810 flags=5
- ph1 PT_LOAD p_offset=0x3218F0 p_vaddr=0x81325000 p_filesz=0x21BA74 p_memsz=0x3AA4A0 flags=6
- ph2 SCE_RELA p_offset=0x53D370 filesz=0x8C388
- ph3 SCE_RELA p_offset=0x5C9700 filesz=0x33BB8
- SEG1 文件偏移 = 0x1000 + 0x3218F0 = 0x3228F0   ← 正确
- `E:\P3DP5D\xrd757\...\image\appImageDDS_BC7.c` 位于 `eboot.bin` 文件偏移 `0x2EF0AC`。
- - 真实 VA = `0x81000000 + (0x2EF0AC - 0x10E0)` = **`0x812EDFCC`**
- - 偏移镜像 VA = `0x81000000 + (0x2EF0AC - 0x180)` = **`0x812EEF2C`**
- Ghidra 在旧项目里报告该字符串地址为 `0x812EEF2C` `[Ghidra]`，差值恰为 `0xF60`。
- 真实映射下 `0x8119817C` 起 16 字节为：
- `0x81198184` 处的 `0xE8DF 0xF011` 正是 `tbh [pc, r1, lsl #1]`，表基址 `0x81198188`。
- 这与会话中独立记录的探针点 `0x8119826A` / `0x81198270`（见表项 case 7）完全吻合。
- 指针解析率提高 7 倍，是映射正确性的强证据：镜像整体偏移时，所有绝对指针都会指错位置。
- | `C:\dev\p5d_build\p5d_true.elf` | 用 `header_len + p_offset` 重建的标准 ELF32（2×PT_LOAD，`e_entry=0x81198464`） |
- ARCH_25 表格中，区段 14 写作 `0x810FD1F0–0x811475E0`，而区段 15/16/17 的起点
- （`0x811199F6` / `0x81133190` / `0x8114EF90`）都落在 14 的区间内部。
- 正确镜像下函数 VA 上限是 `0x812B2AF0`，且该处之后到字符串表起点 `0x812E5130`
- （约 206 KB）已无函数 `[Ghidra]`。补齐这一段的证据：
- **A 段 `0x8119807C–0x81222000`**（约 550 KB）—— 引用字符串特征 `[Ghidra]`：
- ⇒ **GX 着色器/GXP 编译器 + GFD 图元 + 后处理效果参数**，尾部与区段 26（title）交叠。
- **B 段 `0x81222000–0x812B2AF0`**（约 580 KB，1,003 个有字符串引用的函数）—— 字符串表目标
- VA 集中在 `0x81300000–0x81320000`，关键字特征 `[Ghidra]`：
- ⇒ **静态链接的 CRIWARE 中间件**（CriAtom / CriFs / CriMvPly / Sofdec(Sj) / Mana / Cpk Binder
- | 15 | `0x811199F6` | `0x81133190` | musical_note_sequencer（谱面/判定） | **235** |
- | 16 | `0x81133190` | `0x81150D5A` | music_select（选歌） | 184 |
- | 17 | `0x8114EF90` | `0x81153A18` | polygon | 93 |
- | 18 | `0x81153A18` | `0x811593F8` | movie（CRI 影片） | 205 |
- | 19 | `0x811593F8` | `0x81159BDA` | app_system/utility（interleave） | 4 |
- | 20 | `0x81159BDA` | `0x8115FB9E` | app_system/main + Allocator | 233 |
- | 21 | `0x8116133C` | `0x8116E5D8` | appMath / appOt / 图形原语 | 330 |
- | 22 | `0x8116E65E` | `0x8116F9BC` | **appPad（输入）** | 60 |
- | 23 | `0x81170D34` | `0x81174C8A` | **appSound / appTouch / appThread** | 201 |
- | 24 | `0x81174DD0` | `0x811813F6` | app_system/utility（后半） | 364 |
- | 25 | `0x8118145A` | `0x8118FDF4` | kernel / debugmenu / staff / test / init | 301 |

---

## `ARCH_350.md`

**标题**：ARCH_350 — 已知明文对：A/C 对齐取得三组 (存储码 → 真实字符) 锚点 ｜ 0. 本轮结论 ｜ 1. 已知明文对的取得 ｜ 1.1 共享句尾 ｜ 1.2 推导 ｜ 1.3 佐证：品牌名对齐 ｜ 2. 否证：映射不是「保序」的 ｜ 3. `.fnt` 侧补充（ARCH_349 §1–§2 摘录） ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步（有了锚点后变得可执行） ｜ 6. 复现命令 ｜ A/C 同 id 记录对齐与锚点

**发现/结论**（2 条，最多列 40）：

- 由此确认（**这是本轮的核心成果**）：
- - 魔数 `FNT0`，`+0x08 = 0x00200020` ⇒ 字形单元格 **32×32**，表偏移 24，项数 **772/772/274**。

---

## `ARCH_351.md`

**标题**：ARCH_351 — 修正文本终止符判据：记录数 == 表项数（全部 22 成员自洽） ｜ 0. 本轮成果 ｜ 1. Bug：终止符判据用错 ｜ 2. 修正 ｜ 3. 验证结果 ｜ 3.1 记录数 == 表项数（全部成员） ｜ 3.2 越界指示器归零 ｜ 3.3 交叉验证 ｜ 3.4 记录数变化 ｜ 4. 附：批量已知明文锚点（供后续使用） ｜ 5. 本轮确定 / 未确定 ｜ 6. 下一步 ｜ 7. 复现命令 ｜ 期望：IDENTICAL over 10910 lines；每个成员 records == tableEnt

**发现/结论**（8 条，最多列 40）：

- 找到并修正 `msg` 记录提取的一个**真 bug**。修正后，**每一个成员的记录数
- 都精确等于其 stride-8 表项数**（A/C 两版共 22 个成员，无一例外），
- 证据（A 版 `msg_helpwindow.bmd`）：
- 修正前 `msg_helpwindow.bmd` 有 30 条记录出现 `hw_`/`itdrs_` 等内嵌名。
- 修正前: msg_helpwindow 30 条 / msg_item_dress 68 条 / msg_commu_reward 38 条 内嵌记录名
- | 成员 | 修正前 | 修正后 | 表项数 |
- 1. 文本终止符是 2 字节 `0A 00`（此前 4 字节判据错误，已修正）。
- 1. 用修正后**干净**的记录（不再越界）重新做 A/C 逐记录对齐：

---

## `ARCH_352.md`

**标题**：ARCH_352 — A/C 不共用字符空间（否证跨版本外推） ｜ 0. 结论 ｜ 1. 证据 ｜ 1.1 标点约定完全不同 ｜ 1.2 锚点码在两版中的频次 ｜ 2. 由此否证的推理 ｜ 3. 对目标项 (5) 的影响 ｜ 4. 下一步 ｜ 5. 复现命令 ｜ 6. 本轮确定 / 未确定

**发现/结论**（2 条，最多列 40）：

- - 同时**加强**了 ARCH_350 §2 的结论：映射**不保序**。
- 仍解码为标准标点 —— 这是一个**强约束**，可用于快速排除错误假设。

---

## `ARCH_353.md`

**标题**：ARCH_353 — T2 不是中文映射表；语言目录结构确认 ｜ 0. 本轮工作 ｜ 1. 结果：T2 解码失败 ｜ 2. T2 的性质（新发现，含自我更正线索） ｜ 2.1 T2 含**简体**中文字 ｜ 2.2 T2 呈**连续 Unicode 码点**特征 ｜ 3. 语言目录结构 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步

**发现/结论**（3 条，最多列 40）：

- 而 T2 出现简体字，与此**不一致**。该前提应视为**待重新确认**，
- 这与 ARCH_345 §1 中 `eboot.bin @0x3BECC8` 的 3535 个连续码点同属一类现象。
- **待重新确认（本轮提出）：**

---

## `ARCH_354.md`

**标题**：ARCH_354 — 更正 ARCH_353 §2.2；T1/T2 的简繁与单调性测量 ｜ 0. 本轮结论 ｜ 1. 更正：T2 不是码点枚举 ｜ 2. 简繁专用字分布 ｜ 3. 单调性（排序依据） ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步

**发现/结论**（7 条，最多列 40）：

- 不构成「码点区间枚举」的证据。
- ARCH_353 提出的「验证语言归属」**本轮未获结论**，仍需其它判据。
- （对既有关键前提「对话是繁体中文」的处置：本轮**不推翻**，仅确认
- T2 单独不足以证实或否证它。）
- 保序子序列」结论**一致**。
- 则其 ordinal 空间与 T1 **不同**（这再次确认 ARCH_346 §2 的结论，
- 2. 若确认 `data/C/` 为某一变体，则 `font.ctbl` 中应存在与之配对的表；

---

## `ARCH_355.md`

**标题**：ARCH_355 — `.tga` 可视化解码管线；对语言归属判定的否定结果 ｜ 0. 本轮工作 ｜ 1. `.tga` 解码管线（已建立，可复用） ｜ 2. 结果：内容不回答语言问题 ｜ 3. 本轮确定 / 未确定 ｜ 4. 下一步 ｜ 5. 复现命令 ｜ 然后用 read_image 直接查看该 PNG

**发现/结论**（2 条，最多列 40）：

- → 本轮对 ARCH_354 §5 的目标**未获结论**。判据需再换。
- 即可判定语言。注意 `tutorial_02.tga` 的教训：先看内容再下结论，

---

## `ARCH_356.md`

**标题**：ARCH_356 — `tutorial_p5` 双版本 diff：否定结果 ｜ 0. 本轮工作 ｜ 1. 结果：差异由贴图数据主导，无法隔离文本 ｜ 2. 附带可确认的事实 ｜ 3. 本轮确定 / 未确定 ｜ 4. 下一步

**发现/结论**（8 条，最多列 40）：

- first diff        = 0x000802B4   （位于 pict.spr6 内部，该成员起于 0x80268）
- last  diff        = 0x0108654F
- [0x00687E00]  A: FE 2B 0F 0F 0F 0F …    C: FC 18 10 10 10 10 …
- [0x00688200]  A: 2B 66 66 66 66 66 …    C: 18 51 51 51 51 51 …
- [0x006883DD]  A: 05 05 03 03 05 03 …    C: 04 04 02 02 04 02 …
- **结论：该 diff 无法用于隔离文本字段，ARCH_355 §4.1 的路径否证。**
- [0x000802B4]
- 2. 具体做法：用 ARCH_351 修正后的可靠记录边界，对 91 条 ASCII 锚点记录

---

## `ARCH_357.md`

**标题**：ARCH_357 — `.fnt` 表后区域不是裸位图（解释此前 4 个假设全失败） ｜ 0. 本轮工作 ｜ 1. 测量结果 ｜ 2. 结论：不是裸位图 ｜ 3. 附带：表后区域开头似又一层索引 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步

**发现/结论**（6 条，最多列 40）：

- 实测分布以 `FF` 极端主导、并伴随 `FE`/`7F`/`BF`（位边界效应），
- **但配对方式尚未确定**（步长 4 的多种对齐都不自洽），**本轮不作为结论**。
- 2. 表后区域的真实格式（位打包方案 / 是否有压缩）。
- 则基本排除通用压缩，转为**位打包**假设。
- - 单元格 32×32（`+0x08 = 0x00200020`）
- 98.7 B/字形 对 32×32 的常见紧凑格式（1bpp=128B、2bpp=256B）都不整，

---

## `ARCH_358.md`

**标题**：ARCH_358 — `.fnt` 关键发现：0xF24 处的**位偏移表**与文件真实布局 ｜ 0. 本轮成果 ｜ 1. 发现：0xF24 处的严格递增 u32 表 ｜ 2. 单位判定：值是**位**偏移（几何自洽） ｜ 确定的 `.fnt` 布局 ｜ 3. 字形尺寸分布（差值直方图） ｜ 4. 渲染尝试（未成功，如实记录） ｜ 5. 本轮确定 / 未确定 ｜ 6. 下一步（收敛后）

**发现/结论**（2 条，最多列 40）：

- 真实字体位图的背景位应占多数（约 50–90% 为 0），而实测绝大多数为 1 ——
- **本轮未能渲染出可辨认的字。** 该结论不作为正解。

---

## `ARCH_359.md`

**标题**：ARCH_359 — `.fnt` 位流：极性已定，但"裸位图"仍被否证 ｜ 0. 本轮工作 ｜ 1. 已确定 ｜ 空字形证据 ｜ 2. 否证：位流不是 32×32 裸位图 ｜ 2.1 最密字形的墨占比异常低 ｜ 2.2 `delta` 分解不出合理的高×宽 ｜ 2.3 渲染结果 ｜ 3. 未确定 ｜ 4. 下一步

**发现/结论**（3 条，最多列 40）：

- 实测连"最密"的也才刚过半 —— 与任何字体位图的统计特征都不符。
- **本轮未能渲染出可辨认的字。该结论不作为正解。**
- 逐字节人工检视，可能比 `font_p5` 更快定出格式。

---

## `ARCH_35_FUNC_TABLES.md`

**标题**：ARCH_35 — 函数指针表反演：mns 谱面事件表（48 项）与引擎分发表总清单 ｜ 0. 本轮结论 ｜ 1. 方法：函数指针表反演 ｜ 1.1 动机 ｜ 1.2 算法 `[算术]` ｜ 1.3 结果 `[算术]` ｜ 2. 补齐缺失函数 ｜ 3. mns 谱面事件处理表（`0x813A5C9C`，48 项）—— 目标第 3 项核心 ｜ 3.1 表内容 `[字节]` ｜ 3.2 引用者 `[算术]` ｜ 3.3 「48 个函数」之谜的解答 ｜ 3.4 对第 3 项的推进 ｜ 4. 其他关键表 ｜ 4.1 场景图 vtable 系列 `[字节]` ｜ 4.2 修正 ARCH_26：`0x813A7ED0` 不是判定窗口表 ｜ 4.3 `0x813A5EDC`：`{函数指针, 计数}` 交替表 ｜ 4.4 疑似误判（必须标注） ｜ 5. 覆盖矩阵更新（在 ARCH_34 §7 基础上） ｜ 5.1 新增产物 ｜ 6. 下一轮待办

**发现/结论**（38 条，最多列 40）：

- > **证据规范**：`[字节]` 原始文件字节 · `[算术]` 可复算推导 · `[Ghidra]` 反编译/反汇编 ·
- > **前置**：ARCH_34（Ghidra 镜像 0xF60 修正）。本文所有地址均为**真实 VA**。
- | 4 | **`0x813A5C9C` 是 mns 谱面事件处理表，48 项，48/48 全部落在 `0x81126E84–0x81132504`（mnsProcess.c 范围）** |
- | 5 | **解开了目标文本「mnsProcess.c(…, 48 个函数)」之谜**：48 = 这张表的项数，范围 = 断言引用点跨度 |
- | 6 | **修正 ARCH_26**：`0x813A7ED0` 不是"判定窗口表"，它被 `musicData.c` 的 14 个 getter 引用 |
- | 7 | 确认场景图 vtable 系列 `0x814D2F2C + n*0x54` |
- for 数据区 R in { SEG0数据段 [0x812B2AF0, 0x81321810),
- SEG1       [0x81325000, 0x81540A74) }:
- `is_code_ptr(w) = (w & 1) == 1 且 0x81000000 ≤ (w & ~1) < 0x812B2AF0`
- （Thumb 函数指针的 bit0 = 1；上限 `0x812B2AF0` 是 ARCH_34 确认的 SEG0 代码末址。）
- ⇒ **eboot 的真实函数数 ≥ 12,398 + 519 = 12,917** `[Ghidra]`。
- 表地址 `0x813A5C9C`，位于 SEG1 数据段；`0x813A5C98` 处为 0（空槽）。
- 而表**内容**的 78 处 `movw/movt` 引用来自 `0x8112250E` 起连续 +0x10 的小函数
- （`0x8112250E/1E/2E/3E/4E/5E…`，即 `0x813A5EDC` 那张 `{ptr, count=2}` 表的成员）——
- 目标文本写 `mnsProcess.c(0x81121536–0x81132548, 69KB, 48 个函数)`。
- 本轮证据表明这三个数字来自**三个不同的东西** `[算术]`：
- | 文本中的值 | 真实含义 | 证据 |
- | `48 个函数` | **`0x813A5C9C` 表的项数 = 48** | `[字节]` 表长 48，成员全在 mns 段 |
- | `0x81121536–0x81132548` | **断言引用点的最小/最大 VA**，不是函数边界 | ARCH_34 §6：二者分别落在 `FUN_81121200`(+0x336) 与 `FUN_81132504`(+0x44) 内部 |
- | `69 KB` | `0x81132548-0x81121536 = 0x11012 = 69,650 ≈ 68 KB` | `[算术]` |
- ⇒ 旧结论把「表项数」「引用点跨度」「跨度字节数」当成「函数数」「代码范围」「代码大小」。
- - 谱面事件的**分发结构**已确定：事件类型 → `0x813A5C9C[index]` → 处理函数（48 个）。
- - 索引 0 对应的 `0x813A5C98` 为 0 ⇒ **事件类型 0 无处理函数**（或该表是 1-based）。
- `0x814D2F2C`，10 项，7 项已识别：
- 与先前记录的「vtable `0x814D2F2C + [node+0x20]*4`」一致 `[Ghidra]`。
- 其后 `0x814D2F5C / 0x814D2FB0 / 0x814D3004 / … / 0x814D34F0` 每 **0x54** 字节一张 20 项表，
- `[算术]` 有 **14 处 `movw/movt` 引用**指向 `0x813A7ED0`，**全部**来自 `musicData.c`：
- `musicData.c` 里全是 48–168 字节的小 getter ⇒ 这是**歌曲/难度参数表**，不是判定窗口。
- **真正的判定窗口仍需另外定位**：候选是 `0x813A5A5C`（整数表 `2,3,4,1,2,1,4,3,4,5,0,3`，
- 被 `FUN_8111A844`(mnsHandle.c, 1808 B) 引用）与 `0x813A5AA4`
- （`0x1624/1/1/2/0x10000000` 结构，被 `FUN_8111DCC6`(58 B) 引用）。
- `0x814D2F5C`（20 项，`known=0`）的成员为 `811CE58C, 811CE590, 811CE592, 811CE596, 811CE598,
- 同理 `0x81308A1C`（12 项，成员全为 `8117DEF0` 重复）与 `0x813214FC`（141 项，成员全为 `812B091C`）
- ⇒ 157 张表中约 3–8 张属此类，其余有效。**该误差范围已显式列出，不做含糊处理。**
- 1. **第 3 项收尾**：逐个读 `0x813A5C9C` 表 48 个函数的伪代码，命名事件语义；
- 解析 `0x813A5EDC` 二级表；用 `mnsProcess_811251D6`(3612 B) 打开谱面文件头的解析代码，
- 闭合 `+0x18` 条目格式；结算评分/连击链路。
- 4. 用 ARCH_34 §3.1 的 5,040 个引用点还原断言行号。

---

## `ARCH_360.md`

**标题**：ARCH_360 — `.fnt` 真实结构：哨兵值表 + 位偏移表 + 位打包数据 ｜ 0. 本轮成果 ｜ 1. `font_staff.fnt` 的完整布局 ｜ 2. 哨兵值：`0xFFFFFFFF` = 未定义字形 ｜ 3. 由此更正 ARCH_358 / ARCH_359 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步

**发现/结论**（10 条，最多列 40）：

- +0x04 u32 0x00010000
- +0x08 u32 0x00200020      (u16 32, u16 32 -> 单元格 32x32)
- +0x342 ...                274 项 u32 **位偏移表**（含 0xFFFFFFFF 哨兵）
- 即 **87 项取 `0xFFFFFFFF`**，其余为递增的位偏移。
- 而相邻项之间夹有 `0xFFFFFFFF` 哨兵，使差值既可能跨越哨兵、也可能对应空字形。
- （274 项，含 `0xFFFFFFFF` 哨兵） → `0x78A` 起位打包数据 → `0x5000` 后为 0xFF 填充。
- 2. `0xFFFFFFFF` 是未定义字形哨兵；`font_p5` 同格式。
- 1. **按哨兵过滤后重建字形序列**：只保留 `a[i]` 与 `a[i+1]` 均非 `0xFFFFFFFF`
- 4. 定位 `font_p5` 中 `0x18` 起点那张表（应为 772 项），确认两文件布局一致。
- **方法论**：本轮的关键教训是 —— **在统计前必须排除哨兵值**。

---

## `ARCH_361.md`

**标题**：ARCH_361 — 撤回 ARCH_360 §2「哨兵值」结论；两文件表几何确认 ｜ 0. 本轮工作 ｜ 1. 更正：`0xFFFFFFFF` 不是哨兵，是 `0xFF` 填充 ｜ 2. 两文件的表几何（已确认，同一格式） ｜ 3. 新暴露的矛盾：表项数 ≠ header 字形数 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步

**发现/结论**（8 条，最多列 40）：

- **ARCH_360 §2 的「274 项中 87 项是 `0xFFFFFFFF` 哨兵」是把窗口取到
- `[208, 208+274)`、越过表尾进入 `0xFF` 填充数据区所致 —— 该结论撤回。**
- 三例几何自洽。→ **格式确认：`[头][u32 位偏移表][位打包数据]`，无哨兵。**
- → **该矛盾未解决，本轮不作结论。**
- 1. `0xFFFFFFFF` **不是**哨兵，是 `0xFF` 填充；ARCH_360 §2/§3 相关推论**撤回**。
- 2. 三文件的表几何自洽，格式 = `[头][u32 位偏移表][位打包数据]`。
- 1. **在确认无哨兵的前提下**，用 `font_staff` 的 160 个真实区间重算尺寸直方图
- **方法论**：连续两轮（ARCH_358/360）的结论都被下一轮实测推翻，

---

## `ARCH_362.md`

**标题**：ARCH_362 — 字形尺寸不是 1024；两字体众数不同 ｜ 0. 本轮工作 ｜ 1. 结果 ｜ 2. 否证：「字形 = 32×32 1bpp = 1024 位」 ｜ 3. 新观察到的事实 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步

**发现/结论**（2 条，最多列 40）：

- 尺寸随字体变化，说明字形宽度可变，或单元格并非 32×32（`+0x08 = 0x00200020`
- 2. `32×32 单元格` 这一前提（来自 `+0x08 = 0x00200020`）**待验证**，此前被当作既定事实。

---

## `ARCH_363.md`

**标题**：ARCH_363 — `.fnt` 墨占比正常但渲染仍为噪声；发现"墨密度自字形起始处单调衰减" ｜ 0. 本轮工作 ｜ 1. 墨占比：正常字体特征（极性确证） ｜ 2. 宽度搜索：行剖面平滑度 ｜ 3. 新现象：墨密度自字形起始处**单调衰减** ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步

**发现/结论**（3 条，最多列 40）：

- → 确认 ARCH_359 §1 的极性判断：**1 = 背景，0 = 墨**。
- **结论：行剖面平滑度给出了宽度候选（w≈42）但不足以证实；
- **方法论**：本轮第二次因**边界/窗口错误**而撤回前轮结论

---

## `ARCH_364.md`

**标题**：ARCH_364 — 字形位区间开头存在字段（跨字形重复的 16 位前缀） ｜ 0. 本轮工作 ｜ 1. 硬证据：16 位前缀跨字形重复 ｜ 2. 墨分布方向 ｜ 3. 八种读法渲染（w × 方向 × 翻转） ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步

**发现/结论**（3 条，最多列 40）：

- 这是本轮唯一一条**独立的、可复现的结构性证据**（不依赖几何假设）。
- 白带），`w=32` 为均匀噪声。这与 ARCH_363 §2 的宽度搜索结论（最优 w=42）**一致**。
- **方法论**：本轮首次拿到**不依赖几何假设**的结构证据（前缀重复）。

---

## `ARCH_365.md`

**标题**：ARCH_365 — 撤回「开头字段」结论；位流确为正向读取的字形位图 ｜ 0. 本轮工作 ｜ 1. 结果：LCP 极不一致 → 无定宽字段 ｜ 2. 真正有价值的结论：位流是正向读取的字形位图 ｜ 3. 几何的现有限制 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步

**发现/结论**（4 条，最多列 40）：

- 若开头存在**定宽字段**，同组字形的 LCP 应当**一致**且等于字段宽度。实测不一致。
- → **ARCH_364 §1「字形位区间开头存在字段」结论撤回。**
- 实测 `delta` 覆盖 1068…2119，**不是任何单一 h 的整数倍**
- 则差距应来自宽度；可解 `w1×h=1038`、`w2×h=2454` ⇒ `w2/w1 = 2.365`，

---

## `ARCH_366.md`

**标题**：ARCH_366 — `.fnt` 位偏移表之前的区域：有结构但不是码表 ｜ 0. 本轮工作 ｜ 1. 实测 ｜ 2. 形态分析 ｜ 3. 否证：不是「码 → 字符」直表 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步

**发现/结论**（2 条，最多列 40）：

- **该推测未验证，不作为结论。**
- 若恰好覆盖其一，推测即被证实，并同时解出表项数与 `+0x14` 的矛盾。

---

## `ARCH_367.md`

**标题**：ARCH_367 — `{起始, 数量}` 游程解释否证 ｜ 0. 本轮工作 ｜ 1. 结果：否证 ｜ 2. 但字段分布给出了新的约束 ｜ 3. 本轮确定 / 未确定 ｜ 4. 下一步

**发现/结论**（4 条，最多列 40）：

- 第二字段之和（排除 0xFFFF）= 3,894,759
- 第二字段之和（排除 0xFFFF）= 91,119
- 改为**只用已确证的部分**产出交付物（容器/表/位流的平台无关解析器 +
- **方法论**：本轮是"换轴 → 否证"的收尾。`.fnt` 至此已确认的结构为：

---

## `ARCH_368.md`

**标题**：ARCH_368 — `.fnt` 已确证结论汇总（含未解项清单） ｜ 1. 已确证（可复现） ｜ 1.1 头部（`font_p5` / `c_font_p5` / `font_staff` 三个文件一致） ｜ 1.2 布局 ｜ 1.3 数据性质 ｜ 1.4 日/中字体差异 ｜ 2. 已否证（本轮及前几轮） ｜ 3. 未解项（如实列出） ｜ 4. 结论 ｜ 5. 复现命令

**发现/结论**（8 条，最多列 40）：

- 本轮按 ARCH_367 §4.3 执行：几何连续受阻，转为**固化已确证部分**，
- +0x04 u32 0x00010000
- +0x08 u32 0x00200020        (u16 32, u16 32)
- +0x0C u32 0                  <- font_p5 / font_staff ; c_font_p5 为 0x6C310000
- | 假设 | 结论 | 文档 |
- | `0xFFFFFFFF` 为哨兵 | 否证（是 `0xFF` 填充，越界读取） | ARCH_361 §1 |
- 1. **二维几何（宽 × 高）**——位流的起点、单位、读序均已确证，
- **建议**：目标项 (3) 应记为「格式解明（可复现）+ 渲染未达成」两段，

---

## `ARCH_369.md`

**标题**：ARCH_369 — `.fnt` 解析器交付（容器 + 位偏移表 + 位流），交叉验证通过 ｜ 0. 本轮成果 ｜ 1. 解析器覆盖范围 ｜ 2. 修正：`+0x10` 不是位偏移表的位置 ｜ 3. 三文件的解析结果（交叉验证一致） ｜ 4. 交叉验证 ｜ 5. 已达成 / 未达成 ｜ 6. 复现命令 ｜ 比对：期望 "IDENTICAL over 21 lines" ｜ 7. 下一步

**发现/结论**（6 条，最多列 40）：

- 同时修正一条此前的头部误读（见 §2）。
- [0, 24)          头部 ("FNT0", +0x04, +0x08=0x00200020, +0x0C, +0x10, +0x14)
- 实测：按 `+0x10` 取表只能得到 **2 项**，全部指标崩坏
- 符合「同格式、同规模」预期。
- （1=背景）均已确证，**唯一未定是二维几何（宽×高）**。
- 2. 若几何仍不解，目标项 (3) 应记为**「格式解明（已交付）+ 渲染未达成」**，

---

## `ARCH_36_JUDGE_CORE.md`

**标题**：ARCH_36 — 判定核心闭合：判定链 / 判定窗口表 / musicID 映射 / 谱面条目编码 ｜ 0. 首先更正 ARCH_35 的两处过度判断 ｜ 0.1 撤回：`0x813A7ED0` **就是**判定窗口表 ｜ 0.2 撤回：`0x813A5C9C` 的 48 项**不是**"谱面事件处理表" ｜ 1. 判定链完整闭合 ｜ 1.1 顶层：`mnsProcess_811251d6`（3612 B，mnsProcess.c）的 case 0 `[Ghidra]` ｜ 1.2 判定本体：`FUN_81123274`（162 B）`[Ghidra]` ｜ 1.3 判定窗口的取值链 ｜ 2. `musicData.c` 参数矩阵全清单 `[Ghidra]` ｜ 3. musicID ↔ 表下标映射（闭合）`[Ghidra]` + `[算术]` ｜ 4. 谱面条目格式（`+0x18` 条目） ｜ 4.1 stride 与字段宽度 `[Ghidra]` ｜ 4.2 16 位压缩值的解码 `FUN_811335d0` `[Ghidra]` ｜ 4.3 尚未闭合的部分（诚实标注） ｜ 5. 评分链路 ｜ 5.1 连续判定系数 `FUN_81123318`（198 B）`[Ghidra]` ｜ 5.2 评分/连击主逻辑 ｜ 6. 第 3 项完成度自评 ｜ 7. 下一轮待办

**发现/结论**（40 条，最多列 40）：

- > **文档编码**：UTF-8。所有地址为**真实 VA**（见 ARCH_34 的 0xF60 修正）。
- > **证据规范**：`[字节]` 原始字节 · `[算术]` 可复算 · `[Ghidra]` 伪代码/反汇编 ·
- 诚实优先：ARCH_35 在同一天内先"修正"后又被本轮证据推翻的两条，明确撤回。
- 不是判定窗口"，并称需按 ARCH_26"修正"。**该判断错误。**
- ⇒ `0x813A7ED0` 是 **`float[4][4]`**，以 `[level*4 + difficulty]` 访问，4 难度 × 4 等级。
- case flag & 4:   color = 0xff0000;     // 红
- case flag & 0x10: color = 0xff00ff00;  // 绿
- default:          color = 0xffffff00;  // 黄
- ⇒ 该表是 **mns 音符/图标的「坐标 getter + 绘制器」表**（红/绿/黄三色对应判定状态），
- 第 1–3 项是坐标对 getter（表 `0x813A6038 / 0x813A6080 / 0x813A60C8`，各 9 项 × 8 字节的
- `(x,y)` 屏幕坐标 float 对，实测 `(108,454) (852,454) (64,272) (896,272)…` = 960×544 布局）。
- **正确表述**：`0x813A5C9C` = mns 模块渲染器表（48 项）。
- ARCH_35 §3.4 中「谱面事件分发结构已确定」的结论作废；谱面事件的真实结构见本文 §4。
- **打包格式 `[算术]`**：`bit[15]` = 后半拍，`bit[16..31]` = 小节号，`bit[1..14]` = 小节内 1/16 拍。
- ⇒ **谱面坐标是「小节 + 半拍 + 1/16 拍」三级定位**，说明最小判定精度为 **1/16 音符的 1/2**（32 分音符）。
- └─ musicData_811331e8(difficulty, level)   ← 难度/等级系数表 0x813A7ED0
- 全部函数带 `difficulty < 4` 断言，行号取自断言字符串（**证明断言行号可精确还原**）：
- | 函数 VA | 表基址 | 索引公式 | 断言行号 | 类型 |
- | `8113342C` | `813A8018` | `[a*0x80 + b*0x20 + c*8]` | 0x5A/0x5B/0x5C | 3 维（与上一项相邻 +4 ⇒ 同结构体两个字段） |
- ⇒ 判定/评分参数是一个 **`难度 × 等级 × 子项` 的三维矩阵**，最宽维度 `a*0x80` ⇒ 4×4×4。
- uVar3 = (musicID * 0x3E0AD7A3 /* = 2^38/100 */) >> 0x26;      // == musicID / 100
- | 0 – 99 | `0x813A83F4` | musicID |
- | 100 – 199 | `0x813A84A0` | musicID − 100 |
- | 200 – 299 | `0x813A8550` | musicID − 200 |
- | 300 – 399 | `0x813A8600` | musicID − 300 |
- 除法常数验证 `[算术]`：`0x3E0AD7A3 = 2748779070 = 2^38 / 100`，
- `(n * 2748779070) >> 38 == n / 100` 对 `n ∈ [0, 999]` **全部成立**（实测 0 失败）。
- `0x811336C0`（上界）使用同样的常数 `DAT_81133764 = 0x3E0AD7A3`。
- **运行时状态**：4 张分页表初值**全部为 `0x813A834C`** `[字节]`：
- ⇒ 这些是**运行时指针表**：`0x813A834C` 是占位/默认块，谱面文件
- ⇒ **每拍记录 stride = `0x54` = 84 字节**；字段以 **16 位**为单位读取。
- **编码格式（3 种模式）**：
- ⇒ 谱面文件用**差分/查表压缩**存储判定窗口，符合 `mns%03d_%d.bin` 体积特征。
- - `0x54` 字节记录内**除 `+0`/`+2` 外的 40 个字段偏移尚未逐个确认**——需要继续追踪
- ⇒ **返回 0..1 的连续系数**，`window` 为窗口宽度、`interval` 为拍间隔、`delta` 为时间偏差。
- 后从 1 降到 0。**乘积峰值出现在 `0.4×window ~ window` 之间** ⇒ 这是「PERFECT 区间中心」的形状。
- **本轮未逐行读**，故不给出结论；列入下一轮。
- | 子任务 | 状态 | 证据 |
- | 判定窗口表 `0x813A7ED0` | ✅ **闭合** | `musicData_811331E8` 伪代码 + 4×4 float 实测值 |
- | `+0x18` 条目格式 | 🟡 **部分** | stride `0x54` + 16 位三模式编码已确定；40 个字段偏移未逐一确认 |

---

## `ARCH_370.md`

**标题**：ARCH_370 — span 的 gcd=1：否证「定高变宽位图」；两字体 span 几乎完全不同 ｜ 0. 本轮工作 ｜ 1. 结果 ｜ 2. 新事实：两字体 span 几乎完全不同 ｜ 3. 本轮确定 / 未确定 ｜ 4. 下一步（方向性调整）

**发现/结论**（6 条，最多列 40）：

- 实测 gcd = 1 ⇒ **不存在恒定的高度**。
- （源自 ARCH_349 的 `+0x08 = 0x00200020` 解读）的**第三次独立否证**：
- 1. **span 的 gcd = 1**（三文件）⇒ 不存在恒定高度，「定高变宽位图」否证。
- 但 ARCH_359 §1 已排除通用压缩（zlib/gzip/bz2/xz/lz4）。两者并不矛盾：
- 3. 若确认为裸位图且 gcd=1，则唯一的可能是**字形的行数也在变化**
- 极简、且不依赖任何几何假设的判据，它一次性排除了整类模型。

---

## `ARCH_371.md`

**标题**：ARCH_371 — RLE 否证（游程数与 span 相关 0.99）；自相关定步长未果 ｜ 0. 本轮工作 ｜ 1. 结果：位流是裸位图（RLE 否证） ｜ 2. 自相关定行距：未果 ｜ 3. 本轮确定 / 未确定 ｜ 4. 下一步

**发现/结论**（7 条，最多列 40）：

- 实测相反，且 `runs/span` 在两字体中稳定（0.310 / 0.167）。
- **同时确认**（第三次、这次是定量判据）：
- 既然确认为裸位图，尝试用**自相关**定位行距 `w`
- → **自相关法在本数据上不确定，不作为结论。**
- 3. 读序、极性、起点、单位四项均已确证。
- - 已确证 `gcd(span) = 1`（ARCH_370）⇒ **高度不恒定**，
- ⇒ 高度不恒定。定高是字体位图的常态，故必有别的机制：

---

## `ARCH_372.md`

**标题**：ARCH_372 — 否证「span = w×h + 常数填充」 ｜ 0. 本轮工作 ｜ 1. 结果：全部否证 ｜ 2. 累计已否证的几何/编码模型 ｜ 3. 剩余的可能性（均标注为推测，未验证） ｜ 4. 下一轮的可执行判据（针对可能性 1） ｜ 5. 本轮确定 / 未确定 ｜ 6. 结论

**发现/结论**（4 条，最多列 40）：

- | 模型 | 结论 | 依据 |
- ⇒ 即便「宽高均可变」模型也受限。
- `.fnt` 的字形几何在本会话中**未能解开**。已确证并交付的是
- 建议：目标项 (3) 记为**「格式解明（已交付）+ 渲染未达成」**两段，

---

## `ARCH_373.md`

**标题**：ARCH_373 — 同 span 连续段存在；`font_p5` 空白字形恰为 1038 位 ｜ 0. 本轮工作 ｜ 1. 结果：同 span 连续段真实存在，且对应空白或密集块 ｜ 2. 关键数字：一个「完整字形单元」占 1038 位 ｜ 3. 「32 高 + 开销」的定量检验：未命中 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一轮的可执行判据 ｜ 6. 结论

**发现/结论**（6 条，最多列 40）：

- - 对整张表不成立 ⇒ **非满单元字形不含（或含不同的）开销**。
- ⇒ `span` 具结构性，非纯内容长度。
- ⇒ 一个「满单元」占 1038 位。
- - 或接受几何未解，把目标项 (3) 记为「格式解明（已交付）+ 渲染未达成」。
- `.fnt` 的字形几何在本会话中未能解开。已确证并交付的是
- 目标项 (3) 应记为**「格式解明（已交付）+ 渲染未达成」**两段，不得合并为完成。

---

## `ARCH_374.md`

**标题**：ARCH_374 — 空白字形的零位集中在前 ~20 位；字形按周期 4 重复 ｜ 0. 本轮工作 ｜ 1. 结果：零位全部集中在前 ~20 位 ｜ 2. 推论（标注为推测） ｜ 3. 本轮确定 / 未确定 ｜ 4. 下一轮可执行判据

**发现/结论**（2 条，最多列 40）：

- **未验证，均不作为结论。**
- 2. 字形内容**以周期 4 重复**（两文件独立确认）。

---

## `ARCH_375.md`

**标题**：ARCH_375 — 用户提供的参考文档奏效：空白字形组精确符合「宽度字段 + h×w」 ｜ 0. 本轮输入 ｜ 1. 直接推论与验证 ｜ 2. 但该模型不覆盖全表 ｜ 3. 同时否证：周期 4 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一轮可执行判据

**发现/结论**（2 条，最多列 40）：

- 用户提供了一份**同型结构**的字体格式文档（来自另一游戏 The Tone Rebellion /
- - 头部 `+0x08 = 0x00200020` → `(32, 32)`，与「rows / columns」候选吻合；

---

## `ARCH_376.md`

**标题**：ARCH_376 — 渲染「几何确定」子集：仍非字符；F=14 判为巧合 ｜ 0. 本轮工作 ｜ 1. 结果：后段全空（符合空白字形），前段仍为噪声 ｜ 2. 本轮结论 ｜ 3. 已确证清单（累计，可复现） ｜ 4. 未达成 ｜ 5. 下一步（本会话内的最高价值动作）

**发现/结论**（3 条，最多列 40）：

- （由参考文档结构推得 `span = F + h·w`，取 `h=32, F=14` ⇒ `w = (span−14)/32`）。
- —— Persona 4 Golden 的 FNT 格式文档。**若能取得该文件内容（或仓库源码），
- 我即可直接对照已确证的结构验证。

---

## `ARCH_377.md`

**标题**：ARCH_377 — 列主序否证；本会话 `.fnt` 几何结论与阻塞点 ｜ 1. 本轮结果 ｜ 2. `.fnt` 已否证模型汇总（13 类） ｜ 3. `.fnt` 已确证事实（可复现，已交付） ｜ 4. 阻塞点（具体、可复现） ｜ 5. 目标项状态

**发现/结论**（3 条，最多列 40）：

- - 位流的**起点、单位、读序、极性**四项均已确证；
- - 唯一有效的外部线索（用户提供的同类格式文档）只覆盖**空白字形组**。
- jsDelivr / unpkg / github.io），因此无法读取社区已文档化的格式说明：

---

## `ARCH_378.md`

**标题**：ARCH_378 — 突破：`.fnt` 是 `FNT0` 压缩格式；渲染成功 ｜ 0. 突破来源 ｜ 1. 格式真相 ｜ 2. 直接调用社区库解析我们的文件 ｜ 3. 几何完全解释 ｜ 4. 渲染成功（目标项 3 后半达成） ｜ 5. 根因：此前的 13 项否证全部作用在**压缩数据**上 ｜ 6. 对目标项 (5) 的价值 ｜ 7. 复现 ｜ 产物：C:\dev\p5d_build\_fnt\pe_out\{font_p5,c_font_p5,font_staff}.fnt.png ｜ C:\dev\p5d_build\_fnt\pe_out\{font_p5,c_font_p5}.fnt.table.xml ｜ 8. 方法论（本会话最重要的一条）

**发现/结论**（4 条，最多列 40）：

- → **`.fnt` 是 `FNT0` 格式，且属于「压缩变体」**：字形数据经**自定义字典压缩**
- **结论：ARCH_362–377 中所有针对「位图几何」的否证均作废**
- `gcd=1` 等"强证据" → 否证整类模型 → 越来越深地偏离真相。
- **统计量再漂亮，也不能替代「先确认数据是什么」。**

---

## `ARCH_379.md`

**标题**：ARCH_379 — 重大更正：A（`data/`）是**韩文**版，不是日文原版 ｜ 0. 本轮发现 ｜ 证据 1：`font_p5.fnt` 把反斜杠渲染为 `₩` ｜ 证据 2：`c_font_p5.fnt` 同一位置是普通 `\` ｜ 证据 3：`font_p5.fnt` 含韩文（Hangul） ｜ 结论 ｜ 1. 这解释了此前的反常观察 ｜ 2. 必须作废的推论 ｜ 2.1 ARCH_350 的三组锚点**作废** ｜ 2.2 「A = 日文原版」这一全局前提作废 ｜ 2.3 稠密索引映射 `(b1−0x80)*256 + b2` **否证** ｜ 3. 本轮确定 / 未确定 ｜ 4. 下一步 ｜ 5. 复现 ｜ 已导出的图集 ｜ 本轮对比图

**发现/结论**（4 条，最多列 40）：

- 在 ARCH_378 打通 `.fnt` 渲染后，直接查看字形图集，得到两条决定性证据：
- `ます` 映射**不成立**。**ARCH_350 §1.2 的结论撤回。**
- C 版一致，此前我在 ARCH_353/354 中对该前提的"待确认"标注现在可以
- 肉眼确认「这是什么」，比在未确认的数据上堆统计量有效得多。**

---

## `ARCH_37_MNS_COMPLETE.md`

**标题**：ARCH_37 — mns 判定核心全部闭合：条目字段布局 / 对象结构 / 连击·评级·回调链 ｜ 0. 本轮结论 ｜ 1. 谱面条目格式（第 3 项核心，完全闭合） ｜ 1.1 五个访问器 `[Ghidra]` ｜ 1.2 记录布局 `[算术]` ｜ 2. mns 判定对象结构 `[Ghidra]` ｜ 3. 评分 / 连击链路（闭合） ｜ 3.1 判定成功的处理 `[Ghidra]`（`mnsProcess_811251d6` case 0 尾部） ｜ 3.2 评级系数的更新规则 `[Ghidra]` ｜ 3.3 断连与回调 `[Ghidra]` ｜ 3.4 判定评分函数 `FUN_81123318`（见 ARCH_36 §5.1） ｜ 4. 对前文两处的更正 ｜ 4.1 `0x813A5EDC` 是事件对象模板，不是分派表 ｜ 4.2 `FUN_811233DE` 是音符布局计算，不是评分函数 ｜ 5. 第 3 项完成度（最终） ｜ 6. 下一轮待办（剩余 4 轮）

**发现/结论**（26 条，最多列 40）：

- > **编码** UTF-8。地址为**真实 VA**（ARCH_34 的 0xF60 修正）。
- > **证据**：`[字节]` 原始字节 · `[算术]` 可复算 · `[Ghidra]` 伪代码 · `[推断]` 未确证。
- | 1 | **谱面条目（stride `0x54`）的完整字段布局已实证解出**（第 3 项「`+0x18` 条目格式」闭合） |
- | 4 | 更正 ARCH_35 §4.3：`0x813A5EDC` 不是"二级分派表"，而是 **0x90 字节的事件对象模板** |
- | 函数 VA | 断言行号 | 返回表达式 |
- **⇒ 每拍最多 4 个音符/事件**，每个音符 3 个 32 位字段（偏移 `+0x04 / +0x0C / +0x10`）。
- **断言行号序列的旁证价值** `[算术]`：`0xBA(186) → 0xC6(198) → 0xD2(210) → 0xD7(215) → 0xDF(223)`
- 是源码中连续定义的 5 个访问器，证实**行号可从断言字符串精确还原**（第 4 项要求的能力）。
- | 偏移 | 类型 | 语义 | 证据 |
- | `+0x2D8` | **float** | **评级系数 ∈ [−1, 1]** | `±0.2`、钳位 `0x3F800000`/`0xBF800000` |
- *(int *)(self + 0x28C) += 1;                      // ★ 连击数 +1
- if (*(int *)(self + 0x3E0) != 0) {                    // ★ 判定回调（成功）
- | 同上 | 断连时置 `0xBF800000`（`−1.0`） | 归零 |
- ⇒ **评级系数为负（刚断连）时只能拿二级判定**。这解释了 P5D 中"断连后立即按键无法拿到最高判定"。
- ((*(uint*)(self + 0x210) & 0x10000000) != 0 && !bVar4)) {
- *(undefined4*)(self + 0x2D8) = 0xBF800000;        // 评级 = −1.0
- (**(code **)(self + 0x3E0))(2, &local_1170);    // ★ 判定回调（失败）
- "事件类型 → 处理函数组"的二级表。**本轮证据推翻** `[Ghidra]`：
- `FUN_812b2270` 是 `memcpy(dst, src, n)` 形式的复制；`0x813A5EDC` 是**模板源**，
- ⇒ `0x813A5EDC` = **mns 事件对象的初始模板（144 字节）**。
- ⇒ 它按 BPM 与拍位缩放计算**每个音符的滚动位置**，服务于渲染，不是评分。
- | 子任务 | 状态 | 证据位置 |
- | 判定窗口表 `0x813A7ED0` | ✅ **闭合** | ARCH_36 §0.1 / §2（`float[4][4]`，4 难度 × 4 等级） |
- | `+0x18` 起的条目格式 | ✅ **闭合** | 本文 §1（stride `0x54`，2×ushort 窗口 + 4×20 B 子项） |
- 1. **第 2 项**：用 `func_tables.tsv` + app 状态机（7 态）+ 序列调度器 `0x811DB1F0`
- 故时序表将以**静态调用链 + 已有帧号证据（fc 2810–2960 渲染 LOGO）** 给出，并明确标注哪些帧号是实测、哪些是推断。

---

## `ARCH_380.md`

**标题**：ARCH_380 — 图集可直接读字；`(b1−0x88)*256+b2` 方向正确但仍有偏差 ｜ 0. 本轮工作 ｜ 1. 索引公式的界内性检验 ｜ 2. 目视读字结果 ｜ 3. 关键进展与意义 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步（具体可执行）

**发现/结论**（2 条，最多列 40）：

- ARCH_378 打通 `.fnt` 渲染、ARCH_379 确认 **A（`data/`）= 韩文版**之后，
- **上述均为推测，不作为结论。**

---

## `ARCH_381.md`

**标题**：ARCH_381 — 否证「纯线性基址」；索引必为分段 ｜ 0. 本轮工作 ｜ 1. 结果：无任何基址读得出通顺文本 ｜ 2. 与既有证据一致：索引必为**分段** ｜ 3. 本轮确定 / 未确定 ｜ 4. 下一步（方向） ｜ 5. 复现 ｜ base_sweep.png 由本轮内联脚本生成（同目录）

---

## `ARCH_382.md`

**标题**：ARCH_382 — 占用图否证「行内连续分段」；更正 ARCH_381 的一处推理缺陷 ｜ 0. 本轮工作 ｜ 1. 结果：每行都是稀疏占用，无一行连续 ｜ 2. 更正：ARCH_381 §2 的推理前提有误 ｜ 3. 占用图本身给出的新事实 ｜ 4. 本轮确定 / 未确定 ｜ 5. 交接说明（round 60/60，最后一轮）

**发现/结论**（4 条，最多列 40）：

- **BASE 滑窗实测** —— 6 个候选基址读出的字形均不成词 ——
- ARCH_381 的滑窗实测否证。
- 4. 线性基址模型仍否证（依据为 BASE 滑窗实测）。
- - **目标项 (3)**：`ARCH_378` —— `.fnt` 是 `FNT0` **自定义字典压缩**格式；

---

## `ARCH_383.md`

**标题**：ARCH_383 — C（中文）版同样映射到图集：读出中文字但索引仍有偏移 ｜ 0. 本轮工作 ｜ 1. 检验对象 ｜ 2. 结果：读出中文字，但语义不符 ｜ 3. 与 A（韩文）版的对照 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一轮建议（上下文耗尽，交接） ｜ 6. 复现 ｜ 本轮：从 c_font_p5 图集读 C 版锚记录的码 ｜ 见 ARCH_383 内联脚本；产物 read_C005.png ｜ 通用工具：_fnt/read_atlas.py（改 PNG 路径即可切到 c_font_p5）

**发现/结论**（3 条，最多列 40）：

- - **是中文字** ⇒ **映射方向正确**（码 → 图集索引 → 字形），
- - **但语义不符**（应为「前往 / 進入」一类） ⇒ **索引仍有偏移或分段**。
- 且该机制**不是** `(b1−B)*256 + b2` 的纯线性式（ARCH_381 的 BASE 滑窗已实测否证）。

---

## `ARCH_384.md`

**标题**：ARCH_384 — `RectTable` 独立确认图集几何与字形数 ｜ 1. 结果 ｜ 2. 与独立测量的一致性 ｜ 3. 意义 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一轮建议（新增一条更省力的判据） ｜ 6. 复现 ｜ RectTable 由 ARCH_378 的 C# 探针导出：

**发现/结论**（1 条，最多列 40）：

- → **图集几何确认**：`243 行 × 16 列`，索引

---

## `ARCH_385.md`

**标题**：ARCH_385 — 用 `RectTable` 有效性搜索映射：判据偏弱，未定案 ｜ 0. 本轮工作 ｜ 1. 结果 ｜ 2. 判据的缺陷（本轮发现） ｜ 3. 本轮确定 / 未确定 ｜ 4. 状态与交接（轮次已用尽）

**发现/结论**（1 条，最多列 40）：

- - **目标项 (3)**：`.fnt` 为 `FNT0` **自定义字典压缩**格式（ARCH_378）；

---

## `ARCH_386.md`

**标题**：ARCH_386 — 图集可读：C 版为**繁体**中文（确证用户所述）；映射偏移非常数 ｜ 1. 重大确证：C 版是繁体中文 ｜ 2. 锚点字符已在图集中定位 ｜ 3. 但映射偏移**不是常数** ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步（已非常具体） ｜ 6. 复现 ｜ 本轮产图

**发现/结论**（6 条，最多列 40）：

- **这同时定案了 ARCH_353/354 的遗留问题**：当时我把该前提标为「待重新确认」
- **该细节仍未解，但 C 版为繁体这一点已确证。**
- **两处偏移不同（258 vs 189）⇒ 单一线性式再次否证。**
- 但两处偏移相差约 70，远大于该误差，故「偏移非常数」的结论稳健。
- **行/列号待用程序精确定位后修正。**）
- 用户在会话早期的说法成立，ARCH_353/354 的「待确认」标注可结案。

---

## `ARCH_387.md`

**标题**：ARCH_387 — 两组精确点对 + 行步长 ≈ 188（SJIS 尾字节数） ｜ 1. 方法：带列分隔线渲染，逐列数出位置 ｜ 2. 两组精确点对 ｜ 3. 由点对导出的约束 ｜ 4. 本轮确定 / 未确定 ｜ 5. 下一步（明确、可执行） ｜ 6. 复现 ｜ 本轮产图（列分隔线，5 倍放大）

**发现/结论**（3 条，最多列 40）：

- 图集的可读性已由 ARCH_386 确认。为**精确定位**，把目标行的 16 格
- R + 24 = 211   ⇒   R = 187
- 4. C 版为繁体中文（ARCH_386 已确证）。

---

## `ARCH_388.md`

**标题**：ARCH_388 — 🎯 目标项 (5) 达成：`msg` CJK 映射公式确证 ｜ 0. 结论 ｜ 1. 推导过程 ｜ 1.1 由两组精确点对导出（ARCH_387） ｜ 1.2 独立验证（第三种记录，本轮） ｜ 2. 与历史线索的一致性 ｜ 3. 本轮确定 / 未确定 ｜ 4. 下一步 ｜ 5. 复现 ｜ 验证图 ｜ 图集

**发现/结论**（3 条，最多列 40）：

- **`msg` 的 CJK 双字节码到字形图集索引的映射公式已确证，并通过独立记录的目视验证。**
- **两点偏移完全相同（−654）** ⇒ 公式成立，且行步长 188
- 确认公式同构（常数可能要调）。

---

## `ARCH_389.md`

**标题**：ARCH_389 — 资源解析搬进移植代码：现状核实 + 补上最后一个空洞（FCT0） ｜ 0. 任务 ｜ 1. 核实结果：该普查已严重过时 ｜ 2. 补上唯一真正的空洞：FCT0（`.ctbl`） ｜ 交叉验证 ｜ 3. 仍然缺少解析器的格式（据本轮核实） ｜ 4. 本轮确定 / 未确定 ｜ 5. 建议 ｜ 6. 复现 ｜ 期望：IDENTICAL over 9 lines ｜ 命中数普查

**发现/结论**（10 条，最多列 40）：

- 按同一方法（`src/` 下 `*.h/*.cpp` 全文搜索）实测：
- | magic | `UNRESOLVED.md` 报告 | **实测** |
- → **「移植代码没有任何资源格式解析器」这一结论不成立。**
- `FCT0` 实测仍为 0 命中，故本轮新增平台无关解析器：
- | 格式 | 规模 | 现状 |
- 1. `UNRESOLVED.md` §2.1 的「0 命中」普查**已过时**：实测 `GFS0/ GMD/ GNF/ PLG0/
- 2. **`FCT0` 是唯一真为 0 的格式**，本轮已补解析器并交叉验证
- 3. 上述仍缺格式的解析。
- 按实测状态改写——当前版本会误导后续工作（把已完成项当作结构性缺口）。
- 2. 阻断级第 2 项应重新表述为「**补齐剩余少数格式**」而非「从零搬解析器」。

---

## `ARCH_38_RUNTIME_CHAIN.md`

**标题**：ARCH_38 — 运行链路全量：app 状态机 7 态实证 / 三层调度 / 逐阶段时序 ｜ 0. 本轮结论 ｜ 1. app 状态机 7 态（实证）`[字节]` + `[Ghidra]` ｜ 1.1 状态表 ｜ 1.2 状态切换协议 `sequence_81185616`（kernel/sequence.c，394 B）`[Ghidra]` ｜ 2. 三层调度结构 ｜ 2.1 具名序列清单 ｜ 3. 逐阶段时序表 ｜ 3.1 实测部分 `[实测]` ｜ 3.2 静态推断部分 `[推断]` ｜ 3.3 必须明说的边界 ｜ 4. 子系统协作（静态归属） ｜ 5. 第 2 项完成度自评 ｜ 6. 下一轮待办（剩余 3 轮）

**发现/结论**（38 条，最多列 40）：

- > **编码** UTF-8。地址为**真实 VA**（ARCH_34 的 0xF60 修正）。
- > **证据**：`[字节]` · `[算术]` · `[Ghidra]` 伪代码 · `[实测]` 运行时观测 · `[推断]` 未确证。
- | 1 | **app 状态机 7 态的名字与函数指针全部实证解出**（表项内嵌状态名字符串） |
- | 2 | 表基址确认为 `0x813B01B4`（`DAT_813b01ac` 是运行时指向它的变量），步进 `0x50` |
- | 4 | 三层调度结构闭合：宿主循环 → app 状态机（每帧 update）→ 任务/序列调度器 `0x811DB1F0` |
- | 5 | 逐阶段时序表给出，**实测与推断逐条分离** |
- 表基址 `0x813B01B4`，步进 `0x50`；每项 `+0x00` 为 12 字节**状态名字符串**，
- （state 7 处无有效表项：`0x813B03E4` 的 `+0x44` 是 `0x8130989C`，不是代码指针 ⇒ **共 7 态**。）
- +00 = 0x55424544 -> 44 45 42 55 = "DEBU"
- +04 = 0x454D2047 -> 47 20 4D 45 = "G ME"
- +08 = 0x0000554E -> 4E 55 00 00 = "NU"
- **状态 0 的 enter 桩只有 2 字节** `[字节]`：`0x811858CE` 处为 `70 47` = `BX LR`
- ⇒ 正式版中 DEBUG MENU 是**空实现**（被编译掉），与其余 6 态均有实体实现一致。
- **交叉验证** `[Ghidra]`：state 3「COMMU」的 enter 指向 `0x810C84D8`，该函数被归属为
- - `update` 返回 `1` ⇒ 允许离开 ⇒ 自动调 `leave`
- - 离开后状态置 `-1`（与实测的「state `-1` → `1`」完全一致）
- ③ 每帧任务/序列调度器 0x811DB1F0
- for (t = [0x816CD9E0]; t; t = t[+0x4C])
- ④ 序列框架（唯一具名节点工厂 0x8116F7CA）
- create/注册双链表；find 按 name hash 0x81165288 = Σ s[i]<<(i&7)
- （③④ 的字段与函数地址来自 ARCH_30 的逐条 `[DIS]` 证据，本轮复核一致。）
- | B | `initDataRead` 的 9 个子任务 | **= state 1「initread」**，与实测卡住点一致 |
- | 阶段 | 证据 | 引擎帧号 |
- ⇒ **唯一有帧号实测的阶段是 LOGO（fc 2810–2960，约 150 帧 = 2.5 s @60fps）。**
- | LOGO 三相 | 1 内相位（区段 26 尾 logo.c） | 区段 26 | **2810–2960（实测）** |
- **app state 1「initread」的 `appSound.c:1003` 断言**处失败：
- [ASSERT-ABORT] #1 lr=0x8115F9D0  r1=0x81306D04  r3=0x813072A0  r5=0x3EB
- ⇒ `initDataRead` 无法完成 → 状态机停在 `1` → **影片/标题/主菜单/选歌/对局/结算
- deltaTime 由两次 `sceKernelGetProcessTimeWide` 推导，实测约 6.8× 过速，
- 且 `DISPCALL=0`/`SETFB=0` ⇒ **无 vsync**）。
- ⇒ **第 2 项中「含引擎帧号与耗时」这一子要求只能部分满足**：
- LOGO 段有实测帧号；其余阶段的帧号与耗时**在修复音频断言与时钟之前无法取得**。
- | 场景图 | 21/27 | 3 条全局链 `0x816CD468[idx*4]`，vtable `0x814D2F2C`（ARCH_35 §4.1） | 每帧 walk `0x811CBE9A` |
- | 输入 | 22 | `appPad.c`、`app_pad_device.c` | 每帧；全局 `0x816CDFB0/B2/B4` |
- | 计时 | 19/20 | 双 `sceKernelGetProcessTimeWide`（`0x812071F8`） | 每帧 |
- | app 状态机 7 态 | ✅ **本轮实证**（名字 + 6 个回调指针 + 切换协议） |
- | 每帧任务/序列调度器 `0x811DB1F0` | ✅（ARCH_30 逐条 `[DIS]` 证据） |
- | 逐阶段时序 | 🟡 **部分**：LOGO 段实测；其余因端口卡在 state 1 无运行时数据 |

---

## `ARCH_392_AUDIO_FIX.md`

**标题**：ARCH_392 — 修音频：根因（未推送 WAV）+ 流式播放器 + 上机取证 ｜ 0. 一句话结论 ｜ 1. 根因 ①：数据从来没推过 ｜ 2. 根因 ②：播放器实现（三个独立缺陷） ｜ 2.1 一次性加载 62 MB（设计缺陷） ｜ 2.2 ★ 单位混用：`winStart_ + winBytes_`（帧 + 字节） ｜ 2.3 Android 上 `FILE*` 路径不稳（改用低层调用） ｜ 3. 实测教训：music218.wav 前 ~40 秒本来就是静音 ｜ 4. 顺手修掉的两个隐患 ｜ 5. 新增/改动的文件 ｜ 5.1 上机验证的环境坑：锁屏会掐掉音频 ｜ 6. 可复现步骤 ｜ 1) PC 侧：流式读取必须与一次性读取逐字节相同（28 首） ｜ 期望：每首 STREAM OK + fnv 相同，STREAM_SUMMARY bad=0 total=28 ｜ 2) 推送要播的那首歌 ｜ 3) 构建 + 上机验证 ｜ 4) 回归红线 ｜ 7. 仍未做 / 已知边界

**发现/结论**（15 条，最多列 40）：

- > 本文所有结论均为**实测**；推测显式标注。禁用近似 hack 冒充正解。
- | # | 根因 | 证据 | 修法 |
- ⇒ `loadWavPcm()` 的 `fopen` 直接失败 ⇒ `audioCb()` 走 `memset` 分支 ⇒ 恒零。
- 与 ARCH_391 §5 的猜测（「极可能只是没找到文件」）**一致，现已确证**。
- 于是窗口 0 被误认为覆盖 `[0, 192000)` 帧 ⇒ `pos_` 越过 48000 帧后**不滑窗**，
- `_wavstream/diag11` 直接给出证据：`pos=48000` 之后 `winStart` 仍为 0。
- ⇒ **不是读不到，而是那一段本来就是零**（见 §3）。但既然已经上机，
- 最初的内容自检只读**头 1 秒**，报 `peak=0`，看起来像"文件是坏的"。三条独立证据推翻它：
- ⇒ **文件正确，前面就是静音前奏**。自检随即改为**全曲 8 段抽样**：
- [audio] 格式一致：2ch/48000Hz 曲长 323.13s
- 窗口重建会重新 `setupScene` ⇒ 重新 `loadWavPcm`，不归零就会"旧帧数 + 新音乐"错位。
- | `src/android/main.cpp` | 改用 `Stream`（原子指针交接给音频线程）；全曲抽样内容自检；格式比对；输出峰值/帧数打点；`audioStop` 归零 |
- | `_wavstream/_diag/` | 定位过程中用过的 13 个诊断程序（保留可复现证据） |
- `termGL` 之后主循环 `if (!g.glReady) continue;` ⇒ 不再渲染、不再打 `[play]` 周期行。
- 实测 `OUT_FRAMES` 与 `g_audioFrames` 同步增长（无 xrun 迹象），但**未做专业 xrun 统计**。

---

## `ARCH_393_FRAMING_FIX.md`

**标题**：ARCH_393 — 修取景：真凶是选错的相机文件（附第一轮的错误路线） ｜ 0. 一句话结论 ｜ 1. 第一轮路线是错的（务必不要重走） ｜ 2. 定位过程（每一步都可复现） ｜ 2.1 上屏 letterbox 不是主因，但它确实是 35% 黑边的直接来源 ｜ 2.2 用截图 + NDC 定量，发现真正的瓶颈 ｜ 2.3 `_camcmp/camcensus`：115 个相机文件的普查 ｜ 2.4 它不是"选错了子镜头"，文件本身结构正常 ｜ 3. 修法（不硬编码文件名） ｜ 4. 新增文件与工具 ｜ 5. 仍未做 / 已知边界

**发现/结论**（13 条，最多列 40）：

- > 所有结论为实测；推测显式标注。
- `HANDOFF_ARCH_391` §4 断言根因是 `p5d_render_core.h:1033` 的
- **照做的实测结果：更糟。**
- `aspect` 变大 ⇒ `proj[0]` 变小 ⇒ **水平方向更窄**。
- 而离屏画布 `FW/FH` 仍是 420×640 不变 ⇒ 只改投影必然让取景更小。
- "证据不足把话说过头"的先例，这次是接手文档自己犯的。
- if (As > Ac) sx = Ac / As;                   // 0.295 ⇒ 左右各留 ~35% 黑
- > **几何上无法两全**：画布 0.656 与屏幕 2.222 之间，等比铺满 ⇒ 只能占 29.5% 宽；
- 垂直半角 7.6° ⇒ 该距离处可见高度 = 2*4438*tan(7.6°) ≈ 1185 单位
- ⇒ 角色占画面高度 ≈ 178/1185 ≈ 15%（NDC 上 0.26 与"角色只占高度 1/4"一致）
- ⇒ 不是解析 bug（位置 `= half × positionScale`，与它自己的 bbox 精确吻合），
- 对每个候选: 解析后算位置跨度; > kCamPosSpanMax ⇒ 拒收并试下一个
- E 相机 .../c218.GFS 位置跨度=4484 > 1500（量级异常，拒收）⇒ 试下一个候选

---

## `ARCH_394_OFFLINE_FONT_BLOCKER.md`

**标题**：ARCH_394 — 离线运行时的真正断点：字体资源未加载 → guest 自己断言中止 ｜ 0. 一句话结论 ｜ 1. 先纠正两条过时结论 ｜ 2. 怎么离线复现（无需真机） ｜ 3. 断点定位（工具化，可复查） ｜ eboot size=0x5FE2C0 header_len=0x1000 ELF@0x1000 entry=0x002B2B00 phnum=4 ｜ ph[0] PT_LOAD vaddr=0x81000000 filesz=0x321810 foff=0x10E0     ← SEG0 代码 ｜ ph[1] PT_LOAD vaddr=0x81325000 filesz=0x21BA74 foff=0x3228F0 ｜ import stubs resolved: 406（35 库） ｜ 3.1 字体获取函数 `0x8102207C` ｜ 3.2 唯一的调用点 `0x8101DDF0`（`appFontUnitList` 初始化） ｜ 3.3 游戏请求的字体资源（字符串表实测） ｜ 4. 下一步：把哑对象换成真实字体对象（**未做**） ｜ 5. 仍未验证 / 已知边界

**发现/结论**（47 条，最多列 40）：

- > 本文只写**实测**；推测显式标注。工具新增于 `C:\dev\p5d_build\_dis\`。
- [ASSERT-ABORT] #1 lr=0x8115F9D0 ...
- [EBOOT-START] stop=3(BADMEM) last_pc=0x8115F9D6 totalInsn=590720000
- 字体文件其实**打开成功**      [QRES] nm='app_system/font/c_fo' r6=0x11D124  (=1167652B = 磁盘上 c_font_p5.fnt 大小)
- ⇒ 空槽塞一个清零哑对象      [FONT-DUMMY] #1..#10 empty font slot -> 0xB2CED000
- ⇒ 下游判空断言             0x8101DDF0 bl 取字体 → cmp r0,#0 → 为 0 就 assert
- ⇒ guest ABORT              0x8115F9D6 → BADMEM 终止
- 而**真正的阻断点是这个断言** —— 两者是不同的事，之前混为一谈。
- | 旧说法 | 实测 |
- [PUMP] mgr=0xB207A030 ctx=0xB1B1A030 候选=5 推进=5
- [PUMP-DONE] req=0xB207602C ret=0x1 f4 0x0 -> 0x1      ← 完成位置位（h145k1152 的修法）
- **但**：加大预算（`P5D_CAP=12e9` / `60e9`）暴露了**新的、更靠后的**断点 —— 断言。
- $env:P5D_CAP='60000000000'      # 指令预算（默认 1.2e9；不够就走不到断言）
- 关掉自动按键可确认它只影响时间线，不影响断点本身。
- | `dis_assert.py` | 解析 FSELF（**`header_len=u32@0x10`，ELF 就在该偏移**）→ VA↔文件偏移 → 反汇编 |
- | `dis2.py` | 同上，并在反汇编里标注 `bl/blx` 命中哪个**导入存根** |
- | `stubs.py` | 导出全部 35 个库 / 406 个导入存根，支持 `VA → 库+NID` 查询 |
- 0x812B2810 -> SceLibKernel nid=0x37691BF8     ← 断言失败路径
- 0x812B22B0 -> SceLibc      nid=0x802FDDF9
- 0x810220C8  ldr  r0, [r4, #4]          ; 槽数组长度
- 0x810220CA  cmp  r6, r0
- 0x810220CC  blo  #0x810220e4           ; 越界则 → 断言(line=0x89=137, file=0x812EEED4)
- 0x810220E4  ldr  r1, [r4]              ; 槽数组基址
- 0x810220E8  ldr.w r6, [r1, r6, lsl #2] ; 槽[index] → 字体对象
- 0x810220EC  cbz  r6, #0x810220f6       ; ★ 空槽 ⇒ 走 epilogue，调用方拿到 NULL
- 0x810220EE  adds r0, r6, #0           ; 非空 ⇒ 返回真实字体对象
- 0x810220F6  <epilogue>                  ← 探针正是在这里塞哑对象（thumb_interpreter.cpp:2104）
- 0x8101DDF0  bl  #0x8102207c      ; 取字体
- 0x8101DDF4  str r0, [r4, #8]     ; 存进条目
- 0x8101DDF6  cmp r0, #0
- 0x8101DDF8  bne #0x8101de20      ; ★ 非空 → 正常路径
- 0x8101DDFA  ... assert(file, 0x16B, ...)   ;   为空 → 断言
- ⇒ **空字体对象是这个断言唯一的直接原因**，与 HLE 缺函数、内存布局、调度都无关。
- 0x812EFAD4  app_system/font/font_p5.fnt      ← assert 所在 appFontUnitList 用这个
- 0x812EFB30  app_system/font/c_font_p5.fnt    ← 本版本(A=韩文/C=中文)实际请求的是 C 版
- 0x812EFB10  app_system/font/c_font_p3.fnt
- 0x812EFAB8  app_system/font/font_p3.fnt
- 0x812EFAF0  app_system/font/font_staff.fnt
- + `font.ctbl`(22024B)。运行日志里 `r6=0x11D124` = **1167652** ⇒ 打开的正是 `c_font_p5.fnt`。
- 1. **文件入口**：探针 FS HLE 已能打开并读到字节（`[QRES] … r6=0x11D124`），

---

## `ARCH_395_PROBE_GATES_AND_FONT.md`

**标题**：ARCH_395 — 打通离线探针的两道探针门控 + 字体存活断言的深入 ｜ 0. 一句话结论 ｜ 1. 本轮实际踩到的现象与诊断过程 ｜ 2. 修法（三处，都已落地） ｜ ★★ 第三道坑：钩子里的 `g_wp_cur` 冗余条件（本轮最大教训） ｜ 3. 字体存活断言的新证据（与 ARCH_394 的结论**不同**） ｜ 3.5 ★★ 381 断言的完整上游链（ARCH_395b，新证据） ｜ 上游三步（反汇编实测） ｜ `0x8102056C`：按字体管理器类型分派（**真正失败的地方**） ｜ `0x8102490C`（type==1 分支）：字形索引查找返回 -1 就返回 NULL ｜ `0x81020D06`：cmap/JIS 区间查找（返回 -1 = "该字符不在字体里"） ｜ 仍未钉住的两点（下一步） ｜ 3.6 ★★★ 字体数据其实**已经载入 guest 内存**（ARCH_395m，本轮最强证据） ｜ 于是矛盾集中到 `0x81020D06` ｜ 3.7 ★★ 字体数据本身完好；矛盾落在"索引语义"上（ARCH_395n/p） ｜ 运行时读到的值 = 磁盘文件逐字节相同 ｜ 索引方式的两处修正（前几轮我算错了） ｜ 由此得到的强怀疑（**未验证**） ｜ 3.8 ★★ 两个"字符码"的真实来源（ARCH_395q） ｜ 由此推翻的两个假设 ｜ 于是 `0x81020D06` 的语义**仍然存疑**（未验证） ｜ 3.9 ★★ `r4`/`r5` 是"缓存键"；补丁点本身也在同一条链上（ARCH_395r） ｜ `0x8102162A` / `0x81021672` 是**缓存查找/填充** ｜ ★ 补丁点 `0x810220F2` 就在同一条链上

**发现/结论**（253 条，最多列 40）：

- > 承接 `ARCH_394_OFFLINE_FONT_BLOCKER.md`（字体未加载 ⇒ guest 断言中止）。
- 两者都不过 ⇒ **整段探针链被跳过，钩子一声不响**。
- `ARCH_394` 的字体补丁点 `0x810220F6`（页 0x220，**本就在 `P[]` 内**）一直正常。
- 0x810220EE   [FONT-SLOT]   零命中
- 0x810233C6   [FONT-ALIVE]  零命中
- 0x8102340A   [FONT-KEEP]   零命中
- 1. 先怀疑地址写错 → 用 `_dis/dis_multi.py` 复核反汇编，地址无误。
- 2. 再怀疑"代码没执行" → 但 `[ASSERT-SITE] caller=0x81023404`
- （= `0x81023400` 那条 `bl` 的返回地址）证明该函数体**确实执行过**。
- //    0x233/0x234 = 字体存活断言链；0x249 = cmap 查找 0x8102490C；0x20D = 0x81020D06
- || ((cur&~1u)>=0x81020500u&&(cur&~1u)<0x81020620u)
- || ((cur&~1u)>=0x810233C0u&&(cur&~1u)<0x81023420u)
- || ((cur&~1u)>=0x81024900u&&(cur&~1u)<0x81024A00u)
- || ((cur&~1u)>=0x81020D00u&&(cur&~1u)<0x81020E30u)
- // ③ ★ 钩子里**不要**再写 `g_wp_cur &&`（ARCH_395f）
- 现象：`0x810216B2`（调用点）与 `0x81020590`（函数内部）的钩子，
- [FONT-PC216] pc=0x810216B2 第2次执行 g_wp_cur=0
- ⇒ **指令确实执行了（还 ≥2 次），但那一刻 `g_wp_cur==0`**。
- 而该链本身被 `_pf_go` 预筛跳过 ⇒ 链内的 `g_wp_cur` 保持初值 0 ⇒
- ARCH_394 假设"哑对象全零 ⇒ `+0x00==0` ⇒ 断言"。**实测推翻了这个假设的一半**：
- [FONT-ALIVE] r0=0xB068B044 [r0]=1    ← 对象是**游戏自建**的（不是我方哑对象 0xB2CED000）
- lr=0x810216C2                          ← 来自 0x810216BE 调用点
- 对象**步进 0x38**（0xB068B044 → 0xB068B07C → 0xB068B0B4 …）
- ⇒ 这是 `appFontUnitList` 的**字体单元条目数组**，游戏**自己建了**它们，且计数初始合法。
- **但 `[ASSERT-MSG] …:381` 仍然出现**，且 `stop=3(BADMEM) @0x8115F9D6` 未变
- （`totalInsn=590720000` 与修复前逐字相同）。⇒
- **结论：`appFontUnitList.c:381` 的断言读的既不是空指针，也不是我方哑对象。**
- ARCH_394 把"空指针 ⇒ 断言"当成因果链，**这一步被实测证伪**（对象非空、字段合法）。
- `0x810233C6` 的真实语义（反汇编）：
- 0x810233E4 ldrsh.w r1,[r4]     ; +0x00 int16
- 0x810233E8 cmp  r1,#0
- 0x810233EA bgt  #0x81023408    ; >0 通过
- 0x81023408 movs r0,#2 / strh r0,[r4,#2] / subs r1,#1 / strh r1,[r4]
- 它是个**递减器**（`+0x00` 减 1，`+0x02` 置 2）。`+0x00` 归零时断言。
- 用"断言分流"钩子（挂在 assert 实现 `0x8115F832` 入口，按 `lr` 过滤）拿到**决定性一行**：
- [ASSERT-CALL] lr=0x81023404 r1=0x17D(line=381) r4=0xB2CED000
- `r4 = 0xB2CED000` = **我方哑对象**。也就是说：走断言分支时 `r0` 确实是我方哑对象，
- 而反汇编显示 `0x810233C8 adds r4,r0,#0` ⇒ `r4==r0`。
- ⇒ 要么 `+0x00` 被别处改写，要么我读到的 `+0x00` 与 guest 视角不是同一处内存。
- **这两条都还没有证据**，是下一步要钉的。

---

## `ARCH_396_FONT_CMAP_CORRECTED.md`

**标题**：ARCH_396 — FNT0 cmap 算法纠正,以及字体单元 release 断言的真正来源 ｜ 1. 结论先行 ｜ 2. cmap 查找的真实算法(逐条来自反汇编) ｜ 3. 复现证据 ｜ 4. 字体子系统的模块边界(新增,来自 `__FILE__` 字符串) ｜ 5. 真实首阻断链(基于 `_probe_chsrc.txt`, totalInsn=590,960,000) ｜ 资源加载侧观察 ｜ 6. 必须撤销的伪解 ｜ 7. 下一步(精确) ｜ 8. 复现命令 ｜ cmap 算法验证(无需 probe) ｜ assert 站点与模块名 ｜ 删除伪解后重建并长跑（干净基线） ｜ 9. 首阻断的精确机制(删除伪解后的干净基线) ｜ 9.1 断言点 ｜ 9.2 `0x81021672` 为何返回 0 ｜ 9.3 空闲链表为何为空 —— 池分配器只有一个调用点,且被 `type` 门住 ｜ 9.4 下一步(替换 §7) ｜ 10. 真正的根因：解释器 `UXTB/UXTH/SXTB/SXTH` 的 imm2 位域取错 ｜ 10.1 证据链（每一步都是实测，可复现） ｜ 10.2 bug 本体 ｜ 10.3 修复 ｜ 10.4 修复后的验证（`eboot_probe_a396h.exe`） ｜ 10.5 必须一并撤销的错误注释

**发现/结论**（130 条，最多列 40）：

- > 本文纠正 ARCH_394/395 的一个**根本性错误结论**。上一轮据"MISS"判定"字体数据缺字/损坏",
- | 上一轮的说法 | 本轮结论 |
- | `0x81020D06` 用 `low_byte(code)` 索引 cmap,11 个真实码位全部 MISS | **错**。索引用的是 **high byte**,低字节只在页内查区间;11/11 全部 **HIT** |
- | 哑字体对象补丁 + `P5D_FONT_STUB` 是"临时解锁" | **都是伪解**。哑对象把断言从 `appFont.c:363` 推后到 `appFontUnitList.c:381` |
- 反汇编见 `C:\dev\p5d_build\_dis\fb.txt`(`0x81020D06..0x81020E2A`)。
- 该函数位于 **`appFontLetterOrder.c`**(文件名字符串 `0x812EEE64`)。
- r4   = (code >> 8) & 0xFF                     ; 0x81020D1C uxtb.w r4,r6,ror #8
- if (r4 == 0) page = 0                         ; 0x81020D22  ASCII 直通
- assert(page <= 0x3D)                      ; 0x81020DA6
- entry = u32[ base + page*4 ]                  ; 0x81020DC2  <== base 是 u32 页表!
- gbase = u16[ st + 0xF4 ]                      ; 0x81020DD4
- low   = code & 0xFF                           ; 0x81020DDA uxtb.w lr,r6
- 否则 -1(resp. 断言)
- 11 个真实码位(全部来自 probe 实测,见 §5)在两个文件上都命中:
- `0xE040` 页 32 是空页 ⇒ 正确 MISS。
- **⇒ FNT0 的"unknown table"`[24, T)` 语义至此完全解决:它就是"首字节 → 页"的 u32 页表
- | `0x812EED34` | `E:\P3DP5D\xrd757\program\app\app_system\utility\appFont\appFont.c` |
- | `0x812EEDA0` | `...\appFont\appFontCoding.c` |
- | `0x812EEE00` | `...\appFont\appFontDataObject.c` |
- | `0x812EEE64` | `...\appFont\appFontLetterOrder.c` |
- | `0x812EEEE0` | `...\appFont\appFontSystem.c` |
- | `0x812EF018` | `...\appFont\appFontUnitList.c` |
- 对应关系(已由反汇编确认):
- * `0x8101FBC0` = **解码**(`appFontCoding.c`):`str[pos] < 0x80` → 单字节;
- `0x8101FD1A` / `0x8101FD3A` / `0x8101FD5A` 分别是读 `[obj+8]` / `[obj+0xc]` / `[obj+0x10]`。
- * `0x81020D06` = **字形顺序**(`appFontLetterOrder.c`),即 §2 的页表查找。
- * `0x810233C6` = **字体单元 release**(`appFontUnitList.c`):`int16 obj[+0]` 是引用计数,
- [41223] [FONT-OBJ] #1 ENTRY r0=0xB068A01C r1=0x8ACA ...
- [41223] [FONT-OBJ] #1  [r0+8]=0xB056A018  [r0+0xc]=0xB056A304   <== 页表/fontData+772, 完全正确
- [41221] [FONT-MGR] #1 mgr(r5)=0xB068901C [mgr]=2(type) [mgr+0x08]=0xB068A01C
- [41236] [CMAP-BOTH] base=0xB056A018 code=0x8ACA ...
- [41238] [FONT-UNIT-RET] type1(0x8102490C) -> 0x0
- [41536] [FONT-RET] #1 r0=0x0 +0x00=0 +0x02=0 (空槽) lr=0x8102169C     <== 10 次全空槽
- [41602] [FONT-ALIVE] #386 r0=0xB3396000 [r0]=0 (我方哑对象) lr=0x8101E43A
- [41603] [ASSERT-CALL] lr=0x81023404 r0=0x812EF018(file) r1=0x17D(line=381)
- [41611] [EBOOT-START] stop=3(BADMEM) last_pc=0x8115F9D6 totalInsn=590960000
- 1. `0x8102207C`(`appFont.c`)取字形入口:
- 0x81022086  r4 = [0x8132AD74]      ; 全局字体表
- 0x810220E4  r1 = [r4]              ; 表数组
- 0x810220E8  r6 = r1[fontIndex]

---

## `ARCH_398_PERF_AND_PROGRESS.md`

**标题**：ARCH_398 — 性能坍塌根因(越窗地址的 O(R) 线性扫描)、标题序列跑通、新阻断点定位 ｜ 1. 性能坍塌:根因、取证、修复 ｜ 1.1 现象 ｜ 1.2 根因 ｜ 1.3 取证(先量后改) ｜ 1.4 修复 ｜ 1.5 效果(同一 `P5D_CAP=820000000`) ｜ 2. 推进结果:标题序列完整跑通 ｜ 3. 新阻断点(精确) ｜ 3.1 上游原因：`appResourceList.c:207` ｜ 4. 下一步(精确) ｜ 5. 复现命令 ｜ 重建探针 ｜ 短跑（看 [LINHI] 直方图 + 4 个窗口的 MIPS） ｜ 长推进（跑完标题序列）

**发现/结论**（40 条，最多列 40）：

- ⇒ 这 900 万次慢路径**几乎全部命中**,即它们是**合法地址**,问题不在"找不到",而在"找得太慢"。
- const SceUInt32 PT_BASE = 0x81000000u;
- const SceUInt32 PT_N    = 0x33000u;      // 旧值：覆盖到 0xB4000000
- for (auto& r : g_regions)            // ★ 落窗外 ⇒ 扫【全部】region
- **9,114,238 / 9,116,875 = 99.97% 落在 `0xB4xxxxxx` / `0xB5xxxxxx`** —— 正好紧贴旧上界 `0xB4000000` 之上。
- 旁证:同一轮捕获的宿主崩溃现场里,地址也是 `0xB54F5000 / 0xB4EF2040 / 0xB4EB301C`。
- 量化:9.1e6 × 2.35e4 ≈ **2.1 × 10¹¹ 次比较**,与实测的 130 秒同数量级。
- 窗口上界 `0xB4000000` → **`0xC0000000`**(用户空间上界):
- const SceUInt32 PT_N = 0x3F000u;   // *4KB = 0x3F000000，覆盖到 0xC0000000
- [EBOOT-START] stop=3(BADMEM) last_pc=0x810053E2 totalInsn=1235920000
- [TSEQ-ST #1431] titleSequence state=8   (**>=8 ⇒ 尾部 return 0 = 结束**)
- **⇒ 从 ARCH_394 的 590.84M（字体断言中止）到 1,235,920,000，并且 TITLE 阶段完整走完。**
- [EBOOT-START] stop=3(BADMEM) last_pc=0x810053E2 r0=0x0 sp=0xA016DFE8 curThread=0x0
- 反汇编 `0x810053DC`：
- 0x810053DC  push.w   {r4,r5,r6,r7,r8,sb,sl,lr}
- 0x810053E0  adds     r6, r0, #0        ; r6 = arg0
- 0x810053E2  ldr      r0, [r6, #8]      ; ★ BADMEM —— 因为 r6 == 0
- 0x810053E4  ands     r0, r0, #1
- 0x810053E8  bne      #0x81005402
- 0x810053EA  ...      assert(appGxModel.c:809)
- 0x81005402  ldr      r0, [r6, #4]
- 0x81005404  subs     r0, #1
- 0x81005406  str      r0, [r6, #4]      ; 引用计数递减
- 0x81005408  bne.w    #0x810055b8       ; 归零则销毁
- * 该函数属于 **`appGxModel.c`**（字符串 `0x812E9340`），是**模型对象的引用计数释放**。
- * `r0=0x0` ⇒ **guest 对 NULL 模型指针调了 release**。
- * 调用者之一：`0x810052D2 mov r0, r8` / `0x810052D4 bl 0x810053DC`（`r8` 即那个 NULL）。
- 长跑中该断言出现 5 次：
- [ASSERT-SITE] caller=0x8117BB80 r0=0x81307D60(file) r1=0xCF(line=207)
- r2=0x81307DB4("ASSERT:%s") r3=0xF8   (后又出现 r3=0xF7)
- 0x8117BB52..0x8117BB60   循环：按 stride 0xc 在表里查 key
- 0x8117BB62  movs     r5, #0        ; 未找到 ⇒ r5 = 0
- 0x8117BB68  cbnz     r5, #0x8117bb80
- 0x8117BB6A  ...      assert(appResourceList.c:207)     ; ★ 条目不存在
- 0x8117BB80  ldr      r5, [r5, #4]
- 0x8117BB8A  blx      r5            ; 调用条目的回调
- **⇒ `appResourceList.c:207` = 「资源列表里没有这条目」**(`r3` = `0xF8`/`0xF7` 是条目键/ID)。
- **⇒ 因果链**:某资源没被注册进资源列表 → 断言(不致命,程序继续) → 依赖它的
- 1. **在 `0x8117BB6A` 之前取证**:打印被查找的 key、`r3`、以及资源表的项数/内容
- （`0x8117BB52` 循环入口处的 `r5`/`r4`/`lr` 已是现成的量）。目标是拿到**缺失资源的名字或 ID**。

---

## `ARCH_39_FINAL_ACCEPTANCE.md`

**标题**：ARCH_39 — 最终验收：覆盖矩阵 / 永久丢失项 / 勘误表 / 四项自评 ｜ 1. 交付物清单 ｜ 2. 覆盖矩阵 ｜ 2.1 函数 ｜ 2.2 代码区段（28 段，逐段有条目） ｜ 2.3 导入 —— **与目标文本的 410 不符，实测 385** ｜ 2.4 源文件 —— 334 条路径 → **317 个真实文件** ｜ 2.5 函数指针表 ｜ 2.6 具名序列注入（第 1 项最后补齐） ｜ 3. strip 导致的永久丢失项（显式、不含糊） ｜ 4. 勘误表（本轮五个 ARCH 的更正历史） ｜ 5. 四项验收自评 ｜ (1)【源码级全量】 ｜ (2)【运行原理全量】 ｜ (3)【判定核心逐函数】 ｜ (4)【验收与诚实边界】 ｜ 6. 未完成项（剩余 1 轮可推进） ｜ 6.1 仍然无法闭合的项（不是本轮遗漏，而是客观不可达）

**发现/结论**（51 条，最多列 40）：

- > **编码** UTF-8。地址为**真实 VA**（ARCH_34 的 0xF60 修正）。
- | 项 | 值 | 证据 |
- | 合计（下界） | **12,917** | 最终导出实测：`EXPORTED ok=12917 failed=0` |
- | SEG0 代码末址 | `0x812B2AF8` | ARCH_39 §2.2 精度修正（原写 `0x812B2AF0`） |
- ⇒ **ARCH_25 的 26 段全部有条目；另新增 27/28 两段补上原表的 1.1 MB 空缺。**
- + 起点之前 1 个未映射函数（VA 0x81000000）:      1
- **精度修正**：ARCH_34/35 把 SEG0 代码末址写作 `0x812B2AF0`，但该地址处**还有一个 8 字节函数**
- （`_h1213` 输出末行为 `812b2af0 size=8`）。因此代码区真正末址是 **`0x812B2AF8`**，
- 区段 28 的函数数为 3,621（而非 3,620）。此修正由本轮的数量闭合校验发现。
- 1. 先用宽松正则 `Cri|Sofdec|Atom|Mana|Cpk|Sj` ⇒ 得到 905 个"CRI 相关"函数，
- 最小 VA = `0x8103B422` —— **明显异常**（落在 appGraphics 区段）。
- ⇒ **856 个函数，最小 VA = `0x8122215C`，`0x81220000` 以下为 0 个**（离群点全部消失）。
- 4. `0x81221788`（27 段末函数）与 `0x8122215C`（28 段首函数）之间是 **2,516 字节**的字面量池/数据 `[推断]`。
- 逐条目文件 `import_matrix.tsv`（381 行）。`[字节]` 实测：
- detail rows with NID = 381  (与 num_funcs 完全一致)
- **诚实说明**：目标文本写「410 个导入」，实测为 **385**（381 函数 + 4 变量）。
- ② 或统计了 SELF 的 `SceLibKernel` 直接 syscall 桩。**本文件采用可复现的实测值 385。**
- 原因：这些文件内**没有项目自定义断言**（`ASSERT:%s`/`ABORT:%s`），
- 而归属的唯一线索就是断言里的 `__FILE__`。`appImagePVR.c` / `frFontMemory.c` 等
- **141/141 全部匹配** ⇒ 名字与 hash 配对无误，提取可靠。
- `sequence observer(caller`（截断名）与其它变体重复计数 `[推断]`。**本报告采用实测 141。**
- | 8 | **源文件 ↔ 函数的完整映射** | 归属只能靠断言字符串 | ❌ **308/317 = 97.2%** 是上限，剩余 9 个文件不可归属 |
- | 10 | **判定/评分参数表的字段名** | 同上（`0x813A7ED0` 等） | 🟡 值、维度、索引公式已知（ARCH_36 §2），名字不可得 |
- | 12 | **NID → 函数名的映射** | 需 Sony 官方 NID 数据库 | 🟡 381 个 NID 全部解析为 `unknown_0xNNNN` |
- > （2,551/12,917 ≈ 19.8%），因为只有携带断言的文件才能被定位。
- | 时间 | 文档 | 原结论 | 更正 | 依据 |
- | 本轮 | ARCH_25 §区段 | 26 个区段 | **区间重叠，且漏 `0x811980FE–0x812B2AF0` 约 1.1 MB** | 函数 VA 上限 + CRI 关键字字符串 |
- | 本轮 | ARCH_25 §mns | `mnsProcess.c` = `0x81121536–0x81132548`，48 函数 | **是「断言引用点跨度」+「表项数」**，非文件范围/函数数 | `0x81121536` 在 `FUN_81121200`+0x336 内 |
- | 本轮 | ARCH_35 §4.2 | `0x813A7ED0` 是"参数表，**不是**判定窗口表" | **撤回**：它就是判定窗口表 `float[4][4]` | `musicData_811331E8` 伪代码 |
- | 本轮 | ARCH_35 §3 | `0x813A5C9C` = "mns 谱面**事件**处理表" | **撤回**：是 mns **渲染器**表（坐标 getter + 绘制） | 读成员本体（三色绘制 `0xff0000/0xff00ff00/0xffffff00`） |
- | 本轮 | ARCH_35 §4.3 | `0x813A5EDC` = "`{ptr,count}` 二级分派表" | **撤回**：是 **0x90 字节事件对象模板** | `FUN_812b2270(dst, &DAT_813a5edc, 0x90)` |
- | 前轮 | ARCH_23 | TBH 表基址 = `opcode + 4` | 正确为 **`opcode + 4` = `0x81198188`**（本报告实测印证） | ARCH_34 §5 表项全部命中真实函数入口 |
- | 前轮 | ARCH_25/29 | 歌单表 VA `0x813A4EB0` | **`0x813A86A0`**（SEG1 公式） | ARCH_34 |
- 而未读函数体**。ARCH_36/37 通过读函数本体纠正。凡本报告中标注 `[推断]` 的结论，
- | 对 SEG0+SEG1 全部代码反编译 | ✅ | SEG0 全段；SEG1 确认为纯数据（0 函数）|
- | 注入探针确认地址 / 字符串引用 | ✅ | 14,700 条字符串引用；5,040 个 `__FILE__` 引用点 |
- | 注入 145 具名序列 | ✅ **已完成**（实测 141 个，见 §2.6） | 141 个名字标签 + 135 个函数注释注入 Ghidra |
- | app 状态机（7 态） | ✅ **本轮实证**（名字 + 回调指针 + 切换协议） |
- | 每帧任务/序列调度器 `0x811DB1F0` | ✅ |
- | **逐阶段时序（含帧号与耗时）** | 🟡 **部分**：仅 LOGO 段有实测帧号（fc 2810–2960）；其余因端口卡在 state 1 无运行时数据，耗时为 0 |

---

## `ARCH_3_resource.md`

**标题**：ARCH_3：资源加载与场景序列系统（Persona 5 Dancing in Starlight / PCSH10109） ｜ 1. 模块职责（一句话） ｜ 2. 关键函数地址表 ｜ 2.1 加载请求（req）产生与推进 ｜ 2.2 序列（任务）系统 ｜ 2.3 主菜单六条件判定（本模块的直接卡点） ｜ 3. 关键数据结构 ｜ 3.1 加载请求 req（堆对象，步长 `0x2000`，例 `0xB200202C`、`0xB200402C`、`0xB200602C`…） ｜ 3.2 加载管理器 mgr（堆对象，属"主内存"而非 ELF 段） ｜ 3.3 序列链表 + 节点 node（地址模式 `0xXXXXX01C`） ｜ 3.4 cf / 任务对象（堆 `0xA002A9D0` `0xA002B3E0` `0xA002AB20` `0xA002AB90` …） ｜ 4. 生命周期与调用关系 ｜ 4.1 资源请求链（req：从发起到"可采样"） ｜ 4.2 序列（任务）链 ｜ 4.3 app 侧消费（谁在等这些） ｜ 4.4 场景切换时的管理器轮换（本模块最重要的现象） ｜ 5. HLE 接口现状 ｜ 5.1 拦截机制 ｜ 5.2 文件 IO 三层的现状 ｜ 5.3 已知缺口（按性质分类） ｜ 6. 未解问题清单（按优先级） ｜ P0-1　C5/C6 的真实值到底是多少，卡在哪一步？ ｜ P0-2　换场景后"谁该继续泵新 mgr"？ ｜ P0-3　`'mainmenu load'` 节点（hash 0x7BAF）的创建/销毁是否成对？

**发现/结论**（187 条，最多列 40）：

- > 分析对象：`C:\dev\p5d_build\eboot.bin`（ARM32 Thumb，加载基址 `0x81000000`，imports=410，exports=1）
- > 反汇编工具：`py C:\dev\p5d_build\_p5d_dis.py <VA> <n>`
- > 主要证据来源（≥5 处）：
- > 5. eboot.bin 反汇编原文（本文中所有 `VA 指令原文` 均为实测输出）
- > 「推测名」列凡标注推测者，均为依据探针命名 + 反汇编语义推断，非官方符号。
- | VA | 推测名 | 作用 | 证据 |
- | `0x8117BF02` | 加载请求构造器——推测 | 建 req 并返回其指针；内部 `bl 0x81016296`（取当前管理器）+ `bl 0x81165288`（对资源名求哈希） | 反汇编：`8117BF1C ldr r0,[r5]` / `8117BF20 bl #0x81016296` / `8117BF26 mov r0,sl` / `8117BF28 bl #0x81165288` / `8117BF30 mov sb,r0`；探针注释 `thumb_
- | `0x8117C0EE` | 请求发起入口 A：**从散文件路径** | 调用前 `r1=mode, r2=资源名, r3=归档组名(此处为文件路径)` | 探针 `thumb_interpreter.cpp:1426`；日志 `[LRQREQ#1] fn=0x8117C0EE mode=0xF8 owner=0xB1B1A030 nm='DRESS_FORMAT' grp='data/dressFormat.bin' lr=0x8108EC66`（`grp` 全部是文件
- | `0x8117C1BC` | 请求发起入口 B：**从已加载 ARC 容器** | 同上 | 日志 `[LRQREQ#8] fn=0x8117C1BC mode=0xF5 owner=0xB1B1A030 nm='P5SPR_BUTTON' grp='P5MSGARC' lr=0x81031A7E`（`grp` 全部是 ARC 名：`P5MSGARC`、`P5MSGARC_BACKLOG`、`COMMON_ARC_P5`、`TITLE_BIN`）——两条入口的 grp 语
- | `0x8117C118` / `0x8117C1DE` | 两个入口的**构造返回点**（此时 `r0`=req） | 用于把 req 指针与请求名配对 | 探针 `thumb_interpreter.cpp:1431-1434`；日志 `[LRQPTR#1] req=0xB200202C f4=0x0 nm='DRESS_FORMAT'` |
- | `0x8117BA30` | **加载 worker**（每帧推进一步） | `worker(ctx=r0, req=r1)`；`[req+4] & 1` 已置位 → `movs r0,#1` 返回"已完成" | 反汇编：`8117BA44 adds r7,r1,#0` / `8117BA46 ldr r2,[r7,#4]` / `8117BA4A ands r0,r2,#1` / `8117BA4E beq #0x8117ba54` / `8117BA50 movs r
- | `0x81174D64` | **ARC 目录/条目查找**——推测 | `(arc, idx, &size)`；索引越界直接返回 0 | 反汇编 `8117BAAC ldr r4,[r5,#0x3c]` / `8117BAAE ldr r1,[r7,#0x48]` / `8117BAB6 bl #0x81174d64`；探针注释 `thumb_interpreter.cpp:1446-1450`；日志 `[ARCFIND#1] arc=0xB23A9000 idx=0x
- | `0x8117BB80`→`0x8117BB8A` | **按资源类型的处理器调用点** | `r5 = 处理器地址; blx r5(ctx=r4, arg1=r6, arg2=r7)` | 反汇编：`8117BB80 ldr r5,[r5,#4]` / `8117BB82 cbz r5,#0x8117bb94` / `8117BB84 adds r0,r4,#0` / `8117BB8A blx r5` / `8117BB8C cmp r0,#0` / `8117BB8
- | `0x8117BB8C` | **处理器返回值判定** | 返回 0 ⇒ 本帧失败、请求不完成（每帧重试） | 同上反汇编 + 探针注释 `thumb_interpreter.cpp:1463`；日志 `[LHANDLER #2] handler=0x8117C8A1 req=0xB200202C ctx=0xB1B1A030 nm='DRESS_FORMAT'` |
- | `0x8117C960`…`0x8117C990` | **游戏自己的 worker 调用点 + 完成位置位** | `bl 0x8117BA30` 后，**由调用方**置 `[req+4] bit0` | 反汇编：`8117C982 bl #0x8117ba30` / `8117C986 cmp r0,#0` / `8117C988 beq #0x8117c9bc` / `8117C98A ldr r0,[r7,#4]` / `8117C98C orrs r0,r0,#
- | `0x8117C3B6` | 加载管理器**忙查询** | `return ([mgr+4] & 1) ? 0 : 1`（0=忙） | 反汇编：`8117C3B6 ldr r0,[r0,#4]` / `8117C3B8 ands r0,r0,#1` / `8117C3BE movs r0,#0` / `8117C3C2 movs r0,#1` / `8117C3C4 bx lr`；探针 `thumb_interpreter.cpp:6475` 打印 `[LRQDUMP q
- | `0x8117C486` | 完成后的**摘除路径** | 注释：`bics r0,r0,#1` 清 `[req+4] bit0` | 代码注释 `thumb_interpreter.cpp:1417`；探针 `thumb_interpreter.cpp:1442`；日志 `[MMCLEAR#1] done req=0xB1B1B02C nm='' lr=0x8109975E` |
- | `0x8117C7E7` | 资源处理器：**spr6 精灵**——推测 | `mode=0xF5` 请求的 handler | 日志 `[LHANDLER #10] handler=0x8117C7E7 req=0xB201602C ctx=0xB1B1A030 nm='P5SPR_BUTTON'` |
- | `0x8117C8A1` | 资源处理器：**普通文件 / .bin**——推测 | `mode=0xF8` 请求的 handler | 日志 `[LHANDLER #2] handler=0x8117C8A1 … nm='DRESS_FORMAT'`（全部 `0xF8` 请求都用它） |
- | `0x8117C93B` | 资源处理器：**管理器自身内部条目**——推测 | 出现在 `req=0xB1B1B02C ctx=0xB1B1B030 nm=''`（无名内部请求） | 日志 `[LHANDLER #1] handler=0x8117C93B req=0xB1B1B02C ctx=0xB1B1B030 nm=''` |
- | `0x8117CA5F` | 资源处理器：**多边形**——推测 | `mode=0xFC` | 日志 `[LHANDLER #15] handler=0x8117CA5F req=0xB201C02C … nm='P5POLYGON_BACKLOG'`（该请求 `mode=0xFC`） |
- | VA | 推测名 | 作用 | 证据 |
- | `0x8116F900` | **按名查找序列节点** | 先 `bl 0x81165288` 求名字哈希；遍历链表 `[0x815554C0+4]` 比对 `[node+0x18]`；命中后再按名核对；返回 **node** | 反汇编：`8116F902 movw r1,#0x54c0` / `8116F906 movt r1,#0x8155` / `8116F90A ldr r4,[r1,#4]` / `8116F90E bl #0x81165288` / `811
- | `0x81165288` | **名字哈希** | `hash = Σ (int8)name[i] << (i & 7)`，i 从 0 起 | 反汇编：`8116528C adds r5,r0,#0` / `8116528E movs r7,#0` / `811652A2 ldrsb r0,[r4],#1` / `811652A6 ands r1,r6,#7` / `811652AA lsls r0,r1` / `811652AC adds r7,r7,r0` / `81
- | `0x8116FA58` | 取对象**名字指针** | `ldr r0,[r0,#0x14]; bx lr` | 反汇编：`8116FA58 ldr r0,[r0,#0x14]` / `8116FA5A bx lr` |
- | `0x8116F8A8` | 取**序列上下文对象** `[node+8]` | `ldr r0,[r0,#8]; bx lr` —— 只有 2 条指令的独立小函数 | 反汇编：`8116F8A8 ldr r0,[r0,#8]` / `8116F8AA bx lr`（注意它紧跟在 `8116F8A2 blx 0x812b2810` / `8116F8A6 nop` 之后，是真正的新函数入口，**不是** `0x8116F8AC` 的一部分） |
- | `0x8116F8AC` | **按 node 推进序列** | 在链表中按 node 匹配 → 取 `[node+0xC]`（cf）→ `bl 0x8116FA30` → `0x811DAD8C` | 反汇编：`8116F8BC cmp r1,r0` / `8116F8C0 ldr r0,[r0,#0xc]` / `8116F8CC bl #0x8116fa30`；`8116FA30 push {r4,lr}` / `8116FA32 bl #0x811dad8c`；代
- | `0x8116F75E` | **任务摘链（从链表移除）** | 双向链 `next/prev` 摘除，随后 `blx w[+4]` | 代码注释 `thumb_interpreter.cpp:6235-6242`；日志 `[REMTASK] #1 node=0xB1B2401C hash=0x6E2D name='' lr=0x8116FA6E`（摘除者统一是 `0x8116FA6E` = `0x8116FA66` 函数体内的返回点） |
- | `0x8116F7CA` | **任务创建** | `(name=r1, size=r2, update=r3, draw=[sp])` | 代码注释 `thumb_interpreter.cpp:6261-6263`；日志 `[TASKCREATE #43] r0=0x0 r1='title sequence' r2=0xC8 r3=0x0 lr=0x811980F8` |
- | `0x811980FC` | `titleSequence` **取对象**函数 | `return 0x8116F900("title sequence")` | 反汇编：`811980FC push {r4,lr}` / `811980FE movw r0,#0xb140` / `81198102 movt r0,#0x8130` / `81198106 movs r1,#0` / `81198108 bl #0x8116f900` / `8119810C pop {
- | `0x81198172` / `0x81198184` | `titleSequence` **状态机** | `*obj` 经 `tbh` 跳转；state ≥ 8 落入 `0x811982A4`（`movs r0,#0; pop`）= 序列结束 | 代码注释 `thumb_interpreter.cpp:915-920`；`_h1164` 注入点 `0x81198184` 实测生效：`[TSEQ-SKIP] #1 state 7 -> 8`，随后 `[TSEQ-ST 
- | `0x811968B0`–`0x811968EE` | title 场景发起 `TITLE/CLOTH` 请求 | 发起时 `r0 = 该请求所属的管理器上下文对象`（`r7[0xC]`） | 代码注释 `thumb_interpreter.cpp:379-381`；日志 `[LRQREQ#33] fn=0x8117C0EE mode=0xF8 owner=0xB4E76030 nm='TITLE_BIN' grp='title/c_title_p5.bin' lr=0x
- 主菜单 state1 在 `0x81184B68` 起顺序判定 6 个条件，任一不满足即 `bne 0x81184BA6`（跳出）：
- 81184B68  bl        #0x810e865c          ; C1
- 81184B6E  bne       #0x81184ba6          ; C1 != 0 → 跳出
- 81184B70  bl        #0x810f0e68          ; C2
- 81184B76  beq       #0x81184ba6          ; C2 == 0 → 跳出
- 81184B78  bl        #0x8111952e          ; C3
- 81184B7E  beq       #0x81184ba6          ; C3 == 0 → 跳出
- 81184B80  bl        #0x810f6992

---

## `ARCH_400_BADMEM_FIRSTHAND.md`

**标题**：ARCH_400 — 第一手 BADMEM 取证(`mr_fail`)、致命越界写的精确定位、以及三次方法失败的教训 ｜ 1. 教训先行:三轮"按推测挂钩子"全部落空 ｜ 1.1 落地:`thumb_interpreter.cpp` 的 `mr_fail` 增加 stdout 取证 ｜ 2. 实测结果(`eboot_probe_a400d.exe`,`P5D_CAP=1300000000`) ｜ 2.1 #1..#3:**断言机制自身**在解引用 NULL(非致命) ｜ 2.2 #4:致命的越界写(`0x81024B5A`) ｜ 3. 下一步(精确) ｜ 4. 工具使用规约(本轮新增,避免重犯) ｜ 5. 复现命令

**发现/结论**（40 条，最多列 40）：

- | ARCH_400 | 顺着 `0x81005204` → `0x8100721C` → `0x81007306` 挂 `0x81007324/328` | **一次没触发** |
- | ARCH_400b | 挂析构循环 `0x81005532`,只在 `[node+0x20]==0` 时打印 | **一次没触发** |
- | ARCH_400c | 直接在 `release` 入口 `0x810053DC` 拦 `r0==0` | **一次没触发** |
- 三次的共同错误:**先按反汇编"推测"一条调用链,再在链上挂 PC 钩子**。
- `0x810053DC` 实际有 **11 个调用点**,而我从一个调用点往回追,自然追错;
- [MRFAIL] #1 addr=0x00000000 pc=0x8115F9D6 lr=0x8115F9D1 r5=0xCF  r3=0x81307D60
- [MRFAIL] #4 addr=0xA3D79400 pc=0x81024B5A lr=0x00FFFFFF      ← 致命
- [EBOOT-START] stop=3(BADMEM) last_pc=0x81024B5A totalInsn=1043320000
- * `pc=0x8115F9D6`,`addr=0x0`,`r5=0xCF`(=207),`r3=0x81307D60`(= `appResourceList.c` 路径串)
- * 栈上有 `0x45535341`/`0x253A5452` = **"ASSE" / "RT:%"**,即正在拼 `ASSERT:%s`
- * **`0x8115F9D6` 正是 ARCH_394 基线(590,720,000)停下来的那个地址** —— 当时判成"字体断言中止",
- 实际是**断言打印路径自己踩空**。
- `mr_fail` 被调用过之后程序仍继续 ⇒ 用 `stop=` 单点判断阻断会误导。
- 0x81024B3A  add.w fp, r2, r1            ; fp = 目的地索引
- 0x81024B48  add.w r2, sl, fp, lsl #2    ; r2 = sl + fp*4      ← sl = 位图缓冲基址
- 0x81024B4C  adds  r3, r3, r0            ; r3 = 源字节指针
- 0x81024B50  ldrb  r0, [r3], #1
- 0x81024B54  mov   r4, lr
- 0x81024B56  bfi   r4, r0, #0x18, #8     ; 把源字节塞进 32 位字的高 8 位
- 0x81024B5A  str   r4, [r2], #4          ; ★ BADMEM
- | `r10`(=sl) | `0xA3579480` | 位图缓冲基址 |
- | `r11`≈fp | `0x1FFFE0` | 元素索引 |
- | `r2` | `0xA3D79404` | 实际写入地址(已后增) |
- | `r3` | `0xB068A03B` | 源字节指针 |
- | `r4` | `0xFFFFFF` | —— |
- | `[sp+0x24]` | `0xE01C811F` | **cmap 区间表的值** ⇒ 确认是**字形位图展开** |
- **验算**:`0xA3579480 + 0x1FFFE0*4 = 0xA3579480 + 0x7FFF80 = 0xA3D79400` ✓ **精确吻合**。
- [REGDUMP] fault=0xA3D79400 region_count=30063 pt_builds=1
- **⇒ 基址 `0xA3579480` 与目标 `0xA3D79400` 都在 `±0x80000` 之外无区域**
- （两点相距 `0x7FFF80 < 0x80000`，所以两点**都未注册**）。
- **⇒ 两次同时成立:(a) 字形位图缓冲所在的那块内存**从未注册进 `p5d::mem`**;
- (b) 写入索引 `fp≈0x1FFFE0` 本身也是荒谬的(远超任何字形尺寸)。**
- 1. **先回答 (a)**:`0xA3579480` 这块内存是谁分配的?
- * 在 `p5d::mem::register_region` 里记录**所有落在 `0xA3000000..0xA4000000` 的注册**,
- 看是否**根本没有**覆盖 `0xA3579480` 的项;
- * 若没有 ⇒ 某个 HLE 分配器返回了地址却没注册 region ⇒ 这才是"越界写"的真因
- * 这条思路与 §2.1 呼应:`0x8115F9D6` 的 `addr=0x0` 也说明**映射缺口**是这一阶段的共同特征。
- 2. **再回答 (b)**:`fp` 由 `0x81024B3A add.w fp, r2, r1` 得到,`r1=0xFFFF`。
- 追 `r1`/`r2` 的来源(即字形宽度/行距的来源对象字段),确认是否因为某个
- > 不要先按反汇编推测调用链再挂 PC 钩子。**

---

## `ARCH_401_GLYPH_METRIC_CHAIN.md`

**标题**：ARCH_401 — `mr_fail` 一手证据的后续:字形度量链、两条非确定性故障、以及一处**未解决的自洽性问题** ｜ 1. 本轮的一手 BADMEM（`eboot_probe_a401b.exe`, `P5D_CAP=1300000000`） ｜ 2. `0x81024B5A`（8 MB 越界写）的完整上游链（已确证部分） ｜ 3. `[GLYPHF]` 实测的度量字段（`0x81023256` 处读回） ｜ 4. ★ 未解决的自洽性问题（**不得跳过**） ｜ 5. 下一步（精确、可执行） ｜ 6. 工具使用规约（延续 ARCH_400 §4） ｜ 7. 复现命令

**发现/结论**（58 条，最多列 40）：

- 继 `ARCH_400`(改用 `mr_fail` 取证)。本轮把 `0x81024B5A` 那条链追到了底,
- 但**留下一个明确的不自洽点,不做猜测性结论**。
- [MRFAIL] #1..#4 addr=0x00000000 pc=0x8115F9D6 lr=0x8115F9D1 r5=0xCF  ← 断言机制自身踩空（非致命，与 ARCH_400 一致）
- [MRFAIL] #5     addr=0xB5C3A044 pc=0x810053E2 lr=0x810F0D53
- r0=0xB5C3A03C r6=0xB5C3A03C
- [EBOOT-START] stop=3(BADMEM) last_pc=0x810053E2 totalInsn=1229680000
- **#5 的性质与 ARCH_398 的 `0x810053E2` 不同**：
- | **本轮** | **`0xB5C3A03C`** | **`0xB5C3A044`** | **悬垂指针（该地址已不在任何 region 内）** |
- 调用点 `lr=0x810F0D53` —— 是 `0x810053DC` 的 **11 个调用点之一**，与 ARCH_398 追的
- `0x810052D4` 不是同一个（这也是上一轮"按推测挂钩子"失败的原因）。
- **⇒ 同一函数、同一 `addr` 形态（`r0+8`）下，至少存在 NULL 与悬垂两种成因 ⇒ 必须按成因分开处理。**
- 0x81021672 (字体单元获取)
- └─ 0x810216DE  bl 0x81023200        ; (arg0=[fontObj+4], arg1=node, arg2=code, arg3=r8)
- 0x8102322C  ldr   r0, [r6, #4]  ; node[+4] 必须为 0（否则 assert 行 341，实测未触发）
- 0x81023248  str   r5, [r6, #4]  ; node[+4] = arg2 (= 字符码)
- 0x8102324A  adds.w r2, r6, #0x14
- 0x81023252  bl    0x81023108    ; (arg0=[fontObj+4]=管理器, arg1=node, arg2=&node+0x14, arg3=r4)
- 0x81023164  ldr.w r8, [r6, #4]     ; ★ 按【管理器+4】分派
- 0x8102316E  cmp   r8, #0
- 0x81023180  cmp   r8, #1
- 0x81023184  bne   0x810231B0      ; ★★ 既非 0 也非 1 ⇒ 直接跳到存值
- 0x810231B0  vmov s19,r7 / s18,ip / s17,lr / s16,r3   ; 该路径下这四个未赋值
- 0x810231E8  vstr s19..s16, [node+0x14..0x20]
- 0x81023256  vldr  s0..s3, [node+0x14..0x20]
- 0x81023268  vcvt.u32.f32 s0..s3
- 0x81023294  uxth  r3, r3            ; = 0xFFFF（饱和）
- 0x81023296  bl    0x810249F0        ; (base, w, h, 0xFFFF, ...)
- 0x81024A14  uxth.w sb, fp           ; sb = (u16)arg3 = 0xFFFF
- 0x81024A48  ...                     ; 内层 0xFFFF 次迭代
- 0x81024B5A  str   r4, [r2], #4      ; ★ BADMEM：0xA3579480 + 0x1FFFE0*4 = 0xA3D79400
- 1. `0x81023108` 的分派值取自 **`[管理器+4]`**（`r6 = arg0`，不是 node）。
- 2. `0x810231B0` 这条"直落"路径在成立时，`r7/ip/lr/r3` **尚未被赋值**
- （`0x81023168..0x6C` 只是把入参 `r7` 抄进 `ip/lr/r3`，而 `r7` 是调用方残留）
- ⇒ 会把**垃圾**写进度量字段。`vcvt.f32.u32` 对 `0x80000000` 饱和/转换得
- **`0x4F000000`（= 2³¹，浮点）**，与 `[GLYPHF]` 实测的原始位型**精确吻合**。
- **旁证:** `0x812B0DD0` / `0x812B0E70` / `0x812B0E8E` 曾被我误当"导入桩"，
- **⇒ 这一层不是 HLE 缺口。**
- #1..#9   obj=0xB068B044  +0x14=00000000 +0x18=4F000000 +0x1c=4F000000 +0x20=42000000
- * `+0x20 = 32.0` —— **正确**（FNT0 头部 `0x00200020` 的 cell 尺寸）。
- `[METRIC-DISP]` 实测 **`[管理器+4] = 0x0`**（16 次全部为 0），即走 **kind==0** 分支：

---

## `ARCH_402_VMOV_RT_BITFIELD.md`

**标题**：ARCH_402 — 第二个解释器位域 BUG:`VMOV Sn,Rt` 的 `Rt` 多算了一位(且越界读 `r[16..31]`) ｜ 1. 从"不自洽"到"根因"的三步实验 ｜ 步骤 1（ARCH_402）：在**存值点** `0x810231B0` 同时钉住三件事 ｜ 步骤 2（ARCH_402b）：在四条 `vstr` 之前直接读 `s16..s19` ｜ 步骤 3（ARCH_402c）：把 `vmov` 与 `vcvt` 分开 ｜ 2. bug 本体：`Rt` 位域多算了一位 ｜ 修复 ｜ 3. 修复验证 ｜ 4. 剩余的致命 BADMEM（性质已明确） ｜ 5. 方法论要点（第四轮验证） ｜ 6. 复现命令

**发现/结论**（35 条，最多列 40）：

- * `[METRIC-DISP]` 实测 `[管理器+4] = 0` ⇒ 走 kind==0 分支 ⇒ 度量四项都应 ≤ 65535；
- * `[GLYPHF]` 实测 `node+0x18 = 0x4F000000`（= 2³¹）。
- [METRIC-DISP]  #1 node=0xB068B044 dst=0xB068B058      ← dst = node+0x14
- ⇒ 源寄存器完全正常（0 / 0 / 32 / 32），**矛盾不在分支，也不在对象复用**。
- ⇒ 但 `0x810231E8` 起应写 `(0, 0, 0x42000000, 0x42000000)`，
- 而紧随其后的 `[GLYPHF]`（`0x81023256`，中间只隔 `vmov/vcvt/vstr`）读到
- `(0, 0x4F000000, 0x4F000000, 0x42000000)`。
- [VFP-STORE] r4=0xB068B058 sn16=0x42000000 sn17=0x4F000000 sn18=0x4F000000 sn19=0x00000000
- ⇒ **只有 s17 / s18 错**，且值是 `0x4F000000`（= `vcvt.f32.u32(0x80000000)`）。
- [VFP-A] 仅 vmov 之后: sn16=0x00000020 sn17=0x7FFFFFFF sn18=0x7FFFFFFF sn19=0x00000000
- [VFP-B] vcvt 之后:     sn16=0x42000000 sn17=0x4F000000 sn18=0x4F000000 sn19=0x00000000
- ⇒ **`vmov` 本身没写进 s17/s18**（它们保留旧值 `0x7FFFFFFF`）；
- `vcvt` 是忠实的——把 `0x7FFFFFFF` 转成 `0x4F000000`。
- **⇒ 根因在 `vmov`。**
- | 地址 | 反汇编 | `h` | `h2` | `Ss` | 旧 `Rt` | 正确 `Rt` | 是否暴露 |
- | `0x810231B0` | `vmov s19,r7` | `EE09` | `7A90` | 19 ✓ | r7 | r7 | 否（`h2[15]=0`） |
- | `0x810231B4` | `vmov s18,ip` | `EE09` | `CA10` | 18 ✓ | **r28** ✗ | **r12** | **是** |
- | `0x810231B8` | `vmov s17,lr` | `EE08` | `EA90` | 17 ✓ | **r30** ✗ | **r14** | **是** |
- | `0x810231BC` | `vmov s16,r3` | `EE08` | `3A10` | 16 ✓ | r3 | r3 | 否（`h2[15]=0`） |
- （本处读到的正是 `0x7FFFFFFF` 这类相邻成员的值）。
- **为什么长期不可见**：`h2[15]==0` 时两种写法等价 ⇒ 只有约一半编码暴露。
- 修复前: [VFP-A] sn16=0x00000020 sn17=0x7FFFFFFF sn18=0x7FFFFFFF sn19=0x00000000
- 修复后: [VFP-A] sn16=0x00000020 sn17=0x00000020 sn18=0x00000000 sn19=0x00000000   ← 全部符合期望
- 修复后: [VFP-B] sn16=0x42000000 sn17=0x42000000 sn18=0x00000000 sn19=0x00000000   ← 全部符合期望
- | `0x81024B5A`（字形 8 MB 越界写） | 出现（致命） | **整个 12.6 亿指令运行中【不再出现】** |
- | 剩余致命 BADMEM | `0x810053E2` | `0x810053E2`（仅此一条） |
- **⇒ `ARCH_400/401` 追的整条"字形度量 → `vcvt` 饱和 → `0xFFFF` → 8 MB 越界写"链路被彻底消除。**
- [MRFAIL] #5 addr=0xB5F2B044 pc=0x810053E2 lr=0x810F0D53
- r0=0xB5F2B03C  (r0+8 = 0xB5F2B044 已不在任何 region 内)
- * 调用点 `lr=0x810F0D53` —— `0x810053DC`（release）**11 个调用点之一**。
- * `r7=0xA002B7D0`、`sp+0x0C=0xA002B7D0`。
- **⇒ 指向"我们过早释放了 guest 仍在使用的内存"**（`release_region` 会标记 `name="free"`
- 并放进 `g_free_set`）。这是 HLE 生命周期问题，需按 refcount/释放语义修正。
- > **并且每一步都要求"实测 == 期望"**，不一致就记录下来继续查，而不是挑一个说得通的解释。
- `mr_fail 给出 PC` → `[METRIC-DISP]/[METRIC-STORE] 排除分支假设` →

---

## `ARCH_403_DANGLING_CLASSIFIED.md`

**标题**：ARCH_403 — 致命 BADMEM 自动分流:确证为「我方回收过的悬垂指针」,并排除双重/外来释放 ｜ 1. `mr_fail` 现在会自动判定成因 ｜ 2. 本轮实测(`eboot_probe_a403b.exe`, `P5D_CAP=1300000000`) ｜ 3. 释放来源:guest 自己调 libc `free` ｜ 4. 排除两种最可能的**我方**成因 ｜ 5. 崩溃点的精确位置 ｜ 6. ★ 尚未定论的部分（**不得当成已解决**） ｜ 7. 工具限制（本轮暴露，需修） ｜ 8. 下一步（精确） ｜ 9. 复现命令

**发现/结论**（31 条，最多列 40）：

- ⇒ **「从未注册」与「曾注册后被回收」是可以区分的**。
- * 返回：`0 = NEVER-MAPPED(洞)` / `1 = RELEASED(★悬垂)` / `3 = STILL-MAPPED(异常)`。
- [MRFAIL] #1 addr=0x00000000  pc=0x8115F9D6     成因=NEVER-MAPPED(洞)      ← 断言机制自身踩空，非致命
- [MRFAIL] #5 addr=0xB5DC4044  pc=0x810053E2     成因=RELEASED(★悬垂:我方回收过)
- 命中 region: va=0xB5DC4000 size=0x1000 host=0
- r0=0xB5DC403C  (r0+8 = 故障地址)
- [EBOOT-START] stop=3(BADMEM) last_pc=0x810053E2 totalInsn=1,257,640,000
- **⇒ 致命 BADMEM 的性质是确定的：guest 持有的指针指向一块【我们已回收】的 4 KB 内存。**
- * 释放时的 guest PC = `0x812B2250`，在 `veneer_map.txt` 中查得
- **`lib=BE43BB07 nid=5B9BB802 SceLibc`**，而 `v3k_nids.inc` 中该 NID 的名字就是 **`free`**。
- * 绑定处：`bind_by_nid(0x5B9BB802u,n_free)`，实现为
- * `[MEMREL]` 流水（本轮起带 PC）显示相邻地址 `0xB2E4E000`/`0xB2E4F000` 被反复
- **⇒ 这次释放是：单次、地址合法、注册表里确实有它的记录。**
- **⇒ 不是双重释放，也不是 import 绑定/指针语义导致的"外来释放"。**
- 0x810F0D4C  ldr  r0, [r4, #0x48]      ; r0 = obj->[0x48]
- 0x810F0D4E  bl   0x810053dc           ; ★ release(obj->[0x48])  ← BADMEM
- 0x810F0D52  ldr.w r0, [r4, #0x208]
- ⇒ 这是一条**清理/析构路径**，逐个释放 `r4` 身上的子对象；出问题的是 `[r4+0x48]`。
- `obj->[0x48] = 0xB5DC403C`，而释放的 region 基址是 `0xB5DC4000`（`size=0x1000`）。
- **⇒ 我们的 `malloc` 不会返回 `基址+0x3C` 这种内部偏移。**
- * 分类结果为 `host=0` ⇒ 该块**释放后没有被复用**（否则 `host` 会重新非空）。
- **⇒ 已知事实支持："guest 持有一个指向已释放块的【内部】指针（`base+0x3C`），且该块未被复用"。**
- **但"为什么 guest 会持有它"尚未定论。** 两种仍需实证的可能：
- 而释放整块时仍存在活引用 ⇒ 应当是 guest 侧问题，但要能解释"为什么在真机上不发生"；
- ⇒ 需要用"每次 malloc 返回值"的流水与 `obj->[0x48]` 对账。
- **⇒ 下一步必须先做这个对账**，不允许用"大概是 guest 的 bug"收尾。
- （例如被切到 PC=0 的线程上）。`[MEMREL]` 里多数条目有正常 PC（如 `0x812B2250`），
- 1. **对账**：记录每次 `malloc/calloc/memalign` 的返回值（仅 `>= 0xA0000000` 范围，
- 带上 guest PC），然后检查致命故障的 `r0`（如 `0xB5DC403C`）是否**出现在**任何一次
- * 若**出现** ⇒ 我方把内部/共享块的地址交给了 guest ⇒ 分配器语义缺陷，可修。
- * 若**不出现** ⇒ 它是块内偏移 ⇒ 转向 guest 自有分配器的块内布局。

---

## `ARCH_404_ALLOC_LEDGER.md`

**标题**：ARCH_404 — 分配对账完成:悬垂指针 = guest 自有 arena 的内部偏移;并指出一处**平台语义偏差** ｜ 1. 新增:分配返回值总账 ｜ 2. 对账结果（决定性） ｜ 3. 本轮同时修掉一个**我方仪表 bug** ｜ 4. ★ 新发现的一处**平台语义偏差**（不是猜测，是行为对比） ｜ 为什么这不等于"用 hack 掩盖" ｜ 5. 下一步（精确） ｜ 6. 复现命令

**发现/结论**（17 条，最多列 40）：

- [MRFAIL] #5 addr=0xB56AB044 pc=0x810053E2 lr=0x810F0D53
- 成因=RELEASED(★悬垂:我方回收过)   命中 region: va=0xB56AB000 size=0x1000 host=0
- ★ 该 region 是被 release_region 回收的；释放时的 guest PC=0x812B2250
- ★ r0=0xB56AB03C 不是分配返回值，但落在分配 0xB56AB000 (pc=0x812B22A0) 的 0x1000 内
- ⇒ 内部偏移 +0x3C
- | PC | NID | 函数 |
- | `0x812B22A0` | `775A0CB2` | **SceLibc `malloc`** |
- | `0x812B2250` | `5B9BB802` | **SceLibc `free`** |
- ⇒ 我方 `malloc` **从不**返回 `基址+0x3C` 这类内部偏移。
- 2. `0xB56AB03C` **不是**分配返回值，而是那次 4KB 分配的**内部偏移**。
- 3. 同一次观测里 `r4 = 0xB552001C` 也是 `基址+0x1C` 形态
- ⇒ **guest 有一套自有的 arena/slab 分配器**：向我方 `malloc` 拿 4KB 大块，
- 修正后 `释放时的 guest PC` 立刻变为 `0x812B2250`，与预期一致。
- **⇒ 即"`free` 之后该地址立刻不可访问、读取即 BADMEM"。**
- **⇒ 因此这条 BADMEM 有可能是"我方内存模型比真实平台更严格"造成的伪故障。**
- * 修正方向是"让内存层更像被模拟的平台"，属于保真度修正；
- ⇒ 必须**同时**保留一条显式的"访问已释放块"计数器，把"不再报错"变成"可观测"，

---

## `ARCH_405_FREE_ACCESSIBILITY.md`

**标题**：ARCH_405 — `free` 后的可访问性:一次**被实测证伪**的方案(v1)与一次成功方案(v2) ｜ 1. v1（被证伪）：free 后**保持映射**、保留宿主缓冲 ｜ 2. v2（采用）：仍解除映射，但失败路径返回**共享毒页** ｜ 实测（`eboot_probe_a405b.exe`） ｜ 3. 本轮停在另一个 BADMEM（性质不同） ｜ 4. 诚实结论 ｜ 5. 下一步（精确） ｜ 6. 复现命令

**发现/结论**（27 条，最多列 40）：

- > ⇒ 这条 BADMEM 可能是"我方内存模型比真实平台更严格"造成的伪故障。
- 实测（`eboot_probe_a405.exe`，`P5D_CAP=3000000000`）：
- **⇒ MIPS 崩到 1/10，`linear` 涨了 4 个数量级。**
- **原因**：跳过 `pt_unmap_region` 后**地址空间无法回收** ⇒ `allocate_vaddr` 的
- 暴增 ⇒ `translate` 大量走 O(R) 线性扫描 —— 与 `ARCH_398` 修掉的那条慢路径同源。
- **⇒ v1 不可用，已回退。** 这条数据本身有价值：它说明"保持映射"与
- ++g_uaf_acc;                                  // ★ 可观测：不是静默放过
- * 地址空间照旧可回收（`pt_unmap_region` 保留）⇒ 性能不受影响；
- **⇒ 性能与修改前一致（`ARCH_398` 之后的正常水平），没有引入新的慢路径。**
- * **`0x810053E2`（悬垂 release）在整个 12.5 亿指令运行中【不再出现】**；
- 长跑（`P5D_CAP=3000000000`，`_probe_a405b_long.txt`）确认：
- [MRFAIL] #1..#4 addr=0x00000000 pc=0x8115F9D6   ← 断言机制自身 NULL 解引用（出现 4 次，不终止）
- [MRFAIL] #5     addr=0x00000000 pc=0x8115F9D6   ← 最后一次终止了运行
- [EBOOT-START] stop=3(BADMEM) last_pc=0x8115F9D6 totalInsn=1,104,400,000
- **⇒ `0x810053E2`（悬垂 release）在整个运行中【一次都没有出现】—— 这是本轮修复的直接证据。**
- **⇒ 但运行深度没有增加（1.1044B），因为另一个故障接替了它。**
- * 这是**断言机制自身**在解引用 NULL（栈上有 `ASSE`/`RT:%`，即正在拼 `ASSERT:%s`）。
- ⇒ **又是非确定性**（同一二进制多次运行停止点不同，已在 `ARCH_399` 证实）。
- * **⇒ 单次运行的 `totalInsn` 不能用来判定"某个修复有效"**；判定必须靠
- "某类故障是否还在出现"（本轮 `0x810053E2` 消失）与多次运行的分布。
- 因为**另一个非确定性故障**（断言机制的 NULL 解引用）接替了它。
- * 用 `ARCH_399` 已经证实的方法（同二进制多跑几次）统计停止点的分布；
- * 重点看 `0x8115F9D6`（断言机制 NULL 解引用）为何**有时致命、有时不致命**
- —— 这直接关系到"断言策略"是否应该改成不终止运行。
- 2. **`0x8115F9D6` 本身**：断言函数在格式化 `ASSERT:%s` 时对 NULL 解引用。
- 栈上有 `0x45535341`("ASSE") / `0x253A5452`("RT:%") ⇒ 说明 `%s` 的实参是 NULL。
- 需要确认：是 guest 传了 NULL，还是**断言的实参收集方式**在我方有偏差。

---

## `ARCH_406_ABORT_AND_HANDLER_TABLE.md`

**标题**：ARCH_406 — `0x8115F9D6` 不是 bug,而是 guest 的**故意 abort**;handler 表的完整键集已确定 ｜ 1. `0x8115F9D6` = 标准 `abort()` 的空指针写 ｜ 2. handler 表的完整键集（静态模板已定位） ｜ 静态回溯到此为止 ｜ 3. 对判据的更新（写入 HANDOFF） ｜ 4. 下一步（精确） ｜ 5. 复现命令 ｜ 反汇编 abort 路径 ｜ 定位 13 项模板（见 §2 的脚本思路：按 12 字节表项模式搜索）

**发现/结论**（41 条，最多列 40）：

- 回答 `ARCH_405 §5.2`："`0x8115F9D6` 为何有时致命、有时不致命"。
- 反汇编 `0x8115F9CA` 起：
- 0x8115F9CA  push     {r4, lr}
- 0x8115F9CC  bl       #0x8115f7e0        ; 询问"是否已安装 abort handler"
- 0x8115F9D0  cmp      r0, #0
- 0x8115F9D2  beq      #0x8115f9d8        ; r0==0（无 handler）才走下面
- 0x8115F9D4  movs     r0, #0
- 0x8115F9D6  str      r0, [r0]           ; ★ 故意写地址 0 —— 强制产生访问违例（= abort）
- 0x8115F9D8  pop      {r4, pc}
- **⇒ 这是刻意构造的"强制异常"路径，不是解引用错误。**
- **⇒ `stop=3(BADMEM) @0x8115F9D6` 的含义是：guest 自己决定中止程序。**
- 这条结论同时解释了三件事：
- 2. 为什么 `addr` 恒为 `0x00000000` 且 `r0` 恒为 0；
- 3. 为什么栈上能看到 `0x45535341`("ASSE") / `0x253A5452`("RT:%") —— 它紧接着断言格式化走下来。
- **⇒ 因此它不需要"修"；需要修的是它上游的那条断言。**
- 而唯一的断言（`ARCH_399b` 全范围普查：整轮只有 1–5 次）是
- **`appResourceList.c:207`**（资源/handler 表查找失败，`caller=0x8117BB80`）。
- 用 `[RESLIST]` 实测到的**表项字节模式**（`key | fn | flag`，stride 0xC）在二进制里搜索，
- 在 **SEG1** 命中 ≥10 份完全相同的 13 项模板（如 `0x8132F4C4`、`0x813305C4`、`0x81330A94`、
- `0x81331620`、`0x81331B1C`、`0x81335068`、`0x8133624C`、`0x81336398`、`0x81384D10`、`0x813865F8`）：
- | 0 | `0xF2` | `0x8117C6C3` | 0 |
- | 1 | `0xF5` | `0x8117C7E7` | 0 |
- | 2 | `0xF0` | `0x8117C59F` | 0 |
- | 3 | `0xF1` | `0x8117C625` | 0 |
- | 4 | `0xF4` | `0x8117C78F` | 1 |
- | 5 | `0xF6` | `0x8117C85D` | 1 |
- | 6 | `0xF7` | `0x8117C8A1` | 1 |
- | 7 | `0xF8` | `0x8117C8A1` | 1 |
- | 8 | `0xF3` | `0x8117C725` | 0 |
- | 9 | `0xF9` | `0x8117C8E5` | 0 |
- | 10 | `0xFA` | `0x8117C93B` | 0 |
- | 11 | `0xFB` | `0x8117CA09` | 0 |
- | 12 | `0xFC` | `0x8117CA5F` | 0 |
- **⇒ 完整键集是 `{0xF0 … 0xFC}` 共 13 个。**
- **⇒ 运行期实例的 `count`（实测 13 / 4 / 1）说明它是**逐项注册**的，某些实例只填了一部分。**
- * ⇒ 它们是通过**基址 + 偏移**访问的（例如某个 SEG1 结构体数组的成员），
- **⇒ 改为运行期取证**：在 `0x8117BB4C`（查找入口）已知表指针 `r6` 的前提下，
- * 之前把 `0x8115F9D6` 当作"又一个 BADMEM 故障"是**分类错误**：它是 abort 的实现方式。
- * **`成因=NEVER-MAPPED(洞)` + `pc=0x8115F9D6` ⇒ guest 主动 abort**（不是内存模型问题）；
- * **`成因=RELEASED(悬垂)` ⇒ 我方生命周期/内存模型问题**（`ARCH_405` 已修）。

---

## `ARCH_408_INTERPRETER_AUDIT.md`

**标题**：ARCH_408 — 解释器审计:位域解码核查 + 一个**可达性 BUG**(处理器被兜底吞掉) ｜ 1. 审计口径 ｜ A 中两处"看起来可疑"的，经核查不是 bug ｜ 2. ★ 找到的 BUG：处理器被通用兜底**吞掉**（不可达） ｜ 验证 ｜ 3. 对风险表述的修正（写回 HANDOFF） ｜ 4. 一个可复用的经验（值得写进工具规约） ｜ 5. 下一步 ｜ 6. 复现命令

**发现/结论**（13 条，最多列 40）：

- | 检查 | 方法 | 结论 |
- 对 `vshr.u64`（`h2=0x30B2`）：`(h2>>8)&0xF == 0 != 1` ⇒ **大块的守卫为真** ⇒ 进入大块 ⇒
- 内部无匹配 ⇒ 落到兜底 `vmiss + return true` ⇒ **第 9550 行永远到不了**。
- **⇒ 即 `h145k68` 的"修复"是【静态标定】的**（用工具枚举了 13 条 `vshr` 编码并核对字段公式），
- **⇒ 修法（只改判定顺序，语义不变）**：把 VSHR 族整段**上移到兜底之前**。
- 源码中已留下 `ARCH_408 更正` 注释与运行期证据。
- **⇒ 至此，整轮运行里【没有任何未实现指令】。**
- 上一轮我说"没有理由认为只有两个位域 BUG" —— 这个说法**方向对，但量级需要修正**：
- * ✅ 但**解码覆盖度**经实测是好的：12 亿条指令里未实现指令为 **0/1 条**；
- **⇒ 结论：解释器仍有残余风险，但已从"无界"收敛为"有界"**——
- 现在的界点是 guest 的**故意 abort**（`0x8115F9D6`），上游是 `appResourceList.c:207`
- 实例的 count 实测为 1/2/4/5/13 ⇒ 每个实例按自己的需要构造固定子集。
- **⇒ 下一步要回答"谁构造了这个只有 2 项的实例、它该不该有 0xF7"。**

---

## `ARCH_40_CLOSING_REPORT.md`

**标题**：ARCH_40 — 结项报告：达成边界、阻塞链更正、交付索引 ｜ 1. 本轮新增的一处更正（重要） ｜ 「阻塞在 `appSound.c:1003`」这一历史结论**证据不足**，予以更正 ｜ 1.1 更正后可以确证的事实 ｜ 1.2 方法论教训（第二次同源错误） ｜ 2. 四项达成边界（最终） ｜ (1)【源码级全量】— ✅ 达成 ｜ (2)【运行原理全量】— 🟡 **部分达成（唯一未达成项）** ｜ (3)【判定核心逐函数】— ✅ 达成（5/5） ｜ (4)【验收与诚实边界】— ✅ 达成（含两处口径差异） ｜ 3. 交付物总索引 ｜ 3.1 二进制与工程 ｜ 3.2 伪代码（源码级交付） ｜ 3.3 索引与矩阵（全部逐条目） ｜ 3.4 技术文档 ｜ 4. 对「可支撑完整游玩一局」的实际支撑度 ｜ 5. 目标状态判定

**发现/结论**（41 条，最多列 40）：

- > **编码** UTF-8。地址为**真实 VA**（ARCH_34 的 0xF60 修正）。
- 该结论在此前多轮文档中被反复引用，作为"游戏无法进入影片之后阶段"的根因。
- `[ASSERT-ABORT]` 日志记录的是 `r1=0x81306D04 r3=0x813072A0`。
- 0x81306D04 -> "%s\n%s:%d\n"                                   ← 格式串，不是文件路径
- 0x813072A0 -> "E:\P3DP5D\xrd757\...\app_sound\appSound.c"     ← 文件路径
- ⇒ `r3` 才是文件路径，`r1` 是格式串。旧记录把二者混为一谈。
- **问题 2：行号 1003 不在 appSound.c 的断言行号集合中**
- 在 `true_seg0_final.c` 中穷举 `appSound.c` 的**全部 37 个断言调用点**（25 个不同的
- 源码行号，参数形式确认为 `FUN_8115f832(file, line, cond)`，见下例）：
- **问题 3：`lr=0x8115F9D0` 不是断言调用点**
- `0x8115F9D0 = 0x8115F9CC + 4`，即 **`bl FUN_8115f7e0` 的返回地址**，不是断言调用点。
- | 触发了一次断言，其 file 字符串确为 `appSound.c` | `r3=0x813072A0` 直接读取 `[字节]` |
- | 断言 helper 是 `FUN_8115f832`，参数为 `(file, line, cond)` | 37 个调用点的伪代码 |
- - ARCH_35：用"引用者身份"推断 `0x813A7ED0` 的性质，未读函数体 → 错
- ⇒ **凡可由 `[字节]` 直接确证的内容，不得用寄存器快照或引用关系代替。**
- | SEG0+SEG1 全部代码反编译 | ✅ SEG0 全段；SEG1 确认为**纯数据、0 函数** |
- | 注入探针确认地址 / 字符串引用 | ✅ 14,700 条字符串引用；5,040 个 `__FILE__` 引用点（movw/movt 独立解码） |
- | 注入 145 具名序列 | ✅ **实测 141**（hash 公式 141/141 验证通过）；141 标签 + 135 函数注释 |
- | app 状态机（7 态） | ✅ **实证**：7 个状态名（表项内嵌字符串）+ 6 组 enter/update/leave 指针 + 切换协议 |
- | 每帧任务/序列调度器 `0x811DB1F0` | ✅ 三层调度结构 + task 对象 0x50 字节布局 |
- | 145 个具名序列 | ✅ 清单（实测 141 个可提取，8 类归档） |
- - **实测帧号只有 LOGO 段**：fc 2810–2960（ATLUS / P-STUDIO / CRIWARE 三家），
- `sceKernelGetProcessTimeWide` 推导，实测约 6.8× 过速），且 `DISPCALL=0`/`SETFB=0`
- ⇒ **无 vsync**。即使有帧号也算不出可信耗时。
- - **原因**：一次断言（file 字符串确证为 `appSound.c`，见 §1）使 `initDataRead` 无法完成，
- ⇒ 这不是分析能力的缺口，而是**运行时阻塞**：要取得帧号/耗时，**必须先修工程侧**
- （在 `FUN_8115f832` 内加探针确认断言条件 → 修复 → 消除时钟过速 → 恢复 vsync）。
- | `mnsProcess.c` 全部剖析 | ✅ 真实范围 `0x81121588`–`0x81132500`；48 项渲染器表 + 10 个具名函数 |
- | 谱面事件流 `+0x18` 条目格式 | ✅ stride **`0x54`** = 2×ushort 窗口 + 4×20 B 子项 |
- | 判定窗口表 `0x813A7ED0` | ✅ **`float[4][4]`**（4 难度 × 4 等级），由 `musicData_811331E8` 访问 |
- `48 个函数` = `0x813A5C9C` 表的项数；`0x81121536–0x81132548` = 断言引用点跨度；
- | 所有结论附证据 | ✅ 五级标注：`[字节]`/`[算术]`/`[Ghidra]`/`[实测]`/`[推断]` |
- | 覆盖矩阵：410 导入 | 🟡 **实测 385**（30 模块 = 381 函数 + 4 变量），逐条目 `import_matrix.tsv`；差异 25 无法复现，已说明 |
- | `decomp_seg1.c` | SEG1 = 纯数据（377 B，0 函数）的证据 |
- | **ARCH_34** | 三处重大修正（`0xF60` 偏移 / 区段重叠与 1.1 MB 缺口 / mns 范围）+ 逐函数索引 + TBH 解密 |
- | **ARCH_38** | 运行链路：app 状态机 7 态实证 / 三层调度 / 逐阶段时序 |
- | 改输入 | `appPad` 区段（60 函数）+ 全局 `0x816CDFB0/B2/B4` |
- | 改音频 | `appSound.c` 37 个断言点 + CRIWARE 区段（28 段 3,621 函数） |
- | 接判定/谱面 | 判定窗口表、谱面条目 stride `0x54` 字段布局、musicID 映射 |
- 从 state 2「FREE MODE」到结算的全部阶段，文档只有静态结构，没有时序与实测行为。

---

## `ARCH_410_FAILING_LOOKUP_CHAIN.md`

**标题**：ARCH_410 — 失败查找的调用链定位;并纠正一处**回溯读取误判**;确认"忽略断言"这条路已堵死 ｜ 1. 先排除一条已走过的路（省下一轮） ｜ 2. 失败查找的完整现场（ARCH_410 实测） ｜ 3. ★ 纠正一处回溯读取误判（方法学，值得记下） ｜ 4. 触发查找的具体指令 ｜ 5. handler 表的构造者（ARCH_409 数据） ｜ 6. 下一步（精确） ｜ 7. 复现命令

**发现/结论**（43 条，最多列 40）：

- `thumb_interpreter.cpp` 第 3239-3244 行的历史注释（h145k1153）已经记录了实验结论：
- 0x8117BB42 表 miss -> r5=0 -> assert appResourceList.c:207 -> 忽略
- -> 0x8117BB80 ldr r5,[r5,#4] (badmem 0x4) -> req[0x40]=NULL
- -> 0x81181CFA ldr r0,[r3,#0x94] (badmem 0x94) -> stop=3(BADMEM)
- **⇒ "忽略断言看看能走多远"这条路**已经**被走过了，结果只是把崩溃推迟 —— 必须修根因。**
- [RESLIST] #1 table(r6)=0xB5A14030 arr=0xB5A1404C count=4 key=0xF7
- sp=0xA016DFE8
- bt=0x811C2A36 0x811E6966 0x812B4150 0x812B414C 0x81159DE4
- 0x812B4150 0x811994D4 0x811F3494 0x8130680C 0x81007D92
- 0x812B4150 0x81198CDC 0x81206BEC
- 所在块 base=0xB5A14000（我方已登记分配，alloc pc=0x812B22A0 = SceLibc `malloc`）
- [ASSERT-ALL] #1..#3 lr=0x8117BB80 line=207 file=appResourceList.c
- [ASSERT-ALL] #4     lr=0x81005402 line=809 file=appGxModel.c
- [EBOOT-START] stop=3(BADMEM) last_pc=0x8115F9D6 totalInsn=1,243,720,000
- **⇒ 确认两条断言先后触发：**
- 3. 最后 `0x8115F9D6` = **故意 abort**（`ARCH_406`）。
- bt 里频繁出现的 `0x812B4150` / `0x812B414C` **不是返回地址**，而是**栈金丝雀值**：
- 0x811E6976  movw  r7, #0x414c
- 0x811E697A  movt  r7, #0x812b
- 0x811E697E  ldr   r1, [r7]        ; r1 = *(0x812B414C) —— canary
- 取 canary 后压栈。我的"扫栈找 `[0x81000000,0x81330000)` 内的值"启发式把它们当成了代码地址。
- **⇒ 过滤掉 canary 后的真实调用链是：**
- 0x811C2A36 → 0x811E6966 → 0x81159DE4 → 0x811994D4 → 0x811F3494
- → 0x8130680C → 0x81007D92 → 0x81198CDC → 0x81206BEC
- **⇒ 后续读 bt 时应先剔除 `0x812B4150`/`0x812B414C`（其余宽范围扫描也有同类噪声）。**
- 0x811C2A32  bl    #0x811e6894       ; ★ 这次调用（传递地）触发了 key=0xF7 的查找
- 0x811C2A36  ldr.w r1, [r4, #0xb4]   ; ← 栈上的返回地址就是这里
- 0x811C2A3A  cmp   r1, #0
- 0x811C2A3C  beq   #0x811c29fe
- 0x811C2A40  ldr.w r1, [r4, #0xb8]
- 0x811C2A46  blx   r2                ; 调用 r4[+0xb4]（对象上的函数指针）
- ⇒ 该函数还会调用 `0x811E64F4`（×2）与 `0x811CD376`，并在结尾 `blx r4[+0xb4]`；
- | `0xB1B19030` | 5 | `{F7,F9,F8,FA,F5}` | `0x8117BC66 ← 0x810995D2` |
- | `0xB1B1A030` | **13** | 完整模板 | `0x8117BC66 ← 0x8117C93A`（**在 FA 的 handler 内部**） |
- | `0xB24F3030` | 1 | `{F5}` | `0x8117BC66 ← 0x8117C93A`（同上） |
- | `0xB24F9030` | 1 | `{F5}` | `0x8117BC66 ← 0x810E65C8` |
- | `0xB5A6E030` | 4 | `{F8,F2,F5,F0}` | `0x8117BC66 ← 0x81196B76` |
- | `0xB352F030` | 5 | `{F8,F5,F7,…}` | `0x8117BC66 ← 0x810EF402` |
- **⇒ 两点新认识：**
- 1. **存在嵌套派发** —— 从 bt 能看到 handler 自身（`0x8117C93A`/`0x8117C986`）在查找的调用链上；

---

## `ARCH_411_DISPATCH_DECODED.md`

**标题**：ARCH_411 — 派发函数 `0x8117BA30` 完全解读;以及一次**失败的仪表尝试**(记录限制) ｜ 1. 派发函数 `0x8117BA30(dst, job)` 的对象布局（本轮确证） ｜ 2. 失败发生在**拆除/注销**路径上 ｜ 3. ⚠ 一次失败的仪表尝试（把限制记下来，避免重复） ｜ 4. 下一步（精确） ｜ 5. 复现命令

**发现/结论**（27 条，最多列 40）：

- 反汇编 `0x8117BA30..0x8117BB50`：
- | `[job+8]` | ★ **查找键**（实测 `0xF0..0xFC`） |
- 断言（都在 `appResourceList.c`）：
- | **349** | `0x8117BA90` | `[provider+8] != 0xF8` ⇒ 断言（**provider 类型标签必须是 `0xF8`**） |
- | 385 | `0x8117BB1C` | `[job+0x44] == NULL` ⇒ 断言 |
- | **207** | `0x8117BB7C` | **在 `[dst+8]` 的数组里按 `[job+8]` 查不到** ⇒ 断言 |
- `0x8117BAB6 bl 0x81174d64` 用它们做实际派发。
- **⇒ 关键结论：失败的查找是"目标对象 `dst` 的 handler 表里没有 `job` 要求的那个键"。**
- 表的 count 实测有 1 / 4 / 5 / 13 四种，且 `ARCH_407` 已证 **count 建好后从不变动**。
- `0x811E6894`（失败链上 `0x811C2A32` 的直接被调者）是一个**注销循环**：
- 0x811E6894  push  {r4,r5,r6,r7,r8,lr}
- 0x811E6898  movw r6, #0x3e74 / movt r6, #0x8151   ; &count
- 0x811E68A0  r0 = [r6]                              ; count
- 0x811E68AA  movw r5, #0xd680 / movt r5, #0x8169   ; 表基址 0x8169D680
- 0x811E68B2  r1 = [r5]                              ; entry.fn
- 0x811E68B4  r0 = r5 + 4                            ; entry.arg
- 0x811E68B6  blx r1                                 ; ← 调用每个登记项
- 0x811E68BA  r7++; r5 += 0x20; if (r7 < count) loop
- 0x811E68C2  [0x81513E74] = 0                       ; count 清零
- **⇒ 即"把所有登记的服务逐个调函数注销、然后把计数清零"。**
- **⇒ 失败那次查找（`0xF7`）就发生在这条拆除链里。**
- 这与紧接着的 `appGxModel.c:809`（引用计数断言）一致：
- 而实测：13 项表的内容与 SEG1 静态模板**逐字节相同** ⇒ **它是被 `memcpy` 拷进去的**，
- 因此写路径上看不到；4 项表不是模板前缀 ⇒ 也不是"拷前 4 项"。
- **⇒ 结论：要抓建表者，必须走 `memcpy` 这条路径（在 HLE 的 `n_memcpy` 里挂钩），
- 1. **钩派发函数 `0x8117BA30` 入口**，打印 `(dst, job, [job+4], [job+8])`（带上限），
- 即可看到"从哪个静态模板拷了多少字节"⇒ 直接给出建表者与它选择的子集。

---

## `ARCH_412_TABLE_BUILDER_LOCATED.md`

**标题**：ARCH_412 — 建表函数与**失败表的精确建表点**已锁定;并记下我自己的扫描器两处 bug ｜ 1. 建表函数 `0x8117BD50(src, count)` 完全解读 ｜ 2. 三个子模板与其装载点（本轮全部定位） ｜ 建表现场（`0x81196850..0x811968C0`） ｜ 3. ★ 我自己的扫描器有两处 bug（记录，避免复用错工具） ｜ 4. 现在的问题被压到最窄 ｜ 5. 下一步（精确） ｜ 6. 复现命令 ｜ 4 项子模板 + 装载点（修正后的扫描器）

**发现/结论**（43 条，最多列 40）：

- 0x8117BD50  push.w {r4,r5,r6,r7,r8,lr}
- 0x8117BD54  r4 = count
- 0x8117BD56  r8 = (count*3)<<2 = count*12          ; 表项总字节
- 0x8117BD5E  movw r1,#0xcab5 / movt r1,#0x8117     ; 类型标签 0x8117CAB5
- 0x8117BD64  r0 = r8 + 0x1c                        ; 对象大小 = 0x1C + count*12
- 0x8117BD6C  bl 0x8101612c                         ; 按类型标签分配
- 0x8117BD96  [obj+0]  = r7
- 0x8117BD9A  [obj+8]  = obj+0x1c                   ; ★ 表项数组内联在 +0x1C
- 0x8117BD9E  [obj+0xc]= count                      ; ★ count
- 0x8117BDA0  blx 0x812b2270                        ; ★ memcpy(表项数组, src, count*12)
- 0x8117BDA4  [obj+0x18]=0 ; [obj+0x10]=1 ; [obj+0x14]=0
- 0x8117BDB6  r8 = [0x81556AF0]                     ; 全局链表头
- 0x8117BDC0  if (r8) { [r8+0x14]=obj ; [obj+0x18]=r8 }
- 0x8117BDD0  [0x81556AF0] = obj                    ; 挂进全局链表
- 0x8117BDD4  return obj
- **⇒ 表对象布局与 `ARCH_411` 的推断完全一致；内容是**从静态子模板 memcpy 进来的**，
- | 5 项 | `0x81386CF0` | — | `0x810994CA` | 5 |
- | 13 项 | `0x81386D2C` | 完整 `{F0..FC}` | `0x810994EE` | 13 |
- | **4 项** | **`0x813B1AEC`**（全库唯一） | **`{F8,F2,F5,F0}`** | **`0x81196876`** | **4** |
- **⇒ 4 项子模板正是失败表 `0xB5A14030` 的内容 ⇒ `0x81196876` 就是那张表的建表点。**
- 0x8119685E  bl 0x8115d36a          ; 分配 424 字节对象
- 0x8119686A  blx 0x812b2260         ; memset(obj, 0, 424)      （0x812b2260 = SceLibc memset）
- 0x81196870  movw r1,#0x1aec
- 0x81196876  movt r1,#0x813b        ; ★ r1 = 0x813B1AEC
- 0x8119687A  movs r2,#0x30          ; ★ 48 = 4×12
- 0x8119687C  blx 0x812b2270         ; memcpy(栈缓冲, 模板, 48)
- 0x81196882  movs r1,#4
- 0x81196884  bl 0x8117bd50          ; ★ 建表(count=4)
- 0x8119688E  str r6,[r7,#0xc]       ; 挂到 424 字节对象的 +0xc
- 0x811968BC  movs r1,#0xf8          ; ★★ 紧接着用 key = 0xF8
- **⇒ 这个 424 字节对象是"只注册 `{F8,F2,F5,F0}` 四个 handler"的**设计如此**，
- | bug | 现象 | 修正 |
- | ① opcode 掩码含 `i` 位 | `(h1>>4)&0x7F` 把 bit10 并进来 ⇒ MOVW 变成 `0x64`，条件 `op in (0x24,0x2C)` 永不成立，**结果恒为 0 命中** | 改用 `(h1>>4)&0x3F` |
- 自检方法：拿 `0x810994C4 movw r1,#0x6cf0` + `0x810994CA movt r1,#0x8138`
- （已知它们要组成 `0x81386CF0`）反算，立刻暴露两处错误。
- 修正后一次命中 3 个目标地址，全对。
- **⇒ 规约：自制的解码/扫描工具，必须先用一个**已知正确答案的样本**自检，再看结果。**
- 1. 失败表 `{F8,F2,F5,F0}` 由 `0x81196876` **按设计**建成（count=4，静态子模板）；
- 4. 断言 `appResourceList.c:207` → 随后 `appGxModel.c:809`（引用计数）→ `0x8115F9D6` 故意 abort。
- **⇒ 唯一待答的问题：「为什么一个 key = `0xF7` 的 job 被派发到这张表？」**

---

## `ARCH_414_DISPATCH_MISMATCH.md`

**标题**：ARCH_414 / ARCH_415 — MAIN MENU 派发键不匹配(真实阻断点)取证 ｜ 0. 结论摘要 ｜ 1. 被证伪的假设(ARCH_413 → ARCH_414) ｜ 2. 离线证据(可复现,不依赖解释器) ｜ 3. 运行时证据 ｜ 3.1 失败派发(日志 66060 / 66093 / 66125) ｜ 3.2 注册表(谁注册了 0xF7) ｜ 3.3 配对标定(ARCH_415,成功案例) ｜ 4. 关键地址与对象(本轮到手的确定事实) ｜ 4.1 建表器 `0x8117BD50(src, count)` — 逐行 ｜ 4.2 释放器 `0x8117B87C`(引用计数) ｜ 4.3 派发循环 `0x8117C93A`(真实函数起点) ｜ 5. ROM 安全策略(应要求) ｜ 6. 复现命令 ｜ 离线:解出 data.cpk 成员 -> CRILAYLA 解压 -> 遍历容器 ｜ 在线:构建并运行探针(指向副本) ｜ 7. ARCH_416 / ARCH_417 — 根因锁定(在-guest 双重确认) ｜ 7.1 `[ARCG]` 在-guest 直接读出 `ip`,证明离线模型与解释器都正确 ｜ 7.2 根因(地址与前几轮不同,结构完全一致) ｜ 7.3 调用现场:`lr=0xFFFFFFFE` ⇒ 协程尾调用 ｜ 7.4 嵌套派发链(bt 证据) ｜ 7.5 ARCH_418 — 执行 PC 环形缓冲(已实现并运行) ｜ 7.6 ARCH_419 — 静态交叉核对:失败入口不是 guest 代码可达的 ｜ 7.7 ARCH_420 — 根因确定并修复(已在 guest 内验证)

**发现/结论**（184 条，最多列 40）：

- > 结论**证伪**、给出经两种独立方法验证的真实原因。
- | # | 结论 | 证据 |
- | 1 | **阻断点不是**"`0x81174D64` 目录查找越界返回 0" | 控制流重读 + 离线复算容器项数 = **19** |
- | 2 | 真实原因:`key=0xF7` 被派发到 **TITLE_BIN 的表 `0xB5A32030`**(只注册 `{F8,F2,F5,F0}`),而该 job 的 provider 是 **MAINMENU_BIN 的请求对象 `0xB5B0202C`**,F7 注册在 **MAINMENU_BIN 自己的表 `0xB352F030`** | `[DISPATCH-MISS]` + `[LRQREQ#38]` + `[LRQPTR#36]` |
- | 3 | `dst` 由**派发循环** `0x8117C93A` 从 **slot 对象的 `+0x3c`** 取出;`job` 由容器迭代器产出 | `0x8117C954 / 0x8117C982` 反汇编 |
- | 4 | 表对象布局与生命周期已完全确定(建表器 `0x8117BD50`、释放器 `0x8117B87C`、全局双向链表 `[0x81556AF0]`) | 逐行反汇编 |
- 0x8117BAAC  r4 = [provider+0x3c]        ; ARC 句柄
- 0x8117BAB6  bl 0x81174D64               ; arcGet(handle, idx, &size)
- 0x8117BABA  r4 = r0
- 0x8117BABC  cmp r4,#0 ; beq 0x8117bb10   ; ← 误以为"返回 0"才会落到查表
- **实际控制流(完整反汇编 0x8117BA30..0x8117BBC8):**
- 0x8117BABC  cmp  r4,#0
- 0x8117BABE  beq  0x8117bb10
- 0x8117BAC0  ldr  r0,[r7,#0xc]        ; flags
- 0x8117BAC2  ands r0,r0,#1
- 0x8117BAC6  beq  0x8117bb0a           ; 无 bit0 -> 只存缓存
- 0x8117BB0A  str  r4,[r7,#0x34]       ; 成功:缓存 arcGet 结果
- 0x8117BB0C  ldr  r0,[sp,#0x10]
- 0x8117BB0E  str  r0,[r7,#0x38]       ; 成功:缓存 size
- 0x8117BB10  ldr  r0,[r7,#4]          ; ★ 成功路径**同样**落到这里
- 0x8117BB12  movs r4,#0
- 0x8117BB14  ands r0,r0,#8            ; flags bit3
- 0x8117BB18  beq  0x8117bb42          ; bit3=0 -> 直接按键查表
- 0x8117BB1A  ldr  r2,[r7,#0x44]       ; bit3=1 时额外校验 provider
- 0x8117BB1C  cbnz r2,0x8117bb38
- 0x8117BB38  ldr  r0,[r2,#4]
- 0x8117BB3A  movs r4,#1
- 0x8117BB3C  ands r0,r0,#1
- 0x8117BB40  beq  0x8117bb90          ; provider 未就绪 -> 退出
- 0x8117BB42  ldr.w lr,[r6,#0xc]       ; ★ 按键查表(dst 的 count)
- 0x8117BB4A  ldr  r3,[r7,#8]          ; key
- 0x8117BB62  movs r5,#0               ; 未找到
- 0x8117BB68  cbnz r5,0x8117bb80
- 0x8117BB6A  ... assert 207 ...
- 0x8117BB80  ldr  r5,[r5,#4]          ; r5==0 -> BADMEM 0x4(真机同样致命)
- **⇒ 无论 `arcGet` 成功还是失败,只要 `flags` 的 bit3(0x8)为 0,都必然到 `0x8117BB42` 按键查表。**
- `0x81174D64(r0=handle, r1=idx, r2=out)` 的逐行语义(**从反汇编直译**):
- ⇒ 节点布局:节点头 `0x24` 字节(其中 `+0x00..+0x1F` 为 0x20 字节名字,`+0x20` 为 payload 长度),
- **B. 真实成员**(探针日志 `[IO] PREAD fd=0x4002 off=0x6E131800 size=0x897A4` 对应的 `data.cpk` 成员):
- member @0x6E131800 len=0x897A4 -> 563108 字节

---

## `ARCH_41_BLOCKER_FIX.md`

**标题**：ARCH_41 — 工程突破：`appSound.c:1003` 阻塞已消除 ｜ 0. 结论 ｜ 1. 根因链（完整证据） ｜ 1.1 运行时参数（决定性） ｜ 1.2 断言点 `[Ghidra]` ｜ 1.3 `0x812B2670` 的真实身份 `[算术]` ｜ 1.4 真正的缺陷：返回值约定错误 `[Ghidra]` + `[算术]` ｜ 2. 修复 ｜ 3. 验证结果 ｜ 4. 下一个阻塞点（已定位） ｜ 5. 更正 ARCH_40 §1（我上一轮的结论是错的） ｜ 5.1 方法论教训（第三次，且方向相反） ｜ 6. 目标状态更新（第 (2) 项）

**发现/结论**（38 条，最多列 40）：

- > **编码** UTF-8。地址为**真实 VA**（ARCH_34 的 0xF60 修正）。
- | 1 | **阻塞根因定位**：`_FDclass`（SceLibm/libstdcxx NID `0xBD8EF217`）的 HLE 实现用了 **glibc 返回值约定**，而 PSVita 是 **ARM RTABI 位掩码约定** |
- | 2 | **已修复**：改为 ARM RTABI 后，`appSound.c:1003` 断言**不再触发** |
- [ASSERT-SITE] caller=0x81171546 r0=0x813072A0 r1=0x3EB r2=0x813072E4 r3=0x81670B48
- - `r0 = 0x813072A0` → 直接读取为 **`E:\P3DP5D\xrd757\...\appSound.c`** `[字节]`
- - `r2 = 0x813072E4` → `"ASSERT:%s"`（cond 参数）
- `mnsProcess` 无关；断言在 `appSound_81171476`（`0x81171476`，appSound.c，332 B）内：
- if (iVar2 == 2) assert(appSound.c, 0x3EB);   // 行 1003 ★
- iVar2 = FUN_812b2670(0x3f800000);        // 传 1.0f
- FUN_81172e76(0x3f800000, param_1);
- iVar2 = FUN_812b2670(0x3f800000);
- 调用点反汇编 `[Ghidra]`（capstone，Thumb）：
- 81171526  blx  #0x812b2670
- 8117152c  bne  #0x81171546         ; != 2 则跳过断言
- 81171542  bl   #0x8115f832         ; assert helper
- - 它位于 **ARM veneer 区** `0x812B11A0–0x812B2AF0`：全段 **406 个 `mvn r0,#0; bx lr`**
- 0x812B2670 -> module=SceLibstdcxx library_nid=0xCDB4F8BD func_nid=0xBD8EF217 (index 36)
- [NIDCALL] 1 impl=Y func=0xBD8EF217 lib=0xCDAE3C7D  SceLibm/func=0xBD8EF217
- // eboot_probe.cpp:6581   bind_by_nid(0xBD8EF217u, n_fdclass);   // _FDclass（浮点分类）
- ⇒ `0xBD8EF217` = **`_FDclass` / `__fpclassifyf`（浮点分类）**。
- 于是：`_FDclass(0.0f)` 在端口返回 **2**，断言 `cmp r0,#2` 命中 → 误报！
- **这也解释了断言的语义**：`assert(_FDclass(x) != FP_INFINITE)`，即
- 「断言这个浮点值不是无穷大」—— 而不是「不是零」。用 glibc 约定读它必然误判。
- | 断言位置 | `appSound.c:1003` | **`appFontUnitList.c:381`（更靠后）** |
- | 停止原因 | `stop=3(BADMEM)` @ `0x8115F9D6` | 同左（但到达时间晚一倍） |
- [ASSERT-SITE] caller=0x81023404 r0=0x812EF018 r1=0x17D r2=0x812EF00C r3=0x8115D7C7
- 断言所在函数 `[Ghidra]`：
- // VA 0x810233C6, size 76, appFontUnitList.c
- if (sVar1 < 1)         assert(appFontUnitList.c, 0x17D);   // 381 ★ 触发
- ⇒ **字体单位列表的第 0 项值 < 1**，即字体资源未被正确加载/填充。
- 1. 找 `appFontUnitList_810233c6` 的调用者，确认 `param_1` 指向的对象来源
- 3. 检查 `appFontSystem`（`0x812EEF1C` 名字串，ARCH_30 §6.1 A 类）的初始化序列
- ARCH_40 §1 声称「`appSound.c:1003` 证据不足」，理由是「1003 不在 appSound.c 的断言行号集合内」。
- 提取断言行号的正则 `appSound\.c",(0x...)` **只匹配同行写法**，
- 实测：同行匹配 37 处、**换行匹配 22 处（全被漏掉）**，其中就包含
- - ARCH_40 的错误：**用不完整的静态提取集合去否定一个正确的运行时结论**
- ⇒ 正确做法：**静态提取必须自证完整性**（本次漏掉了 37% 的调用点），
- | ① 定位阻塞条件 | ✅ **已完成**（`_FDclass` 约定错误，含完整证据链） |

---

## `ARCH_42_RUNTIME_PROGRESS.md`

**标题**：ARCH_42 — 从「断言崩溃」到「持续运行」：第二阻塞点与运行时推进 ｜ 0. 本轮结论 ｜ 1. 进展量化 ｜ 2. 第二阻塞点分析 ｜ 2.1 断言位置 `[Ghidra]` ｜ 2.2 调用链 `[Ghidra]`（`true_seg0_final.c` 输出行 35724） ｜ 2.3 权宜处理与结论 ｜ 3. 越过断言后的运行时行为（首次出现的事件） ｜ 3.1 影片定位成功 ｜ 3.2 CPK 加载器状态机推进 ｜ 3.3 fiber（协程）调度 ｜ 3.4 多线程活跃 ｜ 4. 自旋点的真实身份（澄清） ｜ 5. 当前真正的限制：指令预算 ｜ 6. 下一步 ｜ 6.1 本轮已执行：`P5D_CAP=3000000000` 实测结果 ｜ 6.2 资源查找已到达影片层（新证据） ｜ 6.3 接下来的工作 ｜ 7. 对目标第 (2) 项的状态更新

**发现/结论**（37 条，最多列 40）：

- | 2 | 越过该断言后，**游戏不再崩溃**：`stop=6(RUNNING)`，单轮运行 277 秒、**12.0 亿条指令** |
- ⇒ 相对起点，**可执行指令量提升 4.45×**，且**从"崩溃"变为"持续运行"**。
- [ASSERT-SITE] caller=0x81023404 r0=0x812EF018 r1=0x17D r2=0x812EF00C r3=0x8115D7C7
- 反汇编 `appFontUnitList_810233c6`（`0x810233C6`，76 B）`[Ghidra]`：
- 810233ca  bne      #0x810233e4
- ... 断言 appFontUnitList.c:0x17C (380)   ; obj == NULL
- 810233ea  bgt      #0x81023408              ; 计数 > 0 才继续
- ... 断言 appFontUnitList.c:0x17D (381)   ; ★ 触发的断言
- if (*obj <= 0)                    assert(appFontUnitList.c, 381);  // ★
- 断言失败 = **对一个引用计数已 ≤ 0 的对象再释放一次**（double-release）。
- ⇒ 属**应用层引用计数管理**问题，不是单个 HLE 函数的返回值约定问题（与第一阻塞点性质不同）。
- `P5D_ASSERT_IGNORE=1` 让该断言返回"忽略"，函数继续执行 `(*obj)--`（计数变 −1）。
- 实测**未引发后续 NULL 解引用**，游戏继续推进 —— 说明此处忽略是安全的。
- > 本次不同：该断言只影响字体对象的引用计数，不参与指针有效性判定。
- [CPKST] #1 loader=0xA050F028 state=3  lr=0x81293F05
- [CPKST] #3 state=4  lr=0x81287AEB
- [FIBCHK] #1 curFiber=0x812B4150 snap=0x812B4150 YIELD reqState=[r8+c]=2 ... total48=0x800
- 与 ARCH_7（fiber/异常）的分析吻合：`0x812B4150` 是 fiber 快照锚点。
- [EBOOT-START] stop=6(RUNNING) last_pc=0x811C2742 r0=0x1A sp=0xA0388FC0 curThread=0x6008
- 其包含的三个 SceLibKernel 调用经 NID 库确认为：
- | NID | 函数 |
- | `0x91FA6614` | `sceKernelUnlockLwMutex` |
- | `0x0FB972F9` | `sceKernelGetThreadId` |
- | `0x3148C6B6` | `sceKernelLockLwMutexCB` |
- // 该自旋点的四个 SVC 正是：LockLwMutex(0x46E7BE7B) / LockLwMutexCB(0x3148C6B6) /
- //   UnlockLwMutex(0x91FA6614) / GetThreadId(0x0FB972F9)
- ⇒ **不是新缺陷**，无需处理。
- ⇒ **1.2e9 是探针预算，不是游戏卡住**。源码注释本身就指出这一点。
- [EBOOT-START] stop=3(BADMEM) last_pc=0x81181CFA totalInsn=712,160,000
- - 崩溃点 `0x81181CFA` ∈ `FUN_81181CE4`（170 B，**无源文件归属**，区段 25 kernel 侧）
- `last_pc=0x811C2742` ∈ `FUN_811C241E`（1348 B，同样无归属）
- ⇒ **运行是非确定性的**（多线程 + 协作调度 + fiber 抢占），
- 两次的停止点不同（`0x81181CFA` vs `0x811C2742`），且都是 `BADMEM`。
- ⇒ **指令预算 `CAP` 目前不是瓶颈**，真正的瓶颈是这些散落的 `BADMEM` 点。
- ⇒ 端口**故意**不伪造影片帧，让引擎按时间判据推进影片相位（与 ARCH_34 的影片分析一致）。
- 1. 反汇编 `FUN_81181CE4`(0x81181CE4, 170 B) 与 `FUN_811C241E`(0x811C241E, 1348 B)，
- 3. 若影片相位能走完，再确认是否进入 `TITLE`（app state 6）。

---

## `ARCH_43_TITLE_REACHED.md`

**标题**：ARCH_43 — 运行时首次到达 TITLE 阶段 ｜ 0. 本轮结论（重大） ｜ 1. 三次运行的对照 ｜ 2. `P5D_NO_PREEMPT` 的作用与 `0x81181CFA` 的因果 ｜ 2.1 端口源码已记录该 BADMEM 的完整因果链 `[源码]` ｜ 2.2 本轮独立验证 ｜ 2.3 崩溃点 A 的反汇编（供后续参考）`[Ghidra]` ｜ 3. 首次到达 TITLE 阶段的证据 ｜ 3.1 序列创建与查找 `[实测]` ｜ 3.2 titleSequence 状态机推进 —— **整轮走完** ｜ 3.5 阶段实证：运行时创建的具名序列 ｜ 3.6 正在加载主菜单资源 ｜ 3.3 并行出现的其它阶段活动 ｜ 3.4 一个需要注意的端口开关 ｜ 4. 配置配方（供复现） ｜ 5. 目标第 (2) 项状态更新 ｜ 6. 下一步

**发现/结论**（39 条，最多列 40）：

- | 4 | 澄清 `0x81181CFA` BADMEM 的因果链：**它是全局 IGNORE 的副作用，不是独立缺陷** |
- | `ASSERT_IGNORE=1`（有抢占） | `BADMEM` @ `0x81181CFA` | 7.12 亿 或 12.0 亿（**非确定**） | 影片加载 |
- ⇒ 可执行指令量相对起点 **≥ 9.7×**。
- //   0x8117BB42 table miss -> r5=0 -> assert appResourceList.c:207 -> ignore
- //   -> 0x8117BB80 ldr r5,[r5,#4] (badmem 0x4) -> req[0x40]=NULL
- //   -> 0x81181CFA ldr r0,[r3,#0x94] (badmem 0x94) -> stop=3(BADMEM).
- //   实测证据：进程在 `appJobQueue.c`（job 队列链表插入）处以 BADMEM 终止
- 本轮实测 `P5D_NO_PREEMPT=1` 后：
- ⇒ **BADMEM 是抢占导致的竞态**；关抢占后消失。
- ⇒ `0x81181CFA`（`FUN_81181CE4` 的 `ldr r0,[r3,#0x94]`）本身**没有 NULL 检查**，
- 81181d34  blx    #0x812b2270                          ; memcpy（0x100 字节）
- 函数形态：从 `param_1` 指向的结构体中拷贝 `0x100` 字节记录（`0x812b2270` = `memcpy`，
- [TASKCREATE #43] r0=0x0 r1='title sequence' r2=0xC8 r3=0x0 lr=0x811980F8
- [FIND900] #1 key=0x8130B140 'title sequence' hash=0x7935 listhead=0xB2E4C01C |
- n0=0xB2E4C01C w18=0x7935 wC=0xA002B3E0 w10=0xB2DD101C <==MATCH
- | 名字串 VA `0x8130B140` | `'title sequence'` 的串地址 | ARCH_25 / ARCH_39 §2.2 备注 |
- | 创建点 `lr=0x811980F8` | 创建点 VA `811980F4`（+4 为返回地址） | **ARCH_30 §6.1** |
- ⇒ 静态注入的 141 个序列、节点布局、hash 公式在**运行时全部得到印证**。
- 对照 **ARCH_34 §5** 解出的 TBH 分发表（表基址 `0x81198188`）：
- | 0 | `0x8119819A` | ✅ |
- | 1 | `0x811981E4` | ✅ |
- | 2 | `0x811981F6` | 未采样到 `[推断]`：可能在本轮采样窗口外 |
- | 3 | `0x81198214` | ✅ |
- | 4 | `0x81198224` | ✅ |
- | 5 | `0x81198236` | ✅ |
- | 6 | `0x81198244` | ✅ |
- | 7 | `0x8119826A` | 被端口开关 `[TSEQ-SKIP]` 直接推到 8 |
- | 8 | `0x811982A4`（返回） | ✅ |
- ⇒ **静态解出的 9 路 TBH 表在运行时被逐一验证**，这是 ARCH_34 §5 的强证据。
- **⇒ 运行时已走完 `LOGO → 影片 → 标题 → 主菜单` 的序列创建链。**
- ⇒ 资源二分查找的目标已从 `movie/*.usm`（影片）转为 `mainmenu/*`（主菜单），
- [LRQREQ#16]  fn=0x8117C1BC nm='MUSIC_TITLE_P5' grp='COMMON_ARC_P5'   ; 标题音乐资源请求
- [LHANDLER #28] handler=0x8117C7E7 req=0xB24DC02C nm='MUSIC_TITLE_P5'
- [LRQDUMP q=12] mgr=0xB24F1030 busy=1 n=32 PENDING: ... COMMON_ARC_P5 | COMMON_P5 | ...
- ⇒ **资源请求系统正在为 TITLE / MAIN MENU 预加载资源**（`MUSIC_TITLE_P5`、`COMMON_ARC_P5`）。
- set P5D_ASSERT_IGNORE=1        REM 越过 appFontUnitList.c:381 的良性断言
- | ① 定位阻塞条件 | ✅ 已完成（两个断言点均已定位到汇编级） |
- ⇒ 第 (2) 项从"未达成"推进到**仅剩 ③④**，且 ③④ 现在**具备执行条件**。
- 1. 用 `P5D_CAP` 跑到 40 亿，确认是否能继续到 **MAIN MENU（state 5）→ 选歌**。

---

## `ARCH_44_RESOURCE_FORMATS.md`

**标题**：ARCH_44 — 资源格式全量解析（第一轮）：全量清单与 GFS0 统一容器 ｜ 0. 本轮结论 ｜ 1. 全量资源清单（10,833 个文件） ｜ 1.1 容器 ｜ 1.2 按 magic 分类（这是关键发现） ｜ 2. `.gap`：6,972 个角色动画（66%） ｜ 2.1 目录分布（说明它是什么） ｜ 2.2 GFS0 头部结构（120 文件差分实测） ｜ 2.3 字符串表内容（说明容器语义） ｜ 3. 为什么 eboot 里找不到 GFS0 解析器 ｜ 4. 解析状态矩阵（更新 ARCH_20 §4.6） ｜ 5. 下一步（资源解析专项）

**发现/结论**（21 条，最多列 40）：

- > **编码** UTF-8。这是"资源格式全量解析"专项的第一份产出。
- > 前置：ARCH_20（数据格式，早期）、ARCH_34（地址修正）。
- | 2 | 通过 **magic 分类**发现：**`GFS0` 是统一容器格式，覆盖 7,919 个文件（占 data.cpk 的 75%）** |
- | 3 | **`.gap` 不是独立格式**——它的 magic 就是 `GFS0`，6,972 个全部位于 `dance/player/`（角色动画库） |
- ⇒ `p3`/`p5` = 角色代际；`pcXXX` = 角色/服装；`_hNN`/`_pNN` = **动作（舞蹈）ID**。
- | `.gfs` | `+0x12D` / `+0x179` | `RootNodeH` / `camera22O`（**第二次出现 ⇒ 容器含 ≥2 个块**） |
- ⇒ **GFS0 = 分块容器，每块含 (类型标记 + 字符串表 + 浮点数据区)**；
- **实测** `[字节]`：
- va=0x813366D8 / 0x813B1F08 / 0x813B297C   （嵌入式资源模板，非代码常量）
- "RootNodeH" 出现 2 次，同样在数据区（0x813B1FFA / 0x813B2A6E）
- "RootNode" 出现 3 次，其中 0x812FB040 处于字符串池：
- ⇒ **eboot 代码不做 magic 比较**。GFS0 由**静态链接的 GFD 代码**（`gfd_app_gx_common.c` /
- ⇒ 因此解析 GFS0 有两条路：
- ⇒ 这些模板决定了"哪个资源在什么条件下被加载"，可直接接入 ARCH_25 的路径串清单。
- | `0x8107AAC2` | 8 | 9 event |
- | `0x81091A6C` | 4 | 10 game |
- | `0x810457F0` | 3 | 7 dance |
- | `0x810900FA` | 3 | 10 game |
- | `0x8104092A` / `0x81040912` / `0x81040956` … | 1–2 | 7 dance |
- | 格式 | 文件数 | 状态 | 依据 |
- *本文为「资源格式全量解析」专项第 1 轮产出。*

---

## `ARCH_45_GFS0_SECTIONS.md`

**标题**：ARCH_45 — 资源格式全量解析（第二轮）：GFS0 段结构 ｜ 0. 本轮结论 ｜ 1. GFD 名字条目（已确证） ｜ 1.1 证据 ｜ 1.2 名字后的 4 字节不是哈希 ｜ 2. 段标识与段表 ｜ 2.1 段标识（已确证） ｜ 2.2 段表存在（初步） ｜ 2.3 文件布局（推断） ｜ 3. 已确认 vs 待解（GFS0） ｜ 4. 下一步（可复现的方法）

**发现/结论**（20 条，最多列 40）：

- | # | 结论 | 置信度 |
- | 1 | **GFD 名字条目格式** = `[u8 len][name[len]][4 字节数据]` | **已确证**（`camera1` len=7 完全吻合） |
- | 2 | **段标识** `01 10 50 90`（`.gap`/`.gfs`）与 `01 10 40 50`（`.gmd`） | **已确证**（150+ 文件扫描，占比压倒性） |
- | 3 | **段表存在**，条目形如 `{u32 id, u32 offset}` | **初步**（1 例实测，待多文件验证） |
- | 5 | 那 4 字节**不是**名字哈希（已排除 crc32/fnv1/fnv1a/djb2/sum） | **已排证** |
- ⇒ **格式确定**：`[u8 len][name]`，名字**不以 NUL 结尾**，长度即字符数。
- ⇒ 它是**数据字段**，不是名字的校验值。
- ⇒ 段标识的第 3–4 字节是**段类型**：`50 90`（最常见）、`40 50`、`50 70`、`51 00` 等。
- （注意：字节序使 `0x90501001` 与 `0x01105090` 两种读法都存在，本报告统一按**字节序列**书写。）
- 0x00000001   0x00000010    0x00000001
- ⇒ `+0x17F7` 处的 u32 = **`0x10`** = 段 1 的标识偏移，其前一个 u32 = **`0x01`**。
- | 段标识与段类型字节 | ✅ 已确证 |
- | 名字条目 `[len][name][4B]` | ✅ 已确证 |
- 统计命中率 → 若高，则段表条目格式确立。
- （`.env` 几乎全是 `5100`、`.epl` 偏 `5070` ⇒ 段类型与内容种类强相关）。
- （float 密集 ⇒ 变换/关键帧；字节密集 ⇒ 索引；含 `dds` 名 ⇒ 材质引用）。
- 反汇编其中 2–3 个（`0x810457F0`(dance)、`0x8107AAC2`(event)），
- 看它们按什么偏移访问容器 —— 可直接确认段表位置。
- 每轮推进一个已确证的小结构，并在文档中区分「已确证 / 初步 / 推断」。
- *本文为「资源格式全量解析」专项第 2 轮产出。*

---

## `ARCH_46_SIMPLE_FORMATS.md`

**标题**：ARCH_46 — 资源格式全量解析（第三轮）：简单格式解出 + GFS0 最小样本 ｜ 0. 本轮结论 ｜ 1. 撤回：段表假阳性（诚实记录） ｜ 2. `.bf`：magic 是 `FLW0`（事件流程脚本） ｜ 2.1 原始字节 `[字节]`（`c_event\e100\e102_012.bf`，982 字节） ｜ 2.2 解出的字段 ｜ 3. `.spr6`：精灵图集 ｜ 3.1 原始字节（`mainmenu\collection\P3\resource\costume_icon.spr6`，18786 字节） ｜ 3.2 字段 ｜ 4. 三个格式的「段偏移表」模式（关键共性） ｜ 4.1 `.ctbl`（`app_system\font\font.ctbl`，22024 字节） ｜ 4.2 `.lbn`（`mainmenu\collection\P3\layout\character.lbn`，4312 字节） ｜ 4.3 `.plg`（`mainmenu\akarusa_win.plg`，576 字节） ｜ 5. GFS0 最小样本（119 字节）—— 后续分析的基准 ｜ 5.1 与 `.plg` 的共性启示 ｜ 6. 解析状态矩阵更新 ｜ 7. 下一步

**发现/结论**（21 条，最多列 40）：

- | # | 结论 | 置信度 |
- | 2 | **`.bf` 的 magic 是 `FLW0`**（位于 `+0x08`），是**事件流程脚本**，含事件标签名 | **已确证** |
- | 3 | **`.spr6`** 头部解出：`+0x08` = 文件大小、`+0x20` = 名字、`+0x4C` = 显示名 | **已确证** |
- | 4 | **`.ctbl` / `.lbn` / `.plg` 都含「段偏移表」**（`u32[]`，末项常等于文件大小） | **已确证** |
- | 5 | 多个格式共享头部模式：`magic[4] + 00 00 01 00 + 段偏移表` | **已确证** |
- ⇒ 偏移候选值本身很小（`0x4`、`0x10`、`0x24`…），而**小整数在文件中遍地都是**，
- **结论：该方法不可靠，ARCH_45 §2.2 的推断作废。**
- 教训：用"搜索某个值的出现"来证明结构存在时，必须排除**值域过小**导致的假阳性。
- ⇒ `e1xx`/`e2xx` = 事件编号；`c_` 前缀 = "cut"（过场）；`.bf` = FLW0 流程。
- **意义**：`ARCH_20 §4.6` 把 `.bf` 列为「未解析」。本轮确认它是**事件流程脚本**，
- | `+0x04` | `00 00 01 00` | 版本（**多个格式共有**） |
- | `+0x38` | `0x3AEBCC` = 3861452 | `[推断]`纹理数据偏移 |
- ⇒ **`.spr6` = 精灵图集容器**：`magic + 版本 + 文件大小 + 名字 + 显示名 + 纹理/精灵块`。
- ⇒ **`+0x0C..` 是段偏移表**，`+0x20` 起是字形宽度数据（`02 00 06 00 07 00 16 00 …`）。
- ⇒ `.lbn` = **UI 布局二进制**，引用 `.spr6` 图集。与 ARCH_22（UI）对应。
- | `+0x04` | `00 00 00 02` | 版本（注意与其它格式的 `00 00 01 00` 不同） |
- ⇒ **`.plg` 的段偏移表 = `u32[4]`，末项等于文件大小**，
- | 格式 | 文件数 | 上一轮 | **本轮** |
- | **`.bf`** | 226 | ❌ | 🟡 **magic=`FLW0`，指令表格式已见**（本文 §2） |
- ⇒ **本轮把 5 个格式从"未解析"推进到"头部已解"**。
- *本文为「资源格式全量解析」专项第 3 轮产出。*

---

## `ARCH_47_EFFECT_FORMATS.md`

**标题**：ARCH_47 — 资源格式全量解析（第四轮）：.ep/.ef/.gnf/.fnt 解出 + GFS0 否定结果 ｜ 0. 本轮结论 ｜ 1. GFS0 的否定结果（排除一个假设） ｜ 1.1 判据与对照 ｜ 1.2 附加排除：没有"等于文件大小"的字段 ｜ 1.3 一个确证的规律：`.gap` 的 `+0x18` ｜ 1.4 GFS0 结构假设的当前状态 ｜ 2. `.ep`：粒子效果字节码（19 个文件） ｜ 2.1 原始字节 `[字节]`（`app_system\effect\ep\CHARGE1_0.ep`，208 字节） ｜ 2.2 结构 ｜ 3. `.ef`：效果定义（14 个文件） ｜ 3.1 原始字节（`app_system\effect\testFile.ef`，944 字节） ｜ 3.2 结构 ｜ 4. `.gnf`：标准 Sony GNF 纹理（3 个文件） ｜ 5. `.fnt`：点阵字体（3 个文件） ｜ 6. 解析状态矩阵更新 ｜ 7. 下一步

**发现/结论**（31 条，最多列 40）：

- | # | 结论 | 置信度 |
- | 2 | **`.gap` 的 `+0x18`（BE u32）= 文件大小 − 32**（40/40 文件一致） | **已确证** |
- | 3 | **`.ep` = 粒子效果字节码**：4 字节指令流 `{u8 opcode, u24 参数}` | **已确证** |
- | 4 | **`.ef` = 效果定义**：含纹理名（`triangle.tga`）与效果名（`testFile`） | **已确证** |
- | 5 | **`.gnf` = 标准 Sony GNF 纹理**（magic `GNF `，头大小 `0xF8`） | **已确证** |
- | 6 | **`.fnt` = 点阵字体**：字形尺寸 `0x20×0x20` + 字形宽度表 | **已确证** |
- ⇒ **判据有效，但 GFS0 不满足它** ⇒ **GFS0 没有偏移表形式的段结构**。
- ⇒ GFS0 **不存储自身的总大小**（与 `.spr6`/`.plg` 明显不同）。
- ⇒ **`.gap` 的 `+0x18`（按 BE 读 u32）= 文件大小 − 32**。
- | 偏移表段结构（如 `.plg`） | ❌ **本轮排除** |
- | 存有总大小字段 | ❌ **本轮排除** |
- | `[u8 len][name][4B]` 名字条目 | ✅ 已确证（ARCH_45 §1） |
- | `01 10 XX YY` 段/块类型标记 | ✅ 已确证（ARCH_45 §2.1） |
- | `+0x00` | `2E 45 50 00` | magic `.EP\0`（**小端**，与其它格式相反） |
- 01 00 00 10      op=0x01  arg=0x100000
- 02 00 00 10      op=0x02  arg=0x100000
- 05 00 10 20      op=0x05  arg=0x201000
- 06 00 00 20      op=0x06  arg=0x200000
- 07 00 00 20      op=0x07  arg=0x200000
- ⇒ `.ep` = **24 个粒子（19 个文件）+ 1 个 `testFile`/`effect` 的字节码脚本**，
- 指令 opcode 语义未定，但**格式框架已明确**。
- ⇒ **`.ef` = 具名效果定义：`magic + 版本 + 计数 + 纹理引用 + 效果名 + 指令流`**。
- | `+0x0C` | `00 01 01 00` | 格式 / 维度标记 |
- ⇒ 这是 **Sony 官方的 GNF（GNF = Graphics Native Format）纹理容器**，
- 与 `.dds`/`.tga`/`.png` 并列为第 4 种纹理格式。只有 3 个，位于
- | `+0x0C` | `0` 或 `0x6C310000` | `[推断]`位图偏移 |
- ⇒ **`.fnt` = 点阵字体**：magic + 字形单元尺寸 + `(宽, 位图偏移)` 表 + 位图数据。
- | 格式 | 文件数 | 上轮 | **本轮** |
- | **GFS0 容器** | **7,919** | 🟡 | 🟡 **排除"偏移表"与"总大小字段"两个假设**；`.gap` 的 `+0x18`=size−32 已确证 |
- 1. **GFS0 结构**：现有证据指向「**顺序块 + `01 10 XX YY` 类型标记 + 名字条目**」，
- *本文为「资源格式全量解析」专项第 4 轮产出。*

---

## `ARCH_48_RESOURCE_TREE.md`

**标题**：ARCH_48 — 引擎绑定资源：完整目录树与功能定性 ｜ 0. 本轮结论 ｜ 1. 顶层资源树（`data.cpk` 解包后） ｜ 1.1 关键比例 ｜ 2. 二级分解与功能定性 ｜ 2.1 `dance/`（音游对局） ｜ 2.2 `mainmenu/`（主菜单 / 选歌 / 鉴赏） ｜ 2.3 `event/` + `c_event/`（事件系统） ｜ 2.4 `sound/`（音频） ｜ 2.5 `app_system/`（引擎资源） ｜ 2.6 其它 ｜ 3. GFD 模块在 eboot 中的形态（本轮发现） ｜ 3.1 实测：这些是**运行时对象操作**，不是文件解析 ｜ 4. 资源格式解析状态（累计） ｜ 5. 下一步（按价值排序）

**发现/结论**（13 条，最多列 40）：

- > **编码** UTF-8。承接 ARCH_44–47（资源格式专项）。
- | 5 | 资源格式专项累计：**22 个扩展名中 9 个完全解析、12 个头部已解、3 个未解** |
- - `.gap` 6,972 个（占全部文件 66%）全部位于 `dance/player/` ⇒ **角色舞蹈动画**
- ⇒ `event` 与 `c_event` 是**同一套事件系统的两份**（普通 + 过场），共 **547 个文件**。
- eboot 里静态链接了 **13 个 `gfd_app_gx_model.c` 函数**（`0x81014932`–`0x81015xxx`）等：
- // 0x8101508E —— 包围盒读取
- // 0x81015204 —— 节点链表遍历，按类型号筛选
- // 0x8101556C —— 节点指针数组 + 节点内 u16 类型
- ⇒ **运行时的 GFD 对象模型**：`{节点指针数组, 数量}` + 每节点首字段类型号、
- ⇒ 文件解析器可能在更底层（运行时库或独立模块），或以不同形式内联。
- | 状态 | 格式（文件数） |
- - 途径 A：找 `gfd` 库的**文件→对象**转换点（可能在 `0x81014CCE`(430 B) / `FUN_8101582E`(408 B) 等大函数）
- *本文为「资源格式全量解析」专项第 5 轮产出。*

---

## `ARCH_49_GFS0_SECTION_CHAIN.md`

**标题**：ARCH_49 — GFS0 段结构完全确证（覆盖 75% 资源） ｜ 0. 本轮结论 ｜ 1. 段结构规则 ｜ 1.1 文件布局 ｜ 1.2 走链算法（可复现） ｜ 期望：off == len - 16 ｜ 2. 验证数据（120 个文件，全部通过） ｜ 2.1 三个实例链（逐字节可复算） ｜ 2.2 与此前观察的一致性 ｜ 3. `.env` / `.epl` 的例外（未解） ｜ 4. GFS0 剩余缺口：段内数据布局 ｜ 5. 资源格式解析状态（更新） ｜ 6. 下一步

**发现/结论**（11 条，最多列 40）：

- > **编码** UTF-8。承接 ARCH_44–48。**本文是资源格式专项的关键突破。**
- | # | 结论 | 置信度 |
- | 1 | **GFS0 段结构完全解出**：段头 16 字节，`+0x08` 为 **BE u32 段大小（含头）**，段首尾相接 | **已确证（120/120 文件）** |
- | 2 | 文件布局 = `12 字节文件头 + 段链 + 16 字节终止块` | **已确证** |
- | 3 | 段链**精确铺满** `0x10 … len−16`，无空洞 | **已确证** |
- | 4 | `.gfs`/`.gap`/`.gmd` 三类共 120 个文件**全部满足**该规则 | **已确证** |
- ⇒ **三次独立自洽**（单段链、三段链、两段链），且**末尾余数恒为 16**。
- ⇒ 它们的段标识**第 3–4 字节不同**（`5100` / `5070`），
- - **名字条目** `[u8 len][name][4B]`（ARCH_45 §1，已确证）
- | 格式 | 文件数 | 状态 |
- *本文为「资源格式全量解析」专项第 6 轮产出。*

---

## `ARCH_4_render.md`

**标题**：ARCH_4 — 渲染管线（Rendering Pipeline） ｜ 1. 模块职责（一句话） ｜ 2. 关键函数地址表 ｜ 2.1 guest 侧（eboot.bin，Thumb，加载基址 0x81000000） ｜ 2.2 host 侧（`src/gxm/`） ｜ 2.3 着色器程序 ↔ 哈希 ↔ 角色（本模块最关键的对照表） ｜ 3. 关键数据结构：字段偏移 + 含义 ｜ 3.1 `SceGxmTexture` 控制块（16 B，guest 内存） ｜ 3.2 颜色面（`SceGxmColorSurface`）——由 `[CSINIT]` 实测 ｜ 3.3 顶点格式表 `g_vfmt[eh]`（host 侧，探针采集） ｜ 3.4 主机侧 `OffscreenRT`（RT 池） ｜ 3.5 顶点解包输出布局（host 侧 `s_hvb`） ｜ 4. 生命周期与调用关系 ｜ 4.1 一帧的完整调用链（由日志顺序 + 探针聚合实证） ｜ 4.2 一帧的 draw 序列（两条相位，实测） ｜ 4.3 「引擎从不提交游戏内容几何」的真正含义 ｜ 5. HLE 接口现状 ｜ 5.1 拦截机制 ｜ 5.2 `SceGxm` 拦截情况 ｜ 5.3 内核/线程侧缺口（与渲染管线强相关） ｜ 5.4 纹理上传路径与 RT 别名（专项核证） ｜ 5.4.1 控制块（16 B `SceGxmTexture`）位域总表 —— 以 `texture_format.cpp:64` 的**代码实际移位**为准 ｜ 5.4.2 完整上传调用链（已核证） ｜ 5.4.3 RT 别名（render target as texture）机制

**发现/结论**（242 条，最多列 40）：

- > 本文档的每一条结论都附**反汇编行（含 VA + 指令原文）**或**日志行原文（含日志文件名）**。
- > 已实证结论，本文档直接复用并标注其出处行号。
- **证据来源（本文档实际读取过的）**
- | 日志 `_h1164.log`（12.6 MB，最新） | 全文检索 | 全部实测数据 |
- | 反汇编器 `py C:\dev\p5d_build\_p5d_dis.py <VA> <n>` | — | guest 侧地址核证 |
- 一句话的证据（三段式流水在本工程里各有独立代码路径）：
- | VA | 推测名 | 作用 | 证据（反汇编原文 / 日志原文） |
- | `0x811E45B6` | `engineDrawIndexed`（绘制提交点 #1） | 引擎的索引绘制包装函数；`sceGxmDraw` 的主要调用者（占 2/3 draw） | 反汇编：`811E45AC ldr.w r0,[r0,#0x270]` / `811E45B2 blx #0x812b15f0` → 返回地址即 `0x811E45B6`。日志：`[DRAWCALLER] draws=2000 … lr=0x811E45B6 count=1333`（`_h
- | `0x811EC006` | `engineDrawArrays`（绘制提交点 #2） | 非索引绘制包装（走 `gxm_draw_arrays`），占 1/3 draw | 反汇编：`811EC002 blx #0x812b15f0` → 返回地址 `0x811EC006`；其后 `811EC006 bl #0x811c52b6`。日志：`[DRAWCALLER] draws=2000 … lr=0x811EC006 count=667`（`_h1164.log`） |
- | `0x812B15F0` | `sceGxmDraw` 导入桩（Thumb 陷阱 `svc #0xAB; bx lr`） | 所有绘制的唯一咽喉点 | 反汇编：`812B15C0 movs r0,r0` / `812B15C2 b #0x812b1d86` 等为邻近 ARM 桩；`0x812B15F0` 被 `blx` 命中（见上两行）。`import_bridge.h:4-8` 说明桩格式 |
- | `0x8119826A` | `titleSequence` state=7 的异步等待点 | 轮询任务对象的完成位；**这是卡死点** | 反汇编：`8119826A ldr r0,[r4,#8]` / `8119826C bl #0x81193a6a` / `81198270 adds r5,r0,#0` / `81198272 beq #0x811982a4` |
- | `0x81193A6A` | 任务完成位读取器 | `r0=[[obj+8]]; r0 &= 2; bit1 ? [obj+8] : 0` | 反汇编：`81193A6A push {r4,lr}` / `81193A6C bl #0x8116f8a8` / `81193A70 ldr r1,[r0]` / `81193A72 ands r1,r1,#2` / `81193A76 beq #0x81193a7c` / `81193A78 ldr r0,[r0,#8]` /
- | `0x8119817C` | `titleSequence` 分派器 | `state=[r4]`；`state>8` → 退出；`tbh` 按 state 跳转 | 反汇编：`8119817C ldr r1,[r4]` / `8119817E cmp r1,#8` / `81198180 bhi.w #0x811982a4` / `81198184 tbh [pc,r1,lsl #1]` |
- | `0x8116F8AC` | 序列推进助手 `seqDrive(node)` | 全镜像 59 处调用；按 node 取序列对象 | 反汇编（`81198176 bl #0x8116f8a8` 的兄弟入口）；注释 `thumb_interpreter.cpp:1784-1791`。日志：`[SEQDRV] #1 node=0xB32F201C … #7 node=0xB2D8601C`（`_h1164.log`，共 7 个不同 node） |
- | `0x811EA89A` | 颜色面初始化调用者（主 RT 三缓冲） | 建立 960×544/stride1024 的三个颜色面 | 日志：`[CSINIT#1] surface=816CBC74 … stride=1024 data=A0F00000 … lr=0x811EA89A`（`_h1164.log`） |
- | `0x811E6C96` | 颜色面初始化调用者（离屏/小尺寸） | 建立 224×68 与 960×544/stride960 面 | 日志：`[CSINIT#4] surface=AA9A1008 … 224x68 stride=224 data=AE000000 … lr=0x811E6C96`（`_h1164.log`） |
- | `0x811E7D60` | 后处理链的顶点流设置点 | 224×68 直方图 pass 的 stream 写入 | 日志：`[VSET] va=AE600000 armed=1 lr=811E7D60 data8: 3DB6DB70 3F800000 …`（`_h1164.log`） |
- | `0x811E44C8` | 主 RT pass 的顶点流设置点 | 全屏合成 quad 的 stream 写入 | 日志：`[VSET] va=A2D501E0 armed=1 lr=811E44C8 data8: … 44700000 …`（`_h1164.log`，`44700000`=960.0f） |
- | `[EHMAP] vert='gxp_vert_0000.gxp.spv' eh=A71B28370132FF67 in_vfmt=1` | 清屏用全屏三角形（3 顶点，stride 12） |
- | `[EHMAP] vert='gxp_vert_0014.gxp.spv' eh=ABE1E100963724B0 in_vfmt=1` | 全屏合成 quad（4 顶点，stride 16） |
- | `[EHMAP] vert='gxp_vert_0015.gxp.spv' eh=172A9E736C7DC35E in_vfmt=1` | 屏幕空间 sprite/UI（stride 24，自投影） |
- | `[EHMAP] vert='gxp_vert_0006.gxp.spv' eh=4A3A4C8DFD6083F6 in_vfmt=1` | 直方图 pass 顶点（stride 20） |
- | `[EHMAP] vert='gxp_vert_0008.gxp.spv' eh=8D301D508EDD0DF5 in_vfmt=1` | 自适应亮度 pass 顶点（stride 20） |
- | `[EHMAP] vert='gxp_vert_0007.gxp.spv' eh=EC5D3D7E8BC3DADA in_vfmt=1` | 色调映射写回顶点（stride 28） |
- > `frag_0002 / frag_0003 → 224x68`，而 `[REFETCH]` 实测 224×68 上是
- > 任务书写的 `frag(0x6952EF6C507D515E)` 实为 **frag_0084**（内容层），
- | 字 | 位域 | 含义 | 证据 |
- | w2 | bit2..27 | 数据地址（字地址回填） | `sce_gxm.cpp:51` `((byte>>2)&0x3FFFFFFF)<<2` |
- 实测用法（`vulkan_backend.cpp:4032`）：`_base=_w2&0xFFFFFFFCu` 取数据面地址。
- **实测控制块实例**（`_h1164.log`，`[BGBIND d6330]`）：
- | `[CSINIT#1] surface=816CBC74 fmt=0 stype=00000000 scale=1 w=960 h=544 stride=1024 data=A0F00000 oreg=0 lr=0x811EA89A sp=0xA016DF28` | w/h/stride/data 四个字段 |
- | # | surface VA | w×h | stride | data | 备注 |
- > ⚠ **`[RTPOOL]` 实测的尺寸与任务书不一致**：任务书说 backbuffer 是 1024×544，
- > 实际 `[RTPOOL] create cd=A0F00000 960x544 pool=3`（`_h1164.log`）——**宽 960、stride 1024**。
- | 字段 | 含义 | 证据 |
- | `stride[]` | **权威** stride = `SceGxmVertexStream{ u16 stride; u16 indexSource; }` | `eboot_probe.cpp:336-343` |
- **实测样例**（`_h1164.log`）：
- [V14ATTR] eh=ABE1E100963724B0 size=2 strides[0..3]=16,0,0,0 stream0=0xA2D501E0: [0 st0 off0 fmt9 cc3 reg0->loc0] [1 st0 off12 fmt4 cc4 reg4->loc1]
- → `vert_0014` = 单流 stride=16；`loc0` = `F32×3 @off0`（位置）；`loc1` = `fmt4=U8N ×4 @off12`（颜色 RGBA8）。
- [VATTR-HIT] id=96 gxp=0xAA842008 fnv=172A9E736C7DC35E sz=445 r2=0xA016DD7C r3=0x3

---

## `ARCH_50_GFD_NODE_SECTION.md`

**标题**：ARCH_50 — 资源格式全量解析（第六轮）：段内结构起步（节点定义段） ｜ 0. 本轮结论 ｜ 1. `.env` / `.epl` 排除段链结构 ｜ 2. camera `.gfs`：段分布恒定性 ｜ 3. 段 0 体的结构（225 字节） ｜ 3.1 结构解读 ｜ 3.2 与运行时对象模型对应（ARCH_48 §3） ｜ 4. 段内解析的当前状态 ｜ 5. 下一步

**发现/结论**（8 条，最多列 40）：

- | # | 结论 | 置信度 |
- | 2 | **camera `.gfs` 的段 0 恒为 241 字节** —— 是固定的「**节点定义段**」 | **已确证（4 文件）** |
- | 3 | 段 0 体 = `RootNode` → `camera1`（子节点）+ 单位矩阵 + 相机参数（3 个 float） | **已确证** |
- | 4 | 段 1 = 数据段，大小随文件变化（3,582–35,811） | **已确证** |
- ⇒ **`.env` / `.epl` 是另一套容器布局**，与 `.gap`/`.gmd`/`.gfs` 不同。
- ⇒ **段 0 大小恒为 241 字节**，段 1 大小随文件变化。
- ⇒ **可对应但未逐一确证**，标注为推断。
- *本文为「资源格式全量解析」专项第 7 轮产出。*

---

## `ARCH_51_GFD_SECTION_BODIES.md`

**标题**：ARCH_51 — 资源格式全量解析（第七轮）：段体类型判定与 camera 段内结构 ｜ 0. 本轮结论 ｜ 1. 段类型判定（定量） ｜ 2. camera `.gfs` 段 0 的精确结构（已确证） ｜ 2.1 布局 ｜ 2.2 节点头的原始字节（`c108_106.GFS`） ｜ 2.3 与运行时对象模型的对应（已确证） ｜ 3. camera `.gfs` 段 1 的结构（部分解出） ｜ 3.1 float 数组起点 = 段体偏移 0 ｜ 3.2 float 数量与 stride 检验 ｜ 4. `.gap` 的反直觉结果 ｜ 5. 段内解析状态（更新） ｜ 6. 下一步

**发现/结论**（15 条，最多列 40）：

- | # | 结论 | 置信度 |
- | 1 | **camera `.gfs` 段 0 体 = 201 字节节点头 + 6 个 float** | **已确证（4/4 文件）** |
- | 2 | 段 0 末尾的 **6 个 float = 包围盒**（min.xyz + max.xyz），与运行时 `+0x58..0x6C` 对应 | **已确证** |
- | 3 | **camera `.gfs` 段 1 = 以 float 为主的数据流**（合理率 0.995–0.999） | **已确证（4/4）** |
- | 4 | 段 1 的 float 数量**不是 3/4/7/10 等常见 stride 的整数倍** ⇒ 含非 float 元数据 | **已排证** |
- | 5 | **`.gap` 段体找不到连续 float 区**（99% 阈值下），但 79% 的 4 字节窗口像 float ⇒ **强混合** | **已确证** |
- | 6 | `.gmd` 为多段混合（段 2 最大，f-ok ≈ 0.75） | 已确证 |
- ⇒ **三种容器的段构成不同**：
- ⇒ 节点头含：**两个名字条目**（`RootNode`、`camera1`/`camera22`）、
- ⇒ **6 个 float 的位置与含义对上了** —— 这是容器→运行时的**第一条确证映射**。
- ⇒ **没有统一的 stride**（3/4/7/10 都不普适）。
- 且每个文件都有 **0.4–0.5% 的窗口不是合理 float**（6–9 个窗口）⇒ **含少量非 float 元数据**。
- ⇒ **`.gap` 段体内浮点与其它数据交替出现**（不是"头部 + 浮点数组"，而是**穿插**）。
- | 段体：float 关键帧流 | 🟡 已确证存在（camera 段 1），内部通道表未解 |
- *本文为「资源格式全量解析」专项第 8 轮产出。*

---

## `ARCH_52_GMD_SECTIONS.md`

**标题**：ARCH_52 — 资源格式全量解析（第八轮）：`.gmd` 段语义与名字表 ｜ 0. 本轮结论 ｜ 1. `.gmd` 的段结构与名字表 ｜ 1.1 `pc200_00.GMD`（角色模型，88,301 字节） ｜ 1.2 `m001_001.GMD`（场景物件，362,859 字节） ｜ 1.3 段语义（初步） ｜ 2. 关键发现：Maya 导出术语 ｜ 3. 名字条目格式的第三次确证 ｜ 4. 顶点 stride 检测的失败（诚实记录） ｜ 5. 段内解析状态（更新） ｜ 6. 下一步

**发现/结论**（15 条，最多列 40）：

- | # | 结论 | 置信度 |
- | 1 | **名字条目 `[u8 len][name]` 第三次独立确证**（`prev-byte = 0x11 = 17 = len("thumbnail_000.dds")`） | **已确证** |
- | 2 | **`.gmd` 段内是「名字表 + 数据块」交替**，不是单一顶点数组 | **已确证** |
- | 4 | 发现 **Maya 导出术语**：`currentUVSet`、`map1`、`pv27_mt`（材质）、`m_thumbnail`（网格） | **已确证** |
- | 5 | 顶点 stride 检测**失败**（16–64 字节 × 对齐全试）——段内非纯顶点数组 | **已排证** |
- ⇒ `.gmd` 的来源是 **Maya 导出的中间格式**，与 GFD 工具链一致。
- @0x000026   prev-byte = 0x11 = 17 = len("thumbnail_000.dds")  ✔
- @0x0156BE   prev-byte = 0x11 = 17                              ✔
- 对 `.gmd` 的大段尝试 **12 种 stride × 16 种对齐**（16/20/24/28/32/36/40/44/48/52/56/64 字节），
- ⇒ **段内不是"从头到尾的顶点数组"**。结合 §1 的名字表统计（`m001_001` 段 2 有 1,135 个名字条目），
- 可确认段内是 **名字表与数据块交替**，因此整体 stride 检测必然失败。
- **教训**：在"块交织"的容器里做全局 stride 扫描是无效的 —— 必须**先切块**。
- | float 关键帧通道表 | 🟡 已确证存在，通道表未解 |
- 再对**纯数据片段**做 stride 检测（此时判据才有效）。
- *本文为「资源格式全量解析」专项第 9 轮产出。*

---

## `ARCH_53_GFD_VERTEX_STRIDE.md`

**标题**：ARCH_53 — 资源格式全量解析（第九轮）：`.gmd` 顶点 stride 确证 ｜ 0. 本轮结论 ｜ 1. 切块方法（可复现） ｜ 1.1 切块结果 ｜ 2. 顶点 stride 实测 ｜ 2.1 `m001_001.GMD` —— **stride = 44 字节** ｜ 2.2 `pc200_00.GMD` —— **stride = 64 字节** ｜ 2.3 两种 stride 并存 ｜ 3. 最干净的顶点样本 ｜ 4. 未验证项（诚实列出） ｜ 5. 资源格式解析累计状态 ｜ 6. 下一步

**发现/结论**（33 条，最多列 40）：

- > 本文只固化**已确证**的结论；未验证的推测一律标注。
- | # | 结论 | 置信度 |
- | 1 | **切块判据成立**：`.gmd` 段体按「名字条目 + ≥4096 字节间隔」可切成数据块 | **已确证** |
- | 2 | **`.gmd` 存在 stride = 44 字节的顶点格式**（`m001_001.GMD` 三个块一致：446 / 568 / 307 行） | **已确证** |
- | 3 | **`.gmd` 存在 stride = 64 字节的顶点格式**（`pc200_00.GMD`：315 / 639 / 193 行） | **已确证** |
- | 4 | 44 字节格式中**有 5 个 u32 slot 的 float 率 = 1.00**（20 字节纯 float），其余 24 字节为压缩/整数 | **已确证** |
- **关键**：上一轮顶点 stride 扫描失败的原因是**未先切块**；本轮按名字条目切块后扫描**立即成功**。
- 且 name 之后一字节非 printable —— 用以排除"从字符串中间开始"的假匹配）
- stride 候选 12/16/20/24/28/32/36/40/44/48/56/64。
- | 数据块 | 长度 | **stride** | 行数 | 每 slot 的 float 率（11 个 slot） |
- | `@0x0001AB` | 7,130 | **44** | 161 | `0.96 0.96 0.96 0.96 0.74 \| 0.65 0.71 0.65 0.50 0.49 0.96` |
- | **`@0x001D85`** | 19,656 | **44** | **446** | **`1.00 1.00 1.00 1.00 1.00`** \| `0.60 0.67 0.64 0.54 0.52 0.41` |
- | `@0x006A4D` | 4,609 | **44** | 104 | `0.58 0.38 0.42` \| **`1.00 0.96 1.00 1.00 1.00`** |
- | `@0x007C4E` | 25,002 | **44** | **568** | `0.55 0.56 0.49 0.60 0.48 0.58` \| **`1.00 1.00 1.00 1.00 1.00`** |
- | `@0x00DDF8` | 13,517 | **44** | **307** | `0.54 0.51 0.43` \| **`1.00 1.00 1.00 1.00 1.00`** |
- **注意**：`@0x001D85` 与 `@0x00DDF8` 的 float 组在**前 5 个 slot**，
- 而 `@0x007C4E` 的在**后 5 个 slot** ⇒ **块内可能存在两种子布局**（或有前导元数据偏移），
- | 数据块 | 长度 | **stride** | 行数 | 每 slot 的 float 率（16 个 slot） |
- | `@0x000020` | 20,183 | **64** | 315 | `0.35 0.62 0.27 0.98 \| 0.27 0.66 0.26 0.97 \| 0.26 0.67 0.28 0.97 \| 0.31 0.63 0.29 0.95` |
- | `@0x004EF7` | 9,826 | **64** | 153 | `0.13 1.00 0.49 0.55 \| 0.16 0.94 0.56 0.44 \| 0.20 1.00 0.56 0.50 \| 0.18 0.95 0.50 0.59` |
- | `@0x007559` | 40,961 | **64** | **639** | `0.22 0.96 0.30 0.51 \| 0.23 0.98 0.33 0.51 \| 0.22 0.96 0.29 0.57 \| 0.20 0.97 0.27 0.56` |
- | `@0x01155A` | 12,358 | **64** | 193 | `0.91 0.95 0.81 0.59 \| 0.97 0.98 0.83 0.49 \| 0.92 0.92 0.86 0.59 \| 0.91 0.93 0.86 0.65` |
- ⇒ **4 组「3 个压缩值 + 1 个 float」**。
- | 文件 | 模型类型 | stride |
- ⇒ **顶点格式随模型而变**，与段头类型标记（`5090`/`4050`/`55AA`…）相关 `[推断]`。
- **`m001_001.GMD` 块 `@0x001D85`：19,656 字节 / stride 44 / 446 行 / 前 5 个 slot 全 1.00**
- | `@0x007C4E` 的 float 组为何在后 5 slot | ❌ 未解释（`[推断]`：块内有前导元数据） |
- | `.gmd` 内是否存在索引缓冲及其格式 | ❌ 未定位 |
- | 状态 | 格式（文件数） |
- | 🟡 **段内数据** | **顶点 stride 已确证（44 / 64）**；字段语义未验 |
- 1. **字段级验证**（对 `@0x001D85` 块）：把那 5 个 float 按 `position.xyz + uv.xy` 解读，
- 3. 定位 `.gmd` 的索引缓冲（顶点数 446 ⇒ 索引数应为 3×三角形数）
- *本文为「资源格式全量解析」专项第 10 轮产出。*

---

## `ARCH_54_ASSERT_WHITELIST.md`

**标题**：ARCH_54 — 工程侧成果：断言白名单机制（已验证） ｜ 0. 本轮结论 ｜ 1. 为什么需要白名单（问题背景） ｜ 1.1 断言桩的两难 ｜ 1.2 需要的中间态 ｜ 2. 实现 ｜ 2.1 源码修改（`src/arm/thumb_interpreter.cpp`，`0x8115F7E0` 分支内） ｜ 2.2 用法 ｜ 3. 关键教训：白名单的键必须是 file 指针 ｜ 3.1 第一次尝试失败（用 LR 做键） ｜ 3.2 正确做法（用 file 指针 `r[3]`） ｜ 4. 验证结果 ｜ 4.1 由此澄清的历史疑点 ｜ 5. 完整的可玩推进配置（当前最佳） ｜ 6. 目标第 (2) 项状态

**发现/结论**（33 条，最多列 40）：

- > **编码** UTF-8。承接 ARCH_41–43（运行时推进）、ARCH_44–53（资源格式）。
- | # | 结论 | 证据 |
- | 1 | **断言白名单机制已实现并验证生效** | `[ASSERT-POLICY] ABORT in place skip=1` + `[ASSERT-SKIP] #1 lr=0x8115F9D0 (whitelisted)` |
- | 2 | **只放行单点断言**，其他断言仍 ABORT | 到 12 亿指令 `ASSERT-ABORT` 计数 = **0** |
- | 3 | **白名单的键必须是 file 指针 `r[3]`，不能是 LR** | `0x8115F7E0` 的 LR 恒为 `0x8115F9D0`（所有断言共用） |
- | 4 | **澄清历史疑点**：`0x81181CFA` BADMEM **不是**被吞掉的断言造成的 | 白名单版本到 12 亿指令无任何其他断言失败 ⇒ 该 BADMEM 源于**抢占竞态** |
- `0x8115F7E0` 是断言决策桩，由 `0x8115F9CA` 调用，返回 `0` = 忽略继续、返回 `1` = 主动崩溃
- | 全局忽略 | `P5D_ASSERT_IGNORE=1` | 所有断言都返回 0 | **良性与恶性断言一起吞掉** |
- | 全部中止 | （默认） | 所有断言都返回 1 | 良性断言（如字体引用计数）也导致停滞 |
- //   0x8117BB42 table miss -> r5=0 -> assert appResourceList.c:207 -> ignore
- //   -> 0x8117BB80 ldr r5,[r5,#4] (badmem 0x4) -> req[0x40]=NULL
- //   -> 0x81181CFA ldr r0,[r3,#0x94] (badmem 0x94) -> stop=3(BADMEM).
- **只忽略某一条已知良性的断言**（`appFontUnitList.c:381`，字体对象引用计数递减），
- 其余保持 ABORT —— 这样既越过良性断言，又能暴露真实错误。
- `P5D_ASSERT_SKIP=81023404`（断言调用点的返回地址）→ **没有匹配**，日志显示仍在
- 断言调用点 0x81023404  ──bl──▶  0x8115F832 (assert helper)
- ──bl──▶  0x8115F9CA
- ──bl──▶  0x8115F7E0  ← 此处的 LR = 0x8115F9D0
- `0x8115F7E0` 内的 `r[14]` 恒为 **`0x8115F9D0`**（＝`0x8115F9CA` 的返回地址），
- **所有断言共用这一个值**，因此用它做键等于全局 IGNORE。
- lr=0x8115F9D0  r0=0xA016DCAC  r1=0x81306D04  r2=0xA016D8AC
- r3=0x812EF018  r4=0xA016DCAC  r5=0x17D
- - **`r3 = 0x812EF018`** = **file 字符串指针**（内容为 `…\appFontUnitList.c`）
- ⇒ 用 `r[3]` 做键可**精确到源文件**。实测 `P5D_ASSERT_SKIP=812EF018` 立即生效。
- [ASSERT-SKIP] #1 lr=0x8115F9D0 (whitelisted)      ← 字体断言被单点跳过
- | 其他断言未被放行 | `ASSERT-ABORT = 0`（12 亿指令内无其他断言失败）✔ |
- 此前（ARCH_42）推测 `0x81181CFA` BADMEM 可能源于"全局 IGNORE 吞掉了 `appResourceList.c:207`
- 的资源表 miss"。**白名单版本证伪了这个推测**：到 12 亿指令为止**不存在任何其他断言失败**。
- ⇒ 该 BADMEM 的成因是**抢占竞态**，与断言无关；已由 `P5D_NO_PREEMPT=1` 消除（ARCH_42/43 已实证）。
- set P5D_ASSERT_SKIP=812EF018   :: 单点放行 appFontUnitList.c 的良性断言
- | ① 定位阻塞条件 | ✅ 完成（两个断言点均到汇编级；`_FDclass` 约定错误已修） |
- **第 (2) 项仍差 ③④。** ③ 的可行起点：ARCH_19 已定位 `0x812071F8`（双次
- `sceKernelGetProcessTimeWide` 推导 deltaTime，实测约 6.8× 过速），

---

## `ARCH_55_HANDOFF.md`

**标题**：ARCH_55 — 会话交接文档（Handoff） ｜ 0. 一句话现状 ｜ 1. 四项验收状态 ｜ 2. 第 (2) 项：已完成的部分 ｜ 2.1 结构（100% 闭合） ｜ 2.2 运行时已到达的阶段（实测） ｜ 3. 运行时推进的**可复现配置**（关键） ｜ 4. 本会话做过的三处关键修复（不要再踩） ｜ 4.1 `_FDclass` 的 ARM RTABI 约定（第一阻塞点的真因） ｜ 4.2 断言白名单（`src/arm/thumb_interpreter.cpp`，`0x8115F7E0` 分支） ｜ 4.3 Ghidra 镜像的 `0xF60` 偏移（影响一切静态地址） ｜ 5. 未完成项与切入点（**下一步从这里开始**） ｜ 5.1 ③ 修时钟过速（约 6.8×）+ 无 vsync ｜ 5.2 ④ 埋帧号探针，产出逐阶段时序 ｜ 5.3 资源格式：段内字段语义 ｜ 5.4 其它未解 ｜ 6. 关键文件路径 ｜ 6.1 二进制 / 工程 ｜ 6.2 索引 / 矩阵 ｜ 6.3 文档（`C:\dev\p5dsrc\`，共 53 份 `ARCH_*.md`） ｜ 6.4 分析脚本（`C:\dev\p5d_build\_*.py`） ｜ 7. 工作方式注意（给下一个会话的提示） ｜ 8. 诚实边界

**发现/结论**（36 条，最多列 40）：

- - **app 状态机 7 态**，状态名**内嵌在表项里**（`0x813B01B4 + state*0x50`，`+0x00` 是 12 字节名字，`+0x44/+0x48/+0x4C` = enter/update/leave）：
- - **状态切换协议** `sequence_81185616`（`kernel/sequence.c`，394 B）：update 返回 1 ⇒ 调 leave ⇒ 状态置 `-1`；支持同步/异步切换
- - **三层调度**：宿主解释器循环 → app 状态机（每帧 `+0x48` update）→ 任务/序列调度器 `0x811DB1F0`
- - **`titleSequence` 的 9 路 TBH**：`tbh [pc,r1,lsl #1]` @ `0x81198184`，表基址 `0x81198188`，case 0..8 → `0x8119819A`…`0x811982A4`
- **运行时实测到的序列创建**（与 ARCH_30 的 141 序列清单逐条对应）：
- `titleSequence` 状态实测走过 **0→1→3→4→5→6→8**（8 = 结束；7 被端口调试开关跳过）。
- set P5D_ASSERT_SKIP=812EF018    :: 只放行 appFontUnitList.c 的良性断言（file 指针键）
- `eboot_probe.cpp` 中 `n_fdclass`（绑定 NID `0xBD8EF217`）原来用 **glibc** 约定：
- 因为调用链是 `断言点 → 0x8115F832 → 0x8115F9CA → 0x8115F7E0`，
- 其 LR 恒为 `0x8115F9D0`（**所有断言共用**），用它等于全局 IGNORE。
- `r[3]` 精确到源文件（`appFontUnitList.c` = `0x812EF018`）。
- SEG0: fo = VA - 0x81000000 + 0x10E0     filesz=0x321810
- SEG1: fo = VA - 0x81325000 + 0x3228F0   filesz=0x21BA74
- 修正后字符串引用数 2,104 → **14,700**（7×），可归属函数 78 → **2,541**（32×）。
- - 游戏侧计时点 **`0x812071F8`**：连续两次 `sceKernelGetProcessTimeWide` 求差得 deltaTime
- - 无 vsync 证据：`DISPCALL count: 0`、`SETFB count: 0`（ARCH_19）
- - 实测 6.8× 过速（历史另有 ~2.4× 的记录，说明漂移随阶段变化）
- 3. **不要硬编码 1/6.8**——先扫几档（如 0.15 / 0.30 / 0.50），用实测帧率反推正确系数
- 4. 恢复 vsync：让帧提交阻塞到显示刷新（或至少把 `[DISPCALL]`/`[SETFB]` 计数器打出来确认调用路径）
- - **app 状态切换**：`sequence_81185616`（`0x81185616`，`kernel/sequence.c`）——在 enter/leave 处打印
- - **每帧**：`0x811DB1F0`（任务调度器）或主循环处递增计数器
- **探针输出格式建议**：
- | LOGO | 实测 2810–2960 | | | |
- > 唯一已知的实测帧号是 **LOGO 段 fc 2810–2960**（来自截图序列 `_h1152_sheet.png`）。
- - **已确证**：名字条目 `[u8 len][name]`（5 处独立验证）；camera 段 0 = 201 B 节点头 + **6 float 包围盒**（与运行时 `+0x58..0x6C` 对应）
- - **顶点 stride 已确证**：`m001_001.GMD` = **44 字节**（446/568/307 行）、`pc200_00.GMD` = **64 字节**（315/639/193 行）
- - **最干净样本**：`m001_001.GMD` 的块 `@0x001D85`（19,656 B = 446 × 44，前 5 个 u32 slot float 率恰好 1.00）
- - **切块判据**（重要）：`.gmd` 段体 = 「名字表 + 数据块」交替，按**名字条目间 ≥4096 字节的间隔**切块，**切块后再做 stride 扫描**（不切块直接扫必然失败）
- | `.env` / `.epl` | 已排除段链结构；段标识为 `01 10 51 00` / `01 10 50 70`，布局待解 |
- - `ARCH_34_CORRECTIONS.md` — `0xF60` 偏移修正（**必读**）
- - `ARCH_38` — app 状态机 7 态实证
- - `ARCH_43` — 到达 TITLE 的完整证据
- - `ARCH_53` — 顶点 stride
- - `ARCH_54` — 断言白名单
- 可复现的分析脚本，如 `_reh_chain.py`（段链验证）、`_req_blockstride.py`（切块+stride）、
- - **资源段内字段语义**未验（只有 stride）

---

## `ARCH_56_STAGE_TIMELINE.md`

**标题**：ARCH_56 — 逐阶段时序（验收项 ② 的收尾）与时钟过速根因 ｜ 0. 一句话结论 ｜ 1. 验收项 (2) 的补全：逐阶段时序 ｜ 1.1 探针设计（两条独立通路） ｜ 1.2 实测阶段表 ｜ 1.3 帧推进与引擎吞吐 ｜ 2. 时钟过速：量化根因与修正 ｜ 2.1 根因 ｜ 2.2 修正（已实施，向后兼容） ｜ 2.3 验证结果（同一配置，仅换时钟源） ｜ 2.4 附带发现：游戏侧时间消费点 ｜ 2.5 关于 vsync ｜ 3. 状态机变量：地址修正 ｜ 3.1 状态切换协议（Ghidra 反编译，`0x81185616`） ｜ 3.2 状态表实测复刻（与 handoff 一致，索引已校正） ｜ 3.3 已证伪的地址 ｜ 4. TITLE 卡点：实证真因 ｜ 4.1 直接证据（完整取证） ｜ 4.2 因果链 ｜ 4.3 已排除的错误归因（重要修正） ｜ 4.4 与时钟的关系 ｜ 4.5 下一步切入点（给后续会话） ｜ 5. 工具与复现 ｜ 5.1 构建

**发现/结论**（63 条，最多列 40）：

- > **本文件新增**：验收项 (2) 的最后一块——**逐阶段时序表（含引擎帧号与耗时）**；时钟过速的**量化根因**与**已实施修正**；TITLE 卡点的**实证真因**。
- 1. **过速 6.8× 已定位并修正**：guest 时钟按**调用次数**推进（每次固定 +16666 µs），而游戏每帧采样 **5.83 次** ⇒ 引擎时间 = 真实时间 × 6.46。新增 `P5D_TICKWALL=1` 改用墙钟源后，实测 **guest/wall = 1.00×**（454.9 s vs 454.9 s）。
- 3. **状态机变量地址修正**：handoff 记的 `0x813658E8` 是误判；真值经 Ghidra 反编译确证为 `*(u32*)(FUN_8116f8a8(DAT_813b01a8) + 4)`。
- 4. **TITLE 卡点真因**：`appResourceList.c:207` 断言（资源字典查找未命中）⇒ 35 个加载请求**永久 pending** ⇒ 标题场景资源永远不上传。
- | `[SEQCHG]` / `[STAGE]` | `src/arm/thumb_interpreter.cpp`，每指令一次 `cur` 比较，命中 `0x81185616`（状态切换协议入口） | `r0` = 目标状态槽位；由 `p5d_stg_note()` 记录帧号/墙钟/指令数 | 1 次比较/指令 |
- （下一条 guest VFP 指令即 `[UNKVFP2] pc=0x81258146` 终止）。
- **结论**：吞吐随渲染段开启从 41 MIPS 掉到 ~2.9 MIPS（**14×**），这是弱机/主循环段的真瓶颈；
- 与时钟无关（时钟修正前后帧推进速率一致）。
- 时钟推进量是**调用次数**的函数，与真实时间无关。实测采样：
- | 指标 | 实测值 | 含义 |
- ⇒ 比值 ≈ 6.5，与实测 6.46 吻合。**这就是历史记录里 6.8× / 2.4× 漂移的来源**
- | calls | frame | calls/frame | wall/frame | guestTick | 实测 wall | **guest/wall** |
- [CLOCKRESET] n=1 clock=0xB34D0CF8 lr=0x812ACD94
- [CONSUME] n=1 clock=0xB34D0CF8 sel=1 n(r3)=... 前[+0x10]=0x0 [+0x14]=0x2800 lr=0x812ACD9E
- [CLOCKOBJ] n=1 obj=0xB34D4768 vtable=0x815409D0 method[+0x14]=0x812AD947 lr=0x812A51E0
- - `0x812ACD94` = 游戏重置/读取其 CRI 播放时钟的位置；
- 所以它同样是从**进程时钟**派生的，现已被 `P5D_TICKWALL` 一并纠正。
- - 但**实测帧率仅 10~16 fps**（远低于 60 Hz），`MAILBOX`/`FIFO` 都不会成为节流项：
- - **结论**：不需要为「过速」做 vsync 修复；过速完全由 §2.1 的假时钟解释。
- *(undefined4 *)(iVar2 + 4) = 0xffffffff;            // 状态 <- -1
- | `DAT_813b01ac` | `0x813B01B4` | 状态表基址（全镜像**唯一**构造点 `0x811858BE`） |
- | 状态索引 | `*(u32*)(FUN_8116f8a8(0x813B01A8) + 4)` | 当前状态，`-1` = 无 |
- state 0  va=0x813B01B4  name='initread'   enter=0x8118591F update=0x81185927 leave=0x81185943
- state 1  va=0x813B0204  name='FREE MODE'  enter=0x81185A0D update=0x81185A15 leave=0x81185A27
- state 2  va=0x813B0254  name='COMMU'      enter=0x810C84D9 update=0x810C8567 leave=0x810C8583
- state 3  va=0x813B02A4  name='TUTORIAL'   enter=0x81185A51 update=0x81185A59 leave=0x81185A6B
- state 4  va=0x813B02F4  name='MAIN MENU'  enter=0x811859C1 update=0x811859D3 leave=0x811859E5
- state 5  va=0x813B0344  name='TITLE'      enter=0x81185975 update=0x81185987 leave=0x81185999
- state -1 va=0x813B0164  name='DEBUG MENU' enter=0x811858CF update=0x811858D1 leave=0x811858ED
- - `enter/update/leave` 的**最低位恒为 1**，证明是 Thumb 函数指针（`0x8118591E|1`），
- | 地址 | 事实 | 结论 |
- | `0x813658E8` | 初值 `0x1000`，运行时**恒不变** | **不是**状态变量（handoff 误判） |
- | `0x813B0160` | 初值 0，运行时恒为 0 | 也不是（不是对象 +0x00 字段） |
- [LRQREQ#1] fn=0x8117C0EE mode=0xF8 owner=0xB1B1A030 nm='DRESS_FORMAT' grp='data/dressFormat.bin'
- [LRQPTR#1] req=0xB200202C f4=0x0 nm='DRESS_FORMAT'      ← +4 bit0 = 0 ⇒ 未完成
- [LRQDUMP q=10000] mgr=0xB49C7030 busy=0 n=35 PENDING:
- **(b) 请求全表（本轮实测，`mode` 为低字节）**
- 它们**没有一条完成** ⇒ 标题永远画不出来，主菜单资源也永远不到位。
- TITLE enter (0x81185975)
- → update 0x81185987 轮询 titleSequence 节点

---

## `ARCH_57_GAP_ANALYSIS.md`

**标题**：ARCH_57 — 三个覆盖缺口的最终查明 ｜ 0. 结论速览 ｜ 1. 缺口 1：SEG1 到底是什么 ｜ 1.1 此前的误判与它的来源 ｜ 1.2 五条独立证据（判定：**数据**） ｜ 1.3 SEG1 的实际布局（2,158.6 KB） ｜ 1.4 一个仍需注意的隐患 ｜ 2. 缺口 2：SEG0 尾部与 ARM 态 ｜ 2.1 SEG0 尾部 `0x812B2AF0 .. 0x81321810` = **元数据区**（确认） ｜ 2.2 ARM 态代码：**存在但极少** ｜ 3. 缺口 3：导入实现率 **53.6%** ｜ 3.1 `import_matrix.tsv` 作废（本轮新发现） ｜ 3.2 从 eboot.bin 重建（可信） ｜ 3.3 库级缺失分布（**对移植排期最重要的一张表**） ｜ 3.4 读法（诚实评估） ｜ 4. 对原结论的修正 ｜ 5. 脚本与产物 ｜ 6. 诚实边界

**发现/结论**（59 条，最多列 40）：

- > **任务**：把 (1) SEG1 代码簇、(2) SEG0 尾部与 ARM 态、(3) 导入 NID 实现率 三个缺口全部查明白。
- > **结论**：三个缺口**全部闭合**，其中 **(1) 的结论推翻了此前的判定**——SEG1 不是代码。
- > UTF-8。所有数字均为实测。脚本见 §5。
- | **(1) SEG1 内 8,767 个"代码簇"** | 最大盲区，疑似未分析的真代码 | **不是代码，是数据段**。游戏镜像只有 SEG0 一个代码段 | 运行时 0 命中 + 熵 7.92 + 指令合法性 59% + PUSH 后无函数体，**5 条独立证据** |
- | **(2) SEG0 尾部 13.8% 未反编译** | 疑似数据区（低风险） | **确认是元数据区**（SCE 模块信息 + 导入表 + ARM 存根 + 字符串池） | 结构定位到具体字节 |
- | **(3) 导入实现率** | 不可信（矩阵与绑定交集为 0） | **53.6%**（403 声明 / 216 已绑定）；并给出**库级缺失分布** | 从 eboot.bin 重建 NID 表，33 段与 34 库名一一对应 |
- 随机数据命中率约 1/64 ≈ 1.56% —— **与实测的 1.46% 几乎完全一致**。
- 同理 `PUSH16`（`1011 0100 rrrrrrrr`，1/64）在 960 KB 里期望命中 ≈ 2,400 次，实测 2,461 次。
- **(a) 运行时执行 0 次 —— 最强证据**
- | SEG1 `0x81325000..` | **3.28** | 结构化数据表 |
- | **SEG1 `0x813E5000..`（所谓"代码区"）** | **7.92** | **压缩/打包数据** |
- | SEG1 `0x814D5000..` | **2.94** | 结构化数据表 |
- | SEG1 `0x813E5000..` | 2,461 | 1,452 | **59.0%** |
- 0x813E5540: B5B2 5108 B65A 352D CE92 7DC9 9A2D 1CD9 0E22 0AA8 2082 8550 ...   (240 B)
- 0x813E573C: B554 AD79 EDC3 EDD3 1ED3 E579 E4CE 15E0 7111 1A52 9555 ECEB ...   (242 B)
- 0x813E5FAC: B5E2 9E79 1567 8E6C B8E3 AE21 AA8E 4A4E 82F2 69AD 569A 538E ...   (144 B)
- | `0x81325000 .. 0x813E5000` | 768.0 KB | 35.6% | 数据（字符串 27–70%、指针表 29–59%，熵 3.28） |
- | `0x813E5000 .. 0x814D5000` | 960.0 KB | 44.5% | 数据（**高熵 7.92**，疑似压缩/打包块） |
- | `0x814D5000 .. 0x81540A74` | 430.6 KB | 19.9% | 数据（字符串 79–100%，熵 2.94） |
- （保留作为反例，**不要**按它做反汇编）。
- 静态上 SEG1 是**可达的**：SEG0 中有 **163 个 `BL/BLX` 目标**落在 `0x813E5000..0x814D5000`，
- | SCE 模块信息（含模块名 `xrd757_psp2_p5d_master`） | 0x2B3BD0 | `0x812B2AF0` 起 |
- | 导入描述符表（0x34 步距，34 条） | 0x2B3C70 | `0x812B2B90` |
- | 库名池（`SceAppMgrUser` … `SceThreadmgrCoredumpTime`） | 0x2B4398 | `0x812B32B8` |
- | **函数 NID 表**（升序 / 每库一段） | 0x2B4560 .. 0x2B5230 | `0x812B348C` .. |
- | ARM 导入存根（`E3E00000 E12FFF1E` = `mvn r0,#0; bx lr`） | 0x2B3B30 起 | `0x812B2A50` 起 |
- **`e_entry = 0x002B2B00` 是文件偏移**，对应 VA `0x812B2B00` —— 即 SCE 模块信息结构体本身。
- | 其余（NID/元数据/重定位引用） | — | — |
- **结论**：尾部**不是**代码，是导入/模块元数据 + 字符串池。此前"低风险"的排除是对的，
- | 模块入口低位 | `0x2B2B00 & 1 = 0` ⇒ 入口是 **Thumb**（但见下面的存根） |
- - 有 **164 个真正成小簇的 ARM 区域**（如 `0x81008884..0x81008E48` 4 个序言、`0x81013660..0x81013C94` 4 个），
- **未真正反汇编验证**（本轮未做 ARM 模式反编译）。
- | `import_matrix.tsv` 行数 / 不同 NID | 381 / 170 |
- | `stub_vaddr` 列 | **全为 `0x00000000`** |
- | 已确认绑定且运行中被调用的 `0xB110C123` 是否在其中 | **否** |
- ⇒ 该矩阵**不是**可用的覆盖面判据，**此前基于它的结论全部作废**。
- 段表 4 项：2×`PT_LOAD`（代码 `0x81000000` / 数据 `0x81325000`）+ 2×`PT_SCE_RELA`。
- NID 表按「严格升序段」切分，每段对应一个库；33 段与 34 个库名**顺序一一对应**。
- | 游戏声明（不同函数 NID） | **403** |
- | **已实现（交集）** | **216 ⇒ 53.6%** |

---

## `ARCH_58_TASKS.md`

**标题**：ARCH_58 — 三项关键任务进展（任务 3 完成 / 任务 1 诊断中 / 任务 2 待启） ｜ 1. 任务 3：C++ 运行库导入 —— ✅ **完成** ｜ 1.1 关键突破：找到了 NID → 函数名的完整数据库 ｜ 1.2 补齐的 60 个函数 ｜ 1.3 实现要点（ABI） ｜ 1.4 实测验证 ｜ 2. 任务 1：TITLE 加载僵死 —— 🔍 诊断取得关键进展（含修正） ｜ 2.1 已解析 titleSequence 状态机全貌（Ghidra） ｜ 2.2 关键实测数据 ｜ 2.3 **修正**：此前的两处判断需要更新 ｜ 2.4 新的、更精确的卡点图景（**已定位到具体机制**） ｜ 2.6 **更正**：前两轮"TITLE 的 update 从未被调用"是**探针假象**（ARCH_60 实测推翻） ｜ 2.7 结论与修复方向（ARCH_60 定位） ｜ 2.8 **修复已实施并实测生效**（ARCH_61）—— TITLE 僵死解除 ｜ 2.10 **任务 1 达成：app 推进到 MAIN MENU**（ARCH_62） ｜ (a) 关键修正：状态表索引是 **0-based，0 = DEBUG MENU** ｜ (b) 实测结果（`run_h63_mm.txt`） ｜ (c) 诚实边界：进入方式与后续观察 ｜ 2.11 附：修复前的现象与语义分歧（保留为排查记录） ｜ 3. 任务 2：资源解析 C++ 化 —— 🟢 已落地首版并测得真实规则 ｜ 3.1 已交付 ｜ 3.2 本轮测出的**真实规则**（修正了 Python 文档里的两处） ｜ 3.6 **接入尝试与架构发现**（ARCH_65）：HLE 侧根本接触不到资源文件 ｜ 3.5 **顶点/索引字段语义已钉死**（ARCH_64）—— 改用运行时权威数据，放弃静态猜测

**发现/结论**（119 条，最多列 40）：

- > **本轮结果**：**任务 3 完成并实测通过**；任务 1 取得关键诊断突破（含一次**修正**）；任务 2 待启。
- > UTF-8。所有数字实测。
- 在**项目内**发现了 vita-headers 的 NID 数据库：
- C:\dev\p5dsrc\refs\vita-headers\db\360\*.yml     (162 个文件，9,326 个 NID 映射)
- 格式为 `functions:` 下的 `函数名: 0xNID`。用它把 60 个缺失 NID **100% 解析出真实函数名**
- **实测有效的约定**：PSVita 用户态为**软浮点 ABI** —— `float`/`double` 参数经 `r0-r3` 传入、
- | 游戏声明 NID | 403 | 403 |
- 0x81198172  titleSequence update 函数（286 字节）
- switch (*puVar1) {              // tbh 跳转表 @0x81198184
- | `[TSEQ-OBJ]`（地址 `0xA002B3E0`）的 `words[0]` | **41 次采样全部 = 2**（帧 119 → 4919 不变） |
- | `[TSEQ-UPD]`（`0x81198172` 入口） | **0 次触发** —— 该函数**从未被调用** |
- | 此前结论 | 修正 |
- | "`0xA002B3E0` 是 titleSequence 对象" | 该对象 `words[0]` 恒为 2 且 update 从未被调用 ⇒ 它要么**不是** titleSequence，要么其 update 由别的入口驱动。**该地址的归属需重新确认** |
- **核心矛盾（实测）**：
- | titleSequence 对象 | `0xB2E4B01C`（不是 `0xA002B3E0`）；`[TSEQ8]` 的 `lr=0x8119817A` | 状态机确实在执行 |
- | **app 状态机的 update 调用** | `[ST-UPD-CALL]` **48 次全部是 state=1（initread）** | **TITLE(state=6) 的 update `0x81185986` 一次都没被调用** |
- | app 状态机离开点 | `[TITLE-LEAVE]`(0x81185878) **0 次**；`[ST-END]` **0 次** | app 从未离开 TITLE |
- | `0x811980FC`（TITLE 的判定包装） | `[TSEQ9]` **0 次触发** | 它从未被调用 |
- line 34679  [ST-ENTER] state=6 enterFn=0x81185975  ← TITLE enter 执行
- line 34686  [TSEQ8] state=0  ← titleSequence 开始跑（由 0x8119817A 驱动）
- **结论**：卡点在**调度层** —— app 状态机在 `TITLE` 的 enter 执行之后，
- **再也没有调用过 TITLE 的 update**。而 `0x81185616`（`sequence.c` 的状态协议函数，
- **修法（假设，待验证）**：让 TITLE 的 update（`0x81185986` → `0x811980FC`）每帧被调用。
- 届时它会看到 titleSequence 已结束（`FUN_8116F900` 返回 0）并返回 1 ⇒
- `0x81185616` 走 `+0x4c` leave ⇒ 状态索引置 -1 ⇒ app 切换到 MAIN MENU。
- ARCH_60 反汇编并埋点后，卡点的**真实机制**如下。
- **(a) 每帧驱动器 = `0x811857C2`（`sequence_811857c2`，192 B）**
- if ((*puVar1 & 1) == 0) {         // 同步模式  → 0x81185838
- iVar3 = (**(code**)(base + uVar4*0x50 + 0x48))();   // 0x8118586E  blx  ← 每帧 update
- if (iVar3 != 1) { puVar1[1] = 0xffffffff; ... }     // 0x81185878  ← 状态置 -1
- **(b) 实测（`[DISPATCH]` 探针，读 flags/countdown/stateIdx）**
- | 86 | **6** | **0x1** | **1** | **5** | TITLE 异步切换（`r3=5` ⇒ 异步） |
- ⇒ 驱动器**每帧都在跑**，且 `stateIdx=6` 时**已处于同步模式** ⇒ **它每帧都在调用 TITLE 的 update**。
- **(c) 前两轮结论错在哪里**
- `[ST-UPD-CALL]`（`0x8118586E`）的打印条件是 **`if(__up++<48)`** —— 前 48 次。
- `[TITLE-LEAVE]`（`0x81185878`，打印上限 6 次）实测 **0 次** ⇒
- TITLE 的 update = `0x81185986`：
- bool FUN_81185986(void) { return FUN_811980fc() != 0; }      // 非 0 ⇒ 返回 1 ⇒ 继续等
- 而 `0x811980FC`（已反编译）= **`FUN_8116F900("title sequence", 0)`** ——
- ⇒ **只要 `title sequence` 的节点还在链表里，`FUN_8116F900` 就返回非 0**，

---

## `ARCH_5_media.md`

**标题**：ARCH_5_media — 模块：音频与视频（SceNgs / SceAudio / SceVideodecUser / CRI Sofdec2 USM） ｜ 1. 模块职责（一句话） ｜ 2. 关键函数地址表 ｜ 2.1 应用层影片任务状态机（appMovie） ｜ 2.2 CRI 播放器状态与显示门 ｜ 2.3 CRI 内部状态链（三层状态码） ｜ 2.4 解码任务链（AU 环 → H.264 解码 → 时钟） ｜ 2.5 HLE 侧桩地址（`SceVideodecUser`） ｜ 3. 关键数据结构 ｜ 3.1 应用层影片任务对象（大小 0xC8，`appMovieTask`） ｜ 3.2 应用层播放器对象（`ply=0xB477B02C`，CRI 侧对象 `0x815538C0`） ｜ 3.3 CRI 对象 `0xB477F028`（`player+8`） ｜ 3.4 生产者对象 `p = 0xB4780170`（「CHAIN」直读） ｜ 3.5 解码上下文 `ctx`（`0xB481B750`，即在 `[cri+0x30]` 链条之下） ｜ 3.6 CRI 时钟对象（`clock`，可能存在多个实例） ｜ 3.7 `SceAvcdecPicture` / `SceAvcdecFrame`（`pp = 0xB1B12F20`，HLE 唯一可写的解码输出对象） ｜ 3.8 已解码帧记录 `rec`（PTS 通路） ｜ 3.9 NGS / 音频相关 ｜ 4. 生命周期与调用关系 ｜ 4.1 影片相位总链（从 title sequence 到「播完」） ｜ 4.2 内层三层状态码的实际演化（关键实测） ｜ 4.3 影片解码链（数据流） ｜ 4.4 音频链 ｜ 4.5 末期真实状态（本次运行的最终结局）

**发现/结论**（289 条，最多列 40）：

- - 目标二进制：`C:\dev\p5d_build\eboot.bin`（FSELF，内嵌 ELF @0x1000，加载基址 `0x81000000`，
- segment `0x81000000..0x816CF4A0`，imports=410 / exports=1）
- 1) `C:\dev\p5d_build\_h1164.log`（最新一次完整运行，147523 行，本次交付的主要运行时证据）
- 2) `C:\dev\p5dsrc\src\arm\thumb_interpreter.cpp`（约 8369 行，含大量 `h145kNNN` 已验证结论注释）
- 6) 反汇编器 `py C:\dev\p5d_build\_p5d_dis.py <VA> <n>`（本轮现场反汇编，下文凡标「反汇编」者均为本轮实测输出）
- - 映射关系：文件偏移 = VA − 0x81000000 + 0x10E0（**注意**：当 VA ≥ 0x83000000 时该线性映射失效，
- > 「证据」列中 `Lxxxx` 指 `_h1164.log` 行号；「反汇编」为 `_p5d_dis.py` 本轮输出原文。
- | VA | 推测名 | 作用 | 证据 |
- | `0x8115849E` | `appMovieTask_update` | 影片任务 updateFn（大小 0xC8）。`st=0` 等子场景→开播；`st=1` 轮询播放器；`st=2` 回调结束 | 反汇编：`8115849E push {r4,r5,r6,r7,lr}` / `811584BA ldr r1,[r4,#4]` / `811584CA ble #0x81158532`（st≤1）/ `811584CE ble.w #0x8115869A`（st≤2
- | `0x811588F6` | `openMovie(player, name, 0)` | **打开影片文件**（`[MOVIEOPEN]` 探针点），`r1` = 片名字符串指针 | L126474 `[MOVIEOPEN] #1 player=0xB477B02C name='movie/c_p5d000.usm' r2=0x0` |
- | `0x81158FA0` | `appMoviePlayer_QueryState`（内层） | 应用层状态查询：`cri = 0x8115423C(player[8])` → `tbb` 映射为 1..6 | 反汇编：`81158FBC ldr r1,[r4,#4]` / `81158FCA bl #0x8115466a` / `81158FCE ldr r0,[r4,#8]` / `81158FD0 bl #0x8115423c` / `81158FD4 adds r
- | `0x81158FD4` | cri_status 返回点（观测点） | 此处 `r0` = `0x8115423C` 的原始返回 | L126517 `[CRIST] #1 0x8115423C(criPlayer)=1` |
- | `0x8115900C` | `0x81158FA0` 返回点（观测点） | `[APPST]` 实测恒 `1` | L126518 `[APPST] #1 0x81158FA0 返回=1` |
- | `0x811589E8` | `appMoviePlayer_Poll`（外层） | 再按 `tbb` 分派内层返回值；其返回 4 才让任务机进 `st=2` | 反汇编：`81158A04 ldr r0,[r4,#0x70]` / `81158A20 bl #0x81158fa0` / `81158A24 cmp r0,#5` / `81158A26 bhi #0x81158a60`。注释 `thumb_interpreter.cpp:5330-5331` |
- | `0x81158A60` | 外层共用出口 | `pop {r4,pc}`；**所有分支汇合点**，被用作强制「影片播完」的注入点 | 注释 `thumb_interpreter.cpp:5279-5284` |
- | `0x81158A24` / `0x81158A60` | 内层/外层返回观测点 | `[INNRET]` / `[OUTRET]` | L126519 `[INNRET] n=1 innerRet(r0)=0x1`；L126520 `[OUTRET] n=1 outerRet(r0)=0x1 (0=等 2/4=推进)` |
- | `0x81158538` | 任务机读取轮询结果处 | `[MOVIEPOLL]` 探针点 | L126522 `[MOVIEPOLL] n=1 status=0x1` |
- | VA | 推测名 | 作用 | 证据 |
- | `0x8115423C` | `criPlayer_GetAppStatus` | 应用可见的 CRI 播放器状态。`[player+4]` 为状态键（0..4），经 `tbh` 分派；`[player+8]` = CRI streamer 指针 | 反汇编：`8115423C push {r4,r5,r6,r7,lr}` / `81154248 ldrsb.w r0,[r6,#0x55c]` / `8115428A ldr r1,[r6,#4]` / `8115428C
- | `0x811542A2` | 状态键 2 分支（state2 handler） | `0x812970F2` 建 streamer、`0x81298208` 设参数、写入 `0x7512`/`0x3E8` 常量 | 反汇编：`811542A6 bl #0x812970f2` / `811542AA str r0,[r6,#8]` / `811542B4 movw r1,#0x7512` / `811542C0 movs.w r2,#0x3e8` |
- | `0x81154002` | CRI 应用状态机 state2 推进器 | 内部以 `cmp r1,#6` → `'PLAY END\n'`、`cmp r1,#7` → `'MANA STATUS ERROR\n'` 上报 | 注释 `thumb_interpreter.cpp:5362-5368`（含字符串 VA `0x813060E4` / `0x813060F0`） |
- | `0x8115402A` | 每帧把影片时间交给 CRI | `[MOVTIME]` 探针点，读 `[r5+0x3E0]/[r5+0x3E4]/[r5+0x3E8]/[r5+0x3EC]` | L130512 `[MOVTIME] n=15 lo[r5+0x3E0]=0x3E8 hi[r5+0x3E4]=0x0 v1[r5+0x3E8]=0x7512 v2[r5+0x3EC]=0x0` |
- | `0x811540BA` | 取当前视频帧返回点 | `[FRAMEAVAIL]`：返回 0 ⇒ 本帧被跳过 | L127148 `[FRAMEAVAIL] #3 0x81297BFE 返回=1 (有帧)`；L126665 `#1 返回=0` |
- | `0x81154170` | 帧闸判据（`cmp r0,#1`） | `0x81298838` 返回 1 才继续，否则跳过 `+0x50C=1` | L126672 `[GATE838] #1 0x81298838 返回=0 (闸没过 ⇒ 跳过 0x50C=1)` |
- | `0x8115420E` | `strb.w r4(1),[r5,#0x50c]` | **一旦执行，影片四边形即可开始提交**（关键里程碑） | 注释 `thumb_interpreter.cpp:5470`；本次运行 `[F50C-SET]` **0 命中**（`_h1164.log` 全文无该标签） |
- | `0x811543F4` | 显示入口 | 读 `[player+0x50C]` / `[player+0x54C]` | 注释 `thumb_interpreter.cpp:5480-5483` |
- | `0x81154578` / `0x8115457C` | CRI 执行点 A/B | `[CREXEC-A]`/`[CREXEC-B]` 在每帧收敛处 | L131694 附近 `[CREXEC-A] n=26 r0=0x1 lr=0x81154578` / `[CREXEC-B] n=26 r0=0x0 lr=0x8115457C` |
- | VA | 推测名 | 作用 | 证据 |
- | `0x81297100` | CRI 内部状态函数 | `[r0+0x84]==1` ⇒ 直接返回 7（错误）；否则取 `[r0+0x30]` → `0x8129D0A8` → `tbb` 映射为 1/5/6… | 反汇编：`8129711A ldr.w r1,[r0,#0x84]` / `8129711E cmp r1,#1` / `81297120 ldr r0,[r0,#0x30]` / `81297122 bne #0x81297128` / `81297128 
- | `0x8129D0A8` | 内层状态码映射 | `r0 = 0x812A4398([r4+4])`（即 `[[cri+0x30]+4][+4]` = 最内层状态）；`[r4+0x770]==1` ⇒ 返回 7；否则 `(r0-1)` 经 `tbb`（表 @`0x8129D0CC`）映射 | 反汇编：`8129D0B0 ldr r0,[r4,#4]` / `8129D0B2 bl #0x812a4398` / `8129D0B6 ldr.w r1,[r4,#0x770]`
- | `0x812A4398` | `x[+4]` 取字段 thunk | 被 `0x8129D0A8` 调用以取得最内层状态 | 注释 `thumb_interpreter.cpp:5387` |
- | `0x8129C6D8` | **节拍/时间判据回调** | 被 `[cri[0x30]+0x6AC]` 虚表项调用；返回 1 才让 `0x8129896C` 返回 1 | 注释 `thumb_interpreter.cpp:5531-5538`（setter `0x8129C6C8`，由 `0x8129CA18 / 0x8129CC94` 写入） |
- | `0x8129C93C` | 回调内 `vsub.f32 s2,s1,s0` | **NaN 产生点**；由于 `d16 = out+0x28 = 0`，`0/0 → NaN`，后续 `bge` 因无序比较永不成立 | L129980 `[MS] #1 r8=0x7512 s0=-nan s1=257.5684 s2=4294967296.0000 ...`；注释 `thumb_interpreter.cpp:5580-5587` |
- | `0x8129C9EE` | 回调返回点 | `[CB-RET]` 实测**恒 0** | L129983 `[CB-RET] #1 回调返回=0 (返回1: 0/1)` … L130581 `#4 回调返回=0 (返回1: 0/4)` |
- | VA | 推测名 | 作用 | 证据 |
- | `0x812A9FAE` | **decodeLoop 入口** | `push.w {r4-r8,sb,lr}`；函数指针存于 `0x8153CFCC`（CRI 解码器对虚表项），故为间接调用 | 反汇编 `0x812A9FE8` 前一段；注释 `thumb_interpreter.cpp:3971-3974`。L131694 附近 `[AUENT] n=21 obj=0xB481B750 lr=0x812A8370 r1=0x812A9FAF …` |
- | `0x812A9FE4` | `bl 0x812ACDB4`（vtable+0x14 虚调用） | 在 `[r8+0x34]`（AU 环）上取「剩余可读量」 | 注释 `thumb_interpreter.cpp:4168-4179`。L131694 附近 `[GATE] n=21 obj=0xB483B7C8 vt=0x815409E8 m(vt+14)=0x812AD947 | +8=… +10=0x0 +14=0xB483B898 …` |
- | `0x812A9FE8` | **解码 gate** | `cmp r0,#0` / `beq.w 0x812AA1C2` | 反汇编原文：`812A9FE8 cmp r0,#0` / `812A9FEA beq.w #0x812aa1c2`。L131695 `[GATERET] n=21 r0=0 (0 ⇒ 门失败, 直接跳到 0x812AA1C2)` |
- | `0x812A9FEE`..`0x812AA000` | 入口其余判据 | `[r8+0x14]!=1 → 0x812AA0DC`；`[r8+0x4C]!=[r8+0x48] → 0x812AA0DC` | 反汇编：`812A9FEE ldr.w r0,[r8,#0x14]` / `812A9FF2 cmp r0,#1` / `812A9FF4 bne #0x812aa0dc` / `812A9FF6 ldr.w r0,[r8,#0x48]` / `812A9FFA ld
- | `0x812AA00E` | `bl 0x812ACD96` | "pop vtbl+8"：从时钟取出一个 AU 数据块 | 反汇编：`812AA002 ldr.w r0,[r8,#0x34]` / `812AA00E bl #0x812acd96`。注释 `thumb_interpreter.cpp:4005-4007` |

---

## `ARCH_66_NATIVE_REWRITE_START.md`

**标题**：ARCH_66 — 原生重写启动：资源加载链与资源管理器结构 ｜ 1. 已打通的`.gmd` 加载调用链（全部反编译确证） ｜ 1.1 `danceResource_810479d8(sys, path)` — 资源管理器（`dance/danceResource.c`，344 B） ｜ 1.2 `FUN_8104a024(str)` — 路径哈希（32 B） ｜ 2. 一个必须记录的技术澄清：**没有独立的 float32→S16N 转换层** ｜ 3. `.gmd` 的段结构（上一轮已得，此处汇总） ｜ 4. ARCH_67 追加：`.gmd` 块解析框架（已定位） ｜ 4.1 从"资源句柄"到"块解析器"的完整控制流 ｜ 4.2 块解析器 `FUN_811B7686`（1504 B）的核心逻辑 ｜ 4.3 关键函数清单（原生重写待重实现） ｜ 4.5 ARCH_68 追加：`FUN_811c7094` 的真实作用与一个新矛盾 ｜ 新发现的矛盾（下一步必须澄清） ｜ 4.6 ARCH_69：段类型字典 + 网格段数据形态（本轮突破） ｜ (a) 段类型字典（扫描 400 个 `.gmd`/`.gfs` 文件） ｜ (b) 网格段的数据形态（决定性） ｜ (c) 新发现：该格式**混用字节序** ｜ (d) 仍未解决 ｜ (e) 运行时验证受阻（记录） ｜ 4.7 ARCH_70：网格段结构探测——遇到瓶颈（诚实记录） ｜ (a) float 合法率剖面：**没有纯 float 区** ｜ (b) u16 递增序列探测：**为空** ｜ (c) 块头协议仍未解 ｜ (d) 为什么会卡在这里（诚实分析） ｜ (e) 本轮的排除性成果（有价值的部分）

**发现/结论**（1062 条，最多列 40）：

- > **本轮性质**：纯代码分析（阶段 0 的 Vita3K 对照基线经用户确认暂缓）。
- │     ├─ obj[0x04] = danceResource_810479d8(danceSystem, path)   ★ 真正加载
- for (i = 0; i < count; i++) {                    // 遍历资源表   e = table + i*0x34;                            // ★ 表项固定 0x34 (52) 字节
- e[0x14] = (i & 0xFFFF) | (g_counter << 16);  // ★ 句柄 = 槽索引 | 计数器<<16（带版本）
- FUN_810424be(&sys[3], e, e);                 // ★ 入加载队列（异步）
- **⇒ 可直接在原生代码里复刻**（无符号 32 位回绕）。
- 先前分析（`VERTEX_FORMAT.md`）发现 GPU 侧位置格式是 `S16N×3`，而 `.gmd`/运行时里的顶点是 float32，
- `eboot_probe.cpp` 的顶点解包代码（行 2400-2405）直接读 guest 内存、按 stride 取字节：
- SceUInt32 _vstride = 16u;
- if (_vh == 0xA71B28370132FF67ull) _vstride = 12u;      // ← 硬编码
- else if (_vh == 0x4A3A4C8DFD6083F6ull) _vstride = 20u;
- **⇒ 转换完全在 guest 侧完成**：guest 把 `.gmd` 解析并组装成 GPU 需要的最终顶点缓冲，
- **另外发现一个应修的问题**：上述 stride 是**硬编码**的（按 vertex program hash 列举），
- 而注释自己也说"真正 stride 应由权威属性表 `g_vfmt[eh].stride[]` 取"。
- | kind | 用途 | 实测样例 |
- **段类型字节不在代码里做魔数比较**（`0x50401001`/`0x90500110` 在全量反编译中 0 命中），
- │  状态 1 = 普通模型(.gmd)  → ★
- ├─ FUN_811b7ec4(dataPtr, size)               ★ 创建异步任务
- │     alloc(0xF0) ; task = { fn=0x811bb575, obj, dataPtr, size }
- │        └─ 任务函数 0x811BB575 (= FUN_811bb574)
- │              └─ FUN_811b7686(obj, *task, 0)   ★★ 真正的块解析器（1504 B）
- if (blkType == 0x10003) {            // ★ 顶点/索引块
- n = FUN_811ded54(ctx);                              // 顶点数         gpu = FUN_811dcfd8(n, 0x40, 0, 0xff000007);         // 创建 GPU 资源
- layout = FUN_811c7094(n);                           // ★ 顶点布局解析
- *puVar1     = FUN_811dcfd8(n, 0x40, 0, 0xff000007);   // [0] 顶点资源， *64 字节对齐**
- puVar1[1]   = FUN_811dcfd8(n, 2,    0, 0xff000007);   // [1] 索引资源， *2 字节对齐**
- **再次独立印证索引宽度**：索引资源用 **2 字节对齐** ⇒?`U16`。
- 这是第三条独立证据（前两条：`VERTEX_FORMAT.md` 的 GXP attribute、块解析器的 `n << 1`）。
- uVar3 = FUN_811dcfd8(iVar1, 0x40, 0, 0xff000007);   // 分配：size = iVar1
- - 若 `iVar1` = **顶点数**，则顶点 stride = **64 字节**
- - 但 `VERTEX_FORMAT.md` 从 GXP 权威表实测的 stride 是 **12–36**（115 个 program，**无一例等于 64**）
- 3. `.gmd` 里的顶点格式与 GPU 最终格式**确实不同**（即转换层存在于"读入 → GPU 资源"之间），
- 64 是**源**格式、12–36 是**目标**格式。
- | **`0x00010003`** | 400 | 69,363 | 6,958,083 | **网格几何**（顶点 索引）—— 与 `FUN_811b7686` 处理的一致 |
- | `0x000100FC` | 274 | 87,573 | 16,604,997 | **纹理/图片资源表 *（名字是 `.dds`） |
- | `0x000100FB` | 274 | 673 | 31,121 | **材质定义** |
- | `0x000100FD` | 204 | 5,832 | 1,493,140 | **节点层级/场景图 * |
- section[0] type=0x000100FC  size=109,592  纹理
- section[1] type=0x000100FB  size=1,682    材质
- section[2] type=0x00010003  size=601,544  网格    ← 顶点/索引在这里

---

## `ARCH_6_scene.md`

**标题**：ARCH_6 — 场景 / 序列 / UI 系统（Persona 5 Dancing in Starlight, PCSH10109） ｜ 1. 模块职责 ｜ 2. 关键函数地址表 ｜ 2.1 序列框架（框架层） ｜ 2.2 返回值语义（**本节是本次分析最重要的修正**） ｜ 2.3 titleSequence（`program/app/title/titleSequence.c`） ｜ 2.4 mainmenu 六条件与就绪谓词 ｜ 2.5 就绪状态写入方与子模块 ｜ 3. 关键数据结构（字段偏移 + 含义） ｜ 3.1 全局锚点 ｜ 3.2 注册节点（registry node，`0x8116F7CA` 的返回值 / `0x8116F900` 的返回值） ｜ 3.3 task 对象（`[node+0x0C]`，实测 `'initDataRead'` → `0xA002A570`） ｜ 3.4 work/状态块（`[node+0x08]`，titleSequence 实测 `0xB2D4F01C`） ｜ 4. 生命周期与调用关系 ｜ 4.1 通用模型（实证） ｜ 4.2 titleSequence 调用链 ｜ 4.3 mainmenu 调用链 ｜ 4.4 实测状态轨迹（对照） ｜ 5. HLE 接口现状 ｜ 5.1 桥的实现方式（与本模块直接相关） ｜ 5.2 对本模块而言的关键结论 ｜ 5.3 已知缺口（实证） ｜ 6. 未解问题清单（按优先级排序） ｜ P0 — 让 `title sequence`／`movie play` 真正收尾（当前唯一卡点的因果源头）

**发现/结论**（186 条，最多列 40）：

- > 逆向依据：`eboot.bin`（ARM32 Thumb，基址 0x81000000，imports=410）反汇编 +
- > 记号约定：**实证**＝有反汇编原文或日志原文；**推测**＝推理所得，未直接取到证据。
- mainmenu 序列再用 **六条件就绪判定**（`0x81184B60`）把这些节点逐一查空，全通过才允许进入主菜单。
- | VA | 推测名 | 作用 | 证据 |
- | `0x8116F7CA` | `seqNodeCreate`（任务/序列创建） | 创建注册节点 + task 对象 + work 块；`r1`=名字指针，`r2`=周期(0xC8)，栈上传 step/cleanup | 反汇编调用点 `0x811980EE ldr r1? / 0x811980F4 bl #0x8116f7ca`；日志 `[TASKCREATE #43] r0=0x0 r1='title sequence' r2=0xC8 r3=0x0 lr=0x811
- | `0x8116F900` | `seqFindByName`（按名查节点） | `hash=0x81165288(name)`；遍历 `[0x815554C0+4]` 链表 `next=[node+0x10]`，比 `[node+0x18]==hash` 后再用 `[node+0xC]+0x14`（名字串）做 `strcmp(0x812B2200)`；**命中返回节点指针 r4，未命中返回 0** | 反汇编：`8116F90A ldr r4,[r1,#4]` / `81
- | `0x8116F8A8` | `seqNodeWork`（取 work/状态块） | **就一条指令**：`ldr r0,[r0,#8]` | 反汇编 `8116F8A8 ldr r0,[r0,#8]` / `8116F8AA bx lr` |
- | `0x8116FA58` | `taskObjName` | `ldr r0,[r0,#0x14]`：取 task 对象的**名字串指针** | 反汇编 `8116FA58 ldr r0,[r0,#0x14]` |
- | `0x8116F748` | `seqInvokeStep`（调用节点的 step） | 置全局当前节点 `[0x815554C0]=node` → `blx node[+0x00]` → 清全局；**返回值即 step 的返回值** | 反汇编 `8116F752 str r0,[r4]`(r4=0x815554C0) / `8116F754 ldr r1,[r0]` / `8116F756 blx r1` / `8116F758 movs r1,#0; str r1,
- | `0x8116FA5C` | 运行表包装（`0x8116FA5D`） | `ldr r0,[r0,#0x34]; bl 0x8116f748`：从 task 对象回到注册节点并调用 step | 反汇编 `8116FA5C push {r4,lr}; 8116FA5E ldr r0,[r0,#0x34]; 8116FA60 bl #0x8116f748` |
- | `0x8116F75E` | `seqNodeDestroy`（销毁节点） | 置当前节点 → `blx node[+0x04]`（**清理函数**）→ 清当前 → 用 `[node+0x10]`(next)/`[node+0x14]` 从 `[0x815554C0+4]` 摘链 | 反汇编 `8116F76A str r4,[r5]` / `8116F76C ldr r1,[r4,#4]; blx r1` / `8116F774 ldr.w lr,[r4,#0x10]`
- | `0x811DAD8C` | `taskFinish`（结束 task 对象） | 被"step 返回 -1"与 `0x8116F8AC` 调用 | 反汇编 `8116FA30 push {r4,lr}; 8116FA32 bl #0x811dad8c`；`811DB270 bl #0x811dad8c` |
- | `0x8116F8AC` | `seqFinishByNode` | `node→[node+0xC]→0x8116FA30→0x811DAD8C`：结束该节点对应的 task 对象 | 反汇编 `8116F8C0 ldr r0,[r0,#0xc]` / `8116F8CC bl #0x8116fa30` |
- | `0x81165288` | `nameHash` | `h = Σ (signed char)name[i] << (i & 7)`（内部两次 `blx 0x812B22E0` = `strlen`） | 反汇编 `811652A2 ldrsb r0,[r4],#1` / `811652A6 ands r1,r6,#7` / `811652AA lsls r0,r1` / `811652AC adds r7,r7,r0`；**手算校验**：`hash('help str
- | `0x811DB1F0` | **每帧调度器循环**（运行表） | `for(obj = [sl]; obj; obj = [obj+0x4C])`，仅当 `obj[+0]==2` 且 `(obj[+4]&1)==0` 时执行 `blx obj[+0x28]`（r0=obj） | 反汇编 `811DB1E6 ldr.w r5,[sl]` / `811DB1F0 ldr r0,[r5]` / `811DB1F2 cmp r0,#2` / `811DB1F6 ldr r0,[
- `0x811DB24E..0x811DB258` 原文：
- 811DB254  beq       #0x811db26e      ; ret == -1  -> 811DB26E bl 0x811dad8c(obj)  = 结束该 task
- 811DB256  cbz       r3, #0x811db25a ; ret == 0   -> 保持 obj[+0x28] 不变，下一帧继续调同一个函数
- ⇒ **step 的返回值约定**：
- - `-1` = "我完成了" → `0x811DAD8C(obj)` 结束该 task 对象；
- > ⚠️ **必须修正的既有结论**：此前记录为
- > "state 8 → `0x811982A4`（`movs r0,#0; pop`）= 序列结束"。
- > 实测该处字节为 `0x811982A4: 00 20 70 BD` = `movs r0,#0; pop {r4,r5,r6,pc}`（**返回 0**），
- > `titleSequence` 真正"结束"的路径是 `0x81198294 / 0x8119829A` 的 `movs.w r0,#-1`（`5F F0 FF 30`）。
- > 因此把 state 强推为 8 **并不会让序列完成**，只会让它永远返回 0 —— 这与"强制跳过 C5/C6 反而更早卡死"的实测一致。
- | VA | 推测名 | 作用 | 证据 |
- | `0x81198172` | `titleSequenceStep`（注册节点 `+0x00`） | `r5 = node`；`r4 = 0x8116F8A8(node) = [node+8] = work`；`r1 = work[0]` = 自身 state | 反汇编 `81198172 push {r4,r5,r6,lr}` / `81198174 adds r5,r0,#0` / `81198176 bl #0x8116f8a8` / `8119817C ldr 
- | `0x81198188` | state 跳转表（**已逐字验证**） | TBH，指令字节 `DF E8 11 F0`（`0xF011` = TBH，Rm=r1）；表 = 9 个 halfword，**实际目标 = 表基址 + 2×halfword** | 表字节 `09 00 2E 00 37 00 46 00 4E 00 57 00 5E 00 71 00 8E 00` ⇒ state0→`0x8119819A`、1→`0x811981E4`、2→`0x811981
- | `0x8119819A` | state 0 | `0x81178CE2(0)`→`0x81178F0A()`；`==0`→尾部保活；`==1`→`0x81178DCE(1,1)`；`work[4]==1`→`work[0]=6`+尾部，否则 `work[0]=1` 并**贯穿进 state 1 体**；末尾调 `0x81172470 / 0x81172374(0) / 0x81160696(0)`（`'COMSE'`/`'BGM'`） | 反汇编 `8119819C b
- | `0x811981E4` | state 1 | 阶段门 `0x81178F0A()`；`work[0]=2`；查 `ldrsb [0x813B1B1C]`（跳过开关）非 0 → `work[0]=6`+尾部；否则 `0x81193344()`→`work[8]`，`work[0]=3`，贯穿进 state 3 体 | 反汇编 `811981F2 movs r0,#2; str r0,[r4]` / `811981F6 movw r0,#0x1b1c; movt r0,#
- | `0x81198214` | state 3 | `r0 = 0x811933B8(work[8])`；`==0`→尾部；否则 `work[0]=4`→尾部 | 反汇编 `81198216 bl #0x811933b8` / `8119821C beq 0x811982a4` / `8119821E movs r0,#4; str r0,[r4]` / `81198222 b 0x811982a4` |
- | `0x81198224` | state 4 | `work[0]=5`；`work[8]=0x811580CC(1)`；`0x811581DC(1)`；贯穿进 state 5 体 | 反汇编 `81198224 movs r0,#5; str r0,[r4]` / `8119822A bl #0x811580cc` / `8119822E str r0,[r4,#8]` / `81198232 bl #0x811581dc` |
- | `0x81198236` | state 5 | `r0 = 0x81158112()`；`!=0`→尾部（**保活等 movie**）；`==0`→`work[0]=6`→尾部 | 反汇编 `81198236 bl #0x81158112` / `8119823C bne 0x811982a4` / `8119823E movs r0,#6; str r0,[r4]` |
- | `0x81198244` | state 6 | `work[0]=7`；`work[8]=0x81193A00(node)`；清 `[0x813B1B1C]`；贯穿进 state 7 体 | 反汇编 `81198244 movs r0,#7; str r0,[r4]` / `81198248 adds r0,r5,#0; bl #0x81193a00` / `8119824E str r0,[r4,#8]` / `81198258 ldrsb.w r0,[r0]` / 
- | `0x8119826A` | state 7 | `r5 = 0x81193A6A(work[8])`；`==0`→尾部保活；否则 `0x81193A80(work[8])`、`work[8]=0`、`r5-=1`、`if(r5>4)`→尾部，再 `tbb [pc,r5]` | 反汇编 `8119826A ldr r0,[r4,#8]` / `8119826C bl #0x81193a6a` / `81198272 beq 0x811982a4` / `81198276 
- | `0x811982A4` | 尾部（保活） | `movs r0,#0; pop {r4,r5,r6,pc}` | 字节 `00 20 70 BD` |
- | `0x81198294 / 0x8119829A` | **序列完成** | `movs.w r0,#-1; b 0x811982a6` | 字节 `5F F0 FF 30 05 E0` / `5F F0 FF 30 02 E0` |
- | `0x811982A8`（注册为 `0x811982A9`） | `titleSequenceCleanup`（节点 `+0x04`） | state6 等任务的清理函数 | 反汇编创建点 `811980D0 movw r1,#0x82a9; 811980DA movt r1,#0x8119; 811980DE strd r1,r4,[sp,#8]`；源码注释 `thumb_interpreter.cpp:6292 "阶段状态机 0x811982A9（清理）"` |
- | `0x81178CE2` | `appPhaseGet`（推测） | 传 0 返回"应用主序列"管理器；随后作为 `0x81178F0A` 的入参 | 反汇编 `8119819A movs r0,#0; 8119819C bl #0x81178ce2; 811981A0 bl #0x81178f0a` |
- | `0x81178F0A` | `appPhaseMap`（阶段门） | `mgr=0x8116F8A8(arg)`；`mgr==0`→返回 1；`r0=mgr[4]`；`>5`→返回 1；否则 `tbb [pc,r0]` 映射 0/1/2 | 反汇编 `81178F0C bl #0x8116f8a8` / `81178F10 cmp r0,#0` / `81178F14 movs r0,#1` / `81178F18 ldr r0,[r0,#4]` / `81178F1A

---

## `ARCH_7_fiber_exc.md`

**标题**：ARCH_7：SceFiber（6 导入）+ 异常/回调机制 + 与宿主调度器的交互 ｜ 0. 本轮最重要的三条结论（先读这一节） ｜ 1. 本子系统职责（一句话） ｜ 2. 导入/关键函数地址表 ｜ 2.1 SceFiber 的 6 个导入（**全部已绑定，无一空桩**） ｜ 2.2 游戏侧（guest）关键函数 ｜ 2.3 异常/断言/回调相关导入 ｜ 2.4 与"异常/回调"相关的缺口 NID（本子系统视角） ｜ 3. 关键数据结构（字段偏移 + 含义） ｜ 3.1 guest fiber 对象（游戏 `malloc` 得到；实测两例 `0xA014D000` / `0xA015E000`） ｜ 3.2 HLE 侧 fiber 状态（`eboot_probe.cpp:181-190`、`5872-5865`） ｜ 3.3 断言/资源相关（BADMEM 链上的对象） ｜ 4. 生命周期与调用关系（调用链） ｜ 4.1 fiber 世界的进入与双 fiber 乒乓（实测完整序列） ｜ 4.2 fiber 与宿主调度器的交互（本子系统最核心的机制） ｜ 4.3 断言链（BADMEM 的完整上游，本 subsystem 的核心发现） ｜ 5. 当前 HLE 实现状态 ｜ 5.1 绑了哪些 / 怎么实现的 ｜ 5.2 fiber 子系统：**无空桩缺口，但有 3 处语义薄弱** ｜ 5.3 缺口清单（按严重度） ｜ 5.4 明确**不存在**的缺口（否定性结论，避免后续重复劳动） ｜ 6. 对「能否走到并打完一局」的影响评估 ｜ 6.1 SceFiber 本身：**次要项（不是阻塞项，也不是可暂缓项）** ｜ 6.2 异常/断言机制：**当前第一阻塞项（P0）**

**发现/结论**（172 条，最多列 40）：

- > 分析对象：`C:\dev\p5d_build\eboot.bin`（ARM32 Thumb，加载基址 `0x81000000`，imports=410）
- > 反汇编器：`py C:\dev\p5d_build\_p5d_dis.py <VA> <字节数>`（**注意第二参数是字节数不是指令条数**）
- > 映射：`file_off = VA − 0x81000000 + 0x10E0`
- > 本轮证据来源（共 12 处，全部逐一核对原文）：
- > 1. `_h1164.log`（最新一次完整运行，147531 行，stop=3(BADMEM)）—— 导入表 / NIDCALL / SVCWIN / NATIVE fiber 轨迹 / 断言链 / 末态报告
- > 2. `_h1164.log` 导入表（第 30 行 `Import lib: SceFiber (libNID=0xF88DAA70, funcs=6, vars=0)`）
- > 3. `C:\dev\p5d_build\veneer_map.txt`（stub VA → libNID/NID 权威对照，SceFiber 六条逐字）
- > 4. `C:\dev\p5d_build\v3k_nids.inc`（NID → 符号名）
- > 5. eboot.bin 反汇编原文（`0x81206Bxx`、`0x812067xx`、`0x81198Bxx`、`0x81198Cxx`、`0x8115F7xx`、`0x8115FDxx`、`0x81181Cxx`、`0x8117BAxx`、`0x8128F6xx`）
- > 7. `C:\dev\p5dsrc\src\arm\thumb_interpreter.cpp`（第 2198-2208 ASSERT-IGNORE、2245-2317 fiber/断言探针、723-734 context_switch 语义）
- > 10. `C:\dev\p5dsrc\ARCH_0_OVERVIEW.md`（勘误表第 5 条：`0x812B4150` 系栈金丝雀）
- > 11. `C:\dev\p5d_build\imports_dump.txt`（确认其为 exe 的 PE 导入表，**与 guest 导入无关**，不作为证据使用）
- > 12. `C:\dev\p5d_build\eboot.bin` 段布局（SEG0 `0x81000000+0x321810`、SEG1 `0x81325000+0x3AA4A0`）
- | # | 结论 | 关键证据 |
- | 1 | **`0x812B4150` 是代码地址，不是"栈金丝雀常量"**。它是 `0x81206BEC` 的下一条指令（`ldr.w r3,[r4,#0x94]` 所在位置），也是启动时主 fiber 栈上保存的 `lr`。金丝雀**指针**是 `0x812B414C`，`[0x812B414C]` 才是金丝雀**值**。 | 反汇编 `81206BEC ldr.w r3,[r4,#0x94]`；日志 `[ROT-RET] sp=0xA015D078 saved=[sp
- | 2 | **BADMEM `0x81181CFA` 的直接上游就是被 HLE 放行的断言 `appResourceList.c:207`**，不是"两件独立的事"。完整因果链在 §5.3 给出（断言判据 → 忽略 → 落空 → 两次 NULL 解引用）。 | 日志 `[ASSERT-SITE] caller=0x8117BB80 r0=0x81307D60 r1=0xCF …` + `[ASSERT-IGNORE] #7` + `[ERROR] ThumbExec bad
- | 3 | **游戏确实使用 SceFiber，而且是"双 fiber 乒乓"结构**（2 次 Init、1 次 Run、6179 次 Switch，恰好 3089 vs 3090 对开），**fiber 世界与宿主单执行器调度目前没有直接冲突**（宿主用 `sched_save/enter` 把线程 0 现场重定向到 fiber）。但"fiber 内让出"经由 `g_fiber_pump` 把时间片交给 OS worker，二者的现场所有权交界是本子系统最大的潜在风险点。 
- 本子系统覆盖 **guest 侧"协程（fiber）+ 断言/栈保护/回调"这一整套"非线程的流程与错误处理设施"**：游戏用 SceFiber 把主逻辑做成两个互相切换的协程（而不是线程），用 `assert` + `__stack_chk_fail` 做错误处理（真机由弹窗/中止接管），用 `sceKernelCheckCallback` / `WaitEventFlagCB` / `DelayThreadCB` 把 SDK 内部回调钩进线程节拍；而宿主侧必须为 fibe
- 来源：`veneer_map.txt`（stub → NID）+ `v3k_nids.inc`（NID → 符号）+ `_h1164.log` 的 `[NIDCALL]`。
- | stub VA | NID | 推测名（= `v3k_nids.inc` 权威名） | 作用 | 实测被调用次数 | HLE 绑定 | 证据 |
- | `0x812B1B60` | `0x3B42921F` | `sceFiberReturnToThread` | 在 fiber 内调用，结束当前 Run，回宿主线程 | **0**（TOTAL_MATCH=0） | `n_fiber_return`（`eboot_probe.cpp:6396`） | `veneer_map.txt: 812B1B60 lib=F88DAA70 nid=3B42921F SceFiber`；`_h1110_callers.py 0x812
- | `0x812B1B70` | `0x414D8CA5` | `sceFiberGetSelf` | 取当前 fiber 对象指针并回填 `*out` | **6179** | `n_fiber_self`（`:6392`） | `[NIDCALL] 6179 impl=Y func=0x414D8CA5 lib=0xF88DAA70 SceFiber/func=0x414D8CA5`；调用点 `812067B2 blx #0x812b1b70` / `81207644 b
- | `0x812B1B80` | `0x7DF23243` | `sceFiberRun` | 宿主线程进入 fiber 世界的唯一非对称入口 | **1** | `n_fiber_run`（`:6395`） | `[NIDCALL] 1 impl=Y func=0x7DF23243`；`[NATIVE] fiberRun ENTER fiber=0xA014D000 NEW argOnRunTo=0x0 retp=0x0` |
- | `0x812B1B90` | `0xE160F844` | `sceFiberFinalize` | 销毁 fiber | **0** | **`n_noop`**（`:6397`，空桩） | `veneer_map.txt` 该行；调用点 `812069D2 blx` / `81206B88 blx`（SDK 内部） |
- | `0x812B1BA0` | `0xE4283144` | `sceFiberSwitch` | 协程切换（保存当前、切到 target） | **6179** | `n_fiber_switch`（`:6394`） | `[NIDCALL] 6179 impl=Y func=0xE4283144`；`[NATIVE] fiberSwitch -> RESUME fiber=0xA014D000 pc=0x812B1BA2 from=0xA015E000 lr=0x812
- | `0x812B1BB0` | `0xF24A298C` | `_sceFiberInitializeImpl` | 用游戏已 malloc 的对象登记一个未启动 fiber | **2** | `n_fiber_init`（`:6393`） | `[NIDCALL] 2 impl=Y func=0xF24A298C`；`8120679C`/`0x812B3D64` 有该 veneer 的字面量引用 |
- `_sceFiberAttachContextAndRun(0x7D0C7DDB)`、`_sceFiberAttachContextAndSwitch(0xE00B9AFE)`、
- `_sceFiberInitializeWithInternalOptionImpl(0xC6A3F9BB)`、`sceFiberGetInfo(0x189599B4)`、
- `sceFiberOptParamInitialize(0x801AB334)`、`sceFiberPopUserMarkerWithHud(0x4EC01F9C)`、
- `sceFiberPushUserMarkerWithHud(0x9C414B09)`、`sceFiberRenameSelf(0xDD50DF3D)`、
- `sceFiberStartContextSizeCheck(0xCD543A71)`、`sceFiberStopContextSizeCheck(0x67F70867)`。
- ⇒ **`0xCD543A71` / `0x67F70867` 未使用，故"不实现 fiber 栈越界检查"是可接受缺口**。
- | VA | 推测名 | 作用 | 证据 |
- | `0x81206BDC` | **SDK fiber trampoline** | `r4=r0`（fiber 对象）→ `r0=[r4+0x88]`(arg) / `r2=[r4+0x84]`(entry) → `blx r2`；返回后把 fiber 从运行队列摘除（改 `[0x816CBC08+0xA18/0xA1C]` 双链表）、把状态位 `[r4+0x80]|=2` | 反汇编：`81206BDC push {r4,lr}` / `81206BDE adds r4
- | `0x812067A0` | **fiber 轮转/让出函数（SDK）** | `GetSelf(&self)` → 读 `self+0x80`(flags)/`+0x94`/`+0x98`；把 self 挂入/摘出运行队列；必要时 `blx 0x812B1BA0`(Switch) —— 即"让出并轮转" | 反汇编：`812067B0 add r0,sp,#0` / `812067B2 blx #0x812b1b70` / `812067B8 ldr.w r2,[r3,
- | `0x81198BE8` | **fiber1 主体（本作"主协程"）** | `bl 0x811E5CDA` → `blx 0x812B2AC0`(`sceKernelCheckCallback`) → `bl 0x812067A0`(让出/轮转) → `ands r0,#2` 判 `[0x816CC630]` bit1，未清则回环 | 反汇编见上；探针 `[F1-ENTRY] reached! sp=0xA015D098 lr=0x81206BEC` |
- | `0x81198C0A` | **fiber2 主体（本作"任务驱动器"）** | 取 `r4=arg`(=入口参数 `0xA5FFFF28`)，调 `[r4+0x70]`；随后 `blx 0x812B2200`(strcmp)、`0x81199D7E`、`0x811995AC`(双 fiber 轮转点 `0x81198CF2 → 0x812067A0`)；后续是 state6/资源子流推进 | 反汇编 `81198C0A push {r4,r5,r6,lr}` … `8
- | `0x8120767D` | 通用线程 trampoline（**同文件内也调 GetSelf**） | `81207644 blx 0x812b1b70`（`sceFiberGetSelf`）→ `[sp]` 非 0 时用 `[0x816CBC08+0xA20]` 与 `GetThreadId()` 比对 | 反汇编 `81207640 str r0,[sp,#4]` / `81207642 add r0,sp,#0` / `81207644 blx #0x812b1b
- | `0x81181CE4` | **资源对象名字/字符串格式化（含栈金丝雀）** | 取 `[r0+0x94]`(名字指针)、`[r0]`(回调)、`[r0+0x88]`(备用缓冲)，做 `memcpy(dst, src, 0x100)` 后按内容格式化 | 反汇编 `81181CE4 push.w {r4,r5,r6,r7,r8,sb,sl,lr}` / `81181CEE movt sl,#0x812b` / `81181CF2 ldr.w r3,[sl]`(金丝雀) 
- | `0x8117BA30` | 资源加载 worker（ARCH_3 已述） | 在 `8117BB42` 用"表 + stride 0xC + 遍历次数"查一项，查不到则断言 `appResourceList.c:207` | 反汇编见 §5.3 |

---

## `ARCH_85_FINAL_REPORT.md`

**标题**：ARCH_85 — P5D 原生重写：阶段报告与路线图（第 20 轮收尾） ｜ 1. 目标完成度（诚实评估） ｜ 1.1 里程碑② 的细分完成度 ｜ 2. 已建成的资产（可直接复用） ｜ 3. 工作量估算（最终，第 20 轮） ｜ 4. 路线图（三阶段） ｜ 阶段 A — 语义提取平台（**约 90% 完成**） ｜ 阶段 B — 原生核心（用 C++ 直接实现游戏逻辑 + Vulkan 渲染，不再解释 guest 指令） ｜ 阶段 C — Android 移植 ｜ 5. 遗留问题清单（按风险从高到低） ｜ 6. 方法论（本项目 5 次同类错误的沉淀） ｜ 7. 可视化证据清单 ｜ 8. 结论 ｜ 9. ★ 增量更新（第 21–24 轮，2026-09-26 23:00 – 09-27 00:30） ｜ 9.1 §1.1 逐行修订 ｜ 9.2 剩余工作（诚实修订版） ｜ 9.4 ★ 阶段 A 收尾（第 25 轮，用户选定"性价比最高"的一项） ｜ 9.5 建议的优先序（修订）

**发现/结论**（50 条，最多列 40）：

- > 所有数字都来自实测（脚本 + 日志 + 反编译），每条结论都注明证据来源。
- | 里程碑 | 状态 | 证据 |
- | **② 私有资源格式语义** | 🟡 **大部分完成** | 见 §1.1 |
- | 子项 | 状态 | 关键证据 |
- | `.gap` 的真实身份 | ✅ 已澄清 | 不是未知格式，是 GFS0；占 37.5%（此前误记为 57%） |
- | 几何段字节格式 | ✅ **全语料 571/571 = 100%** | `_h114_batch.py`：解完 flags/控制点/索引后游标**必然**落在合法节点表上 |
- | 字节序 | ✅ 文件 BE / 内存 LE | `FUN_811df13c` 的 bswap 反汇编 |
- | 源 stride 规则 | ✅ | `FUN_811ae444`：`flags2` 按位累加 |
- | 目标 stride 与逐属性转换 | ✅ | `FUN_811ec808`（1148 字节） |
- | **float32→S16N 转换层** | ✅ **已定位，且纠正了命题本身** | 转换层 = `FUN_811ec808`；压缩函数 `FUN_811dd7de` = **float32→F16**（指数偏置 112 / `0x7bff` / `uxth` ⇒ 教科书式 half 转换） |
- | 索引 | ✅ 已解 | N 个 BE u16，`distinct = N` ⇒ 近排列；越界 2.2% 为边界效应 |
- | **`flags2` 位 → 语义名** | ✅ **已解** | §4.24：`0x02→position`、`0x10→normal`、`0x100→texcoord0`；两个实测点 stride 精确吻合（24 / 32） |
- **一句话**：**"格式与字节"这一层基本打通（且经过全语料验证）；"语义与绑定"这一层尚未开始。**
- | 工作类型 | 实际表现 | 结论 |
- | **有公开标准**的格式（DDS / BC1 / RGB565 / GFS0） | 数轮内完成 | 快、可预测 |
- | **游戏私有格式**（`.gmd`/`.gap` 段语义） | 第 12–19 轮，8 轮攻坚 | 慢，但**在本轮被"读游戏代码"而非"猜字节"的方法击穿** |
- 已得：容器 / 纹理 / 着色器（4,331 个 SPIR-V）/ UI 顶点格式（两族，已钉死）/
- 几何段格式（全语料验证）/ 转换层定位 / 可跑到主菜单并渲染 UI。
- **输入已齐备**：容器、纹理、着色器、顶点格式与转换规则。
- | 1 | 几何 `flags2` 位 → 语义标签 | 中 | 字节数与格式已知，语义未知 |
- **共同模式**：**结论的强度超过了证据的覆盖范围。**
- | 1 | ARCH_53：把"干净样本"的 padding 当成 stride 44/64 | 判据被填充值欺骗 |
- | 5 | §4.20：读了函数前半段就断言"不存在转换层" | **判定"不存在"前必须读完函数** |
- 4. **"全盘失败"是测量逻辑有缺陷的信号**——真实世界的格式不会在 571 个样本上全部失败。
- 5. **分层判定优先于单点实验**（§4.12 (f) 的 RT 回读一次排除整层嫌疑）。
- - **格式层已闭合**（全语料 571/571 验证），**语义层刚开始**。
- - **目标③已达成**（有可复现的可视证据）；**目标①未做**，因而③的"比对"部分无法完成。
- | **`.gap` 关键帧语义（`k=28`）** | ⬜ 未解 | **✅ 完全解出**：`k=28` stride 14 = **7 × IEEE-754 半精度（大端）= 平移(3)+四元数(4)**，字段顺序由单位四元数判据唯一确定（**9955676/9955676 = 100.00%**，最大偏差 8.46e-04 = half 精度）；`FUN_811dd816` 的机器码就是标准 half→float。前导 `count×u32` 是时间表（30fps） |
- | **骨骼动画 / 蒙皮驱动** | ⬜ 未解 | **✅ 完成（含干净帧验收）**：原生 C++ 渲染器（`p5d_gap.h` + `p5d_gmd.h` + `p5d_char_render.cpp`）已由游戏自身 `.gap`（身体 + 服装双层叠加）驱动渲染出**干净的非 T-pose 舞蹈姿态**。<br>关键修复（ARCH_137）：顶点蒙皮 20 字节里 **权重数组与骨骼下标数组是逆序配对**（`w[0]↔i[3]`）—— 正是它造成 113 个拉伸三角形
- | 顶点蒙皮布局（ARCH_109/126 修正） | ✅ 已解 | **✅ 修正**：`[4×f32 BE 权重][4×u8 骨骼下标]` 布局正确，但**配对是逆序的**（`w[0]` 配 `i[3]`）。此前"权重和≈1 + 下标<节点数"两条判据在 2233 个网格上 100% 通过却完全测不出该错误 —— 因为求和与配对顺序无关、范围也不变 | ARCH_66 §6.10(c) |
- | 材质 → 纹理绑定 | 🟡 名字已备 | **🟡 大部分已解**：`0x000100FC` 段 = `[u32 1][u16 len][name][完整 DDS]`，**贴图内嵌在 `.gmd` 里**；全库 **800/800 张抽出并解码**。仅剩"材质段 → 具体槽位"的显式映射表未逐项确认 | §4.39 |
- | 顶点属性描述符表 | （未列入） | **✅ 拿到**：28 项 `(标志, 读取函数, stride)`，运行时 dump | §4.37 |
- | 纹理通道顺序 | （未列入） | **✅ 验证并修正**：`sw=(5,4,3,6)`=`{SW_B,SW_G,SW_R,SW_A}`；渲染器通过 Vulkan 视图 component mapping 正确施加；**是我的导出器截取点错了** | §4.38 |
- | `.gap` 的真实身份 | ✅ "是 GFS0" | **✅ 进一步修正：是 pose/动画数据，不是网格**（路径 `dance/player/p5/pose/pcp201_1_h26.GAP`） | §4.39 |
- **⇒ 所以"原生重写"这个目标的完成度，要分两层看：**
- | **阶段 A：资源语义提取** | **≈ 90%** | 容器/纹理/着色器/顶点格式/几何/骨骼/贴图/组装规则 全部实测锁定 |
- | 阶段 B-1 剩余（骨骼节点树父子、脸贴图覆盖） | ✅ **完成**：`flags&4` 的 N×64B 实测为**逆绑定矩阵**，`W = inv(IBM)`；父子关系由「局部变换必须精确等于某条名字记录」唯一确定；**绑定自检 `|S_bind−I| = 6.1e-05`** 独立验证了整条矩阵链路（ARCH_66 §6.2–6.4） |
- | 阶段 B-2 动画（`.gap` 关键帧语义 + 骨骼动画 + 插值） | ✅ **完成**：`k=28` = **7×IEEE-754 半精度**（T3+Q4），单位四元数 **9955676/9955676 = 100.00%**；`FUN_811dd816` 机器码级确认为标准 half→float；平移为绑定饱和副本（无动画信息），四元数承载全部动画（108/109 通道跨帧变化）。原生渲染器已由 `pcp201_1.GAP` 驱动渲染出**非 T-pose 舞蹈
- | **阶段 B-3 起步：原生运行时** | ✅ **完成**（`p5d_asset_db.h` 124 行 + `p5d_runtime.h` 107 行）：<br>① **资源层**按游戏命名规则枚举/查询/报缺（角色 201：21 套服装 / 10 种头饰 / 3 个动作；实测出**服装 07 缺动作 0 的布料轨**等真实缺口）；<br>② **实时帧循环** `prepare()` 一次性解析 + `Player::step()` 逐帧渲染：**连续 300 帧，
- | 阶段 B-2 动画（`.gap` 关键帧语义 + 骨骼动画 + 插值） | ✅ **已完成**（原估 8–18 轮 → 实际 4 轮）；**残留**：多 `.gap` 叠加（`pcp201_1` 主体 + `pcp201_1_10` 服装）、少量骨名多义（0.67% 三角形）、法线/切线修正 ≈ 2 – 4 轮 |

---

## `ARCH_8_heap_mem.md`

**标题**：ARCH_8：内存与堆管理 + SceLibc(40) / SceLibm(54) / SceLibstdcxx(1) ｜ 0. 本轮新钉死的结论（与 BADMEM 直接相关） ｜ 1. 本子系统职责（一句话） ｜ 2. 导入/关键函数地址表 ｜ 2.1 宿主侧关键函数（`src/mem/` 与 `eboot_probe.cpp`） ｜ 2.2 SceLibc 函数导入（40 个，`libNID=0xBE43BB07`） ｜ 2.3 SceLibm 函数导入（54 个，`libNID=0xCDAE3C7D`） ｜ 2.4 SceLibstdcxx（1 个，`libNID=0xF7629240`） ｜ 2.5 SceLibc:var（3 个） ｜ 3. 关键数据结构（字段偏移 + 含义） ｜ 3.1 `p5d::mem::Region`（`address_space.cpp:15`，宿主侧） ｜ 3.2 页表（`address_space.cpp:24-29`） ｜ 3.3 线程/纤维栈相关 ｜ 3.4 guest 侧由谁分配（**推测**） ｜ 4. 生命周期与调用关系（调用链） ｜ 4.1 guest 侧分配/释放 ｜ 4.2 BADMEM 的完整调用链（本子系统与停机点的关系） ｜ 4.3 地址空间生命周期 ｜ 5. 当前 HLE 实现状态 ｜ 5.1 已绑定且真实现（有真实语义） ｜ 5.2 未绑定（走默认桩 `return 0`） ｜ 5.3 已实现但语义上有缺口（★ 按风险排序） ｜ 5.4 结论：BADMEM 与堆的因果关系 ｜ 6. 对「能否走到并打完一局」的影响评估

**发现/结论**（174 条，最多列 40）：

- > （`last_pc=0x81181CFA`，r3=0）与堆的因果关系判定。
- > 证据规则：`file_off = VA - 0x81000000 + 0x10E0`；反汇编
- | # | 结论 | 证据 |
- | 2 | 因此 **BADMEM 不是“malloc 返回 NULL”造成的**，而是“调用方手里本来就拿着 0” | `_h1164.log:146552 [ERROR] ThumbExec badmem addr=0x00000094 @pc=0x81181CFA sp=0xA016DE48 lr=0x8115FD5B r0=0x0 r1=0x0` |
- | 3 | BADMEM 的**直接调用链已完整钉死**（三段，全部有反汇编） | `0x8115FC28 bl 0x8115FD50` → `0x8115FD50 bl 0x81181CE4`（r0 原样传递）→ `0x81181CE4 adds r3,r0,#0 / 0x81181CFA ldr.w r0,[r3,#0x94]`。lr=0x8115FD5B 恰为 `0x8115FD56` 的下一条 |
- | 4 | 入口 arg0 的**唯一静态零来源**在 `0x8102EF34`：`ldr r0,[r4,#0x20]; cbnz r0,+8; adds r0,r1,#0; movs r1,#0; bl 0x8115FC28`，而 r1 来自更早的 `ldr r1,[r4,#4]`（`cbz r1` 分支后为 0） | `py _p5d_dis.py 0x8102EF00 30` 与 `0x8102EF2C 12` |
- | 5 | `0x8115FC28` 共 **29 个静态 BL 调用点**（无 BLX） | 全代码段 Thumb BL/BLX 解码扫描：`0x81002546, 0x8100463e, 0x810059ea, 0x810078d2, 0x8102ef34, 0x810310cc, 0x810310fa, 0x81031128, 0x8103f150, …`（共 29 个） |
- | 6 | **SceLibc:var 三个 NID 的宿主 stub 已定位，但名字无法从 vita3k 的 NID 库确认**（NID 库 7360 条里查不到这三个）| `F00 0x3CE6109D stub=0x812C1C04`、`F01 0xDDF9BB09 stub=0x812C1C18`、`F02 0xD662E07C stub=0x812C1C4C`（**推测**：这三个是 SceLibc 的“取 errno / 取浮点环境 / 取 locale 之类**
- | 7 | **`_sceLibcErrnoLoc`(0x9E248B76) 未绑定，且它是一个“写空指针”的定时炸弹** | 反汇编 `0x8115BF20 blx 0x812b2300 / 0x8115BF24 movs r1,#0xc / 0x8115BF26 str r1,[r0]`——默认桩返回 0 ⇒ 若该分支被走到就 `str 0xC,[0]`。实测本局**从未走到**（A 段条件 `0xFFC0 <= r7` 未成立） |
- | 名称 | 位置 | 作用 | 证据 |
- | `n_malloc` | `eboot_probe.cpp:201` | 导入 0x775A0CB2 | — |
- | `n_calloc` | `eboot_probe.cpp:205` | 导入 0xE7EC3D0B | — |
- | `n_memalign` | `eboot_probe.cpp:206` | 导入 0xA9363E6B（**注意 r1 才是 size，此实现只取 r1**）| `n_memalign=[](SceUInt32,SceUInt32 n,...)` |
- | `n_free` | `eboot_probe.cpp:207` | 导入 0x5B9BB802；只回收**块首地址** | `auto it=g_heap.find(va)` |
- | `n_memcpy` | `eboot_probe.cpp:5115` | 导入 0x7205BFDB；结尾 `std::memmove(p,q,n)` | `eboot_probe.cpp:5185` |
- | `n_memset_l` | `eboot_probe.cpp:5281` | 导入 0x6DC1F0D8；越界时**静默跳过** | `eboot_probe.cpp:5290` |
- 解析来源：`eboot.bin` 内 `SceModuleImports`（file_off `0x2B41A4`，VA `0x812B30C4`），
- NID 名来自 `v3k_nids.inc`；**调用次数**取自 `_h1164.log` 的 `[NIDCALL]`（该表只列 count>=1 的 195 项）；
- | NID | 推测名 | 作用 | 运行调用次数 | 静态 BL 站点 | 证据 |
- | 0x7205BFDB | memcpy | 拷贝 | **376840** | 469 | `[NIDCALL] 376840 … SceLibc/func=0x7205BFDB` |
- | 0x6DC1F0D8 | memset | 填充 | **260269** | 375 | 同上 |
- | 0x8AECC873 | strlen | 长度 | **142212** | 75 | 同上 |
- | 0x775A0CB2 | malloc | 分配 | **84430** | 1 | `[SVCHOT] stub=0x812B22A0 … count=84430` |
- | 0x5B9BB802 | free | 释放 | **75136** | 2 | `[SVCHOT] stub=0x812B2250 … count=75136` |
- | 0x1B58FA3B | strcmp | 比较 | **22505** | 101 | `[NIDCALL] 22505` |
- | 0x7449B359 | sprintf | 格式化 | **2253** | 267 | `eboot_probe.cpp:6418` 绑定 |
- | 0xA9363E6B | memalign | 对齐分配 | **333** | 0（间接） | `[NIDCALL] 333` |
- | 0x85B924B7 | strcpy | 拷贝串 | **321** | 72 | `eboot_probe.cpp:5653` |
- | 0xE4299DCB | strncmp | 定长比较 | **124** | 23 | `eboot_probe.cpp:5656` |
- | 0x0D5200CB | strstr | 子串 | **100** | 5 | `eboot_probe.cpp:5657`（注释记录漏绑曾致 6.7 万次返回 NULL） |
- | 0xE7EC3D0B | calloc | 清零分配 | **29** | 0（间接） | `[NIDCALL] 29` |
- | 0x7747F6D7 | memcmp | 比较 | **21** | 15 | `eboot_probe.cpp:5651` |
- | 0x9F87712D | strncpy | 定长拷贝 | **20** | 10 | `eboot_probe.cpp:5654` |
- | 0x802FDDF9 | vsprintf | 变参格式化 | **13** | 10 | `eboot_probe.cpp` 附近 |
- | 0xFE83F2E4 | vsnprintf | 变参格式化 | **3** | — | `[NIDCALL] 3` |
- | 0xDAE8D60F | time | 秒级时间 | **2** | 2 | `bind_by_nid(0xDAE8D60F,n_time)` |
- | 0xA1BFF606 | snprintf | 格式化 | **2** | 2 | `[NIDCALL] 2` |
- | 0xFBA69BC2 | strncat | 追加 | **2** | 2 | `eboot_probe.cpp:5662` |
- | 0xBFE02B3A | __cxa_set_dso_handle_main | C++ 运行时初始化 | **1** | 0（间接） | `[NIDCALL] 1` |
- | 0x9A004680 | printf | 输出 | **1** | 5 | `eboot_probe.cpp` |

---

## `ARCH_9_sysdial.md`

**标题**：ARCH_9 — 系统对话框 / 存档 / 模块加载 / 电源 / 更新 子系统 ｜ 0. 本轮纠正的错误结论（最高优先级） ｜ 1. 本子系统职责（一句话） ｜ 2. 导入 / 关键函数地址表 ｜ 2.0 六库的导入表事实（二进制直读，非推测） ｜ 2.1 SceAppMgrUser（2 个） ｜ 2.2 SceSysmodule（3 个） ｜ 2.3 SceAppUtil（8 个） ｜ 2.4 SceCommonDialog（18 个） ｜ 2.5 ScePower（1 个） ｜ 2.6 SceGameUpdate（3 个） ｜ 2.7 汇总 ｜ 3. 关键数据结构 ｜ 3.1 stub（导入桩）的地址布局（本文档所有 VA 的基准，实证） ｜ 4. 生命周期与调用关系 ｜ 4.1 调用链总览 ｜ 4.2 存档（SaveData）生命周期 ｜ 4.3 DLC（addcont）枚举生命周期 ｜ 5. 当前 HLE 实现状态 ｜ 5.1 绑定机制（先讲清楚「impl=Y/N」是什么） ｜ 5.2 已绑（本子系统共 9 条） ｜ 5.3 空桩 / 未绑缺口清单（4 条，全部在 SceAppUtil） ｜ 6. 对「能否走到并打完一局」的影响评估 ｜ 6.1 结论

**发现/结论**（183 条，最多列 40）：

- > 证据规则：映射 `file_off = VA - 0x81000000 + 0x10E0`；反汇编 `py C:\dev\p5d_build\_p5d_dis.py <VA> <条数>`；
- > 本文件所有 VA/NID 均可用 `py C:\dev\p5d_build\_p5d_imp3.py <模块名>`（导入表原样解析）与 `_p5d_stubmap.txt`（stub→NID 全表）复现。
- > 凡未实证处均标「推测」。
- | # | 此前结论 | 实际（附证据） | 影响 |
- | 1 | `SceAppUtil/0x6A140498`（59 次）最大缺口，是「AppUtil 存档/系统参数」，对「存档数据 → 主菜单可用性」有直接影响（`ARCH_1_boot.md:367`、`:411` 问题 6） | 它是 **`sceAppUtilDrmClose`**（`docs/translation_layer_functions.md:599`），59 次 = **DLC(addcont) 枚举循环的尾调用**，一次一个条目：`[_h1164.log
- | 2 | 需要「按 NID 反查函数名并实现 `0x6A140498`」 | 59 次里 58 次是同一循环的重复；DrmClose 正确语义就是「卸载已挂载的 DLC」，离线无 DLC 时返回 0 是**正确行为** | 该条从未解问题清单中撤除 |
- | 3 | `nid_resolver.h` 是模块 libNID 的权威表 | **它是错的**：`nid_resolver.h:35` 写 `ScePower=0x4332C10D`，而二进制导入表自带的 `library_name` 字符串证明 `0x4332C10D` = `SceNpTrophy`；`nid_resolver.h:52` 写 `SceCommonDialog=0xC6083AE1`，实际 = `SceNetCtl`。该头文件里有 **12 个 lib
- `library_name` 指针取自 `SceModuleInfo.import` 表的 `+0x14`（`0x812B2B7C..0x812B3298`）：
- | 模块 | libNID | funcs | 导入表条目 VA | func_nid_table | func_entry_table |
- | SceAppMgrUser | `0xA6605D6F` | 2 | `0x812B2B7C` | `0x812B3480` | `0x812B3AE8` |
- | SceSysmodule | `0x03FCF19D` | 3 | `0x812B2C80` | `0x812B362C` | `0x812B3C90` |
- | SceAppUtil | `0x48E01D74` | 8 | `0x812B2CE8` | `0x812B3648` | `0x812B3CAC` |
- | SceCommonDialog | **`0xE537816C`** | 18 | `0x812B2DEC` | `0x812B36B0` | `0x812B3D14` |
- | ScePower | **`0x1082DA7F`** | 1 | `0x812B2F8C` | `0x812B3780` | `0x812B3DE4` |
- | SceGameUpdate | **`0x2DD9A62D`** | 3 | `0x812B2FC0` | `0x812B3784` | `0x812B3DE8` |
- 交叉验证：`[_h1164.log:29] [INFO]   Import lib: SceCommonDialog (libNID=0xE537816C, funcs=18, vars=0)`、
- `[_h1164.log:37] [INFO]   Import lib: ScePower (libNID=0x1082DA7F, funcs=1, vars=0)`、
- `[_h1164.log:38] [INFO]   Import lib: SceGameUpdate (libNID=0x2DD9A62D, funcs=3, vars=0)` —— 与二进制逐字一致。
- `nid_resolver.h` 与上表冲突的 12 条：SceAudio/ScePower/SceFiber/SceNgs/SceVideodecUser/SceNet/SceNetCtl/SceHttp/SceSsl/SceNpManager/SceNpActivity/SceNpScore/SceNpTrophy/SceNpTus/SceCommonDialog/SceGameUpdate（举例 `nid_resolver.h:41` 把 `0x438BB957` 标成
- | NID | stub VA | 名 | 作用 | h1164 调用次数 | 证据 |
- | `0x10B5765F` | `0x812B11A0` | `sceAppMgrReceiveSystemEvent` | 取下一个系统事件；**无事件必须返 EAGAIN(`0x80010001`)** | **0** | 绑定 `eboot_probe.cpp:6449 bind_by_nid(0x10B5765Fu,n_eagain)`；`n_eagain` 定义 `eboot_probe.cpp:213`（注释：返 0 会让事件循环空转 1.27 亿次）；h116
- | `0x5E86319A` | `0x812B11B0` | `_sceAppMgrGetAppState` | 把 `r0` 指向的状态缓冲清零（`r1`=字节数），返回 OK | **3090** | `[_h1164.log:146961] [NIDCALL] 3090 impl=Y func=0x5E86319A lib=0xA6605D6F`；实现 `eboot_probe.cpp:217-219 n_appmgr_getstate`（`memset(q,0,sz
- > 注意：`0x5E86319A` 的 3090 次与 `sceGxmDisplayQueueAddEntry(0xEC5C26B5)` 的 3089 次几乎相等（`[_h1164.log:146963]` 附近），**推测**它是每帧调一次的应用状态查询。
- | NID | stub VA | 名 | 作用 | h1164 调用次数 | 证据 |
- | `0x31D87805` | `0x812B1850` | `sceSysmoduleUnloadModule` | 卸载系统模块 | **0** | 导入表；`[NIDCALL]` 无 |
- | `0x53099B7A` | **`0x812B1E90`**（孤立槽，不与其他两条相邻） | `sceSysmoduleIsLoaded` | 查询模块是否已加载（`0`=已加载） | **11** | `[_h1164.log:147023] [NIDCALL] 11 impl=Y func=0x53099B7A lib=0x03FCF19D`；绑定 `eboot_probe.cpp:6434 → n_noop`（恒 0 ⇒ 一律答「已加载」）；调用点 `0x8125
- | `0x79A0160A` | `0x812B1860` | `sceSysmoduleLoadModule` | 加载系统模块 | **8** | `[_h1164.log:147029] [NIDCALL] 8 impl=Y func=0x79A0160A`；绑定 `eboot_probe.cpp:6433 → n_noop`；`[_h1164.log:1703] [SVCWIN] stub=0x812B1860 nid=0x79A0160A SceSysmodule/
- **实测被请求的模块 ID（BL 调用点静态扫描 + 探针）——全部是网络/奖杯/NGS，没有任何 SceAvPlayer / SceJpeg / ScePng：**
- | 调用点 VA | 模块 ID | 枚举名（`refs/vita-headers/include/psp2/sysmodule.h`） | 证据 |
- | `0x81058354` / `0x81058CE2` | `4` | `SCE_SYSMODULE_HTTPS` | `81058352 movs r0,#4` / `81058CE0 movs r0,#4` `blx 0x812b1860` |
- | `0x81058360` | `0x15` | `SCE_SYSMODULE_NP` | `8105835E movs r0,#0x15` |
- | `0x8105836C` | `0x24` | `SCE_SYSMODULE_NP_ACTIVITY` | `8105836A movs r0,#0x24`（其后 `81058378 blx 0x812b1c40` = SceNpActivity `0xE0FFEE97`，见 `_p5d_stubmap.txt`） |
- | `0x81059FF4` | `0x1E` | `SCE_SYSMODULE_NP_SCORE_RANKING` | `81059FF2 movs r0,#0x1e` |
- | `0x8105A29C` | `0x25` | `SCE_SYSMODULE_NP_TROPHY` | `8105A29A movs r0,#0x25`；探针 `[_h1164.log:28066] [TROPHY-S2a] sysmoduleLoad(0x25) ret=0x0` |
- | `0x8105A494` | `0x2C` | `SCE_SYSMODULE_NP_TUS` | `8105A492 movs r0,#0x2c` |
- | `0x8125C268` / `0x8125C6B8` | `0x0B` | `SCE_SYSMODULE_NGS` | `8125C266 movs r0,#0xb`（前置 `8125C25E blx 0x812b1e90` = IsLoaded，说明是「未加载才加载」写法） |
- | `0x81198A04` / `0x81198A0E` | **表值 `0x5555`** | 非合法 ID | `811989F8 movw r6,#0x1f04` / `811989FC movt r6,#0x813b`；`81198A00 ldrh r0,[r6]` → `0x813B1F04` 处镜像字节 = `55 55 55 55`（`_p5d_stubmap` 同级证据：文件偏移直读）。**推测**该表由更早的初始化写入；本次运行读到的是未初始化填充值 |
- | NID | stub VA | 名 | 作用 | h1164 调用次数 | 证据 |
- | `0x2DB7BE3B` | `0x812B18A0` | `sceAppUtilDrmOpen` | 挂载/查询 DLC(addcont)；`r0=dirName, r1=mountPoint` | **59** | `[_h1164.log:146986] [NIDCALL] 59 impl=Y func=0x2DB7BE3B`；`[_h1164.log:79949] [NID18A0] ... r0=0xB332DFC0 r1=0x0`（r0=栈上名缓冲，r1=0）
- | `0x5DFB9CA0` | `0x812B18B0` | `sceAppUtilSystemParamGetInt` | 取系统参数（整数） | **1** | `[_h1164.log:147076] [NIDCALL] 1 impl=Y func=0x5DFB9CA0`；绑定 `eboot_probe.cpp:6622 → n_misc_ok`（恒 0，且**不回写** `*value`）；调用点 `811845FA movs r0,#1` / `811845F8 

---

## `HANDOFF.md`

**标题**：HANDOFF — P5D 原生重写（截至 ARCH_1205 / 2026-10-03） ｜ 0.0c ★★★ 续接摘要（**最新**，截至 ARCH_910 / 2026-10-02）—— 下一次从这里开始 ｜ A. 目标进度（诚实版，全部有实测证据） ｜ B. ★★★★★ ⑤ 的现状（**先读这里，避免重复我走过的弯路**） ｜ C. 本会话新增的可用工具/仪器（复现用） ｜ A. 目标进度（诚实版） ｜ B. ★★★ TITLE 阻断：**最终定位（ARCH_596 版）** ｜ C. 任务框架地图（本会话解出，均有反汇编依据） ｜ D. 出境链（八层，每层都有独立运行时证据，**全部 0 次**） ｜ E. ⇒ 下一步（**只剩一个问题**） ｜ F. 本会话被**否证 / 撤回**的结论（勿重走） ｜ 0.0 本会话（2026-09-29，ARCH_392 → ARCH_910）一页汇总 ｜ 一、已修（都有可复现证据） ｜ 二、本会话新发现、**仍未闭环**的阻断（已同步进 `UNRESOLVED.md` §8） ｜ 三、本会话**证否**的猜测（别再重走） ｜ 四、★ 一条方法论级的坑（会让你得出错结论） ｜ 五、红线状态 ｜ 六、下轮起点（明确） ｜ 0.1 解释器审计（ARCH_408，本轮完成） ｜ 0.2 当前离线断点（ARCH_414 / ARCH_418，进行中） ｜ (1) 「绘制在某一轮之后消失」是**假警报** —— 是运行非确定性 ｜ (2) 历史运行里的"绘制"**不是**场景渲染 ｜ (0) ★★★★★★★★ 用户提醒（本轮起点，又一次省掉大量重复劳动） ｜ (1) ★★★★★★★★ 检索结果（旧文档早已做过）

**发现/结论**（13051 条，最多列 40）：

- > 本文件是**主入口之二**：自包含。只写**本会话实测确证**的内容，未确证的显式标注「未验证」。
- | 子目标 | 状态 | 关键证据 |
- | **⑤ 选歌 → 对局** | ❌ **未达成**（**但已排除整条输入线**，见 B） | `[SEQNAME]` 已见 `DanceReady_P5`(frame 3700) → `config menu` → `modewindow` → `appWindow` → `wipe`；**`[DRSET]`（置阶段 6）恒 0**；**`[C30]`=0 ⇒ `case 0x30` 从未到达** |
- **结论：输入链是通的；卡点在游戏内部状态机的推进。** 三条**已用实测否定**的假设（勿再重复）：
- · `appPad_8116f02e` **读的是未用字段 `+0x24`（每帧被清零）** ⇒ 拿它当"游戏是否收到输入"
- · **不同 CAP 的运行会被同一墙钟超时砍在不同帧**（5040/4680/4020…）⇒ **帧数不可跨运行比较**；
- 1. 插桩 `FUN_811479ba` 的**内部状态 `[obj+0]` 的每一次写** ⇒ 得到**状态推进轨迹**，
- 2. 用**更长墙钟**跑到 `wipe`(frame ~4000) **之后** ⇒ 确认 `wipe` 是否就是"进入对局"的转场
- （若是 ⇒ 此前多次"停滞"可能只是**运行被超时切断在转场中**）；
- 3. 若确认 `wipe` 后应进对局而没进 ⇒ 再回到 `sequence_81185616` 的 12 个调用点排查。
- · 反汇编：`C:\dev\p5d_build\_dis\dis2.py <lo> <hi> <count>`
- · 运行：**CWD 必须含 `spv_all/`** ⇒ 用 `C:\dev\p5d_build\_advrun`；
- > 这一节是为"下一次继续"写的。前面 30 多条 ARCH 记录是**过程**，这一节是**结论与起点**。
- | 字体对象真实加载 | ✅ **早已解决**（`ARCH_396`：cmap 用 high byte，11/11 HIT；`START_HERE.md:216` 记「字体断言与资源表断言都是**真修掉**的」）。**目标陈述里的"第一阻断点"已过期。** |
- | 剩余格式解析器（`.gap` 1,655 MB 等） | ❌ **未开始** |
- > **从未被提交**（状态字段停在 `2`、不在提交链上）⇒ 因此**从不被调度**。**
- > **⇒ 缺口 = "submit 这一步对这 28 个没有发生"。**
- | **`+0x00`** | **全部 = 3** | **全部 = 2** ★ |
- | **`+0x48`** | 非 0（链表指针） | **全部 = 0** ★ |
- 构造     0x811DAB8C(node, 名字, …, fp)      ← 7 个调用者（含 titleSequence 的 0x811992B2/0x811992E4）
- ⇒ 66 个描述符；+0x34=任务节点；+0x00=2
- 状态推进 0x811DAD8C(node) 按 [node] 分派    ← 3 个调用者（含驱动者 0x811DB270）
- └ 状态 1 ⇒ 挂链 0x816CD9D0 + 置 [+0x00]=3   （写点 0x811DAE1A；`0x816CC630+0x13A0`）
- 调度     0x811DB32C 遍历 0x816CD9D0 ⇒ run_task(obj)=`ldr r0,[obj+0x34]; bl 0x8116F75E`
- ⇒ 只有"已提交（状态 3）"的会被跑
- ⇒ 38 个加载类被跑；28 个运行时类停在状态 2，从不被跑
- **可复现证据链**：`ARCH_580`（名字）→ `ARCH_581`（注册 vs 调度清单）→ `ARCH_584`（宿主/被驱动集合）
- **⇒ `title sequence` 的特例澄清**：它不是"没被创建"、也不是"参数不对"——
- 注册  0x8116F7CA(名字, 上下文, nextA, nextB, ...)
- ├ 0x81165288        名字 → 哈希（用 SceLibc strlen）
- ├ 0x8116F9DC        注册路径里按名字定位（其内 0x8116F9F8 调 0x8116F728）
- ├ 0x8116F728        按哈希在链表里查找节点
- └ 全局链表头 0x815554C0（[+4] 头、[+8] 首、[+0] 当前）
- 调度  0x8116F75E   [0x815554C0]=当前; ldr r1,[node,#4]; blx r1; 再按 +0x10/+0x14 摘链
- 0x8116F74A   调用 node[+0]
- 取getter  0x8116F8A8 = `ldr r0,[r0,#8]; bx lr`  ← 本会话最常误读的一个；它是"取对象的 +8 字段"
- **任务名字符串（在**代码段**内；`0x813xxxxx` 中 < `0x81325000` 的属代码段 —— 判段看区间，别凭高位）：
- `app_main_start` `0x813067C8`、`fadeCalc` `0x81307A9C`、`filer_draw` `0x81307BA4`、
- `mainDraw` `0x813095C0`、`mainDrawOt` `0x813095CC`、`appFontSystem` `0x812EEF1C`、
- `title sequence` `0x8130B140`。**

---

## `HANDOFF_ARCH_391.md`

**标题**：HANDOFF — P5D 原生重写（截至 ARCH_391，2026-09-28） ｜ 0. 一句话现状 ｜ 1. 项目与路径 ｜ 2. 环境约束（踩过的坑，务必遵守） ｜ 3. 已确证可用（真机验证） ｜ 3.1 运行表现 ｜ 3.2 横屏 ✅ ｜ 3.3 无 UI ✅ ｜ 3.4 触摸 → 判定 ✅（本会话新增，真机验证） ｜ 3.5 格式解析器已大量落地到 C++ ｜ 4. 未修缺陷 ①：取景被竖屏投影挤在中间窄带（根因已定位） ｜ 5. 未修缺陷 ②：没有声音（根因未定位） ｜ 6. 用户明确的设计要求（尚未完全落实） ｜ 7. 本会话的两个目标均已达成（已标 complete） ｜ 8. 关键文档索引 ｜ 9. 下轮建议顺序

**发现/结论**（21 条，最多列 40）：

- > **给新对话的接手文档**。前一会话上下文已触顶。本文只写**本会话实测确证**的内容，
- 实测 `surface 2400x1080`、`APP_CMD_WINDOW_RESIZED -> 2400x1080`。
- （`P5D_HUD=1` 可临时开启）。实测 `[hud] 已由 P5D_HUD=0 关闭`。
- `p5d_input.h:119 MapConfig::touchToDpad = false` 只填 `state.touch`，**无人消费** ⇒ 全漏。
- 实测命中：`GMD 152 / GFS0 61 / GNF 12 / PLG0 10 / NBLS 8 / MSG1 8 / FNT0 6 / FCT0 4 / FLW0 4 / SPR6 3`。
- **离线路径仍走 420×640**。改完**必须跑 `offline_regression.ps1` 确认摘要与 9 步不变**。
- 与 1033 的关系未查清。**下轮先确认投影到底用哪一个**，再决定覆盖点。
- `ch != g_srcCh` 导致走 memset 分支。**下轮先加一行日志确认 `g_pcmHave` 与 `loadWavPcm` 的返回值**。
- 3. **不要虚拟手柄，音游部分要用原生触屏** —— ⚠️ **需向用户说明一个实测约束**：
- 但紧邻注释是项目自己测出的结论：
- //   ⇒ 本通道目前**没有任何已证实的判定效果**，接上它只是为了忠实承载原版的掩码。
- **与 16..21 不重叠**。⇒ 原版游戏的触屏一定是在**更早的输入刷新层**被转成按键掩码的，
- - **`.fnt` 字形表 + 渲染**：`.fnt` 是 **`FNT0` 自定义字典压缩**格式（这是此前 13 轮
- **A 从来不是日文原版**（证据：`font_p5.fnt` 把 `\` 渲染为 `₩` 且含 Hangul；
- | `.fnt` 格式真相 + 渲染 | `ARCH_378`、`ARCH_384`、`ARCH_389` |
- | 格式解析迁移现状核实 + FCT0 | `ARCH_389` |
- | 前期全部逆向结论 | `ARCH_0`…`ARCH_389` |
- 2. **修取景**（§4 根因明确）——改完**必跑离线回归**确认摘要不变。
- **纪律**：语义一律逐字读参考实现，不信注释；关键结论换第二处复验；
- 每个里程碑要有可复现证据，不用近似 hack 冒充正解；推测一律标注。
- 本项目已有多次「证据不足把话说过头」被实测纠正的先例。

---

## `HANDOFF_ARCH_392.md`

**标题**：HANDOFF — P5D 原生重写（截至 ARCH_392，2026-09-28） ｜ 0. 一句话现状 ｜ 1. 项目与路径 ｜ 2. 环境约束（踩过的坑，务必遵守） ｜ 3. 已确证可用（真机验证） ｜ 3.1 运行表现 ｜ 3.2 横屏 ✅ / 3.3 无 UI ✅ ｜ 3.4 触摸 → 判定 ✅ ｜ 3.5 ★ 音频 ✅（本轮修复，详见 `ARCH_392_AUDIO_FIX.md`） ｜ 3.6 格式解析器 ｜ 4. ★ 已修（ARCH_393）：取景窄带 —— 真凶是**选错了相机文件** ｜ 5. 未修缺陷 ②：40 秒静音前奏来源不明 ｜ 6. 用户明确的设计要求 ｜ 7. 关键文档索引 ｜ 8. 下轮建议顺序

**发现/结论**（26 条，最多列 40）：

- > 本文只写**实测确证**的内容，未确证的显式标注「未验证」。
- **★ ARCH_393 更正**：`HANDOFF_ARCH_391` §4 说"取景窄带根因是 `p5d_render_core.h:1033` 的 aspect"
- | **歌曲推送** | **`_android\push_song.ps1 -Song 218`**（★ 本轮新增，不推就是静音） |
- ⇒ **不要在 `.ps1` 的 `-Pattern` 里写中文**：本轮的 `峰值=[1-9]` 就是这样失败的
- 10. **★ NDK 交叉编译独立探针**（本轮新增经验）：要单独验证某段头文件代码在 **Android 上**的行为，
- 11. **★ 上机验证必须先解锁屏幕并保持常亮**（本轮踩了三次）：
- ⇒ **锁屏会让渲染循环和音频一起停**，日志停在 `APP_CMD_TERM_WINDOW`，
- 设备上**根本没有 `data_extracted/sound/`**。⇒ 用 `push_song.ps1 -Song 218`。
- **实测教训**：music218.wav **开头约 40 秒本来就是静音前奏**。
- 内容自检只看头 1 秒会误报"整段静音"⇒ 已改为**全曲 8 段抽样**。
- `filesystem/cpk_reader.h`、`common/crilayla.h`。实测命中：
- **现象**：横屏 2400×1080 下画面内容只在中间竖带（截图实测内容带 = **708×1080**），左右各约 35% 纯黑。
- 实测反而更糟：`NDCx` 从 `[-0.23,0.15]` 缩到 `[-0.07,0.04]`。
- ⇒ `c218.GFS` 是**全库唯一一个量级异常**的相机文件。`ARCH_258` 把它设成"按曲号优先"之后，
- E 相机 .../c218.GFS 位置跨度=4484 > 1500（量级异常，拒收）⇒ 试下一个候选
- **已排除**：文件损坏 / 推送损坏 / 读取器错误（见 §3.5 的四环验证）。
- （可用 `wavstream` 或直接 Python `wave`），确认包络形状；再与谱面首音符时间比对。
- 3. **不要虚拟手柄，音游部分要用原生触屏** —— ⚠️ 需向用户说明一个实测约束：
- 但紧邻注释是项目自己测出的结论：与判定位 `kNoteKeyMask[6] = {0x08,0x80,0x02,0x10,0x04,0x40}`
- （内部位 0..7）**不重叠** ⇒ 原版游戏的触屏一定是在**更早的输入刷新层**被转成按键掩码的，
- | `.fnt` 格式真相 + 渲染 | `ARCH_378`、`ARCH_384`、`ARCH_389` |
- | 前期全部逆向结论 | `ARCH_0`…`ARCH_391` |
- 让投影与上屏宽高比一致 ⇒ 黑边消失且不变形。改动面大（`main.cpp` 里 20+ 处 `FW/FH`），
- 2. **查 40 秒静音前奏**（§5）——代价小，能确认"一进游戏就有音乐"。
- **纪律**：语义一律逐字读参考实现，不信注释；关键结论换第二处复验；
- 每个里程碑要有可复现证据，不用近似 hack 冒充正解；推测一律标注。

---

## `README.md`

**标题**：P5D Native Android Port ｜ 项目定位 ｜ 与模拟器的区别 ｜ 技术栈 ｜ 目录结构 ｜ 阶段里程碑 ｜ 法律声明

**发现/结论**（1 条，最多列 40）：

- - [ ] **阶段1**：资源格式逆向（模型/纹理/音频/脚本解析）

---

## `START_HERE.md`

**标题**：P5D 原生重写 · 唯一入口（START HERE） ｜ 0. 三十秒版本 ｜ 1. 只读这四份，其余按需 ｜ 2. 文档地图：权威 / 增量 / 已作废 ｜ 2.1 权威性与新鲜度（按 mtime） ｜ 2.2 ★ 已被推翻 / 不可再用的结论（照抄会出错） ｜ 3. 剩余工作（**合并后的唯一清单**） ｜ 3.1 主线：真正卡住推进的四道 ｜ 3.2 主线：其余未闭环项 ｜ 3.3 ★ 只存在于 ARCH_390–414 语料 / 探针线，**两份清单都漏掉**的活任务 ｜ 3.4 探针线自己的剩余工作（**两份清单完全没收录**） ｜ 4. 硬规则与红线（**全部收在一处**） ｜ 4.1 回归红线（用户点名，不可破） ｜ 4.2 ROM / 写盘红线 ｜ 4.3 工程红线 ｜ 4.4 探针线特有红线 ｜ 4.5 断言策略（**两者不是一回事，别混用**） ｜ 4.6 方法论纪律（本会话反复付出代价） ｜ 5. 仪器目录（**造新仪器前先查这里**） ｜ 5.1 帧捕获 / 看图（**帧号会漂移 ⇒ 优先用按内容触发**） ｜ 5.2 guest / HLE 内存写入 ｜ 5.3 逐 draw / 纹理 / 顶点 ｜ 5.4 运行控制与预算 ｜ 5.5b ★ 真机数据（ARCH_553/554 新增）：**离线副本 + 读取器**

**发现/结论**（118 条，最多列 40）：

- | 顶点格式 | **`VERTEX_FORMAT.md`（在仓库根，不在 `docs/`）** |
- | 3 | `ARCH_390–ARCH_414_*.md` | 09-28 | 增量证据；**`ARCH_414` 是最大号**（不存在 399/407/409/413/415+ 文件） |
- | 8 | `ARCH_55/56/57/58`、`ARCH_85/198/199`、`ARCH_34/39/40/42/43/54` | 09-26～27 | 历史；**多处结论已被推翻** |
- | 旧结论 | 出处 | 现状 |
- | `.gmd` stride = **44 B / 64 B** | `ARCH_53`、`UNRESOLVED.md:64` | ❌ **已作废**（`VERTEX_FORMAT.md:29-34`：在 115 个权威 stride 里出现 **0 次**） |
- | "池为空 ⇒ 需按需分配" | `ARCH_395` §9.2/9.3 | ❌ 作废（池本来就有货，`ARCH_396:330`） |
- | **`appGxModel.c:809` 断言 = TITLE 卡住的原因** | 本会话 ARCH_538–547（十余轮取证） | ❌ **已否证**（`ARCH_558`）：换成真实策略 `ABORT in place` 后，**仍然走到同一个 `TITLE->TITLE`**（frame 3717），断言之后游戏继续，最终死在游戏自己的主动崩桩 `0x8115F9D6`。**⇒ 该断言只是转移时的伴随现象，不是因果项。**围绕它的双重释放/池块回收/引用计数取证全
- | **TITLE 卡点是"HLE 返回值不对"** | 本会话多轮直觉 | ❌ **已否证**（`ARCH_563`）：40 个候选的敏感度扫描（`P5D_PERTURB`）结果 **`BEYOND = 0`** —— 没有任何单个 HLE 返回值能改变 TITLE 的推进。结合 `ARCH_562`（游戏按自己状态表的定义行动）⇒ **不要再用扫描式猜测 HLE 返回值。** |
- | **"动态写监控抓到 0 条写入 ⇒ 没人写那块内存"** | `ARCH_538c`/`542`/`543`/`545`/`547`/`548`/`565`/`567` | ❌ **已否证**（`ARCH_568`）：**不是仪器坏，是我 grep 错了标签**。`dw_hit` 打印的是 `[DYN32]`/`[DYN16]`/`[DYN8]`（`w*8`），而历次 grep 用的是 `[DYN4]` ⇒ `wr32` 的命中**一直都在**（回查旧日志：`[DYN3
- | "0x8115F9D6 BADMEM 是内存模型问题" | `ARCH_400/401/405` | ❌ 是 **guest 故意的 `abort()`**（`ARCH_406:11-22`） |
- | "忽略断言可以多走" | 多处 | ❌ **明确堵死**：只会把崩溃推迟（`ARCH_410:5-15`） |
- | # | 项 | 现状 | 证据 |
- | **N1** | ~~字形顶点程序步长落到错误默认 16~~ | ❌ **已证伪（ARCH_533）** | 全运行只出现 7 个顶点程序，**步长全部与权威表 `g_vfmt` 一致**；`vert_0010`（唯一走默认 16 的）原始字节 `04 0C B2 42 / 4E 19 BB C2 / 80 3C B3 C1 / 00 00 80 3F` = 四个连续 F32 ⇒ **16 是对的**。`HANDOFF.md` §ARCH_533 |
- | **N1b** | 字形位图被裁切 / 每字形度量错 / `幾` 完全不绘制 | 🔴 形态已定死，机制待定（**已排除步长**） | 异常槽 `wid=32,lft=0,rgt=0`；同一 `為`/`列` 各有两种光栅化；`幾` draw **0** 次。见 `HANDOFF.md` §ARCH_507~510 |
- | **N2** | **间歇 `NULL+0x4C` BADMEM**（`0x81207290` = `ldrex/strex` 原子自增） | 🔴 命中区间 **2.3e9~4.0e9 指令**；同一二进制一次跑满 4.0e9、一次 2.335e9 就死 | `HANDOFF.md` §ARCH_463/464；**3 个候选调用点已定**：`0x811B468E`、`0x811B4A04`、`0x811C6E08` |
- | **N4** | **★ 用户点名：「主界面贴图显示不完全」** | 🟠 **候选根因（算术成立），但渲染对照否定 ⇒ 未建立因果** | 候选：像素→NDC 用了**给内容 pass 标定的全局魔数** `sx=19.76/sy=6.85`，把像素空间的 `vert_0010` 压成 ~10×24 像素细条（`vert_0015` 被 `_selfproj` 排除，**不受影响**）。修法 `P5D_PX2NDC_SPACE=1`（按本 draw 自己的 loc0 范围
- | B | **B3 导入解析 / 运行库 handler** | 🟢 **"不可识别"已消除（ARCH_536），"未实现"仍待补**：机制根因 = `self_loader.h` 的 `typedef NidResolver` 与 `nid_resolver.h` 的 `class NidResolver` **撞名** ⇒ 该解析类**从来编译不过、从未被调用**；另有 16 个 `SceNid` 常量错位、5 个模块缺失、内核 handler 注册在错的 libNID
- | D | D4 小格式 PLG0/NBLS/SPR4/EF/EP/PAK | 🟡 部分 |
- | N | **N3** `heap_alloc` 忽略 `zero` 参数 | 🟡 已确认，**故意不改**（填垃圾属近似 hack；且 N1 三帧确定性 ⇒ 不是 N1 的因） |
- | — | `UNRESOLVED.md` **§5 八条"做了但未闭环"**（SEG1 高熵未解压、SEG1 静态可达性、ARM 164 簇、ARM BL 41.2% 命中率、187 个缺失 NID 语义、段表 400 vs 403 差 3、`.bin` 507、`e_entry` ARM 存根） | 🔴 **§6/§8 未收录，需补** |
- | 1 | ~~**`appGxModel.c:809` 断言**~~ ⇒ **已降级**：它**不是**因果项 | `ARCH_414:456-469` → **`ARCH_558`** | **`ARCH_558` 实证**：换成真实策略 `ABORT in place` 后仍走到同一个 `TITLE->TITLE`。**真正的卡点是"标题 update 每帧在调 `try_restart_title`、并发出 `scene_change(6)` 重开自己"**（见 `A
- | 2 | **`ARCH_395` 三个钩子零命中无解释** | `ARCH_395:746-750` | `0x81020590/0x810205AE/0x810205B8` 仍零命中，"这个现象没有解释" |
- | 3 | **`0x816CC630+0x7C` 的 `+0x20` 本应由谁写入** | `HANDOFF.md:2016-2018` | 这就是 N2 的根：源对象 `[+0x20]=NULL` |
- | 4 | **相机时长缺口**：`c108` 只有 162 s 而歌曲 323 s ⇒ 后半首歌相机停在末帧；`c218` 是否为后半段从未核对 | `HANDOFF_ARCH_392.md:246` | |
- | 6 | **真正全屏**：离屏画布 420×640 vs 屏幕 2.222 ⇒ 左右黑边 | `HANDOFF_ARCH_392.md:185-188` | |
- | # | 判据 | 要求 | 实测 | 状态 |
- > 为什么"取走请求"的 4 个站点从未执行？已确证：推入 **551** 次、表头摘除 1 次、`release` 3 次、
- **★ 已否决的方向（勿重走）** `progress.md:41066-41069`、`:41341-41342`：0x1B 队列/线程 0x6009、
- **★ 新事实（不在任何清单里）**：**Vita3K 参考实现已可正常运行并渲染**
- （`progress.md:41188-41231`，红舞台+灯光+金发人物，"完全不是黑屏 ⇒ 黑屏确实出在我们 port 侧"）。
- ⇒ 这**关闭了长期挂着的"目标① Vita3K 对照基线 = 0%"**（`ARCH_85:112/158`、`ARCH_198:75`）。
- 注意：**Vita3K 不 trace 逐次 export 调用**（`progress.md:41266-41285`）⇒"逐条 diff API 序列"**不成立**；
- `C:\dev\p5d_rom`（`crc32=0x81EA7585`，563,108 B，IDENTICAL）。
- - **存档里程碑（F1）会写盘 ⇒ 必须继续走副本，绝不可写备份。**
- （同类事故：`progress.md:41247` 的 `config.yml` 中文用户名变乱码 ⇒ 进程启动即崩。）
- 与基线对照应给出 `stop=3(BADMEM)@0x8115F9D6`、`totalInsn=590720000`。
- - **"跑了多少帧"绝不能当进展指标** ⇒ 必须**送显 RT 抓图 + 看图**判定。
- - **不要用帧号指定画面**（帧号↔场景会漂移）⇒ 用**按内容触发**。
- - 关掉 dynarmic 内存监听（每次运行堆 3.5 GB 日志）；那份日志**先别删**（原始证据）。
- | `P5D_ASSERT_SKIP=<hex>`（白名单，**键必须是 file 指针 `r[3]`**，不能是 LR —— LR 恒为 `0x8115F9D0`） | ✅ **有明文授权且经实测**（`ARCH_54:94-144`） |

---

## `UNRESOLVED.md`

**标题**：UNRESOLVED — 尚未解析/未实现项全量清单 ｜ 0. 一句话回答 ｜ 1. 资产全量普查（按 magic，10,555 文件 / 2,883.6 MB） ｜ 2. 逐格式解析状态 ｜ 2.1 移植代码中资源解析器**存在性**核对（源码全文搜索，2026 复核） ｜ 3. 系统行为层（未实现/未验） ｜ 4. 运行时路径（**2026-09-29 更正：已走到 ~2/5**，原为 "~1/5"） ｜ 5. 静态层面仍标为"未验证"的结论 ｜ 6. 优先级建议（按"是否阻断移植"排序） ｜ 阻断级 ｜ 高价值 ｜ 中长尾 ｜ 可延后（功能缺口，非阻塞） ｜ 7. 诚实边界 ｜ 8. 本会话（2026-09-29）新增的阻断项 —— 之前不在任何清单里 ｜ 另有一条**方法论级**的坑（不是子目标，但会让取证得出错结论） ｜ 本会话同时也**证否**了若干猜测（避免后人重走）

**发现/结论**（43 条，最多列 40）：

- > **方法**：按**magic**（而非扩展名）普查全部 10,555 个资产 + 逐项核对移植源码的解析器存在性。全部为实测。
- | **② 字段语义 + 少数格式缺解析器** | 解析器**已存在**（§2.1 实测：`GFS0` 61 / `GMD` 152 / `GNF` 12 / `PLG0` 10 / `NBLS` 8 / `MSG1` 8 / `FNT0` 6 / `FCT0` 4 / `FLW0` 4 / `SPR6` 3 命中）；**真正缺**的是 `.arc` 条目布局、`.bin` 的 507 个另类布局、`.pak`、`.usm`（合计约 863 MB），以及 `GFS0` 家族与
- | 格式家族 | 文件 | MB | 占比 | 来源扩展名 |
- **三条重要修正（相对此前的认知）**：
- 1. **`.gap` 就是 GFS0**（magic `GFS0`），不是未知格式。它占 1,655 MB / 57%，
- > **说明**：本表原版把所有格式的「移植代码解析器」列标为「❌ 无」。
- > 经 2026 复核，该列**已按实测重写**（见 §2.1）。
- | 格式 | 规模 | 容器层 | 字段语义 | 移植代码解析器 | 状态 |
- | ├ `.gmd`（几何） | 286 MB | ✅ | 🟡 **⚠ 本行 stride 已作废**：`ARCH_53` 的 `44 B / 64 B` 在 `VERTEX_FORMAT.md:29-34` 的 115 个权威 stride 中出现 **0 次**（原文：「ARCH_53 的 stride 结论作废」）；真实分布见 `VERTEX_FORMAT.md:11-27`（主导 30，16 只占 4/115）。场景几何已解（32 舞台 `IDENTICAL over
- | **GNF**（Vita 标准纹理） | 0.1 MB | ✅ | 🟡 Vita 标准格式，可查文档 | ✅ `p5d_fmt.h`(12) + `gxm/texture_format.*` | 🟡 |
- | TGA / DDS / PNG | 229 MB | ✅ 标准格式 | ✅ | ✅ `p5d_dds.h` 等（Vulkan 后端直接用） | ✅ |
- > **命中全为 0**，并据此得出「移植层目前没有任何资源格式解析器」。
- > 下表为按同一方法（`src/**/*.{h,cpp,c}` 全文搜索）**实测**结果。
- | 搜索项 | 实测命中数 | 对应实现 |
- | `0x30534647`（GFS0 小端字） | 0 | 解析器按**字节串**比较，不用该数值常数——故此搜索项**不适合**作为存在性判据 |
- ⇒ **「移植层没有任何资源格式解析器」不成立。**
- | **镜头/骨骼/动画数据** | 🔴 未验 | `.gmd` stride 已知，语义未知 |
- | **纹理上传路径** | 🟡 大幅推进 | 原"Vulkan 后端存在，但 586 个纹理对象实测只上传 2 张（ARCH_55）"**已过时**。本会话实测：全程 **2000 次上传**且格式/尺寸分布正常（`[TEXUPLOAD]`）；整运行普查得到 **84 张不同的线性纹理并全部转储**（`ARCH_489`，零遗漏；6 个 RT 别名）。**仍未闭环**：字形位图被裁切（见 §8 N1） |
- | TITLE 渲染 | ✅ **已到达并渲染**。原"🔴 卡死（35 条加载请求 100% 未完成）"**已过时**：`ARCH_396` 修掉解释器 `UXTB/UXTH/SXTB/SXTH` 的 `imm2` 位域取错（写成 `h2[6:5]`，正确 `h2[5:4]`）后，字体 cmap 页映射恢复，字体断言消失；本会话已在 frame 4300 拿到主界面截图（`_prefix_baseline/rt_main_1.png`） |
- ⇒ **移植最容易出错的路径（对局、结算、存档、网络）一次都没跑过。**
- 诚实标注（这些不是"没做"，而是**做了但结论未闭环**）：
- | **ARM 态（164 簇）** | 只统计了序言/尾声，**未逐一反汇编确认为真函数** |
- | **187 个缺失 NID 的语义** | 只有哈希，**无函数名**（需外部 NID 数据库） |
- | **`e_entry` 指向 ARM 存根** | 已确认存根存在，但**谁在什么条件下跳到这里**未完全追清 |
- 另 `ARCH_58_TASKS.md:228-264` 也早有实测（`[TITLE-LEAVE] n=1`、stateIdx 6→−1）。
- **⇒ 本条不应再列在阻断级。**
- 2. **补齐剩余少数格式的解析器**——**已修正**：原表述为「目前 `GFS0/SPR6/GMD/...`
- ⚪ **降级（2026-09-29 更正）：这 60 个 NID 在最长运行里\*\*一次都没被调用\*\***
- （`ARCH_395_PROBE_GATES_AND_FONT.md:573-608`；`[NIDCALL] impl=N` 全轮仅 7 条）。
- 原文结论：「这 60 个函数游戏一次都没调用 ⇒ 优先级应低于**实际被调用的**」。
- **⇒ 不是阻断项。**
- 9. `SceSysmem` / `SceCommonDialog` 调用点确认（可能影响存档/错误路径）。
- 11. **剩余小格式**：~~FNT0~~（✅ 已完成，ARCH_378/384，渲染可辨认）、
- **仍未解**：PLG0 / NBLS / SPR4 / EF / EP / PAK（合计 < 1 MB，但可能是关键小格式）。
- - "移植代码无解析器"是**全文搜索**结论（0 命中），不是抽样。
- | # | 项 | 严重性 | 现有证据 |
- | **N1** | 原表述：**"字形顶点程序的真实步长落到了错误的默认值 16"** | ❌ **已证伪（ARCH_533）** | 全运行只出现 **7** 个顶点程序（`[EHMAP]` 每名一次 ⇒ 全集）；`s_hvb` 的 if-else 链**特判了其中 6 个**，唯一走默认 16 的是 `vert_0010`（`4DDC6FAE3183381B`），而它的原始字节 `04 0C B2 42 / 4E 19 BB C2 / 80 3C B3 C1 / 00 
- | **N1b** | **字形位图被裁切 / 每字形度量错 / `幾` 完全未绘制**（N1 中**仍然成立**的那部分） | 🔴 主界面文字**肉眼可见地错** | 异常槽位 `wid=32, lft=0, rgt=0`（正常 CJK 有 2~4 px 留白）；同一字符 `為`/`列` 各有**两种光栅化结果**；`幾` draw 次数 **0**。**已排除"步长"这一条**。详见 `HANDOFF.md` ARCH_507~510 |
- | **N4** | ★ **用户点名："主界面贴图显示不完全"** —— 像素→NDC 用了**给内容 pass 标定的全局魔数** | 🟡 **根因已定、修复已实现，A/B 渲染对照中** | `eboot_probe.cpp:3378` 硬编码 `_sx2=19.76/_sy2=6.85/_ox2=1.729/_oy2=0.043`（注释自述是给内容 pass 标的：内容顶点 x∈[288,2967] ⇒ 除数应≈2.79）。三个**不同坐标空间**的程序共用它：`ve
- | **N2** | **间歇性 `NULL+0x4C` BADMEM**（`0x81207290` = ARM `ldrex/strex` 原子自增，对 `NULL+0x4C` 做加法） | 🔴 让任何长跑都不确定 —— 实测同一二进制：`a469` 跑满 4.0e9 指令，`a470` 在 **2.335e9** 就死 | `HANDOFF.md` ARCH_464/ARCH_463；命中区间约 **2.3e9~4.0e9 指令** |

---

## `VERTEX_FORMAT.md`

**标题**：VERTEX_FORMAT — 游戏真实顶点格式（权威数据，来自运行时 GXM API） ｜ 1. 真实 stride 分布 ｜ ⚠️ 对 ARCH_53 的最终否证 ｜ 2. 一致性验证 ｜ 3. 每个 stride 出现的 attribute 格式（聚合） ｜ 4. 字段语义（钉死） ｜ 4.1 位置恒定：`S16N × 3` = **12 字节**（不是 float32！） ｜ 4.2 两种典型布局（实样例） ｜ 4.3 语义映射（GXP 参数名 → 用途） ｜ 5. 索引格式（也已钉死） ｜ 6. 结论与对移植的影响

**发现/结论**（29 条，最多列 40）：

- > 的 **streams 数组**（`SceGxmVertexStream{ u16 stride; u16 indexSource; }`）与 GXP 参数表。
- > 这不是静态推测，是**游戏自己交给 GPU 的 stride 与 attribute 布局**。
- | stride | 使用它的 program 数 |
- **distinct strides = {12, 16, 18, 20, 22, 24, 26, 28, 30, 32, 36}**
- `ARCH_53` 曾给出 `m001_001.GMD = 44 字节`、`pc200_00.GMD = 64 字节` 两个 stride。
- **在这 115 个权威 stride 中出现次数为 0。** 结合 ARCH_58 §3.4 的发现
- **ARCH_53 的 stride 结论作废**。
- 判据：对每个 program 的每个 stream，`max(attribute.offset + attribute.size) <= stride`。
- ⇒ attribute 布局与 stride **自洽**，数据可信。
- | stride | 格式直方图 |
- 格式名：`S16N` = signed 16-bit normalized，`U16N` = unsigned 16-bit normalized，
- > | 族 | 用途 | stride | 布局 | 位置坐标系 |
- > 实测 A 族原始顶点：`(872.900, 455.600, 0.000) col=00000000 uv(0.002,0.375)` ——
- ⇒ **游戏在 GPU 侧使用压缩位置（signed 16-bit normalized × 3）**，
- **布局 A：stride=12（纯位置，2 个 program）**
- **布局 B：stride=30（主力，63 个 program）**
- **布局 C：stride=26**
- | GXP 参数名 | 用途 | 实测格式 |
- [DRAW#1] prim=0 idxfmt=0 idx=0xA2600000 count=3 vgxp=0x81539944 fgxp=0x8153985C
- [DRAW#2] prim=0 idxfmt=0 idx=0xA2D50220 count=6 vgxp=0xAA837008 fgxp=0xAA83B3A4
- **实测 8/8 次 draw 的 `idxfmt` 全部为 0**。
- ⇒ **索引缓冲是 16 位无符号整数（`U16`）**，不是 U32。
- > 不是场景几何。**尚未采到大模型的 draw**，故 stride 分布可能偏向 UI 类。
- > 但 stride 与 attribute 数据来自 vertex program（与 draw 无关），仍然权威。
- 1. **顶点格式已钉死**，可直接用于移植的顶点解码：
- - stride 由每个 program 的 `SceGxmVertexStream.stride` 决定（12–36）
- - 蒙皮：权重 `F16×4`（off=22）+ 索引 `U8×4`（off=26）（stride=30 主力布局）
- 2. **索引格式已钉死**：`U16`（`idxfmt=0`）。
- 这一层尚未定位——但它解释了为什么在 `.gmd` 上做 stride/float 扫描得不到结果。

---

## `_ARCHIVE_NOTE.md`

**标题**：归档说明（docs\_archive\_ARCHIVE_NOTE.md） ｜ 路径映射（重要） ｜ 为什么保留 ｜ 注意（对脚本的影响）

**发现/结论**（3 条，最多列 40）：

- ⇒ 即：**索引中的路径前加 `docs\_archive\` 就是实际位置。**
- 这些文档含**不可再生的证据链**（原版字节分析、反汇编片段、探针日志摘录、逐轮试错记录）。
- `P5D_MASTER.md` 只保留**结论 + 证据指针**；追细节时回查对应原文。

---

## `_h1120_handoff.md`

**标题**：h1120 交接：P5D 影片"引擎自身路径"播放 —— 卡点定位到此为止 ｜ 一、最终结论（全部有实测数字，同轮内自洽） ｜ 二、数据结构（已逐条反汇编确认） ｜ 三、解码任务的门（`0x812A9FAE`，每帧被调用） ｜ 四、被证伪 / 需撤回的旧结论 ｜ 五、方法论坑（务必遵守） ｜ 六、下一步（离打通只差这一步） ｜ 七、可复现命令

**发现/结论**（31 条，最多列 40）：

- [RINGW] pc=0x812AD9CE addr=+0x10 <-0xB48AF888 … <-0x0     ×7
- 入队指令是 `0x812ADB32 str r1,[r4,#0x10]`（push 到通道 `r1` 的表头），
- 环对象：`[decoder+0x34]`（例如 0xB48AF7C8），两个通道：
- | 0 | `[ring+0x10]` | **AU（压缩输入）** | 解码任务 `0x812AA20C`，`r1=0` |
- | 1 | `[ring+0x14]` | 已解码画面 | 渲染侧 `0x812A9B16`，`r1=1` |
- - `0x812AD974` = **pop**：`r4 = this + 4*r1`；`r5 = [r4+0x10]`；空则 `*out=0`；
- 非空则 `*out = [node+8]/[node+0xC]`，头前进到 `node->next`（`0x812AD9CE`），
- **并把节点还回空闲链**（`0x812AD9DA str r5,[r6,#0x24]`）——所以池不会因 pop 枯竭
- - `0x812ADABC` 起 = **push**：从 `[ring+0x24]` 取空闲节点，**池空则静默放弃**（`0x812ADB14 beq`），
- 否则填 `[node+8]/[node+0xC]` 并 `0x812ADB32 str r1,[r4,#0x10]`
- - `0x812A9A46` = "环里有几个 AU"：`vtable[+0x14](ring,1)` 后经**魔数除法 `umull`+`lsrs #7`（÷160）**
- 812A9FC6  beq.w 0x812aa1c2     ; r0==0
- 812A9FD0  beq.w 0x812aa1c2     ; [d+0x10]==1
- 812A9FDA  bne.w 0x812aa1c2     ; [d+0xC]!=2
- 812A9FE4  bl 0x812ACDB4        ; vtable[+0x14](ring,0) ← ★ 实测 1120/960/…/320 后恒 0，每帧挡回
- 812A9FEA  beq.w 0x812aa1c2     ; 上者返回 0
- 812A9FF4  bne 0x812aa0dc       ; [d+0x14]!=1（实测恒 0）
- CRI 时钟本身正常：`[MOVTIME]` 每 tick +1000，末值 0x382ED0。
- `0x812a6862`（**仍会取帧**）那条分支；它是被当**对象指针**用的句柄，不是标志位
- ⇒ `P5D_FORCE1178=1` 必然 BADMEM，验收②在语义上不成立。
- `0x812A83D6` 返回真实 NV12 帧指针 `0xB1140000 / 0xB11FF400`（相差 0xBF400 = 一帧）；
- 虚表项 `[[p+0x1148]+4][+0x3C] = 0x812A9AE9`（入口是 `0x812A9AE8`，bit0=1 是 Thumb 标记，
- 从 0x812A9AE9 反汇编必然乱码）。
- 4. **渲染侧没有"偷"解码侧的环条目**：两边是**同一对象 `0xB48BF750`、同一环、同一 pop 实现
- `0x812AD975`**，但**不同通道**（`r1=0` vs `r1=1`），各自独立推进。
- 1. 已知 push 函数体在 `0x812ADABC`–`0x812ADB36`（含空闲池空则静默放弃的分支）。
- 用 `_h1110_callers.py` 找不到对 `0x812ADABC` 的直接 `bl` ⇒ 它也是**间接/虚调用**，
- 需在 `0x812ADB12` 挂探针打印 `lr` 与 `r1`（通道号）来确定调用者。
- 2. 顺调用者回溯到 `.usm` 解复用 / Fios-CPK 读取点，确认是"读返回 0/EOF"还是
- → 节拍回调 `0x8129C6D8` 返回 1 → `0x8115420E player+0x50C=1`
- → 显示入口 `0x811543F4` 不再早退 → **影片四边形由引擎自身提交（验收③）**。

---

## `docs\ARCH_535_NID_LEDGER.md`

**标题**：ARCH_535 — NID / 运行库导入 台账（离线生成，可复现） ｜ 1. 游戏真实导入表（这是本作的 ground truth） ｜ 2. 本地 NID 库能命名多少 ｜ 3. ★ `nid_resolver.h` 的 `SceNid` 常量 vs 游戏真实 libNID ｜ 4. ★ 哪些库**真的注册了 handler**

**发现/结论**（75 条，最多列 40）：

- - 本地 NID 名库：`docs/nid_database.json`（9274 条）
- | 模块 | libNID | 导入函数数 |
- | `SceGxm` | `0xF76B66BD` | 100 |
- | `SceLibm` | `0xCDAE3C7D` | 54 |
- | `SceLibc` | `0xBE43BB07` | 40 |
- | `SceNgs` | `0xB01598D9` | 34 |
- | `SceLibKernel` | `0xCAE9ACE6` | 29 |
- | `SceCommonDialog` | `0xE537816C` | 18 |
- | `SceThreadmgr` | `0x859A24B1` | 15 |
- | `SceNpTus` | `0xAE84BDEA` | 13 |
- | `mceFios2` | `0x3AD4D630` | 12 |
- | `SceNpScore` | `0x75F052DB` | 12 |
- | `SceVideodecUser` | `0x163C3727` | 9 |
- | `SceAppUtil` | `0x48E01D74` | 8 |
- | `SceFiber` | `0xF88DAA70` | 6 |
- | `SceNpManager` | `0xD8835093` | 6 |
- | `SceNpTrophy` | `0x4332C10D` | 5 |
- | `SceAudio` | `0x438BB957` | 5 |
- | `SceIofilemgr` | `0xF2FF276E` | 5 |
- | `SceRtcUser` | `0x6704D3C6` | 4 |
- | `SceTouch` | `0x3E4F4A81` | 4 |
- | `SceSysmodule` | `0x03FCF19D` | 3 |
- | `SceNpActivity` | `0xFEF492ED` | 3 |
- | `SceGameUpdate` | `0x2DD9A62D` | 3 |
- | `SceLibc:var` | `0xBE43BB07` | 3 |
- | `SceSysmem` | `0x37FE725A` | 3 |
- | `SceAppMgrUser` | `0xA6605D6F` | 2 |
- | `SceDisplay` | `0x5ED8F994` | 2 |
- | `SceCtrl` | `0xD197E3C7` | 2 |
- | `SceDisplayUser` | `0x4FAACD11` | 1 |
- | `SceNet` | `0x6BF8B2A2` | 1 |
- | `SceNetCtl` | `0xC6083AE1` | 1 |
- | `SceHttp` | `0xE8F15CDE` | 1 |
- | `SceSsl` | `0xCAF5F2E9` | 1 |
- | `ScePower` | `0x1082DA7F` | 1 |
- | `SceLibstdcxx` | `0xF7629240` | 1 |
- | `SceLibKernel:var` | `0xCAE9ACE6` | 1 |
- | `SceProcessmgr` | `0x2DD91812` | 1 |
- | `SceThreadmgrCoredumpTime` | `0x5E8D0E22` | 1 |
- **410 / 410 = 100.0%** 的导入在本机 NID 库里有名字。

---

## `docs\ARCH_536_IMPORT_LEDGER.md`

**标题**：ARCH_536 — 真实导入表台账（410 条，已重定位，权威） ｜ 1. 按模块 ｜ 2. ★ 本机库**查不到名字**的（这些才是真正需要自己逆的） ｜ 3. 用法

**发现/结论**（44 条，最多列 40）：

- **⇒ 407 / 410 = 99.3% 的导入函数能在本机资源里查到名字。**
- | 模块 | libNID | 导入数 | 已命名 |
- | `SceGxm` | `0xF76B66BD` | 100 | 100 |
- | `SceLibm` | `0xCDAE3C7D` | 54 | 54 |
- | `SceLibc` | `0xBE43BB07` | 40 | 40 |
- | `SceNgs` | `0xB01598D9` | 34 | 34 |
- | `SceLibKernel` | `0xCAE9ACE6` | 29 | 29 |
- | `SceCommonDialog` | `0xE537816C` | 18 | 18 |
- | `SceThreadmgr` | `0x859A24B1` | 15 | 15 |
- | `SceNpTus` | `0xAE84BDEA` | 13 | 13 |
- | `mceFios2` | `0x3AD4D630` | 12 | 12 |
- | `SceNpScore` | `0x75F052DB` | 12 | 12 |
- | `SceVideodecUser` | `0x163C3727` | 9 | 9 |
- | `SceAppUtil` | `0x48E01D74` | 8 | 8 |
- | `SceFiber` | `0xF88DAA70` | 6 | 6 |
- | `SceNpManager` | `0xD8835093` | 6 | 6 |
- | `SceNpTrophy` | `0x4332C10D` | 5 | 5 |
- | `SceAudio` | `0x438BB957` | 5 | 5 |
- | `SceIofilemgr` | `0xF2FF276E` | 5 | 5 |
- | `SceRtcUser` | `0x6704D3C6` | 4 | 4 |
- | `SceTouch` | `0x3E4F4A81` | 4 | 4 |
- | `SceSysmodule` | `0x03FCF19D` | 3 | 3 |
- | `SceNpActivity` | `0xFEF492ED` | 3 | 3 |
- | `SceGameUpdate` | `0x2DD9A62D` | 3 | 3 |
- | `SceLibc:var` | `0xBE43BB07` | 3 | 0 |
- | `SceSysmem` | `0x37FE725A` | 3 | 3 |
- | `SceAppMgrUser` | `0xA6605D6F` | 2 | 2 |
- | `SceDisplay` | `0x5ED8F994` | 2 | 2 |
- | `SceCtrl` | `0xD197E3C7` | 2 | 2 |
- | `SceDisplayUser` | `0x4FAACD11` | 1 | 1 |
- | `SceNet` | `0x6BF8B2A2` | 1 | 1 |
- | `SceNetCtl` | `0xC6083AE1` | 1 | 1 |
- | `SceHttp` | `0xE8F15CDE` | 1 | 1 |
- | `SceSsl` | `0xCAF5F2E9` | 1 | 1 |
- | `ScePower` | `0x1082DA7F` | 1 | 1 |
- | `SceLibstdcxx` | `0xF7629240` | 1 | 1 |
- | `SceLibKernel:var` | `0xCAE9ACE6` | 1 | 1 |
- | `SceProcessmgr` | `0x2DD91812` | 1 | 1 |
- | `SceThreadmgrCoredumpTime` | `0x5E8D0E22` | 1 | 1 |
- | 模块 | libNID | func_nid | stub_vaddr |

---

## `docs\ARCH_537_DIFF_cases.md`

**标题**：ARCH_537 differential fuzz (cases)

---

## `docs\ARCH_537_DIFF_sweep16.md`

**标题**：ARCH_537 differential fuzz (sweep16) ｜ Mnemonic histogram (top 30) ｜ uc_faulted_we_didnt (3528) ｜ value_mismatch (387) ｜ we_stopped_uc_ok (319)

**发现/结论**（1 条，最多列 40）：

- **ARMv7-A-relevant encodings only（排除 ARMv8-M 专有的 `bxns`/`blxns` 与

---

## `docs\ARCH_537_DIFF_sweep32.md`

**标题**：ARCH_537 differential fuzz (sweep32) ｜ uc_faulted_we_didnt (26071) ｜ value_mismatch (2668) ｜ we_stopped_uc_ok (670)

---

## `docs\ARCH_539_REMAINING.md`

**标题**：ARCH_539 — 距"足以支持整个游戏"还剩多少（**按代码清点**） ｜ A. 导入面：410 个导入 / 35 个模块 ｜ B. 资源格式面（锚点：10,555 文件 / 2,883.6 MB） ｜ C. 运行时路径（锚点：历史日志的 `[STAGE]`） ｜ D. 已知保真度缺口（代码级、可复查） ｜ E. 汇总（可加总的项） ｜ F. 必须说清楚的三件事 ｜ G. 正确汇总（替代 §E） ｜ H. 但"19 项"会误导，真正的硬阻断只有 3 条 ｜ I. 必须同时说清的三条边界 ｜ ARCH_647：③ .gap 的 type 映射已离线补全（并更正 ARCH_81） ｜ ARCH_648/649：③ .gap 的 type 行语义与对象尺寸（离线可复现） ｜ ARCH_650：③ .gap 的对象模型（分配链逐层可复现） ｜ ARCH_651/652: ③ .gap 的读取核心与注册表（与任务管理器同块） ｜ ARCH_653: ③ .gap 注册表区块的真机离线核对 ｜ ARCH_654: ③ 注册表的跨线交叉验证（探针 ↔ 真机） ｜ ARCH_655: ★ 用三方判据发现任务框架状态块的真实差异 ｜ ARCH_656/657: ① 新线索——浮点累加器 0x816CD9A0 在跑但喂进垃圾 ｜ ARCH_658: ① 线索澄清——浮点垃圾是算出来的（内存初值为 0） ｜ ARCH_659/660: ① 源头锁定为 0x811DB804；三条线共用一块内存 ｜ ★★★ ARCH_661/662/663: ① 最终根因——解释器未实现 vcvt.f32.u32 ｜ ★★★ ARCH_665/666: **撤回** ARCH_663 —— 解释器早已有 vcvt.f32.u32 ｜ ARCH_667..671: ① 收敛到 **VFP 段内部** ｜ ★★★ ARCH_672/673: ① **排除 PRNG** —— vcvt/vmul 全部正确

**发现/结论**（238 条，最多列 40）：

- > 探针**不用** `sce*` 命名它的处理器，也**不调** `install_svc(`（实测 **0 次**）；
- > 它用 **214 个 `n_*` lambda**（`static auto n_\w+ =` 唯一名实测 **214**）。
- > **⇒ 正确硬指标：探针实现了 214 个命名 HLE 处理器。** 按关键字归类：
- > **⇒ 因此 §E 里"导入模块完全空白 / 无任何探针绑定"两行作废；正确汇总见文末 §G。**
- | `.gap` 字段语义 | ❌ 未闭环 | **1,655 MB / 57%**；公开渠道查不到格式文档 |
- | PLG0/NBLS/SPR4/EF/EP/PAK 小格式 | 🟡 部分 |  |
- ⇒ 未闭环 **4** 项（`.arc`/`.bin`/`.usm`/`.gap`），部分 **4** 项。
- | 选歌 → 对局 | ❌ 未到达 ⇒ **卡点已定位到状态机层（ARCH_800~825）：`sequence_811857c2` 在 handler 返回非 1 时把 `state` 置 -1 ⇒ 状态机空转；且存在**多个 seq 对象**，需先枚举** |
- ⇒ 7 个阶段到达 **4** 个（BOOT→DEBUG MENU、initread→TITLE、**TITLE→MAIN MENU**、**MAIN MENU→选歌**）；
- | 资源格式**未闭环** | **3**（`.arc` / `.bin`(507 种) / `.usm`；**`.gap` 已由 GFD-Studio 权威实现覆盖（ARCH_813）**） |
- | 资源格式**仅部分** | **4** |
- **⇒ 合计 = 0 + 34 + 4 + 4 + 5 + 5 = 52 项**（其中有重叠与层级关系，见下）
- - `TITLE → 下一阶段`（当前前沿；已定位到 `appGxModel.c:809` 断言现场）
- - `.gap`（**1,655 MB / 57%**，且无公开格式文档）
- 这些是网络/奖杯，本作在离线路径上未必调用。真正要按调用次数排序看 `[NIDCALL]`。
- | 资源格式未闭环 | **4** | `.arc` / `.bin`(507 种) / `.usm` / **`.gap`(1,655 MB = 57%)** |
- | 资源格式仅部分 | **4** | `.pak` / `.gmd` / SPR6 / 小格式族 |
- **⇒ 加总 = 5 + 4 + 4 + 1 + 5 = 19 项。**
- 1. **`TITLE → 下一阶段`** —— 当前唯一前沿。**已定位到精确断言现场**
- （`appGxModel.c:809`，条件 `(*(u32*)(obj+8)) & 1 == 0`，对象 `0xB4F3E03C`，调用点 `0x810F0D53`）。
- 2. ~~**`mceFios2`** —— 12 个导入~~ ⇒ ✅ **ARCH_643 已证：已完成**（12/12 全部绑定处理器；`n_fios_read` 走真 `fread`；运行时累计真读 **7,255,848 B**，`got == size` 每次成立）。（原"未闭环"说法已过期）
- 3. **`.gap`** —— 1,655 MB / **57%**，公开渠道查不到格式文档 ⇒ 自己逆。**状态更新（ARCH_645/646）**：已解出**解析入口 `FUN_811bd60a`**、**type 读取点 `0x811BD612`**、**stream 版本族 `0x1104xxx/0x1105xxx`**、**type 表 `0x813E1330`（行距 0x38，field[0]=名字指针、field[1..9]=处理函数）**、**type 名 
- 要么只在特定场景才需要（`.usm` 影片、小格式）。
- 1. **214 个处理器 ≠ 410 个导入已实现。** 处理器与 NID 不是一对一（有的一处理器服务多个 NID，
- 有的 NID 共用）。**按"调用次数"排序看 `[NIDCALL]` 才知道真正被用到的有几个。**
- **更正**：ARCH_81 说 type 表 0x813E1330 在文件里是 0、运行时才填 —— **不成立**。实测**文件里就是满的**，且运行时 dump 与文件逐字一致 ⇒ 静态数据。
- **完整 type 映射（离线，0x813E1330 + t*0x38，行距 0x38）**：
- **行结构**：field[0]=名字指针；field[1]=分配器（Thumb 指针，需清 bit0 再反汇编）；field[2..]=读取/标志处理函数。
- | 2 | mesh | 0x811B595A | 0xF0 = 240 |
- | 4 | geometry | 0x811AD486 | 0x140 = 320 |
- | 5 | camera | 0x811A480E | 0x1A0 = 416 |
- | 6 | light | 0x811AFB20 | 0xA0 = 160 |
- | 7 | epl | 0x811CC22E | 0x50 = 80 |
- | 8 | eplleaf | 0x811CDA66 | 0x50 = 80 |
- | 3 | node | 0x811BC06A | 走全局表 0x816CDA40（另一条路） |
- | 9 | morph | 0x811BB8BE | 先 ldr r4,[r5+0x10]（另一条路） |
- 公共分配器 0x811da534（eplleaf 用相邻的 0x811da55c）。
- 待办：解 0x811da534/0x811da55c；解 node 的 0x816CDA40；解公共尾 0x811ad21e。
- - 构造器 **0x811D9F4A**；对象头 **0x1C 字节**（r4 = fp+0x1c）
- - 底层分配经 0x811E6A08 → **malloc**（0x812B22A0 / SceLibc 0x775A0CB2）与 0x811E6A10 → **memalign**（0x812B2340 / SceLibc 0xA9363E6B）

---

## `docs\CAMERA_FORMAT.md`

**标题**：P5D 相机资源格式（`dance/camera/c*.GFS`） ｜ 1. 文件构成 ｜ 2. GAP 内部的相机控制器 ｜ 3. `KeyType 28 = NodePRHalf_2` 的键布局（stride = 14 字节） ｜ 4. 帧采样 ｜ 5. `KeyType 23 = CameraFieldOfView` ｜ 6. 坐标系约定 ｜ 7. 文件命名 —— **已从 eboot 代码解出**（见 `docs/DANCE_CAMERA_CODE.md`） ｜ 8. 已知的「不是 bug」

**发现/结论**（18 条，最多列 40）：

- FileHeader(16)  magic='GFS0'(0x47465330)  version=0x01105090  type=1  reserved=0
- Chunk #0        version=0x01105090  type=0x00010003 (GMD Model)         len=241
- Chunk #1        version=0x01105090  type=0x000100FD (AnimationPack)     len=35811
- | 0 | 1 Node | `RootNode` | 2 `NodePRS` | 1 | 静态 | 根节点，单位变换（stride 40 = f32×3 + f32×4 + f32×3） |
- 偏移 0  : f16 × 3   位置 X,Y,Z   —— **归一化**（实测 absmax = 1.000000）
- 偏移 6  : f16 × 4   旋转 X,Y,Z,W —— 单位四元数（实测 |q| = 0.9993…1.0000）
- 确认这是量化尺度而非巧合。
- `KeyType 26 (NodePRHalf)` 与 `28` 同为 stride 14；`KeyType 27 (NodePRSHalf)` 为 stride 20
- | stride | 4（单个 **f32**） |
- | 插值 | **线性**；但键成对重复 ⇒ 实际表现为「保持 + 约 1 帧内瞬切」的**镜头切** |
- 两者由同一个构造函数生成（`0x8104092A`，签名 `danceCameraGetPath(char* dest, int A, int B)`）：
- * **`_NNN` / `B` = `charaId`（角色号）**。强证据：
- * 装载时**最多取 4 条轨**（`0x8103BA18`）：槽 0 = `c<stageId>`；
- `500` 也不是合法 stageId（最大 311）。推测 500 是菜单/鉴赏用的特殊相机，**无证据**。
- **这本身是相机行为的正确结果**，但原因需要分清「已证实」与「推断」：
- * **已证实（定量）**：相机的瞄准点在整条轨道上会大幅游走。`c108` 在 t=0/t=129 瞄准角色竖轴
- * **推断（未证实）**：瞄准点的游走意味着舞者位于**原点之外的位置**（编队站位）。
- 所以「动作 018 属于歌曲 018」是**推断**，不是已验证的结论。

---

## `docs\CONTROL_SEMANTICS.md`

**标题**：操控语义（P5D / PCSH10109） ｜ 1. 原版操控方式（用户口述，权威） ｜ 2. 由代码侧确认/印证的对应事实 ｜ 3. ★★ 输入的**集合**已由用户确定：6 + 1 = 7 ｜ 3.1 三条独立事实在这里**交汇** ｜ 3.2 仍然未知的**只剩一项** ｜ 3.3 已知与这个顺序相关的线索（尚未能定案） ｜ 4. 逻辑动作位（`ARCH_235` 的数据侧结论） ｜ 4. 尚未建立的一环（明确记录） ｜ 4.1 `ARCH_239` 对"配置路径"的收窄 ｜ 4.2 ⚠ 工具适用范围（避免日后得出假否定）

**发现/结论**（18 条，最多列 40）：

- > **来源**：用户提供（**权威**），并由代码侧独立证据印证。
- > 相关：`docs/MNS_FORMAT.md`（谱面格式）、`ARCH_230`–`ARCH_238`（取证过程）
- | 输入读取每帧取 `SceCtrlData`，并同时处理**手柄与 2 点触摸** | `ARCH_231`：`0x811DA9A4` 每帧读控制器缓冲，`+8` 处是 `buttons`；`+0x148` 目标为触摸相关 |
- | **每帧的规范输入表示是「逻辑动作掩码」**，不是物理按键字 | `ARCH_231`：`0x816CDFB0 = (cur ^ prev) & cur`（上升沿）；`ARCH_238`：回放记录 `REPLAY:KEY` / `REPLAY:KEY_HIT` 正是这一对 |
- | **刮盘走摇杆**：代码里有三处「读 `lx/ly`、`rx/ry` 字节 → 减 `0x80` 中位 → 死区」的函数 | `ARCH_233`（`0x811A62xx`，死区 ±0.1 归一化）、`ARCH_236`（`0x811A76EA`，死区 ±32） |
- | 逻辑动作位里 `bit8` 对应 **L** | `ARCH_230`：`1 << id`；id `8` ⇒ `0x100` = `SceCtrlData.buttons` 的 **L**（LTRIGGER）——与"刮盘可改为 L/R"吻合 |
- ⇒ **音符输入 6 个 + 刮盘 1 个 = 7 个**。
- ⇒ **强推断（三条独立证据交汇，但仍是推断）：`id 8` = 刮盘；`id 0..5` = 六个音符输入。**
- ⇒ 说明游戏内部把 6 个输入**按对分组**；但"对"的含义未确定
- - id 是**位序号**（`ARCH_230`：`1 << id`）⇒ 它们是一组**逻辑动作位**；
- **⇒ 需要从两条路径之一取证**：
- - **配置/设置数据**：设置菜单为 `mainmenu/config/configMenu.c`（字符串 `0x812FE2C0`），
- 可在读 id 处（`0x8111B68A`）与输入比较点下探针。
- 配置模块代码范围 = **`0x810DF740`–`0x810E47D8`**（`configMenu.c` 被引用 24 处）。
- ⇒ 默认键位只有两种存在形式（都未验证）：
- 1. 存在**配置资源 / 初始存档**里（`CONFIG` 资源 `0x812FE410`，引用点
- `0x810E27A6` / `0x810E31F2` / `0x810E34AE`）；
- **在拿到证据之前，不要写 id → 物理键的表**（项目纪律：不得用反推值冒充正解）。

---

## `docs\DANCE_CAMERA_CODE.md`

**标题**：从 eboot 代码解出的「歌曲 / 舞台 / 相机」映射体系 ｜ 1. 源码级坐标：字符串区给出了模块与函数名 ｜ 2. `stageId` 体系 —— 97 首歌 ｜ 3. 相机路径构造函数（`0x8104092A`） ｜ 4. 相机装载逻辑（函数 `0x8103BA18`）—— **最多 4 条相机轨，按槽位绑定** ｜ 5. `_NNN` 后缀 = **charaId**（强证据） ｜ 6. 对之前结论的更正 ｜ 7. 仍未解 ｜ 8. 编舞号 → 歌曲号：`pc<chara>_<NNN>` 的 `NNN` 对应 stageId ｜ 9. 相机瞄准点的实测统计（`camaim.py`） ｜ 10. `p3/` 侧的实测命名

**发现/结论**（31 条，最多列 40）：

- > 状态：**已解**。本文的结论来自**反汇编实证**，不是从表格猜的。
- > （capstone + `C:\dev\p5d_build\eboot.bin`，代码段 file `0x10E0` / VA `0x81000000` / size `0x321810`）。
- `0x812F0600..0x812F0B80` 是 `danceFormat.c` / `danceCamera.c` 的诊断串：
- ⇒ 整族资源都是 **`(stageId, 变体)`** 两个数（`enemy` 只有一个数）。
- ⇒ `stageId` 是**总键**：`danceGetStageFormat(stageId)` 就是 `docs/DANCE_TABLES.md` 里那张 DSFT 表。
- * `ARCH_20_data_formats.md` §2.4 实测 MNS 只有 **45** 个曲目号（108,116,117,120..125 / 201..225 / 301..311）。
- 45 < 97 ⇒ **MNS 是 97 首歌的子集**，不是另一个编号空间。
- * `ARCH_28_music_select.md` §6.2 的「三套 ID 空间」结论需要修正：**谱面号、相机号、stage_format 的 stageId 是同一个
- 81040932  beq   #0x81040944       ; B == 0 → 单参数模板
- 8104093E  blx   #0x812b2280       ; sprintf(dest, 模板, A, B)
- 8104094E  blx   #0x812b2280       ; sprintf(dest, 模板, A)
- ⇒ 签名 **`danceCameraGetPath(char* dest, int A, int B)`**；**`B == 0` 时不写 `_NNN` 后缀**。
- 全 eboot 只有 **3 个调用点**，都在 `0x8103BA18` 这个函数里。
- 8103BAEC  bne     #0x8103bb74
- 8103BB04  bl      #0x8104a07e          ; 装载
- 8103BB12  bl      #0x8103e48c          ; bind(owner, slot=0, buf)
- 8103BB20  cbz     r2, #0x8103bb44      ; B == 0 → 跳过（不装该槽）
- 8103BB40  bl      #0x8103e48c          ; bind(owner, slot=i, buf)
- 8103BB48  blo     #0x8103bb1c          ; 共 2 次
- 8103BB4E  bne     #0x8103bb74
- 8103BB70  bl      #0x8103e48c          ; bind(owner, slot=3, buf)
- | 230 | `_111 _112 _116` + `_210` + `_001 _002` | **P3 与 P5 混合** ⇒ 联动曲 | 见下 |
- ⇒ 230 是**跨作品联动曲**，其演员表同时含两作角色。这与「变体号 = charaId」互为印证。
- 推测 500 是菜单/鉴赏用的特殊相机，但**无证据**，标为未解。
- | `_NNN` 是「特定段落/特定槽位」，尚未确认 | **是 charaId**（§5 的强证据），`B == 0` 时省略后缀 |
- * `obj[4]` / `obj[0x0F]` / `obj[0x24]` / `obj[0x34]` 的**写入点**（`0x8103BA18` 的调用者）。
- * 多相机轨的**用途**：是分屏、还是按玩家/舞者槽各选一条，尚未确认
- （`bind(owner, slot, buf)` 的接收方 `0x8103E48C` 未追）。
- ⇒ 早前「`c108` 与 `pc201_018` 不匹配」的判断是对的，但原因不是「两者属于不同歌曲」这么简单：
- 且**没有任何一条相机全程对着原点** ⇒ **舞者确实站在原点之外的编队槽位上**。
- `pc<chara>_<NNN>[_<variant>][_<costume>].GAP`，`NNN` 实测为 `002,004,…,030,340,342,344,345,347`

---

## `docs\DANCE_TABLES.md`

**标题**：P5D `dance/table/*.arc` 结构（实测） ｜ 1. `.arc` 是**嵌套归档的拼接**，不是一个归档 ｜ 2. 归档 A 的字节布局（实测） ｜ 3. payload **是小端**（与 `GFS0` 容器的大端相反） ｜ 4. DSFT 记录（`stage_format_00N.bin`，每条 **96 字节**） ｜ 5. 对 `ARCH_20` §4.3 的更正 ｜ 6. DTBL 记录 ｜ 7. 尚未解出的（**重要：这些答案不在本文件里**）

**发现/结论**（12 条，最多列 40）：

- * **自校验证据**：按 §2 的布局顺序走归档 A 的 45 个条目，游标**恰好**停在 `0x1740`，
- （实测分别为 5908 与 …），也**不等于 payload 总和**，语义未定。
- `ARCH_58_TASKS.md` §3.2(b) 把它读作「偏移表」结构；实测条目**是内联连续的**，
- 且顺序走查能精确落到归档 B 的起点，因此本文以实测布局为准。
- 按大端读是 `0x3C000000 = 1006632960`（无意义），按**小端**读才是 `60`。
- +0x10  u32      stageId，实测从 101 单调递增到 143（与 stage_format 条目序一一对应）
- **实测到的强相关性**（43 条无一例外）：
- （可能是 BPM/时长/相机参数，但无证据）。
- 实测：`+0x0C = 60`（= 0x3C ✔）、`+0x10` 递增 ✔（101…143）、
- ⇒ **`ARCH_20` 的 f32 偏移整体偏了一个 u32**，且未说明按小端读。字段语义结论不受影响。
- | `partner_type_table.bin` | 64 | `DTBL` + `1` + `17` + `2` + …（`0x0000FFFF` 等位域） |
- ⇒ 因此**当前无法**把角色放到「相机真正对准的位置」上。可用的唯一线索是

---

## `docs\GAP_FORMAT.md`

**标题**：P5D GAP（动画）格式 —— 逆向进展 ｜ 1. 资源分布与命名 ｜ 2. 容器结构（已确证） ｜ 3. AnimationPack（chunk type 0x000100FD） ｜ 4. Animation（`Animation.ReadCore`，IL 544 字节） ｜ 5. AnimationController（目标 = 骨骼/材质/相机/表情） ｜ 6. AnimationLayer（关键帧数据） ｜ 7. ★ 实测验证（`201/pc201_001_10.GAP`，6954324 字节） ｜ 8. ★ Key 子类的读取器（已全部 dump，`stride_of` 表验证正确） ｜ 9. ★★ 实测：times 与 bodies 是**分组**存储，不是交错 ｜ 10. ★★★ 表驱动解析器已完成并验证（`p5d_gap2.h`） ｜ 11. 现有实现的问题（`p5d_gap.h`，201 行） ｜ 12. 后续工作

**发现/结论**（35 条，最多列 40）：

- > 但对 `201/` 下的舞蹈文件失败。本文件记录已确证的结构，供表驱动重写。
- magic   u32 = 0x47465330  ("GFS0")
- version u32 = 0x01105090
- type    u32 = 0x000100FD  (AnimationPack)   ← 与 GMD 的 0x00010003(Model) 同级
- 实测 `pc201_001.GAP`：`length = 0x004489EA`，`0x10 + length = 4491786 = 文件大小` ✓
- `pose/pcp201_1.GAP` 的 length = `0x0001D33A`。
- if (version > 0x01104950):  Flags u32          // 0x01105090 > → 读
- if (version >= 0x02109900): METAPHOR_AnimArray3 = ReadAnimations()   // 本版本不读
- if (version > 0x01104110):  Flags u32           // 0x01105090 > → 读
- if (count == 0 && version >= 0x02000000) { u32; u32; }   // 本版本跳过
- if (Flags.HasFlag(0x10000000)):   Field10 循环 { Entry.Read; Epl; StringWithHash }
- if (Flags.HasFlag(0x20000000)):   Field14 = ReadResource<AnimationBit29Data>()
- if (Flags.HasFlag(0x80000000)):   Bit31Data { Int32; StringWithHash }
- if (version >= 0x02000000): PositionScale = ReadVector3(); ScaleScale = ReadVector3()
- (本版本 0x01105090 < 0x02000000 → 跳过)
- 现有 `p5d_gap.h` 里的 `stride_of(int k)` 37 项表就是这个映射的**运行时 dump 结果**，
- @0x00 FileHeader : GFS0, ver 0x01105090, type 1
- @0x10 Chunk      : ver 0x01105090, type 0x000100FD (AnimationPack), len 6954292
- Flags             u32 = 0x40000001
- hash        u32 = 0xE49E2B38
- **这确认了整条链路**：`AnimationPack.Flags` → `Animation[]` → 每个
- 说明实际编号空间更大。`p5d_gap.h` 里那张 37 项 `stride_of` 表
- **与 `p5d_gap.h` 里那张 37 项 `stride_of` 表逐项对照，完全吻合：**
- **结论：`stride_of` / `has_bbox` 这两张表可靠，重写时直接沿用。**
- 这大幅缩小了重写范围：**只需把定位换成表驱动，stride 逻辑一行都不用动。**
- bodies[keyCount]     ← 每个 stride_of(KeyType) 字节
- —— 但取值不符合「缩放」语义，**这一点尚未确认，下一轮要用轨迹核对
- 是否在 version 分支内（IL 里 `version >= 0x02000000` 才读 PositionScale）**。
- `Animation` 的尾部确认为 **24 字节**：`@591120..@591143` 处 6×f32，
- 紧随其后的 `@591144` 正是 `Animation[1].flags = 0x40000001`（与 `Animation[0]` 相同）。
- Anim[0] flags=0x40000001 dur=16.60 ctrls=108  keys=45752
- Anim[1] flags=0x40000001 dur=11.83 ctrls=108  keys=38824
- - 用 dump 来的 `stride_of` 表跳过数据体
- - `_gfdtool --il`：反汇编 `ReadCore` / `Read`
- 第一件该做的实测：录 `pose/pcp201_1.GAP`（能解析的）与 `201/pc201_001_10.GAP`

---

## `docs\GMD_FORMAT.md`

**标题**：P5D GMD (GFS0) 二进制格式 —— 逆向结果 ｜ 0. 最关键的结论：字节序 ｜ 1. 顶层结构 ｜ 2. Model（chunk type 0x00010003） ｜ 3. Node ｜ Attachment ｜ 4. Mesh ｜ 顶点属性（按 bit 升序读取） ｜ MorphTargetList ｜ 5. UserPropertyDictionary ｜ 6. Camera attachment（type = 5） ｜ 7. 材质与混合模式（MaterialDictionary，chunk type 0x000100FB） ｜ 8. 面部表情（morph / blendshape） ｜ 9. ★ 一个 GMD 内嵌多张贴图 —— 必须按材质取用 ｜ 更换瞳孔颜色 ｜ 10. 交叉验证结果 ｜ 7. 排查方法论（可复用）

**发现/结论**（51 条，最多列 40）：

- > 反汇编（IL）+ 运行时读取轨迹（TraceStream 记录每一次 `READ @off len`）作为 ground truth。
- - 按**大端**读 = `0x47465330` = `ResourceFileIdentifier.Model`
- - 按小端读 = `0x30534647` = `ResourceFileIdentifier.Model_LittleEndian`
- magic      u32 = 0x47465330  ("GFS0")
- version    u32 = 0x01105090  (pc201_f1.GMD)
- | 0x10 | 0x000100FC | 39685 | TextureDictionary |
- | 39701 | 0x000100FB | 1696 | MaterialDictionary（6 个材质） |
- | 41397 | 0x00010003 | 344135 | **Model** |
- 其它已知类型：`0x000100F8`、`0x000100F9`、`0x000100FD`(AnimationPack)。
- if version > 0x02040001:  u8
- if version <= 0x01090000:      u8
- if version > 0x01060000:       hasProps u8; 若为真则 UserPropertyDictionary
- if version > 0x01104230:       fieldE0 f32
- | `V_NODE_SKIP_BYTE` | 0x01090000 | 低于此值多读一个 u8 |
- | `V_NODE_PROPS` | 0x01060000 | 高于此值读 hasProps |
- | `V_NODE_FIELD_E0` | 0x01104230 | 高于此值读 fieldE0 |
- | `V_MESH_STRIDE_TYPE` | 0x02110205 | 高于此值读 strideType(u8) |
- | `V_MESH_FIELD14` | 0x01103020 | 高于此值读 field14(u32) |
- 本文件 `version = 0x01105090`，因此：跳过 u8、读 hasProps、读 fieldE0、
- 跳过 strideType、读 field14。
- if version > 0x02110205:  strideType u8
- if version > 0x01103020:  field14 u32
- **注意：没有 lod 字段。** 反汇编里存在 `LodStart/LodEnd` 的 `ReadSingle`，
- 但实测（`m_sirome`：bsphere 结束于 `@42834`，紧接着就是 `hasProps`）本版本不读。
- | 1 | 0x00000002 | Position | 3×f32 |
- | 4 | 0x00000010 | Normal | 3×f32 |
- | 28 | 0x10000000 | Tangent | 3×f32 |
- | 29 | 0x20000000 | Binormal | 3×f32 |
- | 6 | 0x00000040 | Color0 | u32 |
- | 8 | 0x00000100 | TexCoord0 | 2×f32 |
- | 9 | 0x00000200 | TexCoord1 | 2×f32 |
- | 10 | 0x00000400 | TexCoord2 | 2×f32 |
- | 11 | 0x00000800 | Color1 | u32 |
- | 30 | 0x40000000 | Color2 | u32 |
- **★ 蒙皮权重与顶点是交错的，不是独立的尾部块。**
- 实测 `pc201_11.GMD` 的 `m_tops_outline`：`attrFlags=0x12`（24 字节属性），
- `flags=0x8000001f`（含 HasVertexWeights），顶点 1894 个。
- 按 stride 24 或 32 算出来的位置都是错的。
- 把权重当成「顶点之后的独立块」会让 stride 少 20，索引位置全部算错 ——
- A     u32   (实测 = 2)

---

## `docs\MNS_FORMAT.md`

**标题**：MNS 谱面格式（P5D / PCSH10109） ｜ 1. 文件与容器 ｜ 2. 时间字段 `A` —— 打包定点时间 ｜ 3. 音符字段 `B` ｜ 4. 变速事件 ｜ 5. 已知异常（未解，勿当噪声丢弃） ｜ 6. ★ 已解（`ARCH_249`）：音符 id → 具体按键 ｜ 6.0 ⚠ 方法性更正：VA → file_offset 映射 ｜ 6.1 ★ `ARCH_247` 解出的部分：`id` **就是**紧凑动作域的位序号 ｜ 6.2 ★ 缺口闭合（`ARCH_249`）

**发现/结论**（54 条，最多列 40）：

- > **音符 id → 具体按键已解出**（`ARCH_249`，见 §6.2）：可复现证据
- | 资源名 | `MNS_DATA`，VA `0x813033D0` | `ARCH_219`（`ARCH_20` 记的 `0x8130F234` 是错的） |
- 来源：`ARCH_220`，在 `0x811220B0` 处**逐指令读出**；`ARCH_235` 全量验证。
- `低8 < 2^(指数+2)` —— 该断言在 **72936 条记录上零失败**。
- - **低字节 = 音符 id**，`bit7` 为**有效位**：`ARCH_225` 在 `0x81124338` 处读到
- `id = (b & 0x80) ? 0x7F : (b & 0x7F)`（bit7 置位 ⇒ 哨兵 `0x7F`）。
- - **id 是位序号**：`ARCH_230` 在 `0x8111B692` 读到 `lsl.w r1, r1, r6`（`1 << id`），
- > ⚠ **`ARCH_236` 修正**：这**只说明 id 被当作位序号用**，
- > （持续按住 `0x816CDAA0` 与上升沿 `0x816CDFB0` 的消费者分别是摇杆处理与菜单）。
- - **id 重映射**：`ARCH_228/229` 在 `0x8111AEEE` 读到一张 6 项表写回 `byte@+4`：
- ⇒ `ARCH_224`/`ARCH_223` 认为 `_0` 的记录流不能与 `_2/_3` 同构处理。
- | CODE | `0xE0` | `0x81000000` | `0x10E0 + delta` |
- | DATA | `0x3218F0` | `0x81325000` | `0x3228F0 + delta` |
- 此前沿用的单一公式 `VA - 0x80FFEF20` **在 CODE 段正确、在 DATA 段偏 `0x27F0`**。
- 所有**数据段**的历史结论都需按正确映射重读（详见 `ARCH_249` §1、§7 影响清单）。
- 以下为历史排查记录（结论仍成立的部分保留）：
- | 掩码位序是否读错 | **已独立确认**为标准 `SceCtrlData` 位序（`ARCH_232` 菜单消费者：bit4=UP、bit6=DOWN、bit13=○） |
- | 上升沿掩码 `0x816CDFB0` 的其他消费者 | 只有菜单，**判定不读它**（`ARCH_233` 排除端口副本盲区） |
- | `0x811A62xx` | 实为**摇杆归一化**，非判定（`ARCH_233`） |
- | `0x8116F520`（按键→逻辑位掩码的形状） | **已解释**（`ARCH_249` §4）：它就在 `0x8116F4F8` 内，读法**正确**；当年判"不符"是**数据段映射偏 0x27F0** 造成的假象 |
- | **u16 按键掩码表**（全代码段+数据段穷举） | **不存在**。判据：连续 n 个整数全属 `{0x0001..0x8000} ∪ {0x0300}` 且 ≥4 个不同值；n=6/7/8（u16）与 n=4/5/6（u32）全部命中均为**滑动窗口伪命中**（`0x813AC450` 一带把全部位平铺了一遍）（`ARCH_245` §2） |
- | **代码中同窗口 materialize 多个音符位** | 全段 6117 条相关立即数 → **仅 1 个窗口** `0x811AE77C`；`ARCH_248` §2.2 已把"逐位搬运按键"降级为待定（那更像结构体 flags 搬运） |
- ⇒ **不存在"id → 位"的重映射表**（`ARCH_245`）——因为 `id` 本身就是紧凑域位序号（§6.1）。
- **但"物理按键 → 内部位"的表是存在的**，即 `0x813AFC40`（§6.2），
- **★ `ARCH_246` 修正**：`[manager+0x424]` 经查**不是物理输入掩码**，
- 而是 `mnsReplay.c` 解析资源 **`"replay dance"`**（字面量 `0x81303388`）
- - 构造函数 `0x81132fae`，分配点归属字符串 `0x81303B00 = mnsReplay.c` 行 `0x34c`(844)；
- 与 `ARCH_244` 从**读取器** `0x811330d8` 反推出的契约**逐字段吻合**（两路互证，已确证）；
- `0`→下一个 u16；否则取 `hi`），由 `0x81132f1c` 展开为扁平 u16 数组，
- - 全代码段只有 **2 个调用者**（`0x8105595C`、`0x8112630A`），且只安装一次
- （前置断言"原值必须为 0"）——与"每曲一份回放轨道"一致。
- ⇒ `ARCH_242` 中"判定函数的 `r2` 是输入动作掩码"应修正为：
- **建议的后续取证方式**（静态反汇编已连续多轮只见排除项）：
- **输入侧已确证的事实**（`ARCH_231`）：
- `0x816CDFB0 = (cur ^ prev) & cur`，其中 `cur = SceCtrlData.buttons & 0xFFFF`
- （`0x811e6a78` 从控制器缓冲 `+8` 处取出），即**每帧"新按下"的 16 位掩码**。
- **建议的后续取证方式**（静态反汇编已连续多轮只见排除项）：
- 可在 `0x8111B68A`（读 id）与输入比较点下探针，直接观察 id 与按键的对应；
- 判定用的掩码由 `0x81122B3A` 起的实机分支构造（`ARCH_247` §3），
- | `0..5` | 六个音符（对应 `id 0..5`） | `0x8116ef8e(0,0,keyMask)` 六次，<br>keyMask 依次 `0x08,0x80,0x02,0x10,0x04,0x40` |

---

## `docs\P5D_LOGIC_SPEC.md`

**标题**：P5D 运行逻辑 & 画面逻辑 —— 规范说明（逐条带证据） ｜ 0.3 ★★★ 既有文档索引（**先读这些，别重复做**） ｜ 0.3.0 ★★★★ 已确认"旧文档早就做过"的实例（2026-10-03，用户提醒后检索） ｜ 0.3.1 既有文档索引表 ｜ 0. 文档地图（已有专项文档，避免重复劳动） ｜ 0.1 ★★ 方法论：**离线优先**（2026-10-03 用户指正后确定） ｜ 0.2 文档地图 ｜ 1. 启动与状态机（运行逻辑） ｜ 1.0 ★★ 启动阶段全序列（**探针运行时实测，2026-10-03**） ｜ ★ 切换序列（**探针实测，仅 2 次**） ｜ ★ entry(6) 的 update handler（**探针实测**） ｜ ★ UBO（每帧变换矩阵）地址已暴露（**U-4 的取证入口**） ｜ ★★ UBO 成员**名字**已暴露（2026-10-03，**重大**） ｜ ★ UBO **值** dump 实测（2026-10-03，**既有开关拿不到值**） ｜ 1.1 状态表（**已确证**） ｜ 1.2 每帧状态机（**已确证**） ｜ 1.3 切换协议（**已确证**） ｜ 1.4 标题相关状态机（**已确证地址**） ｜ 2. 画面逻辑（**逐屏构成**） ｜ 2.1 启动序列顺序（**已确证素材 + 次序由用户纠正**） ｜ 2.2 影片清单（**已确证**） ｜ 2.3 标题界面（**已确证**） ｜ ★ U-2 结论（**用户更正 + 抽帧验证，2026-10-03**） ｜ 2.4 资源格式（**已确证**）

**发现/结论**（196 条，最多列 40）：

- > ★★ **2026-10-03 重要更正**：用户提醒"阅读我们以前的文档，说不定有些部分我们之前做过了"
- > ⇒ **检索后确认：本仓已有大量专项文档，覆盖了我"未决清单"的大部分项**！
- > ⇒ **后续必须先读这些文档并汇总，而不是重新反向**（见 §0.3 既有文档索引）。
- 【实例 1】`ARCH_16_app_states.md` **已含"状态表全表 + 逐条反汇编注释"**：
- · **state 6 update = `0x81185986`**，注释写明：
- 81185988  bl 0x811980FC   ; **pollTitleSequence()**
- 81198108  bl 0x8116F900   ; **findTask**
- 8118598C  cmp r0,#0 ; beq …  ; 任务仍在 ⇒ 保持；任务消失 ⇒ 退出（下一帧 state=-1）
- · **state 6 leave = `0x81185998`**（含断言 line 0x77）
- · **每帧驱动链**：`0x811850FC` 在 MAIN init 里被调用 = **注册帧/子系统的入口**
- （同族 `0x81185152` / `0x811851DA`）；state 6 入口 `0x811850DA` 由它调用
- · ★ **关键结构性结论**：**状态机不是独立线程**，而是挂在 **GFD 任务池**
- （`0x811DB24E`，即每帧调度器 `0x811DB1F0` 族）的**一个 job** 里，
- **跟着任务链每帧跑一次** ⇒ 这解释了"同一条 PC 被反复执行几万次"
- （`DEBUG MENU`：`0x811858CE`（`bx lr` 空）/ `0x811858D0` / `0x811858EC`）
- 【实例 4】`ARCH_14_io_display.md` 已含**输入层**（`sceCtrlPeekBufferPositive` NID `0xA9C3CED6`）
- ⇒ ⇒ ⇒ **结论：我的"未决清单"里 ①/U-1/U-5/U-7/U-8/U-9 大多**有既有答案**；
- **必须先读旧文档并汇总**，再对"文档未覆盖/证据不足"的部分做新分析。
- | **`ARCH_22_ui_system.md`** | **7 个 app 状态真实名**（`DEBUG MENU`/`initread`/`FREE MODE`/`COMMU`/`TUTORIAL`/`MAIN MENU`…）+ 各状态 handler 地址（如状态 0：`0x811858CE` 空 / `0x811858D0` / `0x811858EC`） | **U-9**、① |
- | **`ARCH_14_io_display.md`** | 输入层：`sceCtrlPeekBufferPositive`（NID `0xA9C3CED6`）"读按键"；`SceTouch` 暂缓项 | **U-8** |
- ⇒ ⇒ **行动：下一阶段先把上表**逐份读入并汇总**，把已有结论**合并进本规范并标注证据**，
- **只对"文档未覆盖/证据不足"的部分做新的离线分析**。
- > 1. 每一条结论**必须**标注证据来源，格式：`[证据: eboot 0xADDR 反汇编 | 资源字节 | 探针日志 | GitHub 实现 | 截图]`
- > 3. 无法确证的条目**不得**写成结论；进"未决清单"。
- > 版本：v0.1（2026-10-03）—— 骨架 + 已确证条目；后续逐轮补全。
- ⇒ ⇒ **完全成立**，据此确定三条硬规则：
- ① **离线是第一证据源**：资源文件字节 + 反汇编 + GitHub 权威实现
- ⇒ **凡是"静态数据里就能查到"的，一律不进运行时**
- `VUBO … floats:` **恒为 0**）⇒ **不得把探针输出当唯一依据**
- ③ **每个结论标注证据类型**，并**优先引用离线证据**（资源字节 / 反汇编 / 权威实现）
- [证据: eboot_probe_a1004.exe + P5D_STAGE=title + P5D_VERBOSE=1；日志 `C:\dev\out\_probe_title.log`
- ⇒ ⇒ **重要修正：启动**不是**"厂商 LOGO → OP → 标题"那么简单 —— 表里实际经过
- 而 `[SEQCHG] n=1 ENTRY idx=0x1 lr=0x8118475A` 与 `0x8118474E` 的静态调用点**吻合** ✓
- ⇒ **`DEBUG MENU`（状态 0）此前未记录** ⇒ 见 U-9
- [SEQCHG] n=1 ENTRY idx=0x1 entry=0x0 r2=0x0 r3=0x0 **lr=0x8118475A**
- [SEQCHG] n=2 ENTRY idx=0x6 entry=0x0 r2=0x0 r3=0x5 **lr=0x811850F8**
- ⇒ ⇒ **整个启动只有 2 次状态切换**（`→1` 与 `→6`）
- · `→1`：`lr=0x8118475A` ⇒ 与静态调用点 `0x8118474E`（`bl sequence_81185616(1,…)`）**同一处** ✓
- · `→6`：`lr=0x811850F8`，且 **`r3=0x5`** ⇒ 与静态调用点 `0x811850EC`（→6）**同一处** ✓
- · **`BOOT->0` 不属于 `[SEQCHG]`** ⇒ 状态 0 是**状态表的初始值**（"boot state from table"）✓

---

## `docs\P5D_SWITCHES.md`

**标题**：`P5D_*` 开关清单与正式化进度 ｜ 一、已删除（前提被证伪的 hack） ｜ 二、已确证的正解（已落地为「渲染语义表」） ｜ 三、功能性开关（保留，是特性不是 hack） ｜ 四、诊断开关（保留） ｜ 五、纯 A/B 对照（保留，仅供回退比较，不是语义） ｜ 六、进度

**发现/结论**（2 条，最多列 40）：

- **验证**：删除后同一场景渲染结果**逐行 0 差异**（848×640）—— 确认是死代码。
- | `kLightDir` | (0.30,0.50,0.80) | 旧值 z 为负 ⇒ 全裁到 `lam=0.32`（背光） |

---

## `docs\RENDER_SEMANTICS.md`

**标题**：P5D 渲染语义表 ｜ 1. 设备与光栅化 ｜ 2. 背面剔除 ｜ 3. 深度 ｜ 4. 纹理采样 ｜ 5. 光照（toon） ｜ 6. 混合（按材质 DrawMethod） ｜ 7. 顶点解码 ｜ 8. 动画（GAP） ｜ 9. 已知资源特性（不是 bug） ｜ 10. 尚未确定的项

**发现/结论**（14 条，最多列 40）：

- | 背景色 | **RGB(16,16,16)** | 与参考对拍时确认 |
- | 面积符号（等价判据） | 保留 `area < 0` | 推导：A=(0,0)、B=(a,0)、C=(0,-d) ⇒ area = −ad < 0 |
- | 深度符号 | `z = z_raw`（默认）；`z = -z_raw` 仅在 `P5D_ZFLIP` 对照下 | 相机在 +Z ⇒ z 越大越近 |
- | 环绕 | **clamp 到边缘**（不是 REPEAT） | 与参考一致；本模型 UV 未越界，两种方式实测零差异 |
- | 光方向 L | **(0.30, 0.50, 0.80)** 归一化 | 早期沿用参考的 `(0.45,-0.55,-0.72)`，z 为负 ⇒ 光源在模型背后 ⇒ 所有朝相机面片被裁到 `lam=0.32`（整体背光）。相机在 +Z ⇒ 光源 z 分量必须为正 |
- | 法线 | 顶点法线的**均值**（除以 3），不再二次归一化 | 曾多除一次 3 ⇒ 整体暗约 3 倍 |
- | 属性顺序 | 按 `attrFlags` **位升序**：Position(2) → Normal(0x10) → Tangent(0x10000000) → Binormal(0x20000000) → Color0(0x40) → UV0(0x100) → UV1(0x200) → UV2(0x400) → Color1(0x800) → Color2(0x40000000) | `Mesh.ReadCore` 旧版路径 IL + 实际字节 |
- | 蒙皮权重布局 | 顶点末尾 **20 字节 = 4×f32 权重（前 16B）+ 4×u8 骨骼索引（后 4B）** | 实测 `raw20 = 3d b8 51 ec 3f 21 47 ae 3e 8f 5c 29 00 00 00 00 00 04 03 02` ⇒ 权重和 1.0000、索引 [0,4,3,2] |
- | 权重与顶点 | **交错**（每顶点 +20B），不是尾部独立块 | 当成独立块会让 stride 少 20 ⇒ 索引位置全错（身体/头发只出 1 个网格） |
- | 索引格式 | `indexFormat`：1 = u16，2 = u32 | — |
- | 容器 | 与 GMD 同构：`GFS0` + chunk `0x000100FD`(AnimationPack) | — |
- | KeyType → 层语义 | **31 = 位置**（Vector3Half）、**32 = 旋转**（QuaternionHalf，|q|≈1）、**33 = 缩放**（Vector3Half） | 实测：32 层四元数模长 0.9998 |
- | 女巫装（`pc201_10`）在 t=0 抬头时颈部有洞 | **该服装不含颈部皮肤网格**。同样管线渲染标准装 `pc201_01` 完全正常；服装 10 靠高领+红围巾覆盖颈部，t=5.0 姿势下被挡住。扫描 294 个 GMD 后确认「窄柱状 y∈[138,152]」的网格只有 `pc201_01/25` 的耳机配件，无其它候选 |
- - `KeyType` 2 / 26 / 27 的骨骼（当前适配层跳过 ⇒ 这些骨骼不动）

---

## `docs\_append_h144k18.md`

**标题**：2026-09-21（h144k17 / h144k18）**根因命中：解释器缺 `vcmp.f32/f64` 与 `vmrs APSR_nzcv,FPSCR`** → 高斯权重核越界踩掉顶点流指针槽 → 首个真实 3D draw 被兜底跳 ｜ 一、`+0x5B0` 之谜的完整证据链（推翻 h14xx「时序交错」旧结论） ｜ 二、修复（`src/arm/thumb_interpreter.cpp`，h144k18） ｜ 三、修复后的实测变化（`_b7_out.log` / `_b8_*` / `_b9_*`） ｜ 四、修复后画面仍为纯色，但定位已收敛 ｜ 五、下一步（优先级）

**发现/结论**（20 条，最多列 40）：

- 旧结论是「GFD 调度未就绪，float 归一化数组与指针槽在时间上交错」。本轮用**通用 guest 写监视** + **capstone 反汇编新增工具**把整条链钉死：
- 1. `P5D_KWATCH=816CC190-816CC1D0`（渲染器单例 `0x816CBC08 + 0x588..0x5C8`）抓到的写入顺序：
- - `pc=811E73C0 / 811E7440` 写 `+0x5B0/+0x5B4/+0x5B8…` 共 N 个 **有效显存指针**（`0xAA8B3C20/3E20/4020/…`，每次 +0x200），`lr=8120735C`；
- - **随后** `pc=811E7336` 写浮点 `0xBF4571F7`、`pc=811E7360` 写浮点 `0x3B532713`，地址覆盖 `0x816CC190..0x816CC1C0`，**把刚写好的指针全部踩成浮点**。
- 2. 新增解释器探针（`P5D_DBG_5B0=1`，PC=0x811E7202/73C0/734C/7D52）给出寄存器真值：
- - `[5B0-SB] r4(obj)=0x816CBC08 sb(r9)=0x816CC0F8`（= obj+0x4F0）；
- - `[5B0-NORM] sb=0x816CC0F8 cnt(r6)=25`，且**同一函数被连调多次，sb 每次 +0x34（13 float）**：
- `[0x816CC0F8,0x816CC15C)` → `[0x816CC12C,0x816CC190)` → `[0x816CC160,0x816CC1C4)` ← **第 3 级正好盖住 +0x5B0**；
- - `[5B0-PTR] … slot=0x816CC1C0 (readback=0x3B532713)` ⇒ 指针写入时槽里已经是浮点；
- - `[5B0-DRAW] r8=0x816CBC08 slot(r8+5b0)=0x3B532713` ⇒ `0x811E7D52 ldr.w r1,[r8,#0x5b0]` 拿到浮点 → `[VSTREAM-DUMMY]` 兜底把 r1 换成清零哑对象 → **真实顶点流被丢弃**。
- 3. 新增工具 `tools/dis_rng.py`（解析 SELF 内嵌 ELF + capstone Thumb2）反汇编 `0x811E72xx`：
- 811E73C0 str.w r0,[r6,#0x5b0]      ; 把 0x81207734 分配的 0x50 常量块指针写进 +0x5B0
- **半径 2 的 L1 球应有 13 个 tap ⇒ 数组按 13 float 步进正好首尾相接、互不重叠**；而解释器里 `vcmp/vmrs` 是空操作，条件标志保持上一条整数指令的残留 ⇒ **剔除分支永不成立，生成 25 个 tap**，每级多写 12 个 float，第 3 级就越界覆盖了 `+0x5B0`。
- ⇒ 「越界」不是时序问题，是**计算错误**；h14xx 的"真实 PSV 延迟渲染不会在此窗口绘制"结论**作废**。
- | `[5B0-PTR] readback` | `0x3B532713`（浮点） | **`0x0` → 真正被指针写入** |
- | `[5B0-DRAW] slot(r8+5b0)` | 浮点 0x3B532713 | **`0xAA8B3C20/0xAA8B4020/0xAA8B4420`** 每帧稳定 |
- - 首轮普查结果：**只有 3 个 NEON 未实现编码、各 1 次**（`FFA0/30B2@811A4E80`、`FFC7/0F10@811A810E`、`FFC7/4F10@811A812C`）⇒ NEON 覆盖面已经不错，**VFP(0xEE) 空间尚未普查**（下一步补）。
- - 全部 draw 跳过（`P5D_SKIP_F=frag_`）→ `rt_main_1` **全黑** ⇒ 黄**不是清屏色**，是某一路 draw 画出来的（后端 clear 已是中性黑）。
- - 跳过合成 `frag_0003` → rt_main **仍然纯黄** ⇒ 黄**不是**合成 pass。
- **工具新增（可复用）**：`tools/dis_rng.py`（SELF→ELF→capstone Thumb2 按 VA 反汇编，用法 `python dis_rng.py 811E7300+160`、`--hex` 看原始字节）——本项目第一个按 VA 反汇编工具，配合 `P5D_KWATCH`/`P5D_DBG_5B0` 形成「写监视 → 反汇编 → 定位缺指令」闭环。

---

## `docs\_append_h144k19.md`

**标题**：2026-09-21（h144k19）**整屏纯色的直接来源锁定 + 一个结构性大坑：顶点 UBO 与片元 UBO 抢同一个 binding0** ｜ 一、修复 vcmp/vmrs 之后，`rt_main` 仍是纯 (255,188,0) —— 用新装的二分开关把「谁画的」钉死 ｜ 二、稳态帧的真实绘制结构（`P5D_DBG_DRAWSEQ=1`，窗口 es=8100..8300，`cw>=900` 只留主 RT） ｜ 三、`frag_0000` 的 SPIR-V 只有一次 UBO 读取 —— 但 UBO 里不是 clearColor ｜ 四、结构性大坑（本轮最重要的结论）：**顶点 UBO 与片元 UBO 抢同一个 `binding 0`** ｜ 五、下一步（按优先级，明确可执行）

**发现/结论**（10 条，最多列 40）：

- | 跳过的管线 | `rt_main_1` 结果 | 结论 |
- - `[DRAWHIST frame=11760] totalDraw=33335 **real=33335 nohit=0 nocnt=0**`（旧基线是 `tot=80000 distinct=14`）⇒ 现在**每一路 draw 都命中真实管线**，管线数 9 条。
- 1580: OpStore %8(Output loc0) = %380             ⇒ 输出 = UBO[0][0]
- [UBODUMP] lastnm='clearColor' lastub=0xB3F69000 es=8004 floats: -nan(ind) -nan(ind) -2.000 1.000 0 0 0 ...
- ⇒ 清屏 draw（vert_0000）不在名单里 ⇒ 片元阶段拿到的是**顶点常量块**（其中恰好含黄色数据）⇒ 整屏黄。
- - **实验证据**：`P5D_FORCE_UBO=1,0,1,1`（把 binding0 整块填成品红）→ `rt_main` 变成**全黑而不是品红**——正因为顶点阶段也被喂了品红，全屏三角形顶点崩掉、什么都没画上（反证顶点阶段确实读 binding0）。
- - **这条同时解释了本项目长期悬案「frag_0005 的 bloom 常量源找不到」**：那些片元常量确实被 `SetUniformDataF` 写进了片元 UBO，而多数 draw 时片元阶段读的是顶点 UBO ⇒ 永远读不到。（此前排除的「4 个注册默认 UBO 全零 / 153 个 UBO 块 slots 7..11 全零」也都是在这个错误前提下取的证。）
- 抓写入者 PC，然后 `tools/dis_rng.py` 反汇编该 PC —— 与破 vcmp 用的是同一套闭环方法。
- 3. `frag_0003` 的 `PA[1]` 恒 0、`frag_0005` bloom —— **在 UBO 绑定修好之后重测**，之前的结论可能全部作废。
- 5. VFP/NEON 缺口普查结论：**NEON 只有 3 个未实现编码各 1 次；0xEE 空间经 capstone 反证无「静默跳过」**

---

## `docs\_append_h144k20.md`

**标题**：2026-09-21（h144k20）**顶点/片元 UBO 正式拆分为两个 binding + 全库重生成 4331 个 SPV + 修掉 `vcvt.f32.u32` 真 bug** ｜ 一、结构性修复：GXM 的顶点/片元默认常量缓冲本来就是两个 ｜ 二、实测效果（RTDUMP / BMP 证据） ｜ 三、顺着 `clearColor` 的数据链一路挖到底（硬证据链） ｜ 四、仍未解决 / 下一步（明确）

**发现/结论**（11 条，最多列 40）：

- 上轮（h144k19）已证实：GXM 有 `sceGxmSetVertexDefaultUniformBuffer` / `sceGxmSetFragmentDefaultUniformBuffer` **两个独立**缓冲，而离线生成器把两个 stage 的 UBO 都发成 `binding 0`，后端也只有**一个** UBO 槽 ⇒ 任何时刻只有一个 stage 能拿到正确常量。本轮正式拆开：
- | 校验 | 新工具 `tools/bind_chk.py`：实测新库 `frag_0000=[Binding 1]`、`vert_0000/vert_0015=[Binding 0]`、`frag_0084/frag_0005=[Binding 1, Binding 2]` ✅ |
- ⇒ **片元 UBO 通道已经真正打通**，剩下的问题只是「送进去的数据是垃圾」。
- 1. `[SETUNI2] 'clearColor' ... src=0x81514A90` —— 源地址**恒为固定全局 0x81514A90**（1053 次全同）。
- 3. `P5D_KWATCH=816CBE80-816CBEB0` → `0x816CBE94`（=渲染器单例 `0x816CBC08` **+0x28C**）的写入者：
- 4. 反汇编函数头 `0x811EA3F2`：这是一个**颜色归一化 setter**——
- 旧实现写成 `res=(float)(SceUInt32)a`（`a=fs(Sn)` 是浮点**值**）⇒ 例如位模式 `0x000000FF`（整数 255，作为 float 是 denormal 3.6e-43）被算成 **0.0**，颜色全部归零。
- 1. **`clearColor` 仍是 (NaN, NaN, -2, 1)**：写入者已锁定为同一条 `0x811EA440/448`（即上面那个归一化 setter），说明**送进去的 s0/s1 本身已是 NaN**。下一步：
- - 用 `P5D_DBG_CLRC=1`（本轮已加探针：函数入口打印 3 字节、0x811EA440 打印 s0..s3 原始位模式）抓现场（注意该函数在 ~90s 后才首次执行，需跑满 100s）；
- - 追两个调用者 `lr=0x8116347A`（0x8116344E 把 0xAARRGGBB 拆成 3 字节到栈，`[sp+3]` 未写）与 `lr=0x811A9A94`。
- 3. 旧卡点 `frag_0003` 的 `PA[1]` 恒 0 / `frag_0005` bloom 常量源，**必须在 UBO 拆分后的新基线上重测**（此前所有取值结论都建立在「片元读顶点 UBO」的错误前提上）。

---

## `docs\_append_h144k21.md`

**标题**：2026-09-21（h144k21）**目标①实质突破：`vert_0015` 顶点几何由全退化 → 正常四边形（740×411）** —— 根因是**5 个 VFP 解码 bug** ｜ 一、结果先行（这次是硬证据） ｜ 二、为什么之前一直查不到：`clearColor` 是一条能把解释器错误"照出来"的探针 ｜ 三、修复前后对照（`clearColor` 这条链） ｜ 四、当前画面状态与下一步

**发现/结论**（9 条，最多列 40）：

- [5B0-DRAW]  r8=0x816CBC08 slot(r8+5b0)=0xAA880C20   ← 顶点流槽是有效显存指针
- `SetUniformDataF(src=0x81514A90 固定全局)` ← `memcpy(d=0x81514A90, s=0x816CBE94=渲染器单例+0x28C)` ← 写入者 `0x811EA3F2`（颜色归一化 setter）。
- 用 `P5D_DBG_CLRC`（函数入口 + `0x811EA440` 两处探针）+ `P5D_KWATCH` 抓到的**寄存器真值**一路反推，挖出 5 个解释器解码 bug（全部用 capstone 逐位翻转/全遍历标定，新增工具 `tools/vfp_cal.py`、`tools/vfp_enum.py`）：
- | # | 位置 | 错误 | 实测症状 | 修复 |
- | 1 | `vcvt.f32.u32` | 源寄存器里是**整数位模式**，旧码却写 `(float)(SceUInt32)a`（a=浮点**值**） | 位模式 `0x000000FF`（整数255，作为float是 denormal）被算成 0.0 | `{SceUInt32 uv; memcpy(&uv,&a,4); res=(float)uv;}` |
- | 4 | **`vldr/vstr` 单精度寄存器号** | `sd=((hb&0x80)>>3)\|((hb&0x20)>>3)\|((hb&0x10)>>1)` —— hb 位序**反了**且漏了 hb.b6 | `vstr s2` 实际写 s8、`vstr s1` 实际写 s0 ⇒ **引擎所有单精度访存都在读写错误寄存器** | `sd=((hb&0xF0)>>3)\|((h>>6)&1)`（`hb=h2>>8`；sd[4:1]=h2[15:12]，sd[0]=h.
- `[CLRC-W]` 现场打印也证明 `vstr` 存的值与寄存器一致了（此前 `v=0x44080000` 而 `s2=0` 的矛盾消失）。
- 1. 用 `P5D_DBG_UBODUMP` 扩展打印 **`vert_0015` 的顶点默认 UBO**（矩阵）前 16 个 float，确认变换矩阵是否合理（`vldr` 刚修好，矩阵装载此前也是错的）；
- 3. 复核 `frag_0003` 的 `PA[1]` 恒 0 与 `frag_0005` bloom 常量（旧结论均建立在解码错误的前提下，需重测）。

---

## `docs\_append_h144k22.md`

**标题**：2026-09-21（h144k22）**主 RT 从「全黑」变为「有内容」：定位到通用顶点解包缺「像素→NDC」变换**（实验验证成功） ｜ 一、承接 h144k21：顶点几何已正常，但主 RT 仍全黑 —— 卡点在坐标系 ｜ 二、根因：通用顶点解包没有补「像素→NDC」，只有 `vert_0014` 的专用分支补了 ｜ 三、实验验证（`P5D_PX2NDC=1`，只对 vert_0015 生效） ｜ 四、但要诚实说明当前程度（放大核对后的结论） ｜ 五、下一步（明确）

**发现/结论**（11 条，最多列 40）：

- 但 `rt_main_1/3` 仍全黑。新增 `P5D_DBG_VTX=1` 探针（打印 `vert_0015` 顶点流前 3 顶点原始数据）抓到**决定性证据**：
- [VTX15] va=A2A737F0 cw=960 ch=544 stride24:
- ⇒ 顶点是**像素坐标**（288~1020, 165~572，目标面 960×544），而 `vert_0015` 的 SPIR-V 是
- `gl_Position.xy = 顶点属性.xy` **直通**（已用 `spv_inspect` + 常量表确认 `%27=0.0`、`%28=1.0`，即 `(x*1.0, y*1.0, ..., 1.0)`）。
- 在 GL/GXM 的 `[-1,1]` clip 约定下 `|x| > w` ⇒ **整个四边形被裁剪掉**，主缓冲 53% 的绘制量零片元 ⇒ **主 RT 全黑的直接原因**。
- `eboot_probe.cpp` 的 `[h141d] 统一展开 stride128` 段里，`vert_0014` 专用分支**有**这个变换：
- `vert_0015` 走的是通用分支（它的权威属性表已采集）⇒ 位置保持像素坐标 ⇒ 被裁剪。
- **⇒ 坐标系判断得到证实：主 pass 的卡点确实是「像素坐标未转 NDC 被裁剪」。**
- ⇒ **目标（真实游戏画面）仍未达成**；本轮的价值是把「主 RT 为什么黑」从一个模糊的几何问题**收窄成了明确的坐标系问题并验证**。
- 现只对 `vert_0015`(0x172A9E736C7DC35E) 生效；其它「着色器直通 position」的管线（`vert_0000`/`vert_0014`/`vert_0006`/`vert_0007`/`vert_0008`）
- 2. **查 present 面与 `rt_main_2` 不一致**：`dbg_rtA`（present）是密点噪声、`rt_main_2` 是 24% 实体块 ⇒

---

## `docs\_append_h144k23.md`

**标题**：2026-09-21（h144k23）主力 pass 深挖：**定位到 z 裁剪**，并用「换着色器 / 换目标面回读」把嫌疑收窄到管线-渲染通道层 ｜ 一、逐管线坐标系普查（`[PXCENSUS]`）——确认只有 vert_0015 需要像素→NDC ｜ 二、**发现第二个裁剪原因：z**（本轮最硬的新结论） ｜ 三、修完 x/y 与 z 之后仍然全黑 —— 用四个单变量实验把范围锁死 ｜ 四、另一个必须先解释的矛盾（下轮第一件事） ｜ 五、下一步（明确、按优先）

**发现/结论**（13 条，最多列 40）：

- | 顶点程序 hash | 代表 pass | loc0x/y 范围 | 结论 |
- | `172A9E736C7DC35E` | **vert_0015（主力）** | x[284.2,1024.9] y[163.3,574.3] | **像素坐标** ⇒ 必须补像素→NDC |
- ⇒ 上轮 `P5D_PX2NDC` 只覆盖 vert_0015 **方向是对的**；但判据要写成「值域 >8 或整体 >1」，
- x/y 已经正确转成 NDC，但 **z = 2.125 而 w = 1** ⇒ Vulkan 裁剪要求 `|z| ≤ w`，
- | 实验 | 结果 | 排除/确认 |
- | …再加 z 映射 | 全黑 | 排除「z 是唯一原因」 |
- | …再把片元换成**红色诊断着色器**(`P5D_REALFS=r`) | 全黑 | **排除片元着色器输出问题** |
- | …再把顶点换成**内置 passthru**(`P5D_DBG_VS_BUILTIN=1`)+红 FS | 全黑 | **排除两个着色器的 SPIR-V** |
- | `P5D_RTDUMP_CD=A159C000,A0F7C000` 回读**它真正的绘制目标** | 全黑 | 排除「看错面」 |
- ⇒ 顶点内存拷贝 OK、索引 OK、命令缓冲在 render pass 内 OK、管线非空。
- - `P5D_SKIP_F=frag_0084`（跳过主力 pass，其余全放行）+ PX2NDC ⇒ `rt_main_2` 有 **51857** 次颜色变化（有内容）
- - `P5D_ONLY_F=frag_0084` / `P5D_ONLY_F=frag_0003`（只放行一路）⇒ 主 RT **全黑**
- 两者矛盾 ⇒「有内容」既不是 frag_0084、也不是 frag_0003 单独产生的。必须先把「哪一路在写主 RT」钉死（计划：给 `gxm_draw_indexed_real` 加一个「按 passCD 累计写入 draw 数」的计数表，而非依赖跳过后回读）。

---

## `docs\_append_h144k24.md`

**标题**：六、（h144k24）追加排除项：管线 × render pass 兼容性 —— **不是原因**

**发现/结论**（3 条，最多列 40）：

- **结果**：`[REAL-PIPE] #` 仍然只有 **9 条**（= (v,f) 组合数）⇒ 说明实际运行中每条管线只用过一个 RP，
- 至此，主力 pass「零像素」已排除：像素坐标(x/y)、z 裁剪、片元着色器、顶点着色器 SPIR-V、看错目标面、
- 「按 passCD 累计 draw 次数 + vkCmdDrawIndexed 后对目标 image 做一次 1 像素探针回读」的直接证据，

---

## `docs\_append_h144k25.md`

**标题**：2026-09-21（h144k25）**转折：找到画面有内容的帧区间；红片元证明几何完全正常；噪声根因 = RT 别名采样的 layout 未转换** ｜ 一、先纠正一个此前一直误导排查的前提：**画面只在 frame 100~500 有内容，600 之后全黑** ｜ 二、红片元实验：**几何完全正确，噪声来自片元着色器的纹理采样** ｜ 三、根因：**RT 被当纹理采样时 layout 停在 COLOR_ATTACHMENT_OPTIMAL**（`[ALIAS-SKIP]`） ｜ 四、下一步修复方案（明确） ｜ 五、本轮新增诊断

**发现/结论**（7 条，最多列 40）：

- ⇒ **窗口在启动后约前 500 帧确实在显示内容**（非黑约 26%），600 帧后转全黑。
- 对照实验（关掉 `P5D_PX2NDC`）同样是 13.6 万非黑 ⇒ **这些内容与我的坐标系实验无关，是引擎本来就渲染出来的**。
- ⇒ vert_0006 是**全屏后处理/合成 pass**（UV 到 0.938，典型的「宽 960/1024」归一化）。
- ⇒ **几何 100% 正确、全屏覆盖**；**噪声完全来自片元着色器采样到的纹理内容**；
- 并且「600 帧后变黑」在红片元下变成「600 帧后仍纯红」⇒ 那也是**片元侧（采样结果）**的问题，不是几何停止。
- ⇒ 当引擎在**同一帧内既渲染某张面、又把它（或同池的面）当纹理采样**（frag_0003 的 `out = PA0+PA1+PA2` 自反馈求和，
- **教训**：固定时刻抓帧会系统性漏掉**瞬时/早期**画面；用户肉眼反馈「白点和线」直接把排查方向从「几何」纠正到了「采样」。

---

## `docs\_append_h144k26.md`

**标题**：六、（h144k26）**已实施修复：RT 采样快照副本** —— 满屏噪声根因消除 ｜ 七、本轮方法论收获（写进铁律）

**发现/结论**（4 条，最多列 40）：

- **实测结果**（`P5D_DBG_FRAMES=1`，同前一轮的对照条件）：
- ⇒ **「RT 被当纹理采样时 layout 非法」这个根因被彻底消除**：画面从「满屏随机小方块噪声」变成「干净的底色」，
- 导致连续多轮都在"全黑"这个假前提上排查。`P5D_DBG_FRAMES=1` 连续采样一次就纠正了方向。
- 3. 「换最简着色器」是分离几何/片元问题的最高性价比手段：几何正常而画面异常 ⇒ 问题必在片元输入（纹理/常量）。

---

## `docs\_append_h144k27.md`

**标题**：2026-09-21（h144k27）**纠正一处数据误读 + 发现引擎应用状态机只推进到 state=6 就不动了** ｜ 一、先纠正我上一轮的一处误读（重要） ｜ 二、同帧 cross-check：swapchain 抓帧与 RT 池回读**一致**（回读链路可信） ｜ 三、**本轮最重要的发现：引擎应用状态机只走到 state=6 就不再推进** ｜ 四、下一步（方向已改变）

**发现/结论**（11 条，最多列 40）：

- 实测复核（同一指标「非黑像素数」）：
- ⇒ **快照修复确实把「未定义采样噪声」彻底消掉了**；画面变成「干净的黑」。这一点没有反复。
- ⇒ 两者一致，`save_rt_screenshot` 的回读是可信的（此前"看错面"的怀疑可以排除）。
- [ST-CHANGE] state -1 -> 1  updateFn=0x81185927
- [ST-CHANGE] state  1 -> 6  updateFn=0x81185987
- [ST-ENTER]  state=6 enterFn=0x81185975 arg=0xB0EDD01C
- 即：状态机 `-1 → 1 → 6` 后**停住**。而 `0x81185975`（Thumb 指针，真入口 `0x81185974`）反汇编是一个正常的小函数
- 1. **先确认 state=6 是「卡住」还是「正常长时间等待」**：把 `[ST-UPD-CALL]` 的限流改成
- 2. 若是卡住：查进入 state 6 的条件（状态表 `0x813B11AC`，每项 0x50，enter=+0x44 / update=+0x48）
- 以及 state 6 的 update（`0x81185987`）在等什么——这决定了"真实游戏画面"能否出现。
- `[ST-UPD-CALL]` 限流 48 次恰好只覆盖了 state=1 那段，差点让我得出"引擎卡在 state=1"的错误结论；

---

## `docs\_append_h144k28.md`

**标题**：2026-09-21（h144k28）**参考 Vita3K 权威源码**：拿到 GXM 窗口坐标公式，发现我们完全忽略了视口 offset/scale ｜ 一、权威确认（与我们的实现一致的部分） ｜ 二、**权威给出 GXM 的窗口坐标公式 —— 而我们的视口实现是错的/缺失的** ｜ 三、由权威公式反推出的一个硬结论（反证我们的顶点数据有问题） ｜ 四、下一步（按优先）

**发现/结论**（13 条，最多列 40）：

- ⇒ **与我们在 `eboot_probe.cpp` 里用的 `_fsz` 表完全一致** ✓（这一块不用再怀疑）
- 另：`is_stream_instancing(source)` = `EACH_INSTANCE_16BIT || EACH_INSTANCE_32BIT`；顶点流大小按 `max_index * stream.stride` 推算（与我们一致）。
- **对照我们的实现**：`eboot_probe.cpp` 的 `n_set_viewport`（NID `0x3EB3380B`）只是打印诊断，**从未把 offset/scale 传给后端**；
- ⇒ **引擎设置的正交投影（offset/scale）被我们整体丢掉了**，这是"几何落不到正确位置"的一个明确缺陷。
- **而且该 HLE 的参数解析本身就可疑**：实测日志
- [VIEWPORT#1] ENTER ctx=0xA0ABE000 vp=0x0 contains=0
- [VIEWPORT#2..6] ENTER ctx=0xA0ABE000 vp=0x4 contains=0
- 说明 `0x3EB3380B` 的签名不是「(ctx, viewport*)」，或参数在别的寄存器。**必须先把它的真实签名查清**。
- 而我们实测主力 pass 上传到 GPU 的顶点是：
- guest 原始数据是**像素量级的 (288.94,165.69) 而 w=1** ⇒ `x/w = 288.94`，在真机上**必被裁剪**。
- ⇒ **结论：这不是真机数据，说明我们的顶点解析（或 w 分量的来源）仍有问题**，
- 1. **查清 `0x3EB3380B` 的真实签名与 NID 归属**（Vita3K 的模块实现文件不在本地副本里，需另找；
- 那么顶点本就该是 [-1,1] 的量级 ⇒ 我们读到的 288.94 说明**读错了缓冲/stride/offset**，

---

## `docs\_append_h144k29.md`

**标题**：八、（h144k29）Vita3K 权威源码核对结果 —— 我们的像素→NDC **方向是对的**，但视口 offset/scale 仍未实现 ｜ 8.1 权威给出 GXM 顶点位置处理（`shader/src/spirv_recompiler.cpp:1663-1717`） ｜ 8.2 权威给出视口换算，而我们的实现完全忽略引擎 offset/scale ｜ 8.3 本轮实验（回归 + 定位） ｜ 8.4 下一步（按优先）

**发现/结论**（12 条，最多列 40）：

- // Transform screen space coordinate to ndc when viewport is disabled.   ★
- store(out_var.w, out_var.z);   // ★ z = w，避开深度/裁剪
- **结论 1**：**Vita 上确实存在"顶点位置就是屏幕空间坐标"的模式**，触发条件是 **viewport 被禁用**；
- **结论 2**：把我们的实测值代进去 —— `x=288.94, width=960` ⇒ `x_ndc = 288.94*2/960 - 1 = -0.3980`，
- ⇒ **此前 h144k28 里「PX2NDC 方向可能错了」的怀疑撤销：方向与权威实现一致**。
- 我们的 `n_set_viewport`（NID `0x3EB3380B`）**只打印诊断、从不把 offset/scale 传给后端**，后端永远只设全屏视口。
- 且实测该 HLE 的 r1 取值是 `0x0 / 0x4`（`contains=0`，不是有效指针）⇒ **签名不对，必须查清**。
- | 实验 | 结果 | 结论 |
- ⇒ **当前最关键的未知量**：`g_dbg_pass_color`（绘制目标）与 `present_frame(cd)`（显示目标）**到底是不是同一张 image**。
- 排除"回读/present 取了另一张 image"这一可能；若确认命令确实没生效，再回到 framebuffer/viewport 层面查。
- 2. 查清 `0x3EB3380B` 的真实签名（本地 Vita3K 副本不含模块实现文件），**实现真正的 viewport offset/scale + viewport_flat + viewport_flip**。
- 并纠正了我上一轮基于推理的错误怀疑。

---

## `docs\_append_h144k30.md`

**标题**：九、（h144k30）**渲染链路全链路验证通过** —— 把"画面黑"的怀疑从渲染层彻底排除 ｜ 9.1 实验：全 draw 插绿块 + 同帧对拍 ｜ 9.2 由此得到的四条硬结论 ｜ 9.3 引擎侧现状（回到 h144k27/28 的线索） ｜ 9.4 下一步（重心转向引擎侧）

**发现/结论**（12 条，最多列 40）：

- ⇒ 此前"绿块插入了却看不到"的困惑，是因为当时用 `P5D_ONLY_F=frag_0084` 单独放行，绿块打在 A159C000，
- 2. **`[PRES]` / `[RTSHOT]` 的 `img` 指针逐一对应** ⇒ 「绘制目标 ↔ 显示目标 ↔ 回读目标」**是同一张 image**，
- 4. ⇒ **整条链路（guest draw → 管线 → render pass → framebuffer → image → copy → swapchain → 抓帧/回读）全部正确**。
- 这条结论把此前所有渲染层的怀疑（几何、管线、RP 兼容性、layout、采样、坐标变换、回读、present 路径）
- **全部一次性排除**，排查重心应当回到**引擎侧**。
- `if (0x8116F900(0x8130B140,0) != 0) return 1(keep); else return 0(结束)`，实测一直 keep。
- - state 6 的 enter（`0x81185974` → `0x8119807C`）反汇编显示它**创建/初始化了一个 0xa4 字节、flag=1 的对象**
- （`0x8130B100` 为回调表、`0x8115d3ce` 创建），update 则在**轮询该对象是否就绪**。
- ⇒ 形态上是「**等待某个异步任务/资源完成**」。
- 1. **追 state 6 在等什么**：反汇编 `0x8119807C`（创建）与 `0x8115d3ce` 的语义，
- 并给 `0x8130B140` / `0x8130B100` 两个全局对象加**读写监视**（`P5D_KWATCH`），
- 2. 若确认是**资源加载未完成**，则与文件 IO / CPK 加载链（h144k 早期已经打通过一轮）对接，

---

## `docs\_append_h144k31.md`

**标题**：十、（h144k31）长跑确认引擎停滞 + 排除"等待输入" + 方向调整（移植 ≠ 模拟器） ｜ 10.1 5 分钟长跑：引擎不会自行走出停滞 ｜ 10.2 排除"引擎在等玩家按键" ｜ 10.3 引擎侧现状汇总（都是"稳态循环"的特征） ｜ 10.4 方向调整（用户提醒：本项目是**安卓原生移植**，不是模拟器） ｜ 10.5 下一步

**发现/结论**（12 条，最多列 40）：

- ⇒ 引擎**不会自己走出来**；这一帧全屏蓝很可能是某个 pass 输出常量（历史上记录过的 "CONSTOUT 蓝"）。
- `n_ctrl_peek` 此前**恒返回"无按键"**（每次把 pad 缓冲清零），因此"引擎停在标题画面等按键"是一个必须排除的假设。
- `timeStamp8 + buttons4 + stick4 + reserved16 = 0x20`）。实测：
- ⇒ **引擎不是在等输入**。（该开关保留作诊断；正式实现应接 Android 原生输入，而不是靠模拟按键。）
- | `[FLAGS-W]` | `0x03 ↔ 0x13` 无限交替（pc=0x81198CF0/0x81198D02） | 正常的帧心跳，**不是**推进 |
- | `[OBJRDYRET]` | `ready(r0)=0x0`，对象 `[impl+0x28]` 恒 `0x80000000`(bit31=忙) | 某个对象始终"未就绪" |
- | `[JOBLIFE]` / `[RDYQ]` | **0 条** | 0x811CA78E(投递)/0x811CB10C(完成清bit31)/0x811CA9F0(就绪查询) 从未被调用 |
- | `[TASKSTEP]` | 两个 GFD 任务节点互相循环（`state=-1` / `0x7FFFFFFF`，`next` 成链） | 任务链表在稳态轮转 |
- | 管线构成 | 9 条全是 **全屏三角清屏 / 全屏 quad 后处理 / bloom / 合成** | **没有任何"3D 模型"管线** ⇒ 引擎尚未进入"画游戏画面"的阶段 |
- 这条提醒很关键，它纠正了我使用 Vita3K 的方式：
- 目标：查清 `0x3EB3380B`（我们当成 `sceGxmSetViewport`）的真实签名与所属函数，
- - state 6 的 enter 创建的对象（`0x8119807C` → `0x8130B100` 回调表 + `0x8115d3ce`）语义；

---

## `docs\_append_h144k32.md`

**标题**：十一、（h144k32）**决定性排除：vert_0015 的问题不在顶点数据，而在 draw 调用层面** ｜ 11.1 先澄清两个此前可能误判的点（用直接扫描 SPIR-V 的方式） ｜ 11.2 决定性实验：把 vert_0015 的顶点**强行替换成全屏 NDC** ｜ 11.3 排除 render pass 兼容性（权威且彻底） ｜ 11.4 与 h144k30 的 MARK 实验合起来看，问题被夹在一个很窄的区间里 ｜ 11.5 下一步（很具体）

**发现/结论**（7 条，最多列 40）：

- ⇒ **`frag_0084` 有 Output（`colorWriteMask=0xF`）、也不含 `OpKill`**；只有 `frag_0079` 确实无输出（已知）。
- **⇒ 排除"颜色写入掩码为 0"和"片元被 discard"这两个假设。**
- ⇒ **顶点数据完全合法、覆盖全屏、片元恒红，仍然一个像素都没有**。
- **⇒ 结论：vert_0015 画不出来的原因不在顶点数据（也不在坐标系/PX2NDC），而在「这条 draw 的提交层面」。**
- ⇒ **RP 兼容性不是原因**（h144k24 的修复保留，但它不是本问题的根因）。
- | `[MAINDRAW] copyok=1 / inRP=1 / match=1 / 管线非空`、`[RPCHK] match=1` | 实测 |
- ⇒ **问题精确落在 `gxm_draw_indexed_real` 内部、`vkCmdDrawIndexed` 之前的"绑定"环节**，候选只有三个：

---

## `docs\_append_h144k33.md`

**标题**：十二、（h144k33）把 vert_0015 零像素的五个环节**逐一证真** —— 得到一个"完美矛盾" ｜ 下一步（很具体）

**发现/结论**（8 条，最多列 40）：

- 按上轮列的三个候选（索引缓冲 / 管线绑定 / 描述符集）逐项加了**精确校验**，结果全部正确；再加上此前的排除，现在五个环节都被证真：
- | **顶点数据** | `P5D_V15_FULLSCREEN=1` 把顶点强行改写成覆盖全屏的合法 NDC + 红片元 | **仍然全黑** ⇒ 与顶点数据无关 |
- | **多线程踩踏** | `[DRAWPIPE]` 打印线程 id | 全部同一 tid ⇒ 单线程，`g_current_real_pipe` 不会被别的线程覆盖 |
- ⇒ **命令缓冲在 pass 内、管线是本条、索引正确、顶点合法（全屏）、描述符集正常、单线程 —— 但 `vkCmdDrawIndexed` 依然一个像素都不写。**
- 这是一个**"完美矛盾"**，它的价值在于：**说明我对"链路已经正确"的某个前提仍然有错**，而不再是"某处没设对"。排除到这一步后，剩下的可能性已经很窄：
- 3. 管线创建时的**顶点输入布局**（stride 128 / location 0..7 各 16B）与实际写入的 128B 打包是否真的一致
- 2. 在 `gxm_draw_indexed_real` 里**按管线声明的 stride/location/offset 规则**，从 `vertex_buffer_mapped+_vbo` **反解 loc0** 并打印，
- （stride/format/offset/location 八项），与"打包规则"逐项核对。

---

## `docs\_append_h145k442_h447.md`

**标题**：h145k442–h145k447：把「引擎自己画了什么」这件事重新测准（★含一次重要自我更正） ｜ 0. 本轮结论速览 ｜ 1. h145k442：池内 RT 亮度判决（RTMEAN） ｜ 2. ★★★ h145k444：去掉一切诊断后的"纯净扫描"，推翻我此前所有"看到内容"的印象 ｜ 3. h145k445：`P5D_MOVIE_FS=AUTO`（guest 请求驱动 + 播完自动关闭） ｜ 4. h145k446/447：引擎自绘内容的"放大碎片"钉到数值级 ｜ 4.1 UVCPY 判决（否掉"属性槽错配"） ｜ 4.2 新探针 `[CDGEO]`（内容窗口 2245–2295 内每次内容 draw 的 loc0/loc1/loc2 范围） ｜ 5. ★ 应用层加载队列：32 条只完成 5 条（很可能是标题画面出不来的原因） ｜ 6. 待办（下一轮）

**发现/结论**（22 条，最多列 40）：

- 按墙钟 30fps 全屏彩色输出（BT.601 限幅）⇒ 位置/颜色/尺寸/速度全部正确。**已验证**。
- 2. **★ 重要更正**：h145k435/h436 里那些"LOGO / 主视觉 / 标题画面"**全部是我自己的诊断 blit 画的**，
- **去掉一切诊断后**（h145k444，纯引擎）实测：0–3300 帧内引擎**只在帧 2252–2286 画过一次内容**，
- 一直挂在队列里、管理器 `busy=1` 不变 ⇒ 这**很可能就是"标题画面/主视觉出不来的环节"**（见 §5）。
- FAKE_VDEC=1 + `P5D_RTMEAN=60`，跑到解码之后（帧 3013–3020 已确认 7 次 Decode）：
- ⇒ 解码阶段**池内各 RT 全黑**。
- **但这条证据要打折**：`[TEXMAT]` 里对"RT 别名"纹理的 `nz=0/4096 mean=0.0` 是**读 guest 内存**得到的，
- 所以"RT 全黑"只能作为**弱证据**，不能单独定罪。
- 对照 h145k435/h436 的日志：那两轮**都有** `[MOVIEBLIT] … 全屏 blit` ⇒ 当时窗口里看到的东西
- - `thumb_interpreter.cpp` 的 `[MOVIEOPEN]`（0x811588F6，app 的 `openMovie(player,name,…)`）现在把
- 证据文件：`p5d_seq_3020.png`（红色天空+楼群+彩色纸屑）、`p5d_seq_3260.png`（碎裂水晶闪光）
- ⇒ **对内容 pass 而言，属性槽错配这条假设被否掉**（上一轮"SAVE 徽章修正"的解释需要重审）。
- `eh=172A9E736C7DC35E`（顶点程序） = 内容 pass，`fh=6952EF6C507D515E`（片元）。实测（变换**之后**）：
- ⇒ **尺寸达到 1.9–2.4 倍屏幕**。这就是"只看到一块放大碎片"的直接原因：quad 比屏幕大得多。
- 3. `loc1`（frag_0084 的隐式采样坐标槽）在**所有顶点上恒为 (1,1)** ⇒ 若采样真的走 loc1，quad 只会是**单色**；
- 而实测有图案 ⇒ 采样坐标实际来自 `loc2`（`l2=(0.0020,0.0039)…(0.7441,0.3750)` = 图集里的一块 UV 矩形）。
- 按 Vita3K 语义此时**顶点程序应当输出 NDC**；而原始 loc0 是 ~2000 量级 ⇒ **坐标空间与语义不一致**。
- **⇒ 单一全局倍数不可能同时正确 ⇒ 真正可疑的是"像素→NDC"这条启发式本身**（对内容 pass 应当关掉或改成读应用自己的变换）。
- [LRQDUMP q=3300] mgr=0xB1C19030 busy=1 n=32 PENDING: 15:MUSIC_TITLE_P5 | 16:BG_TXST02 | 17:HELP |
- ⇒ 高度怀疑：**标题/主视觉那批资源的加载一直没完成**（`busy` 恒 1），所以标题场景永远画不出东西。
- 下一步要查的是**完成路径 0x8117C486 由谁驱动**（IO 回调 / worker 线程 / job 队列）。
- 2. 查加载完成路径（0x8117C486 的驱动者）：把"32 条只完成 5 条"变成可修的具体环节。

---

## `docs\_append_h145k448_h458.md`

**标题**：h145k448–h145k458：默认值落地 + 「引擎只画了一个 SAVE 徽章」+ 加载链路钉到 app 自己的 .spr 解码 ｜ 0. 本轮成果速览 ｜ 1. 默认值改动（h145k449） ｜ 2. `[CDGEOF]` 内容逐帧普查（h145k450/452） ｜ 3. ★★ 加载链路完整取证：卡在 app 自己的 `.spr` 解码推进 ｜ 3.1 worker → 按类型处理器（`[LHANDLER]`，h145k454） ｜ 3.2 精灵处理器与协程（`[SPRH]/[SPROBJ]/[SPRCORO]/[SPRSTEP]`，h145k456/457） ｜ 3.3 待判定（长跑 h145k458 正在跑） ｜ 4. 新工具（本轮自建，后续都能用） ｜ 5. 下一轮（按优先级）

**发现/结论**（35 条，最多列 40）：

- 2. 实测徽章 bbox `x[842,942] y[415,518] = 101x104`、中心 `(892,466)`（参考 `74x74`、中心 `(894,466)`）。
- 但只有 2252–2286 这 35 帧有像素 ⇒ 它画的就是**SAVE 徽章**（小图标 + 动画），别的时候 alpha/内容不可见。
- 4. **★ 加载卡点钉到 app 自己的 .spr 流式解码**（见 §3）——这是"标题/帮助/加载画面贴图永远不出现"的环节。
- - 展示轮（无环境变量）实测：
- [MOVIEFS] 影片播完（共 3084 帧 @ 30.0 fps）⇒ 关闭全屏覆盖，交回引擎内容
- - ⇒ "内容只会偶发爆发"这个旧印象要改：**内容 pass 一直在画，只是通常不可见**。
- worker `0x8117BA30` 尾部 `0x8117BB8A blx r5`（r5 = 资源注册表条目里的**类型处理器**）：
- `cmp r0,#0; bne 完成` ⇒ **处理器返回 0 就永远不完成**。实测分工：
- 0x8117C8A1  数据/归档类（DRESS_FORMAT / MUSIC_SPLIT / P5MSGARC / SENTAKUSI_WIN …）→ r0=1  ✅完成
- 0x8117C7E7  精灵类（P5SPR_BUTTON / P5SPR_MIND / P5SPR_HERO / BG_TXST02 / HELP_P5 …）→ r0=0  ❌永不完成
- 0x8117C93B  另一类（空名字，9 次）
- ⇒ **卡住的全是"贴图/精灵"，正常完成的全是"数据"**，与 h145k07 的观察一致，但这次拿到了"谁返回 0"。
- 0x8117C7E6(phase=0, ctx, req):
- if (req[0x3C]==0) req[0x3C] = 0x8102ED76("resource.spr", req[0x34]=ARC条目数据);
- return 0x8102EF00(req[0x3C]);          // 每帧轮询
- 0x8102EF00(obj):
- if (obj[4]!=0 && 0x8115FC0E(obj[4])==0) return 0;
- if (obj[0x28]==0) obj[0x28] = 0x8103067C() 决定的协程(1→0x8102F487 2→0x8102F6AB 3→0x8102F801);
- if (协程(obj,1)==0) return 0;           // ★ 第二段：实测这里返回 0
- 0x8102F800(obj, phase=1)：                  // 协程第二段（.spr 解码推进）
- if (条目流==0) 条目流 = 0x8116332A(arcData + 偏移);   // 建流
- if (0x81163170(条目流)==0) return 0;                  // ★★ 推进失败 ⇒ 整段返回 0
- [SPROBJ] obj=0xB211C01C [0]=0x2 [4]=0x0 [0x20]=0xB24AB028(ARC数据) [0x28]=0x8102F801
- [SPRCORO] 即将调用协程 r1=0x8102F801  →  [SPRCORO-RET] r0=0x1 (有进展)
- [SPRSTEP] 0x81163170 返回 r0=0x0（几乎每次都 0，少数几次 1）
- [SPRRET] 0x8102EF00 返回 r0=0x0（未完成）
- ⇒ **根因定位：`0x81163170(流)`（= `0x81163FA4` → `0x811CA9F0`，app 自己的 .spr 解码器推进）返回 0，
- `0x81163170` 的 0 是"**这一帧还没解完**（流式、每帧有限步）"还是"**永远解不完**"：
- - 若是前者 ⇒ 只是**极慢**（每帧推进一步），把运行时间拉长（CAP=1.2e10，跑到 ~14000 帧）应能看到
- 贴图终于加载、标题/帮助贴图出现 ⇒ 与 h145k10「不是死锁、是极慢」的结论一致；
- - 若是后者 ⇒ 在 `0x8116332A/0x81163170` 这条解码器链里还有具体死点（下一步用 `[SPRDEC]` 打数据头与流的字段）。
- - `_h145k453_dis.py`（capstone）：按 `VA-0x81000000+0x10E0` 反汇编 eboot 的 Thumb-2 代码；
- **注意奇数地址是 Thumb 位**（`0x8102F801` 要按 `0x8102F800` 反汇编，否则解出垃圾）。
- - `_h145k455_str.py`：读 guest 字符串（用于辨认断言/文件名，例如确认这是 `app_spr.c` 精灵系统）。
- 3. 若"死"：用 `[SPRDEC]`（已埋）打 `0x8116332A` 的数据头 + `0x81163170` 流的字段，继续往解码器内部钉。

---

## `docs\_append_h145k459_h461.md`

**标题**：h145k459–h145k461：影片为何永不结束 + 「播完再推进」的组合方案 ｜ 1. h145k458 长跑（无 hack）：引擎流程全貌 ｜ 2. h145k459：`FORCE_MOVEND` 长跑 —— 证明"卡过影片后引擎会继续"，但随后崩溃 ｜ ★ 影片 #1 卡住的直接原因（新） ｜ 3. h145k461：把"推过影片"改成**在我们自己的全屏影片播完之后**（组合方案） ｜ 4. 工具补充

**发现/结论**（17 条，最多列 40）：

- - 行 19172：`hash=0x6A29 'initDataRead'`（node 0xB1C1E01C）被摘除。
- ⇒ 与 h145k10 的结论一致：**不是死锁，是极慢**。
- ⇒ **"标题/主视觉"在影片之后**，所以"影片永不结束"= 永远到不了标题场景。
- [APPOLL] ap=0xB493801C [ap+4]=0x1 [ap+8]=0x815538C0 cri[+4]=0x1 cri[+0xC]=0x0
- [CRISTATE] player=0x815538C0 state=[+4]=0x1 streamer=[+8]=**0x0**   ← ★ CRI 播放器的 streamer 根本没建
- [TITLEMOVIEPLAYER] #1 factory called → [MOVIEOPEN] #2 name='movie/c_p5d001.usm'   ← ★ 第二部影片（标题 LOGO）
- [APPOLL] ap=0xB495901C [ap+4]=0x5 cri[+4]=0x2
- [CRISTATE] state=[+4]=0x2 streamer=[+8]=0xB493B028                ← 这部片的 streamer 建起来了
- [ERROR] ThumbExec badmem addr=0x0000000C @pc=0x810160CE sp=0xA016DF88 lr=0x81207351 r0=0x0 r1=0x1
- 2. **`FORCE_MOVEND` 一上来就谎报"已结束"会把状态弄脏，最终崩在 `0x810160CE`（空指针 +0xC）**，
- 与旧文档的警告一致 ⇒ 不能作为正式路径。
- 而影片 #2 的同一字段是 `0xB493B028`、状态 `cri[+4]=0x2`。
- ⇒ **影片 #1 的 streamer 没有建起来 ⇒ CRI 播放器永远不进入"播放中" ⇒ 影片永不结束。**
- （`0x81158F02(player,name,arg)` 的尾部会把 `player[4]|=1`；而"建流"是在 `player[8]` 这个对象上做的，
- - `thumb_interpreter.cpp` 的 `0x81158A60` 探针现在 `_act = 环境变量 || p5d_movend_armed()`：
- （h145k459 实测崩点 `pc=0x810160CE lr=0x81207351`）——本轮要确认"播完再推"是否更干净。
- - 引擎自绘内容证据图：`_h145k449_keep/engine_badge_zoom.png`（帧 2280 徽章放大）、

---

## `docs\_append_h145k461_h465.md`

**标题**：h145k461–h145k465：组合方案打通流程 + title 场景出现新绘制管线（以及一处结论修正） ｜ 1. h145k461【组合方案验证成功】AUTO 影片播完 → 自动把引擎推过影片这一环 ｜ 2. h145k462/464 长跑（影片快进 `P5D_MOVIE_FPS=600` 直奔 title 场景） ｜ 2.1 title sequence 完整走完并在 2/3 循环 ｜ 2.2 ★ 结论修正：不是"从不提交场景几何"，而是"后期的绘制没走通用解包路径" ｜ 3. 下一步（明确）

**发现/结论**（12 条，最多列 40）：

- 默认配置（无任何环境变量）实测时间线（`_h145k461_combo`）：
- L56099 [MOVIEFS] 影片播完（共 3084 帧 @ 30.0 fps）⇒ 关闭全屏覆盖
- 全程 **无崩溃**（对照：`P5D_FORCE_MOVEND` 一上来就谎报结束，最终崩在 `badmem addr=0x0000000C @pc=0x810160CE lr=0x81207351`）。
- ⇒ **"影片播完再推"是干净路径**；只在一开始就跳过会脏。
- ⇒ **title 场景确实提交了新的绘制**（不是"什么都没画"）。
- ⇒ **新绘制的顶点程序没有走我们的通用解包/变换路径**（大概率是属性布局没识别/解包失败），
- 因此它们的几何很可能没有被正确提交/变换 ⇒ 画面看不到。
- 早期： lr=0x811E45B6(全屏黑幕 vert_0014|frag_0079) / lr=0x811EC006(清屏 vert_0000|frag_0000)
- 后期： 再加 lr=0x811E7DD2 / lr=0x811E827E（各约 1260 次）
- 2. 若不可见 ⇒ 查 `vert_0006` 的属性布局与解包：
- 把 `_vf.attrs`（attribute 描述符）与 `[AINNER]`/`[PXCENSUS]` 输出对照，确认是"解包失败"还是"变换不对"。
- 3. 若可见但被覆盖 ⇒ 回到"谁在同一帧更晚把它抹掉"（`[CAPHSEQ]`/`[SEQFH]` 有序序列）。

---

## `docs\_append_h145k470_h475.md`

**标题**：h145k470–h145k475：启动/运行速度优化（**+86%**）与"为什么只有 ~9fps"的根因 ｜ 0. 结论速览（用户诉求：启动太慢） ｜ 1. ★ 根因：解释器主循环里**每条指令要走过 530 个探针** ｜ 2. 已落地的三项优化（**默认生效**） ｜ 3. 两次失败尝试（**必须记录，避免后人重踩**） ｜ 4. 下一步（要把速度再提 3–5 倍，唯一的路） ｜ 5. 给使用者的实用建议

**发现/结论**（10 条，最多列 40）：

- `thumb_interpreter.cpp` 的 `run()` 主循环里实测：
- ⇒ 每条指令 ≈ 530 次 `if(g_wp_cur && (cur&~1u)==0x…)` 测试
- ⇒ **探针开销占 80–90%**，这才是 ~9fps 的真正原因（**不是** Vulkan、**不是**日志、**不是**影片转换）。
- 日志侧实测（`_h145k467_miss`，2 分钟）：
- 合计约 70% 的日志量，正常运行不需要 ⇒ 直接 `freopen("NUL","w",stderr)`。
- ⇒ `P5D_VERBOSE=1` 恢复。
- - **v1**：把 387 个"纯打印"探针全部禁用 ⇒ **运行崩掉**（进程退出、无 [FRAME]）。
- - **v2**：加严副作用检测（`return` / `set_reg` / `npc` / 指针写 / 数组写 / 全局赋值），禁用 353 个 ⇒ **仍然崩**。
- ⇒ **结论：静态检测不足以判定全部"状态修补型"探针**；`P5D_PROBE_LEGACY=1` 必须能一键恢复。
- - 想快速跳过影片：`P5D_MOVIE_FPS=600`（影片按 600fps 播完 ⇒ 立刻进入下一段流程）。

---

## `docs\_append_h145k476_h480.md`

**标题**：h145k476–h145k480：速度优化的**最终结果**与一处重要修正（探针**不是**帧率瓶颈） ｜ 1. 最终落地结果（默认生效） ｜ 2. ★ 重要修正：**探针测试不是帧率瓶颈**（推翻本节早先的推断） ｜ 3. 那么 90 ms/帧 花在哪（下一步的唯一入口） ｜ 4. 给使用者的实用结论 ｜ 5. 本轮脚本/备份（都在 C:\dev\p5d_build / src\arm）

**发现/结论**（9 条，最多列 40）：

- - 我先把探针从 **530 个**降到 **306 个**（额外禁用 224 个）、再降到 456 个（禁用 74 个）逐档实测：
- ⇒ **每指令 300+ 次探针测试并不是限制帧率的东西**。
- - **GPU 同步**：`[CEND]` 3441 条 / 2880 帧 ⇒ 每帧约 1.2 次 `submit+waitIdle`，≈ 每帧 1–2 ms；
- `CAP=3.5e9 → 约 4000 帧` ⇒ 每帧约 875k 指令；70 秒 660 帧 ⇒ **每帧约 106 ms**
- ⇒ **约 360 cycles/指令**（按 3 GHz 估）。正常解释器是 20–50 cycles/指令，**差 10 倍**。
- 1. **`ThumbInterpreter::run()` 单函数近 5000 行** ⇒ 代码体积巨大、**I-cache 命中率极差**，
- - `_h145k472_probeoff.py`(v1, 禁用 387 ⇒ **会卡**) / `_h145k473_probeoff2.py`(v2, 353 ⇒ **会卡**)
- / `_h145k474_probeoff3.py`(v3, 74 ⇒ **可用，已采用**) / `_h145k478_gen.py`(可调 N 用于二分)
- - `_h145k478_bisect.ps1`（自动二分脚本；实测其"生成→编译"步骤未生效，需人工执行生成+编译）

---

## `docs\_append_h145k481.md`

**标题**：h145k481：性能定位收官 —— 热点**极度分散**，瓶颈是"解释器每条指令的基础成本" ｜ 1. 方法 ｜ 2. 判读（关键） ｜ 3. 本轮真正落地的收益（默认生效，实测）

**发现/结论**（7 条，最多列 40）：

- 用 `P5D_PCHIST=1` 开启（**默认关**，开销约 5 cycles/指令）。跑 45 秒（帧 180）实测：
- pc=0x81278C62 count=15048      pc=0x81236D2A..D5E count≈14098
- pc=0x8122C886 count=13056      pc=0x812B0B66/B82/B0D58 count≈10000
- 而 **最高**的槽只有 15048 次（≈ 平均的 3 倍）⇒ **热点极度分散、几乎没有"热点代码段"**。
- 1. **禁用 224 个探针 vs 74 个探针，帧率几乎不变**（10.24 vs 10.4 fps）⇒ 探针测试不是瓶颈；
- ⇒ 开销是"均匀摊在每条指令上"的：**取指（I-cache miss，因为 `run()` 近 5000 行）+
- - ⇒ 结论：**要继续提速（目标 5–10 倍），必须改变执行引擎本身**，可选路线：

---

## `docs\_append_h145k486_h487.md`

**标题**：h145k486–h145k487：★ 引擎**首次进入标题场景并请求资源**——卡在"没有任务轮询新加载管理器" ｜ 1. 突破性观察（`_h145k486_title`，性能优化后跑到帧 5460） ｜ 2. 但它是**最后一道门**：请求永远 pending ｜ 3. 下一步（已埋探针）

**发现/结论**（14 条，最多列 40）：

- [LRQREQ#34] fn=0x8117C1BC mode=0xF5 nm='TITLE'  grp='TITLE_BIN' lr=0x811968D8
- [LRQREQ#35] fn=0x8117C1BC mode=0xF0 nm='CLOTH'  grp='TITLE_BIN' lr=0x811968F2
- [LRQPTR#33] req=0xB4BB402C f4=0x0 nm='TITLE_BIN'
- [FIN in] inner=0xB4BB3000 state=17 sess=0xA03ED1E0 A0=0 nm='title/c_title_p5'
- ⇒ **请求数从 32 增到 35**，组名 `TITLE_BIN`、资源名 `TITLE`/`CLOTH`、并在打开 `title/c_title_p5`
- [LRQDUMP q=8500] mgr=0xB4BB2030 busy=1 n=35 PENDING: 33:0xB4BB502C f4=0x4 TITLE |
- - **加载管理器换成了新的 `0xB4BB2030`**（旧的是 `0xB1C19030`）；
- - `[LRQDUMP]` 的 `q` **停在 8500 不再增长** ⇒ **没有任何任务在轮询这个新的加载管理器**
- （与 h145k09/h145k10 那条"轮询停止"的结论同源，但这次对象是**标题场景的加载管理器**）；
- - 结果：`TITLE`/`TITLE_BIN`/`CLOTH` 永远不完成 ⇒ 标题画面的贴图/精灵永远不齐
- ⇒ 标题场景只做"空合成"（`GEODEDUP distinct=5`：只有内容 pass 的徽章 + 4 个后处理几何）。
- `[TASKCREATE]`（在任务创建函数 `0x8116F7CA` 处打印**每个被创建任务的名字**）已经加好并在跑，
- - **若日志里根本没有与 title 相关的任务被创建** ⇒ 标题场景的加载任务**从未建立**（要查 0x811968xx 那段发射代码）；
- - **若创建了但随后被摘除/不再被 batch 调用** ⇒ 要查 `0x8116F748` 这条批量调用链为何停转。

---

## `docs\_append_h145k488.md`

**标题**：h145k488：★ LOGO / DLC / 标题画面"零像素"的**直接原因**已定位 ｜ 1. 任务侧：LOGO 与 DLC 的任务**都创建了**（`[TASKCREATE]`，h145k487） ｜ 2. 绘制侧：新管线**在提交**，但**没走顶点解包** ｜ 3. 结论（本轮最重要的定位） ｜ 4. 下一步（明确、单变量）

**发现/结论**（6 条，最多列 40）：

- `[TASKCREATE]`（在任务创建函数 `0x8116F7CA` 处打印任务名）完整列出了 48 个任务，其中与目标直接相关的：
- ⇒ **不是"任务没建"**：标题序列、LOGO 绘制、DLC 弹窗的任务**全都创建了**。
- ⇒ **`vert_0006/0007/0008` 的绘制根本没有进入解包段**（连"通用解包"都没进）
- ⇒ **它们的顶点数据从未被展开**（GPU 拿到的是未展开的原始数据/空数据）
- ⇒ **这些绘制必然零像素**。
- 1. 找到解包段的**进入条件**（`s_pd_vb` / manifest 命中 / `g_cur_vgxp` 等），确认 `vert_0006/0007/0008`

---

## `docs\_append_h145k489.md`

**标题**：h145k489：★ 最终定位 —— LOGO/DLC/标题画面零像素的原因是"**只解包了索引绘制**" ｜ 1. 代码证据（`eboot_probe.cpp` n_draw） ｜ 2. 与实测现象完全吻合 ｜ 3. 结论 ｜ 4. 下一步（明确、单变量，下一轮第一件事）

**发现/结论**（4 条，最多列 40）：

- | `[EHMAP]` 只列出 `vert_0000`/`vert_0014`（都 `in_vfmt=1`） | 只有它们进了索引绘制路径 ⇒ 只有它们走到解包段 |
- | `[DRAWDEDUP] distinct=10`（`vert_0006/0007/0008` 在提交）却零像素 | 它们走 `gxm_draw_arrays` ⇒ 顶点未解包 |
- >   **只挂在索引绘制路径上** ⇒ 顶点从未展开 ⇒ 必然零像素。**
- - 设置 `s_pd_vb=(cnt)*stride`（stride 由权威属性表 `g_vfmt[eh].stride[0]` 给出，取不到再用 16/24 兜底）；

---

## `docs\_append_h145k490_h495.md`

**标题**：h145k490–h145k495：★★ 两处重要更正 + 抓帧开关的性能警告 ｜ 1. 更正一：**探针不是帧率瓶颈**（h145k479 实测） ｜ 2. 更正二：**新管线其实"解包成功、管线选中、几何在屏内"**（h145k494，帧 3180） ｜ 3. ★ 抓帧开关的性能警告（实测） ｜ 4. 本轮已确认的任务清单（`[TASKCREATE]`，48 个） ｜ 5. 下一轮（第一件事）

**发现/结论**（10 条，最多列 40）：

- ⇒ 早先"每指令 530 个探针测试占 80–90%"的推断**不成立**；
- ⇒ 瓶颈是"每条指令的解释器基础成本"（取指/I-cache + 译码 + 访存翻译，约 360 cycles/指令）。
- 因为当时（帧 <1500）这些管线还没出现**（它们在 title 场景才出现）。到 title 场景后实测：
- ⇒ **它们解包成功、属性表命中、管线选中、几何在屏幕内**！
- ⇒ 所以"LOGO/DLC 零像素"的结论**要收回**：它们**很可能已经画出来了**，
- "全黑"的判断来自 `[SHOTSTAT]`（只在抓帧时才有）⇒ **属于"没测到"而不是"没有"**。
- ⇒ **测速/正常玩时绝对不要开** `P5D_DBG_FRAMES` / `P5D_CAPH` / `P5D_SEQ_RT` / `P5D_MEMWATCH`；
- ⇒ 任务层面**齐备**；下一步要验证的是"它们在 title 场景**是否真的产出了像素**"。
- - 若有像素 ⇒ 目标达成度大幅提升，接下来只是"位置/颜色/标定"；
- - 若确实全黑 ⇒ 回到纹理/混合/深度（几何与管线已排除）。

---

## `docs\_append_h145k497_h498.md`

**标题**：h145k497–h145k498：title 场景**确实全黑**（RT 整池转储判定）+ vita3k 源码对照收获 ｜ 1. 定点抓帧（帧 3200 / 3400） ｜ 2. ★ RT 整池转储：24 张全部没有真实画面（帧 3400） ｜ 3. 与 `[EHGEO]`/`[PIPESEL]` 合起来看（矛盾点，下一轮的入口） ｜ 4. vita3k 源码对照（本轮按用户建议做了，收获两条） ｜ 4.1 `sceGxmDisplayQueueAddEntry` 的真实语义（`modules/SceGxm/SceGxm.cpp:448`） ｜ 4.2 ★ sync object 的等待语义 —— **我们完全没实现**（`SceGxm.cpp:103/448/725`） ｜ 4.3 `sceDisplaySetFrameBuf` 在 vita3k 里是 `UNIMPLEMENTED()` ｜ 5. 下一轮清单（按优先级）

**发现/结论**（15 条，最多列 40）：

- [SHOTSTAT] 全为 max=0 ⇒ 画面全黑
- 本次保留 stderr、不设 SEQ_MIN 后确认：**帧就是全黑的**。）
- rt_pool0_cA0F00000_1.bmp         全黑 ← ★ 这就是 [SETFB] 里游戏指定的显示缓冲 pBase
- ⇒ **结论：title 场景下，主 RT、游戏指定的显示缓冲 `A0F00000`、以及所有池 RT 都没有真实画面。**
- ⇒ 几何在屏内、管线选中、顶点解包成功，**但像素为 0**。
- ⇒ 剩下的可能只有三类：**① 片元输出本身是黑的/alpha=0；② 深度/模板把像素丢掉；③ 采样到的纹理是黑的**。
- - **强制这些管线的片元输出常量色**（复用现有 `P5D_REALFS=r` 机制）⇒ 若出现像素 ⇒ 问题在"片元/纹理"；
- - 若无像素 ⇒ 问题在**深度/模板/colorWriteMask**（查 `[RPCHK]` 的 pass 与 depth 设置）。
- ⇒ **上屏动作发生在回调里**，回调是 `sceGxmInitialize` 注册的那个（我们已知 = `0x811EE7C3`），
- if (fragmentSyncObject) renderer::wishlist(sync, SyncObjectSubject::DisplayQueue | SyncObjectSubject::Fragment);  // ★ 阻塞等待
- ⇒ 游戏靠这套"三缓冲 + 等待"来**节流**并保证"要显示的缓冲已经画完"。
- ⇒ 游戏**从不在 BeginScene 处阻塞** ⇒ 它可能比渲染跑得快得多（缓冲被复用/显示未完成的帧）。
- ⇒ 说明 **vita3k 不靠这个 API 上屏**，所以"显示缓冲 pBase"**不是** vita3k 的选面判据；
- 1. **强制新管线片元常量色**（`P5D_REALFS`）⇒ 判断"有没有像素"这一层，一次运行定案。
- 3. 按 vita3k 的 scene 语义**修正 present 选面**（用"最后一次 EndScene 的 color surface"而非"最后宽 pass"）。

---

## `docs\_append_h145k499.md`

**标题**：h145k499：★ 决定性判决 —— 几何/管线/光栅化**全部正常**，黑的是"片元算出来的颜色" ｜ 1. 单变量实验：`P5D_REALFS=r`（把所有片元替换成"输出红色"） ｜ 2. 这条判决一次性排除的东西 ｜ 3. 由此得到的根因收敛 ｜ 4. 下一轮（唯一入口，按顺序） ｜ 5. 本轮 vita3k 对照的结论（保留备查）

**发现/结论**（11 条，最多列 40）：

- ⇒ **整屏纯色（mean=255.00）**。
- - **顶点解包 / 几何**：有像素 ⇒ 几何被正确光栅化；
- - **视口/裁剪/深度/模板/colorWriteMask**：若其中之一把像素丢掉，"强制红"也会是黑的 —— 实测不是；
- - **present 链路**：swapchain 能显示内容（满屏红就是证据）。
- ⇒ 下一轮的范围被压缩到**一处**：**这些片元的采样源（RT 别名纹理）到底是不是黑的**。
- （我们的 RT 是 VkImage、本就不写 guest 内存）⇒ **那是伪影，不能作为"黑"的证据**；
- ⇒ 直接判定"上游 RT 是不是黑的"。
- 2. 若上游 RT 确实黑 ⇒ 继续往上游走（谁本该往它里面画）：用 `[CAPHSEQ]`/`[EHGEO]` 的几何 + `[TEXMAT]` 的绑定，
- 3. 若上游 RT 不黑而最终屏幕黑 ⇒ 问题在**后处理链的采样坐标/格式**（`[TEXMAT]` 已显示别名被声明为 1024x544
- ⇒ 游戏不会在这里被节流/同步，缓冲可能被提前复用（值得下一轮按 vita3k 补齐）；
- - `sceDisplaySetFrameBuf` 在 vita3k 是 `UNIMPLEMENTED()` ⇒ 上屏**不靠**它，

---

## `docs\_append_h145k500_h501.md`

**标题**：h145k500–h145k501：又排除三项，根因锁定"title 场景的片元输出为黑" ｜ 1. `[RTSTAT]`（**VkImage 回读**，非 `[TEXMAT]` 的 guest 内存伪影） ｜ 2. 排除"黑幕覆盖内容"（`[CAPHSEQ]` 有序绘制序列，内容帧） ｜ 3. 排除"present 选错面" ｜ 4. 到此为止，被实测**排除**的环节（清单） ｜ 5. 结论与下一轮唯一入口

**发现/结论**（11 条，最多列 40）：

- title 场景(3360)： 三张池 RT = max=255 mean=3.02                 （★ 仍然只有徽章，没有新内容）
- [CAPHSEQ] d=15704 fh=6952EF6C507D515E rt=A0E1D000 cw=960 vh=172A9E736C7DC35E   ← ★ 内容在最后
- ⇒ **内容 draw 排在黑幕之后**，不会被抹掉（与"徽章可见"一致）。
- **旧假设"内容被同一帧更晚的黑幕抹掉"正式排除。**
- [PRESENT-CD] 强制上屏源 = 0xA0E1D000
- ⇒ **强制用主 RT 上屏仍然全黑** ⇒ **present 选面不是根因**（主 RT 在 title 场景本身也是黑的）。
- | 视口/裁剪/深度/模板/写色/光栅化 | `P5D_REALFS=r` ⇒ **满屏红 mean=255.00** |
- > 结合 `P5D_REALFS=r` 的判决 ⇒ **问题在片元的着色计算/纹理采样**，而不是几何或链路。
- （扩展 `P5D_REALFS` 的同类机制，或强制"采样返回白色"）⇒ 若出现像素 ⇒ 确认是**纹理内容是黑**；
- 2. **回读这些片元实际绑定的纹理/RT 的 VkImage**（不是 guest 内存！）⇒ 直接看它是不是黑的；
- 3. 若纹理有内容而画面仍黑 ⇒ 查**采样坐标/格式**（`1024x544 vs 960x544` 的错配仍是头号嫌疑）。

---

## `docs\_append_h145k505_h507.md`

**标题**：h145k505–h145k507：★ 因果链闭合 —— 「纹理创建 586 个 / 上传 2 张 / 绘制只用 1 张」 ｜ 1. 先破掉两个"cap 假象" ｜ 2. 真正的矛盾 ｜ 3. ★ 因果链（本轮结论） ｜ 4. 下一轮（唯一的根因） ｜ 5. 本轮新增/修复的探针

**发现/结论**（19 条，最多列 40）：

- | 探针 | 原 cap | 放开后实测 | 结论 |
- 放开后带帧号的实测（`_h145k507_texinit`）：
- [TEXINIT #0]  fc=0   tex=0x816CBD7C data=0xA0F00000 fmt=0xC000000 1024x544   ← 三张池 RT 的纹理视图
- [TEXINIT #1]  fc=0   data=0xA1300000 1024x544
- [TEXINIT #2]  fc=0   data=0xA1700000 1024x544
- [TEXINIT #578] fc=241 tex=0xB2FB1000 data=0xA632DC80 fmt=0x95001000 512x512
- [TEXINIT #583] fc=403 tex=0xB3027000 data=0xA6413180 fmt=0x95001000 512x128
- [TEXINIT #584] fc=463 tex=0xB3095000 data=0xA6423680 fmt=0x95001000 2048x128
- [TEXINIT #585] fc=522 tex=0xB30D1000 data=0xA6463B80 fmt=0x95001000 512x512
- ⇒ **纹理对象创建一直在进行、data 地址都在合法堆区** ⇒ **资源加载/解码链本身是在产出纹理的**。
- ⇒ **创建了 586 个纹理，但整个运行只真正上传了 2 张；所有绘制都只绑定那一张 256×512。**
- 地址或内容变了就会重传 —— 实测之所以"只 2 张"，是因为**游戏根本没有把别的纹理绑到绘制上**。）
- ├─ 请求进入【新的加载管理器 mgr=0xB4BB2030】，但 `[LRQDUMP]` 的 q 停在 8500 不再增长
- │  ⇒ **没有任何任务在泵这个新的 mgr**（与 h145k09「轮询停止」同源）
- 这些资源永远 pending ⇒ 贴图永远不解码/不上传
- 片元采样不到正确贴图 ⇒ **输出的颜色是黑**（与 h145k499 的 `P5D_REALFS` 判决完全一致：
- 若"根本没创建" ⇒ 查发起请求的 `0x811968D8` 那段（title 资源发射代码）为什么没有配套任务。
- 2. **HLE 层代泵**（务实）：每帧遍历该 mgr 的请求链，对 pending 请求调用 worker `0x8117BA30(r0,r1=req)`
- （签名/上下文可从 `[LHANDLER] ctx=` 与 `0x8117C986` 的现场取证）；

---

## `docs\_append_h145k509_h514.md`

**标题**：h145k509–h145k514：★ 实现"代泵"——纹理上传 2→274（但画面仍黑，指出下一环） ｜ 1. 代泵实现（`P5D_PUMP=1`） ｜ 1.1 第一版崩了（记录教训） ｜ 2. ★ 效果：纹理上传从 **2 张 → 274 张** ｜ 3. 但画面**仍然全黑**（新工具：直接截游戏窗口） ｜ 4. 下一环（明确） ｜ 5. 本轮新增的实用工具（后续都可用）

**发现/结论**（14 条，最多列 40）：

- （新 mgr `0xB4BB2030` 无人轮询）⇒ 贴图不上传 ⇒ 画出来是黑。
- `0x8117BA30(r0=ctx, r1=req)`；请求表在解释器侧用 `[LRQPTR]` 探针收集并导出给 HLE 层：
- 第一版用 `ctx = mgr[+0x14]`（旧 mgr `0xB1C19030` 的该字段确实是 `0xB1C1A030`），但**新 mgr 的 `+0x14` 是 0**
- [PUMP] mgr=0xB261A030 ctx=0x0 候选=5 推进=5
- [ERROR] ThumbExec badmem addr=0x0000000C @pc=0x8117BB42   ← worker 里 `ldr lr,[r6,#0xc]`，r6=ctx=0
- **修法**：改为**记录"游戏自己调用 worker 时的 r0"**（在 `0x8117BB8A` 的 `[LHANDLER]` 探针里 `g_lrq_ctx=r[1]`），
- 代泵用这个 ctx —— 之后**不再崩溃**，`ctx=0xB261A030`。
- [LRQDUMP q=8400] mgr=0xB261A030 PENDING: 31:HELP_P5
- [LRQDUMP q=8500] mgr=0xB4BB2030 PENDING: 33:TITLE      ← 新 mgr 的 TITLE 仍在（ctx 不匹配，见 §4）
- ⇒ **代泵确实打通了"贴图进 GPU"这条链**（上传的都是有大面积非零内容的真实贴图）。
- ⇒ **贴上去了 ≠ 画面就出来了**：还缺后续环节（下一节）。
- - **新 mgr `0xB4BB2030` 的 `TITLE` 请求仍然 pending**：因为代泵用的 `ctx` 是**旧 mgr 的**
- ⇒ worker 对新 mgr 的请求**没有真正推进**（不崩，但也没完成）。
- - 因此 title 场景的**场景对象/贴图绑定**仍未建立 ⇒ 绘制时绑定的还是旧纹理 ⇒ 黑。

---

## `docs\_append_h145k515.md`

**标题**：h145k515：按 mgr 配对 ctx 的尝试（**配对不可靠**，下一轮改用"从 mgr 结构直接找 ctx"） ｜ 1. 动机 ｜ 2. 实测：配对确实建立了，但**不可靠** ｜ 3. 下一轮做法（更可靠，二选一） ｜ 4. 本轮其它已确认结论（不变）

**发现/结论**（11 条，最多列 40）：

- h145k509 的代泵只记"最后一次 worker 的 ctx"，导致**新 mgr 用旧 ctx** ⇒ 新 mgr 的 `TITLE` 请求仍不完成。
- 于是本轮加了 **(mgr → ctx) 配对表**：在 `0x8117C3B6(mgr)` 记当前 mgr、在 `0x8117BB8A(worker)` 把 ctx 记到它名下；
- [LRQPAIR] mgr=0xB1C19030 <-> ctx=0xB1C19030（新配对）
- [LRQPAIR] mgr=0xB261A030 <-> ctx=0xB1C19030（新配对）     ← 与上一行同 ctx，明显错位
- [LRQPAIR] mgr=0xB2614030 <-> ctx=0xB261A030（新配对）
- [PUMP]    mgr=0xB261A030 ctx=0xB1C1A030 候选=8 推进=8      ← 期望的配对没命中，退回"最后一次 ctx"
- [LRQDUMP q=1700] mgr=0xB1C19030 busy=1 PENDING: 14 COMMON_P5 | 15 MUSIC_TITLE_P5 |
- 两者各自被别的调用刷新 ⇒ 配对会错位（上表第二行就是证据：两个不同 mgr 记到了同一个 ctx）。
- （旧 mgr 的该字段是 `mgr+0x14 = 0xB1C1A030`，可作为"形状参照"。）
- 2. **在 worker 的调用现场取 mgr**：`0x8117C986`（`bl 0x8117BA30`）的上游调用链里一定带着 mgr，
- - 但**画面仍全黑**（`_h145k514_title.png`，窗口截图，帧 3540）⇒ 还缺"新 mgr 的请求被真正推进"这一环；

---

## `docs\_append_h145k516_h518.md`

**标题**：h145k516–h145k518：代泵的三次尝试与**失败原因**（并修正一处判断） ｜ 1. 硬成果：反汇编拿到了 worker 的**真实调用约定**（`0x8117C93A` 现场） ｜ 2. 三次代泵尝试与实测结果 ｜ 3. ★ 一处重要修正（来自本轮日志） ｜ 4. 下一轮方向（更稳） ｜ 5. 本轮保留的实验开关

**发现/结论**（15 条，最多列 40）：

- 8117C980  adds r1,r7,#0           ; → worker 的 r1 = 经 0x81016296 → 0x81016810 → 0x81016818 变换后的节点
- 8117C982  bl 0x8117BA30
- 8117C98A  ldr r0,[r7,#4]; orrs #1; str r0,[r7,#4]   ; ★ 完成位由【这个 phase 函数】设置
- ⇒ 正确的代泵方式应当是**调用这个 phase 函数**（签名 `r0=phase, r1=未用, r2=req`），而不是自己去拼 worker 的参数。
- | ① | `mgr[+0x14]` | 新 mgr 该字段为 **0** ⇒ `badmem addr=0x0000000C @pc=0x8117BB42` |
- | ② | `req[0x3C]`（反汇编得出的"正确"值） | 该对象此刻**尚未初始化** ⇒ `badmem addr=0x00000004 @pc=0x8117BB80` |
- | ③ | 调 `phase` 函数 `0x8117C93A(0,0,req)` | `req[0x3C]=0` ⇒ `badmem addr=0x00000004 @pc=0x8117C956` |
- **结论**：**直接"替游戏调用它的内部函数"太脆弱** —— 成败取决于游戏内部对象的初始化时机与调用次数，
- [LHANDLER #12000] handler=0x8117C7E7 req=0xB217802C ctx=0xB261A030 nm='BG_TXST02'
- ⇒ **游戏其实一直在调用 worker 泵加载**（ctx = `0xB261A030`）！
- > 游戏**在泵 `0xB261A030` 这个管理器**，但**`0xB4BB2030` 这个管理器（`TITLE` 所在的那个）没有人在泵**。
- ⇒ 下一轮该查的是：**`0xB4BB2030` 是谁创建的、本该由哪个任务/入口泵它**（而不是盲目代泵）。
- 1. **查 `0xB4BB2030` 的来源**：谁分配了它（`[RALLOC]`/`[NATIVE] malloc` 探针 + 回溯调用栈）；
- 以及发起 `TITLE`/`TITLE_BIN`/`CLOTH` 请求的那段代码（`lr=0x811968D8/0x811968F2`）是否**同时负责创建泵它的任务**。
- 3. 若确认"任务没被调度" ⇒ 那是**调度器层面的缺陷**（比代泵稳得多，也更接近真机语义）。

---

## `docs\_append_h145k519.md`

**标题**：h145k519：反汇编定位「请求的 owner」与「title 任务创建模式」+ 运行不稳定的实测记录 ｜ 1. 反汇编成果一：**每个加载请求都带一个 owner（r0 = 所属管理器/上下文）** ｜ 2. 反汇编成果二：title 任务的创建是标准"建任务 + 按名查找"模式 ｜ 3. ★ 实测：加载卡死的"现场特征"（本轮多次复现） ｜ 4. 下一轮（一步就能定位"谁该泵"）

**发现/结论**（16 条，最多列 40）：

- `0x811968B0~0x811968EE`（title 资源发射处）：
- 811968D4  bl 0x8117C1BC           ;   TITLE 请求（mode=0xF5）
- 811968EE  bl 0x8117C1BC           ;   CLOTH 请求（mode=0xF0）
- ⇒ 已给 `[LRQREQ]` 探针加上 `owner=` 输出（`g_lrq_owner[]`）。实测 owner 随加载组变化：
- [LRQREQ#21..#30] mode=0xF7 owner=0xB1C1A030 nm='NOTIFY_LINE9'…'LOADING_LOADING_SPR_P5' grp='COMMON_ARC_P5'
- [LRQREQ#31]      mode=0xF5 owner=0xB2614030 nm='KEY_HELP_P5'  grp='mainmenu/keyhelp/P5/c_help_key.spr6'
- [LRQREQ#32]      mode=0xF5 owner=0xB261A030 nm='HELP_P5'      grp='mainmenu/helpstring/P5/c_help.spr6'
- `0x811980E0` 附近（`title sequence` 任务的创建点，`[TASKCREATE #43] lr=0x811980F8`）：
- 811980E6  movw r1,#0xB140 / movt r1,#0x8130   ; r1 = 任务名字符串 0x8130B140
- 811980F4  bl 0x8116F7CA                        ; ★ 创建任务（我们的 [TASKCREATE] 探针就在这里）
- 81198108  bl 0x8116F900                        ; 按名查找任务
- ⇒ `title sequence`/`logoDrawProc`/`logoTMX` 都是这样建的（上一轮 `[TASKCREATE]` 已列全 48 个）。
- [LRQDUMP q=3200] mgr=0xB1C19030 busy=1 PENDING: 15 MUSIC_TITLE_P5 | 16 BG_TXST02 | 17 HELP |
- ⇒ **帧循环不推进，但加载查询计数 `q` 仍在增长** ⇒ 游戏**卡在"等加载完成"的循环里**，
- 给 `[LRQDUMP]` 探针加上**调用者 lr**（`0x8117C3B6` 的 `r[14]`）：
- - 再对该 lr 加计数探针 ⇒ 直接看出**它从哪一帧起不再被调用**；

---

## `docs\anim_update_analysis.md`

**标题**：0x81077250 骨骼动画蒙皮更新函数 逆向分析 ｜ 函数确认 ｜ 基本信息 ｜ VFP浮点操作统计 ｜ 函数序言分析 ｜ 蒙皮计算核心（VFP密集区域） ｜ 典型的加权矩阵计算（0x810785de区域） ｜ 第二个蒙皮区域（0x81078934区域） ｜ 调用频率分析 ｜ 循环结构（29个循环） ｜ 推测的C伪代码 ｜ 关键发现 ｜ 对移植的意义 ｜ 产出文件

**发现/结论**（21 条，最多列 40）：

- - **地址**: 0x81077250 - 0x81078CDF
- 0x81077250: push {r4-r11,lr}           ; 保存9个寄存器
- 0x81077254: vpush {s16-s27}             ; 保存12个VFP寄存器
- 0x81077258: sub sp,#0x17c               ; 380字节栈帧
- 0x8107725a: movw r4,#0xbae4             ; 全局变量地址
- 0x8107725e: movt r4,#0x810a
- 0x81077262: ldr r4,[r4]                 ; 加载全局状态
- 0x81077266: add r5,sp,#0x1d0            ; r5 = 参数结构体指针
- 0x81077268: ldr r6,[r5,#0x10]           ; 加载参数...
- ble 0x81078976              ; 如果权重<=0，跳过
- | 0x8107f9f0 | 14 | 矩阵乘法/变换辅助函数 |
- | 0x8107c960 | 9 | 骨骼矩阵计算辅助 |
- | 0x8107593c | 3 | 向量运算 |
- | 0x81086738 | 3 | 四元数运算 |
- | 0x81074a88 | 2 | ECS组件getter（获取动画组件） |
- | 0x8106c0a8 | 2 | 未知辅助 |
- | 0x8107cce8 | 2 | 内存操作 |
- | 0x810715a6 | 2 | 数学辅助 |
- | 0x810ac348/0x810ac418 | 各1 | 导入函数（blx） |
- 4. **ECS集成**: 调用0x81074a88获取动画组件
- - `docs/anim_update_0x81077250_full.txt` — 完整反汇编（2211条指令）

---

## `docs\architecture.md`

**标题**：架构设计 ｜ 整体架构 ｜ 核心设计决策 ｜ 1. CPU执行策略 ｜ 2. 模块加载机制 ｜ 3. 图形翻译：GXM → Vulkan ｜ 4. 内存模型 ｜ 5. 文件系统 ｜ 关键技术难点 ｜ 与vita2hos的对比

**发现/结论**（3 条，最多列 40）：

- - 通过自定义链接器加载PSV的SELF/ELF格式
- - 实现自定义ELF加载器，解析PSV SELF格式
- - 纹理格式转换（GXM专用格式 → Vulkan格式）

---

## `docs\char_cmp_analysis.md`

**标题**：0x81074a88 忽略大小写的字符比较函数 逆向分析 ｜ 重大修正 ｜ 基本信息 ｜ 函数签名 ｜ 完整反汇编 ｜ 算法分析 ｜ 对之前分析的修正 ｜ 主循环0x810650D8中的调用 ｜ 变换评估0x8107f9f0中的48次调用 ｜ ECS组件getter的真实身份 ｜ 关键发现 ｜ 产出文件

**发现/结论**（27 条，最多列 40）：

- **0x81074a88不是ECS组件getter！** 它是一个忽略大小写的单字符比较函数。
- - **地址**: 0x81074a88 - 0x81074AB3
- 0x81074a88: ldrb r1, [r1]         ; r1 = *c2 (加载第二个字符)
- 0x81074a8a: mov  r3, r1           ; r3 = r1 (保存原始值)
- 0x81074a8c: ldrb r2, [r0]         ; r2 = *c1 (加载第一个字符)
- 0x81074a8e: cmp  r2, r3           ; 原始字符比较
- 0x81074a90: bne  0x81074a96       ; 不相等则需要大小写转换
- 0x81074a92: movs r0, #0x1         ; 相等，返回1
- 0x81074a94: b    0x81074ab2       ; 快速返回
- 0x81074a96: cmp  r2, #0x61        ; r2 >= 'a'?
- 0x81074a98: blt  0x81074aa0       ; 小于'a'跳过
- 0x81074a9a: cmp  r2, #0x7a        ; r2 <= 'z'?
- 0x81074a9c: bgt  0x81074aa0       ; 大于'z'跳过
- 0x81074a9e: subs r2, #0x20        ; 'a'-'z' -> 'A'-'Z' (减0x20)
- 0x81074aa0: cmp  r1, #0x61        ; r1 >= 'a'?
- 0x81074aa2: blt  0x81074aaa       ; 小于'a'跳过
- 0x81074aa4: cmp  r1, #0x7a        ; r1 <= 'z'?
- 0x81074aa6: bgt  0x81074aaa       ; 大于'z'跳过
- 0x81074aa8: subs r1, #0x20        ; 'a'-'z' -> 'A'-'Z'
- 0x81074aaa: movs r0, #0x0         ; 默认返回0
- 0x81074aac: cmp  r2, r1           ; 比较大写后的字符
- 0x81074aae: bne  0x81074ab2       ; 不相等返回0
- 0x81074ab0: movs r0, #0x1         ; 相等返回1
- 0x81074ab2: bx   lr               ; 返回
- bl 0x81074a88       ; 获取组件
- bl 0x81074a88       ; 比较两个字符串的首字符（忽略大小写）
- - `docs/char_cmp_0x81074a88.txt` — 完整反汇编

---

## `docs\community\GFD_STUDIO_GUIDE.md`

**标题**：GFD Studio 下载与验证指引（ARCH_174） ｜ 一、下载地址 ｜ 二、要打开的文件（本机路径） ｜ 三、★ 要观察的三件事（这三点能直接回答我卡住的问题） ｜ 四、背景：我已经确认的结论

**发现/结论**（5 条，最多列 40）：

- 眼镜   ...\p5\face\pc201_f1.GMD        ← 我实测：4 个网格全是眼镜部件
- ...\p5\face\pc210_f1.GMD        ← 我实测：mesh1(393顶点) 是一张完整的脸
- - 我实测 mesh1 = 393 顶点 = 脸皮，mesh0 = 32 顶点
- - **社区材质预设**（`GFDStudio/Presets/Dancing_Character.yml`）确认
- **⇒ 所以只要 GFD Studio 能正常显示 `pc201` 的脸，答案一定在它的装配逻辑里**

---

## `docs\community\P5D_COMMUNITY_NOTES.md`

**标题**：权威工具：GFD Studio ｜ ★ P5D 角色材质预设：GFDStudio/Presets/Dancing_Character.yml ｜ 未解决

**发现/结论**（4 条，最多列 40）：

- TexCoordFlags0: 0xFFFFFFF8
- TexCoordFlags1: 0xFFFFFFF8
- 1. **DisableBackfaceCulling: 0 ⇒ 游戏确实做背面剔除** —— 与 ARCH_151 修正后的
- GFD Studio 能完整显示 P5D 角色（含脸）⇒ 数据一定完整。

---

## `docs\component_system_analysis.md`

**标题**：libsmart 组件系统架构分析 ｜ 核心结论：不是ECS架构，是传统C风格面向对象 ｜ 1. 无虚函数证据 ｜ 2. 内存访问模式 ｜ 主循环基址寄存器使用 ｜ 惊人发现：几乎所有访问都是偏移0 ｜ 3. 参数结构体模式 ｜ 4. 类型标签系统 ｜ 5. 字符串密集型设计 ｜ 6. LAPACK静态链接 ｜ 7. 架构总结 ｜ 8. 对移植的影响 ｜ 9. 产出文件

**发现/结论**（11 条，最多列 40）：

- **结论**：引擎不使用C++虚函数，没有运行时类型识别（RTTI），使用静态分发而非动态分发。
- 主循环开头连续8次调用字符比较函数 `0x81074a88`：
- char_cmp(r9, 0x8120c834)  // 比较r9首字节与常量首字节
- char_cmp(r9, 0x8120c838)
- char_cmp(r9, 0x8120c83c)
- char_cmp(r9, 0x8120c840)
- char_cmp(r8, 0x8120c834)  // 同样比较r8
- char_cmp(r8, 0x8120c838)
- char_cmp(r8, 0x8120c83c)
- char_cmp(r8, 0x8120c840)
- **结论**：引擎大量使用字符串标识符（资源名、节点名、标签），且比较时忽略大小写（容错设计）。

---

## `docs\dancer_slots_runtime.md`

**标题**：舞者/舞台定位点 —— 运行时实测（ARCH_209） ｜ 表项 +0x700 起的定位点数组（每 16 字节：u32 id + 3 float） ｜ 判读 ｜ ★ 追加：上轮那个候选被推翻（AABB，不是舞者站位） ｜ 本轮确立的硬事实（可复用）

**发现/结论**（14 条，最多列 40）：

- 宿主机：Vita3K (JIT)，客机内存映射 = 宿主 0x400000000 + 客机地址
- *(0x8132F460) = 0x826BB98C   ← 运行时舞曲格式表指针（ARCH_165 §4 说文件里读不到）
- | 108 | 0x8270288C | (+0x700) id=147   (-80.3647, 140.1482, -4.7749)<br>(+0x710) id=0     (80.3647, 140.1482, -4.7749)<br>(+0x720) id=0     (-80.3647, 145.9247, -4.7749)<br>(+0x730) id=0     (80.3647, 145.9247, -4.7749)<br>(+0x740) id=0    
- | 101 | 0x826FC76C | (+0x700) id=0     (0.0000, 0.0000, 0.0000)<br>(+0x710) id=0     (0.0000, 0.0000, 0.0000)<br>(+0x720) id=0     (0.0000, 0.0000, 0.0000)<br>(+0x730) id=0     (0.0000, 0.0000, -0.0000)<br>(+0x740) id=0     (0.0000, -0.0000
- * x 严格成对取 ±80.3647 ⇒ 是**关于舞台中线对称**的空间点，不是随机数值；
- * y 两档（140.1482 / 145.9247）、z 两档（-4.7749 / +3.5080）⇒ 像是**同一组站位在不同高度/纵深上的采样**；
- * 只有**当前活动歌曲**的表项被填充，其余表项在 +0x700 处仍是 vita3k 的 E5E5E5E5 未初始化填充 ⇒ 运行时构造，与 ARCH_167 的结论一致。
- **stageId 109 里出现 4 份 `(+/-1,+/-1,+/-1)` 的单位立方体** ⇒ 铁证：这是 **AABB 包围盒**数据。
- ⇒ 上一轮在 stageId 108 的 `+0x700` 看到的 `+/-80.3647` 那 8 点，是**同一类包围盒**，
- 1. `*(0x8132F460)` = 运行时舞曲格式表指针（本轮实测 `0x826BB98C`，每次运行会变）；
- 反验：`0x826BB98C + 0x40000 + 8*0xDE0 = 0x8270288C`（与实读地址一致）。
- 3. **客机内存映射 = 宿主 `0x400000000` + 客机地址**（vita3k 用 JIT）；
- 实证：`"DancerEntity"` 字符串在宿主 `0x4812F0694` = `0x400000000 + 0x812F0694`；
- 4. ⇒ **现在可以直接读写 PSV 运行时内存了**，这是本轮最大的能力提升。

---

## `docs\env-setup.md`

**标题**：开发环境搭建指南 ｜ 已完成（本项目自动搭建） ｜ 需要手动安装的工具 ｜ 1. Ghidra（逆向分析） ｜ 2. Android NDK（安卓构建） ｜ 3. Vulkan SDK（图形开发） ｜ 4. Python（工具脚本） ｜ 5. PSV ROM导出工具 ｜ 桌面端编译测试 ｜ 参考代码获取 ｜ 下一步 ｜ Ghidra 已自动配置 ｜ Ghidra加载P5D步骤

**发现/结论**（2 条，最多列 40）：

- - **NoNpDrm**：PSV插件，用于导出游戏卡带为未加密格式
- 5. **Base Address设为 0x81000000**

---

## `docs\function_chain_analysis.md`

**标题**：函数调用链逆向分析：0x810650D8 -> 0x8107624c -> 0x81071676 ｜ 调用链概览 ｜ 0x81071676: 忽略空格的字符串比较函数 ｜ 基本信息 ｜ 函数签名 ｜ 反汇编分析 ｜ 功能确认 ｜ 0x8107624c: 字符串查找/匹配函数 ｜ 基本信息 ｜ 调用频率 ｜ 结构分析 ｜ 0x810650D8: 主循环中的字符串查找 ｜ 调用上下文 ｜ 修正之前的推测 ｜ 关键发现 ｜ 产出文件

**发现/结论**（70 条，最多列 40）：

- 主循环 0x810650D8 (28KB, 10045指令)
- └─ 调用 0x8107624c 90次
- └─ 调用 0x81071676 129次
- - **地址**: 0x81071676 - 0x810716F5
- 0x81071676: push {r4,r5,r6,lr}
- 0x81071678: mov r12, r3           ; r12 = len2
- 0x8107167a: adds r4, r2, #0       ; r4 = len1
- 0x8107167c: adds r3, r0, r4       ; r3 = s1 + len1 (s1结束地址)
- 0x8107167e: adds.w r2, r1, r12    ; r2 = s2 + len2 (s2结束地址)
- 0x81071682: cmp r4, r12           ; if len1 > len2
- 0x81071684: bgt 0x810716bc        ;   跳转到第二阶段（s2先结束）
- 0x81071686: cmp r0, r3            ; while (s1 < s1_end)
- 0x81071688: bcs 0x810716a4        ;   跳过空格处理
- 0x8107168a: ldrb.w r12, [r1]      ;   c2 = *s2
- 0x81071690: ldrb r4, [r0]         ;   c1 = *s1
- 0x81071692: cmp r4, r12           ;   if (c1 != c2)
- 0x81071694: beq 0x8107169c        ;
- 0x81071696: subs.w r0, r4, r12    ;     return c1 - c2
- 0x8107169a: b 0x810716f4          ;
- 0x8107169c: adds r0, #1           ;   s1++
- 0x8107169e: adds r1, #1           ;   s2++
- 0x810716a0: cmp r0, r3            ;
- 0x810716a2: bcc 0x8107168a        ; loop
- 0x810716a4: cmp r1, r2            ; while (s2 < s2_end)
- 0x810716a6: bcs 0x810716f2        ;   return 0
- 0x810716a8: ldrb r0, [r1]         ;   c = *s2
- 0x810716aa: cmp r0, #0x20         ;   if (c != ' ')
- 0x810716ac: beq 0x810716b4        ;
- 0x810716ae: rsbs.w r0, r0, #0x20  ;     return ' ' - c (负数)
- 0x810716b2: b 0x810716f4          ;
- 0x810716b4: adds r1, #1           ;   s2++ (跳过空格)
- 0x810716b6: cmp r1, r2            ;
- 0x810716b8: bcc 0x810716a8        ; loop
- 0x810716ba: b 0x810716f2          ; return 0
- 0x810716bc: cmp r1, r2            ; while (s2 < s2_end)
- 0x810716be: bcs 0x810716dc        ;   跳过空格处理
- 0x810716c0: ldrb r4, [r1]         ;   c2 = *s2
- 0x810716c4: ldrb.w r12, [r0]      ;   c1 = *s1
- 0x810716c8: cmp r12, r4           ;   if (c1 != c2)
- 0x810716ca: bne 0x810716d6        ;

---

## `docs\h1164_scene_sequence_forensics.md`

**标题**：P5D → Windows 移植：`scene sequence` 为何永不 ready —— 取证报告（基于 _h1164 及历史轮） ｜ 1. 结论摘要 ｜ 2. 现场证据 ｜ 2.1 最新一轮的尾部：运行是怎么结束的 ｜ 2.2 打死进程的两次 NULL 解引用（`_h1164.log`） ｜ 2.3 三个 NID 是什么，以及这个 spin 意味着什么 ｜ 2.4 终止时刻的线程/等待图（`_h1164.log:146755-146834`，逐字） ｜ 2.5 强制之后 app 什么都没做（`_h1163.log` 统计） ｜ 3. 序列状态机 ｜ 3.1 反汇编（`eboot.bin`，`_p5d_dis.py`，Thumb，H=0x10E0） ｜ 3.2 state 值时间线（`_h1164.log` 原行） ｜ 3.3 注入点与源码注释（`thumb_interpreter.cpp`） ｜ 4. HLE 缺口清单（按优先级） ｜ P0 — `sceKernelLockLwMutexCB` 语义错绑（不是缺失，是**错实现**） ｜ P1 — 资源"代泵"绕开 guest 自己的 job 队列，且完成位判定错误 ｜ P2 — `ASSERT-IGNORE`：把断言降级为"继续跑"，直接把可恢复错误变成空指针 ｜ P3 — title 序列"完成位"的数据源：异步加载以 `data=0x0` 报 DONE ｜ P4 — 空实现 import（`impl=N`，默认桩返回 0） ｜ P5 — 源码里明确的"缺口"清单（grep 结果，择要） ｜ 5. 下一步验证动作（可执行实验，按性价比排序） ｜ 附：本轮结论的边界与未证实项

**发现/结论**（189 条，最多列 40）：

- 源码 `src/loader/eboot_probe.cpp`、`src/arm/thumb_interpreter.cpp`；静态反汇编 `C:\dev\p5d_build\eboot.bin`（`_p5d_dis.py`，H=0x10E0）。
- 2. **HLE 层（为什么走不完）**：`titleSequence` state 6 会创建子任务（`0x81198248 bl 0x81193A00`），state 7 用 `0x81193A6A` 轮询它的完成位；完成位来自资源加载链（appResourceList / LRQ pump）。这条链在本移植里被 HLE **代泵**（`eboot_probe.cpp:955-1001`）绕开游戏自己的 job 队列直接 `call_guest`，并且在 `eboot_p
- 3. **同步原语层（终局阻塞）**：强制之后 guest 立刻掉进 **LwMutex try-lock 自旋**。`_h1164.log` 最后 400 条系统调用窗口里 **99% 只有三个 NID**：`0x0FB972F9 sceKernelGetThreadId` / `0x3148C6B6 sceKernelLockLwMutexCB` / `0x91FA6614 sceKernelUnlockLwMutex`（§2.3）。而 `eboot_probe.cpp:
- `[WAITGRAPH]` 显示 6 个 job worker 线程（entry `0x811E68C9`）全部 `wait=sema`，sema uid `0x11`/`0x14`(×4)/`0x17` 的 `cur count=0` —— **没有任何人 signal 它们**（`_h1164.log:146756-146761`）。
- 146580 [TSEQ-ST #960] titleSequence state=8 obj=0xB2D4F01C  (**>=8 ……尾部 return 0 = 结束**)
- 146581 [TSEQ2] own stage=8 mgr=0xB2D4F01C sub=3
- 146582 [LRQPAIR] mgr=0xB4E76030 <-> ctx=0xB4E76030（新配对）
- 146583 [ERROR] ThumbExec badmem addr=0x00000094 @pc=0x81181CFA sp=0xA016DE48 lr=0x8115FD5B r0=0x0 r1=0x0
- 146584 [EBOOT-START] stop=3(BADMEM) last_pc=0x81181CFA r0=0x0 sp=0xA016DE48 curThread=0x0 totalInsn=708800000 (start=0x81198464)
- | `_h1164.log` | 立刻 | `stop=3(BADMEM) last_pc=0x81181CFA totalInsn=708800000`（第 146584 行） |
- | `_h1163.log` | 继续跑 | `stop=6(RUNNING) last_pc=0x8115DA60 totalInsn=1200000000`（第 396362 行） |
- | `_h1162.log` | 继续跑 | `stop=6(RUNNING) last_pc=0x812B0BD0 totalInsn=1200000000`（第 328792 行） |
- | `_h1161.log` | 继续跑 | `stop=6(RUNNING) last_pc=0x81185F66 totalInsn=1200000000`（第 90615 行） |
- | `_h1160.log` | 继续跑 | `stop=6(RUNNING) last_pc=0x812B0BE0 totalInsn=1200000000`（第 77309 行） |
- | `_h1152.log` | 无 TSEQ-SKIP（未开注入） | `stop=6(RUNNING) last_pc=0x811A872E totalInsn=1200000000`（第 400535 行） |
- ⇒ `_h1164` 是**回归**：它在强制完成序列之后仅约 1 亿条指令（708.8M vs 1200M）就被 NULL 解引用打死。`stop=3` 是 `BADMEM` 停机码，说明不是 insn 配额耗尽（配额 1.2G 未到）。
- 146514 [ASSERT-SITE] caller=0x8117BB80 r0=0x81307D60 r1=0xCF r2=0x81307DB4 r3=0xF8
- 146515 [ASSERT-MSG] ASSERT:%s?E:\P3DP5D\xrd757\program\app\app_system\utility\appResourceList\appResourceList.c:207? | lr=0x8115F8AC sp=0xA016D9A8
- 146516 [ASSERT-IGNORE] #7 lr=0x8115F9D0
- 146517 [ERROR] ThumbExec badmem addr=0x00000004 @pc=0x8117BB80 sp=0xA016DFE8 lr=0x8117BB81 r0=0x812B4150 r1=0x812B4150
- 146518 [PUMP-DONE] req=0xB4E7802C ret=0x812B4150 f4 0x0 -> 0x1
- 146523 [ASSERT-SITE] caller=0x8117BB80 r0=0x81307D60 r1=0xCF r2=0x81307DB4 r3=0xF0
- 146524 [ASSERT-MSG] ASSERT:%s?E:\P3DP5D\xrd757\program\app\app_system\utility\appResourceList\appResourceList.c:207? | lr=0x8115F8AC sp=0xA016D9A8
- 146526 [ERROR] ThumbExec badmem addr=0x00000004 @pc=0x8117BB80 sp=0xA016DFE8 lr=0x8117BB81 r0=0x812B4150 r1=0x812B4150
- 146527 [PUMP-DONE] req=0xB4E7A02C ret=0x812B4150 f4 0x4 -> 0x5
- 断言串已从 `eboot.bin` 直接取出（不是从 mojibake 推的）：
- * `VA 0x81307D60` → `"E:\P3DP5D\xrd757\program\app\app_system\utility\appResourceList\appResourceList.c"`
- * `VA 0x81307DB4` → `"ASSERT:%s"`
- * `r1=0xCF` = **207** ⇒ 断言位置 = `appResourceList.c:207`；`r3` = 查表键（= 该 LRQ 请求的 `mode`：`0xF8`=TITLE_BIN、`0xF0`=CLOTH）。
- 8117BB56  bne       #0x8117bb5a
- 8117BB58  b         #0x8117bb64
- 8117BB60  blt       #0x8117bb52
- 8117BB62  movs      r5, #0                ; ← 没找到 ⇒ r5 = NULL
- 8117BB68  cbnz      r5, #0x8117bb80       ; 找到才跳过断言
- 8117BB6A  movw      r0, #0x7d60           ; = 0x81307D60 文件串
- 8117BB6E  movw      r2, #0x7db4           ; = 0x81307DB4 "ASSERT:%s"
- 8117BB7C  bl        #0x8115f832           ; 断言处理器
- 8117BB80  ldr       r5, [r5, #4]          ; ★ r5==0 ⇒ 读 0x00000004 ⇒ BADMEM
- 8117BB82  cbz       r5, #0x8117bb94
- `badmem addr=0x00000004 @pc=0x8117BB80 lr=0x8117BB81` 与 `8117BB80 ldr r5,[r5,#4]`、`r5==0` **完全吻合**（`lr=0x8117BB81` 是上一句 `8117BB7C bl` 留下的返回地址）。

---

## `docs\libsmart_summary.md`

**标题**：libsmart.suprx (GFD引擎核心) 分析总结 ｜ 基本信息 ｜ 内存布局 ｜ Ghidra分析结果（Thumb模式，已完成） ｜ 函数大小分布（Thumb模式） ｜ 最大函数（核心功能候选） ｜ 最大函数0x810650D8深度分析 ｜ 导出函数 ｜ 字符串发现 ｜ 关键发现 ｜ 产出文件 ｜ 下一步

**发现/结论**（18 条，最多列 40）：

- - **入口点**: 0x810AB528
- | .text | 0x81000000 | 0x20DAF4 (2.1MB) | R-X |
- | .data | 0x8120E000 | 0x3444 (13KB) | RW- |
- - **反汇编指令**: 930,414条（0失败，ARM模式有大量失败）
- | 0x810650D8 | 28KB (14134指令) | 主更新/渲染循环（ECS组件系统） |
- | 0x81077250 | 6.8KB | 动画/舞蹈更新 |
- | 0x81115D0C | 6.3KB | 场景/资源管理 |
- | 0x8107ABB8 | 5.6KB | 渲染提交 |
- | 0x810A0CD8 | 5.6KB | 物理/碰撞 |
- - 多次调用getter函数 `0x81074a88` 获取不同组件
- - **结论**: 极可能是引擎主更新循环，遍历实体并更新各组件
- - 共248个导出函数，地址范围 0x81000F01 - 0x810A2D45
- 1. **Thumb-2模式**: 所有代码都是Thumb-2，Ghidra需设置TMode=1才能正确反汇编
- - `docs/libsmart_thumb_report.txt` — Thumb模式完整反汇编报告（2527函数+最大函数200条指令）
- - [x] 用Ghidra Thumb模式重新反汇编，正确识别指令
- - [ ] 深度逆向最大函数0x810650D8（主循环）的完整控制流
- - [ ] 分析getter函数0x81074a88，理解组件系统
- - [ ] 建立导出函数NID→名称映射

---

## `docs\main_loop_analysis.md`

**标题**：主循环 0x810650D8 深度逆向分析 ｜ 函数概览 ｜ 函数序言 ｜ 调用频率分析 ｜ 循环结构（10个） ｜ 4对双入口循环（每对有两个入口点） ｜ 2个小循环（函数末尾） ｜ 调用密度分布 ｜ ECS组件系统证据 ｜ 推测的主循环结构 ｜ 关键发现 ｜ 产出文件 ｜ 下一步

**发现/结论**（50 条，最多列 40）：

- - **地址**: 0x810650D8 - 0x8106BF43
- 0x810650d8: push {r4,r5,r6,r7,r8,r9,r10,r11,lr}  ; 保存9个寄存器
- 0x810650dc: vpush {s16,s17}                          ; 保存VFP寄存器
- 0x810650e0: sub sp,#0x17c                            ; 380字节栈帧
- | 0x8107624c | 90 | 矩阵/向量数学运算（最频繁） |
- | 0x8106bf44 | 70 | 共享尾部/内联辅助（函数结束后地址） |
- | 0x8106dfac | 48 | 实体更新 |
- | 0x81077250 | 40 | 动画/舞蹈更新（6.8KB大函数） |
- | 0x81078ce0 | 36 | 变换计算 |
- | 0x8106dd98 | 32 | 组件处理 |
- | 0x810795a0 | 16 | 碰撞/物理 |
- | 0x810791fc | 16 | 渲染准备 |
- | 0x81074ab4 | 16 | 组件获取变体 |
- | 0x8106e3bc | 15 | 状态更新 |
- | 0x8106e86c | 15 | 事件处理 |
- | 0x8106ed38 | 14 | 资源访问 |
- | 0x81074a88 | 8 | 组件getter（ECS核心） |
- 1. **0x810671b4/0x8106719a -> 0x8106723a/0x8106723c** (134-162字节)
- 2. **0x810675cc/0x810675b2 -> 0x8106765e/0x81067660** (146-174字节)
- 3. **0x81069920/0x8106990a -> 0x810699b2/0x810699b4** (146-170字节)
- 4. **0x81069d40/0x81069d32 -> 0x81069de0/0x81069de2** (160-176字节)
- 5. **0x8106bd72 -> 0x8106bd84** (18字节) - 内存清零/拷贝
- 6. **0x8106bda0 -> 0x8106bdb2** (18字节) - 内存清零/拷贝
- - 0x810650D8-0x810680D8: ~30 calls/2KB（初始化+前半处理）
- - 0x810680D8-0x810690D8: 34-36 calls/2KB（峰值，核心处理）
- - 0x810690D8-0x8106B8D8: ~32 calls/2KB（后半处理）
- - 0x8106B8D8-0x8106BF43: 23 calls/2KB（收尾清理）
- - 组件getter `0x81074a88` 被调用8次，每次传入不同组件ID
- - 调用模式: `movw r1, #0xc834; movt r1, #0x8120; mov r0, <entity>; bl 0x81074a88`
- // 阶段1: 初始化 (0x810650D8-0x810658D8)
- // 阶段2: 遍历实体更新 (0x810658D8-0x810698D8)
- 获取变换组件 -> 0x81074a88(0xc834)
- 获取动画组件 -> 0x81074a88(0xc838)
- 获取渲染组件 -> 0x81074a88(0xc83c)
- 获取物理组件 -> 0x81074a88(0xc840)
- 0x81077250()  // 动画更新
- 0x8107624c()  // 矩阵运算
- 0x810795a0()  // 碰撞检测
- 0x8106dfac()  // 实体响应
- // 阶段3: 渲染提交 (0x810698D8-0x8106B8D8)

---

## `docs\math_helper_analysis.md`

**标题**：0x810715a6 数学辅助函数 逆向分析 ｜ 函数身份 ｜ 基本信息 ｜ 函数签名 ｜ 完整反汇编 ｜ C伪代码 ｜ 算法分析 ｜ 阶段1：计算|a| ｜ 阶段2：单精度转双精度 ｜ 阶段3：应用b的符号 ｜ 在LAPACK中的用途 ｜ 为什么不直接用fabs + copysign？ ｜ 对移植的意义 ｜ VFP指令统计 ｜ 产出文件

**发现/结论**（20 条，最多列 40）：

- - **地址**: 0x810715a6 - 0x810715DF
- 0x810715a6: vldr.32 s0, [r0]      ; s0 = *a (加载第一个浮点数)
- 0x810715aa: movs r0, #0           ; r0 = 0
- 0x810715ac: vmov s2, r0           ; s2 = 0.0f
- 0x810715b0: vldr.32 s1, [r1]      ; s1 = *b (加载第二个浮点数)
- 0x810715b4: vcmpe.f32 s0, s2      ; 比较 s0 和 0.0
- 0x810715b8: vmrs apsr, fpscr      ; 读取FPSCR状态寄存器
- 0x810715bc: bge 0x810715c2        ; 如果 s0 >= 0，跳过取反
- 0x810715be: vneg.f32 s0, s0       ; s0 = -s0 (取绝对值)
- 0x810715c2: vcvt.f64.f32 d16, s0  ; d16 = (double)|s0| (单精度转双精度)
- 0x810715c6: vcmpe.f32 s1, s2      ; 比较 s1 和 0.0
- 0x810715ca: vmrs apsr, fpscr      ; 读取FPSCR
- 0x810715ce: vneg.f64 d17, d16     ; d17 = -d16 (先假设b为负)
- 0x810715d2: bge 0x810715d6        ; 如果 s1 >= 0，跳转到正数分支
- 0x810715d4: b 0x810715da          ; 保持d17 = -d16
- 0x810715d6: vmov d17, d16         ; d17 = d16 (b为正，返回正数)
- 0x810715da: vmov d0, d17          ; d0 = 返回值
- 0x810715de: bx lr                 ; 返回
- 4. **LAPACK依赖**：进一步证实引擎静态链接了LAPACK
- - `docs/math_helper_0x810715a6.txt` — 完整反汇编

---

## `docs\progress.md`

**标题**：进度跟踪 ｜ 当前阶段：阶段0 - 环境搭建 ｜ 阶段0：环境搭建与准备 ｜ ROM提取与文件分析详情（已完成） ｜ 阶段1：资源格式逆向 ｜ 资源结构详情（已完成） ｜ 阶段2：可执行文件逆向 ｜ Ghidra分析详情（已完成） ｜ 阶段3：加载器与核心系统 ｜ 阶段4：图形翻译层 ｜ 阶段5：音频与输入 ｜ 阶段6：游戏逻辑跑通 ｜ 阶段7：安卓优化与发布 ｜ 问题记录 ｜ 里程碑（2026-09-13）：USSE VDP 矩阵变换彻底打通，游戏着色器成功出图并定量验证 ｜ 根因定位：VDP 两个操作数角色被搞反 ｜ 落地修改（src/usse/usse_translator.cpp，generate_vertex_shader_from_usse64） ｜ 验证结果（MSVC + RTX5060，960x544） ｜ 下一阶段架构决策：顶点解包交给 Vulkan 固定功能 ｜ 里程碑（2026-09-13）M1：GXM draw 状态机打通，顶点不再写死 ｜ 目标 ｜ 落地改动 ｜ 定量验证（MSVC + RTX5060，960x544） ｜ 意义与下一步

**发现/结论**（10094 条，最多列 40）：

- - **eboot.bin**: SELF格式，ELF从偏移0xA0提取，ARMv7，入口点0x002B2B00
- - **ELF段**: 代码段vaddr=0x81000000 size=0x321810 (R-X)，数据段vaddr=0x81325000 filesz=0x21BA74 memsz=0x3AA4A0 (RW-)
- - **资源包**: data.cpk(1825MB) + vita.cpk(1510MB)，均已解密（CPK格式，熵值3.12）
- - **参考架构**: vita2hos的load.c已研读，掌握SCE_Rel格式（short=8字节/long=12字节）和sce_module_info结构
- - [x] ELF格式与段结构分析
- - [x] Ghidra深度反汇编（9835函数，116万条指令）
- - [x] 导入NID→函数名映射（vita-headers db/360完整NID数据库，9274函数，275库，30个导入模块全部匹配）
- - [x] 主循环与场景管理分析（0x810650D8主循环已逆向，28KB，ECS遍历）
- - [x] NID数据库获取（vita-headers已克隆到refs/vita-headers，翻译层函数清单已生成）
- - **内存映射**: 代码段0x81000000(size=0x321810) + 数据段0x81325000(size=0x3AA4A0)
- - **入口点**: 0x812B2B00
- - **反汇编**: 1,166,739条ARM指令
- - [x] SELF/ELF解析器（完整实现：ET_SCE_RELEXEC支持、程序头偏移修正0x34→0x40）
- - [x] NID解析器框架（模块NID定义、函数NID映射基础结构）
- - [x] NID映射表集成（SceIofilemgr 59函数 + 内核6库30函数）
- - [x] sceGxm 完整函数框架（267函数声明+stub实现+NID映射表）
- - [x] 交换链创建（vkCreateSwapchainKHR + 格式/呈现模式选择）
- - [x] GXP格式分析（头部580字节，USSE代码从0xC0开始，uniform变量名）
- - [x] USSE反汇编器框架（usse_translator.cpp，基础指令识别）
- 对照 Vita3K `alu.cpp::vdp` / `usse_utilities.cpp::load` / `usse_decode_helpers.cpp`，并 dump 游戏首个 GXP 的全部 35 条 64 位 USSE 指令后确认：
- - **动态矩阵（旋转+aspect+平移0.35,0.25）**：实测包围盒 X500-769/Y69-351，与按矩阵手算的理论值 X501-772/Y69-351 误差 <3px；平移项 w 正确生效（旧实现会丢平移、停在屏幕中心）。
- - 片元 64 位指令尚未翻译，临时输出固定绿色 (0.12,0.90,0.12,1) 以便目视确认（见 milestone_vdp_fixed_green_triangle.png）。
- 指令 0-30（I32MAD/I16MAD/I8MAD=整数乘加 dest=src1*src2+src0、SOP2M 位运算、VMAD2、VPCK）用于把 8/16 位打包顶点格式解成浮点。**原生移植策略：解析 GXP 顶点属性的 SceGxmAttributeFormat，直接映射到对应 VK_FORMAT（如 R8G8B8A8_UNORM、R16G16_SNORM），由 Vulkan 顶点装配硬件自动 unpack 成 float，这些整数解包指令在翻译层即可跳过**，
- - 仍缺：SetVertexProgram 时尚未按 GXP 实时翻译并缓存 pipeline（当前 pipeline 由测试预建）；顶点格式仍固定 vec4+vec3、未按 GXP attribute format 动态声明；uniform 仍由测试每帧填充，缺 sceGxmSetUniformDataF 真实实现。下一步 M2：纹理 + 片元真实翻译，显示真实游戏画面元素。
- - 排查中确认：sampler/image 类型声明、UniformConstant 变量、Binding/DescriptorSet 装饰、OpLoad SampledImage、varying vec2 输入本身都合法，单独存在不会崩。
- - **结果**：`[SMP] AUTO-TRANSLATED SMP fragment pipeline created OK`；反汇编 smp_fragment.spv 确认 OpTypeImage(2D)/OpTypeSampledImage/OpImageSampleImplicitLod(87, coord=vec2) 各 1 条、EntryPoint interface=[out_color,in_color,in_uv,u_smp]，1360 word 线性遍历
- - 结论：游戏真实 USSE 二进制里的 SMP 已能被**自动**识别并翻译成 Vulkan 可执行的采样，不再依赖手写 SPIR-V；字段位区间、bank 解码、double_reg、combined sampler 模式全部经真实 GXP 验证。
- - 重拉 Vita3K texture.cpp（ref 84184a36）确认：lod_mode 为 2 位（0-3），**Proj 不由 lod 字段编码**而由坐标投影分量 prod_pos 决定；**Bias(lod1) 配 OpImageSampleImplicitLod(87)+Bias**（非 ExplicitLod），Lod(lod2)/Grad(lod3) 才配 OpImageSampleExplicitLod(88)。
- - 经 Vita3K `grammar.yaml`(SHA c8c81cf2) + `texture.cpp`(SHA d2b39e48) 权威确认：USSE SMP 的 dim 是 bits[43:42] Imm2，field0=1D、field1=2D、**field2=Cube（坐标是 xyz 三分量方向向量，硬件按最大分量选面，SPIR-V Dim=3）**、field3=3D volume(Dim=2)。
- 把测试用程序化棋盘替换为从游戏 data.cpk 提取的真实纹理；按 GXP sampler 参数表的 texture unit(resource_index) 绑定 descriptor，而不是写死 binding1/2；打通 SceGxmTexture 格式→VkFormat。
- - `parse_gxp_samplers`：按 Vita3K 权威布局解析 SceGxmProgramParameter（category=2 SAMPLER，resource_index=texture unit，is_cube=semantic_index bit4）。实测 #114=[diffuse u0/displacement u1]、#781=[diffuse u0/reflectionMap u3 cube]、双 sampler 例=[u0/u2/u3/u10
- - 证明：真实 GXP sampler 表→texture unit→descriptor binding=unit+1 的多 sampler 模型成立，真实 BC1/BC3 游戏纹理可被原生 Vulkan 采样；为运行时 SceGxmTexture 绑定打下格式与绑定基础。
- - vulkan_backend：upload_texture_to_unit 增 4 个 swizzle 参数（默认 IDENTITY），在 VkImageViewCreateInfo.components 上赋值；load_dds_to_unit 增 swizzle_index（0xFFFFFFFF=IDENTITY，0..7 走 texture_swizzle4 并打印映射）；新增 upload_rgba_swizzled()（RGBA8 按 PSV swizzle 重
- - **base_format 掩码 0x9F000000**：5bit(bits28:24) **外加 bit31(format0)**；UBC1=0x85000000 的 0x80 位即 format0。get_format = base<<24 | format0<<31 | swizzle<<12。
- - SceGxmTextureInfo 补 data_addr_word/data_byte_addr(=word<<2)/palette/format0/normalize/lod_bias/is_cube；**修正 parse**：宽高按 type 解码、base_format 合并 format0<<31、w0 位域偏移订正(mip_count>>17、gamma>>27)、data_addr 还原。
- - **Init->parse 往返自检**：InitSwizzled(UBC1|ABGR,256x256,vaddr=0x10000000) 重新 resolve 得 256x256/base=0x85000000/vkfmt=133(BC1)/comp=4/type=0/vaddr 完全一致（ROUNDTRIP OK）；InitLinear 非二次幂 130x77 正确走 field+1（type=3）。
- - 测试把角色脸 BC1 字节 register_region(0x10000000)，Init 控制块 data 只填该 vaddr，SetFragmentTexture(unit1)：
- - 负例 vaddr=0x77700000 未注册：translate 失败、resolved 返回 -3 并打 ERROR，安全跳过。
- 游戏 sceKernelAlloc(注册 vaddr→host) → sceGxmMapMemory(标记可见) → sceGxmTextureInit*(打包控制块) → sceGxmSetFragmentTexture(绑定 unit) → sceGxmDraw 前 materialize_fragment_textures(translate vaddr + 脏缓存 + 格式/swizzle 解析 + 上传 Vulkan unit) → shader 按 sa 精确绑定
- - mip 链/stride/pitch、cube 6 面 vaddr 连续布局；PVRTC/调色板转码；运行时从真实命令流批量捕获控制块。

---

## `docs\psv-api-surface.md`

**标题**：P5D (PCSH10109) PSV API 导入清单 ｜ 模块概览 ｜ 按翻译层分类 ｜ 图形 (sceGxm → Vulkan) ｜ 音频 (sceAudio → AAudio/OpenSL ES) ｜ 输入 (sceCtrl/sceTouch → NDK Input) ｜ 文件系统 (sceIo → stdio/AAsset) ｜ 内核/系统 (sceKernel → pthread/NDK) ｜ C/C++标准库 ｜ 网络/PSN（可stub） ｜ 其他 ｜ 实现优先级建议 ｜ 第一阶段（启动必需） ｜ 第二阶段（图形初始化） ｜ 第三阶段（音频+输入） ｜ 第四阶段（完整功能） ｜ 可直接stub（返回成功）

**发现/结论**（61 条，最多列 40）：

- | # | 模块名 | 模块NID | 函数数 | 变量数 | 翻译层优先级 |
- | 0 | SceAppMgrUser | 0xA6605D6F | 2 | 0 | 中 |
- | 1 | SceGxm | 0xF76B66BD | 100 | 0 | **高** |
- | 2 | SceDisplayUser | 0x4FAACD11 | 1 | 0 | **高** |
- | 3 | SceDisplay | 0x5ED8F994 | 2 | 0 | **高** |
- | 4 | SceCtrl | 0xD197E3C7 | 2 | 0 | **高** |
- | 5 | SceSysmodule | 0x03FCF19D | 3 | 0 | **高** |
- | 6 | SceRtcUser | 0x6704D3C6 | 4 | 0 | 中 |
- | 7 | SceAppUtil | 0x48E01D74 | 8 | 0 | 中 |
- | 8 | SceTouch | 0x3E4F4A81 | 4 | 0 | **高** |
- | 9 | SceNet | 0x3AD4D630 | 12 | 0 | 低 |
- | 10 | SceNetCtl | 0x6BF8B2A2 | 1 | 0 | 低 |
- | 11 | SceCommonDialog | 0xC6083AE1 | 1 | 0 | 低 |
- | 12 | SceFiber | 0xE537816C | 18 | 0 | 中 |
- | 13 | SceNpManager | 0xF88DAA70 | 6 | 0 | 低 |
- | 14 | SceNpActivity | 0xD8835093 | 6 | 0 | 低 |
- | 15 | SceHttp | 0xFEF492ED | 3 | 0 | 低 |
- | 16 | SceSsl | 0xE8F15CDE | 1 | 0 | 低 |
- | 17 | SceNpScore | 0xCAF5F2E9 | 1 | 0 | 低 |
- | 18 | SceNpTrophy | 0x75F052DB | 12 | 0 | 低 |
- | 19 | ScePower | 0x4332C10D | 5 | 0 | 中 |
- | 20 | SceGameUpdate | 0x1082DA7F | 1 | 0 | 低 |
- | 21 | SceNpTus | 0x2DD9A62D | 3 | 0 | 低 |
- | 22 | SceAudio | 0xAE84BDEA | 13 | 0 | **高** |
- | 23 | SceNgs | 0x438BB957 | 5 | 0 | 低 |
- | 24 | SceVideodecUser | 0xB01598D9 | 34 | 0 | 低 |
- | 25 | SceLibc | 0x163C3727 | 9 | 0 | 中 |
- | 26 | SceLibm | 0xBE43BB07 | 40 | 3 | 中 |
- | 27 | SceLibstdcxx | 0xCDAE3C7D | 54 | 0 | 中 |
- | 28 | SceLibKernel | 0xF7629240 | 1 | 0 | **高** |
- | 29 | SceIofilemgr | 0xCAE9ACE6 | 29 | 1 | **高** |
- - **SceGxm**: 100 函数 (NID=0xF76B66BD)
- - **SceDisplayUser**: 1 函数 (NID=0x4FAACD11)
- - **SceDisplay**: 2 函数 (NID=0x5ED8F994)
- - **SceAudio**: 13 函数 (NID=0xAE84BDEA)
- - **SceNgs**: 5 函数 (NID=0x438BB957)
- - **SceCtrl**: 2 函数 (NID=0xD197E3C7)
- - **SceTouch**: 4 函数 (NID=0x3E4F4A81)
- - **SceIofilemgr**: 29 函数 (NID=0xCAE9ACE6)
- - **SceAppMgrUser**: 2 函数 (NID=0xA6605D6F)

---

## `docs\transform_eval_analysis.md`

**标题**：0x8107f9f0 3D变换/动画评估函数 逆向分析 ｜ 函数身份确认 ｜ 基本信息 ｜ VFP浮点操作统计 ｜ 关键发现：双精度浮点计算 ｜ 调用频率分析（TOP 15） ｜ ECS组件系统集成 ｜ 函数结构推测 ｜ 导入函数分析 ｜ 对移植的意义 ｜ 产出文件

**发现/结论**（25 条，最多列 40）：

- - **地址**: 0x8107f9f0 - 0x81085CA9
- ; 向量归一化典型模式（0x81080bae区域）
- blx 0x810ac348           ; 调用sqrt库函数（导入函数）
- | 0x81074a88 | **48** | ECS组件getter（获取大量组件） |
- | 0x810715a6 | 20 | 数学辅助函数 |
- | 0x81075148 | 18 | 矩阵/向量运算 |
- | 0x810ac348 | 15 | **导入函数：sqrt/数学库** |
- | 0x810ac418 | 14 | 导入函数 |
- | 0x8108640c | 14 | 矩阵运算 |
- | 0x8106d70c | 14 | 未知辅助 |
- | 0x810861b4 | 12 | 四元数运算 |
- | 0x81076230 | 11 | 字符串/查找 |
- | 0x8107593c | 11 | 向量运算 |
- | 0x8107581c | 8 | 矩阵运算 |
- | 0x8106c0a8 | 5 | 辅助函数 |
- | 0x8107c19c | 4 | 骨骼运算 |
- | 0x81086810 | 4 | 四元数 |
- | 0x8107f9f0 | 4 | **递归调用自身！** |
- 函数调用0x81074a88（组件getter）**48次**，获取多种组件：
- // 四元数插值 (0x810861b4, 0x81086810)
- // 矩阵变换 (0x8108640c, 0x81075148)
- - **0x810ac348** (15次调用): 在vsqrt之后调用，很可能是 `sqrt()` 或 `sqrtf()` 库函数
- - **0x810ac418** (14次调用): 可能是其他数学库函数（sin/cos/atan2等）
- 3. **导入函数**: 0x810ac348/0x810ac418需要映射到NDK数学库
- - `docs/transform_eval_0x8107f9f0_full.txt` — 完整反汇编（8369条指令）

---

## `docs\translation_layer_functions.md`

**标题**：P5D 翻译层函数清单 ｜ SceAppMgrUser (NID=0xA6605D6F) ｜ SceGxm (NID=0xF76B66BD) ｜ SceDisplayUser (NID=0x4FAACD11) ｜ SceDisplay (NID=0x5ED8F994) ｜ SceCtrl (NID=0xD197E3C7) ｜ SceSysmodule (NID=0x03FCF19D) ｜ SceRtcUser (NID=0x6704D3C6) ｜ SceAppUtil (NID=0x48E01D74) ｜ SceTouch (NID=0x3E4F4A81) ｜ SceNet (NID=0x3AD4D630) ｜ SceNetCtl (NID=0x6BF8B2A2) ｜ SceCommonDialog (NID=0xC6083AE1) ｜ SceFiber (NID=0xE537816C) ｜ SceNpManager (NID=0xF88DAA70) ｜ SceNpActivity (NID=0xD8835093) ｜ SceHttp (NID=0xFEF492ED) ｜ SceSsl (NID=0xE8F15CDE) ｜ SceNpScore (NID=0xCAF5F2E9) ｜ SceNpTrophy (NID=0x75F052DB) ｜ ScePower (NID=0x4332C10D) ｜ SceGameUpdate (NID=0x1082DA7F) ｜ SceNpTus (NID=0x2DD9A62D) ｜ SceAudio (NID=0xAE84BDEA)

**发现/结论**（3362 条，最多列 40）：

- 基于vita-headers db/360 NID数据库生成
- 总NID数据库: 9274 个函数
- - NID数据库: 128 函数
- | 函数名 | NID | 优先级 |
- | _sceAppMgrGetAppState | 0x5E86319A | 高 |
- | sceAppMgrAcidDirSet | 0x31EFA73A | 中 |
- | sceAppMgrAcquireSoundOutExclusive3 | 0xD97AC34E | 低 |
- | sceAppMgrAddContAddMount | 0x2B40BA12 | 低 |
- | sceAppMgrAddContMount | 0x553AA57B | 低 |
- | sceAppMgrAppDataMount | 0x291AFBDC | 低 |
- | sceAppMgrAppDataMountById | 0xE8BD32BD | 低 |
- | sceAppMgrAppMount | 0x127F3BEB | 低 |
- | sceAppMgrAppParamGetInt | 0x9E57853D | 高 |
- | sceAppMgrAppParamGetString | 0x0C733AE2 | 高 |
- | sceAppMgrAppParamSetString | 0x5B07C10B | 中 |
- | sceAppMgrAppUmount | 0x9619C150 | 低 |
- | sceAppMgrBgdlGetQueueStatus | 0x7BC8D5B6 | 高 |
- | sceAppMgrCaptureFrameBufDMACByAppId | 0x44F8347C | 低 |
- | sceAppMgrCaptureFrameBufIFTUByAppId | 0x8C24D566 | 低 |
- | sceAppMgrCheckRifGD | 0x6E5CBCA2 | 低 |
- | sceAppMgrContentInstallPeriodStart | 0x251A224D | 高 |
- | sceAppMgrContentInstallPeriodStop | 0xB3DFC681 | 低 |
- | sceAppMgrConvertVs0UserDrivePath | 0xADAA658E | 低 |
- | sceAppMgrDeclareShellProcess2 | 0xF261879F | 低 |
- | sceAppMgrDestroyAppByName | 0x4570DC15 | 低 |
- | sceAppMgrDrmClose | 0x29293B46 | 低 |
- | sceAppMgrDrmOpen | 0xD43FDF20 | 高 |
- | sceAppMgrForceUmount | 0x54AD0F7F | 低 |
- | sceAppMgrGameDataMount | 0x4993876C | 低 |
- | sceAppMgrGetAppInfo | 0x72ABDC61 | 高 |
- | sceAppMgrGetAppMgrState | 0x98724E3F | 高 |
- | sceAppMgrGetAppParam | 0x9B2422E0 | 高 |
- | sceAppMgrGetAppParam2 | 0x3D816DFA | 高 |
- | sceAppMgrGetBootParam | 0x73E5D57D | 高 |
- | sceAppMgrGetBudgetInfo | 0x5F00F261 | 高 |
- | sceAppMgrGetCoredumpStateForShell | 0x27CEDB95 | 高 |
- | sceAppMgrGetCurrentBgmState | 0xACB3826A | 高 |
- | sceAppMgrGetCurrentBgmState2 | 0x62BEBD65 | 高 |
- | sceAppMgrGetDevInfo | 0x67705391 | 高 |
- | sceAppMgrGetFgAppInfo | 0x59F8C1C1 | 高 |

---

## `docs\usse_instruction_set.md`

**标题**：USSE指令集逆向分析报告 ｜ 概述 ｜ GXP文件格式 ｜ 文件结构 ｜ USSE代码布局 ｜ 示例（第一个GXP） ｜ USSE指令编码格式 ｜ 位字段分配 ｜ 操作码表 ｜ 特殊操作（opcode=0x3E） ｜ 寄存器映射 ｜ 通用寄存器 ｜ 特殊寄存器 ｜ 寄存器区间 ｜ 顶点着色器分析（骨骼蒙皮） ｜ 操作序列模式 ｜ 片元着色器分析 ｜ Uniform变量 ｜ 翻译策略 ｜ USSE→SPIR-V映射 ｜ 寄存器→SPIR-V变量 ｜ 待完善 ｜ 参考

**发现/结论**（4 条，最多列 40）：

- - **Version**: 0x03500501 (0x04)
- - **分隔**: 连续>=2个nop (0x00000000)
- 2. **寄存器语义**: 需要确认64-127/128-191区间的准确含义
- 3. **立即数格式**: MOVI的立即数字段需要确认是整数还是浮点编码

---

## `docs\usse_opcode_coverage.md`

**标题**：USSE 指令覆盖度普查（真实 P5D 着色器库） ｜ 指令类型分布（按频率 = 翻译优先级） ｜ Vertex：1631 program / 83,921 指令 ｜ Fragment：2700 program / 80,157 指令 ｜ 权威 opcode 映射（来自 Vita3K usse_translator_entry.cpp，op1 = instr>>59 低 5 位） ｜ 翻译器实现优先级建议

**发现/结论**（2 条，最多列 40）：

- **结论：解码器（指令类型识别）对 164,078 条真实 primary 指令达到 100% 覆盖，UNKNOWN=0。**
- | 00010 | V16NMAD | F16 向量运算（格式同 V32NMAD） |

---

## `docs\vulkan_backend_architecture.md`

**标题**：Vulkan后端架构设计 ｜ 概述 ｜ 核心映射 ｜ 1. 初始化 ｜ 2. 渲染流程 ｜ 3. 状态设置 ｜ 4. 表面/渲染目标 ｜ 5. 内存映射 ｜ 关键技术挑战 ｜ 1. USSE着色器翻译 ｜ 2. 纹理格式转换 ｜ 3. 内存模型差异 ｜ 4. 同步模型 ｜ 文件结构 ｜ 条件编译 ｜ 实现进度 ｜ 参考实现

**发现/结论**（3 条，最多列 40）：

- PSV纹理格式（GXM格式）到Vulkan格式的映射：
- ├── gxm_texture.h       # 纹理格式转换（待实现）
- └── gxm_texture.cpp     # 纹理格式转换实现（待实现）

---

## `game\README.md`

**标题**：游戏ROM目录

---

## `tools\cpk_tools\vgmstream-win64\README.md`

**标题**：vgmstream ｜ Getting vgmstream ｜ Usage ｜ Windows binaries ｜ Linux binaries ｜ macOS binaries ｜ More info

---

## `tools\cpk_tools\vgmstream-win64\USAGE.md`

**标题**：Usage ｜ Needed extra files ｜ Components ｜ vgmstream-cli (command line decoder) ｜ in_vgmstream (Winamp plugin) ｜ Plugin priority ｜ foo_input_vgmstream (foobar2000 plugin) ｜ Playlist issues ｜ Plugin priority ｜ Default title and playlist columns ｜ xmp-vgmstream (XMPlay plugin) ｜ Missing subsongs ｜ Plugin priority ｜ Audacious plugin ｜ Playlist issues ｜ Enabling subsongs ｜ Plugin priority ｜ vgmstream123 (command line player) ｜ Extra features ｜ Related projects ｜ Special cases ｜ Subsongs ｜ Common and unknown extensions ｜ Related issues

**发现/结论**（1 条，最多列 40）：

- 0x3a160928.srsa: 0x272c6efb.srst

---

## `tools\ghidra_scripts\README.md`

**标题**：Ghidra 逆向分析脚本 ｜ 环境配置 ｜ 脚本列表 ｜ analyze_imports.py ｜ PSV逆向要点 ｜ NID系统 ｜ 模块结构 ｜ 关键分析目标

**发现/结论**（5 条，最多列 40）：

- PSV不使用字符串导入，而是使用32位NID（Name ID）哈希。
- - 函数名 → NID的映射表需要从PSV SDK或已知文档获取
- - 常用NID数据库：`psv_nid_db.txt`（需自行收集）
- - `eboot.bin`：主程序（SELF格式，解密后为ELF）
- 3. **资源加载**：找到文件加载函数，理解资源格式

---


## DANCE RUN-LOGIC (consolidated, master doc refs)
- state machine: danceMain_81043fee (VA 81043fee): states 0..0x14 (see master 18.2876..18.2883)
- frame pipeline: master 18.2879 (time = FUN_81043ee2 audio clock; delta (state[7]+DAT_8132F420)/30;
  time adjuster danceTimeAdjuster_81049c1e; order: event -> sequencer -> scene -> movie -> voice -> screen)
- init orchestrator: danceMain_81043b6e (VA 81043b6e) - music/voice/scene/screen/event/movie
- loop body: FUN_81042e24 (VA 81042e24) - singer mask, last cue 0x1D0, loop rebase state[0x10E],
  special id 0xE6
- pause/music-end: FUN_81042d44 (VA 81042d44), states 0xD/0x11, fades 10/0x14
- result: FUN_81043226 (VA 81043226) + m_app_dance_ui::result_81051324 / result_81051172
- shutdown object map: master 18.2880 (13 slots 0x41C..0x4A8)

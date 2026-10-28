#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""hle_probe.py — P5D 静态 AOT harness 的 HLE 桩调用探针（只读、离线）

用途
----
读取 recomp_harness.exe 的运行日志（out*.txt / err*.txt），统计各 HLE 桩
（guest 链接桩地址 0x812Bxxxx）的调用次数，按 NID / 模块 / 函数名汇总，
并找出「高频 + harness 只返回常量 / 根本没有实现（默认返回 0）」的可疑桩，
用于定位“boot 后调度空转 / 线程饿死 / 加载卡住”一类根因。

数据来源（全部为本地只读文件）
------------------------------
* 日志里的三类结构化证据（harness recomp_harness.cpp:605/3450/2430 附近）:
    [HLE] #n va=812Bxxxx lr=... a0=...    harness 只打印前 6000 次 HLE 调用
    [HLECOUNT] 段内 "  812Bxxxx xN"       运行结束时按全量调用次数排序的 TOP40
    [WD] ... hle_ring: 812Bxxxx@lr ...   2 秒看门狗打印的最后 12 次 HLE 调用
* harness 源码（默认 C:\\dev\\tmp\\recomp_split\\recomp_harness.cpp）:
    解析 hle_call() 内 switch 的 case 地址，并对每个 case 做
    “常量返回 / 真有实现 / 未实现(缺 case)” 的启发式分类。
* 桩地址 -> NID 映射:
    docs/eboot_imports_with_nid.json      (guest stub VA -> func_nid/module)
    src/loader/nid_import_names.h        (func_nid -> 模块名/函数名)

用法
----
    py -3 tools\\hle_probe.py <log.txt> [<log2.txt> ...]
        [--top 30] [--min-count 100]
        [--harness PATH] [--map PATH] [--nid-names PATH]
        [--json out.json] [--quiet]

示例
----
    py -3 tools\\hle_probe.py C:\\dev\\tmp\\recomp_split\\out100m_682a2.txt
    py -3 tools\\hle_probe.py out30m_682a2.txt --top 15 --min-count 10
    py -3 tools\\hle_probe.py run.log --json probe.json

约束
----
只读；不访问网络；不读取/不输出任何游戏数据；仅使用 Python 标准库
（Python 3.8+）。harness 日志常为 PowerShell 重定向产生的 UTF-16LE，
本工具会自动识别 BOM / NUL 分布。
"""

import argparse
import json
import os
import re
import sys
from collections import Counter, defaultdict

# ---------------------------------------------------------------------------
# 路径与常量
# ---------------------------------------------------------------------------

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(SCRIPT_DIR)                     # ...\p5dsrc
DEFAULT_MAP = os.path.join(ROOT, "docs", "eboot_imports_with_nid.json")
DEFAULT_NID_NAMES = os.path.join(ROOT, "src", "loader", "nid_import_names.h")
DEFAULT_HARNESS_CANDIDATES = [
    os.path.join(os.path.dirname(ROOT), "tmp", "recomp_split", "recomp_harness.cpp"),
    r"C:\dev\tmp\recomp_split\recomp_harness.cpp",
]

# 已知“桩语义必须返回真实值/句柄，返回常量会导致 guest 行为错误”的 NID。
# 说明来自旧运行时 src/loader/eboot_probe.cpp（行号为当前工作区源码）。
KNOWN_RISK = {
    0x0FB972F9: "GetThreadId: 旧实现返回真实线程 id（主线程 0x4001 / worker 0x6001+，eboot_probe.cpp:6385-6403）；"
                "返回常量会让 guest 的 LwMutex owner 判定/主线程判定全部失效",
    0x4B675D05: "DelayThread: 旧 n_thread_yield 会 sched_save(PC=svc+2)+sched_pick+sched_enter 切到其他线程"
                "（eboot_probe.cpp:7441-7462）；只 return 0 不切换 = 让出无效",
    0x9C0180E1: "DelayThreadCB: 旧运行时与 DelayThread 同绑 n_thread_yield（eboot_probe.cpp:7568）；"
                "缺失时 guest 的 CriFs/线程等待循环 0x8127BE36/0x8127C0F8 空转",
    0x23F79274: "sceRtcGetCurrentTick: 旧实现写 64 位递增虚拟 tick 并推进时钟（n_rtc_tick eboot_probe.cpp:6921, ts_advance :6515-6524）；"
                "缺失时 guest 时钟恒 0",
    0xB110C123: "sceKernelGetProcessTimeWide: 旧实现返回递增 64 位 tick（eboot_probe.cpp:6629-6650）；"
                "缺失时定时等待/超时逻辑全部退化",
    0x3EFD3165: "sceKernelGetTimerTimeWide: 计时器时间，缺失时依赖它的超时可能立即到期或永不到期",
    0x46E7BE7B: "LockLwMutex: 旧实现登记持锁并回写 work+4(lockCount)/work+8(holder)（eboot_probe.cpp:7117-7124,7519-7557）；"
                "不写 work 时 guest 的 try-lock 永远读不到“自己是持有者”而自旋",
    0x3148C6B6: "LockLwMutexCB: 与 LockLwMutex 同实现（eboot_probe.cpp:7575）",
    0x91FA6614: "UnlockLwMutex: 旧实现递减 work 计数并清 holder（eboot_probe.cpp:7493-7508）",
    0xDA6EC8EF: "CreateLwMutex: 旧实现返回 0 并清零 guest 传入的 work 0x20 字节（eboot_probe.cpp:6926-6929）；"
                "返回非 0/malloc 指针会被 guest 当失败并留下空 work 指针",
    0x1BD67366: "CreateSema: 初始计数取 r2(initCount)（eboot_probe.cpp:6563）；用错参数会让 worker 一创建就阻塞",
    0x8516D040: "CreateEventFlag: 初始位模取 r2(initPattern)（eboot_probe.cpp:6527-6538）",
    0xE6B761D1: "SignalSema: 旧实现按 r1(cnt，0 当 1)累加并唤醒等待者（eboot_probe.cpp:6565-6627）",
    0x0C7B834B: "WaitSema: 旧实现在计数不足时登记等待 + sched_yield，恢复后重放 SVC（eboot_probe.cpp:7351-7391）；"
                "AOT 里返回 0 但不重试等于谎报成功",
    0x83C0E2AF: "WaitEventFlag: 旧实现按 mode bit0=OR/bit2=clear 判定，回填 cur，不足时 yield+重放（eboot_probe.cpp:7301-7348）",
    0xE737B1DF: "WaitEventFlagCB: 与 WaitEventFlag 同实现（eboot_probe.cpp:7580）",
    0xDDB395A9: "WaitThreadEnd: 应等目标线程结束；缺失时立即返回 0 会提前回收线程/上下文",
    0x7A410B64: "sceDisplaySetFrameBuf: 旧实现解析 SceDisplayFrameBuf 并记录翻页（eboot_probe.cpp:1549-1560）",
    0x05F27764: "sceDisplayWaitVblankStartMultiCB: 旧运行时绑 n_thread_yield 做帧节拍（eboot_probe.cpp:7654）；"
                "按 GetFrameBuf 语义实现是错的（stub 0x812B1810 当前就是如此）",
    0x7D9864A8: "sceDisplayWaitSetFrameBufMulti: 旧实现 n_display_ok 恒 0（eboot_probe.cpp:1561）",
    0x2DB7BE3B: "sceAppUtilDrmOpen: 离线无 DLC 必须返回 ENOENT 0x80010002（eboot_probe.cpp:7669-7673）；返回 0 会让 DLC 全部误判存在",
    0x10B5765F: "sceAppMgrReceiveSystemEvent: 无系统事件必须返回 EAGAIN 0x80010001 打破轮询（eboot_probe.cpp:431-434,7679）",
    0x5E86319A: "_sceAppMgrGetAppState: 旧实现把事件缓冲清零（r1 字节数）后返 0（eboot_probe.cpp:435-441）",
    0xB8EF5818: "sceKernelGetMemBlockBase: 旧实现只把 base VA 写进 *basep（4 字节，eboot_probe.cpp:6024-6028）；"
                "写成 32 字节会覆盖 guest 栈/结构",
    0xB9D5EBDE: "sceKernelAllocMemBlock: 旧实现对 CDRAM(type 高字节 0x09/0x0D) 做 1MB 对齐（eboot_probe.cpp:5994-6022）",
    0x169A1D58: "sceTouchRead: 旧实现清零本次 SceTouchData 并返回 1（eboot_probe.cpp:452-458）；返回 0 会触发 guest 断言",
    0x02DB3F5F: "sceAudioOutOutput: 旧实现首次让出一个时间片、重放即通过，用作音频节拍（eboot_probe.cpp:7466-7470）",
    0x1B58FA3B: "strcmp: 默认 0 = 所有字符串“相等”，路径/资源比较会误判",
    0x0D5200CB: "strstr: 默认 NULL 会让扩展名/前缀匹配恒失败（旧运行时专门修复过，见 eboot_probe.cpp:6425）",
    0x7747F6D7: "memcmp: 默认 0 = 所有内存块“相等”",
    0x3B0AE9A9: "sceNpCheckCallback: 旧实现同 n_noop 返 0（eboot_probe.cpp:8249）",
}

# ---------------------------------------------------------------------------
# 基础工具
# ---------------------------------------------------------------------------


def eprint(*a, **kw):
    # 注意: 输出到 stdout（PowerShell 把 native 进程的 stderr 当 ErrorRecord，
    # 会让 `py ... | Select-Object` 这类管道误报 NativeCommandError）
    print(*a, **kw)


def detect_encoding(path):
    """识别文本编码：UTF-16 BOM / UTF-8 BOM / 无 BOM 的 UTF-16 / UTF-8。

    harness 日志经常是 PowerShell `>` 重定向产物（UTF-16LE + BOM），
    直接当 UTF-8 读会得到满屏 NUL，所以必须先探测。
    """
    with open(path, "rb") as f:
        head = f.read(4096)
    if head.startswith(b"\xef\xbb\xbf"):
        return "utf-8-sig"
    if head.startswith(b"\xff\xfe"):
        return "utf-16"        # LE
    if head.startswith(b"\xfe\xff"):
        return "utf-16-be"
    # 无 BOM：统计 NUL 的奇偶位置分布
    if head:
        nul = head.count(0)
        if nul:
            even_nul = sum(1 for i in range(0, len(head), 2) if head[i] == 0)
            odd_nul = sum(1 for i in range(1, len(head), 2) if head[i] == 0)
            if even_nul > odd_nul * 4 and len(head) >= 4:
                return "utf-16"
            if odd_nul > even_nul * 4 and len(head) >= 4:
                return "utf-16-be"
    try:
        head.decode("utf-8")
        return "utf-8-sig"
    except UnicodeDecodeError:
        return "latin-1"


def iter_lines(path, encoding):
    """流式逐行读取（避免 100MB+ 日志一次性进内存）。"""
    with open(path, "r", encoding=encoding, errors="replace", newline=None) as f:
        for line in f:
            yield line.rstrip("\r\n")


# ---------------------------------------------------------------------------
# 桩 -> NID / 名字映射
# ---------------------------------------------------------------------------

RE_NID_ENTRY = re.compile(
    r'\{\s*0x([0-9A-Fa-f]+)u,\s*0x([0-9A-Fa-f]+)u,\s*"([^"]+)",\s*"([^"]+)"\s*\}'
)


def load_nid_names(path):
    """解析 nid_import_names.h: nid -> (lib, name)。"""
    table = {}
    if not path or not os.path.isfile(path):
        return table
    try:
        src = open(path, "r", encoding="utf-8", errors="replace").read()
    except OSError:
        return table
    for m in RE_NID_ENTRY.finditer(src):
        nid = int(m.group(2), 16)
        table[nid] = (m.group(3), m.group(4))
    return table


def load_stub_map(path):
    """解析 eboot_imports_with_nid.json: stub_va -> {module, func_nid}。"""
    out = {}
    if not path or not os.path.isfile(path):
        return out
    try:
        data = json.load(open(path, "r", encoding="utf-8", errors="replace"))
    except (OSError, ValueError):
        return out
    for e in data:
        try:
            va = int(e.get("stub_vaddr", 0))
        except (TypeError, ValueError):
            continue
        if va:
            out[va] = {
                "module": e.get("module", "?"),
                "lib_nid": int(e.get("library_nid", 0) or 0),
                "func_nid": int(e.get("func_nid", 0) or 0),
            }
    return out


# ---------------------------------------------------------------------------
# harness 源码解析：case 地址 + 常量/实现分类
# ---------------------------------------------------------------------------

RE_CASE = re.compile(r"case\s+0x(812B[0-9A-Fa-f]{4})u\s*:")
RE_COMMENT = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)
RE_CONST_ASSIGN = re.compile(
    r"r0\s*=\s*(0x[0-9A-Fa-f]+|\d+)[uUlL]*\s*;\s*break\s*;", re.S
)
RE_SIDE_EFFECT = re.compile(
    r"\b(?:printf|fprintf|fflush|GP\(|std::memcpy|std::memset|std::snprintf|"
    r"memcpy|memset|snprintf|hle_malloc|heap_alloc|gxmvk_|fibre_|fiber_|"
    r"hle_thr_pump|fios_|cpk_read|g_[A-Za-z_]\w*\s*(?:\[|\+|\-|\||&|=|\+\+|--)|"
    r"switch\s*\(|for\s*\(|while\s*\(|if\s*\(|return\s+s->|return\s+[A-Za-z_])"
)


def strip_comments(text):
    text = RE_COMMENT.sub("", text)
    return text


def classify_block(code):
    """返回 (kind, const_value)。kind ∈ {const, impl, other}。"""
    c = strip_comments(code).strip()
    # 去掉包裹整个 case 块的花括号
    while c.startswith("{") and c.endswith("}"):
        c = c[1:-1].strip()
    if not c:
        return ("other", None)
    m = RE_CONST_ASSIGN.fullmatch(c)
    if m:
        try:
            return ("const", int(m.group(1), 0))
        except ValueError:
            return ("other", None)
    # 形如 "{ uint32_t x = ...; ... }" 一律视为有实现
    if RE_SIDE_EFFECT.search(c):
        return ("impl", None)
    return ("other", None)


def analyze_harness(path):
    """解析 harness hle_call() 的 switch，返回 {stub_va: {kind,value,comment}}。

    解析失败/文件不存在时返回 None（工具仍可工作，只是分类降级为“未知”）。
    """
    if not path or not os.path.isfile(path):
        return None
    try:
        src = open(path, "r", encoding="utf-8", errors="replace").read()
    except OSError:
        return None
    m = re.search(r"switch\s*\(\s*stub_va\s*\)", src)
    if not m:
        return None
    start = m.end()
    dm = re.search(r"\n\s*default\s*:", src[start:])
    body = src[start: start + dm.start()] if dm else src[start:]
    labels = []
    for mm in RE_CASE.finditer(body):
        line_start = body.rfind("\n", 0, mm.start()) + 1
        labels.append((mm.start(), mm.end(), int(mm.group(1), 16), line_start))
    cases = {}
    for idx, (ls, le, addr, line_start) in enumerate(labels):
        nxt = labels[idx + 1][0] if idx + 1 < len(labels) else len(body)
        chunk = body[le:nxt]
        # case 行上的注释（紧跟在同一行后的 // xxx）作为摘要
        same_line = body[le: body.find("\n", le) if body.find("\n", le) >= 0 else len(body)]
        comment = ""
        cm = re.search(r"//\s*(.+)$", same_line)
        if cm:
            comment = cm.group(1).strip()
        kind, value = classify_block(chunk)
        cases[addr] = {"kind": kind, "value": value, "comment": comment,
                       "line": src.count("\n", 0, start + ls) + 1}
    return cases


# ---------------------------------------------------------------------------
# 日志解析
# ---------------------------------------------------------------------------

RE_HLE = re.compile(
    r"\[HLE\]\s+#(\d+)\s+va=([0-9A-Fa-f]{8})\s+lr=([0-9A-Fa-f]{8})"
)
RE_F2HLE = re.compile(r"\[F2HLE\]\s+#(\d+)\s+va=([0-9A-Fa-f]{8})\s+lr=([0-9A-Fa-f]{8})")
RE_HLCOUNT = re.compile(r"^\s*([0-9A-Fa-f]{8})\s+x(\d+)\s*$")
RE_HLCOUNT_HEAD = re.compile(r"unique=(\d+)\s+total=(\d+)")
RE_RING = re.compile(r"\b([0-9A-Fa-f]{8})@([0-9A-Fa-f]{8})\b")
RE_TOPBLK = re.compile(r"^\s*blk\s+([0-9A-Fa-f]{8})\s+x(\d+)\s*$")


def parse_log(path):
    """解析单个日志，返回统计字典。"""
    enc = detect_encoding(path)
    stats = {
        "file": path,
        "encoding": enc,
        "lines": 0,
        "hle_lines": 0,
        "f2hle_lines": 0,
        "hle_capped": False,
        "hl_total": None,
        "hl_unique": None,
        "counts_hle": Counter(),      # [HLE] 行（前 6000 次调用）
        "counts_hl": Counter(),       # [HLECOUNT] 段（全量 top40）
        "counts_ring": Counter(),     # [WD] hle_ring 出现次数（只说明“最近还在调”）
        "lr_hle": defaultdict(Counter),
        "lr_ring": defaultdict(Counter),
        "topblk": Counter(),
        "wd_lines": 0,
        "finished": False,
        "done_blocks": None,
        "last_frame": "",
    }
    in_hl = False
    max_hle_idx = -1
    for line in iter_lines(path, enc):
        stats["lines"] += 1
        s = line.strip()
        if not s:
            continue
        if s.startswith("done: blocks="):
            stats["finished"] = True
            m = re.search(r"blocks=(\d+)", s)
            if m:
                stats["done_blocks"] = int(m.group(1))
            continue
        if s.startswith("[FRAMEDUMP]"):
            stats["last_frame"] = s[:220]
            continue
        if s.startswith("[HLE]"):
            m = RE_HLE.match(s)
            if m:
                idx = int(m.group(1))
                va = int(m.group(2), 16)
                lr = int(m.group(3), 16)
                stats["counts_hle"][va] += 1
                stats["lr_hle"][va][lr] += 1
                stats["hle_lines"] += 1
                max_hle_idx = max(max_hle_idx, idx)
            continue
        if s.startswith("[F2HLE]"):
            m = RE_F2HLE.match(s)
            if m:
                stats["counts_hle"][int(m.group(2), 16)] += 1
                stats["lr_hle"][int(m.group(2), 16)][int(m.group(3), 16)] += 1
                stats["f2hle_lines"] += 1
            continue
        if s.startswith("[HLECOUNT]"):
            in_hl = True
            m = RE_HLCOUNT_HEAD.search(s)
            if m:
                stats["hl_unique"] = int(m.group(1))
                stats["hl_total"] = int(m.group(2))
            continue
        if in_hl:
            m = RE_HLCOUNT.match(line)
            if m:
                stats["counts_hl"][int(m.group(1), 16)] = int(m.group(2))
                continue
            in_hl = False
        if s.startswith("[TOPBLK]"):
            stats["_in_topblk"] = True
            continue
        if stats.get("_in_topblk"):
            m = RE_TOPBLK.match(line)
            if m:
                stats["topblk"][int(m.group(1), 16)] = int(m.group(2))
                continue
            stats["_in_topblk"] = False
        if s.startswith("[WD]"):
            stats["wd_lines"] += 1
            if "hle_ring:" in s:
                tail = s.split("hle_ring:", 1)[1]
                for va_s, lr_s in RE_RING.findall(tail):
                    if va_s.upper().startswith("812B"):
                        va = int(va_s, 16)
                        lr = int(lr_s, 16)
                        stats["counts_ring"][va] += 1
                        stats["lr_ring"][va][lr] += 1
            continue
    stats.pop("_in_topblk", None)
    if max_hle_idx >= 5999:
        stats["hle_capped"] = True
    return stats


# ---------------------------------------------------------------------------
# 汇总 / 输出
# ---------------------------------------------------------------------------


def merge_counts(stats_list):
    counts = Counter()
    lr_hle = defaultdict(Counter)
    lr_ring = defaultdict(Counter)
    hle_capped = False
    hl_total = 0
    hl_present = False
    for st in stats_list:
        # [HLE] 行覆盖前 6000 次调用（含低频桩），[HLECOUNT] 是运行结束时的全量 top40；
        # 两者取并集 + max，才不会漏掉“在 top40 之外但确实调用过”的桩。
        for va, n in st["counts_hle"].items():
            counts[va] = max(counts[va], n)
        for va, n in st["counts_hl"].items():
            counts[va] = max(counts[va], n)      # 多份日志取各桩最大（通常只有一份）
            hl_present = True
        for va, c in st["lr_hle"].items():
            lr_hle[va].update(c)
        for va, c in st["lr_ring"].items():
            lr_ring[va].update(c)
        hle_capped = hle_capped or st["hle_capped"]
        if st["hl_total"]:
            hl_total += st["hl_total"]
    return counts, lr_hle, lr_ring, hle_capped, hl_present, hl_total


def fmt_addr(va):
    return "0x%08X" % va if va else "0x????????"


def build_rows(counts, stub_map, nid_names, cases):
    rows = []
    for va, n in counts.items():
        meta = stub_map.get(va)
        nid = meta["func_nid"] if meta else 0
        module = meta["module"] if meta else "?"
        nm = nid_names.get(nid, ("?", "?"))
        if nm[0] != "?":
            module = nm[0]
        name = nm[1] if nm[1] != "?" else ("unknown_0x%08X" % nid if nid else "?")
        if cases is not None:
            if va in cases:
                kind = cases[va]["kind"]
                value = cases[va]["value"]
                comment = cases[va]["comment"]
            else:
                kind, value, comment = "missing", None, ""
        else:
            kind, value, comment = "unknown", None, ""
        rows.append({
            "stub": va, "count": n, "nid": nid, "module": module,
            "name": name, "kind": kind, "value": value,
            "comment": comment, "known_risk": KNOWN_RISK.get(nid, ""),
        })
    rows.sort(key=lambda r: (-r["count"], r["stub"]))
    return rows


def kind_label(row):
    if row["kind"] == "missing":
        return "未实现(默认0)"
    if row["kind"] == "const":
        return "常量 0x%X" % (row["value"] or 0)
    if row["kind"] == "impl":
        return "有实现"
    if row["kind"] == "unknown":
        return "未知(无harness)"
    return "其它"


def print_report(args, stats_list, rows, counts, lr_hle, lr_ring, cases,
                 stub_map, nid_names, hl_total, hl_present, hle_capped):
    total_calls = sum(counts.values())
    print("=" * 100)
    print("P5D HLE 桩调用探针 — hle_probe.py")
    print("=" * 100)
    for st in stats_list:
        print("日志: %s" % st["file"])
        print("      编码=%-10s 行数=%-10d [HLE]行=%-6d [WD]行=%-6d"
              % (st["encoding"], st["lines"], st["hle_lines"] + st["f2hle_lines"], st["wd_lines"]))
    print("-" * 100)
    print("解析到 %d 个唯一桩，累计 %d 次调用%s"
          % (len(counts), total_calls if not hl_total else hl_total,
             "（[HLECOUNT] 全量口径）" if hl_present else "（仅前 6000 条 [HLE] 行，可能偏少）"))
    if hl_present and hl_total and hl_total != total_calls:
        print("  注意: [HLECOUNT] total=%d 与已解析计数 %d 不同（只记录了 top40）" % (hl_total, total_calls))
    if hle_capped:
        print("  注意: [HLE] 行已到打印上限（#5999），低频桩的次数以 [HLECOUNT] 段为准")
    for st in stats_list:
        if st["done_blocks"] is not None or st["finished"]:
            print("  运行结束: blocks=%s%s" % (st["done_blocks"], "（finished）" if st["finished"] else ""))
        else:
            print("  运行结束: 未检测到 \"done: blocks=\" 行 —— 日志可能被截断/超时 killed")
        if st["last_frame"]:
            print("  最后 FRAMEDUMP: %s" % st["last_frame"])
    if cases is None:
        print("  注意: 未找到/未解析 harness 源码，无法判定“未实现/常量返回”，分类显示为未知")
    else:
        const_n = sum(1 for c in cases.values() if c["kind"] == "const")
        impl_n = sum(1 for c in cases.values() if c["kind"] == "impl")
        missing_n = sum(1 for r in rows if r["kind"] == "missing")
        print("  harness switch 解析到 %d 个 case（常量返回 %d / 有实现 %d）；本日志命中的未实现桩 %d 个"
              % (len(cases), const_n, impl_n, missing_n))
    print("=" * 100)

    top = rows[: max(1, args.top)]
    print()
    print("[1] TOP %d 桩（按调用次数）  —— kind: 有实现 / 常量返回 / 未实现(缺 case, 默认返回 0)" % len(top))
    print("%-4s %-10s %-11s %-10s %-24s %-40s %-14s %s" %
          ("#", "stub", "count", "NID", "module", "function", "harness", "备注/风险"))
    for i, r in enumerate(top, 1):
        risk = r["known_risk"]
        note = "★ " + risk[:60] if risk else (r["comment"][:48] if r["comment"] else "")
        print("%-4d %-10s %-11d %-10s %-24s %-40s %-14s %s" %
              (i, fmt_addr(r["stub"]), r["count"], "%08X" % r["nid"] if r["nid"] else "-",
               r["module"][:23], r["name"][:39], kind_label(r), note))
        if args.show_lr:
            merged_lr = Counter(lr_hle.get(r["stub"], {}))
            merged_lr.update(lr_ring.get(r["stub"], {}))
            if merged_lr:
                s = ", ".join("%08X×%d" % (lr, c) for lr, c in merged_lr.most_common(3))
                print("     调用点 lr: %s" % s)

    # [2] 可疑桩：高频且（未实现 / 常量返回）
    suspicious = [r for r in rows
                  if r["kind"] in ("missing", "const") and r["count"] >= args.min_count]
    suspicious.sort(key=lambda r: -r["count"])
    print()
    print("[2] 可疑桩（次数 >= %d 且 harness 未实现或只返回常量）共 %d 个"
          % (args.min_count, len(suspicious)))
    if suspicious:
        print("%-10s %-11s %-10s %-24s %-38s %-13s %s" %
              ("stub", "count", "NID", "module", "function", "harness", "旧运行时语义提示"))
        for r in suspicious[: max(args.top * 2, 40)]:
            print("%-10s %-11d %-10s %-24s %-38s %-13s %s" %
                  (fmt_addr(r["stub"]), r["count"],
                   "%08X" % r["nid"] if r["nid"] else "-",
                   r["module"][:23], r["name"][:37], kind_label(r),
                   (r["known_risk"] or "(见 HLE_DIFF_REPORT.md)")[:70]))
    else:
        print("  （无）")

    # [3] 按模块汇总
    print()
    print("[3] 按模块汇总")
    mod = Counter()
    for r in rows:
        mod[r["module"]] += r["count"]
    for name, n in mod.most_common(20):
        print("  %-30s %12d" % (name, n))

    # [4] 未实现桩清单（本日志命中）
    missing_rows = [r for r in rows if r["kind"] == "missing"]
    print()
    print("[4] 本日志命中、但 harness 没有 case 的桩（默认 r0=0）：%d 个" % len(missing_rows))
    for r in missing_rows[:60]:
        print("  %-10s %10d  %s  %s/%s" %
              (fmt_addr(r["stub"]), r["count"], "%08X" % r["nid"], r["module"], r["name"]))
    if len(missing_rows) > 60:
        print("  ...(共 %d 个，省略)" % len(missing_rows))

    # [5] 常量返回桩清单
    const_rows = [r for r in rows if r["kind"] == "const"]
    print()
    print("[5] 本日志命中、但 harness 只返回常量的桩：%d 个" % len(const_rows))
    for r in const_rows[:60]:
        print("  %-10s %10d  %-13s %s  %s/%s" %
              (fmt_addr(r["stub"]), r["count"], kind_label(r),
               "%08X" % r["nid"], r["module"], r["name"]))
    if len(const_rows) > 60:
        print("  ...(共 %d 个，省略)" % len(const_rows))

    # [6] 已知高风险 NID 中本日志未命中的桩（说明该路径本次没走到或被别处掩盖）
    nid2stub = {}
    for va, meta in stub_map.items():
        if meta["func_nid"] and meta["func_nid"] not in nid2stub:
            nid2stub[meta["func_nid"]] = va
    unhit = []
    for nid, reason in KNOWN_RISK.items():
        va = nid2stub.get(nid)
        if va and va not in counts:
            meta = stub_map.get(va, {})
            nm = nid_names.get(nid, ("?", "?"))
            unhit.append((va, nid, nm[0], nm[1], reason))
    print()
    print("[6] 已知高风险 NID 但本日志未命中的桩：%d 个" % len(unhit))
    for va, nid, module, name, reason in sorted(unhit):
        print("  %-10s %08X  %s/%s" % (fmt_addr(va), nid, module, name))
        print("      期望: %s" % reason)

    print()
    print("=" * 100)
    print("提示: [HLE] 行只覆盖前 6000 次调用；完整次数看 [HLECOUNT] 段。")
    print("      “未实现(默认0)”= recomp_harness.cpp hle_call() 的 switch 里没有该桩的 case。")
    print("      已知高风险 NID 的旧运行时期望语义见脚本内 KNOWN_RISK 与 docs/HLE_DIFF_REPORT.md。")
    print("=" * 100)


def dump_json(path, stats_list, rows, counts, hl_total):
    data = {
        "logs": [{"file": st["file"], "encoding": st["encoding"], "lines": st["lines"],
                  "hle_lines": st["hle_lines"], "hle_capped": st["hle_capped"],
                  "hl_total": st["hl_total"], "hl_unique": st["hl_unique"]}
                 for st in stats_list],
        "unique_stubs": len(counts),
        "parsed_calls": sum(counts.values()),
        "hl_total": hl_total,
        "stubs": [
            {"stub": "0x%08X" % r["stub"], "count": r["count"],
             "nid": "0x%08X" % r["nid"] if r["nid"] else None,
             "module": r["module"], "function": r["name"],
             "harness": r["kind"],
             "const_value": r["value"] if r["kind"] == "const" else None,
             "known_risk": r["known_risk"] or None}
            for r in rows
        ],
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
    eprint("[hle_probe] JSON 已写入 %s" % path)


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------


def main(argv=None):
    if hasattr(sys.stdout, "reconfigure"):
        try:
            sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        except (AttributeError, ValueError):
            pass
    ap = argparse.ArgumentParser(
        description="P5D 静态 AOT harness HLE 桩调用统计/可疑常量桩探针（只读）",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__.split("约束", 1)[0],
    )
    ap.add_argument("logs", nargs="+", help="harness 运行日志（out*.txt / err*.txt，可多个）")
    ap.add_argument("--top", type=int, default=30, help="输出前 N 个桩（默认 30）")
    ap.add_argument("--min-count", type=int, default=100,
                    help="可疑桩的最小调用次数阈值（默认 100）")
    ap.add_argument("--harness", default=None,
                    help="recomp_harness.cpp 路径（默认自动探测 C:\\dev\\tmp\\recomp_split）")
    ap.add_argument("--map", default=DEFAULT_MAP,
                    help="stub->NID 映射 JSON（默认 docs/eboot_imports_with_nid.json）")
    ap.add_argument("--nid-names", default=DEFAULT_NID_NAMES,
                    help="NID->名字头文件（默认 src/loader/nid_import_names.h）")
    ap.add_argument("--json", default=None, help="把结果另存为 JSON")
    ap.add_argument("--show-lr", action="store_true", default=True,
                    help="在 TOP 表下显示调用点 lr（默认开）")
    ap.add_argument("--no-lr", dest="show_lr", action="store_false", help="不显示调用点 lr")
    ap.add_argument("--quiet", action="store_true", help="只输出结论摘要")
    args = ap.parse_args(argv)

    harness = args.harness
    if harness is None:
        for c in DEFAULT_HARNESS_CANDIDATES:
            if c and os.path.isfile(c):
                harness = c
                break
    cases = analyze_harness(harness) if harness else None
    stub_map = load_stub_map(args.map)
    nid_names = load_nid_names(args.nid_names)
    if not stub_map:
        eprint("[hle_probe] 警告: 未能读取桩->NID 映射: %s" % args.map)
    if not nid_names:
        eprint("[hle_probe] 警告: 未能读取 NID 名字表: %s" % args.nid_names)

    stats_list = []
    for path in args.logs:
        if not os.path.isfile(path):
            eprint("[hle_probe] 跳过不存在的日志: %s" % path)
            continue
        st = parse_log(path)
        if st["hle_lines"] == 0 and not st["counts_hl"] and not st["counts_ring"]:
            eprint("[hle_probe] 警告: %s 中没有找到 [HLE]/[HLECOUNT]/[WD] 证据（是否不是 harness 日志？）" % path)
        stats_list.append(st)
    if not stats_list:
        eprint("[hle_probe] 错误: 没有可解析的日志")
        return 2

    counts, lr_hle, lr_ring, hle_capped, hl_present, hl_total = merge_counts(stats_list)
    rows = build_rows(counts, stub_map, nid_names, cases)

    if args.quiet:
        sus = [r for r in rows if r["kind"] in ("missing", "const") and r["count"] >= args.min_count]
        print("unique=%d calls=%d suspicious=%d" % (len(counts), sum(counts.values()), len(sus)))
        for r in rows[:args.top]:
            print("%s %d %s/%s %s" % (fmt_addr(r["stub"]), r["count"],
                                      r["module"], r["name"], kind_label(r)))
    else:
        print_report(args, stats_list, rows, counts, lr_hle, lr_ring, cases,
                     stub_map, nid_names, hl_total, hl_present, hle_capped)

    if args.json:
        dump_json(args.json, stats_list, rows, counts, hl_total)
    return 0


if __name__ == "__main__":
    sys.exit(main())

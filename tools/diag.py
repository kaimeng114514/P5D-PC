# SPDX-License-Identifier: MIT
#!/usr/bin/env python3
"""diag.py - 一键分析一次 harness 运行日志（结构化、符号化、可自动化）

用法:
    py -3 tools/diag.py <out.txt> <err.txt> [--exit-code 3] [--json] [--no-disasm]
常用: 运行 harness 后:
    py -3 tools/diag.py out.txt err.txt --exit-code $LASTEXITCODE

功能:
  * 汇总: HACKS 状态 / init 序列次数 / assert / BADMEM / CRASH / GXM / frame / 日志行数
  * 符号化: 地址 -> 函数名+原始源文件 (func_index.tsv / function_bounds.json)
  * 阻塞点: 提取 [BADMEM]/[CRASH]/[H7344]-BAD/[PUMPSW] 等关键行并符号化
  * pcreg: 取最后 [WD] pcring 并逐项符号化
  * 反汇编: 默认对首个 BADMEM block 前后 0x30 字节做 thumb 反汇编
  * --json: 机器可读输出（供 run_checkpoints.py / CI 使用）

设计目标: 每轮调试不再手写一次性 Python 脚本；分析步骤从 ~10 分钟降到 ~5 秒。
"""
import argparse
import io
import json
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FUNC_INDEX = os.path.join(ROOT, "..", "p5d_build", "func_index.tsv")
FUNC_BOUNDS = os.path.join(ROOT, "..", "p5d_build", "..", "tmp", "function_bounds.json")
FUNC_TABLE = os.environ.get("P5D_FUNC_INDEX", r"C:\dev\p5d_build\func_index.tsv")
FUNC_BOUNDS_JSON = os.environ.get("P5D_FUNCTION_BOUNDS", r"C:\dev\tmp\function_bounds.json")
ELF_PATH = os.environ.get("P5D_ELF", r"C:\dev\p5d_build\p5d_true.elf")


def load_symbols():
    """返回 [(va, size, name, src_file)], 按 va 排序。"""
    rows = []
    try:
        with io.open(FUNC_TABLE, encoding="utf-8", errors="replace") as f:
            for line in f:
                p = line.rstrip("\n").split("\t")
                if len(p) < 3:
                    continue
                try:
                    va = int(p[0], 16)
                    sz = int(p[1])
                except ValueError:
                    continue
                name = p[2]
                src = p[5] if len(p) > 5 else ""
                rows.append((va, sz, name, src))
    except OSError:
        pass
    if not rows:
        try:
            data = json.load(io.open(FUNC_BOUNDS_JSON, encoding="utf-8"))
            for e in data:
                rows.append((e.get("va", 0), e.get("size", 0), e.get("name", "?"), ""))
        except (OSError, ValueError):
            pass
    rows.sort()
    return rows


def symbolize(rows, va):
    va &= ~1
    lo, hi = 0, len(rows) - 1
    best = None
    while lo <= hi:
        mid = (lo + hi) // 2
        v, sz, name, src = rows[mid]
        if v <= va:
            best = rows[mid]
            lo = mid + 1
        else:
            hi = mid - 1
    if best and best[0] <= va < best[0] + max(best[1], 1):
        v, sz, name, src = best
        base = os.path.basename(src.replace("\\", "/")) if src else ""
        return "%s+0x%X%s" % (name, va - v, (" [" + base + "]") if base else "")
    # 不在任何函数里: 找最近的更低起点
    if best:
        return "?%08X (nearest %s)" % (va, best[2])
    return "?%08X" % va


def find_func(rows, va):
    va &= ~1
    best = None
    for r in rows:
        if r[0] <= va < r[0] + max(r[1], 1):
            best = r
    return best


def disasm_block(va, nbytes=0x40):
    try:
        from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB, CS_MODE_LITTLE_ENDIAN
    except ImportError:
        return ["(capstone 不可用)"]
    try:
        d = open(ELF_PATH, "rb").read()
    except OSError:
        return ["(ELF 不可用: %s)" % ELF_PATH]
    e_phoff = struct.unpack_from("<I", d, 0x1C)[0]
    e_phnum = struct.unpack_from("<H", d, 0x2C)[0]
    segs = []
    for i in range(e_phnum):
        off = e_phoff + i * 0x20
        t, o, v, pa, fsz, msz, fl, al = struct.unpack_from("<8I", d, off)
        if t == 1 and fsz:
            segs.append((v, v + fsz, o))

    def vo(x):
        for lo, hi, o in segs:
            if lo <= x < hi:
                return o + (x - lo)
        return None

    base = va & ~1
    off = vo(base)
    if off is None:
        return ["(地址 %08X 不在 ELF 段内)" % base]
    md = Cs(CS_ARCH_ARM, CS_MODE_THUMB | CS_MODE_LITTLE_ENDIAN)
    out = []
    for ins in md.disasm(d[off:off + nbytes], base):
        out.append("%08X  %-12s %s" % (ins.address, ins.mnemonic, ins.op_str))
    return out


def analyze(out_path, err_path, exit_code=None):
    txt = io.open(out_path, "rb").read().replace(b"\x00", b"").decode("utf-8", errors="replace")
    err = io.open(err_path, "rb").read().replace(b"\x00", b"").decode("utf-8", errors="replace")
    lines = txt.splitlines()
    elines = err.splitlines()
    rows = load_symbols()

    rep = {"stdout_lines": len(lines), "stderr_lines": len(elines), "exit_code": exit_code}
    rep["hacks"] = ""
    for l in lines:
        if "[HACKS]" in l:
            rep["hacks"] = "ENABLED" if "ENABLED" in l else "DISABLED"
            break
    rep["init"] = {}
    for l in lines:
        m = re.search(r"\[INITW\] ([\w\-]+) #(\d+)", l)
        if m:
            rep["init"][m.group(1)] = max(rep["init"].get(m.group(1), 0), int(m.group(2)))
    rep["asserts"] = [l.strip() for l in lines if "[ASSERT]" in l and "[ASSERT-MSG]" not in l]
    rep["gxm"] = sum(1 for l in lines if "[GXM]" in l)
    rep["flprobe"] = sum(1 for l in lines if "[FLPROBE]" in l)
    rep["framedump"] = sum(1 for l in lines if "[FRAMEDUMP]" in l)
    reels = [l for l in lines if "[FRAMEDUMP]" in l]
    ctrs = set()
    for l in reels:
        m = re.search(r"ctr=(\d+)", l)
        if m:
            ctrs.add(int(m.group(1)))
    rep["framectr"] = sorted(ctrs)[-1] if ctrs else None
    rep["pumpsw"] = [l.strip() for l in lines if "[PUMPSW]" in l][:5]
    rep["h7344bad"] = [l.strip() for l in lines if "[H7344]-BAD" in l][:5]

    badmem = [l for l in elines if "BADMEM" in l]
    rep["badmem_count"] = len(badmem)
    rep["badmem"] = []
    for l in badmem[:4]:
        m = re.search(r"guest va=0x([0-9A-Fa-f]+)\s+unmapped block=0x([0-9A-Fa-f]+).*?ring_va", l)
        if m:
            va, blk = int(m.group(1), 16), int(m.group(2), 16)
            rep["badmem"].append({"va": va, "block": blk,
                                  "sym": symbolize(rows, blk),
                                  "func": (find_func(rows, blk) or [None])[2]})
    crash = [l for l in lines + elines if "[CRASH]" in l]
    rep["crash"] = crash[:2]
    print("=== [WD] last pc ring (symbolized) ===")
    ring = []
    for l in reversed(lines):
        if "[WD] pcring" in l:
            addrs = re.findall(r"\b([0-9A-F]{8})\b", l.split("pcring:")[-1])
            ring = [int(a, 16) for a in addrs]
            break
    rep["pcring"] = [{"va": a, "sym": symbolize(rows, a)} for a in ring]
    rep["last_line"] = lines[-1].strip()[:200] if lines else ""
    return rep, rows


def print_report(rep, rows, show_disasm=True):
    print("=== RUN SUMMARY ===")
    print("  exit=%s  stdout_lines=%d  stderr_lines=%d  hacks=%s" % (
        rep["exit_code"], rep["stdout_lines"], rep["stderr_lines"], rep["hacks"] or "?"))
    if rep["init"]:
        bad = [k for k, v in rep["init"].items() if v != 1]
        print("  init: %s   %s" % (", ".join("%s=%d" % (k, v) for k, v in sorted(rep["init"].items())),
                                   "OK" if not bad else "!! DUPLICATED: " + ",".join(bad)))
    print("  asserts=%d  gxm=%d  flprobe=%d  framedump=%d  framectr=%s" % (
        len(rep["asserts"]), rep["gxm"], rep["flprobe"], rep["framedump"], rep["framectr"]))
    print("  badmem=%d  crash=%d" % (rep["badmem_count"], len(rep["crash"])))
    print("")
    if rep["asserts"]:
        print("--- ASSERTS ---")
        for l in rep["asserts"][:8]:
            print("  " + l[:220])
    if rep["pumpsw"]:
        print("--- PUMPSW ---")
        for l in rep["pumpsw"]:
            print("  " + l[:220])
    if rep["h7344bad"]:
        print("--- H7344-BAD ---")
        for l in rep["h7344bad"]:
            print("  " + l[:300])
    if rep["badmem"]:
        print("--- BADMEM ---")
        for b in rep["badmem"]:
            print("  va=0x%X block=0x%08X  %s" % (b["va"], b["block"], b["sym"]))
    if rep["crash"]:
        print("--- CRASH ---")
        for l in rep["crash"]:
            print("  " + l[:300])
    if rep["pcring"]:
        print("--- last pcring ---")
        for e in rep["pcring"][-12:]:
            print("  %08X  %s" % (e["va"], e["sym"]))
    print("--- last line ---")
    print("  " + rep["last_line"])
    if show_disasm and rep["badmem"]:
        print("")
        print("--- disasm around first BADMEM block ---")
        for l in disasm_block(rep["badmem"][0]["block"]):
            print("  " + l)


def main(argv):
    ap = argparse.ArgumentParser(description="P5D harness run diagnostics")
    ap.add_argument("out_file")
    ap.add_argument("err_file")
    ap.add_argument("--exit-code", type=int, default=None)
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--no-disasm", action="store_true")
    args = ap.parse_args(argv)
    rep, rows = analyze(args.out_file, args.err_file, args.exit_code)
    if args.json:
        print(json.dumps(rep, ensure_ascii=False, indent=2))
    else:
        print_report(rep, rows, show_disasm=not args.no_disasm)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

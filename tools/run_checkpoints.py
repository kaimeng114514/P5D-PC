# SPDX-License-Identifier: MIT
#!/usr/bin/env python3
"""run_checkpoints.py - 确定性检查点回归（效率+稳定性基础设施）

用途:
  以固定块数跑 harness，提取结构化结果并与基线 JSON 对比。任何偏差 => FAIL。
  因为 M669 已把 pump 调度改为确定性（默认不用墙钟），同一二进制的检查点结果应当完全可复现。

用法:
  py -3 tools/run_checkpoints.py --exe C:\\dev\\tmp\\recomp_split\\recomp_harness.exe \\
      --runs 2000000,10000000,30000000
  py -3 tools/run_checkpoints.py ... --update        # 更新基线（确认结果正确后）
  py -3 tools/run_checkpoints.py ... --only 2000000  # 只跑某个检查点

对比字段: exit_code, stdout_lines, init(每个 init 次数), asserts, gxm, framectr,
          badmem(va/block 列表), pcring_hash(最后 pc ring 哈希), last_line.

退出码: 0 = PASS, 1 = 有检查点 FAIL, 2 = 运行错误。
"""
import argparse
import hashlib
import io
import json
import os
import subprocess
import sys
import time

TOOLS = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, TOOLS)
import diag  # noqa: E402

TMP = r"C:\dev\tmp\_chk"


def pcring_hash(rep):
    s = "|".join("%08X" % e["va"] for e in rep.get("pcring", []))
    return hashlib.sha256(s.encode()).hexdigest()[:16]


def snapshot(exe, entry, blocks):
    os.makedirs(TMP, exist_ok=True)
    outf = os.path.join(TMP, "out_%d.txt" % blocks)
    errf = os.path.join(TMP, "err_%d.txt" % blocks)
    env = dict(os.environ)
    env.setdefault("TMP", r"C:\dev\tmp\gtmp")
    env.setdefault("TEMP", r"C:\dev\tmp\gtmp")
    env.pop("P5D_HACKS", None)
    env.pop("P5D_PUMP_WALLCLOCK", None)
    t0 = time.time()
    with open(outf, "wb") as fo, open(errf, "wb") as fe:
        proc = subprocess.run([exe, entry, str(blocks)], stdout=fo, stderr=fe, env=env)
    dt = time.time() - t0
    rep, rows = diag.analyze(outf, errf, proc.returncode)
    snap = {
        "blocks": blocks,
        "exit_code": proc.returncode,
        "stdout_lines": rep["stdout_lines"],
        "init": rep["init"],
        "asserts": len(rep["asserts"]),
        "gxm": rep["gxm"],
        "framectr": rep["framectr"],
        "badmem": [{"va": b["va"], "block": b["block"]} for b in rep["badmem"]],
        "pcring_hash": pcring_hash(rep),
        "last_line": rep["last_line"],
        "elapsed_s": round(dt, 1),
    }
    return snap, rep


def compare(base, cur, line_tolerance=8):
    diffs = []
    for k in ("exit_code", "init", "asserts", "gxm", "framectr",
              "badmem", "pcring_hash", "last_line"):
        if base.get(k) != cur.get(k):
            diffs.append("%s: baseline=%r current=%r" % (k, base.get(k), cur.get(k)))
    bl, cl = base.get("stdout_lines", 0), cur.get("stdout_lines", 0)
    if abs(bl - cl) > line_tolerance:
        diffs.append("stdout_lines: baseline=%r current=%r (tolerance=%d)" % (bl, cl, line_tolerance))
    return diffs


def main(argv):
    ap = argparse.ArgumentParser(description="P5D deterministic checkpoint regression")
    ap.add_argument("--exe", default=r"C:\dev\tmp\recomp_split\recomp_harness.exe")
    ap.add_argument("--entry", default="0x81198464")
    ap.add_argument("--runs", default="2000000,10000000,30000000")
    ap.add_argument("--baseline", default=os.path.join(TMP, "baseline.json"))
    ap.add_argument("--update", action="store_true")
    ap.add_argument("--only", default=None, help="只跑指定块数")
    ap.add_argument("--line-tolerance", type=int, default=8, help="stdout 行数容差（进程 abort 时缓冲尾部可能丢几行）")
    args = ap.parse_args(argv)

    if not os.path.isfile(args.exe):
        print("run_checkpoints: exe not found: %s" % args.exe)
        return 2
    runs = [int(x) for x in args.runs.split(",") if x.strip()]
    if args.only:
        runs = [int(args.only)]

    baseline = {}
    if os.path.isfile(args.baseline):
        try:
            baseline = json.load(io.open(args.baseline, encoding="utf-8"))
        except ValueError:
            baseline = {}
    failures = 0
    newbase = dict(baseline)
    print("run_checkpoints: %d checkpoint(s), baseline=%s" % (len(runs), args.baseline))
    for blocks in runs:
        snap, rep = snapshot(args.exe, args.entry, blocks)
        key = str(blocks)
        print("  [%s] exit=%s lines=%d init=%s asserts=%d gxm=%d badmem=%d pcring=%s (%.1fs)" % (
            key, snap["exit_code"], snap["stdout_lines"], snap["init"], snap["asserts"],
            snap["gxm"], len(snap["badmem"]), snap["pcring_hash"], snap["elapsed_s"]))
        if args.update or key not in baseline:
            newbase[key] = snap
        else:
            diffs = compare(baseline[key], snap, args.line_tolerance)
            if diffs:
                failures += 1
                print("    FAIL")
                for d in diffs:
                    print("      - " + d)
            else:
                print("    PASS")
    if args.update or not baseline:
        os.makedirs(TMP, exist_ok=True)
        io.open(args.baseline, "w", encoding="utf-8", newline="\n").write(
            json.dumps(newbase, ensure_ascii=False, indent=2))
        print("run_checkpoints: baseline written -> %s" % args.baseline)
    if failures:
        print("run_checkpoints: FAIL (%d checkpoint mismatch)" % failures)
        return 1
    print("run_checkpoints: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

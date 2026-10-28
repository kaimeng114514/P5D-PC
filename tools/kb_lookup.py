# SPDX-License-Identifier: MIT
﻿# kb_lookup.py - MANDATORY first step of every work round: search master doc for existing
# conclusions relevant to current problem. Usage: py -3 tools\kb_lookup.py <kw|addr> ...
import io, os, sys

DOC = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "docs", "P5D_MASTER.md"))

def main():
    if len(sys.argv) < 2:
        print("usage: kb_lookup.py <keyword|address> [--limit=N]")
        return 1
    limit = 40
    terms = []
    for a in sys.argv[1:]:
        if a.startswith("--limit="):
            limit = int(a.split("=")[1])
        else:
            terms.append(a.lower())
    lines = io.open(DOC, encoding="utf-8", errors="replace").read().split("\n")
    hits = []
    for i, ln in enumerate(lines):
        low = ln.lower()
        if any(t in low for t in terms):
            hits.append((i + 1, ln))
    print("KB_LOOKUP terms=%s hits=%d" % (",".join(terms), len(hits)))
    heads = [(n, l) for (n, l) in hits if l.startswith("#")]
    miles = [(n, l) for (n, l) in hits if l.startswith("| M") or "| §18." in l]
    rest = [(n, l) for (n, l) in hits if (n, l) not in heads and (n, l) not in miles]
    shown = 0
    for grp, label in ((heads, "HEADINGS"), (miles, "MILESTONES"), (rest, "BODY")):
        if not grp:
            continue
        print("\n== %s ==" % label)
        for n, l in grp:
            if shown >= limit:
                print("... truncated (%d hits)" % len(hits))
                return 0
            print("%6d: %s" % (n, l[:360]))
            shown += 1
    print("\nKB_LOOKUP_DONE - read any relevant hit before implementing.")
    return 0

if __name__ == "__main__":
    sys.exit(main())

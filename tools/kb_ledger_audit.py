# kb_ledger_audit.py - anti-forgetting ledger consistency audit.
#
# WHY: docs/P5D_MASTER.md starts with a CANONICAL STATE ledger whose milestone
# table points at section ids. If a milestone row points at a missing section
# (or the newest section is not in the ledger / has no kb anchor), knowledge is
# effectively forgotten even though the text may exist somewhere.
#
# CHECKS:
#   1. CANONICAL STATE ledger exists and has milestone rows `| M<n> | ... | §... |`
#   2. every section id referenced by a milestone row exists as a `### <id>` heading
#   3. the newest `### 18.x` section is referenced by the newest milestone row
#   4. the newest three sections have anchors in tools/kb_audit.py
#
# OUTPUT: LEDGER_AUDIT_PASS / LEDGER_AUDIT_FAIL
# Exit: 0 pass, 2 fail.
import io, os, re, sys

DOC = r"C:\dev\p5dsrc\docs\P5D_MASTER.md"
KB = r"C:\dev\p5dsrc\tools\kb_audit.py"


def main():
    if not os.path.exists(DOC):
        print("LEDGER_AUDIT_FAIL (missing master doc)")
        return 2
    doc = io.open(DOC, encoding="utf-8", errors="replace").read()
    fail = []

    # 1) ledger + milestone rows
    if "### CANONICAL STATE" not in doc:
        fail.append("CANONICAL STATE ledger heading missing")
    rows = re.findall(r"^\| (M\d+) \|([^\n]*)$", doc, re.M)
    if len(rows) < 5:
        fail.append("milestone rows too few: %d" % len(rows))

    # 2) every section id in a milestone row must exist as a heading
    headings = re.findall(r"^### (18\.\d+)", doc, re.M)
    heading_set = set(headings)
    for mid, line in rows:
        ids = re.findall(r"(18\.\d+)", line)
        if not ids:
            fail.append("%s row has no section id" % mid)
        for sid in ids:
            if sid not in heading_set:
                fail.append("%s references missing section %s" % (mid, sid))

    # 3) newest section must be referenced by the newest milestone row
    if headings and rows:
        last_doc_sec = headings[-1]
        last_m_ids = re.findall(r"(18\.\d+)", rows[-1][1])
        if last_doc_sec not in last_m_ids:
            fail.append("newest section %s is not referenced by newest milestone %s"
                        % (last_doc_sec, rows[-1][0]))

    # 4) newest three sections must have kb anchors
    kbtxt = ""
    if os.path.exists(KB):
        kbtxt = io.open(KB, encoding="utf-8", errors="replace").read()
    for sid in headings[-3:]:
        if sid not in kbtxt:
            fail.append("kb_audit.py has no anchor for %s" % sid)

    if fail:
        for f in fail:
            print("  FAIL " + f)
        print("LEDGER_AUDIT_FAIL")
        return 2
    print("  milestone rows=%d, headings=%d, newest=%s" % (len(rows), len(headings), headings[-1]))
    print("LEDGER_AUDIT_PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())

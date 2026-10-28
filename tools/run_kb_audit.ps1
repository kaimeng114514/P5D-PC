# run_kb_audit.ps1 - verification line 15: anti-forgetting coverage audit
$ErrorActionPreference = "Stop"
$py = "C:\dev\p5dsrc\tools\kb_audit.py"
$out = & py -3 $py --quiet 2>&1 | Out-String
Write-Output $out.TrimEnd()
if ($out -match "KB_AUDIT_PASS") {
    $ledger = & py -3 "C:\dev\p5dsrc\tools\kb_ledger_audit.py" 2>&1 | Out-String
    Write-Output $ledger.TrimEnd()
    if ($ledger -match "LEDGER_AUDIT_PASS") {
        Write-Output "KB_AUDIT_LINE_PASS"
        exit 0
    }
    Write-Output "KB_AUDIT_LINE_FAIL"
    exit 2
}
Write-Output "KB_AUDIT_LINE_FAIL"
exit 2

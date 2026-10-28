# verify_doc_integrity.ps1 - anti-corruption checks for the authoritative master document
$ErrorActionPreference = 'Stop'
$master = 'C:\dev\p5dsrc\docs\P5D_MASTER.md'
$bak    = 'C:\dev\p5dsrc\docs\_MASTER_BAK'
$fail = 0

if (!(Test-Path $master)) { Write-Output 'DOC_INTEGRITY_FAIL missing master'; exit 1 }
$bytes = (Get-Item $master).Length
Write-Output ("  size_bytes = {0}" -f $bytes)
if ($bytes -lt 28000000) { Write-Output '  => size check FAIL'; $fail = 1 } else { Write-Output '  => size check PASS' }

$sections = (Select-String -Path $master -Pattern '^### ').Count
Write-Output ("  sections = {0}" -f $sections)
if ($sections -lt 12400) { Write-Output '  => section check FAIL'; $fail = 1 } else { Write-Output '  => section check PASS' }

$text = [System.IO.File]::ReadAllText($master, [System.Text.Encoding]::UTF8)
$fffd = ([regex]::Matches($text, [char]0xFFFD)).Count
Write-Output ("  U+FFFD = {0}" -f $fffd)
if ($fffd -ne 0) { Write-Output '  => encoding check FAIL'; $fail = 1 } else { Write-Output '  => encoding check PASS' }

$anchors = @('18.3393','18.3395','18.3400','18.3406','18.3408','18.3415','18.3418')
foreach ($a in $anchors) {
    if ($text.Contains("### $a")) { Write-Output ("  anchor $a OK") }
    else { Write-Output ("  anchor $a MISSING"); $fail = 1 }
}

if (Test-Path $bak) {
    $n = (Get-ChildItem $bak -File -ErrorAction SilentlyContinue).Count
    Write-Output ("  backup files = {0}" -f $n)
    if ($n -lt 1) { Write-Output '  => backup check FAIL'; $fail = 1 } else { Write-Output '  => backup check PASS' }
} else { Write-Output '  => backup dir MISSING'; $fail = 1 }

Write-Output ''
if ($fail -eq 0) { Write-Output 'DOC_INTEGRITY_PASS' } else { Write-Output 'DOC_INTEGRITY_FAIL' }
exit $fail

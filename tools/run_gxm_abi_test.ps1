# run_gxm_abi_test.ps1 - standalone integration test for the generated SceGxm ABI layer.
#
# Compiles tests\gxm_abi_test.cpp together with the generated
# C:\dev\tmp\recomp_split\gxm_abi.cpp using the MinGW-w64 g++ that the rest of the
# project uses, then runs the test binary and reports PASS/FAIL.
#
# The test is a pure dispatch/ABI test: the 26 runtime backend functions the wrappers
# can call are replaced by recording test doubles defined in gxm_abi_test.cpp, and the
# three weak translation hooks (p5d_abi_translate64 / p5d_abi_translate /
# p5d_abi_untranslate) are overridden with strong test definitions.  No Vulkan device
# is created and no runtime .o file is linked, so the test runs headless and needs no
# game data.
#
# Usage:  powershell -ExecutionPolicy Bypass -File C:\dev\p5dsrc\tools\run_gxm_abi_test.ps1
#         [ -KeepBinary ]     keep the exe (it is kept by default; -KeepBinary is accepted
#                             for symmetry with other scripts and is a no-op)
#         [ -Clean ]          delete the test exe before building
#
# Exit codes: 0 = test PASS, 1 = test FAIL, 2 = build/setup FAIL.
# Pure ASCII.

param(
    [switch]$KeepBinary,
    [switch]$Clean
)

$ErrorActionPreference = 'Continue'

$GXX      = 'C:\dev\mingw64\bin\g++.exe'
$ROOT     = 'C:\dev\p5dsrc'
$GEN_DIR  = 'C:\dev\tmp\recomp_split'
$TEST_SRC = Join-Path $ROOT 'tests\gxm_abi_test.cpp'
$GEN_SRC  = Join-Path $GEN_DIR 'gxm_abi.cpp'
$GEN_HDR  = Join-Path $GEN_DIR 'gxm_abi.h'
$EXE      = Join-Path $GEN_DIR 'gxm_abi_test.exe'
$TMPDIR   = 'C:\dev\tmp\gtmp'

Write-Host '=== GXM ABI layer integration test build ==='

# ---- preflight -------------------------------------------------------------
if (-not (Test-Path $GXX)) {
    Write-Host "GXM_ABI_TEST_COMPILE_FAIL: compiler not found: $GXX"
    exit 2
}
if (-not (Test-Path $TEST_SRC)) {
    Write-Host "GXM_ABI_TEST_COMPILE_FAIL: test source not found: $TEST_SRC"
    exit 2
}
if (-not (Test-Path $GEN_SRC)) {
    Write-Host "GXM_ABI_TEST_COMPILE_FAIL: generated ABI layer not found: $GEN_SRC"
    Write-Host "  run: py -3 $ROOT\tools\gxm_abi_gen.py"
    exit 2
}
if (-not (Test-Path $GEN_HDR)) {
    Write-Host "GXM_ABI_TEST_COMPILE_FAIL: generated ABI header not found: $GEN_HDR"
    exit 2
}

# ---- build environment -----------------------------------------------------
New-Item -ItemType Directory -Force $TMPDIR | Out-Null
New-Item -ItemType Directory -Force $GEN_DIR | Out-Null
$env:TMP = $TMPDIR
$env:TEMP = $TMPDIR

if ($Clean -and (Test-Path $EXE)) {
    Remove-Item -Force $EXE
} else {
    # never let a stale binary masquerade as a fresh test result
    Remove-Item -Force $EXE -ErrorAction SilentlyContinue
}

# ---- compile ---------------------------------------------------------------
$incs = @(
    "-I$ROOT\src",
    "-I$ROOT\src\common",
    "-I$ROOT\src\gxm",
    "-I$ROOT\third_party\vulkan\include",
    "-I$ROOT\refs\vita-headers\include",
    "-I$GEN_DIR"
)
$flags = @(
    '-std=c++17',
    '-O2',
    '-DP5D_HAS_VULKAN',
    '-Wall',
    '-Wextra'
)
$srcs = @($TEST_SRC, $GEN_SRC)

$buildLog = & $GXX @flags @incs @srcs '-o' $EXE '-static' 2>&1
$buildLog | ForEach-Object { Write-Host $_ }
$compileExit = $LASTEXITCODE

if ($compileExit -ne 0 -or -not (Test-Path $EXE)) {
    Write-Host "GXM_ABI_TEST_COMPILE_FAIL (g++ exit $compileExit)"
    exit 2
}

# ---- run -------------------------------------------------------------------
Write-Host ''
$runLog = & $EXE 2>&1
$runExit = $LASTEXITCODE
$runLog | ForEach-Object { Write-Host $_ }

$sawPass = [bool]($runLog | Select-String -Pattern 'GXM_ABI_TEST_PASS' -SimpleMatch |
                  Select-Object -Last 1)
$sawFail = [bool]($runLog | Select-String -Pattern 'GXM_ABI_TEST_FAIL' -SimpleMatch |
                  Select-Object -Last 1)

Write-Host ''
if ($runExit -eq 0 -and $sawPass -and -not $sawFail) {
    Write-Host 'GXM_ABI_TEST_RUN_PASS'
    exit 0
}
Write-Host "GXM_ABI_TEST_RUN_FAIL (test exit $runExit)"
exit 1

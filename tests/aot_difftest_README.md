# aot_difftest - instruction-semantics differential test for the AOT recompiler

`tools/aot_difftest.py` verifies that the C++ code produced by the AOT recompiler implements
ARMv7-A/Thumb-2 instruction semantics correctly.  It is meant to catch the regression classes
that have cost us debugging time before: missing post-index writeback, LDRD/STRD pairing,
shift-by-32 / shift-amount masking, ASR sign fill, ROR, three-register shifted operands with
`setf_nz`, ADC/SBC carry chains, PUSH/POP/LDM/STM ordering, and BLX indirect calls (old LR vs
new LR).

## Run

```
py -3 C:\dev\p5dsrc\tools\aot_difftest.py                     # everything
py -3 C:\dev\p5dsrc\tools\aot_difftest.py --mode core         # golden cases only (fast, no batch scan)
py -3 C:\dev\p5dsrc\tools\aot_difftest.py --mode blocks       # production block diff only
py -3 C:\dev\p5dsrc\tools\aot_difftest.py --strict            # known baseline failures are fatal too
py -3 C:\dev\p5dsrc\tools\aot_difftest.py --only post --verbose
```

Exit code: `0` = pass (including documented known-baseline deviations), `1` = unexpected
failure, `2` = framework error (compile error, reference-model self-check failure, missing ELF).

Runtime is ~5 s; all temporary host sources/executables go to `C:\dev\tmp\gtmp\aot_difftest_<pid>`
and are deleted afterwards (`--keep` preserves them).  Nothing outside `tests/` and `tools/` is
modified.

## Files

| file | purpose |
| --- | --- |
| `aot_difftest_cases.json` | golden cases: instruction bytes + initial registers/memory/APSR, with hand-derived `expect` values on the tricky ones |
| `aot_difftest_host.cpp` | host template; the driver substitutes the production preamble, the extracted production block functions and the generated case tables |
| `aot_difftest_baseline.json` | known deviations of the current generator (each with the concrete ARM requirement).  They are executed and printed as `KNOWN`; `--strict` makes them fatal.  Entries that start passing are reported as `STALE`. |

## How the three oracles work

1. **Reference model** (`tools/aot_difftest.py`, `RefState`/`execute`): an independent Python
   implementation of the ARMv7-A Thumb subset used by the cases.  It is validated on every run
   against the hand-derived constants in `cases.json`, and (when unicorn is installed) against
   unicorn for every golden case (`refcheck` section).
2. **Golden cases** (`--mode core`): each case instruction is translated with the *production
   generator* (`_gen_batch2.py`'s `emit()`, the same function `_gen_split.py` imports), compiled
   into the host and executed.  Registers, memory probes and APSR are compared with the model.
3. **Production blocks** (`--mode blocks`): short straight-line blocks and control-flow
   terminator blocks are extracted verbatim from `C:\dev\tmp\recomp_split\batch_*.cpp`, their
   instruction stream is recovered from `C:\dev\p5d_build\p5d_true.elf`, and the real generated
   block body is executed and diffed against the model.  When unicorn is available these same
   blocks are also emulated with unicorn and diffed against the generated C++ directly
   (`unicorn` section).  The `--opt` flag defaults to `-O2`, matching the production batch build.

## Interpreting the output

* `PASS` - generated code matched ARM semantics on the asserted fields.
* `WARN` - only APSR C/V bits deviated outside the bits the case asserts (the generator currently
  sets N/Z from shifts/logical ops but not C = shifter carry and not V preservation; reported for
  information, not fatal).
* `KNOWN` - a mismatch listed in `aot_difftest_baseline.json` (documented generator defect).
* `FAIL` - unexpected mismatch: a real regression (or a new bug) - exit code 1.

## Known generator deviations currently baselined (found by this test)

1. **Immediate shift by 32** (`emit()`): the immediate shift amount is masked with `& 0x1F`, so
   `LSR/ASR #32` (encoded as shift field 0) becomes "shift by 0" instead of 0 / sign fill.
2. **Register shift amount masking** (`emit()`): the full 32-bit Rs is compared against 32 instead
   of using ARM `Rs[7:0]`, so e.g. a shift register value of 0x108 is treated as ">= 32" instead
   of "shift by 8" (`LSL/LSR/ASR` register forms, 3-operand and 2-operand).
3. **ADC/SBC** (`emit()`): the 3-register forms start from `Rd` instead of `Rn`
   (`adcs.w r0,r1,r3` is wrong whenever `Rd != Rn`), the carry chain is computed in 32-bit
   arithmetic (`b+c` / `b+(1-C)` can wrap and lose the carry/borrow), and the non-S `adc.w` /
   `sbc.w` forms wrongly update flags.  Found live in production code, e.g. `blk_8105AA16`
   (`adc.w r0, r1, r3` in `batch_004.cpp`).
4. **IT blocks**: `it` is emitted as a comment and the condition suffix is stripped from the
   conditional instruction, so it executes unconditionally.  Example in production:
   `blk_8100AD80` contains `cmp r3,r5; it eq; cmpeq r2,r4; bne`; the generated block always
   executes the second compare and then branches on the wrong Z (`p5d_true.elf` has 205 `it`
   instructions, 126 of them `cmpeq`, e.g. at 0x8100AD92).

Additionally, flag-setting shifts/logical ops do not compute C from the shifter carry and clear
V instead of preserving it (`WARN` sections).  The reference model is validated against unicorn
(golden cases: `refcheck`) and the real production blocks are directly diffed against unicorn
(`unicorn` section) whenever python-unicorn is installed.


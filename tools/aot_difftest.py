#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""aot_difftest.py - instruction-semantics differential test for the P5D static AOT recompiler.

Two independent sources of "correct ARM behaviour" are used to check the recompiled C++:

  Mode B (golden cases, always available)
    * tests/aot_difftest_cases.json holds one-instruction golden cases: real instruction bytes
      taken from the game ELF (or hand-encoded edge encodings verified with capstone) plus an
      initial register/memory/APSR state.  Each instruction is translated by the *production
      generator* (_gen_batch2.py emit(), the very function _gen_split.py imports) into C++,
      compiled into a small host and executed.  The result is compared against an independent
      Python ARMv7 reference model in this file.  Cases are chosen to pin the bug classes that
      have bitten us before: post/pre-index writeback, LDRD/STRD pairing, shift-by-32 and
      shift amount masking, ASR sign fill, ROR, three-register shifted operands with flags,
      ADC/SBC carry chains, PUSH/POP/LDM/STM ordering.
    * real production blocks are additionally extracted verbatim from
      C:\\dev\\tmp\\recomp_split\\batch_*.cpp (single- and short multi-instruction blocks whose
      instruction stream is recovered from the ELF) and executed the same way.  Control-flow
      block terminators (BL, BLX reg, BLX LR, BX LR, POP {..,PC}, conditional B) are exercised
      through the real generated block bodies, which is where the "BLX LR captures old LR"
      fix lives.

  Mode A (Unicorn, optional)
    If python unicorn is available, a subset of the real production blocks is also emulated
    instruction-by-instruction by unicorn from the identical initial state, and unicorn's final
    register/flag/memory state is compared with the generated C++ block's result.  This is a
    second, fully independent reference for the same production code.

Usage:
  py -3 C:\\dev\\p5dsrc\\tools\\aot_difftest.py                 # all checks
  py -3 ...\\aot_difftest.py --mode core                      # golden cases only (no batch scan)
  py -3 ...\\aot_difftest.py --mode blocks --no-unicorn       # production blocks only
  py -3 ...\\aot_difftest.py --strict                         # known baseline failures are fatal too
  py -3 ...\\aot_difftest.py --only post --verbose            # filter case ids

Exit codes: 0 = pass (known-baseline failures reported but not fatal), 1 = unexpected failure,
2 = framework/compile/reference-model error.

Writes only temporary files under C:\\dev\\tmp\\gtmp\\aot_difftest_*; never modifies the
recompiler, the batch sources or the docs.  Pure ASCII.
"""

import argparse
import glob
import json
import os
import random
import re
import shutil
import struct
import subprocess
import sys
import time

# ------------------------------------------------------------------------------------------------
# paths
# ------------------------------------------------------------------------------------------------
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TESTS_DIR = os.path.join(ROOT, "tests")
HOST_TEMPLATE = os.path.join(TESTS_DIR, "aot_difftest_host.cpp")
CASES_JSON = os.path.join(TESTS_DIR, "aot_difftest_cases.json")
BASELINE_JSON = os.path.join(TESTS_DIR, "aot_difftest_baseline.json")

ELF_PATH = r"C:\dev\p5d_build\p5d_true.elf"
GENERATOR = r"C:\dev\tmp\_gen_batch2.py"
BATCH_DIR = r"C:\dev\tmp\recomp_split"
GXX = r"C:\dev\mingw64\bin\g++.exe"
GTMP = r"C:\dev\tmp\gtmp"

# APSR bits (the value carried in RecState.apsr)
F_N = 0x80000000
F_Z = 0x40000000
F_C = 0x20000000
F_V = 0x10000000
F_NZCV = 0xF0000000
F_NZ = 0xC0000000

GUEST_LO = 0x80600000
GUEST_HI = 0x84600000
MEM_SIZE = 0x4000000  # 64 MiB: covers GUEST_LO..GUEST_HI

# Stubs used by the generated host for indirect transfers.  The expected return values must be
# kept in sync with tests/aot_difftest_host.cpp.
STUB_IDREG = 0xFFFFFFE0
STUB_IDREGV = 0xFFFFFFE1
STUB_IDTBB = 0xFFFFFFE2
STUB_IDTBH = 0xFFFFFFE3
STUB_IDMEM = 0xFFFFFFE4
STUB_HLE = 0xFFFFFFF0


def eprint(*a):
    print(*a, file=sys.stderr)


# ------------------------------------------------------------------------------------------------
# capstone + generated instruction decoders
# ------------------------------------------------------------------------------------------------
try:
    from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB, CS_MODE_LITTLE_ENDIAN
    from capstone.arm import ARM_OP_REG, ARM_OP_IMM, ARM_OP_MEM
except Exception as exc:  # pragma: no cover
    eprint("FATAL: capstone is required (py -3 -m pip show capstone): %r" % (exc,))
    sys.exit(2)

# capstone operand.type values used by the generator / this model
OP_INVALID, OP_REG, OP_IMM, OP_MEM = 0, 1, 2, 3

_MD = Cs(CS_ARCH_ARM, CS_MODE_THUMB | CS_MODE_LITTLE_ENDIAN)
_MD.detail = True


def disasm_one(code, va):
    insns = list(_MD.disasm(code, va))
    if not insns:
        return None
    return insns[0]


def base_mnemonic(m):
    return m[:-2] if m.endswith(".w") else m


# capstone register id -> guest register index (ARM regs only)
def cap_reg_idx(reg):
    if 66 <= reg <= 78:
        return reg - 66          # R0..R12
    if reg == 12:
        return 13               # SP
    if reg == 10:
        return 14               # LR
    if reg == 11:
        return 15               # PC
    raise Unsupported("non-GPR register id %d" % reg)


def is_gpr(op):
    return op.type == OP_REG and (66 <= op.reg <= 78 or op.reg in (12, 10, 11))


SHIFT_LSL, SHIFT_LSR, SHIFT_ASR, SHIFT_ROR = 2, 3, 1, 4  # capstone arm shift type codes
SHIFT_NAME = {SHIFT_LSL: "LSL", SHIFT_LSR: "LSR", SHIFT_ASR: "ASR", SHIFT_ROR: "ROR"}
SHIFT_INSN = {"lsl", "lsls", "lsr", "lsrs", "asr", "asrs", "ror", "rors"}
# mnemonics whose "s" suffix means "set flags" (the generated code uses this distinction heavily)
FLAGSET_MNEM = {"adds", "subs", "ands", "orrs", "eors", "bics", "movs", "mvns", "rsbs",
                "adcs", "sbcs", "lsls", "lsrs", "asrs", "rors", "muls"}
LOGIC_MNEM = {"and", "orr", "eor", "bic"}
ARITH_MNEM = {"add", "sub", "rsb", "adc", "sbc"}
MEM_MNEM = {"ldr", "ldrb", "ldrh", "ldrsb", "ldrsh", "str", "strb", "strh", "ldrd", "strd"}
MULTI_MNEM = {"ldm", "ldmia", "ldmdb", "stm", "stmia", "stmdb"}
CONTROL_BASE = {"b", "bx", "blx", "bl", "pop", "cbz", "cbnz"}

COND_PRED = {
    "cond_eq": lambda st: st.z == 1,
    "cond_ne": lambda st: st.z == 0,
    "cond_cs": lambda st: st.c == 1,
    "cond_hs": lambda st: st.c == 1,
    "cond_cc": lambda st: st.c == 0,
    "cond_lo": lambda st: st.c == 0,
    "cond_mi": lambda st: st.n == 1,
    "cond_pl": lambda st: st.n == 0,
    "cond_vs": lambda st: st.v == 1,
    "cond_vc": lambda st: st.v == 0,
    "cond_hi": lambda st: st.c == 1 and st.z == 0,
    "cond_ls": lambda st: st.c == 0 or st.z == 1,
    "cond_ge": lambda st: st.n == st.v,
    "cond_lt": lambda st: st.n != st.v,
    "cond_gt": lambda st: st.z == 0 and st.n == st.v,
    "cond_le": lambda st: st.z == 1 or st.n != st.v,
}

VFP_PREFIX = ("v", "q", "d")  # mnemonics starting with these are VFP/NEON; not modelled

# IT-block condition suffixes (capstone appends them to the conditional instruction's mnemonic)
COND_ALIAS = {
    "eq": "cond_eq", "ne": "cond_ne", "cs": "cond_cs", "hs": "cond_hs",
    "cc": "cond_cc", "lo": "cond_lo", "mi": "cond_mi", "pl": "cond_pl",
    "vs": "cond_vs", "vc": "cond_vc", "hi": "cond_hi", "ls": "cond_ls",
    "ge": "cond_ge", "lt": "cond_lt", "gt": "cond_gt", "le": "cond_le",
}


def eval_cond(st, cond):
    name = COND_ALIAS.get(cond, cond)
    fn = COND_PRED.get(name)
    if fn is None:
        raise Unsupported("condition %r" % cond)
    return fn(st)


class Unsupported(Exception):
    """Instruction (or operand shape) outside the reference model's supported subset."""


# ------------------------------------------------------------------------------------------------
# independent Python reference model of the ARMv7-A Thumb subset used by the cases
# ------------------------------------------------------------------------------------------------
def _mask32(x):
    return x & 0xFFFFFFFF


class RefState(object):
    __slots__ = ("r", "apsr", "mem", "writes", "it_cond")

    def __init__(self, regs=None, apsr=0):
        self.r = [0] * 16
        if regs:
            for k, v in regs.items():
                self.r[int(k)] = _mask32(int(v))
        self.apsr = _mask32(apsr) & 0xF0000000
        self.mem = {}      # guest address -> byte
        self.writes = []   # (addr, size, value) in execution order
        self.it_cond = None  # pending IT-block condition (single-instruction IT only)

    # ---- flag helpers ----------------------------------------------------------------
    @property
    def n(self):
        return 1 if self.apsr & F_N else 0

    @property
    def z(self):
        return 1 if self.apsr & F_Z else 0

    @property
    def c(self):
        return 1 if self.apsr & F_C else 0

    @property
    def v(self):
        return 1 if self.apsr & F_V else 0

    def set_logic_flags(self, result, carry):
        a = self.apsr & ~(F_N | F_Z | F_C)
        if result & 0x80000000:
            a |= F_N
        if result == 0:
            a |= F_Z
        if carry:
            a |= F_C
        self.apsr = a  # V unchanged

    def set_arith_flags(self, result, carry, overflow):
        a = self.apsr & ~F_NZCV
        if result & 0x80000000:
            a |= F_N
        if result == 0:
            a |= F_Z
        if carry:
            a |= F_C
        if overflow:
            a |= F_V
        self.apsr = a

    # ---- memory ----------------------------------------------------------------------
    def _check(self, addr, size):
        if addr < GUEST_LO or addr + size > GUEST_HI:
            raise Unsupported("out-of-range guest access 0x%08X+%d" % (addr, size))

    def read(self, addr, size):
        self._check(addr, size)
        val = 0
        for i in range(size):
            val |= self.mem.get(addr + i, 0) << (8 * i)
        return val

    def write(self, addr, size, value):
        self._check(addr, size)
        self.writes.append((addr, size, value & ((1 << (8 * size)) - 1)))
        for i in range(size):
            self.mem[addr + i] = (value >> (8 * i)) & 0xFF

    def seed(self, seed_list):
        for addr, size, value in seed_list:
            self.write(addr, size, value)
        self.writes = []

    def apply_bytes(self, addr, data):
        self._check(addr, len(data))
        for i, b in enumerate(data):
            self.mem[addr + i] = b


def seed_memory_spec():
    """Deterministic initial memory pattern shared by the host and the reference model."""
    out = []
    for i in range(0x100):
        out.append((0x806E0000 + i * 4, 4, (0xA5A50000 + i * 0x00010001) & 0xFFFFFFFF))
    for i in range(0x40):
        out.append((0x806F0000 - 0x100 + i * 4, 4, (0x5A5A0000 + i * 0x00010001) & 0xFFFFFFFF))
    for i in range(0x40):
        out.append((0x80601000 + i * 4, 4, (0x11223344 + i) & 0xFFFFFFFF))
    return out


SEED_MEM = seed_memory_spec()


def shift_result(kind, value, amount, carry_in):
    """ARM shift with carry out.  'amount' is the raw amount (register: only low 8 bits used)."""
    if kind == "LSL":
        if amount == 0:
            return value & 0xFFFFFFFF, carry_in
        if amount < 32:
            return (value << amount) & 0xFFFFFFFF, (value >> (32 - amount)) & 1
        if amount == 32:
            return 0, value & 1
        return 0, 0
    if kind == "LSR":
        if amount == 0:
            return value & 0xFFFFFFFF, carry_in
        if amount < 32:
            return value >> amount, (value >> (amount - 1)) & 1
        if amount == 32:
            return 0, (value >> 31) & 1
        return 0, 0
    if kind == "ASR":
        if amount == 0:
            return value & 0xFFFFFFFF, carry_in
        sign = 1 if value & 0x80000000 else 0
        if amount < 32:
            return ((value >> amount) | (0xFFFFFFFF << (32 - amount))) & 0xFFFFFFFF, \
                   (value >> (amount - 1)) & 1
        return (0xFFFFFFFF if sign else 0), sign
    if kind == "ROR":
        if amount == 0:
            return value & 0xFFFFFFFF, carry_in
        amt = amount & 31
        if amt == 0:
            return value & 0xFFFFFFFF, (value >> 31) & 1
        res = ((value >> amt) | (value << (32 - amt))) & 0xFFFFFFFF
        return res, (res >> 31) & 1
    raise Unsupported("bad shift kind %r" % (kind,))


def _op_shift(op):
    """Return (kind, amount) for a shifted register operand, or None."""
    sh = getattr(op, "shift", None)
    if sh is None:
        return None
    try:
        st, sv = sh.type, sh.value
    except Exception:
        try:
            st, sv = sh[0], sh[1]
        except Exception:
            return None
    if st in SHIFT_NAME and sv != 0:
        return SHIFT_NAME[st], sv
    return None


def operand2(st, op):
    """Decode ARM operand2 -> (value, shifter_carry_out, shifted_flag)."""
    if op.type == OP_IMM:
        return op.imm & 0xFFFFFFFF, st.c, False
    if op.type != OP_REG:
        raise Unsupported("operand2 type %d" % op.type)
    idx = cap_reg_idx(op.reg)
    val = st.r[idx]
    sh = _op_shift(op)
    if sh is None:
        return val, st.c, False
    kind, amt = sh
    res, carry = shift_result(kind, val, amt & 0xFFFFFFFF, st.c)
    return res, carry, True


def _reg(st, op):
    if op.type != OP_REG:
        raise Unsupported("expected register operand")
    return st.r[cap_reg_idx(op.reg)], cap_reg_idx(op.reg)


def _mem_addr(st, op, ins):
    """Effective address for a load/store memory operand, plus writeback info."""
    m = op.mem
    base = st.r[cap_reg_idx(m.base)]
    addr = base
    if m.index:
        idx = st.r[cap_reg_idx(m.index)]
        sh = _op_shift(op)
        if sh is not None:
            idx, _ = shift_result(sh[0], idx, sh[1], st.c)
        scale = getattr(m, "scale", 1) or 1
        addr = (addr + idx * scale) & 0xFFFFFFFF
    addr = (addr + (m.disp & 0xFFFFFFFF)) & 0xFFFFFFFF
    return addr


def _insn_flagset(ins):
    m = base_mnemonic(ins.mnemonic)
    return m in FLAGSET_MNEM


def execute(st, ins):
    """Execute one ARM/Thumb instruction on the reference state.  Raises Unsupported."""
    raw = ins.mnemonic
    m = base_mnemonic(raw)
    # ---- IT block: only single-instruction IT is modelled (enough for the diff cases) ----
    if m == "it":
        cond = (ins.op_str or "").strip().lower()
        if not cond:
            raise Unsupported("IT without condition")
        st.it_cond = cond
        return
    if st.it_cond is not None:
        cond = st.it_cond
        st.it_cond = None
        if m.endswith(cond) and len(m) > len(cond):
            m = m[:-len(cond)]
        # NOTE: when the caller disassembled the instruction without IT context, capstone
        # returns the bare mnemonic ("cmp" instead of "cmpeq"); the IT condition still applies.
        if not eval_cond(st, cond):
            return  # condition false: instruction skipped, flags unchanged
    flagset = m in FLAGSET_MNEM
    base = m[:-1] if flagset else m
    ops = ins.operands

    # ---- move / extend ---------------------------------------------------------------
    if base == "mov" or base == "movw" or base == "movt":
        rd = cap_reg_idx(ops[0].reg)
        if base == "movw":
            st.r[rd] = ops[1].imm & 0xFFFF
        elif base == "movt":
            st.r[rd] = ((st.r[rd] & 0xFFFF) | ((ops[1].imm & 0xFFFF) << 16)) & 0xFFFFFFFF
        else:
            val, carry, _ = operand2(st, ops[1])
            st.r[rd] = val & 0xFFFFFFFF
            if flagset:
                st.set_logic_flags(st.r[rd], carry)
        return
    if base == "mvn":
        rd = cap_reg_idx(ops[0].reg)
        val, carry, _ = operand2(st, ops[-1])
        st.r[rd] = (~val) & 0xFFFFFFFF
        if flagset:
            st.set_logic_flags(st.r[rd], carry)
        return
    if base in ("uxtb", "uxth", "sxtb", "sxth"):
        rd = cap_reg_idx(ops[0].reg)
        rm = st.r[cap_reg_idx(ops[1].reg)]
        w = 8 if base.endswith("b") else 16
        if base.startswith("s"):
            sign = 1 << (w - 1)
            v = rm & ((1 << w) - 1)
            st.r[rd] = (v - (1 << w)) & 0xFFFFFFFF if v & sign else v
        else:
            st.r[rd] = rm & ((1 << w) - 1)
        return
    if base in ("rev", "rev16", "rev32"):
        rd = cap_reg_idx(ops[0].reg)
        src = st.r[cap_reg_idx(ops[1].reg)] if len(ops) > 1 else st.r[rd]
        if base == "rev16":
            st.r[rd] = (((src >> 8) & 0x00FF00FF) | ((src & 0x00FF00FF) << 8)) & 0xFFFFFFFF
        else:
            st.r[rd] = int.from_bytes(src.to_bytes(4, "little")[::-1], "little")
        return

    # ---- logic -----------------------------------------------------------------------
    if base in LOGIC_MNEM:
        rd = cap_reg_idx(ops[0].reg)
        if len(ops) >= 3:
            rn = st.r[cap_reg_idx(ops[1].reg)]
            op2 = ops[2]
        else:
            rn = st.r[rd]
            op2 = ops[1]
        v2, carry, _ = operand2(st, op2)
        if base == "and":
            res = rn & v2
        elif base == "orr":
            res = rn | v2
        elif base == "eor":
            res = rn ^ v2
        else:  # bic
            res = rn & (~v2 & 0xFFFFFFFF)
        st.r[rd] = res & 0xFFFFFFFF
        if flagset:
            st.set_logic_flags(st.r[rd], carry)
        return

    # ---- add/sub/rsb/adc/sbc ---------------------------------------------------------
    if base in ARITH_MNEM:
        rd = cap_reg_idx(ops[0].reg)
        if len(ops) >= 3:
            rn = st.r[cap_reg_idx(ops[1].reg)]
            op2 = ops[2]
        else:
            rn = st.r[rd]
            op2 = ops[1]
        v2, _shcarry, _ = operand2(st, op2)
        if base in ("add", "adc"):
            cin = st.c if base == "adc" else 0
            big = rn + v2 + cin
            res = big & 0xFFFFFFFF
            carry_out = 1 if big > 0xFFFFFFFF else 0
            overflow = 1 if ((~(rn ^ v2)) & (rn ^ res) & 0x80000000) else 0
        elif base in ("sub", "rsb"):
            if base == "sub":
                a, b = rn, v2
            else:
                a, b = v2, rn
            res = (a - b) & 0xFFFFFFFF
            carry_out = 1 if a >= b else 0
            overflow = 1 if ((a ^ b) & (a ^ res) & 0x80000000) else 0
        else:  # sbc: rn - op2 - !C
            sub = v2 + (1 - st.c)
            res = (rn - sub) & 0xFFFFFFFF
            carry_out = 1 if rn >= sub else 0
            overflow = 1 if ((rn ^ sub) & (rn ^ res) & 0x80000000) else 0
        st.r[rd] = res
        if flagset:
            st.set_arith_flags(res, carry_out, overflow)
        return

    # ---- compare / test --------------------------------------------------------------
    if base in ("cmp", "cmn", "tst", "teq"):
        rn = st.r[cap_reg_idx(ops[0].reg)]
        v2, shcarry, _ = operand2(st, ops[1])
        if base == "cmp":
            res = (rn - v2) & 0xFFFFFFFF
            st.set_arith_flags(res, 1 if rn >= v2 else 0,
                               1 if ((rn ^ v2) & (rn ^ res) & 0x80000000) else 0)
        elif base == "cmn":
            big = rn + v2
            res = big & 0xFFFFFFFF
            st.set_arith_flags(res, 1 if big > 0xFFFFFFFF else 0,
                               1 if ((~(rn ^ v2)) & (rn ^ res) & 0x80000000) else 0)
        elif base == "tst":
            res = rn & v2
            st.set_logic_flags(res, shcarry)
        else:
            res = rn ^ v2
            st.set_logic_flags(res, shcarry)
        return

    # ---- shifts ----------------------------------------------------------------------
    if base in ("lsl", "lsr", "asr", "ror"):
        if len(ops) >= 3:
            rd = cap_reg_idx(ops[0].reg)
            rn = st.r[cap_reg_idx(ops[1].reg)]
            amt_op = ops[2]
        else:
            rd = cap_reg_idx(ops[0].reg)
            rn = st.r[rd]
            amt_op = ops[1]
        if amt_op.type == OP_IMM:
            amount = amt_op.imm & 0xFFFFFFFF
        elif amt_op.type == OP_REG:
            amount = st.r[cap_reg_idx(amt_op.reg)] & 0xFF  # ARM: Rs[7:0]
        else:
            raise Unsupported("shift amount operand type %d" % amt_op.type)
        res, carry = shift_result(base.upper(), rn, amount, st.c)
        st.r[rd] = res
        if flagset:
            st.set_logic_flags(res, carry)
        return

    # ---- multiply --------------------------------------------------------------------
    if base in ("mul", "mla", "mls"):
        rd = cap_reg_idx(ops[0].reg)
        if base == "mul":
            if len(ops) >= 3:
                val = st.r[cap_reg_idx(ops[1].reg)] * st.r[cap_reg_idx(ops[2].reg)]
            else:
                val = st.r[cap_reg_idx(ops[1].reg)] * st.r[rd]
            st.r[rd] = val & 0xFFFFFFFF
            if flagset:
                st.set_logic_flags(st.r[rd], st.c)
        elif base == "mla":
            st.r[rd] = (st.r[cap_reg_idx(ops[1].reg)] * st.r[cap_reg_idx(ops[2].reg)]
                        + st.r[cap_reg_idx(ops[3].reg)]) & 0xFFFFFFFF
        else:
            st.r[rd] = (st.r[cap_reg_idx(ops[3].reg)]
                        - st.r[cap_reg_idx(ops[1].reg)] * st.r[cap_reg_idx(ops[2].reg)]) & 0xFFFFFFFF
        return

    # ---- bitfield --------------------------------------------------------------------
    if base in ("ubfx", "sbfx", "bfi", "bfc"):
        rd = cap_reg_idx(ops[0].reg)
        lsb = ops[2].imm
        width = ops[3].imm if len(ops) > 3 else ops[2].imm
        if base == "bfc":
            lsb = ops[1].imm
            width = ops[2].imm
            mask = ((1 << width) - 1) << lsb
            st.r[rd] = st.r[rd] & ~mask
        elif base == "bfi":
            mask = ((1 << width) - 1) << lsb
            st.r[rd] = (st.r[rd] & ~mask) | ((st.r[cap_reg_idx(ops[1].reg)] << lsb) & mask)
        else:
            mask = (1 << width) - 1
            src = st.r[cap_reg_idx(ops[1].reg)] >> lsb
            if base == "ubfx":
                st.r[rd] = src & mask
            else:
                sign = 1 << (width - 1)
                v = src & mask
                st.r[rd] = (v - (1 << width)) & 0xFFFFFFFF if v & sign else v
        return

    # ---- single loads/stores ---------------------------------------------------------
    if base in ("ldr", "ldrb", "ldrh", "ldrsb", "ldrsh", "str", "strb", "strh"):
        rt_idx = cap_reg_idx(ops[0].reg)
        mem_op = None
        post_imm = None
        for op in ops[1:]:
            if op.type == OP_MEM and mem_op is None:
                mem_op = op
            elif op.type == OP_IMM and mem_op is not None:
                post_imm = op.imm
        if mem_op is None:
            raise Unsupported("load/store without memory operand")
        addr = _mem_addr(st, mem_op, ins)
        is_load = base.startswith("ld")
        if is_load:
            if base == "ldr":
                val = st.read(addr, 4)
            elif base == "ldrb":
                val = st.read(addr, 1)
            elif base == "ldrh":
                val = st.read(addr, 2)
            elif base == "ldrsb":
                v = st.read(addr, 1)
                val = (v - 0x100) & 0xFFFFFFFF if v & 0x80 else v
            else:
                v = st.read(addr, 2)
                val = (v - 0x10000) & 0xFFFFFFFF if v & 0x8000 else v
            if rt_idx == 15:
                raise Unsupported("load into PC (block terminator)")
            st.r[rt_idx] = val
        else:
            if base == "str":
                st.write(addr, 4, st.r[rt_idx])
            elif base == "strb":
                st.write(addr, 1, st.r[rt_idx])
            else:
                st.write(addr, 2, st.r[rt_idx])
        # writeback
        if getattr(ins, "writeback", False):
            if post_imm is not None:
                st.r[cap_reg_idx(mem_op.mem.base)] = (st.r[cap_reg_idx(mem_op.mem.base)] + post_imm) & 0xFFFFFFFF
            else:
                st.r[cap_reg_idx(mem_op.mem.base)] = (st.r[cap_reg_idx(mem_op.mem.base)]
                                                    + (mem_op.mem.disp & 0xFFFFFFFF)) & 0xFFFFFFFF
        return

    # ---- dual loads/stores -----------------------------------------------------------
    if base in ("ldrd", "strd"):
        rt = cap_reg_idx(ops[0].reg)
        rt2 = cap_reg_idx(ops[1].reg)
        mem_op = ops[2]
        post_imm = ops[3].imm if len(ops) > 3 and ops[3].type == OP_IMM else None
        addr = _mem_addr(st, mem_op, ins)
        if base == "ldrd":
            st.r[rt] = st.read(addr, 4)
            st.r[rt2] = st.read(addr + 4, 4)
        else:
            st.write(addr, 4, st.r[rt])
            st.write(addr + 4, 4, st.r[rt2])
        if getattr(ins, "writeback", False):
            b = cap_reg_idx(mem_op.mem.base)
            delta = post_imm if post_imm is not None else (mem_op.mem.disp & 0xFFFFFFFF)
            st.r[b] = (st.r[b] + delta) & 0xFFFFFFFF
        return

    # ---- multiple loads/stores / push/pop -------------------------------------------
    if base in ("push", "pop"):
        regs = sorted(cap_reg_idx(op.reg) for op in ops if op.type == OP_REG)
        n = len(regs)
        sp = st.r[13]
        if base == "push":
            new_sp = (sp - 4 * n) & 0xFFFFFFFF
            for i, rg in enumerate(regs):
                st.write((new_sp + 4 * i) & 0xFFFFFFFF, 4, st.r[rg])
            st.r[13] = new_sp
        else:
            for i, rg in enumerate(regs):
                v = st.read((sp + 4 * i) & 0xFFFFFFFF, 4)
                if rg == 15:
                    raise Unsupported("pop PC (block terminator)")
                st.r[rg] = v
            st.r[13] = (sp + 4 * n) & 0xFFFFFFFF
        return

    if base in MULTI_MNEM:
        base_reg = cap_reg_idx(ops[0].reg)
        regs = sorted(cap_reg_idx(op.reg) for op in ops[1:] if op.type == OP_REG)
        if not regs:
            raise Unsupported("multiple mem without register list")
        n = len(regs)
        db = "db" in m
        start = (st.r[base_reg] - 4 * n) & 0xFFFFFFFF if db else st.r[base_reg]
        if base.startswith("ldm"):
            for i, rg in enumerate(regs):
                v = st.read((start + 4 * i) & 0xFFFFFFFF, 4)
                if rg == 15:
                    raise Unsupported("ldm PC")
                st.r[rg] = v
        else:
            for i, rg in enumerate(regs):
                st.write((start + 4 * i) & 0xFFFFFFFF, 4, st.r[rg])
        if getattr(ins, "writeback", False):
            if db:
                st.r[base_reg] = (st.r[base_reg] - 4 * n) & 0xFFFFFFFF
            else:
                st.r[base_reg] = (st.r[base_reg] + 4 * n) & 0xFFFFFFFF
        return

    if base in ("nop", "it", "itt", "ite", "itet", "ittt", "itee", "hint", "yield", "wfi",
                "wfe", "sev", "dsb", "dmb", "isb", "clrex", "pld", "pldw"):
        return
    if base in ("svc", "bkpt", "udf", "hlt"):
        raise Unsupported("exception instruction %s" % raw)
    raise Unsupported("mnemonic %r" % raw)


def insn_is_terminator(ins):
    """Mirror of the generator's is_term(): block terminators end a generated block."""
    m = base_mnemonic(ins.mnemonic)
    if m in CONTROL_BASE:
        return True
    if m.startswith("b") and m not in ("b", "bl", "bx", "blx", "bic", "bics", "bfi", "bfc"):
        return True
    if m in ("ldr", "ldrb", "ldrh") and any(op.type == OP_REG and op.reg == 11 for op in ins.operands):
        return True
    if m in ("tbb", "tbh", "svc", "bkpt", "udf", "hlt"):
        return True
    return False


# ------------------------------------------------------------------------------------------------
# ELF helpers
# ------------------------------------------------------------------------------------------------
class ElfImage(object):
    def __init__(self, path):
        self.data = open(path, "rb").read()
        e_phoff = struct.unpack_from("<I", self.data, 0x1C)[0]
        e_phnum = struct.unpack_from("<H", self.data, 0x2C)[0]
        self.segments = []  # (va_lo, va_hi, file_off)
        for i in range(e_phnum):
            t, o, v, pa, fsz, msz, fl, al = struct.unpack_from("<8I", self.data, e_phoff + i * 0x20)
            if t == 1 and fsz:
                self.segments.append((v, v + fsz, o))

    def va_to_off(self, va):
        for lo, hi, off in self.segments:
            if lo <= va < hi:
                return off + (va - lo)
        return None

    def read(self, va, size):
        off = self.va_to_off(va)
        if off is None or off + size > len(self.data):
            return None
        return self.data[off:off + size]

    def disasm_range(self, va, end):
        """Disassemble [va,end) and return the instruction list; raise Unsupported if the range
        does not decode exactly or is not contiguous."""
        code = self.read(va, end - va)
        if code is None:
            raise Unsupported("ELF range 0x%08X..0x%08X not mapped" % (va, end))
        insns = list(_MD.disasm(code, va))
        if not insns:
            raise Unsupported("no instruction at 0x%08X" % va)
        cur = va
        for ins in insns:
            if ins.address != cur:
                raise Unsupported("non-contiguous decode at 0x%08X" % cur)
            cur += ins.size
        if cur != end:
            raise Unsupported("range 0x%08X..0x%08X decodes to end 0x%08X" % (va, end, cur))
        return insns


# ------------------------------------------------------------------------------------------------
# production generator hook (the exact emit() used to produce batch_*.cpp)
# ------------------------------------------------------------------------------------------------
class Generator(object):
    def __init__(self, gen_path, batch0_path):
        src = open(gen_path, encoding="utf-8").read()
        marker = "out = []"
        if marker not in src:
            raise RuntimeError("cannot find generator main marker 'out = []' in %s" % gen_path)
        ns = {}
        # _gen_batch2.py parses sys.argv at import time (NFUNCS = int(sys.argv[1]) ...); give the
        # semantics part a harmless argv while exec'ing it, then restore ours.
        saved_argv = sys.argv
        try:
            sys.argv = ["_gen_batch2_semantics"]
            exec(compile(src[:src.index(marker)], "_gen_batch2_semantics", "exec"), ns)
        finally:
            sys.argv = saved_argv
        self.ns = ns
        for fn in ("emit", "is_term", "base_m", "RIDX", "R"):
            if fn not in ns:
                raise RuntimeError("generator semantics module lacks %s()" % fn)
        self.emit = ns["emit"]
        self.sup = []

        # preamble: verbose shared prologue of a production batch (RecState + helpers), copied
        # verbatim so the test host has exactly the same helper semantics as the shipped code.
        b0 = open(batch0_path, encoding="utf-8").read()
        idx = b0.find("static uint32_t blk_")
        if idx < 0:
            raise RuntimeError("cannot find block declarations in %s" % batch0_path)
        self.preamble = b0[:idx].rstrip() + "\n"

    def translate(self, insn):
        """Translate one instruction; raises RuntimeError if the generator marks it unsupported."""
        del self.sup[:]
        lines = self.emit(insn, self.sup)
        if self.sup:
            raise RuntimeError("generator marked instruction unsupported: %r" % (self.sup,))
        for ln in lines:
            if "UNSUPPORTED" in ln:
                raise RuntimeError("generator emitted UNSUPPORTED line: %s" % ln)
        return lines


# ------------------------------------------------------------------------------------------------
# Case model
# ------------------------------------------------------------------------------------------------
class Case(object):
    def __init__(self, cid, group, init_regs, init_apsr, init_mem, exp, mask, desc,
                 source=None, known=None):
        self.id = cid
        self.group = group
        self.init_regs = init_regs          # guest idx -> value
        self.init_apsr = init_apsr
        self.init_mem = init_mem            # list of (addr, bytes)
        self.exp = exp                      # dict: ret, apsr, regs, probes, idreg, idregv
        self.mask = mask
        self.desc = desc
        self.source = source
        self.known = known


def parse_hex(s, default=0):
    if s is None:
        return default
    if isinstance(s, int):
        return s
    return int(str(s), 0)


def load_json(path):
    with open(path, encoding="utf-8") as fh:
        return json.load(fh)


def load_baseline(path):
    """Known-failing case ids -> reason."""
    if not os.path.exists(path):
        return {}
    try:
        data = load_json(path)
    except Exception as exc:
        eprint("WARN: cannot parse baseline %s: %r" % (path, exc))
        return {}
    out = {}
    for k, v in data.get("known_failures", {}).items():
        out[k] = v if isinstance(v, str) else json.dumps(v)
    return out


def ref_read(state, addr, size):
    if size == 4:
        return state.read(addr, 4)
    if size == 2:
        return state.read(addr, 2)
    return state.read(addr, 1)


def build_golden_cases(gen, elf, warnings):
    """Mode B-1: golden cases from tests/aot_difftest_cases.json.

    A case is normally one instruction; 'more_bytes' allows a short instruction sequence
    (used for IT-block semantics, where the generator currently drops the predicate)."""
    data = load_json(CASES_JSON)
    cases = []
    for i, spec in enumerate(data["cases"]):
        cid = spec["id"]
        va = parse_hex(spec.get("va"), 0x81000000 + i * 4)
        parts = [spec["bytes"]] + list(spec.get("more_bytes", []))
        insns = []
        cur = va
        for part in parts:
            code = bytes.fromhex(part)
            ins = disasm_one(code, cur)
            if ins is None or ins.size != len(code):
                raise RuntimeError("case %s: bytes %s do not decode to one instruction" %
                                   (cid, part))
            if insn_is_terminator(ins):
                raise RuntimeError("case %s: instruction %r is a block terminator; mode B-1 "
                                   "only tests non-terminator instructions" % (cid, ins.mnemonic))
            insns.append(ins)
            cur += len(code)
        emit_lines = []
        for ins in insns:
            emit_lines.extend(gen.translate(ins))
        body = " ".join(emit_lines) + " return 0u;"

        init = spec.get("init", {})
        init_regs = {}
        for k, v in init.get("r", {}).items():
            init_regs[int(k)] = parse_hex(v)
        init_apsr = parse_hex(init.get("apsr"), 0)
        init_mem = []
        for ent in spec.get("mem", []):
            init_mem.append((parse_hex(ent[0]), bytes.fromhex(ent[1])))

        mask_name = spec.get("flags", "nzcv")
        mask = {"nzcv": F_NZCV, "nz": F_NZ, "none": 0}[mask_name]

        # --- reference execution ---------------------------------------------------------
        st = RefState(init_regs, init_apsr)
        st.seed(SEED_MEM)
        for addr, blob in init_mem:
            st.apply_bytes(addr, blob)
        st.writes = []
        for ins in insns:
            execute(st, ins)
        probes_spec = [(parse_hex(p[0]), int(p[1])) for p in spec.get("probes", [])]
        probes = []
        for addr, size in probes_spec:
            probes.append((addr, size, ref_read(st, addr, size)))

        exp = {
            "ret": 0,
            "apsr": st.apsr,
            "regs": list(st.r),
            "probes": probes,
            "idreg": None,
            "idregv": None,
        }
        # hand-derived expectations double as a reference-model self check
        hand = spec.get("expect", {})
        drift = []
        for k, v in hand.get("r", {}).items():
            want = parse_hex(v)
            got = st.r[int(k)]
            if want != got:
                drift.append("r%s want=0x%08X model=0x%08X" % (k, want, got))
        if "apsr" in hand:
            want = parse_hex(hand["apsr"])
            if want != st.apsr:
                drift.append("apsr want=0x%08X model=0x%08X" % (want, st.apsr))
        if "ret" in hand:
            want = parse_hex(hand["ret"])
            if want != exp["ret"]:
                drift.append("ret want=0x%08X model=0x%08X" % (want, exp["ret"]))
        if drift:
            raise RuntimeError("case %s: hand expectation disagrees with reference model: %s"
                               % (cid, "; ".join(drift)))

        cases.append(Case(cid, "core", init_regs, init_apsr, init_mem, exp, mask,
                          spec.get("desc", ""), source=spec.get("src")))
        cases[-1]._emit_body = body
        cases[-1].probes_spec = probes_spec
        cases[-1].insns = insns
    return cases


# ------------------------------------------------------------------------------------------------
# production block discovery
# ------------------------------------------------------------------------------------------------
BLK_DEF_RE = re.compile(
    r"^static uint32_t blk_([0-9A-F]{8})\(RecState\* s\) \{ g_cur_block = 0x[0-9A-F]+u; (.*) \}$")
RET_RE = re.compile(r"return 0x([0-9A-F]+)u;$")


def _lacks_risky_insns(insns):
    """Reject instructions whose generated semantics is knowingly incomplete/deviating, so the
    production-block suite stays a meaningful regression net instead of re-reporting the curated
    golden-case findings (those live in tests/aot_difftest_baseline.json)."""
    for ins in insns:
        m = base_mnemonic(ins.mnemonic)
        flagset = m in FLAGSET_MNEM
        base = m[:-1] if flagset else m
        if any(ins.mnemonic.startswith(p) and not ins.mnemonic.startswith(("dmb", "dsb"))
               for p in VFP_PREFIX):
            return False
        if m in ("it", "itt", "ite", "itet", "ittt", "itee", "hint"):
            return False
        if m not in ("b", "bl", "bx", "blx") and _has_cond_suffix(m):
            return False
        if base in ("lsl", "lsr", "asr", "ror"):
            amt_op = ins.operands[2] if len(ins.operands) >= 3 else ins.operands[1]
            if amt_op.type == OP_REG:
                return False  # shift amount masking bug (golden cases cover it)
            if amt_op.type == OP_IMM and amt_op.imm == 32:
                return False  # imm LSR/ASR #32 bug (golden cases cover it)
        if base in ("adc", "sbc") and not flagset:
            return False  # generated code wrongly updates flags for non-S ADC/SBC
        for op in ins.operands:
            if op.type == OP_REG and _op_shift(op) is not None:
                kind, amt = _op_shift(op)
                if kind in ("LSR", "ASR") and amt == 32:
                    return False  # shifted operand LSR/ASR #32 bug
            if op.type == OP_MEM and op.mem.base == 11:
                return False  # PC-relative: generated code uses s->r[15] set by the harness only
        if any(op.type == OP_REG and op.reg == 11 for op in ins.operands):
            return False
    return True


def _has_cond_suffix(m):
    for suf in ("eq", "ne", "cs", "hs", "cc", "lo", "mi", "pl", "vs", "vc", "hi", "ls",
                "ge", "lt", "gt", "le"):
        if m.endswith(suf) and base_mnemonic(m[:-2]) in (set(LOGIC_MNEM) | set(ARITH_MNEM) |
                                                         {"mov", "mvn", "cmp", "cmn", "tst",
                                                          "teq", "ldr", "str"}):
            return True
    return False


def _insn_class(insns):
    """Classify a production block's instruction stream for corpus selection."""
    classes = set()
    for ins in insns:
        m = base_mnemonic(ins.mnemonic)
        flagset = m in FLAGSET_MNEM
        base = m[:-1] if flagset else m
        if base in MEM_MNEM and any(op.type == OP_MEM for op in ins.operands):
            if getattr(ins, "writeback", False):
                classes.add("writeback")
            if base in ("ldrd", "strd"):
                classes.add("dual")
        if base in ("push", "pop") or base in MULTI_MNEM:
            classes.add("multi")
        if base in ("lsl", "lsr", "asr", "ror"):
            amt = ins.operands[2] if len(ins.operands) >= 3 else ins.operands[1]
            if amt.type == OP_IMM:
                classes.add("shiftimm")
            else:
                classes.add("shiftreg")
        if base in ("add", "sub", "and", "orr", "eor", "cmp", "cmn", "tst", "teq", "rsb"):
            if any(op.type == OP_REG and _op_shift(op) is not None for op in ins.operands):
                classes.add("alushift")
        if base in ("adc", "sbc") and flagset:
            classes.add("carry")
    return classes


class BlockCase(Case):
    def __init__(self, cid, group, init_regs, init_apsr, exp, mask, desc, source,
                 block_va, block_line, kind):
        Case.__init__(self, cid, group, init_regs, init_apsr, [], exp, mask, desc, source)
        self.block_va = block_va
        self.block_line = block_line
        self.kind = kind


def _b2_init_attempt(va, attempt):
    rng = random.Random((va * 2654435761 + attempt * 0x9E3779B9) & 0xFFFFFFFF)
    regs = {}
    for i in range(13):
        regs[i] = 0x806E0000 + (rng.randrange(0, 0x400) & ~3)
    regs[13] = 0x806F0000
    regs[14] = 0x81000000 + (rng.randrange(0, 0x100) * 4)
    apsr = rng.randrange(16) << 28
    return regs, apsr


def _simulate_block(elf, prefix_insns):
    st = RefState()
    st.seed(SEED_MEM)
    st.writes = []
    for ins in prefix_insns:
        execute(st, ins)
    return st


def _flag_mask_for(insns):
    """Decide which APSR bits are asserted: flag-setting shifts/logic only promise N/Z in the
    current generator (C = shifter carry and V preservation are known deviations)."""
    for ins in insns:
        m = base_mnemonic(ins.mnemonic)
        flagset = m in FLAGSET_MNEM
        base = m[:-1] if flagset else m
        if not flagset:
            if base in ("adc", "sbc"):
                continue
            continue
        if base in ("lsl", "lsr", "asr", "ror", "mov", "mvn") or base in LOGIC_MNEM:
            return F_NZ
        if base in ("adc", "sbc"):
            return F_NZ
    return F_NZCV


def _make_probes_from_writes(writes):
    seen = {}
    out = []
    for addr, size, val in writes:
        for byte_addr in range(addr, addr + size):
            word = byte_addr & ~3
            if word not in seen:
                if word + 4 > GUEST_HI:
                    continue
                seen[word] = True
                out.append((word, 4))
    out.sort()
    return out


def _control_kind(body):
    """Cheap classification of a generated block body's control-flow terminator."""
    if body.endswith("/* bx lr */"):
        return "bx_lr"
    if "_tgt = s->r[14]" in body:
        return "blx_lr"
    if "return id_reg(" in body:
        return "blx_reg"
    if "/* bl */" in body:
        return "bl"
    if "uint32_t _pc = rd32" in body:
        return "pop_pc"
    if body.startswith("if (cond_"):
        return "cond"
    return None


def build_block_cases(elf, args, stats):
    """Mode B-2: extract real production block functions from batch_*.cpp and diff them."""
    if not os.path.isdir(BATCH_DIR):
        stats["warn"].append("batch dir %s not found; production-block tests skipped" % BATCH_DIR)
        return []
    batch_files = sorted(glob.glob(os.path.join(BATCH_DIR, "batch_*.cpp")))
    if not batch_files:
        stats["warn"].append("no batch_*.cpp in %s; production-block tests skipped" % BATCH_DIR)
        return []
    # NOTE: register-amount shift blocks are intentionally *not* used here: the generator's
    # Rs[7:0] masking bug is covered by the curated golden cases (baseline), and random register
    # values would make this production corpus noisy.
    limits = {"writeback": 8, "shiftimm": 6, "dual": 4, "multi": 5, "alushift": 6,
              "carry": 3, "any": 6}
    ctrl_limits = {"blx_lr": 3, "blx_reg": 3, "bl": 2, "bx_lr": 2, "pop_pc": 3, "cond": 3}
    counts = {k: 0 for k in limits}
    ctrl_counts = {k: 0 for k in ctrl_limits}
    max_total = args.max_blocks
    selected = []
    used_va = set()
    stats["scanned_blocks"] = 0
    t_scan = time.time()
    scan_budget = getattr(args, "scan_seconds", 8.0)

    def ft_full():
        return all(counts[k] >= limits[k] for k in limits)

    def ctrl_full():
        return all(ctrl_counts[k] >= ctrl_limits[k] for k in ctrl_limits)

    def need_more():
        total = sum(counts.values()) + sum(ctrl_counts.values())
        if total >= max_total:
            return False
        return not (ft_full() and ctrl_full())

    def over_budget():
        return (time.time() - t_scan) > scan_budget

    for bf in batch_files:
        if not need_more() or over_budget():
            break
        try:
            with open(bf, encoding="utf-8", errors="replace") as fh:
                for line in fh:
                    if not line.startswith("static uint32_t blk_"):
                        continue
                    if not need_more() or over_budget():
                        break
                    mo = BLK_DEF_RE.match(line.rstrip("\n"))
                    if not mo:
                        continue
                    va = int(mo.group(1), 16)
                    if va in used_va:
                        continue
                    body = mo.group(2)
                    line = line.rstrip("\n")

                    # control-flow block terminators (BL / BLX / BX LR / POP {..PC} / cond B)
                    ckind = _control_kind(body)
                    if ckind is not None:
                        if ctrl_counts[ckind] >= ctrl_limits[ckind]:
                            continue
                        try:
                            cc = _try_control_block(elf, va, body, line)
                        except Exception:
                            cc = None
                        if cc is not None and ctrl_counts[ckind] < ctrl_limits[ckind]:
                            ctrl_counts[ckind] += 1
                            used_va.add(va)
                            selected.append(cc)
                        continue

                    # straight-line fallthrough block
                    ret_m = RET_RE.search(body)
                    if not ret_m:
                        continue
                    if ("id_" in body or "hle_call" in body or "UNSUPPORTED" in body
                            or "cond_" in body):
                        continue
                    fall = int(ret_m.group(1), 16)
                    d = fall - va
                    if d <= 0 or d > 24 or d % 2:
                        continue
                    try:
                        insns = elf.disasm_range(va, fall)
                    except Exception:
                        continue
                    if len(insns) > args.max_insns:
                        continue
                    if any(insn_is_terminator(i) for i in insns):
                        continue
                    classes = _insn_class(insns)
                    cls = None
                    for cand in ("writeback", "dual", "multi", "alushift", "carry", "shiftimm"):
                        if cand in classes and counts[cand] < limits[cand]:
                            cls = cand
                            break
                    if cls is None:
                        if counts["any"] >= limits["any"]:
                            continue
                        cls = "any"
                    if not _lacks_risky_insns(insns):
                        continue
                    stats["scanned_blocks"] += 1
                    try:
                        bc = _finish_fallthrough_block(va, line, fall, insns, cls)
                    except Exception:
                        bc = None
                    if bc is not None:
                        counts[cls] += 1
                        used_va.add(va)
                        selected.append(bc)
        except Exception as exc:
            stats["warn"].append("scan %s: %r" % (os.path.basename(bf), exc))
    stats["scan_s"] = time.time() - t_scan
    stats["block_classes"] = dict(counts)
    stats["ctrl_classes"] = dict(ctrl_counts)
    missing = [k for k in limits if counts[k] < limits[k]]
    missing += [k for k in ctrl_limits if ctrl_counts[k] < ctrl_limits[k]]
    if missing:
        stats["warn"].append("block corpus under target for: %s (scan budget %.1fs)"
                             % (", ".join(missing), scan_budget))
    for case in selected:
        stats["by_group"].setdefault("block:" + case.kind, 0)
        stats["by_group"]["block:" + case.kind] += 1
    return selected


def _finish_fallthrough_block(va, line, fall, insns, cls):
    """Try deterministic initial states until the straight-line block runs with valid memory,
    then build the expected state with the reference model."""
    for attempt in range(6):
        regs, apsr = _b2_init_attempt(va, attempt)
        st = RefState(regs, apsr)
        st.seed(SEED_MEM)
        st.writes = []
        try:
            for ins in insns:
                execute(st, ins)
        except Unsupported:
            continue
        mask = _flag_mask_for(insns)
        probes = _make_probes_from_writes(st.writes)
        exp = {"ret": fall, "apsr": st.apsr, "regs": list(st.r),
               "probes": [(a, s, ref_read(st, a, s)) for (a, s) in probes],
               "idreg": None, "idregv": None}
        desc = "production block 0x%08X (%d insn%s): %s" % (
            va, len(insns), "" if len(insns) == 1 else "s",
            "; ".join("%s %s" % (i.mnemonic, i.op_str) for i in insns))
        bc = BlockCase("blk_%08X" % va, "block:" + cls, regs, apsr, exp, mask, desc,
                       "batch", va, line, cls)
        bc.probes_spec = probes
        bc.insns = insns
        return bc
    return None


def _find_terminator(elf, va, kind, max_insns=10):
    """Disassemble forward until the instruction matching 'kind' is found."""
    insns = []
    cur = va
    for _ in range(max_insns):
        code = elf.read(cur, 4)
        if code is None or len(code) < 2:
            return None
        one = list(_MD.disasm(code, cur))
        if not one:
            return None
        ins = one[0]
        insns.append(ins)
        m = base_mnemonic(ins.mnemonic)
        hit = False
        if kind == "bx_lr":
            hit = m == "bx" and ins.operands and ins.operands[0].type == OP_REG and ins.operands[0].reg == 10
        elif kind in ("blx_lr", "blx_reg"):
            hit = m == "blx"
        elif kind == "bl":
            hit = m == "bl"
        elif kind == "pop_pc":
            hit = m == "pop" and any(op.type == OP_REG and op.reg == 11 for op in ins.operands)
        elif kind == "cond":
            hit = m in ("beq", "bne", "bcs", "bhs", "bcc", "blo", "bmi", "bpl", "bvs", "bvc",
                        "bhi", "bls", "bge", "blt", "bgt", "ble")
        if hit:
            return insns
        cur += ins.size
    return None


def _try_control_block(elf, va, body, line):
    if "UNSUPPORTED" in body or "id_tb" in body or "idmem" in body:
        return None
    kind = None
    fall = None
    target = None
    idreg = None
    if body.endswith("/* bx lr */"):
        kind = "bx_lr"
    elif "t_gen = s->r[14]" in body:  # cannot happen; keep explicit below
        kind = "blx_lr"
    elif "_tgt = s->r[14]" in body:
        kind = "blx_lr"
    elif "return id_reg(" in body:
        kind = "blx_reg"
    elif "/* bl */" in body:
        kind = "bl"
    elif "uint32_t _pc = rd32" in body:
        kind = "pop_pc"
    elif body.startswith("if (cond_"):
        kind = "cond"
    if kind is None:
        return None
    insns = _find_terminator(elf, va, kind)
    if insns is None:
        return None
    if len(insns) > 8:
        return None
    prefix = insns[:-1]
    if any(insn_is_terminator(i) for i in prefix):
        return None
    if not _lacks_risky_insns(prefix):
        return None
    if kind == "cond" and _flag_mask_for(prefix) != F_NZCV:
        # the branch predicate may depend on flag bits that the generator's shift/logic
        # sequences do not maintain exactly (see golden-case baseline)
        return None
    if kind == "blx_lr":
        mo = re.search(r"s->r\[14\] = 0x([0-9A-F]+)u; return id_regv", body)
        if not mo:
            return None
        fall = int(mo.group(1), 16)
    elif kind == "blx_reg":
        mo = re.search(r"s->r\[14\] = 0x([0-9A-F]+)u; return id_reg\((\d+), s\)", body)
        if not mo:
            return None
        fall = int(mo.group(1), 16)
        idreg = int(mo.group(2))
    elif kind == "bl":
        mo = re.search(r"s->r\[14\] = 0x([0-9A-F]+)u; return 0x([0-9A-F]+)u;", body)
        if not mo:
            return None
        fall = int(mo.group(1), 16)
        target = int(mo.group(2), 16)
    elif kind == "cond":
        mo = re.search(r"if \((cond_\w+)\(s\)\) return 0x([0-9A-F]+)u; return 0x([0-9A-F]+)u;", body)
        if not mo:
            return None
    for attempt in range(6):
        regs, apsr = _b2_init_attempt(va, attempt)
        st = RefState(regs, apsr)
        st.seed(SEED_MEM)
        st.writes = []
        try:
            for ins in prefix:
                execute(st, ins)
        except Unsupported:
            continue
        exp = {"probes": [], "idreg": None, "idregv": None}
        if kind == "bx_lr":
            exp["ret"] = st.r[14]
            exp["regs"] = list(st.r)
            exp["apsr"] = st.apsr
        elif kind == "blx_lr":
            exp["ret"] = STUB_IDREGV
            exp["idregv"] = st.r[14]
            st.r[14] = fall
            exp["regs"] = list(st.r)
            exp["apsr"] = st.apsr
        elif kind == "blx_reg":
            exp["ret"] = STUB_IDREG
            exp["idreg"] = idreg
            st.r[14] = fall
            exp["regs"] = list(st.r)
            exp["apsr"] = st.apsr
        elif kind == "bl":
            exp["ret"] = target
            st.r[14] = fall
            exp["regs"] = list(st.r)
            exp["apsr"] = st.apsr
        elif kind == "pop_pc":
            sp = st.r[13]
            regs_list = sorted(cap_reg_idx(op.reg) for op in insns[-1].operands
                               if op.type == OP_REG)
            pcval = None
            for i, rg in enumerate(regs_list):
                v = st.read((sp + 4 * i) & 0xFFFFFFFF, 4)
                if rg == 15:
                    pcval = v
                else:
                    st.r[rg] = v
            st.r[13] = (sp + 4 * len(regs_list)) & 0xFFFFFFFF
            if pcval is None:
                continue
            exp["ret"] = pcval
            exp["regs"] = list(st.r)
            exp["apsr"] = st.apsr
        elif kind == "cond":
            mo = re.search(r"if \((cond_\w+)\(s\)\) return 0x([0-9A-F]+)u; return 0x([0-9A-F]+)u;",
                           body)
            pred = COND_PRED[mo.group(1)]
            taken = pred(st)
            exp["ret"] = int(mo.group(2), 16) if taken else int(mo.group(3), 16)
            exp["regs"] = list(st.r)
            exp["apsr"] = st.apsr
        mask = _flag_mask_for(prefix)
        desc = "production %s block 0x%08X: %s" % (
            kind, va, "; ".join("%s %s" % (i.mnemonic, i.op_str) for i in insns))
        bc = BlockCase("blk_%08X" % va, "block:ctrl", regs, apsr, exp, mask, desc,
                       "batch", va, line, "ctrl_" + kind)
        bc.probes_spec = []
        bc.insns = insns
        return bc
    return None


# ------------------------------------------------------------------------------------------------
# host generation / compilation / execution
# ------------------------------------------------------------------------------------------------
def _cpp_u32(v):
    return "0x%08Xu" % (v & 0xFFFFFFFF)


def _gen_init_fn(case, idx):
    out = ["static void init_case_%d(RecState* s) {" % idx]
    for r in sorted(case.init_regs):
        out.append("  s->r[%d] = %s;" % (r, _cpp_u32(case.init_regs[r])))
    out.append("  s->apsr = %s;" % _cpp_u32(case.init_apsr))
    for addr, blob in case.init_mem:
        for off, b in enumerate(blob):
            out.append("  P(s, 0x%08Xu)[0] = (uint8_t)0x%02Xu;" % (addr + off, b))
    out.append("}")
    return out


def _gen_probe_arrays(case, idx):
    probes = getattr(case, "probes_spec", None) or []
    if not probes:
        return ["static const uint32_t probe_addr_%d[1] = {0u};" % idx,
                "static const uint16_t probe_size_%d[1] = {0u};" % idx]
    addrs = ", ".join("0x%08Xu" % a for a, _ in probes)
    sizes = ", ".join("%du" % s for _, s in probes)
    return ["static const uint32_t probe_addr_%d[%d] = {%s};" % (idx, len(probes), addrs),
            "static const uint16_t probe_size_%d[%d] = {%s};" % (idx, len(probes), sizes)]


def _gen_run_fn(case, idx):
    if isinstance(case, BlockCase):
        return ["static uint32_t run_case_%d(RecState* s) { return %s(s); }"
                % (idx, "blk_%08X" % case.block_va)]
    body = getattr(case, "_emit_body", "return 0u;")
    return ["static uint32_t run_case_%d(RecState* s) { %s }" % (idx, body)]


def generate_host_source(gen, golden_cases, block_cases, host_path):
    template = open(HOST_TEMPLATE, encoding="utf-8").read()
    cases = list(golden_cases) + list(block_cases)

    # verbatim block function definitions (deduplicated)
    seen = set()
    block_lines = []
    for case in block_cases:
        if case.block_va in seen:
            continue
        seen.add(case.block_va)
        block_lines.append(case.block_line)
    blocks_text = "\n".join(block_lines)

    chunks = []
    chunks.append("// Generated by tools/aot_difftest.py - do not edit.")
    chunks.append("static void seed_mem(RecState* s) {")
    chunks.append("  for (uint32_t i = 0; i < 0x100u; ++i) wr32(s, 0x806E0000u + i * 4u, "
                  "(uint32_t)(0xA5A50000u + i * 0x00010001u));")
    chunks.append("  for (uint32_t i = 0; i < 0x40u; ++i) wr32(s, 0x806F0000u - 0x100u + i * 4u, "
                  "(uint32_t)(0x5A5A0000u + i * 0x00010001u));")
    chunks.append("  for (uint32_t i = 0; i < 0x40u; ++i) wr32(s, 0x80601000u + i * 4u, "
                  "(uint32_t)(0x11223344u + i));")
    chunks.append("}")
    for idx, case in enumerate(cases):
        chunks.extend(_gen_init_fn(case, idx))
        chunks.extend(_gen_probe_arrays(case, idx))
        chunks.extend(_gen_run_fn(case, idx))
    table = ["static const CaseDef g_cases[] = {"]
    for idx, case in enumerate(cases):
        nprobe = len(getattr(case, "probes_spec", None) or [])
        table.append('  {"%s", init_case_%d, run_case_%d, probe_addr_%d, probe_size_%d, %d},'
                     % (case.id, idx, idx, idx, idx, nprobe))
    table.append("};")
    chunks.extend(table)
    cases_text = "\n".join(chunks)

    out = template.replace("/*@@PREAMBLE@@*/", gen.preamble)
    out = out.replace("/*@@BLOCKS@@*/", blocks_text)
    out = out.replace("/*@@CASES@@*/", cases_text)
    with open(host_path, "w", encoding="utf-8") as fh:
        fh.write(out)
    return host_path


def compile_host(host_path, exe_path, args, stats):
    cmd = [args.gxx, "-std=c++17", args.opt, "-w", host_path, "-o", exe_path]
    env = dict(os.environ)
    os.makedirs(GTMP, exist_ok=True)
    env["TMP"] = GTMP
    env["TEMP"] = GTMP
    t0 = time.time()
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=args.compile_timeout,
                           env=env)
    except subprocess.TimeoutExpired:
        raise RuntimeError("host compilation timed out after %ss" % args.compile_timeout)
    dt = time.time() - t0
    stats["compile_s"] = dt
    if r.returncode != 0:
        err = (r.stderr or "")[-4000:]
        raise RuntimeError("host compilation failed (rc=%d):\n%s" % (r.returncode, err))
    return exe_path


def run_host(exe_path, args):
    env = dict(os.environ)
    env["TMP"] = GTMP
    env["TEMP"] = GTMP
    t0 = time.time()
    try:
        r = subprocess.run([exe_path], capture_output=True, text=True, timeout=args.run_timeout,
                           env=env)
    except subprocess.TimeoutExpired:
        raise RuntimeError("host run timed out after %ss" % args.run_timeout)
    dt = time.time() - t0
    if r.returncode != 0:
        raise RuntimeError("host exited rc=%d\nstdout tail:\n%s\nstderr tail:\n%s"
                           % (r.returncode, (r.stdout or "")[-2000:], (r.stderr or "")[-2000:]))
    return r.stdout or "", dt


def parse_host_output(text):
    results = {}
    probes = {}
    for line in text.splitlines():
        if line.startswith("RESULT "):
            parts = line.split()
            name = parts[1]
            rec = {"status": parts[2]}
            if rec["status"] == "OK":
                rec["ret"] = int(parts[3], 16)
                rec["apsr"] = int(parts[4], 16)
                rec["idreg"] = int(parts[5], 10)
                rec["idregv"] = int(parts[6], 16)
                rec["hle"] = int(parts[7], 16)
                regs = []
                for tok in parts[8:24]:
                    regs.append(int(tok, 16))
                rec["regs"] = regs
            else:
                rec["va"] = int(parts[3], 16) if len(parts) > 3 else 0xFFFFFFFF
            results[name] = rec
        elif line.startswith("PROBE "):
            parts = line.split()
            name = parts[1]
            addr = int(parts[2], 16)
            size = int(parts[3], 10)
            value = int(parts[4], 16)
            probes.setdefault(name, []).append((addr, size, value))
    return results, probes


# ------------------------------------------------------------------------------------------------
# comparison / reporting
# ------------------------------------------------------------------------------------------------
class GroupStats(object):
    def __init__(self, name):
        self.name = name
        self.pass_n = 0
        self.warn_n = 0
        self.known_n = 0
        self.fail_n = 0
        self.details = []    # (status, case id, text)
        self.stale_baseline = []

    def add(self, status, cid, text=None):
        if status == "PASS":
            self.pass_n += 1
        elif status == "WARN":
            self.warn_n += 1
        elif status == "KNOWN":
            self.known_n += 1
        else:
            self.fail_n += 1
        if text:
            self.details.append((status, cid, text))


def _flag_diff_detail(exp_apsr, got_apsr):
    names = (("N", F_N), ("Z", F_Z), ("C", F_C), ("V", F_V))
    parts = []
    for name, bit in names:
        if (exp_apsr & bit) != (got_apsr & bit):
            parts.append("%s exp=%d got=%d" % (name, 1 if exp_apsr & bit else 0,
                                               1 if got_apsr & bit else 0))
    return ", ".join(parts)


def compare_case(case, result, probe_records, known_map, strict):
    problems = []   # (severity, text) severity: FATAL/WARN
    if result is None:
        return "FAIL", "case did not produce a RESULT line"
    if result.get("status") != "OK":
        return "FAIL", "BADMEM 0x%08X" % result.get("va", 0)
    exp = case.exp
    # registers
    for i in range(16):
        want = exp["regs"][i]
        got = result["regs"][i]
        if want != got:
            problems.append(("FATAL", "r%d exp=0x%08X got=0x%08X" % (i, want, got)))
    # return / next-PC value
    if "ret" in exp and exp["ret"] is not None and result["ret"] != exp["ret"]:
        problems.append(("FATAL", "ret exp=0x%08X got=0x%08X" % (exp["ret"], result["ret"])))
    # APSR: asserted mask is fatal, deviations outside mask are reported as WARN
    if (exp["apsr"] & case.mask) != (result["apsr"] & case.mask):
        problems.append(("FATAL", "apsr(asserted) [%s]" %
                         _flag_diff_detail(exp["apsr"] & case.mask,
                                           result["apsr"] & case.mask)))
    elif exp["apsr"] != result["apsr"]:
        problems.append(("WARN", "apsr [%s]" %
                         _flag_diff_detail(exp["apsr"], result["apsr"])))
    # memory probes
    got_probes = {(a, s): v for (a, s, v) in probe_records.get(case.id, [])}
    for addr, size, want in exp["probes"]:
        key = (addr, size)
        if key not in got_probes:
            problems.append(("FATAL", "missing probe 0x%08X/%d" % (addr, size)))
            continue
        got = got_probes[key]
        if got != want:
            problems.append(("FATAL", "mem[0x%08X]/%d exp=0x%08X got=0x%08X"
                             % (addr, size, want, got)))
    # indirect transfer stubs
    if exp.get("idreg") is not None and result["idreg"] != exp["idreg"]:
        problems.append(("FATAL", "id_reg idx exp=%d got=%d" % (exp["idreg"], result["idreg"])))
    if exp.get("idregv") is not None and result["idregv"] != exp["idregv"]:
        problems.append(("FATAL", "id_regv target exp=0x%08X got=0x%08X"
                         % (exp["idregv"], result["idregv"])))
    if result["hle"] != 0xFFFFFFFF:
        problems.append(("FATAL", "unexpected HLE call 0x%08X" % result["hle"]))
    fatal = [t for sev, t in problems if sev == "FATAL"]
    warns = [t for sev, t in problems if sev == "WARN"]
    detail = "; ".join(fatal + warns)
    if fatal:
        if case.id in known_map and not strict:
            return "KNOWN", "known deviation (" + known_map[case.id] + "): " + detail
        return "FAIL", detail
    if warns:
        return "WARN", "; ".join(warns)
    return "PASS", None


# ------------------------------------------------------------------------------------------------
# optional Unicorn cross-check (mode A)
# ------------------------------------------------------------------------------------------------
def unicorn_available():
    try:
        import unicorn  # noqa: F401
        return True
    except Exception:
        return False


def _unicorn_compare(case, result, probe_records, stats):
    """Emulate the same instruction stream with unicorn; compare generated result vs unicorn."""
    import unicorn
    import unicorn.arm_const as uac
    from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_LITTLE_ENDIAN
    insns = getattr(case, "insns", None)
    if not insns:
        return None
    mu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_LITTLE_ENDIAN)
    try:
        mu.mem_map(GUEST_LO, MEM_SIZE)
    except Exception as exc:
        stats["warn"].append("unicorn mem_map failed: %r" % (exc,))
        return None
    # seed memory
    try:
        for addr, size, val in SEED_MEM:
            mu.mem_write(addr, val.to_bytes(size, "little"))
        for addr, blob in case.init_mem:
            mu.mem_write(addr, bytes(blob))
        for ins in insns:
            mu.mem_write(ins.address, bytes(ins.bytes))
        for rg in range(16):
            mu.reg_write(getattr(uac, "UC_ARM_REG_R%d" % rg), case.init_regs.get(rg, 0))
        mu.reg_write(uac.UC_ARM_REG_CPSR, case.init_apsr | 0x20)
        start = insns[0].address
        end = insns[-1].address + insns[-1].size
        mu.emu_start(start | 1, end, timeout=0, count=len(insns))
    except Exception as exc:
        return {"error": "%s: %r" % (type(exc).__name__, exc)}
    uregs = [mu.reg_read(getattr(uac, "UC_ARM_REG_R%d" % rg)) for rg in range(16)]
    cpsr = mu.reg_read(uac.UC_ARM_REG_CPSR)
    uapsr = cpsr & 0xF0000000
    # Compare generated (host) against unicorn.  The Python model should already agree; if the
    # model and unicorn disagree the framework itself is suspect and we say so.
    diffs = []
    for i in range(15):
        if result["regs"][i] != uregs[i]:
            diffs.append("r%d gen=0x%08X unicorn=0x%08X" % (i, result["regs"][i], uregs[i]))
    if (case.mask & result["apsr"]) != (case.mask & uapsr):
        diffs.append("apsr assert-bits gen=0x%08X unicorn=0x%08X" % (result["apsr"] & case.mask,
                                                                  uapsr & case.mask))
    model_diffs = []
    for i in range(15):
        want = case.exp["regs"][i]
        if want != uregs[i]:
            model_diffs.append("r%d model=0x%08X unicorn=0x%08X" % (i, want, uregs[i]))
    if (case.exp["apsr"] & case.mask) != (case.mask & uapsr):
        model_diffs.append("apsr model=0x%08X unicorn=0x%08X" % (case.exp["apsr"], uapsr))
    for addr, size, want in case.exp["probes"]:
        try:
            got = int.from_bytes(mu.mem_read(addr, size), "little")
        except Exception:
            continue
        if want != got:
            model_diffs.append("mem[0x%08X] model=0x%08X unicorn=0x%08X" % (addr, want, got))
    return {"diffs": diffs, "model_diffs": model_diffs}


# ------------------------------------------------------------------------------------------------
# main
# ------------------------------------------------------------------------------------------------
def main(argv=None):
    ap = argparse.ArgumentParser(description="P5D AOT instruction-semantics differential test")
    ap.add_argument("--mode", choices=["all", "core", "blocks"], default="all")
    ap.add_argument("--no-unicorn", action="store_true", help="skip the optional unicorn cross-check")
    ap.add_argument("--strict", action="store_true", help="treat known-baseline failures as fatal")
    ap.add_argument("--only", default=None, help="substring filter on case ids")
    ap.add_argument("--verbose", action="store_true")
    ap.add_argument("--keep", action="store_true", help="keep the generated host sources")
    ap.add_argument("--gxx", default=GXX)
    ap.add_argument("--opt", default="-O2",
                    help="optimisation level for the generated host "
                         "(-O2 matches the production batch compilation)")
    ap.add_argument("--elf", default=ELF_PATH)
    ap.add_argument("--max-blocks", type=int, default=52)
    ap.add_argument("--scan-seconds", type=float, default=8.0,
                    help="time budget for scanning batch_*.cpp for production blocks")
    ap.add_argument("--max-insns", type=int, default=8)
    ap.add_argument("--compile-timeout", type=int, default=120)
    ap.add_argument("--run-timeout", type=int, default=90)
    ap.add_argument("--list-cases", action="store_true")
    args = ap.parse_args(argv)

    t_start = time.time()
    stats = {"warn": [], "by_group": {}, "scanned_blocks": 0}
    known_map = load_baseline(BASELINE_JSON)

    print("=== AOT DIFFTEST (mode=%s, strict=%s) ===" % (args.mode, args.strict))
    print("root        : %s" % ROOT)
    print("generator   : %s" % GENERATOR)
    print("batch dir   : %s" % BATCH_DIR)
    print("elf         : %s" % args.elf)
    if not os.path.exists(args.elf):
        eprint("FATAL: ELF not found: %s" % args.elf)
        return 2
    elf = ElfImage(args.elf)
    try:
        gen = Generator(GENERATOR, os.path.join(BATCH_DIR, "batch_000.cpp"))
    except Exception as exc:
        eprint("FATAL: cannot load generator semantics: %r" % (exc,))
        return 2

    golden_cases = []
    block_cases = []
    try:
        if args.mode in ("all", "core"):
            golden_cases = build_golden_cases(gen, elf, stats)
        if args.mode in ("all", "blocks"):
            block_cases = build_block_cases(elf, args, stats)
    except RuntimeError as exc:
        eprint("FATAL: %s" % exc)
        return 2
    if args.only:
        golden_cases = [c for c in golden_cases if args.only in c.id]
        block_cases = [c for c in block_cases if args.only in c.id]

    if args.list_cases:
        for c in golden_cases:
            print("core   %-28s %s" % (c.id, c.desc))
        for c in block_cases:
            print("block  %-28s %s" % (c.id, c.desc))
        return 0
    if not golden_cases and not block_cases:
        eprint("FATAL: no test cases selected")
        return 2

    # ---- generate + compile + run ------------------------------------------------------
    os.makedirs(GTMP, exist_ok=True)
    work = os.path.join(GTMP, "aot_difftest_%d" % os.getpid())
    if os.path.isdir(work):
        shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work, exist_ok=True)
    host_path = os.path.join(work, "aot_difftest_host.cpp")
    exe_path = os.path.join(work, "aot_difftest_host.exe")
    try:
        generate_host_source(gen, golden_cases, block_cases, host_path)
        stats["gen_bytes"] = os.path.getsize(host_path)
        compile_host(host_path, exe_path, args, stats)
        stdout, run_dt = run_host(exe_path, args)
    except RuntimeError as exc:
        eprint("FATAL: %s" % exc)
        if args.keep:
            eprint("generated host kept at %s" % host_path)
        else:
            shutil.rmtree(work, ignore_errors=True)
        return 2
    stats["run_s"] = run_dt
    results, probes = parse_host_output(stdout)

    # ---- compare -----------------------------------------------------------------------
    groups = {}
    failed_cases = []

    def group(name):
        if name not in groups:
            groups[name] = GroupStats(name)
        return groups[name]

    for case in golden_cases + block_cases:
        status, detail = compare_case(case, results.get(case.id), probes, known_map, args.strict)
        group_name = "core" if case in golden_cases else (
            case.group if case.group.startswith("block:ctrl") else "blocks")
        g = group(group_name)
        g.add(status, case.id, detail)
        if status == "FAIL":
            failed_cases.append(case.id)
        if status == "KNOWN":
            if case.id not in known_map:
                g.stale_baseline.append(case.id)

    # ---- optional unicorn cross-check ---------------------------------------------------
    unicorn_group = None
    if args.mode in ("all", "blocks") and not args.no_unicorn and unicorn_available():
        unicorn_group = GroupStats("unicorn")
        n_done = 0
        for case in block_cases:
            if case.kind.startswith("ctrl"):
                continue
            if n_done >= args.max_blocks:
                break
            if case.id not in results or results[case.id].get("status") != "OK":
                continue
            r = _unicorn_compare(case, results[case.id], probes, stats)
            if r is None:
                continue
            n_done += 1
            if "error" in r:
                unicorn_group.add("WARN", case.id, "unicorn skip: " + r["error"])
                continue
            if r["model_diffs"]:
                unicorn_group.add("FAIL", case.id,
                                  "REFERENCE MODEL vs UNICORN: " + "; ".join(r["model_diffs"][:4]))
            elif r["diffs"]:
                unicorn_group.add("FAIL", case.id, "; ".join(r["diffs"][:6]))
            else:
                unicorn_group.add("PASS", case.id)

    # ---- optional unicorn cross-check of the *reference model* on the golden cases ---------
    ref_group = None
    if (golden_cases and not args.no_unicorn and unicorn_available()
            and args.mode in ("all", "core")):
        ref_group = GroupStats("refcheck")
        for case in golden_cases:
            res = {"regs": list(case.exp["regs"]), "apsr": case.exp["apsr"]}
            r = _unicorn_compare(case, res, {}, stats)
            if r is None:
                ref_group.add("WARN", case.id, "no instruction stream")
                continue
            if "error" in r:
                ref_group.add("WARN", case.id, "unicorn skip: " + r["error"])
            elif r["diffs"] or r["model_diffs"]:
                ref_group.add("FAIL", case.id,
                              "REFERENCE MODEL vs UNICORN: " +
                              "; ".join((r["model_diffs"] + r["diffs"])[:4]))
            else:
                ref_group.add("PASS", case.id)

    # ---- report ------------------------------------------------------------------------
    print("")
    print("--- cases ---------------------------------------------------------------------")
    print("generator emit cases : %d" % len(golden_cases))
    print("production blocks    : %d (classes: %s)" %
          (len(block_cases), json.dumps(stats.get("block_classes", {}), sort_keys=True)))
    print("  ctrl blocks        : %s" % json.dumps(stats.get("ctrl_classes", {}), sort_keys=True))
    print("host compile/run     : %.2fs / %.2fs (%d bytes of generated C++)" %
          (stats.get("compile_s", 0.0), stats.get("run_s", 0.0), stats.get("gen_bytes", 0)))
    print("")
    total_fail = 0
    for name in sorted(groups):
        g = groups[name]
        print("[%-16s] PASS=%-3d WARN=%-3d KNOWN=%-3d FAIL=%-3d" %
              (name, g.pass_n, g.warn_n, g.known_n, g.fail_n))
        total_fail += g.fail_n
    if unicorn_group is not None:
        g = unicorn_group
        print("[%-16s] PASS=%-3d WARN=%-3d FAIL=%-3d  (generated-vs-unicorn)" %
              ("unicorn", g.pass_n, g.warn_n, g.fail_n))
        total_fail += g.fail_n
    if ref_group is not None:
        g = ref_group
        print("[%-16s] PASS=%-3d WARN=%-3d FAIL=%-3d  (reference-model-vs-unicorn)" %
              ("refcheck", g.pass_n, g.warn_n, g.fail_n))
        total_fail += g.fail_n

    def dump(g, statuses, title):
        rows = [(s, cid, txt) for (s, cid, txt) in g.details if s in statuses]
        if not rows and not (g.stale_baseline and "stale" in statuses):
            return
        print("")
        print("--- %s ---" % title)
        for s, cid, txt in rows:
            print("  %-6s %-26s %s" % (s, cid, txt or ""))
        if "stale" in statuses:
            for cid in g.stale_baseline:
                print("  STALE  %-26s listed in baseline but now passes; remove it" % cid)

    all_groups = [groups[k] for k in sorted(groups)]
    if unicorn_group is not None:
        all_groups.append(unicorn_group)
    if ref_group is not None:
        all_groups.append(ref_group)
    for g in all_groups:
        dump(g, ("FAIL",), "failures (%s)" % g.name)
    for g in all_groups:
        dump(g, ("KNOWN",), "known baseline deviations (%s)" % g.name)
    for g in all_groups:
        dump(g, ("WARN",), "warnings / C-V flag deviations (%s)" % g.name)
    for g in all_groups:
        dump(g, ("ok", "stale"), "stale baseline entries (%s)" % g.name)

    for w in stats["warn"]:
        print("WARN: %s" % w)

    n_pass = sum(g.pass_n for g in all_groups)
    n_warn = sum(g.warn_n for g in all_groups)
    n_known = sum(g.known_n for g in all_groups)
    dt = time.time() - t_start
    print("")
    print("SUMMARY: PASS=%d WARN=%d KNOWN=%d FAIL=%d  (%.1fs)" %
          (n_pass, n_warn, n_known, total_fail, dt))
    if total_fail:
        print("RESULT: FAIL")
        if args.keep:
            print("generated host kept at %s" % host_path)
        else:
            shutil.rmtree(work, ignore_errors=True)
        return 1
    print("RESULT: PASS")
    if not args.keep:
        shutil.rmtree(work, ignore_errors=True)
    else:
        print("generated host kept at %s" % host_path)
    return 0


if __name__ == "__main__":
    sys.exit(main())

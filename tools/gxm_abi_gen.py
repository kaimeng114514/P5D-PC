# SPDX-License-Identifier: MIT
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""gxm_abi_gen.py -- generate the per-NID SceGxm HLE ABI wrapper layer.

WHY
---
The PS Vita eboot calls the imported `sceGxm*` functions through their real ARM
AAPCS ABI: the first four arguments live in r0..r3, the rest on the guest stack,
pointers are 32-bit guest virtual addresses.  The old runtime
(`src/gxm/sce_gxm.*`, compiled into `C:\dev\tmp\recomp_split\gxm_*.o`) is plain
host C++ code whose prototypes are *not* always laid out like the PSV ABI (e.g.
`sceGxmDraw` and `sceGxmSetViewport` reorder float/pointer arguments, and
`sceGxmBeginScene` has 8 PSV arguments vs 7 runtime arguments).  The generated
wrappers decode the guest ABI and re-assemble the call for the runtime backend.

INPUTS
------
  C:\\dev\\p5dsrc\\_gxm_table.csv
      NID,Name,Stub,Native,Calls -- the 100 GXM stubs imported by the game's
      eboot (stub VAs 0x812B11C0..0x812B17F0).  This is the mapping table.
  C:\\dev\\p5dsrc\\src\\gxm\\sce_gxm.h   (+ sce_gxm.cpp)
      The old runtime / backend.  `sce_gxm.h` is authoritative for *how to call
      the backend*; `sce_gxm.cpp` is scanned to classify placeholder `(void)`
      functions as STUB_LOG-only or as having a real body.
  C:\\dev\\p5dsrc\\refs\\vita-headers\\include\\psp2\\gxm.h
      The guest (PSV) prototypes.  This is authoritative for *decoding the
      guest registers*.  If this file is missing, typed prototypes from
      `sce_gxm.h` are used as a fallback (reported as `fallback`).
  MANUAL_ARG_MAP / RAW_PTR_ARGS below
      Small hand-written override tables.  All overrides are reported with
      their count so the audit trail stays explicit.

OUTPUT (the only files this tool writes)
----------------------------------------
  <out-dir>/gxm_abi.cpp
  <out-dir>/gxm_abi.h
  default out-dir: C:\\dev\\tmp\\recomp_split   (override: --out-dir / env
  P5D_GXM_ABI_OUT)

ABI CONTRACT OF THE GENERATED CODE
----------------------------------
  uint32_t gxm_abi_call_<NIDHEX>(uint32_t* regs, uint32_t* stack_args);
      regs[0..3]  = guest r0..r3 (i.e. RecState::r[0..3])
      stack_args  = host pointer to the guest stack words that hold argument
                    5 and up: stack_args[0] is the 5th argument (guest [sp+0]).
                    May be NULL when the function takes <= 4 arguments.
      return      = value to put into guest r0.

  Pointers are translated with the optional weak host hooks (checked in this
  order, all may be left undefined -- the generated file ships weak defaults):
      uint64_t p5d_abi_translate64(uint32_t guest_va)   <- preferred on x86_64
      uint32_t p5d_abi_translate (uint32_t guest_va)    <- requested 32-bit hook
      uint32_t p5d_abi_untranslate(const void* host)    <- reverse, for handles
  A weak default returning 0 is emitted for every hook, so the file always
  links even when the host does not implement translation (all translated
  pointers then become NULL, which is safe for the current stub backend).

  A handful of parameters are semantically guest VAs (not host pointers) for
  this runtime and are therefore passed through untranslated (RAW_PTR_ARGS).

USAGE
-----
  py -3 tools/gxm_abi_gen.py                       # generate into default dir
  py -3 tools/gxm_abi_gen.py --out-dir C:\\dev\\tmp\\recomp_split
  py -3 tools/gxm_abi_gen.py --call-stub-impls     # make stub wrappers call the
                                                   # runtime STUB_LOG body too
The script prints a coverage report (mapped / typed / stub / signature-failure
counts) and exits non-zero only on a hard error (bad CSV / unwritable output).
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# paths
# ---------------------------------------------------------------------------
SCRIPT_DIR = Path(__file__).resolve().parent
ROOT = SCRIPT_DIR.parent                      # C:\dev\p5dsrc
CSV_PATH = ROOT / "_gxm_table.csv"
BACKEND_H = ROOT / "src" / "gxm" / "sce_gxm.h"
BACKEND_CPP = ROOT / "src" / "gxm" / "sce_gxm.cpp"
GUEST_H = ROOT / "refs" / "vita-headers" / "include" / "psp2" / "gxm.h"
DEFAULT_OUT_DIR = Path(os.environ.get("P5D_GXM_ABI_OUT", r"C:\dev\tmp\recomp_split"))

# ---------------------------------------------------------------------------
# manual overrides (fully reported by the generator)
# ---------------------------------------------------------------------------
# backend-parameter -> guest-argument-index maps, needed where the old runtime
# prototype is NOT in PSV ABI order (or has a different arity).
#   sceGxmBeginScene  guest: (ctx, flags, rt, validRegion, vsync, fsync, color, depth)  [8]
#                     runtime: (ctx, flags, rt, color, depth, vsync, fsync)             [7]
#   sceGxmDraw        guest: (ctx, primType, indexType, indexData, indexCount)
#                     runtime: (ctx, mode, indexType, count, indices)
#   sceGxmSetViewport guest: (ctx, xOffset, xScale, yOffset, yScale, zOffset, zScale)
#                     runtime: (ctx, xOffset, yOffset, xScale, yScale, zScale, zOffset)
MANUAL_ARG_MAP = {
    "sceGxmBeginScene": [0, 1, 2, 6, 7, 4, 5],
    "sceGxmDraw": [0, 1, 2, 4, 3],
    "sceGxmSetViewport": [0, 1, 3, 2, 4, 6, 5],
}
MANUAL_ARG_MAP_NOTE = {
    "sceGxmBeginScene": "8->7 args: drops validRegion, rt/color/depth/vsync/fsync reorder",
    "sceGxmDraw": "indexData/indexCount swapped vs runtime (count, indices)",
    "sceGxmSetViewport": "x/y scale/offset order differs from runtime",
}

# guest parameters that must be handed to the backend as RAW guest VAs (the
# runtime stores/uses the 32-bit VA itself, it must not be host-translated).
RAW_PTR_ARGS = {
    ("sceGxmMapMemory", "base"),                  # backend casts it back to uint32 vaddr
    ("sceGxmMapFragmentUsseMemory", "base"),      # ditto semantics
    ("sceGxmMapVertexUsseMemory", "base"),
    ("sceGxmTextureInitLinear", "data"),          # stored as data_byte_addr in the 16B control block
    ("sceGxmTextureInitSwizzled", "data"),
    ("sceGxmTextureInitTiled", "data"),
}

# ---------------------------------------------------------------------------
# small helpers
# ---------------------------------------------------------------------------
WEAK_BEGIN = '__attribute__((weak))'


def die(msg: str, code: int = 2):
    sys.stderr.write("gxm_abi_gen: ERROR: %s\n" % msg)
    raise SystemExit(code)


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        die("cannot read %s: %s" % (path, exc))


def norm_ws(s: str) -> str:
    return " ".join(s.split())


def strip_param_name(param: str):
    """Split 'const SceGxmProgram *program' -> ('const SceGxmProgram *', 'program')."""
    p = param.strip()
    if not p:
        return None
    m = re.match(r"^(.*?)([A-Za-z_]\w*)\s*$", p, re.S)
    if not m:
        return (p, "")
    typ = norm_ws(m.group(1))
    name = m.group(2)
    if not typ:  # e.g. a bare 'void'
        return (p, "")
    return (typ, name)


def split_top_level(params: str):
    """Split a C parameter list on top-level commas."""
    out, depth, cur = [], 0, []
    for ch in params:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth = max(0, depth - 1)
        if ch == "," and depth == 0:
            out.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    tail = "".join(cur)
    out.append(tail)
    return [s for s in (x.strip() for x in out) if s and s.strip() not in ("void",)]


TYPEDEF_RE = re.compile(r"\btypedef\s+([A-Za-z_][^;{}]*?)\s+([A-Za-z_]\w*)\s*;", re.S)


def parse_typedefs(text: str) -> dict:
    """name -> base type spelling for simple `typedef <type> <name>;` lines."""
    out = {}
    for m in TYPEDEF_RE.finditer(text):
        base = norm_ws(m.group(1))
        name = m.group(2)
        if not base or "(" in base or ")" in base:
            continue  # function-pointer typedefs / struct bodies
        out[name] = base
    return out


def classify_type(t: str, typedefs: dict) -> str:
    """ABI class of a C type spelling: void / ptr / float / f64 / int.

    Typedefs are resolved so that e.g. the runtime's `SceGxmContext` (which is
    `typedef void* SceGxmContext;`) is classified as a pointer even though the
    spelling has no '*'.
    """
    s = norm_ws(t)
    if not s:
        return "void"
    if s == "void":
        return "void"
    if "*" in s:
        return "ptr"
    # resolve bare typedef names transitively (depth-limited)
    seen = set()
    cur = s
    for _ in range(8):
        cur_n = norm_ws(cur)
        if cur_n in seen:
            break
        seen.add(cur_n)
        base = typedefs.get(cur_n)
        if base is None:
            break
        if "*" in base:
            return "ptr"
        cur = base
    s = norm_ws(cur)
    if re.search(r"\b(float|SceFloat)\b", s):
        return "float"
    if re.search(r"\b(double|SceDouble)\b", s):
        return "f64"
    return "int"


class Param:
    __slots__ = ("type", "name", "cls")

    def __init__(self, ptype: str, name: str, typedefs: dict):
        self.type = ptype
        self.name = name
        self.cls = classify_type(ptype, typedefs)

    def __repr__(self):
        return "Param(%r,%r,%s)" % (self.type, self.name, self.cls)


class Func:
    __slots__ = ("name", "ret", "ret_cls", "params")

    def __init__(self, name: str, ret: str, params_raw: str, typedefs=None):
        typedefs = typedefs or {}
        self.name = name
        self.ret = norm_ws(ret)
        self.ret_cls = classify_type(ret, typedefs)
        parts = split_top_level(params_raw)
        self.params = []
        for p in parts:
            got = strip_param_name(p)
            if got is None:
                continue
            self.params.append(Param(got[0], got[1], typedefs))

    def signature(self) -> str:
        ps = ", ".join((p.type + (" " + p.name if p.name else "")).strip() for p in self.params)
        return "%s %s(%s)" % (self.ret, self.name, ps if ps else "void")

    def __repr__(self):
        return "Func(%r)" % (self.signature(),)


# ---------------------------------------------------------------------------
# parsers
# ---------------------------------------------------------------------------
# A declaration is '<ret> name ( params ) ;'.  The return type is restricted to
# identifier/space/'*' characters so that comments / directives cannot leak in.
DECL_RE = re.compile(
    r"([A-Za-z_][A-Za-z0-9_\s\*]*?)\b((?:sceGxm|_sceGxm)\w+)\s*\(([^;{}]*?)\)\s*;",
    re.S,
)


def parse_decls(text: str, typedefs=None) -> dict:
    typedefs = typedefs or {}
    out = {}
    for m in DECL_RE.finditer(text):
        ret, name, params = m.group(1), m.group(2), m.group(3)
        ret = norm_ws(ret)
        # guard against a stray statement before the declaration being swallowed
        if not ret or any(tok in ret for tok in (";", "{", "}", "=", "#")):
            continue
        out[name] = Func(name, ret, params, typedefs)
    return out


CPP_DEF_RE = re.compile(
    r'extern\s+"C"\s+([A-Za-z_][A-Za-z0-9_\s\*]*?)\b((?:sceGxm|_sceGxm)\w+)\s*\(([^;{}]*?)\)\s*\{',
    re.S,
)


def classify_backend_bodies(text: str) -> dict:
    """name -> 'impl' | 'stub' (STUB_LOG-only body) for extern "C" definitions."""
    out = {}
    for m in CPP_DEF_RE.finditer(text):
        name = m.group(2)
        end = text.find("\n}", m.end())
        body = text[m.end(): end + 2 if end >= 0 else m.end() + 400]
        head = body.lstrip()
        if head.startswith("STUB_LOG"):
            out[name] = "stub"
        else:
            out[name] = "impl"
    return out


# ---------------------------------------------------------------------------
# generation
# ---------------------------------------------------------------------------
CPP_HEADER = """\
// AUTO-GENERATED by C:\\dev\\p5dsrc\\tools\\gxm_abi_gen.py -- DO NOT EDIT BY HAND.
// Regenerate:  py -3 C:\\dev\\p5dsrc\\tools\\gxm_abi_gen.py
//
// SceGxm HLE ABI wrapper layer: guest ARM/AAPCS calls -> old runtime backend.
//
// Coverage (see the generator report for the authoritative numbers):
//   - @@MAPPED@@/@@CSV_TOTAL@@ imported GXM stubs from _gxm_table.csv are mapped
//   - @@TYPED@@ decode the guest ABI and call a typed runtime implementation
//   - @@STUB@@ target runtime functions that only exist as `(void)` STUB_LOG
//     placeholders; by default they return 0 without calling the backend
//     (build with -DGXM_ABI_CALL_STUB_IMPLS=1 to invoke the STUB_LOG bodies)
//   - @@REALBODY@@ placeholder with a real body is always called
//   - @@MANUAL@@ argument-order remaps (see MANUAL_ARG_MAP in the generator)
//   - @@RAW@@ guest-VA parameters deliberately passed untranslated (RAW_PTR_ARGS)
//
// Host translation hooks (weak symbols; strong definitions in the harness win):
//   uint64_t p5d_abi_translate64(uint32_t guest_va);   // preferred on x86_64
//   uint32_t p5d_abi_translate  (uint32_t guest_va);   // requested 32-bit hook
//   uint32_t p5d_abi_untranslate(const void* host_ptr);
// Weak defaults in this file return 0, so translated pointers become NULL when
// the harness does not implement the hooks yet.
//
// On x86_64 the guest image usually lives above 4 GiB: implement
// p5d_abi_translate64 (64-bit host pointer).  The 32-bit p5d_abi_translate
// cannot carry such a pointer width and must not be used alone.
"""

CPP_HOOKS = """\
#include "gxm_abi.h"

#include <cstdint>
#include <cstring>

#ifndef GXM_ABI_CALL_STUB_IMPLS
#define GXM_ABI_CALL_STUB_IMPLS @@CALL_STUBS@@
#endif

// ---------------------------------------------------------------------------
// optional host translation hooks (weak: a strong definition elsewhere wins)
// ---------------------------------------------------------------------------
#ifdef __GNUC__
#define GXM_ABI_WEAK_DEF __attribute__((weak))
#else
#define GXM_ABI_WEAK_DEF
#endif

extern "C" {
GXM_ABI_WEAK_DEF uint64_t p5d_abi_translate64(uint32_t guest_va) {
    (void)guest_va;
    return 0u;
}
GXM_ABI_WEAK_DEF uint32_t p5d_abi_translate(uint32_t guest_va) {
    (void)guest_va;
    return 0u;
}
GXM_ABI_WEAK_DEF uint32_t p5d_abi_untranslate(const void* host_ptr) {
    (void)host_ptr;
    return 0u;
}
}

namespace {

inline float gxm_abi_f32(uint32_t bits) {
    float f;
    std::memcpy(&f, &bits, sizeof(f));
    return f;
}

inline uint32_t gxm_abi_f32_bits(float f) {
    uint32_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    return bits;
}

// guest VA -> host pointer.  Prefer the 64-bit hook; fall back to the 32-bit
// hook for 32-bit hosts / <=4GiB mappings.  Unmapped => NULL (safe).
inline void* gxm_abi_xlate(uint32_t guest_va) {
    if (guest_va == 0u) return nullptr;
    uint64_t host = p5d_abi_translate64(guest_va);
    if (!host) host = (uint64_t)p5d_abi_translate(guest_va);
    return host ? reinterpret_cast<void*>(static_cast<uintptr_t>(host)) : nullptr;
}

inline uint32_t gxm_abi_untranslate_ptr(const void* host_ptr) {
    if (!host_ptr) return 0u;
    return p5d_abi_untranslate(host_ptr);
}

}  // namespace

// ---------------------------------------------------------------------------
// runtime backend prototypes (declared here rather than including sce_gxm.h so
// that this translation unit has no dependency on the runtime's own headers)
// ---------------------------------------------------------------------------
extern "C" {
@@BACKEND_DECLS@@
}

// ---------------------------------------------------------------------------
// per-stub ABI wrappers
// ---------------------------------------------------------------------------
"""


def backend_decl(fn: Func) -> str:
    ret = {"void": "void", "ptr": "void*", "f64": "double"}.get(fn.ret_cls, "int32_t")
    if fn.ret_cls in ("int", "float"):
        ret = "int32_t" if fn.ret_cls == "int" else "float"
    args = []
    for p in fn.params:
        if p.cls == "ptr":
            args.append("void*")
        elif p.cls == "float":
            args.append("float")
        elif p.cls in ("int",):
            args.append("uint32_t")
        elif p.cls == "f64":
            args.append("double")
        else:
            args.append("uint32_t")
    return "extern \"C\" %s %s(%s);" % (ret, fn.name, ", ".join(args) if args else "void")


def gen_backend_decls(backend: dict, used_names: list) -> str:
    seen, lines = set(), []
    for name in used_names:
        if name in seen or name not in backend:
            continue
        seen.add(name)
        lines.append(backend_decl(backend[name]))
    return "\n".join("    " + l for l in lines)


def gen_wrapper_typed(rec: dict, guest: Func, backend: Func, mapping: list, mode: str) -> str:
    """Decode guest arguments and forward them to a typed runtime function."""
    name = rec["name"]
    nid = rec["nid"]
    local = "gxm_abi_call_" + nid
    out = []
    out.append("extern \"C\" uint32_t %s(uint32_t* regs, uint32_t* stack_args) {" % local)
    out.append("    // guest ABI:   %s" % guest.signature())
    out.append("    // backend ABI: %s" % backend.signature())
    note = MANUAL_ARG_MAP_NOTE.get(name)
    out.append("    // arg mapping: %s%s" % (mode, (" -- " + note) if note else ""))
    out.append("    (void)regs;")
    out.append("    (void)stack_args;")

    used = sorted(set(mapping))
    for gi in used:
        src = ("regs[%du]" % gi) if gi < 4 else ("stack_args[%du]" % (gi - 4))
        cond = ("regs ? %s : 0u" % src) if gi < 4 else ("stack_args ? %s : 0u" % src)
        out.append("    const uint32_t a%d = %s;   // %s" %
                   (gi, cond, guest.params[gi].type if gi < len(guest.params) else "?"))

    call_args = []
    prep = []
    fixups = []
    for bi, gi in enumerate(mapping):
        b = backend.params[bi]
        g = guest.params[gi]
        key = (name, g.name)
        raw = key in RAW_PTR_ARGS
        out_handle = ("*" in g.type and g.type.count("*") >= 2 and b.cls == "ptr")
        if out_handle:
            var = "out%d" % bi
            prep.append("    void* %s = nullptr;   // host-side out handle, converted back below" % var)
            call_args.append("&%s" % var)
            fixups.append((bi, gi, var))
        elif b.cls == "ptr":
            var = "p%d" % gi
            if raw:
                prep.append("    void* %s = reinterpret_cast<void*>(static_cast<uintptr_t>(a%d));   // RAW guest VA" % (var, gi))
            else:
                prep.append("    void* %s = gxm_abi_xlate(a%d);   // %s" % (var, gi, g.type))
            call_args.append(var)
        elif b.cls == "float":
            var = "f%d" % gi
            prep.append("    const float %s = gxm_abi_f32(a%d);" % (var, gi))
            call_args.append(var)
        else:  # scalar
            call_args.append("(uint32_t)a%d" % gi)

    out.extend(prep)
    fname = backend.name
    ret_expr = ""
    if backend.ret_cls == "void":
        out.append("    %s(%s);" % (fname, ", ".join(call_args)))
        ret_expr = "0u"
    elif backend.ret_cls == "ptr":
        out.append("    void* rv = %s(%s);" % (fname, ", ".join(call_args)))
        ret_expr = "gxm_abi_untranslate_ptr(rv)"
    elif backend.ret_cls == "float":
        out.append("    float rv = %s(%s);" % (fname, ", ".join(call_args)))
        ret_expr = "gxm_abi_f32_bits(rv)"
    elif backend.ret_cls == "f64":
        out.append("    double rv = %s(%s);" % (fname, ", ".join(call_args)))
        out.append("    return static_cast<uint32_t>(static_cast<uint64_t>(rv));")
        ret_expr = None
    else:
        out.append("    int32_t rv = %s(%s);" % (fname, ", ".join(call_args)))
        ret_expr = "(uint32_t)rv"

    # out-handle fix-up: backend wrote a host pointer into the host temporary;
    # convert it back to a 32-bit guest handle and store it into guest memory.
    for bi, gi, var in fixups:
        out.append("    if (%s) {" % var)
        out.append("        void* slot = gxm_abi_xlate(a%d);" % gi)
        out.append("        if (slot) {")
        out.append("            uint32_t guest_handle = gxm_abi_untranslate_ptr(%s);" % var)
        out.append("            std::memcpy(slot, &guest_handle, sizeof(guest_handle));")
        out.append("        }")
        out.append("    }")
    if ret_expr is not None:
        out.append("    return %s;" % ret_expr)
    out.append("}")
    return "\n".join(out)


def gen_wrapper_stub(rec: dict, guest: Func, backend: Func, kind: str, call_always: bool) -> str:
    """Backend only exists as a `(void)` placeholder; report 0 (or call it)."""
    name, nid = rec["name"], rec["nid"]
    local = "gxm_abi_call_" + nid
    body = []
    body.append("extern \"C\" uint32_t %s(uint32_t* regs, uint32_t* stack_args) {" % local)
    body.append("    // guest ABI:   %s" % guest.signature())
    body.append("    // backend ABI: %s   [runtime has a `(void)` placeholder%s]" %
                (backend.signature(), ", real body" if kind == "realbody" else " / STUB_LOG only"))
    body.append("    (void)regs;")
    body.append("    (void)stack_args;")
    if kind == "realbody" or call_always:
        body.append("    return (uint32_t)%s();" % name)
    else:
        body.append("#if GXM_ABI_CALL_STUB_IMPLS")
        body.append("    return (uint32_t)%s();   // logs [STUB] via STUB_LOG" % name)
        body.append("#else")
        body.append("    return 0u;   // same observable value as the current stub")
        body.append("#endif")
    body.append("}")
    return "\n".join(body)


def gen_dispatch(entries: list) -> str:
    n = len(entries)
    lines = []
    lines.append("// ---------------------------------------------------------------------------")
    lines.append("// stub-address dispatch table (sorted by stub VA)")
    lines.append("// ---------------------------------------------------------------------------")
    lines.append("extern \"C\" {")
    lines.append("const gxm_abi_entry g_gxm_abi_table[GXM_ABI_ENTRY_COUNT] = {")
    for e in entries:
        lines.append("    {0x%08Xu, 0x%08Xu, \"%s\", gxm_abi_call_%s, %du, %du}," % (
            e["nid_int"], e["stub_int"], e["name"], e["nid"], e["nargs"], e["kind_int"]))
    lines.append("};")
    lines.append("const unsigned g_gxm_abi_count = %du;" % n)
    lines.append("")
    lines.append("const gxm_abi_entry* gxm_abi_lookup(uint32_t stub_va) {")
    lines.append("    unsigned lo = 0u, hi = GXM_ABI_ENTRY_COUNT;")
    lines.append("    while (lo < hi) {")
    lines.append("        const unsigned mid = lo + (hi - lo) / 2u;")
    lines.append("        const gxm_abi_entry* e = &g_gxm_abi_table[mid];")
    lines.append("        if (e->stub == stub_va) return e;")
    lines.append("        if (e->stub < stub_va) lo = mid + 1u; else hi = mid;")
    lines.append("    }")
    lines.append("    return nullptr;")
    lines.append("}")
    lines.append("")
    lines.append("uint32_t gxm_abi_dispatch(uint32_t stub_va, uint32_t* regs, uint32_t* stack_args) {")
    lines.append("    const gxm_abi_entry* e = gxm_abi_lookup(stub_va);")
    lines.append("    if (e == nullptr) return 0u;   // unmapped stub")
    lines.append("    return e->fn(regs, stack_args);")
    lines.append("}")
    lines.append("}  // extern \"C\"")
    return "\n".join(lines)


def gen_header(entries: list, stats: dict) -> str:
    n = len(entries)
    lines = []
    lines.append("// AUTO-GENERATED by C:\\dev\\p5dsrc\\tools\\gxm_abi_gen.py -- DO NOT EDIT BY HAND.")
    lines.append("// Regenerate:  py -3 C:\\dev\\p5dsrc\\tools\\gxm_abi_gen.py")
    lines.append("//")
    lines.append("// SceGxm HLE ABI wrapper layer for the P5D static-AOT harness.")
    lines.append("//")
    lines.append("// Each imported GXM stub (0x812B11C0 .. 0x812B17F0) has one entry point")
    lines.append("// that decodes the PSV ARM/AAPCS argument registers and calls the old")
    lines.append("// runtime GXM->Vulkan backend:")
    lines.append("//")
    lines.append("//     uint32_t gxm_abi_call_<NID>(uint32_t* regs, uint32_t* stack_args)")
    lines.append("//       regs[0..3]  guest r0..r3 (RecState::r[0..3])")
    lines.append("//       stack_args  host pointer to guest stack words (arg5 at [0]); NULL ok")
    lines.append("//       return      value for guest r0")
    lines.append("//")
    lines.append("// Dispatch helpers:")
    lines.append("//     const gxm_abi_entry* gxm_abi_lookup(uint32_t stub_va);   // NULL if unmapped")
    lines.append("//     uint32_t gxm_abi_dispatch(uint32_t stub_va, uint32_t* regs, uint32_t* stack_args);")
    lines.append("//")
    lines.append("// Pointers are translated through optional weak host hooks (strong definitions")
    lines.append("// in the harness override the weak defaults in gxm_abi.cpp):")
    lines.append("//     uint64_t p5d_abi_translate64(uint32_t guest_va);   // preferred on x86_64")
    lines.append("//     uint32_t p5d_abi_translate  (uint32_t guest_va);   // requested 32-bit hook")
    lines.append("//     uint32_t p5d_abi_untranslate(const void* host_ptr);")
    lines.append("// Without them, translated pointers are NULL (safe no-op behaviour).")
    lines.append("//")
    lines.append("// Coverage: %d/%d imported stubs mapped (%d typed runtime calls, %d runtime" %
                 (stats["mapped"], stats["csv_total"], stats["typed"], stats["stub"]))
    lines.append("// `(void)` placeholders, %d of them with a real body; %d manual arg remaps)." %
                 (stats["realbody"], stats["manual"]))
    lines.append("")
    lines.append("#pragma once")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append("extern \"C\" {")
    lines.append("#endif")
    lines.append("")
    lines.append("typedef uint32_t (*gxm_abi_fn)(uint32_t* regs, uint32_t* stack_args);")
    lines.append("")
    lines.append("typedef struct gxm_abi_entry {")
    lines.append("    uint32_t    nid;    /* NID from _gxm_table.csv */")
    lines.append("    uint32_t    stub;   /* guest stub VA (0x812Bxxxx) */")
    lines.append("    const char* name;   /* sceGxm* entry name */")
    lines.append("    gxm_abi_fn  fn;     /* ABI wrapper */")
    lines.append("    uint16_t    nargs;  /* guest argument count (regs + stack) */")
    lines.append("    uint8_t     kind;   /* 0=typed runtime call, 1=STUB_LOG placeholder,")
    lines.append("                           2=placeholder with real body */")
    lines.append("} gxm_abi_entry;")
    lines.append("")
    lines.append("#define GXM_ABI_ENTRY_COUNT %du" % n)
    lines.append("")
    lines.append("extern const gxm_abi_entry g_gxm_abi_table[GXM_ABI_ENTRY_COUNT];")
    lines.append("extern const unsigned g_gxm_abi_count;")
    lines.append("")
    lines.append("const gxm_abi_entry* gxm_abi_lookup(uint32_t stub_va);")
    lines.append("uint32_t gxm_abi_dispatch(uint32_t stub_va, uint32_t* regs, uint32_t* stack_args);")
    lines.append("")
    lines.append("/* --- per-NID ABI wrappers --- */")
    for e in sorted(entries, key=lambda x: x["stub_int"]):
        lines.append("uint32_t gxm_abi_call_%s(uint32_t* regs, uint32_t* stack_args);" % e["nid"])
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append("}")
    lines.append("#endif")
    lines.append("")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
def load_csv() -> list:
    if not CSV_PATH.is_file():
        die("mapping table not found: %s" % CSV_PATH)
    with CSV_PATH.open("r", encoding="utf-8-sig", newline="") as fh:
        rows = list(csv.DictReader(fh))
    if not rows:
        die("empty mapping table: %s" % CSV_PATH)
    recs = []
    seen_stub, seen_nid = {}, {}
    for row in rows:
        name = (row.get("Name") or "").strip()
        nid = (row.get("NID") or "").strip().upper()
        stub = (row.get("Stub") or "").strip()
        if not name or not nid or not stub:
            die("bad CSV row: %r" % (row,))
        nid = nid[2:] if nid.lower().startswith("0x") else nid
        if not re.fullmatch(r"[0-9A-F]{8}", nid):
            die("bad NID %r for %s" % (nid, name))
        stub = stub[2:] if stub.lower().startswith("0x") else stub
        if not re.fullmatch(r"[0-9A-Fa-f]{8}", stub):
            die("bad Stub %r for %s" % (stub, name))
        stub_int = int(stub, 16)
        nid_int = int(nid, 16)
        if stub_int in seen_stub:
            die("duplicate stub 0x%08X: %s / %s" % (stub_int, seen_stub[stub_int], name))
        if nid_int in seen_nid:
            die("duplicate NID 0x%08X: %s / %s" % (nid_int, seen_nid[nid_int], name))
        seen_stub[stub_int] = name
        seen_nid[nid_int] = name
        recs.append({
            "name": name,
            "nid": nid,
            "nid_int": nid_int,
            "stub": "0x" + stub.upper().zfill(8),
            "stub_int": stub_int,
            "native": (row.get("Native") or "").strip(),
            "calls": (row.get("Calls") or "").strip(),
        })
    return recs


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="Generate SceGxm ABI wrappers for the P5D AOT HLE layer.")
    ap.add_argument("--out-dir", default=str(DEFAULT_OUT_DIR), help="output directory (default: %(default)s)")
    ap.add_argument("--call-stub-impls", action="store_true",
                    help="generate wrappers that always call STUB_LOG-only runtime placeholders")
    ap.add_argument("--verbose", action="store_true", help="print the per-function mapping")
    args = ap.parse_args(argv)

    out_dir = Path(args.out_dir)
    out_cpp = out_dir / "gxm_abi.cpp"
    out_h = out_dir / "gxm_abi.h"

    if not BACKEND_H.is_file():
        die("runtime header not found: %s" % BACKEND_H)
    if not GUEST_H.is_file():
        sys.stderr.write("gxm_abi_gen: WARNING: guest header not found (%s); "
                         "falling back to sce_gxm.h typed prototypes\n" % GUEST_H)

    csv_recs = load_csv()
    backend_text = read_text(BACKEND_H)
    backend_typedefs = parse_typedefs(backend_text)
    backend = parse_decls(backend_text, backend_typedefs)
    if GUEST_H.is_file():
        guest_text = read_text(GUEST_H)
        guest = parse_decls(guest_text, parse_typedefs(guest_text))
    else:
        guest = {}
    bodies = classify_backend_bodies(read_text(BACKEND_CPP)) if BACKEND_CPP.is_file() else {}

    stats = {
        "csv_total": len(csv_recs),
        "mapped": 0,
        "typed": 0,
        "stub": 0,
        "realbody": 0,
        "manual": 0,
        "quoted_guest": 0,
        "fallback_guest": 0,
        "raw": 0,
        "stack_funcs": 0,
        "guest_src_gxm_h": 0,
    }
    failures = []
    entries = []

    for rec in csv_recs:
        name = rec["name"]
        bfn = backend.get(name)
        if bfn is None:
            failures.append((name, "backend declaration missing in sce_gxm.h"))
            continue
        gfn = guest.get(name)
        if gfn is None and len(bfn.params) > 0:
            # fallback: the runtime's own typed prototype (only valid for the
            # typed subset; these are exactly the functions whose ABI the
            # runtime already matches or that the manual table handles)
            alt = Func(name, bfn.ret,
                       ", ".join((p.type + " " + p.name).strip() for p in bfn.params),
                       backend_typedefs)
            if alt.params:
                gfn = alt
                stats["fallback_guest"] += 1
        if gfn is None:
            failures.append((name, "guest ABI prototype missing (no vita-headers entry)"))
            continue
        stats["guest_src_gxm_h"] += 1

        backend_placeholder = len(bfn.params) == 0
        nargs = len(gfn.params)
        rec_entry = dict(rec)
        rec_entry["nargs"] = nargs

        if backend_placeholder:
            kind = bodies.get(name, "stub")
            if kind == "impl":
                stats["realbody"] += 1
                kind_int = 2
            else:
                stats["stub"] += 1
                kind_int = 1
            body = gen_wrapper_stub(rec, gfn, bfn, kind, args.call_stub_impls)
            rec_entry["kind_int"] = kind_int
            stats["mapped"] += 1
            if nargs > 4:
                stats["stack_funcs"] += 1
            entries.append(rec_entry)
            if args.verbose:
                print("  [stub]  %-46s nid=%s nargs=%d kind=%s" %
                      (name, rec["nid"], nargs, kind))
            continue

        # typed runtime implementation: build the guest -> backend argument map
        mode = "positional"
        mapping = MANUAL_ARG_MAP.get(name)
        if mapping is not None:
            mode = "manual"
            stats["manual"] += 1
        else:
            if len(gfn.params) != len(bfn.params):
                failures.append((name, "arity mismatch: guest=%d backend=%d" %
                                 (len(gfn.params), len(bfn.params))))
                continue
            mapping = list(range(len(bfn.params)))
        ok = True
        for bi, gi in enumerate(mapping):
            if gi >= len(gfn.params) or bi >= len(bfn.params):
                failures.append((name, "manual arg map out of range"))
                ok = False
                break
            bcls, gcls = bfn.params[bi].cls, gfn.params[gi].cls
            if bcls != gcls:
                failures.append((name, "arg %d class mismatch: guest=%s backend=%s" %
                                 (gi, gcls, bcls)))
                ok = False
                break
        if not ok:
            continue
        # note: it is fine for positional order to differ from PSV order only when
        # lengths match and class sequence matches; reordering is handled by
        # MANUAL_ARG_MAP above.
        for gi in mapping:
            g = gfn.params[gi]
            if ((name, g.name) in RAW_PTR_ARGS) and "*" in g.type:
                stats["raw"] += 1
        body = gen_wrapper_typed(rec, gfn, bfn, mapping, mode)
        rec_entry["kind_int"] = 0
        stats["mapped"] += 1
        stats["typed"] += 1
        if nargs > 4:
            stats["stack_funcs"] += 1
        entries.append(rec_entry)
        if args.verbose:
            print("  [typed] %-46s nid=%s nargs=%d mode=%s" %
                  (name, rec["nid"], nargs, mode))

    if not entries:
        die("no wrappers generated -- refusing to write empty ABI layer")

    entries.sort(key=lambda e: e["stub_int"])
    stats["mapped"] = len(entries)

    # ---- assemble .cpp ----
    header_txt = CPP_HEADER
    for token, value in (
            ("@@MAPPED@@", str(stats["mapped"])),
            ("@@CSV_TOTAL@@", str(stats["csv_total"])),
            ("@@TYPED@@", str(stats["typed"])),
            ("@@STUB@@", str(stats["stub"])),
            ("@@REALBODY@@", str(stats["realbody"])),
            ("@@MANUAL@@", str(stats["manual"])),
            ("@@RAW@@", str(stats["raw"]))):
        header_txt = header_txt.replace(token, value)
    hooks_txt = CPP_HOOKS.replace(
        "@@CALL_STUBS@@", "1" if args.call_stub_impls else "0").replace(
        "@@BACKEND_DECLS@@", gen_backend_decls(backend, [e["name"] for e in entries]))
    cpp = [header_txt, hooks_txt]
    bodies_out = []
    for e in entries:
        name = e["name"]
        bfn = backend[name]
        gfn = guest.get(name)
        if gfn is None:
            gfn = Func(name, bfn.ret,
                       ", ".join((p.type + " " + p.name).strip() for p in bfn.params),
                       backend_typedefs)
        if e["kind_int"] == 0:
            mapping = MANUAL_ARG_MAP.get(name) or list(range(len(bfn.params)))
            bodies_out.append(gen_wrapper_typed(e, gfn, bfn, mapping,
                                                "manual" if name in MANUAL_ARG_MAP else "positional"))
        else:
            kind = "realbody" if e["kind_int"] == 2 else "stub"
            bodies_out.append(gen_wrapper_stub(e, gfn, bfn, kind, args.call_stub_impls))
    cpp.append("\n\n".join(bodies_out))
    cpp.append("\n\n" + gen_dispatch(entries) + "\n")
    cpp_text = "\n".join(cpp)

    header_text = gen_header(entries, stats)

    try:
        out_dir.mkdir(parents=True, exist_ok=True)
        out_cpp.write_text(cpp_text, encoding="utf-8", newline="\n")
        out_h.write_text(header_text, encoding="utf-8", newline="\n")
    except OSError as exc:
        die("cannot write output: %s" % exc)

    # ---- report ----
    typed_decls = sum(1 for f in backend.values() if len(f.params) > 0)
    guest_typed = sum(1 for f in guest.values() if len(f.params) > 0) if guest else 0
    print("=== gxm_abi_gen report ===")
    print("  csv            : %s (%d rows, unique NID+stub enforced)" %
          (CSV_PATH, stats["csv_total"]))
    print("  runtime header : %s (%d declarations, %d typed, %d `(void)`)" %
          (BACKEND_H, len(backend), typed_decls, len(backend) - typed_decls))
    print("  guest header   : %s (%d declarations, %d typed)%s" % (
        GUEST_H, len(guest), guest_typed,
        "" if GUEST_H.is_file() else "  [MISSING -> fallback used]"))
    print("  wrappers       : %d mapped / %d csv rows (%.0f%%)" %
          (stats["mapped"], stats["csv_total"],
           100.0 * stats["mapped"] / max(1, stats["csv_total"])))
    print("    typed runtime calls : %d" % stats["typed"])
    print("    `(void)` stub calls : %d (default returns 0; %d real-body always called)" %
          (stats["stub"], stats["realbody"]))
    print("    manual arg remaps   : %d  %s" %
          (stats["manual"], sorted(MANUAL_ARG_MAP.keys())))
    print("    raw guest-VA params : %d  %s" %
          (stats["raw"], sorted(RAW_PTR_ARGS)))
    print("    funcs reading stack : %d (nargs > 4)" % stats["stack_funcs"])
    print("    guest ABI source    : vita-headers=%d fallback=%d" %
          (stats["guest_src_gxm_h"], stats["fallback_guest"]))
    print("  signature failures: %d" % len(failures))
    for name, why in failures:
        print("    FAIL %-44s %s" % (name, why))
    print("  output:")
    print("    %s (%d bytes)" % (out_h, out_h.stat().st_size))
    print("    %s (%d bytes)" % (out_cpp, out_cpp.stat().st_size))
    print("  compile check:")
    print("    g++ -std=c++17 -c -O1 -Isrc -Isrc/common -Isrc/loader -Isrc/gxm "
          "-Ithird_party/vulkan/include -DP5D_HAS_VULKAN %s -o %s.o" % (out_cpp, out_cpp))
    if stats["stub"] and not args.call_stub_impls:
        print("  note: %d stub wrappers return 0 by default (no [STUB] log spam); "
              "regenerate with --call-stub-impls or build with -DGXM_ABI_CALL_STUB_IMPLS=1 "
              "to execute the runtime placeholders." % stats["stub"])
    return 0


if __name__ == "__main__":
    sys.exit(main())

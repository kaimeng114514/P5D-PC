// aot_difftest_host.cpp - host template for tools/aot_difftest.py (AOT differential test).
//
// This file is NOT compiled directly. tools/aot_difftest.py substitutes three markers:
//   PREAMBLE marker  -> the production per-batch preamble (RecState + memory/flags helpers),
//                        copied verbatim from C:\dev\tmp\recomp_split\batch_000.cpp, so the test
//                        host uses exactly the same helper semantics as the shipped batches.
//   BLOCKS marker    -> verbatim block function definitions extracted from production batch_*.cpp
//                        (only for the generated-block part of the diff test).
//   CASES marker     -> generated init/run/probe tables (one CaseDef per test case).
//
// The produced executable runs every case in a fresh zero-filled guest memory image and prints
// one RESULT line plus one PROBE line per probed address, in a format the Python driver parses.
// Pure ASCII.

/*@@PREAMBLE@@*/

#include <csetjmp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

// ---- fault capture: production preamble calls harness_badmem() on unmapped guest addresses ----
static jmp_buf g_jmp;
static volatile uint32_t g_badmem_va = 0xFFFFFFFFu;
extern "C" uint32_t g_cur_block = 0;
extern "C" int g_watch_on = 0;
extern "C" void harness_badmem(uint32_t va) { g_badmem_va = va; std::longjmp(g_jmp, 1); }
extern "C" void wr32_watch(uint32_t a, uint32_t v) { (void)a; (void)v; }

// ---- indirect-transfer capture (stubs replace the real dispatcher; the driver only asserts
//      that the generated block computed the right target / link register) ----
static int g_idreg = -1;
static uint32_t g_idregv = 0xFFFFFFFFu;
static uint32_t g_hle_stub = 0xFFFFFFFFu;
extern "C" uint32_t hle_call(uint32_t stub, RecState* s) { g_hle_stub = stub; s->r[0] = 0u; return 0xFFFFFFF0u; }
extern "C" uint32_t id_reg(int idx, RecState* s) { (void)s; g_idreg = idx; return 0xFFFFFFE0u; }
extern "C" uint32_t id_regv(uint32_t tgt, RecState* s) { (void)s; g_idregv = tgt; return 0xFFFFFFE1u; }
extern "C" uint32_t id_tbb(uint32_t pc, uint32_t addr, RecState* s) { (void)pc; (void)addr; (void)s; g_idreg = 100; return 0xFFFFFFE2u; }
extern "C" uint32_t id_tbh(uint32_t pc, uint32_t base, uint32_t index, RecState* s) { (void)pc; (void)base; (void)index; (void)s; g_idreg = 101; return 0xFFFFFFE3u; }
extern "C" uint32_t id_mem(uint32_t addr, RecState* s) { (void)s; g_idregv = addr; return 0xFFFFFFE4u; }

// ---- guest memory image: first 64 MiB of the guest map (0x80600000..0x84600000) ----
static const uint32_t AOT_MEM_SIZE = 0x4000000u;
static uint8_t* g_mem = nullptr;

static uint32_t goff(uint32_t a) {
  if (a >= 0x80600000u && a < 0x84600000u) return a - 0x80600000u;
  return 0xFFFFFFFFu;
}

struct CaseDef {
  const char* name;
  void (*init)(RecState*);
  uint32_t (*run)(RecState*);
  const uint32_t* probe_addr;
  const uint16_t* probe_size;
  unsigned nprobe;
};

/*@@BLOCKS@@*/

/*@@CASES@@*/

int main() {
  g_mem = (uint8_t*)std::calloc(1, AOT_MEM_SIZE);
  if (!g_mem) { std::fprintf(stderr, "HOST: calloc failed\n"); return 2; }
  const unsigned n = (unsigned)(sizeof(g_cases) / sizeof(g_cases[0]));
  for (unsigned ci = 0; ci < n; ++ci) {
    const CaseDef& c = g_cases[ci];
    std::memset(g_mem, 0, AOT_MEM_SIZE);
    RecState st;
    std::memset(&st, 0, sizeof(st));
    st.base = g_mem;
    st.mem_size = AOT_MEM_SIZE;
    seed_mem(&st);
    g_badmem_va = 0xFFFFFFFFu;
    g_idreg = -1;
    g_idregv = 0xFFFFFFFFu;
    g_hle_stub = 0xFFFFFFFFu;
    if (setjmp(g_jmp) != 0) {
      std::printf("RESULT %s BADMEM 0x%08X\n", c.name, (unsigned)g_badmem_va);
      std::fflush(stdout);
      continue;
    }
    c.init(&st);
    uint32_t ret = c.run(&st);
    std::printf("RESULT %s OK 0x%08X 0x%08X %d 0x%08X 0x%08X", c.name, ret, st.apsr, g_idreg, g_idregv, g_hle_stub);
    for (int i = 0; i < 16; ++i) std::printf(" 0x%08X", st.r[i]);
    std::printf("\n");
    for (unsigned pi = 0; pi < c.nprobe; ++pi) {
      uint32_t a = c.probe_addr[pi];
      unsigned sz = c.probe_size[pi];
      uint32_t off = goff(a);
      uint32_t v = 0xFFFFFFFFu;
      if (off != 0xFFFFFFFFu && off + 4u <= AOT_MEM_SIZE) {
        if (sz == 1) v = g_mem[off];
        else if (sz == 2) { uint16_t x = 0; std::memcpy(&x, g_mem + off, 2); v = x; }
        else { std::memcpy(&v, g_mem + off, 4); }
      }
      std::printf("PROBE %s 0x%08X %u 0x%08X\n", c.name, a, sz, v);
    }
    std::fflush(stdout);
  }
  std::free(g_mem);
  std::printf("HOST_DONE %u\n", n);
  return 0;
}

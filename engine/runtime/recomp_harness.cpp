// 18.3648: REAL libc HLE implementations in the harness (the placeholder returning one scratch pointer broke
// memcpy/memset/strlen/strcpy/malloc/free on the boot path). Stub VAs verified against vitasdk yml:
//   0x812B2370 _sce_set_dso_handle_main, 0x812B22A0 malloc, 0x812B2270 memcpy, 0x812B2260 memset,
//   0x812B22E0 strlen, 0x812B22D0 strcpy, 0x812B2250 free, 0x812B2360 __at_quick_exit,
//   0x812B2910 sceKernelCreateLwMutex, 0x812B2830 sceKernelLockLwMutex, 0x812B28A0 sceKernelUnlockLwMutex,
//   0x812B19B0 sceFiosInitialize, 0x812B19C0 sceFiosFHClose, 0x812B1910 sceAppUtilInit, 0x812B2210 abort,
//   0x812B22A0 malloc etc.
#include <cstdio>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <map>
#include <utility>
#include <string>
#include <vector>
static int g_loadclear_env = 0;
static int g_hostthreads = 0;
static volatile int g_shutdown = 0;
static uint32_t g_last_alloc_sz = 0, g_last_alloc_fp = 0;
static int g_allocf_n = 0;
static int g_free2_n = 0;
static int g_big2n = 0;
static int g_vblank_n = 0;
static uint32_t g_fb_pitch = 0;
static std::mutex g_hle_mtx;
static uint32_t g_cur_hle_stub = 0;   // 18.4239: guest threads as real host threads (P5D_HOSTTHREADS=1)
static int g_ht_n = 0;
static std::mutex g_lwm_mtx;
static std::map<uint32_t, std::pair<uint32_t,uint32_t> > g_lwm_real;   // work -> (owner_tid, count)
static thread_local uint32_t tl_ht_tid = 0;
static thread_local uint32_t tl_pc_ring[64];
static thread_local unsigned tl_pc_n = 0;
static void guest_thread_run(uint32_t tid);
static int g_ht_bad_n = 0;
static int g_task_n = 0;
static int g_sem2_n = 0, g_sig2_n = 0, g_evt2_n = 0;
static int g_thrst_n = 0;
static int g_thrtrace = 0;   // 18.4232 trace off by default (P5D_THRTRACE=1 enables)
static int g_thrtr_n = 0;
static int g_thrpump_env = 0;
static int g_tidp_n = 0;   // 18.4228c
static int g_current_loop_pc = 0;   // 18.4226 watchdog: current dispatch pc
#include <chrono>
#include "gxm_vulkan.h"
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <map>
#include <algorithm>
#include "cpk_reader.h"

struct RecState { uint32_t r[16]; uint32_t apsr; uint8_t* base; uint32_t mem_size; uint64_t d[32]; };
static RecState* SPTR(RecState* x) { return x; }
template<class T> static RecState* SPTR(T& x) { return &x; }
extern "C" uint32_t g_cur_block = 0;
static int g_thry_n = 0;
static int g_fact_bit = 0;
static void fix682a(RecState* s, uint32_t* ppc);
static int g_swsusp_n = 0;   // 18.4222 context-mix probe counter
static int g_stall2_n = 0;
static unsigned g_loop_cnt[4] = {0u,0u,0u,0u};
static int g_wd_thread = 0;
static int g_hacks = 0;
static int g_pump_wallclock = 0;
static int g_cb_yield = 0;
static int g_nested_stop_n = 0;
static int g_fiber_nested = 0;
static uint32_t g_nested_until = 0;   // 18.4219 hybrid: allow nested fiber exec only until this block count   // 18.4213: 0 = old-runtime semantics (defer fiber switch, no nested exec); P5D_FIBER_NESTED=1 restores old nested-exec path   // 18.4212: 0 = CheckCallback noop (old runtime n_noop); P5D_CB_YIELD=1 restores cooperative yield   // 18.4209: deterministic-by-default pump scheduling (set P5D_PUMP_WALLCLOCK=1 for the old wall-clock budget)
static int g_fsw_n = 0;
static int g_switch_pending = 0;   // 18.4217: synchronous fiber switch request (consumed by the outer dispatcher with next-pc)
static uint32_t g_switch_target = 0;
static int g_fiblog_n = 0;
static int g_boot_thunk = 0;   // 18.4218: start the pump fiber from the real boot thunk (g_fiber_entry=0x81206BDD) like the nested path did (that path reached GXM=12)
static int g_loop_site = -1;
static uint32_t g_f2_seen = 0;   // 18.4208b: last fiber whose context was really saved by execution (used to decide whether the pump may reset fiber2 to its entry)   // 18.4208: which dispatch loop executed the current block (0=fiber_exec,1=thread pump,2=main,3=fiber2 pump)   // 18.4205: semantic patch stack (XSKIP/REFNULL/NULLOBJ/RET/RETFAM/DUMMYPROG/NULLDESC/ABORTSKIP) - OFF by default for a clean baseline; P5D_HACKS=1 enables
typedef uint32_t (*BlockFn)(RecState*);
extern "C" uint32_t (*find_block_000(uint32_t pc))(RecState*);
#define FB(N) extern "C" uint32_t (*find_block_##N(uint32_t pc))(RecState*);
FB(000) FB(001) FB(002) FB(003) FB(004) FB(005) FB(006) FB(007) FB(008) FB(009)
FB(010) FB(011) FB(012) FB(013) FB(014) FB(015) FB(016) FB(017) FB(018) FB(019)
FB(020) FB(021) FB(022) FB(023) FB(024) FB(025)
extern "C" const void* get_table_000(unsigned* cnt);
extern "C" const void* get_table_001(unsigned* cnt);
extern "C" const void* get_table_002(unsigned* cnt);
extern "C" const void* get_table_003(unsigned* cnt);
extern "C" const void* get_table_004(unsigned* cnt);
extern "C" const void* get_table_005(unsigned* cnt);
extern "C" const void* get_table_006(unsigned* cnt);
extern "C" const void* get_table_007(unsigned* cnt);
extern "C" const void* get_table_008(unsigned* cnt);
extern "C" const void* get_table_009(unsigned* cnt);
extern "C" const void* get_table_010(unsigned* cnt);
extern "C" const void* get_table_011(unsigned* cnt);
extern "C" const void* get_table_012(unsigned* cnt);
extern "C" const void* get_table_013(unsigned* cnt);
extern "C" const void* get_table_014(unsigned* cnt);
extern "C" const void* get_table_015(unsigned* cnt);
extern "C" const void* get_table_016(unsigned* cnt);
extern "C" const void* get_table_017(unsigned* cnt);
extern "C" const void* get_table_018(unsigned* cnt);
extern "C" const void* get_table_019(unsigned* cnt);
extern "C" const void* get_table_020(unsigned* cnt);
extern "C" const void* get_table_021(unsigned* cnt);
extern "C" const void* get_table_022(unsigned* cnt);
extern "C" const void* get_table_023(unsigned* cnt);
extern "C" const void* get_table_024(unsigned* cnt);
extern "C" const void* get_table_025(unsigned* cnt);
static const uint32_t IMG_VA_BASE = 0x80600000;
static const uint32_t IMG_SIZE = 0x30000000;   // 18.4164: 768MB - low image 64MB + heap 96MB + big arena 256MB at 0x20000000 + region B
static const uint32_t REGIONB_OFF = 0x7000000u;  // region B base -> image+0x7A00000 (VA 0x88000000), no aliasing
static uint32_t g_fiber_entry = 0;
static int g_fiber_dbg = 0;

static int g_ef2_n = 0, g_ef2b_n = 0, g_ef2c_n = 0, g_ef2d_n = 0, g_ef2e_n = 0, g_ef2f_n = 0, g_ef2g_n = 0, g_ef2h_n = 0;
static int g_chaind_n = 0;
static int g_dp_n = 0;
static int g_ret1_n = 0;
static int g_efe9a_n = 0;
static uint32_t g_cur_hle_lr = 0;
static thread_local uint32_t tl_last_malloc_n = 0;
struct ARec { uint32_t p, sz, stub, lr; };
static std::mutex g_arec_mtx;
static std::vector<ARec> g_arec;
static uint32_t g_obj20_addr = 0;
static int g_obj20_n = 0;
static int g_obj20w_n = 0;
static int g_refnull_n = 0;
static int g_atombad_n = 0;
static int g_rt_n = 0;
static int g_allocbig2_n = 0;
static uint32_t g_allocbig_total = 0;
static int g_allocbig_n = 0;
static uint32_t g_p2_last = 0;
static int g_p2_chg_n = 0;
static int g_d044_n = 0;
static int g_pfmt_n = 0;
static int g_objh_n = 0;
static int g_cb0_n = 0;
static int g_obj24_n = 0;
static int g_xswap_n = 0;
static uint32_t g_p2_prog = 0; static int g_p2_pass = 0; static int g_p2_log = 0;
static int g_cb98_n = 0;
static int g_pump_cool = 0;
static int g_pump_timeout_n = 0;
static int g_pump_cool_n = 0;
static int g_pump_brk_n = 0;
static int g_yield_to_main = 0;
static uint32_t g_yield_main_tick = 0;
static int g_ytest_n = 0;
static int g_uncond3_n = 0;
static int g_always3 = 0;
static int g_force3_done = 0;
static uint32_t g_force3_obj = 0;
static int g_loop8cc_n = 0;
static int g_w8fex_n = 0;
static uint32_t g_w8c8_watch = 0;
static int g_objw_n = 0;
static int g_w8c8_n = 0;
static int g_delay_n = 0;
static int g_ydelay_n = 0;
static int g_calloc_n = 0;
static int g_nx9a_n = 0;
static int g_f2blk_n = 0;
static uint32_t g_yield_rr = 0;
static uint32_t g_yield_tick = 0;
static int g_yield_n = 0;
static int g_p682a_n = 0;
static int g_fex_budget = 0;   // 18.4167: max blocks per fiber slice (0 = unlimited)
static int g_fex_slice_n = 0;
static int g_gxm_stub_n = 0;
static int g_c1d2_n=0;
static int g_restart_n=0;
static int g_pc0_n=0;
static int g_postmain_n=0;
static int g_loadspin_n=0;
static int g_lp2_n=0;
static int g_schedrun_n=0;
static int g_spoll_n=0;
static int g_ctx_n=0;
static int g_stkw_n=0;
static int g_cf_n=0;
static int g_fp_n=0;
static int g_e14_n=0;
static int g_wp_n=0;
static int g_alloc_n=0; static int g_alloc2_n=0;
static uint32_t g_field_last=0; static int g_field_n=0; static uint32_t g_field_blk=0;
static int g_val44_n = 0;
static uint32_t g_snap_r[16]; static uint32_t g_snap_blk=0; static int g_snap_on=1;
static int g_fval_n = 0;
static int g_ea552_n = 0;
static int g_gcpy2_n = 0;
static int g_poolc_n = 0;
static int g_pool_n = 0;
static int g_gxmp_n = 0;
static int g_nset_n = 0;
static int g_tblw_n = 0, g_dsp_n = 0;
static int g_cbtr_n = 0;
static int g_lktr_n = 0;
static int g_acc_n = 0, g_lkin_n = 0, g_lkout_n = 0;
extern "C" int g_watch_on = 1;
// moved-globals-4177
static uint32_t g_watch_addr = 0;
static uint32_t g_watch_val = 0;
static int g_layout_n = 0;
static int g_store_n = 0;
static int g_m8tr_n = 0;
static int g_f7c2_n = 0;
static int g_f7ce_n = 0;
static int g_poolst_n = 0;

static int g_e522_n = 0;
static int g_d1d0_n = 0;
static int g_e6a8_n = 0;
static int g_84776_n = 0;
static int g_1949c_n = 0;
static int g_164f4_n = 0;
static int g_g18_n = 0;
static int g_d3a6_n = 0;
static int g_d3b2_n = 0;
static int g_d8e0_n = 0;
static int g_7c2_n = 0;
static int g_731d8_n = 0;
static int g_lw_n = 0;
static int g_ac9cc_n = 0;
static int g_ac934_n = 0;
static int g_f458_n = 0;
static int g_f448_n = 0;
static int g_flistfix_n = 0;
static int g_fiberdbg2_n = 0;
static int g_after_total = 0;
static int g_cw_on = 0;
static uint32_t g_cw_snap = 0;
static int g_cw_n = 0;
static int g_canaryfix_n = 0;
static int g_683c_n = 0;
static int g_679c_n = 0;
static int g_call683c_n = 0;
static int g_682a_n = 0;
static int g_5928_n = 0;
static uint32_t g_ws_addr = 0;
static uint32_t g_ws_snap = 0;
static int g_ws_on = 0;
static int g_ws_n = 0;
static int g_gxmvk_tick = 0;
static int g_gxmvk_present_n = 0;
static uint32_t g_stk_snap[16] = {0};
static int g_stk_range_on = 0;
static int g_stkrange_n = 0;
static uint32_t g_e6a1e_sp[8] = {0};
static uint32_t g_e6a1e_n = 0;
static uint32_t g_rslot_addr = 0;
static uint32_t g_rslot_exp = 0;
static int g_rslot_watch = 0;
static int g_rslotclob_n = 0;
static int g_d308_n = 0;
static int g_d72c_n = 0;
static int g_seq_n = 0;
static int g_d29a_n = 0;
static int g_slot_watch = 0;
static uint32_t g_save_slot = 0;
static int g_slot_n = 0;
static int g_stack_watch = 0;
static uint32_t g_stk_last = 0;
static int g_stkchg_n = 0;
static uint32_t g_fiber_arg = 0;

struct FCtx { uint32_t r[16]; uint32_t apsr; };
static std::map<uint32_t, FCtx> g_fctx;
struct HThread { uint32_t entry, arg, stack; FCtx ctx; int alive, ready, waiting; uint32_t wait_obj, wait_bits, wait_mode; RecState gctx; std::thread* hth; int exit_flag; };
static std::map<uint32_t, HThread> g_thr;
static uint32_t g_cur_thr = 0;
static int g_thr_blocked = 0;
static std::map<uint32_t, uint32_t> g_sema;
static std::map<uint32_t, uint32_t> g_eflag;
static int g_thr_log_n = 0;
static int g_evw_n = 0;
static int g_efc_n = 0, g_sc_n = 0, g_tc_n = 0;
static uint32_t g_next_tid = 0x40010000u, g_next_sema = 0x50010000u, g_next_eflag = 0x60010000u;
static uint32_t g_thr_wait_obj = 0, g_thr_wait_bits = 0, g_thr_wait_mode = 0;
static void hle_thr_pump(RecState* s, int quantum);
static uint32_t g_cur_fiber = 0;
static uint32_t g_last_fiber = 0;
static uint32_t g_prev_block = 0;
static int g_hm_n = 0;
static int g_hm2_n = 0;
static int g_fiosmiss_n = 0;
static int g_in_frame_pump = 0;
static int g_ctrl_n = 0;
static int g_mt_n = 0;
static int g_boot_trace_n = 0;
static int g_fslot_att = 0;
static int g_canary_n = 0;
static int g_rest_n = 0;
static int g_sw2_n = 0;
static int g_switch_budget = 0;
static int g_slice_back_n = 0;
static int g_t2_on = 0, g_t2_n = 0;
static uint32_t g_c_mgr = 0, g_c_67b0 = 0, g_c_67e8 = 0, g_c_680e = 0, g_c_681c = 0, g_c_6820 = 0, g_c_6822 = 0;
static int g_mgr_dbg = 0;
static int g_back_n = 0;
static int g_c_bea = 0, g_c_5cda = 0, g_c_5928 = 0, g_c_683c = 0, g_c_6860 = 0;
static uint32_t g_sw2_n2 = 0;
static int g_t7_on = 0, g_t7_n = 0;
static int g_mainret_n = 0;
static int g_nofn_n = 0;
static uint32_t g_pc_ring[64] = {0};
static uint32_t g_tr_pc[16] = {0}, g_tr_r0[16] = {0}, g_tr_r7[16] = {0};
static uint32_t g_tr_n = 0;
static int g_trans_n = 0;
static int g_in_eaa4 = 0, g_eaa4_ent_n = 0;
static int g_sw_dbg_n = 0;
static int g_fx_n = 0;
static int g_in_fex = 0, g_nested_n = 0;
static uint32_t g_mr_pc[40] = {0}, g_mr_nx[40] = {0};
static uint32_t g_mr_n = 0;
static uint32_t g_pc_ring_n = 0;
static int g_fslot_n = 0;
static int g_gs_n = 0;
static int g_pumped = 0;
static int g_pump_n = 0;
static int g_pt_on = 0;
static int g_pt_n = 0;
static int g_pd_n = 0;
static uint32_t g_pump_tick = 0;
static int g_tbl_n = 0;
static int g_fact_n = 0;
static uint32_t g_fb_addr = 0;
static int g_fb_set_n = 0;
static int g_finbit_n = 0;
static int g_mloop_n = 0;
static int g_ml2 = 0;
static int g_nullout_n = 0;
static int g_dummy_gs_n = 0;
static int g_ep_n = 0;
static int g_mgr_n = 0;
static int g_b6_n = 0;
static int g_e6a_n = 0;
static int g_gs_dbg_n = 0;
static int g_gxm_ctx_n = 0, g_gxm_begin_n = 0, g_gxm_end_n = 0, g_gxm_present_n = 0, g_gxm_fin_n = 0, g_gfx_rep_n = 0;
static uint32_t g_gsr_in[24] = {0}, g_gsr_out[24] = {0}, g_gsr_f[24] = {0};
static uint32_t g_gsr_n = 0;
static uint32_t g_dr_pc[24] = {0}, g_dr_r0[24] = {0}, g_dr_r3[24] = {0}, g_dr_sp[24] = {0};
static uint32_t g_dfe_pc[20] = {0}, g_dfe_r0[20] = {0}, g_dfe_sp[20] = {0}, g_dfe_n = 0;
static uint32_t g_drg_pc[20] = {0}, g_drg_r0[20] = {0}, g_drg_sp[20] = {0}, g_drg_n = 0;
static uint32_t g_dr_n = 0;
static uint32_t g_blkcnt_total = 1;
static bool nested_allowed(void) { return g_fiber_nested || (g_nested_until && g_blkcnt_total < g_nested_until); }
static int g_gxm_n = 0;
static int g_hm3_n = 0;
static int g_hm4_n = 0, g_hm5_n = 0;
static std::map<uint32_t, std::pair<uint32_t,uint32_t>> g_memblks;
static uint32_t g_memuid = 0x100;
static int g_memdbg_n = 0;
static int g_d683c = 0;
static int g_d7734 = 0;
static int g_slice_n = 0;
static int g_dheapn = 0;
static int g_dsl_n = 0;
static int g_dret_n = 0;
static uint32_t g_thread_regs[16];
static uint32_t g_thread_apsr = 0;
static int g_in_fiber_exec = 0;
static p5d::fs::CpkReader g_cpk, g_cpk2;
static bool g_cpk_ready = false;
static std::map<uint32_t, std::vector<uint8_t>> g_fios_data;
static std::map<uint32_t, size_t> g_fios_pos;
static std::map<uint32_t, std::vector<uint8_t>> g_io_data;   // 18.4255: sceIo* layer (SceIofilemgr) - CRI/GFD loader path
static std::map<uint32_t, size_t> g_io_pos;
static uint32_t g_io_id = 0;
static int g_io_n = 0;
static uint32_t g_fh_id = 0x3000u, g_op_id = 0x5000u;
static std::string g_inject_name;
static int g_pumping2 = 0, g_pump2_n = 0;
static int g_pump_disabled_ab = 1;   // A/B: 1 = pump disabled
static int g_after_nofn = 0;
static int g_stub_n = 0;
static int g_stub_fix_n = 0;
static int g_eaa4_cnt = 0, g_after_loop = 0, g_fback_n = 0;
static int g_stub_total = 0;
static uint32_t g_pre_pc[8] = {0};
static uint32_t g_pre_idx = 0;
static int g_after_stub = 0;
static uint32_t g_fread_n = 0, g_fread_bytes = 0;
static bool cpk_read_any_impl(const std::string& nm, std::vector<uint8_t>& out) {
  std::string k = nm; for (char& c : k) if (c == '\\') c = '/';
  static const char* pp[] = {"app0:/","app0:","host0:/","host0:","cache0:/","savedata0:/","ux0:"};
  for (const char* pre : pp) { size_t l = std::strlen(pre); if (k.size() >= l && _strnicmp(k.c_str(), pre, l) == 0) { k = k.substr(l); break; } }
  while (!k.empty() && k[0] == '/') k.erase(k.begin());
  if (g_cpk.contains(k) && g_cpk.read_member(k, out)) return true;
  if (g_cpk2.contains(k) && g_cpk2.read_member(k, out)) return true;
  // 18.3840: the loader prefixes paths with "app0:/data/" ("%s%s/%s.%s%s" with [0x816CDA1C]="/data"),
  // but CPK entries are stored WITHOUT the "data/" prefix (e.g. "title/title_p5.bin") -> retry stripped.
  if (k.size() > 5 && k.compare(0, 5, "data/") == 0) {
    std::string k2 = k.substr(5);
    if (g_cpk.contains(k2) && g_cpk.read_member(k2, out)) { printf("[FIOS] (stripped data/) %s -> %s\n", k.c_str(), k2.c_str()); return true; }
    if (g_cpk2.contains(k2) && g_cpk2.read_member(k2, out)) return true;
  }
  { std::string rel = k; for (char& c : rel) if (c == '/') c = '\\';
    std::string hp = std::string("C:\\dev\\p5dsrc\\game\\PCSH10109\\vita_extracted\\") + rel;
    FILE* fp = fopen(hp.c_str(), "rb");
    if (fp) { fseek(fp, 0, SEEK_END); long sz = ftell(fp); fseek(fp, 0, SEEK_SET);
      if (sz > 0 && sz <= (1L << 30)) { out.resize((size_t)sz); size_t got = fread(out.data(), 1, (size_t)sz, fp); fclose(fp);
        if (got == (size_t)sz) return true; } else fclose(fp); } }
  return false;
}
static void fios_ensure_cpk() {
  if (g_cpk_ready) return; g_cpk_ready = true;
  if (g_cpk.open("C:\\dev\\p5d_rom\\data\\data.cpk")) printf("[FIOS] mounted data.cpk files=%zu\n", g_cpk.file_count());
  else printf("[FIOS] WARN: data.cpk not mounted\n");
  if (g_cpk2.open("C:\\dev\\p5d_rom\\data\\vita.cpk")) printf("[FIOS] mounted vita.cpk files=%zu\n", g_cpk2.file_count());
  else printf("[FIOS] WARN: vita.cpk not mounted\n");
  { int n = 0; for (auto& kv : g_cpk.entries()) { if (n < 24) printf("[CPKNAME] %s\n", kv.first.c_str()); ++n; }
    if (n) { int pick = 0; for (auto& kv : g_cpk.entries()) { if (pick == 0) g_inject_name = kv.first; ++pick; }
      printf("[CPKNAME] picked first entry: '%s' (total %zu)\n", g_inject_name.c_str(), g_cpk.file_count()); } }
}
static std::map<uint32_t,int> g_hlecnt;
static std::map<uint32_t,int> g_blkcnt;
static uint32_t g_watch2 = 0;
static uint32_t g_watch2_val = 0;
static int g_watch2_n = 0;
static int g_path_n = 0;
static int g_finit_n = 0;
static int g_f2_n = 0;
static std::vector<uint32_t> g_fiber_list;
static int g_run_alt = 0;
static int g_sw_alt = 0;
static int g_sw_alt2 = 0;
static int g_sw_trig = 0;
static int g_sw_dbg = 0;
static int g_swp_n = 0;
static int g_swrst_n = 0;
static int g_e59_n = 0;
static int g_fiber_dbg2 = 0;
static int g_fiber_dbg3 = 0;
static int g_fsv_n = 0;
static int g_epi2_n2 = 0;
static int g_fiber_flag_return = 0;
static uint32_t g_fiber_switch_to = 0;
static uint32_t fiber_exec_to_completion(RecState* s);
static void fiber_load(RecState* s, uint32_t fiber, uint32_t entry, uint32_t arg);
static void fiber_restore_thread(RecState* s);
static FCtx g_caller_ctx;   // 18.4216: caller context saved around nested fiber execution
static void rc_save(RecState* s) { std::memcpy(g_caller_ctx.r, s->r, sizeof(s->r)); g_caller_ctx.apsr = s->apsr; }
static void rc_restore(RecState* s) { std::memcpy(s->r, g_caller_ctx.r, sizeof(s->r)); s->apsr = g_caller_ctx.apsr; }
static BlockFn gbn(uint32_t pc);
static uint32_t gbn_back(uint32_t t, int* pback);
static int g_stall_n = 0;
static uint32_t run_guest(RecState* s, uint32_t entry, uint32_t arg);
static uint8_t* gp_img = nullptr;
static inline uint8_t* GP(uint32_t va) {
  uint32_t off;
  if (va >= 0x80600000u && va < 0x84600000u) off = va - 0x80600000u;
  else if (va >= 0xA0000000u && va < 0xA2000000u) off = (va - 0xA0000000u) + 0x28000000u;   // 18.4184: Region B extended to 32MB at a fresh host offset
  else if (va >= 0xA2000000u && va < 0xA8000000u) off = (va - 0xA2000000u) + 0x2A000000u;   // 18.4251: 96MB extra guest window (host tail) for memblock capacity
  else if (va >= 0x84600000u && va < 0x8C600000u) off = (va - 0x84600000u) + 0x4000000u;   // heap arena 96MB
  else if (va >= 0x8C600000u && va < 0xA0000000u) off = (va - 0x8C600000u) + 0x10000000u;  // 18.4165: 320MB large-block arena
  else if (va >= 0x9F000000u && va < 0xA0000000u) off = (va - 0x9F000000u) + 0x10000000u;  // 18.4162: 16MB Vita user-memblock window (fibers/scratch below Region B)
  else { fprintf(stderr, "[HLE-BADMEM] hle access va=%08X blk=%08X stub=%08X tid=%u tlring:", va, g_cur_block, g_cur_hle_stub, tl_ht_tid);
         for (int i = 0; i < 24; ++i) fprintf(stderr, " %08X", tl_pc_ring[(unsigned)i]); fprintf(stderr, " ring:");
         for (int i = 0; i < 32; ++i) fprintf(stderr, " %08X", g_pc_ring[(g_pc_ring_n + (uint32_t)i) & 63u]);
         fprintf(stderr, "\n"); return nullptr; }
  if (off >= IMG_SIZE) { fprintf(stderr, "[HLE-BADMEM] hle access va=%08X off=%08X blk=%08X stub=%08X tid=%u tlring:", va, off, g_cur_block, g_cur_hle_stub, tl_ht_tid);
         for (int i = 0; i < 24; ++i) fprintf(stderr, " %08X", tl_pc_ring[(unsigned)i]); fprintf(stderr, " ring:");
    for (int i = 0; i < 32; ++i) fprintf(stderr, " %08X", g_pc_ring[(g_pc_ring_n + (uint32_t)i) & 63u]);
    fprintf(stderr, "\n"); return nullptr; }
  return gp_img + off;
}
static int g_682a_skip_n = 0;
static void fix682a(RecState* s, uint32_t* ppc) {   // 18.4226: canary-epilogue guard (r5 restore + skip inconsistent frame)
  uint32_t pc = *ppc & ~1u;
  if (pc != 0x8120682Au) return;
  if (s->r[5] == 0u) s->r[5] = 0x812B414Cu;
  uint32_t sp = s->r[13], v1 = 0, cv = 0;
  uint8_t* q = GP(sp + 4u); if (q) std::memcpy(&v1, q, 4);
  q = GP(0x812B414Cu); if (q) std::memcpy(&cv, q, 4);
  if (v1 != cv) {   // frame belongs to another context -> do not run the canary epilogue; return to lr
    if (g_682a_skip_n++ < 12) { printf("[682A-SKIP] sp=%08X [sp+4]=%08X canary=%08X lr=%08X fib=%08X\n", sp, v1, cv, s->r[14], g_cur_fiber); fflush(stdout); }
    *ppc = s->r[14];
  }
}
static uint32_t g_heap_ptr = 0x82800000u;
static uint32_t g_big_ptr = 0x8C600000u, g_big_end = 0xA0000000u;   // 18.4165: big arena 320MB   // bump allocator area inside the image (8MB below image end)
static uint32_t g_heap_end = 0x82E00000u;
static uint32_t g_heap3_ptr = 0x83600000u;
static uint32_t g_heap3_end = 0x84000000u;
static uint32_t g_heapA_ptr = 0x84600000u;   // 18.3863: 96MB heap arena (image+0x4000000)
static uint32_t g_heapA_end = 0x8C600000u;
static uint32_t g_heap2_ptr = 0xA0000000u;  // secondary bump heap in region B (below fiber stacks)
static uint32_t g_heap2_end = 0xA0DF0000u;  // 18.3862: enlarged 6MB -> 14MB
static uint32_t g_heap4_ptr = 0x82E00000u, g_heap4_end = 0x83600000u;
static uint32_t g_resA_ptr = 0x81700000u, g_resA_end = 0x82800000u;
static uint32_t g_big2_ptr = 0xA2000000u, g_big2_end = 0xA8000000u;   // 18.4251: +96MB capacity window   // 18.4250: reserve arena in the unused image window (batch P() maps this VA range)  // 18.3862: free low-image window (8MB)
static uint32_t g_heap5_ptr = 0x84000000u, g_heap5_end = 0x84600000u;  // 18.3862: free low-image window (6MB)
static uint32_t g_target_alloc = 0;
static int g_objalloc_n = 0;
static uint32_t hle_malloc_ret(uint32_t p) {
  if (p >= 0x84600000u && p < 0x8C600000u) {
    { std::lock_guard<std::mutex> _lk(g_arec_mtx); g_arec.push_back({p, tl_last_malloc_n, g_cur_hle_stub, g_cur_hle_lr}); }
    if (p == 0x8460F150u || (p >= 0x8460F000u && p < 0x84610000u)) {
      printf("[AREC-TARGET] p=%08X sz=%08X stub=%08X lr=%08X\n", p, tl_last_malloc_n, g_cur_hle_stub, g_cur_hle_lr); fflush(stdout); }
  }
  if (p == 0x84C0A520u || tl_last_malloc_n == 0xC8u) {
    printf("[CHILDALLOC] p=%08X sz=%08X stub=%08X lr=%08X blk=%08X fib=%08X thr=%08X ring:", p, tl_last_malloc_n, g_cur_hle_stub, g_cur_hle_lr, g_cur_block, g_cur_fiber, tl_ht_tid);
    for (int _k=0;_k<32;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 32u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout); }
  if (g_target_alloc && p == g_target_alloc && g_objalloc_n++ < 6) {
    printf("[OBJALLOC] p=%08X lr=%08X blk=%08X fib=%08X ring:", p, g_cur_hle_lr, g_cur_block, g_cur_fiber);
    for (int _k=0;_k<16;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 16u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout); }
  return p;
}
static uint32_t hle_malloc(uint32_t n) {
  std::lock_guard<std::mutex> _hlk(g_hle_mtx);
  tl_last_malloc_n = n;   // 18.4241: host-thread model - allocator must be thread-safe
  uint64_t sz64 = ((uint64_t)n + 15ull) & ~15ull;
  if (sz64 > 0x20000000ull) {   // 18.4191: reject absurd sizes - the 32-bit +15 wrap used to bypass the arena checks and memset() crashed natively
    printf("[HLE] malloc REJECT huge n=%08X\n", n); fflush(stdout); return 0;
  }
  uint32_t sz = (uint32_t)sz64;
  if (n >= 0x100000u) { uint32_t bp = (g_big_ptr + 15u) & ~15u;   // 18.4163: big blocks from the extended arena
    if ((uint64_t)bp + sz64 < (uint64_t)g_big_end) { g_big_ptr = bp + sz; std::memset(GP(bp), 0, sz); if (g_hm_n < 15) { ++g_hm_n; printf("[BIGARENA] n=%08X -> %08X\n", n, bp); } return hle_malloc_ret(bp); } }
  if (n >= 0x100000u) { uint32_t ra = (g_resA_ptr + 15u) & ~15u;   // 18.4250: reserve arena, LARGE allocations only (VA 0x81700000-0x82800000 is mapped by both GP and batch P())
    if ((uint64_t)ra + sz64 < (uint64_t)g_resA_end) { g_resA_ptr = ra + sz; std::memset(GP(ra), 0, sz);
      if (g_hm_n < 25) { ++g_hm_n; printf("[RESARENA] n=%08X -> %08X\n", n, ra); } return hle_malloc_ret(ra); } }
  { uint32_t pa = (g_heapA_ptr + 15u) & ~15u;
    if ((uint64_t)pa + sz64 < (uint64_t)g_heapA_end) { g_heapA_ptr = pa + sz; std::memset(GP(pa), 0, sz); return hle_malloc_ret(pa); } }
  uint32_t p = (g_heap_ptr + 15u) & ~15u;
  if ((uint64_t)p + sz64 < (uint64_t)g_heap_end) { g_heap_ptr = p + sz; std::memset(GP(p), 0, sz); return hle_malloc_ret(p); }
  uint32_t q = (g_heap2_ptr + 15u) & ~15u;
  if ((uint64_t)q + sz64 < (uint64_t)g_heap2_end) { g_heap2_ptr = q + sz; std::memset(GP(q), 0, sz);
    if (g_hm2_n++ < 15) printf("[HM2] heap2 n=%u -> %08X\n", n, q);
    return hle_malloc_ret(q); }
  uint32_t r3 = (g_heap3_ptr + 15u) & ~15u;
  if ((uint64_t)r3 + sz64 < (uint64_t)g_heap3_end) { g_heap3_ptr = r3 + sz; std::memset(GP(r3), 0, sz);
    if (g_hm3_n++ < 15) printf("[HM3] heap3 n=%u -> %08X\n", n, r3);
    return hle_malloc_ret(r3); }
  uint32_t r4 = (g_heap4_ptr + 15u) & ~15u;
  if ((uint64_t)r4 + sz64 < (uint64_t)g_heap4_end) { g_heap4_ptr = r4 + sz; std::memset(GP(r4), 0, sz);
    if (g_hm4_n++ < 8) printf("[HM4] heap4 n=%u -> %08X\n", n, r4);
    return hle_malloc_ret(r4); }
  uint32_t r5 = (g_heap5_ptr + 15u) & ~15u;
  if ((uint64_t)r5 + sz64 < (uint64_t)g_heap5_end) { g_heap5_ptr = r5 + sz; std::memset(GP(r5), 0, sz);
    if (g_hm5_n++ < 8) printf("[HM5] heap5 n=%u -> %08X\n", n, r5);
    return hle_malloc_ret(r5); }
  { uint32_t b2 = (g_big2_ptr + 15u) & ~15u;   // 18.4251: 96MB extra window (VA 0xA2000000-0xA8000000)
    if ((uint64_t)b2 + sz64 < (uint64_t)g_big2_end) { g_big2_ptr = b2 + sz; std::memset(GP(b2), 0, sz);
      if (g_big2n++ < 30) { printf("[BIG2] n=%08X -> %08X\n", n, b2); fflush(stdout); } return hle_malloc_ret(b2); } }
  printf("[HLE] malloc OOM n=%u (all heaps full)\n", n); return 0;
}
static std::map<uint32_t,uint32_t> g_prog_header_by_id;   // 18.4259: SceGxmShaderPatcher register map (id -> GXP program header VA)
static uint32_t g_prog_id_ctr = 0;
static uint32_t g_fprog_last = 0, g_vprog_last = 0, g_last_rt = 0;
static inline uint32_t guest_rd32(uint32_t va) { uint32_t v = 0; uint8_t* p = GP(va); if (p) std::memcpy(&v, p, 4); return v; }
static inline void guest_wr32(uint32_t va, uint32_t v) { uint8_t* p = GP(va); if (p) std::memcpy(p, &v, 4); }
static int g_gxm_new_n = 0;
static int g_hle_n = 0;
static int g_touch_n = 0;
static int g_big_n = 0;

static uint32_t hle_canary_emulated_ret(RecState* s) { return s->r[0]; }
static uint32_t g_hle_ring_va[128]; static uint32_t g_hle_ring_lr[128]; static int g_hle_ring_n=0;
static int g_f2hle_n = 0; static uint32_t g_f2_first_pc = 0;
static int g_fam_n = 0;
static int g_assert_n = 0;
static int g_amsg_n = 0;
static void assert_msg_probe(RecState* s, uint32_t pcv) {   // 18.4206: at 0x8115F9CA entry r0 points to the formatted assert message
  if ((pcv & ~1u) != 0x8115F9CAu || g_amsg_n++ >= 20) return;
  uint32_t p = s->r[0]; char buf[200] = {0}; int n = 0;
  if (p >= 0x80000000u) { for (int i = 0; i < 199; ++i) { uint8_t* q = GP(p + (uint32_t)i); if (!q || !*q) break; buf[i] = (char)*q; n = i + 1; } }
  for (int _i = 0; _i < n; ++_i) if (buf[_i] == '\n' || buf[_i] == '\r') buf[_i] = '|';
  printf("[ASSERT-MSG] '%s' lr=%08X r0=%08X r1=%08X r2=%08X r3=%08X sp=%08X fib=%08X\n", buf, s->r[14], s->r[0], s->r[1], s->r[2], s->r[3], s->r[13], g_cur_fiber); fflush(stdout);
}

static int g_iw_n[8] = {0};
static void initwatch_probe(RecState* s, uint32_t pcv) {   // 18.4207: count entries of the four app-init functions and log their caller lr
  uint32_t p = pcv & ~1u; const char* nm = 0; int slot = -1;
  if (p == 0x8116E63Cu) { nm = "appPad-init"; slot = 0; }
  else if (p == 0x8117339Cu) { nm = "appTouch-init"; slot = 1; }
  else if (p == 0x8115D5E2u) { nm = "appAllocHandler-init"; slot = 2; }
  else if (p == 0x81008222u) { nm = "appGxScene-init"; slot = 3; }
  else if (p == 0x81172EFCu) { nm = "appSystem-init"; slot = 4; }
  else if (p == 0x81184536u) { nm = "appSystem-init-callblk"; slot = 5; }
  else if (p == 0x811845C8u) { nm = "sysInitAgg"; slot = 6; }
  else if (p == 0x8115A336u) { nm = "initEntry-A-336"; slot = 7; }
  else return;
  if (g_iw_n[slot]++ < 6) { printf("[INITW] %s #%d loop=%d lr=%08X sp=%08X fib=%08X r0=%08X", nm, g_iw_n[slot], g_loop_site, s->r[14], s->r[13], g_cur_fiber, s->r[0]);
    if (slot == 6) { printf(" chain:"); for (int k = 0; k < 24; ++k) { uint8_t* q = GP(s->r[13] + (uint32_t)k * 4u); if (!q) break; uint32_t w = 0; std::memcpy(&w, q, 4); uint32_t a = w & ~1u; if (a >= 0x81000000u && a < 0x81330000u) printf(" %08X", a); } }
    printf("\n"); fflush(stdout); }
}

static int g_h7344_n = 0;
static void h7344_probe(RecState* s, uint32_t pcv) {   // 18.4209: who passes r0=2 to 0x81207344?
  uint32_t p = pcv & ~1u;
  if (p != 0x81207344u && p != 0x81207352u) return;
  const bool bad = (s->r[0] < 0x80000000u);
  if (bad || g_h7344_n < 12) {
    ++g_h7344_n;
    printf("[H7344]%s pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X fib=%08X loop=%d", bad?"-BAD":"", p, s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], s->r[13], g_cur_fiber, g_loop_site);
    printf(" ring:"); for (int k = 64 - 20; k < 64; ++k) printf(" %08X", g_pc_ring[(g_pc_ring_n + (uint32_t)k) & 63u]);
    printf(" bt:"); for (int k = 0; k < 20; ++k) { uint8_t* q = GP(s->r[13] + (uint32_t)k * 4u); if (!q) break; uint32_t w = 0; std::memcpy(&w, q, 4); uint32_t a = w & ~1u; if (a >= 0x81000000u && a < 0x81330000u) printf(" %08X", a); }
    printf("\n"); fflush(stdout);
  }
}

static int assert_hle(RecState* s, uint32_t pcv) {   // 18.4205: platform ASSERT decision stub (0x8115F7E0) - emulates the dialog default "ignore"
  if ((pcv & ~1u) != 0x8115F7E0u) return 0;
  char msg[160] = {0}; char file[160] = {0};
  uint32_t m = s->r[2], f = s->r[3];
  if (m >= 0x80000000u) { uint8_t* q = GP(m); if (q) for (int i = 0; i < 159 && q[i]; ++i) msg[i] = (char)q[i]; }
  if (f >= 0x80000000u) { uint8_t* q = GP(f); if (q) for (int i = 0; i < 159 && q[i]; ++i) file[i] = (char)q[i]; }
  if (g_assert_n++ < 40) { printf("[ASSERT] #%d msg='%s' file='%s' lr=%08X sp=%08X fib=%08X\n", g_assert_n, msg, file, s->r[14], s->r[13], g_cur_fiber); fflush(stdout); }
  s->r[0] = 0u; s->r[15] = s->r[14];
  return 1;
}

static int family_skip(RecState* s, uint32_t pcv) {   // 18.4200: skip the draw-traversal family when the model/material +0x20 resource is null
  static const uint32_t k[] = {0x811E2B50u,0x811E314Cu,0x811E3472u,0x811E3858u,0x811E3960u,0x811E3B54u,0x811E3CD8u,0x811E3D18u,0x811E3D58u,0x811E3D8Cu,0x811E3DB0u,0x811E3DCCu,0x811E3DF0u,0x811E3E0Cu,0x811E3E2Au,0x811E3E62u,0x811E3EAAu,0x811E3F10u,0x811E3F76u};
  if (!g_hacks) return 0;
  uint32_t pc = pcv & ~1u; int hit = 0;
  for (unsigned i = 0; i < sizeof(k)/sizeof(k[0]); ++i) if (k[i] == pc) { hit = 1; break; }
  if (!hit) return 0;
  auto chainnull = [&](uint32_t b) -> int {
    if (b < 0x80000000u) return 0;
    uint8_t* q; uint32_t A = 0, B = 0, C = 0;
    q = GP(b + 0x40u); if (q) std::memcpy(&A, q, 4);
    if (A >= 0x80000000u) { q = GP(A + 0x78u); if (q) std::memcpy(&B, q, 4); }
    if (B >= 0x80000000u) { q = GP(B + 0x20u); if (q) std::memcpy(&C, q, 4); }
    return (A >= 0x80000000u && B >= 0x80000000u && C == 0u) ? 1 : 0;
  };
  if (chainnull(s->r[1]) || chainnull(s->r[0])) {
    s->r[15] = s->r[14];
    if (g_fam_n++ < 20) { printf("[RETFAM] skip %08X base=%08X fib=%08X\n", pc, s->r[1], g_cur_fiber); fflush(stdout); }
    return 1;
  }
  return 0;
}

static int g_ndesc_n = 0;
static int nulldesc_skip(RecState* s, uint32_t pcv) {   // 18.4200b: length provider 0x811DE912 with null descriptor -> return 0
  if (!g_hacks) return 0;
  if ((pcv & ~1u) != 0x811DE912u || s->r[0] >= 0x80000000u) return 0;
  s->r[0] = 0u; s->r[15] = s->r[14];
  if (g_ndesc_n++ < 10) { printf("[NULLDESC] 0x811DE912 null r0 -> 0 fib=%08X\n", g_cur_fiber); fflush(stdout); }
  return 1;
}

static void ef2_probe(uint32_t pc, RecState* s, int site) {
  uint32_t p = pc & ~1u;
  if (p == 0x811E2B50u && g_chaind_n++ < 6) {
    uint32_t _sl=s->r[1], _a=0,_b=0,_c=0; uint8_t* _q;
    _q=GP(_sl+0x40u); if(_q) std::memcpy(&_a,_q,4);
    if (_a>=0x80000000u) { _q=GP(_a+0x78u); if(_q) std::memcpy(&_b,_q,4); }
    if (_b>=0x80000000u) { _q=GP(_b+0x20u); if(_q) std::memcpy(&_c,_q,4); }
    printf("[CHAIN] sl=%08X A=[sl+0x40]=%08X B=[A+0x78]=%08X B+0x20=%08X fib=%08X\n", _sl,_a,_b,_c,g_cur_fiber); fflush(stdout);
  }
  if (g_hacks && p == 0x811C4122u && s->r[0] == 0u) {   // 18.4199: creator stores the stub getter result (0) into [obj+0x20]; substitute a dummy program object at the source
    static uint32_t _dp = 0;
    if (!_dp) { _dp = hle_malloc(0x100u); if (_dp) { uint8_t* _q = GP(_dp); if (_q) { uint32_t w[4] = { 0x47534650u, _dp + 4u, 0u, 0u }; std::memcpy(_q, w, 16); } } }
    if (_dp) { if (g_dp_n++ < 8) { printf("[DUMMYPROG] creator +0x20 <- %08X fib=%08X\n", _dp, g_cur_fiber); fflush(stdout); } s->r[0] = _dp; }
  }
  if (p == 0x811EFE9Eu && g_ef2f_n++ < 6) { printf("[GXMCR] real creator 0x811EFE9E called r0=%08X r1=%08X fib=%08X\n", s->r[0], s->r[1], g_cur_fiber); fflush(stdout); }
  if (p == 0x811C40F8u && g_ef2g_n++ < 8) { printf("[LOOKUP] FUN_811C3F3E -> r6=%08X (0=miss, non0=hit) fib=%08X\n", s->r[6], g_cur_fiber); fflush(stdout); }
  if (p == 0x811E3520u && g_ef2b_n++ < 8) {
    uint32_t _v = 0; uint8_t* _q = GP(s->r[4] + 0x20u); if (_q) std::memcpy(&_v,_q,4);
    printf("[EF2] site=%d blk=E3520 r4=%08X [r4+0x20]=%08X sp=%08X fib=%08X\n", site, s->r[4], _v, s->r[13], g_cur_fiber); fflush(stdout); }
  if (g_hacks && p == 0x811F01AEu && s->r[0] == 0u) {   // 18.4197: null source -> fabricate a zeroed source object so factory returns src+4 instead of 4
    static uint32_t _nobj = 0;
    if (!_nobj) _nobj = hle_malloc(0x100u);
    if (_nobj) { s->r[0] = _nobj; if (g_ef2e_n++ < 8) { printf("[NULLOBJ] F01AE null source -> %08X fib=%08X\n", _nobj, g_cur_fiber); fflush(stdout); } }
  }
  if (p == 0x811F01AEu && g_ef2c_n++ < 8) {
    printf("[EF2] site=%d enter F01AE r0=%08X r1=%08X r2=%08X lr=%08X sp=%08X\n", site, s->r[0], s->r[1], s->r[2], s->r[14], s->r[13]); fflush(stdout); }
  if (p == 0x811E3528u && g_ef2d_n++ < 8) {
    printf("[EF2] site=%d after F01AE r0=%08X r4=%08X sp=%08X\n", site, s->r[0], s->r[4], s->r[13]); fflush(stdout); }
}

static int guest_format(uint32_t dst, uint32_t cap, uint32_t fmtva, RecState* s) {
  uint8_t* d = GP(dst); uint8_t* f = GP(fmtva); if (!d || !f || cap == 0) return 0;
  uint32_t written = 0;
  uint32_t argi = 0;
  auto next = [&]() -> uint32_t {
    if (argi == 0) { ++argi; return s->r[3]; }
    uint32_t a = 0; uint8_t* sp = GP(s->r[13] + (argi - 1) * 4u); if (!sp) return 0;
    std::memcpy(&a, sp, 4); ++argi; return a; };
  auto put = [&](char c) { if (written + 1 < cap) d[written] = (uint8_t)c; ++written; };
  for (uint32_t i = 0; f[i] && written + 1 < cap; ++i) {
    if (f[i] != '%') { put((char)f[i]); continue; }
    char c = (char)f[i+1];
    if (c == '%') { put('%'); ++i; continue; }
    if (c == 's') { uint32_t a = next(); uint8_t* q = GP(a); if (q) for (uint32_t k=0; q[k] && written+1<cap; ++k) put((char)q[k]); ++i; }
    else if (c == 'c') { put((char)next()); ++i; }
    else if (c == 'd' || c == 'i') { char b[16]; int32_t v=(int32_t)next(); int n=0; bool ng=v<0; uint32_t uv=ng?(uint32_t)(-v):(uint32_t)v; if(!uv)b[n++]='0'; while(uv&&n<15){b[n++]=(char)('0'+uv%10);uv/=10;} if(ng)b[n++]='-'; while(n>0) put(b[--n]); ++i; }
    else if (c == 'u') { char b[16]; uint32_t uv=next(); int n=0; if(!uv)b[n++]='0'; while(uv&&n<15){b[n++]=(char)('0'+uv%10);uv/=10;} while(n>0) put(b[--n]); ++i; }
    else if (c == 'x' || c == 'X') { char b[16]; const char* hx = (c=='x')?"0123456789abcdef":"0123456789ABCDEF"; uint32_t uv=next(); int n=0; if(!uv)b[n++]='0'; while(uv&&n<15){b[n++]=hx[uv&15];uv>>=4;} while(n>0) put(b[--n]); ++i; }
    else { put('%'); }
  }
  if (cap) d[written < cap ? written : cap-1] = 0;
  return (int)written;
}
static void hle_eflag_set(uint32_t obj, uint32_t bits) {
  g_eflag[obj] |= bits;
  for (auto& kv : g_thr) if (kv.second.waiting && kv.second.wait_obj == obj) {
    bool _or = (kv.second.wait_mode & 1u) != 0u;
    bool _ok = kv.second.wait_bits == 0u || (_or ? ((g_eflag[obj] & kv.second.wait_bits) != 0u)
                                                 : ((g_eflag[obj] & kv.second.wait_bits) == kv.second.wait_bits));
    if (_ok) { kv.second.waiting = 0; kv.second.ready = 1; } }
}
extern "C" uint32_t hle_call(uint32_t stub_va, RecState* s) {
  g_cur_hle_stub = stub_va;   // 18.4241: diagnostics for host-thread BADMEM
  g_cur_hle_lr = s->r[14];
  if (g_hle_n < 6000) printf("[HLE] #%d va=%08X lr=%08X a0=%08X a1=%08X a2=%08X\n", g_hle_n, stub_va, s->r[14], s->r[0], s->r[1], s->r[2]);
  { g_hle_ring_va[g_hle_ring_n & 127] = stub_va; g_hle_ring_lr[g_hle_ring_n & 127] = s->r[14]; ++g_hle_ring_n; }
  if (g_in_fiber_exec && g_cur_fiber && g_cur_fiber != g_fiber_list.empty() && 0) {}
  if (g_in_fiber_exec && g_cur_fiber && g_fiber_list.size() > 1 && g_cur_fiber == g_fiber_list[1] && g_f2hle_n++ < 90) {
    printf("[F2HLE] #%d va=%08X lr=%08X r0=%08X r1=%08X r2=%08X r3=%08X pc_ring_tail:", g_f2hle_n, stub_va, s->r[14], s->r[0], s->r[1], s->r[2], s->r[3]);
    for (int _k=4;_k>0;--_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
  }
  if (stub_va == 0x812B2280u && GP(s->r[0]) == nullptr && g_pfmt_n++ < 20) {
    printf("[PFMT] #%d r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X blk=%08X fib=%08X mem0=", g_pfmt_n, s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], g_cur_block, g_cur_fiber);
    uint8_t* q = GP(s->r[1]); if (q) { char buf[64]; int i=0; for (; i<40; ++i) { char c = (char)q[i]; buf[i] = (c>=32&&c<127)?c:'.'; if (!c) break; } buf[i]=0; printf("fmt='%s'", buf); } else printf("fmt=BAD");
    printf(" ring:"); for (int _k=0;_k<12;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 12u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
  }
  ++g_hlecnt[stub_va];
  if (stub_va == 0x812B2810u) {   // __stack_chk_fail: REPAIR the canary slot and RETRY the compare block
    {
      uint32_t sp = s->r[13];
      uint32_t fb = sp + 0xCu;
      uint32_t sr4=0, sr5=0, slr=0;
      uint8_t* q;
      q=GP(fb); if(q) std::memcpy(&sr4,q,4);
      q=GP(fb+4u); if(q) std::memcpy(&sr5,q,4);
      q=GP(fb+8u); if(q) std::memcpy(&slr,q,4);
      printf("[CHKFIX2] repair canary at sp=%08X (copy from 0x812B414C); saved_lr=%08X saved_r4=%08X saved_r5=%08X fib=%08X\n", sp, slr, sr4, sr5, g_cur_fiber); fflush(stdout);
      // write the expected canary value (contents of the global canary pointer cell) into [sp+4]
      uint32_t cval=0; uint8_t* cq=GP(0x812B414Cu); if(cq) std::memcpy(&cval,cq,4);
      uint8_t* dst=GP(sp+4u); if(dst) std::memcpy(dst,&cval,4);
      s->r[15] = 0x8120682Au;      // re-run the compare block
      s->r[0] = 0u;
    }
    return hle_canary_emulated_ret(s);
  }
  if (stub_va == 0x812B2810u) {   // __stack_chk_fail: print canary state to find the mismatch source
    uint32_t c1=0,c2=0,csp=0,csp4=0; uint8_t* q;
    q=GP(0x812B414Cu); if(q) std::memcpy(&c1,q,4);
    q=GP(0x812B4150u); if(q) std::memcpy(&c2,q,4);
    q=GP(s->r[13]); if(q) std::memcpy(&csp,q,4);
    q=GP(s->r[13]+4u); if(q) std::memcpy(&csp4,q,4);
    printf("[CHK] CANARY_FAIL glob414c=%08X glob4150=%08X r0=%08X r1=%08X sp=%08X [sp]=%08X [sp+4]=%08X lr=%08X fib=%08X thr=%08X\n", c1,c2,s->r[0],s->r[1],s->r[13],csp,csp4,s->r[14],g_cur_fiber,g_cur_thr); fflush(stdout);
  }
  if (stub_va == 0x812B22A0u && s->r[0] > 0x1000000u && g_big_n < 12) { ++g_big_n;
    printf("[BIGMALLOC] #%d a0=%08X lr=%08X block=%08X sp=%08X\n", g_hle_n, s->r[0], s->r[14], g_cur_block, s->r[13]); }
  ++g_hle_n;
  uint32_t r0 = 0;
  switch (stub_va) {
    case 0x812B2370u: r0 = 0; break;                                   // _sce_set_dso_handle_main
    case 0x812B22A0u: {                                                // malloc
      uint32_t n = s->r[0]; r0 = hle_malloc(n);
      if (n >= 0x1000u && n <= 0x100000u && g_hm_n < 15) { ++g_hm_n;
        printf("[HM] n=%08X -> %08X heap_ptr=%08X lr=%08X block=%08X\n", n, r0, g_heap_ptr, s->r[14], g_cur_block); }
      if (n > 0x100000u) printf("[BIGALLOC] n=%08X -> %08X lr=%08X block=%08X\n", n, r0, s->r[14], g_cur_block);
      break; }
    case 0x812B2250u: r0 = 0; break;                                   // free
    case 0x812B2340u: {                                                // memalign(align, size) [SceLibc 0xA9363E6B]
      uint32_t al = s->r[0] ? s->r[0] : 16u;
      if (al < 16u) al = 16u;
      uint32_t sz = s->r[1];
      uint32_t raw = hle_malloc(sz + al + 16u);
      if (raw) r0 = (raw + al - 1u) & ~(al - 1u);
      if (sz > 0x100000u || g_big_n < 0) printf("[MEMALIGN] al=%08X sz=%08X raw=%08X -> %08X lr=%08X\n", al, sz, raw, r0, s->r[14]);
      break; }
    case 0x812B2270u: {                                                // memcpy(dst,src,n) -> dst
      uint32_t dst = s->r[0], src = s->r[1], n = s->r[2];
      uint8_t* d = GP(dst); uint8_t* sp = GP(src);
      if (d && sp && n < 0x1000000u) std::memcpy(d, sp, n);
      r0 = dst; break; }
    case 0x812B2260u: {                                                // memset(dst,c,n) -> dst
      uint32_t dst = s->r[0], c = s->r[1], n = s->r[2];
      uint8_t* d = GP(dst);
      if (d && n < 0x1000000u) std::memset(d, (int)(c & 0xFF), n);
      r0 = dst; break; }
    case 0x812B22E0u: {                                                // strlen(s)
      uint32_t a = s->r[0]; uint8_t* p = GP(a); r0 = 0;
      if (p) { while (r0 < 0x100000u && p[r0]) ++r0; }
      break; }
    case 0x812B22D0u: {                                                // strcpy(dst,src) -> dst
      uint32_t dst = s->r[0], src = s->r[1];
      uint8_t* d = GP(dst); uint8_t* sp = GP(src);
      if (d && sp) { uint32_t i = 0; do { d[i] = sp[i]; } while (sp[i++] && i < 0x100000u); }
      r0 = dst; break; }
    case 0x812B2360u: r0 = 0; break;                                   // __at_quick_exit
    case 0x812B2200u: {                                               // 18.4256: strcmp(s1,s2) [NID 1B58FA3B] (was MISSING -> always 0!)
      uint8_t* a = GP(s->r[0]); uint8_t* b = GP(s->r[1]); r0 = 0;
      if (a && b) { for (uint32_t i = 0; i < 0x100000u; ++i) { if (a[i] != b[i]) { r0 = (a[i] < b[i]) ? (uint32_t)-1 : 1u; break; } if (!a[i]) break; } }
      break; }
    case 0x812B2290u: {                                               // memcmp(a,b,n) [NID 7747F6D7]
      uint8_t* a = GP(s->r[0]); uint8_t* b = GP(s->r[1]); uint32_t n = s->r[2]; r0 = 0;
      if (a && b) { for (uint32_t i = 0; i < n; ++i) { if (a[i] != b[i]) { r0 = (a[i] < b[i]) ? (uint32_t)-1 : 1u; break; } } }
      break; }
    case 0x812B2350u: {                                               // memmove(dst,src,n) [NID AF5C218D]
      uint32_t dst = s->r[0], src = s->r[1], n = s->r[2];
      uint8_t* d = GP(dst); uint8_t* q = GP(src);
      if (d && q && n < 0x1000000u) std::memmove(d, q, n);
      r0 = dst; break; }
    case 0x812B2310u: {                                               // strncpy(dst,src,n) [NID 9F87712D]
      uint32_t dst = s->r[0], src = s->r[1], n = s->r[2];
      uint8_t* d = GP(dst); uint8_t* q = GP(src);
      if (d && q && n < 0x1000000u) { uint32_t i = 0; for (; i < n && q[i]; ++i) d[i] = q[i]; for (; i < n; ++i) d[i] = 0; }
      r0 = dst; break; }
    case 0x812B23B0u: {                                               // strncmp(a,b,n) [NID E4299DCB]
      uint8_t* a = GP(s->r[0]); uint8_t* b = GP(s->r[1]); uint32_t n = s->r[2]; r0 = 0;
      if (a && b) { for (uint32_t i = 0; i < n; ++i) { if (a[i] != b[i]) { r0 = (a[i] < b[i]) ? (uint32_t)-1 : 1u; break; } if (!a[i]) break; } }
      break; }
    case 0x812B2320u: {                                               // snprintf(dst,size,fmt,...) [NID A1BFF606]
      r0 = (uint32_t)guest_format(s->r[0], s->r[1], s->r[2], s); break; }
    case 0x812B2300u: {                                               // __errno_loc [NID 9E248B76]
      static uint32_t g_errno_va = 0; if (!g_errno_va) g_errno_va = hle_malloc(16);
      r0 = g_errno_va; break; }
    case 0x812B2220u: r0 = 0; break;                                  // __cxa_atexit
    case 0x812B2230u: r0 = s->r[0]; break;                            // malloc_usable_size -> request size
    case 0x812B2380u: r0 = 0; break;                                  // clock
    case 0x812B2390u: { if (s->r[0]) { uint8_t* q = GP(s->r[0]); if (q) { uint32_t z = 0; std::memcpy(q, &z, 4); } } r0 = 0; break; }  // time
    case 0x812B2400u: r0 = 0; break;                                  // localtime
    case 0x812B2910u: r0 = hle_malloc(64); break;                      // sceKernelCreateLwMutex -> struct ptr
    case 0x812B2830u: {                                               // sceKernelLockLwMutex (real lock in host-thread mode, INCLUDING main thread: otherwise host threads and main race on guest heap)
      if (g_hostthreads) { uint32_t me = tl_ht_tid ? tl_ht_tid : 0x40010000u; uint32_t w = s->r[0]; auto& e = g_lwm_real[w];
        for (;;) { if (e.first == 0u || e.first == me || g_shutdown) break; std::this_thread::sleep_for(std::chrono::microseconds(200)); }
        e.first = me; ++e.second; }
      r0 = 0; break; }
    case 0x812B28A0u: {                                               // sceKernelUnlockLwMutex (real lock in host-thread mode, including main thread)
      if (g_hostthreads) { uint32_t w = s->r[0]; auto it = g_lwm_real.find(w); if (it != g_lwm_real.end()) { if (it->second.second > 1u) --it->second.second; else { it->second.first = 0u; it->second.second = 0u; } } }
      r0 = 0; break; }
    case 0x812B1910u: r0 = 0; break;                                   // sceAppUtilInit
    case 0x812B11B0u: {                                               // 18.4086: pre-registration init writes a NON-ZERO output
      { static int _gr=0; if (_gr++ < 4) { printf("[GXMREG] r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X\n", s->r[0],s->r[1],s->r[2],s->r[3],s->r[14]); fflush(stdout); } }
      uint32_t _out = s->r[0];
      uint8_t* _p = GP(_out);
      if (_p) { uint32_t _one = 1u; std::memcpy(_p, &_one, 4); }
      r0 = 0; break;
    }
    case 0x812B11A0u: r0 = 0; break;                                  // 18.4086: registration setup -> success
    case 0x812B2210u: r0 = 0; break;                                   // abort (should not happen)
    case 0x812B1860u: r0 = 0; break;                                   // sceSysmoduleLoadModule -> 0 success
    case 0x812B27B0u: {   // 18.4228c: the guest uses GetThreadId result as a POINTER base (BADMEM va = tid+2) -> return a real TLS-like guest object
      static uint32_t g_tid_ptr = 0; if (!g_tid_ptr) g_tid_ptr = hle_malloc(0x80u);
      if (g_tidp_n++ < 6) { printf("[TIDPTR] GetThreadId -> %08X (was 0x100/0x4001)\n", g_tid_ptr); fflush(stdout); }
      r0 = g_tid_ptr ? g_tid_ptr : 0x100u; break; }                              // sceKernelGetThreadId -> plausible tid
    case 0x812B22C0u: {
      printf("[EXITLOG] libc exit called lr=%08X hle_total=%d\n", s->r[14], g_hle_n);
      int cnt = g_hle_ring_n < 64 ? g_hle_ring_n : 64;
      for (int i = cnt-1; i >= 0; --i) { int idx = (g_hle_ring_n - cnt + i) & 127; printf("[EXITLOG]   #%d va=%08X lr=%08X\n", cnt-1-i, g_hle_ring_va[idx], g_hle_ring_lr[idx]); }
      fflush(stdout);
      r0 = 0; break; }
    case 0x812B1B80u: {                                               // sceFiberRun(fiber, ...): start/resume the fiber
      uint32_t fb = s->r[0] & ~1u;
      { static int _fr=0; if (_fr++ < 12) { printf("[FRUN] r0=%08X r1=%08X r2=%08X lr=%08X sp=%08X fib=%08X thr=%08X ring:", s->r[0], s->r[1], s->r[2], s->r[14], s->r[13], g_cur_fiber, g_cur_thr);
          for (int _k=0;_k<24;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 24u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout); } }
      if (fb < 0x80600000u || fb >= 0x8C600000u) { if (fb < 0xA0000000u || fb >= 0xA1000000u) {
        printf("[FRUN-BAD] invalid fiber %08X lr=%08X -> substitute g_fiber_list0\n", fb, s->r[14]); fflush(stdout);
        fb = g_fiber_list.empty() ? 0u : g_fiber_list[0]; s->r[0] = fb; } }
      if (g_fiber_dbg++ < 6) {
        uint32_t off = fb - IMG_VA_BASE; uint32_t f84=0,f88=0;
        if (off + 0xA0 <= IMG_SIZE) { std::memcpy(&f84, gp_img+off+0x84, 4); std::memcpy(&f88, gp_img+off+0x88, 4); }
        printf("[FIBER] run fiber=%08X entry=%08X [f84]=%08X [f88]=%08X\n", fb, g_fiber_entry, f84, f88);
      }
      if (g_in_fex >= 1) { g_fiber_switch_to = fb; r0 = 0; break; }   // 18.3901: defer non-nested switch to loop-top
      if (g_in_fiber_exec) { g_fiber_switch_to = fb; r0 = 0; break; }
      if (!nested_allowed()) { g_switch_pending = 1; g_switch_target = fb; r0 = 0; break; }   // 18.4217: synchronous switch request; consumed by the dispatcher
      g_in_fiber_exec = 1;
      rc_save(s); fiber_load(s, fb, g_fiber_entry, g_fiber_arg);
      r0 = fiber_exec_to_completion(s);   // 18.4168: arbitrary-block slicing reverted (unsafe resume); g_fex_budget stays 0
      rc_restore(s);
      g_in_fiber_exec = 0;
      // cooperative round-robin: every 3rd run, also advance another fiber (task deliverer / loader)
      if (g_fiber_list.size() > 1 && (++g_run_alt % 3) == 0) {
        uint32_t idx = (uint32_t)((g_run_alt / 3) % g_fiber_list.size());
        uint32_t fb2 = g_fiber_list[idx];
        if (fb2 != fb) {
      if (g_in_fex >= 1) { g_fiber_switch_to = fb2; r0 = 0; break; }   // 18.3901: defer non-nested switch to loop-top
          g_in_fiber_exec = 1;
          rc_save(s); fiber_load(s, fb2, g_fiber_entry, g_fiber_arg);
          printf("[SWR2] loaded fb2=%08X pc=%08X sp=%08X gbn=%d entry=%08X\n",
                 fb2, s->r[15], s->r[13], gbn(s->r[15]) ? 1 : 0, g_fiber_entry);
          fiber_exec_to_completion(s);
          rc_restore(s);
          g_in_fiber_exec = 0;
        }
      }
      break; }
    case 0x812B1BA0u: {                                               // sceFiberSwitch(fiber, ...): start/resume target
      { static int _fs=0; if (_fs++ < 16) { printf("[FSW] r0=%08X r1=%08X r2=%08X lr=%08X sp=%08X fib=%08X thr=%08X ring:", s->r[0], s->r[1], s->r[2], s->r[14], s->r[13], g_cur_fiber, g_cur_thr);
          for (int _k=0;_k<24;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 24u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout); } }
      if (g_swp_n < 40) { ++g_swp_n; printf("[SWP] fb=%08X in=%d cur=%08X sp=%08X\n", s->r[0] & ~1u, g_in_fiber_exec, g_cur_fiber, s->r[13]); }
      if (g_sw_dbg < 12) { ++g_sw_dbg; printf("[SWD] list=%zu in=%d alt=%d fb=%08X cur=%08X\n",
          g_fiber_list.size(), g_in_fiber_exec, g_sw_alt2, s->r[0] & ~1u, g_cur_fiber); }
      // cooperative round-robin at the main-loop level: every 100th switch also run another fiber
      if (false) {   // 18.4213: disable nested-exec round-robin (old-runtime semantics)
        uint32_t idx = (uint32_t)((g_sw_alt2 / 100) % g_fiber_list.size());
        uint32_t fb2 = g_fiber_list[idx];
        if (g_sw_trig < 12) { ++g_sw_trig; printf("[SWRR] alt2=%d list=%zu idx=%u fb2=%08X cur=%08X entry=%08X\n",
            g_sw_alt2, g_fiber_list.size(), idx, fb2, g_cur_fiber, g_fiber_entry); }
        if (fb2 != g_cur_fiber) {
      if (g_in_fex >= 1) { g_fiber_switch_to = fb2; r0 = 0; break; }   // 18.3901: defer non-nested switch to loop-top
          g_in_fiber_exec = 1;
          rc_save(s); fiber_load(s, fb2, g_fiber_entry, g_fiber_arg);
          printf("[SWR3] loaded fb2=%08X pc=%08X sp=%08X gbn=%d\n", fb2, s->r[15], s->r[13], gbn(s->r[15]) ? 1 : 0);
          fiber_exec_to_completion(s);
          printf("[SWR4] after exec pc=%08X r0=%08X cur=%08X\n", s->r[15], s->r[0], g_cur_fiber);
          // 18.3722: a fiber that ran to completion (pc=0) is restarted so its worker loop keeps running.
          if (s->r[15] == 0u) {
            uint32_t off2 = fb2 - IMG_VA_BASE; uint32_t entry2 = 0, argOn2 = 0;
            if (off2 + 0x90 <= IMG_SIZE) { std::memcpy(&entry2, gp_img + off2 + 0x84, 4); std::memcpy(&argOn2, gp_img + off2 + 0x88, 4); }
            if (entry2 && (entry2 >> 24) == 0x81u) {
              FCtx& c2 = g_fctx[fb2];
              c2.r[0] = argOn2 ? argOn2 : fb2;
              c2.r[14] = 0xFFFFFFF0u; c2.r[15] = entry2 & ~1u;
              printf("[SWRRESTART] fb2=%08X entry=%08X\n", fb2, entry2);
            }
          }
          rc_restore(s);
          g_in_fiber_exec = 0;
        }
      }
      uint32_t fb = s->r[0] & ~1u;
      if (!fb) { r0 = 0; break; }
      if (g_in_fiber_exec) {
        // 18.3812: switch requested from INSIDE a fiber. Do NOT run the blocking pump / restore-thread path
        // (it overwrote the current fiber registers with the main-thread snapshot, corrupting fiber1).
        // Defer the switch to fiber_exec_to_completion's switch handler instead.
        { bool _ok = (fb >= 0x80600000u && fb < 0x84600000u) || (fb >= 0x84600000u && fb < 0x8C600000u) || (fb >= 0xA0000000u && fb < 0xA1000000u);
          if (!_ok) { printf("[FSWSAN] invalid switch-request fiber %08X (cur=%08X) -> ignored\n", fb, g_cur_fiber); fflush(stdout); } else if (fb != g_cur_fiber) g_fiber_switch_to = fb; }
        r0 = 0; break;
      }
      if (fb == g_cur_fiber) {                                         // self-switch/yield
        // cooperative round-robin: every 50th self-switch, run the next fiber (task deliverer / loader)
        if (false) {   // 18.4213: disable nested-exec round-robin (old-runtime semantics)
          uint32_t idx = (uint32_t)((g_sw_alt / 50) % g_fiber_list.size());
          uint32_t fb2 = g_fiber_list[idx];
          if (fb2 != g_cur_fiber) {
      if (g_in_fex >= 1) { g_fiber_switch_to = fb2; r0 = 0; break; }   // 18.3901: defer non-nested switch to loop-top
            g_in_fiber_exec = 1;
            rc_save(s); fiber_load(s, fb2, g_fiber_entry, g_fiber_arg);
            fiber_exec_to_completion(s);
            rc_restore(s);
            g_in_fiber_exec = 0;
          }
        }
        r0 = 0; break; }
      if (g_in_fex >= 1) { g_fiber_switch_to = fb; r0 = 0; break; }   // 18.3901: defer non-nested switch to loop-top
      if (g_in_fiber_exec) { g_fiber_switch_to = fb; r0 = 0; break; }   // handled by the outer exec loop
      if (!nested_allowed()) { g_switch_pending = 1; g_switch_target = fb; r0 = 0; break; }   // 18.4217: synchronous switch request; consumed by the dispatcher with the real next-pc
      g_in_fiber_exec = 1;
      rc_save(s); fiber_load(s, fb, g_fiber_entry, g_fiber_arg);
      r0 = fiber_exec_to_completion(s);
      rc_restore(s);
      g_in_fiber_exec = 0;
      break; }
    case 0x812B1B60u: r0 = 0; g_fiber_flag_return = 1; break;         // sceFiberReturnToThread
    case 0x812B1BB0u: {                                               // _sceFiberInitializeImpl(fiber, name, entry, arg, ...)
      if (g_finit_n++ < 10) {
        uint32_t f84 = 0, f88 = 0; uint32_t off0 = s->r[0] - IMG_VA_BASE;
        if (off0 + 0x90 <= IMG_SIZE) { std::memcpy(&f84, gp_img+off0+0x84, 4); std::memcpy(&f88, gp_img+off0+0x88, 4); }
        printf("[FINIT] fiber=%08X name=%08X entry=%08X arg=%08X [f84]=%08X [f88]=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], f84, f88);
      }
      uint32_t fb0_return = s->r[0];
      { uint32_t fb0 = s->r[0]; bool found = false; for (uint32_t x : g_fiber_list) if (x == fb0) found = true;
        if (!found) g_fiber_list.push_back(fb0); }
      if (g_fiblog_n++ < 10) { printf("[FIBINIT] fiber=%08X entry=%08X arg=%08X list=%zu\n", s->r[0], s->r[2], s->r[3], g_fiber_list.size()); fflush(stdout); }
      g_fiber_entry = s->r[2]; g_fiber_arg = s->r[3]; r0 = 0; break; }   // 0 = success (wrapper checks cmp r0,#0)
    case 0x812B1B70u:                                                 // sceFiberGetSelf(out): return the CURRENT fiber object
      { // 18.3768: real sceFiberGetSelf returns the CURRENT fiber, i.e. 0 when not running inside a fiber.
        // Inside fiber_exec return the real fiber object (fiber list mgmt needs non-zero); at thread level return 0
        // so the game's main loop takes its "no current fiber" exit path.
        static uint32_t g_dummy_getself = 0;
        uint32_t f;
        if (g_in_fiber_exec) {
          f = (g_cur_fiber ? g_cur_fiber : (g_last_fiber ? g_last_fiber : (g_fiber_list.empty() ? 0x82F00000u : g_fiber_list[0])));
        } else {
          // 18.3904: returning 0 at thread level made the game dereference NULL (ldr r3,[sp]=0; ldr r2,[r3+0x94]
          // -> BADMEM 0x94 at block 0x812067B6). Historically (M138/M143) a dummy fiber pointer activated the
          // fiber path; use a real allocated dummy object with [obj+0x80]|=1 (active) instead of 0.
          if (!g_dummy_getself) {
            g_dummy_getself = hle_malloc(0xA0u);
            if (g_dummy_getself) { uint8_t* q0 = GP(g_dummy_getself + 0x80u); if (q0) { uint32_t one = 1u; std::memcpy(q0, &one, 4); } }
          }
          f = g_dummy_getself ? g_dummy_getself : 0x82F00000u;
          if (g_dummy_gs_n++ < 5) printf("[GETSELF-DUMMY] thread-level -> %08X\n", f);
        }
        if (g_gs_dbg_n++ < 40) printf("[GSDBG] in_exec=%d out=%08X f=%08X cur=%08X\n", g_in_fiber_exec, s->r[0], f, g_cur_fiber);
        { uint32_t i = g_gsr_n++ % 24u; g_gsr_in[i] = (uint32_t)g_in_fiber_exec; g_gsr_out[i] = s->r[0]; g_gsr_f[i] = f; }
        if (s->r[0]) { uint8_t* p = GP(s->r[0]); if (p) std::memcpy(p, &f, 4); } if (s->r[0] == 0xA0DFFFD0u) { g_ws_addr = 0xA0DFFFD0u; g_ws_on = 1; uint8_t* _q = GP(g_ws_addr); if (_q) std::memcpy(&g_ws_snap, _q, 4); printf("[WS-ARM] armed at GetSelf out=A0DFFFD0 snap=%08X\n", g_ws_snap); }
        // 18.3774: PUMP the second fiber. The game created fiber2 but our harness only ever ran fiber1, so the
        // scheduler waited forever. At thread level (GetSelf returns 0) run fiber2 to completion once.
        if (nested_allowed() && !g_pumped && g_fiber_list.size() > 1 && g_fiber_list[1] != f) {   // 18.4221: no nested exec after the hybrid cutoff
          g_pumped = 1;
          g_in_fiber_exec = 1;
          g_pt_on = 1;
          { auto itp = g_fctx.find(g_fiber_list[1]);
            if (itp != g_fctx.end() && g_pd_n++ < 10)
              printf("[FCTX] fiber2=%08X pc=%08X sp=%08X r0=%08X r14=%08X\n",
                     g_fiber_list[1], itp->second.r[15], itp->second.r[13], itp->second.r[0], itp->second.r[14]); }
      if (g_in_fex >= 1) { g_fiber_switch_to = g_fiber_list[1]; r0 = 0; break; }   // 18.3901: defer non-nested switch to loop-top
          fiber_load(s, g_fiber_list[1], g_fiber_entry, g_fiber_arg);
          uint32_t pr = fiber_exec_to_completion(s);
          g_pt_on = 0;
          g_in_fiber_exec = 0;
          if (g_pump_n++ < 5) printf("[PUMP] fiber2=%08X ran -> pc=%08X ret=%08X listhead=%08X\n",
                 g_fiber_list.size()>1?g_fiber_list[1]:0, s->r[15], pr, 0u);
        }
        // 18.3781: the loader 0x811E5CDA'|s callback [0x81513DEC] is default-NULL by design (init function
        // 0x811E5C0A stores 0 there), so fiber1's loop is the game's per-frame pump. Re-pump fiber2 periodically
        // so the game keeps making frame progress instead of idling forever.
        if (nested_allowed() && g_fiber_list.size() > 1 && (++g_pump_tick % 20000) == 0) {   // 18.4221: no nested exec after the hybrid cutoff
          g_in_fiber_exec = 1;
          fiber_load(s, g_fiber_list[1], g_fiber_entry, g_fiber_arg);
          uint32_t pr2 = fiber_exec_to_completion(s);
          g_in_fiber_exec = 0;
          if (g_pump_n < 12) { ++g_pump_n; printf("[PUMP2] tick=%u fiber2=%08X pc=%08X ret=%08X\n",
                 g_pump_tick, g_fiber_list[1], s->r[15], pr2); }
        }
        if (g_tbl_n++ < 6) {
          uint32_t t[8] = {0,0,0,0,0,0,0,0};
          for (int k = 0; k < 8; ++k) { uint8_t* q = GP(0x81513DD4u + (uint32_t)k*4u); if (q) std::memcpy(&t[k], q, 4); }
          printf("[TBL] %08X %08X %08X %08X %08X %08X %08X %08X\n", t[0],t[1],t[2],t[3],t[4],t[5],t[6],t[7]); }
        if (g_gs_n++ < 25) {
          uint32_t fh = 0; { uint8_t* q = GP(0x816CBC08u + 0xA18u); if (q) std::memcpy(&fh, q, 4); }
          uint32_t f80 = 0; { if (fh) { uint8_t* q2 = GP(fh + 0x80u); if (q2) std::memcpy(&f80, q2, 4); } }
          uint32_t st1 = 0, st2 = 0;
          if (g_fiber_list.size() > 0) { uint8_t* q = GP(g_fiber_list[0] + 0x80u); if (q) std::memcpy(&st1, q, 4); }
          if (g_fiber_list.size() > 1) { uint8_t* q = GP(g_fiber_list[1] + 0x80u); if (q) std::memcpy(&st2, q, 4); }
          printf("[GSELF] out=%08X ret=%08X listhead=%08X head80=%08X cur=%08X f1=%08X/st=%08X f2=%08X/st=%08X\n",
                 s->r[0], f, fh, f80, g_cur_fiber,
                 g_fiber_list.size()>0?g_fiber_list[0]:0, st1, g_fiber_list.size()>1?g_fiber_list[1]:0, st2); }
        r0 = 0; break; }
    case 0x812B29F0u: {                                               // sceKernelAllocMemBlock(name,type,size,opt) -> uid
      uint32_t size = s->r[2];
      if (size > 0x4000000u) { printf("[MEMA-BIG] size=%08X name=%08X lr=%08X\n", size, s->r[0], s->r[14]); fflush(stdout); }
      uint32_t addr = hle_malloc(size + 0x100u);
      uint32_t uid = ++g_memuid;
      g_memblks[uid] = std::make_pair(addr, size);
      printf("[MEMA] AllocMemBlock uid=%08X size=%08X addr=%08X name=%08X lr=%08X\n", uid, size, addr, s->r[0], s->r[14]); fflush(stdout);
      if (g_memdbg_n++ < 12) printf("[MEMBLK] uid=%08X size=%08X addr=%08X name=%08X\n", uid, size, addr, s->r[0]);
      r0 = uid; break; }
    case 0x812B29E0u: {                                               // sceKernelGetMemBlockInfo(uid, info) -> 0
      auto it = g_memblks.find(s->r[0]);
      if (it == g_memblks.end()) { printf("[MEMB] GetMemBlockInfo uid=%08X MISS (info=%08X) lr=%08X\n", s->r[0], s->r[1], s->r[14]); fflush(stdout); }
      if (it != g_memblks.end() && s->r[1]) {
        uint8_t* p = GP(s->r[1]);
        if (p) { uint32_t vals[8] = { it->second.first, it->second.second, it->second.second, 0, 0, 0, 0, 0 };
          std::memcpy(p, vals, 32); }   // 18.3758: game reads [info+0] as the mapped BASE */
        printf("[MEMB] GetMemBlockInfo uid=%08X -> base=%08X size=%08X info=%08X lr=%08X hit=1\n", s->r[0], it->second.first, it->second.second, s->r[1], s->r[14]); fflush(stdout);
        if (g_memdbg_n < 20) { printf("[MEMINFO] uid=%08X -> base=%08X size=%08X info=%08X\n", s->r[0], it->second.first, it->second.second, s->r[1]); }
      }
      r0 = 0; break; }
    case 0x812B1580u: {                                               // 18.4192: sceGxmGetRenderTargetMemSize (B291C959) - game reads the size from the OUT pointer in r1 too
      uint32_t sz = 0x33D000u;   // ARCH_10 empirical 960x544 scenes=3 msaa=2 tiles=30x17 -> 0x33D000
      uint8_t* out = GP(s->r[1]);
      if (out) std::memcpy(out, &sz, 4);
      if (g_rt_n++ < 12) {
        uint32_t _p[8] = {0,0,0,0,0,0,0,0}; uint8_t* pp = GP(s->r[0]);
        if (pp) std::memcpy(_p, pp, 32);
        printf("[RTMEM] size=%08X params=%08X out=%08X lr=%08X p=%08X/%08X/%08X/%08X/%08X/%08X/%08X/%08X\n",
               sz, s->r[0], s->r[1], s->r[14], _p[0],_p[1],_p[2],_p[3],_p[4],_p[5],_p[6],_p[7]); fflush(stdout);
      }
      r0 = sz; break; }
    case 0x812B1640u: r0 = 0; break;                                  // sceGxmMapVertexUsseMem -> 0 (success)
    case 0x812B1830u:                                                 // sceCtrlPeekBufferPositive(pad,count)
    case 0x812B1840u: {                                               // sceCtrlReadBufferPositive(pad,count)
      // 18.3807: inject a "confirm" button state so the game's start/menu logic can fire.
      // First 200 polls report CROSS+START pressed, then release (simulates a click).
      if (s->r[0]) { uint8_t* p = GP(s->r[0]);
        if (p) { uint32_t vals[4] = { 0u, (g_ctrl_n < 200) ? 0x4008u : 0u, 0u, 0u }; std::memcpy(p, vals, 16); } }
      if (g_ctrl_n++ < 6) printf("[CTRL] poll #%d -> buttons=%08X\n", g_ctrl_n, (g_ctrl_n <= 200) ? 0x4008u : 0u);
      r0 = 0; break; }
    case 0x812B2280u: {                                               // 18.3838: path formatter (dest,fmt,r2,r3,[sp]...) - %s only
      uint8_t* dst = GP(s->r[0]);
      uint8_t* fmt = GP(s->r[1]);
      uint32_t args[8] = { s->r[2], s->r[3], 0, 0, 0, 0, 0, 0 };
      { uint8_t* sp = GP(s->r[13]); if (sp) for (int k = 0; k < 4; ++k) { uint32_t v; std::memcpy(&v, sp + k * 4, 4); args[2 + k] = v; } }
      uint32_t oi_out = 0;
      if (dst && fmt) {
        char out[512]; int oi = 0; int ai = 0;
        for (int i = 0; fmt[i] && oi < 500; ++i) {
          if (fmt[i] == '%' && fmt[i+1]) {
            char spec = (char)fmt[i+1];
            if (spec == '%') { out[oi++] = '%'; ++i; continue; }
            uint32_t av = (ai < 8) ? args[ai] : 0u; ++ai;
            if (spec == 's') {
              uint8_t* src = GP(av);
              if (src) { for (int k = 0; src[k] && oi < 500; ++k) out[oi++] = (char)src[k]; }
            } else if (spec == 'd' || spec == 'i') {
              oi += std::snprintf(out + oi, 32, "%d", (int)av); if (oi > 500) oi = 500;
            } else if (spec == 'u') {
              oi += std::snprintf(out + oi, 32, "%u", av); if (oi > 500) oi = 500;
            } else if (spec == 'x') {
              oi += std::snprintf(out + oi, 32, "%x", av); if (oi > 500) oi = 500;
            } else if (spec == 'X') {
              oi += std::snprintf(out + oi, 32, "%X", av); if (oi > 500) oi = 500;
            } else if (spec == 'c') {
              out[oi++] = (char)(av & 0xFF);
            } else { out[oi++] = '%'; out[oi++] = spec; }
            ++i;
          } else out[oi++] = (char)fmt[i];
        }
        out[oi] = 0;
        std::memcpy(dst, out, (size_t)oi + 1);
        oi_out = (uint32_t)oi;
        if (g_path_n++ < 12) printf("[PATH] '%s' args=%08X,%08X,%08X,%08X,%08X\n", out, args[0], args[1], args[2], args[3], args[4]);
      }
      if (g_pfmt_n < 6) { printf("[PFMT2] dst=%08X fmt=%08X oi=%u ret=%u\n", s->r[0], s->r[1], oi_out, oi_out); fflush(stdout); }
      r0 = oi_out; break; }   // 18.4189: formatter returns the number of bytes written (caller does ptr += r0)
    case 0x812B1800u: {                                               // 18.4243: sceDisplaySetFrameBuf (NID 7A410B64, SceDisplayUser) - REAL mapping
      g_fb_addr = s->r[0]; g_fb_pitch = s->r[1];
      if (g_fb_set_n++ < 10) printf("[DISPLAY] SetFrameBuf addr=%08X pitch=%08X fmt=%08X sync=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3]);
      r0 = 0; break; }
    case 0x812B1810u: {                                               // 18.4243: sceDisplayWaitVblankStartMultiCB (NID 05F27764) - NOT GetFrameBuf!
      uint32_t cnt = s->r[0]; if (cnt == 0u) cnt = 1u; if (cnt > 4u) cnt = 4u;
      ++g_vblank_n;
      if (tl_ht_tid) std::this_thread::sleep_for(std::chrono::microseconds(16000u * cnt));
      if (g_vblank_n <= 8) { printf("[VBLANK] count=%u n=%d tid=%08X\n", s->r[0], g_vblank_n, tl_ht_tid); fflush(stdout); }
      r0 = 0; break; }
    case 0x812B1820u: {                                               // 18.4243: sceDisplayWaitSetFrameBufMulti (NID 7D9864A8) - NOT SetFrameBuf
      if (tl_ht_tid) std::this_thread::sleep_for(std::chrono::microseconds(1000));
      r0 = 0; break; }
    case 0x812B2410u: {                                               // 18.4243: strncat (NID FBA69BC2, SceLibc) - was missing
      uint32_t dst = s->r[0], src = s->r[1], n = s->r[2];
      uint8_t* dq = GP(dst); uint8_t* sq = GP(src);
      if (dq && sq) { size_t dl = std::strlen((const char*)dq); size_t k = 0; while (k < n && sq[k]) { dq[dl + k] = sq[k]; ++k; } dq[dl + k] = 0; }
      r0 = dst; break; }
    case 0x812B16F0u: r0 = 0; g_gxm_ctx_n++; if (g_gxm_ctx_n <= 3) printf("[GXM-CTX] CreateContext n=%d\n", g_gxm_ctx_n); break;
      { static int _gb=0; if (_gb++ < 4) { printf("[GXMBLK] #%d n=%u pc=%08X\n", _gb, g_blkcnt_total, g_cur_block); fflush(stdout); } }
    case 0x812B1470u: gxmvk_begin_frame(); r0 = 0; g_gxm_begin_n++; if (g_gxm_begin_n <= 3) printf("[GXM-BEGIN] BeginScene n=%d\n", g_gxm_begin_n); break;
    case 0x812B17F0u: gxmvk_end_frame(); r0 = 0; g_gxm_end_n++; if (g_gxm_end_n <= 5) printf("[GXM-ENDSCENE] EndScene n=%d\n", g_gxm_end_n); break;
    case 0x812B1720u: gxmvk_present_now(); r0 = 0; g_gxm_present_n++; if (g_gxm_present_n <= 10) printf("[GXM-PRESENT] DisplayQueueAddEntry n=%d\n", g_gxm_present_n); break;
    case 0x812B15E0u: r0 = 0; if (g_gxm_present_n && g_gxm_fin_n++ < 5) printf("[GXM-DQFIN] DisplayQueueFinish n=%d\n", g_gxm_fin_n); break;
    case 0x812B1730u: {                                               // sceGxmColorSurfaceInit(surface, fmt, ...)
      uint32_t surf = s->r[0]; uint8_t* p = surf ? GP(surf) : nullptr;
      if (p) { uint32_t w[8] = { s->r[1], s->r[5], s->r[4], s->r[6], s->r[7], s->r[3], 0, 0 };
        std::memcpy(p, w, sizeof(w)); }
      if (g_gxm_n++ < 12) printf("[GXM] ColorSurfaceInit surf=%08X fmt=%08X w=%u h=%u stride=%u\n", surf, s->r[1], s->r[5], s->r[4], s->r[6]);
      r0 = 0; break; }
    case 0x812B13D0u: {                                               // sceGxmSyncObjectCreate(&sync)
      if (s->r[0]) { uint32_t obj = hle_malloc(0x40u); uint8_t* p = GP(s->r[0]); if (p) std::memcpy(p, &obj, 4);
        if (g_gxm_n++ < 12) printf("[GXM] SyncObjectCreate -> obj=%08X\n", obj); }
      r0 = 0; break; }
    case 0x812B1340u: {                                               // sceGxmTextureInitLinear(tex, data, fmt, w, h, ...)
      uint32_t tex = s->r[0]; uint8_t* p = tex ? GP(tex) : nullptr;
      if (p) { uint32_t w[8] = { s->r[1], s->r[2], s->r[3], s->r[4], s->r[5], 0, 0, 0 };
        std::memcpy(p, w, sizeof(w)); }
      if (g_gxm_n++ < 12) printf("[GXM] TextureInitLinear tex=%08X data=%08X fmt=%08X w=%u h=%u\n", tex, s->r[1], s->r[2], s->r[3], s->r[4]);
      r0 = 0; break; }
    case 0x812B1660u: {                                               // sceGxmDepthStencilSurfaceInit(surface, ...)
      uint32_t surf = s->r[0]; uint8_t* p = surf ? GP(surf) : nullptr;
      // 18.4143: the real surface struct's +0x14 holds the depthData pointer that the game fills right
      // before this call; the old 8-word memcpy zeroed it (that was the gsc+0x304 clobber).
      if (p) { uint32_t w[5] = { s->r[1], s->r[2], s->r[3], s->r[4], s->r[5] };
        std::memcpy(p, w, 0x14u); }
      if (g_gxm_n++ < 12) printf("[GXM] DepthStencilInit surf=%08X fmt=%08X\n", surf, s->r[1]);
      r0 = 0; break; }
    case 0x812B19B0u: fios_ensure_cpk(); r0 = 0; break;                // sceFiosInitialize
    case 0x812B1970u: r0 = 0; break;                                  // sceFiosTerminate
    case 0x812B19F0u: {                                               // sceFiosFHOpen(op, fhPtr, path, params)
      fios_ensure_cpk();
      std::string path; { uint8_t* p = GP(s->r[2]); if (p) path = (const char*)p; }
      uint32_t fh = 0xFFFFFFFFu, op = ++g_op_id;
      std::vector<uint8_t> data;
      if (!path.empty() && cpk_read_any_impl(path, data)) {
        fh = ++g_fh_id; g_fios_data[fh] = std::move(data); g_fios_pos[fh] = 0;
        printf("[FIOS] OPEN '%s' -> fh=0x%X size=%zu\n", path.c_str(), fh, g_fios_data[fh].size());
      } else {
        // 18.3798: a MISS returned fh=0xFFFFFFFF and made the game's async op fail (status 6) -> it retried
        // forever. Serve a valid EMPTY handle instead so the loader can make progress (content gaps are logged).
        fh = ++g_fh_id; g_fios_data[fh] = std::vector<uint8_t>(); g_fios_pos[fh] = 0;
        if (g_fiosmiss_n++ < 40) printf("[FIOS] OPEN-MISS '%s' -> empty fh=0x%X\n", path.c_str(), fh);
      }
      if (s->r[1]) { uint8_t* q = GP(s->r[1]); if (q) std::memcpy(q, &fh, 4); }
      hle_eflag_set(0x60014000u, 1u); hle_eflag_set(0x60015000u, 1u);   // 18.4264: wake guest Fios worker threads (op completion notification)
      r0 = op; break; }
    case 0x812B19D0u: {                                               // sceFiosFHRead(op, fh, dst, ...; size at [sp])
      ++g_fread_n;
      auto it = g_fios_data.find(s->r[1]);
      uint32_t size = 0; { uint8_t* q = GP(s->r[13]); if (q) std::memcpy(&size, q, 4); }
      if (it == g_fios_data.end()) { r0 = 0; break; }
      size_t& pos = g_fios_pos[s->r[1]];
      size_t avail = it->second.size() - pos;
      uint32_t n = (uint32_t)(size < avail ? size : avail);
      uint8_t* dst = GP(s->r[2]);
      if (dst && n) std::memcpy(dst, it->second.data() + pos, n);
      pos += n; r0 = n; g_fread_bytes += n;
      hle_eflag_set(0x60014000u, 1u); hle_eflag_set(0x60015000u, 1u);   // 18.4264: op completion notification
      break; }
    case 0x812B1A00u: { auto it = g_fios_data.find(s->r[0]); r0 = (it == g_fios_data.end()) ? 0u : (uint32_t)it->second.size(); break; } // sceFiosFHGetSize
    case 0x812B2970u: {                                               // 18.4255: sceIoOpen (SceIofilemgr, NID 0x34EFD876) - CRI/GFD resource loader path
      fios_ensure_cpk();
      std::string path; { uint8_t* p = GP(s->r[0]); if (p) path = (const char*)p; }
      std::vector<uint8_t> data;
      uint32_t fd;
      if (!path.empty() && cpk_read_any_impl(path, data)) { fd = ++g_io_id; g_io_data[fd] = std::move(data); g_io_pos[fd] = 0;
        if (g_io_n++ < 60) printf("[IO] OPEN '%s' -> fd=%u size=%zu\n", path.c_str(), fd, g_io_data[fd].size()); }
      else { fd = ++g_io_id; g_io_data[fd] = std::vector<uint8_t>(); g_io_pos[fd] = 0;
        if (g_io_n++ < 60) printf("[IO] OPEN-MISS '%s' -> empty fd=%u\n", path.c_str(), fd); }
      r0 = fd; break; }
    case 0x812B29B0u: {                                               // sceIoRead(fd, buf, size)
      auto it = g_io_data.find(s->r[0]); if (it == g_io_data.end()) { r0 = 0; break; }
      size_t& pos = g_io_pos[s->r[0]]; size_t avail = it->second.size() - pos;
      uint32_t n = (uint32_t)(s->r[2] < avail ? s->r[2] : avail);
      uint8_t* dst = GP(s->r[1]); if (dst && n) std::memcpy(dst, it->second.data() + pos, n);
      pos += n; r0 = n; break; }
    case 0x812B2990u: {                                               // sceIoLseek32(fd, offset, whence)
      auto it = g_io_data.find(s->r[0]); if (it == g_io_data.end()) { r0 = 0xFFFFFFFFu; break; }
      int32_t off = (int32_t)s->r[1]; size_t& pos = g_io_pos[s->r[0]];
      int64_t np = (s->r[2] == 0 ? 0 : s->r[2] == 1 ? (int64_t)pos : (int64_t)it->second.size()) + off;
      if (np < 0) np = 0; pos = (size_t)np; r0 = (uint32_t)pos; break; }
    case 0x812B29A0u: case 0x812B2980u:                               // sceIoClose / sceIoDclose
      g_io_data.erase(s->r[0]); g_io_pos.erase(s->r[0]); r0 = 0; break;
    case 0x812B19C0u: case 0x812B19A0u: g_fios_data.erase(s->r[1]); g_fios_pos.erase(s->r[1]); r0 = 0; break;  // FHClose / FHCloseSync
    case 0x812B1960u: r0 = 1; break;                                  // sceFiosOpIsDone -> done
    case 0x812B1980u: hle_eflag_set(0x60014000u, 1u); hle_eflag_set(0x60015000u, 1u); r0 = 0; break;  // sceFiosOpWait (+ op completion notify)
    case 0x812B1990u: r0 = 0; break;                                  // sceFiosOpCancel
    case 0x812B19E0u: r0 = 0; break;                                  // sceFiosOpGetError
    case 0x812B1A10u: r0 = 0; break;                                  // sceFiosOpDelete
    case 0x812B2900u: {                                               // sceKernelCreateThread
      uint32_t entry = s->r[1];
      uint32_t tid = (g_next_tid += 0x1000u);
      if (entry != 0x811E68C9u && entry != 0x8120767Du) printf("[THR2] create tid=%08X entry=%08X lr=%08X sp=%08X\n", tid, entry, s->r[14], s->r[13]);
      if (g_tc_n++ < 40) { printf("[TCREATE2] tid=%08X entry=%08X prio=%08X stack=%08X lr=%08X thr=%08X\n", tid, entry, s->r[2], s->r[3], s->r[14], g_cur_thr); fflush(stdout); }
      if (tid) { HThread& t = g_thr[tid]; std::memset(&t, 0, sizeof(t)); t.entry = entry; t.alive = 1;
        if (g_thr_log_n++ < 20) printf("[THR] create tid=%08X entry=%08X lr=%08X\n", tid, entry, s->r[14]); }
      r0 = tid; break; }
    case 0x812B2940u: {                                               // sceKernelStartThread(tid, arglen, argp)
      uint32_t tid = s->r[0], arglen = s->r[1], argp = s->r[2];
      auto it = g_thr.find(tid);
      if (it != g_thr.end()) { HThread& t = it->second;
        t.arg = argp;
        if (!t.stack) { uint32_t base = hle_malloc(0x40000); t.stack = base ? (base + 0x3F000u) : 0xA0F00000u; }
        std::memset(&t.ctx, 0, sizeof(t.ctx));
        // Vita thread entry signature: int entry(SceSize args, void *argp) -> r0=arglen, r1=argp
        t.ctx.r[13] = t.stack; t.ctx.r[14] = 0xFFFFFFF0u; t.ctx.r[15] = t.entry & ~1u;
        // [THRARG] copy the argument block out of the creator's (soon-reused) stack frame
        uint32_t argbuf = argp;
        if (arglen && argp) { uint32_t nb = hle_malloc(arglen); if (nb) {
          uint8_t* src = GP(argp); uint8_t* dst = GP(nb);
          if (src && dst) { std::memcpy(dst, src, arglen); argbuf = nb; 
            printf("[THRARG] copied arglen=%u argp=%08X -> %08X val0=%08X\n", arglen, argp, argbuf, *(uint32_t*)dst); } } }
        t.ctx.r[0] = arglen; t.ctx.r[1] = argbuf;
        t.alive = 1; t.ready = 1; t.waiting = 0;
        if (g_hostthreads) {
          t.gctx = RecState(); std::memset(&t.gctx, 0, sizeof(t.gctx));
          t.gctx.base = s->base; t.gctx.mem_size = s->mem_size;   // 18.4239: guest memory mapping must be set per thread
          t.gctx.r[13] = t.ctx.r[13]; t.gctx.r[14] = 0xFFFFFFF0u; t.gctx.r[15] = t.ctx.r[15];
          t.gctx.r[0] = t.ctx.r[0]; t.gctx.r[1] = t.ctx.r[1]; t.gctx.r[9] = t.ctx.r[9];
          { uint32_t tls = hle_malloc(0x400u); if (tls) { uint8_t* tq = GP(tls); if (tq) std::memset(tq, 0, 0x400); } t.gctx.r[9] = tls; }
          t.exit_flag = 0;
          if (!t.hth) { t.hth = new std::thread(guest_thread_run, tid); }   // 18.4251: keep joinable for clean shutdown
          if (g_ht_n++ < 20) { printf("[HT-START] tid=%08X entry=%08X sp=%08X\n", tid, t.ctx.r[15], t.ctx.r[13]); fflush(stdout); }
        }
        if (t.entry != 0x811E68C9u && t.entry != 0x8120767Du) printf("[THR2] start tid=%08X entry=%08X arglen=%u argp=%08X lr=%08X sp=%08X\n", tid, t.entry, arglen, argp, s->r[14], t.stack);
        if (g_thr_log_n++ < 20) printf("[THR] start tid=%08X entry=%08X arglen=%u argp=%08X sp=%08X\n", tid, t.entry, arglen, argp, t.stack); }
      r0 = 0; break; }
    case 0x812B27C0u: {                                               // sceKernelCreateSema
      uint32_t cnt = s->r[2];   // 18.4232: sceKernelCreateSema(name,attr,initCount,maxCount) - initCount is r2, NOT r1(attr)
      uint32_t obj = (g_next_sema += 0x1000u);
      if (obj) g_sema[obj] = cnt;
      { uint32_t _np = s->r[0]; char _nm[48]; int _i=0; uint8_t* _q = GP(_np); if (_q) { for (; _i<40; ++_i) { char _c=(char)_q[_i]; if(!_c) break; _nm[_i] = (_c>=32&&_c<127)?_c:'.'; } } _nm[_i]=0; if (g_sc_n++ < 30) { printf("[SCREATE] obj=%08X init=%u name='%s' lr=%08X thr=%08X\n", obj, cnt, _nm, s->r[14], g_cur_thr); fflush(stdout); } }
      if (g_thr_log_n++ < 20) printf("[THR] create_sema obj=%08X init=%u\n", obj, cnt);
      r0 = obj; break; }
    case 0x812B27A0u: {                                               // sceKernelWaitSema
      uint32_t obj = s->r[0], need = s->r[1] ? s->r[1] : 1u;
      auto it = g_sema.find(obj);
      uint32_t have = (it == g_sema.end()) ? 0u : it->second;
      if (g_thr_log_n++ < 80) printf("[SEM] wait obj=%08X need=%u have=%u cur_thr=%08X lr=%08X\n", obj, need, have, g_cur_thr, s->r[14]);
      if (tl_ht_tid) { for (;;) { if (g_shutdown) break; uint32_t _h = (g_sema.count(obj) ? g_sema[obj] : 0u); if (_h >= need) { g_sema[obj] = _h - need; break; } std::this_thread::sleep_for(std::chrono::microseconds(200)); } r0 = 0; break; }
      if (g_sem2_n++ < 60) { printf("[SEM2] wait obj=%08X need=%u have=%u thr=%08X lr=%08X\n", obj, need, have, g_cur_thr, s->r[14]); fflush(stdout); }
      if (have >= need) { if (it != g_sema.end()) it->second = have - need; r0 = 0; }
      else if (g_cur_thr) { g_thr_blocked = 1; g_thr_wait_obj = obj; g_thr_wait_bits = 0; r0 = 0; }
      else {   // main-block-pump: main thread must BLOCK by running workers until sema satisfied
        int _spins = 0;
        while (_spins++ < 4000) {
          uint32_t _c = (g_sema.count(obj) ? g_sema[obj] : 0u);
          if (_c >= need) break;
          hle_thr_pump(s, 400);
          if (g_thr.empty()) break;
        }
        uint32_t _c2 = (g_sema.count(obj) ? g_sema[obj] : 0u);
        if (_c2 >= need) g_sema[obj] = _c2 - need;
        if (g_thr_log_n++ < 80) printf("[SEM] main-block obj=%08X need=%u got=%u spins=%d\n", obj, need, _c2, _spins);
        r0 = 0;
      }
      break; }
    case 0x812B2AD0u: {                                               // sceKernelSignalSema
      uint32_t obj = s->r[0];
      g_sema[obj] += 1;
      if (g_thr_log_n++ < 80) printf("[SEM] signal obj=%08X count=%u cur_thr=%08X lr=%08X\n", obj, g_sema[obj], g_cur_thr, s->r[14]);
      if (g_sig2_n++ < 60) { printf("[SIG2] obj=%08X count=%u thr=%08X lr=%08X\n", obj, g_sema[obj], g_cur_thr, s->r[14]); fflush(stdout); }
      for (auto& kv : g_thr) if (kv.second.waiting && kv.second.wait_obj == obj) { kv.second.waiting = 0; kv.second.ready = 1; }
      r0 = 0; break; }
    case 0x812B2890u: {                                               // sceKernelCreateEventFlag
      uint32_t init = s->r[2];   // 18.4232: sceKernelCreateEventFlag(name,attr,initPattern,opt) - init pattern is r2
      uint32_t obj = (g_next_eflag += 0x1000u);
      if (obj) g_eflag[obj] = init;
      { uint32_t _np = s->r[0]; char _nm[48]; int _i=0; uint8_t* _q = GP(_np); if (_q) { for (; _i<40; ++_i) { char _c=(char)_q[_i]; if(!_c) break; _nm[_i] = (_c>=32&&_c<127)?_c:'.'; } } _nm[_i]=0; if (g_efc_n++ < 30) { printf("[EFCREATE] obj=%08X init=%08X name='%s' lr=%08X thr=%08X\n", obj, init, _nm, s->r[14], g_cur_thr); fflush(stdout); } }
      if (g_thr_log_n++ < 20) printf("[THR] create_eflag obj=%08X init=%08X\n", obj, init);
      r0 = obj; break; }
    case 0x812B2930u:                                                 // 18.4259: sceKernelWaitEventFlagCB (NID E737B1DF) - same semantics as WaitEventFlag
    case 0x812B2880u: {                                               // 18.4234: sceKernelWaitEventFlag - old-runtime mode semantics (bit0=OR, bit2=CLEAR, bit3? old uses 4=CLEAR_ALL, 2=CLEAR_BITS)
      uint32_t obj = s->r[0], bits = s->r[1], mode = s->r[2], outp = s->r[3];
      if (obj == 0x60014000u || obj == 0x60015000u) g_eflag[obj] |= 1u;   // 18.4264: auto-wake guest Fios worker threads

      bool orMode = (mode & 1u) != 0u;
      auto eval = [&](uint32_t curv) -> bool { return bits == 0u || (orMode ? ((curv & bits) != 0u) : ((curv & bits) == bits)); };
      uint32_t cur = (g_eflag.count(obj) ? g_eflag[obj] : 0u);
      bool ok = eval(cur);
      if (tl_ht_tid) { for (;;) { if (g_shutdown) break; cur = (g_eflag.count(obj) ? g_eflag[obj] : 0u); ok = eval(cur); if (ok) break; std::this_thread::sleep_for(std::chrono::microseconds(200)); }
        if (outp) { uint8_t* p = GP(outp); if (p) std::memcpy(p, &cur, 4); }
        if (mode & 4u) g_eflag[obj] = 0u; else if (mode & 2u) g_eflag[obj] = cur & ~bits;
        r0 = 0; break; }
      if (!ok && !g_cur_thr) {   // main thread: run workers / retry (they may Set the flag)
        for (int _sp = 0; _sp < 2000 && !ok; ++_sp) { hle_thr_pump(s, 400); cur = (g_eflag.count(obj) ? g_eflag[obj] : 0u); ok = eval(cur); }
      }
      if (g_evw_n++ < 40) { printf("[EVW] wait obj=%08X bits=%08X mode=%X cur=%08X ok=%d thr=%08X lr=%08X\n", obj, bits, mode, cur, (int)ok, g_cur_thr, s->r[14]); fflush(stdout); }
      if (ok) {
        if (outp) { uint8_t* p = GP(outp); if (p) std::memcpy(p, &cur, 4); }
        if (mode & 4u) g_eflag[obj] = 0u;                      // CLEAR_ALL
        else if (mode & 2u) g_eflag[obj] = cur & ~bits;        // CLEAR waited bits
        r0 = 0;
      } else if (g_cur_thr) { g_thr_blocked = 1; g_thr_wait_obj = obj; g_thr_wait_bits = bits; g_thr_wait_mode = mode; r0 = 0; }
      else { if (outp) { uint8_t* p = GP(outp); if (p) std::memcpy(p, &cur, 4); } r0 = 0; }
      break; }
    case 0x812B2AE0u: {                                               // sceKernelSetEventFlag
      uint32_t obj = s->r[0], bits = s->r[1];
      g_eflag[obj] |= bits;
      if (g_thr_log_n++ < 80) printf("[EVT] set obj=%08X bits=%08X cur_thr=%08X lr=%08X\n", obj, bits, g_cur_thr, s->r[14]);
      if (g_evt2_n++ < 60) { printf("[EVT2] set obj=%08X bits=%08X thr=%08X lr=%08X\n", obj, bits, g_cur_thr, s->r[14]); fflush(stdout); }
      { uint32_t cur2 = g_eflag[obj];
        for (auto& kv : g_thr) if (kv.second.waiting && kv.second.wait_obj == obj) {
          bool _or = (kv.second.wait_mode & 1u) != 0u;
          bool _ok = kv.second.wait_bits == 0u || (_or ? ((cur2 & kv.second.wait_bits) != 0u) : ((cur2 & kv.second.wait_bits) == kv.second.wait_bits));
          if (_ok) { kv.second.waiting = 0; kv.second.ready = 1; } } }
      r0 = 0; break; }
    case 0x812B1930u: {                                               // 18.4266: sceTouchRead(port,pData,nBuf) - old-runtime semantics (clear 16-byte header + nBuf*0x20 reports, return nBuf?1:0). DO NOT clear 0x140: nBuf buffers sit right before appJobQueue slots (appJobQueue.c:194 assert).
      uint32_t nBuf = s->r[2];
      uint32_t reps = nBuf > 8u ? 8u : nBuf;
      uint32_t bytes = 16u + reps * 0x20u;
      uint8_t* q = GP(s->r[1]);
      if (q) std::memset(q, 0, bytes);
      if (g_touch_n++ < 8) { printf("[TOUCH] Read port=%u pData=%08X nBuf=%u -> %u\n", s->r[0], s->r[1], nBuf, nBuf ? 1u : 0u); fflush(stdout); }
      r0 = nBuf ? 1u : 0u; break; }
    case 0x812B2AB0u: r0 = 0; break;                                  // sceKernelDeleteSema
    case 0x812B1250u: r0 = 0; break;                                  // 18.4259: DepthStencilSurfaceSetForceStoreMode / gxm no-op
    case 0x812B1290u: {                                               // 18.4259: sceGxmCreateRenderTarget(params, outObj)
      uint32_t obj = hle_malloc(0x1000); if (obj) std::memset(GP(obj), 0, 0x1000);
      if (s->r[1]) guest_wr32(s->r[1], obj); g_last_rt = obj;
      if (g_gxm_new_n++ < 8) printf("[GXM-RT] out=%08X params=%08X -> obj=%08X lr=%08X\n", s->r[1], s->r[0], obj, s->r[14]);
      r0 = 0; break; }
    case 0x812B12D0u: {                                               // 18.4259: sceGxmShaderPatcherRegisterProgram(patcher, programHeader, idOut)
      uint32_t hdr = s->r[1], idOut = s->r[2];
      uint32_t id = ++g_prog_id_ctr; if (!id) id = ++g_prog_id_ctr;
      g_prog_header_by_id[id] = hdr;
      if (idOut) guest_wr32(idOut, id);
      if (g_gxm_new_n++ < 8) { uint32_t magic = hdr ? guest_rd32(hdr) : 0;
        printf("[GXM-REG] hdr=%08X magic=%08X id=%u idOut=%08X lr=%08X\n", hdr, magic, id, idOut, s->r[14]); }
      r0 = 0; break; }
    case 0x812B13A0u: {                                               // 18.4259: sceGxmProgramFindParameterBySemantic(program, semantic, index)
      uint32_t prog = s->r[0], sem = s->r[1], idx = s->r[2]; r0 = 0;
      if (prog) {
        uint32_t cnt = guest_rd32(prog + 0x24); if (cnt > 0x400u) cnt = 0;
        uint32_t base = prog + 0x28u + guest_rd32(prog + 0x28u);
        for (uint32_t i = 0; i < cnt; ++i) {
          uint32_t pp = base + i * 16u; uint8_t* pb = GP(pp + 6u);
          if (pb && pb[0] == (uint8_t)(sem & 0xFFu) && pb[1] == (uint8_t)(idx & 0xFFu)) { r0 = pp; break; } } }
      if (g_gxm_new_n++ < 10) printf("[GXM-FPS] prog=%08X sem=%u idx=%u -> %08X lr=%08X\n", prog, sem, idx, r0, s->r[14]);
      break; }
    case 0x812B1350u: {                                               // 18.4259: sceGxmShaderPatcherCreateFragmentProgram(patcher,id,outFmt,msaa,blend,[sp]vprog,[sp+4?]out)
      uint32_t id = s->r[1]; uint32_t sp = s->r[13];
      uint32_t hdr = (g_prog_header_by_id.count(id) ? g_prog_header_by_id[id] : 0u);
      uint32_t obj = hle_malloc(0x400); if (obj) { std::memset(GP(obj), 0, 0x400); if (hdr) guest_wr32(obj, hdr); }
      if (sp) guest_wr32(sp + 8u, obj); g_fprog_last = obj;
      if (g_gxm_new_n++ < 8) printf("[GXM-CFP] id=%u hdr=%08X -> fprog=%08X lr=%08X\n", id, hdr, obj, s->r[14]);
      r0 = 0; break; }
    case 0x812B15C0u: {                                               // 18.4259: sceGxmShaderPatcherCreateVertexProgram(patcher,id,outputs,precomp,[sp]program,[sp+4]out)
      uint32_t id = s->r[1]; uint32_t sp = s->r[13];
      uint32_t hdr = (g_prog_header_by_id.count(id) ? g_prog_header_by_id[id] : 0u);
      uint32_t obj = hle_malloc(0x800); if (obj) { std::memset(GP(obj), 0, 0x800); if (hdr) guest_wr32(obj, hdr); }
      if (sp) guest_wr32(sp + 4u, obj); g_vprog_last = obj;
      if (g_gxm_new_n++ < 8) printf("[GXM-CVP] id=%u hdr=%08X -> vprog=%08X lr=%08X\n", id, hdr, obj, s->r[14]);
      r0 = 0; break; }
    case 0x812B11D0u: {                                               // 18.4260: sceGxmCreateContext(params, outObj) -> fake 0x4000 context
      uint32_t obj = hle_malloc(0x4000); if (obj) std::memset(GP(obj), 0, 0x4000);
      if (s->r[1]) guest_wr32(s->r[1], obj);
      if (g_gxm_new_n++ < 8) printf("[GXM-CTX] params=%08X out=%08X -> obj=%08X lr=%08X\n", s->r[0], s->r[1], obj, s->r[14]);
      r0 = 0; break; }
    case 0x812B14B0u: {                                               // 18.4260: sceGxmGetNotificationRegion -> stable fake region
      static uint32_t nreg = 0; if (!nreg) nreg = hle_malloc(0x40);
      r0 = nreg; break; }
    case 0x812B1370u: {                                               // 18.4260: sceGxmProgramParameterGetResourceIndex(param) -> resourceIndex@+0xC
      uint32_t po = s->r[0]; r0 = po ? guest_rd32(po + 0xCu) : 0u; break; }
    case 0x812B1210u: case 0x812B1240u: case 0x812B1280u: case 0x812B1310u: case 0x812B1330u:
    case 0x812B1380u: case 0x812B1570u: case 0x812B16E0u: case 0x812B17D0u:
      r0 = 0; break;                                                  // 18.4260: sceGxm init/state setters -> no-op success
    case 0x812B17C0u:                                                 // 18.4260: Map*UsseMemory (second variant) - same as 11C0
    case 0x812B11C0u: {                                               // 18.4259: sceGxmMapFragmentUsseMemory(base,size,offsetOut) -> *out=0
      if (s->r[2]) guest_wr32(s->r[2], 0u);
      r0 = 0; break; }
    case 0x812B1530u: {                                               // sceGxmShaderPatcherGetProgramFromId(id) -> registered header, else fake
      auto itp = g_prog_header_by_id.find(s->r[0]);
      if (itp != g_prog_header_by_id.end() && itp->second) { r0 = itp->second; break; }
      static uint32_t fp_prog = 0;
      if (!fp_prog) { fp_prog = hle_malloc(0x200); }
      if (g_gxm_stub_n++ < 6) printf("[GXMS] GetProgramFromId id=%08X -> prog=%08X lr=%08X\n", s->r[0], fp_prog, s->r[14]);
      r0 = fp_prog; break; }
    case 0x812B12A0u: {                                               // sceGxmProgramFindParameterByName(program, name) -> fake param ptr
      static uint32_t fp_param = 0;
      if (!fp_param) { fp_param = hle_malloc(0x200); if (fp_param) { uint8_t* q = GP(fp_param + 0x10u); if (q) { uint32_t v = 1u; std::memcpy(q, &v, 4); } } }
      if (g_gxm_stub_n++ < 6) printf("[GXMS] FindParam prog=%08X name=%08X -> param=%08X lr=%08X\n", s->r[0], s->r[1], fp_param, s->r[14]);
      r0 = fp_param; break; }
    case 0x812B1270u: {                                               // sceGxmProgramParameterGetCategory(param) -> category
      r0 = 1u;                                                       // 1 = vector-ish category so the loader table keeps the param
      break; }
    case 0x812B2AC0u: {                                               // sceKernelCheckCallback -> noop (18.4212, old-runtime n_noop); P5D_CB_YIELD=1 restores the old cooperative-yield behaviour
      r0 = 0;
      if (!g_cb_yield) break;
      if (g_in_fex == 1 && ((++g_yield_main_tick & 0x3F) == 0)) { g_yield_to_main = 1;
        if (g_ytest_n++ < 8) { printf("[YMAIN] request from CheckCallback fib=%08X pc=%08X\n", g_cur_fiber, s->r[15]); fflush(stdout); } break; }
      if (g_in_fex > 0 && g_in_fiber_exec && !g_fiber_switch_to && g_fiber_list.size() > 1) {
        uint32_t cur = g_cur_fiber ? g_cur_fiber : g_last_fiber;
        for (size_t k = 1; k <= g_fiber_list.size(); ++k) {
          uint32_t cand = g_fiber_list[(g_yield_rr + k) % g_fiber_list.size()];
          if (cand != cur) { g_fiber_switch_to = cand; g_yield_rr = (uint32_t)((g_yield_rr + k) % g_fiber_list.size());
            if (g_yield_n++ < 12) { printf("[YIELD] from=%08X to=%08X\n", cur, cand); fflush(stdout); } break; } }
      }
      break; }
    case 0x812B23E0u: {                                               // SceLibc 0xE7EC3D0B = calloc(nmemb, size)
      uint32_t n = s->r[0], sz = s->r[1];
      uint64_t tot = (uint64_t)n * (uint64_t)sz;
      if (sz == 0u || tot == 0u) { r0 = hle_malloc(1u); break; }
      if (tot > 0x4000000u) { printf("[CALLOC-BIG] n=%u sz=%u\n", n, sz); fflush(stdout); }
      r0 = hle_malloc((uint32_t)tot);
      if (g_calloc_n++ < 12) printf("[CALLOC] n=%u sz=%u -> %08X lr=%08X\n", n, sz, r0, s->r[14]);
      break; }
    case 0x812B2A50u: {                                               // sceKernelDelayThread -> cooperative yield (per-frame pacing)
      r0 = 0;
      if (tl_ht_tid) { uint32_t _us = s->r[0]; if (_us > 2000u) _us = 2000u; if (_us < 100u) _us = 100u; std::this_thread::sleep_for(std::chrono::microseconds(_us)); break; }
      if (g_in_fex == 1 && ((++g_yield_main_tick & 0x3F) == 0)) { g_yield_to_main = 1;
        if (g_ytest_n++ < 8) { printf("[YMAIN] request from DelayThread fib=%08X pc=%08X\n", g_cur_fiber, s->r[15]); fflush(stdout); } break; }
      if (g_delay_n++ < 8) printf("[DELAY] usec=%u lr=%08X fib=%08X\n", s->r[0], s->r[14], g_cur_fiber);
      if (g_in_fex == 0 && !g_in_fiber_exec) { if (g_thry_n++ < 6) { printf("[THRYIELD] DelayThread -> pump thr (thr count=%zu)\n", g_thr.size()); fflush(stdout); } hle_thr_pump(s, 20000); }
      else if (g_thrpump_env && !g_cur_thr) { if (g_thry_n++ < 6) { printf("[THRYIELD] env DelayThread -> pump thr (count=%zu fib=%08X)\n", g_thr.size(), g_cur_fiber); fflush(stdout); } hle_thr_pump(s, 20000); }
      if (g_in_fex > 0 && g_in_fiber_exec && !g_fiber_switch_to && g_fiber_list.size() > 1) {
        uint32_t cur = g_cur_fiber ? g_cur_fiber : g_last_fiber;
        for (size_t k = 1; k <= g_fiber_list.size(); ++k) {
          uint32_t cand = g_fiber_list[(g_yield_rr + k) % g_fiber_list.size()];
          if (cand != cur) { g_fiber_switch_to = cand; g_yield_rr = (uint32_t)((g_yield_rr + k) % g_fiber_list.size());
            if (g_ydelay_n++ < 8) { printf("[YDELAY] from=%08X to=%08X\n", cur, cand); fflush(stdout); } break; } }
      }
      break; }
    default: r0 = 0; break;                                            // default: success code 0
  }
  s->r[0] = r0;
  if (g_ws_on) { uint32_t _v = 0; uint8_t* _q = GP(g_ws_addr); if (_q) std::memcpy(&_v, _q, 4); if (_v != g_ws_snap && g_ws_n++ < 20) { printf("[WSLOT-HLE] blk=%08X old=%08X new=%08X\n", g_cur_block, g_ws_snap, _v); g_ws_snap = _v; fflush(stdout); } }
  return s->r[14];
}
typedef BlockFn (*FinderFn)(uint32_t);
static FinderFn ALL_FINDERS[26] = {
  find_block_000, find_block_001, find_block_002, find_block_003, find_block_004, find_block_005,
  find_block_006, find_block_007, find_block_008, find_block_009, find_block_010, find_block_011,
  find_block_012, find_block_013, find_block_014, find_block_015, find_block_016, find_block_017,
  find_block_018, find_block_019, find_block_020, find_block_021, find_block_022, find_block_023,
  find_block_024, find_block_025 };
struct GBlk { uint32_t va; BlockFn fn; };
static std::vector<GBlk> g_all;
static void fiber_save(RecState* s) {
  if (g_cur_fiber) { g_f2_seen = g_cur_fiber; FCtx& c = g_fctx[g_cur_fiber]; std::memcpy(c.r, s->r, sizeof(c.r)); c.apsr = s->apsr;
    if (g_fsv_n++ < 40) printf("[FSAVE] fiber=%08X pc=%08X sp=%08X r5=%08X\n", g_cur_fiber, c.r[15], c.r[13], c.r[5]);
    if ((c.r[15] & ~1u) == 0x81206BDCu || c.r[0] == 2u)
      printf("[FSAVE-BAD] fiber=%08X r0=%08X r13=%08X r15=%08X\n", g_cur_fiber, c.r[0], c.r[13], c.r[15]); }
}
static void fiber_load(RecState* s, uint32_t fiber, uint32_t entry, uint32_t arg) {
  // 18.3773: mark the fiber ACTIVE ([fiber+0x80] bit0) on first run - the game's fiber manager checks this bit
  // (ands r1,r0,#1) to decide whether to unlink/track the fiber; a zero state made it spin forever.
  if (g_fact_bit) { uint8_t* q = GP(fiber + 0x80u); if (q) { uint32_t v; std::memcpy(&v, q, 4);
      if (!(v & 1u)) { v |= 1u; std::memcpy(q, &v, 4); if (g_fact_n++ < 10) printf("[FACT] fiber=%08X state|=1 -> %08X\n", fiber, v); } } }   // 18.4229 P0-B: OFF by default (P5D_FACTBIT=1 restores) - guest scheduler reads bit0 as finished
  if (g_cur_fiber == 0) { std::memcpy(g_thread_regs, s->r, sizeof(g_thread_regs)); g_thread_apsr = s->apsr; }
  g_cur_fiber = fiber; g_last_fiber = fiber;
  if (g_watch2 == 0) { g_watch2 = 0x816CBEA8u; uint32_t o2 = g_watch2 - IMG_VA_BASE; if (o2 + 4 <= IMG_SIZE) std::memcpy(&g_watch2_val, gp_img + o2, 4); }
  auto it = g_fctx.find(fiber);
  if (it == g_fctx.end()) {
    // read the fiber's own user entry ([fiber+0x84]) and argOnRun ([fiber+0x88]) when available
    uint32_t off = fiber - IMG_VA_BASE;
    uint32_t f84 = 0, f88 = 0, f13 = arg;
    if (off + 0x90 <= IMG_SIZE) { std::memcpy(&f84, gp_img+off+0x84, 4); std::memcpy(&f88, gp_img+off+0x88, 4); }
    if (g_fiber_dbg2++ < 6) printf("[FLOAD] fiber=%08X f84=%08X f88=%08X arg=%08X -> f13=%08X\n", fiber, f84, f88, arg, f13);
    // use the fiber stack (argOnRun) only if it looks like a stack address; otherwise allocate a dedicated stack
    if (f88 >= 0x82E00000u && f88 < 0x83000000u) f13 = f88;
    else if (arg >= 0x82E00000u && arg < 0x83000000u) f13 = arg;
    else { static uint32_t s_next_stack = 0x82EBF000u; f13 = s_next_stack; s_next_stack -= 0x40000u; }
    uint32_t idx0 = 0; for (size_t k = 0; k < g_fiber_list.size(); ++k) if (g_fiber_list[k] == fiber) { idx0 = (uint32_t)k; break; }
    f13 = 0xA0E00000u - idx0 * 0x800000u;   // 8MB stack per fiber in region B (mapped to image +0x1000000)
    FCtx c; std::memset(&c, 0, sizeof(c));
    c.r[0] = fiber;   // 18.3890: the fiber START STUB expects r0 = fiber object (r4=r0; reads [r4+0x84]/[r4+0x88]);
                      // the old code used r0=arg (e.g. 2) when f84==0, so the stub dereferenced [2+0x88] -> BADMEM 0x8A.
    c.r[13] = f13; c.r[14] = 0xFFFFFFF0u; c.r[15] = (entry ? entry : 0) & ~1u;
    if (f84) { c.r[15] = g_fiber_entry & ~1u; }  // start stub reads the user entry itself
    g_fctx[fiber] = c;
  }
  FCtx& c = g_fctx[fiber];
  // 18.3728: do NOT reset SP here - the fiber's stack frame must survive yields (the old per-load reset
  // destroyed frames and made pop-pc return garbage, e.g. 0x8120683A). The first-start stack is set below.
  if (g_fiber_dbg3++ < 8)
    printf("[FL2] fiber=%08X entry=%08X sp=%08X r0=%08X\n", fiber, c.r[15], c.r[13], c.r[0]);
  if ((c.r[15] & ~1u) == 0x81206BDCu || c.r[0] < 0x80000000u) {
    printf("[FLOAD-BAD] fiber=%08X r0=%08X r13=%08X r15=%08X -> repair r0\n", fiber, c.r[0], c.r[13], c.r[15]);
    if (c.r[0] < 0x80000000u) c.r[0] = fiber;   // 18.3891: never restore a garbage fiber object pointer
    if ((c.r[15] & ~1u) == 0x81206BDCu && c.r[13] == 0u) c.r[13] = 0xA0E00000u;
  }
  std::memcpy(s->r, c.r, sizeof(c.r)); s->apsr = c.apsr;
}
static void fiber_restore_thread(RecState* s) {
  fiber_save(s);
  g_cur_fiber = 0;
  std::memcpy(s->r, g_thread_regs, sizeof(g_thread_regs)); s->apsr = g_thread_apsr;
}
static int g_fiber_runs = 0, g_fiber_switches = 0, g_fiber_returns = 0;
static uint32_t fiber_exec_to_completion(RecState* s) {
  ++g_in_fex;
  if (g_in_fex > 1) {
    // 18.3900: nested fiber execution shares the SAME RecState, so the inner exec corrupts the outer
    // fiber's registers (r0/r7/r15) -> after returning, the outer loop resumes with garbage (stub r0=2 crash).
    // Refuse nesting at this level: the caller (HLE fiber run/switch) must instead arm g_fiber_switch_to so
    // the current fiber_exec handles the switch at its safe point (loop top).
    if (g_nested_n++ < 20)
      printf("[NESTED-REFUSED] depth=%d cur=%08X pc=%08X r0=%08X\n", g_in_fex, g_cur_fiber, s->r[15], s->r[0]);
    --g_in_fex;
    return 0u;
  }
  int guard = 0;
  if (g_blkcnt_total >= 14000000u && g_fx_n++ < 40)
    printf("[FEXEC-IN] n=%u cur=%08X pc=%08X r0=%08X r7=%08X r14=%08X r13=%08X\n",
           g_blkcnt_total, g_cur_fiber, s->r[15], s->r[0], s->r[7], s->r[14], s->r[13]);
  uint32_t exec_origin_fiber = g_cur_fiber;   // 18.3819: who we should return to after a switched-to fiber finishes
  ++g_fiber_runs;
  if (g_fiber_runs % 20000 == 0)
    printf("[FSTAT] runs=%d switches=%d returns=%d fibers=%zu cur=%08X\n", g_fiber_runs, g_fiber_switches, g_fiber_returns, g_fctx.size(), g_cur_fiber);
  int slice = 0;
  while (guard++ < 2000000) {
    if (g_nested_until && g_blkcnt_total >= g_nested_until && g_in_fiber_exec) {   // 18.4220: stop a nested fiber exec cleanly at the hybrid cutoff (saves its context, returns to the outer dispatcher)
      fiber_save(s);
      if (g_nested_stop_n++ < 6) { printf("[NESTED-STOP] total=%u pc=%08X cur=%08X\n", g_blkcnt_total, s->r[15], g_cur_fiber); fflush(stdout); }
      break;
    }
    if (g_yield_to_main && g_in_fex == 1) { g_yield_to_main = 0; fiber_save(s);
      if (g_ytest_n++ < 8) { printf("[YMAIN] unwind fiber=%08X pc=%08X\n", g_cur_fiber, s->r[15]); fflush(stdout); } break; }
    if (g_fex_budget > 0 && ++slice > g_fex_budget) {
      if (g_fex_slice_n++ < 12) printf("[SLICEOUT] fiber=%08X pc=%08X r0=%08X r1=%08X r4=%08X r5=%08X r6=%08X r7=%08X sp=%08X lr=%08X after=%d\n",
          g_cur_fiber, s->r[15], s->r[0], s->r[1], s->r[4], s->r[5], s->r[6], s->r[7], s->r[13], s->r[14], slice);
      fiber_save(s);
      break;
    }
    if (g_fiber_flag_return) { g_fiber_flag_return = 0; ++g_fiber_returns;
      if (g_fiber_returns < 30) printf("[FEVT] return cur=%08X runs=%d\n", g_cur_fiber, g_fiber_runs);
      break; }
    if (g_fiber_switch_to) { uint32_t t = g_fiber_switch_to; g_fiber_switch_to = 0; ++g_fiber_switches;
      { bool _ok = (t >= 0x80600000u && t < 0x84600000u) || (t >= 0x84600000u && t < 0x8C600000u) || (t >= 0xA0000000u && t < 0xA1000000u);
        if (!_ok) { printf("[FSWSAN] invalid switch target %08X (cur=%08X) -> ignored\n", t, g_cur_fiber); fflush(stdout); continue; } }
      if (g_blkcnt_total >= 1840000u && g_blkcnt_total <= 1940000u && g_sw_dbg_n++ < 60) {   // 18.4222: context-mix window
        auto itd = g_fctx.find(t);
        printf("[SWDBG] n=%u cur=%08X savepc=%08X save_r0=%08X save_r5=%08X save_r7=%08X -> t=%08X has=%d ctx pc=%08X r0=%08X r5=%08X r7=%08X r13=%08X\n",
               g_blkcnt_total, g_cur_fiber, s->r[15], s->r[0], s->r[5], s->r[7], t, (int)(itd != g_fctx.end()),
               itd != g_fctx.end() ? itd->second.r[15] : 0u, itd != g_fctx.end() ? itd->second.r[0] : 0u,
               itd != g_fctx.end() ? itd->second.r[5] : 0u, itd != g_fctx.end() ? itd->second.r[7] : 0u,
               itd != g_fctx.end() ? itd->second.r[13] : 0u);
      }
      if (g_fiber_switches < 30) printf("[FEVT] switch cur=%08X to=%08X runs=%d\n", g_cur_fiber, t, g_fiber_runs);
      fiber_save(s); auto it = g_fctx.find(t);
      // 18.3730: if the target fiber already finished (pc==0), restart it from its user entry so the
      // game can schedule its task again (the scheduler re-switches to it every round).
      if (it != g_fctx.end() && it->second.r[15] == 0u) {
        uint32_t off2 = t - IMG_VA_BASE; uint32_t entry2 = 0;
        if (off2 + 0x90 <= IMG_SIZE) std::memcpy(&entry2, gp_img + off2 + 0x84, 4);
        if (entry2) {
          uint32_t idx1 = 0; for (size_t k = 0; k < g_fiber_list.size(); ++k) if (g_fiber_list[k] == t) { idx1 = (uint32_t)k; break; }
          FCtx cc; std::memset(&cc, 0, sizeof(cc));
          cc.r[0] = t; cc.r[13] = 0xA0E00000u - idx1 * 0x800000u; cc.r[14] = 0xFFFFFFF0u; cc.r[15] = entry2 & ~1u;
          it->second = cc;
          if (g_swrst_n++ < 12) printf("[SWRST2] rebuild fiber=%08X entry=%08X sp=%08X\n", t, entry2, cc.r[13]);
        }
      }
      if (it == g_fctx.end()) {
        uint32_t _off = t - IMG_VA_BASE, _entry = 0, _arg = 0;
        if (_off + 0x90u <= IMG_SIZE) { std::memcpy(&_entry, gp_img + _off + 0x84u, 4); std::memcpy(&_arg, gp_img + _off + 0x88u, 4); }
        printf("[FIBER-AUTOCREATE] t=%08X entry=%08X arg=%08X\n", t, _entry, _arg);
        fiber_load(s, t, _entry, _arg);
        it = g_fctx.find(t);
        if (it == g_fctx.end()) { printf("[FIBER-SW] unknown target %08X (still after autoload)\n", t); break; }
      }
      g_cur_fiber = t; std::memcpy(s->r, it->second.r, sizeof(it->second.r)); s->apsr = it->second.apsr;
      if ((s->r[15] == 0x8120682Au || s->r[5] == 0u) && g_blkcnt_total >= 1800000u && g_swsusp_n++ < 40) {   // 18.4222: suspicious restored context
        printf("[SWSUSP] n=%u cur=%08X from=%08X restored pc=%08X r0=%08X r5=%08X r7=%08X sp=%08X\n",
               g_blkcnt_total, g_cur_fiber, t, s->r[15], s->r[0], s->r[5], s->r[7], s->r[13]); fflush(stdout);
      }
      // 18.3893: a fiber restored AT THE START STUB must have r0 = fiber object (stub: r4=r0; reads [r4+0x84]/[r4+0x88]).
      // The switch path bypassed fiber_load's repair, so a saved context with r0=2 reached the stub -> BADMEM 0x8A.
      if ((s->r[15] & ~1u) == 0x81206BDCu && (s->r[0] < 0x80000000u)) {
        printf("[SWFIX] fiber=%08X r0=%08X -> %08X (r15=stub)\n", t, s->r[0], t);
        s->r[0] = t;
      }
      g_switch_budget = 3000;   // 18.3822: time-slice the switched-to fiber so it cannot monopolize the exec
      continue; }
    uint32_t pc = s->r[15];
    if (g_thrpump_env && !g_cur_thr && g_blkcnt_total > 1500000u && (g_blkcnt_total % 20000u) == 0u) { hle_thr_pump(s, 20000); pc = s->r[15]; }
    if (pc==0x811DA024u) { RecState* _ps = SPTR(s); g_last_alloc_sz = _ps->r[0]; g_last_alloc_fp = _ps->r[4]; }
    else if ((pc==0x811DA028u||pc==0x811DA02Au||pc==0x811DA030u||pc==0x811DA036u) && SPTR(s)->r[0]==0u && g_allocf_n++ < 24) { RecState* _ps = SPTR(s); printf("[ALLOCFAIL] pc=%08X size=%08X fp=%08X r0=%08X r1=%08X r4=%08X r5=%08X r6=%08X r7=%08X sp=%08X lr=%08X fib=%08X thr=%08X\n", pc, g_last_alloc_sz, g_last_alloc_fp, _ps->r[0],_ps->r[1],_ps->r[4],_ps->r[5],_ps->r[6],_ps->r[7],_ps->r[13],_ps->r[14],g_cur_fiber,g_cur_thr); fflush(stdout); }
    if (pc==0x811C0D74u || pc==0x81207956u || pc==0x81207982u) { if (g_free2_n++ < 80) { RecState* _ps = SPTR(s); printf("[FREE2] pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X sp=%08X lr=%08X fib=%08X thr=%08X\n", pc, _ps->r[0],_ps->r[1],_ps->r[2],_ps->r[3],_ps->r[4],_ps->r[5],_ps->r[6],_ps->r[7],_ps->r[13],_ps->r[14],g_cur_fiber,g_cur_thr); fflush(stdout); } }
    if (pc == 0x811DD4BCu || pc == 0x811DD4C8u) { static int _bswap=0; if (_bswap++ < 30) { RecState* _ps=SPTR(s); printf("[BSWAP] pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X sp=%08X lr=%08X fib=%08X thr=%08X\n", pc, _ps->r[0],_ps->r[1],_ps->r[2],_ps->r[3],_ps->r[13],_ps->r[14],g_cur_fiber,g_cur_thr); fflush(stdout); } }
    if (pc == 0x811DEB44u || pc == 0x811DEB4Au) { static int _rb=0; if (_rb++ < 20) { RecState* _ps=SPTR(s); uint32_t _o=_ps->r[4]; uint32_t _f10=0,_f12=0,_f18=0,_f1c=0,_f20=0; uint8_t* q=GP(_o+0x10u); if(q){uint16_t _h=0; std::memcpy(&_h,q,2); _f10=_h;} q=GP(_o+0x12u); if(q){uint16_t _h=0; std::memcpy(&_h,q,2); _f12=_h;} q=GP(_o+0x18u); if(q) std::memcpy(&_f18,q,4); q=GP(_o+0x1Cu); if(q) std::memcpy(&_f1c,q,4); q=GP(_o+0x20u); if(q) std::memcpy(&_f20,q,4); printf("[RBUF] pc=%08X obj=%08X f10=%04X f12=%04X f18=%08X f1c=%08X f20=%08X ret=%08X lr=%08X\n", pc, _o, _f10, _f12, _f18, _f1c, _f20, _ps->r[0], _ps->r[14]); fflush(stdout); } }
    if (pc == 0x811DEAB4u) { static int _rf=0; if (_rf++ < 30) { RecState* _ps=SPTR(s); uint32_t _t=_ps->r[0]&0xFFFF, _f=_ps->r[1]&0xFFFF, _pp=_ps->r[2]; uint32_t _w[3]={0,0,0}; uint8_t* q=GP(_pp); if(q) std::memcpy(_w,q,12); printf("[RFACT] type=%04X flags=%04X param=%08X p0=%08X p1=%08X p2=%08X lr=%08X fib=%08X thr=%08X\n", _t,_f,_pp,_w[0],_w[1],_w[2],_ps->r[14],g_cur_fiber,g_cur_thr); fflush(stdout); } }
    if (pc == 0x811DEAB4u) { static int _g=0; if (_g++ < 4) { uint32_t _gv[10]; for (int _i=0;_i<10;++_i) { _gv[_i]=0; uint8_t* q=GP(0x81513DD4u + (uint32_t)_i*4u); if(q) std::memcpy(&_gv[_i],q,4); } printf("[GLOB] DD4=%08X DD8=%08X DDC=%08X DE0=%08X DE4=%08X DE8=%08X DEC=%08X DF0=%08X DF4=%08X DF8=%08X\n", _gv[0],_gv[1],_gv[2],_gv[3],_gv[4],_gv[5],_gv[6],_gv[7],_gv[8],_gv[9]); fflush(stdout); } }
    if (pc == 0x8118308Au || pc == 0x8118308Bu) { static int _rd=0; if (_rd++ < 24) { RecState* _ps=SPTR(s); uint32_t _o=_ps->r[0], _out=_ps->r[1]; int16_t _t=0; uint32_t _p=0,_sz=0; uint8_t* q=GP(_o+4u); if(q) std::memcpy(&_t,q,2); q=GP(_o+0x8cu); if(q) std::memcpy(&_p,q,4); q=GP(_o+0x90u); if(q) std::memcpy(&_sz,q,4); printf("[RDR] pc=%08X obj=%08X type=%d ptr=%08X size=%08X out=%08X lr=%08X\n", pc, _o, (int)_t, _p, _sz, _out, _ps->r[14]); fflush(stdout); } }
    if (pc == 0x811C6C74u) { static int _cd=0; if (_cd++ < 2) { RecState* _ps=SPTR(s); uint32_t _o=_ps->r[4]; uint8_t* q=GP(_o); printf("[CREATED] obj=%08X:", _o); if(q){ for (int _i=0;_i<0xA0;_i+=4){uint32_t _v=0; std::memcpy(&_v,q+_i,4); printf(" +%02X=%08X",_i,_v);} } printf("\n"); fflush(stdout); } }
    if (pc == 0x811DEAB4u) { static int _g2=0; if (_g2++ < 2) { RecState* _ps=SPTR(s); uint32_t _o=_ps->r[4]; uint8_t* q=GP(_o); (void)q; } }
    if (pc == 0x811C6C72u || pc == 0x811C6C74u) { static int _cd2=0; if (_cd2++ < 3) { RecState* _ps=SPTR(s); uint32_t _o=_ps->r[0]; uint8_t* q=GP(_o); printf("[CREATED2] pc=%08X obj=%08X:", pc, _o); if(q){ for (int _i=0;_i<0xA0;_i+=4){uint32_t _v=0; std::memcpy(&_v,q+_i,4); printf(" +%02X=%08X",_i,_v);} } printf("\n"); fflush(stdout); } }
    if (pc == 0x811C6C72u) { static int _ch=0; if (_ch++ < 4) { RecState* _ps=SPTR(s); uint32_t _o=_ps->r[0]; uint32_t _c=0,_t=0,_sz=0,_ptr=0,_q88=0,_q94=0,_qa0=0; uint8_t* q=GP(_o); if(q) std::memcpy(&_c,q,4); if (_c < 0x80000000u) { printf("[CHILD] root=%08X child=%08X (invalid)\n", _o, _c); fflush(stdout); } else { q=GP(_c+4u); if(q){uint16_t _h=0;std::memcpy(&_h,q,2);_t=_h;} q=GP(_c+0x88u); if(q) std::memcpy(&_sz,q,4); q=GP(_c+0x94u); if(q) std::memcpy(&_ptr,q,4); q=GP(_c+0x8cu); if(q) std::memcpy(&_q88,q,4); q=GP(_c+0x94u); if(q) std::memcpy(&_q94,q,4); q=GP(_c+0xa0u); if(q){uint8_t _b=0;std::memcpy(&_b,q,1);_qa0=_b;} uint32_t _asz=0,_astub=0,_alr=0; { std::lock_guard<std::mutex> _lk(g_arec_mtx); for (auto& _a : g_arec) if (_a.p == _c) { _asz=_a.sz; _astub=_a.stub; _alr=_a.lr; } }
        printf("[CHILD] root=%08X child=%08X ctype=%d c88=%08X c8c=%08X c94=%08X ca0=%d asz=%08X astub=%08X alr=%08X\n", _o,_c,(int)_t,_sz,_q88,_q94,(int)_qa0,_asz,_astub,_alr); fflush(stdout); } } }
    if (pc == 0x811DA55Cu) { static int _ga=0; if (_ga++ < 4000) { RecState* _ps=SPTR(s); printf("[GALLOC] size=%08X lr=%08X r1=%08X fib=%08X thr=%08X\n", _ps->r[0],_ps->r[14],_ps->r[1],g_cur_fiber,g_cur_thr); fflush(stdout); } }
    if (pc == 0x811DA54Cu || pc == 0x811DA55Cu) { static int _gc=0; if (_gc++ < 40) { RecState* _ps=SPTR(s); printf("[GCA] pc=%08X r0=%08X r1=%08X r2=%08X lr=%08X fib=%08X thr=%08X\n", pc,_ps->r[0],_ps->r[1],_ps->r[2],_ps->r[14],g_cur_fiber,g_cur_thr); fflush(stdout); } }
    if ((pc & ~1u) == 0x811E5EC6u || (pc & ~1u) == 0x811E5EC2u || (pc & ~1u) == 0x811E5ECEu) { static int _wl=0; if (_wl++ < 12) { RecState* _ps=SPTR(s); uint32_t _r4=_ps->r[4]; uint16_t _st=0; uint32_t _fl=0; uint8_t* q=GP(_r4+2u); if(q) std::memcpy(&_st,q,2); q=GP(0x816CC630u); if(q) std::memcpy(&_fl,q,4); printf("[WAITLOOP] pc=%08X r4=%08X state=%u flag816CC630=%08X r0=%08X r5=%08X sp=%08X fib=%08X thr=%08X\n", pc,_r4,(unsigned)_st,_fl,_ps->r[0],_ps->r[5],_ps->r[13],g_cur_fiber,g_cur_thr); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EAAE4u) {
      if (!g_in_eaa4) { g_in_eaa4 = 1;
        if (g_blkcnt_total >= 14000000u && g_eaa4_ent_n++ < 40)
          printf("[EAA4-ENTER] n=%u r14=%08X r0=%08X r7=%08X r13=%08X cur=%08X\n",
                 g_blkcnt_total, s->r[14], s->r[0], s->r[7], s->r[13], g_cur_fiber); }
    } else { g_in_eaa4 = 0; }
    g_tr_pc[g_tr_n & 15] = pc; g_tr_r0[g_tr_n & 15] = s->r[0]; g_tr_r7[g_tr_n & 15] = s->r[7]; ++g_tr_n;
    if (g_tr_n >= 2) { uint32_t prev_pc = g_tr_pc[(g_tr_n - 2u) & 15u];
      if ((prev_pc & ~1u) == 0x811EAAE4u && (pc & ~1u) != 0x811EAAE4u && g_trans_n++ < 12)
        printf("[TRANS] 811EAAE4 -> pc=%08X r0(new=%08X prev=%08X) r7=%08X r14=%08X sp=%08X cur=%08X\n",
               pc, s->r[0], g_tr_r0[(g_tr_n - 2u) & 15u], s->r[7], s->r[14], s->r[13], g_cur_fiber); }
    { static uint32_t s_prev_sp = 0; static int s_spn = 0; static uint32_t s_prev_blk = 0;
      if (s->r[13] != s_prev_sp && s_spn < 40) {
        int in_stack = (s->r[13] >= 0x82E00000u && s->r[13] < 0x83000000u);
        if (!in_stack && s->r[13] != 0) {
          printf("[SPCHG] block=%08X sp=%08X->%08X (out of stack!) n=%d\n", s_prev_blk, s_prev_sp, s->r[13], s_spn);
          ++s_spn;
        }
        s_prev_sp = s->r[13]; s_prev_blk = pc;
      } }


    if ((pc & ~1u) == 0x811E5970u && g_e59_n < 4) { ++g_e59_n;
      printf("[E59F] enter 0x811E5970: r5=%08X r6=%08X r0=%08X r1=%08X r9=%08X sp=%08X cur=%08X\n",
             s->r[5],s->r[6],s->r[0],s->r[1],s->r[9],s->r[13],g_cur_fiber); }
    if (pc == 0u || pc == 0xFFFFFFF0u) {
      // 18.4225: mark the finished fiber's saved context as finished (pc=0). Otherwise the outer restart logic
      // keeps restoring the stale pre-return context (pc=0x8120682A) and livelocks on [8120682A, 81206832].
      if (g_cur_fiber) { FCtx& _c = g_fctx[g_cur_fiber]; _c.r[15] = 0u; }
      // 18.3766: when a fiber finishes (pc=0), publish the completion bit [fiber+0x80] |= 2 so the game's
      // main loop (0x812067A0/0x8120683C) observes it and can continue the init sequence.
      if (pc == 0u && g_cur_fiber) {
        uint8_t* q = GP(g_cur_fiber + 0x80u);
        if (q) { uint32_t v; std::memcpy(&v, q, 4); v |= 2u; std::memcpy(q, &v, 4);
          if (g_finbit_n++ < 10) printf("[FINBIT] fiber=%08X [fiber+0x80]|=2 (now %08X)\n", g_cur_fiber, v); }
      }
      // 18.3819: if we were executing a SWITCHED-TO fiber and it just finished, restore the ORIGINAL fiber and
      // continue its loop instead of abandoning it (this was why fiber1 never returned to the loader slot).
      if (g_cur_fiber != exec_origin_fiber && exec_origin_fiber != 0) {
        auto ito = g_fctx.find(exec_origin_fiber);
        if (ito != g_fctx.end()) {
          g_cur_fiber = exec_origin_fiber;
          std::memcpy(s->r, ito->second.r, sizeof(s->r)); s->apsr = ito->second.apsr;
          if (g_rest_n++ < 10) printf("[RESTORE] switched fiber done -> back to %08X pc=%08X\n", exec_origin_fiber, s->r[15]);
          continue;
        }
      }
      break;
    }
    if ((pc & ~1u) == 0x8120732Cu) g_t7_on = 1;
    if (g_t7_on && g_t7_n++ < 80) printf("[T732C] %08X r0=%08X r14=%08X sp=%08X\n", pc, s->r[0], s->r[14], s->r[13]);
    if (g_cur_fiber != exec_origin_fiber) { ++g_sw2_n2;
      if ((g_sw2_n2 % 100000u) == 0u && g_sw2_n < 20) { ++g_sw2_n;
        printf("[SW2] blocks_in_switched=%u pc=%08X fiber=%08X origin=%08X sp=%08X\n",
               g_sw2_n2, pc, g_cur_fiber, exec_origin_fiber, s->r[13]); } }
    if (g_switch_budget > 0) {
      if (--g_switch_budget == 0) {
        auto ito = g_fctx.find(exec_origin_fiber);
        if (ito != g_fctx.end() && exec_origin_fiber != 0) {
          g_cur_fiber = exec_origin_fiber;
          std::memcpy(s->r, ito->second.r, sizeof(s->r)); s->apsr = ito->second.apsr;
          g_switch_budget = 0;
          if (g_slice_back_n++ < 8) printf("[SLICEBACK] switched fiber time-slice done -> back to %08X pc=%08X\n", exec_origin_fiber, s->r[15]);
          continue;
        }
      }
    }
    if (pc == 0x812067A0u) { if (++g_c_mgr % 20000u == 0u && g_mgr_dbg < 6) { ++g_mgr_dbg;
        uint32_t lh = 0; { uint8_t* q = GP(0x816CBC08u + 0xA18u); if (q) std::memcpy(&lh, q, 4); }
        printf("[MGR] n=%u pc=%08X sp=%08X r5=%08X r9=%08X listhead=%08X cur=%08X\n", g_c_mgr, pc, s->r[13], s->r[5], s->r[9], lh, g_cur_fiber); } }
    if (pc == 0x812067B0u) ++g_c_67b0;
    if (pc == 0x812067E8u) ++g_c_67e8;
    if (pc == 0x8120680Eu) ++g_c_680e;
    if (pc == 0x8120681Cu) ++g_c_681c;
    if (pc == 0x81206820u) ++g_c_6820;
    if (pc == 0x81206822u) ++g_c_6822;
    if (pc == 0x8120682Au) { g_t2_on = 1; g_t2_n = 0; }
    if (g_t2_on && g_t2_n < 120) { ++g_t2_n; printf("[T682A] %08X r0=%08X r14=%08X sp=%08X\n", pc, s->r[0], s->r[14], s->r[13]); }
    if (pc == 0x81198BEAu) ++g_c_bea;
    if (pc == 0x811E5CDAu) ++g_c_5cda;
    if (pc == 0x811E5928u) ++g_c_5928;
    if (pc == 0x8120683Cu) ++g_c_683c;
    if (pc == 0x81206860u) ++g_c_6860;
    if ((pc & ~1u) == 0x812067B6u && (s->r[3] == 0u || s->r[13] < 0x82E00000u))
      printf("[B67B6-RG] r3=%08X r13=%08X r0=%08X prev=%08X cur=%08X\n", s->r[3], s->r[13], s->r[0], g_prev_block, g_cur_fiber);
    if ((pc & ~1u) == 0x812067A0u && g_mgr_n++ < 12) printf("[MGR67A0] sp=%08X r0=%08X cur=%08X\n", s->r[13], s->r[0], g_cur_fiber);
    if ((pc & ~1u) == 0x812067B6u && g_b6_n++ < 12) {
      uint32_t w0 = 0; uint8_t* q0 = GP(s->r[13]); if (q0) std::memcpy(&w0, q0, 4);
      printf("[B67B6-ENTRY] r0=%08X r1=%08X r2=%08X r3=%08X sp=%08X [sp]=%08X cur=%08X in_fex=%d\n",
             s->r[0], s->r[1], s->r[2], s->r[3], s->r[13], w0, g_cur_fiber, g_in_fex);
    }
    if ((pc & ~1u) == 0x8115D29Au && g_ep_n++ < 10) {
      printf("[EPI-FE] sp=%08X cur=%08X stk:", s->r[13], g_cur_fiber);
      for (int i = 0; i < 9; ++i) { uint32_t w = 0; uint8_t* q = GP(s->r[13] + 4u + (uint32_t)i * 4u); if (q) std::memcpy(&w, q, 4); printf(" %08X", w); }
      { auto itc = g_fctx.find(g_cur_fiber); printf(" fcsp=%08X\n", itc != g_fctx.end() ? itc->second.r[13] : 0u); }
    }
    { uint32_t i = g_dfe_n++ % 20u; g_dfe_pc[i] = pc; g_dfe_r0[i] = s->r[0]; g_dfe_sp[i] = s->r[13]; }
    if (g_stack_watch) {
      uint32_t _w = 0; uint8_t* _q = GP(s->r[13]); if (_q) std::memcpy(&_w, _q, 4);
      if (_w != g_stk_last && g_stkchg_n++ < 40) {
        printf("[STKCHG] prev_blk=%08X now_pc=%08X sp=%08X stk0=%08X was=%08X r0=%08X\n",
               g_cur_block, pc, s->r[13], _w, g_stk_last, s->r[0]);
        g_stk_last = _w;
      }
    }
    if ((pc & ~1u) == 0x811DA534u) { g_stack_watch = 1; g_stk_last = 0; }
    if ((pc & ~1u) == 0x8115D206u) { g_stack_watch = 0; }
    if (g_slot_watch && g_save_slot) {
      uint32_t _sv = 0; uint8_t* _sq = GP(g_save_slot); if (_sq) std::memcpy(&_sv, _sq, 4);
      if (_sv != 0x8115D1F6u && g_slot_n++ < 20) {
        printf("[SAVESLOT] clobbered! slot=%08X val=%08X prev_blk=%08X now_pc=%08X sp=%08X r0=%08X\n",
               g_save_slot, _sv, g_cur_block, pc, s->r[13], s->r[0]);
      }
    }
    if ((pc & ~1u) == 0x8115D200u) { g_save_slot = s->r[13] - 4u; g_slot_watch = 1; printf("[SAVESLOT] armed slot=%08X (sp=%08X)\n", g_save_slot, s->r[13]); }
    if ((pc & ~1u) == 0x8115D206u) {
      { uint32_t _v0 = 0, _v1 = 0; uint8_t* _q0 = GP(s->r[13]); uint8_t* _q1 = GP(s->r[13] + 4u);
        if (_q0) std::memcpy(&_v0, _q0, 4); if (_q1) std::memcpy(&_v1, _q1, 4);
        printf("[SAVESLOT-POP] sp=%08X [sp]=%08X [sp+4]=%08X slotwatch=%d n=%d\n", s->r[13], _v0, _v1, g_slot_watch, g_slot_n); }
      g_slot_watch = 0;
    }
    if ((pc & ~1u) == 0x8115D29Au && g_d29a_n++ < 6) {
      uint32_t _a=0,_b=0,_c=0; uint8_t* _p0=GP(s->r[13]); if(_p0) std::memcpy(&_a,_p0,4);
      uint8_t* _p1=GP(s->r[13]+4u); if(_p1) std::memcpy(&_b,_p1,4);
      uint8_t* _p2=GP(s->r[13]+8u); if(_p2) std::memcpy(&_c,_p2,4);
      uint32_t _d=0,_e=0,_f=0,_g=0,_h2=0;
      { uint8_t* _q=GP(s->r[13]+0x20u); if(_q) std::memcpy(&_d,_q,4); }
      { uint8_t* _q=GP(s->r[13]+0x24u); if(_q) std::memcpy(&_e,_q,4); }
      { uint8_t* _q=GP(s->r[13]+0x28u); if(_q) std::memcpy(&_f,_q,4); }
      { uint8_t* _q=GP(s->r[13]+0x2Cu); if(_q) std::memcpy(&_g,_q,4); }
      { uint8_t* _q=GP(s->r[13]+0x30u); if(_q) std::memcpy(&_h2,_q,4); }
      printf("[D29A] sp=%08X [sp]=%08X [sp+4]=%08X [sp+8]=%08X [sp+20]=%08X [sp+24]=%08X [sp+28]=%08X [sp+2C]=%08X [sp+30]=%08X r0=%08X r14=%08X cur=%08X\n",
             s->r[13], _a, _b, _c, _d, _e, _f, _g, _h2, s->r[0], s->r[14], g_cur_fiber);
    }
    if ((pc & ~1u) == 0x81207308u) {
      uint32_t _lr = s->r[14];
      if (_lr >= 0x812B3000u || _lr < 0x81000000u || g_d308_n++ < 3) {
        printf("[D308] lr=%08X sp=%08X r0=%08X r4=%08X r5=%08X (n=%d)\n", _lr, s->r[13], s->r[0], s->r[4], s->r[5], g_d308_n);
        fflush(stdout);
      }
    }
    if ((pc & ~1u) == 0x8120732Cu) {
      uint32_t _lr2 = s->r[14];
      uint32_t _w2[8]; for (int _i = 0; _i < 8; ++_i) { _w2[_i] = 0; uint8_t* _q = GP(s->r[13] + (uint32_t)(_i * 4) + 0x8u); if (_q) std::memcpy(&_w2[_i], _q, 4); }
      uint32_t _pb = 0, _pn = 0; if (g_mr_n > 0) { uint32_t _k = (g_mr_n - 1u) % 40u; _pb = g_mr_pc[_k]; _pn = g_mr_nx[_k]; }
      printf("[D72C] lr=%08X sp=%08X [sp+8]=%08X [sp+C]=%08X [sp+10]=%08X [sp+14]=%08X [sp+18]=%08X [sp+1C]=%08X prev=%08X>%08X\n",
             _lr2, s->r[13], _w2[0], _w2[1], _w2[2], _w2[3], _w2[4], _w2[5], _pb, _pn);
      if (_w2[3] == 0x81555E98u) {
        printf("[D72C-BAD] mr:"); for (int _i = 0; _i < 40; ++_i) { uint32_t _k = (g_mr_n + (uint32_t)_i) % 40u; printf(" %08X>%08X", g_mr_pc[_k], g_mr_nx[_k]); } printf("\n");
        printf("[D72C-BAD] fe:"); for (int _i = 0; _i < 20; ++_i) printf(" %08X", g_dfe_pc[(g_dfe_n + (uint32_t)_i) % 20u]); printf("\n"); printf("[D72C-BAD] e6a1e_sp(n=%u):", g_e6a1e_n); for (int _i = 0; _i < 8; ++_i) printf(" %08X", g_e6a1e_sp[(g_e6a1e_n + (uint32_t)_i) & 7u]); printf("\n"); printf("[D72C-BAD] pre:"); for (int _i = 0; _i < 8; ++_i) printf(" %08X", g_pre_pc[(g_pre_idx + (uint32_t)_i) & 7u]); printf(" ctx:"); for (auto& _kv : g_fctx) printf(" %08X/sp=%08X", _kv.first, _kv.second.r[13]); printf("\n");
      }
      fflush(stdout);
    }
    if (g_rslot_watch) {
      uint32_t _sv = 0; uint8_t* _sq = GP(g_rslot_addr); if (_sq) std::memcpy(&_sv, _sq, 4);
      if (_sv != g_rslot_exp && g_rslotclob_n++ < 20) {
        printf("[SLOTCLOB] val=%08X exp=%08X prev_blk=%08X now_pc=%08X sp=%08X\n", _sv, g_rslot_exp, g_cur_block, pc, s->r[13]);
        fflush(stdout);
      }
    }
    if ((pc & ~1u) == 0x81207308u) { g_rslot_addr = s->r[13] - 4u; g_rslot_exp = s->r[14]; g_rslot_watch = 1; }
    if ((pc & ~1u) == 0x8120732Cu) { g_rslot_watch = 0; }
    if (g_ws_on) { uint32_t _v = 0; uint8_t* _q = GP(g_ws_addr); if (_q) std::memcpy(&_v, _q, 4); if (_v != g_ws_snap && g_ws_n++ < 20) { printf("[WSLOT-FE] prev=%08X now=%08X sp=%08X old=%08X new=%08X cur=%08X\n", g_cur_block, pc, s->r[13], g_ws_snap, _v, g_cur_fiber); g_ws_snap = _v; fflush(stdout); } }
    if ((pc & ~1u) == 0x811E5928u && g_5928_n++ < 20) {
      printf("[5928] cur=%08X sp=%08X r14=%08X r0=%08X (n=%d)\n", g_cur_fiber, s->r[13], s->r[14], s->r[0], g_5928_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8120682Au && g_682a_n++ < 20) {
      printf("[682A] r5=%08X r13=%08X r0=%08X r1=%08X r14=%08X cur=%08X (n=%d)\n", s->r[5], s->r[13], s->r[0], s->r[1], s->r[14], g_cur_fiber, g_682a_n);
      { uint32_t _sv = 0, _gv = 0; uint8_t* _q1 = GP(s->r[13] + 4u); if (_q1) std::memcpy(&_sv, _q1, 4); uint8_t* _q2 = GP(0x812B414Cu); if (_q2) std::memcpy(&_gv, _q2, 4); printf("[CANARY682A] saved=%08X global=%08X match=%d sp=%08X r5=%08X in_exec=%d\n", _sv, _gv, (int)(_sv == _gv), s->r[13], s->r[5], g_in_fiber_exec); fflush(stdout); }
      if (s->r[5] == 0u) { s->r[5] = 0x812B414Cu; printf("[682A-R5FIX] r5 0 -> 0x812B414C (sp=%08X)\n", s->r[13]); fflush(stdout); }
      if (s->r[5] == 0u) { printf("[682A-ZERO] fctx:"); for (auto& _kv : g_fctx) printf(" %08X:pc=%08X sp=%08X r5=%08X", _kv.first, _kv.second.r[15], _kv.second.r[13], _kv.second.r[5]); printf("\n"); fflush(stdout); }
      if (s->r[5] == 0u && s->r[13] == 0xA0DFFFE8u) { s->r[5] = 0x812B414Cu; s->r[13] = 0xA0DFFFD0u; { uint8_t* _q = GP(s->r[13] + 4u); if (_q) { uint32_t _v = 0x812B4150u; std::memcpy(_q, &_v, 4); } } g_cw_on = 1; g_cw_snap = 0x812B4150u; printf("[682A-FIX2] emulated prologue + saved-r5 slot: r5=812B414C sp=A0DFFFD0 [sp+4]=812B414C\n"); fflush(stdout); }
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8120679Cu && g_679c_n++ < 20) {
      printf("[679C] r0=%08X r4=%08X r5=%08X r13=%08X cur=%08X (n=%d)\n", s->r[0], s->r[4], s->r[5], s->r[13], g_cur_fiber, g_679c_n);
      fflush(stdout);
    }
    if (((pc & ~1u) == 0x81198A84u || (pc & ~1u) == 0x81198A92u) && g_call683c_n++ < 20) {
      printf("[CALL683C] pc=%08X r0=%08X r4=%08X r5=%08X r13=%08X (n=%d)\n", pc, s->r[0], s->r[4], s->r[5], s->r[13], g_call683c_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8120683Cu && g_683c_n++ < 20) {
      printf("[683C] r0=%08X r4=%08X r5=%08X r13=%08X prev_blk=%08X cur=%08X in_exec=%d (n=%d)\n", s->r[0], s->r[4], s->r[5], s->r[13], g_cur_block, g_cur_fiber, g_in_fiber_exec, g_683c_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8120682Au && s->r[13] == 0xA0DFFFE8u) {
      uint32_t _glob = 0; uint8_t* _qg = GP(0x812B414Cu); if (_qg) std::memcpy(&_glob, _qg, 4);
      uint32_t _old = 0; uint8_t* _qs = GP(s->r[13] + 4u); if (_qs) std::memcpy(&_old, _qs, 4);
      if (_glob && _old != _glob) {
        uint8_t* _qw = GP(s->r[13] + 4u); if (_qw) std::memcpy(_qw, &_glob, 4);
        printf("[CANARY-FIX] sp=%08X [sp+4]: %08X -> %08X (n=%d)\n", s->r[13], _old, _glob, ++g_canaryfix_n);
        fflush(stdout);
      }
    }
    if (g_cw_on) { uint32_t _cv = 0; uint8_t* _cq = GP(0xA0DFFFD4u); if (_cq) std::memcpy(&_cv, _cq, 4); if (_cv != g_cw_snap && g_cw_n++ < 12) { printf("[CWATCH] prev=%08X now=%08X old=%08X new=%08X sp=%08X cur=%08X\n", g_cur_block, pc, g_cw_snap, _cv, s->r[13], g_cur_fiber); fflush(stdout); g_cw_on = 0; } }
    if ((pc & ~1u) == 0x8120682Au && s->r[5] == 0u) { s->r[5] = 0x812B414Cu; }  // [R5HOIST]
    if ((pc & ~1u) == 0x8120682Au && g_fiberdbg2_n++ < 10) {
      uint32_t _h18=0,_h1c=0,_f1p=0,_f1n=0,_f2p=0,_f2n=0;
      uint8_t* _q; _q = GP(0x816CBC08u + 0xa18u); if (_q) std::memcpy(&_h18,_q,4);
      _q = GP(0x816CBC08u + 0xa1cu); if (_q) std::memcpy(&_h1c,_q,4);
      _q = GP(0x84676750u + 0x94u); if (_q) std::memcpy(&_f1p,_q,4);
      _q = GP(0x84676750u + 0x98u); if (_q) std::memcpy(&_f1n,_q,4);
      _q = GP(0x846867F0u + 0x94u); if (_q) std::memcpy(&_f2p,_q,4);
      _q = GP(0x846867F0u + 0x98u); if (_q) std::memcpy(&_f2n,_q,4);
      printf("[FLIST2] h18=%08X h1c=%08X f1p=%08X f1n=%08X f2p=%08X f2n=%08X r3=%08X cur=%08X (n=%d)\n", _h18,_h1c,_f1p,_f1n,_f2p,_f2n, s->r[3], g_cur_fiber, g_fiberdbg2_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8120682Au) {
      uint32_t _f2n = 0, _f1p = 0;
      uint8_t* _q1 = GP(0x846867F0u + 0x98u); if (_q1) std::memcpy(&_f2n, _q1, 4);
      uint8_t* _q2 = GP(0x84676750u + 0x94u); if (_q2) std::memcpy(&_f1p, _q2, 4);
      if (_f2n == 0u && _f1p == 0x846867F0u) {
        uint8_t* _qw = GP(0x846867F0u + 0x98u); if (_qw) { uint32_t _fv = 0x84676750u; std::memcpy(_qw, &_fv, 4); }
        if (g_flistfix_n++ < 10) { printf("[FLISTFIX] fiber2.next 0 -> fiber1 (sp=%08X cur=%08X n=%d)\n", s->r[13], g_cur_fiber, g_flistfix_n); fflush(stdout); }
      }
    }
    if ((pc & ~1u) == 0x8115F448u && g_f448_n++ < 200) {
      printf("[F448] cb=%08X a1=%08X a2=%08X a3=%08X size=%08X sp=%08X cur=%08X (n=%d)\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[3]+0x14u, s->r[13], g_cur_fiber, g_f448_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115F458u && g_f458_n++ < 40) {
      printf("[F458] malloc_ret=%08X r4=%08X r5=%08X r2=%08X sp=%08X (n=%d)\n", s->r[0], s->r[4], s->r[5], s->r[2], s->r[13], g_f458_n);
      fflush(stdout);
    }
    if (g_hacks && (pc & ~1u) == 0x8115F9D0u) { s->r[0] = 0u; }  // [ABORTSKIP] force checker result 0 so the deliberate null-write path is skipped
    if ((pc & ~1u) == 0x811AC934u && g_ac934_n++ < 20) {
      printf("[AC934] r0=%08X lr=%08X r1=%08X sp=%08X cur=%08X (n=%d)\n", s->r[0], s->r[14], s->r[1], s->r[13], g_cur_fiber, g_ac934_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x811AC9CCu && g_ac9cc_n++ < 20) {
      printf("[AC9CC] r8=%08X prev_blk=%08X lr=%08X r0=%08X sp=%08X cur=%08X (n=%d)\n", s->r[8], g_cur_block, s->r[14], s->r[0], s->r[13], g_cur_fiber, g_ac9cc_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x81207344u && g_lw_n++ < 400) {
      printf("[LW] r0=%08X prev=%08X lr=%08X r1=%08X r2=%08X sp=%08X cur=%08X (n=%d)\n", s->r[0], g_cur_block, s->r[14], s->r[1], s->r[2], s->r[13], g_cur_fiber, g_lw_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x811731D8u && g_731d8_n++ < 400) {
      printf("[731D8] r0=%08X lr=%08X prev=%08X r1=%08X sp=%08X cur=%08X (n=%d)\n", s->r[0], s->r[14], g_cur_block, s->r[1], s->r[13], g_cur_fiber, g_731d8_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115F7C2u && g_7c2_n++ < 120) {
      uint32_t _obj = s->r[0], _f4 = 0; uint8_t* _q = GP(_obj + 4u); if (_q) std::memcpy(&_f4, _q, 4);
      printf("[7C2] obj=%08X [obj+4]=%08X sp=%08X cur=%08X (n=%d)\n", _obj, _f4, s->r[13], g_cur_fiber, g_7c2_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115D8E0u && g_d8e0_n++ < 200) {
      printf("[D8E0] r0=%08X lr=%08X prev=%08X r1=%08X sp=%08X cur=%08X (n=%d)\n", s->r[0], s->r[14], g_cur_block, s->r[1], s->r[13], g_cur_fiber, g_d8e0_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115D3A6u && g_d3a6_n++ < 12) {
      uint32_t _r8 = s->r[8], _v = 0; uint8_t* _q = GP(_r8 + 4u); if (_q) std::memcpy(&_v, _q, 4);
      printf("[D3A6] r8=%08X [r8+4]=%08X r9=%08X r0_in=%08X (n=%d)\n", _r8, _v, s->r[9], s->r[0], g_d3a6_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115D3B2u && g_d3b2_n++ < 12) {
      printf("[D3B2] r0_out=%08X r1=%08X r7=%08X sb=%08X (n=%d)\n", s->r[0], s->r[1], s->r[7], s->r[9], g_d3b2_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x81016818u && g_g18_n++ < 200) {
      printf("[G18] r0=%08X lr=%08X prev=%08X sp=%08X cur=%08X (n=%d)\n", s->r[0], s->r[14], g_cur_block, s->r[13], g_cur_fiber, g_g18_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x810164F4u && g_164f4_n++ < 60) {
      uint32_t _mgr = s->r[0], _f8 = 0, _fc = 0;
      uint8_t* _q8 = GP(_mgr + 8u); if (_q8) std::memcpy(&_f8, _q8, 4);
      uint8_t* _qc = GP(_mgr + 0xCu); if (_qc) std::memcpy(&_fc, _qc, 4);
      printf("[164F4] mgr=%08X id=%08X [mgr+8]=%08X [mgr+0xC]=%08X sp=%08X (n=%d)\n", _mgr, s->r[1], _f8, _fc, s->r[13], g_164f4_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x81184776u && g_84776_n++ < 5) { printf("[84776] ENTER init-sequence n=%d\n", g_84776_n); fflush(stdout); }
    if ((pc & ~1u) == 0x8101949Cu && g_1949c_n++ < 5) { printf("[1949C] ENTER pre-reg-init n=%d\n", g_1949c_n); fflush(stdout); }
    if ((pc & ~1u) == 0x8116E6A8u && g_e6a8_n++ < 30) {
      uint32_t _mgr = s->r[13]; uint32_t _m8 = 0;
      uint8_t* _q = GP(_mgr + 8u); if (_q) std::memcpy(&_m8, _q, 4);
      printf("[E6A8] n=%d r8=%08X r7=%08X r6=%08X r5=%08X sp=%08X [sp]=%08X [mgr?+8]=%08X\n", g_e6a8_n, s->r[8], s->r[7], s->r[6], s->r[5], s->r[13], s->r[0], _m8);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115D1D0u && g_d1d0_n++ < 300) {
      uint32_t _c = s->r[0], _b = 0, _cap = 0, _cnt = 0;
      uint8_t* _q = GP(_c); if (_q) { std::memcpy(&_b,_q,4); std::memcpy(&_cap,_q+4,4); std::memcpy(&_cnt,_q+8,4); }
      printf("[D1D0] n=%d container=%08X base=%08X cap=%08X count=%08X lr=%08X\n", g_d1d0_n, _c, _b, _cap, _cnt, s->r[14]);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8116E522u && g_e522_n++ < 40) {
      printf("[E522] n=%d container=%08X node=%08X r2=%08X lr=%08X\n", g_e522_n, s->r[0], s->r[1], s->r[2], s->r[14]);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x810164F4u && g_poolst_n++ < 40) {
      uint32_t _v1=0,_v2=0,_v3=0,_w1=0,_w2=0,_w3=0,_x1=0,_x2=0,_x3=0;
      { uint8_t* q; if((q=GP(0xA0039000u+0x1Cu))) std::memcpy(&_v1,q,4); if((q=GP(0xA0039000u+0x20u))) std::memcpy(&_v2,q,4); if((q=GP(0xA0039000u+0x2Cu))) std::memcpy(&_v3,q,4);
        if((q=GP(0xA005A000u+0x1Cu))) std::memcpy(&_w1,q,4); if((q=GP(0xA005A000u+0x20u))) std::memcpy(&_w2,q,4); if((q=GP(0xA005A000u+0x2Cu))) std::memcpy(&_w3,q,4);
        if((q=GP(0xA0115000u+0x1Cu))) std::memcpy(&_x1,q,4); if((q=GP(0xA0115000u+0x20u))) std::memcpy(&_x2,q,4); if((q=GP(0xA0115000u+0x2Cu))) std::memcpy(&_x3,q,4); }
      printf("[POOLST] n=%d p1(c=%X p=%X q=%X) p2(c=%X p=%X q=%X) p3(c=%X p=%X q=%X)\n", g_poolst_n, _v1,_v2,_v3, _w1,_w2,_w3, _x1,_x2,_x3);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115F7C2u && g_f7c2_n++ < 300) {
      uint32_t _o = s->r[0], _f4 = 0; uint8_t* _q = GP(_o + 4u); if (_q) std::memcpy(&_f4, _q, 4);
      printf("[F7C2] n=%d obj=%08X [obj+4]=%08X lr=%08X\n", g_f7c2_n, _o, _f4, s->r[14]); fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115F7CEu && g_f7ce_n++ < 300) {
      uint32_t _o = s->r[0], _f4 = 0; uint8_t* _q = GP(_o + 4u); if (_q) std::memcpy(&_f4, _q, 4);
      printf("[F7CE] n=%d obj=%08X [obj+4]=%08X lr=%08X\n", g_f7ce_n, _o, _f4, s->r[14]); fflush(stdout);
    }
    if ((pc & ~1u) == 0x8116E690u || (pc & ~1u) == 0x8116E698u || (pc & ~1u) == 0x8116E6A8u || (pc & ~1u) == 0x8116E6D6u) {
      if (g_m8tr_n++ < 30) {
        uint32_t _mgr = 0, _m8 = 0; { uint8_t* q = GP(0x813AFC0Cu); if (q) std::memcpy(&_mgr, q, 4); }
        if (_mgr) { uint8_t* q2 = GP(_mgr + 8u); if (q2) std::memcpy(&_m8, q2, 4); }
        printf("[M8TR] n=%d pc=%08X mgr=%08X [mgr+8]=%08X\n", g_m8tr_n, (pc & ~1u), _mgr, _m8); fflush(stdout);
      }
    }
    if ((pc & ~1u) == 0x8116E6CAu && g_layout_n++ < 10) {
      uint32_t _sb = s->r[9], _v0 = 0, _v8 = 0, _vC = 0, _glob = 0;
      { uint8_t* q; if ((q=GP(0x813AFC0Cu))) std::memcpy(&_glob,q,4); if ((q=GP(_sb))) std::memcpy(&_v0,q,4); if ((q=GP(_sb+8u))) std::memcpy(&_v8,q,4); if ((q=GP(_sb+0xCu))) std::memcpy(&_vC,q,4); }
      { uint32_t _obj8=0, _objC=0; uint8_t* q3; if ((q3=GP(_v0+8u))) std::memcpy(&_obj8,q3,4); if ((q3=GP(_v0+0xCu))) std::memcpy(&_objC,q3,4);
      printf("[LAYOUT3] arr=%08X obj=%08X glob=%08X [obj+8]=%08X [obj+0xC]=%08X [arr+8]=%08X [arr+0xC]=%08X\n", _sb, _v0, _glob, _obj8, _objC, _v8, _vC); }
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8116E688u && g_store_n++ < 10) {
      uint32_t _obj = s->r[6], _tgt = s->r[0], _old = 0;
      { uint8_t* q = GP(_tgt); if (q) std::memcpy(&_old, q, 4); }
      printf("[STORE] obj=%08X target=%08X [targetbefore]=%08X\n", _obj, _tgt, _old);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x81016818u && g_acc_n++ < 40) {
      printf("[ACC] n=%d r0=%08X lr=%08X blk=%08X\n", g_acc_n, s->r[0], s->r[14], g_cur_block); fflush(stdout);
    }
    if ((pc & ~1u) == 0x81016404u && g_lkin_n++ < 40) {
      uint32_t _cb = 0; { uint8_t* q = GP(s->r[0] + 0xCu); if (q) std::memcpy(&_cb, q, 4); }
      printf("[LKIN] n=%d mgr=%08X id=%08X cb=%08X lr=%08X\n", g_lkin_n, s->r[0], s->r[1], _cb, s->r[14]); fflush(stdout);
    }
    if ((pc & ~1u) == 0x810164ACu && g_lkout_n++ < 40) {
      uint32_t _o10 = 0, _o14 = 0;
      { uint8_t* q = GP(s->r[13] + 0x10u); if (q) std::memcpy(&_o10, q, 4); q = GP(s->r[13] + 0x14u); if (q) std::memcpy(&_o14, q, 4); }
      printf("[LKOUT] n=%d ret=%08X out10=%08X out14=%08X lr=%08X\n", g_lkout_n, s->r[5], _o10, _o14, s->r[14]); fflush(stdout);
    }
    if ((pc & ~1u) == 0x8101651Eu) { if (g_lktr_n++ < 80) { printf("[LKTR] AFTER_INNER r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s->r[0], s->r[5], s->r[7], s->r[13], s->r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x81016522u) { if (g_lktr_n++ < 80) { printf("[LKTR] RET0 r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s->r[7], s->r[5], s->r[7], s->r[13], s->r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x81016526u) { if (g_lktr_n++ < 80) { printf("[LKTR] INSERT r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s->r[4], s->r[5], s->r[7], s->r[13], s->r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8101653Eu) { if (g_lktr_n++ < 80) { printf("[LKTR] RETOK r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s->r[4], s->r[5], s->r[7], s->r[13], s->r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x81016450u) { if (g_lktr_n++ < 80) { printf("[LKTR] INNER_RET0 r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s->r[7], s->r[5], s->r[7], s->r[13], s->r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x810164ACu) { if (g_lktr_n++ < 80) { printf("[LKTR] INNER_RETNODE r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s->r[5], s->r[5], s->r[7], s->r[13], s->r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8101647Eu) { if (g_lktr_n++ < 80) { printf("[LKTR] ALLOC_DONE r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s->r[0], s->r[5], s->r[7], s->r[13], s->r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x81016456u) { if (g_lktr_n++ < 80) { printf("[LKTR] ALLOC_START r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s->r[4], s->r[5], s->r[7], s->r[13], s->r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8116F3C6u) { if (g_cbtr_n++ < 60) { printf("[CBTR] CB_ENTRY r0=%08X r1=%08X r2=%08X r3=%08X r5=%08X lr=%08X sp=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[5], s->r[14], s->r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8116F3DCu) { if (g_cbtr_n++ < 60) { printf("[CBTR] CB_AFTER_ACC r0=%08X r1=%08X r2=%08X r3=%08X r5=%08X lr=%08X sp=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[5], s->r[14], s->r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8116F3E2u) { if (g_cbtr_n++ < 60) { printf("[CBTR] CB_RET0PATH r0=%08X r1=%08X r2=%08X r3=%08X r5=%08X lr=%08X sp=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[5], s->r[14], s->r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8101643Cu) { if (g_cbtr_n++ < 60) { printf("[CBTR] AFTER_CB r0=%08X r1=%08X r2=%08X r3=%08X r5=%08X lr=%08X sp=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[5], s->r[14], s->r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x81016440u) { if (g_cbtr_n++ < 60) { printf("[CBTR] CB_NONZERO r0=%08X r1=%08X r2=%08X r3=%08X r5=%08X lr=%08X sp=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[5], s->r[14], s->r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x81016444u) { if (g_cbtr_n++ < 60) { printf("[CBTR] SET_MINUS1 r0=%08X r1=%08X r2=%08X r3=%08X r5=%08X lr=%08X sp=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[5], s->r[14], s->r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8125AC60u) {
      if (g_tblw_n++ < 200) {
        uint32_t _idx = s->r[0], _val = s->r[1], _tbl = 0;
        { uint8_t* q = GP(0x8153F9D4u + _idx * 4u); if (q) std::memcpy(&_tbl, q, 4); }
        printf("[TBLW] n=%d idx=%08X val=%08X old=%08X lr=%08X blk=%08X\n", g_tblw_n, _idx, _val, _tbl, s->r[14], g_cur_block); fflush(stdout);
      }
    }
    if ((pc & ~1u) == 0x8125AC84u) {
      if (g_dsp_n++ < 120) {
        uint32_t _idx = s->r[0], _arg = s->r[1], _ent = 0;
        { uint8_t* q = GP(0x8153F9D4u + _idx * 4u); if (q) std::memcpy(&_ent, q, 4); }
        printf("[DSP] n=%d idx=%08X arg=%08X entry=%08X lr=%08X blk=%08X\n", g_dsp_n, _idx, _arg, _ent, s->r[14], g_cur_block); fflush(stdout);
      }
    }
    if ((pc & ~1u) == 0x8116F716u) { if (g_nset_n++ < 200000) { printf("[NSET] SET716 r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X sp=%08X blk=%08X\n", s->r[0], s->r[1], s->r[2], s->r[4], s->r[14], s->r[13], g_cur_block); fflush(stdout); } }
    if ((pc & ~1u) == 0x8115DEACu) { if (g_nset_n++ < 200000) { printf("[NSET] DEAC r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X sp=%08X blk=%08X\n", s->r[0], s->r[1], s->r[2], s->r[4], s->r[14], s->r[13], g_cur_block); fflush(stdout); } }
    if ((pc & ~1u) == 0x8115DCB2u) { if (g_nset_n++ < 200000) { printf("[NSET] DCB2 r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X sp=%08X blk=%08X\n", s->r[0], s->r[1], s->r[2], s->r[4], s->r[14], s->r[13], g_cur_block); fflush(stdout); } }
    if ((pc & ~1u) == 0x8115D3C8u) { if (g_nset_n++ < 200000) { printf("[NSET] D3C8 r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X sp=%08X blk=%08X\n", s->r[0], s->r[1], s->r[2], s->r[4], s->r[14], s->r[13], g_cur_block); fflush(stdout); } }
    if ((pc & ~1u) == 0x8115F8ACu) { if (g_nset_n++ < 200000) { printf("[NSET] F8AC r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X sp=%08X blk=%08X\n", s->r[0], s->r[1], s->r[2], s->r[4], s->r[14], s->r[13], g_cur_block); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EAB44u) {
      if (g_gxmp_n++ < 20) { printf("[GXMP] AB44 sb(r9)=%08X r7=%08X lr=%08X sp=%08X r0=%08X\n", s->r[9], s->r[7], s->r[14], s->r[13], s->r[0]); fflush(stdout); }
    }
    if ((pc & ~1u) == 0x812B1380u || (pc & ~1u) == 0x812B1660u || (pc & ~1u) == 0x812B1250u) {
      if (g_gxmp_n++ < 60) { printf("[GXMP] HLE %08X r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", (pc & ~1u), s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], s->r[13]); fflush(stdout); }
    }
    if ((pc & ~1u) == 0x811EB6BAu) { if (g_pool_n++ < 60) { printf("[POOL] POOL_ENTER r4=%08X r5=%08X r6=%08X r7=%08X r9=%08X lr=%08X\n", s->r[4], s->r[5], s->r[6], s->r[7], s->r[9], s->r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB702u) { if (g_pool_n++ < 60) { printf("[POOL] POOL_WRITE r4=%08X r5=%08X r6=%08X r7=%08X r9=%08X lr=%08X\n", s->r[4], s->r[5], s->r[6], s->r[7], s->r[9], s->r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB6ECu) { if (g_pool_n++ < 60) { printf("[POOL] POOL_ALLOC r4=%08X r5=%08X r6=%08X r7=%08X r9=%08X lr=%08X\n", s->r[4], s->r[5], s->r[6], s->r[7], s->r[9], s->r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB58Cu) { if (g_poolc_n++ < 80) { printf("[POOLC] F58C r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], s->r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811990A2u) { if (g_poolc_n++ < 80) { printf("[POOLC] CALLSITE r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], s->r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB5C0u) { if (g_poolc_n++ < 80) { printf("[POOLC] MID r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], s->r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB600u) { if (g_poolc_n++ < 80) { printf("[POOLC] MID2 r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], s->r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB650u) { if (g_poolc_n++ < 80) { printf("[POOLC] MID3 r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], s->r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB6A0u) { if (g_poolc_n++ < 80) { printf("[POOLC] MID4 r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], s->r[13]); fflush(stdout); } }
      if ((pc & ~1u) == 0x812A398Eu) { if (g_gcpy2_n++ < 60) { printf("[GCPY2] CPY_ENTER r8=%08X r6=%08X r5=%08X r4=%08X r1=%08X lr=%08X\n", s->r[8],s->r[6],s->r[5],s->r[4],s->r[1],s->r[14]); fflush(stdout); } }
      if ((pc & ~1u) == 0x812A3994u) { if (g_gcpy2_n++ < 60) { printf("[GCPY2] CPY_STORE r8=%08X r6=%08X r5=%08X r4=%08X r1=%08X lr=%08X\n", s->r[8],s->r[6],s->r[5],s->r[4],s->r[1],s->r[14]); fflush(stdout); } }
      if ((pc & ~1u) == 0x812A397Cu) { if (g_gcpy2_n++ < 60) { printf("[GCPY2] CPY_PRE r8=%08X r6=%08X r5=%08X r4=%08X r1=%08X lr=%08X\n", s->r[8],s->r[6],s->r[5],s->r[4],s->r[1],s->r[14]); fflush(stdout); } }
      if ((pc & ~1u) == 0x810510A0u) { if (g_gcpy2_n++ < 60) { printf("[GCPY2] CPY_510 r8=%08X r6=%08X r5=%08X r4=%08X r1=%08X lr=%08X\n", s->r[8],s->r[6],s->r[5],s->r[4],s->r[1],s->r[14]); fflush(stdout); } }
      if ((pc & ~1u) == 0x8103F528u) { if (g_gcpy2_n++ < 60) { printf("[GCPY2] CPY_3F5 r8=%08X r6=%08X r5=%08X r4=%08X r1=%08X lr=%08X\n", s->r[8],s->r[6],s->r[5],s->r[4],s->r[1],s->r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EA552u) { if (g_ea552_n++ < 60) { printf("[EA552] ENTER r0=%08X r1=%08X r2=%08X r7=%08X r9=%08X lr=%08X fib=%08X thr=%08X\n", s->r[0], s->r[1], s->r[2], s->r[7], s->r[9], s->r[14], g_cur_fiber, g_cur_thr); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EA5FEu) { if (g_ea552_n++ < 60) { printf("[EA552] MAPLOOP r0=%08X r1=%08X r2=%08X r7=%08X r9=%08X lr=%08X fib=%08X thr=%08X\n", s->r[0], s->r[1], s->r[2], s->r[7], s->r[9], s->r[14], g_cur_fiber, g_cur_thr); fflush(stdout); } }
    if (pc==0x811DA024u) { RecState* _ps = SPTR(s); g_last_alloc_sz = _ps->r[0]; g_last_alloc_fp = _ps->r[4]; }
    else if ((pc==0x811DA028u||pc==0x811DA02Au||pc==0x811DA030u||pc==0x811DA036u) && SPTR(s)->r[0]==0u && g_allocf_n++ < 24) { RecState* _ps = SPTR(s); printf("[ALLOCFAIL] pc=%08X size=%08X fp=%08X r0=%08X r1=%08X r4=%08X r5=%08X r6=%08X r7=%08X sp=%08X lr=%08X fib=%08X thr=%08X\n", pc, g_last_alloc_sz, g_last_alloc_fp, _ps->r[0],_ps->r[1],_ps->r[4],_ps->r[5],_ps->r[6],_ps->r[7],_ps->r[13],_ps->r[14],g_cur_fiber,g_cur_thr); fflush(stdout); }
    if ((pc & ~1u) == 0x811EAAE4u) { if (g_ea552_n++ < 60) { printf("[EA552] MATLOOP r0=%08X r1=%08X r2=%08X r7=%08X r9=%08X lr=%08X fib=%08X thr=%08X\n", s->r[0], s->r[1], s->r[2], s->r[7], s->r[9], s->r[14], g_cur_fiber, g_cur_thr); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EAB44u) { if (g_ea552_n++ < 60) { printf("[EA552] AB44 r0=%08X r1=%08X r2=%08X r7=%08X r9=%08X lr=%08X fib=%08X thr=%08X\n", s->r[0], s->r[1], s->r[2], s->r[7], s->r[9], s->r[14], g_cur_fiber, g_cur_thr); fflush(stdout); } }
          for (int _si=0;_si<16;++_si) g_snap_r[_si] = s->r[_si]; g_snap_blk = pc;
      if ((pc & ~1u) == 0x811EAB44u && g_val44_n++ < 20) { uint32_t _v=0; uint8_t* _q=GP(s->r[9]); if(_q) std::memcpy(&_v,_q,4); printf("[VAL44B] field=%08X r9=%08X fib=%08X thr=%08X\n", _v, s->r[9], g_cur_fiber, g_cur_thr); fflush(stdout); }
      { uint32_t _v=0; uint8_t* _q=GP(0x816CBF0Cu); if(_q) std::memcpy(&_v,_q,4); if (_v!=g_field_last && g_field_n++<80) { printf("[FW] blk=%08X prevblk=%08X v=%08X prev=%08X r7=%08X r9=%08X lr=%08X\n", pc, g_field_blk, _v, g_field_last, s->r[7], s->r[9], s->r[14]); fflush(stdout); } g_field_last=_v; g_field_blk=pc; }
      if ((pc & ~1u) == 0x811C0D74u && g_alloc_n++ < 40) { printf("[ALLOC] enter r0=%08X r1=%08X lr=%08X blk=%08X\n", s->r[0], s->r[1], s->r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x811C0DA8u && g_alloc2_n++ < 20) { printf("[ALLOC] crashblk r0=%08X r1=%08X lr=%08X blk=%08X\n", s->r[0], s->r[1], s->r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x811C1804u && g_alloc2_n++ < 20) { printf("[C1804] r0=%08X r5=%08X r6=%08X lr=%08X blk=%08X\n", s->r[0], s->r[5], s->r[6], s->r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x8124930Eu && g_wp_n++ < 30) { printf("[WP] r8=%08X r5=%08X fp=%08X r9=%08X lr=%08X r0=%08X blk=%08X fib=%08X thr=%08X\n", s->r[8], s->r[5], s->r[11], s->r[9], s->r[14], s->r[0], g_cur_block, g_cur_fiber, g_cur_thr); fflush(stdout); }
      if ((pc & ~1u) == 0x8124930Eu && g_wp_n++ < 3) { printf("[WPRING] blk=%08X lr=%08X r0=%08X r1=%08X r2=%08X r4=%08X r5=%08X r8=%08X ring:", g_cur_block, s->r[14], s->r[0], s->r[1], s->r[2], s->r[4], s->r[5], s->r[8]); for (int _k=0;_k<20;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 20u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout); }
      if (((pc & ~1u) == 0x81279AFEu || (pc & ~1u) == 0x81279B02u || (pc & ~1u) == 0x812B0DD0u) && g_wp_n++ < 40) { printf("[DIVP] pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X lr=%08X\n", pc, s->r[0], s->r[1], s->r[2], s->r[3], s->r[4], s->r[5], s->r[14]); fflush(stdout); }
      if ((pc & ~1u) == 0x811C0E14u && g_e14_n++ < 8) { printf("[E14P] r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r12=%08X lr=%08X blk=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[4], s->r[5], s->r[6], s->r[7], s->r[12], s->r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x811C0D74u && g_e14_n++ < 8) { printf("[E14P] FUNC_ENTRY r0=%08X r1=%08X lr=%08X blk=%08X\n", s->r[0], s->r[1], s->r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x81206700u && g_fp_n++ < 40) { printf("[FP] CALL_FACT r0=%08X r1=%08X r2=%08X lr=%08X\n", s->r[0], s->r[1], s->r[2], s->r[14]); fflush(stdout); }
      if ((pc & ~1u) == 0x81206760u && g_fp_n++ < 40) { printf("[FP] AFTER_HLE r0=%08X r4=%08X lr=%08X\n", s->r[0], s->r[4], s->r[14]); fflush(stdout); }
      if ((pc & ~1u) == 0x81198A80u && g_fp_n++ < 40) { printf("[FP] CALL_683C r5=%08X r4=%08X r0=%08X lr=%08X\n", s->r[5], s->r[4], s->r[0], s->r[14]); fflush(stdout); }
      if ((pc & ~1u) == 0x8120683Cu && g_fp_n++ < 40) { printf("[FP] ENTER_683C r0=%08X r1=%08X lr=%08X fib=%08X thr=%08X\n", s->r[0], s->r[1], s->r[14], g_cur_fiber, g_cur_thr); fflush(stdout); }
      if ((pc & ~1u) == 0x812067A0u && g_cf_n++ < 60) { printf("[CF] ENTER r0=%08X r1=%08X sp=%08X lr=%08X fib=%08X\n", s->r[0], s->r[1], s->r[13], s->r[14], g_cur_fiber); fflush(stdout); }
      if ((pc & ~1u) == 0x812067B6u && g_cf_n++ < 60) { uint32_t _cv=0; uint8_t* _cq=GP(s->r[13]+4u); if(_cq) std::memcpy(&_cv,_cq,4); printf("[CF] BODY sp=%08X [sp+4]=%08X r0=%08X r3=%08X fib=%08X\n", s->r[13], _cv, s->r[0], s->r[3], g_cur_fiber); fflush(stdout); }
      if ((pc & ~1u) == 0x8120682Eu && g_cf_n++ < 60) { printf("[CF] CMP r0=%08X r1=%08X r5=%08X sp=%08X fib=%08X\n", s->r[0], s->r[1], s->r[5], s->r[13], g_cur_fiber); fflush(stdout); }
      if ((pc & ~1u) == 0x8120682Au && g_cf_n++ < 80) { uint32_t _cv=0; uint8_t* _cq=GP(s->r[13]+4u); if(_cq) std::memcpy(&_cv,_cq,4); printf("[CF2] CMPBLK r0=%08X r1=%08X [sp+4]=%08X sp=%08X fib=%08X\n", s->r[0], s->r[1], _cv, s->r[13], g_cur_fiber); fflush(stdout); }
      if (((pc & ~1u) == 0x811E5928u || (pc & ~1u) == 0x811E5CDAu) && g_ctx_n++ < 60) { printf("[CTX] blk=%08X sp=%08X lr=%08X fib=%08X fex=%d runs=%d sw=%d ring:", g_cur_block, s->r[13], s->r[14], g_cur_fiber, g_in_fex, g_fiber_runs, g_fiber_switches); for (int _k=0;_k<12;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 12u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout); }
    if (g_hacks && (pc & ~1u) == 0x8115F9D0u) { s->r[0] = 0u; }  // [ABORTSKIP2] force checker 0 at the real block entry
      if (((pc & ~1u) == 0x812068D2u || (pc & ~1u) == 0x812068D8u || (pc & ~1u) == 0x812068E4u) && g_spoll_n++ < 60) {
        uint32_t _f80=0,_f94=0,_g18=0,_g1c=0; uint8_t* q;
        q=GP(s->r[5]+0x80u); if(q) std::memcpy(&_f80,q,4);
        q=GP(s->r[5]+0x94u); if(q) std::memcpy(&_f94,q,4);
        q=GP(0x816CC620u); if(q) std::memcpy(&_g18,q,4);
        q=GP(0x816CC624u); if(q) std::memcpy(&_g1c,q,4);
        printf("[SPOLL] pc=%08X r5=%08X r0=%08X lr=%08X f80=%08X f94=%08X g18=%08X g1c=%08X fib=%08X fex=%d fl=%zu\n", g_cur_block, s->r[5], s->r[0], s->r[14], _f80, _f94, _g18, _g1c, g_cur_fiber, g_in_fex, g_fiber_list.size()); fflush(stdout);
      }
      if (((pc & ~1u) == 0x81198BEAu || (pc & ~1u) == 0x81198BF6u || (pc & ~1u) == 0x812067A0u || (pc & ~1u) == 0x81206832u || (pc & ~1u) == 0x81206700u) && g_lp2_n++ < 80) {
        printf("[LP2] blk=%08X r0=%08X r1=%08X r4=%08X r5=%08X sp=%08X lr=%08X fib=%08X\n", g_cur_block, s->r[0], s->r[1], s->r[4], s->r[5], s->r[13], s->r[14], g_cur_fiber); fflush(stdout);
      }
    if ((pc & ~1u) == 0x81198BF6u) {
      uint32_t _lc=0; uint8_t* _lq=GP(0x816CC630u); if(_lq) std::memcpy(&_lc,_lq,4);
      if ((_lc & 2u) && g_loadspin_n++ > (g_loadclear_env ? 200u : 100000000u)) { _lc &= ~2u; if(_lq) std::memcpy(_lq,&_lc,4); printf("[LOADCLEAR] cleared bit1 at 0x816CC630 (iters=%d) -> %08X fib=%08X\n", g_loadspin_n, _lc, g_cur_fiber); fflush(stdout); }
    }
      if ((pc & ~1u) == 0x8127C1D2u && g_c1d2_n++ < 12) {
        printf("[C1D2] enter r0=%08X r1=%08X r2=%08X lr=%08X sp=%08X fib=%08X thr=%08X ring:", s->r[0], s->r[1], s->r[2], s->r[14], s->r[13], g_cur_fiber, g_cur_thr);
        for (int _k=0;_k<20;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 20u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
      }
      if (((pc & ~1u) == 0x8127C1CAu || (pc & ~1u) == 0x8127C1C0u || (pc & ~1u) == 0x8127C1C6u) && g_c1d2_n++ < 30) {
        printf("[C1D2] pre pc=%08X r0=%08X r1=%08X lr=%08X sp=%08X\n", g_cur_block, s->r[0], s->r[1], s->r[14], s->r[13]); fflush(stdout);
      }
if (g_in_fiber_exec && g_cur_fiber && g_fiber_list.size() > 1 && g_cur_fiber == g_fiber_list[1] && ((pc & ~1u) == 0x81207956u || (pc & ~1u) == 0x8120794Au || (pc & ~1u) == 0x81207B3Au || (pc & ~1u) == 0x81207B54u) && g_f2blk_n++ < 40) {
  printf("[F2BLK] pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", pc, s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], s->r[13]); fflush(stdout);
}
if ((pc & ~1u) == 0x81182C9Au && g_nx9a_n++ < 10) {
  printf("[NX9A] r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X sp=%08X lr=%08X fib=%08X ring:", s->r[0], s->r[1], s->r[2], s->r[3], s->r[4], s->r[5], s->r[6], s->r[7], s->r[8], s->r[13], s->r[14], g_cur_fiber);
  for (int _k=0;_k<20;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 20u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
}
{ static int _p[4] = {0,0,0,0};
  if ((pc & ~1u) == 0x81198A76u && _p[0]++ < 4) printf("[FLPROBE] init-set-flag block reached r2=%08X lr=%08X fib=%08X\n", s->r[2], s->r[14], g_cur_fiber);
  if ((pc & ~1u) == 0x81198C8Eu && _p[1]++ < 6) printf("[FLPROBE] frame-loop-top reached flags[=816CC630]=%08X fib=%08X sp=%08X\n", (uint32_t)(GP(0x816CC630u) ? *(uint32_t*)GP(0x816CC630u) : 0xDEADu), g_cur_fiber, s->r[13]);
  if ((pc & ~1u) == 0x81198CE4u && _p[2]++ < 6) printf("[FLPROBE] frame-counter-inc reached r5=%08X fib=%08X\n", s->r[5], g_cur_fiber);
  if ((pc & ~1u) == 0x81198D04u && _p[3]++ < 6) printf("[FLPROBE] frame-loop-test reached r0=%08X fib=%08X\n", s->r[0], g_cur_fiber);
  fflush(stdout); }
if ((pc & ~1u) == 0x811830C8u && g_w8c8_n++ < 6) {
  printf("[W8C8] enter r0=%08X r1=%08X lr=%08X sp=%08X fib=%08X ring:", s->r[0], s->r[1], s->r[14], s->r[13], g_cur_fiber);
  for (int _k=0;_k<24;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 24u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
}
if ((pc & ~1u) == 0x811830C8u && g_w8c8_watch == 0 && s->r[0] >= 0x80000000u) {
  g_w8c8_watch = s->r[0] + 4u; g_watch_addr = g_w8c8_watch; g_watch_val = 0xDEADBEEFu;
  uint8_t* _wq = GP(g_watch_addr); if (_wq) std::memcpy(&g_watch_val, _wq, 4);
  printf("[DWATCH] object=%08X state=%08X initial=%08X\n", s->r[0], g_watch_addr, g_watch_val); fflush(stdout);
}
if ((pc & ~1u) == 0x811E5C98u && g_cb98_n++ < 8) {
  uint32_t _cb[6] = {0,0,0,0,0,0}; const uint32_t _addrs[6] = {0x81513DD4u,0x81513DD8u,0x81513DDCu,0x81513DE0u,0x81513DE4u,0x81513DE8u};
  for (int _i=0;_i<6;++_i) { uint8_t* _q = GP(_addrs[_i]); if (_q) std::memcpy(&_cb[_i], _q, 4); }
  printf("[CB98] cb=%08X/%08X/%08X/%08X/%08X/%08X r0=%08X r1=%08X lr=%08X fib=%08X\n", _cb[0],_cb[1],_cb[2],_cb[3],_cb[4],_cb[5], s->r[0], s->r[1], s->r[14], g_cur_fiber); fflush(stdout);
}
if ((pc & ~1u) == 0x811DD4BCu && g_xswap_n++ < 20) {
  printf("[XSWAP] r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X lr=%08X sp=%08X fib=%08X mem:", s->r[0], s->r[1], s->r[2], s->r[3], s->r[4], s->r[5], s->r[14], s->r[13], g_cur_fiber);
  uint32_t _w0=0,_w1=0,_w2=0; uint8_t* _q;
  _q = GP((s->r[4]) + 0x0u); if (_q) std::memcpy(&_w0,_q,4);
  _q = GP((s->r[4]) + 0x18u); if (_q) std::memcpy(&_w1,_q,4);
  _q = GP((s->r[4]) + 0x1cu); if (_q) std::memcpy(&_w2,_q,4);
  printf(" hdr=%08X/%08X/%08X\n", _w0, _w1, _w2); fflush(stdout);
  if (g_hacks && s->r[2] == 0xFFFFFFFFu) { s->r[2] = 0u; printf("[XSKIP3] length -1 -> 0\n"); fflush(stdout); }
}
if (((pc & ~1u) == 0x811DEAB4u || (pc & ~1u) == 0x811DEB44u || (pc & ~1u) == 0x811DEB34u) && g_obj24_n++ < 24) {
  printf("[OBJ24] pc=%08X r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X fib=%08X", s->r[15], s->r[0], s->r[1], s->r[2], s->r[4], s->r[14], g_cur_fiber);
  uint32_t _o = s->r[4]; if (_o >= 0x80000000u) {
    uint32_t _w[10]; for (int _i=0;_i<10;++_i) { _w[_i]=0; uint8_t* _q=GP(_o + (uint32_t)_i*4u); if (_q) std::memcpy(&_w[_i],_q,4); }
    printf(" struct: %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X", _w[0],_w[1],_w[2],_w[3],_w[4],_w[5],_w[6],_w[7],_w[8],_w[9]);
  }
  printf("\n"); fflush(stdout);
}
if (((pc & ~1u) == 0x811C6C72u || (pc & ~1u) == 0x811C6C84u || (pc & ~1u) == 0x811C6C8Cu) && g_cb0_n++ < 18) {
  uint32_t _h = s->r[0]; if (_h < 0x80000000u) _h = s->r[4]; uint32_t _s = 0, _f8c = 0, _f90 = 0;
  if (_h >= 0x80000000u) { uint8_t* _q;
    _q = GP(_h + 4u); if (_q) std::memcpy(&_s, _q, 4);
    _q = GP(_h + 0x8cu); if (_q) std::memcpy(&_f8c, _q, 4);
    _q = GP(_h + 0x90u); if (_q) std::memcpy(&_f90, _q, 4); }
  printf("[CB0] pc=%08X r0=%08X handle=%08X state=%08X f8c=%08X f90=%08X r2=%08X lr=%08X fib=%08X\n", s->r[15], s->r[0], _h, _s, _f8c, _f90, s->r[2], s->r[14], g_cur_fiber); fflush(stdout);
}
if ((pc & ~1u) == 0x8121D044u) { ++g_d044_n;
  if (g_d044_n < 8) {
    uint32_t _w38=0,_w44=0,_w50=0; uint8_t* _q;
    _q = GP(s->r[13] + 0x38u); if (_q) std::memcpy(&_w38,_q,4);
    _q = GP(s->r[13] + 0x44u); if (_q) std::memcpy(&_w44,_q,4);
    _q = GP(s->r[13] + 0x50u); if (_q) std::memcpy(&_w50,_q,4);
    printf("[D044] #%u sp=%08X w38=%08X w44=%08X w50=%08X lr=%08X fib=%08X ring:", g_d044_n, s->r[13], _w38, _w44, _w50, s->r[14], g_cur_fiber);
    for (int _k=0;_k<16;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 16u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
  } }
if ((pc & ~1u) == 0x81207734u) { ++g_allocbig_total;
  if (g_allocbig_n++ < 40) {
    printf("[ALLOCBIG] #%u r0(size)=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X fib=%08X ring:", g_allocbig_n, s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], s->r[13], g_cur_fiber);
    for (int _k=0;_k<14;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 14u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
  } }
if ((pc & ~1u) == 0x81207290u && s->r[0] < 0x80000000u && g_atombad_n++ < 20) {
  printf("[ATOMBAD] #%u r0=%08X r1=%08X r2=%08X lr=%08X sp=%08X fib=%08X ring:", g_atombad_n, s->r[0], s->r[1], s->r[2], s->r[14], s->r[13], g_cur_fiber);
  for (int _k=0;_k<16;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 16u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
}
if (g_hacks && (pc & ~1u) == 0x811F019Eu && s->r[0] == 0u) { s->r[15] = s->r[14]; if (g_refnull_n++ < 8) { printf("[REFNULL] skip ref(null) lr=%08X fib=%08X\n", s->r[14], g_cur_fiber); fflush(stdout); } continue; }
if ((pc & ~1u) == 0x811E3520u && g_obj20_n++ < 12) {
  uint32_t _o = s->r[4]; uint32_t _v = 0; uint8_t* _q = GP(_o + 0x20u); if (_q) std::memcpy(&_v,_q,4);
  if (_o >= 0x80000000u) { g_obj20_addr = _o + 0x20u; }
  printf("[OBJ20] r4=%08X [r4+0x20]=%08X sp=%08X fib=%08X ring:", _o, _v, s->r[13], g_cur_fiber);
  for (int _k=0;_k<10;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 10u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
}
if ((pc & ~1u) == 0x811C411Cu && g_efe9a_n++ < 6) {
  uint8_t* _q = GP(0x811EFE9Au);
  if (_q) printf("[EFE9A] mem@811EFE9A = %02X %02X %02X %02X %02X %02X %02X %02X  (elf expects 00 20 70 47)\n", _q[0],_q[1],_q[2],_q[3],_q[4],_q[5],_q[6],_q[7]);
  else printf("[EFE9A] GP null\n");
  fflush(stdout);
}
ef2_probe(pc, s, 0);
if (g_hacks && (pc & ~1u) == 0x811E2B50u) {
  uint32_t _a = 0, _b = 0, _c = 0; uint8_t* _q = GP(s->r[1] + 0x40u); if (_q) std::memcpy(&_a,_q,4);
  if (_a >= 0x80000000u) { _q = GP(_a + 0x78u); if (_q) std::memcpy(&_b,_q,4); }
  if (_b >= 0x80000000u) { _q = GP(_b + 0x20u); if (_q) std::memcpy(&_c,_q,4); }
  if (_a >= 0x80000000u && _b >= 0x80000000u && _c == 0u) { s->r[15] = s->r[14];
    if (g_ret1_n++ < 8) { printf("[RET1] skip FUN_811e2b50: model+0x20 null (r1=%08X lr=%08X fib=%08X)\n", s->r[1], s->r[14], g_cur_fiber); fflush(stdout); } continue; }
}
if (family_skip(s, pc)) continue;
if (nulldesc_skip(s, pc)) continue;
if (assert_hle(s, pc)) continue;
assert_msg_probe(s, pc);
initwatch_probe(s, pc);
g_loop_site = 0; ++g_loop_cnt[0]; g_current_loop_pc = (int)pc; h7344_probe(s, pc);
    fix682a(s, &pc);
BlockFn fn = gbn(pc);
    if (!fn) { int back = 0; uint32_t bq = gbn_back(pc, &back);
      if (bq && back <= 16) { fn = gbn(bq); if (fn) { if (g_fback_n++ < 8) printf("[FBACK] pc=%08X -> block %08X (back %d)\n", pc, bq, back); pc = bq; } } }
    if (!fn) { printf("[FIBER-EXEC] no block for pc=%08X (cur=%08X)\n", pc, g_cur_fiber); fiber_save(s); break; }
    if (pc==0x811DA024u) { RecState* _ps = SPTR(s); g_last_alloc_sz = _ps->r[0]; g_last_alloc_fp = _ps->r[4]; }
    else if ((pc==0x811DA028u||pc==0x811DA02Au||pc==0x811DA030u||pc==0x811DA036u) && SPTR(s)->r[0]==0u && g_allocf_n++ < 24) { RecState* _ps = SPTR(s); printf("[ALLOCFAIL] pc=%08X size=%08X fp=%08X r0=%08X r1=%08X r4=%08X r5=%08X r6=%08X r7=%08X sp=%08X lr=%08X fib=%08X thr=%08X\n", pc, g_last_alloc_sz, g_last_alloc_fp, _ps->r[0],_ps->r[1],_ps->r[4],_ps->r[5],_ps->r[6],_ps->r[7],_ps->r[13],_ps->r[14],g_cur_fiber,g_cur_thr); fflush(stdout); }
    if ((pc & ~1u) == 0x811EAAE4u) { ++g_eaa4_cnt; }
    if ((pc & ~1u) == 0x811EAB18u) { printf("[LOOPEXIT] eaa4_iter=%d cur=%08X\n", g_eaa4_cnt, g_cur_fiber); g_after_loop = 40; }
    if (g_after_loop > 0) { --g_after_loop; printf("[AFTERLOOP] #%d pc=%08X r0=%08X r7=%08X sp=%08X lr=%08X\n", 40 - g_after_loop, pc, s->r[0], s->r[7], s->r[13], s->r[14]); }
    ++g_blkcnt[pc];
    if ((pc & ~1u) == 0x81198AA8u && g_canary_n++ < 10) {
      uint32_t c8 = 0, cstk = 0;
      { uint8_t* q = GP(s->r[8]); if (q) std::memcpy(&c8, q, 4); }
      uint32_t a = s->r[13] + 0x6cu; { uint8_t* q = GP(a); if (q) std::memcpy(&cstk, q, 4); }
      printf("[CANARY] r8=%08X [r8]=%08X [sp+6c]=%08X r1=%08X r2=%08X sp=%08X\n",
             s->r[8], c8, cstk, s->r[1], s->r[2], s->r[13]);
    }
    if (g_boot_trace_n++ < 400) printf("[BOOT] %08X\n", pc);
    if (pc >= 0x81198000u && pc < 0x8119A000u && g_mt_n++ < 400) printf("[MT] %08X r0=%08X r14=%08X\n", pc, s->r[0], s->r[14]);
    { uint32_t gpc = pc; if ((gpc & ~1u) == 0x81206BDCu && s->r[0] != 0u && s->r[0] < 0x80000000u) {
        // 18.3871: diagnostic guard - the stub is being entered with a small/garbage r0 (M330: r0=2).
        printf("[GUARD-FE] stub r0=%08X r13=%08X r14=%08X r7=%08X r1=%08X cur=%08X pump=%d ring:", s->r[0], s->r[13], s->r[14], s->r[7], s->r[1], g_cur_fiber, g_pumping2);
        for (int i = 0; i < 32; ++i) printf(" %08X", g_pc_ring[(g_pc_ring_n + (uint32_t)i) & 63u]);
        printf("\n");
        { uint32_t r7 = s->r[7], p304 = 0; uint8_t* q = GP(r7 + 0x304u); if (q) std::memcpy(&p304, q, 4);
          printf("[GUARD*] r7=%08X [r7+0x304]=%08X\n", r7, p304); }
        printf("[GUARD*] g_tr_n=%u blkcnt=%u in_fex=%d miniring:", g_tr_n, g_blkcnt_total, g_in_fex);
        for (int k = 0; k < 40 && g_mr_n >= 40u; ++k) { uint32_t idx = (g_mr_n - 40u + (uint32_t)k) % 40u;
          printf(" %08Xto%08X", g_mr_pc[idx], g_mr_nx[idx]); }
        printf("\n");
        printf("[GUARD*] prev=%08X trace:", g_prev_block);
        for (int k = 0; k < 16 && g_tr_n >= 16; ++k) { uint32_t idx = (g_tr_n - 16u + (uint32_t)k) & 15u;
          printf(" [%08X r0=%08X r7=%08X]", g_tr_pc[idx], g_tr_r0[idx], g_tr_r7[idx]); }
        printf("\n");
        fflush(stdout); std::abort(); } }
    g_pc_ring[g_pc_ring_n++ & 63u] = pc;
    // 18.3802: FRAME SLOT - fiber1 calls the loader 0x811E5CDA once per frame. Run one fiber2 frame pass here
    // (save fiber1 ctx, run fiber2 to completion, restore fiber1) instead of from the GetSelf idle path.
    if ((pc & ~1u) == 0x811E5CDAu) { ++g_fslot_att;
      if (g_fslot_att <= 4) printf("[FSLOT-ATT] #%d in_exec=%d pump=%d inflist=%zu cur=%08X\n",
             g_fslot_att, g_in_fiber_exec, g_in_frame_pump, g_fiber_list.size(), g_cur_fiber); }
    if (false && (pc & ~1u) == 0x811E5CDAu && !g_in_frame_pump && g_fiber_list.size() > 1) {   // 18.3824: DISABLED nested exec (context-mixing source); use single-level exec + game-driven sceFiberSwitch (probe model, M148/§4GN)
      g_in_frame_pump = 1;
      uint32_t ffib = g_fiber_list[0], sfib = g_fiber_list[1];
      fiber_save(s);
      uint32_t save_cur = g_cur_fiber;
      fiber_load(s, sfib, g_fiber_entry, g_fiber_arg);
      fiber_exec_to_completion(s);
      auto itf = g_fctx.find(ffib);
      if (itf != g_fctx.end()) { std::memcpy(s->r, itf->second.r, sizeof(s->r)); s->apsr = itf->second.apsr; }
      g_cur_fiber = save_cur;
      g_in_frame_pump = 0;
      if (g_fslot_n++ < 8) printf("[FSLOT] frame pump done fibers=%zu f1=%08X f2=%08X\n",
             g_fiber_list.size(), ffib, sfib);
    }
    if (g_pt_on && g_pt_n++ < 6000) printf("[PT] %08X r0=%08X r14=%08X\n", pc, s->r[0], s->r[14]);
    // 18.3778: defensive fix - the game's LwMutex-create helper FUN_81207308 does `str r5,[r4]` and one caller
    // passes r4=out=0. Redirect a null out pointer to a scratch slot so the task chain can continue.
    if ((pc & ~1u) == 0x8120732Cu && s->r[4] == 0u) { s->r[4] = 0x82DFFF00u;
      if (g_nullout_n++ < 8) printf("[NULLOUT] block 8120732C r4==0 -> scratch 82DFFF00\n"); }
    if ((g_blkcnt_total % 1000000u) == 0u && g_gfx_rep_n++ < 40)
      printf("[GFX] blk=%u ctx=%d begin=%d end=%d present=%d dqfin=%d\n", g_blkcnt_total, g_gxm_ctx_n, g_gxm_begin_n, g_gxm_end_n, g_gxm_present_n, g_gxm_fin_n);
    if ((++g_blkcnt_total % 200000u) == 0u) {
      uint32_t cb = 0; uint8_t* q = GP(0x81513DECu); if (q) std::memcpy(&cb, q, 4);
      printf("[CB] blocks=%u cb[0x81513DEC]=%08X in_exec=%d\n", g_blkcnt_total, cb, g_in_fiber_exec); }
    if (pc >= 0x81206800u && pc < 0x81206900u && g_ml2 < 30) { ++g_ml2;
      printf("[BLK68] pc=%08X argfiber=%08X cur=%08X sp=%08X\n", pc, s->r[5], g_cur_fiber, s->r[13]); }
    if (((pc & ~1u) == 0x8120684Eu || (pc & ~1u) == 0x8120683Cu || (pc & ~1u) == 0x812068D8u) && g_mloop_n < 30) { ++g_mloop_n;
      uint32_t argf = s->r[5]; uint32_t f80 = 0; { uint8_t* q = GP(argf + 0x80u); if (q) std::memcpy(&f80, q, 4); }
      uint32_t lh = 0; { uint8_t* q = GP(0x816CBC08u + 0xA18u); if (q) std::memcpy(&lh, q, 4); }
      printf("[MLOOP] pc=%08X argfiber=%08X [f+80]=%08X listhead=%08X r4=%08X cur=%08X sp=%08X\n", pc, argf, f80, lh, s->r[4], g_cur_fiber, s->r[13]); }
    // 18.3736 cooperative time-slice: every 2000 blocks hand off to the other fiber (pump runnable worker).
    if (g_in_fiber_exec && g_fiber_list.size() > 1 && (++g_slice_n % 2000) == 0) {
      uint32_t t = (g_cur_fiber == g_fiber_list[0]) ? g_fiber_list[1] : g_fiber_list[0];
      auto it2 = g_fctx.find(t);
      if (it2 != g_fctx.end()) {
        if (it2->second.r[15] == 0u) {
          uint32_t off3 = t - IMG_VA_BASE; uint32_t e3 = 0;
          if (off3 + 0x90 <= IMG_SIZE) std::memcpy(&e3, gp_img + off3 + 0x84, 4);
          if (e3) { uint32_t idx3 = 0; for (size_t k2 = 0; k2 < g_fiber_list.size(); ++k2) if (g_fiber_list[k2] == t) { idx3 = (uint32_t)k2; break; }
            FCtx c3; std::memset(&c3, 0, sizeof(c3)); c3.r[0] = t; c3.r[13] = 0xA0E00000u - idx3 * 0x800000u;
            c3.r[14] = 0xFFFFFFF0u; c3.r[15] = e3 & ~1u; it2->second = c3; }
        }
        if (it2->second.r[15] != 0u) {
          fiber_save(s);
          g_cur_fiber = t;
          std::memcpy(s->r, it2->second.r, sizeof(it2->second.r)); s->apsr = it2->second.apsr;
          if (g_slice_n < 4000) printf("[SLICE] -> %08X pc=%08X sp=%08X\n", t, s->r[15], s->r[13]);
          continue;
        }
      }
    }
    if (((pc & ~1u) == 0x811DA03Au || (pc & ~1u) == 0x811DA08Eu) && g_dsl_n < 12) { ++g_dsl_n;
      printf("[DSL] pc=%08X r0=%08X r1=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X sl=%08X sb=%08X sp=%08X\n",
             pc, s->r[0], s->r[1], s->r[4], s->r[5], s->r[6], s->r[7], s->r[8], s->r[10], s->r[9], s->r[13]); }
    if (((pc & ~1u) == 0x8120786Cu || (pc & ~1u) == 0x8120786Eu) && g_dret_n < 10) { ++g_dret_n;
      printf("[DRET] pc=%08X r0=%08X r1=%08X r4=%08X r5=%08X r6=%08X r7=%08X sp=%08X\n", pc, s->r[0], s->r[1], s->r[4], s->r[5], s->r[6], s->r[7], s->r[13]); }
    if (((pc & ~1u) == 0x811D9F4Au || (pc & ~1u) == 0x811DA040u || (pc & ~1u) == 0x811DA044u || (pc & ~1u) == 0x811DA56Cu || (pc & ~1u) == 0x811DA534u) && g_dheapn < 24) { ++g_dheapn;
      printf("[DHEAP] pc=%08X r0=%08X r1=%08X r4=%08X r5=%08X r9=%08X sp=%08X\n", pc, s->r[0], s->r[1], s->r[4], s->r[5], s->r[9], s->r[13]);
      if ((pc & ~1u) == 0x811D9F4Au) { uint32_t fp = s->r[0]; uint32_t fl = 0; { uint8_t* q = GP(fp + 0x14u); if (q) std::memcpy(&fl, q, 4); }
        printf("[FLIST] heap=%08X +14=%08X\n", fp, fl);
        if (fl) { uint32_t nd[8]; uint8_t* q2 = GP(fl); if (q2) { std::memcpy(nd, q2, 32);
          printf("[FLNODE] %08X: +0=%08X +4=%08X +8=%08X +c=%08X +10=%08X +14=%08X +18=%08X +1c=%08X\n", fl, nd[0],nd[1],nd[2],nd[3],nd[4],nd[5],nd[6],nd[7]); } } } }
    if (((pc & ~1u) == 0x81207734u || (pc & ~1u) == 0x81207740u || (pc & ~1u) == 0x8120776Eu || (pc & ~1u) == 0x81207768u) && g_d7734 < 10) { ++g_d7734;
      { uint32_t hp = 0; { uint8_t* q = GP(0x816CDA3Cu); if (q) std::memcpy(&hp, q, 4); }
        printf("[D7734] pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r9=%08X lr=%08X sp=%08X prev=%08X cur=%08X heap=%08X\n",
             pc, s->r[0], s->r[1], s->r[2], s->r[3], s->r[4], s->r[5], s->r[9], s->r[14], s->r[13], g_prev_block, g_cur_fiber, hp);
        if (hp) { uint32_t f[8]; uint8_t* q2 = GP(hp); if (q2) { std::memcpy(f, q2, 32);
            printf("[HEAP] +0=%08X +4=%08X +8=%08X +c=%08X +10=%08X +14=%08X +18=%08X +1c=%08X\n",
                   f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]); } } } }
    if ((pc & ~1u) == 0x8120683Cu && g_d683c < 8) { ++g_d683c;
      printf("[D683C] r0=%08X r5=%08X r13=%08X r14=%08X r15=%08X prev=%08X cur=%08X\n",
             s->r[0], s->r[5], s->r[13], s->r[14], s->r[15], g_prev_block, g_cur_fiber); }
    g_prev_block = pc;
    if ((pc & ~1u) == 0x811830CCu && s->r[4] >= 0x80000000u) {   // uncond3: always satisfy the wait state
      uint8_t* _f3 = GP(s->r[4] + 4u);
      if (_f3) { uint16_t _c = 0; std::memcpy(&_c, _f3, 2);
        if (_c != 3) { uint16_t _t = 3; std::memcpy(_f3, &_t, 2); if (g_uncond3_n++ < 8) { printf("[UNCOND3] object=%08X state %u -> 3\n", s->r[4], (unsigned)_c); fflush(stdout); } } }
    }
    if ((pc & ~1u) == 0x811830CCu && g_loop8cc_n++ < 12) {
      printf("[LOOP8CC] EX r4=%08X r0=%08X sp=%08X lr=%08X fib=%08X ring:", s->r[4], s->r[0], s->r[13], s->r[14], g_cur_fiber);
      for (int _k=0;_k<24;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 24u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
      if (s->r[4] >= 0x80000000u) {   // 18.4178: always satisfy the state wait (diagnostic)
        uint8_t* _f3 = GP(s->r[4] + 4u);
        if (_f3) { uint16_t _cur = 0; std::memcpy(&_cur, _f3, 2); if (_cur != 3) { uint16_t _three = 3; std::memcpy(_f3, &_three, 2); if (g_always3++ < 10) { printf("[ALWAYS3] object=%08X state %u -> 3\n", s->r[4], (unsigned)_cur); fflush(stdout); } } }
      }
      if (false && g_loop8cc_n >= 6 && s->r[4] >= 0x80000000u && s->r[4] != g_force3_obj) {
        g_force3_obj = s->r[4]; uint8_t* _f3 = GP(s->r[4] + 4u);
        if (_f3) { uint16_t _three = 3; std::memcpy(_f3, &_three, 2);
          printf("[FORCE3] wrote state=3 at %08X (object %08X) to unblock the poll loop\n", s->r[4] + 4u, s->r[4]); fflush(stdout); }
      }
      if (g_w8c8_watch == 0 && s->r[4] >= 0x80000000u) {
        g_w8c8_watch = s->r[4] + 4u; g_watch_addr = g_w8c8_watch; g_watch_val = 0xDEADBEEFu;
        uint8_t* _wq = GP(g_watch_addr); if (_wq) std::memcpy(&g_watch_val, _wq, 4);
        printf("[DWATCH] object=%08X state=%08X initial=%08X (from loop body)\n", s->r[4], g_watch_addr, g_watch_val); fflush(stdout);
      }
    }
    if ((pc & ~1u) == 0x811830C8u && g_w8fex_n++ < 6) {
      printf("[W8FEX] enter r0=%08X r1=%08X lr=%08X sp=%08X fib=%08X ring:", s->r[0], s->r[1], s->r[14], s->r[13], g_cur_fiber);
      for (int _k=0;_k<24;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 24u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
      if (g_w8c8_watch == 0 && s->r[0] >= 0x80000000u) {
        g_w8c8_watch = s->r[0] + 4u; g_watch_addr = g_w8c8_watch; g_watch_val = 0xDEADBEEFu;
        uint8_t* _wq = GP(g_watch_addr); if (_wq) std::memcpy(&g_watch_val, _wq, 4);
        printf("[DWATCH] object=%08X state=%08X initial=%08X\n", s->r[0], g_watch_addr, g_watch_val); fflush(stdout);
      }
    }
    s->r[15] = fn(s);
    g_mr_pc[g_mr_n % 40u] = pc; g_mr_nx[g_mr_n % 40u] = s->r[15]; ++g_mr_n;
    { uint32_t nx = s->r[15];
      if ((nx & ~1u) == 0x81206BDCu || (nx & ~1u) == 0x81206BDCu) {
        printf("[STUBRET] from pc=%08X nx=%08X r0=%08X r7=%08X r14=%08X sp=%08X cur=%08X\n",
               pc, nx, s->r[0], s->r[7], s->r[14], s->r[13], g_cur_fiber);
      }
      if ((pc & ~1u) == 0x811EAAE4u && nx != 0x811EAAE4u) {
        printf("[EAA4-EXIT] iter=%d ret=%08X r0=%08X r7=%08X r14=%08X sp=%08X cur=%08X\n",
               g_eaa4_cnt, nx, s->r[0], s->r[7], s->r[14], s->r[13], g_cur_fiber);
        g_after_loop = 40;
      }
      if (g_after_loop > 0) { --g_after_loop; printf("[AFTERLOOP2] nx=%08X r0=%08X r7=%08X r1=%08X\n", nx, s->r[0], s->r[7], s->r[1]); }
    }
    if (g_cur_fiber == 0x8287E650u && g_f2_n < 80) { ++g_f2_n;
      printf("[F2] pc=%08X next=%08X sp=%08X r0=%08X r9=%08X\n", pc, s->r[15], s->r[13], s->r[0], s->r[9]); }
    if ((pc & ~1u) == 0x811E5928u && g_path_n < 40) { ++g_path_n;
      printf("[PATH] 811E5928 -> next=%08X sp=%08X r0=%08X\n", s->r[15], s->r[13], s->r[0]); }
    if ((pc & ~1u) == 0x811E5BA0u && g_epi2_n2 < 10) { ++g_epi2_n2;
      uint32_t canary_saved = 0; { uint32_t a = s->r[13];
        if (a + 0x420u >= IMG_VA_BASE && (a + 0x420u) - IMG_VA_BASE + 4 <= IMG_SIZE) std::memcpy(&canary_saved, gp_img + (a + 0x420u - IMG_VA_BASE), 4); }
      printf("[EPI2] 0x811E5BA0 r0=%08X r1=%08X sb(r9)=%08X saved_canary=%08X sp=%08X\n",
             s->r[0], s->r[1], s->r[9], canary_saved, s->r[13]); }
    { static uint32_t prev_sp = 0; static int leak_n = 0;
      uint32_t sp = s->r[13];
      if (prev_sp && sp + 0x100u < prev_sp && leak_n < 20) {
        printf("[LEAK] block=%08X sp %08X->%08X delta=-%u\n", pc, prev_sp, sp, prev_sp - sp);
        ++leak_n;
      }
      prev_sp = sp; }
    if (g_watch2) { uint32_t o2 = g_watch2 - IMG_VA_BASE; if (o2 + 4 <= IMG_SIZE) { uint32_t v2; std::memcpy(&v2, gp_img + o2, 4);
      if (v2 != g_watch2_val && g_watch2_n < 30) { printf("[W2] block=%08X old=%08X new=%08X\n", pc, g_watch2_val, v2); ++g_watch2_n; g_watch2_val = v2; } } }
  }
  fiber_save(s);
  --g_in_fex;
  return s->r[0];
}
static uint32_t run_guest(RecState* s, uint32_t entry, uint32_t arg) {
  return fiber_exec_to_completion(s);
}
static void build_all_blocks() {
  for (int i = 0; i < 26; ++i) {
    unsigned cnt = 0;
    const GBlk* tab = (const GBlk*)(
        i == 0 ? get_table_000(&cnt) : i == 1 ? get_table_001(&cnt) : i == 2 ? get_table_002(&cnt) :
        i == 3 ? get_table_003(&cnt) : i == 4 ? get_table_004(&cnt) : i == 5 ? get_table_005(&cnt) :
        i == 6 ? get_table_006(&cnt) : i == 7 ? get_table_007(&cnt) : i == 8 ? get_table_008(&cnt) :
        i == 9 ? get_table_009(&cnt) : i == 10 ? get_table_010(&cnt) : i == 11 ? get_table_011(&cnt) :
        i == 12 ? get_table_012(&cnt) : i == 13 ? get_table_013(&cnt) : i == 14 ? get_table_014(&cnt) :
        i == 15 ? get_table_015(&cnt) : i == 16 ? get_table_016(&cnt) : i == 17 ? get_table_017(&cnt) :
        i == 18 ? get_table_018(&cnt) : i == 19 ? get_table_019(&cnt) : i == 20 ? get_table_020(&cnt) :
        i == 21 ? get_table_021(&cnt) : i == 22 ? get_table_022(&cnt) : i == 23 ? get_table_023(&cnt) :
        i == 24 ? get_table_024(&cnt) : get_table_025(&cnt));
    for (unsigned k = 0; k < cnt; ++k) g_all.push_back(tab[k]);
  }
  std::sort(g_all.begin(), g_all.end(), [](const GBlk&a, const GBlk&b){ return a.va < b.va; });
  printf("[TABLE] blocks=%zu\n", g_all.size());
}
static BlockFn gbn(uint32_t pc) {
  pc &= ~1u;   // 18.3939: normalize Thumb bit (block table is keyed by even instruction addresses)
  auto it = std::lower_bound(g_all.begin(), g_all.end(), pc, [](const GBlk&a, uint32_t v){ return a.va < v; });
  if (it != g_all.end() && it->va == pc) return it->fn;
  return nullptr;
}
static void guest_thread_run(uint32_t tid) {
  tl_ht_tid = tid;
  HThread* t = nullptr;
  { auto _it = g_thr.find(tid); if (_it == g_thr.end()) return; t = &_it->second; }
  RecState* s = &t->gctx;
  uint32_t pc = s->r[15];
  if (g_ht_n++ < 40) { printf("[HT-RUN] tid=%08X pc=%08X sp=%08X r0=%08X r1=%08X r4=%08X r5=%08X r9=%08X lr=%08X\n", tid, pc, s->r[13], s->r[0], s->r[1], s->r[4], s->r[5], s->r[9], s->r[14]); fflush(stdout); }
  while (!t->exit_flag && !g_shutdown) {
    if ((pc & ~1u) >= 0xFFFFFF00u || pc == 0u) { if (g_ht_bad_n++ < 20) { printf("[HT-SENTINEL] tid=%08X pc=%08X r0=%08X r1=%08X sp=%08X\n", tid, pc, s->r[0], s->r[1], s->r[13]); fflush(stdout); } break; }
    tl_pc_ring[tl_pc_n & 63u] = pc; ++tl_pc_n;
    { static int _a6=0; if ((pc==0x8122C6B6u||pc==0x8122C6DCu||pc==0x8122C6F6u||pc==0x8122C718u) && _a6++ < 24) { printf("[A6B6] pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X sp=%08X lr=%08X tid=%08X\n", pc, s->r[0],s->r[1],s->r[2],s->r[3],s->r[4],s->r[5],s->r[6],s->r[7],s->r[13],s->r[14],tid); fflush(stdout); } }
    { static int _tr=0; if (_tr++ < 100) { printf("[HTBLK] tid=%08X pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X sp=%08X lr=%08X\n", tid, pc, s->r[0],s->r[1],s->r[2],s->r[3],s->r[4],s->r[5],s->r[13],s->r[14]); fflush(stdout); } }
    BlockFn fn = gbn(pc);
    if (!fn) { if (g_ht_bad_n++ < 20) { printf("[HT-NOBLK] tid=%08X pc=%08X\n", tid, pc); fflush(stdout); } break; }
    g_cur_block = pc;
    uint32_t nx = fn(s);
    if (nx == 0u || nx == 0xFFFFFFF0u || nx == 0xFFFFFFFEu || nx == 0xFFFFFFFDu) break;
    pc = nx;   // 18.4240: follow the block's returned next-pc (not s->r[15]) - this was the host-thread loop bug
  }
  t->alive = 0; t->ready = 0;
  if (g_ht_n++ < 60) { printf("[HT-EXIT] tid=%08X\n", tid); fflush(stdout); }
}
static uint8_t* gimg_ptr() { return gp_img; }
extern "C" uint32_t id_reg(int idx, RecState* s) {
  uint32_t t = s->r[idx & 15] & ~1u;
  if (gbn(t)) return t;
  printf("[INDIRECT-MISS] reg%d=%08X target=%08X block=%08X\n", idx, s->r[idx & 15], t, g_cur_block);
  return 0xFFFFFFFDu;
}
extern "C" uint32_t id_regv(uint32_t tgt, RecState* s) {
  uint32_t t = tgt & ~1u;
  if (gbn(t)) return t;
  printf("[INDIRECT-MISS-V] target=%08X block=%08X\n", tgt, g_cur_block);
  return 0xFFFFFFFDu;
}

static uint32_t gbn_back(uint32_t t, int* pback) {
  // 18.3836: jump-table case targets can land INSIDE a block; execute the containing block start
  // (safe only when the skipped instructions have no side effects - verified 0x811E5992 is lsls).
  { uint32_t q0 = t & ~1u;                     // 18.3886: normalize Thumb bit FIRST (0x8115D7C7 -> 0x8115D7C6)
    if (gbn(q0)) { if (pback) *pback = 0; return q0; } }
  for (int b = 2; b <= 16; b += 2) {
    uint32_t q = (t - (uint32_t)b) & ~1u;
    if (gbn(q)) { if (pback) *pback = b; return q; }
  }
  return 0;
}

extern "C" void harness_badmem(uint32_t va) {
  { printf("[SNAP] blk=%08X r0=%08X r1=%08X r2=%08X r4=%08X r5=%08X r7=%08X r8=%08X r9=%08X r13=%08X r14=%08X\n", g_snap_blk, g_snap_r[0],g_snap_r[1],g_snap_r[2],g_snap_r[4],g_snap_r[5],g_snap_r[7],g_snap_r[8],g_snap_r[9],g_snap_r[13],g_snap_r[14]); fflush(stdout); }
  // 18.3870: dump the run_guest pc ring when a batch P() access faults (M329: block attribution was misleading).
  std::fprintf(stderr, " tid=%u stub=%08X tlring:", tl_ht_tid, g_cur_hle_stub);
  for (int i = 0; i < 28; ++i) std::fprintf(stderr, " %08X", tl_pc_ring[(tl_pc_n - 28u + (unsigned)i) & 63u]);
  std::fprintf(stderr, " heapAalloc:");
  { std::lock_guard<std::mutex> _lk(g_arec_mtx);
    for (auto& _a : g_arec) if (_a.p >= 0x8460F000u && _a.p < 0x84610000u)
      std::fprintf(stderr, " [p=%08X sz=%08X stub=%08X lr=%08X]", _a.p, _a.sz, _a.stub, _a.lr); }
  std::fprintf(stderr, " ring_va=%08X:", va);
  for (int i = 0; i < 32; ++i) std::fprintf(stderr, " %08X", g_pc_ring[(g_pc_ring_n + (uint32_t)i) & 63u]);
  std::fprintf(stderr, "\n");
  std::fprintf(stderr, " gsring(in/out/f):");
  for (int i = 0; i < 20 && g_gsr_n >= 20u; ++i) { uint32_t ix = (g_gsr_n - 20u + (uint32_t)i) % 24u;
    std::fprintf(stderr, " [%u/%08X/%08X]", g_gsr_in[ix], g_gsr_out[ix], g_gsr_f[ix]); }
  std::fprintf(stderr, "\n");
  std::fprintf(stderr, " ctx: in_fex=%d in_fiber_exec=%d pumping2=%d cur=%08X blk=%08X total=%u r5=%08X r7=%08X sp=%08X lr=%08X\n", g_in_fex, g_in_fiber_exec, g_pumping2, g_cur_fiber, g_cur_block, g_blkcnt_total, g_snap_r[5], g_snap_r[7], g_snap_r[13], g_snap_r[14]);
  std::fprintf(stderr, " fe_ring(pc/r0/sp):");
  for (int i = 0; i < 10 && g_dfe_n >= 10u; ++i) { uint32_t ix = (g_dfe_n - 10u + (uint32_t)i) % 20u;
    std::fprintf(stderr, " [%08X/%08X/%08X]", g_dfe_pc[ix], g_dfe_r0[ix], g_dfe_sp[ix]); }
  std::fprintf(stderr, "\n");
  std::fprintf(stderr, " rg_ring(pc/r0/sp):");
  for (int i = 0; i < 10 && g_drg_n >= 10u; ++i) { uint32_t ix = (g_drg_n - 10u + (uint32_t)i) % 20u;
    std::fprintf(stderr, " [%08X/%08X/%08X]", g_drg_pc[ix], g_drg_r0[ix], g_drg_sp[ix]); }
  std::fprintf(stderr, "\n");
}
extern "C" uint32_t id_tbb(uint32_t insn_pc, uint32_t addr, RecState* s) {
  uint32_t off = addr - IMG_VA_BASE;
  uint32_t b = (off < IMG_SIZE) ? gp_img[off] : 0;
  uint32_t t = (((insn_pc + 4) & ~3u) + (b << 1)) & ~1u;
  if (gbn(t)) return t;
  { int back = 0; uint32_t bq = gbn_back(t, &back);
    if (bq) { if (g_back_n++ < 8) printf("[TBB-BACK] target=%08X -> block %08X (back %d)\n", t, bq, back); return bq; } }
  printf("[TBB-MISS] pc=%08X addr=%08X b=%u target=%08X block=%08X\n", insn_pc, addr, b, t, g_cur_block);
  return 0xFFFFFFFDu;
}
extern "C" uint32_t id_tbh(uint32_t insn_pc, uint32_t base, uint32_t index, RecState* s) {
  // 18.3832: TBH semantics - base=align4(insn_pc+4); entry = uint16[base + index*2]; target = base + 2*entry
  uint32_t off = (base + index * 2u) - IMG_VA_BASE;
  uint32_t hval = 0;
  if (off + 2 <= IMG_SIZE) hval = (uint32_t)gp_img[off] | ((uint32_t)gp_img[off + 1] << 8);
  uint32_t t = (base + (hval << 1)) & ~1u;
  if (gbn(t)) return t;
  { int back = 0; uint32_t bq = gbn_back(t, &back);
    if (bq) { if (g_back_n++ < 8) printf("[TBH-BACK] target=%08X -> block %08X (back %d)\n", t, bq, back); return bq; } }
  printf("[TBH-MISS] pc=%08X base=%08X idx=%u h=%u target=%08X block=%08X\n", insn_pc, base, index, hval, t, g_cur_block);
  { uint32_t a = t & ~1u; printf("[TBH-NEAR] target=%08X:", a);
    for (int k = -8; k <= 8; ++k) { uint32_t q = (uint32_t)((int32_t)a + k); if (gbn(q)) printf(" %08X=YES", q); }
    printf("\n");
    // also dump table entries 0..6
    uint32_t off2 = base - IMG_VA_BASE;
    if (off2 + 16 <= IMG_SIZE) { printf("[TBH-TBL] base=%08X:", base);
      for (int k2 = 0; k2 < 8; ++k2) { uint32_t hw = (uint32_t)gp_img[off2 + k2*2] | ((uint32_t)gp_img[off2 + k2*2 + 1] << 8); printf(" [%d]=%u", k2, hw); }
      printf("\n"); } }
  return 0xFFFFFFFDu;
}

extern "C" uint32_t id_mem(uint32_t addr, RecState* s) {
  uint32_t off = addr - IMG_VA_BASE;
  uint32_t t = 0xFFFFFFFFu;
  if (off + 4 <= IMG_SIZE) std::memcpy(&t, gp_img + off, 4);
  t &= ~1u;
  if (gbn(t)) return t;
  { int back = 0; uint32_t bq = gbn_back(t, &back);
    if (bq) { if (g_back_n++ < 8) printf("[IND-BACK] target=%08X -> block %08X (back %d)\n", t, bq, back); return bq; } }
  printf("[INDIRECT-MISS] mem[%08X]=%08X target=%08X block=%08X\n", addr, t, t, g_cur_block);
  return 0xFFFFFFFDu;
}
static std::vector<uint8_t> g_img;
static void* load_elf(const char* path) {
  FILE* f = fopen(path, "rb"); if (!f) { printf("cannot open %s\n", path); return nullptr; }
  fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
  std::vector<uint8_t> d(sz); if (fread(d.data(), 1, sz, f) != (size_t)sz) { fclose(f); return nullptr; } fclose(f);
  if (memcmp(d.data(), "\x7f""ELF", 4) != 0) { printf("not ELF\n"); return nullptr; }
  uint32_t e_phoff = *(uint32_t*)(d.data()+0x1C); uint16_t e_phnum = *(uint16_t*)(d.data()+0x2C);
  g_img.assign(IMG_SIZE, 0);
  for (int i = 0; i < e_phnum; ++i) {
    const uint8_t* ph = d.data() + e_phoff + i*0x20;
    uint32_t p_type = *(uint32_t*)(ph+0); uint32_t p_off = *(uint32_t*)(ph+4); uint32_t p_va = *(uint32_t*)(ph+8);
    uint32_t p_fsz = *(uint32_t*)(ph+16);
    if (p_type != 1 || !p_fsz) continue;
    if (p_va < IMG_VA_BASE || p_va + p_fsz > IMG_VA_BASE + IMG_SIZE) { printf("seg out of image: %08X+%X\n", p_va, p_fsz); continue; }
    memcpy(g_img.data() + (p_va - IMG_VA_BASE), d.data() + p_off, p_fsz);
    printf("loaded segment VA %08X size %X\n", p_va, p_fsz);
  }
  gp_img = g_img.data();
  return g_img.data();
}
static int g_watch_n = 0;
static int g_entry_n = 0;
static int g_r9n = 0;
static int g_epi_n = 0;
static int g_epi2_n = 0;
static int g_f08_n = 0;
static int g_r10n = 0;
static int g_wrap_n = 0;
static int g_evt_n = 0;
static int g_sp_n = 0;
static int g_ww_n = 0;
static int g_h68_n = 0;
static int g_f60_n = 0;


// 18.4041: initialize the job-pool table + three pools per docs 4GM/4GN (guest-held structures).
// pool layout: +0x00 jobSema, +0x04 capSema, +0x08 doneEvt, +0x0C LwMutexWork ptr, +0x10 state,
// +0x14 ringBase, +0x18 ringCap, +0x1C consIdx, +0x20 prodIdx, +0x24 workerDesc, +0x28 workerCnt, +0x2C pending
static void w32img(uint32_t va, uint32_t v) { uint8_t* q = GP(va); if (q) std::memcpy(q, &v, 4); }
static void init_job_pools() {
  const uint32_t pools[3] = { 0xA0039000u, 0xA005A000u, 0xA0115000u };
  for (int i = 0; i < 3; ++i) w32img(0x8169E080u + (uint32_t)i * 4u, pools[i]);   // pool table
  // pool2 (measured: sema 0x14, cap 0x15(10240), evt 0x16, cap slots 0x2800, 4 workers)
  w32img(0xA005A000u + 0x00u, 0x14u); w32img(0xA005A000u + 0x04u, 0x15u); w32img(0xA005A000u + 0x08u, 0x16u);
  w32img(0xA005A000u + 0x0Cu, 0xA0007000u); w32img(0xA005A000u + 0x10u, 1u);
  w32img(0xA005A000u + 0x14u, 0xA005B000u); w32img(0xA005A000u + 0x18u, 0x2800u);
  w32img(0xA005A000u + 0x1Cu, 0u); w32img(0xA005A000u + 0x20u, 0u);
  w32img(0xA005A000u + 0x24u, 0xA005A100u); w32img(0xA005A000u + 0x28u, 4u); w32img(0xA005A000u + 0x2Cu, 0u);
  // pool3 (measured: sema 0x17, cap 0x18(8), evt 0x19, cap slots 8, 1 worker)
  w32img(0xA0115000u + 0x00u, 0x17u); w32img(0xA0115000u + 0x04u, 0x18u); w32img(0xA0115000u + 0x08u, 0x19u);
  w32img(0xA0115000u + 0x0Cu, 0xA0007000u); w32img(0xA0115000u + 0x10u, 1u);
  w32img(0xA0115000u + 0x14u, 0xA0116000u); w32img(0xA0115000u + 0x18u, 8u);
  w32img(0xA0115000u + 0x1Cu, 0u); w32img(0xA0115000u + 0x20u, 0u);
  w32img(0xA0115000u + 0x24u, 0xA0115100u); w32img(0xA0115000u + 0x28u, 1u); w32img(0xA0115000u + 0x2Cu, 0u);
  // pool1 (params less certain; state/counters only)
  w32img(0xA0039000u + 0x0Cu, 0xA0007000u); w32img(0xA0039000u + 0x10u, 1u);
  w32img(0xA0039000u + 0x1Cu, 0u); w32img(0xA0039000u + 0x20u, 0u);
  w32img(0xA0039000u + 0x28u, 1u); w32img(0xA0039000u + 0x2Cu, 0u);
  // LwMutexWork (attr/lockCount/holder per doc 3.3)
  w32img(0xA0007000u + 0x00u, 0u); w32img(0xA0007000u + 0x04u, 0u); w32img(0xA0007000u + 0x08u, 0u);
  printf("[POOL-INIT] table=8169E080 -> A0039000/A005A000/A0115000, lwmutex@A0007000\n");
  fflush(stdout);
}


extern "C" void wr32_watch(uint32_t a, uint32_t v) {
  static int g_f630_n = 0;
  if (a == 0x816CC630u && g_f630_n++ < 40) { printf("[FLAG630] v=%08X blk=%08X fib=%08X sp=%08X fex=%d thr=%08X\n", v, g_cur_block, g_cur_fiber, g_snap_r[13], g_in_fex, g_cur_thr); fflush(stdout); }

  static int g_chw_n = 0;
  static int n = 0;
  if (g_obj20_addr && a == g_obj20_addr && g_obj20w_n++ < 60) {
    printf("[OBJ20W] a=%08X v=%08X blk=%08X fib=%08X sp=%08X\n", a, v, g_cur_block, g_cur_fiber, g_snap_r[13]); fflush(stdout); }

  if (a >= 0x84C0A480u && a < 0x84C0A600u && g_chw_n++ < 300) {   // 18.4260: watch root reader / child stream object init writes
    printf("[CHW] a=%08X v=%08X blk=%08X fib=%08X sp=%08X fex=%d\n", a, v, g_cur_block, g_cur_fiber, g_snap_r[13], g_in_fex); fflush(stdout); }
  if (a >= 0x84CE12B0u && a < 0x84CE1370u && g_objh_n++ < 200) {
    printf("[OBJH] a=%08X v=%08X blk=%08X fib=%08X sp=%08X fex=%d\n", a, v, g_cur_block, g_cur_fiber, g_snap_r[13], g_in_fex); fflush(stdout); }

  if (a >= 0x85003D50u && a < 0x85003DF0u && g_objw_n++ < 60) {
    printf("[OBJW] a=%08X v=%08X blk=%08X fib=%08X sp=%08X fex=%d\n", a, v, g_cur_block, g_cur_fiber, g_snap_r[13], g_in_fex); fflush(stdout); }
  static int g_stkwh_n = 0;
  if (a >= 0xA0DFFFC0u && a < 0xA0E00000u) {   // dedicated always-on stack-top write watch
    static int owner = 0;
    if (a == 0xA0DFFFDCu) {
      if (v == 0x812B4150u) owner = 1;
      else if (owner) { printf("[CANARY-CLOBBER] A0DFFFDC v=%08X blk=%08X\n", v, g_cur_block); fflush(stdout); owner = 0; }
    }
    if (g_stkwh_n++ < 120) { printf("[STKW] a=%08X v=%08X blk=%08X enter_sp=%08X fib=%08X fex=%d\n", a, v, g_cur_block, g_snap_r[13], g_cur_fiber, g_in_fex); fflush(stdout); }
    return;
  }
  if (n > 200) return;
  if (a >= 0xA0DFFFC0u && a < 0xA0E00000u) {
    printf("[STKW] a=%08X v=%08X blk=%08X enter_sp=%08X fib=%08X fex=%d\n", a, v, g_cur_block, g_snap_r[13], g_cur_fiber, g_in_fex); fflush(stdout); ++n; return;
  }
  if (a == 0x813AFC0Cu) {
    printf("[SLOTW] GLOB813AFC0C v=%08X blk=%08X\n", v, g_cur_block); fflush(stdout); ++n; return;
  }
  if (a >= 0x816CBF0Cu && a < 0x816CBF0Cu + 0x60u * 16u) {
    printf("[SLOTW] slot a=%08X v=%08X blk=%08X\n", a, v, g_cur_block); fflush(stdout); ++n; return;
  }
  if (a >= 0x816CBC08u && a < 0x816CBC08u + 0x310u) {
    printf("[SLOTW] gsc a=%08X v=%08X blk=%08X\n", a, v, g_cur_block); fflush(stdout); ++n; return;
  }
}


static void hle_thr_pump(RecState* s, int quantum) {
  if (g_cur_thr || g_hostthreads) return;   // 18.4239: in host-thread mode the guest threads run on their own host threads
  if (g_in_fiber_exec && !g_thrpump_env) return;   // 18.4232: worker-trace mode (P5D_THRPUMP=1) allows pumping from the fiber world
  if (g_thrtrace && g_thrst_n++ < 20) { printf("[THRST] n=%zu:", g_thr.size()); for (auto& _kv : g_thr) { auto& _t=_kv.second; printf(" %08X(a%d,r%d,w%d,obj=%08X,bits=%08X,sp=%08X)", _kv.first, _t.alive?1:0, _t.ready?1:0, _t.waiting?1:0, _t.wait_obj, _t.wait_bits, _t.ctx.r[13]); } printf("\n"); fflush(stdout); }   // 18.4228: allow the pump from the main-loop world (g_cur_fiber is the app fiber there); never nest inside thread execution
  for (auto& kv : g_thr) {
    HThread& t = kv.second;
    if (!t.alive || !t.ready) continue;
    RecState saved = *s;
    std::memcpy(s->r, t.ctx.r, sizeof(t.ctx.r)); s->apsr = t.ctx.apsr;
    g_cur_thr = kv.first; g_thr_blocked = 0;
    uint32_t pc = t.ctx.r[15] ? t.ctx.r[15] : (t.entry & ~1u);
    if (g_thrtrace) { for (int _k=0;_k<16;++_k) { } if (g_thrtrace && g_thrtr_n++ < 160) { printf("[THRTR] tid=%08X entry=%08X sp=%08X r0=%08X r1=%08X r9=%08X pc=%08X\n", kv.first, t.entry, t.ctx.r[13], t.ctx.r[0], t.ctx.r[1], t.ctx.r[9], pc); fflush(stdout); } }
    int n = 0;
    while (n++ < quantum) {
      if (g_thrtrace && g_thrtr_n++ < 160) { printf("[THRBLK] tid=%08X #%d pc=%08X lr=%08X r0=%08X r4=%08X r5=%08X sp=%08X\n", kv.first, n, pc, s->r[14], s->r[0], s->r[4], s->r[5], s->r[13]); fflush(stdout); }
      if (g_thr_blocked) { t.ready = 0; t.waiting = 1; t.wait_obj = g_thr_wait_obj; t.wait_bits = g_thr_wait_bits; t.wait_mode = g_thr_wait_mode; break; }
    if ((pc & ~1u) == 0x811EA552u) { if (g_ea552_n++ < 60) { printf("[EA552] ENTER r0=%08X r1=%08X r2=%08X r7=%08X r9=%08X lr=%08X fib=%08X thr=%08X\n", s->r[0], s->r[1], s->r[2], s->r[7], s->r[9], s->r[14], g_cur_fiber, g_cur_thr); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EA5FEu) { if (g_ea552_n++ < 60) { printf("[EA552] MAPLOOP r0=%08X r1=%08X r2=%08X r7=%08X r9=%08X lr=%08X fib=%08X thr=%08X\n", s->r[0], s->r[1], s->r[2], s->r[7], s->r[9], s->r[14], g_cur_fiber, g_cur_thr); fflush(stdout); } }
    if (pc==0x811DA024u) { RecState* _ps = SPTR(s); g_last_alloc_sz = _ps->r[0]; g_last_alloc_fp = _ps->r[4]; }
    else if ((pc==0x811DA028u||pc==0x811DA02Au||pc==0x811DA030u||pc==0x811DA036u) && SPTR(s)->r[0]==0u && g_allocf_n++ < 24) { RecState* _ps = SPTR(s); printf("[ALLOCFAIL] pc=%08X size=%08X fp=%08X r0=%08X r1=%08X r4=%08X r5=%08X r6=%08X r7=%08X sp=%08X lr=%08X fib=%08X thr=%08X\n", pc, g_last_alloc_sz, g_last_alloc_fp, _ps->r[0],_ps->r[1],_ps->r[4],_ps->r[5],_ps->r[6],_ps->r[7],_ps->r[13],_ps->r[14],g_cur_fiber,g_cur_thr); fflush(stdout); }
    if ((pc & ~1u) == 0x811EAAE4u) { if (g_ea552_n++ < 60) { printf("[EA552] MATLOOP r0=%08X r1=%08X r2=%08X r7=%08X r9=%08X lr=%08X fib=%08X thr=%08X\n", s->r[0], s->r[1], s->r[2], s->r[7], s->r[9], s->r[14], g_cur_fiber, g_cur_thr); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EAB44u) { if (g_ea552_n++ < 60) { printf("[EA552] AB44 r0=%08X r1=%08X r2=%08X r7=%08X r9=%08X lr=%08X fib=%08X thr=%08X\n", s->r[0], s->r[1], s->r[2], s->r[7], s->r[9], s->r[14], g_cur_fiber, g_cur_thr); fflush(stdout); } }
            for (int _si=0;_si<16;++_si) g_snap_r[_si] = s->r[_si]; g_snap_blk = pc;
      if ((pc & ~1u) == 0x811EAB44u && g_val44_n++ < 20) { uint32_t _v=0; uint8_t* _q=GP(s->r[9]); if(_q) std::memcpy(&_v,_q,4); printf("[VAL44B] field=%08X r9=%08X fib=%08X thr=%08X\n", _v, s->r[9], g_cur_fiber, g_cur_thr); fflush(stdout); }
      { uint32_t _v=0; uint8_t* _q=GP(0x816CBF0Cu); if(_q) std::memcpy(&_v,_q,4); if (_v!=g_field_last && g_field_n++<80) { printf("[FW] blk=%08X prevblk=%08X v=%08X prev=%08X r7=%08X r9=%08X lr=%08X\n", pc, g_field_blk, _v, g_field_last, s->r[7], s->r[9], s->r[14]); fflush(stdout); } g_field_last=_v; g_field_blk=pc; }
      if ((pc & ~1u) == 0x811C0D74u && g_alloc_n++ < 40) { printf("[ALLOC] enter r0=%08X r1=%08X lr=%08X blk=%08X\n", s->r[0], s->r[1], s->r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x811C0DA8u && g_alloc2_n++ < 20) { printf("[ALLOC] crashblk r0=%08X r1=%08X lr=%08X blk=%08X\n", s->r[0], s->r[1], s->r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x811C1804u && g_alloc2_n++ < 20) { printf("[C1804] r0=%08X r5=%08X r6=%08X lr=%08X blk=%08X\n", s->r[0], s->r[5], s->r[6], s->r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x8124930Eu && g_wp_n++ < 30) { printf("[WP] r8=%08X r5=%08X fp=%08X r9=%08X lr=%08X r0=%08X blk=%08X fib=%08X thr=%08X\n", s->r[8], s->r[5], s->r[11], s->r[9], s->r[14], s->r[0], g_cur_block, g_cur_fiber, g_cur_thr); fflush(stdout); }
      if ((pc & ~1u) == 0x8124930Eu && g_wp_n++ < 3) { printf("[WPRING] blk=%08X lr=%08X r0=%08X r1=%08X r2=%08X r4=%08X r5=%08X r8=%08X ring:", g_cur_block, s->r[14], s->r[0], s->r[1], s->r[2], s->r[4], s->r[5], s->r[8]); for (int _k=0;_k<20;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 20u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout); }
      if (((pc & ~1u) == 0x81279AFEu || (pc & ~1u) == 0x81279B02u || (pc & ~1u) == 0x812B0DD0u) && g_wp_n++ < 40) { printf("[DIVP] pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X lr=%08X\n", pc, s->r[0], s->r[1], s->r[2], s->r[3], s->r[4], s->r[5], s->r[14]); fflush(stdout); }
      if ((pc & ~1u) == 0x811C0E14u && g_e14_n++ < 8) { printf("[E14P] r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r12=%08X lr=%08X blk=%08X\n", s->r[0], s->r[1], s->r[2], s->r[3], s->r[4], s->r[5], s->r[6], s->r[7], s->r[12], s->r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x811C0D74u && g_e14_n++ < 8) { printf("[E14P] FUNC_ENTRY r0=%08X r1=%08X lr=%08X blk=%08X\n", s->r[0], s->r[1], s->r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x81206700u && g_fp_n++ < 40) { printf("[FP] CALL_FACT r0=%08X r1=%08X r2=%08X lr=%08X\n", s->r[0], s->r[1], s->r[2], s->r[14]); fflush(stdout); }
      if ((pc & ~1u) == 0x81206760u && g_fp_n++ < 40) { printf("[FP] AFTER_HLE r0=%08X r4=%08X lr=%08X\n", s->r[0], s->r[4], s->r[14]); fflush(stdout); }
      if ((pc & ~1u) == 0x81198A80u && g_fp_n++ < 40) { printf("[FP] CALL_683C r5=%08X r4=%08X r0=%08X lr=%08X\n", s->r[5], s->r[4], s->r[0], s->r[14]); fflush(stdout); }
      if ((pc & ~1u) == 0x8120683Cu && g_fp_n++ < 40) { printf("[FP] ENTER_683C r0=%08X r1=%08X lr=%08X fib=%08X thr=%08X\n", s->r[0], s->r[1], s->r[14], g_cur_fiber, g_cur_thr); fflush(stdout); }
      if ((pc & ~1u) == 0x812067A0u && g_cf_n++ < 60) { printf("[CF] ENTER r0=%08X r1=%08X sp=%08X lr=%08X fib=%08X\n", s->r[0], s->r[1], s->r[13], s->r[14], g_cur_fiber); fflush(stdout); }
      if ((pc & ~1u) == 0x812067B6u && g_cf_n++ < 60) { uint32_t _cv=0; uint8_t* _cq=GP(s->r[13]+4u); if(_cq) std::memcpy(&_cv,_cq,4); printf("[CF] BODY sp=%08X [sp+4]=%08X r0=%08X r3=%08X fib=%08X\n", s->r[13], _cv, s->r[0], s->r[3], g_cur_fiber); fflush(stdout); }
      if ((pc & ~1u) == 0x8120682Eu && g_cf_n++ < 60) { printf("[CF] CMP r0=%08X r1=%08X r5=%08X sp=%08X fib=%08X\n", s->r[0], s->r[1], s->r[5], s->r[13], g_cur_fiber); fflush(stdout); }
      if ((pc & ~1u) == 0x8120682Au && g_cf_n++ < 80) { uint32_t _cv=0; uint8_t* _cq=GP(s->r[13]+4u); if(_cq) std::memcpy(&_cv,_cq,4); printf("[CF2] CMPBLK r0=%08X r1=%08X [sp+4]=%08X sp=%08X fib=%08X\n", s->r[0], s->r[1], _cv, s->r[13], g_cur_fiber); fflush(stdout); }
      if (((pc & ~1u) == 0x811E5928u || (pc & ~1u) == 0x811E5CDAu) && g_ctx_n++ < 60) { printf("[CTX] blk=%08X sp=%08X lr=%08X fib=%08X fex=%d runs=%d sw=%d ring:", g_cur_block, s->r[13], s->r[14], g_cur_fiber, g_in_fex, g_fiber_runs, g_fiber_switches); for (int _k=0;_k<12;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 12u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout); }
    if (g_hacks && (pc & ~1u) == 0x8115F9D0u) { s->r[0] = 0u; }  // [ABORTSKIP2] force checker 0 at the real block entry
      if (((pc & ~1u) == 0x812068D2u || (pc & ~1u) == 0x812068D8u || (pc & ~1u) == 0x812068E4u) && g_spoll_n++ < 60) {
        uint32_t _f80=0,_f94=0,_g18=0,_g1c=0; uint8_t* q;
        q=GP(s->r[5]+0x80u); if(q) std::memcpy(&_f80,q,4);
        q=GP(s->r[5]+0x94u); if(q) std::memcpy(&_f94,q,4);
        q=GP(0x816CC620u); if(q) std::memcpy(&_g18,q,4);
        q=GP(0x816CC624u); if(q) std::memcpy(&_g1c,q,4);
        printf("[SPOLL] pc=%08X r5=%08X r0=%08X lr=%08X f80=%08X f94=%08X g18=%08X g1c=%08X fib=%08X fex=%d fl=%zu\n", g_cur_block, s->r[5], s->r[0], s->r[14], _f80, _f94, _g18, _g1c, g_cur_fiber, g_in_fex, g_fiber_list.size()); fflush(stdout);
      }
      if (((pc & ~1u) == 0x81198BEAu || (pc & ~1u) == 0x81198BF6u || (pc & ~1u) == 0x812067A0u || (pc & ~1u) == 0x81206832u || (pc & ~1u) == 0x81206700u) && g_lp2_n++ < 80) {
        printf("[LP2] blk=%08X r0=%08X r1=%08X r4=%08X r5=%08X sp=%08X lr=%08X fib=%08X\n", g_cur_block, s->r[0], s->r[1], s->r[4], s->r[5], s->r[13], s->r[14], g_cur_fiber); fflush(stdout);
      }
    if ((pc & ~1u) == 0x81198BF6u) {
      uint32_t _lc=0; uint8_t* _lq=GP(0x816CC630u); if(_lq) std::memcpy(&_lc,_lq,4);
      if ((_lc & 2u) && g_loadspin_n++ > (g_loadclear_env ? 200u : 100000000u)) { _lc &= ~2u; if(_lq) std::memcpy(_lq,&_lc,4); printf("[LOADCLEAR] cleared bit1 at 0x816CC630 (iters=%d) -> %08X fib=%08X\n", g_loadspin_n, _lc, g_cur_fiber); fflush(stdout); }
    }
      if ((pc & ~1u) == 0x8127C1D2u && g_c1d2_n++ < 12) {
        printf("[C1D2] enter r0=%08X r1=%08X r2=%08X lr=%08X sp=%08X fib=%08X thr=%08X ring:", s->r[0], s->r[1], s->r[2], s->r[14], s->r[13], g_cur_fiber, g_cur_thr);
        for (int _k=0;_k<20;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 20u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
      }
      if (((pc & ~1u) == 0x8127C1CAu || (pc & ~1u) == 0x8127C1C0u || (pc & ~1u) == 0x8127C1C6u) && g_c1d2_n++ < 30) {
        printf("[C1D2] pre pc=%08X r0=%08X r1=%08X lr=%08X sp=%08X\n", g_cur_block, s->r[0], s->r[1], s->r[14], s->r[13]); fflush(stdout);
      }
if (g_in_fiber_exec && g_cur_fiber && g_fiber_list.size() > 1 && g_cur_fiber == g_fiber_list[1] && ((pc & ~1u) == 0x81207956u || (pc & ~1u) == 0x8120794Au || (pc & ~1u) == 0x81207B3Au || (pc & ~1u) == 0x81207B54u) && g_f2blk_n++ < 40) {
  printf("[F2BLK] pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", pc, s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], s->r[13]); fflush(stdout);
}
if ((pc & ~1u) == 0x81182C9Au && g_nx9a_n++ < 10) {
  printf("[NX9A] r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X sp=%08X lr=%08X fib=%08X ring:", s->r[0], s->r[1], s->r[2], s->r[3], s->r[4], s->r[5], s->r[6], s->r[7], s->r[8], s->r[13], s->r[14], g_cur_fiber);
  for (int _k=0;_k<20;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 20u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
}
{ static int _p[4] = {0,0,0,0};
  if ((pc & ~1u) == 0x81198A76u && _p[0]++ < 4) printf("[FLPROBE] init-set-flag block reached r2=%08X lr=%08X fib=%08X\n", s->r[2], s->r[14], g_cur_fiber);
  if ((pc & ~1u) == 0x81198C8Eu && _p[1]++ < 6) printf("[FLPROBE] frame-loop-top reached flags[=816CC630]=%08X fib=%08X sp=%08X\n", (uint32_t)(GP(0x816CC630u) ? *(uint32_t*)GP(0x816CC630u) : 0xDEADu), g_cur_fiber, s->r[13]);
  if ((pc & ~1u) == 0x81198CE4u && _p[2]++ < 6) printf("[FLPROBE] frame-counter-inc reached r5=%08X fib=%08X\n", s->r[5], g_cur_fiber);
  if ((pc & ~1u) == 0x81198D04u && _p[3]++ < 6) printf("[FLPROBE] frame-loop-test reached r0=%08X fib=%08X\n", s->r[0], g_cur_fiber);
  fflush(stdout); }
if ((pc & ~1u) == 0x811830C8u && g_w8c8_n++ < 6) {
  printf("[W8C8] enter r0=%08X r1=%08X lr=%08X sp=%08X fib=%08X ring:", s->r[0], s->r[1], s->r[14], s->r[13], g_cur_fiber);
  for (int _k=0;_k<24;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 24u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
}
if ((pc & ~1u) == 0x811830C8u && g_w8c8_watch == 0 && s->r[0] >= 0x80000000u) {
  g_w8c8_watch = s->r[0] + 4u; g_watch_addr = g_w8c8_watch; g_watch_val = 0xDEADBEEFu;
  uint8_t* _wq = GP(g_watch_addr); if (_wq) std::memcpy(&g_watch_val, _wq, 4);
  printf("[DWATCH] object=%08X state=%08X initial=%08X\n", s->r[0], g_watch_addr, g_watch_val); fflush(stdout);
}
if ((pc & ~1u) == 0x811E5C98u && g_cb98_n++ < 8) {
  uint32_t _cb[6] = {0,0,0,0,0,0}; const uint32_t _addrs[6] = {0x81513DD4u,0x81513DD8u,0x81513DDCu,0x81513DE0u,0x81513DE4u,0x81513DE8u};
  for (int _i=0;_i<6;++_i) { uint8_t* _q = GP(_addrs[_i]); if (_q) std::memcpy(&_cb[_i], _q, 4); }
  printf("[CB98] cb=%08X/%08X/%08X/%08X/%08X/%08X r0=%08X r1=%08X lr=%08X fib=%08X\n", _cb[0],_cb[1],_cb[2],_cb[3],_cb[4],_cb[5], s->r[0], s->r[1], s->r[14], g_cur_fiber); fflush(stdout);
}
if ((pc & ~1u) == 0x811DD4BCu && g_xswap_n++ < 20) {
  printf("[XSWAP] r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X lr=%08X sp=%08X fib=%08X mem:", s->r[0], s->r[1], s->r[2], s->r[3], s->r[4], s->r[5], s->r[14], s->r[13], g_cur_fiber);
  uint32_t _w0=0,_w1=0,_w2=0; uint8_t* _q;
  _q = GP((s->r[4]) + 0x0u); if (_q) std::memcpy(&_w0,_q,4);
  _q = GP((s->r[4]) + 0x18u); if (_q) std::memcpy(&_w1,_q,4);
  _q = GP((s->r[4]) + 0x1cu); if (_q) std::memcpy(&_w2,_q,4);
  printf(" hdr=%08X/%08X/%08X\n", _w0, _w1, _w2); fflush(stdout);
  if (g_hacks && s->r[2] == 0xFFFFFFFFu) { s->r[2] = 0u; printf("[XSKIP3] length -1 -> 0\n"); fflush(stdout); }
}
if (((pc & ~1u) == 0x811DEAB4u || (pc & ~1u) == 0x811DEB44u || (pc & ~1u) == 0x811DEB34u) && g_obj24_n++ < 24) {
  printf("[OBJ24] pc=%08X r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X fib=%08X", s->r[15], s->r[0], s->r[1], s->r[2], s->r[4], s->r[14], g_cur_fiber);
  uint32_t _o = s->r[4]; if (_o >= 0x80000000u) {
    uint32_t _w[10]; for (int _i=0;_i<10;++_i) { _w[_i]=0; uint8_t* _q=GP(_o + (uint32_t)_i*4u); if (_q) std::memcpy(&_w[_i],_q,4); }
    printf(" struct: %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X", _w[0],_w[1],_w[2],_w[3],_w[4],_w[5],_w[6],_w[7],_w[8],_w[9]);
  }
  printf("\n"); fflush(stdout);
}
if (((pc & ~1u) == 0x811C6C72u || (pc & ~1u) == 0x811C6C84u || (pc & ~1u) == 0x811C6C8Cu) && g_cb0_n++ < 18) {
  uint32_t _h = s->r[4]; uint32_t _s = 0, _f8c = 0, _f90 = 0;
  if (_h >= 0x80000000u) { uint8_t* _q;
    _q = GP(_h + 4u); if (_q) std::memcpy(&_s, _q, 4);
    _q = GP(_h + 0x8cu); if (_q) std::memcpy(&_f8c, _q, 4);
    _q = GP(_h + 0x90u); if (_q) std::memcpy(&_f90, _q, 4); }
  printf("[CB0] pc=%08X r0=%08X r4=%08X state=%08X f8c=%08X f90=%08X r2=%08X lr=%08X fib=%08X\n", s->r[15], s->r[0], s->r[4], _s, _f8c, _f90, s->r[2], s->r[14], g_cur_fiber); fflush(stdout);
}
if ((pc & ~1u) == 0x8121D044u) { ++g_d044_n;
  if (g_d044_n < 8) {
    uint32_t _w38=0,_w44=0,_w50=0; uint8_t* _q;
    _q = GP(s->r[13] + 0x38u); if (_q) std::memcpy(&_w38,_q,4);
    _q = GP(s->r[13] + 0x44u); if (_q) std::memcpy(&_w44,_q,4);
    _q = GP(s->r[13] + 0x50u); if (_q) std::memcpy(&_w50,_q,4);
    printf("[D044] #%u sp=%08X w38=%08X w44=%08X w50=%08X lr=%08X fib=%08X ring:", g_d044_n, s->r[13], _w38, _w44, _w50, s->r[14], g_cur_fiber);
    for (int _k=0;_k<16;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 16u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
  } }
if ((pc & ~1u) == 0x81207734u) { ++g_allocbig_total;
  if (s->r[0] > 0x2000000u && g_allocbig_n++ < 12) {
    printf("[ALLOCBIG] #%u r0(size)=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X fib=%08X ring:", g_allocbig_n, s->r[0], s->r[1], s->r[2], s->r[3], s->r[14], s->r[13], g_cur_fiber);
    for (int _k=0;_k<14;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 14u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
  } }
if ((pc & ~1u) == 0x81207290u && s->r[0] < 0x80000000u && g_atombad_n++ < 20) {
  printf("[ATOMBAD] #%u r0=%08X r1=%08X r2=%08X lr=%08X sp=%08X fib=%08X ring:", g_atombad_n, s->r[0], s->r[1], s->r[2], s->r[14], s->r[13], g_cur_fiber);
  for (int _k=0;_k<16;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 16u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
}
if (g_hacks && (pc & ~1u) == 0x811F019Eu && s->r[0] == 0u) { s->r[15] = s->r[14]; if (g_refnull_n++ < 8) { printf("[REFNULL] skip ref(null) lr=%08X fib=%08X\n", s->r[14], g_cur_fiber); fflush(stdout); } continue; }
if ((pc & ~1u) == 0x811E3520u && g_obj20_n++ < 12) {
  uint32_t _o = s->r[4]; uint32_t _v = 0; uint8_t* _q = GP(_o + 0x20u); if (_q) std::memcpy(&_v,_q,4);
  if (_o >= 0x80000000u) { g_obj20_addr = _o + 0x20u; }
  printf("[OBJ20] r4=%08X [r4+0x20]=%08X sp=%08X fib=%08X ring:", _o, _v, s->r[13], g_cur_fiber);
  for (int _k=0;_k<10;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 10u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
}
if ((pc & ~1u) == 0x811C411Cu && g_efe9a_n++ < 6) {
  uint8_t* _q = GP(0x811EFE9Au);
  if (_q) printf("[EFE9A] mem@811EFE9A = %02X %02X %02X %02X %02X %02X %02X %02X  (elf expects 00 20 70 47)\n", _q[0],_q[1],_q[2],_q[3],_q[4],_q[5],_q[6],_q[7]);
  else printf("[EFE9A] GP null\n");
  fflush(stdout);
}
ef2_probe(pc, s, 1);
if (g_hacks && (pc & ~1u) == 0x811E2B50u) {
  uint32_t _a = 0, _b = 0, _c = 0; uint8_t* _q = GP(s->r[1] + 0x40u); if (_q) std::memcpy(&_a,_q,4);
  if (_a >= 0x80000000u) { _q = GP(_a + 0x78u); if (_q) std::memcpy(&_b,_q,4); }
  if (_b >= 0x80000000u) { _q = GP(_b + 0x20u); if (_q) std::memcpy(&_c,_q,4); }
  if (_a >= 0x80000000u && _b >= 0x80000000u && _c == 0u) { s->r[15] = s->r[14];
    if (g_ret1_n++ < 8) { printf("[RET1] skip FUN_811e2b50: model+0x20 null (r1=%08X lr=%08X fib=%08X)\n", s->r[1], s->r[14], g_cur_fiber); fflush(stdout); } continue; }
}
if (family_skip(s, pc)) continue;
if (nulldesc_skip(s, pc)) continue;
if (assert_hle(s, pc)) continue;
assert_msg_probe(s, pc);
initwatch_probe(s, pc);
g_loop_site = 1; ++g_loop_cnt[1]; g_current_loop_pc = (int)pc; h7344_probe(s, pc);
    fix682a(s, &pc);
BlockFn fn = gbn(pc);
      if (!fn) { int _back = 0; uint32_t _rp = gbn_back(pc, &_back); if (_rp) { pc = _rp; fn = gbn(pc); } }
      if (!fn) { if (g_thr_log_n++ < 40) printf("[THR-BAD] tid=%08X pc=%08X\\n", kv.first, pc); t.alive = 0; break; }
      uint32_t np = fn(s);
      if (np == 0xFFFFFFF0u) { if (g_thr_log_n++ < 40) printf("[THR] exit tid=%08X blocks=%d\\n", kv.first, n); t.alive = 0; t.ready = 0; break; }
      pc = np;
    }
    std::memcpy(t.ctx.r, s->r, sizeof(t.ctx.r)); t.ctx.apsr = s->apsr; t.ctx.r[15] = pc;
    *s = saved;
    g_cur_thr = 0;
  }
}

static int g_wd_started = 0;
static void wd_loop() {
  uint32_t last_n = 0; int stall = 0;
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    uint32_t n = g_blkcnt_total;
    if (n == last_n) { ++stall; } else { stall = 0; last_n = n; }
    printf("[WD] n=%u blk=%08X fib=%08X thr=%08X stall=%d hle_ring:", n, g_cur_block, g_cur_fiber, g_cur_thr, stall);
    int cnt = g_hle_ring_n < 12 ? g_hle_ring_n : 12;
    for (int i = 0; i < cnt; ++i) { int idx = (g_hle_ring_n - cnt + i) & 127; printf(" %08X@%08X", g_hle_ring_va[idx], g_hle_ring_lr[idx]); }
    printf("\n[WD] pcring:");
    for (int i = 0; i < 16; ++i) printf(" %08X", g_pc_ring[(g_pc_ring_n - 16u + (uint32_t)i) & 63u]);
    printf("\n"); fflush(stdout);
  }
}
static LONG WINAPI dsh_crash_filter(EXCEPTION_POINTERS* ep) {
  printf("[CRASH] code=%08X addr=%p blk=%08X fib=%08X pc_last=%08X\n",
         ep->ExceptionRecord->ExceptionCode, (void*)ep->ExceptionRecord->ExceptionAddress,
         g_cur_block, g_cur_fiber, g_pc_ring[(g_pc_ring_n - 1) & 63]);
  printf("[CRASH] pcring:");
  for (int i = 0; i < 24; ++i) printf(" %08X", g_pc_ring[(g_pc_ring_n - 24u + (uint32_t)i) & 63u]);
  printf("\n"); fflush(stdout);
  return EXCEPTION_EXECUTE_HANDLER;
}

static void shutdown_threads() {
  g_shutdown = 1;
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  for (auto& kv : g_thr) { if (kv.second.hth) { if (kv.second.hth->joinable()) kv.second.hth->join(); } }
}
int main(int argc, char** argv) {
  SetUnhandledExceptionFilter(dsh_crash_filter);
  std::atexit(shutdown_threads);
  if (!g_wd_thread) { g_wd_thread = 1; std::thread([](){
      for (;;) { std::this_thread::sleep_for(std::chrono::seconds(2));
        printf("[WD] loop=%d cnt=%u/%u/%u/%u total=%u blk=%08X fib=%08X fex=%d ex=%d pump=%d curpc=%08X\n",
               g_loop_site, g_loop_cnt[0], g_loop_cnt[1], g_loop_cnt[2], g_loop_cnt[3], g_blkcnt_total, g_cur_block, g_cur_fiber, g_in_fex, g_in_fiber_exec, g_pumping2, g_current_loop_pc);
        printf("[WDRING]"); for (int i = 52; i < 64; ++i) printf(" %08X", g_pc_ring[(g_pc_ring_n + (unsigned)i) & 63u]); printf("\n");
        for (auto& kv : g_fctx) { printf("[WDCTX] fib=%08X pc=%08X sp=%08X r0=%08X r5=%08X\n", kv.first, kv.second.r[15], kv.second.r[13], kv.second.r[0], kv.second.r[5]); }
        fflush(stdout);
      } }).detach(); }
  { const char* _pw = getenv("P5D_PUMP_WALLCLOCK"); g_pump_wallclock = (_pw && atoi(_pw) != 0) ? 1 : 0; }
  { const char* _cy = getenv("P5D_CB_YIELD"); g_cb_yield = (_cy && atoi(_cy) != 0) ? 1 : 0; }
  { const char* _fn = getenv("P5D_FIBER_NESTED"); g_fiber_nested = (_fn && atoi(_fn) != 0) ? 1 : 0; }
  { const char* _nu = getenv("P5D_NESTED_UNTIL"); g_nested_until = _nu ? (uint32_t)strtoul(_nu, 0, 0) : 0u; }
  { const char* _lc = getenv("P5D_LOADCLEAR"); g_loadclear_env = (_lc && atoi(_lc) != 0) ? 1 : 0; }
  { const char* _bt = getenv("P5D_BOOT_THUNK"); g_boot_thunk = (_bt && atoi(_bt) != 0) ? 1 : 0; }
  { const char* _hx = getenv("P5D_HACKS"); g_hacks = (_hx && atoi(_hx) != 0) ? 1 : 0;
  { const char* _ht = getenv("P5D_HOSTTHREADS"); g_hostthreads = (_ht && atoi(_ht) != 0) ? 1 : 0; }
  { const char* _tp = getenv("P5D_THRPUMP"); g_thrpump_env = (_tp && atoi(_tp) != 0) ? 1 : 0; }   // 18.4265: re-wire the fiber-world thread pump (was dead code since the env parse was missing)
    printf("[HACKS] semantic patch stack %s\n", g_hacks ? "ENABLED (P5D_HACKS=1)" : "DISABLED (clean baseline)"); fflush(stdout); }
  g_target_alloc = 0x8534B160u;   // 18.4194: trace who allocates the object whose +0x20 is null
  g_obj20_addr = 0x8534B180u;     // 18.4194b: watch [object+0x20] writes from the very start (deterministic heap)
  std::atexit([]{
    printf("[PCRING]");
    for (int i = 0; i < 64; ++i) printf(" %08X", g_pc_ring[(g_pc_ring_n + (uint32_t)i) & 63u]);
    printf("\n");
  });
  setvbuf(stdout, NULL, _IONBF, 0);
  uint32_t start = argc > 1 ? (uint32_t)strtoul(argv[1], nullptr, 0) : 0x81198464u;
  int maxb = argc > 2 ? atoi(argv[2]) : 50000;
  if (!load_elf("C:\\dev\\p5d_build\\p5d_true.elf")) return 2;
  gxmvk_init();
  init_job_pools();
  { uint32_t o = 0x8130CC68u - IMG_VA_BASE; uint32_t v = 0;
    if (o + 4 <= IMG_SIZE) std::memcpy(&v, g_img.data() + o, 4);
    printf("[INIT8130] after load [0x8130CC68]=%08X (ELF expects 0C20D060) [0x8130CC6C]=%08X\n", v, [](uint8_t* b, uint32_t oo){ uint32_t x; std::memcpy(&x, b+oo+4, 4); return x; }(g_img.data(), o)); }
  build_all_blocks();
  if (!g_wd_started) { g_wd_started = 1; std::thread(wd_loop).detach(); }   // 18.4179: stall watchdog
  RecState s; memset(&s, 0, sizeof(s));
  s.base = g_img.data(); s.mem_size = IMG_SIZE;
  s.r[13] = 0x82F00000u;
  // 18.3698: initialize the global list/array area 0x816CE000..0x816CF000 (0x816CDA2C+0x13FC list head lives here;
  // the game's own memset only covers up to 0x816CDAA0, so this area must be zero-initialized)
  { uint32_t o = 0x816CE000u - IMG_VA_BASE; if (o + 0x1000 <= IMG_SIZE) std::memset(g_img.data() + o, 0, 0x1000); }
  // 18.3652 hypothesis test: the game reads the main heap/state pointer from global 0x816CDA44; initialize it
  // (and the related global word 0x816CDA3C..0x816CDA50) with plausible pointers inside our image heap area.
  { auto put = [&](uint32_t va, uint32_t v) { uint32_t o = va - IMG_VA_BASE; if (o + 4 <= IMG_SIZE) { std::memcpy(g_img.data()+o, &v, 4); } };
    // real values dumped from the probe for the 0x816CDA3C global area
    put(0x816CDA3Cu, 0xA000B000u);
    put(0x816CDA40u, 0xA001E000u);
    put(0x816CDA44u, 0x00000080u);
    put(0x816CDA48u, 0xA0028000u);
    put(0x816CDA4Cu, 0x00000100u);
    put(0x816CDA50u, 0xA0032000u);
    put(0x816CDA54u, 0x00000100u); }
  s.r[14] = 0xFFFFFFFFu;
  printf("start=%08X max_blocks=%d SP=%08X\n", start, maxb, s.r[13]);
  RecState g_init_state = s;  // [RESTART] snapshot for re-running the guest main thread after it returns
  uint32_t pc = start;
  int n = 0;
  uint32_t prev_blk = 0;
  while (n < maxb) {
    if (g_pump_cool > 0) --g_pump_cool;   // 18.4181: pump cooldown tick
    if ((n & 0x1FF) == 0 && !g_thr.empty()) hle_thr_pump(&s, 400);
    if (false && n == 20000) {   // 18.4176: INJECT disabled - the injected fake load-request nodes are what the wait callback 0x811830C8 was waiting on
      // 18.3841: inject MANY load requests (a linked chain) using real CPK names - one node = one file.
      // node format: +0 flags, +4 name ptr, +8 -1, +0xc/+0x10/+0x14 payload, +0x20 -1,
      // +0x24 prev, +0x28 next, +0x2c name buffer.
      uint32_t head = 0, created = 0; int idx = 0;
      for (auto& kv : g_cpk.entries()) {
        if ((idx++ % 10) != 0) continue;
        if (created >= 300) break;
        uint32_t node = hle_malloc(0x2c + 256);
        uint8_t* p = GP(node);
        if (!p) continue;
        std::memset(p, 0, 0x2c + 256);
        uint32_t nameptr = node + 0x2cu;
        uint32_t f[8] = { 0u, nameptr, 0xFFFFFFFFu, 0u, 0u, 0u, 0xFFFFFFFFu, 0u };
        std::memcpy(p, f, sizeof(f));
        const char* nm = kv.first.c_str();
        std::memcpy(p + 0x2cu, nm, std::strlen(nm) + 1);
        uint32_t zero = 0; std::memcpy(p + 0x24u, &zero, 4); std::memcpy(p + 0x28u, &head, 4);
        head = node; ++created;
      }
      uint8_t* hd = GP(0x816CDA2Cu); if (hd) std::memcpy(hd, &head, 4);
      printf("[INJECT] load-request chain head=%08X count=%u\n", head, created);
    }
    if ((pc & ~1u) == 0x8120732Cu && s.r[4] == 0u) { s.r[4] = 0x82DFFF00u;
      if (g_nullout_n++ < 8) printf("[NULLOUT-RG] block 8120732C r4==0 -> scratch\n"); }
    if ((pc & ~1u) == 0x812067B6u && (s.r[3] == 0u || s.r[13] < 0x82E00000u))
      printf("[B67B6-RG] r3=%08X r13=%08X r0=%08X prev=%08X cur=%08X\n", s.r[3], s.r[13], s.r[0], g_prev_block, g_cur_fiber);
    if ((pc & ~1u) == 0x8115D29Au && g_ep_n++ < 10) {
      printf("[EPI-RG] sp=%08X cur=%08X stk:", s.r[13], g_cur_fiber);
      for (int i = 0; i < 9; ++i) { uint32_t w = 0; uint8_t* q = GP(s.r[13] + 4u + (uint32_t)i * 4u); if (q) std::memcpy(&w, q, 4); printf(" %08X", w); }
      { auto itc = g_fctx.find(g_cur_fiber); printf(" fcsp=%08X\n", itc != g_fctx.end() ? itc->second.r[13] : 0u); }
    }
    { uint32_t i = g_drg_n++ % 20u; g_drg_pc[i] = pc; g_drg_r0[i] = s.r[0]; g_drg_sp[i] = s.r[13]; }
    if ((pc & ~1u) == 0x812067B6u && g_b6_n++ < 12) {
      uint32_t w0 = 0; uint8_t* q0 = GP(s.r[13]); if (q0) std::memcpy(&w0, q0, 4);
      printf("[B67B6-ENTRY-RG] r0=%08X r1=%08X r2=%08X r3=%08X sp=%08X [sp]=%08X cur=%08X in_fex=%d in_fexec=%d\n",
             s.r[0], s.r[1], s.r[2], s.r[3], s.r[13], w0, g_cur_fiber, g_in_fex, g_in_fiber_exec);
      if (w0 == 0u) {
        printf("[B67B6-BAD] pcring:");
        for (int k = 0; k < 64; ++k) printf(" %08X", g_pc_ring[(g_pc_ring_n + (uint32_t)k) & 63u]);
        printf("\n[B67B6-BAD] trring:");
        for (int k = 0; k < 16; ++k) printf(" %08X", g_tr_pc[(g_tr_n + (uint32_t)k) & 15u]);
        printf("\n");
      }
    }
    if (((pc & ~1u) == 0x811E6A08u || (pc & ~1u) == 0x811E6A0Eu) && g_e6a_n++ < 20) {
      uint32_t w0 = 0; uint8_t* q0 = GP(s.r[13]); if (q0) std::memcpy(&w0, q0, 4);
      printf("[E6A] pc=%08X sp=%08X [sp]=%08X r0=%08X r4=%08X r14=%08X cur=%08X in_fexec=%d\n",
             pc, s.r[13], w0, s.r[0], s.r[4], s.r[14], g_cur_fiber, g_in_fiber_exec);
    }
    if (g_stack_watch) {
      uint32_t _w = 0; uint8_t* _q = GP(s.r[13]); if (_q) std::memcpy(&_w, _q, 4);
      if (_w != g_stk_last && g_stkchg_n++ < 40) {
        printf("[STKCHG] prev_blk=%08X now_pc=%08X sp=%08X stk0=%08X was=%08X r0=%08X\n",
               g_cur_block, pc, s.r[13], _w, g_stk_last, s.r[0]);
        g_stk_last = _w;
      }
    }
    if ((pc & ~1u) == 0x811DA534u) { g_stack_watch = 1; g_stk_last = 0; }
    if ((pc & ~1u) == 0x8115D206u) { g_stack_watch = 0; }
    if (g_slot_watch && g_save_slot) {
      uint32_t _sv = 0; uint8_t* _sq = GP(g_save_slot); if (_sq) std::memcpy(&_sv, _sq, 4);
      if (_sv != 0x8115D1F6u && g_slot_n++ < 20) {
        printf("[SAVESLOT] clobbered! slot=%08X val=%08X prev_blk=%08X now_pc=%08X sp=%08X r0=%08X\n",
               g_save_slot, _sv, g_cur_block, pc, s.r[13], s.r[0]);
      }
    }
    if ((pc & ~1u) == 0x8115D200u) { g_save_slot = s.r[13] - 4u; g_slot_watch = 1; if (g_slot_n < 20) printf("[SAVESLOT] armed slot=%08X (sp=%08X)\n", g_save_slot, s.r[13]); }
    if ((pc & ~1u) == 0x8115D206u) {
      { uint32_t _v0 = 0, _v1 = 0; uint8_t* _q0 = GP(s.r[13]); uint8_t* _q1 = GP(s.r[13] + 4u);
        if (_q0) std::memcpy(&_v0, _q0, 4); if (_q1) std::memcpy(&_v1, _q1, 4);
        printf("[SAVESLOT-POP] sp=%08X [sp]=%08X [sp+4]=%08X slotwatch=%d n=%d\n", s.r[13], _v0, _v1, g_slot_watch, g_slot_n); }
      g_slot_watch = 0;
    }
    if ((pc & ~1u) == 0x8115D29Au && g_d29a_n++ < 6) {
      uint32_t _a=0,_b=0,_c=0; uint8_t* _p0=GP(s.r[13]); if(_p0) std::memcpy(&_a,_p0,4);
      uint8_t* _p1=GP(s.r[13]+4u); if(_p1) std::memcpy(&_b,_p1,4);
      uint8_t* _p2=GP(s.r[13]+8u); if(_p2) std::memcpy(&_c,_p2,4);
      uint32_t _d=0,_e=0,_f=0,_g=0,_h2=0;
      { uint8_t* _q=GP(s.r[13]+0x20u); if(_q) std::memcpy(&_d,_q,4); }
      { uint8_t* _q=GP(s.r[13]+0x24u); if(_q) std::memcpy(&_e,_q,4); }
      { uint8_t* _q=GP(s.r[13]+0x28u); if(_q) std::memcpy(&_f,_q,4); }
      { uint8_t* _q=GP(s.r[13]+0x2Cu); if(_q) std::memcpy(&_g,_q,4); }
      { uint8_t* _q=GP(s.r[13]+0x30u); if(_q) std::memcpy(&_h2,_q,4); }
      printf("[D29A] sp=%08X [sp]=%08X [sp+4]=%08X [sp+8]=%08X [sp+20]=%08X [sp+24]=%08X [sp+28]=%08X [sp+2C]=%08X [sp+30]=%08X r0=%08X r14=%08X cur=%08X\n",
             s.r[13], _a, _b, _c, _d, _e, _f, _g, _h2, s.r[0], s.r[14], g_cur_fiber);
    }
    if ((pc & ~1u) == 0x81207308u) {
      uint32_t _lr = s.r[14];
      if (_lr >= 0x812B3000u || _lr < 0x81000000u || g_d308_n++ < 3) {
        printf("[D308] lr=%08X sp=%08X r0=%08X r4=%08X r5=%08X (n=%d)\n", _lr, s.r[13], s.r[0], s.r[4], s.r[5], g_d308_n);
        fflush(stdout);
      }
    }
    if ((pc & ~1u) == 0x8120732Cu) {
      uint32_t _lr2 = s.r[14];
      uint32_t _w2[8]; for (int _i = 0; _i < 8; ++_i) { _w2[_i] = 0; uint8_t* _q = GP(s.r[13] + (uint32_t)(_i * 4) + 0x8u); if (_q) std::memcpy(&_w2[_i], _q, 4); }
      uint32_t _pb = 0, _pn = 0; if (g_mr_n > 0) { uint32_t _k = (g_mr_n - 1u) % 40u; _pb = g_mr_pc[_k]; _pn = g_mr_nx[_k]; }
      printf("[D72C] lr=%08X sp=%08X [sp+8]=%08X [sp+C]=%08X [sp+10]=%08X [sp+14]=%08X [sp+18]=%08X [sp+1C]=%08X prev=%08X>%08X\n",
             _lr2, s.r[13], _w2[0], _w2[1], _w2[2], _w2[3], _w2[4], _w2[5], _pb, _pn);
      if (_w2[3] == 0x81555E98u) {
        printf("[D72C-BAD] mr:"); for (int _i = 0; _i < 40; ++_i) { uint32_t _k = (g_mr_n + (uint32_t)_i) % 40u; printf(" %08X>%08X", g_mr_pc[_k], g_mr_nx[_k]); } printf("\n");
        printf("[D72C-BAD] fe:"); for (int _i = 0; _i < 20; ++_i) printf(" %08X", g_dfe_pc[(g_dfe_n + (uint32_t)_i) % 20u]); printf("\n"); printf("[D72C-BAD] e6a1e_sp(n=%u):", g_e6a1e_n); for (int _i = 0; _i < 8; ++_i) printf(" %08X", g_e6a1e_sp[(g_e6a1e_n + (uint32_t)_i) & 7u]); printf("\n"); printf("[D72C-BAD] pre:"); for (int _i = 0; _i < 8; ++_i) printf(" %08X", g_pre_pc[(g_pre_idx + (uint32_t)_i) & 7u]); printf(" ctx:"); for (auto& _kv : g_fctx) printf(" %08X/sp=%08X", _kv.first, _kv.second.r[13]); printf("\n");
      }
      fflush(stdout);
    }
    if (g_rslot_watch) {
      uint32_t _sv = 0; uint8_t* _sq = GP(g_rslot_addr); if (_sq) std::memcpy(&_sv, _sq, 4);
      if (_sv != g_rslot_exp && g_rslotclob_n++ < 20) {
        printf("[SLOTCLOB] val=%08X exp=%08X prev_blk=%08X now_pc=%08X sp=%08X\n", _sv, g_rslot_exp, g_cur_block, pc, s.r[13]);
        fflush(stdout);
      }
    }
    if ((pc & ~1u) == 0x81207308u) { g_rslot_addr = s.r[13] - 4u; g_rslot_exp = s.r[14]; g_rslot_watch = 1; }
    if ((pc & ~1u) == 0x8120732Cu) { g_rslot_watch = 0; }
    if ((pc & ~1u) == 0x811E5928u && g_5928_n++ < 20) {
      printf("[5928] cur=%08X sp=%08X r14=%08X r0=%08X (n=%d)\n", g_cur_fiber, s.r[13], s.r[14], s.r[0], g_5928_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8120682Au && g_682a_n++ < 20) {
      printf("[682A] r5=%08X r13=%08X r0=%08X r1=%08X r14=%08X cur=%08X (n=%d)\n", s.r[5], s.r[13], s.r[0], s.r[1], s.r[14], g_cur_fiber, g_682a_n);
      { uint32_t _sv = 0, _gv = 0; uint8_t* _q1 = GP(s.r[13] + 4u); if (_q1) std::memcpy(&_sv, _q1, 4); uint8_t* _q2 = GP(0x812B414Cu); if (_q2) std::memcpy(&_gv, _q2, 4); printf("[CANARY682A] saved=%08X global=%08X match=%d sp=%08X r5=%08X in_exec=%d\n", _sv, _gv, (int)(_sv == _gv), s.r[13], s.r[5], g_in_fiber_exec); fflush(stdout); }
      if (s.r[5] == 0u) { s.r[5] = 0x812B414Cu; printf("[682A-R5FIX] r5 0 -> 0x812B414C (sp=%08X)\n", s.r[13]); fflush(stdout); }
      if (s.r[5] == 0u) { printf("[682A-ZERO] fctx:"); for (auto& _kv : g_fctx) printf(" %08X:pc=%08X sp=%08X r5=%08X", _kv.first, _kv.second.r[15], _kv.second.r[13], _kv.second.r[5]); printf("\n"); fflush(stdout); }
      if (s.r[5] == 0u && s.r[13] == 0xA0DFFFE8u) { s.r[5] = 0x812B414Cu; s.r[13] = 0xA0DFFFD0u; { uint8_t* _q = GP(s.r[13] + 4u); if (_q) { uint32_t _v = 0x812B4150u; std::memcpy(_q, &_v, 4); } } g_cw_on = 1; g_cw_snap = 0x812B4150u; printf("[682A-FIX2] emulated prologue + saved-r5 slot: r5=812B414C sp=A0DFFFD0 [sp+4]=812B414C\n"); fflush(stdout); }
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8120679Cu && g_679c_n++ < 20) {
      printf("[679C] r0=%08X r4=%08X r5=%08X r13=%08X cur=%08X (n=%d)\n", s.r[0], s.r[4], s.r[5], s.r[13], g_cur_fiber, g_679c_n);
      fflush(stdout);
    }
    if (((pc & ~1u) == 0x81198A84u || (pc & ~1u) == 0x81198A92u) && g_call683c_n++ < 20) {
      printf("[CALL683C] pc=%08X r0=%08X r4=%08X r5=%08X r13=%08X (n=%d)\n", pc, s.r[0], s.r[4], s.r[5], s.r[13], g_call683c_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8120683Cu && g_683c_n++ < 20) {
      printf("[683C] r0=%08X r4=%08X r5=%08X r13=%08X prev_blk=%08X cur=%08X in_exec=%d (n=%d)\n", s.r[0], s.r[4], s.r[5], s.r[13], g_cur_block, g_cur_fiber, g_in_fiber_exec, g_683c_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8120682Au && s.r[13] == 0xA0DFFFE8u) {
      uint32_t _glob = 0; uint8_t* _qg = GP(0x812B414Cu); if (_qg) std::memcpy(&_glob, _qg, 4);
      uint32_t _old = 0; uint8_t* _qs = GP(s.r[13] + 4u); if (_qs) std::memcpy(&_old, _qs, 4);
      if (_glob && _old != _glob) {
        uint8_t* _qw = GP(s.r[13] + 4u); if (_qw) std::memcpy(_qw, &_glob, 4);
        printf("[CANARY-FIX] sp=%08X [sp+4]: %08X -> %08X (n=%d)\n", s.r[13], _old, _glob, ++g_canaryfix_n);
        fflush(stdout);
      }
    }
    if (g_cw_on) { uint32_t _cv = 0; uint8_t* _cq = GP(0xA0DFFFD4u); if (_cq) std::memcpy(&_cv, _cq, 4); if (_cv != g_cw_snap && g_cw_n++ < 12) { printf("[CWATCH] prev=%08X now=%08X old=%08X new=%08X sp=%08X cur=%08X\n", g_cur_block, pc, g_cw_snap, _cv, s.r[13], g_cur_fiber); fflush(stdout); g_cw_on = 0; } }
    if ((pc & ~1u) == 0x8120682Au && s.r[5] == 0u) { s.r[5] = 0x812B414Cu; }  // [R5HOIST]
    if ((pc & ~1u) == 0x8120682Au && g_fiberdbg2_n++ < 10) {
      uint32_t _h18=0,_h1c=0,_f1p=0,_f1n=0,_f2p=0,_f2n=0;
      uint8_t* _q; _q = GP(0x816CBC08u + 0xa18u); if (_q) std::memcpy(&_h18,_q,4);
      _q = GP(0x816CBC08u + 0xa1cu); if (_q) std::memcpy(&_h1c,_q,4);
      _q = GP(0x84676750u + 0x94u); if (_q) std::memcpy(&_f1p,_q,4);
      _q = GP(0x84676750u + 0x98u); if (_q) std::memcpy(&_f1n,_q,4);
      _q = GP(0x846867F0u + 0x94u); if (_q) std::memcpy(&_f2p,_q,4);
      _q = GP(0x846867F0u + 0x98u); if (_q) std::memcpy(&_f2n,_q,4);
      printf("[FLIST2] h18=%08X h1c=%08X f1p=%08X f1n=%08X f2p=%08X f2n=%08X r3=%08X cur=%08X (n=%d)\n", _h18,_h1c,_f1p,_f1n,_f2p,_f2n, s.r[3], g_cur_fiber, g_fiberdbg2_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8120682Au) {
      uint32_t _f2n = 0, _f1p = 0;
      uint8_t* _q1 = GP(0x846867F0u + 0x98u); if (_q1) std::memcpy(&_f2n, _q1, 4);
      uint8_t* _q2 = GP(0x84676750u + 0x94u); if (_q2) std::memcpy(&_f1p, _q2, 4);
      if (_f2n == 0u && _f1p == 0x846867F0u) {
        uint8_t* _qw = GP(0x846867F0u + 0x98u); if (_qw) { uint32_t _fv = 0x84676750u; std::memcpy(_qw, &_fv, 4); }
        if (g_flistfix_n++ < 10) { printf("[FLISTFIX] fiber2.next 0 -> fiber1 (sp=%08X cur=%08X n=%d)\n", s.r[13], g_cur_fiber, g_flistfix_n); fflush(stdout); }
      }
    }
    if ((pc & ~1u) == 0x8115F448u && g_f448_n++ < 200) {
      printf("[F448] cb=%08X a1=%08X a2=%08X a3=%08X size=%08X sp=%08X cur=%08X (n=%d)\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[3]+0x14u, s.r[13], g_cur_fiber, g_f448_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115F458u && g_f458_n++ < 40) {
      printf("[F458] malloc_ret=%08X r4=%08X r5=%08X r2=%08X sp=%08X (n=%d)\n", s.r[0], s.r[4], s.r[5], s.r[2], s.r[13], g_f458_n);
      fflush(stdout);
    }
    if (g_hacks && (pc & ~1u) == 0x8115F9D0u) { s.r[0] = 0u; }  // [ABORTSKIP] force checker result 0 so the deliberate null-write path is skipped
    if ((pc & ~1u) == 0x811AC934u && g_ac934_n++ < 20) {
      printf("[AC934] r0=%08X lr=%08X r1=%08X sp=%08X cur=%08X (n=%d)\n", s.r[0], s.r[14], s.r[1], s.r[13], g_cur_fiber, g_ac934_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x811AC9CCu && g_ac9cc_n++ < 20) {
      printf("[AC9CC] r8=%08X prev_blk=%08X lr=%08X r0=%08X sp=%08X cur=%08X (n=%d)\n", s.r[8], g_cur_block, s.r[14], s.r[0], s.r[13], g_cur_fiber, g_ac9cc_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x81207344u && g_lw_n++ < 400) {
      printf("[LW] r0=%08X prev=%08X lr=%08X r1=%08X r2=%08X sp=%08X cur=%08X (n=%d)\n", s.r[0], g_cur_block, s.r[14], s.r[1], s.r[2], s.r[13], g_cur_fiber, g_lw_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x811731D8u && g_731d8_n++ < 400) {
      printf("[731D8] r0=%08X lr=%08X prev=%08X r1=%08X sp=%08X cur=%08X (n=%d)\n", s.r[0], s.r[14], g_cur_block, s.r[1], s.r[13], g_cur_fiber, g_731d8_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115F7C2u && g_7c2_n++ < 120) {
      uint32_t _obj = s.r[0], _f4 = 0; uint8_t* _q = GP(_obj + 4u); if (_q) std::memcpy(&_f4, _q, 4);
      printf("[7C2] obj=%08X [obj+4]=%08X sp=%08X cur=%08X (n=%d)\n", _obj, _f4, s.r[13], g_cur_fiber, g_7c2_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115D8E0u && g_d8e0_n++ < 200) {
      printf("[D8E0] r0=%08X lr=%08X prev=%08X r1=%08X sp=%08X cur=%08X (n=%d)\n", s.r[0], s.r[14], g_cur_block, s.r[1], s.r[13], g_cur_fiber, g_d8e0_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115D3A6u && g_d3a6_n++ < 12) {
      uint32_t _r8 = s.r[8], _v = 0; uint8_t* _q = GP(_r8 + 4u); if (_q) std::memcpy(&_v, _q, 4);
      printf("[D3A6] r8=%08X [r8+4]=%08X r9=%08X r0_in=%08X (n=%d)\n", _r8, _v, s.r[9], s.r[0], g_d3a6_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115D3B2u && g_d3b2_n++ < 12) {
      printf("[D3B2] r0_out=%08X r1=%08X r7=%08X sb=%08X (n=%d)\n", s.r[0], s.r[1], s.r[7], s.r[9], g_d3b2_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x81016818u && g_g18_n++ < 200) {
      printf("[G18] r0=%08X lr=%08X prev=%08X sp=%08X cur=%08X (n=%d)\n", s.r[0], s.r[14], g_cur_block, s.r[13], g_cur_fiber, g_g18_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x810164F4u && g_164f4_n++ < 60) {
      uint32_t _mgr = s.r[0], _f8 = 0, _fc = 0;
      uint8_t* _q8 = GP(_mgr + 8u); if (_q8) std::memcpy(&_f8, _q8, 4);
      uint8_t* _qc = GP(_mgr + 0xCu); if (_qc) std::memcpy(&_fc, _qc, 4);
      printf("[164F4] mgr=%08X id=%08X [mgr+8]=%08X [mgr+0xC]=%08X sp=%08X (n=%d)\n", _mgr, s.r[1], _f8, _fc, s.r[13], g_164f4_n);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x81184776u && g_84776_n++ < 5) { printf("[84776] ENTER init-sequence n=%d\n", g_84776_n); fflush(stdout); }
    if ((pc & ~1u) == 0x8101949Cu && g_1949c_n++ < 5) { printf("[1949C] ENTER pre-reg-init n=%d\n", g_1949c_n); fflush(stdout); }
    if ((pc & ~1u) == 0x8116E6A8u && g_e6a8_n++ < 30) {
      uint32_t _mgr = s.r[13]; uint32_t _m8 = 0;
      uint8_t* _q = GP(_mgr + 8u); if (_q) std::memcpy(&_m8, _q, 4);
      printf("[E6A8] n=%d r8=%08X r7=%08X r6=%08X r5=%08X sp=%08X [sp]=%08X [mgr?+8]=%08X\n", g_e6a8_n, s.r[8], s.r[7], s.r[6], s.r[5], s.r[13], s.r[0], _m8);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115D1D0u && g_d1d0_n++ < 300) {
      uint32_t _c = s.r[0], _b = 0, _cap = 0, _cnt = 0;
      uint8_t* _q = GP(_c); if (_q) { std::memcpy(&_b,_q,4); std::memcpy(&_cap,_q+4,4); std::memcpy(&_cnt,_q+8,4); }
      printf("[D1D0] n=%d container=%08X base=%08X cap=%08X count=%08X lr=%08X\n", g_d1d0_n, _c, _b, _cap, _cnt, s.r[14]);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8116E522u && g_e522_n++ < 40) {
      printf("[E522] n=%d container=%08X node=%08X r2=%08X lr=%08X\n", g_e522_n, s.r[0], s.r[1], s.r[2], s.r[14]);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x810164F4u && g_poolst_n++ < 40) {
      uint32_t _v1=0,_v2=0,_v3=0,_w1=0,_w2=0,_w3=0,_x1=0,_x2=0,_x3=0;
      { uint8_t* q; if((q=GP(0xA0039000u+0x1Cu))) std::memcpy(&_v1,q,4); if((q=GP(0xA0039000u+0x20u))) std::memcpy(&_v2,q,4); if((q=GP(0xA0039000u+0x2Cu))) std::memcpy(&_v3,q,4);
        if((q=GP(0xA005A000u+0x1Cu))) std::memcpy(&_w1,q,4); if((q=GP(0xA005A000u+0x20u))) std::memcpy(&_w2,q,4); if((q=GP(0xA005A000u+0x2Cu))) std::memcpy(&_w3,q,4);
        if((q=GP(0xA0115000u+0x1Cu))) std::memcpy(&_x1,q,4); if((q=GP(0xA0115000u+0x20u))) std::memcpy(&_x2,q,4); if((q=GP(0xA0115000u+0x2Cu))) std::memcpy(&_x3,q,4); }
      printf("[POOLST] n=%d p1(c=%X p=%X q=%X) p2(c=%X p=%X q=%X) p3(c=%X p=%X q=%X)\n", g_poolst_n, _v1,_v2,_v3, _w1,_w2,_w3, _x1,_x2,_x3);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115F7C2u && g_f7c2_n++ < 300) {
      uint32_t _o = s.r[0], _f4 = 0; uint8_t* _q = GP(_o + 4u); if (_q) std::memcpy(&_f4, _q, 4);
      printf("[F7C2] n=%d obj=%08X [obj+4]=%08X lr=%08X\n", g_f7c2_n, _o, _f4, s.r[14]); fflush(stdout);
    }
    if ((pc & ~1u) == 0x8115F7CEu && g_f7ce_n++ < 300) {
      uint32_t _o = s.r[0], _f4 = 0; uint8_t* _q = GP(_o + 4u); if (_q) std::memcpy(&_f4, _q, 4);
      printf("[F7CE] n=%d obj=%08X [obj+4]=%08X lr=%08X\n", g_f7ce_n, _o, _f4, s.r[14]); fflush(stdout);
    }
    if ((pc & ~1u) == 0x8116E690u || (pc & ~1u) == 0x8116E698u || (pc & ~1u) == 0x8116E6A8u || (pc & ~1u) == 0x8116E6D6u) {
      if (g_m8tr_n++ < 30) {
        uint32_t _mgr = 0, _m8 = 0; { uint8_t* q = GP(0x813AFC0Cu); if (q) std::memcpy(&_mgr, q, 4); }
        if (_mgr) { uint8_t* q2 = GP(_mgr + 8u); if (q2) std::memcpy(&_m8, q2, 4); }
        printf("[M8TR] n=%d pc=%08X mgr=%08X [mgr+8]=%08X\n", g_m8tr_n, (pc & ~1u), _mgr, _m8); fflush(stdout);
      }
    }
    if ((pc & ~1u) == 0x8116E6CAu && g_layout_n++ < 10) {
      uint32_t _sb = s.r[9], _v0 = 0, _v8 = 0, _vC = 0, _glob = 0;
      { uint8_t* q; if ((q=GP(0x813AFC0Cu))) std::memcpy(&_glob,q,4); if ((q=GP(_sb))) std::memcpy(&_v0,q,4); if ((q=GP(_sb+8u))) std::memcpy(&_v8,q,4); if ((q=GP(_sb+0xCu))) std::memcpy(&_vC,q,4); }
      { uint32_t _obj8=0, _objC=0; uint8_t* q3; if ((q3=GP(_v0+8u))) std::memcpy(&_obj8,q3,4); if ((q3=GP(_v0+0xCu))) std::memcpy(&_objC,q3,4);
      printf("[LAYOUT3] arr=%08X obj=%08X glob=%08X [obj+8]=%08X [obj+0xC]=%08X [arr+8]=%08X [arr+0xC]=%08X\n", _sb, _v0, _glob, _obj8, _objC, _v8, _vC); }
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x8116E688u && g_store_n++ < 10) {
      uint32_t _obj = s.r[6], _tgt = s.r[0], _old = 0;
      { uint8_t* q = GP(_tgt); if (q) std::memcpy(&_old, q, 4); }
      printf("[STORE] obj=%08X target=%08X [targetbefore]=%08X\n", _obj, _tgt, _old);
      fflush(stdout);
    }
    if ((pc & ~1u) == 0x81016818u && g_acc_n++ < 40) {
      printf("[ACC] n=%d r0=%08X lr=%08X blk=%08X\n", g_acc_n, s.r[0], s.r[14], g_cur_block); fflush(stdout);
    }
    if ((pc & ~1u) == 0x81016404u && g_lkin_n++ < 40) {
      uint32_t _cb = 0; { uint8_t* q = GP(s.r[0] + 0xCu); if (q) std::memcpy(&_cb, q, 4); }
      printf("[LKIN] n=%d mgr=%08X id=%08X cb=%08X lr=%08X\n", g_lkin_n, s.r[0], s.r[1], _cb, s.r[14]); fflush(stdout);
    }
    if ((pc & ~1u) == 0x810164ACu && g_lkout_n++ < 40) {
      uint32_t _o10 = 0, _o14 = 0;
      { uint8_t* q = GP(s.r[13] + 0x10u); if (q) std::memcpy(&_o10, q, 4); q = GP(s.r[13] + 0x14u); if (q) std::memcpy(&_o14, q, 4); }
      printf("[LKOUT] n=%d ret=%08X out10=%08X out14=%08X lr=%08X\n", g_lkout_n, s.r[5], _o10, _o14, s.r[14]); fflush(stdout);
    }
    if ((pc & ~1u) == 0x8101651Eu) { if (g_lktr_n++ < 80) { printf("[LKTR] AFTER_INNER r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s.r[0], s.r[5], s.r[7], s.r[13], s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x81016522u) { if (g_lktr_n++ < 80) { printf("[LKTR] RET0 r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s.r[7], s.r[5], s.r[7], s.r[13], s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x81016526u) { if (g_lktr_n++ < 80) { printf("[LKTR] INSERT r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s.r[4], s.r[5], s.r[7], s.r[13], s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8101653Eu) { if (g_lktr_n++ < 80) { printf("[LKTR] RETOK r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s.r[4], s.r[5], s.r[7], s.r[13], s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x81016450u) { if (g_lktr_n++ < 80) { printf("[LKTR] INNER_RET0 r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s.r[7], s.r[5], s.r[7], s.r[13], s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x810164ACu) { if (g_lktr_n++ < 80) { printf("[LKTR] INNER_RETNODE r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s.r[5], s.r[5], s.r[7], s.r[13], s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8101647Eu) { if (g_lktr_n++ < 80) { printf("[LKTR] ALLOC_DONE r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s.r[0], s.r[5], s.r[7], s.r[13], s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x81016456u) { if (g_lktr_n++ < 80) { printf("[LKTR] ALLOC_START r=%08X r5=%08X r7=%08X sp=%08X lr=%08X\n", s.r[4], s.r[5], s.r[7], s.r[13], s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8116F3C6u) { if (g_cbtr_n++ < 60) { printf("[CBTR] CB_ENTRY r0=%08X r1=%08X r2=%08X r3=%08X r5=%08X lr=%08X sp=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[5], s.r[14], s.r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8116F3DCu) { if (g_cbtr_n++ < 60) { printf("[CBTR] CB_AFTER_ACC r0=%08X r1=%08X r2=%08X r3=%08X r5=%08X lr=%08X sp=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[5], s.r[14], s.r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8116F3E2u) { if (g_cbtr_n++ < 60) { printf("[CBTR] CB_RET0PATH r0=%08X r1=%08X r2=%08X r3=%08X r5=%08X lr=%08X sp=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[5], s.r[14], s.r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8101643Cu) { if (g_cbtr_n++ < 60) { printf("[CBTR] AFTER_CB r0=%08X r1=%08X r2=%08X r3=%08X r5=%08X lr=%08X sp=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[5], s.r[14], s.r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x81016440u) { if (g_cbtr_n++ < 60) { printf("[CBTR] CB_NONZERO r0=%08X r1=%08X r2=%08X r3=%08X r5=%08X lr=%08X sp=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[5], s.r[14], s.r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x81016444u) { if (g_cbtr_n++ < 60) { printf("[CBTR] SET_MINUS1 r0=%08X r1=%08X r2=%08X r3=%08X r5=%08X lr=%08X sp=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[5], s.r[14], s.r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8125AC60u) {
      if (g_tblw_n++ < 200) {
        uint32_t _idx = s.r[0], _val = s.r[1], _tbl = 0;
        { uint8_t* q = GP(0x8153F9D4u + _idx * 4u); if (q) std::memcpy(&_tbl, q, 4); }
        printf("[TBLW] n=%d idx=%08X val=%08X old=%08X lr=%08X blk=%08X\n", g_tblw_n, _idx, _val, _tbl, s.r[14], g_cur_block); fflush(stdout);
      }
    }
    if ((pc & ~1u) == 0x8125AC84u) {
      if (g_dsp_n++ < 120) {
        uint32_t _idx = s.r[0], _arg = s.r[1], _ent = 0;
        { uint8_t* q = GP(0x8153F9D4u + _idx * 4u); if (q) std::memcpy(&_ent, q, 4); }
        printf("[DSP] n=%d idx=%08X arg=%08X entry=%08X lr=%08X blk=%08X\n", g_dsp_n, _idx, _arg, _ent, s.r[14], g_cur_block); fflush(stdout);
      }
    }
    if ((pc & ~1u) == 0x8116F716u) { if (g_nset_n++ < 200000) { printf("[NSET] SET716 r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X sp=%08X blk=%08X\n", s.r[0], s.r[1], s.r[2], s.r[4], s.r[14], s.r[13], g_cur_block); fflush(stdout); } }
    if ((pc & ~1u) == 0x8115DEACu) { if (g_nset_n++ < 200000) { printf("[NSET] DEAC r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X sp=%08X blk=%08X\n", s.r[0], s.r[1], s.r[2], s.r[4], s.r[14], s.r[13], g_cur_block); fflush(stdout); } }
    if ((pc & ~1u) == 0x8115DCB2u) { if (g_nset_n++ < 200000) { printf("[NSET] DCB2 r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X sp=%08X blk=%08X\n", s.r[0], s.r[1], s.r[2], s.r[4], s.r[14], s.r[13], g_cur_block); fflush(stdout); } }
    if ((pc & ~1u) == 0x8115D3C8u) { if (g_nset_n++ < 200000) { printf("[NSET] D3C8 r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X sp=%08X blk=%08X\n", s.r[0], s.r[1], s.r[2], s.r[4], s.r[14], s.r[13], g_cur_block); fflush(stdout); } }
    if ((pc & ~1u) == 0x8115F8ACu) { if (g_nset_n++ < 200000) { printf("[NSET] F8AC r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X sp=%08X blk=%08X\n", s.r[0], s.r[1], s.r[2], s.r[4], s.r[14], s.r[13], g_cur_block); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EAB44u) {
      if (g_gxmp_n++ < 20) { printf("[GXMP] AB44 sb(r9)=%08X r7=%08X lr=%08X sp=%08X r0=%08X\n", s.r[9], s.r[7], s.r[14], s.r[13], s.r[0]); fflush(stdout); }
    }
    if ((pc & ~1u) == 0x812B1380u || (pc & ~1u) == 0x812B1660u || (pc & ~1u) == 0x812B1250u) {
      if (g_gxmp_n++ < 60) { printf("[GXMP] HLE %08X r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", (pc & ~1u), s.r[0], s.r[1], s.r[2], s.r[3], s.r[14], s.r[13]); fflush(stdout); }
    }
    if ((pc & ~1u) == 0x811EB6BAu) { if (g_pool_n++ < 60) { printf("[POOL] POOL_ENTER r4=%08X r5=%08X r6=%08X r7=%08X r9=%08X lr=%08X\n", s.r[4], s.r[5], s.r[6], s.r[7], s.r[9], s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB702u) { if (g_pool_n++ < 60) { printf("[POOL] POOL_WRITE r4=%08X r5=%08X r6=%08X r7=%08X r9=%08X lr=%08X\n", s.r[4], s.r[5], s.r[6], s.r[7], s.r[9], s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB6ECu) { if (g_pool_n++ < 60) { printf("[POOL] POOL_ALLOC r4=%08X r5=%08X r6=%08X r7=%08X r9=%08X lr=%08X\n", s.r[4], s.r[5], s.r[6], s.r[7], s.r[9], s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB58Cu) { if (g_poolc_n++ < 80) { printf("[POOLC] F58C r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[14], s.r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811990A2u) { if (g_poolc_n++ < 80) { printf("[POOLC] CALLSITE r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[14], s.r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB5C0u) { if (g_poolc_n++ < 80) { printf("[POOLC] MID r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[14], s.r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB600u) { if (g_poolc_n++ < 80) { printf("[POOLC] MID2 r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[14], s.r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB650u) { if (g_poolc_n++ < 80) { printf("[POOLC] MID3 r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[14], s.r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EB6A0u) { if (g_poolc_n++ < 80) { printf("[POOLC] MID4 r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[14], s.r[13]); fflush(stdout); } }
    if ((pc & ~1u) == 0x812A398Eu) { if (g_gcpy2_n++ < 60) { printf("[GCPY3] CPY_ENTER r8=%08X r6=%08X r5=%08X r4=%08X r1=%08X lr=%08X\n", s.r[8],s.r[6],s.r[5],s.r[4],s.r[1],s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x812A3994u) { if (g_gcpy2_n++ < 60) { printf("[GCPY3] CPY_STORE r8=%08X r6=%08X r5=%08X r4=%08X r1=%08X lr=%08X\n", s.r[8],s.r[6],s.r[5],s.r[4],s.r[1],s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x812A397Cu) { if (g_gcpy2_n++ < 60) { printf("[GCPY3] CPY_PRE r8=%08X r6=%08X r5=%08X r4=%08X r1=%08X lr=%08X\n", s.r[8],s.r[6],s.r[5],s.r[4],s.r[1],s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x810510A0u) { if (g_gcpy2_n++ < 60) { printf("[GCPY3] CPY_510 r8=%08X r6=%08X r5=%08X r4=%08X r1=%08X lr=%08X\n", s.r[8],s.r[6],s.r[5],s.r[4],s.r[1],s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x8103F528u) { if (g_gcpy2_n++ < 60) { printf("[GCPY3] CPY_3F5 r8=%08X r6=%08X r5=%08X r4=%08X r1=%08X lr=%08X\n", s.r[8],s.r[6],s.r[5],s.r[4],s.r[1],s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x812A39A0u) { if (g_gcpy2_n++ < 60) { printf("[GCPY3] CPY_POST r8=%08X r6=%08X r5=%08X r4=%08X r1=%08X lr=%08X\n", s.r[8],s.r[6],s.r[5],s.r[4],s.r[1],s.r[14]); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EA552u) { if (g_ea552_n++ < 60) { printf("[EA552] ENTER r0=%08X r1=%08X r2=%08X r7=%08X r9=%08X lr=%08X fib=%08X thr=%08X\n", s.r[0], s.r[1], s.r[2], s.r[7], s.r[9], s.r[14], g_cur_fiber, g_cur_thr); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EA5FEu) { if (g_ea552_n++ < 60) { printf("[EA552] MAPLOOP r0=%08X r1=%08X r2=%08X r7=%08X r9=%08X lr=%08X fib=%08X thr=%08X\n", s.r[0], s.r[1], s.r[2], s.r[7], s.r[9], s.r[14], g_cur_fiber, g_cur_thr); fflush(stdout); } }
    if (pc==0x811DA024u) { RecState* _ps = SPTR(s); g_last_alloc_sz = _ps->r[0]; g_last_alloc_fp = _ps->r[4]; }
    else if ((pc==0x811DA028u||pc==0x811DA02Au||pc==0x811DA030u||pc==0x811DA036u) && SPTR(s)->r[0]==0u && g_allocf_n++ < 24) { RecState* _ps = SPTR(s); printf("[ALLOCFAIL] pc=%08X size=%08X fp=%08X r0=%08X r1=%08X r4=%08X r5=%08X r6=%08X r7=%08X sp=%08X lr=%08X fib=%08X thr=%08X\n", pc, g_last_alloc_sz, g_last_alloc_fp, _ps->r[0],_ps->r[1],_ps->r[4],_ps->r[5],_ps->r[6],_ps->r[7],_ps->r[13],_ps->r[14],g_cur_fiber,g_cur_thr); fflush(stdout); }
    if ((pc & ~1u) == 0x811EAAE4u) { if (g_ea552_n++ < 60) { printf("[EA552] MATLOOP r0=%08X r1=%08X r2=%08X r7=%08X r9=%08X lr=%08X fib=%08X thr=%08X\n", s.r[0], s.r[1], s.r[2], s.r[7], s.r[9], s.r[14], g_cur_fiber, g_cur_thr); fflush(stdout); } }
    if ((pc & ~1u) == 0x811EAB44u) { if (g_ea552_n++ < 60) { printf("[EA552] AB44 r0=%08X r1=%08X r2=%08X r7=%08X r9=%08X lr=%08X fib=%08X thr=%08X\n", s.r[0], s.r[1], s.r[2], s.r[7], s.r[9], s.r[14], g_cur_fiber, g_cur_thr); fflush(stdout); } }
        for (int _si=0;_si<16;++_si) g_snap_r[_si] = s.r[_si]; g_snap_blk = pc;
    if ((pc & ~1u) == 0x811EAB44u && g_val44_n++ < 20) { uint32_t _v=0; uint8_t* _q=GP(s.r[9]); if(_q) std::memcpy(&_v,_q,4); printf("[VAL44] field=%%08X r9=%%08X fib=%%08X thr=%%08X\n", _v, s.r[9], g_cur_fiber, g_cur_thr); fflush(stdout); }
      { uint32_t _v=0; uint8_t* _q=GP(0x816CBF0Cu); if(_q) std::memcpy(&_v,_q,4); if (_v!=g_field_last && g_field_n++<80) { printf("[FW] blk=%08X prevblk=%08X v=%08X prev=%08X r7=%08X r9=%08X lr=%08X\n", pc, g_field_blk, _v, g_field_last, s.r[7], s.r[9], s.r[14]); fflush(stdout); } g_field_last=_v; g_field_blk=pc; }
      if ((pc & ~1u) == 0x811C0D74u && g_alloc_n++ < 40) { printf("[ALLOC] enter r0=%08X r1=%08X lr=%08X blk=%08X\n", s.r[0], s.r[1], s.r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x811C0DA8u && g_alloc2_n++ < 20) { printf("[ALLOC] crashblk r0=%08X r1=%08X lr=%08X blk=%08X\n", s.r[0], s.r[1], s.r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x811C1804u && g_alloc2_n++ < 20) { printf("[C1804] r0=%08X r5=%08X r6=%08X lr=%08X blk=%08X\n", s.r[0], s.r[5], s.r[6], s.r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x8124930Eu && g_wp_n++ < 30) { printf("[WP] r8=%08X r5=%08X fp=%08X r9=%08X lr=%08X r0=%08X blk=%08X fib=%08X thr=%08X\n", s.r[8], s.r[5], s.r[11], s.r[9], s.r[14], s.r[0], g_cur_block, g_cur_fiber, g_cur_thr); fflush(stdout); }
      if ((pc & ~1u) == 0x8124930Eu && g_wp_n++ < 3) { printf("[WPRING] blk=%08X lr=%08X r0=%08X r1=%08X r2=%08X r4=%08X r5=%08X r8=%08X ring:", g_cur_block, s.r[14], s.r[0], s.r[1], s.r[2], s.r[4], s.r[5], s.r[8]); for (int _k=0;_k<20;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 20u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout); }
      if (((pc & ~1u) == 0x81279AFEu || (pc & ~1u) == 0x81279B02u || (pc & ~1u) == 0x812B0DD0u) && g_wp_n++ < 40) { printf("[DIVP] pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X lr=%08X\n", pc, s.r[0], s.r[1], s.r[2], s.r[3], s.r[4], s.r[5], s.r[14]); fflush(stdout); }
      if ((pc & ~1u) == 0x811C0E14u && g_e14_n++ < 8) { printf("[E14P] r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r12=%08X lr=%08X blk=%08X\n", s.r[0], s.r[1], s.r[2], s.r[3], s.r[4], s.r[5], s.r[6], s.r[7], s.r[12], s.r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x811C0D74u && g_e14_n++ < 8) { printf("[E14P] FUNC_ENTRY r0=%08X r1=%08X lr=%08X blk=%08X\n", s.r[0], s.r[1], s.r[14], g_cur_block); fflush(stdout); }
      if ((pc & ~1u) == 0x81206700u && g_fp_n++ < 40) { printf("[FP] CALL_FACT r0=%08X r1=%08X r2=%08X lr=%08X\n", s.r[0], s.r[1], s.r[2], s.r[14]); fflush(stdout); }
      if ((pc & ~1u) == 0x81206760u && g_fp_n++ < 40) { printf("[FP] AFTER_HLE r0=%08X r4=%08X lr=%08X\n", s.r[0], s.r[4], s.r[14]); fflush(stdout); }
      if ((pc & ~1u) == 0x81198A80u && g_fp_n++ < 40) { printf("[FP] CALL_683C r5=%08X r4=%08X r0=%08X lr=%08X\n", s.r[5], s.r[4], s.r[0], s.r[14]); fflush(stdout); }
      if ((pc & ~1u) == 0x8120683Cu && g_fp_n++ < 40) { printf("[FP] ENTER_683C r0=%08X r1=%08X lr=%08X fib=%08X thr=%08X\n", s.r[0], s.r[1], s.r[14], g_cur_fiber, g_cur_thr); fflush(stdout); }
      if ((pc & ~1u) == 0x812067A0u && g_cf_n++ < 60) { printf("[CF] ENTER r0=%08X r1=%08X sp=%08X lr=%08X fib=%08X\n", s.r[0], s.r[1], s.r[13], s.r[14], g_cur_fiber); fflush(stdout); }
      if ((pc & ~1u) == 0x812067B6u && g_cf_n++ < 60) { uint32_t _cv=0; uint8_t* _cq=GP(s.r[13]+4u); if(_cq) std::memcpy(&_cv,_cq,4); printf("[CF] BODY sp=%08X [sp+4]=%08X r0=%08X r3=%08X fib=%08X\n", s.r[13], _cv, s.r[0], s.r[3], g_cur_fiber); fflush(stdout); }
      if ((pc & ~1u) == 0x8120682Eu && g_cf_n++ < 60) { printf("[CF] CMP r0=%08X r1=%08X r5=%08X sp=%08X fib=%08X\n", s.r[0], s.r[1], s.r[5], s.r[13], g_cur_fiber); fflush(stdout); }
      if ((pc & ~1u) == 0x8120682Au && g_cf_n++ < 80) { uint32_t _cv=0; uint8_t* _cq=GP(s.r[13]+4u); if(_cq) std::memcpy(&_cv,_cq,4); printf("[CF2] CMPBLK r0=%08X r1=%08X [sp+4]=%08X sp=%08X fib=%08X\n", s.r[0], s.r[1], _cv, s.r[13], g_cur_fiber); fflush(stdout); }
      if (((pc & ~1u) == 0x811E5928u || (pc & ~1u) == 0x811E5CDAu) && g_ctx_n++ < 60) { printf("[CTX] blk=%08X sp=%08X lr=%08X fib=%08X fex=%d runs=%d sw=%d ring:", g_cur_block, s.r[13], s.r[14], g_cur_fiber, g_in_fex, g_fiber_runs, g_fiber_switches); for (int _k=0;_k<12;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 12u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout); }
    if (g_hacks && (pc & ~1u) == 0x8115F9D0u) { s.r[0] = 0u; }  // [ABORTSKIP2] force checker 0 at the real block entry
      if (((pc & ~1u) == 0x812068D2u || (pc & ~1u) == 0x812068D8u || (pc & ~1u) == 0x812068E4u) && g_spoll_n++ < 60) {
        uint32_t _f80=0,_f94=0,_g18=0,_g1c=0; uint8_t* q;
        q=GP(s.r[5]+0x80u); if(q) std::memcpy(&_f80,q,4);
        q=GP(s.r[5]+0x94u); if(q) std::memcpy(&_f94,q,4);
        q=GP(0x816CC620u); if(q) std::memcpy(&_g18,q,4);
        q=GP(0x816CC624u); if(q) std::memcpy(&_g1c,q,4);
        printf("[SPOLL] pc=%08X r5=%08X r0=%08X lr=%08X f80=%08X f94=%08X g18=%08X g1c=%08X fib=%08X fex=%d fl=%zu\n", g_cur_block, s.r[5], s.r[0], s.r[14], _f80, _f94, _g18, _g1c, g_cur_fiber, g_in_fex, g_fiber_list.size()); fflush(stdout);
      }
    if ((pc & ~1u) == 0x812068D8u && g_schedrun_n < 4 && g_in_fex == 0 && !g_in_fiber_exec) {
      ++g_schedrun_n;
      uint32_t fb = (g_fiber_list.size() > 1 ? g_fiber_list[1] : (g_fiber_list.empty() ? 0u : g_fiber_list[0]));
      if (fb) {
        uint32_t off = fb - IMG_VA_BASE, fentry = 0, farg = 0;
        if (off + 0x90u <= IMG_SIZE) { std::memcpy(&fentry, gp_img+off+0x84u, 4); std::memcpy(&farg, gp_img+off+0x88u, 4); }
        printf("[SCHEDRUN] #%d run fiber=%08X entry=%08X arg=%08X\n", g_schedrun_n, fb, fentry, farg); fflush(stdout);
        {   // [SCHEDRUN3] force-reset the fiber context: never resume at a mid-function PC with stale registers
          FCtx cc; std::memset(&cc, 0, sizeof(cc));
          cc.r[0] = fb; cc.r[13] = 0xA0D80000u; cc.r[14] = 0xFFFFFFF0u; cc.r[15] = (fentry ? fentry : 0x81198C0Bu) & ~1u;
          g_fctx[fb] = cc;
          printf("[SCHEDRUN3] ctx reset fiber=%08X spy=%08X entry=%08X\n", fb, cc.r[13], cc.r[15]); fflush(stdout);
        }
        fiber_load(&s, fb, fentry, farg);
        uint32_t fr = fiber_exec_to_completion(&s);
        fiber_restore_thread(&s);
        { uint8_t* q = GP(fb+0x80u); if (q) { uint32_t v=0; std::memcpy(&v,q,4); v |= 2u; std::memcpy(q,&v,4); } }  // [SCHEDRUN2] publish fiber completion bit
        printf("[SCHEDRUN] fiber=%08X ret=%08X done bit2-set\n", fb, fr);
        { int _cnt = g_hle_ring_n < 40 ? g_hle_ring_n : 40;
          printf("[FIBHLE] fiber=%08X last HLE calls (%d):\n", fb, _cnt);
          for (int _i = _cnt-1; _i >= 0; --_i) { int _idx = (g_hle_ring_n - _cnt + _i) & 127; printf("[FIBHLE]   va=%08X lr=%08X\n", g_hle_ring_va[_idx], g_hle_ring_lr[_idx]); }
          fflush(stdout); }
      }
    }
      if (((pc & ~1u) == 0x81198BEAu || (pc & ~1u) == 0x81198BF6u || (pc & ~1u) == 0x812067A0u || (pc & ~1u) == 0x81206832u || (pc & ~1u) == 0x81206700u) && g_lp2_n++ < 80) {
        printf("[LP2] blk=%08X r0=%08X r1=%08X r4=%08X r5=%08X sp=%08X lr=%08X fib=%08X\n", g_cur_block, s.r[0], s.r[1], s.r[4], s.r[5], s.r[13], s.r[14], g_cur_fiber); fflush(stdout);
      }
    if ((pc & ~1u) == 0x81198BF6u) {
      uint32_t _lc=0; uint8_t* _lq=GP(0x816CC630u); if(_lq) std::memcpy(&_lc,_lq,4);
      if ((_lc & 2u) && g_loadspin_n++ > (g_loadclear_env ? 200u : 100000000u)) { _lc &= ~2u; if(_lq) std::memcpy(_lq,&_lc,4); printf("[LOADCLEAR] cleared bit1 at 0x816CC630 (iters=%d) -> %08X fib=%08X\n", g_loadspin_n, _lc, g_cur_fiber); fflush(stdout); }
    }
    if ((pc & ~1u) == 0u && g_pc0_n++ < 20) { printf("[PC0PROBE] pc=0 block=%08X fex=%d fexec=%d fib=%08X thrs=%zu\n", g_cur_block, g_in_fex, g_in_fiber_exec, g_cur_fiber, g_thr.size()); fflush(stdout); }
    if (((pc & ~1u) == 0u || pc == 0xFFFFFFF0u) && g_postmain_n < 3 && !g_in_fiber_exec && g_in_fex == 0) {
      ++g_postmain_n;
      printf("[POSTMAIN] main thread returned pc=%08X; running %zu fibers\n", pc, g_fiber_list.size()); fflush(stdout);
      for (size_t fi = 0; fi < g_fiber_list.size(); ++fi) {
        uint32_t fb = g_fiber_list[fi];
        uint32_t off = fb - IMG_VA_BASE, fentry = 0, farg = 0;
        if (off + 0x90u <= IMG_SIZE) { std::memcpy(&fentry, gp_img+off+0x84u, 4); std::memcpy(&farg, gp_img+off+0x88u, 4); }
        { FCtx cc; std::memset(&cc, 0, sizeof(cc)); cc.r[0] = fb; cc.r[13] = 0xA0E00000u - (uint32_t)fi*0x800000u; cc.r[14] = 0xFFFFFFF0u; cc.r[15] = (fentry?fentry:0x81198BE9u) & ~1u; g_fctx[fb] = cc; }
        printf("[POSTMAIN] run fiber[%zu]=%08X entry=%08X arg=%08X\n", fi, fb, fentry, farg); fflush(stdout);
        fiber_load(&s, fb, fentry, farg);
        uint32_t fr = fiber_exec_to_completion(&s);
        fiber_restore_thread(&s);
        printf("[POSTMAIN] fiber=%08X ret=%08X done\n", fb, fr); fflush(stdout);
      }
    }
      if ((pc & ~1u) == 0x8127C1D2u && g_c1d2_n++ < 12) {
        printf("[C1D2] enter r0=%08X r1=%08X r2=%08X lr=%08X sp=%08X fib=%08X thr=%08X ring:", s.r[0], s.r[1], s.r[2], s.r[14], s.r[13], g_cur_fiber, g_cur_thr);
        for (int _k=0;_k<20;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 20u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
      }
      if (((pc & ~1u) == 0x8127C1CAu || (pc & ~1u) == 0x8127C1C0u || (pc & ~1u) == 0x8127C1C6u) && g_c1d2_n++ < 30) {
        printf("[C1D2] pre pc=%08X r0=%08X r1=%08X lr=%08X sp=%08X\n", g_cur_block, s.r[0], s.r[1], s.r[14], s.r[13]); fflush(stdout);
      }
if (g_in_fiber_exec && g_cur_fiber && g_fiber_list.size() > 1 && g_cur_fiber == g_fiber_list[1] && ((pc & ~1u) == 0x81207956u || (pc & ~1u) == 0x8120794Au || (pc & ~1u) == 0x81207B3Au || (pc & ~1u) == 0x81207B54u) && g_f2blk_n++ < 40) {
  printf("[F2BLK] pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X\n", pc, s.r[0], s.r[1], s.r[2], s.r[3], s.r[14], s.r[13]); fflush(stdout);
}
if ((pc & ~1u) == 0x81182C9Au && g_nx9a_n++ < 10) {
  printf("[NX9A] r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X sp=%08X lr=%08X fib=%08X ring:", s.r[0], s.r[1], s.r[2], s.r[3], s.r[4], s.r[5], s.r[6], s.r[7], s.r[8], s.r[13], s.r[14], g_cur_fiber);
  for (int _k=0;_k<20;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 20u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
}
{ static int _p[4] = {0,0,0,0};
  if ((pc & ~1u) == 0x81198A76u && _p[0]++ < 4) printf("[FLPROBE] init-set-flag block reached r2=%08X lr=%08X fib=%08X\n", s.r[2], s.r[14], g_cur_fiber);
  if ((pc & ~1u) == 0x81198C8Eu && _p[1]++ < 6) printf("[FLPROBE] frame-loop-top reached flags[=816CC630]=%08X fib=%08X sp=%08X\n", (uint32_t)(GP(0x816CC630u) ? *(uint32_t*)GP(0x816CC630u) : 0xDEADu), g_cur_fiber, s.r[13]);
  if ((pc & ~1u) == 0x81198CE4u && _p[2]++ < 6) printf("[FLPROBE] frame-counter-inc reached r5=%08X fib=%08X\n", s.r[5], g_cur_fiber);
  if ((pc & ~1u) == 0x81198D04u && _p[3]++ < 6) printf("[FLPROBE] frame-loop-test reached r0=%08X fib=%08X\n", s.r[0], g_cur_fiber);
  fflush(stdout); }
if ((pc & ~1u) == 0x811830C8u && g_w8c8_n++ < 6) {
  printf("[W8C8] enter r0=%08X r1=%08X lr=%08X sp=%08X fib=%08X ring:", s.r[0], s.r[1], s.r[14], s.r[13], g_cur_fiber);
  for (int _k=0;_k<24;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 24u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
}
if ((pc & ~1u) == 0x811830C8u && g_w8c8_watch == 0 && s.r[0] >= 0x80000000u) {
  g_w8c8_watch = s.r[0] + 4u; g_watch_addr = g_w8c8_watch; g_watch_val = 0xDEADBEEFu;
  uint8_t* _wq = GP(g_watch_addr); if (_wq) std::memcpy(&g_watch_val, _wq, 4);
  printf("[DWATCH] object=%08X state=%08X initial=%08X\n", s.r[0], g_watch_addr, g_watch_val); fflush(stdout);
}
if ((pc & ~1u) == 0x811830CCu && s.r[4] >= 0x80000000u) {   // uncond3-main
  uint8_t* _f3m = GP(s.r[4] + 4u);
  if (_f3m) { uint16_t _cm = 0; std::memcpy(&_cm, _f3m, 2);
    if (_cm != 3) { uint16_t _tm = 3; std::memcpy(_f3m, &_tm, 2); if (g_uncond3_n++ < 8) { printf("[UNCOND3] MAIN object=%08X state %u -> 3\n", s.r[4], (unsigned)_cm); fflush(stdout); } } }
}
if ((pc & ~1u) == 0x811830CCu && g_loop8cc_n++ < 12) {
  printf("[LOOP8CC] MAIN r4=%08X r0=%08X sp=%08X lr=%08X fib=%08X\n", s.r[4], s.r[0], s.r[13], s.r[14], g_cur_fiber); fflush(stdout);
  if (g_w8c8_watch == 0 && s.r[4] >= 0x80000000u) {
    g_w8c8_watch = s.r[4] + 4u; g_watch_addr = g_w8c8_watch; g_watch_val = 0xDEADBEEFu;
    uint8_t* _wq = GP(g_watch_addr); if (_wq) std::memcpy(&g_watch_val, _wq, 4);
    printf("[DWATCH] object=%08X state=%08X initial=%08X (from main loop)\n", s.r[4], g_watch_addr, g_watch_val); fflush(stdout);
  }
}
if ((pc & ~1u) == 0x811E5C98u && g_cb98_n++ < 8) {
  uint32_t _cb[6] = {0,0,0,0,0,0}; const uint32_t _addrs[6] = {0x81513DD4u,0x81513DD8u,0x81513DDCu,0x81513DE0u,0x81513DE4u,0x81513DE8u};
  for (int _i=0;_i<6;++_i) { uint8_t* _q = GP(_addrs[_i]); if (_q) std::memcpy(&_cb[_i], _q, 4); }
  printf("[CB98] cb=%08X/%08X/%08X/%08X/%08X/%08X r0=%08X r1=%08X lr=%08X fib=%08X\n", _cb[0],_cb[1],_cb[2],_cb[3],_cb[4],_cb[5], s.r[0], s.r[1], s.r[14], g_cur_fiber); fflush(stdout);
}
if ((pc & ~1u) == 0x811DD4BCu && g_xswap_n++ < 20) {
  printf("[XSWAP] r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X lr=%08X sp=%08X fib=%08X mem:", s.r[0], s.r[1], s.r[2], s.r[3], s.r[4], s.r[5], s.r[14], s.r[13], g_cur_fiber);
  uint32_t _w0=0,_w1=0,_w2=0; uint8_t* _q;
  _q = GP((s.r[4]) + 0x0u); if (_q) std::memcpy(&_w0,_q,4);
  _q = GP((s.r[4]) + 0x18u); if (_q) std::memcpy(&_w1,_q,4);
  _q = GP((s.r[4]) + 0x1cu); if (_q) std::memcpy(&_w2,_q,4);
  printf(" hdr=%08X/%08X/%08X\n", _w0, _w1, _w2); fflush(stdout);
  if (g_hacks && s.r[2] == 0xFFFFFFFFu) { s.r[2] = 0u; printf("[XSKIP3] length -1 -> 0\n"); fflush(stdout); }
}
if (((pc & ~1u) == 0x811DEAB4u || (pc & ~1u) == 0x811DEB44u || (pc & ~1u) == 0x811DEB34u) && g_obj24_n++ < 24) {
  printf("[OBJ24] pc=%08X r0=%08X r1=%08X r2=%08X r4=%08X lr=%08X fib=%08X", s.r[15], s.r[0], s.r[1], s.r[2], s.r[4], s.r[14], g_cur_fiber);
  uint32_t _o = s.r[4]; if (_o >= 0x80000000u) {
    uint32_t _w[10]; for (int _i=0;_i<10;++_i) { _w[_i]=0; uint8_t* _q=GP(_o + (uint32_t)_i*4u); if (_q) std::memcpy(&_w[_i],_q,4); }
    printf(" struct: %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X", _w[0],_w[1],_w[2],_w[3],_w[4],_w[5],_w[6],_w[7],_w[8],_w[9]);
  }
  printf("\n"); fflush(stdout);
}
if (((pc & ~1u) == 0x811C6C72u || (pc & ~1u) == 0x811C6C84u || (pc & ~1u) == 0x811C6C8Cu) && g_cb0_n++ < 18) {
  uint32_t _h = s.r[0]; if (_h < 0x80000000u) _h = s.r[4]; uint32_t _s = 0, _f8c = 0, _f90 = 0;
  if (_h >= 0x80000000u) { uint8_t* _q;
    _q = GP(_h + 4u); if (_q) std::memcpy(&_s, _q, 4);
    _q = GP(_h + 0x8cu); if (_q) std::memcpy(&_f8c, _q, 4);
    _q = GP(_h + 0x90u); if (_q) std::memcpy(&_f90, _q, 4); }
  printf("[CB0] pc=%08X r0=%08X handle=%08X state=%08X f8c=%08X f90=%08X r2=%08X lr=%08X fib=%08X\n", s.r[15], s.r[0], _h, _s, _f8c, _f90, s.r[2], s.r[14], g_cur_fiber); fflush(stdout);
}
if ((pc & ~1u) == 0x8121D044u) { ++g_d044_n;
  if (g_d044_n < 8) {
    uint32_t _w38=0,_w44=0,_w50=0; uint8_t* _q;
    _q = GP(s.r[13] + 0x38u); if (_q) std::memcpy(&_w38,_q,4);
    _q = GP(s.r[13] + 0x44u); if (_q) std::memcpy(&_w44,_q,4);
    _q = GP(s.r[13] + 0x50u); if (_q) std::memcpy(&_w50,_q,4);
    printf("[D044] #%u sp=%08X w38=%08X w44=%08X w50=%08X lr=%08X fib=%08X ring:", g_d044_n, s.r[13], _w38, _w44, _w50, s.r[14], g_cur_fiber);
    for (int _k=0;_k<16;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 16u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
  } }
if ((pc & ~1u) == 0x81207734u) { ++g_allocbig_total;
  if (s.r[0] > 0x2000000u && g_allocbig_n++ < 12) {
    printf("[ALLOCBIG] #%u r0(size)=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X fib=%08X ring:", g_allocbig_n, s.r[0], s.r[1], s.r[2], s.r[3], s.r[14], s.r[13], g_cur_fiber);
    for (int _k=0;_k<14;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 14u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
  } }
if ((pc & ~1u) == 0x81207290u && s.r[0] < 0x80000000u && g_atombad_n++ < 20) {
  printf("[ATOMBAD] #%u r0=%08X r1=%08X r2=%08X lr=%08X sp=%08X fib=%08X ring:", g_atombad_n, s.r[0], s.r[1], s.r[2], s.r[14], s.r[13], g_cur_fiber);
  for (int _k=0;_k<16;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 16u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
}
if (g_hacks && (pc & ~1u) == 0x811F019Eu && s.r[0] == 0u) { s.r[15] = s.r[14]; if (g_refnull_n++ < 8) { printf("[REFNULL] skip ref(null) lr=%08X fib=%08X\n", s.r[14], g_cur_fiber); fflush(stdout); } continue; }
if ((pc & ~1u) == 0x811E3520u && g_obj20_n++ < 12) {
  uint32_t _o = s.r[4]; uint32_t _v = 0; uint8_t* _q = GP(_o + 0x20u); if (_q) std::memcpy(&_v,_q,4);
  if (_o >= 0x80000000u) { g_obj20_addr = _o + 0x20u; }
  printf("[OBJ20] r4=%08X [r4+0x20]=%08X sp=%08X fib=%08X ring:", _o, _v, s.r[13], g_cur_fiber);
  for (int _k=0;_k<10;++_k) printf(" %08X", g_pc_ring[(g_pc_ring_n - 10u + (uint32_t)_k) & 63u]); printf("\n"); fflush(stdout);
}
if ((pc & ~1u) == 0x811C411Cu && g_efe9a_n++ < 6) {
  uint8_t* _q = GP(0x811EFE9Au);
  if (_q) printf("[EFE9A] mem@811EFE9A = %02X %02X %02X %02X %02X %02X %02X %02X  (elf expects 00 20 70 47)\n", _q[0],_q[1],_q[2],_q[3],_q[4],_q[5],_q[6],_q[7]);
  else printf("[EFE9A] GP null\n");
  fflush(stdout);
}
ef2_probe(pc, &s, 2);
if (g_hacks && (pc & ~1u) == 0x811E2B50u) {
  uint32_t _a = 0, _b = 0, _c = 0; uint8_t* _q = GP(s.r[1] + 0x40u); if (_q) std::memcpy(&_a,_q,4);
  if (_a >= 0x80000000u) { _q = GP(_a + 0x78u); if (_q) std::memcpy(&_b,_q,4); }
  if (_b >= 0x80000000u) { _q = GP(_b + 0x20u); if (_q) std::memcpy(&_c,_q,4); }
  if (_a >= 0x80000000u && _b >= 0x80000000u && _c == 0u) { s.r[15] = s.r[14];
    if (g_ret1_n++ < 8) { printf("[RET1] skip FUN_811e2b50: model+0x20 null (r1=%08X lr=%08X fib=%08X)\n", s.r[1], s.r[14], g_cur_fiber); fflush(stdout); } continue; }
}
if (family_skip(&s, pc)) continue;
if (nulldesc_skip(&s, pc)) continue;
if (assert_hle(&s, pc)) continue;
assert_msg_probe(&s, pc);
initwatch_probe(&s, pc);
g_loop_site = 2; ++g_loop_cnt[2]; g_current_loop_pc = (int)pc; h7344_probe(&s, pc);
    fix682a(&s, &pc);
BlockFn fn = gbn(pc);
    { uint32_t gpc = pc; if ((gpc & ~1u) == 0x81206BDCu && s.r[0] != 0u && s.r[0] < 0x80000000u) {
        // 18.3871: diagnostic guard - the stub is being entered with a small/garbage r0 (M330: r0=2).
        printf("[GUARD-RG] stub r0=%08X r13=%08X r14=%08X r7=%08X r1=%08X cur=%08X pump=%d ring:", s.r[0], s.r[13], s.r[14], s.r[7], s.r[1], g_cur_fiber, g_pumping2);
        for (int i = 0; i < 32; ++i) printf(" %08X", g_pc_ring[(g_pc_ring_n + (uint32_t)i) & 63u]);
        printf("\n");
        { uint32_t r7 = s.r[7], p304 = 0; uint8_t* q = GP(r7 + 0x304u); if (q) std::memcpy(&p304, q, 4);
          printf("[GUARD*] r7=%08X [r7+0x304]=%08X\n", r7, p304); }
        fflush(stdout); std::abort(); } }
    if (g_after_nofn > 0) { --g_after_nofn; if (g_after_total++ < 400) printf("[AFTER] #%d pc=%08X r0=%08X r14=%08X sp=%08X\n", 12 - g_after_nofn, pc, s.r[0], s.r[14], s.r[13]); }
    if ((pc & ~1u) == 0x81206BDCu && s.r[0] == 0u) {
      // 18.3866: the game sometimes calls the fiber start stub with r0=0 (stub then derefs [r0+0x88]).
      // Supply the current fiber object so the stub can jump via [fiber+0x84].
      uint32_t fobj = g_cur_fiber ? g_cur_fiber : (g_fiber_list.empty() ? 0u : g_fiber_list[0]);
      s.r[0] = fobj;
      if (g_stub_fix_n++ < 8) printf("[STUBFIX] r0=0 -> %08X (cur=%08X sp=%08X lr=%08X)\n", fobj, g_cur_fiber, s.r[13], s.r[14]);
    }
    if ((pc & ~1u) == 0x81206BDCu) {
      ++g_stub_total;
      { uint32_t f = s.r[0];
        uint32_t e = 0, a = 0, l94 = 0, l98 = 0;
        { uint8_t* q = GP(f + 0x84); if (q) std::memcpy(&e, q, 4); }
        { uint8_t* q = GP(f + 0x88); if (q) std::memcpy(&a, q, 4); }
        { uint8_t* q = GP(f + 0x94); if (q) std::memcpy(&l94, q, 4); }
        { uint8_t* q = GP(f + 0x98); if (q) std::memcpy(&l98, q, 4); }
        printf("[STUB2] #%d fiber=%08X entry=%08X arg=%08X l94=%08X l98=%08X gbn_entry=%p\n",
               g_stub_total, f, e, a, l94, l98, (void*)gbn(e ? e : 0)); }
      g_after_stub = 10;
      if (g_stub_n++ < 40) {
        printf("[STUB] #%d r0=%08X r13=%08X r14=%08X cur=%08X inexec=%d pump=%d prev:",
               g_stub_total, s.r[0], s.r[13], s.r[14], g_cur_fiber, g_in_fiber_exec, g_pumping2);
        for (int k = 0; k < 6; ++k) printf(" %08X", g_pre_pc[(g_pre_idx + 8 - 6 + k) & 7]);
        printf("\n");
        if (g_stub_total > 30) printf("[STUB] stopping detailed prints after 30\n");
      }
    }
    if (g_after_stub > 0) { --g_after_stub; printf("[AFTERSTUB] pc=%08X r0=%08X r14=%08X\n", pc, s.r[0], s.r[14]); }
    g_pre_pc[g_pre_idx++ & 7] = pc; if (g_ws_on) { uint32_t _v = 0; uint8_t* _q = GP(g_ws_addr); if (_q) std::memcpy(&_v, _q, 4); if (_v != g_ws_snap && g_ws_n++ < 20) { printf("[WSLOT-RG] prev=%08X now=%08X sp=%08X old=%08X new=%08X cur=%08X\n", g_cur_block, pc, s.r[13], g_ws_snap, _v, g_cur_fiber); printf("[WSLOT-WIN] A0DFFFC0..DC:"); for (int _i = 0; _i < 8; ++_i) { uint32_t _w = 0; uint8_t* _q = GP(0xA0DFFFC0u + (uint32_t)_i * 4u); if (_q) std::memcpy(&_w, _q, 4); printf(" %08X", _w); } printf(" pre:"); for (int _i = 0; _i < 4; ++_i) printf(" %08X", g_pre_pc[(g_pre_idx + 8u - 4u + (uint32_t)_i) & 7u]); printf("\n"); g_ws_snap = _v; fflush(stdout); } } if (g_gxmvk_present_n < 3 && ++g_gxmvk_tick > 2000) { ++g_gxmvk_present_n; gxmvk_present_clear(0.10f, 0.35f, 0.80f); }
    if (g_stk_range_on) {
      uint32_t _cur[16]; int _chg = 0;
      for (int _i = 0; _i < 16; ++_i) { _cur[_i] = 0; uint8_t* _q = GP(0xA05FFF00u + (uint32_t)_i * 4u); if (_q) std::memcpy(&_cur[_i], _q, 4); if (_cur[_i] != g_stk_snap[_i]) _chg = 1; }
      if (_chg && g_stkrange_n++ < 40) {
        printf("[STKRANGE] prev_blk=%08X now_pc=%08X sp=%08X:", g_cur_block, pc, s.r[13]);
        for (int _i = 0; _i < 16; ++_i) printf(" %08X", _cur[_i]);
        printf("\n"); fflush(stdout);
        for (int _i = 0; _i < 16; ++_i) g_stk_snap[_i] = _cur[_i];
      }
    }
    if ((pc & ~1u) == 0x811DA554u) { for (int _i = 0; _i < 16; ++_i) { g_stk_snap[_i] = 0; uint8_t* _q = GP(0xA05FFF00u + (uint32_t)_i * 4u); if (_q) std::memcpy(&g_stk_snap[_i], _q, 4); } g_stk_range_on = 1; }
    if ((pc & ~1u) == 0x8120732Cu) { g_stk_range_on = 0; }
 if ((pc & ~1u) == 0x811E6A1Eu) { g_e6a1e_sp[g_e6a1e_n++ & 7u] = s.r[13]; }
    if (fn) ++g_blkcnt[pc];   // 18.3829: also count run_guest (main-thread) blocks, not only fiber_exec's
    g_pc_ring[g_pc_ring_n++ & 63u] = pc;   // 18.3869: ring for crash attribution in run_guest too
    if (((pc & ~1u) == 0x811E5CDAu || (n % 2000) == 0) && !g_pumping2 && g_fiber_list.size() > 1 && g_pump_cool == 0) {   // 18.4217: pump more often so the loader fiber gets enough time to run GXM init
      // 18.3860: fiber2 pump v4 - per-frame task; FIRST pump force-resets fiber2's FCtx to the user entry
      // (M319: the leftover FCtx had a non-zero pc=0x8115D7C4 which corrupted every pass).
      if (!g_pump_disabled_ab) g_pumping2 = 1; if (g_seq_n++ < 60) printf("[SEQ] #%d pump_start\n", g_seq_n);
      static int pump_first = 1;
      uint32_t f1 = g_cur_fiber ? g_cur_fiber : g_fiber_list[0];
      uint32_t f2 = (g_fiber_list[1] == f1) ? g_fiber_list[0] : g_fiber_list[1];
      if (g_fiblog_n++ < 10) {
        printf("[FIBLIST] pump-start count=%zu:", g_fiber_list.size());
        for (size_t _i = 0; _i < g_fiber_list.size(); ++_i) {
          uint32_t _fb = g_fiber_list[_i], _off = _fb - IMG_VA_BASE, _e84 = 0, _e88 = 0;
          if (_off + 0x90 <= IMG_SIZE) { std::memcpy(&_e84, gp_img + _off + 0x84, 4); std::memcpy(&_e88, gp_img + _off + 0x88, 4); }
          uint32_t _cp = 0, _cr0 = 0, _csp = 0; auto _it = g_fctx.find(_fb); if (_it != g_fctx.end()) { _cp = _it->second.r[15]; _cr0 = _it->second.r[0]; _csp = _it->second.r[13]; }
          printf(" [%zu]=%08X entry=%08X arg=%08X ctxpc=%08X r0=%08X sp=%08X", _i, _fb, _e84, _e88, _cp, _cr0, _csp);
        }
        printf("\n"); fflush(stdout);
      }

      { FCtx& c = g_fctx[f1]; std::memcpy(c.r, s.r, sizeof(c.r)); c.apsr = s.apsr; }
      FCtx f1_pre = g_fctx[f1]; uint32_t f1_pre_pc = f1_pre.r[15];   // 18.4210: pre-pump snapshot of the suspended fiber (the pump must not perturb it)
      static int g_p2r_n = 0;
      FCtx& c2 = g_fctx[f2];
      const bool f2_ran = (g_f2_seen == f2);
      if (pump_first && g_boot_thunk && g_fiber_entry) {   // 18.4218: run the boot thunk as the nested path did
        std::memset(&c2, 0, sizeof(c2));
        c2.r[0] = f2; c2.r[13] = 0xA0E00000u; c2.r[14] = 0xFFFFFFF0u; c2.r[15] = g_fiber_entry & ~1u;
        if (g_pump2_n <= 6) { printf("[BOOTTHUNK] fiber=%08X entry=%08X sp=%08X\n", f2, c2.r[15], c2.r[13]); fflush(stdout); }
      } else if ((pump_first && !f2_ran) || c2.r[15] == 0u) {
        pump_first = 0;
        uint32_t off2 = f2 - IMG_VA_BASE, arg2 = 0;
        if (off2 + 0x90 <= IMG_SIZE) std::memcpy(&arg2, gp_img + off2 + 0x88, 4);
        std::memset(&c2, 0, sizeof(c2));
        c2.r[0] = arg2 ? arg2 : f2;
        c2.r[13] = 0xA0600000u; c2.r[14] = 0xFFFFFFF0u; c2.r[15] = 0x81198C0Au;
        if (g_pump2_n <= 6) printf("[PMP2] fiber2 FORCE-RESET user-entry r0=%08X\n", c2.r[0]);
      } else {
        if (g_pump2_n <= 6) printf("[PMP2] fiber2 RESUME saved ctx pc=%08X (f2_ran=%d)\n", c2.r[15], (int)f2_ran);
        pump_first = 0;
      }
      std::memcpy(s.r, c2.r, sizeof(s.r)); s.apsr = c2.apsr; g_cur_fiber = f2;
      auto g_pump_t0 = std::chrono::steady_clock::now();
      int budget = 20000; uint32_t pb = 0;   // 18.4182: larger pump budget
      uint32_t g_pump_total0 = g_blkcnt_total;   // 18.4180: nested progress base
      while (budget-- > 0) {
        uint32_t ppc = s.r[15];
        { static uint32_t _lg2=0; if (g_blkcnt_total - _lg2 > 300000u) { _lg2 = g_blkcnt_total;
            if (g_stall2_n++ < 24) { printf("[PMPTRACE] n=%u ppc=%08X blk=%08X fib=%08X sp=%08X\n", g_blkcnt_total, ppc, g_cur_block, g_cur_fiber, s.r[13]); fflush(stdout); } } }
        if (g_hacks && (ppc & ~1u) == 0x8115F9D0u) { s.r[0] = 0u; }  // [ABORTSKIP3] fiber2 pump loop
        if ((ppc & ~1u) == 0x81207734u && g_allocbig2_n++ < 80) {
          printf("[ALLOCBIG2] #%d r0(size)=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X fib=%08X\n", g_allocbig2_n, s.r[0], s.r[1], s.r[2], s.r[3], s.r[14], s.r[13], g_cur_fiber); fflush(stdout);
        }
        if ((ppc & ~1u) == 0x81207290u && s.r[0] < 0x80000000u && g_atombad_n++ < 20) {
          printf("[ATOMBAD] #%u r0=%08X r1=%08X r2=%08X lr=%08X sp=%08X fib=%08X\n", g_atombad_n, s.r[0], s.r[1], s.r[2], s.r[14], s.r[13], g_cur_fiber); fflush(stdout);
        }
        if (g_hacks && (ppc & ~1u) == 0x811F019Eu && s.r[0] == 0u) { ppc = s.r[14]; if (g_refnull_n++ < 8) { printf("[REFNULL] skip ref(null) lr=%08X fib=%08X\n", s.r[14], g_cur_fiber); fflush(stdout); } continue; }
if (g_hacks && (ppc & ~1u) == 0x811E2B50u) {
  uint32_t _a = 0, _b = 0, _c = 0; uint8_t* _q = GP(s.r[1] + 0x40u); if (_q) std::memcpy(&_a,_q,4);
  if (_a >= 0x80000000u) { _q = GP(_a + 0x78u); if (_q) std::memcpy(&_b,_q,4); }
  if (_b >= 0x80000000u) { _q = GP(_b + 0x20u); if (_q) std::memcpy(&_c,_q,4); }
  if (_a >= 0x80000000u && _b >= 0x80000000u && _c == 0u) { ppc = s.r[14];
    if (g_ret1_n++ < 8) { printf("[RET1] skip FUN_811e2b50: model+0x20 null (r1=%08X lr=%08X fib=%08X)\n", s.r[1], s.r[14], g_cur_fiber); fflush(stdout); } continue; }
}
        if (g_hacks && (ppc & ~1u) == 0x811C4122u && s.r[0] == 0u) {   // 18.4199b: pump-loop variant of the [obj+0x20] source substitution
          static uint32_t _dp2 = 0;
          if (!_dp2) { _dp2 = hle_malloc(0x100u); if (_dp2) { uint8_t* _q = GP(_dp2); if (_q) { uint32_t w[4] = { 0x47534650u, _dp2 + 4u, 0u, 0u }; std::memcpy(_q, w, 16); } } }
          if (_dp2) { if (g_dp_n++ < 8) { printf("[DUMMYPROG] creator +0x20 <- %08X (pump) fib=%08X\n", _dp2, g_cur_fiber); fflush(stdout); } s.r[0] = _dp2; }
        }
        if ((ppc & ~1u) == 0x811E2B50u && g_chaind_n++ < 6) {
          uint32_t _sl=s.r[1], _a=0,_b=0,_c=0; uint8_t* _q;
          _q=GP(_sl+0x40u); if(_q) std::memcpy(&_a,_q,4);
          if (_a>=0x80000000u) { _q=GP(_a+0x78u); if(_q) std::memcpy(&_b,_q,4); }
          if (_b>=0x80000000u) { _q=GP(_b+0x20u); if(_q) std::memcpy(&_c,_q,4); }
          printf("[CHAIN] sl=%08X A=%08X B=%08X B+0x20=%08X fib=%08X (pump)\n", _sl,_a,_b,_c,g_cur_fiber); fflush(stdout);
        }
        if ((ppc & ~1u) == 0x8120682Au && g_p682a_n++ < 20) { printf("[P682A] pump pc=%08X r0=%08X r5=%08X r7=%08X sp=%08X fib=%08X\n", ppc, s.r[0], s.r[5], s.r[7], s.r[13], g_cur_fiber); fflush(stdout); }
        if (family_skip(&s, ppc)) { ppc = s.r[15]; continue; }
        if (nulldesc_skip(&s, ppc)) { ppc = s.r[15]; continue; }
        if (assert_hle(&s, ppc)) { ppc = s.r[15]; continue; }
        assert_msg_probe(&s, ppc);
        initwatch_probe(&s, ppc);
        g_loop_site = 3; ++g_loop_cnt[3]; g_current_loop_pc = (int)ppc;
        h7344_probe(&s, ppc);
        fix682a(&s, &ppc);
        BlockFn f2fn = gbn(ppc);
        if (!f2fn) { int back = 0; uint32_t bq = gbn_back(ppc, &back); if (bq && back <= 16) { f2fn = gbn(bq); ppc = bq; } }
        if (!f2fn) { if (g_pump2_n <= 8) printf("[PMP2] fiber2 no block pc=%08X\n", ppc); break; }
        if (pb < 12) printf("[PMP2B] ppc=%08X r0=%08X sp=%08X\n", ppc, s.r[0], s.r[13]);
        if ((pb & 0x3F) == 0) {   // 18.4181 / 18.4209: wall-clock budget for the pump pass - DISABLED by default so runs are deterministic (P5D_PUMP_WALLCLOCK=1 restores it)
          if (g_pump_wallclock) {
            auto _now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(_now - g_pump_t0).count() > 80) { g_pump_timeout_n++; break; }
          }
        }
        uint32_t nx = f2fn(&s); ++pb;
        if (g_fiber_switch_to || g_switch_pending || g_fiber_flag_return) {   // 18.4210b / 18.4214 / 18.4217
          if (false && g_fiber_switch_to && g_fiber_switch_to != f2) {   // 18.4215 disabled (18.4216): pure deferral is faster and equally non-progressing switch target is the main fiber (idle in the main loop) -> keep the loader fiber running (no useful state lost)
            static int g_pumpcont_n = 0;
            if (g_pumpcont_n++ < 6) { printf("[PUMPCONT] keep f2: requested=%08X cur=%08X ppc=%08X resume=%08X\n", g_fiber_switch_to, f2, ppc, nx); fflush(stdout); }
            g_fiber_switch_to = 0;
            s.r[15] = nx;
            continue;
          }
          s.r[15] = nx;   // CRITICAL 18.4214: save the RESUME pc (next block) - saving the switch-call block makes the pump re-enter the same switch forever
          static int g_pumpsw_n = 0;
          if (g_pumpsw_n++ < 10) { printf("[PUMPSW] stop pump pass: switch_to=%08X flag_return=%d ppc=%08X resume=%08X\n", g_fiber_switch_to, g_fiber_flag_return, ppc, nx); fflush(stdout); }
          break; }
        if (g_yield_to_main || (g_blkcnt_total - g_pump_total0) > 200000u) { g_yield_to_main = 0;
          if (g_pump_brk_n++ < 8) { printf("[PMPBREAK] pass=%d pb=%u total_delta=%u\n", g_pump2_n, pb, g_blkcnt_total - g_pump_total0); fflush(stdout); } break; }
        if (pb <= 12) printf("[PMP2C] nx=%08X r0=%08X\n", nx, s.r[0]);
        if (nx == 0u || nx == 0xFFFFFFF0u || nx == 0x81555E98u) { s.r[15] = 0u; break; }   // 18.3954: fiber2 task completion marker
        s.r[15] = nx;
      }
      FCtx& c = g_fctx[f2];
      if (g_cur_fiber != f2) {   // 18.4212: never store another fiber's live context into f2 (f2_pc=0x81207344 corruption entry)
        static int g_fsrej_n = 0;
        if (g_fsrej_n++ < 10) { printf("[FSAVE-REJECT] cur=%08X f2=%08X pc=%08X r0=%08X\n", g_cur_fiber, f2, s.r[15], s.r[0]); fflush(stdout); }
      } else { std::memcpy(c.r, s.r, sizeof(c.r)); c.apsr = s.apsr; }
      {
        g_p2_prog = c.r[14];
        if (c.r[15] != g_p2_last && g_p2_chg_n++ < 200) { g_p2_last = c.r[15];
          printf("[P2PC] pass=%d pc=%08X lr=%08X r1=%08X\n", g_p2_pass, c.r[15], c.r[14], c.r[1]); fflush(stdout); }
        if ((++g_p2_pass % 100) == 0 && g_p2_log++ < 300) { printf("[P2PROG] pass=%d pc=%08X lr=%08X r1=%08X pb=%u total=%u\n", g_p2_pass, c.r[15], c.r[14], c.r[1], pb, g_blkcnt_total); fflush(stdout); } } if (g_seq_n++ < 60) { uint32_t _v = 0; auto _it = g_fctx.find(f2); if (_it != g_fctx.end()) _v = _it->second.r[15]; printf("[SEQ] #%d save_f2_r15=%08X\n", g_seq_n, _v); }
      { std::memcpy(s.r, f1_pre.r, sizeof(s.r)); s.apsr = f1_pre.apsr; }   // 18.4210: restore the pre-pump snapshot
      { uint32_t _cb815 = 0; uint8_t* _q815 = GP(0x81513DECu); if (_q815) std::memcpy(&_cb815, _q815, 4);
        if (g_p2r_n < 12 || f1_pre_pc != 0x81198BF2u) { if (g_p2r_n++ < 60) printf("[PMP2RESTORE] n=%d pre_pc=%08X pre_lr=%08X pre_r0=%08X pre_sp=%08X restored_pc=%08X cb=%08X f2_pc=%08X\n", g_p2r_n, f1_pre_pc, f1_pre.r[14], f1_pre.r[0], f1_pre.r[13], s.r[15], _cb815, g_fctx[f2].r[15]); fflush(stdout); } }
      if (g_seq_n++ < 60) printf("[SEQ] #%d restore_f1_r15=%08X\n", g_seq_n, s.r[15]);
      g_cur_fiber = f1; g_pumping2 = 0; g_pump_cool = 1000;   // 18.4181: pump cooldown so the main loop can advance
      if (g_pump2_n++ < 8) printf("[PMP2] pass #%d blocks=%u\n", g_pump2_n, pb);
      // 18.3988: fix stale fn - re-evaluate pc/fn from the RESTORED fiber1 context (the stale fn made the loop dispatch the pump-trigger block and jump back into the loader chain -> BADMEM 0x94)
      pc = s.r[15]; fix682a(&s, &pc); h7344_probe(&s, pc); fn = gbn(pc);
    }
    if (!fn) {
      // 18.3804: unknown pc right after the main thread returned = fiber return address into non-AOT space.
      // Resume the live fiber instead of stopping so the game keeps running in its fiber world.
      uint32_t fb = g_fiber_list.empty() ? 0u : g_fiber_list[0];
      auto itf = g_fctx.find(fb);
      if (fb && itf == g_fctx.end()) {
        // 18.3864: the game re-created its fiber in the (new) heap arena -> our FCtx is missing.
        // Build a fresh context from the fiber object (M323: resume fiber=0x84676750 had no FCtx).
        uint32_t off = fb - IMG_VA_BASE, entry = 0, arg = 0;
        if (off + 0x90 <= IMG_SIZE) { std::memcpy(&entry, gp_img + off + 0x84, 4); std::memcpy(&arg, gp_img + off + 0x88, 4); }
        FCtx cc; std::memset(&cc, 0, sizeof(cc));
        cc.r[0] = arg ? arg : fb;
        cc.r[13] = 0xA0E00000u;
        cc.r[14] = 0xFFFFFFF0u;
        cc.r[15] = (entry ? entry : 0x81206BDCu) & ~1u;
        g_fctx[fb] = cc;
        itf = g_fctx.find(fb);
        if (g_nofn_n <= 8) printf("[NOFN] rebuilt FCtx new fiber=%08X entry=%08X arg=%08X\n", fb, entry, arg);
      }
      if (fb && itf != g_fctx.end() && g_nofn_n < 2000000) {
        ++g_nofn_n;
        if (g_seq_n++ < 60) printf("[SEQ] #%d nofn pc=%08X\n", g_seq_n, pc); if (g_seq_n <= 60) { printf("[SEQ-PRE] g_pre_idx=%u pre_ring:", g_pre_idx); for (int _i = 0; _i < 8; ++_i) printf(" %08X", g_pre_pc[(g_pre_idx + (uint32_t)_i) & 7u]); printf("\n"); } if (g_nofn_n <= 6) printf("[NOFN] unknown pc=%08X -> resume fiber=%08X (count %d) fctx_r0=%08X sp=%08X pc=%08X\n",
               pc, fb, g_nofn_n, itf->second.r[0], itf->second.r[13], itf->second.r[15]);

    if (g_nofn_n <= 6) { printf("[NOFN-FE] g_dfe_n=%u fe_ring:", g_dfe_n); for (int _i = 0; _i < 20; ++_i) printf(" %08X", g_dfe_pc[(g_dfe_n + (uint32_t)_i) % 20u]); printf("\n"); }

    if (g_nofn_n <= 6) { printf("[NOFN-MR] mr_n=%u:", g_mr_n); for (int _i = 0; _i < 40; ++_i) { uint32_t _k = (g_mr_n + (uint32_t)_i) % 40u; printf(" %08X>%08X", g_mr_pc[_k], g_mr_nx[_k]); } printf("\n"); }    if (g_nofn_n <= 6) { printf("[NOFN-PC] blk=%08X ring:", g_cur_block); for (int _i = 0; _i < 32; ++_i) printf(" %08X", g_pc_ring[(g_pc_ring_n + (uint32_t)_i) & 63u]); printf("\n"); }        g_after_nofn = 12;
        std::memcpy(s.r, itf->second.r, sizeof(s.r)); s.apsr = itf->second.apsr;
        g_cur_fiber = fb; g_in_fiber_exec = 1;
        uint32_t rnxt = s.r[15]; if (rnxt == 0u) rnxt = g_fiber_entry & ~1u;
        pc = rnxt; g_after_nofn = 20; continue;
      }
      printf("[HARNESS] no block for pc=%08X (last translated block 0x%08X) after %d blocks\n", pc, g_cur_block, n);
      break;
    }
    if (((pc & ~1u) == 0x81198A34u || (pc & ~1u) == 0x81198E2Eu) && g_entry_n < 16) {
      ++g_entry_n;
      printf("[CALLSITE] pc=%08X r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X r9=%08X r10=%08X r11=%08X sp=%08X\n",
             pc, s.r[0],s.r[1],s.r[2],s.r[3],s.r[4],s.r[5],s.r[6],s.r[7],s.r[8],s.r[9],s.r[10],s.r[11],s.r[13]);
    }
    if ((pc & ~1u) == 0x811D9DA2u && g_entry_n < 12) {
      ++g_entry_n;
      printf("[ENTRY] FUN_811d9da2 r0=%08X r1=%08X r2=%08X r3=%08X sp=%08X stack:",
             s.r[0], s.r[1], s.r[2], s.r[3], s.r[13]);
      for (int q = 0; q < 8; ++q) { uint32_t a = s.r[13] + q*4; uint32_t o = a - IMG_VA_BASE;
        if (o < IMG_SIZE) { uint32_t v; std::memcpy(&v, g_img.data()+o, 4); printf(" [%08X]=%08X", a, v); } }
      printf("\n");
    }
    if ((pc & ~1u) == 0x811D9F44u && g_epi_n < 6) {
      ++g_epi_n;
      uint32_t sp = s.r[13];
      printf("[EPI] before pop.w: r9=%08X sp=%08X", s.r[9], sp);
      for (int q = 0; q < 10; ++q) { uint32_t a = sp + q*4; uint32_t o = a - IMG_VA_BASE;
        if (o + 4 <= IMG_SIZE) { uint32_t v; std::memcpy(&v, g_img.data()+o, 4); printf(" [%08X]=%08X", a, v); } }
      printf("\n");
    }
    if ((pc & ~1u) == 0x81198F08u && g_f08_n < 6) {
      ++g_f08_n;
      printf("[F08] enter block: r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X r9=%08X r10=%08X r11=%08X sp=%08X\n",
             s.r[0],s.r[1],s.r[2],s.r[3],s.r[4],s.r[5],s.r[6],s.r[7],s.r[8],s.r[9],s.r[10],s.r[11],s.r[13]);
      printf("[F08] r5 target = %08X (rd32 at r5), sb+0x64 = %08X\n", s.r[5], s.r[9] + 0x64);
    }
    if (((pc & ~1u) == 0x8120732Cu || (pc & ~1u) == 0x81207308u) && g_wrap_n < 12) {
      ++g_wrap_n;
      printf("[WRAP] enter 0x%08X: r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X r9=%08X r10=%08X r11=%08X sp=%08X\n",
             pc, s.r[0],s.r[1],s.r[2],s.r[3],s.r[4],s.r[5],s.r[6],s.r[7],s.r[8],s.r[9],s.r[10],s.r[11],s.r[13]);
    }
    if (((pc & ~1u) == 0x811E603Eu) && g_evt_n < 8) {
      ++g_evt_n;
      printf("[EVT] enter 0x811E603E from block 0x%08X: r0=%08X r1=%08X r2=%08X r3=%08X sp=%08X lr=%08X\n",
             g_cur_block, s.r[0],s.r[1],s.r[2],s.r[3],s.r[13],s.r[14]);
    }
    if (((pc & ~1u) == 0x812074E4u || (pc & ~1u) == 0x8120756Cu || (pc & ~1u) == 0x81207542u) && g_sp_n < 16) {
      ++g_sp_n;
      printf("[SPT] enter 0x%08X sp=%08X r0=%08X r9=%08X lr=%08X\n", pc, s.r[13], s.r[0], s.r[9], s.r[14]);
    }
    if (g_watch_addr == 0) { g_watch_addr = 0x85003D54u; g_watch_val = 0xDEADBEEFu;   // 18.4173: watch the wait-object state field (calloc 0x85003D50 + 4)
      { uint32_t o = g_watch_addr - IMG_VA_BASE; if (o + 4 <= IMG_SIZE) std::memcpy(&g_watch_val, gp_img + o, 4); }
      printf("[WATCHADDR] set to %08X initial=%08X (expect saved lr 811F0ADE)\n", g_watch_addr, g_watch_val); }
    if ((pc & ~1u) == 0x8120683Cu && g_h68_n < 6) {
      ++g_h68_n;
      printf("[H68] enter 0x8120683C: r0=%08X r1=%08X r2=%08X r3=%08X lr=%08X sp=%08X block=%08X\n",
             s.r[0],s.r[1],s.r[2],s.r[3],s.r[14],s.r[13],g_cur_block);
    }
    if ((pc & ~1u) == 0x81206760u && g_f60_n < 6) {
      ++g_f60_n;
      printf("[F60] before cmp: r0=%08X r4=%08X r1=%08X sp=%08X lr=%08X block=%08X\n",
             s.r[0],s.r[4],s.r[1],s.r[13],s.r[14],g_cur_block);
    }
    if (((pc & ~1u) == 0x8120679Au || (pc & ~1u) == 0x8120679Cu || (pc & ~1u) == 0x81198A5Au
         || (pc & ~1u) == 0x81198A80u) && g_fx_n < 24) {
      ++g_fx_n;
      printf("[FX] pc=%08X r0=%08X r4=%08X r5=%08X sp=%08X lr=%08X block=%08X\n",
             pc, s.r[0],s.r[4],s.r[5],s.r[13],s.r[14],g_cur_block);
    }
    if ((pc & ~1u) == 0x811E5970u && g_e59_n < 4) {
      ++g_e59_n;
      printf("[E59] enter 0x811E5970: r5=%08X r6=%08X r0=%08X r1=%08X r9=%08X sp=%08X\n",
             s.r[5],s.r[6],s.r[0],s.r[1],s.r[9],s.r[13]);
    }
    uint32_t rb[13]; for (int i = 0; i < 13; ++i) rb[i] = s.r[i];
    uint32_t nxt = fn(&s);
    { static uint32_t _lastg=0; if (g_blkcnt_total - _lastg > 500000u) { _lastg = g_blkcnt_total;
        if (g_stall_n++ < 20) { printf("[STALLTRACE] n=%u pc=%08X blk=%08X fib=%08X loop=%d sp=%08X fex=%d\n", g_blkcnt_total, pc, g_cur_block, g_cur_fiber, g_loop_site, s.r[13], g_in_fex); fflush(stdout); } } }
    if (g_switch_pending || g_fiber_switch_to) {   // 18.4217: synchronous switch consumed at the dispatcher's safe point with the REAL next-pc (nx)
      uint32_t _t = g_switch_pending ? g_switch_target : g_fiber_switch_to;
      g_switch_pending = 0; g_fiber_switch_to = 0;
      if (_t && _t != g_cur_fiber) {
        if (g_cur_fiber) { FCtx& _c = g_fctx[g_cur_fiber]; std::memcpy(_c.r, s.r, sizeof(s.r)); _c.apsr = s.apsr; _c.r[15] = nxt; }
        auto _it = g_fctx.find(_t);
        if (_it == g_fctx.end() || _it->second.r[15] == 0u) {
          uint32_t _off = _t - IMG_VA_BASE, _ent = 0, _arg = 0;
          if (_off + 0x90 <= IMG_SIZE) { std::memcpy(&_ent, gp_img + _off + 0x84, 4); std::memcpy(&_arg, gp_img + _off + 0x88, 4); }
          FCtx _cc; std::memset(&_cc, 0, sizeof(_cc));
          _cc.r[0] = _arg ? _arg : _t; _cc.r[13] = 0xA0E00000u; _cc.r[14] = 0xFFFFFFF0u; _cc.r[15] = _ent ? (_ent & ~1u) : 0u;
          if (_cc.r[15]) { g_fctx[_t] = _cc; _it = g_fctx.find(_t); }
        }
        if (_it != g_fctx.end() && _it->second.r[15] != 0u) {
          std::memcpy(s.r, _it->second.r, sizeof(s.r)); s.apsr = _it->second.apsr; g_cur_fiber = _t; nxt = s.r[15];
          if (g_fsw_n++ < 12) { printf("[MSW] sync switch -> %08X pc=%08X r0=%08X\n", _t, s.r[15], s.r[0]); fflush(stdout); }
        }
      }
    }
    if (s.r[10] != rb[10] && g_r10n < 40) {
      printf("[R10CHG] block=0x%08X old=%08X new=%08X n=%d next=%08X\n", g_cur_block, rb[10], s.r[10], n+1, nxt);
      ++g_r10n;
    }
    ++n;
    if (g_watch_addr != 0) {   // 18.4173: GP-based watch (heap addresses are not at va-IMG_VA_BASE)
      uint8_t* _wp = GP(g_watch_addr);
      if (_wp) { uint32_t v; std::memcpy(&v, _wp, 4);
        if (v != g_watch_val) { printf("[WWRITE] block=%08X old=%08X new=%08X n=%d addr=%08X\n", g_cur_block, g_watch_val, v, n, g_watch_addr); g_watch_val = v; if (g_ww_n++ > 6) g_watch_addr = 0; } }
    }
    if (g_cur_block == 0x811D9F44u && g_epi2_n < 6) {
      ++g_epi2_n;
      printf("[EPI2] after pop.w: r9=%08X sp=%08X next=%08X\n", s.r[9], s.r[13], nxt);
    }
    if ((n % 5000) == 0) {
      { uint32_t _fw[4] = {0,0,0,0}; uint8_t* _fq0 = GP(0x816CC630u); if (_fq0) std::memcpy(_fw, _fq0, 16);
  printf("[FRAMEDUMP] n=%d flags=%08X t4=%u t8=%u ctr=%u pc=%08X blk=%08X sp=%08X fib=%08X gxm(b/e/p)=%d/%d/%d\n", n, _fw[0], _fw[1], _fw[2], _fw[3], nxt, g_cur_block, s.r[13], g_cur_fiber, g_gxm_begin_n, g_gxm_end_n, g_gxm_present_n); }
if (0) { uint32_t _fc = 0; uint8_t* _fq = GP(0x816CC63Cu); if (_fq) std::memcpy(&_fc, _fq, 4);
  printf("[FRAMECTR-old] %u\n", _fc); }
    }
    if (nxt == 0u) {
      { static int _mrd=0; if (_mrd++ < 12) { uint32_t _f0=(g_fiber_list.empty()?0:g_fiber_list[0]); uint32_t _c15=0; auto _it=g_fctx.find(_f0); if(_it!=g_fctx.end()) _c15=_it->second.r[15];
        printf("[MRDBG] nxt0 cur_fib=%08X fex=%d list=%zu f0=%08X f0ctx15=%08X thread15=%08X fentry=%08X blk=%08X sp=%08X\n", g_cur_fiber, g_in_fex, g_fiber_list.size(), _f0, _c15, g_thread_regs[15], g_fiber_entry, g_cur_block, s.r[13]); fflush(stdout); } }
      // 18.3804: the guest MAIN THREAD returning 0 means main() returned - but the fiber world is still alive.
      // Don't stop: switch back into a live fiber and continue the block loop (the game keeps running).
      uint32_t fb = g_fiber_list.empty() ? 0u : g_fiber_list[0];
      auto itf = g_fctx.find(fb);
      if (fb && itf != g_fctx.end()) {
        {   // [RESTART2] if BOTH main thread and all fibers are finished, restart the guest main thread instead of resuming a dead stub forever
          uint32_t _cp = itf->second.r[15];
          bool _fibfin = (_cp == 0u || _cp == 0x81206C5Cu);
          bool _thrdone = (g_thread_regs[15] == 0u);
          if (_fibfin && _thrdone && g_restart_n < 3) {
            ++g_restart_n;
            s = g_init_state;
            pc = start; nxt = start;
            printf("[RESTART2] main+fiber finished -> restart guest main from %08X (n=%d attempt %d)\n", pc, n, g_restart_n); fflush(stdout);
            g_cur_fiber = 0; g_in_fiber_exec = 0;
            continue;
          }
        }
        std::memcpy(s.r, itf->second.r, sizeof(s.r)); s.apsr = itf->second.apsr;
        g_cur_fiber = fb; g_in_fiber_exec = 1;
        nxt = s.r[15];
        if (nxt == 0u) nxt = g_fiber_entry & ~1u;
        if (g_mainret_n++ < 6) printf("[MAINRET] main returned at n=%d -> resume fiber=%08X pc=%08X\n", n, fb, nxt);
        prev_blk = g_cur_block;
        pc = nxt;
        continue;
      }
      if (s.r[15] == 0u && g_restart_n < 3) {
        ++g_restart_n;
        s = g_init_state;
        pc = start; nxt = start;
        printf("[RESTART] guest main thread returned; restarting from %08X (n=%d, attempt %d)\n", pc, n, g_restart_n); fflush(stdout);
        continue;
      }
      printf("[HARNESS] return-to-caller after %d blocks (block 0x%08X)\n", n, g_cur_block); break;
    }
    if (nxt == 0xFFFFFFFFu) { printf("[HARNESS] no-block marker after %d blocks (block 0x%08X)\n", n, g_cur_block); break; }
    if (nxt == 0xFFFFFFFEu) { printf("[HARNESS] indirect untranslated at pc=%08X after %d blocks (block 0x%08X)\n", pc, n, g_cur_block); break; }
    if (nxt == 0xFFFFFFFDu) { printf("[HARNESS] indirect miss at pc=%08X after %d blocks (block 0x%08X)\n", pc, n, g_cur_block); break; }
    if (s.r[9] != rb[9] && g_r9n < 40) {
      printf("[R9CHG] block=0x%08X old=%08X new=%08X n=%d next=%08X\n", g_cur_block, rb[9], s.r[9], n, nxt);
      ++g_r9n;
    }
    {
      int zd = -1;
      for (int i = 4; i <= 11; ++i) if (s.r[i] == 0 && rb[i] != 0) { zd = i; break; }
      if (false && zd >= 0) {   // 18.3806: disabled - this block printed [WATCH] on almost every block (339,855 lines, 137MB log) and was the dominant printf cost
        if (g_watch_n < 60) { printf("[WATCH] r%d zeroed after block 0x%08X (prev 0x%08X) n=%d next=%08X\n", zd, g_cur_block, prev_blk, n, nxt); ++g_watch_n; }
        { uint32_t sp = s.r[13]; printf("[WATCH] stack @%08X:", sp);
          for (int q = 0; q < 16; ++q) { uint32_t a = sp + q*4; uint32_t o = a - IMG_VA_BASE;
            if (o >= IMG_SIZE) { printf(" [%08X=??]", a); continue; }
            uint32_t v; std::memcpy(&v, g_img.data()+o, 4); printf(" [%08X]=%08X", a, v); }
          printf("\n"); }
        printf("[WATCH] before: r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X r9=%08X r10=%08X r11=%08X r12=%08X\n",
               rb[0],rb[1],rb[2],rb[3],rb[4],rb[5],rb[6],rb[7],rb[8],rb[9],rb[10],rb[11],rb[12]);
        printf("[WATCH] after : r0=%08X r1=%08X r2=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X r9=%08X r10=%08X r11=%08X r12=%08X\n",
               s.r[0],s.r[1],s.r[2],s.r[3],s.r[4],s.r[5],s.r[6],s.r[7],s.r[8],s.r[9],s.r[10],s.r[11],s.r[12]);
      }
    }
    if (n <= 120) printf("  after blk %d: pc=%08X r0=%08X r9=%08X sp=%08X\n", n, pc, s.r[0], s.r[9], s.r[13]);
    prev_blk = g_cur_block;
    pc = nxt;
  }
  { printf("[THRDUMP] count=%zu\n", (size_t)g_thr.size());
    for (auto& kv : g_thr) { auto& t = kv.second;
      printf("  tid=%08X entry=%08X alive=%d ready=%d waiting=%d obj=%08X bits=%08X mode=%08X exit=%d\n",
             kv.first, t.entry, t.alive, t.ready, t.waiting, t.wait_obj, t.wait_bits, t.wait_mode, t.exit_flag); }
    printf("[SEMA]"); for (auto& kv : g_sema) printf(" %08X=%u", kv.first, kv.second); printf("\n");
    printf("[EFLAGS]"); for (auto& kv : g_eflag) printf(" %08X=%u", kv.first, kv.second); printf("\n"); }
  { std::vector<std::pair<int,uint32_t>> vb; for (auto& kv : g_blkcnt) vb.push_back({kv.second, kv.first});
    std::sort(vb.begin(), vb.end());
    printf("[TOPBLK] unique=%zu\n", vb.size());
    for (int i = (int)vb.size()-1, k = 0; i >= 0 && k < 30; --i, ++k) printf("  blk %08X x%d\n", vb[i].second, vb[i].first); }
  { std::vector<std::pair<int,uint32_t>> v; for (auto& kv : g_hlecnt) v.push_back({kv.second, kv.first});
    std::sort(v.begin(), v.end());
    printf("[HLECOUNT] unique=%zu total=%d\n", v.size(), g_hle_n);
    for (int i = (int)v.size()-1, k = 0; i >= 0 && k < 200; --i, ++k) printf("  %08X x%d\n", v[i].second, v[i].first); }
  printf("done: blocks=%d pc=%08X last_block=0x%08X r0=%08X r9=%08X\n", n, pc, g_cur_block, s.r[0], s.r[9]);
  printf("[CNT] 81198BEA=%d 811E5CDA=%d 811E5928=%d 8120683C=%d 81206860=%d FSLOT=%d\n",
         g_c_bea, g_c_5cda, g_c_5928, g_c_683c, g_c_6860, g_fslot_att);
  printf("[CNT2] mgr=%u 67B0=%u 67E8=%u 680E=%u 681C=%u 6820=%u 6822=%u\n",
         g_c_mgr, g_c_67b0, g_c_67e8, g_c_680e, g_c_681c, g_c_6820, g_c_6822);
  printf("[CONSUME] FHRead calls=%u bytes=%u FIOS-opens=%u\n", g_fread_n, g_fread_bytes, g_fh_id);
  return 0;
}

// 18.4204: symbols required by the old GXM/Vulkan backend (C:/dev/p5dsrc/src) when linked into this harness
static int g_vkmark_n = 0;
extern "C" unsigned p5d_last_exec_pc(void) { return (unsigned)g_cur_block; }
extern "C" unsigned p5d_guest_lr(void) { return 0u; }
extern "C" unsigned p5d_guest_reg(int i) { (void)i; return 0u; }
extern "C" double p5d_t_since_start(void) { return 0.0; }
extern "C" void p5d_t_mark(const char* what) {
  if (what && g_vkmark_n++ < 8) { printf("[VKMARK] %s\n", what); fflush(stdout); }
}

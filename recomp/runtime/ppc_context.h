// ppc_context.h — minimal XenonUtils-compatible PPC context for FH2 Android.
// Full XenonUtils ppc_context.h is used during offline recompilation
// (tools/run_recomp.sh passes the real header). This trimmed header lets the
// Android runtime + CI build without requiring the full submodule checkout.
#pragma once
#include <cstdint>

struct PPCContext {
    uint64_t gpr[32] = {};
    double fpr[32] = {};
    uint32_t cr = 0;
    uint32_t xer = 0;
    uint32_t lr = 0;
    uint32_t ctr = 0;
    uint32_t pc = 0;
    uint32_t fpscr = 0;
    uint32_t vrsave = 0;
    // Vector regs (Altivec) stored as 4x u32 for NDK Clang compatibility.
    uint32_t vr[32][4] = {};
    uint8_t* membase = nullptr;
    uint32_t membase_size = 0;
};

// Guest memory helpers (byte-swapped: Xbox 360 is big-endian).
inline uint8_t  ppc_lb(PPCContext* ctx, uint32_t ea) { return ctx->membase[ea]; }
inline void     ppc_stb(PPCContext* ctx, uint32_t ea, uint8_t v) { ctx->membase[ea] = v; }

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ppc_config.h"
#include "ppc_context.h"

void fh2_stub_hit(const char* name);

namespace {
uint8_t* g_base = nullptr;
static constexpr uint64_t LOADER_BASE = 0x88900000ULL;
static constexpr uint64_t HEAP_BASE = 0x88A00000ULL;
static constexpr uint64_t HEAP_END = 0x9E000000ULL;
std::atomic<uint64_t> g_heap{0};

inline uint32_t* G32(uint32_t va) {
    return reinterpret_cast<uint32_t*>(g_base + va);
}

uint32_t heap_alloc(uint32_t size) {
    uint32_t sz = (size + 0xFFFu) & ~0xFFFu;
    if (sz == 0) sz = 0x1000;
    uint64_t cur = g_heap.fetch_add(sz);
    if (cur + sz > HEAP_END) {
        fprintf(stderr, "[heap] OOM for %u bytes\n", sz);
        return 0;
    }
    return static_cast<uint32_t>(cur);
}
}  // namespace

extern "C" void fh2_kernel_init(uint8_t* base) {
    g_base = base;
    g_heap.store(HEAP_BASE);

    // Minimal loader shim. The CRT reads a module pointer from the loader
    // globals (0x8200076C / 0x8200092C), dereferences [+0] (module list head)
    // and [+88] (XEX header base). Seed a self-referential module + a fake
    // XEX header so the sanity gate in sub_82BFFC18/CF0 passes.
    uint32_t module_va = (uint32_t)LOADER_BASE;
    uint32_t header_va = (uint32_t)(LOADER_BASE + 0x1000);
    uint32_t zero_va   = (uint32_t)(LOADER_BASE + 0x2000);
    *G32(0x8200076C) = module_va;
    *G32(0x8200092C) = module_va;
    *G32(module_va + 0)  = module_va;    // list head self (non-zero)
    *G32(module_va + 88) = header_va;    // XEX header base
    *G32(header_va + 0)  = 0x58455832u;  // 'XEX2'
    *G32(header_va + 4)  = 0;            // module flags
    *G32(header_va + 8)  = 0x18;         // header size
    *G32(header_va + 12) = 0;            // reserved
    *G32(header_va + 16) = 0;            // security offset
    *G32(header_va + 20) = 1;            // header count
    *G32(header_va + 24) = 0x00020401u;  // opt-header key probed by the CRT
    *G32(header_va + 28) = zero_va;      // value -> zero word (gate wants 0)
    fprintf(stderr, "[loader] module=0x%08X header=0x%08X g[0x76C]=0x%08X g[0x92C]=0x%08X\n",
            module_va, header_va, *G32(0x8200076C), *G32(0x8200092C));
}

PPC_FUNC(__imp__RtlImageXexHeaderField) {
    uint32_t header = ctx.r3.u32;
    uint32_t key = ctx.r4.u32;
    uint32_t count = header ? *G32(header + 20) : 0;
    for (uint32_t i = 0; i < count && i < 64; i++) {
        uint32_t k = *G32(header + 24 + i * 8);
        uint32_t v = *G32(header + 24 + i * 8 + 4);
        if (k == key) {
            fprintf(stderr, "[loader] RtlImageXexHeaderField(hdr=0x%08X,key=0x%08X)=0x%08X (*=%u)\n",
                    header, key, v, v ? *G32(v) : 0xFFFFFFFFu);
            ctx.r3.u32 = v;
            return;
        }
    }
    fprintf(stderr, "[loader] RtlImageXexHeaderField(hdr=0x%08X,key=0x%08X)=MISS (count=%u)\n",
            header, key, count);
    ctx.r3.u32 = 0;
}

PPC_FUNC(__imp__NtAllocateVirtualMemory) {
    uint32_t out_base = ctx.r4.u32;
    uint32_t out_size = ctx.r6.u32;
    uint32_t req_base = out_base ? *G32(out_base) : 0;
    uint32_t req_size = out_size ? *G32(out_size) : 0x1000;
    uint32_t got = req_base ? req_base : heap_alloc(req_size);
    if (!got) { ctx.r3.s64 = static_cast<int32_t>(0xC0000017); return; }
    if (out_base) *G32(out_base) = got;
    if (out_size) *G32(out_size) = (req_size + 0xFFFu) & ~0xFFFu;
    ctx.r3.s64 = 0;
}

PPC_FUNC(__imp__NtFreeVirtualMemory) { ctx.r3.s64 = 0; }
PPC_FUNC(__imp__KeGetCurrentProcessType) { ctx.r3.s64 = 1; }

PPC_FUNC(__imp__RtlInitializeCriticalSection) {
    if (ctx.r3.u32) memset(G32(ctx.r3.u32), 0, 28);
    ctx.r3.s64 = 0;
}
PPC_FUNC(__imp__RtlEnterCriticalSection) { ctx.r3.s64 = 0; }
PPC_FUNC(__imp__RtlLeaveCriticalSection) { ctx.r3.s64 = 0; }
PPC_FUNC(__imp__RtlDeleteCriticalSection) { ctx.r3.s64 = 0; }

PPC_FUNC(__imp__ExAllocatePool) { ctx.r3.u32 = heap_alloc(ctx.r3.u32); }
PPC_FUNC(__imp__ExAllocatePoolWithTag) { ctx.r3.u32 = heap_alloc(ctx.r3.u32); }
PPC_FUNC(__imp__ExAllocatePoolTypeWithTag) { ctx.r3.u32 = heap_alloc(ctx.r3.u32); }
PPC_FUNC(__imp__ExFreePool) { (void)ctx; }
PPC_FUNC(__imp__ExFreePoolWithTag) { (void)ctx; }

PPC_FUNC(__imp__DbgPrint) { fh2_stub_hit("DbgPrint"); }
PPC_FUNC(__imp__DbgBreakPoint) { fh2_stub_hit("DbgBreakPoint"); }
PPC_FUNC(__imp__DbgBreakPointWithStatus) { fh2_stub_hit("DbgBreakPointWithStatus"); }

PPC_FUNC(__imp__HalReturnToFirmware) {
    fprintf(stderr, "[kernel] HalReturnToFirmware(%u)\n", ctx.r3.u32);
    exit(0);
}
PPC_FUNC(__imp__XamLoaderTerminateTitle) {
    fprintf(stderr, "[kernel] XamLoaderTerminateTitle\n");
    exit(0);
}
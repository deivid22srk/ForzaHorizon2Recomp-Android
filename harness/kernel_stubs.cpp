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
static constexpr uint64_t HEAP_BASE = 0x88000000ULL;
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
// Host boot harness for FH2 recompiled code (LOCAL ONLY, x86_64 Linux).
// Own code — no game bytes. Links against generated TUs at build time.
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

#include "ppc_config.h"
#include "ppc_context.h"
#include "ppc_recomp_shared.h"

extern "C" void fh2_kernel_init(uint8_t* base);

static std::atomic<uint64_t> g_stub_calls{0};
static uint8_t* g_base = nullptr;
// Guest VAs are absolute (image at 0x82000000, data up to ~0x83700000):
// base must accept base+VA, so reserve a 2.5GB sparse window (overcommit;
// only touched pages cost RAM). Stack/TLS live near the top, outside the image.
static constexpr size_t GUEST_SIZE = 0xA0000000ULL;

void fh2_stub_hit(const char* name) {
    uint64_t n = ++g_stub_calls;
    if (n < 40 || (n % 1000000) == 0) {
        fprintf(stderr, "[stub %llu] %s\n", (unsigned long long)n, name);
    }
}

static void on_fatal(int sig) {
    fprintf(stderr, "\nFATAL signal %d after %llu stub calls\n",
            sig, (unsigned long long)g_stub_calls.load());
    _exit(128 + sig);
}

extern "C" void fh2_broken_hit(uint32_t addr) {
    fprintf(stderr, "HIT analyser-split fragment sub_%08X\n", addr);
}

int main(int argc, char** argv) {
    const char* img_path = (argc > 1) ? argv[1] : "fh2dec.bin";
    FILE* f = fopen(img_path, "rb");
    if (!f) { perror("open decrypted image"); return 2; }
    fseek(f, 0, SEEK_END);
    long fsz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* img = (uint8_t*)malloc(fsz);
    if (fread(img, 1, fsz, f) != (size_t)fsz) { perror("read"); return 2; }
    fclose(f);

    g_base = (uint8_t*)mmap(nullptr, GUEST_SIZE, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (g_base == MAP_FAILED) { perror("mmap"); return 2; }

    // Copy PE sections (rawptr -> vaddr) for initialized globals.
    uint32_t lfanew;
    memcpy(&lfanew, img + 0x3C, 4);
    uint16_t nsec;
    memcpy(&nsec, img + lfanew + 6, 2);
    size_t shoff = lfanew + 248;
    for (int i = 0; i < nsec; i++) {
        char name[9] = {0};
        memcpy(name, img + shoff, 8);
        uint32_t vs, va, srd, ptr;
        memcpy(&vs, img + shoff + 8, 4);
        memcpy(&va, img + shoff + 12, 4);
        memcpy(&srd, img + shoff + 16, 4);
        memcpy(&ptr, img + shoff + 20, 4);
        // .reloc sits past PPC_IMAGE_SIZE and is not needed for static
        // recompilation; copying it would clobber the dispatch LUT.
        if (strcmp(name, ".reloc") != 0 &&
            (size_t)PPC_IMAGE_BASE + va + vs <= GUEST_SIZE &&
            ptr + srd <= (uint32_t)fsz && srd > 0) {
            size_t gva = (size_t)PPC_IMAGE_BASE + va;
            memcpy(g_base + gva, img + ptr, srd < vs ? srd : vs);
        }
        shoff += 40;
    }
    free(img);

    // Populate the perfect-hash dispatch table consumed by PPC_LOOKUP_FUNC:
    //   *(PPCFunc**)(base + PPC_IMAGE_BASE + PPC_IMAGE_SIZE + (va-CODE_BASE)*2)
    // Indirect calls in the generated code read this; without it every
    // virtual call dereferences NULL (the first crash at __imp__sub_82BFFCF0).
    {
        uint8_t* lut = g_base + PPC_IMAGE_BASE + PPC_IMAGE_SIZE;
        size_t mapped = 0;
        for (PPCFuncMapping* m = PPCFuncMappings; m->guest != 0; ++m) {
            if (m->guest < PPC_CODE_BASE ||
                m->guest >= PPC_CODE_BASE + PPC_CODE_SIZE) continue;
            size_t off = (size_t)(m->guest - PPC_CODE_BASE) * 2;
            memcpy(lut + off, &m->host, sizeof(void*));
            mapped++;
        }
        fprintf(stderr, "LUT at +0x%llx entries=%zu\n",
                (unsigned long long)(PPC_IMAGE_BASE + PPC_IMAGE_SIZE), mapped);
    }

    fh2_kernel_init(g_base);

    signal(SIGSEGV, on_fatal);
    signal(SIGTRAP, on_fatal);
    signal(SIGILL, on_fatal);
    signal(SIGABRT, on_fatal);
    alarm(120); // watchdog (SIGALRM default kills; topological spins die here)

    static PPCContext ctx{};
    memset(&ctx, 0, sizeof(ctx));
    ctx.r1.u64 = 0x9FFFFFF0ULL;   // stack top (high end, outside image)
    ctx.r13.u64 = 0x9F000000ULL;  // TLS-ish scratch
    ctx.r2.u64 = 0;

    fprintf(stderr, "BOOT _xstart guest=%p size=0x%zx r1=0x%llx\n",
            (void*)g_base, GUEST_SIZE, (unsigned long long)ctx.r1.u64);
    _xstart(ctx, g_base);
    fprintf(stderr, "RETURNED from _xstart (r3=0x%llx) stubs=%llu\n",
            (unsigned long long)ctx.r3.u64, (unsigned long long)g_stub_calls.load());
    return 0;
}

static void fh2_missing_target(PPCContext&, uint8_t*) {}

extern "C" void* fh2_lookup_guest(uint8_t* base, uint32_t guest) {
    static std::atomic<uint64_t> misses{0};
    uint8_t* lut = base + PPC_IMAGE_BASE + PPC_IMAGE_SIZE;
    if (guest >= PPC_CODE_BASE && guest < PPC_CODE_BASE + PPC_CODE_SIZE) {
        void* fn = nullptr;
        memcpy(&fn, lut + (size_t)(guest - PPC_CODE_BASE) * 2, sizeof(void*));
        if (fn) return fn;
    }
    uint64_t n = ++misses;
    if (n < 60) fprintf(stderr, "[indirect-miss %llu] guest=0x%08X\n",
                        (unsigned long long)n, guest);
    return (void*)&fh2_missing_target;
}

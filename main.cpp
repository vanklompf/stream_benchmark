#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <benchmark/benchmark.h>
#include <sys/resource.h>

#define N  (1024ll * 1024 * 1024 * 4/ sizeof(uint32_t))
const uint8_t* src = nullptr;
uint8_t* dst = nullptr;

static void rgba_basic(benchmark::State& state) {
    for (auto _ : state) {
        for(uint64_t i=0; i<N; i+=4) {
            dst[i+0] = src[(i*4)+0];
            dst[i+1] = src[(i*4)+1];
            dst[i+2] = src[(i*4)+2];
            dst[i+3] = src[(i*4)+3];
        }
    }
    state.SetBytesProcessed(state.iterations() * N*sizeof(uint32_t));
}

static void DoSetup(const benchmark::State& state) {
    setpriority(PRIO_PROCESS, 0, 90);

    // allocate src
    int inFd = open("/mnt/ramdisk/stream_in.dat", O_RDONLY);
    src = (const uint8_t*)mmap(NULL, N*sizeof(uint32_t), PROT_READ, MAP_PRIVATE | MAP_NORESERVE, inFd, 0);

    // allocate dst
    int outFd = open("/mnt/ramdisk/stream_out.dat", O_RDWR);
    ftruncate(outFd, N);
    dst = (uint8_t*)mmap(NULL, N, PROT_WRITE | PROT_READ, MAP_SHARED, outFd, 0);
}

BENCHMARK(rgba_basic)->Setup(DoSetup)->MinTime(10)->Unit(benchmark::kMillisecond);
BENCHMARK_MAIN();
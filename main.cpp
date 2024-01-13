#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <benchmark/benchmark.h>
#include <sys/resource.h>
#include <x86intrin.h>

#define BYTES_SRC (1024ll * 1024 * 1024 * 4)
#define BYTES_DST (BYTES_SRC / 4)

#define UNITS_SRC (BYTES_SRC / sizeof(uint32_t))
#define UNITS_DST (BYTES_DST / sizeof(uint32_t))
#define UNITS_PAGE (4094  / sizeof(uint32_t))

const uint8_t* src = nullptr;
uint8_t* dst = nullptr;

static void rgba_basic(benchmark::State& state) {
    auto _src = (const uint32_t*)src;
    auto _dst = (uint32_t*)dst;
    for (auto _ : state) {
        for(uint64_t i=0; i<UNITS_DST; i+=4) {
            _dst[i+0] = _src[(i*4)+0];
            _dst[i+1] = _src[(i*4)+1];
            _dst[i+2] = _src[(i*4)+2];
            _dst[i+3] = _src[(i*4)+3];
        }
    }
    state.SetBytesProcessed(state.iterations() * BYTES_SRC);
}

static void rgba_avx_unaligned(benchmark::State& state) {
    for (auto _ : state) {
        __m128i v;
        for(uint64_t i=0; i<BYTES_SRC; i+=64) {
            v = _mm_loadu_si128((__m128i*)(src+i));
            _mm_storeu_si128((__m128i*)(dst+(i/4)), v);
        }
    }
    state.SetBytesProcessed(state.iterations() * BYTES_SRC);
}

static void rgba_avx_aligned(benchmark::State& state) {
    for (auto _ : state) {
        for(uint64_t i=0; i<BYTES_SRC; i+=64) {
            _mm_store_si128((__m128i*)(dst+(i/4)), _mm_load_si128((__m128i*)(src+i)));
        }
    }
    state.SetBytesProcessed(state.iterations() * BYTES_SRC);
}

static void rgba_avx_stream_load(benchmark::State& state) {
    for (auto _ : state) {
        for(uint64_t i=0; i<BYTES_SRC; i+=64) {
            _mm_store_si128((__m128i*)(dst+(i/4)), _mm_stream_load_si128((__m128i*)(src+i)));
        }
    }
    state.SetBytesProcessed(state.iterations() * BYTES_SRC);
}

template<int R>
static void rgba_avx_stream_storeX(benchmark::State& state) {
    for (auto _ : state) {
        for(uint64_t k=0; k<BYTES_SRC-4096; k+=(R*4096)) {
            for(unsigned i=k; i<(k+4096); i+=64) {
                for(int r=0; r<R; r++)
                    _mm_stream_si128((__m128i*)(dst+(((r*4096)+i)/4)), _mm_load_si128((__m128i*)(src+(r*4096)+i)));
            }
        }
    }
    state.SetBytesProcessed(state.iterations() * BYTES_SRC);
}



static void DoSetup(const benchmark::State& state) {
    setpriority(PRIO_PROCESS, 0, PRIO_MAX-2);

    // allocate src
    int inFd = open("/mnt/ramdisk/stream_in.dat", O_RDONLY);
    src = (const uint8_t*)mmap(NULL, BYTES_SRC, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, inFd, 0);

    // allocate dst
    int outFd = open("/mnt/ramdisk/stream_out.dat", O_RDWR);
    ftruncate(outFd, BYTES_DST);
    dst = (uint8_t*)mmap(NULL, BYTES_DST, PROT_WRITE | PROT_READ, MAP_SHARED, outFd, 0);
}

BENCHMARK(rgba_basic)->Setup(DoSetup)->MinTime(5)->Unit(benchmark::kMillisecond);
//BENCHMARK(rgba_avx_unaligned)->MinTime(5)->Unit(benchmark::kMillisecond);
//BENCHMARK(rgba_avx_aligned)->MinTime(5)->Unit(benchmark::kMillisecond);
//BENCHMARK(rgba_avx_stream_load)->MinTime(5)->Unit(benchmark::kMillisecond);
//BENCHMARK(rgba_avx_stream_store)->MinTime(5)->Unit(benchmark::kMillisecond);
BENCHMARK(rgba_avx_stream_storeX<1>)->MinTime(5)->Unit(benchmark::kMillisecond);
BENCHMARK(rgba_avx_stream_storeX<2>)->MinTime(5)->Unit(benchmark::kMillisecond);
BENCHMARK(rgba_avx_stream_storeX<3>)->MinTime(5)->Unit(benchmark::kMillisecond);
BENCHMARK(rgba_avx_stream_storeX<4>)->MinTime(5)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
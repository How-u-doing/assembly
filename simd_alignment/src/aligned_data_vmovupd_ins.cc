#include "xsimd/xsimd.hpp"

#include <benchmark/benchmark.h>
#include <cstdio>

constexpr size_t size = 1'000'000;

#define SMALL_SHIFT 32
struct alignas(64) {
    char unused[SMALL_SHIFT];
    char x[sizeof(double) * size];
    char z[sizeof(double) * size];
} unaligned_buffer;

#define ALIGNMENT 64
struct {
    alignas(ALIGNMENT) double x[size];
    alignas(ALIGNMENT) double z[size];
} aligned_buffer;

// If we put them in the function body, the compiler is smart enough to optimize
// load/store_unaligned (vmovupd) to load/store_aligned (vmovapd) when the address
// is known to the compiler to be aligned to a simd vector size.
double *x_aligned = aligned_buffer.x;
double *z_aligned = aligned_buffer.z;

double *x_unaligned = (double *)unaligned_buffer.x;
double *z_unaligned = (double *)unaligned_buffer.z;

#define BM_UNARY_OP_XSIMD(fn, aligned)                                             \
    static void BM_xsimd_##fn##_##aligned(benchmark::State &state) {               \
        static bool printed = false;                                               \
        double *x = x_##aligned;                                                   \
        double *z = z_##aligned;                                                   \
        if (!printed) {                                                            \
            printf(#fn "_" #aligned " x addr: %p, z addr: %p\n", x, z);            \
            printed = true;                                                        \
        }                                                                          \
        for (size_t i = 0; i < size; ++i) {                                        \
            x[i] = double(0.5) + std::sqrt(double(i)) * double(9.) / double(size); \
        }                                                                          \
        using B = xsimd::batch<double>;                                            \
        constexpr size_t simd_size = xsimd::batch<double>::size;                   \
        size_t vec_size = size - size % simd_size;                                 \
        for (auto _ : state) {                                                     \
            for (size_t i = 0; i < vec_size; i += simd_size) {                     \
                B bx = B::load_##aligned(&x[i]);                                   \
                B bz = xsimd::fn(bx);                                              \
                bz.store_##aligned(&z[i]);                                         \
            }                                                                      \
        }                                                                          \
    }                                                                              \
    BENCHMARK(BM_xsimd_##fn##_##aligned);

#define BM_UNARY_OP_FNS(fn)        \
    BM_UNARY_OP_XSIMD(fn, aligned) \
    BM_UNARY_OP_XSIMD(fn, unaligned)

BM_UNARY_OP_FNS(abs)

BENCHMARK_MAIN();

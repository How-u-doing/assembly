#include "xsimd/xsimd.hpp"

#include <benchmark/benchmark.h>
#include <cstdio>

constexpr size_t size = 1'000'000;

template <class T>
using bench_vector_aligned = std::vector<T, xsimd::aligned_allocator<T>>;

template <class T>
using bench_vector_unaligned = std::vector<T>;

#ifndef SMALL_SHIFT
#define SMALL_SHIFT 1
#endif
struct alignas(64) {
    char unused[SMALL_SHIFT];
    char x[sizeof(double) * size];
    char y[sizeof(double) * size];
    char z[sizeof(double) * size];
} unaligned_buffer;

#define ALIGNMENT 64
struct {
    alignas(ALIGNMENT) double x[size];
    alignas(ALIGNMENT) double y[size];
    alignas(ALIGNMENT) double z[size];
} aligned_buffer;

void set_x_unaligned(double *&x) { x = (double *)unaligned_buffer.x; }
void set_y_unaligned(double *&y) { y = (double *)unaligned_buffer.y; }
void set_z_unaligned(double *&z) { z = (double *)unaligned_buffer.z; }

void set_x_aligned(double *&x) { x = aligned_buffer.x; }
void set_y_aligned(double *&y) { y = aligned_buffer.y; }
void set_z_aligned(double *&z) { z = aligned_buffer.z; }

#define BM_UNARY_OP_STD(fn, aligned)                                               \
    static void BM_std_##fn##_##aligned(benchmark::State &state) {                 \
        /* bench_vector_##aligned<double> x(size), z(size); */                     \
        double *x, *z;                                                             \
        set_x_##aligned(x);                                                        \
        set_z_##aligned(z);                                                        \
        for (size_t i = 0; i < size; ++i) {                                        \
            x[i] = double(0.5) + std::sqrt(double(i)) * double(9.) / double(size); \
        }                                                                          \
        for (auto _ : state) {                                                     \
            for (size_t i = 0; i < size; ++i) {                                    \
                z[i] = std::fn(x[i]);                                              \
            }                                                                      \
        }                                                                          \
    }                                                                              \
    BENCHMARK(BM_std_##fn##_##aligned);

#define BM_UNARY_OP_XSIMD(fn, aligned)                                             \
    static void BM_xsimd_##fn##_##aligned(benchmark::State &state) {               \
        static bool printed = false;                                               \
        /* bench_vector_##aligned<double> x(size), z(size); */                     \
        double *x, *z;                                                             \
        set_x_##aligned(x);                                                        \
        set_z_##aligned(z);                                                        \
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

#define BM_BINARY_OP_STD(fn, aligned)                                              \
    static void BM_std_##fn##_##aligned(benchmark::State &state) {                 \
        /* bench_vector_##aligned<double> x(size), y(size), z(size); */            \
        double *x, *y, *z;                                                         \
        set_x_##aligned(x);                                                        \
        set_y_##aligned(y);                                                        \
        set_z_##aligned(z);                                                        \
        for (size_t i = 0; i < size; ++i) {                                        \
            x[i] = double(0.5) + std::sqrt(double(i)) * double(9.) / double(size); \
            y[i] = double(10.2) / double(i + 2) + double(0.25);                    \
        }                                                                          \
        for (auto _ : state) {                                                     \
            for (size_t i = 0; i < size; ++i) {                                    \
                z[i] = std::fn(x[i], y[i]);                                        \
            }                                                                      \
        }                                                                          \
    }                                                                              \
    BENCHMARK(BM_std_##fn##_##aligned);

#define BM_BINARY_OP_XSIMD(fn, aligned)                                                \
    static void BM_xsimd_##fn##_##aligned(benchmark::State &state) {                   \
        static bool printed = false;                                                   \
        /* bench_vector_##aligned<double> x(size), y(size), z(size); */                \
        double *x, *y, *z;                                                             \
        set_x_##aligned(x);                                                            \
        set_y_##aligned(y);                                                            \
        set_z_##aligned(z);                                                            \
        if (!printed) {                                                                \
            printf(#fn "_" #aligned " x addr: %p, y addr: %p, z addr: %p\n", x, y, z); \
            printed = true;                                                            \
        }                                                                              \
        for (size_t i = 0; i < size; ++i) {                                            \
            x[i] = double(0.5) + std::sqrt(double(i)) * double(9.) / double(size);     \
            y[i] = double(10.2) / double(i + 2) + double(0.25);                        \
        }                                                                              \
        using B = xsimd::batch<double>;                                                \
        constexpr size_t simd_size = xsimd::batch<double>::size;                       \
        size_t vec_size = size - size % simd_size;                                     \
        for (auto _ : state) {                                                         \
            for (size_t i = 0; i < vec_size; i += simd_size) {                         \
                B bx = B::load_##aligned(&x[i]), by = B::load_##aligned(&y[i]);        \
                B bz = xsimd::fn(x[i], y[i]);                                          \
                bz.store_##aligned(&z[i]);                                             \
            }                                                                          \
        }                                                                              \
    }                                                                                  \
    BENCHMARK(BM_xsimd_##fn##_##aligned);

#define BM_UNARY_OP_FNS(fn)          \
    BM_UNARY_OP_XSIMD(fn, aligned)   \
    BM_UNARY_OP_XSIMD(fn, unaligned) \
    BM_UNARY_OP_STD(fn, aligned)     \
    BM_UNARY_OP_STD(fn, unaligned)

#define BM_BINARY_OP_FNS(fn)          \
    BM_BINARY_OP_XSIMD(fn, aligned)   \
    BM_BINARY_OP_XSIMD(fn, unaligned) \
    BM_BINARY_OP_STD(fn, aligned)     \
    BM_BINARY_OP_STD(fn, unaligned)

// BM_UNARY_OP_FNS(sqrt)
// BM_UNARY_OP_FNS(log)
// BM_UNARY_OP_FNS(log1p)
// BM_UNARY_OP_FNS(exp)
// BM_UNARY_OP_FNS(round)
// BM_UNARY_OP_FNS(floor)
BM_UNARY_OP_FNS(ceil)
BM_UNARY_OP_FNS(abs)
// BM_UNARY_OP_FNS(signbit)
// BM_UNARY_OP_FNS(sin)
// BM_UNARY_OP_FNS(cos)
// BM_UNARY_OP_FNS(sinh)
// BM_UNARY_OP_FNS(cosh)

// BM_BINARY_OP_FNS(pow)
// BM_BINARY_OP_FNS(max)
// BM_BINARY_OP_FNS(min)
// BM_BINARY_OP_FNS(hypot)
// BM_BINARY_OP_FNS(fmod)
// BM_BINARY_OP_FNS(remainder)

BENCHMARK_MAIN();

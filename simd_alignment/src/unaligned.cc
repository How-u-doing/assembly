#include "xsimd/xsimd.hpp"
#include <cstdio>

constexpr int N = 10;

#define SMALL_SHIFT 1
struct {
    char unused[SMALL_SHIFT];
    char x[sizeof(double) * N];
    char y[sizeof(double) * N];
} unaligned_buffer;

using batch_type = xsimd::batch<double>;

void sin_xsimd(const double *x, double *y, size_t size) {
    constexpr size_t simd_size = batch_type::size;
    printf("simd_size: %lu\n", simd_size);
    size_t vec_size = size - size % simd_size;
    for (size_t i = 0; i < vec_size; i += simd_size) {
        batch_type x_batch = batch_type::load_aligned(&x[i]);  // should crash here
        batch_type y_batch = xsimd::sin(x_batch);
        y_batch.store_unaligned(&y[i]);
    }
    for (size_t i = vec_size; i < size; ++i) {
        y[i] = xsimd::sin(x[i]);
    }
}

int main() {
    double *x, *y;
    x = (double *)unaligned_buffer.x;
    y = (double *)unaligned_buffer.y;
    printf("x addr: %p, y addr: %p\n", x, y);
    for (size_t i = 0; i < N; ++i)
        x[i] = i;

    sin_xsimd(x, y, N);

    for (int i = 0; i < N; ++i)
        printf("%f ", y[i]);

    printf("\n");
    for (int i = 0; i < N; ++i)
        printf("%f ", std::sin(x[i]));
    return 0;
}

#include <cstdio>
#include <cstdlib>
#include <ctime>

constexpr int array_size = 100'000'000;
constexpr int unroll_factor = 4;

int sum_simple(int arr[], int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

int sum_unrolling(int arr[], int size) {
    int sum = 0;
    for (int i = 0; i < size; i += unroll_factor) {
        sum += arr[i];
        sum += arr[i + 1];
        sum += arr[i + 2];
        sum += arr[i + 3];
    }
    return sum;
}

// use local sums to apply instruction-level parallelism (ILP)
// so these 4 increments can run concurrently ;)
int sum_unrolling_local_array(int arr[], int size) {
    int sum[unroll_factor] = {0};
    for (int i = 0; i < size; i += unroll_factor) {
        sum[0] += arr[i];
        sum[1] += arr[i + 1];
        sum[2] += arr[i + 2];
        sum[3] += arr[i + 3];
    }
    return sum[0] + sum[1] + sum[2] + sum[3];
}

int arr[array_size];

int main() {
    int i, sum;
    clock_t start, end;
    double time_taken;

    // Initialize array with random values
    for (i = 0; i < array_size; i++) {
        arr[i] = rand() % 100;
        if (arr[i] % 2) {
            arr[i] *= -1;
        }
    }

    start = clock();
    sum = sum_simple(arr, array_size);
    end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("sum_simple: %d\n", sum);
    printf("time taken: %f seconds\n", time_taken);

    start = clock();
    sum = sum_unrolling(arr, array_size);
    end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("sum_unrolling: %d\n", sum);
    printf("time taken: %f seconds\n", time_taken);

    start = clock();
    sum = sum_unrolling_local_array(arr, array_size);
    end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("sum_unrolling_local_array: %d\n", sum);
    printf("time taken: %f seconds\n", time_taken);

    return 0;
}

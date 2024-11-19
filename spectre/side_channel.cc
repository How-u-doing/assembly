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

constexpr std::string_view secret = "It's a s3kr3t!";

char leak_byte(std::string_view text, int index) {
    constexpr int stride = 256; // 128
    static uint8_t timing_array[256 * stride];
    memset(timing_array, 0, sizeof timing_array);

    const char *data = text.begin();

    uint64_t latencies[256]{};
    int scores[256]{};
    int best = 0, runner_up = 0;

    for (int run = 0; run < 1000; run++) {
        for (int i = 0; i < 256; i++)
            clflush(&timing_array[i * stride]);

        for (int i = 0; i < 100; i++)
            force_read(&timing_array[data[index] * stride]);

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
            if (latencies[i] < avg_latency * 3 / 4)
                scores[i]++;

        std::tie(best, runner_up) = top_two_indices(scores, 256);

        if (scores[best] > (2 * scores[runner_up] + 400))
            break;
    }

    printf("best: '%c', score=%d, runner_up='%c', score=%d\n",
            best, scores[best], runner_up, scores[runner_up]);
    return best;
}

int main() {
    for (size_t i = 0; i < secret.size(); i++)
        leak_byte(secret, i);

    return 0;
}

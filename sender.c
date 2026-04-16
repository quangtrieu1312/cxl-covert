#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <string.h>

#define CHUNK_SIZE (1024 * 1024)   // 1 MB per contention burst
#define CACHELINE 64
int bit_window_us;

static inline uint64_t rdtscp() {
    unsigned int lo, hi;
    asm volatile("rdtscp" : "=a"(lo), "=d"(hi) :: "rcx");
    return ((uint64_t)hi << 32) | lo;
}

static inline uint64_t now_us() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000ull + t.tv_nsec / 1000ull;
}

static inline uint64_t next_minute_us() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)(t.tv_sec+(60-(t.tv_sec%60))) * 1000000ull;
}

static inline uint64_t next_second_us() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)(t.tv_sec+1) * 1000000ull;
}

static inline uint64_t next_bit_us() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    uint64_t now = (uint64_t)t.tv_sec * 1000000ull + t.tv_nsec / 1000ull;
    return now + (bit_window_us - (now % bit_window_us));
}

static inline void clflush_line(void *addr) {
    asm volatile("clflush (%0)" :: "r"(addr) : "memory");
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <buffer_mb> <bit_window_us> <message>\n", argv[0]);
        return 1;
    }

    size_t total_mb = (size_t)atoi(argv[1]);
    size_t buf_sz = total_mb * 1024ull * 1024ull;
    bit_window_us = atoi(argv[2]);
    
    // Covert message to send
    const char *bitstream = argv[3];
    size_t nbits = strlen(bitstream);

    unsigned char *buf = mmap(NULL, buf_sz,
                              PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Touch pages
    for (size_t i = 0; i < buf_sz; i += 4096)
        buf[i] = 0;

    printf("Sender PID=%d BUF=%p SIZE=%zu MB\n", getpid(), buf, total_mb);
    fflush(stdout);
    printf("Sending bits repeatedly: %s\n", bitstream);
    fflush(stdout);

    size_t iteration = 0;

    uint64_t start_attack = next_bit_us();
    uint64_t idle_time = 0;
    uint64_t now = now_us();
    if (start_attack > now) {
        idle_time = start_attack - now;
    }
    usleep(idle_time);
    // Infinite loop over bitstream
    while (1) {
        for (size_t b = 0; b < nbits; b++) {
            uint64_t next_bit_window = next_bit_us();
	        printf("next_bit_window = %ld\n", next_bit_window);
            fflush(stdout);
            if (bitstream[b] == '1') {
                // BIT = 1 → create contention
                while (now_us() < next_bit_window) {
                    unsigned char *region =
                        buf + ((iteration * CHUNK_SIZE) % buf_sz);
                    for (size_t off = 0; off < CHUNK_SIZE && now_us() < next_bit_window; off += CACHELINE) {
                        unsigned char *addr = region + off;
				        *(volatile unsigned long long *)addr = 0xdeadbeefdeadbeefull;
                        clflush_line(addr);
                    }
                    asm volatile("sfence" ::: "memory");
                    iteration++;
                }
            } else {
                // BIT = 0 → idle
                idle_time = 0;
                now = now_us();
                if (next_bit_window > now) {
                    idle_time = next_bit_window - now;
                }
                usleep(idle_time);
            }

        }
    }

    munmap(buf, buf_sz);
    return 0;
}


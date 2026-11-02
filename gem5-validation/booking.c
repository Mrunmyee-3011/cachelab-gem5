/*
 * booking.c - shared-counter workload for the GEM5 validation of CacheLab.
 *
 * Every thread performs ITERS atomic increments.
 *   mode 0 "shared": all threads increment ONE counter (the "seats left" counter) -> true sharing,
 *                    the cache line ping-pongs between cores (invalidations, cache-to-cache transfers).
 *   mode 1 "padded": each thread increments its OWN counter, each on its own 64-byte line -> no sharing.
 *
 * Usage: booking <threads> <iters> <mode>
 *
 * GEM5 SE mode does not time-slice, so run with exactly as many cores as threads. Thread 0 is the main thread
 * (it works too), so <threads> hardware contexts are enough.
 *
 * Build (static, required by GEM5 SE mode):  gcc -O2 -static -pthread -o booking booking.c
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_THREADS 16
#define LINE 64

typedef struct {
    volatile long v;
    char pad[LINE - sizeof(long)];
} __attribute__((aligned(LINE))) line_t;

static line_t shared_ctr;
static line_t padded_ctr[MAX_THREADS];

static int g_iters, g_mode;

static void *worker(void *arg) {
    long id = (long)arg;
    volatile long *c = (g_mode == 0) ? &shared_ctr.v : &padded_ctr[id].v;
    for (int i = 0; i < g_iters; i++)
        __atomic_fetch_add(c, 1, __ATOMIC_SEQ_CST);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <threads 1..%d> <iters> <mode 0=shared 1=padded>\n", argv[0], MAX_THREADS);
        return 2;
    }
    int n = atoi(argv[1]);
    g_iters = atoi(argv[2]);
    g_mode = atoi(argv[3]);
    if (n < 1 || n > MAX_THREADS || g_iters < 1 || (g_mode != 0 && g_mode != 1)) {
        fprintf(stderr, "bad arguments\n");
        return 2;
    }

    pthread_t t[MAX_THREADS];
    for (long i = 1; i < n; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    worker((void *)0);
    for (int i = 1; i < n; i++) pthread_join(t[i], NULL);

    long total = 0;
    if (g_mode == 0) total = shared_ctr.v;
    else for (int i = 0; i < n; i++) total += padded_ctr[i].v;

    long expected = (long)n * g_iters;
    printf("threads=%d iters=%d mode=%s total=%ld expected=%ld %s\n", n, g_iters,
           g_mode == 0 ? "shared" : "padded", total, expected, total == expected ? "OK" : "MISMATCH");
    return total == expected ? 0 : 1;
}

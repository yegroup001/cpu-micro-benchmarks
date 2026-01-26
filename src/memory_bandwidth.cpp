// Memory bandwidth microbenchmark.
// Measures sequential bandwidth for: read, write, copy, triad (STREAM-like).

#include "include/utils.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static FILE *fp;
static volatile uint64_t sink;

static void *aligned_alloc_or_null(size_t alignment, size_t bytes) {
  void *p = NULL;
  if (posix_memalign(&p, alignment, bytes) != 0) {
    return NULL;
  }
  return p;
}

static void init_u64(uint64_t *p, size_t n, uint64_t seed) {
  for (size_t i = 0; i < n; i++) {
    seed = seed * 6364136223846793005ULL + 1;
    p[i] = seed;
  }
}

static void kernel_read(const uint64_t *__restrict a, size_t n) {
  // Use multiple accumulators to reduce loop-carried dependency chains.
  // This makes the kernel more representative of sequential load bandwidth.
  uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
  size_t i = 0;
  for (; i + 4 <= n; i += 4) {
    s0 += a[i + 0];
    s1 += a[i + 1];
    s2 += a[i + 2];
    s3 += a[i + 3];
  }
  for (; i < n; i++) {
    s0 += a[i];
  }
  sink += (s0 + s1 + s2 + s3);
}

static void kernel_write(uint64_t *__restrict a, size_t n) {
  // Fill kernel (STREAM-like). Mix in sink so stores aren't a fixed pattern.
  const uint64_t base = sink;
  for (size_t i = 0; i < n; i++) {
    a[i] = base + (uint64_t)i;
  }
  sink += a[n / 2];
}

static void kernel_copy(const uint64_t *__restrict src,
                        uint64_t *__restrict dst, size_t n) {
  // Keep the implementation loop-based (like triad) so results are comparable
  // across ops and not dominated by libc's tuned memcpy.
  size_t i = 0;
  for (; i + 4 <= n; i += 4) {
    dst[i + 0] = src[i + 0];
    dst[i + 1] = src[i + 1];
    dst[i + 2] = src[i + 2];
    dst[i + 3] = src[i + 3];
  }
  for (; i < n; i++) {
    dst[i] = src[i];
  }
  sink += dst[n / 2];
}

static void kernel_triad(uint64_t *__restrict a, const uint64_t *__restrict b,
                         const uint64_t *__restrict c, size_t n) {
  const uint64_t scalar = 3;
  for (size_t i = 0; i < n; i++) {
    a[i] = b[i] + scalar * c[i];
  }
  sink += a[n / 3];
}

struct result {
  double seconds;
  uint64_t cycles;
  uint64_t instructions;
};

static result time_loop(void (*fn)(void *ctx), void *ctx, int warmup,
                        int iterations, bool perf) {
  for (int i = 0; i < warmup; i++) {
    fn(ctx);
  }

  uint64_t cycles_before = 0, instructions_before = 0;
  uint64_t before = get_time();
#ifdef __linux__
  if (perf) {
    cycles_before = perf_read_cycles();
    instructions_before = perf_read_instructions();
  }
#endif

  for (int i = 0; i < iterations; i++) {
    fn(ctx);
  }

  uint64_t cycles_after = 0, instructions_after = 0;
  uint64_t after = get_time();
#ifdef __linux__
  if (perf) {
    cycles_after = perf_read_cycles();
    instructions_after = perf_read_instructions();
  }
#endif

  result r;
  r.seconds = (double)(after - before) / 1e9;
  r.cycles = cycles_after - cycles_before;
  r.instructions = instructions_after - instructions_before;
  return r;
}

static result time_best_of(void (*fn)(void *ctx), void *ctx, int warmup,
                           int iterations, int repeats, bool perf) {
  if (repeats < 1) {
    repeats = 1;
  }

  // Warmup once so it doesn't skew the repeats.
  for (int i = 0; i < warmup; i++) {
    fn(ctx);
  }

  result best = {};
  best.seconds = 1e300;
  for (int r = 0; r < repeats; r++) {
    result cur = time_loop(fn, ctx, 0, iterations, perf);
    if (cur.seconds < best.seconds) {
      best = cur;
    }
  }
  return best;
}

struct read_ctx {
  const uint64_t *a;
  size_t n;
};
static void run_read(void *ctx) {
  read_ctx *c = (read_ctx *)ctx;
  kernel_read(c->a, c->n);
}

struct write_ctx {
  uint64_t *a;
  size_t n;
};
static void run_write(void *ctx) {
  write_ctx *c = (write_ctx *)ctx;
  kernel_write(c->a, c->n);
}

struct copy_ctx {
  const uint64_t *src;
  uint64_t *dst;
  size_t n;
};
static void run_copy(void *ctx) {
  copy_ctx *c = (copy_ctx *)ctx;
  kernel_copy(c->src, c->dst, c->n);
}

struct triad_ctx {
  uint64_t *a;
  const uint64_t *b;
  const uint64_t *c;
  size_t n;
};
static void run_triad(void *ctx) {
  triad_ctx *c = (triad_ctx *)ctx;
  kernel_triad(c->a, c->b, c->c, c->n);
}

static int iterations_for_target_bytes(uint64_t target_bytes,
                                       uint64_t bytes_per_iter) {
  if (bytes_per_iter == 0) {
    return 1;
  }
  uint64_t iters = target_bytes / bytes_per_iter;
  if (iters == 0) {
    iters = 1;
  }
  if (iters > (uint64_t)INT32_MAX) {
    iters = INT32_MAX;
  }
  return (int)iters;
}

static void test_size(size_t bytes, int warmup, int iterations_override,
                      uint64_t target_bytes, int repeats, bool perf) {
  if (bytes < sizeof(uint64_t)) {
    bytes = sizeof(uint64_t);
  }
  size_t n = bytes / sizeof(uint64_t);
  bytes = n * sizeof(uint64_t);

  uint64_t *a = (uint64_t *)aligned_alloc_or_null(64, bytes);
  uint64_t *b = (uint64_t *)aligned_alloc_or_null(64, bytes);
  uint64_t *c = (uint64_t *)aligned_alloc_or_null(64, bytes);
  if (!a || !b || !c) {
    free(a);
    free(b);
    free(c);
    return;
  }

  init_u64(a, n, 1);
  init_u64(b, n, 2);
  init_u64(c, n, 3);

  // read
  {
    read_ctx ctx{a, n};
    uint64_t bytes_per_iter = bytes;
    int iters = iterations_override > 0
                    ? iterations_override
                    : iterations_for_target_bytes(target_bytes, bytes_per_iter);
    result r = time_best_of(run_read, &ctx, warmup, iters, repeats, perf);
    double gbps = (double)(bytes_per_iter) * iters / r.seconds / 1e9;
#ifdef __linux__
    if (perf) {
      fprintf(fp, "%zu,read,%d,%llu,%.0f,%.3f,%llu,%llu\n", bytes, iters,
              (unsigned long long)bytes_per_iter * (unsigned long long)iters,
              r.seconds * 1e9, gbps, (unsigned long long)r.cycles,
              (unsigned long long)r.instructions);
    } else {
      fprintf(fp, "%zu,read,%d,%llu,%.0f,%.3f\n", bytes, iters,
              (unsigned long long)bytes_per_iter * (unsigned long long)iters,
              r.seconds * 1e9, gbps);
    }
#else
    fprintf(fp, "%zu,read,%d,%llu,%.0f,%.3f\n", bytes, iters,
            (unsigned long long)bytes_per_iter * (unsigned long long)iters,
            r.seconds * 1e9, gbps);
#endif
  }

  // write
  {
    write_ctx ctx{a, n};
    uint64_t bytes_per_iter = bytes;
    int iters = iterations_override > 0
                    ? iterations_override
                    : iterations_for_target_bytes(target_bytes, bytes_per_iter);
    result r = time_best_of(run_write, &ctx, warmup, iters, repeats, perf);
    double gbps = (double)(bytes_per_iter) * iters / r.seconds / 1e9;
#ifdef __linux__
    if (perf) {
      fprintf(fp, "%zu,write,%d,%llu,%.0f,%.3f,%llu,%llu\n", bytes, iters,
              (unsigned long long)bytes_per_iter * (unsigned long long)iters,
              r.seconds * 1e9, gbps, (unsigned long long)r.cycles,
              (unsigned long long)r.instructions);
    } else {
      fprintf(fp, "%zu,write,%d,%llu,%.0f,%.3f\n", bytes, iters,
              (unsigned long long)bytes_per_iter * (unsigned long long)iters,
              r.seconds * 1e9, gbps);
    }
#else
    fprintf(fp, "%zu,write,%d,%llu,%.0f,%.3f\n", bytes, iters,
            (unsigned long long)bytes_per_iter * (unsigned long long)iters,
            r.seconds * 1e9, gbps);
#endif
  }

  // copy (read + write)
  {
    copy_ctx ctx{a, b, n};
    uint64_t bytes_per_iter = 2ULL * bytes;
    int iters = iterations_override > 0
                    ? iterations_override
                    : iterations_for_target_bytes(target_bytes, bytes_per_iter);
    result r = time_best_of(run_copy, &ctx, warmup, iters, repeats, perf);
    double gbps = (double)(bytes_per_iter) * iters / r.seconds / 1e9;
#ifdef __linux__
    if (perf) {
      fprintf(fp, "%zu,copy,%d,%llu,%.0f,%.3f,%llu,%llu\n", bytes, iters,
              (unsigned long long)bytes_per_iter * (unsigned long long)iters,
              r.seconds * 1e9, gbps, (unsigned long long)r.cycles,
              (unsigned long long)r.instructions);
    } else {
      fprintf(fp, "%zu,copy,%d,%llu,%.0f,%.3f\n", bytes, iters,
              (unsigned long long)bytes_per_iter * (unsigned long long)iters,
              r.seconds * 1e9, gbps);
    }
#else
    fprintf(fp, "%zu,copy,%d,%llu,%.0f,%.3f\n", bytes, iters,
            (unsigned long long)bytes_per_iter * (unsigned long long)iters,
            r.seconds * 1e9, gbps);
#endif
  }

  // triad (2 reads + 1 write)
  {
    triad_ctx ctx{a, b, c, n};
    uint64_t bytes_per_iter = 3ULL * bytes;
    int iters = iterations_override > 0
                    ? iterations_override
                    : iterations_for_target_bytes(target_bytes, bytes_per_iter);
    result r = time_best_of(run_triad, &ctx, warmup, iters, repeats, perf);
    double gbps = (double)(bytes_per_iter) * iters / r.seconds / 1e9;
#ifdef __linux__
    if (perf) {
      fprintf(fp, "%zu,triad,%d,%llu,%.0f,%.3f,%llu,%llu\n", bytes, iters,
              (unsigned long long)bytes_per_iter * (unsigned long long)iters,
              r.seconds * 1e9, gbps, (unsigned long long)r.cycles,
              (unsigned long long)r.instructions);
    } else {
      fprintf(fp, "%zu,triad,%d,%llu,%.0f,%.3f\n", bytes, iters,
              (unsigned long long)bytes_per_iter * (unsigned long long)iters,
              r.seconds * 1e9, gbps);
    }
#else
    fprintf(fp, "%zu,triad,%d,%llu,%.0f,%.3f\n", bytes, iters,
            (unsigned long long)bytes_per_iter * (unsigned long long)iters,
            r.seconds * 1e9, gbps);
#endif
  }

  fflush(fp);

  free(a);
  free(b);
  free(c);
}

int main(int argc, char *argv[]) {
  fp = fopen_outputs_file("memory_bandwidth.csv", "w");
  assert(fp);

  int warmup = 2;
  int iterations = 0; // 0 => auto based on target bytes
  uint64_t target_bytes = 1ULL << 30; // 1GiB moved bytes per op
  int repeats = 5;
  bool perf = false;

  int opt;
  while ((opt = getopt(argc, argv, "w:i:B:r:p")) != -1) {
    switch (opt) {
    case 'w':
      sscanf(optarg, "%d", &warmup);
      break;
    case 'i':
      sscanf(optarg, "%d", &iterations);
      break;
    case 'B': {
      unsigned long long v = 0;
      sscanf(optarg, "%llu", &v);
      target_bytes = (uint64_t)v;
      break;
    }
    case 'r':
      sscanf(optarg, "%d", &repeats);
      break;
    case 'p':
      perf = true;
      break;
    default:
      fprintf(stderr,
              "Usage: %s [-w warmup] [-i iterations] [-B target_bytes] [-r repeats] [-p]\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  bind_to_core();

#ifdef __linux__
  if (perf) {
    setup_perf_cycles();
    setup_perf_instructions();
  }
#endif

  std::map<const char *, size_t> cache_sizes = get_cache_sizes();
  for (auto it : cache_sizes) {
    const char *name = it.first;
    size_t size = it.second;
    if (size != 0) {
      fprintf(fp, "%s cache: %zu bytes\n", name, size);
    }
  }

#ifdef __linux__
  if (perf) {
    fprintf(fp,
            "size,op,iterations,bytes_moved,time(ns),bandwidth(GB/s),cycles,instructions\n");
  } else {
    fprintf(fp,
            "size,op,iterations,bytes_moved,time(ns),bandwidth(GB/s)\n");
  }
#else
  fprintf(fp, "size,op,iterations,bytes_moved,time(ns),bandwidth(GB/s)\n");
#endif

  // Size sweep (bytes). Chosen to cover L1/L2/LLC/DRAM regimes.
  const size_t sizes[] = {
      1024,
      1024 * 2,
      1024 * 4,
      1024 * 8,
      1024 * 16,
      1024 * 32,
      1024 * 64,
      1024 * 128,
      1024 * 256,
      1024 * 512,
      1024 * 1024,
      1024 * 1024 * 2,
      1024 * 1024 * 4,
      1024 * 1024 * 8,
      1024 * 1024 * 16,
      1024 * 1024 * 32,
      1024 * 1024 * 64,
      1024 * 1024 * 128,
      1024 * 1024 * 256,
      1024 * 1024 * 512,
      1024ULL * 1024 * 1024,
  };

  for (size_t sz : sizes) {
    test_size(sz, warmup, iterations, target_bytes, repeats, perf);
  }

  printf("Results are written to memory_bandwidth.csv\n");
  return 0;
}

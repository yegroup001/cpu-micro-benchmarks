#include "include/utils.h"
#include <arm_neon.h>
#include <arm_sve.h>
#include <time.h>
#include <unistd.h>

int res = 0;
const int repeat = 4000;
const int unroll = 16;

#ifdef SVE_FP64_LOAD
void test1(double *indices) {
  svbool_t pg = svptrue_b64();
  for (int i = 0; i < repeat; i++) {
    double *p = indices + (i & 0x7f);
    svfloat64_t v0 = svld1_f64(pg, p);
    svfloat64_t v1 = svld1_f64(pg, p + 1);
    svfloat64_t v2 = svld1_f64(pg, p + 2);
    svfloat64_t v3 = svld1_f64(pg, p + 3);
    svfloat64_t v4 = svld1_f64(pg, p + 4);
    svfloat64_t v5 = svld1_f64(pg, p + 5);
    svfloat64_t v6 = svld1_f64(pg, p + 6);
    svfloat64_t v7 = svld1_f64(pg, p + 7);
    svfloat64_t v8 = svld1_f64(pg, p + 8);
    svfloat64_t v9 = svld1_f64(pg, p + 9);
    svfloat64_t v10 = svld1_f64(pg, p + 10);
    svfloat64_t v11 = svld1_f64(pg, p + 11);
    svfloat64_t v12 = svld1_f64(pg, p + 12);
    svfloat64_t v13 = svld1_f64(pg, p + 13);
    svfloat64_t v14 = svld1_f64(pg, p + 14);
    svfloat64_t v15 = svld1_f64(pg, p + 15);
    res += (int)(svaddv_f64(pg, v0) + svaddv_f64(pg, v1) +
                 svaddv_f64(pg, v2) + svaddv_f64(pg, v3) +
                 svaddv_f64(pg, v4) + svaddv_f64(pg, v5) +
                 svaddv_f64(pg, v6) + svaddv_f64(pg, v7) +
                 svaddv_f64(pg, v8) + svaddv_f64(pg, v9) +
                 svaddv_f64(pg, v10) + svaddv_f64(pg, v11) +
                 svaddv_f64(pg, v12) + svaddv_f64(pg, v13) +
                 svaddv_f64(pg, v14) + svaddv_f64(pg, v15));
  }
}
#endif

#ifdef SVE_FP64_STORE
void test1(double *indices) {
  svbool_t pg = svptrue_b64();
  svfloat64_t v = svld1_f64(pg, indices);
  double tmp[svcntd() * 256] __attribute__((aligned(64)));
  for (int i = 0; i < repeat; i++) {
    double *p = tmp + (i & 0x7f) * svcntd();
    svst1_f64(pg, p, v);
    svst1_f64(pg, p + 1 * svcntd(), v);
    svst1_f64(pg, p + 2 * svcntd(), v);
    svst1_f64(pg, p + 3 * svcntd(), v);
    svst1_f64(pg, p + 4 * svcntd(), v);
    svst1_f64(pg, p + 5 * svcntd(), v);
    svst1_f64(pg, p + 6 * svcntd(), v);
    svst1_f64(pg, p + 7 * svcntd(), v);
    svst1_f64(pg, p + 8 * svcntd(), v);
    svst1_f64(pg, p + 9 * svcntd(), v);
    svst1_f64(pg, p + 10 * svcntd(), v);
    svst1_f64(pg, p + 11 * svcntd(), v);
    svst1_f64(pg, p + 12 * svcntd(), v);
    svst1_f64(pg, p + 13 * svcntd(), v);
    svst1_f64(pg, p + 14 * svcntd(), v);
    svst1_f64(pg, p + 15 * svcntd(), v);
  }
  res += tmp[0];
}
#endif

#ifdef NEON_FP64_LOAD
void test1(double *indices) {
  for (int i = 0; i < repeat; i++) {
    double *p = indices + (i & 0x7f) * 2;
    float64x2_t v0 = vld1q_f64(p);
    float64x2_t v1 = vld1q_f64(p + 2);
    float64x2_t v2 = vld1q_f64(p + 4);
    float64x2_t v3 = vld1q_f64(p + 6);
    float64x2_t v4 = vld1q_f64(p + 8);
    float64x2_t v5 = vld1q_f64(p + 10);
    float64x2_t v6 = vld1q_f64(p + 12);
    float64x2_t v7 = vld1q_f64(p + 14);
    float64x2_t v8 = vld1q_f64(p + 16);
    float64x2_t v9 = vld1q_f64(p + 18);
    float64x2_t v10 = vld1q_f64(p + 20);
    float64x2_t v11 = vld1q_f64(p + 22);
    float64x2_t v12 = vld1q_f64(p + 24);
    float64x2_t v13 = vld1q_f64(p + 26);
    float64x2_t v14 = vld1q_f64(p + 28);
    float64x2_t v15 = vld1q_f64(p + 30);
    res += (int)(vaddvq_f64(v0) + vaddvq_f64(v1) + vaddvq_f64(v2) +
                 vaddvq_f64(v3) + vaddvq_f64(v4) + vaddvq_f64(v5) +
                 vaddvq_f64(v6) + vaddvq_f64(v7) + vaddvq_f64(v8) +
                 vaddvq_f64(v9) + vaddvq_f64(v10) + vaddvq_f64(v11) +
                 vaddvq_f64(v12) + vaddvq_f64(v13) + vaddvq_f64(v14) +
                 vaddvq_f64(v15));
  }
}
#endif

#ifdef NEON_FP64_STORE
void test1(double *indices) {
  float64x2_t v = vld1q_f64(indices);
  double tmp[2 * 256] __attribute__((aligned(64)));
  for (int i = 0; i < repeat; i++) {
    double *p = tmp + (i & 0x7f) * 2;
    vst1q_f64(p + 0 * 2, v);
    vst1q_f64(p + 1 * 2, v);
    vst1q_f64(p + 2 * 2, v);
    vst1q_f64(p + 3 * 2, v);
    vst1q_f64(p + 4 * 2, v);
    vst1q_f64(p + 5 * 2, v);
    vst1q_f64(p + 6 * 2, v);
    vst1q_f64(p + 7 * 2, v);
    vst1q_f64(p + 8 * 2, v);
    vst1q_f64(p + 9 * 2, v);
    vst1q_f64(p + 10 * 2, v);
    vst1q_f64(p + 11 * 2, v);
    vst1q_f64(p + 12 * 2, v);
    vst1q_f64(p + 13 * 2, v);
    vst1q_f64(p + 14 * 2, v);
    vst1q_f64(p + 15 * 2, v);
  }
  res += tmp[0];
}
#endif

int main(int argc, char *argv[]) {

  int opt;
  while ((opt = getopt(argc, argv, "")) != -1) {
    switch (opt) {
    default:
      fprintf(stderr, "Usage: %s\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  bind_to_core();
  setup_perf_instructions();
  setup_perf_cycles();

  double indices[256] __attribute__((aligned(64)));
  for (int i = 0; i < 256; i++) {
    indices[i] = i + 1.0;
  }

  int warmup = 1000;

  for (int i = 0; i < warmup; i++) {
    test1(indices);
  }

  int m = 50000;
  uint64_t cycles_before = perf_read_cycles();
  uint64_t instructions_before = perf_read_instructions();

  for (int i = 0; i < m; i++) {
    test1(indices);
  }

  uint64_t cycles_after = perf_read_cycles();
  uint64_t instructions_after = perf_read_instructions();

  printf("%ld cycles, %ld instructions, %.2lf ipc, %d ans\n",
         (cycles_after - cycles_before) / m / repeat / unroll,
         (instructions_after - instructions_before) / m / repeat / unroll,
         (double)(instructions_after - instructions_before) /
             (cycles_after - cycles_before),
         res);
  return 0;
}

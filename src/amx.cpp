#include "include/utils.h"
#include <immintrin.h>
#include <time.h>
#include <unistd.h>

float res = 0;
const int n = 1000;
int array[n] = {0};
const int repeat = 500;
const int unroll = 8;

void test_1(float *indices) {
  // Test function to be implemented
  #ifdef AMX
  for(int i = 0; i < repeat; i++) {
    __tile_loadd(0, indices, 32);
    __tile_loadd(1, indices, 32);
    __tile_loadd(2, indices, 32);
    __tile_loadd(3, indices, 32);
    __tile_loadd(4, indices, 32);
    __tile_loadd(5, indices, 32);
    __tile_loadd(6, indices, 32);
    __tile_loadd(7, indices, 32);
    __tile_loadd(8, indices, 32);
  }
  #endif
}


int main(int argc, char *argv[]) {

  int opt;
  while ((opt = getopt(argc, argv, "")) != -1) {
    switch (opt) {
    default:
      fprintf(stderr, "Usage: %s [-p]\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  float indices[1024];

  bind_to_core();
  setup_perf_instructions();
  setup_perf_cycles();
   int warmup = 1000;

  for (int i = 0; i < warmup; i++) {
    test_1(indices);
  }

  int m = 50000;
  uint64_t cycles_before = perf_read_cycles();
  uint64_t instructions_before = perf_read_instructions();

  for (int i = 0; i < m; i++) {
    test_1(indices);
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
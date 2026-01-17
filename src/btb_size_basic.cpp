#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "include/utils.h"

extern void btb_size_basic(FILE *fp);
int main(int argc, char *argv[]) {
  FILE *fp = fopen_outputs_file("btb_size_basic.csv", "w");
  assert(fp);
  btb_size_basic(fp);
  printf("Results are written to btb_size_basic.csv\n");
  return 0;
}

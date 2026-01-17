#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "include/utils.h"

extern void phr_size(FILE *fp);
int main(int argc, char *argv[]) {
  FILE *fp = fopen_outputs_file("phr_size.csv", "w");
  assert(fp);
  phr_size(fp);
  printf("Results are written to phr_size.csv\n");
  return 0;
}

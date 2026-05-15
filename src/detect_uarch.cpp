#include "include/uarch.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  int cpu = -1;
  if (argc >= 3 && strcmp(argv[1], "--cpu") == 0) {
    cpu = atoi(argv[2]);
  }

  enum uarch uarch;
  if (cpu >= 0) {
    uarch = get_uarch_of_cpu(cpu);
    fprintf(stderr, "Detected uarch for CPU %d: %s\n", cpu,
            uarch_to_string(uarch));
  } else {
    // default to preferred core for deterministic build-time output
    uarch = get_uarch_of_cpu(get_bind_core());
    fprintf(stderr, "Detected uarch: %s (preferred core %d)\n",
            uarch_to_string(uarch), get_bind_core());
  }

  switch (uarch) {
  case firestorm:
    printf("-DAPPLE_SILICON\n");
    printf("-DAPPLE_PCORE\n");
    printf("-DAPPLE_M1\n");
    printf("-DAPPLE_M1_FIRESTORM\n");
    break;
  case icestorm:
    printf("-DAPPLE_SILICON\n");
    printf("-DAPPLE_M1\n");
    printf("-DAPPLE_M1_ICESTORM\n");
    break;
  case avalanche:
    printf("-DAPPLE_SILICON\n");
    printf("-DAPPLE_PCORE\n");
    printf("-DAPPLE_M2\n");
    printf("-DAPPLE_M2_AVALANCHE\n");
    break;
  case blizzard:
    printf("-DAPPLE_SILICON\n");
    printf("-DAPPLE_M2\n");
    printf("-DAPPLE_M2_BLIZZARD\n");
    break;
  case m4_pcore:
    printf("-DAPPLE_SILICON\n");
    printf("-DAPPLE_PCORE\n");
    printf("-DAPPLE_M4\n");
    printf("-DAPPLE_M4_PCORE\n");
    break;
  case m4_ecore:
    printf("-DAPPLE_SILICON\n");
    printf("-DAPPLE_M4\n");
    printf("-DAPPLE_M4_ECORE\n");
    break;
  case oryon:
    printf("-DQUALCOMM_ORYON\n");
    break;
  case cortex_a35:
    printf("-DARM_CORTEX_A35\n");
    break;
  case cortex_a53:
    printf("-DARM_CORTEX_A53\n");
    break;
  case cortex_a55:
    printf("-DARM_CORTEX_A55\n");
    break;
  case cortex_a510:
    printf("-DARM_CORTEX_A510\n");
    break;
  case cortex_a520:
    printf("-DARM_CORTEX_A520\n");
    break;
  case cortex_a57:
    printf("-DARM_CORTEX_A57\n");
    break;
  case cortex_a72:
    printf("-DARM_CORTEX_A72\n");
    break;
  case cortex_a73:
    printf("-DARM_CORTEX_A73\n");
    break;
  case cortex_a75:
    printf("-DARM_CORTEX_A75\n");
    break;
  case cortex_a76:
    printf("-DARM_CORTEX_A76\n");
    break;
  case cortex_a710:
    printf("-DARM_CORTEX_A710\n");
    break;
  case cortex_a715:
    printf("-DARM_CORTEX_A715\n");
    break;
  case cortex_a720:
    printf("-DARM_CORTEX_A720\n");
    break;
  case cortex_a725:
    printf("-DARM_CORTEX_A725\n");
    break;
  case cortex_a77:
    printf("-DARM_CORTEX_A77\n");
    break;
  case cortex_a78:
    printf("-DARM_CORTEX_A78\n");
    break;
  case cortex_x1:
    printf("-DARM_CORTEX_X1\n");
    break;
  case cortex_x2:
    printf("-DARM_CORTEX_X2\n");
    break;
  case cortex_x3:
    printf("-DARM_CORTEX_X3\n");
    break;
  case cortex_x4:
    printf("-DARM_CORTEX_X4\n");
    break;
  case cortex_x925:
    printf("-DARM_CORTEX_X925\n");
    break;
  case neoverse_n1:
    printf("-DNO_FJCVTZS\n");
    printf("-DARM_NEOVERSE_N1\n");
    break;
  case neoverse_v1:
    printf("-DARM_NEOVERSE_V1\n");
    break;
  case neoverse_n2:
    printf("-DARM_NEOVERSE_N2\n");
    break;
  case neoverse_v2:
    printf("-DARM_NEOVERSE_V2\n");
    break;
  case neoverse_v3:
    printf("-DARM_NEOVERSE_V3\n");
    break;
  case tsv110:
    printf("-DHISILICON_TSV110\n");
    break;
  case unknown_arm64:
    break;
  case unknown_riscv64:
    break;
  case granite_rapids:
    printf("-DINTEL\n");
    printf("-DINTEL_GRANITE_RAPIDS\n");
    break;
  case golden_cove:
    printf("-DINTEL\n");
    printf("-DINTEL_AHYBRID\n");
    break;
  case willow_cove:
    printf("-DINTEL\n");
    printf("-DINTEL_WILLOWCOVE\n");
    break;
  case gracemont:
    printf("-DINTEL\n");
    printf("-DINTEL_AHYBRID\n");
    break;
  case sunny_cove:
    printf("-DINTEL\n");
    printf("-DINTEL_ICELAKE_SERVER\n");
    break;
  case skylake:
    printf("-DINTEL\n");
    printf("-DINTEL_SKYLAKE_SERVER\n");
    break;
  case broadwell:
    printf("-DINTEL\n");
    printf("-DINTEL_BROADWELL\n");
    break;
  case whiskylake:
    printf("-DINTEL\n");
    printf("-DINTEL_WHISKYLAKE\n");
    break;
  case haswell:
    printf("-DINTEL\n");
    printf("-DINTEL_HASWELL\n");
    break;
  case zen1:
    printf("-DAMD\n");
    printf("-DAMD_ZEN1\n");
    break;
  case zen2:
    printf("-DAMD\n");
    printf("-DAMD_ZEN2\n");
    break;
  case zen3:
    printf("-DAMD\n");
    printf("-DAMD_ZEN3\n");
    break;
  case zen4:
    printf("-DAMD\n");
    printf("-DAMD_ZEN4\n");
    break;
  case zen5:
    printf("-DAMD\n");
    printf("-DAMD_ZEN5\n");
    break;
  case unknown_amd64:
    break;
  case riscv64:
    printf("-DRISCV64\n");
    break;
  case spacemit_x60:
    printf("-DRISCV64\n");
    printf("-DSPACEMIT_X60\n");
    break;
  case spacemit_x100:
    printf("-DRISCV64\n");
    printf("-DSPACEMIT_X100\n");
    break;
  case spacemit_a100:
    printf("-DRISCV64\n");
    printf("-DSPACEMIT_A100\n");
    break;
  case sifive_p550:
    printf("-DRISCV64\n");
    printf("-DSIFIVE_P550\n");
    break;
  case la464:
    printf("-DLA464\n");
    break;
  case la664:
    printf("-DLA664\n");
    break;
  case unknown_loongarch64:
    break;
  case power8:
    printf("-DPOWER8\n");
    break;
  case power9:
    printf("-DPOWER9\n");
    break;
  case unknown_ppc64le:
    break;
    break;
  default:
    assert(false);
  }
  return 0;
}

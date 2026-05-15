#ifndef __UARCH_H__
#define __UARCH_H__

enum uarch {
  // special
  unknown,

  // arm64
  // apple
  // m1
  firestorm,
  icestorm,
  // m2
  avalanche,
  blizzard,
  // m4
  m4_pcore,
  m4_ecore,
  // qualcomm
  oryon,
  // arm — efficiency
  cortex_a35,
  cortex_a53,
  cortex_a55,
  cortex_a510,
  cortex_a520,
  // arm — mid performance
  cortex_a57,
  cortex_a72,
  cortex_a73,
  cortex_a75,
  cortex_a76,
  cortex_a710,
  cortex_a715,
  cortex_a720,
  cortex_a725,
  // arm — big performance
  cortex_a77,
  cortex_a78,
  cortex_x1,
  cortex_x2,
  cortex_x3,
  cortex_x4,
  cortex_x925,
  // arm — server
  neoverse_n1,
  neoverse_v1,
  neoverse_n2,
  neoverse_v2,
  neoverse_v3,
  // hisilicon
  tsv110,
  unknown_arm64,
  arm64_begin = firestorm,
  arm64_end = unknown_arm64,

  // riscv64
  spacemit_x60,
  spacemit_x100,
  spacemit_a100,
  sifive_p550,
  riscv64,
  unknown_riscv64,

  // ppc64le
  power8,
  power9,

  unknown_ppc64le,
  ppc64le_begin = power8,
  ppc64le_end = unknown_ppc64le,

  // loongarch
  la464,
  la664,
  unknown_loongarch64,
  loongarch64_begin = la464,
  loongarch64_end = unknown_loongarch64,

  // intel
  granite_rapids,
  golden_cove,
  willow_cove,
  gracemont,
  sunny_cove,
  skylake,
  broadwell,
  whiskylake,
  haswell,
  // amd
  zen1,
  zen2,
  zen3,
  zen4,
  zen5,

  unknown_amd64,

  // valid range
  all_begin = firestorm,
  all_end = unknown_amd64,
};

// detect uarch of the current (bound) CPU
enum uarch get_uarch();
// detect uarch of a specific logical CPU
enum uarch get_uarch_of_cpu(int cpu);
// which core to bind
int get_bind_core();
// number of online CPUs
int get_num_cores();
// human-readable name
const char *uarch_to_string(enum uarch u);

#endif

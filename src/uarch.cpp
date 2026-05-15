#include "include/uarch.h"
#include <assert.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string.h>
#include <string>
#include <vector>
#include <unistd.h>

#ifdef __linux__
#include <sys/syscall.h>
#endif

static std::vector<enum uarch> per_cpu_uarch;
static int preferred_core = 0;
static bool init_done = false;

// global feature flags
static bool has_sve = false;
static bool has_avx512f = false;
static bool has_avx2 = false;
static bool has_amx = false;

// ---------------------------------------------------------------
// per-CPU classification helpers
// ---------------------------------------------------------------

static enum uarch arm_classify(int implementer, int part) {
  // efficiency
  if (implementer == 0x41 && part == 0xd04)
    return cortex_a35;
  else if (implementer == 0x41 && part == 0xd03)
    return cortex_a53;
  else if (implementer == 0x41 && part == 0xd05)
    return cortex_a55;
  else if (implementer == 0x41 && part == 0xd46)
    return cortex_a510;
  else if (implementer == 0x41 && part == 0xd80)
    return cortex_a520;
  // mid performance
  else if (implementer == 0x41 && part == 0xd07)
    return cortex_a57;
  else if (implementer == 0x41 && part == 0xd08)
    return cortex_a72;
  else if (implementer == 0x41 && part == 0xd09)
    return cortex_a73;
  else if (implementer == 0x41 && part == 0xd0a)
    return cortex_a75;
  else if (implementer == 0x41 && part == 0xd0b)
    return cortex_a76;
  else if (implementer == 0x41 && part == 0xd47)
    return cortex_a710;
  else if (implementer == 0x41 && part == 0xd4d)
    return cortex_a715;
  else if (implementer == 0x41 && part == 0xd81)
    return cortex_a720;
  else if (implementer == 0x41 && part == 0xd87)
    return cortex_a725;
  // big performance
  else if (implementer == 0x41 && part == 0xd0d)
    return cortex_a77;
  else if (implementer == 0x41 && part == 0xd41)
    return cortex_a78;
  else if (implementer == 0x41 && part == 0xd44)
    return cortex_x1;
  else if (implementer == 0x41 && part == 0xd48)
    return cortex_x2;
  else if (implementer == 0x41 && part == 0xd4e)
    return cortex_x3;
  else if (implementer == 0x41 && part == 0xd82)
    return cortex_x4;
  else if (implementer == 0x41 && part == 0xd85)
    return cortex_x925;
  // server
  else if (implementer == 0x41 && part == 0xd0c)
    return neoverse_n1;
  else if (implementer == 0x41 && part == 0xd40)
    return neoverse_v1;
  else if (implementer == 0x41 && part == 0xd49)
    return neoverse_n2;
  else if (implementer == 0x41 && part == 0xd4f)
    return neoverse_v2;
  else if (implementer == 0x41 && part == 0xd83)
    return neoverse_v3;
  // hisilicon
  else if (implementer == 0x48 && part == 0xd01)
    return tsv110;
  return unknown;
}

static enum uarch x86_classify(int family, int model) {
  // Intel — development timeline
  if (family == 6 && (model == 60 || model == 69 || model == 70))
    return haswell;
  else if (family == 6 && model == 79)
    return broadwell;
  else if (family == 6 && (model == 78 || model == 85 || model == 142 ||
                            model == 158 || model == 165))
    return skylake;
  else if (family == 6 && (model == 106 || model == 167))
    return sunny_cove;
  else if (family == 6 && model == 140)
    return willow_cove;
  else if (family == 6 && (model == 143 || model == 151 ||
                            model == 183 || model == 207))
    return golden_cove;
  else if (family == 6 && model == 173)
    return granite_rapids;
  // AMD — development timeline
  else if (family == 23 && (model == 1 || model == 8 || model == 24))
    return zen1;
  else if (family == 23 && model == 49)
    return zen2;
  else if (family == 25 && (model == 1 || model == 33))
    return zen3;
  else if (family == 25 && (model == 17 || model == 97))
    return zen4;
  else if (family == 26 && (model == 68 || model == 96 || model == 112))
    return zen5;
  return unknown;
}

static enum uarch riscv_classify(unsigned long mvendorid,
                                 unsigned long marchid) {
  if (mvendorid == 0x710 && marchid == 0x8000000058000001)
    return spacemit_x60;
  else if (mvendorid == 0x710 && marchid == 0x8000000058000002)
    return spacemit_x100;
  else if (mvendorid == 0x710 && marchid == 0x8000000041000002)
    return spacemit_a100;
  else if (mvendorid == 0x489 && marchid == 0x8000000000000008)
    return sifive_p550;
  return unknown;
}

struct cpu_data {
  int processor;
  int family;
  int model;
  int implementer;
  int part;
  unsigned long mvendorid;
  unsigned long marchid;
  std::string model_name;

  cpu_data() : processor(-1), family(0), model(0), implementer(0), part(0),
               mvendorid(0), marchid(0) {}
};

// ---------------------------------------------------------------
// classify a single CPU entry from /proc/cpuinfo
// ---------------------------------------------------------------

static enum uarch classify_cpu(const cpu_data &c) {
#if defined(__x86_64__)
  enum uarch u = x86_classify(c.family, c.model);
  return u != unknown ? u : unknown_amd64;
#elif defined(__aarch64__)
  enum uarch u = arm_classify(c.implementer, c.part);
  return u != unknown ? u : unknown_arm64;
#elif defined(__riscv)
  enum uarch u = riscv_classify(c.mvendorid, c.marchid);
  return u != unknown ? u : riscv64;
#elif defined(__loongarch__)
  if (c.model_name.find(" Loongson") != std::string::npos &&
      c.model_name == " Loongson-3C5000")
    return la464;
  return unknown_loongarch64;
#elif defined(__powerpc64__)
  if (c.model_name == " POWER8 (raw), altivec supported")
    return power8;
  if (c.model_name == " POWER9, altivec supported")
    return power9;
  return unknown_ppc64le;
#else
  return unknown;
#endif
}

// ---------------------------------------------------------------
// parse /proc/cpuinfo, fill per_cpu_uarch
// ---------------------------------------------------------------

static void parse_proc_cpuinfo() {
  std::ifstream t("/proc/cpuinfo");
  std::string line;

  std::vector<struct cpu_data> cpus;
  struct cpu_data cur;

  while (std::getline(t, line)) {
    // blank line separates processor blocks (handled by "processor:" key below)
    if (line.empty())
      continue;
    size_t pos = line.find(':');
    if (pos == std::string::npos)
      continue;
    std::string key = line.substr(0, pos);
    key.erase(key.find_last_not_of(" \t") + 1);
    std::string value = line.substr(pos + 1);

    if (key == "processor") {
      // new processor block — push previous if any
      if (cur.processor >= 0)
        cpus.push_back(cur);
      cur = cpu_data();
      cur.processor = std::stoi(value);
    } else if (key == "cpu family") {
      cur.family = std::stoi(value);
    } else if (key == "model") {
      cur.model = std::stoi(value);
    } else if (key == "CPU implementer") {
      cur.implementer = std::stoi(value, nullptr, 16);
    } else if (key == "CPU part") {
      cur.part = std::stoi(value, nullptr, 16);
    } else if (key == "Model Name") {
      cur.model_name = value;
    } else if (key == "mvendorid") {
      if (cur.mvendorid == 0)
        cur.mvendorid = std::stoul(value, nullptr, 16);
    } else if (key == "marchid") {
      if (cur.marchid == 0)
        cur.marchid = std::stoul(value, nullptr, 16);
    } else if (key == "flags") {
      if (value.find("avx512f") != std::string::npos)
        has_avx512f = true;
      if (value.find("avx2") != std::string::npos)
        has_avx2 = true;
      if (value.find("amx") != std::string::npos)
        has_amx = true;
    } else if (key == "Features") {
      if (value.find("sve") != std::string::npos)
        has_sve = true;
    }
  }
  // last block
  if (cur.processor >= 0)
    cpus.push_back(cur);

  t.close();

  if (cpus.empty()) {
    fprintf(stderr, "Warning: /proc/cpuinfo has no entries\n");
    return;
  }

  // find max processor number for sizing
  int max_proc = 0;
  for (auto &c : cpus)
    if (c.processor > max_proc)
      max_proc = c.processor;

  per_cpu_uarch.assign(max_proc + 1, unknown);

  for (auto &c : cpus)
    per_cpu_uarch[c.processor] = classify_cpu(c);
}

// ---------------------------------------------------------------
// current CPU (via syscall)
// ---------------------------------------------------------------

static int read_current_cpu() {
#ifdef __linux__
  unsigned cpu;
  if (syscall(SYS_getcpu, &cpu, NULL, NULL) == 0)
    return (int)cpu;
#endif
  return 0;
}

// ---------------------------------------------------------------
// DT-based early detection (Apple, QCOM) — fills all CPUs
// ---------------------------------------------------------------

static bool try_dt_detection() {
  // This path is only for platforms where /proc/cpuinfo doesn't
  // give per-core differentiation (Apple, pre-silicon QCOM etc.)
  std::ifstream t("/sys/devices/system/node/node0/cpu0/of_node/compatible");
  std::stringstream buffer;
  buffer << t.rdbuf();
  std::string content = buffer.str();
  t.close();

  enum uarch dt_uarch = unknown;

  if (content.find("apple,icestorm") != std::string::npos) {
    fprintf(stderr, "Apple M1 detected\n");
    fprintf(stderr, "Configured for Apple M1 Firestorm\n");
    dt_uarch = firestorm;
    preferred_core = 4;
  }
  if (content.find("qcom,oryon") != std::string::npos) {
    fprintf(stderr, "Qualcomm Oryon detected\n");
    dt_uarch = oryon;
  }

  if (dt_uarch == unknown) {
    t.open("/sys/firmware/devicetree/base/compatible");
    buffer.clear();
    buffer << t.rdbuf();
    content = buffer.str();
    t.close();
    if (content.find("qcom,sc8280xp") != std::string::npos) {
      fprintf(stderr, "Qualcomm 8cx Gen 3 detected\n");
      fprintf(stderr, "Configured for Cortex-X1\n");
      dt_uarch = cortex_x1;
      preferred_core = 4;
    }
  }

  if (dt_uarch != unknown) {
    int ncpu = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpu <= 0)
      ncpu = 8;
    per_cpu_uarch.assign(ncpu, dt_uarch);
    return true;
  }
  return false;
}

// ---------------------------------------------------------------
// name↔enum mapping table (single source of truth)
// ---------------------------------------------------------------

static const struct { enum uarch u; const char *name; } kUarchNames[] = {
  {firestorm, "Firestorm"},
  {icestorm, "Icestorm"},
  {avalanche, "Avalanche"},
  {blizzard, "Blizzard"},
  {m4_pcore, "M4-P-Core"},
  {m4_ecore, "M4-E-Core"},
  {oryon, "Oryon"},
  {cortex_a35, "Cortex-A35"},
  {cortex_a53, "Cortex-A53"},
  {cortex_a55, "Cortex-A55"},
  {cortex_a510, "Cortex-A510"},
  {cortex_a520, "Cortex-A520"},
  {cortex_a57, "Cortex-A57"},
  {cortex_a72, "Cortex-A72"},
  {cortex_a73, "Cortex-A73"},
  {cortex_a75, "Cortex-A75"},
  {cortex_a76, "Cortex-A76"},
  {cortex_a710, "Cortex-A710"},
  {cortex_a715, "Cortex-A715"},
  {cortex_a720, "Cortex-A720"},
  {cortex_a725, "Cortex-A725"},
  {cortex_a77, "Cortex-A77"},
  {cortex_a78, "Cortex-A78"},
  {cortex_x1, "Cortex-X1"},
  {cortex_x2, "Cortex-X2"},
  {cortex_x3, "Cortex-X3"},
  {cortex_x4, "Cortex-X4"},
  {cortex_x925, "Cortex-X925"},
  {neoverse_n1, "Neoverse-N1"},
  {neoverse_v1, "Neoverse-V1"},
  {neoverse_n2, "Neoverse-N2"},
  {neoverse_v2, "Neoverse-V2"},
  {neoverse_v3, "Neoverse-V3"},
  {tsv110, "TSV110"},
  {unknown_arm64, "Unknown-ARM64"},
  {spacemit_x60, "Spacemit-X60"},
  {spacemit_x100, "Spacemit-X100"},
  {spacemit_a100, "Spacemit-A100"},
  {sifive_p550, "SiFive-P550"},
  {riscv64, "RISC-V"},
  {unknown_riscv64, "Unknown-RISC-V"},
  {power8, "Power8"},
  {power9, "Power9"},
  {unknown_ppc64le, "Unknown-PPC64LE"},
  {la464, "LA464"},
  {la664, "LA664"},
  {unknown_loongarch64, "Unknown-LoongArch"},
  {granite_rapids, "Granite-Rapids"},
  {golden_cove, "Golden-Cove"},
  {willow_cove, "Willow-Cove"},
  {gracemont, "Gracemont"},
  {sunny_cove, "Sunny-Cove"},
  {skylake, "Skylake"},
  {broadwell, "Broadwell"},
  {whiskylake, "Whiskylake"},
  {haswell, "Haswell"},
  {zen1, "Zen1"},
  {zen2, "Zen2"},
  {zen3, "Zen3"},
  {zen4, "Zen4"},
  {zen5, "Zen5"},
  {unknown_amd64, "Unknown-AMD64"},
};

// ---------------------------------------------------------------
// UARCH_OVERRIDE handling
// ---------------------------------------------------------------

static bool try_uarch_override() {
  const char *override = getenv("UARCH_OVERRIDE");
  if (!override)
    return false;
  for (auto &e : kUarchNames) {
    if (strcasecmp(override, e.name) == 0) {
      fprintf(stderr, "Uarch overridden with %s\n", override);
      int ncpu = (int)sysconf(_SC_NPROCESSORS_ONLN);
      if (ncpu <= 0)
        ncpu = 8;
      per_cpu_uarch.assign(ncpu, e.u);
      return true;
    }
  }
  fprintf(stderr, "Unknown UARCH_OVERRIDE: %s, ignoring\n", override);
  return false;
}

// ---------------------------------------------------------------
// macOS sysctl fallback
// ---------------------------------------------------------------

#ifdef __APPLE__
static bool try_macos_sysctl() {
  FILE *fp = popen("sysctl -nx hw.cpufamily", "r");
  if (!fp)
    return false;
  char buf[128];
  if (!fgets(buf, sizeof(buf), fp)) {
    pclose(fp);
    return false;
  }
  pclose(fp);

  enum uarch u = unknown;
  if (strcmp(buf, "0x1b588bb3\n") == 0) {
    fprintf(stderr, "Apple M1 detected\n");
    fprintf(stderr, "Configured for Apple M1 Firestorm\n");
    u = firestorm;
  } else if (strcmp(buf, "0xda33d83d\n") == 0) {
    fprintf(stderr, "Apple M2 detected\n");
    fprintf(stderr, "Configured for Apple M2 Avalanche\n");
    u = avalanche;
    preferred_core = -1;
  } else if (strcmp(buf, "0x6f5129ac\n") == 0) {
    fprintf(stderr, "Apple M4 detected\n");
    fprintf(stderr, "Configured for Apple M4 P core\n");
    u = m4_pcore;
    preferred_core = -1;
  }
  if (u != unknown) {
    int ncpu = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpu <= 0)
      ncpu = 8;
    per_cpu_uarch.assign(ncpu, u);
    return true;
  }
  return false;
}
#endif

// ---------------------------------------------------------------
// init — called once
// ---------------------------------------------------------------

static void init() {
  if (init_done)
    return;
  init_done = true;

  bool detected = try_uarch_override() || try_dt_detection();

  if (!detected) {
    parse_proc_cpuinfo();
    if (per_cpu_uarch.empty()) {
#ifdef __APPLE__
      detected = try_macos_sysctl();
#endif
      if (!detected)
        per_cpu_uarch.assign(1, unknown);
    }
  }

  if (has_avx2)
    fprintf(stdout, "AVX2 detected\n");
  if (has_avx512f)
    fprintf(stdout, "AVX512F detected\n");
  if (has_sve)
    fprintf(stdout, "SVE detected\n");
  if (has_amx)
    fprintf(stdout, "AMX detected\n");

  fprintf(stderr, "Detected %zu CPUs\n", per_cpu_uarch.size());
  size_t start = 0;
  while (start < per_cpu_uarch.size()) {
    size_t end = start;
    while (end + 1 < per_cpu_uarch.size() &&
           per_cpu_uarch[end + 1] == per_cpu_uarch[start])
      end++;
    if (start == end)
      fprintf(stderr, "  CPU %zu: %s\n", start,
              uarch_to_string(per_cpu_uarch[start]));
    else
      fprintf(stderr, "  CPUs %zu-%zu: %s\n", start, end,
              uarch_to_string(per_cpu_uarch[start]));
    start = end + 1;
  }
  fprintf(stderr, "Preferred core: %d\n", preferred_core);
}

// ---------------------------------------------------------------
// public API
// ---------------------------------------------------------------

enum uarch get_uarch() {
  init();
  int cpu = read_current_cpu();
  if (cpu >= 0 && (size_t)cpu < per_cpu_uarch.size())
    return per_cpu_uarch[cpu];
  // fallback: return the preferred core's uarch
  if (preferred_core >= 0 && (size_t)preferred_core < per_cpu_uarch.size())
    return per_cpu_uarch[preferred_core];
  return per_cpu_uarch[0];
}

enum uarch get_uarch_of_cpu(int cpu) {
  init();
  if (cpu >= 0 && (size_t)cpu < per_cpu_uarch.size())
    return per_cpu_uarch[cpu];
  return per_cpu_uarch[0];
}

int get_bind_core() {
  init();
  const char *env = getenv("BIND_CORE_OVERRIDE");
  if (env) {
    int core;
    if (sscanf(env, "%d", &core) > 0) {
      return core;
    }
  }
  return preferred_core;
}

int get_num_cores() {
  init();
  return (int)per_cpu_uarch.size();
}

const char *uarch_to_string(enum uarch u) {
  for (auto &e : kUarchNames)
    if (e.u == u)
      return e.name;
  return "Unknown";
}

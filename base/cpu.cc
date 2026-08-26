#include <dirent.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "base/cpu.h"

namespace base {
namespace {

constexpr int kMaxCpus = 1024;

struct CpuInfoCache {
  int num_cpus = 0;
  int num_numa_domains = 0;
  int numa_map[kMaxCpus];

  CpuInfoCache() {
    std::fill(std::begin(numa_map), std::end(numa_map), -1);
    InitCpus();
    InitNuma();
  }

 private:
  void InitCpus() {
    long sc_res = sysconf(_SC_NPROCESSORS_CONF);
    if (sc_res > 0) {
      num_cpus = static_cast<int>(sc_res);
    } else {
      num_cpus = 1;
    }
  }

  void InitNuma() {
    DIR* dir = opendir("/sys/devices/system/node");
    if (!dir) {
      num_numa_domains = 1;
      std::fill(std::begin(numa_map), std::end(numa_map), 0);
      return;
    }

    int max_node = -1;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      int node_id = -1;
      if (sscanf(entry->d_name, "node%d", &node_id) == 1) {
        max_node = std::max(max_node, node_id);

        char cpu_list_path[256];
        snprintf(cpu_list_path, sizeof(cpu_list_path),
                 "/sys/devices/system/node/node%d/cpulist", node_id);
        ParseCpuList(cpu_list_path, node_id);
      }
    }
    closedir(dir);

    num_numa_domains = (max_node >= 0) ? (max_node + 1) : 1;
  }

  void ParseCpuList(const char* path, int node_id) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return;

    char buf[1024];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return;
    buf[n] = '\0';

    char* ptr = buf;
    while (*ptr) {
      char* end;
      int start = strtol(ptr, &end, 10);
      int stop = start;

      if (*end == '-') {
        ptr = end + 1;
        stop = strtol(ptr, &end, 10);
      }

      for (int cpu = start; cpu <= stop && cpu < kMaxCpus; ++cpu) {
        numa_map[cpu] = node_id;
      }

      ptr = end;
      while (*ptr == ',' || *ptr == ' ' || *ptr == '\n' || *ptr == '\r') {
        ptr++;
      }
    }
  }
};

const CpuInfoCache& GetCpuCache() {
  static const CpuInfoCache cache;
  return cache;
}

}  // namespace

int NumCpus() { return GetCpuCache().num_cpus; }

int NumNumaDomains() { return GetCpuCache().num_numa_domains; }

int NumaDomainFor(int cpu) {
  if (cpu < 0 || cpu >= kMaxCpus) {
    return -1;
  }
  return GetCpuCache().numa_map[cpu];
}

}  // namespace base

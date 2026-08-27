#ifndef BASE_CPU_H_
#define BASE_CPU_H_

#include <cstdint>
#include <vector>

namespace base {

// Returns the number of CPUs in the system.
int NumCpus();

// Returns the CPU on which the calling thread is running.
// [0, NumCpus()).
int CurrentCpu();

// Returns the number of NUMA domains in the system.
int NumNumaDomains();

// Returns the NUMA domain assocaited with 'cpu'.
// [0, NumNumaDomains()) when cpu is [0, NumCpus()).
// -1 otherwise. GIGO.
int NumaDomainFor(int cpu);

// Memoized mapping from CPU to NUMA domain.
class NumaMap final {
 public:
  NumaMap() = default;  // empty map
  ~NumaMap() = default;
  NumaMap(const NumaMap&) = default;
  NumaMap& operator=(const NumaMap&) = default;
  NumaMap(NumaMap&&) = default;
  NumaMap& operator=(NumaMap&&) = default;

  static NumaMap BuildFromSystemTopology();
  static NumaMap BuildFrom(std::vector<int> numa_by_cpu);

  int NumCpus() const { return numa_by_cpu_.size(); }
  int NumaDomainForCpu(int cpu) const { return numa_by_cpu_[cpu]; }

 private:
  std::vector<int> numa_by_cpu_;
};

inline int CurrentCpu() {
#if defined(__x86_64__)
  // Read CPU ID directly from the RDPID instruction (x86_64).
  // IA32_TSC_AUX holds the CPU ID in bits [11:0] and NUMA node in [21:12].
  uint64_t aux;
  asm volatile("rdpid %0" : "=r"(aux));
  return static_cast<int>(aux & 0xFFF);
#elif defined(__aarch64__)
  uint64_t val;
  asm volatile("mrs %0, tpidrro_el0" : "=r"(val));
  return static_cast<int>(val & 0xFFF);
#else
#error "Unsupported architecture: base::CurrentCpu requires x86_64 or ARM64"
#endif
}

}  // namespace base

#endif  // #ifndef BASE_CPU_H_

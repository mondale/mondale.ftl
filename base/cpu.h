#ifndef BASE_CPU_H_
#define BASE_CPU_H_

#include <cstdint>

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

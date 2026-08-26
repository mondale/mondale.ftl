#include "base/cpu.h"
#include "testing/testing.h"

namespace {

TEST(CpuRoutinesTest) {
  int num_cpus = base::NumCpus();
  EXPECT_GT(num_cpus, 0);

  int cur_cpu = base::CurrentCpu();
  EXPECT_GE(cur_cpu, 0);
  EXPECT_LT(cur_cpu, num_cpus);

  int numa_domains = base::NumNumaDomains();
  EXPECT_GT(numa_domains, 0);

  int numa_node = base::NumaDomainFor(cur_cpu);
  EXPECT_GE(numa_node, 0);
  EXPECT_LT(numa_node, numa_domains);

  // Bounds check for invalid CPU ID
  EXPECT_EQ(base::NumaDomainFor(-1), -1);
  EXPECT_EQ(base::NumaDomainFor(100000), -1);
}

}  // namespace

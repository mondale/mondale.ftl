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

TEST(NumaMapDefaultConstruction) {
  base::NumaMap map;
  EXPECT_EQ(map.NumCpus(), 0);
}

TEST(NumaMapBuildFromSystemTopology) {
  base::NumaMap map = base::NumaMap::BuildFromSystemTopology();
  int total_cpus = base::NumCpus();

  EXPECT_EQ(map.NumCpus(), total_cpus);

  for (int cpu = 0; cpu < total_cpus; ++cpu) {
    EXPECT_EQ(map.NumaDomainForCpu(cpu), base::NumaDomainFor(cpu));
  }
}

TEST(NumaMapCopyConstruction) {
  base::NumaMap orig = base::NumaMap::BuildFromSystemTopology();
  base::NumaMap copy(orig);

  EXPECT_EQ(copy.NumCpus(), orig.NumCpus());
  for (int cpu = 0; cpu < orig.NumCpus(); ++cpu) {
    EXPECT_EQ(copy.NumaDomainForCpu(cpu), orig.NumaDomainForCpu(cpu));
  }
}

TEST(NumaMapMoveConstruction) {
  base::NumaMap orig = base::NumaMap::BuildFromSystemTopology();
  int orig_cpus = orig.NumCpus();
  int first_numa = orig_cpus > 0 ? orig.NumaDomainForCpu(0) : -1;

  base::NumaMap moved(std::move(orig));
  EXPECT_EQ(moved.NumCpus(), orig_cpus);
  if (orig_cpus > 0) {
    EXPECT_EQ(moved.NumaDomainForCpu(0), first_numa);
  }
}

TEST(NumaMapCopyAssignment) {
  base::NumaMap orig = base::NumaMap::BuildFromSystemTopology();
  base::NumaMap copy;
  copy = orig;

  EXPECT_EQ(copy.NumCpus(), orig.NumCpus());
  for (int cpu = 0; cpu < orig.NumCpus(); ++cpu) {
    EXPECT_EQ(copy.NumaDomainForCpu(cpu), orig.NumaDomainForCpu(cpu));
  }
}

TEST(NumaMapMoveAssignment) {
  base::NumaMap orig = base::NumaMap::BuildFromSystemTopology();
  int orig_cpus = orig.NumCpus();
  int first_numa = orig_cpus > 0 ? orig.NumaDomainForCpu(0) : -1;

  base::NumaMap moved;
  moved = std::move(orig);

  EXPECT_EQ(moved.NumCpus(), orig_cpus);
  if (orig_cpus > 0) {
    EXPECT_EQ(moved.NumaDomainForCpu(0), first_numa);
  }
}

}  // namespace

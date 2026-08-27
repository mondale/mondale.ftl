#include <set>

#include "base/log_queue_map.h"

namespace base::internal {
namespace {

int CountNumaDomains(const NumaMap& m) {
  std::set<int> domains;
  for (int i = 0; i < m.NumCpus(); ++i) {
    domains.insert(m.NumaDomainForCpu(i));
  }
  return domains.size();
}

std::vector<LogQueue*> BuildMap(
    const std::vector<std::unique_ptr<LogQueue>>& queues, const NumaMap& m) {
  // Invariants and Assumptions:
  // * When there are the same number of CPUs and queues, each CPU is assigned a
  //   queue of its own.
  // * When queue counts, domains, and CPU counts aren't elegantly related
  //   (e.g., don't evenly divide one another), it is acceptable to orphan
  //   queues.
  // * When there are more than kSize queues, each CPU id maps to some queue
  //   used only by other CPUs in the same NUMA domain.
  // * The maximum number of CPUs per Queue is no more than one larger than the
  //   minimum number of CPUs per queue.
  std::vector<LogQueue*> ret(m.NumCpus(), nullptr);
  if (queues.empty()) return ret;

  const int num_qs = queues.size();
  const int num_cpus = m.NumCpus();
  if (num_cpus < 1) return ret;

  // Handle the small CPU count case first.
  if (num_cpus <= num_qs) {
    for (int i = 0; i < num_cpus; ++i) {
      ret[i] = queues[i].get();
    }
    return ret;
  }

  // If there are fewer domains than queues (that's weird) we can handle that
  // too.
  const int num_domains = CountNumaDomains(m);
  if (num_qs < num_domains) {
    const int domains_per_queue = (num_domains + num_qs - 1) / num_qs;
    for (int i = 0; i < num_cpus; ++i) {
      const int d = m.NumaDomainForCpu(i);
      const int qn = d / domains_per_queue;
      ret[i] = queues[qn].get();
    }
    return ret;
  }

  // There are more CPUs than queues. We'll need to worry about NUMA domains.
  const int queues_per_domain = num_qs / num_domains;
  for (int i = 0; i < num_cpus; ++i) {
    const int d = m.NumaDomainForCpu(i);
    const int qn = d * queues_per_domain + (i % queues_per_domain);
    ret[i] = queues[qn].get();
  }

  return ret;
}

}  // namespace

LogQueueMap::LogQueueMap(const std::vector<std::unique_ptr<LogQueue>>& queues,
                         const NumaMap& m)
    : map_(BuildMap(queues, m)) {}

}  // namespace base::internal

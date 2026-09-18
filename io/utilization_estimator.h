#ifndef IO_UTILIZATION_ESTIMATOR_H_
#define IO_UTILIZATION_ESTIMATOR_H_

#include <array>
#include <cstdint>

namespace io {

class UtilizationEstimator final {
 public:
  static constexpr int N = 4;

  UtilizationEstimator() = default;

  void Sample(int64_t busy, int64_t idle) {
    const int64_t t = busy + idle;
    if (0 == t) return;
    const int est = static_cast<int>(busy * 100 / t);
    last_++;
    if (last_ >= N) last_ = 0;
    samples_[last_] = est;
  }

  int Estimate() const {
    int t = 0;
    int ws = 0;
    for (int i = 0; i < N; ++i) {
      const int index = ToIndex(i);
      const int w = ToWeight(i);
      ws += ToWeight(i);
      t += samples_[index] * w;
    }
    return (ws == 0) ? 0 : (t / ws);
  }

 private:
  int ToIndex(int i) const {
    if ((last_ - i) >= 0) return last_ - i;
    return last_ + N - i;
  }

  int ToWeight(int i) const { return N - i; }

  int last_ = 0;
  std::array<int, N> samples_{};
};

}  // namespace io

#endif  // #ifndef IO_UTILIZATION_ESTIMATOR_H_

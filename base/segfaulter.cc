#include <chrono>
#include <iostream>
#include <thread>

#include "base/process.h"
#include "base/thread.h"

// This stack is spinning.
__attribute__((noinline)) void SpinOne() {
  while (true) continue;
}
__attribute__((noinline)) void SpinTwo() { SpinOne(); }
__attribute__((noinline)) void SpinThree() { SpinTwo(); }
__attribute__((noinline)) void Spin() { SpinThree(); }

// This stack is blocked on stdin.
__attribute__((noinline)) void BlockOne() {
  char c;
  std::cin >> c;
}
__attribute__((noinline)) void BlockTwo() { BlockOne(); }
__attribute__((noinline)) void BlockThree() { BlockTwo(); }
__attribute__((noinline)) void Block() { BlockThree(); }

// This stack segfaults.
__attribute__((noinline)) void SegfaultOne() {
  *reinterpret_cast<char*>(&SegfaultOne) = 8;
}
__attribute__((noinline)) void SegfaultTwo() { SegfaultOne(); }
__attribute__((noinline)) void SegfaultThree() { SegfaultTwo(); }
__attribute__((noinline)) void DoingASegfault() { SegfaultThree(); }

int main(int argc, char* argv[]) {
  using namespace std::chrono_literals;
  base::Initialize(argc, argv);
  std::thread blocker(&Block);
  auto spinner = base::CreateThread("Spinner", []() { Spin(); });
  std::this_thread::sleep_for(500ms);
  DoingASegfault();
  return 0;
}

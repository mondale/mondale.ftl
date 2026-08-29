#include <sys/syscall.h>
#include <unistd.h>

#include "base/thread_registry.h"
#include "testing/testing.h"

TEST(ThreadRegistryBasic) {
  pid_t tid = static_cast<pid_t>(syscall(SYS_gettid));
  base::ThreadRegistry* reg = base::ThreadRegistry::GetOrCreate();

  char buf[256] = {};
  reg->Add(tid, "test_thread");

  bool found = reg->Lookup(tid, buf, sizeof(buf));
  EXPECT_TRUE(found);
  EXPECT_EQ(std::string_view(buf), "test_thread");

  reg->Remove(tid);
  EXPECT_FALSE(reg->Lookup(tid, buf, sizeof(buf)));
}

TEST(ScopedThreadRegistrationBasic) {
  pid_t tid = static_cast<pid_t>(syscall(SYS_gettid));

  {
    base::ScopedThreadRegistration scoped_reg("scoped_worker");
    char buf[256] = {};
    auto* reg = base::ThreadRegistry::GetNoCreate();
    EXPECT_TRUE(reg != nullptr);

    bool found = reg->Lookup(tid, buf, sizeof(buf));
    EXPECT_TRUE(found);
    EXPECT_EQ(std::string_view(buf), "scoped_worker");
  }

  if (auto* reg = base::ThreadRegistry::GetNoCreate()) {
    char buf[256] = {};
    EXPECT_FALSE(reg->Lookup(tid, buf, sizeof(buf)));
  }
}

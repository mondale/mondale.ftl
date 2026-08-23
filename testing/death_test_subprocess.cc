#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <string>
#include <thread>

#include "testing/death.h"
#include "testing/death_test_subprocess.h"

namespace testing::internal {
namespace {

void ReadAllNonBlocking(int fd, std::string& out) {
  char buf[256];
  ssize_t bytes;
  while ((bytes = read(fd, buf, sizeof(buf))) > 0) {
    out.append(buf, static_cast<size_t>(bytes));
  }
}

}  // namespace

// static
DeathTestResult DeathTestSubprocess::Execute(std::function<void()> statement) {
  int stdout_pipe[2];
  int stderr_pipe[2];

  if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
    perror("pipe failed");
    return {};
  }

  fflush(stdout);
  fflush(stderr);

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork failed");
    return {};
  }

  if (pid == 0) {  // --- CHILD PROCESS ---
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    dup2(stdout_pipe[1], STDOUT_FILENO);
    dup2(stderr_pipe[1], STDERR_FILENO);

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    statement();  // Run the dead statement

    // If the statement returned normally without dying:
    fflush(stdout);
    fflush(stderr);
    _exit(0);
  }

  // --- PARENT PROCESS ---
  close(stdout_pipe[1]);
  close(stderr_pipe[1]);

  DeathTestResult result;

  // Set pipe read FDs to non-blocking so we don't hang on read() if child
  // stalls
  fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);
  fcntl(stderr_pipe[0], F_SETFL, O_NONBLOCK);

  constexpr auto kTimeout = std::chrono::seconds(30);
  constexpr auto kPollInterval = std::chrono::milliseconds(10);
  auto start_time = std::chrono::steady_clock::now();

  int status = 0;
  bool child_exited = false;

  while (std::chrono::steady_clock::now() - start_time < kTimeout) {
    // Drain output pipes while waiting
    ReadAllNonBlocking(stdout_pipe[0], result.stdout_str);
    ReadAllNonBlocking(stderr_pipe[0], result.stderr_str);

    pid_t res = waitpid(pid, &status, WNOHANG);
    if (res == pid) {
      child_exited = true;
      break;
    } else if (res < 0) {
      perror("waitpid failed");
      break;
    }

    std::this_thread::sleep_for(kPollInterval);
  }

  if (!child_exited) {
    // Process exceeded 30s timeout; kill it
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);  // Reap zombie
    result.timed_out = true;
  }

  // Final drain of any remaining pipe buffer
  ReadAllNonBlocking(stdout_pipe[0], result.stdout_str);
  ReadAllNonBlocking(stderr_pipe[0], result.stderr_str);

  close(stdout_pipe[0]);
  close(stderr_pipe[0]);

  if (!result.timed_out) {
    if (WIFEXITED(status)) {
      result.exited_normal = true;
      result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      result.killed_by_signal = true;
      result.signal_number = WTERMSIG(status);
    }
  }

  return result;
}

}  // namespace testing::internal

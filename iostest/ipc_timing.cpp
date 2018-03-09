// Copyright 2018 leoetlino <leo@leolam.fr>
// Licensed under GPLv2

// Low level IOS IPC timing test.

#include <cinttypes>
#include <cmath>
#include <vector>
#include <ogc/ios.h>

#include "common/CommonTypes.h"
#include "common/hwtests.h"
#include "iostest/ipc.h"

template <typename TestFunction>
static std::vector<ipc::Result> Test(TestFunction function) {
  std::vector<ipc::Result> results;
  for (size_t i = 0; i < 1000; ++i)
    results.emplace_back(function());
  return results;
}

// Test the IPC timing by sending an invalid request that will fail kernel checks
// in its IPC dispatch code very quickly.
static std::vector<ipc::Result> TestInvalidCommandTiming() {
  return Test([] {
    ipc::Request request{};
    request.cmd = ipc::Command(0);
    return ipc::SendRequest(request);
  });
}

// Same test, but with the IPC interrupt.
static std::vector<ipc::Result> TestInvalidCommandTimingWithInterrupt() {
  return Test([] {
    ipc::Request request{};
    request.cmd = ipc::Command(0);
    return ipc::SendRequestWithInterrupt(request);
  });
}

static std::vector<ipc::Result> TestOpenWithFdTableFullTiming() {
  return Test([] {
    for (int i = 0; i < 24; ++i)
      ipc::Open("/dev/stm/immediate", ipc::OpenMode::None);

    const auto res = ipc::Open("/dev/stm/immediate", ipc::OpenMode::None);

    for (int i = 0; i < 24; ++i)
      ipc::Close(i);

    return res;
  });
}

static std::vector<ipc::Result> TestInvalidOpenTiming() {
  return Test([] {
    return ipc::Open("/dev/doesntexist", ipc::OpenMode::None);
  });
}

static std::vector<ipc::Result> TestInvalidFdTiming(int fd) {
  return Test([fd] {
    return ipc::Ioctl(fd, 0, nullptr, 0, nullptr, 0);
  });
}

static void PrintResults(const char* description, const std::vector<ipc::Result>& results) {
  if (results.empty())
    return;

  u64 mean_ack_ticks = 0, mean_reply_ticks = 0;
  for (const ipc::Result& res : results) {
    mean_ack_ticks += res.ack_ticks;
    mean_reply_ticks += res.reply_ticks;
  }
  mean_ack_ticks /= results.size();
  mean_reply_ticks /= results.size();

  double acc_ack = 0.0, acc_reply = 0.0;
  for (const ipc::Result& res : results) {
    acc_ack += (res.ack_ticks - mean_ack_ticks) * (res.ack_ticks - mean_ack_ticks);
    acc_reply += (res.reply_ticks - mean_reply_ticks) * (res.reply_ticks - mean_reply_ticks);
  }
  u32 stdev_ack = std::sqrt(acc_ack / (results.size() - 1));
  u32 stdev_reply = std::sqrt(acc_reply / (results.size() - 1));

  network_printf("\033[1m%s\033[0m (%u tests)\n", description, static_cast<u32>(results.size()));
  network_printf("ack_ticks=%" PRIu64 "\033[2m/%u\033[0m, ", mean_ack_ticks, stdev_ack);
  network_printf("reply_ticks=%" PRIu64 "\033[2m/%u\033[0m\n\n", mean_reply_ticks, stdev_reply);
}

int main() {
  ipc::Init();
  const auto invalid_cmd = TestInvalidCommandTiming();
  const auto invalid_cmd_with_interrupt = TestInvalidCommandTimingWithInterrupt();
  const auto open_with_fd_table_full = TestOpenWithFdTableFullTiming();
  const auto invalid_open = TestInvalidOpenTiming();
  const auto invalid_fd_0 = TestInvalidFdTiming(0);
  const auto invalid_fd_200 = TestInvalidFdTiming(200);
  ipc::Shutdown();

  network_init();
  network_printf("IOS version %u\n\n", IOS_GetVersion());
  PrintResults("Invalid command", invalid_cmd);
  PrintResults("Invalid command (with IPC interrupt)", invalid_cmd_with_interrupt);
  PrintResults("Open with full FD table", open_with_fd_table_full);
  PrintResults("Invalid open", invalid_open);
  PrintResults("Invalid FD (0)", invalid_fd_0);
  PrintResults("Invalid FD (200)", invalid_fd_200);
  network_shutdown();
  return 0;
}

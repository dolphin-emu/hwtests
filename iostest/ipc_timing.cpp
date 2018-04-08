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
#include "iostest/result_printer.h"

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

static void PrintResults(const std::string& description, const std::vector<ipc::Result>& results) {
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

  network_printf("\033[1m%s\033[0m (%u tests)\n", description.c_str(),
                 static_cast<u32>(results.size()));
  network_printf("ack_ticks=%" PRIu64 "\033[2m/%u\033[0m, ", mean_ack_ticks, stdev_ack);
  network_printf("reply_ticks=%" PRIu64 "\033[2m/%u\033[0m\n\n", mean_reply_ticks, stdev_reply);
}

int main() {
  ResultPrinter<std::vector<ipc::Result>> results;
  ipc::Init();
  results.Add("Invalid command", TestInvalidCommandTiming());
  results.Add("Invalid command (with IPC interrupt)", TestInvalidCommandTimingWithInterrupt());
  results.Add("Open with full FD table", TestOpenWithFdTableFullTiming());
  results.Add("Invalid open", TestInvalidOpenTiming());
  results.Add("Invalid FD (0)", TestInvalidFdTiming(0));
  results.Add("Invalid FD (200)", TestInvalidFdTiming(2000));
  ipc::Shutdown();

  network_init();
  network_printf("IOS version %u\n\n", IOS_GetVersion());
  results.Print(PrintResults);
  network_shutdown();
  return 0;
}

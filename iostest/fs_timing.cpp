// Copyright 2018 leoetlino <leo@leolam.fr>
// Licensed under GPLv2

// IOS FS timing test.

#include <cinttypes>
#include <cstring>
#include <numeric>
#include <string>
#include <vector>

#include <ogc/ios.h>
#include <unistd.h>

#include "common/CommonTypes.h"
#include "common/hwtests.h"
#include "iostest/fs.h"
#include "iostest/ipc.h"
#include "iostest/result_printer.h"

namespace {
enum class Expect {
  Success,
  Failure,
};

std::vector<u64> TestOpens(const char* path, Expect expect) {
  std::vector<u64> ticks;
  for (size_t i = 0; i < 100; ++i) {
    const ipc::Result open = ipc::Open(path, ipc::OpenMode::Read);
    if (open.result < 0 && expect == Expect::Success)
      return {};
    if (open.result >= 0 && ipc::Close(open.result).result < 0)
      return {};
    ticks.emplace_back(open.reply_ticks);
  }
  return ticks;
}

std::vector<u64> TestCloses() {
  std::vector<u64> ticks;
  for (size_t i = 0; i < 100; ++i) {
    const int fd = ipc::Open("/shared2/sys/SYSCONF", ipc::OpenMode::Read).result;
    if (fd < 0)
      return {};
    const ipc::Result close = ipc::Close(fd);
    if (close.result < 0)
      return {};
    ticks.emplace_back(close.reply_ticks);
  }
  return ticks;
}

alignas(64) static u8 buffer[0x8000];

enum class ReadMode
{
  Cached,
  Uncached,
};
// size must be <= 0x8000
std::vector<u64> TestReads(const char* path, size_t size, ReadMode mode) {
  std::vector<u64> ticks;
  for (size_t i = 0; i < 100; ++i) {
    const int fd = ipc::Open(path, ipc::OpenMode::Read).result;
    if (fd < 0)
      return {};

    if (mode == ReadMode::Cached) {
      // Bring the file into the cache
      if (ipc::Read(fd, buffer, 0x1).result < 0 || ipc::Seek(fd, 0, ipc::SeekMode::Set).result < 0)
        return {};
    }

    const ipc::Result read = ipc::Read(fd, buffer, std::min(size, sizeof(buffer)));
    if (read.result < 0)
      return {};
    ticks.emplace_back(read.reply_ticks);
    if (ipc::Close(fd).result < 0)
      return {};
  }
  return ticks;
}

std::pair<std::vector<u64>, std::vector<u64>> TestFileCreationAndDeletion() {
  const int fd = ipc::Open("/dev/fs", ipc::OpenMode::None).result;
  if (fd < 0)
    return {};

  std::vector<u64> create_ticks, delete_ticks;
  [&] {
    for (size_t i = 0; i < 20; ++i) {
      const ipc::Result create_res = fs::CreateFile(fd, "/tmp/test", 3, 0, 0, 0);
      if (create_res.result < 0)
        return;
      create_ticks.emplace_back(create_res.reply_ticks);

      const ipc::Result delete_res = fs::DeleteFile(fd, "/tmp/test");
      if (delete_res.result < 0)
        return;
      delete_ticks.emplace_back(delete_res.reply_ticks);
    }
  }();
  ipc::Close(fd);
  return std::make_pair(std::move(create_ticks), std::move(delete_ticks));
}

enum class WriteFileMode {
  BlankFile,
  NonEmptyFile,
};
std::pair<std::vector<u64>, std::vector<u64>> TestWrites(size_t size, WriteFileMode mode) {
  size = std::min(size, sizeof(buffer));
  const int fd = ipc::Open("/dev/fs", ipc::OpenMode::None).result;
  if (fd < 0)
    return {};

  std::vector<u64> write_ticks, close_ticks;
  int file_fd = -1;
  [&] {
    for (size_t i = 0; i < 20; ++i) {
      constexpr const char* path = "/tmp/writetest";
      if (fs::CreateFile(fd, path, 3, 0, 0, 0).result < 0)
        return;

      file_fd = ipc::Open(path, ipc::OpenMode::ReadWrite).result;
      if (file_fd < 0)
        return;

      if (mode == WriteFileMode::NonEmptyFile) {
        if (ipc::Write(file_fd, buffer, sizeof(buffer)).result < 0)
          return;
        if (ipc::Close(file_fd).result < 0)
          return;
        file_fd = ipc::Open(path, ipc::OpenMode::ReadWrite).result;
        if (file_fd < 0)
          return;
      }

      const ipc::Result write = ipc::Write(file_fd, buffer, size);
      if (write.result != size)
        return;

      const ipc::Result close = ipc::Close(file_fd);
      if (close.result < 0)
        return;

      write_ticks.emplace_back(write.reply_ticks);
      close_ticks.emplace_back(close.reply_ticks);

      fs::DeleteFile(fd, path);
    }
  }();
  ipc::Close(fd);
  ipc::Close(file_fd);
  return std::make_pair(std::move(write_ticks), std::move(close_ticks));
}

std::vector<u64> TestGetMetadata(const char* path) {
  const int fd = ipc::Open("/dev/fs", ipc::OpenMode::None).result;
  if (fd < 0)
    return {};

  std::vector<u64> ticks;
  fs::ISFSParams metadata;
  for (size_t i = 0; i < 100; ++i) {
    const ipc::Result result = fs::GetMetadata(fd, path, &metadata);
    if (result.result < 0)
    {
      ipc::Close(fd);
      return {};
    }
    ticks.emplace_back(result.reply_ticks);
  }
  ipc::Close(fd);
  return ticks;
}

}  // end of anonymous namespace

int main() {
  ResultPrinter<std::vector<u64>> results;

  ipc::Init();
  results.Add("Open /invalid/path/", TestOpens("/invalid/path/", Expect::Failure));
  results.Add("Open /title", TestOpens("/title", Expect::Failure));
  results.Add("Open /title/00000001", TestOpens("/title/00000001", Expect::Failure));
  results.Add("Open /sys/cert.sys", TestOpens("/sys/cert.sys", Expect::Success));
  results.Add("Open /shared2/sys/SYSCONF", TestOpens("/shared2/sys/SYSCONF", Expect::Success));
  results.Add("Close (no cache flush)", TestCloses());
  auto test_reads = [&](const char* path, ReadMode mode) {
    for (const size_t size : {0x0, 0x4, 0x100, 0x200, 0x300, 0x800, 0x1000, 0x2000, 0x4000}) {
      const std::string mode_string = mode == ReadMode::Cached ? "cached" : "uncached";
      results.Add("Read " + std::to_string(size) + " bytes (" + mode_string + ")",
                  TestReads(path, size, mode));
    }
  };
  // These are expected to all yield pretty much the same result, because IOS has
  // to read a full cluster (0x4000) from the NAND.
  test_reads("/shared2/sys/SYSCONF", ReadMode::Uncached);
  // These are expected to take linear time because FS has already read the cluster
  // and all it needs to do is memcpy it to PPC memory.
  test_reads("/shared2/sys/SYSCONF", ReadMode::Cached);

  auto create_and_delete_results = TestFileCreationAndDeletion();
  results.Add("CreateFile", std::move(create_and_delete_results.first));
  results.Add("DeleteFileOrDirectory (file)", std::move(create_and_delete_results.second));
  results.Add("GetMetadata /title", TestGetMetadata("/title"));
  results.Add("GetMetadata /title/00000001", TestGetMetadata("/title/00000001"));
  results.Add("GetMetadata /sys/cert.sys", TestGetMetadata("/sys/cert.sys"));
  results.Add("GetMetadata /shared2/sys/SYSCONF", TestGetMetadata("/shared2/sys/SYSCONF"));

  auto test_writes = [&](WriteFileMode mode) {
    for (const size_t size : {0x0, 0x4, 0x100, 0x1000, 0x2000, 0x4000, 0x4001, 0x8000}) {
      const std::string mode_string = mode == WriteFileMode::BlankFile ? "blank" : "non-empty";
      auto write_and_close_results = TestWrites(size, mode);
      results.Add("Write " + std::to_string(size) + " bytes (" + mode_string + ")",
                  std::move(write_and_close_results.first));
      results.Add("  -- Flush", std::move(write_and_close_results.second));
    }
  };
  test_writes(WriteFileMode::BlankFile);
  test_writes(WriteFileMode::NonEmptyFile);

  ipc::Shutdown();

  network_init();
  network_printf("IOS version %u\n\n", IOS_GetVersion());
  results.Print([](const std::string& description, const std::vector<u64>& ticks) {
    if (ticks.empty())
    {
      network_printf("\e[0;34m%s\e[0;0m: error\n", description.c_str());
      return;
    }
    network_printf("\e[0;34m%s\e[0;0m: reply_ticks=%" PRIu64"  (%u tests)\n",
                   description.c_str(),
                   std::accumulate(ticks.begin(), ticks.end(), u64(0)) / ticks.size(),
                   static_cast<u32>(ticks.size()));
  });
  network_shutdown();
  return 0;
}

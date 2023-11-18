// Copyright 2018 leoetlino <leo@leolam.fr>
// Licensed under GPLv2

// IPC functions (with timing information) to communicate with IOS.
// Please ensure that the IPC interrupt is masked to prevent libogc from interfering.

#pragma once

#include <array>

#include "Common/CommonTypes.h"

namespace ipc {

enum class Command : u32 {
  Open = 1,
  Close = 2,
  Read = 3,
  Write = 4,
  Seek = 5,
  Ioctl = 6,
  Ioctlv = 7,
  /// Not a valid command -- IOS sets the command to CMD_REPLY in the reply.
  Reply = 8,
};

enum class OpenMode : u32
{
  None = 0,
  Read = 1,
  Write = 2,
  ReadWrite = 3,
};

enum class SeekMode : u32
{
  Set = 0,
  Current = 1,
  End = 2,
};

struct IoVector
{
  void* data;
  u32 size;
};

struct Request {
  Command cmd;
  int result;
  int fd;
  std::array<u32, 5> arg;
  std::array<u32, 8> user;
};
static_assert(sizeof(Request) == 0x40, "Wrong size");

struct Result {
  /// TB ticks it took for IOS to acknowledge the IPC request.
  u64 ack_ticks;
  /// TB ticks it took for IOS to reply to the IPC request after acknowledging it.
  u64 reply_ticks;
  /// Result code from IOS
  int result;
};

void Init();
void Shutdown();

Result SendRequest(const Request& request);
Result SendRequestWithInterrupt(const Request& request);

Result Open(const char* path, OpenMode mode);
Result Close(int fd);
Result Read(int fd, void* buffer, size_t count);
Result Write(int fd, void* buffer, size_t count);
Result Seek(int fd, u32 offset, SeekMode mode);
Result Ioctl(int fd, u32 number, void* in, size_t in_size, void* out, size_t out_size);
Result Ioctlv(int fd, u32 number, u32 in_count, u32 io_count, IoVector* vectors);

}  // namespace ipc

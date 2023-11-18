// Copyright 2018 leoetlino <leo@leolam.fr>
// Licensed under GPLv2

#include "iostest/ipc.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <ogc/ios.h>
#include <ogc/ipc.h>
#include <ogc/irq.h>

#include "Common/CommonTypes.h"
#include "Common/timebase.h"

namespace ipc {

/* This code comes from HBC's stub which was based on the Twilight Hack code */
// Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Copyright 2008-2009  Andre Heider  <dhewg@wiibrew.org>
// Copyright 2008-2009  Hector Martin  <marcan@marcansoft.com>
#define virt_to_phys(x) ((uintptr_t*)(((uintptr_t)(x))&0x3FFFFFFF))
#define phys_to_virt(x) ((uintptr_t*)(((uintptr_t)(x))|0x80000000))
static void sync_before_read(void* p, u32 len) {
  uintptr_t a = (uintptr_t)p & ~0x1f;
  uintptr_t b = ((uintptr_t)p + len + 0x1f) & ~0x1f;

  for (; a < b; a += 32)
    asm("dcbi 0,%0" : : "b"(a) : "memory");

  asm("sync ; isync");
}

static void sync_after_write(const void* p, u32 len) {
  uintptr_t a = (uintptr_t)p & ~0x1f;
  uintptr_t b = ((uintptr_t)p + len + 0x1f) & ~0x1f;

  for (; a < b; a += 32)
    asm("dcbf 0,%0" : : "b"(a));

  asm("sync ; isync");
}

enum Register : u32 {
  HW_IPC_PPCMSG = 0x0d800000,
  HW_IPC_PPCCTRL = 0x0d800004,
  HW_IPC_ARMMSG = 0x0d800008,
  HW_PPCIRQFLAG = 0x0d800030,
};

static u32 iread32(Register addr) {
  u32 x;
  asm volatile("lwz %0,0(%1) ; sync ; isync" : "=r"(x) : "b"(0xc0000000 | addr));
  return x;
}

static void iwrite32(Register addr, u32 x) {
  asm volatile("stw %0,0(%1) ; eieio" : : "r"(x), "b"(0xc0000000 | addr));
}

static void ipc_bell(u32 w) {
  iwrite32(HW_IPC_PPCCTRL, w);
}

static void ipc_wait_ack() {
  while (!(iread32(HW_IPC_PPCCTRL) & 0x2));  // wait for Y2
}

static void ipc_wait_reply() {
  while (!(iread32(HW_IPC_PPCCTRL) & 0x4));  // wait for Y1
}

static void ipc_ack_interrupt() {
  iwrite32(HW_PPCIRQFLAG, 0x40000000);
}

alignas(64) static Request s_ipc_request{};

Result SendRequestWithInterrupt(const Request& request) {
  struct Context {
    u64 ack_tb;
    u64 reply_tb;
    std::atomic_bool complete{false};
  } context{};

  IRQ_Request(IRQ_PI_ACR, [](u32, void* userdata) {
    auto* context = static_cast<Context*>(userdata);

    if ((iread32(HW_IPC_PPCCTRL) & (1 << 5 | 1 << 1)) == (1 << 5 | 1 << 1)) {
      context->ack_tb = GetTimebase();
      ipc_bell(1 << 4 | 1 << 1);  // set IY1 and clear Y2
      ipc_ack_interrupt();
    }

    if ((iread32(HW_IPC_PPCCTRL) & (1 << 4 | 1 << 2)) == (1 << 4 | 1 << 2)) {
      context->reply_tb = GetTimebase();
      ipc_bell(1 << 2);  // clear Y1
      ipc_bell(1 << 3);
      ipc_ack_interrupt();  // must be done after clearing Y1
      context->complete.store(true);
    }
  }, &context);
  __UnmaskIrq(IRQ_PI_ACR);

  s_ipc_request = request;
  sync_after_write(&s_ipc_request, sizeof(s_ipc_request));
  iwrite32(HW_IPC_PPCMSG, uintptr_t(virt_to_phys(&s_ipc_request)));
  const u64 start_tb = GetTimebase();
  ipc_bell((1 << 5) | (1 << 0));  // set X1 and IY2

  while (!context.complete);
  __MaskIrq(IRQ_PI_ACR);
  IRQ_Free(IRQ_PI_ACR);

  const u32 reply = iread32(HW_IPC_ARMMSG);
  assert(uintptr_t(reply) == uintptr_t(virt_to_phys(&s_ipc_request)));
  sync_before_read(&s_ipc_request, sizeof(s_ipc_request));

  Result result{};
  result.ack_ticks = context.ack_tb - start_tb;
  result.reply_ticks = context.reply_tb - context.ack_tb;
  result.result = s_ipc_request.result;
  return result;
}

Result SendRequest(const Request& request) {
  s_ipc_request = request;

  // Send the request
  sync_after_write(&s_ipc_request, sizeof(s_ipc_request));
  iwrite32(HW_IPC_PPCMSG, uintptr_t(virt_to_phys(&s_ipc_request)));
  const u64 start_tb = GetTimebase();
  ipc_bell(1 << 0);  // set X1

  // Wait for IOS to acknowledge the request
  ipc_wait_ack();
  const u64 ack_tb = GetTimebase();
  ipc_bell(1 << 1);  // clear Y2

  // Wait for IOS to reply
  ipc_wait_reply();
  const u64 reply_tb = GetTimebase();
  const u32 reply = iread32(HW_IPC_ARMMSG);
  assert(uintptr_t(reply) == uintptr_t(virt_to_phys(&s_ipc_request)));
  ipc_bell(1 << 2);  // clear Y1
  ipc_bell(1 << 3);

  // Read the reply
  sync_before_read(&s_ipc_request, sizeof(s_ipc_request));

  Result result{};
  result.ack_ticks = ack_tb - start_tb;
  result.reply_ticks = reply_tb - ack_tb;
  result.result = s_ipc_request.result;
  return result;
}

static raw_irq_handler_t s_previous_ipc_handler;

void Init() {
  __IOS_ShutdownSubsystems();
  __MaskIrq(IRQ_PI_ACR);
  s_previous_ipc_handler = IRQ_Free(IRQ_PI_ACR);
}

void Shutdown() {
  IRQ_Request(IRQ_PI_ACR, s_previous_ipc_handler, nullptr);
  __UnmaskIrq(IRQ_PI_ACR);
  __IPC_Reinitialize();
  __IOS_InitializeSubsystems();
}

Result Open(const char* path, OpenMode mode) {
  Request request{};
  request.cmd = Command::Open;
  request.arg[0] = uintptr_t(virt_to_phys(path));
  request.arg[1] = u32(mode);
  sync_after_write(path, 0x40);
  return SendRequest(request);
}

Result Close(int fd) {
  Request request{};
  request.cmd = Command::Close;
  request.fd = fd;
  return SendRequest(request);
}

Result Read(int fd, void* buffer, size_t count) {
  Request request{};
  request.cmd = Command::Read;
  request.fd = fd;
  request.arg[0] = uintptr_t(virt_to_phys(buffer));
  request.arg[1] = count;
  const Result result = SendRequest(request);
  sync_before_read(buffer, count);
  return result;
}

Result Write(int fd, void* buffer, size_t count) {
  Request request{};
  request.cmd = Command::Write;
  request.fd = fd;
  request.arg[0] = uintptr_t(virt_to_phys(buffer));
  request.arg[1] = count;
  sync_after_write(buffer, count);
  return SendRequest(request);
}

Result Seek(int fd, u32 offset, SeekMode mode) {
  Request request{};
  request.cmd = Command::Seek;
  request.fd = fd;
  request.arg[0] = offset;
  request.arg[1] = u32(mode);
  return SendRequest(request);
}

Result Ioctl(int fd, u32 number, void* in, size_t in_size, void* out, size_t out_size) {
  Request request{};
  request.cmd = Command::Ioctl;
  request.fd = fd;
  request.arg[0] = number;
  request.arg[1] = uintptr_t(virt_to_phys(in));
  request.arg[2] = in_size;
  request.arg[3] = uintptr_t(virt_to_phys(out));
  request.arg[4] = out_size;

  if (in)
    sync_after_write(in, in_size);
  if (out)
    sync_after_write(out, out_size);

  const Result result = SendRequest(request);

  if (in)
    sync_before_read(in, in_size);
  if (out)
    sync_before_read(out, out_size);

  return result;
}

Result Ioctlv(int fd, u32 number, u32 in_count, u32 io_count, IoVector* vectors) {
  Request request{};
  request.cmd = Command::Ioctlv;
  request.fd = fd;
  request.arg[0] = number;
  request.arg[1] = in_count;
  request.arg[2] = io_count;
  request.arg[3] = uintptr_t(virt_to_phys(vectors));

  for (size_t i = 0; i < in_count + io_count; ++i) {
    if (vectors[i].data) {
      sync_after_write(vectors[i].data, vectors[i].size);
      vectors[i].data = virt_to_phys(vectors[i].data);
    }
  }
  sync_after_write(vectors, sizeof(IoVector) * (in_count + io_count));

  const Result result = SendRequest(request);

  for (size_t i = 0; i < in_count + io_count; ++i) {
    if (vectors[i].data)
      sync_before_read(vectors[i].data, vectors[i].size);
  }

  return result;
}

}  // namespace ipc

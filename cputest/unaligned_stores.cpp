#include <algorithm>
#include <cinttypes>

#include <gctypes.h>
#include <ogc/irq.h>
#include <ogc/system.h>
#include <wiiuse/wpad.h>

#include "common/BitUtils.h"
#include "common/hwtests.h"

// This test covers the behavior described in https://bugs.dolphin-emu.org/issues/12565

constexpr size_t PAGE_SIZE = 4096;

static volatile u32* const s_pi_reg = reinterpret_cast<volatile u32*>(0xCC003000);

static volatile bool s_pi_error_occurred = false;

static u32 Read(volatile u8* ptr)
{
  return *reinterpret_cast<volatile u32*>(ptr);
}

static void Write(volatile u8* ptr, u32 value, u32 size)
{
  switch (size)
  {
  case 4:
    asm volatile("stw %0, 0(%1)" :: "r"(value), "r"(ptr));
    break;
  case 2:
    asm volatile("sth %0, 0(%1)" :: "r"(value), "r"(ptr));
    break;
  case 1:
    asm volatile("stb %0, 0(%1)" :: "r"(value), "r"(ptr));
    break;
  }
}

static void WriteWithSimulatedQuirks(uintptr_t alignment, volatile u8* ptr, u32 value, u32 size)
{
  const uintptr_t misalignment_32 = alignment & 3;

  if (misalignment_32 || size < 4)
  {
    const uintptr_t misalignment_64 = alignment & 7;
    const uintptr_t count = misalignment_64 + size;

    value = Common::RotateRight(value, (misalignment_32 + size) * 8);

    for (uintptr_t i = 0; i < count; i += 8)
    {
      Write(ptr - misalignment_64 + i, value, sizeof(u32));
      Write(ptr - misalignment_64 + i + 4, value, sizeof(u32));
    }
  }
  else
  {
    Write(ptr, value, size);
  }
}

static void UnalignedStoresTest(volatile u8* ptr, u32 size, bool cached_memory)
{
  network_printf("Starting test using ptr 0x%" PRIxPTR ", size %u\n",
                 reinterpret_cast<uintptr_t>(ptr), size);

  volatile u8 reference_buffer[32];
  const u32 word = 0x12345678;

  for (size_t i = 0; i <= 28; ++i)
  {
    // Use 32-bit writes to avoid accidentally triggering the behavior we're trying to test
    std::fill(reinterpret_cast<volatile u32*>(ptr), reinterpret_cast<volatile u32*>(ptr + 32), 0);
    std::fill(reference_buffer, reference_buffer + 32, 0);

    s_pi_error_occurred = false;

    // The actual write that we are testing the behavior of
    Write(ptr + i, word, size);

    if (cached_memory)
      Write(reference_buffer + i, word, size);
    else
      WriteWithSimulatedQuirks(i, reference_buffer + i, word, size);

    DO_TEST(std::equal(ptr, ptr + 32, reference_buffer),
            "{}-byte write to {} failed\n"
            "ACTUAL:              EXPECTED:\n"
            "{:08X} {:08X}    {:08X} {:08X}\n"
            "{:08X} {:08X}    {:08X} {:08X}\n"
            "{:08X} {:08X}    {:08X} {:08X}\n"
            "{:08X} {:08X}    {:08X} {:08X}\n",
            size, fmt::ptr(ptr + i),
            Read(ptr), Read(ptr + 4), Read(reference_buffer), Read(reference_buffer + 4),
            Read(ptr + 8), Read(ptr + 12), Read(reference_buffer + 8), Read(reference_buffer + 12),
            Read(ptr + 16), Read(ptr + 20), Read(reference_buffer + 16), Read(reference_buffer + 20),
            Read(ptr + 24), Read(ptr + 28), Read(reference_buffer + 24), Read(reference_buffer + 28));

    // TODO: Why does this trigger even for aligned 32-bit writes? Might be a bug in the test code
    int pi_error_expected = !cached_memory /* && ((i & 3) || size < 4) */;
    int pi_error_actual = s_pi_error_occurred;

    DO_TEST(pi_error_actual == pi_error_expected,
            "{}-byte write to 0x{:08x} failed\n"
            "ACTUAL:              EXPECTED:\n"
            "pi_error_occurred={}  pi_error_occurred={}\n",
            size, reinterpret_cast<uintptr_t>(ptr + i),
            pi_error_actual, pi_error_expected);
  }
}

static void PIErrorHandler([[maybe_unused]] u32 nIrq, [[maybe_unused]] void* pCtx)
{
  // Clear the interrupt
  s_pi_reg[0] = 0x01;

  s_pi_error_occurred = true;
}

static void Initialize()
{
  network_init();

  // Initialise the video system
  VIDEO_Init();

  // This function initialises the attached controllers
  WPAD_Init();

  // Obtain the preferred video mode from the system
  // This will correspond to the settings in the Wii menu
  GXRModeObj* rmode = VIDEO_GetPreferredMode(NULL);

  // Allocate memory for the display in the uncached region
  void* xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

  // Initialise the console, required for printf
  console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

  // Set up the video registers with the chosen mode
  VIDEO_Configure(rmode);

  // Tell the video hardware where our display memory is
  VIDEO_SetNextFramebuffer(xfb);

  // Make the display visible
  VIDEO_SetBlack(FALSE);

  // Flush the video register changes to the hardware
  VIDEO_Flush();

  // Wait for Video setup to complete
  VIDEO_WaitVSync();
  if (rmode->viTVMode & VI_NON_INTERLACE)
    VIDEO_WaitVSync();
}

int main()
{
  Initialize();

  network_printf("Setting up PI ERROR interrupt handler...\n");
  IRQ_Request(IRQ_PI_ERROR, PIErrorHandler, nullptr);
  __UnmaskIrq(IM_PI_ERROR);

  // Get a pointer to a page boundary, with at least 32 bytes available on each side of the boundary
  network_printf("Allocating memory...\n");
  u8* memory_allocation = new u8[PAGE_SIZE + 32 * 4];

  volatile u8* cached_ptr = reinterpret_cast<volatile u8*>(
          reinterpret_cast<uintptr_t>(memory_allocation - 32) & ~(PAGE_SIZE - 1));

  UnalignedStoresTest(cached_ptr - 16, 1, true);
  UnalignedStoresTest(cached_ptr - 16, 2, true);
  UnalignedStoresTest(cached_ptr - 16, 4, true);

  network_printf("Invalidating cache...\n");
  asm volatile("dcbi %0, %1" :: "r"(cached_ptr), "r"(-32));
  asm volatile("dcbi %0, %1" :: "r"(cached_ptr), "r"(0));

  volatile u8* uncached_ptr = reinterpret_cast<volatile u8*>(MEM_K0_TO_K1(cached_ptr));

  UnalignedStoresTest(uncached_ptr - 16, 1, false);
  UnalignedStoresTest(uncached_ptr - 16, 2, false);
  UnalignedStoresTest(uncached_ptr - 16, 4, false);

  delete[] memory_allocation;

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

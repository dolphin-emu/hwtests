#include <gctypes.h>
#include <wiiuse/wpad.h>
#include "common/hwtests.h"

// Check what the Broadway does if we set reserved bits in the graphics quantization registers. The
// CPU manual marks these as zero, but this isn't actually true.
// Some games do this (e.g. Dirt 2) and rely on correct emulation behavior.
static void GQRUnusedBitsTest()
{
  START_TEST();
  uint32_t output;
  // based on hwtest on Wii
  uint32_t expected = 0xFFFFFFFF;
  asm("li   %0, -1;"
      "mtspr 914, %0;"
      "mfspr %0, 914;"
      : "=r"(output));

  DO_TEST(output == expected, "got {:x}, expected {:x}", output, expected);
  END_TEST();
}

// Check what the Broadway does if we set reserved bits in the XER (fixed-point exception) register.
static void XERUnusedBitsTest()
{
  START_TEST();
  uint32_t output;
  // based on hwtest on Wii
  uint32_t expected = 0xE000FF7F;
  asm("li   %0, -1;"
      "mtspr 1, %0;"
      "mfspr %0, 1;"
      : "=r"(output));

  DO_TEST(output == expected, "got {:x}, expected {:x}", output, expected);
  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  GQRUnusedBitsTest();
  XERUnusedBitsTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

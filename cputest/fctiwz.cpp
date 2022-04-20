#include <gctypes.h>
#include <wiiuse/wpad.h>
#include "common/hwtests.h"

// Float Convert To Integer Word with round-to-Zero
static void FctiwzTest()
{
  START_TEST();
  u64 values[][2] = {
      // input               expected output
      {0x0000000000000000, 0xfff8000000000000},  // +0
      {0x8000000000000000, 0xfff8000100000000},  // -0 (!)
      {0x0000000000000001, 0xfff8000000000000},  // smallest positive subnormal
      {0x000fffffffffffff, 0xfff8000000000000},  // largest subnormal
      {0x3ff0000000000000, 0xfff8000000000001},  // +1
      {0xbff0000000000000, 0xfff80000ffffffff},  // -1
      {0xc1e0000000000000, 0xfff8000080000000},  // -(2^31)
      {0x41dfffffffc00000, 0xfff800007fffffff},  // 2^31 - 1
      {0x7ff0000000000000, 0xfff800007fffffff},  // +infinity
      {0xfff0000000000000, 0xfff8000080000000},  // -infinity
      {0xfff8000000000000, 0xfff8000080000000},  // a QNaN
      {0xfff4000000000000, 0xfff8000080000000},  // a SNaN
  };
  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u64 input = values[i][0];
    u64 expected = values[i][1];
    u64 result = 0;
    asm("fctiwz %0, %1" : "=f"(result) : "f"(input));
    DO_TEST(result == expected, "fctiwz(0x{:016x}):\n"
                                "     got 0x{:016x}\n"
                                "expected 0x{:016x}",
            input, result, expected);
  }
  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  FctiwzTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

#include <gctypes.h>
#include <wiiuse/wpad.h>
#include "Common/hwtests.h"

// Float Round to Single Precision
// TODO: check ps1
static void FrspTest()
{
  START_TEST();
  u64 values[][3] = {
      // input               expected output   NI RN
      {0x0000000000000000, 0x0000000000000000, 0b000},  // +0
      {0x8000000000000000, 0x8000000000000000, 0b000},  // -0
      {0x0000000000000001, 0x0000000000000000, 0b000},  // smallest positive double subnormal
      {0x000fffffffffffff, 0x0000000000000000, 0b000},  // largest double subnormal
      {0x3690000000000000, 0x0000000000000000, 0b000},  // largest number rounded to zero
      {0x3690000000000001, 0x36a0000000000000, 0b000},  // smallest positive single subnormal
      {0x380fffffe0000001, 0x0000000000000000, 0b110},  // boundary of architecture-dependent behavior in non-IEEE mode
      {0x380ffffff0000000, 0x0000000000000000, 0b100},  // boundary of architecture-dependent behavior in non-IEEE mode
      {0x380fffffffffffff, 0x0000000000000000, 0b100},  // largest single subnormal
      {0x3810000000000000, 0x3810000000000000, 0b100},  // smallest positive single normal
      {0xb80fffffe0000001, 0x8000000000000000, 0b111},  // boundary of architecture-dependent behavior in non-IEEE mode
      {0xb80ffffff0000000, 0x8000000000000000, 0b100},  // boundary of architecture-dependent behavior in non-IEEE mode
      {0xb80fffffffffffff, 0x8000000000000000, 0b100},  // smallest single subnormal
      {0xb810000000000000, 0xb810000000000000, 0b100},  // largest negative single normal
      {0x7ff0000000000000, 0x7ff0000000000000, 0b000},  // +infinity
      {0xfff0000000000000, 0xfff0000000000000, 0b000},  // -infinity
      {0xfff7ffffffffffff, 0xffffffffe0000000, 0b000},  // a SNaN
      {0xffffffffffffffff, 0xffffffffe0000000, 0b000},  // a QNaN
  };
  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    // Set FPSCR[NI] and FPSCR[RN] (and FPSCR[XE] but that's okay).
    asm("mtfsf 7, %0" ::"f"(values[i][2]));

    u64 input = values[i][0];
    u64 expected = values[i][1];
    u64 result = 0;
    asm("frsp %0, %1" : "=d"(result) : "d"(input));
    DO_TEST(result == expected, "frsp(0x{:016x}, NI={}, RN={}):\n"
                                "     got 0x{:016x}\n"
                                "expected 0x{:016x}",
            input, values[i][2] >> 2, values[i][2] & 0b11, result, expected);
  }
  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  FrspTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

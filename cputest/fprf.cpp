#include <gctypes.h>
#include <limits>
#include <wiiuse/wpad.h>
#include "common/BitUtils.h"
#include "common/hwtests.h"

template <typename T>
struct TestInstance
{
  T input;
  u32 arith_fprf;
  u32 compare_fprf;
};

static void TestArith(TestInstance<double> test)
{
  double temp;
  u64 fprf;
  asm volatile("fadd %0, %1, %2" : "=f"(temp) : "f"(test.input), "f"(-0.0));
  asm volatile("mffs %0" : "=f"(fprf));
  fprf >>= 12;
  fprf &= 0x1F;

  const u32 expected = test.arith_fprf;

  DO_TEST(fprf == expected, "Bad arith FPRF for 0x{:016x}:\n"
                            "     got 0x{:x}\n"
                            "expected 0x{:x}",
          Common::BitCast<u64>(test.input), fprf, expected);
}

static void TestArith(TestInstance<float> test)
{
  float temp;
  u64 fprf;
  asm volatile("fadds %0, %1, %2" : "=f"(temp) : "f"(test.input), "f"(-0.0f));
  asm volatile("mffs %0" : "=f"(fprf));
  fprf >>= 12;
  fprf &= 0x1F;

  const u32 expected = test.arith_fprf;

  DO_TEST(fprf == expected, "Bad arith FPRF for 0x{:08x}:\n"
                            "     got 0x{:x}\n"
                            "expected 0x{:x}",
          Common::BitCast<u32>(test.input), fprf, expected);
}

static void TestCompare(TestInstance<double> test, bool c)
{
  // Force the C bit (top bit of FPRF) to a certain value
  double temp;
  const double value_for_setting_c = c ? std::numeric_limits<double>::quiet_NaN() : 0.0;
  asm volatile("fadd %0, %1, %2" : "=f"(temp) : "f"(value_for_setting_c), "f"(0.0));

  u64 fprf;
  u32 cr;
  asm volatile("fcmpo cr0, %0, %1" :: "f"(test.input), "f"(-0.0));
  asm volatile("mffs %0" : "=f"(fprf));
  asm volatile("mfcr %0" : "=r"(cr));
  fprf >>= 12;
  fprf &= 0x1F;
  cr >>= 28;

  u32 expected = test.compare_fprf;

  DO_TEST(cr == expected, "Bad compare CR for 0x{:016x}:\n"
                          "     got 0x{:x}\n"
                          "expected 0x{:x}",
          Common::BitCast<u64>(test.input), cr, expected);

  if (c)
    expected |= 0x10;

  DO_TEST(fprf == expected, "Bad compare FPRF for 0x{:016x}:\n"
                            "     got 0x{:x}\n"
                            "expected 0x{:x}",
          Common::BitCast<u64>(test.input), fprf, expected);
}

static void TestCompare(TestInstance<float> test, bool c)
{
  // Force the C bit (top bit of FPRF) to a certain value
  float temp;
  const float value_for_setting_c = c ? std::numeric_limits<float>::quiet_NaN() : 0.0f;
  asm volatile("fadds %0, %1, %2" : "=f"(temp) : "f"(value_for_setting_c), "f"(0.0f));

  u64 fprf;
  u32 cr;
  asm volatile("fcmpo cr0, %0, %1" :: "f"(test.input), "f"(-0.0f));
  asm volatile("mffs %0" : "=f"(fprf));
  asm volatile("mfcr %0" : "=r"(cr));
  fprf >>= 12;
  fprf &= 0x1F;
  cr >>= 28;

  u32 expected = test.compare_fprf;

  DO_TEST(cr == expected, "Bad compare CR for 0x{:08x}:\n"
                          "     got 0x{:x}\n"
                          "expected 0x{:x}",
          Common::BitCast<u32>(test.input), cr, expected);

  if (c)
    expected |= 0x10;

  DO_TEST(fprf == expected, "Bad compare FPRF for 0x{:08x}:\n"
                            "     got 0x{:x}\n"
                            "expected 0x{:x}",
          Common::BitCast<u32>(test.input), fprf, expected);
}

// Floating-Point Result Flags test for double-precision operations
static void FprfDoubleTest()
{
  START_TEST();

  TestInstance<double> values[] = {
      {std::numeric_limits<double>::quiet_NaN(),   0b10001, 0b0001},
      {-std::numeric_limits<double>::infinity(),   0b01001, 0b1000},
      {-1.0,                                       0b01000, 0b1000},
      {-std::numeric_limits<double>::denorm_min(), 0b11000, 0b1000},
      {-0.0,                                       0b10010, 0b0010},
      {0.0,                                        0b00010, 0b0010},
      {std::numeric_limits<double>::denorm_min(),  0b10100, 0b0100},
      {1.0,                                        0b00100, 0b0100},
      {std::numeric_limits<double>::infinity(),    0b00101, 0b0100},
  };

  // Ensure FPSCR[NI] is not set (we want operations to be able to output denormals)
  asm("mtfsfi 7, 0");

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    TestArith(values[i]);
    TestCompare(values[i], false);
    TestCompare(values[i], true);
  }
  END_TEST();
}

// Floating-Point Result Flags test for single-precision operations
static void FprfSingleTest()
{
  START_TEST();

  TestInstance<float> values[] = {
      {std::numeric_limits<float>::quiet_NaN(),   0b10001, 0b0001},
      {-std::numeric_limits<float>::infinity(),   0b01001, 0b1000},
      {-1.0f,                                     0b01000, 0b1000},
      {-std::numeric_limits<float>::denorm_min(), 0b11000, 0b1000},
      {-0.0f,                                     0b10010, 0b0010},
      {0.0f,                                      0b00010, 0b0010},
      {std::numeric_limits<float>::denorm_min(),  0b10100, 0b0100},
      {1.0f,                                      0b00100, 0b0100},
      {std::numeric_limits<float>::infinity(),    0b00101, 0b0100},
  };

  // Ensure FPSCR[NI] is not set (we want operations to be able to output denormals)
  asm("mtfsfi 7, 0");

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    TestArith(values[i]);
    TestCompare(values[i], false);
    TestCompare(values[i], true);
  }
  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  FprfDoubleTest();
  FprfSingleTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

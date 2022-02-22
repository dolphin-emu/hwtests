#include <gctypes.h>
#include <wiiuse/wpad.h>
#include "common/BitUtils.h"
#include "common/hwtests.h"

static const double zero = Common::BitCast<double>(0ULL);
static const double smallest_denormal = Common::BitCast<double>(1ULL);

static const double large = Common::BitCast<double>(0x7fefffffffffffffULL);
static const double product = Common::BitCast<double>(0x3ccfffffffffffffULL);

static const double smallest_normal = Common::BitCast<double>(0x0010000000000000ULL);
static const double divisor = Common::BitCast<double>(0x4330000000000000ULL);

// Test of the Non-IEEE bit in FPSCR
static void NiTest()
{
  START_TEST();

  for (u32 ni = 0; ni < 2; ++ni)
  {
    // Set FPSCR[NI] (and FPSCR[RN] and FPSCR[XE] but that's okay).
    const u64 mtfsf_input = ni << 2;
    asm volatile("mtfsf 7, %0" ::"f"(mtfsf_input));
    
    const bool inputs_expected_flushed = false;
    const bool outputs_expected_flushed = ni;

    const bool compare_expected = inputs_expected_flushed;
    const bool compare_result = zero >= smallest_denormal;
    DO_TEST(compare_result == compare_expected,
            "Compare zero and denormal (NI={}):\n"
            "     got {}\n"
            "expected {}",
            ni, compare_result, compare_expected);

    const u64 mul_expected = Common::BitCast<u64>(inputs_expected_flushed ? zero : product);
    const u64 mul_result = Common::BitCast<u64>(smallest_denormal * large);
    DO_TEST(mul_result == mul_expected,
            "Multiply denormal and large number (NI={}):\n"
            "     got 0x{:016x}\n"
            "expected 0x{:016x}",
            ni, mul_result, mul_expected);

    const u64 div_expected = Common::BitCast<u64>(outputs_expected_flushed ? zero : smallest_denormal);
    const u64 div_result = Common::BitCast<u64>(smallest_normal / divisor);
    DO_TEST(div_result == div_expected,
            "Divide smallest normal by other normal (NI={}):\n"
            "     got 0x{:016x}\n"
            "expected 0x{:016x}",
            ni, div_result, div_expected);
  }

  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  NiTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

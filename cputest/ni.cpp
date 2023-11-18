#include <gctypes.h>
#include <wiiuse/wpad.h>
#include "Common/BitUtils.h"
#include "Common/hwtests.h"

static const double zero = Common::BitCast<double>(0ULL);
static const double smallest_denormal = Common::BitCast<double>(1ULL);

static const double large = Common::BitCast<double>(0x7fefffffffffffffULL);
static const double product = Common::BitCast<double>(0x3ccfffffffffffffULL);

static const double smallest_normal = Common::BitCast<double>(0x0010000000000000ULL);
static const double divisor = Common::BitCast<double>(0x4330000000000000ULL);

static const double one_minus_ulp = Common::BitCast<double>(0x3fefffffffffffffULL);

static const float zero_single = Common::BitCast<float>(0);
static const float smallest_normal_single = Common::BitCast<float>(0x00800000);
static const float one_minus_ulp_single = Common::BitCast<float>(0x3f7fffff);

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

    const u64 mul2_expected = Common::BitCast<u64>(outputs_expected_flushed ? zero : smallest_normal);
    const u64 mul2_result = Common::BitCast<u64>(smallest_normal * one_minus_ulp);
    DO_TEST(mul2_result == mul2_expected,
            "Multiply smallest normal by 1 minus ULP (NI={}):\n"
            "     got 0x{:016x}\n"
            "expected 0x{:016x}",
            ni, mul2_result, mul2_expected);

    const u32 mul3_expected = Common::BitCast<u32>(outputs_expected_flushed ? zero_single : smallest_normal_single);
    const u32 mul3_result = Common::BitCast<u32>(smallest_normal_single * one_minus_ulp_single);
    DO_TEST(mul3_result == mul3_expected,
            "Multiply smallest normal single by 1 minus ULP (NI={}):\n"
            "     got 0x{:08x}\n"
            "expected 0x{:08x}",
            ni, mul3_result, mul3_expected);
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

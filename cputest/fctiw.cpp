#include <cmath>

#include <gctypes.h>
#include <wiiuse/wpad.h>

#include "Common/BitUtils.h"
#include "Common/CommonFloat.h"
#include "Common/hwtests.h"

// Algorithm adapted from Appendix C.4.2 in PowerPC Microprocessor Family:
// The Programming Environments Manual for 32 and 64-bit Microprocessors
static u64 fctiw_expected(double b, RoundingMode rounding_mode)
{
  const u64 upper_bits = 0xfff8000000000000ull;
  if (std::isnan(b))
    return upper_bits | 0x80000000;

  const u64 bi = Common::BitCast<u64>(b);

  s32 sign = static_cast<s32>((bi & DOUBLE_SIGN) >> 63);
  s32 exp = static_cast<s32>((bi & DOUBLE_EXP) >> 52);
  u64 frac = ((bi & DOUBLE_FRAC) << 11);

  if (exp > 0)
    frac |= 1ull << 63;

  if (exp > 0)
    exp -= 1023;
  else
    exp = -1022;

  bool gbit = false;
  bool rbit = false;
  bool xbit = false;
  for (s64 i = 0; i < 63 - exp; ++i)
  {
    xbit |= rbit;
    rbit = gbit;
    gbit = frac & 1;
    frac >>= 1;
  }

  u32 inc = 0;
  switch (rounding_mode)
  {
  case RoundingMode::Nearest:
    if (gbit && ((frac & 1) || rbit || xbit))
      inc = 1;
    break;
  case RoundingMode::TowardsZero:
    // Nothing
    break;
  case RoundingMode::TowardsPositiveInfinity:
    if (!sign && (gbit || rbit || xbit))
      inc = 1;
    break;
  case RoundingMode::TowardsNegativeInfinity:
    if (sign && (gbit || rbit || xbit))
      inc = 1;
    break;
  }

  frac += inc;

  if (!sign && frac > 0x7fffffff)
  {
    // Positive large operand or +inf
    frac = 0x7fffffff;
  }
  else if (frac > 0x80000000)
  {
    // Negative large operand or -inf
    frac = 0x80000000;
  }
  else if (sign)
  {
    // Appendix C.4.2 does not cast to 32-bit here, but doing so matches
    // Broadway's behavior of setting bit 31 to 1 for negative zeroes.
    // Bits 0-31 are undefined according to appendix C.4.2.
    frac = static_cast<u64>(~static_cast<u32>(frac)) + 1;
  }

  return upper_bits | frac;
}

static void FctiwTestIndividual(u32 i, RoundingMode rounding_mode)
{
  float input = Common::BitCast<float>(i);
  u64 expected = fctiw_expected(input, rounding_mode);
  u64 result = 0;
  asm("fctiw %0, %1" : "=f"(result) : "f"(input));

  DO_TEST(result == expected, "fctiwz 0x{:08x} ({}):\n"
                              "     got 0x%{:16x} ({})\n"
                              "expected 0x%{:16x} ({})",
          i, input, result, static_cast<s32>(result), expected, static_cast<s32>(expected));
}

static void FctiwTestBothSigns(u32 i, RoundingMode rounding_mode)
{
  FctiwTestIndividual(i, rounding_mode);
  FctiwTestIndividual(FLOAT_SIGN | i, rounding_mode);
}

// Float Convert To Integer Word
static void FctiwTest()
{
  START_TEST();

  const u32 progress_interval = 1 << 22;

  const u64 max_rounding_mode = 4;

  // To also test inputs which do not have fractions close to .0 or .5, set small_numbers_end to 0
  const u32 small_numbers_start = 0;
  const u32 small_numbers_end = 0x200000;  // The point where the ulp becomes 0.25
  const u32 large_numbers_start = Common::BitCast<u32>(static_cast<float>(small_numbers_end));
  const u32 large_numbers_end = Common::BitCast<u32>(static_cast<float>(0x80000000 + progress_interval));
  const u32 numbers_to_test = (small_numbers_end - small_numbers_start) * 6 +
                              (large_numbers_end - large_numbers_start);

  for (u64 rounding_mode = 0; rounding_mode < max_rounding_mode; ++rounding_mode)
  {
    asm volatile("mtfsf 7, %0" :: "f"(rounding_mode));
    network_printf("Rounding mode: %llu\n", rounding_mode);

    for (u32 i = small_numbers_start; i < small_numbers_end; ++i)
    {
      const u32 i_plus_zero = Common::BitCast<u32>(static_cast<float>(i));
      const u32 i_plus_point_five = Common::BitCast<u32>(static_cast<float>(i) + 0.5f);
      const u32 i_plus_one = Common::BitCast<u32>(static_cast<float>(i + 1));

      FctiwTestBothSigns(i_plus_zero, static_cast<RoundingMode>(rounding_mode));
      FctiwTestBothSigns(i_plus_zero + 1, static_cast<RoundingMode>(rounding_mode));
      FctiwTestBothSigns(i_plus_point_five - 1, static_cast<RoundingMode>(rounding_mode));
      FctiwTestBothSigns(i_plus_point_five, static_cast<RoundingMode>(rounding_mode));
      FctiwTestBothSigns(i_plus_point_five + 1, static_cast<RoundingMode>(rounding_mode));
      FctiwTestBothSigns(i_plus_one - 1, static_cast<RoundingMode>(rounding_mode));

      if (i * 6 % progress_interval < 6)
      {
        network_printf("Progress %llu/%llu\n", numbers_to_test * rounding_mode + i * 12,
                       numbers_to_test * max_rounding_mode * 2);

        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
          goto end;
      }
    }

    for (u32 i = large_numbers_start; i < large_numbers_end; ++i)
    {
      FctiwTestBothSigns(i, static_cast<RoundingMode>(rounding_mode));

      if (i % progress_interval == 0)
      {
        network_printf("Progress %llu/%llu\n",
                       numbers_to_test * rounding_mode + small_numbers_end * 12 + (i - large_numbers_start) * 2,
                       numbers_to_test * max_rounding_mode * 2);

        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
          goto end;
      }
    }
  }

end:
  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  FctiwTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

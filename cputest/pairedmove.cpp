#include <gctypes.h>
#include <wiiuse/wpad.h>

#include "Common/BitUtils.h"
#include "Common/CommonFloat.h"
#include "Common/hwtests.h"


static u64 TruncateMantissaBits(u64 bits)
{
  // Truncate the bits (doesn't depend on rounding mode)
  constexpr u64 remove_bits = DOUBLE_FRAC_WIDTH - FLOAT_FRAC_WIDTH;
  constexpr u64 remove_mask = (1 << remove_bits) - 1;
  return bits & ~remove_mask;
}

inline u64 RoundMantissaBits(u64 bits, RoundingMode rounding_mode)
{
  // Round bits in software rather than relying on any hardware float functions
  constexpr u64 remove_bits = DOUBLE_FRAC_WIDTH - FLOAT_FRAC_WIDTH;
  constexpr u64 remove_mask = (1 << remove_bits) - 1;

  u64 round_down = bits & ~remove_mask;
  u64 masked_bits = bits & remove_mask;

  if ((bits & DOUBLE_EXP) == DOUBLE_EXP)
  {
    // For infinite and NaN values, the mantissa is simply truncated
    return round_down;
  }

  // Only round up if the result wouldn't be exact otherwise!
  u64 round_up = round_down + (bits == round_down ? 0 : 1 << remove_bits);
  u64 even_split = 1 << (remove_bits - 1);

  switch (rounding_mode)
  {
  case RoundingMode::Nearest:
    // Round to nearest (ties even)
    if (masked_bits > even_split ||
    (masked_bits == even_split && (bits & (1 << remove_bits)) != 0))
  {
    return round_up;
  }
  else
  {
    return round_down;
  }
  case RoundingMode::TowardsZero:
    return round_down;
  case RoundingMode::TowardsPositiveInfinity:
    if ((bits & DOUBLE_SIGN) == 0)
    {
      return round_up;
    }
    else
    {
      return round_down;
    }
  case RoundingMode::TowardsNegativeInfinity:
    if ((bits & DOUBLE_SIGN) != 0)
    {
      return round_up;
    }
    else
    {
      return round_down;
    }
  default:
    // Unreachable
    return 0;
  }
}


static void MergeTest(const u64* input_ptr, RoundingMode rounding_mode)
{
  double result_ps0;
  double result_ps1;
  u64 input = *input_ptr;
  u64 expected_ps0 = RoundMantissaBits(input, rounding_mode);
  u64 expected_ps1 = TruncateMantissaBits(input);

  asm volatile ("lfd %1, 0(%2)\n"
       "ps_merge00 %0, %1, %1\n"
       "ps_merge11 %1, %0, %0\n"
       : "=f"(result_ps0), "=f"(result_ps1)
       : "r"(input_ptr)
  );

  u64 result_ps0_bits = Common::BitCast<u64>(result_ps0);
  u64 result_ps1_bits = Common::BitCast<u64>(result_ps1);

  DO_TEST(result_ps0_bits == expected_ps0 
          && result_ps1_bits == expected_ps1,
          "ps_merge 0x{:016x} ({}):\n"
          "     got 0x{:016x} ({}) 0x{:016x} ({})\n"
          "expected 0x{:016x} ({}) 0x{:016x} ({})",
          input,  Common::BitCast<double>(input),
          result_ps0_bits, result_ps0,
          result_ps1_bits, result_ps1,
          expected_ps0, Common::BitCast<double>(expected_ps0),
          expected_ps1, Common::BitCast<double>(expected_ps1));
}

static void MrTest(const u64* input_ptr, RoundingMode rounding_mode)
{
  double result_ps0;
  u64 input = *input_ptr;
  u64 expected = RoundMantissaBits(input, rounding_mode);

  // PS1 result doesn't matter because it'd actually be
  // an issue with ps_merge!

  asm volatile ("lfd %0, 0(%1)\n"
       "ps_mr %0, %0\n"
       : "=f"(result_ps0)
       : "r"(input_ptr)
  );

  u64 result_ps0_bits = Common::BitCast<u64>(result_ps0);

  DO_TEST(result_ps0_bits == expected, "ps_mr 0x{:016x} ({}):\n"
                              "     got 0x{:016x} ({})\n"
                              "expected 0x{:016x} ({})",
          input,  Common::BitCast<double>(input),
          result_ps0_bits, result_ps0,
          expected, Common::BitCast<double>(expected));
}

static void NegTest(const u64* input_ptr, RoundingMode rounding_mode)
{
  double result_ps0;
  double result_ps1;
  u64 input = *input_ptr;
  u64 result_unrounded = input ^ DOUBLE_SIGN;
  u64 expected_ps0 = RoundMantissaBits(result_unrounded, rounding_mode);
  u64 expected_ps1 = TruncateMantissaBits(result_unrounded);

  asm volatile ("lfd %0, 0(%2)\n"
       "ps_merge00 %0, %0, %0\n"
       "lfd %0, 0(%2)\n"
       "ps_neg %0, %0\n"
       "ps_merge11 %1, %0, %0\n"
       : "=f"(result_ps0), "=f"(result_ps1)
       : "r"(input_ptr)
  );

  u64 result_ps0_bits = Common::BitCast<u64>(result_ps0);
  u64 result_ps1_bits = Common::BitCast<u64>(result_ps1);

  DO_TEST(result_ps0_bits == expected_ps0 
          && result_ps1_bits == expected_ps1,
          "ps_neg 0x{:016x} ({}):\n"
          "     got 0x{:016x} ({}) 0x{:016x} ({})\n"
          "expected 0x{:016x} ({}) 0x{:016x} ({})",
          input,  Common::BitCast<double>(input),
          result_ps0_bits, result_ps0,
          result_ps1_bits, result_ps1,
          expected_ps0, Common::BitCast<double>(expected_ps0),
          expected_ps1, Common::BitCast<double>(expected_ps1));
}

void AbsTest(const u64* input_ptr, RoundingMode rounding_mode)
{
  double result_ps0;
  double result_ps1;
  u64 input = *input_ptr;
  u64 result_unrounded = input & ~DOUBLE_SIGN;
  u64 expected_ps0 = RoundMantissaBits(result_unrounded, rounding_mode);
  u64 expected_ps1 = TruncateMantissaBits(result_unrounded);

  asm volatile ("lfd %0, 0(%2)\n"
       "ps_merge00 %0, %0, %0\n"
       "lfd %0, 0(%2)\n"
       "ps_abs %0, %0\n"
       "ps_merge11 %1, %0, %0\n"
       : "=f"(result_ps0), "=f"(result_ps1)
       : "r"(input_ptr)
  );

  u64 result_ps0_bits = Common::BitCast<u64>(result_ps0);
  u64 result_ps1_bits = Common::BitCast<u64>(result_ps1);

  DO_TEST(result_ps0_bits == expected_ps0 
          && result_ps1_bits == expected_ps1,
          "ps_abs 0x{:016x} ({}):\n"
          "     got 0x{:016x} ({}) 0x{:016x} ({})\n"
          "expected 0x{:016x} ({}) 0x{:016x} ({})",
          input,  Common::BitCast<double>(input),
          result_ps0_bits, result_ps0,
          result_ps1_bits, result_ps1,
          expected_ps0, Common::BitCast<double>(expected_ps0),
          expected_ps1, Common::BitCast<double>(expected_ps1));
}

static void NabsTest(const u64* input_ptr, RoundingMode rounding_mode)
{
  double result_ps0;
  double result_ps1;
  u64 input = *input_ptr;
  u64 result_unrounded = input | DOUBLE_SIGN;
  u64 expected_ps0 = RoundMantissaBits(result_unrounded, rounding_mode);
  u64 expected_ps1 = TruncateMantissaBits(result_unrounded);

  asm volatile ("lfd %0, 0(%2)\n"
       "ps_merge00 %0, %0, %0\n"
       "lfd %0, 0(%2)\n"
       "ps_nabs %0, %0\n"
       "ps_merge11 %1, %0, %0\n"
       : "=f"(result_ps0), "=f"(result_ps1)
       : "r"(input_ptr)
  );

  u64 result_ps0_bits = Common::BitCast<u64>(result_ps0);
  u64 result_ps1_bits = Common::BitCast<u64>(result_ps1);

  DO_TEST(result_ps0_bits == expected_ps0 
          && result_ps1_bits == expected_ps1,
          "ps_nabs 0x{:016x} ({}):\n"
          "     got 0x{:016x} ({}) 0x{:016x} ({})\n"
          "expected 0x{:016x} ({}) 0x{:016x} ({})",
          input,  Common::BitCast<double>(input),
          result_ps0_bits, result_ps0,
          result_ps1_bits, result_ps1,
          expected_ps0, Common::BitCast<double>(expected_ps0),
          expected_ps1, Common::BitCast<double>(expected_ps1));
}

static void RsqrteTest(const u64* input_ptr)
{
  double result_ps0;
  double result_ps1;
  u64 input_ps0 = *input_ptr;
  double input_ps0_float = Common::BitCast<double>(input_ps0);
  u64 input_ps1 = TruncateMantissaBits(input_ps0);
  double input_ps1_float = Common::BitCast<double>(input_ps1);

  double result_unrounded_ps0 = frsqrte_expected(input_ps0_float);
  u64 expected_ps0 = TruncateMantissaBits(Common::BitCast<u64>(result_unrounded_ps0));
  double result_unrounded_ps1 = frsqrte_expected(input_ps1_float);
  u64 expected_ps1 = TruncateMantissaBits(Common::BitCast<u64>(result_unrounded_ps1));

  asm volatile ("lfd %0, 0(%2)\n"
       "ps_merge00 %1, %0, %0\n"
       "ps_rsqrte %0, %0\n"
       "ps_rsqrte %1, %1\n"
       "ps_merge11 %1, %1, %1\n"
       : "=f"(result_ps0), "=f"(result_ps1)
       : "r"(input_ptr)
  );

  u64 result_ps0_bits = Common::BitCast<u64>(result_ps0);
  u64 result_ps1_bits = Common::BitCast<u64>(result_ps1);

  DO_TEST(result_ps0_bits == expected_ps0
          && result_ps1_bits == expected_ps1,
          "ps_rsqrte 0x{:016x} ({}) 0x{:016x} ({}):\n"
          "     got 0x{:016x} ({}) 0x{:016x} ({})\n"
          "expected 0x{:016x} ({}) 0x{:016x} ({})",
          input_ps0, input_ps0_float,
          input_ps1, input_ps1_float,
          result_ps0_bits, result_ps0,
          result_ps1_bits, result_ps1,
          expected_ps0, Common::BitCast<double>(expected_ps0),
          expected_ps1, Common::BitCast<double>(expected_ps1));
}

static void PSMoveTest()
{
  START_TEST();

  static const u64 inputs[] = {
    // Some basic fractions
    0x0000000000000000, // 0
    0x3ff0000000000000, // 1
    0x3fe0000000000000, // 1 / 2
    0x3fd5555555555555, // 1 / 3
    0x3fc999999999999a, // 1 / 5
    0x3fc2492492492492, // 1 / 7

    // Specifically tuned regular numbers
    0x0000000000000001, // Min double denormal
    0x36a0000000000000, // Min single denormal
    0x3690000000000000, // Min single denormal / 2
    0x36a8000000000000, // Min single denormal * 3 / 2
    0x36a8000000000000, // Min single denormal * 3 / 2
    0x7fefffffffffffff, // Max double denormal
    0x47efffffe0000000, // Max single denormal
    0x47effffff0000000, // Max single denormal + round
    0x0000000010000000, // Double denormal (no round even)
    0x0000000010000001, // Double denormal (round even)
    0x0000000030000000, // Double denormal (round even)
    0x500fffffd0000000, // Double big (no round even)
    0x500fffffd0000001, // Double big (round even)
    0x500ffffff0000000, // Double big (round even)
    0x0123456789abcdef, // Random
    0x76543210fedcba09, // Random

    // Specifically chosen weird numbers
    0x7ff0000000000000, // Infinity
    0x7ff8000000000000, // QNaN
    0x7ff4000000000000, // SNaN
    0x7ffaaaaaaaaaaaaa, // QNaN (extra bits)
    0x7ff5555555555555, // SNaN (extra bits)
  };

  const u64 max_rounding_mode = 4;

  for (u64 rounding_mode_idx = 0; rounding_mode_idx < max_rounding_mode; ++rounding_mode_idx)
  {
    asm volatile("mtfsf 7, %0" :: "f"(rounding_mode_idx));
    network_printf("Rounding mode: %llu\n", rounding_mode_idx);

    for (u64 sign = 0; sign < 2; ++sign)
    {
      RoundingMode rounding_mode = static_cast<RoundingMode>(rounding_mode_idx);

      for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i)
      {
        const u64 input = inputs[i] | (sign << DOUBLE_SIGN_SHIFT);
        const u64 *input_ref = &input;

        MergeTest(input_ref, rounding_mode);
        MrTest(input_ref, rounding_mode);
        NegTest(input_ref, rounding_mode);
        AbsTest(input_ref, rounding_mode);
        NabsTest(input_ref, rounding_mode);
        RsqrteTest(input_ref);
      }
    }
  }

  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  PSMoveTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

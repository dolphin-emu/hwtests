#include <gctypes.h>
#include <wiiuse/wpad.h>

#include "Common/BitUtils.h"
#include "Common/FloatUtils.h"
#include "Common/hwtests.h"


static void MergeTest(const u64* input_ptr, RoundingMode rounding_mode)
{
  double result_ps0;
  double result_ps1;
  u64 input = *input_ptr;
  u64 expected_ps0 = RoundMantissaBits(input, rounding_mode);
  u64 expected_ps1 = TruncateMantissaBits(input);

  asm volatile ("ps_mr %1, %1\n"
       "isync\n"
       "lfd %1, 0(%2)\n"
       "isync\n"
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
       "isync\n"
       "lfd %0, 0(%2)\n"
       "isync\n"
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
       "isync\n"
       "lfd %0, 0(%2)\n"
       "isync\n"
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
       "isync\n"
       "lfd %0, 0(%2)\n"
       "isync\n"
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

void SelTest(const u64* input_ptr, RoundingMode rounding_mode)
{
  // Only tests the select taken case
  // The untaken case should count as an error as well
  double result0_ps0;
  double result0_ps1;
  double result1_ps0;
  double result1_ps1;

  u64 input = *input_ptr;
  u64 expected_ps0 = RoundMantissaBits(input, rounding_mode);
  u64 expected_ps1 = TruncateMantissaBits(input);

  double one = 1.0;

  asm volatile ("lfd %0, 0(%5)\n"
       "ps_merge00 %0, %0, %0\n"
       "isync\n"
       "lfd %0, 0(%5)\n"
       "isync\n"
       "ps_merge00 %0, %0, %0\n"
       "lfd %2, 0(%5)\n"
       "ps_merge00 %2, %2, %2\n"
       "isync\n"
       "lfd %2, 0(%5)\n"
       "isync\n"
       "ps_merge00 %2, %2, %2\n"
       "ps_merge00 %4, %4, %4\n"
       "ps_sel %0, %4, %0, %4\n"
       "ps_merge11 %1, %0, %0\n"
       "ps_neg %4, %4\n"
       "ps_sel %2, %4, %4, %2\n"
       "ps_merge11 %3, %2, %2\n"
       : "=f"(result0_ps0), "=f"(result0_ps1), "=f"(result1_ps0), "=f"(result1_ps1), "+f"(one)
       : "r"(input_ptr)
  );

  u64 result0_ps0_bits = Common::BitCast<u64>(result0_ps0);
  u64 result0_ps1_bits = Common::BitCast<u64>(result0_ps1);
  u64 result1_ps0_bits = Common::BitCast<u64>(result1_ps0);
  u64 result1_ps1_bits = Common::BitCast<u64>(result1_ps1);

  DO_TEST(result0_ps0_bits == expected_ps0 
          && result0_ps1_bits == expected_ps1
          && result1_ps0_bits == expected_ps0
          && result1_ps1_bits == expected_ps1,
          "ps_sel 0x{:016x} ({}):\n"
          "       got >=0: 0x{:016x} ({}) 0x{:016x} ({})\n"
          "            <0: 0x{:016x} ({}) 0x{:016x} ({})\n"
          "expected 0x{:016x} ({}) 0x{:016x} ({})",
          input,  Common::BitCast<double>(input),
          result0_ps0_bits, result0_ps0,
          result0_ps1_bits, result0_ps1,
          result1_ps0_bits, result1_ps0,
          result1_ps1_bits, result1_ps1,
          expected_ps0, Common::BitCast<double>(expected_ps0),
          expected_ps1, Common::BitCast<double>(expected_ps1));
}

static void Sum0Test(const u64* input_ptr)
{
  // Only checks PS1 because PS0 should be rounded to a float,
  // which isn't a move operation
  double result_ps1;
  double one = 1.0;
  u64 input = *input_ptr;
  u64 expected_ps1 = TruncateMantissaBits(input);

  asm volatile ("lfd %0, 0(%2)\n"
       "ps_merge00 %0, %0, %0\n"
       "isync\n"
       "lfd %0, 0(%2)\n"
       "isync\n"
       "ps_sum0 %0, %0, %0, %1\n"
       "ps_merge11 %0, %0, %0\n"
       : "=f"(result_ps1)
       : "f"(one), "r"(input_ptr)
  );

  u64 result_ps1_bits = Common::BitCast<u64>(result_ps1);

  DO_TEST(result_ps1_bits == expected_ps1,
          "ps_sum0 0x{:016x} ({}):\n"
          "     got 0x{:016x} ({})\n"
          "expected 0x{:016x} ({})",
          input,  Common::BitCast<double>(input),
          result_ps1_bits, result_ps1,
          expected_ps1, Common::BitCast<double>(expected_ps1));
}

static void Sum1Test(const u64* input_ptr, RoundingMode rounding_mode)
{
  // The opposite of ps_sum0, only checks ps0
  double result_ps0;
  double one = 1.0;
  u64 input = *input_ptr;
  u64 expected_ps0 = RoundMantissaBitsAssumeFinite(input, rounding_mode);

  asm volatile ("lfd %0, 0(%2)\n"
       "ps_merge00 %0, %0, %0\n"
       "isync\n"
       "lfd %0, 0(%2)\n"
       "isync\n"
       "ps_sum1 %0, %0, %0, %1\n"
       : "=f"(result_ps0)
       : "f"(one), "r"(input_ptr)
  );

  u64 result_ps0_bits = Common::BitCast<u64>(result_ps0);

  DO_TEST(result_ps0_bits == expected_ps0,
          "ps_sum1 0x{:016x} ({}):\n"
          "     got 0x{:016x} ({})\n"
          "expected 0x{:016x} ({})",
          input,  Common::BitCast<double>(input),
          result_ps0_bits, result_ps0,
          expected_ps0, Common::BitCast<double>(expected_ps0));
}

static void ResTest(const u64* input_ptr, bool ni)
{
  double result_ps0;
  double result_ps1;
  u64 input = *input_ptr;
  double input_float = Common::BitCast<double>(input);

  double expected_ps0_float = fres_expected(input_float, ni);
  u64 expected_ps0 = TruncateMantissaBits(Common::BitCast<u64>(expected_ps0_float));
  double expected_ps1_float = expected_ps0_float;
  u64 expected_ps1 = expected_ps0;

  // If the full precision input would've only been a value which *truncates* to 0,
  // it *always* sets the sign of the input for some reason
  if ((input & 0x7fffffffe0000000) == 0 && (input & ~DOUBLE_SIGN) != 0) {
    expected_ps1 |= DOUBLE_SIGN;
    expected_ps1_float = Common::BitCast<double>(expected_ps1);
  }

  asm volatile ("ps_mr %0, %0\n"
       "lfd %0, 0(%2)\n"
       "ps_merge00 %1, %0, %0\n"
       "ps_res %0, %0\n"
       "ps_res %1, %1\n"
       "ps_merge11 %1, %1, %1\n"
       : "=f"(result_ps0), "=f"(result_ps1)
       : "r"(input_ptr)
  );

  u64 result_ps0_bits = Common::BitCast<u64>(result_ps0);
  u64 result_ps1_bits = Common::BitCast<u64>(result_ps1);

  DO_TEST(result_ps0_bits == expected_ps0
          && result_ps1_bits == expected_ps1,
          "ps_res 0x{:016x} ({}):\n"
          "     got 0x{:016x} ({}) 0x{:016x} ({})\n"
          "expected 0x{:016x} ({}) 0x{:016x} ({})",
          input, input_float,
          result_ps0_bits, result_ps0,
          result_ps1_bits, result_ps1,
          expected_ps0, expected_ps0_float,
          expected_ps1, expected_ps1_float);
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

  // If the full precision input would've only been a value which *truncates* to 0,
  // it *always* sets the sign of the input for some reason, which will
  // return NaN here
  if ((input_ps0 & 0x7fffffffe0000000) == 0 && (input_ps0 & ~DOUBLE_SIGN) != 0) {
    expected_ps1 = 0x7ff8000000000000;
  }

  asm volatile ("ps_mr %0, %0\n"
       "lfd %0, 0(%2)\n"
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
    0x36b0000000000000, // Barely over not min accounted reciprocal
    0x36c0000000000000, // Again barely over not min accounted reciprocal
    0x37e0000000000000, // High 0 reciprocal
    0x37f0000000000000, // Not 0 reciprocal
    0x3800000000000000, // Very not 0 reciprocal
    0x000fffffc0000000, // Not max double denormal
    0x380fffff80000000, // Not max single denormal
    0x000fffffffffffff, // Max double denormal
    0x001fffffffffffff, // Not denormal double
    0x380fffffc0000000, // Max single denormal
    0x380fffffe0000000, // Max single denormal + even
    0x380ffffff0000000, // Max single denormal + round
    0x380fffffffffffff, // Max single denormal + big influence
    0x7fefffffffffffff, // Max double normal
    0x47efffffe0000000, // Max single normal
    0x47effffff0000000, // Max single normal + round
    0x0000000010000000, // Double denormal (no round even)
    0x0000000010000001, // Double denormal (round even)
    0x000000001fffffff, // Max min which should round/trunc
    0x0000000020000000, // Min nonzero which should be agreed upon
    0x0000000030000000, // Double denormal (round even)
    0x500fffffd0000000, // Double big (no round even)
    0x500fffffd0000001, // Double big (round even)
    0x500ffffff0000000, // Double big (round even)
    0x3fffffffffffffff, // Smallest number below 2
    0x3fffffffd0000000, // Another small number below 2
    0x3fffffffe0000000, // Small number below 2 (ties even)
    0x3fffffffe0000001, // Small number below 2 (round up)
    0x3fffffffffffffff, // Denormal with influence
    0x1fffffffd0000000, // Similar denormal
    0x1fffffffe0000000, // Similar denormal again (ties even)
    0x1fffffffe0000001, // Similar denormal yet again (round up)
    0x47cfffffc0000000, // Before fres stops working
    0x47d0000000000000, // After fres stops working
    0x47e0000000000000, // Another after fres stops working
    0x47f0000000000000, // Yet another after fres stops working for good measure
    0x4800000000000000, // ...?
    0x0123456789abcdef, // Random
    0x76543210fedcba09, // Random

    // Specifically chosen weird numbers
    0x7ff0000000000000, // Infinity
    0x7ff8000000000000, // QNaN
    0x7ff4000000000000, // SNaN
    0x7ffaaaaaaaaaaaaa, // QNaN (extra bits)
    0x7ff5555555555555, // SNaN (extra bits)
  };

  for (u64 ni = 0; ni < 2; ++ni)
  {
    const u64 max_rounding_mode = 4;

    for (u64 rounding_mode_idx = 0; rounding_mode_idx < max_rounding_mode; ++rounding_mode_idx)
    {
      u64 rounding_setting = ni << 2 | rounding_mode_idx;
      asm volatile("mtfsf 7, %0" :: "f"(rounding_setting));
      network_printf("Rounding mode: %llu | NI: %llu\n", rounding_mode_idx, ni);

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
          SelTest(input_ref, rounding_mode);
          Sum0Test(input_ref);
          Sum1Test(input_ref, rounding_mode);
          ResTest(input_ref, ni != 0);
          RsqrteTest(input_ref);
        }
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

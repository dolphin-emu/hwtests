#pragma once

#include "Common/CommonTypes.h"
#include "Common/BitUtils.h"
#include <cfloat>
#include <limits>

#include <cfloat>
#include <limits>

enum class RoundingMode
{
  Nearest = 0b00,
  TowardsZero = 0b01,
  TowardsPositiveInfinity = 0b10,
  TowardsNegativeInfinity = 0b11,
};

constexpr u64 DOUBLE_SIGN = 0x8000000000000000ULL;
constexpr u64 DOUBLE_EXP = 0x7FF0000000000000ULL;
constexpr u64 DOUBLE_FRAC = 0x000FFFFFFFFFFFFFULL;
constexpr u64 DOUBLE_ZERO = 0x0000000000000000ULL;
constexpr u64 DOUBLE_QBIT = 0x0008000000000000ULL;
constexpr u64 DOUBLE_FRAC_WIDTH = 52;
constexpr u64 DOUBLE_SIGN_SHIFT = 63;

constexpr u32 FLOAT_SIGN = 0x80000000;
constexpr u32 FLOAT_EXP = 0x7F800000;
constexpr u32 FLOAT_FRAC = 0x007FFFFF;
constexpr u32 FLOAT_ZERO = 0x00000000;
constexpr u32 FLOAT_FRAC_WIDTH = 23;


u64 TruncateMantissaBits(u64 bits)
{
  // Truncate the bits (doesn't depend on rounding mode)
  constexpr u64 remove_bits = DOUBLE_FRAC_WIDTH - FLOAT_FRAC_WIDTH;
  constexpr u64 remove_mask = (1 << remove_bits) - 1;
  return bits & ~remove_mask;
}

// Only used on its own by ps_sum1, for some reason
inline u64 RoundMantissaBitsAssumeFinite(u64 bits, RoundingMode rounding_mode)
{
  // Round bits in software rather than relying on any hardware float functions
  constexpr u64 remove_bits = DOUBLE_FRAC_WIDTH - FLOAT_FRAC_WIDTH;
  constexpr u64 remove_mask = (1 << remove_bits) - 1;

  u64 round_down = bits & ~remove_mask;
  u64 masked_bits = bits & remove_mask;

  // Only round up if the result wouldn't be exact otherwise!
  u64 round_up = round_down + (bits == round_down ? 0 : 1 << remove_bits);
  if ((bits & DOUBLE_EXP) == 0) {
    round_up &= ~DOUBLE_EXP;
  }

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

inline u64 RoundMantissaBits(u64 bits, RoundingMode rounding_mode)
{
  if ((bits & DOUBLE_EXP) == DOUBLE_EXP)
  {
    // For infinite and NaN values, the mantissa is simply truncated
    return TruncateMantissaBits(bits);
  }

  return RoundMantissaBitsAssumeFinite(bits, rounding_mode);
}

inline float RoundToFloatWithMode(double input, RoundingMode rounding_mode)
{
  float result;
  u64 round_mode_bits = static_cast<u64>(rounding_mode);
  double scratch;

  asm volatile (
    "mffs %1\n"
    "mtfsf 7, %0\n"
    "frsp %2, %3\n"
    "mtfsf 7, %1\n"
    : "+f"(round_mode_bits), "=f"(scratch), "=f"(result)
    : "f"(input)
  );

  return result;
}


double frsqrte_expected(double val)
{
  static const int estimate_base[] = {
      0x3ffa000, 0x3c29000, 0x38aa000, 0x3572000, 0x3279000, 0x2fb7000, 0x2d26000, 0x2ac0000,
      0x2881000, 0x2665000, 0x2468000, 0x2287000, 0x20c1000, 0x1f12000, 0x1d79000, 0x1bf4000,
      0x1a7e800, 0x17cb800, 0x1552800, 0x130c000, 0x10f2000, 0x0eff000, 0x0d2e000, 0x0b7c000,
      0x09e5000, 0x0867000, 0x06ff000, 0x05ab800, 0x046a000, 0x0339800, 0x0218800, 0x0105800,
  };
  static const int estimate_dec[] = {
      0x7a4, 0x700, 0x670, 0x5f2, 0x584, 0x524, 0x4cc, 0x47e, 0x43a, 0x3fa, 0x3c2,
      0x38e, 0x35e, 0x332, 0x30a, 0x2e6, 0x568, 0x4f3, 0x48d, 0x435, 0x3e7, 0x3a2,
      0x365, 0x32e, 0x2fc, 0x2d0, 0x2a8, 0x283, 0x261, 0x243, 0x226, 0x20b,
  };

  union
  {
    double valf;
    s64 vali;
  };
  valf = val;
  s64 mantissa = vali & DOUBLE_FRAC;
  s64 sign = vali & DOUBLE_SIGN;
  s64 exponent = vali & DOUBLE_EXP;

  // Special case 0
  if (mantissa == 0 && exponent == 0)
    return sign ? -std::numeric_limits<double>::infinity() :
                  std::numeric_limits<double>::infinity();
  // Special case NaN-ish numbers
  if (exponent == DOUBLE_EXP)
  {
    if (mantissa == 0)
    {
      if (sign)
        return std::numeric_limits<double>::quiet_NaN();
      return 0.0;
    }
    return 0.0 + valf;
  }
  // Negative numbers return NaN
  if (sign)
    return std::numeric_limits<double>::quiet_NaN();

  if (!exponent)
  {
    // "Normalize" denormal values
    do
    {
      exponent -= 1LL << 52;
      mantissa <<= 1;
    } while (!(mantissa & (1LL << 52)));
    mantissa &= DOUBLE_FRAC;
    exponent += 1LL << 52;
  }

  bool odd_exponent = !(exponent & (1LL << 52));
  exponent = ((0x3FFLL << 52) - ((exponent - (0x3FELL << 52)) / 2)) & (0x7FFLL << 52);

  int i = (int)(mantissa >> 37);
  vali = sign | exponent;
  int index = i / 2048 + (odd_exponent ? 16 : 0);
  vali |= (s64)(estimate_base[index] - estimate_dec[index] * (i % 2048)) << 26;
  return valf;
}


double fres_expected(double val)
{
  static const s32 estimate_base[] = {
      0xfff000, 0xf07000, 0xe1d400, 0xd41000, 0xc71000, 0xbac400, 0xaf2000, 0xa41000,
      0x999000, 0x8f9400, 0x861000, 0x7d0000, 0x745800, 0x6c1000, 0x642800, 0x5c9400,
      0x555000, 0x4e5800, 0x47ac00, 0x413c00, 0x3b1000, 0x352000, 0x2f5c00, 0x29f000,
      0x248800, 0x1f7c00, 0x1a7000, 0x15bc00, 0x110800, 0x0ca000, 0x083800, 0x041800,
  };
  static const s32 estimate_dec[] = {
      -0x3e1, -0x3a7, -0x371, -0x340, -0x313, -0x2ea, -0x2c4, -0x2a0, -0x27f, -0x261, -0x245,
      -0x22a, -0x212, -0x1fb, -0x1e5, -0x1d1, -0x1be, -0x1ac, -0x19b, -0x18b, -0x17c, -0x16e,
      -0x15b, -0x15b, -0x143, -0x143, -0x12d, -0x12d, -0x11a, -0x11a, -0x108, -0x106,
  };

  union
  {
    float valf;
    u32 vali;
  };
  u64 full_bits = Common::BitCast<u64>(val);
  valf = RoundToFloatWithMode(val, RoundingMode::TowardsZero);
  u32 mantissa = vali & FLOAT_FRAC;
  u32 sign = vali & FLOAT_SIGN;
  s32 exponent = static_cast<s32>(vali & FLOAT_EXP);

  // Special case 0
  if (exponent == 0 && mantissa < 0x200000)
  {
    if ((full_bits & ~DOUBLE_SIGN) == 0)
    {
      return sign ? -std::numeric_limits<double>::infinity() :
                    std::numeric_limits<double>::infinity();
    }
    else
    {
      return sign ? -FLT_MAX : FLT_MAX;
    }
  }

  // Special case NaN-ish numbers
  if ((full_bits & DOUBLE_EXP) >= 0x47f0000000000000ULL)
  {
    // If it's not NaN, it's infinite!
    if (valf == valf)
      return sign ? -0.0 : 0.0;
    return 0.0 + val;
  }

  // Number is denormal, shift the mantissa and adjust the exponent
  if (exponent == 0)
  {
    mantissa <<= 1;
    while ((mantissa & FLOAT_EXP) == 0) {
      mantissa <<= 1;
      exponent -= static_cast<s32>(1 << FLOAT_FRAC_WIDTH);
    }

    mantissa &= FLOAT_FRAC;
  }

  exponent = (253 << FLOAT_FRAC_WIDTH) - exponent;

  u32 key = mantissa >> 18;
  u32 new_mantissa = static_cast<u32>(estimate_base[key] + estimate_dec[key] * static_cast<s32>((mantissa >> 8) & 0x3ff)) >> 1;

  if (exponent <= 0)
  {
    // Result is subnormal, format it properly!
    u32 shift = 1 + (static_cast<u32>(-exponent) >> FLOAT_FRAC_WIDTH);
    vali = sign | (((1 << FLOAT_FRAC_WIDTH) | new_mantissa) >> shift);
  }
  else
  {
    // Result is normal, just string things together
    vali = sign | static_cast<u32>(exponent) | new_mantissa;
  }
  return static_cast<double>(valf);
}
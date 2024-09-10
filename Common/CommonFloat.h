#pragma once

#include "Common/CommonTypes.h"
#include <limits>

enum class RoundingMode
{
  Nearest = 0b00,
  TowardsZero = 0b01,
  TowardsPositiveInfinity = 0b10,
  TowardsNegativeInfinity = 0b11,
};

enum : u64
{
  DOUBLE_SIGN = 0x8000000000000000ULL,
  DOUBLE_EXP = 0x7FF0000000000000ULL,
  DOUBLE_FRAC = 0x000FFFFFFFFFFFFFULL,
  DOUBLE_ZERO = 0x0000000000000000ULL,
  DOUBLE_QBIT = 0x0008000000000000ULL,
};
constexpr u64 DOUBLE_FRAC_WIDTH = 52;
constexpr u64 DOUBLE_SIGN_SHIFT = 63;

enum : u32
{
  FLOAT_SIGN = 0x80000000,
  FLOAT_EXP = 0x7F800000,
  FLOAT_FRAC = 0x007FFFFF,
  FLOAT_ZERO = 0x00000000,
};
constexpr u32 FLOAT_FRAC_WIDTH = 23;

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

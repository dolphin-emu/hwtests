#include <array>
#include <limits>

#include <gctypes.h>
#include <wiiuse/wpad.h>

#include "Common/BitUtils.h"
#include "Common/hwtests.h"

static const u64 QUIET_BIT = 0x0008000000000000ULL;

static const double Q_POS_NAN_0 = Common::BitCast<double>(0x7FF8000000000000ULL);
static const double Q_POS_NAN_1 = Common::BitCast<double>(0x7FF8000100000000ULL);
static const double Q_POS_NAN_2 = Common::BitCast<double>(0x7FF8000200000000ULL);
static const double Q_POS_NAN_3 = Common::BitCast<double>(0x7FF8000300000000ULL);

static const double S_POS_NAN_1 = Common::BitCast<double>(0x7FF0000100000000ULL);
static const double S_POS_NAN_2 = Common::BitCast<double>(0x7FF0000200000000ULL);
static const double S_POS_NAN_3 = Common::BitCast<double>(0x7FF0000300000000ULL);

static const double Q_NEG_NAN_0 = Common::BitCast<double>(0xFFF8000000000000ULL);
static const double Q_NEG_NAN_1 = Common::BitCast<double>(0xFFF8000100000000ULL);
static const double Q_NEG_NAN_2 = Common::BitCast<double>(0xFFF8000200000000ULL);
static const double Q_NEG_NAN_3 = Common::BitCast<double>(0xFFF8000300000000ULL);

static const double S_NEG_NAN_1 = Common::BitCast<double>(0xFFF0000100000000ULL);
static const double S_NEG_NAN_2 = Common::BitCast<double>(0xFFF0000200000000ULL);
static const double S_NEG_NAN_3 = Common::BitCast<double>(0xFFF0000300000000ULL);

static const std::array<u32, 6> SINGLE_SNANS{0x7F800004, 0x7F800005, 0x7F800006,
                                             0x7F800007, 0x7F800008, 0x7F800009};

static const double DEFAULT_NAN = Q_POS_NAN_0;

// a + b
static void TestFadd(double a, double b, double expected)
{
  double result;
  asm("fadd %0, %1, %2" : "=d"(result) : "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fadd a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestFaddWithSharedA(double a, double b, double expected)
{
  double result = a;
  asm("fadd %0, %0, %1" : "+d"(result) : "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fadd (a=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestFaddWithSharedB(double a, double b, double expected)
{
  double result = b;
  asm("fadd %0, %1, %0" : "+d"(result) : "d"(a));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fadd (b=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsAddLower(double a, double b, double expected)
{
  double result, a_tmp, b_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 8(%3), 0, 0\n\t"
      "ps_merge01 %1, %4, %1\n\t"
      "ps_merge01 %2, %5, %2\n\t"
      "ps_add %0, %1, %2"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_add a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsAddUpper(double a, double b, double expected)
{
  double result, a_tmp, b_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 8(%3), 0, 0\n\t"
      "ps_merge00 %1, %1, %4\n\t"
      "ps_merge00 %2, %2, %5\n\t"
      "ps_add %0, %1, %2\n\t"
      "ps_merge11 %0, %0, %0\n\t"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_add a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsAddLowerWithSharedA(double a, double b, double expected)
{
  double result, b_tmp;
  asm("psq_l %0, 0(%2), 0, 0\n\t"
      "psq_l %1, 8(%2), 0, 0\n\t"
      "ps_merge01 %0, %3, %0\n\t"
      "ps_merge01 %1, %4, %1\n\t"
      "ps_add %0, %0, %1"
      : "=d"(result), "=d"(b_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_add (a=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsAddUpperWithSharedA(double a, double b, double expected)
{
  double result, b_tmp;
  asm("psq_l %0, 0(%2), 0, 0\n\t"
      "psq_l %1, 8(%2), 0, 0\n\t"
      "ps_merge00 %0, %0, %3\n\t"
      "ps_merge00 %1, %1, %4\n\t"
      "ps_add %0, %0, %1\n\t"
      "ps_merge11 %0, %0, %0\n\t"
      : "=d"(result), "=d"(b_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_add (a=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsAddLowerWithSharedB(double a, double b, double expected)
{
  double result, a_tmp;
  asm("psq_l %1, 0(%2), 0, 0\n\t"
      "psq_l %0, 8(%2), 0, 0\n\t"
      "ps_merge01 %1, %3, %1\n\t"
      "ps_merge01 %0, %4, %0\n\t"
      "ps_add %0, %1, %0"
      : "=d"(result), "=d"(a_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_add (b=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsAddUpperWithSharedB(double a, double b, double expected)
{
  double result, a_tmp;
  asm("psq_l %1, 0(%2), 0, 0\n\t"
      "psq_l %0, 8(%2), 0, 0\n\t"
      "ps_merge00 %1, %1, %3\n\t"
      "ps_merge00 %0, %0, %4\n\t"
      "ps_add %0, %1, %0\n\t"
      "ps_merge11 %0, %0, %0\n\t"
      : "=d"(result), "=d"(a_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_add (b=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

// a / b
static void TestFdiv(double a, double b, double expected)
{
  double result;
  asm("fdiv %0, %1, %2" : "=d"(result) : "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fdiv a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestFdivWithSharedA(double a, double b, double expected)
{
  double result = a;
  asm("fdiv %0, %0, %1" : "+d"(result) : "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fdiv (a=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestFdivWithSharedB(double a, double b, double expected)
{
  double result = b;
  asm("fdiv %0, %1, %0" : "+d"(result) : "d"(a));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fdiv (b=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsDivLower(double a, double b, double expected)
{
  double result, a_tmp, b_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 8(%3), 0, 0\n\t"
      "ps_merge01 %1, %4, %1\n\t"
      "ps_merge01 %2, %5, %2\n\t"
      "ps_div %0, %1, %2"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_div a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsDivUpper(double a, double b, double expected)
{
  double result, a_tmp, b_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 8(%3), 0, 0\n\t"
      "ps_merge00 %1, %1, %4\n\t"
      "ps_merge00 %2, %2, %5\n\t"
      "ps_div %0, %1, %2\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_div a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsDivLowerWithSharedA(double a, double b, double expected)
{
  double result, b_tmp;
  asm("psq_l %0, 0(%2), 0, 0\n\t"
      "psq_l %1, 8(%2), 0, 0\n\t"
      "ps_merge01 %0, %3, %0\n\t"
      "ps_merge01 %1, %4, %1\n\t"
      "ps_div %0, %0, %1"
      : "=d"(result), "=d"(b_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_div (a=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsDivUpperWithSharedA(double a, double b, double expected)
{
  double result, b_tmp;
  asm("psq_l %0, 0(%2), 0, 0\n\t"
      "psq_l %1, 8(%2), 0, 0\n\t"
      "ps_merge00 %0, %0, %3\n\t"
      "ps_merge00 %1, %1, %4\n\t"
      "ps_div %0, %0, %1\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(b_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_div (a=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsDivLowerWithSharedB(double a, double b, double expected)
{
  double result, a_tmp;
  asm("psq_l %1, 0(%2), 0, 0\n\t"
      "psq_l %0, 8(%2), 0, 0\n\t"
      "ps_merge01 %1, %3, %1\n\t"
      "ps_merge01 %0, %4, %0\n\t"
      "ps_div %0, %1, %0"
      : "=d"(result), "=d"(a_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_div (b=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsDivUpperWithSharedB(double a, double b, double expected)
{
  double result, a_tmp;
  asm("psq_l %1, 0(%2), 0, 0\n\t"
      "psq_l %0, 8(%2), 0, 0\n\t"
      "ps_merge00 %1, %1, %3\n\t"
      "ps_merge00 %0, %0, %4\n\t"
      "ps_div %0, %1, %0\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(a_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_div (b=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

// a * c
static void TestFmul(double a, double c, double expected)
{
  double result;
  asm("fmul %0, %1, %2" : "=d"(result) : "d"(a), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fmul a={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestFmulWithSharedA(double a, double c, double expected)
{
  double result = a;
  asm("fmul %0, %0, %1" : "+d"(result) : "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fmul (a=d) a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestFmulWithSharedC(double a, double c, double expected)
{
  double result = c;
  asm("fmul %0, %1, %0" : "+d"(result) : "d"(a));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fmul (c=d) a={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMulLower(double a, double c, double expected)
{
  double result, a_tmp, c_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 16(%3), 0, 0\n\t"
      "ps_merge01 %1, %4, %1\n\t"
      "ps_merge01 %2, %5, %2\n\t"
      "ps_mul %0, %1, %2"
      : "=d"(result), "=d"(a_tmp), "=d"(c_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_mul a={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMulUpper(double a, double c, double expected)
{
  double result, a_tmp, c_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 16(%3), 0, 0\n\t"
      "ps_merge00 %1, %1, %4\n\t"
      "ps_merge00 %2, %2, %5\n\t"
      "ps_mul %0, %1, %2\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(a_tmp), "=d"(c_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_mul a={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMulLowerWithSharedA(double a, double c, double expected)
{
  double result, c_tmp;
  asm("psq_l %0, 0(%2), 0, 0\n\t"
      "psq_l %1, 16(%2), 0, 0\n\t"
      "ps_merge01 %0, %3, %0\n\t"
      "ps_merge01 %1, %4, %1\n\t"
      "ps_mul %0, %0, %1"
      : "=d"(result), "=d"(c_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_mul (a=d) a={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMulUpperWithSharedA(double a, double c, double expected)
{
  double result, c_tmp;
  asm("psq_l %0, 0(%2), 0, 0\n\t"
      "psq_l %1, 16(%2), 0, 0\n\t"
      "ps_merge00 %0, %0, %3\n\t"
      "ps_merge00 %1, %1, %4\n\t"
      "ps_mul %0, %0, %1\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(c_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_mul (a=d) a={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMulLowerWithSharedC(double a, double c, double expected)
{
  double result, a_tmp;
  asm("psq_l %1, 0(%2), 0, 0\n\t"
      "psq_l %0, 16(%2), 0, 0\n\t"
      "ps_merge01 %1, %3, %1\n\t"
      "ps_merge01 %0, %4, %0\n\t"
      "ps_mul %0, %1, %0"
      : "=d"(result), "=d"(a_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_mul (c=d) a={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMulUpperWithSharedC(double a, double c, double expected)
{
  double result, a_tmp;
  asm("psq_l %1, 0(%2), 0, 0\n\t"
      "psq_l %0, 16(%2), 0, 0\n\t"
      "ps_merge00 %1, %1, %3\n\t"
      "ps_merge00 %0, %0, %4\n\t"
      "ps_mul %0, %1, %0\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(a_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_mul (c=d) a={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

// (a * c) + b
static void TestFmadd(double a, double b, double c, double expected)
{
  double result;
  asm("fmadd %0, %1, %3, %2" : "=d"(result) : "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fmadd a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMaddLower(double a, double b, double c, double expected)
{
  double result, a_tmp, b_tmp, c_tmp;
  asm("psq_l %1, 0(%4), 0, 0\n\t"
      "psq_l %2, 8(%4), 0, 0\n\t"
      "psq_l %3, 16(%4), 0, 0\n\t"
      "ps_merge01 %1, %5, %1\n\t"
      "ps_merge01 %2, %6, %2\n\t"
      "ps_merge01 %3, %7, %3\n\t"
      "ps_madd %0, %1, %3, %2"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp), "=d"(c_tmp)
      : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_madd a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMaddUpper(double a, double b, double c, double expected)
{
  double result, a_tmp, b_tmp, c_tmp;
  asm("psq_l %1, 0(%4), 0, 0\n\t"
      "psq_l %2, 8(%4), 0, 0\n\t"
      "psq_l %3, 16(%4), 0, 0\n\t"
      "ps_merge00 %1, %1, %5\n\t"
      "ps_merge00 %2, %2, %6\n\t"
      "ps_merge00 %3, %3, %7\n\t"
      "ps_madd %0, %1, %3, %2\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp), "=d"(c_tmp)
      : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_madd a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

// -((a * c) + b)
static void TestFnmadd(double a, double b, double c, double expected)
{
  double result;
  asm("fnmadd %0, %1, %3, %2" : "=d"(result) : "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fnmadd a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestFnmaddWithSharedA(double a, double b, double c, double expected)
{
  double result = a;
  asm("fnmadd %0, %0, %2, %1" : "+d"(result) : "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fnmadd (a=d) a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestFnmaddWithSharedB(double a, double b, double c, double expected)
{
  double result = b;
  asm("fnmadd %0, %1, %2, %0" : "+d"(result) : "d"(a), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fnmadd (b=d) a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestFnmaddWithSharedC(double a, double b, double c, double expected)
{
  double result = c;
  asm("fnmadd %0, %1, %0, %2" : "+d"(result) : "d"(a), "d"(b));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fnmadd (c=d) a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestFnmaddWithSharedABC(double input, double expected)
{
  double result = input;
  asm("fnmadd %0, %0, %0, %0" : "+d"(result));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "fnmadd (a=b=c=d) a=b=c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(input),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsNmaddLower(double a, double b, double c, double expected)
{
  double result, a_tmp, b_tmp, c_tmp;
  asm("psq_l %1, 0(%4), 0, 0\n\t"
      "psq_l %2, 8(%4), 0, 0\n\t"
      "psq_l %3, 16(%4), 0, 0\n\t"
      "ps_merge01 %1, %5, %1\n\t"
      "ps_merge01 %2, %6, %2\n\t"
      "ps_merge01 %3, %7, %3\n\t"
      "ps_nmadd %0, %1, %3, %2"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp), "=d"(c_tmp)
      : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_nmadd a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsNmaddUpper(double a, double b, double c, double expected)
{
  double result, a_tmp, b_tmp, c_tmp;
  asm("psq_l %1, 0(%4), 0, 0\n\t"
      "psq_l %2, 8(%4), 0, 0\n\t"
      "psq_l %3, 16(%4), 0, 0\n\t"
      "ps_merge00 %1, %1, %5\n\t"
      "ps_merge00 %2, %2, %6\n\t"
      "ps_merge00 %3, %3, %7\n\t"
      "ps_nmadd %0, %1, %3, %2\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp), "=d"(c_tmp)
      : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_nmadd a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsNmaddLowerWithSharedA(double a, double b, double c, double expected)
{
  double result, b_tmp, c_tmp;
  asm("psq_l %0, 0(%3), 0, 0\n\t"
      "psq_l %1, 8(%3), 0, 0\n\t"
      "psq_l %2, 16(%3), 0, 0\n\t"
      "ps_merge01 %0, %4, %0\n\t"
      "ps_merge01 %1, %5, %1\n\t"
      "ps_merge01 %2, %6, %2\n\t"
      "ps_nmadd %0, %0, %2, %1"
      : "=d"(result), "=d"(b_tmp), "=d"(c_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_nmadd (a=d) a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsNmaddUpperWithSharedA(double a, double b, double c, double expected)
{
  double result, b_tmp, c_tmp;
  asm("psq_l %0, 0(%3), 0, 0\n\t"
      "psq_l %1, 8(%3), 0, 0\n\t"
      "psq_l %2, 16(%3), 0, 0\n\t"
      "ps_merge00 %0, %0, %4\n\t"
      "ps_merge00 %1, %1, %5\n\t"
      "ps_merge00 %2, %2, %6\n\t"
      "ps_nmadd %0, %0, %2, %1\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(b_tmp), "=d"(c_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_nmadd (a=d) a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsNmaddLowerWithSharedB(double a, double b, double c, double expected)
{
  double result, a_tmp, c_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 8(%3), 0, 0\n\t"
      "psq_l %3, 16(%3), 0, 0\n\t"
      "ps_merge01 %1, %4, %1\n\t"
      "ps_merge01 %2, %5, %2\n\t"
      "ps_merge01 %3, %6, %3\n\t"
      "ps_nmadd %0, %1, %3, %2"
      : "=d"(result), "=d"(a_tmp), "=d"(c_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_nmadd (b=d) a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsNmaddUpperWithSharedB(double a, double b, double c, double expected)
{
  double result, a_tmp, c_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %0, 8(%3), 0, 0\n\t"
      "psq_l %2, 16(%3), 0, 0\n\t"
      "ps_merge00 %1, %1, %4\n\t"
      "ps_merge00 %0, %0, %5\n\t"
      "ps_merge00 %2, %2, %6\n\t"
      "ps_nmadd %0, %1, %2, %0\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(a_tmp), "=d"(c_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_nmadd (b=d) a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsNmaddLowerWithSharedC(double a, double b, double c, double expected)
{
  double result, a_tmp, b_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 8(%3), 0, 0\n\t"
      "psq_l %0, 16(%3), 0, 0\n\t"
      "ps_merge01 %1, %4, %1\n\t"
      "ps_merge01 %2, %5, %2\n\t"
      "ps_merge01 %0, %6, %0\n\t"
      "ps_nmadd %0, %1, %0, %2"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_nmadd (c=d) a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsNmaddUpperWithSharedC(double a, double b, double c, double expected)
{
  double result, a_tmp, b_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 8(%3), 0, 0\n\t"
      "psq_l %0, 16(%3), 0, 0\n\t"
      "ps_merge00 %1, %1, %4\n\t"
      "ps_merge00 %2, %2, %5\n\t"
      "ps_merge00 %0, %0, %6\n\t"
      "ps_nmadd %0, %1, %0, %2\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_nmadd (c=d) a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsNmaddLowerWithSharedABC(double input, double expected)
{
  double result;
  asm("psq_l %0, 0(%1), 0, 0\n\t"
      "ps_merge01 %0, %2, %0\n\t"
      "ps_nmadd %0, %0, %0, %0"
      : "=d"(result) : "b"(SINGLE_SNANS.data()), "d"(input));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_nmadd (a=b=c=d) a=b=c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(input),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsNmaddUpperWithSharedABC(double input, double expected)
{
  double result;
  asm("psq_l %1, 0(%1), 0, 0\n\t"
      "ps_merge00 %0, %0, %2\n\t"
      "ps_nmadd %0, %0, %0, %0\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result) : "b"(SINGLE_SNANS.data()), "d"(input));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_nmadd (a=b=c=d) a=b=c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(input),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsSum0(double a, double b, double ps0_expected)
{
  double ps0_result, ps1_result, a_tmp, b_tmp, c_tmp;
  asm("psq_l %2, 0(%5), 0, 0\n\t"
      "psq_l %3, 8(%5), 0, 0\n\t"
      "psq_l %4, 16(%5), 0, 0\n\t"
      "ps_merge01 %2, %6, %2\n\t"
      "ps_merge00 %3, %3, %7\n\t"
      "ps_sum0 %0, %2, %4, %3\n\t"
      "ps_merge11 %1, %0, %0"
      : "=d"(ps0_result), "=d"(ps1_result), "=d"(a_tmp), "=d"(b_tmp), "=d"(c_tmp)
      : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  const u64 ps1_expected = Common::BitCast<u64>(static_cast<double>(Common::BitCast<float>(SINGLE_SNANS[5]))) & ~QUIET_BIT;
  DO_TEST(Common::BitCast<u64>(ps0_result) == Common::BitCast<u64>(ps0_expected) &&
          Common::BitCast<u64>(ps1_result) == ps1_expected, "\n"
          "ps_sum0 a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}, {:#018x}\n"
          "expected {:#018x}, {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(ps0_result), Common::BitCast<u64>(ps1_result),
          Common::BitCast<u64>(ps0_expected), Common::BitCast<u64>(ps1_expected));
}

static void TestPsSum1(double a, double b, double ps1_expected)
{
  double ps0_result, ps1_result, a_tmp, b_tmp, c_tmp;
  asm("psq_l %2, 0(%5), 0, 0\n\t"
      "psq_l %3, 8(%5), 0, 0\n\t"
      "psq_l %4, 16(%5), 0, 0\n\t"
      "ps_merge01 %2, %6, %2\n\t"
      "ps_merge00 %3, %3, %7\n\t"
      "ps_sum1 %0, %2, %4, %3\n\t"
      "ps_merge11 %1, %0, %0"
      : "=d"(ps0_result), "=d"(ps1_result), "=d"(a_tmp), "=d"(b_tmp), "=d"(c_tmp)
      : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b));

  const u64 ps0_expected = Common::BitCast<u64>(static_cast<double>(Common::BitCast<float>(SINGLE_SNANS[4]))) & ~QUIET_BIT;
  DO_TEST(Common::BitCast<u64>(ps0_result) == ps0_expected &&
          Common::BitCast<u64>(ps1_result) == Common::BitCast<u64>(ps1_expected), "\n"
          "ps_sum1 a={:#018x}, b={:#018x}:\n"
          "     got {:#018x}, {:#018x}\n"
          "expected {:#018x}, {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b),
          Common::BitCast<u64>(ps0_result), Common::BitCast<u64>(ps1_result),
          Common::BitCast<u64>(ps0_expected), Common::BitCast<u64>(ps1_expected));
}

static void TestPsMuls0Lower(double a, double c, double expected)
{
  double result, a_tmp, c_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 16(%3), 0, 0\n\t"
      "ps_merge01 %1, %4, %1\n\t"
      "ps_merge01 %2, %5, %2\n\t"
      "ps_muls0 %0, %1, %2"
      : "=d"(result), "=d"(a_tmp), "=d"(c_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_muls0 a={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMuls0Upper(double a, double c, double expected)
{
  double result, a_tmp, c_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 16(%3), 0, 0\n\t"
      "ps_merge00 %1, %1, %4\n\t"
      "ps_merge01 %2, %5, %2\n\t"
      "ps_muls0 %0, %1, %2\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(a_tmp), "=d"(c_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_muls0 a={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMuls1Lower(double a, double c, double expected)
{
  double result, a_tmp, c_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 16(%3), 0, 0\n\t"
      "ps_merge01 %1, %4, %1\n\t"
      "ps_merge00 %2, %2, %5\n\t"
      "ps_muls1 %0, %1, %2"
      : "=d"(result), "=d"(a_tmp), "=d"(c_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_muls1 a={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMuls1Upper(double a, double c, double expected)
{
  double result, a_tmp, c_tmp;
  asm("psq_l %1, 0(%3), 0, 0\n\t"
      "psq_l %2, 16(%3), 0, 0\n\t"
      "ps_merge00 %1, %1, %4\n\t"
      "ps_merge00 %2, %2, %5\n\t"
      "ps_muls1 %0, %1, %2\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(a_tmp), "=d"(c_tmp) : "b"(SINGLE_SNANS.data()), "d"(a), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_muls1 a={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMadds0Lower(double a, double b, double c, double expected)
{
  double result, a_tmp, b_tmp, c_tmp;
  asm("psq_l %1, 0(%4), 0, 0\n\t"
      "psq_l %2, 8(%4), 0, 0\n\t"
      "psq_l %3, 16(%4), 0, 0\n\t"
      "ps_merge01 %1, %5, %1\n\t"
      "ps_merge01 %2, %6, %2\n\t"
      "ps_merge01 %3, %7, %3\n\t"
      "ps_madds0 %0, %1, %3, %2"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp), "=d"(c_tmp)
      : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_madds0 a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMadds0Upper(double a, double b, double c, double expected)
{
  double result, a_tmp, b_tmp, c_tmp;
  asm("psq_l %1, 0(%4), 0, 0\n\t"
      "psq_l %2, 8(%4), 0, 0\n\t"
      "psq_l %3, 16(%4), 0, 0\n\t"
      "ps_merge00 %1, %1, %5\n\t"
      "ps_merge00 %2, %2, %6\n\t"
      "ps_merge01 %3, %7, %3\n\t"
      "ps_madds0 %0, %1, %3, %2\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp), "=d"(c_tmp)
      : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_madds0 a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMadds1Lower(double a, double b, double c, double expected)
{
  double result, a_tmp, b_tmp, c_tmp;
  asm("psq_l %1, 0(%4), 0, 0\n\t"
      "psq_l %2, 8(%4), 0, 0\n\t"
      "psq_l %3, 16(%4), 0, 0\n\t"
      "ps_merge01 %1, %5, %1\n\t"
      "ps_merge01 %2, %6, %2\n\t"
      "ps_merge00 %3, %3, %7\n\t"
      "ps_madds1 %0, %1, %3, %2"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp), "=d"(c_tmp)
      : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_madds1 a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

static void TestPsMadds1Upper(double a, double b, double c, double expected)
{
  double result, a_tmp, b_tmp, c_tmp;
  asm("psq_l %1, 0(%4), 0, 0\n\t"
      "psq_l %2, 8(%4), 0, 0\n\t"
      "psq_l %3, 16(%4), 0, 0\n\t"
      "ps_merge00 %1, %1, %5\n\t"
      "ps_merge00 %2, %2, %6\n\t"
      "ps_merge00 %3, %3, %7\n\t"
      "ps_madds1 %0, %1, %3, %2\n\t"
      "ps_merge11 %0, %0, %0"
      : "=d"(result), "=d"(a_tmp), "=d"(b_tmp), "=d"(c_tmp)
      : "b"(SINGLE_SNANS.data()), "d"(a), "d"(b), "d"(c));

  DO_TEST(Common::BitCast<u64>(result) == Common::BitCast<u64>(expected), "\n"
          "ps_madds1 a={:#018x}, b={:#018x}, c={:#018x}:\n"
          "     got {:#018x}\n"
          "expected {:#018x}",
          Common::BitCast<u64>(a), Common::BitCast<u64>(b), Common::BitCast<u64>(c),
          Common::BitCast<u64>(result), Common::BitCast<u64>(expected));
}

template <typename F>
static void NanAddTest(F f)
{
  START_TEST();

  f(0.0, 0.0, 0.0);

  f(Q_NEG_NAN_1, 0.0, Q_NEG_NAN_1);
  f(0.0, Q_NEG_NAN_2, Q_NEG_NAN_2);

  f(Q_POS_NAN_1, Q_POS_NAN_2, Q_POS_NAN_1);
  f(S_POS_NAN_1, Q_POS_NAN_2, Q_POS_NAN_1);
  f(Q_POS_NAN_1, S_POS_NAN_2, Q_POS_NAN_1);
  f(S_POS_NAN_1, S_POS_NAN_2, Q_POS_NAN_1);

  END_TEST();
}

template <typename F>
static void NanDivTest(F f)
{
  START_TEST();

  f(1.0, 1.0, 1.0);
  f(0.0, 0.0, DEFAULT_NAN);
  f(-0.0, -0.0, DEFAULT_NAN);

  f(Q_NEG_NAN_1, 0.0, Q_NEG_NAN_1);
  f(0.0, Q_NEG_NAN_2, Q_NEG_NAN_2);

  f(Q_POS_NAN_1, Q_POS_NAN_2, Q_POS_NAN_1);
  f(S_POS_NAN_1, Q_POS_NAN_2, Q_POS_NAN_1);
  f(Q_POS_NAN_1, S_POS_NAN_2, Q_POS_NAN_1);
  f(S_POS_NAN_1, S_POS_NAN_2, Q_POS_NAN_1);

  END_TEST();
}

template <typename F>
static void NanMulTest(F f)
{
  START_TEST();

  f(1.0, 1.0, 1.0);
  f(std::numeric_limits<double>::infinity(), 0.0, DEFAULT_NAN);
  f(-std::numeric_limits<double>::infinity(), 0.0, DEFAULT_NAN);

  f(Q_NEG_NAN_1, 0.0, Q_NEG_NAN_1);
  f(0.0, Q_NEG_NAN_2, Q_NEG_NAN_2);

  f(Q_POS_NAN_1, Q_POS_NAN_2, Q_POS_NAN_1);
  f(S_POS_NAN_1, Q_POS_NAN_2, Q_POS_NAN_1);
  f(Q_POS_NAN_1, S_POS_NAN_2, Q_POS_NAN_1);
  f(S_POS_NAN_1, S_POS_NAN_2, Q_POS_NAN_1);

  END_TEST();
}

template <typename F>
static void NanMaddTest(F f, double expected_result_of_first_test)
{
  START_TEST();

  f(1.0, 0.0, 1.0, expected_result_of_first_test);
  f(std::numeric_limits<double>::infinity(), 1.0, 0.0, DEFAULT_NAN);
  f(-std::numeric_limits<double>::infinity(), 1.0, 0.0, DEFAULT_NAN);
  f(std::numeric_limits<double>::infinity(), Q_NEG_NAN_2, 0.0, Q_NEG_NAN_2);

  f(Q_NEG_NAN_1, 0.0, 0.0, Q_NEG_NAN_1);
  f(0.0, Q_NEG_NAN_2, 0.0, Q_NEG_NAN_2);
  f(0.0, 0.0, Q_NEG_NAN_3, Q_NEG_NAN_3);

  f(Q_NEG_NAN_1, Q_NEG_NAN_2, 0.0, Q_NEG_NAN_1);
  f(Q_NEG_NAN_1, 0.0, Q_NEG_NAN_3, Q_NEG_NAN_1);
  f(0.0, Q_NEG_NAN_2, Q_NEG_NAN_3, Q_NEG_NAN_2);

  f(Q_POS_NAN_1, Q_POS_NAN_2, Q_POS_NAN_3, Q_POS_NAN_1);
  f(S_POS_NAN_1, Q_POS_NAN_2, Q_POS_NAN_3, Q_POS_NAN_1);
  f(Q_POS_NAN_1, S_POS_NAN_2, Q_POS_NAN_3, Q_POS_NAN_1);
  f(S_POS_NAN_1, S_POS_NAN_2, Q_POS_NAN_3, Q_POS_NAN_1);
  f(Q_POS_NAN_1, Q_POS_NAN_2, S_POS_NAN_3, Q_POS_NAN_1);
  f(S_POS_NAN_1, Q_POS_NAN_2, S_POS_NAN_3, Q_POS_NAN_1);
  f(Q_POS_NAN_1, S_POS_NAN_2, S_POS_NAN_3, Q_POS_NAN_1);
  f(S_POS_NAN_1, S_POS_NAN_2, S_POS_NAN_3, Q_POS_NAN_1);

  END_TEST();
}

template <typename F>
static void NanMaddWithSharedABCTest(F f)
{
  START_TEST();

  f(1.5, -3.75);
  f(Q_POS_NAN_1, Q_POS_NAN_1);
  f(Q_NEG_NAN_1, Q_NEG_NAN_1);

  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  network_printf("Testing fadd...\n");
  NanAddTest(&TestFadd);
  NanAddTest(&TestFaddWithSharedA);
  NanAddTest(&TestFaddWithSharedB);

  network_printf("Testing ps_add...\n");
  NanAddTest(&TestPsAddLower);
  NanAddTest(&TestPsAddUpper);
  NanAddTest(&TestPsAddLowerWithSharedA);
  NanAddTest(&TestPsAddUpperWithSharedA);
  NanAddTest(&TestPsAddLowerWithSharedB);
  NanAddTest(&TestPsAddUpperWithSharedB);

  network_printf("Testing fdiv...\n");
  NanDivTest(&TestFdiv);
  NanDivTest(&TestFdivWithSharedA);
  NanDivTest(&TestFdivWithSharedB);

  network_printf("Testing ps_div...\n");
  NanDivTest(&TestPsDivLower);
  NanDivTest(&TestPsDivUpper);
  NanDivTest(&TestPsDivLowerWithSharedA);
  NanDivTest(&TestPsDivUpperWithSharedA);
  NanDivTest(&TestPsDivLowerWithSharedB);
  NanDivTest(&TestPsDivUpperWithSharedB);

  network_printf("Testing fmul...\n");
  NanMulTest(&TestFmul);
  NanMulTest(&TestFmulWithSharedA);
  NanMulTest(&TestFmulWithSharedC);

  network_printf("Testing ps_mul...\n");
  NanMulTest(&TestPsMulLower);
  NanMulTest(&TestPsMulUpper);
  NanMulTest(&TestPsMulLowerWithSharedA);
  NanMulTest(&TestPsMulUpperWithSharedA);
  NanMulTest(&TestPsMulLowerWithSharedC);
  NanMulTest(&TestPsMulUpperWithSharedC);

  network_printf("Testing fmadd...\n");
  NanMaddTest(&TestFmadd, 1.0);

  network_printf("Testing ps_madd...\n");
  NanMaddTest(&TestPsMaddLower, 1.0);
  NanMaddTest(&TestPsMaddUpper, 1.0);

  network_printf("Testing fnmadd...\n");
  NanMaddTest(&TestFnmadd, -1.0);
  NanMaddTest(&TestFnmaddWithSharedA, -1.0);
  NanMaddTest(&TestFnmaddWithSharedB, -1.0);
  NanMaddTest(&TestFnmaddWithSharedC, -1.0);
  NanMaddWithSharedABCTest(&TestFnmaddWithSharedABC);

  network_printf("Testing ps_nmadd...\n");
  NanMaddTest(&TestPsNmaddLower, -1.0);
  NanMaddTest(&TestPsNmaddUpper, -1.0);
  NanMaddTest(&TestPsNmaddLowerWithSharedA, -1.0);
  NanMaddTest(&TestPsNmaddUpperWithSharedA, -1.0);
  NanMaddTest(&TestPsNmaddLowerWithSharedB, -1.0);
  NanMaddTest(&TestPsNmaddUpperWithSharedB, -1.0);
  NanMaddTest(&TestPsNmaddLowerWithSharedC, -1.0);
  NanMaddTest(&TestPsNmaddUpperWithSharedC, -1.0);
  NanMaddWithSharedABCTest(&TestPsNmaddLowerWithSharedABC);
  NanMaddWithSharedABCTest(&TestPsNmaddUpperWithSharedABC);

  network_printf("Testing ps_sum0...\n");
  NanAddTest(&TestPsSum0);

  network_printf("Testing ps_sum1...\n");
  NanAddTest(&TestPsSum1);

  network_printf("Testing ps_muls0...\n");
  NanMulTest(&TestPsMuls0Lower);
  NanMulTest(&TestPsMuls0Upper);

  network_printf("Testing ps_muls1...\n");
  NanMulTest(&TestPsMuls1Lower);
  NanMulTest(&TestPsMuls1Upper);

  network_printf("Testing ps_madds0...\n");
  NanMaddTest(&TestPsMadds0Lower, 1.0);
  NanMaddTest(&TestPsMadds0Upper, 1.0);

  network_printf("Testing ps_madds1...\n");
  NanMaddTest(&TestPsMadds1Lower, 1.0);
  NanMaddTest(&TestPsMadds1Upper, 1.0);

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

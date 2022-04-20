#include <gctypes.h>
#include <wiiuse/wpad.h>
#include "common/CommonFuncs.h"
#include "common/hwtests.h"

// Generates a bitmask from the ten bits encoded in the instruction.
// mb and me are what the two 5 bit sections are called in the instruction encoding.
// Mask is what is used by the instructions for determining which bits are used.
// Copied from Dolphin's Interpreter integer operations.
static u32 GetHelperMask(int mb, int me)
{
  // first make 001111111111111 part
  // then make 000000000001111 part, which is used to flip the bits of the first one
  // do the bitflip
  // and invert if backwards
  u32 begin = 0xFFFFFFFF >> mb;
  u32 end = me < 31 ? (0xFFFFFFFF >> (me + 1)) : 0;
  u32 mask = begin ^ end;
  if (me < mb)
    return ~mask;
  else
    return mask;
}

static void rlwimixTest()
{
  START_TEST();

  u32 values[][2] = {
      {0xFFFFFFFF, 0x41414141}, {0x44444444, 0xFFFFFFFF}, {0x41414141, 0x22222222},
  };

#define RLWIMIX_TEST(sh, mb, me)                                                                   \
  do                                                                                               \
  {                                                                                                \
    for (int i = 0; i < (sizeof(values) / sizeof(values[0])); ++i)                                 \
    {                                                                                              \
      u32 mask = GetHelperMask(mb, me);                                                            \
      u32 computed_result = (values[i][0] & ~mask) | (_rotl(values[i][1], sh) & mask);             \
                                                                                                   \
      u32 valueA = values[i][0];                                                                   \
      u32 valueS = values[i][1];                                                                   \
                                                                                                   \
      asm("rlwimi %0, %1, " #sh ", " #mb ", " #me : "+r"(valueA) : "r"(valueS));                   \
      DO_TEST(valueA == computed_result, "rlwimi {:08x}, {:08x}, " #sh ", " #mb ", " #me "\n"      \
                                         "\tgot: {:08x}\n"                                         \
                                         "\texpected: {:08x}\n",                                   \
              values[i][0], values[i][1], valueA, computed_result);                                \
    }                                                                                              \
  } while (0)

  RLWIMIX_TEST(0, 0, 0);
  RLWIMIX_TEST(0, 0, 15);
  RLWIMIX_TEST(0, 0, 31);
  RLWIMIX_TEST(0, 15, 0);
  RLWIMIX_TEST(0, 15, 15);
  RLWIMIX_TEST(0, 15, 31);
  RLWIMIX_TEST(0, 31, 0);
  RLWIMIX_TEST(15, 0, 0);
  RLWIMIX_TEST(15, 0, 15);
  RLWIMIX_TEST(15, 0, 31);
  RLWIMIX_TEST(15, 15, 0);
  RLWIMIX_TEST(15, 15, 15);
  RLWIMIX_TEST(15, 15, 31);
  RLWIMIX_TEST(15, 31, 0);
  RLWIMIX_TEST(15, 31, 15);
  RLWIMIX_TEST(15, 31, 31);
  RLWIMIX_TEST(31, 0, 0);
  RLWIMIX_TEST(31, 0, 15);
  RLWIMIX_TEST(31, 0, 31);
  RLWIMIX_TEST(31, 15, 0);
  RLWIMIX_TEST(31, 15, 15);
  RLWIMIX_TEST(31, 15, 31);
  RLWIMIX_TEST(31, 31, 0);
  RLWIMIX_TEST(31, 31, 15);
  RLWIMIX_TEST(31, 31, 31);
  END_TEST();
}

static void rlwinmxTest()
{
  START_TEST();

  u32 values[] = {
      0xFFFFFFFF, 0x44444444, 0x41414141, 0x12345678, 0x90ABCDEF,
  };

#define RLWINMX_TEST(sh, mb, me)                                                                   \
  do                                                                                               \
  {                                                                                                \
    for (int i = 0; i < (sizeof(values) / sizeof(values[0])); ++i)                                 \
    {                                                                                              \
      u32 mask = GetHelperMask(mb, me);                                                            \
      u32 computed_result = _rotl(values[i], sh) & mask;                                           \
                                                                                                   \
      u32 result = 0;                                                                              \
      u32 valueS = values[i];                                                                      \
                                                                                                   \
      asm("rlwinm %0, %1, " #sh ", " #mb ", " #me : "=r"(result) : "r"(valueS));                   \
      DO_TEST(result == computed_result, "rlwinm {:08x}, " #sh ", " #mb ", " #me "\n"              \
                                         "\tgot: {:08x}\n"                                         \
                                         "\texpected: {:08x}\n",                                   \
              values[i], result, computed_result);                                                 \
    }                                                                                              \
  } while (0)

  RLWINMX_TEST(0, 0, 0);
  RLWINMX_TEST(0, 0, 15);
  RLWINMX_TEST(0, 0, 31);
  RLWINMX_TEST(0, 15, 0);
  RLWINMX_TEST(0, 15, 15);
  RLWINMX_TEST(0, 15, 31);
  RLWINMX_TEST(0, 31, 0);
  RLWINMX_TEST(15, 0, 0);
  RLWINMX_TEST(15, 0, 15);
  RLWINMX_TEST(15, 0, 31);
  RLWINMX_TEST(15, 15, 0);
  RLWINMX_TEST(15, 15, 15);
  RLWINMX_TEST(15, 15, 31);
  RLWINMX_TEST(15, 31, 0);
  RLWINMX_TEST(15, 31, 15);
  RLWINMX_TEST(15, 31, 31);
  RLWINMX_TEST(31, 0, 0);
  RLWINMX_TEST(31, 0, 15);
  RLWINMX_TEST(31, 0, 31);
  RLWINMX_TEST(31, 15, 0);
  RLWINMX_TEST(31, 15, 15);
  RLWINMX_TEST(31, 15, 31);
  RLWINMX_TEST(31, 31, 0);
  RLWINMX_TEST(31, 31, 15);
  RLWINMX_TEST(31, 31, 31);
  END_TEST();
}

static void rlwnmxTest()
{
  START_TEST();

  u32 values[] = {
      0xFFFFFFFF, 0x44444444, 0x41414141, 0x12345678, 0x90ABCDEF,
  };

#define RLWNMX_TEST(mb, me)                                                                        \
  do                                                                                               \
  {                                                                                                \
    for (int sh = 0; sh < 32; ++sh)                                                                \
      for (int i = 0; i < (sizeof(values) / sizeof(values[0])); ++i)                               \
      {                                                                                            \
        u32 mask = GetHelperMask(mb, me);                                                          \
        u32 computed_result = _rotl(values[i], sh & 0x1F) & mask;                                  \
                                                                                                   \
        u32 result = 0;                                                                            \
        u32 valueS = values[i];                                                                    \
                                                                                                   \
        asm("rlwnm %0, %1, %2, " #mb ", " #me : "=r"(result) : "r"(valueS), "r"(sh));              \
        DO_TEST(result == computed_result, "rlwnm {:08x}, {}, " #mb ", " #me "\n"                  \
                                           "\tgot: {:08x}\n"                                       \
                                           "\texpected: {:08x}\n",                                 \
                values[i], sh, result, computed_result);                                           \
      }                                                                                            \
  } while (0)

  RLWNMX_TEST(0, 0);
  RLWNMX_TEST(0, 7);
  RLWNMX_TEST(0, 15);
  RLWNMX_TEST(0, 23);
  RLWNMX_TEST(0, 31);
  RLWNMX_TEST(7, 0);
  RLWNMX_TEST(7, 7);
  RLWNMX_TEST(7, 15);
  RLWNMX_TEST(7, 23);
  RLWNMX_TEST(7, 31);
  RLWNMX_TEST(15, 0);
  RLWNMX_TEST(15, 7);
  RLWNMX_TEST(15, 15);
  RLWNMX_TEST(15, 23);
  RLWNMX_TEST(15, 31);
  RLWNMX_TEST(23, 0);
  RLWNMX_TEST(23, 7);
  RLWNMX_TEST(23, 15);
  RLWNMX_TEST(23, 23);
  RLWNMX_TEST(23, 31);
  RLWNMX_TEST(31, 0);
  RLWNMX_TEST(31, 7);
  RLWNMX_TEST(31, 15);
  RLWNMX_TEST(31, 23);
  RLWNMX_TEST(31, 31);
  END_TEST();
}
int main()
{
  network_init();

  rlwimixTest();
  rlwinmxTest();
  rlwnmxTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

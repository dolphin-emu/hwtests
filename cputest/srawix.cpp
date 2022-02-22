#include <gctypes.h>
#include <limits.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include "common/hwtests.h"

static int GetCarry(int value, int shift)
{
  if (shift == 0)
    return 0;
  return (value < 0) && (((value >> shift) << shift) != value);
}

#define SRAWIX_TEST(shift)                                                                         \
  do                                                                                               \
  {                                                                                                \
    for (int i = 0; i < 0x1000; i++)                                                               \
    {                                                                                              \
      s32 input = i ? rand() : INT_MIN;                                                            \
      s32 output = input;                                                                          \
      s32 carry = 0;                                                                               \
      asm("srawi %0, %0, %2;"                                                                      \
          "addze  %1, %1;"                                                                         \
          : "+r"(output), "+r"(carry)                                                              \
          : "i"(shift));                                                                           \
      DO_TEST(carry == GetCarry(input, shift), "({:x} >> {}), got carry = {:x}, expected {:x}",    \
              input, shift, carry, GetCarry(input, shift));                                        \
      DO_TEST(output == (input >> shift), "({:x} >> {}), got {:x}, expected {:x}", input, shift,   \
              output, (input >> shift));                                                           \
    }                                                                                              \
  } while (0)

int main()
{
  network_init();
  WPAD_Init();

  START_TEST();
  SRAWIX_TEST(0);
  SRAWIX_TEST(1);
  SRAWIX_TEST(2);
  SRAWIX_TEST(3);
  SRAWIX_TEST(4);
  SRAWIX_TEST(5);
  SRAWIX_TEST(6);
  SRAWIX_TEST(7);
  SRAWIX_TEST(8);
  SRAWIX_TEST(9);
  SRAWIX_TEST(10);
  SRAWIX_TEST(11);
  SRAWIX_TEST(12);
  SRAWIX_TEST(13);
  SRAWIX_TEST(14);
  SRAWIX_TEST(15);
  SRAWIX_TEST(16);
  SRAWIX_TEST(17);
  SRAWIX_TEST(18);
  SRAWIX_TEST(19);
  SRAWIX_TEST(20);
  SRAWIX_TEST(21);
  SRAWIX_TEST(22);
  SRAWIX_TEST(23);
  SRAWIX_TEST(24);
  SRAWIX_TEST(25);
  SRAWIX_TEST(26);
  SRAWIX_TEST(27);
  SRAWIX_TEST(28);
  SRAWIX_TEST(29);
  SRAWIX_TEST(30);
  SRAWIX_TEST(31);
  END_TEST();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

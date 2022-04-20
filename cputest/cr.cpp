#include <gctypes.h>
#include <wiiuse/wpad.h>
#include "common/hwtests.h"

// The i parameter uses PowerPC bit ordering (MSB is bit 0)
static void SetBit(u32* n, size_t i)
{
  const u32 mask = (1 << (31 - i));
  *n |= mask;
}

// The i parameter uses PowerPC bit ordering (MSB is bit 0)
static void ClearBit(u32* n, size_t i)
{
  const u32 mask = (1 << (31 - i));
  *n &= ~mask;
}

static void DoNothing([[maybe_unused]] u32* expected)
{
}

static void SetLT(u32* expected)
{
  asm volatile("crset 0");
  SetBit(expected, 0);
}

static void ClearLT(u32* expected)
{
  asm volatile("crclr 0");
  ClearBit(expected, 0);
}

static void SetGT(u32* expected)
{
  asm volatile("crset 1");
  SetBit(expected, 1);
}

static void ClearGT(u32* expected)
{
  asm volatile("crclr 1");
  ClearBit(expected, 1);
}

static void SetEQ(u32* expected)
{
  asm volatile("crset 2");
  SetBit(expected, 2);
}

static void ClearEQ(u32* expected)
{
  asm volatile("crclr 2");
  ClearBit(expected, 2);
}

static void SetSO(u32* expected)
{
  asm volatile("crset 3");
  SetBit(expected, 3);
}

static void ClearSO(u32* expected)
{
  asm volatile("crclr 3");
  ClearBit(expected, 3);
}

// Condition register test
static void CRTest()
{
  START_TEST();

  s32 values[][2] = {
      // input     expected output
      {0,          0b0010},
      {10,         0b0100},
      {-10,        0b1000},
  };

  void (*cr_modifications[])(u32*) = {
      &DoNothing,
      &SetLT,
      &ClearLT,
      &SetGT,
      &ClearGT,
      &SetEQ,
      &ClearEQ,
      &SetSO,
      &ClearSO,
  };

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    for (size_t j = 0; j < 9; j++)
    {
      for (size_t so = 0; so < 2; so++)
      {
        u32 input = values[i][0];
        u32 expected = (values[i][1] | so) << 28;
        void (*cr_modification)(u32*) = cr_modifications[j];

        u32 xer = so << 31;
        u32 result = 0;
        asm volatile("mtxer %0" :: "r"(xer));
        asm volatile("cmpwi cr0, %0, 0" :: "r"(input));
        cr_modification(&expected);
        asm volatile("mfcr %0" : "=r"(result));

        // We only care about cr0
        expected >>= 28;
        result >>= 28;

        DO_TEST(result == expected, "cmpwi(0x{:08x}), j={}, so={}:\n"
                                    "     got {:#x}\n"
                                    "expected {:#x}",
                input, j, so, result, expected);
      }
    }
  }

  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  CRTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

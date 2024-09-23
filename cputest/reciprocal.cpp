// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <ppu_intrinsics.h>
#include <wiiuse/wpad.h>

#include "Common/FloatUtils.h"
#include "Common/hwtests.h"

static inline double fres_intrinsic(double val)
{
  double estimate;
  __asm__("fres %0,%1"
          /* outputs:  */
          : "=f"(estimate)
          /* inputs:   */
          : "f"(val));
  return estimate;
}

static void ReciprocalTest()
{
  START_TEST();

  for (unsigned long long i = 0; i < 0x100000000LL; i += 1)
  {
    union
    {
      long long testi;
      double testf;
    };
    union
    {
      double expectedf;
      long long expectedi;
    };
    testi = i << 32;
    expectedf = frsqrte_expected(testf);
    testf = __frsqrte(testf);
    DO_TEST(testi == expectedi, "Bad frsqrte {} {} {} {} {}", i, testf, testi,
            expectedf, expectedi);
    if (testi != expectedi)
      break;

    testi = i << 32;
    expectedf = fres_expected(testf);
    testf = fres_intrinsic(testf);
    DO_TEST(testi == expectedi, "Bad fres {} {} {} {} {}", i, testf, testi, expectedf,
            expectedi);
    if (testi != expectedi)
      break;

    if (!(i & ((1 << 22) - 1)))
    {
      network_printf("Progress %lld\n", i);
      WPAD_ScanPads();

      if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
        break;
    }
  }
  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  ReciprocalTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}

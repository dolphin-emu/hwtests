// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <initializer_list>
#include "hwtests.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <wiiuse/wpad.h>
#include "cgx.h"
#include "cgx_defaults.h"
#include "gxtest_util.h"
#include <ogcsys.h>

void BitfieldTest()
{
	START_TEST();

	TevReg reg;
	reg.hex = 0;
	reg.low = 0x345678;
	DO_TEST(reg.alpha == 837, "Values don't match (have: %d)", (s32)reg.alpha);
	DO_TEST(reg.red == -392, "Values don't match (have: %d)", (s32)reg.red);
	reg.low = 0x4BC6A8;
	DO_TEST(reg.alpha == -836, "Values don't match (have: %d)", (s32)reg.alpha);
	DO_TEST(reg.red == -344, "Values don't match (have: %d)", (s32)reg.red);
	reg.hex = 0;
	reg.alpha = -263;
	reg.red = -345;
	DO_TEST(reg.alpha == -263, "Values don't match (have: %d)", (s32)reg.alpha);
	DO_TEST(reg.red == -345, "Values don't match (have: %d)", (s32)reg.red);
	reg.alpha = 15;
	reg.red = -619;
	DO_TEST(reg.alpha == 15, "Values don't match (have: %d)", (s32)reg.alpha);
	DO_TEST(reg.red == -619, "Values don't match (have: %d)", (s32)reg.red);
	reg.alpha = 523;
	reg.red = 176;
	DO_TEST(reg.alpha == 523, "Values don't match (have: %d)", (s32)reg.alpha);
	DO_TEST(reg.red == 176, "Values don't match (have: %d)", (s32)reg.red);

	END_TEST();
}

int main()
{
	network_init();
	WPAD_Init();

	GXTest::Init();

	BitfieldTest();

	network_printf("Shutting down...\n");
	network_shutdown();

	return 0;
}

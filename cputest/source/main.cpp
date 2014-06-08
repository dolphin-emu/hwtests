// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <wiiuse/wpad.h>
#include <ogcsys.h>

#include "test.h"

void ReciprocalTest();

int main()
{
	network_init();
	WPAD_Init();

	ReciprocalTest();

	network_printf("Shutting down...\n");
	network_shutdown();

	return 0;
}

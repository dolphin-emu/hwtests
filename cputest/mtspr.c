#include <gctypes.h>
#include <wiiuse/wpad.h>
#include "test.h"

// Check what the PowerPC does if we set bits that we are not supposed to set.
// Some games do this (e.g. Dirt 2) and rely on correct emulation behavior.
static void UnusedBitsTest()
{
	START_TEST();
	uint32_t output;
	uint32_t expected = 0x80048004U;
	asm(
		"lis   %0, 32773;"
		"subi  %0, %0, 32764;"
		"mtspr 914, %0;"
		"mfspr %0, 914;"
		: "=r"(output)
	);

	DO_TEST(output == expected, "got %x, expected %x", output, expected);
	END_TEST();
}

int main()
{
	network_init();
	WPAD_Init();

	UnusedBitsTest();

	network_printf("Shutting down...\n");
	network_shutdown();

	return 0;
}

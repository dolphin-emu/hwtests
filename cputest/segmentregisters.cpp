#include <gctypes.h>
#include <wiiuse/wpad.h>
#include "hwtests.h"

u32 sr_values[][2] =
{
	{0xFFFFFFFF, 0xFFFFFFFF},
	{0x8FFFFFFF, 0x8FFFFFFF},
	{0xCFFFFFFF, 0xCFFFFFFF},
	{0xEFFFFFFF, 0xEFFFFFFF},
	{0xFFFFFFFF, 0xFFFFFFFF},
	{0x4FFFFFFF, 0x4FFFFFFF},
	{0x6FFFFFFF, 0x6FFFFFFF},
	{0x7FFFFFFF, 0x7FFFFFFF},
	{0x2FFFFFFF, 0x2FFFFFFF},
	{0x3FFFFFFF, 0x3FFFFFFF},
	// Bits 4-7 are reserved when bit 0 is 0
	// Bits are retained when written though.
	{0x0F0FFFFF, 0x0F0FFFFF},
};

static void mtsrTest()
{
	START_TEST();
#define MTSR_TEST(x) \
	do \
	{ \
		for (size_t i = 0; i < sizeof(sr_values) / sizeof(sr_values[0]); i++) \
		{ \
			u32 input = sr_values[i][0]; \
			u32 expected = sr_values[i][1]; \
			u32 result = 0; \
			asm("mtsr " #x ", %0" :: "r" (input)); \
			asm("mfsr %0, " #x : "=r" (result)); \
			\
			DO_TEST(result == expected, "mtsr/mfsr(0x%08x):\n" \
			                            "\tgot 0x%08x\n" \
			                            "\texpected 0x%08x", input, result, expected); \
		} \
	} while(0)
	MTSR_TEST(0);
	MTSR_TEST(1);
	MTSR_TEST(2);
	MTSR_TEST(3);
	MTSR_TEST(4);
	MTSR_TEST(5);
	MTSR_TEST(6);
	MTSR_TEST(7);
	MTSR_TEST(8);
	MTSR_TEST(9);
	MTSR_TEST(10);
	MTSR_TEST(11);
	MTSR_TEST(12);
	MTSR_TEST(13);
	MTSR_TEST(14);
	MTSR_TEST(15);

	END_TEST();
}

static void mtsrinTest()
{
	START_TEST();
#define MTSRIN_TEST(x) \
	do \
	{ \
		for (size_t i = 0; i < sizeof(sr_values) / sizeof(sr_values[0]); i++) \
		{ \
			u32 input = sr_values[i][0]; \
			u32 expected = sr_values[i][1]; \
			u32 result = 0; \
			u32 sr = x << 28; \
			asm("mtsrin %0, %1" :: "r" (input) , "r" (sr)); \
			asm("mfsr %0, " #x : "=r" (result)); \
			\
			DO_TEST(result == expected, "mtsrin(sr(%d)0x%08x):\n" \
			                            "\tgot 0x%08x\n" \
			                            "\texpected 0x%08x", x, input, result, expected); \
		} \
	} while(0)
	MTSRIN_TEST(0);
	MTSRIN_TEST(1);
	MTSRIN_TEST(2);
	MTSRIN_TEST(3);
	MTSRIN_TEST(4);
	MTSRIN_TEST(5);
	MTSRIN_TEST(6);
	MTSRIN_TEST(7);
	MTSRIN_TEST(8);
	MTSRIN_TEST(9);
	MTSRIN_TEST(10);
	MTSRIN_TEST(11);
	MTSRIN_TEST(12);
	MTSRIN_TEST(13);
	MTSRIN_TEST(14);
	MTSRIN_TEST(15);

	END_TEST();
}

static void mfsrinTest()
{
	START_TEST();
#define MFSRIN_TEST(x) \
	do \
	{ \
		for (size_t i = 0; i < sizeof(sr_values) / sizeof(sr_values[0]); i++) \
		{ \
			u32 input = sr_values[i][0]; \
			u32 expected = sr_values[i][1]; \
			u32 result = 0; \
			u32 sr = x << 28; \
			asm("mtsr " #x ", %0" :: "r" (input)); \
			asm("mfsrin %0, %1" : "=r" (result) : "r" (sr)); \
			\
			DO_TEST(result == expected, "mfsrin(sr(%d)0x%08x):\n" \
			                            "\tgot 0x%08x\n" \
			                            "\texpected 0x%08x", x, input, result, expected); \
		} \
	} while(0)
	MFSRIN_TEST(0);
	MFSRIN_TEST(1);
	MFSRIN_TEST(2);
	MFSRIN_TEST(3);
	MFSRIN_TEST(4);
	MFSRIN_TEST(5);
	MFSRIN_TEST(6);
	MFSRIN_TEST(7);
	MFSRIN_TEST(8);
	MFSRIN_TEST(9);
	MFSRIN_TEST(10);
	MFSRIN_TEST(11);
	MFSRIN_TEST(12);
	MFSRIN_TEST(13);
	MFSRIN_TEST(14);
	MFSRIN_TEST(15);

	END_TEST();
}
int main()
{
	network_init();

	mtsrTest();
	mtsrinTest();
	mfsrinTest();

	network_printf("Shutting down...\n");
	network_shutdown();

	return 0;
}

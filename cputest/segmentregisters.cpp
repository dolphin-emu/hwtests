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

template<u8 sr>
inline static void DoMtsrTest()
{
	for (size_t i = 0; i < sizeof(sr_values) / sizeof(sr_values[0]); i++)
	{
		u32 input = sr_values[i][0];
		u32 expected = sr_values[i][1];
		u32 result = 0;
		asm("mtsr %0, %1" :: "I" (sr), "r" (input));
		asm("mfsr %0, %1" : "=r" (result) : "I" (sr));

		DO_TEST(result == expected, "mtsr/mfsr(%u, 0x%08x):\n"
		                            "\tgot      0x%08x\n"
		                            "\texpected 0x%08x", sr, input, result, expected);
	}
}

static void MtsrTest()
{
	START_TEST();

	DoMtsrTest<0>();
	DoMtsrTest<1>();
	DoMtsrTest<2>();
	DoMtsrTest<3>();
	DoMtsrTest<4>();
	DoMtsrTest<5>();
	DoMtsrTest<6>();
	DoMtsrTest<7>();
	DoMtsrTest<8>();
	DoMtsrTest<9>();
	DoMtsrTest<10>();
	DoMtsrTest<11>();
	DoMtsrTest<12>();
	DoMtsrTest<13>();
	DoMtsrTest<14>();
	DoMtsrTest<15>();

	END_TEST();
}

template<u8 sr>
inline static void DoMtsrinTest()
{
	u32 rB = (u32)sr << (32 - 4);

	for (size_t i = 0; i < sizeof(sr_values) / sizeof(sr_values[0]); i++)
	{
		u32 input = sr_values[i][0];
		u32 expected = sr_values[i][1];
		u32 result = 0;
		asm("mtsrin %0, %1" :: "r" (input), "r" (rB));
		asm("mfsr %0, %1" : "=r" (result) : "I" (sr));

		DO_TEST(result == expected, "mtsrin(sr(%u), 0x%08x):\n"
		                            "\tgot      0x%08x\n"
		                            "\texpected 0x%08x", sr, input, result, expected);
	}
}

static void MtsrinTest()
{
	START_TEST();

	DoMtsrinTest<0>();
	DoMtsrinTest<1>();
	DoMtsrinTest<2>();
	DoMtsrinTest<3>();
	DoMtsrinTest<4>();
	DoMtsrinTest<5>();
	DoMtsrinTest<6>();
	DoMtsrinTest<7>();
	DoMtsrinTest<8>();
	DoMtsrinTest<9>();
	DoMtsrinTest<10>();
	DoMtsrinTest<11>();
	DoMtsrinTest<12>();
	DoMtsrinTest<13>();
	DoMtsrinTest<14>();
	DoMtsrinTest<15>();

	END_TEST();
}

template<u8 sr>
inline static void DoMfsrinTest()
{
	u32 rB = (u32)sr << (32 - 4);
	for (size_t i = 0; i < sizeof(sr_values) / sizeof(sr_values[0]); i++)
	{
		u32 input = sr_values[i][0];
		u32 expected = sr_values[i][1];
		u32 result = 0;
		asm("mtsr %0, %1" :: "I" (sr), "r" (input));
		asm("mfsrin %0, %1" : "=r" (result) : "r" (rB));

		DO_TEST(result == expected, "mfsrin(0x%08x, sr(%u)):\n"
		                            "\tgot 0x%08x\n"
		                            "\texpected 0x%08x", input, sr, result, expected);
	}
}

static void MfsrinTest()
{
	START_TEST();

	DoMfsrinTest<0>();
	DoMfsrinTest<1>();
	DoMfsrinTest<2>();
	DoMfsrinTest<3>();
	DoMfsrinTest<4>();
	DoMfsrinTest<5>();
	DoMfsrinTest<6>();
	DoMfsrinTest<7>();
	DoMfsrinTest<8>();
	DoMfsrinTest<9>();
	DoMfsrinTest<10>();
	DoMfsrinTest<11>();
	DoMfsrinTest<12>();
	DoMfsrinTest<13>();
	DoMfsrinTest<14>();
	DoMfsrinTest<15>();

	END_TEST();
}

static u32 s_sr_backup[16];

template<u8 sr>
inline static void DoBackup()
{
	asm("mfsr %0, %1" : "=r" (s_sr_backup[sr]) : "I" (sr));
}

static void Backup()
{
	DoBackup<0>();
	DoBackup<1>();
	DoBackup<2>();
	DoBackup<3>();
	DoBackup<4>();
	DoBackup<5>();
	DoBackup<6>();
	DoBackup<7>();
	DoBackup<8>();
	DoBackup<9>();
	DoBackup<10>();
	DoBackup<11>();
	DoBackup<12>();
	DoBackup<13>();
	DoBackup<14>();
	DoBackup<15>();
}

template<u8 sr>
inline static void DoRestore()
{
	asm("mtsr %0, %1" :: "I" (sr), "r" (s_sr_backup[sr]));
}

static void Restore()
{
	DoRestore<0>();
	DoRestore<1>();
	DoRestore<2>();
	DoRestore<3>();
	DoRestore<4>();
	DoRestore<5>();
	DoRestore<6>();
	DoRestore<7>();
	DoRestore<8>();
	DoRestore<9>();
	DoRestore<10>();
	DoRestore<11>();
	DoRestore<12>();
	DoRestore<13>();
	DoRestore<14>();
	DoRestore<15>();
}

int main()
{
	network_init();

	Backup();

	MtsrTest();
	MtsrinTest();
	MfsrinTest();

	Restore();

	network_printf("Shutting down...\n");
	network_shutdown();

	return 0;
}

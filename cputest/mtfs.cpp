#include <gctypes.h>
#include <wiiuse/wpad.h>
#include "hwtests.h"

// 6xx_pem says that bits 1 and 2(FEX and VX) aren't changed
// The reality is that mtfsi can set bits 1 and 2
// mtfsb0 can not clear bits 1 and 2
// mtfsb1 can not set bits 1 and 2
// but bit 20(reserved) is always zero
void FillFPSCR()
{
	asm("mtfsfi 0, 15");
	asm("mtfsfi 1, 15");
	asm("mtfsfi 2, 15");
	asm("mtfsfi 3, 15");
	asm("mtfsfi 4, 15");
	asm("mtfsfi 5, 15");
	asm("mtfsfi 6, 15");
	asm("mtfsfi 7, 15");
}

void ResetFPSCR()
{
	asm("mtfsfi 0, 0");
	asm("mtfsfi 1, 0");
	asm("mtfsfi 2, 0");
	asm("mtfsfi 3, 0");
	asm("mtfsfi 4, 0");
	asm("mtfsfi 5, 0");
	asm("mtfsfi 6, 0");
	asm("mtfsfi 7, 0");
}

u32 GetFPSCR()
{
	double result = 0;
	asm("mffs %0" : "=&f" (result));

	union
	{
		double d;
		u32 di[2];
	} t;
	t.d = result;
	return t.di[1];
}

void readFPSCRTest()
{
	START_TEST();

	FillFPSCR();
	u32 fpscr = GetFPSCR();
	DO_TEST(fpscr == 0xFFFFF7FF, "readFPSCRTest:\n"
	                             "\tgot: 0x%08x\n"
	                             "\texpected: 0xFFFFF7FF", fpscr);

	ResetFPSCR();
	fpscr = GetFPSCR();
	DO_TEST(fpscr ==  0x00000000, "readFPSCRTest:\n"
	                              "\tgot: 0x%08x\n"
	                              "\texpected: 0x00000000", fpscr);
	END_TEST();
}

void ClearTest()
{
	START_TEST();

	FillFPSCR();

	// Get FPSCR completely set
	// Every bit will be 1 except bit 20(reserved)
	u32 fpscrFull = GetFPSCR();

	// Try to clear bit 1, shouldn't be changed
	asm("mtfsb0 1");
	u32 fpscr = GetFPSCR();
	DO_TEST((fpscr & (0x80000000 >> 1)) == (fpscrFull & (0x80000000 >> 1)),
		"ClearTest(1):\n"
		"\tgo: 0x%08x\n"
		"\texpected to not change bit 1", fpscr);

	FillFPSCR();

	// Try to clear bit 2, shouldn't be changed
	asm("mtfsb0 2");
	fpscr = GetFPSCR();
	DO_TEST((fpscr & (0x80000000 >> 2)) == (fpscrFull & (0x80000000 >> 2)),
		"ClearTest(2):\n"
		"\tgo: 0x%08x\n"
		"\texpected to not change bit 2", fpscr);

	// Try to clear bit 20, should stay zero
	asm("mtfsb0 20");
	fpscr = GetFPSCR();
	DO_TEST((fpscr & (0x80000000 >> 20)) == 0,
		"ClearTest(20):\n"
		"\tgo: 0x%08x\n"
		"\texpected bit 20 to be 0", fpscr);

	END_TEST();
}

void SetTest()
{
	START_TEST();

	ResetFPSCR();

	// Get FPSCR completely cleared
	u32 fpscrFull = GetFPSCR();

	// Try to set bit 1, shouldn't be changed
	asm("mtfsb1 1");
	u32 fpscr = GetFPSCR();
	DO_TEST((fpscr & (0x80000000 >> 1)) == (fpscrFull & (0x80000000 >> 1)),
		"SetTest(1):\n"
		"\tgo: 0x%08x\n"
		"\texpected to not change bit 1", fpscr);

	ResetFPSCR();

	// Try to set bit 2, shouldn't be changed
	asm("mtfsb1 2");
	fpscr = GetFPSCR();
	DO_TEST((fpscr & (0x80000000 >> 2)) == (fpscrFull & (0x80000000 >> 2)),
		"SetTest(2):\n"
		"\tgo: 0x%08x\n"
		"\texpected to not change bit 2", fpscr);

	// Try to set bit 20, should stay zero
	asm("mtfsb1 20");
	fpscr = GetFPSCR();
	DO_TEST((fpscr & (0x80000000 >> 20)) == 0,
		"SetTest(20):\n"
		"\tgo: 0x%08x\n"
		"\texpected bit 20 to be 0", fpscr);

	END_TEST();
}

void mtfsb0Test()
{
	START_TEST();

#define MTFSB0_TEST(x) \
	do \
	{ \
		FillFPSCR(); \
		asm("mtfsb0 " #x); \
		u32 fpscr = GetFPSCR(); \
		DO_TEST((fpscr & (0x80000000 >> x)) == 0, "mtfsb0Test(%d):\n" \
		                                 "\tgot: 0x%08x\n" \
		                                 "\texpected to clear bit %d", x, fpscr, x); \
	} while(0)

	MTFSB0_TEST(0);
	// Bit 1 and 2 can't be explicitly unset with mtfsb0
	// MTFSB0_TEST(1);
	// MTFSB0_TEST(2);
	MTFSB0_TEST(3);
	MTFSB0_TEST(4);
	MTFSB0_TEST(5);
	MTFSB0_TEST(6);
	MTFSB0_TEST(7);
	MTFSB0_TEST(8);
	MTFSB0_TEST(9);
	MTFSB0_TEST(10);
	MTFSB0_TEST(11);
	MTFSB0_TEST(12);
	MTFSB0_TEST(13);
	MTFSB0_TEST(14);
	MTFSB0_TEST(15);
	MTFSB0_TEST(16);
	MTFSB0_TEST(17);
	MTFSB0_TEST(18);
	MTFSB0_TEST(19);
	// Bit 20 is reserved and always zero
	// MTFSB0_TEST(20);
	MTFSB0_TEST(21);
	MTFSB0_TEST(22);
	MTFSB0_TEST(23);
	MTFSB0_TEST(24);
	MTFSB0_TEST(25);
	MTFSB0_TEST(26);
	MTFSB0_TEST(27);
	MTFSB0_TEST(28);
	MTFSB0_TEST(29);
	MTFSB0_TEST(30);
	MTFSB0_TEST(31);

	END_TEST();
}

void mtfsb1Test()
{
	START_TEST();

#define MTFSB1_TEST(x) \
	do \
	{ \
		FillFPSCR(); \
		asm("mtfsb1 " #x); \
		u32 fpscr = GetFPSCR(); \
		DO_TEST((fpscr & (0x80000000 >> x)) == (0x80000000 >> x), "mtfsb1Test(%d):\n" \
		                                 "\tgot: 0x%08x\n" \
		                                 "\texpected to set bit %d", x, fpscr, x); \
	} while(0)

	MTFSB1_TEST(0);
	// Bit 1 and 2 can't be explicitly set with mtfsb1
	// MTFSB1_TEST(1);
	// MTFSB1_TEST(2);
	MTFSB1_TEST(3);
	MTFSB1_TEST(4);
	MTFSB1_TEST(5);
	MTFSB1_TEST(6);
	MTFSB1_TEST(7);
	MTFSB1_TEST(8);
	MTFSB1_TEST(9);
	MTFSB1_TEST(10);
	MTFSB1_TEST(11);
	MTFSB1_TEST(12);
	MTFSB1_TEST(13);
	MTFSB1_TEST(14);
	MTFSB1_TEST(15);
	MTFSB1_TEST(16);
	MTFSB1_TEST(17);
	MTFSB1_TEST(18);
	MTFSB1_TEST(19);
	// Bit 20 is reserved and always zero
	// MTFSB1_TEST(20);
	MTFSB1_TEST(21);
	MTFSB1_TEST(22);
	MTFSB1_TEST(23);
	MTFSB1_TEST(24);
	MTFSB1_TEST(25);
	MTFSB1_TEST(26);
	MTFSB1_TEST(27);
	MTFSB1_TEST(28);
	MTFSB1_TEST(29);
	MTFSB1_TEST(30);
	MTFSB1_TEST(31);

	END_TEST();
}

void FlipTheBitsTest()
{
	START_TEST();

#define SET_BIT(x) asm("mtfsb1 " #x)
#define FLIP_TEST(range) \
	DO_TEST(fpscr == expected, \
			"FlipTheBitsTest(" #range "):\n" \
			"\tgot: 0x%08x\n" \
			"\texpected: 0x%08x\n", fpscr, expected)

	u32 fpscr = 0;
	u32 expected = 0;
	ResetFPSCR();

	// Rounding Mode, bit 30-31
	// Other bits unaffected
	SET_BIT(31);
	SET_BIT(30);
	fpscr = GetFPSCR();
	expected = 0x00000003;
	FLIP_TEST("30 - 31");

	ResetFPSCR();

	// Non-IEEE mode, bit 29
	// Other bits unaffected
	SET_BIT(29);
	fpscr = GetFPSCR();
	expected = 0x00000004;
	FLIP_TEST("29");

	ResetFPSCR();

	// Inexact exception enable, bit 28
	// Other bits unaffected
	SET_BIT(28);
	fpscr = GetFPSCR();
	expected = 0x00000008;
	FLIP_TEST("28");

	ResetFPSCR();

	// IEEE division by zero exception enable, bit 27
	// Other bits unaffacted
	SET_BIT(27);
	fpscr = GetFPSCR();
	expected = 0x00000010;
	FLIP_TEST("27");

	ResetFPSCR();

	// IEEE underflow exception enable, bit 26
	// Other bits unaffected
	SET_BIT(26);
	fpscr = GetFPSCR();
	expected = 0x00000020;
	FLIP_TEST("26");

	ResetFPSCR();

	// IEEE overflow exception enable, bit 25
	// Other bits unaffected
	SET_BIT(25);
	fpscr = GetFPSCR();
	expected = 0x00000040;
	FLIP_TEST("25");

	ResetFPSCR();

	// Invalid operation exception enable, bit 24
	// Other bits unaffected
	SET_BIT(24);
	fpscr = GetFPSCR();
	expected = 0x00000080;
	FLIP_TEST("24");

	ResetFPSCR();

	// Invalid operation exception for integer conversion, bit 23
	// Affects VX(bit 2) & FX(bit 0)
	SET_BIT(23);
	fpscr = GetFPSCR();
	expected = 0xA0000100;
	FLIP_TEST("23");

	ResetFPSCR();

	// Invalid operation exception for square root, bit 22
	// Affects VX(bit 2) & FX(bit 0)
	SET_BIT(22);
	fpscr = GetFPSCR();
	expected = 0xA0000200;
	FLIP_TEST("22");

	ResetFPSCR();

	// Invalid operation exception for software request, bit 21
	// Affects VX(bit 2) & FX(bit 0)
	SET_BIT(21);
	fpscr = GetFPSCR();
	expected = 0xA0000400;
	FLIP_TEST("21");

	ResetFPSCR();

	// Bit 20 reserved

	// Floating point result flags, bits 15 - 19
	// Other bits unaffected
	SET_BIT(19);
	SET_BIT(18);
	SET_BIT(17);
	SET_BIT(16);
	SET_BIT(15);
	fpscr = GetFPSCR();
	expected = 0x0001F000;
	FLIP_TEST("15 - 19");

	ResetFPSCR();

	// Fraction inexact, bit 14
	// Other bits unaffected
	SET_BIT(14);
	fpscr = GetFPSCR();
	expected = 0x00020000;
	FLIP_TEST("14");

	ResetFPSCR();

	// Fraction rounded, bit 13
	// Other bits unaffected
	SET_BIT(13);
	fpscr = GetFPSCR();
	expected = 0x00040000;
	FLIP_TEST("13");

	ResetFPSCR();

	// Invalid operation exception for invalid comparison, bit 12
	// Affects VX(bit 2) & FX(bit 0)
	SET_BIT(12);
	fpscr = GetFPSCR();
	expected = 0xA0080000;
	FLIP_TEST("12");

	ResetFPSCR();

	// Invalid operation expcetion for inf * 0, bit 11
	// Affects VX(bit 2) & FX(bit 0)
	SET_BIT(11);
	fpscr = GetFPSCR();
	expected = 0xA0100000;
	FLIP_TEST("11");

	ResetFPSCR();

	// Invalid operation exception for 0 / 0, bit 10
	// Affects VX(bit 2) & FX(bit 0)
	SET_BIT(10);
	fpscr = GetFPSCR();
	expected = 0xA0200000;
	FLIP_TEST("10");

	ResetFPSCR();

	// Invalid operation exception for inf / inf, bit 9
	// Affects VX(bit 2) & FX(bit 0)
	SET_BIT(9);
	fpscr = GetFPSCR();
	expected = 0xA0400000;
	FLIP_TEST("9");

	ResetFPSCR();

	// Invalid operation exception for inf - inf, bit 8
	// Affects VX(bit 2) & FX(bit 0)
	SET_BIT(8);
	fpscr = GetFPSCR();
	expected = 0xA0800000;
	FLIP_TEST("8");

	ResetFPSCR();

	// Invalid operation exception for SNAN, bit 7
	// Affects VX(bit 2) & FX(bit 0)
	SET_BIT(7);
	fpscr = GetFPSCR();
	expected = 0xA1000000;
	FLIP_TEST("7");

	ResetFPSCR();

	// Inexact exception, bit 6
	// Other bits affected!
	SET_BIT(6);
	fpscr = GetFPSCR();
	expected = 0x82000000;
	FLIP_TEST("6");

	ResetFPSCR();

	// Division by zero exception, bit 5
	// Other bits affected!
	SET_BIT(5);
	fpscr = GetFPSCR();
	expected = 0x84000000;
	FLIP_TEST("5");

	ResetFPSCR();

	// Underflow exception, bit 4
	// Other bits affected!
	SET_BIT(4);
	fpscr = GetFPSCR();
	expected = 0x88000000;
	FLIP_TEST("4");

	ResetFPSCR();

	// Overflow exception, bit 3
	// Other bits affected!
	SET_BIT(3);
	fpscr = GetFPSCR();
	expected = 0x90000000;
	FLIP_TEST("3");

	ResetFPSCR();

	// Invalid Operation Exception summary, bit 2
	// Enabled Exception summary, bit 1
	// Can't be set with mtfsb1

	// Exception summary, bit 0
	// Affected by /other/ bits
	SET_BIT(0);
	expected = 0x80000000;
	fpscr = GetFPSCR();
	FLIP_TEST("0");

	ResetFPSCR();

	END_TEST();
}

int main()
{
	network_init();

	readFPSCRTest();
	mtfsb0Test();
	mtfsb1Test();
	ClearTest();
	SetTest();
	FlipTheBitsTest();

	network_printf("Shutting down...\n");
	network_shutdown();

	return 0;
}

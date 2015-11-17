#include <gctypes.h>
#include <wiiuse/wpad.h>
#include "hwtests.h"

#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

#define TEST_LOOP(f, tests) \
	do { \
		START_TEST(); \
		for (size_t i = 0; i < ARRAY_LENGTH(tests); i++) \
		{ \
			f(tests[i]); \
		} \
		END_TEST(); \
	} while(0)

#define TEST_ASM(asm_code) \
	asm \
	( \
		"li %0, 0;" \
		asm_code \
		"b unmet_%=;" \
		"met_%=:" \
		"li %0, 1;" \
		"unmet_%=:" \
		: "=&r"(met) \
		: "r"(a), "r"(b) \
		: "cc", "r0" \
	);

#define TEST_LT(inst, inst_code) \
	TEST_ASM(inst_code "; blt met_%=;") \
	DO_TEST(met == test.lt, inst " Fail: %08x %08x: LT == %d", a, b, met);

#define TEST_NLT(inst, inst_code) \
	TEST_ASM(inst_code "; bge met_%=;") \
	DO_TEST(met != test.lt, inst " Fail: %08x %08x: !LT == %d", a, b, met);

#define TEST_GT(inst, inst_code) \
	TEST_ASM(inst_code "; bgt met_%=;") \
	DO_TEST(met == test.gt, inst " Fail: %08x %08x: GT == %d", a, b, met);

#define TEST_NGT(inst, inst_code) \
	TEST_ASM(inst_code "; ble met_%=;") \
	DO_TEST(met != test.gt, inst " Fail: %08x %08x: !GT == %d", a, b, met);

#define TEST_EQ(inst, inst_code) \
	TEST_ASM(inst_code "; beq met_%=;") \
	DO_TEST(met == test.eq, inst " Fail: %08x %08x: EQ == %d", a, b, met);

#define TEST_NEQ(inst, inst_code) \
	TEST_ASM(inst_code "; bne met_%=;") \
	DO_TEST(met != test.eq, inst " Fail: %08x %08x: !EQ == %d", a, b, met);

// mcrxr cr0 twice to clear SO/OV bits in XER and CR0
#define TEST_SO(inst, inst_code) \
	TEST_ASM("mcrxr cr0; mcrxr cr0; " inst_code "; bc 12, 3, met_%=;") \
	DO_TEST(met == test.so, inst " Fail: %08x %08x: SO == %d", a, b, met);

#define TEST_NSO(inst, inst_code) \
	TEST_ASM("mcrxr cr0; mcrxr cr0; " inst_code "; bc 4, 3, met_%=;") \
	DO_TEST(met != test.so, inst " Fail: %08x %08x: !SO == %d", a, b, met);

#define TEST_FLAGS(inst, inst_code) \
	TEST_LT(inst, inst_code) \
	TEST_NLT(inst, inst_code) \
	TEST_GT(inst, inst_code) \
	TEST_NGT(inst, inst_code) \
	TEST_EQ(inst, inst_code) \
	TEST_NEQ(inst, inst_code)

// Additionally tests SO (Summary Overflow)
#define TEST_FLAGS_O(inst, inst_code) \
	TEST_FLAGS(inst, inst_code) \
	TEST_SO(inst, inst_code) \
	TEST_NSO(inst, inst_code)


// Duplicate everything with minor modifications, just for addic
#define TEST_ASM_ADDIC(asm_code) \
	asm \
	( \
		"li %0, 0;" \
		asm_code \
		"b unmet_%=;" \
		"met_%=:" \
		"li %0, 1;" \
		"unmet_%=:" \
		: "=&r"(met) \
		: "r"(a) \
		: "cc", "r0" \
	);

#define TEST_LT_ADDIC(inst, inst_code, input) \
	TEST_ASM_ADDIC(inst_code "; blt met_%=;") \
	DO_TEST(met == test->lt, inst " Fail: %08x %s: LT == %d", a, input, met);

#define TEST_NLT_ADDIC(inst, inst_code, input) \
	TEST_ASM_ADDIC(inst_code "; bge met_%=;") \
	DO_TEST(met != test->lt, inst " Fail: %08x %s: !LT == %d", a, input, met);

#define TEST_GT_ADDIC(inst, inst_code, input) \
	TEST_ASM_ADDIC(inst_code "; bgt met_%=;") \
	DO_TEST(met == test->gt, inst " Fail: %08x %s: GT == %d", a, input, met);

#define TEST_NGT_ADDIC(inst, inst_code, input) \
	TEST_ASM_ADDIC(inst_code "; ble met_%=;") \
	DO_TEST(met != test->gt, inst " Fail: %08x %s: !GT == %d", a, input, met);

#define TEST_EQ_ADDIC(inst, inst_code, input) \
	TEST_ASM_ADDIC(inst_code "; beq met_%=;") \
	DO_TEST(met == test->eq, inst " Fail: %08x %s: EQ == %d", a, input, met);

#define TEST_NEQ_ADDIC(inst, inst_code, input) \
	TEST_ASM_ADDIC(inst_code "; bne met_%=;") \
	DO_TEST(met != test->eq, inst " Fail: %08x %s: !EQ == %d", a, input, met);

// mcrxr cr0 twice to clear SO/OV bits in XER and CR0
#define TEST_SO_ADDIC(inst, inst_code, input) \
	TEST_ASM_ADDIC("mcrxr cr0; mcrxr cr0; " inst_code "; bc 12, 3, met_%=;") \
	DO_TEST(met == test->so, inst " Fail: %08x %s: SO == %d", a, input, met);

#define TEST_NSO_ADDIC(inst, inst_code, input) \
	TEST_ASM_ADDIC("mcrxr cr0; mcrxr cr0; " inst_code "; bc 4, 3, met_%=;") \
	DO_TEST(met != test->so, inst " Fail: %08x %s: !SO == %d", a, input, met);

#define TEST_FLAGS_ADDIC(inst, inst_code, input) \
	TEST_LT_ADDIC(inst, inst_code, input) \
	TEST_NLT_ADDIC(inst, inst_code, input) \
	TEST_GT_ADDIC(inst, inst_code, input) \
	TEST_NGT_ADDIC(inst, inst_code, input) \
	TEST_EQ_ADDIC(inst, inst_code, input) \
	TEST_NEQ_ADDIC(inst, inst_code, input) \
	TEST_SO_ADDIC(inst, inst_code, input) \
	TEST_NSO_ADDIC(inst, inst_code, input)

#define TEST_ADDIC(input) \
	a = test->a; \
	TEST_FLAGS_ADDIC("addic.", "addic. %%r0, %1, " input, input) \
	TEST_FLAGS_ADDIC("addic. rx, rx, imm", "addic. %1, %1, " input, input) \

typedef struct
{
	u32 a;
	u32 b;
	bool lt;
	bool gt;
	bool eq;
	bool so;
} flag_test;

flag_test subtraction_tests[] = {
	//                       expectations
	// input a   input b     LT     GT     EQ     SO
	{0x00000000, 0x00000000, false, false, true , false},

	{0x00000001, 0x00000001, false, false, true , false},
	{0x00000008, 0x00000008, false, false, true , false},
	{0x00000001, 0x00000008, false, true , false, false},
	{0x00000008, 0x00000001, true , false, false, false},

	{0x10000000, 0x10000000, false, false, true , false},
	{0x80000000, 0x80000000, false, false, true , false},
	{0x10000000, 0x80000000, false, true , false, true },
	{0x80000000, 0x10000000, true , false, false, true },
	{0x80000000, 0x00000000, true , false, false, true },
	{0x00000000, 0x80000000, true , false, false, false},
};

flag_test cmp_tests[] = {
	//                       expectations
	// input a   input b     LT     GT     EQ     SO
	{0x00000000, 0x00000000, false, false, true , false},

	{0x00000001, 0x00000001, false, false, true , false},
	{0x00000008, 0x00000008, false, false, true , false},
	{0x00000001, 0x00000008, true , false, false, false},
	{0x00000008, 0x00000001, false, true , false, false},

	{0x10000000, 0x10000000, false, false, true , false},
	{0x80000000, 0x80000000, false, false, true , false},
	{0x10000000, 0x80000000, false, true , false, false},
	{0x80000000, 0x10000000, true , false, false, false},
	{0x80000000, 0x00000000, true , false, false, false},
	{0x00000000, 0x80000000, false, true , false, false},
};

flag_test addition_tests[] = {
	//                       expectations
	// input a   input b     LT     GT     EQ     SO
	{0x00000000, 0x00000000, false, false, true , false},
	
	{0x00000001, 0x00000001, false, true , false, false},
	
	{0x10000000, 0x10000000, false, true , false, false},
	{0x80000000, 0x80000000, false, false, true , true },
	{0x10000000, 0x80000000, true , false, false, false},
	{0x80000000, 0x10000000, true , false, false, false},
	{0x70000000, 0x10000000, true , false, false, true },
	{0x10000000, 0x70000000, true , false, false, true },
	{0x80000000, 0x00000000, true , false, false, false},
	{0x00000000, 0x80000000, true , false, false, false},

	{0xF7000000, 0x10000000, false, true , false, false},
	{0x10000000, 0xF7000000, false, true , false, false},
};

// Input b (imm) is supplied via macro
flag_test addic_tests[] = {
	//                       expectations
	// input a   input b     LT     GT     EQ     SO
	{0x00000000, 0x00000000, false, false, true , false},
	
	{0xFFFFFFFF, 0x00000001, false, false, true , false},
	{0xFFFF0000, 0x00000001, true , false, false, false},
	
	{0x80000000, 0x0000FFFF, false, true , false, false},
	
	{0x00000001, 0x0000FFFF, false, false, true , false},
};

static void AddTest(const flag_test& test)
{
	u32 a = test.a;
	u32 b = test.b;
	u32 met = 0;

	TEST_FLAGS("add.", "add. %%r0, %1, %2")
	
	// Test for rD == rA
	TEST_FLAGS("add. rx, rx, ry", "add. %1, %1, %2")

	// Test with overflow
	TEST_FLAGS_O("addo.", "addo. %%r0, %1, %2")

	// Untested code path: immediates
}

static void AddcTest(const flag_test& test)
{
	u32 a = test.a;
	u32 b = test.b;
	u32 met = 0;

	TEST_FLAGS("addc.", "addc. %%r0, %1, %2")

	// Test for rD == rA
	TEST_FLAGS("addc. rx, rx, ry", "addc. %1, %1, %2")

	// Test with overflow
	TEST_FLAGS_O("addco.", "addco. %%r0, %1, %2")
}

// Does not test carry input!
static void AddeTest(const flag_test& test)
{
	u32 a = test.a;
	u32 b = test.b;
	u32 met = 0;

	TEST_FLAGS("adde.", "mcrxr cr0; adde. %%r0, %1, %2")

	// Test for rD == rA
	TEST_FLAGS("adde. rx, rx, ry", "mcrxr cr0; adde. %1, %1, %2")

	// Test with overflow
	TEST_FLAGS_O("addeo.", "mcrxr cr0; addeo. %%r0, %1, %2")
}

static void CmpTest(const flag_test& test)
{
	u32 a = test.a;
	u32 b = test.b;
	u32 met = 0;

	TEST_FLAGS_O("cmp", "cmp cr0, %1, %2")
}

static void SubfTest(const flag_test& test)
{
	u32 a = test.a;
	u32 b = test.b;
	u32 met = 0;

	TEST_FLAGS_O("subfo.", "subfo. %%r0, %1, %2")
}

static void AddicTest()
{
	START_TEST();

	// Set in macro
	u32 a = 0;
	u32 met = 0;

	flag_test* test = &addic_tests[0];
	TEST_ADDIC("0")

	test = &addic_tests[1];
	TEST_ADDIC("1")

	test = &addic_tests[2];
	TEST_ADDIC("1")

	test = &addic_tests[3];
	TEST_ADDIC("-1")

	test = &addic_tests[4];
	TEST_ADDIC("-1")

	END_TEST();
}

int main()
{
	network_init();
	WPAD_Init();

	TEST_LOOP(AddTest, addition_tests);
	TEST_LOOP(AddcTest, addition_tests);
	TEST_LOOP(AddeTest, addition_tests);
	AddicTest();
	// addme, addze left out, part of arithXex like adde
	// boolX left out
	TEST_LOOP(CmpTest, cmp_tests);
	// cntlzw, divw, divwu, mulXx left out
	TEST_LOOP(SubfTest, subtraction_tests);

	network_printf("Shutting down...\n");
	network_shutdown();

	return 0;
}

#include <gctypes.h>
#include "hwtests.h"
#include "BitField.h"

// quantize types
enum EQuantizeType
{
	QUANTIZE_FLOAT = 0,
	QUANTIZE_U8    = 4,
	QUANTIZE_U16   = 5,
	QUANTIZE_S8    = 6,
	QUANTIZE_S16   = 7,
};

union UGQR
{
	BitField< 0, 3, u32> st_type;
	BitField< 8, 6, u32> st_scale;
	BitField<16, 3, u32> ld_type;
	BitField<24, 6, u32> ld_scale;

	u32 hex;
};

// dequantize table
const float m_dequantizeTable[] =
{
	1.0 / (1ULL <<  0), 1.0 / (1ULL <<  1), 1.0 / (1ULL <<  2), 1.0 / (1ULL <<  3),
	1.0 / (1ULL <<  4), 1.0 / (1ULL <<  5), 1.0 / (1ULL <<  6), 1.0 / (1ULL <<  7),
	1.0 / (1ULL <<  8), 1.0 / (1ULL <<  9), 1.0 / (1ULL << 10), 1.0 / (1ULL << 11),
	1.0 / (1ULL << 12), 1.0 / (1ULL << 13), 1.0 / (1ULL << 14), 1.0 / (1ULL << 15),
	1.0 / (1ULL << 16), 1.0 / (1ULL << 17), 1.0 / (1ULL << 18), 1.0 / (1ULL << 19),
	1.0 / (1ULL << 20), 1.0 / (1ULL << 21), 1.0 / (1ULL << 22), 1.0 / (1ULL << 23),
	1.0 / (1ULL << 24), 1.0 / (1ULL << 25), 1.0 / (1ULL << 26), 1.0 / (1ULL << 27),
	1.0 / (1ULL << 28), 1.0 / (1ULL << 29), 1.0 / (1ULL << 30), 1.0 / (1ULL << 31),
	(1ULL << 32), (1ULL << 31), (1ULL << 30), (1ULL << 29),
	(1ULL << 28), (1ULL << 27), (1ULL << 26), (1ULL << 25),
	(1ULL << 24), (1ULL << 23), (1ULL << 22), (1ULL << 21),
	(1ULL << 20), (1ULL << 19), (1ULL << 18), (1ULL << 17),
	(1ULL << 16), (1ULL << 15), (1ULL << 14), (1ULL << 13),
	(1ULL << 12), (1ULL << 11), (1ULL << 10), (1ULL <<  9),
	(1ULL <<  8), (1ULL <<  7), (1ULL <<  6), (1ULL <<  5),
	(1ULL <<  4), (1ULL <<  3), (1ULL <<  2), (1ULL <<  1),
};

u32 GenerateGQR(u8 st_type, u8 st_scale, u8 ld_type, u8 ld_scale)
{
	UGQR gqr;
	gqr.st_type = st_type;
	gqr.st_scale = st_scale;
	gqr.ld_type = ld_type;
	gqr.ld_scale = ld_scale;
	return gqr.hex;
}

template<u8 gqr>
void SetGQR(u32 value)
{
	static_assert(gqr < 8, "GQR being set must be below 8!");
	asm("mtspr %[spr], %0" :: "r" (value), [spr] "i" (912 + gqr));
}

static void psqlSingleU8Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		u8 values[256];
		float expectation[256];

		for (u32 i = 0; i < 256; ++i)
			values[i] = i;

		for (u32 i = 0; i < 256; ++i)
			expectation[i] = m_dequantizeTable[scale] * (float)i;

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			float result = 0.0;

			SetGQR<0>(GenerateGQR(0, 0, QUANTIZE_U8, scale));
			asm("psq_l %[result], 0(%[loc]), 1, 0"
				: [result] "=f" (result)
				: [loc] "r" ((u32)&values[i]));
			DO_TEST(result == expectation[i], "psq_l-u8(%d)(%d):\n"
			                                  "\tgot %08x(%f)\n"
			                                  "\texpected %08x(%f)", values[i], scale, result, result, expectation[i], expectation[i]);
		}
	}

	END_TEST();
}

static void psqlSingleS8Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		s8 values[256];
		float expectation[256];

		for (int i = 0; i < 256; ++i)
			values[i] = i - 128;

		for (int i = 0; i < 256; ++i)
			expectation[i] = m_dequantizeTable[scale] * (float)values[i];

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			float result = 0.0;

			SetGQR<0>(GenerateGQR(0, 0, QUANTIZE_S8, scale));
			asm("psq_l %[result], 0(%[loc]), 1, 0"
				: [result] "=f" (result)
				: [loc] "r" ((u32)&values[i]));
			DO_TEST(result == expectation[i], "psq_l-s8(%d)(%d):\n"
			                                  "\tgot %08x(%f)\n"
			                                  "\texpected %08x(%f)", values[i], scale, result, result, expectation[i], expectation[i]);
		}
	}

	END_TEST();
}

static void psqlSingleU16Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		// Cover a reasonable 16bit range
		const u32 NUM_VALS = 0xFF;
		u16 values[NUM_VALS];
		float expectation[NUM_VALS];

		for (u32 i = 0; i < NUM_VALS; ++i)
			values[i] = i;

		for (u32 i = 0; i < NUM_VALS; ++i)
			expectation[i] = m_dequantizeTable[scale] * (float)values[i];

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			float result = 0.0;

			SetGQR<0>(GenerateGQR(0, 0, QUANTIZE_U16, scale));
			asm("psq_l %[result], 0(%[loc]), 1, 0"
				: [result] "=f" (result)
				: [loc] "r" ((u32)&values[i]));
			DO_TEST(result == expectation[i], "psq_l-u16(%d)(%d):\n"
			                                  "\tgot %08x(%f)\n"
			                                  "\texpected %08x(%f)",
			                                  values[i], scale,
			                                  result, result,
			                                  expectation[i], expectation[i]);
		}
	}

	END_TEST();
}

static void psqlSingleS16Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		// Cover a reasonable 16bit range
		const u32 NUM_VALS = 0xFF;
		s16 values[NUM_VALS];
		float expectation[NUM_VALS];

		for (int i = 0; i < NUM_VALS; ++i)
			values[i] = i - 128;

		for (u32 i = 0; i < NUM_VALS; ++i)
			expectation[i] = m_dequantizeTable[scale] * (float)values[i];

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			float result = 0.0;

			SetGQR<0>(GenerateGQR(0, 0, QUANTIZE_S16, scale));
			asm("psq_l %[result], 0(%[loc]), 1, 0"
				: [result] "=f" (result)
				: [loc] "r" ((u32)&values[i]));
			DO_TEST(result == expectation[i], "psq_l-u16(%d)(%d):\n"
			                                  "\tgot %08x(%f)\n"
			                                  "\texpected %08x(%f)",
			                                  values[i], scale,
			                                  result, result,
			                                  expectation[i], expectation[i]);
		}
	}

	END_TEST();
}

static void psqlSingleFloatTest()
{
	START_TEST();

	float values[64];
	for (int i = 0; i < 32; ++i)
		values[i] = 1 << i;

	for (int i = 0; i < 32; ++i)
		values[32 + i] = 1.0f / (1 << i);

	for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
	{
		float result = 0.0;

		// Loading float, quantizer doesn't use scale
		SetGQR<0>(GenerateGQR(0, 0, QUANTIZE_FLOAT, 0));
		asm("psq_l %[result], 0(%[loc]), 1, 0"
			: [result] "=f" (result)
			: [loc] "r" ((u32)&values[i]));
		DO_TEST(result == values[i], "psq_l-float(%f):\n"
		                             "\tgot %08x(%f)\n"
		                             "\texpected %08x(%f)", values[i], result, result, values[i], values[i]);
	}

	END_TEST();
}
static void psqlPairedU8Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		u8 values[256][2];
		float expectation[256][2];

		for (u32 i = 0; i < 256; ++i)
		{
			values[i][0] = i;
			values[i][1] = 255 - i;

			expectation[i][0] = m_dequantizeTable[scale] * (float)values[i][0];
			expectation[i][1] = m_dequantizeTable[scale] * (float)values[i][1];
		}

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			u32 values_offset = (u32)(&values[i] - &values[0]);

			float mid_float[2] = {0.0, 0.0};
			float result_float[2] = {0.0, 0.0};

			SetGQR<0>(GenerateGQR(QUANTIZE_FLOAT, 0, QUANTIZE_U8, scale));
			asm ("psq_l %[result], 0(%[loc]), 0, 0;\n"
			     "psq_st %[result], 0(%[new_loc]), 0, 0;\n"
				: [result] "=f" (mid_float)
				: [loc] "r" ((u32)&values[i][0]),
				  [new_loc] "r" ((u32)&result_float[0]));

			DO_TEST(result_float[0] == expectation[i][0], "psq_st-u8-pair0(0x%08x)(%d):\n"
			                                              "\tgot %f expected %f. Intermediate %f(0x%08x)",
			                                              values[i][0], scale, result_float[0], expectation[i][0], mid_float[0], mid_float[0]);
			DO_TEST(result_float[1] == expectation[i][1], "psq_st-u8-pair1(0x%08x)(%d):\n"
			                                              "\tgot %f expected %f. Intermediate %f(1x%08x)",
			                                              values[i][1], scale, result_float[1], expectation[i][1], mid_float[1], mid_float[1]);
		}
	}

	END_TEST();
}
static void psqlPairedS8Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		s8 values[256][2];
		float expectation[256][2];

		for (u32 i = 0; i < 256; ++i)
		{
			values[i][0] = i - 128;
			values[i][1] = 128 - i;

			expectation[i][0] = m_dequantizeTable[scale] * (float)values[i][0];
			expectation[i][1] = m_dequantizeTable[scale] * (float)values[i][1];
		}

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			u32 values_offset = (u32)(&values[i] - &values[0]);

			float mid_float[2] = {0.0, 0.0};
			float result_float[2] = {0.0, 0.0};

			SetGQR<0>(GenerateGQR(QUANTIZE_FLOAT, 0, QUANTIZE_S8, scale));
			asm ("psq_l %[result], 0(%[loc]), 0, 0;\n"
			     "psq_st %[result], 0(%[new_loc]), 0, 0;\n"
				: [result] "=f" (mid_float)
				: [loc] "r" ((u32)&values[i][0]),
				  [new_loc] "r" ((u32)&result_float[0]));

			DO_TEST(result_float[0] == expectation[i][0], "psq_st-u8-pair0(0x%08x)(%d):\n"
			                                              "\tgot %f expected %f. Intermediate %f(0x%08x)",
			                                              values[i][0], scale, result_float[0], expectation[i][0], mid_float[0], mid_float[0]);
			DO_TEST(result_float[1] == expectation[i][1], "psq_st-u8-pair1(0x%08x)(%d):\n"
			                                              "\tgot %f expected %f. Intermediate %f(1x%08x)",
			                                              values[i][1], scale, result_float[1], expectation[i][1], mid_float[1], mid_float[1]);
		}
	}

	END_TEST();
}

static void psqlPairedU16Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		// Cover a reasonable 16bit range
		const u32 NUM_VALS = 0xFF;
		u16 values[NUM_VALS][2];
		float expectation[NUM_VALS][2];

		for (u32 i = 0; i < NUM_VALS; ++i)
		{
			values[i][0] = 0xFFFF / i;
			values[i][1] = ~(0xFFFF / i);

			expectation[i][0] = m_dequantizeTable[scale] * (float)values[i][0];
			expectation[i][1] = m_dequantizeTable[scale] * (float)values[i][1];
		}

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			u32 values_offset = (u32)(&values[i] - &values[0]);

			float mid_float[2] = {0.0, 0.0};
			float result_float[2] = {0.0, 0.0};

			SetGQR<0>(GenerateGQR(QUANTIZE_FLOAT, 0, QUANTIZE_U16, scale));
			asm ("psq_l %[result], 0(%[loc]), 0, 0;\n"
			     "psq_st %[result], 0(%[new_loc]), 0, 0;\n"
				: [result] "=f" (mid_float)
				: [loc] "r" ((u32)&values[i][0]),
				  [new_loc] "r" ((u32)&result_float[0]));

			DO_TEST(result_float[0] == expectation[i][0], "psq_st-u8-pair0(0x%08x)(%d):\n"
			                                              "\tgot %f expected %f. Intermediate %f(0x%08x)",
			                                              values[i][0], scale, result_float[0], expectation[i][0], mid_float[0], mid_float[0]);
			DO_TEST(result_float[1] == expectation[i][1], "psq_st-u8-pair1(0x%08x)(%d):\n"
			                                              "\tgot %f expected %f. Intermediate %f(1x%08x)",
			                                              values[i][1], scale, result_float[1], expectation[i][1], mid_float[1], mid_float[1]);
		}
	}

	END_TEST();
}

static void psqlPairedS16Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		// Cover a reasonable 16bit range
		const u32 NUM_VALS = 0xFF;
		s16 values[NUM_VALS][2];
		float expectation[NUM_VALS][2];

		for (u32 i = 0; i < NUM_VALS; ++i)
		{
			values[i][0] = 0xFFFF / i;
			values[i][1] = ~(0xFFFF / i);

			expectation[i][0] = m_dequantizeTable[scale] * (float)values[i][0];
			expectation[i][1] = m_dequantizeTable[scale] * (float)values[i][1];
		}

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			u32 values_offset = (u32)(&values[i] - &values[0]);

			float mid_float[2] = {0.0, 0.0};
			float result_float[2] = {0.0, 0.0};

			SetGQR<0>(GenerateGQR(QUANTIZE_FLOAT, 0, QUANTIZE_S16, scale));
			asm ("psq_l %[result], 0(%[loc]), 0, 0;\n"
			     "psq_st %[result], 0(%[new_loc]), 0, 0;\n"
				: [result] "=f" (mid_float)
				: [loc] "r" ((u32)&values[i][0]),
				  [new_loc] "r" ((u32)&result_float[0]));

			DO_TEST(result_float[0] == expectation[i][0], "psq_st-u8-pair0(0x%08x)(%d):\n"
			                                              "\tgot %f expected %f. Intermediate %f(0x%08x)",
			                                              values[i][0], scale, result_float[0], expectation[i][0], mid_float[0], mid_float[0]);
			DO_TEST(result_float[1] == expectation[i][1], "psq_st-u8-pair1(0x%08x)(%d):\n"
			                                              "\tgot %f expected %f. Intermediate %f(1x%08x)",
			                                              values[i][1], scale, result_float[1], expectation[i][1], mid_float[1], mid_float[1]);
		}
	}

	END_TEST();
}

static void psqstSingleFloatTest()
{
	START_TEST();

	float values[64];
	for (int i = 0; i < 32; ++i)
		values[i] = 1 << i;

	for (int i = 0; i < 32; ++i)
		values[32 + i] = 1.0f / (1 << i);

	for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
	{
		float result = 0.0;
		float stored_result = 0.0;

		// Loading float, quantizer doesn't use scale
		SetGQR<0>(GenerateGQR(0, 0, QUANTIZE_FLOAT, 0));
		asm("psq_l %[result], 0(%[loc]), 1, 0;\n"
		    "psq_st %[result], 0(%[new_loc]), 1, 0;\n"
			: [result] "=f" (result)
			: [loc] "r" ((u32)&values[i]),
			  [new_loc] "r" ((u32)&stored_result));
		DO_TEST(stored_result == values[i], "psq_st-float(%f):\n"
		                                    "\tgot %08x(%f)\n"
		                                    "\texpected %08x(%f)", values[i], stored_result, stored_result, values[i], values[i]);
	}

	END_TEST();
}

static void psqstPairedFloatTest()
{
	START_TEST();

	float values[64][2];
	for (int i = 0; i < 32; ++i)
	{
		values[i][0] = 1 << i;
		values[i][1] = 2.0f * i;
	}

	for (int i = 0; i < 32; ++i)
	{
		values[32 + i][0] = 1.0f / (1 << i);
		values[32 + i][1] = 1.0f / i;
	}

	for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
	{
		float results[2] = {0.0, 0.0};
		float stored_results[2] = {0.0, 0.0};

		// Loading float, quantizer doesn't use scale
		SetGQR<0>(GenerateGQR(QUANTIZE_FLOAT, 0, QUANTIZE_FLOAT, 0));
		asm("psq_l %[result], 0(%[loc]), 0, 0;\n"
		    "psq_st %[result], 0(%[new_loc]), 0, 0;\n"
			: [result] "=f" (results)
			: [loc] "r" ((u32)&values[i][0]),
			  [new_loc] "r" ((u32)&stored_results[0]));

		DO_TEST(stored_results[0] == values[i][0], "psq_st-float-paired0(%f):\n"
		                                           "\tgot %08x(%f)\n"
		                                           "\texpected %08x(%f)", values[i][0], stored_results[0], stored_results[0], values[i][0], values[i][0]);
		DO_TEST(stored_results[1] == values[i][1], "psq_lst-float-paired1(%f):\n"
		                                           "\tgot %08x(%f)\n"
		                                           "\texpected %08x(%f)", values[i][1], stored_results[1], stored_results[1], values[i][1], values[i][1]);
	}

	END_TEST();
}

static void psqstSingleU8Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		u8 values[256];

		for (u32 i = 0; i < 256; ++i)
			values[i] = i;

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			float result_float = 0.0;
			u8 result_val = 0;

			SetGQR<0>(GenerateGQR(QUANTIZE_U8, scale, QUANTIZE_U8, scale));
			asm ("psq_l %[result], 0(%[loc]), 1, 0;\n"
			     "psq_st %[result], 0(%[new_loc]), 1, 0;\n"
				: [result] "=f" (result_float)
				: [loc] "r" ((u32)&values[i]),
				  [new_loc] "r" ((u32)&result_val));

			DO_TEST(result_val == values[i], "psq_st-u8(%d)(%d):\n"
			                                 "\tgot 0x%08x expected 0x%08x. Intermediate %f(0x%08x)",
			                                 values[i], scale, result_val, values[i], result_float, result_float);
		}
	}

	END_TEST();
}

static void psqstSingleS8Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		s8 values[256];

		for (int i = 0; i < 256; ++i)
			values[i] = i - 128;

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			u32 values_offset = (u32)(&values[i] - &values[0]);

			float result_float = 0.0;
			s8 result_val = 0;

			SetGQR<0>(GenerateGQR(QUANTIZE_S8, scale, QUANTIZE_S8, scale));
			asm ("psq_l %[result], 0(%[loc]), 1, 0;\n"
			     "psq_st %[result], 0(%[new_loc]), 1, 0;\n"
				: [result] "=f" (result_float)
				: [loc] "r" ((u32)&values[i]),
				  [new_loc] "r" ((u32)&result_val));

			DO_TEST(result_val == values[i], "psq_st-s8(%d)(%d):\n"
			                                 "\tgot 0x%08x expected 0x%08x. Intermediate %f(0x%08x)",
			                                 values[i], scale, result_val, values[i], result_float, result_float);
		}
	}

	END_TEST();
}

static void psqstPairedU8Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		s8 values[256][2];

		for (u32 i = 0; i < 256; ++i)
		{
			values[i][0] = i;
			values[i][1] = 255 - i;
		}

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			u32 values_offset = (u32)(&values[i] - &values[0]);

			float result_float[2] = {0.0, 0.0};
			s8 result_val[2] = {0, 0};

			SetGQR<0>(GenerateGQR(QUANTIZE_U8, scale, QUANTIZE_U8, scale));
			asm ("psq_l %[result], 0(%[loc]), 0, 0;\n"
			     "psq_st %[result], 0(%[new_loc]), 0, 0;\n"
				: [result] "=f" (result_float)
				: [loc] "r" ((u32)&values[i][0]),
				  [new_loc] "r" ((u32)&result_val[0]));

			DO_TEST(result_val[0] == values[i][0], "psq_st-u8-pair0(0x%08x)(%d):\n"
			                                       "\tgot 0x%08x expected 0x%08x. Intermediate %f(0x%08x)",
			                                       values[i][0], scale, result_val[0], values[i][0], result_float[0], result_float[0]);
			DO_TEST(result_val[1] == values[i][1], "psq_st-u8-pair1(0x%08x)(%d):\n"
			                                       "\tgot 0x%08x expected 0x%08x. Intermediate %f(0x%08x)\n",
			                                       values[i][1], scale, result_val[1], values[i][1], result_float[1], result_float[1]);
		}
	}

	END_TEST();
}

static void psqstPairedS8Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		s8 values[256][2];

		for (int i = 0; i < 256; ++i)
		{
			values[i][0] = i - 128;
			values[i][1] = 128 - i;
		}

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			u32 values_offset = (u32)(&values[i] - &values[0]);

			float result_float[2] = {0.0, 0.0};
			s8 result_val[2] = {0, 0};

			SetGQR<0>(GenerateGQR(QUANTIZE_S8, scale, QUANTIZE_S8, scale));
			asm ("psq_l %[result], 0(%[loc]), 0, 0;\n"
			     "psq_st %[result], 0(%[new_loc]), 0, 0;\n"
				: [result] "=f" (result_float)
				: [loc] "r" ((u32)&values[i][0]),
				  [new_loc] "r" ((u32)&result_val[0]));

			DO_TEST(result_val[0] == values[i][0], "psq_st-s8-pair0(0x%08x)(%d):\n"
			                                       "\tgot 0x%08x expected 0x%08x. Intermediate %f(0x%08x)",
			                                       values[i][0], scale, result_val[0], values[i][0], result_float[0], result_float[0]);
			DO_TEST(result_val[1] == values[i][1], "psq_st-s8-pair1(0x%08x)(%d):\n"
			                                       "\tgot 0x%08x expected 0x%08x. Intermediate %f(0x%08x)\n",
			                                       values[i][1], scale, result_val[1], values[i][1], result_float[1], result_float[1]);
		}
	}

	END_TEST();
}

static void psqstSingleU16Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		// Cover a reasonable 16bit range
		const u32 NUM_VALS = 0xFF;
		u16 values[NUM_VALS];

		for (u32 i = 0; i < NUM_VALS; ++i)
			values[i] = 0xFFFF / i;

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			u32 values_offset = (u32)(&values[i] - &values[0]);

			float result_float = 0.0;
			u16 result_val = 0;

			SetGQR<0>(GenerateGQR(QUANTIZE_U16, scale, QUANTIZE_U16, scale));
			asm ("psq_l %[result], 0(%[loc]), 1, 0;\n"
			     "psq_st %[result], 0(%[new_loc]), 1, 0;\n"
				: [result] "=f" (result_float)
				: [loc] "r" ((u32)&values[i]),
				  [new_loc] "r" ((u32)&result_val));

			DO_TEST(result_val == values[i], "psq_st-u16(0x%08x)(%d):\n"
			                                 "\tgot 0x%08x expected 0x%08x. Intermediate %f(0x%08x)",
			                                 values[i], scale, result_val, values[i], result_float, result_float);
		}
	}

	END_TEST();
}

static void psqstSingleS16Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		// Cover a reasonable 16bit range
		const u32 NUM_VALS = 0xFF;
		s16 values[NUM_VALS];

		for (u32 i = 0; i < NUM_VALS; ++i)
			values[i] = 0xFFFF / i;

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			u32 values_offset = (u32)(&values[i] - &values[0]);

			float result_float = 0.0;
			s16 result_val = 0;

			SetGQR<0>(GenerateGQR(QUANTIZE_S16, scale, QUANTIZE_S16, scale));
			asm ("psq_l %[result], 0(%[loc]), 1, 0;\n"
			     "psq_st %[result], 0(%[new_loc]), 1, 0;\n"
				: [result] "=f" (result_float)
				: [loc] "r" ((u32)&values[i]),
				  [new_loc] "r" ((u32)&result_val));

			DO_TEST(result_val == values[i], "psq_st-s16(0x%08x)(%d):\n"
			                                 "\tgot 0x%08x expected 0x%08x. Intermediate %f(0x%08x)",
			                                 values[i], scale, result_val, values[i], result_float, result_float);
		}
	}

	END_TEST();
}

static void psqstPairedS16Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		// Cover a reasonable 16bit range
		const u32 NUM_VALS = 0xFF;
		s16 values[NUM_VALS][2];

		for (u32 i = 0; i < NUM_VALS; ++i)
		{
			values[i][0] = 0xFFFF / i;
			values[i][1] = ~(0xFFFF / i);
		}

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			u32 values_offset = (u32)(&values[i] - &values[0]);

			float result_floats[2] = {0.0, 0.0};
			s16 result_vals[2] = {0, 0};

			SetGQR<0>(GenerateGQR(QUANTIZE_S16, scale, QUANTIZE_S16, scale));
			asm ("psq_l %[result], 0(%[loc]), 0, 0;\n"
			     "psq_st %[result], 0(%[new_loc]), 0, 0;\n"
				: [result] "=f" (result_floats[0])
				: [loc] "r" ((u32)&values[i][0]),
				  [new_loc] "r" ((u32)&result_vals[0]));

			DO_TEST(result_vals[0] == values[i][0], "psq_st-s16-paired0(0x%08x)(%d):\n"
			                                        "\tgot 0x%08x expected 0x%08x. Intermediate %f(0x%08x)",
			                                        values[i][0], scale, result_vals[0], values[i][0], result_floats[0], result_floats[0]);
			DO_TEST(result_vals[1] == values[i][1], "psq_st-s16-paired0(0x%08x)(%d):\n"
			                                        "\tgot 0x%08x expected 0x%08x. Intermediate %f(0x%08x)",
			                                        values[i][1], scale, result_vals[1], values[i][1], result_floats[1], result_floats[1]);
		}
	}

	END_TEST();
}

static void psqstPairedU16Test()
{
	START_TEST();
	for (u8 scale = 0; scale <= 0b111111; ++scale)
	{
		// Cover a reasonable 16bit range
		const u32 NUM_VALS = 0xFF;
		u16 values[NUM_VALS][2];

		for (u32 i = 0; i < NUM_VALS; ++i)
		{
			values[i][0] = 0xFFFF / i;
			values[i][1] = ~(0xFFFF / i);
		}

		for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		{
			u32 values_offset = (u32)(&values[i] - &values[0]);

			float result_floats[2] = {0.0, 0.0};
			u16 result_vals[2] = {0, 0};

			SetGQR<0>(GenerateGQR(QUANTIZE_U16, scale, QUANTIZE_U16, scale));
			asm ("psq_l %[result], 0(%[loc]), 0, 0;\n"
			     "psq_st %[result], 0(%[new_loc]), 0, 0;\n"
				: [result] "=f" (result_floats[0])
				: [loc] "r" ((u32)&values[i][0]),
				  [new_loc] "r" ((u32)&result_vals[0]));

			DO_TEST(result_vals[0] == values[i][0], "psq_st-u16-paired0(0x%08x)(%d):\n"
			                                        "\tgot 0x%08x expected 0x%08x. Intermediate %f(0x%08x)",
			                                        values[i][0], scale, result_vals[0], values[i][0], result_floats[0], result_floats[0]);
			DO_TEST(result_vals[1] == values[i][1], "psq_st-u16-paired0(0x%08x)(%d):\n"
			                                        "\tgot 0x%08x expected 0x%08x. Intermediate %f(0x%08x)",
			                                        values[i][1], scale, result_vals[1], values[i][1], result_floats[1], result_floats[1]);
		}
	}

	END_TEST();
}

int main()
{
	network_init();

	psqlSingleFloatTest();
	psqlSingleU8Test();
	psqlSingleS8Test();
	psqlSingleU16Test();
	psqlSingleS16Test();

	// These paired tests are a bit special in their implementation.
	// GCC can't fully implement them like the others because it doesn't know how to use psq_st.
	// These load tests dequantize the value, and then use psq_st without quantizing back
	// This is effectively a regular paired lfs
	// But if your code doesn't have psq_st fully working it may throw an error.
	psqlPairedU8Test();
	psqlPairedS8Test();
	psqlPairedU16Test();
	psqlPairedS16Test();

	psqstSingleFloatTest();
	psqstPairedFloatTest();

	psqstSingleU8Test();
	psqstSingleS8Test();
	psqstPairedU8Test();
	psqstPairedS8Test();

	psqstSingleU16Test();
	psqstSingleS16Test();
	psqstPairedU16Test();
	psqstPairedS16Test();

	network_printf("Shutting down...\n");
	network_shutdown();

	return 0;
}

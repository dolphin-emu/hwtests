// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#ifndef _CPMEMORY_H
#define _CPMEMORY_H

//#include "Common.h"
#include "BitField.h"

// Vertex array numbers
enum
{
	ARRAY_POSITION	= 0,
	ARRAY_NORMAL	= 1,
	ARRAY_COLOR		= 2,
	ARRAY_COLOR2	= 3,
	ARRAY_TEXCOORD0	= 4,
};

// Vertex components
enum
{
	NOT_PRESENT = 0,
	DIRECT		= 1,
	INDEX8		= 2,
	INDEX16		= 3,
};

enum
{
	FORMAT_UBYTE		= 0,	// 2 Cmp
	FORMAT_BYTE			= 1,	// 3 Cmp
	FORMAT_USHORT		= 2,
	FORMAT_SHORT		= 3,
	FORMAT_FLOAT		= 4,
};

enum
{
	FORMAT_16B_565		= 0,	// NA
	FORMAT_24B_888		= 1,	
	FORMAT_32B_888x		= 2,
	FORMAT_16B_4444		= 3,
	FORMAT_24B_6666		= 4,
	FORMAT_32B_8888		= 5,
};

enum
{
	VAT_0_FRACBITS = 0x3e0001f0,
	VAT_1_FRACBITS = 0x07c3e1f0,
	VAT_2_FRACBITS = 0xf87c3e1f,
};

enum
{
	VA_PTNMTXIDX  =  0,
	VA_TEX0MTXIDX =  1,
	VA_TEX1MTXIDX =  2,
	VA_TEX2MTXIDX =  3,
	VA_TEX3MTXIDX =  4,
	VA_TEX4MTXIDX =  5,
	VA_TEX5MTXIDX =  6,
	VA_TEX6MTXIDX =  7,
	VA_TEX7MTXIDX =  8,
	VA_POS        =  9,
	VA_NRM        = 10,
	VA_CLR0       = 11,
	VA_CLR1       = 12,
	VA_TEX0       = 13,
	VA_TEX1       = 14,
	VA_TEX2       = 15,
	VA_TEX3       = 16,
	VA_TEX4       = 17,
	VA_TEX5       = 18,
	VA_TEX6       = 19,
	VA_TEX7       = 20,
	VA_POSMTXARRAY   = 21,
	VA_NRMMTXARRAY   = 22,
	VA_TEXMTXARRAY   = 23,
	VA_LIGHTARRAY    = 24,
	VA_NBT        = 25,
	VA_MAXATTR    = 26,
	VA_NULL       = 0xff
};

// Vertex attribute component format
enum
{
	VA_FMT_U8  = 0,
	VA_FMT_S8  = 1,
	VA_FMT_U16 = 2,
	VA_FMT_S16 = 3,
	VA_FMT_F32 = 4
};

enum
{
	VA_FMT_RGB565 = 0,
	VA_FMT_RGB8   = 1,
	VA_FMT_RGBX8  = 2,
	VA_FMT_RGBA4  = 3,
	VA_FMT_RGBA6  = 4,
	VA_FMT_RGBA8  = 5
};

// Vertex attribute component types
enum
{
	VA_TYPE_POS_XY  = 0,
	VA_TYPE_POS_XYZ = 1
};

enum
{
	VA_TYPE_NRM_XYZ  = 0,
	VA_TYPE_NRM_NBT  = 1,
	VA_TYPE_NRM_NBT3 = 2
};

enum
{
	VA_TYPE_CLR_RGB  = 0,
	VA_TYPE_CLR_RGBA = 1,
};

enum
{
	VA_TYPE_TEX_S  = 0,
	VA_TYPE_TEX_ST = 1,
};

enum
{
	VTXATTR_NONE    = 0,
	VTXATTR_DIRECT  = 1,
	VTXATTR_INDEX8  = 2,
	VTXATTR_INDEX16 = 3
};

#pragma pack(4)
union TVtxDesc
{
	u64 Hex;

	struct
	{
		u32 Hex0, Hex1;
	};
	u8 byte[8];


	BitField<0,1> PosMatIdx;
	BitField<1,1> Tex0MatIdx;
	BitField<2,1> Tex1MatIdx;
	BitField<3,1> Tex2MatIdx;
	BitField<4,1> Tex3MatIdx;
	BitField<5,1> Tex4MatIdx;
	BitField<6,1> Tex5MatIdx;
	BitField<7,1> Tex6MatIdx;
	BitField<8,1> Tex7MatIdx;
	BitField<9,2> Position;
	BitField<11,2> Normal;
	BitField<13,2> Color0;
	BitField<15,2> Color1;
	BitField<17,2> Tex0Coord;
	BitField<19,2> Tex1Coord;
	BitField<21,2> Tex2Coord;
	BitField<23,2> Tex3Coord;
	BitField<25,2> Tex4Coord;
	BitField<27,2> Tex5Coord;
	BitField<29,2> Tex6Coord;
	BitField<31,2,u64> Tex7Coord;
	// 31 unused bits follow
};

union UVAT_group0
{
	BitField<0,1> PosElements;
	BitField<1,3> PosFormat;
	BitField<4,5> PosFrac;

	BitField<9,1> NormalElements;
	BitField<10,3> NormalFormat;

	BitField<13,1> Color0Elements;
	BitField<14,3> Color0Comp;

	BitField<17,1> Color1Elements;
	BitField<18,3> Color1Comp;

	BitField<21,1> Tex0CoordElements;
	BitField<22,3> Tex0CoordFormat;
	BitField<25,5> Tex0Frac;

	BitField<30,1> ByteDequant;
	BitField<31,1> NormalIndex3;

	u32 Hex;
};

union UVAT_group1
{
	BitField<0,1> Tex1CoordElements;
	BitField<1,3> Tex1CoordFormat;
	BitField<4,5> Tex1Frac;

	BitField<9,1> Tex2CoordElements;
	BitField<10,3> Tex2CoordFormat;
	BitField<13,5> Tex2Frac;

	BitField<18,1> Tex3CoordElements;
	BitField<19,3> Tex3CoordFormat;
	BitField<22,5> Tex3Frac;

	BitField<27,1> Tex4CoordElements;
	BitField<28,3> Tex4CoordFormat;
	// 1 bit unused

	u32 Hex;
};

union UVAT_group2
{
	BitField<0,5> Tex4Frac;

	BitField<5,1> Tex5CoordElements;
	BitField<6,3> Tex5CoordFormat;
	BitField<9,5> Tex5Frac;

	BitField<14,1> Tex6CoordElements;
	BitField<15,3> Tex6CoordFormat;
	BitField<18,5> Tex6Frac;

	BitField<23,1> Tex7CoordElements;
	BitField<24,3> Tex7CoordFormat;
	BitField<27,5> Tex7Frac;

	u32 Hex;
};

struct ColorAttr
{
	u8 Elements;
	u8 Comp;
};

struct TexAttr
{
	u8 Elements;
	u8 Format;
	u8 Frac;
};

struct TVtxAttr
{
	u8 PosElements;
	u8 PosFormat; 
	u8 PosFrac; 
	u8 NormalElements;
	u8 NormalFormat; 
	ColorAttr color[2];
	TexAttr texCoord[8];
	u8 ByteDequant;
	u8 NormalIndex3;
};

// Matrix indices
union TMatrixIndexA
{
	BitField<0,6> PosNormalMtxIdx;
	BitField<6,6> Tex0MtxIdx;
	BitField<12,6> Tex1MtxIdx;
	BitField<18,6> Tex2MtxIdx;
	BitField<24,6> Tex3MtxIdx;

	struct
	{
		u32 Hex : 30;
		u32 unused : 2;
	};
};

union TMatrixIndexB
{
	BitField<0,6> Tex4MtxIdx;
	BitField<6,6> Tex5MtxIdx;
	BitField<12,6> Tex6MtxIdx;
	BitField<18,6> Tex7MtxIdx;
	struct
	{
		u32 Hex : 24;
		u32 unused : 8;
	};
};

#pragma pack()

extern u32 arraybases[16];
extern u8 *cached_arraybases[16];
extern u32 arraystrides[16];
extern TMatrixIndexA MatrixIndexA;
extern TMatrixIndexB MatrixIndexB;

struct VAT
{
	UVAT_group0 g0;
	UVAT_group1 g1;
	UVAT_group2 g2;
};

extern TVtxDesc g_VtxDesc;
extern VAT g_VtxAttr[8];

// Might move this into its own file later.
//void LoadCPReg(u32 SubCmd, u32 Value);

// Fills memory with data from CP regs
//void FillCPMemoryArray(u32 *memory);

#endif // _CPMEMORY_H

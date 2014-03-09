// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <assert.h>
#include "CommonTypes.h"

#pragma once

// NOTE: Only works for sizeof(T)<=4 bytes
// NOTE: Only(?) works for unsigned fields?
template<u32 position, u32 bits, typename T=u32, typename ExposeType=T>
struct BitField
{
private:
	BitField(ExposeType val) = delete;
/*	{
		// This constructor just doesn't make any sense at all, so just assert out of it
		assert(0);
	}*/

	// Had to allow these... very bad feeling tbh!
//	BitField(const BitField& other) = delete;
/*	{
		// creates new storage => NOT what we want
		assert(0);
	}*/

//	BitField& operator = (const BitField& other) = delete;
/*	{
		// creates new storage => NOT what we want
		assert(0);
	}*/

public:
//	BitField() {}; // TODO: Should make sure having this constructor is safe!
	BitField() = default; // TODO: Should make sure having this constructor is safe!

	BitField& operator = (ExposeType val)
	{
		storage = (storage & ~GetMask()) | (((T)val<<position) & GetMask());
		return *this;
	}

	operator ExposeType() const { return (storage & GetMask()) >> position; }

	static ExposeType MaxVal()
	{
		// TODO
		return (1<<bits)-1;
	}

private:
	constexpr T GetMask()
	{
		return (bits == 8*sizeof(T)) ? (~(T)0) :
		       (((T)1 << bits)-(T)1) << position;
	}

	T storage;
};

template<u32 position, u32 bits, typename T=s64, typename ExposeType=T>
struct SignedBitField
{
private:
	SignedBitField(ExposeType val) = delete;

public:
	SignedBitField() = default;

	SignedBitField& operator = (ExposeType val)
	{
		storage = (storage & ~GetMask()) | ((val<<position) & GetMask());
		return *this;
	}

	operator ExposeType() const
	{
		u64 shift = 8 * sizeof(T) - bits;
		return (ExposeType)((storage & GetMask()) << (shift - position)) >> shift;
	}
private:
	constexpr u64 GetMask()
	{
		return (bits == 64) ? 0xFFFFFFFFFFFFFFFF :
		       ((1ull << bits)-1ull) << position;
	}

	T storage;
};
/*template<u32 position, u32 bits, typename T=s32>
struct SignedBitField
{
private:
	SignedBitField(s32 val) = delete;

public:
	SignedBitField() = default;

	SignedBitField& operator = (s32 val)
	{
		storage = (storage & ~GetMask()) | ((val<<position) & GetMask());
		return *this;
	}

	operator s32() const
	{
		u32 shift = 8 * sizeof(T) - bits;
		return (s32)((storage & GetMask()) << (shift - position)) >> shift;
	}

//private:
	constexpr u32 GetMask()
	{
		return ((bits == 32) ? 0xFFFFFFFF : ((1 << bits)-1)) << position;
	}

	T storage;
};
*/
/* Example:
 */
union SomeClass
{
	u32 hex;
	BitField<0,7> first_seven_bits;
	BitField<7,8> next_eight_bits;
	SignedBitField<3,15> some_signed_fields;
	// ...
};

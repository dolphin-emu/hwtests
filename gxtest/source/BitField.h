// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <assert.h>
#include "CommonTypes.h"

#pragma once

// NOTE: Only works for sizeof(T)<=4 bytes
// NOTE: Only(?) works for unsigned fields?
// abstract bitfield class
template<u32 position, u32 bits, typename T=u32>
struct BitField
{
private:
	// This constructor might be considered ambiguous:
	// Would it initialize the storage or just the bitfield?
	// Hence, delete it. Use the assignment operator to set bitfield values!
	BitField(u64 val) = delete;

public:
	// Force default constructor to be created
	// so that we can use this within unions
	BitField() = default;

	BitField& operator = (u64 val)
	{
		storage = (storage & ~GetMask()) | (((T)val<<position) & GetMask());
		return *this;
	}

	operator u64() const
	{
		return (storage & GetMask()) >> position;
	}

	static u64 MaxVal()
	{
		// Stupid border case, but let's implement it anyway
		if (bits == 64)
			return 0xFFFFFFFFFFFFFFFFull;

		return (1ull<<bits)-1ull;
	}

private:
	constexpr T GetMask()
	{
		return (bits == 8*sizeof(T)) ? (~(T)0) :
		       (((T)1 << bits)-(T)1) << position;
	}

	T storage;

	static_assert(bits+position <= 8*sizeof(T), "Bitfield out of range");
};

template<u32 position, u32 bits, typename T=s64>
struct SignedBitField
{
private:
	// This constructor might be considered ambiguous:
	// Would it initialize the storage or just the bitfield?
	// Hence, delete it. Use the assignment operator to set bitfield values!
	SignedBitField(u64 val) = delete;

public:
	// Force default constructor to be created
	// so that we can use this within unions
	SignedBitField() = default;

	SignedBitField& operator = (s64 val)
	{
		storage = (storage & ~GetMask()) | ((val<<position) & GetMask());
		return *this;
	}

	operator s64() const
	{
		if (bits == 64)
			return storage;

		u64 shift = 8 * sizeof(T) - bits;
		return (s64)((storage & GetMask()) << (shift - position)) >> shift;
	}
private:
	constexpr u64 GetMask()
	{
		return (bits == 64) ? 0xFFFFFFFFFFFFFFFFull :
		       ((1ull << bits)-1ull) << position;
	}

	T storage;

	static_assert(bits+position <= 8*sizeof(T), "Bitfield out of range");
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

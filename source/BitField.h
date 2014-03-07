// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <assert.h>
#include "CommonTypes.h"

#pragma once

// NOTE: Only works for sizeof(T)<=4 bytes
// NOTE: Only(?) works for unsigned fields?
template<u32 position, u32 bits, typename T=u32>
struct BitField
{
private:
	BitField(u32 val) = delete;
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

	BitField& operator = (u32 val)
	{
		storage = (storage & ~GetMask()) | ((val<<position) & GetMask());
		return *this;
	}

	operator u32() const { return (storage & GetMask()) >> position; }

	static T MaxVal()
	{
		// TODO
		return (1<<bits)-1;
	}

private:
	constexpr u32 GetMask()
	{
		return ((bits == 32) ? 0xFFFFFFFF : ((1 << bits)-1)) << position;
	}

	T storage;
};

/* Example:
 */
union SomeClass
{
	u32 hex;
	BitField<0,7> first_seven_bits;
	BitField<7,8> next_eight_bits;
	// ...
};

#pragma once

#include <gctypes.h>

#ifndef _rotl
static inline u32 _rotl(u32 x, int shift)
{
	shift &= 31;
	if (!shift) return x;
	return (x << shift) | (x >> (32 - shift));
}

static inline u32 _rotr(u32 x, int shift)
{
	shift &= 31;
	if (!shift) return x;
	return (x >> shift) | (x << (32 - shift));
}
#endif


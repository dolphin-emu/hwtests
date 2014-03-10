// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

namespace GXTest
{

// Four component vector with arbitrary base type
template<typename T>
union Vec4
{
	struct
	{
		T r, g, b, a;
	};
	struct
	{
		T x, y, z, w;
	};
};

// Initialize CGX and GXTest
void Init();

// Draw a white quad spanning the whole viewport
// Modifies the first vertex attribute and descriptor, as well as matrix state
void DrawFullScreenQuad();

// Perform an RGBA8 EFB copy to the internal testing buffer
void CopyToTestBuffer(int left_most_pixel, int top_most_pixel, int right_most_pixel, int bottom_most_pixel);

// Read back result from test buffer
// CopyToTestBuffer needs to be called before using this.
// After that, this function is free to use in terms of performance.
Vec4<u8> ReadTestBuffer(int x, int y, int previous_copy_width);

// Read back output of the last tev stage (all 11 bits)
// The BP register last_cc must have already been written before calling this function.
// The function logic adds 3 additional tev stages
// NOTE: This will only work correctly if the EFB format is set to RGB8
Vec4<int> GetTevOutput(const GenMode& genmode, const TevStageCombiner::ColorCombiner& last_cc);

} // namespace

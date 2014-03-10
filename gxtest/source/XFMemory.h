// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

union LitChannel
{
	BitField<0,1> matsource;
	BitField<1,1> enablelighting;
	BitField<2,4> lightMask0_3;
	BitField<6,1> ambsource;
	BitField<7,2> diffusefunc; // LIGHTDIF_X
	BitField<9,2> attnfunc; // LIGHTATTN_X
	BitField<11,4> lightMask4_7;
	// 17 bits unused

	u32 hex;
    unsigned int GetFullLightMask() const
    {
        return enablelighting ? (lightMask0_3 | (lightMask4_7 << 4)) : 0;
    }
};

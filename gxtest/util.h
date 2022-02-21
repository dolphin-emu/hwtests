// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include "cgx.h"

namespace GXTest
{
// Four component vector with arbitrary base type
template <typename T>
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

// Utility class to draw quads
class Quad
{
public:
  Quad();

  Quad& VertexTopLeft(f32 x, f32 y, f32 z);
  Quad& VertexTopRight(f32 x, f32 y, f32 z);
  Quad& VertexBottomRight(f32 x, f32 y, f32 z);
  Quad& VertexBottomLeft(f32 x, f32 y, f32 z);

  Quad& AtDepth(f32 depth);

  Quad& ColorRGBA(u8 r, u8 g, u8 b, u8 a);

  void Draw();

private:
  f32 x[4], y[4], z[4];

  bool has_color;
  u32 color;
};

// Initialize CGX and GXTest
void Init();

// Draw a white quad spanning the whole viewport
// Modifies the first vertex attribute and descriptor, as well as matrix state
void DrawFullScreenQuad();

// Perform an RGBA8 EFB copy to the internal testing buffer
void CopyToTestBuffer(int left_most_pixel, int top_most_pixel, int right_most_pixel,
                      int bottom_most_pixel, const EFBCopyParams& params = {});

// Read back result from test buffer
// CopyToTestBuffer needs to be called before using this.
// After that, this function is free to use in terms of performance.
Vec4<u8> ReadTestBuffer(int x, int y, int previous_copy_width);

// Read back output of the last tev stage (all 11 bits)
// The BP registers last_cc and last_ac must have already been written before
// calling this function. The function logic adds 3 additional tev stages,
// so care must be taken not to enable more than 13 tev stages before usage.
// NOTE: This will only work correctly if the EFB format is set to RGB8
Vec4<int> GetTevOutput(const GenMode& genmode, const TevStageCombiner::ColorCombiner& last_cc,
                       const TevStageCombiner::AlphaCombiner& last_ac);

void DebugDisplayEfbContents();

}  // namespace

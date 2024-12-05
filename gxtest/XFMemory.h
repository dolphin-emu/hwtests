// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// X.h defines None to be 0, which causes problems with some of the enums
#undef None

#include <array>

#include "Common/BitField.h"
#include "Common/CommonTypes.h"
#include "Common/EnumFormatter.h"

constexpr size_t NUM_XF_COLOR_CHANNELS = 2;

// Lighting

// Projection
enum class TexSize : u32
{
  ST = 0,
  STQ = 1
};
template <>
struct fmt::formatter<TexSize> : EnumFormatter<TexSize::STQ>
{
  constexpr formatter() : EnumFormatter({"ST (2x4 matrix)", "STQ (3x4 matrix)"}) {}
};

// Input form
enum class TexInputForm : u32
{
  AB11 = 0,
  ABC1 = 1
};
template <>
struct fmt::formatter<TexInputForm> : EnumFormatter<TexInputForm::ABC1>
{
  constexpr formatter() : EnumFormatter({"AB11", "ABC1"}) {}
};

enum class NormalCount : u32
{
  None = 0,
  Normals = 1,
  NormalsBinormals = 2
};
template <>
struct fmt::formatter<NormalCount> : EnumFormatter<NormalCount::NormalsBinormals>
{
  constexpr formatter() : EnumFormatter({"None", "Normals only", "Normals and binormals"}) {}
};

// Texture generation type
enum class TexGenType : u32
{
  Regular = 0,
  EmbossMap = 1,  // Used when bump mapping
  Color0 = 2,
  Color1 = 3
};
template <>
struct fmt::formatter<TexGenType> : EnumFormatter<TexGenType::Color1>
{
  static constexpr array_type names = {
      "Regular",
      "Emboss map (used when bump mapping)",
      "Color channel 0",
      "Color channel 1",
  };
  constexpr formatter() : EnumFormatter(names) {}
};

// Source row
enum class SourceRow : u32
{
  Geom = 0,    // Input is abc
  Normal = 1,  // Input is abc
  Colors = 2,
  BinormalT = 3,  // Input is abc
  BinormalB = 4,  // Input is abc
  Tex0 = 5,
  Tex1 = 6,
  Tex2 = 7,
  Tex3 = 8,
  Tex4 = 9,
  Tex5 = 10,
  Tex6 = 11,
  Tex7 = 12
};
template <>
struct fmt::formatter<SourceRow> : EnumFormatter<SourceRow::Tex7>
{
  static constexpr array_type names = {
      "Geometry (input is ABC1)",
      "Normal (input is ABC1)",
      "Colors",
      "Binormal T (input is ABC1)",
      "Binormal B (input is ABC1)",
      "Tex 0",
      "Tex 1",
      "Tex 2",
      "Tex 3",
      "Tex 4",
      "Tex 5",
      "Tex 6",
      "Tex 7",
  };
  constexpr formatter() : EnumFormatter(names) {}
};

enum class MatSource : u32
{
  MatColorRegister = 0,
  Vertex = 1,
};
template <>
struct fmt::formatter<MatSource> : EnumFormatter<MatSource::Vertex>
{
  constexpr formatter() : EnumFormatter({"Material color register", "Vertex color"}) {}
};

enum class AmbSource : u32
{
  AmbColorRegister = 0,
  Vertex = 1,
};
template <>
struct fmt::formatter<AmbSource> : EnumFormatter<AmbSource::Vertex>
{
  constexpr formatter() : EnumFormatter({"Ambient color register", "Vertex color"}) {}
};

// Light diffuse attenuation function
enum class DiffuseFunc : u32
{
  None = 0,
  Sign = 1,
  Clamp = 2
};
template <>
struct fmt::formatter<DiffuseFunc> : EnumFormatter<DiffuseFunc::Clamp>
{
  constexpr formatter() : EnumFormatter({"None", "Sign", "Clamp"}) {}
};

// Light attenuation function
enum class AttenuationFunc : u32
{
  None = 0,  // No attenuation
  Spec = 1,  // Point light attenuation
  Dir = 2,   // Directional light attenuation
  Spot = 3   // Spot light attenuation
};
template <>
struct fmt::formatter<AttenuationFunc> : EnumFormatter<AttenuationFunc::Spot>
{
  static constexpr array_type names = {
      "No attenuation",
      "Point light attenuation",
      "Directional light attenuation",
      "Spot light attenuation",
  };
  constexpr formatter() : EnumFormatter(names) {}
};

// Projection type
enum class ProjectionType : u32
{
  Perspective = 0,
  Orthographic = 1
};
template <>
struct fmt::formatter<ProjectionType> : EnumFormatter<ProjectionType::Orthographic>
{
  constexpr formatter() : EnumFormatter({"Perspective", "Orthographic"}) {}
};

// Registers and register ranges
enum
{
  XFMEM_POSMATRICES = 0x000,
  XFMEM_POSMATRICES_END = 0x100,
  XFMEM_NORMALMATRICES = 0x400,
  XFMEM_NORMALMATRICES_END = 0x460,
  XFMEM_POSTMATRICES = 0x500,
  XFMEM_POSTMATRICES_END = 0x600,
  XFMEM_LIGHTS = 0x600,
  XFMEM_LIGHTS_END = 0x680,
  XFMEM_REGISTERS_START = 0x1000,
  XFMEM_ERROR = 0x1000,
  XFMEM_DIAG = 0x1001,
  XFMEM_STATE0 = 0x1002,
  XFMEM_STATE1 = 0x1003,
  XFMEM_CLOCK = 0x1004,
  XFMEM_CLIPDISABLE = 0x1005,
  XFMEM_SETGPMETRIC = 0x1006,   // Perf0 according to YAGCD
  XFMEM_UNKNOWN_1007 = 0x1007,  // Perf1 according to YAGCD
  XFMEM_VTXSPECS = 0x1008,
  XFMEM_SETNUMCHAN = 0x1009,
  XFMEM_SETCHAN0_AMBCOLOR = 0x100a,
  XFMEM_SETCHAN1_AMBCOLOR = 0x100b,
  XFMEM_SETCHAN0_MATCOLOR = 0x100c,
  XFMEM_SETCHAN1_MATCOLOR = 0x100d,
  XFMEM_SETCHAN0_COLOR = 0x100e,
  XFMEM_SETCHAN1_COLOR = 0x100f,
  XFMEM_SETCHAN0_ALPHA = 0x1010,
  XFMEM_SETCHAN1_ALPHA = 0x1011,
  XFMEM_DUALTEX = 0x1012,
  XFMEM_UNKNOWN_GROUP_1_START = 0x1013,
  XFMEM_UNKNOWN_GROUP_1_END = 0x1017,
  XFMEM_SETMATRIXINDA = 0x1018,
  XFMEM_SETMATRIXINDB = 0x1019,
  XFMEM_SETVIEWPORT = 0x101a,
  XFMEM_SETZSCALE = 0x101c,
  XFMEM_SETZOFFSET = 0x101f,
  XFMEM_SETPROJECTION = 0x1020,
  // XFMEM_SETPROJECTIONB = 0x1021,
  // XFMEM_SETPROJECTIONC = 0x1022,
  // XFMEM_SETPROJECTIOND = 0x1023,
  // XFMEM_SETPROJECTIONE = 0x1024,
  // XFMEM_SETPROJECTIONF = 0x1025,
  // XFMEM_SETPROJECTIONORTHO = 0x1026,
  XFMEM_UNKNOWN_GROUP_2_START = 0x1027,
  XFMEM_UNKNOWN_GROUP_2_END = 0x103e,
  XFMEM_SETNUMTEXGENS = 0x103f,
  XFMEM_SETTEXMTXINFO = 0x1040,
  XFMEM_UNKNOWN_GROUP_3_START = 0x1048,
  XFMEM_UNKNOWN_GROUP_3_END = 0x104f,
  XFMEM_SETPOSTMTXINFO = 0x1050,
  XFMEM_REGISTERS_END = 0x1058,
};

union LitChannel
{
  BitField<0, 1, MatSource> matsource;
  BitField<1, 1, bool, u32> enablelighting;
  BitField<2, 4, u32> lightMask0_3;
  BitField<6, 1, AmbSource> ambsource;
  BitField<7, 2, DiffuseFunc> diffusefunc;
  BitField<9, 2, AttenuationFunc> attnfunc;
  BitField<11, 4, u32> lightMask4_7;
  u32 hex;

  unsigned int GetFullLightMask() const
  {
    return enablelighting ? (lightMask0_3 | (lightMask4_7 << 4)) : 0;
  }
};
template <>
struct fmt::formatter<LitChannel>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template <typename FormatContext>
  auto format(const LitChannel& chan, FormatContext& ctx) const
  {
    return fmt::format_to(
        ctx.out(),
        "Material source: {0}\nEnable lighting: {1}\nLight mask: {2:x} ({2:08b})\n"
        "Ambient source: {3}\nDiffuse function: {4}\nAttenuation function: {5}",
        chan.matsource, chan.enablelighting ? "Yes" : "No", chan.GetFullLightMask(), chan.ambsource,
        chan.diffusefunc, chan.attnfunc);
  }
};

union ClipDisable
{
  BitField<0, 1, bool, u32> disable_clipping_detection;
  BitField<1, 1, bool, u32> disable_trivial_rejection;
  BitField<2, 1, bool, u32> disable_cpoly_clipping_acceleration;
  u32 hex;
};
template <>
struct fmt::formatter<ClipDisable>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template <typename FormatContext>
  auto format(const ClipDisable& cd, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(),
                          "Disable clipping detection: {}\n"
                          "Disable trivial rejection: {}\n"
                          "Disable cpoly clipping acceleration: {}",
                          cd.disable_clipping_detection ? "Yes" : "No",
                          cd.disable_trivial_rejection ? "Yes" : "No",
                          cd.disable_cpoly_clipping_acceleration ? "Yes" : "No");
  }
};

union INVTXSPEC
{
  BitField<0, 2, u32> numcolors;
  BitField<2, 2, NormalCount> numnormals;
  BitField<4, 4, u32> numtextures;
  u32 hex;
};
template <>
struct fmt::formatter<INVTXSPEC>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template <typename FormatContext>
  auto format(const INVTXSPEC& spec, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "Num colors: {}\nNum normals: {}\nNum textures: {}",
                          spec.numcolors, spec.numnormals, spec.numtextures);
  }
};

union TexMtxInfo
{
  BitField<0, 1, u32> unknown;
  BitField<1, 1, TexSize> projection;
  BitField<2, 1, TexInputForm> inputform;
  BitField<3, 1, u32> unknown2;
  BitField<4, 3, TexGenType> texgentype;
  BitField<7, 5, SourceRow> sourcerow;
  BitField<12, 3, u32> embosssourceshift;  // what generated texcoord to use
  BitField<15, 3, u32> embosslightshift;   // light index that is used
  u32 hex;
};
template <>
struct fmt::formatter<TexMtxInfo>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template <typename FormatContext>
  auto format(const TexMtxInfo& i, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(),
                          "Projection: {}\nInput form: {}\nTex gen type: {}\n"
                          "Source row: {}\nEmboss source shift: {}\nEmboss light shift: {}",
                          i.projection, i.inputform, i.texgentype, i.sourcerow, i.embosssourceshift,
                          i.embosslightshift);
  }
};

union PostMtxInfo
{
  BitField<0, 6, u32> index;            // base row of dual transform matrix
  BitField<6, 2, u32> unused;           //
  BitField<8, 1, bool, u32> normalize;  // normalize before send operation
  u32 hex;
};

template <>
struct fmt::formatter<PostMtxInfo>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template <typename FormatContext>
  auto format(const PostMtxInfo& i, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "Index: {}\nNormalize before send operation: {}", i.index,
                          i.normalize ? "Yes" : "No");
  }
};

union NumColorChannel
{
  BitField<0, 2, u32> numColorChans;
  u32 hex;
};

union NumTexGen
{
  BitField<0, 4, u32> numTexGens;
  u32 hex;
};

union DualTexInfo
{
  BitField<0, 1, bool, u32> enabled;
  u32 hex;
};

struct Light
{
  u32 useless[3];
  u8 color[4];
  float cosatt[3];   // cos attenuation
  float distatt[3];  // dist attenuation

  union
  {
    struct
    {
      float dpos[3];
      float ddir[3];  // specular lights only
    };

    struct
    {
      float sdir[3];
      float shalfangle[3];  // specular lights only
    };
  };
};

struct Viewport
{
  float wd;
  float ht;
  float zRange;
  float xOrig;
  float yOrig;
  float farZ;
};

struct Projection
{
  using Raw = std::array<float, 6>;

  Raw rawProjection;
  ProjectionType type;
};

// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include "gxtest/BPMemory.h"
#include "gxtest/CPMemory.h"
#include "gxtest/XFMemory.h"

template <typename T>
static T CGXDefault();

template <typename T>
static T CGXDefault(int);

template <typename T>
static T CGXDefault(int, bool);

template <>
GenMode CGXDefault<GenMode>()
{
  GenMode genmode;
  genmode.hex = BPMEM_GENMODE << 24;
  genmode.numtexgens = 0;
  genmode.numcolchans = 1;
  genmode.numtevstages = 0;  // One stage
  genmode.cullmode = CullMode::None;
  genmode.numindstages = 0;
  genmode.zfreeze = false;
  return genmode;
}

template <>
ZMode CGXDefault<ZMode>()
{
  ZMode zmode;
  zmode.hex = BPMEM_ZMODE << 24;
  zmode.testenable = false;
  zmode.func = CompareMode::Always;
  zmode.updateenable = false;
  return zmode;
}

template <>
TevStageCombiner::ColorCombiner CGXDefault<TevStageCombiner::ColorCombiner>(int stage)
{
  TevStageCombiner::ColorCombiner cc;
  cc.hex = (BPMEM_TEV_COLOR_ENV + 2 * stage) << 24;
  cc.a = TevColorArg::Zero;
  cc.b = TevColorArg::Zero;
  cc.c = TevColorArg::Zero;
  cc.d = TevColorArg::Zero;
  cc.op = TevOp::Add;
  cc.bias = TevBias::Zero;
  cc.scale = TevScale::Scale1;
  cc.clamp = false;
  cc.dest = TevOutput::Prev;
  return cc;
}

template <>
TevStageCombiner::AlphaCombiner CGXDefault<TevStageCombiner::AlphaCombiner>(int stage)
{
  TevStageCombiner::AlphaCombiner ac;
  ac.hex = (BPMEM_TEV_ALPHA_ENV + 2 * stage) << 24;
  ac.a = TevAlphaArg::Zero;
  ac.b = TevAlphaArg::Zero;
  ac.c = TevAlphaArg::Zero;
  ac.d = TevAlphaArg::Zero;
  ac.op = TevOp::Add;
  ac.bias = TevBias::Zero;
  ac.scale = TevScale::Scale1;
  ac.clamp = false;
  ac.dest = TevOutput::Prev;
  return ac;
}

template <>
TwoTevStageOrders CGXDefault<TwoTevStageOrders>(int index)
{
  TwoTevStageOrders orders;
  orders.hex = (BPMEM_TREF + index) << 24;
  orders.texmap0 = GX_TEXMAP_NULL;
  orders.texcoord0 = GX_TEXCOORDNULL;
  orders.enable0 = false;
  orders.colorchan0 = RasColorChan::Color0;
  return orders;
}

template <>
TevReg CGXDefault<TevReg>(int index, bool is_konst_color)
{
  TevReg tevreg;
  tevreg.ra.hex = (BPMEM_TEV_COLOR_RA + 2 * index) << 24;
  tevreg.bg.hex = (BPMEM_TEV_COLOR_BG + 2 * index) << 24;
  tevreg.ra.type = is_konst_color ? TevRegType::Constant : TevRegType::Color;
  tevreg.bg.type = is_konst_color ? TevRegType::Constant : TevRegType::Color;
  return tevreg;
}

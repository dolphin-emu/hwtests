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
  genmode.cullmode = 0;      // No culling
  genmode.numindstages = 0;
  genmode.zfreeze = 0;
  return genmode;
}

template <>
ZMode CGXDefault<ZMode>()
{
  ZMode zmode;
  zmode.hex = BPMEM_ZMODE << 24;
  zmode.testenable = 0;
  zmode.func = COMPARE_ALWAYS;
  zmode.updateenable = 0;
  return zmode;
}

template <>
TevStageCombiner::ColorCombiner CGXDefault<TevStageCombiner::ColorCombiner>(int stage)
{
  TevStageCombiner::ColorCombiner cc;
  cc.hex = (BPMEM_TEV_COLOR_ENV + 2 * stage) << 24;
  cc.a = TEVCOLORARG_ZERO;
  cc.b = TEVCOLORARG_ZERO;
  cc.c = TEVCOLORARG_ZERO;
  cc.d = TEVCOLORARG_ZERO;
  cc.op = TEVOP_ADD;
  cc.bias = 0;
  cc.shift = TEVSCALE_1;
  cc.clamp = 0;
  cc.dest = GX_TEVPREV;
  return cc;
}

template <>
TevStageCombiner::AlphaCombiner CGXDefault<TevStageCombiner::AlphaCombiner>(int stage)
{
  TevStageCombiner::AlphaCombiner ac;
  ac.hex = (BPMEM_TEV_ALPHA_ENV + 2 * stage) << 24;
  ac.a = TEVALPHAARG_ZERO;
  ac.b = TEVALPHAARG_ZERO;
  ac.c = TEVALPHAARG_ZERO;
  ac.d = TEVALPHAARG_ZERO;
  ac.op = TEVOP_ADD;
  ac.bias = 0;
  ac.shift = TEVSCALE_1;
  ac.clamp = 0;
  ac.dest = GX_TEVPREV;
  return ac;
}

template <>
TwoTevStageOrders CGXDefault<TwoTevStageOrders>(int index)
{
  TwoTevStageOrders orders;
  orders.hex = (BPMEM_TREF + index) << 24;
  orders.texmap0 = GX_TEXMAP_NULL;
  orders.texcoord0 = GX_TEXCOORDNULL;
  orders.enable0 = 0;
  orders.colorchan0 = 0;  // equivalent to GX_COLOR0A0
  return orders;
}

template <>
TevReg CGXDefault<TevReg>(int index, bool is_konst_color)
{
  TevReg tevreg;
  tevreg.hex = 0;
  tevreg.low = (BPMEM_TEV_REGISTER_L + 2 * index) << 24;
  tevreg.high = (BPMEM_TEV_REGISTER_H + 2 * index) << 24;
  tevreg.type_ra = is_konst_color;
  tevreg.type_bg = is_konst_color;
  return tevreg;
}

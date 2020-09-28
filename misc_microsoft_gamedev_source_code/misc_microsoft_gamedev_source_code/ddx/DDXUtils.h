//==============================================================================
//
// File: DDXUtils.h
//
// Copyright (c) 2006 Ensemble Studios
//
//==============================================================================
#pragma once
#include "DDXDef.h"
#include "DDXPackParams.h"

class BDDXUtils
{
public:
   static uint calcMaxMips(uint width, uint height);
   static uint calcMaxMipChainLevels(uint width, uint height);
   static bool calcMipDimension(uint& outWidth, uint& outHeight, uint width, uint height, uint level);

   static bool getHeader(const uchar* pDDXData, uint DDXDataSize, BDDXHeader& outHeader);
   static bool setHeader(uchar* pDDXData, uint DDXDataSize, const BDDXHeader& header);

   static bool check(const uchar* pDDXData, uint DDXDataSize);
   static bool getDesc(const uchar* pDDXData, uint DDXDataSize, BDDXDesc& outDesc);
   
   static void endianSwapHeader(BDDXHeader& header, uint actualHeaderSize);
};

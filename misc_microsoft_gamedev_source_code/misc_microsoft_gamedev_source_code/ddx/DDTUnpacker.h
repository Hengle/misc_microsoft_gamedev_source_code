//============================================================================
//
// File: DDTUnpacker.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

class BDDTUnpacker
{
public:
   BDDTUnpacker();

   bool getDesc(const uchar* pDDTData, uint DDTDataSize, BDDXTextureInfo& textureInfo);
   bool unpack(const uchar* pDDTData, uint DDTDataSize, bool convertToABGR, BByteArray& textureData, BDDXTextureInfo& textureInfo);
};

//============================================================================
//
// File: DXTPacker.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "DXTUtils.h"

class BDXTPacker
{
public:
   BDXTPacker();
         
   bool pack(const BRGBAImage& image, BDXTFormat dxtFormat, eDXTQuality quality, bool perceptual, bool dithering, BByteArray& stream, bool favorLargerAlpha = false);

private:
   const BRGBAImage* mpImage;
   BByteArray* mpStream;
   uint mWidth, mHeight, mCellsX, mCellsY, mTotalCells, mBytesPerBlock, mColorBlockOfs;
   BDXTFormat mDXTFormat;
   BDynamicArray<BDXTUtils::BBlockPixel> mBlockColors;
   bool mPerceptual;
   uint mStreamOfs;
          
   uint createColorBlock(uint cx, uint cy, eDXTQuality quality);
   void createDXT3AlphaBlock(uint cx, uint cy);
   void createDXT5AlphaBlock(uint cx, uint cy, uint component, uint dstOfs, bool favorLarger);
};

//------------------------------------------------------------------------------
// File: writetga.h
// Poor man's TGA writer -- handles 24-bit truecolor, 8-bit greyscale
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "stream\stream.h"
//------------------------------------------------------------------------------
enum BTGAImageType
{
  cTGAImageTypeInvalid = 0,

  // memory format: B, G, R 
  cTGAImageTypeBGR,
  
  // memory format: B, G, R, A
  cTGAImageTypeBGRA,
  
  cTGAImageTypeGray,
};
//------------------------------------------------------------------------------
class BTGAWriter
{
  BStream* mpStream;
  uint mWidth, mHeight;
  uint mBytesPerPixel, mBytesPerLine;
  BTGAImageType mImageType;

public:
  BTGAWriter();
  ~BTGAWriter();

  bool open(BStream& stream, uint width, uint height, BTGAImageType mImageType);
  bool close(void);
  bool isOpened(void) const { return NULL != mpStream; }
  bool writeLine(const void* pScanLine);
  
  uint getWidth(void) const { return mWidth; }
  uint getHeight(void) const { return mHeight; }
  uint getBytesPerLine(void) const { return mBytesPerLine; }
  BTGAImageType getImageType(void) const { return mImageType; }
};
//------------------------------------------------------------------------------

// File: hdrUtils.h
// rg [6/9/06] - Created
#pragma once

#include "rgbaImage.h"
#include "utils\dataBuffer.h"

class BHDRCodec
{
public:
   static float packFloatToDXT5HPass1(const BRGBAFImage& srcImage, BRGBAImage& alphaImage, float hdrScale, float maxHDRScale);
   static float packFloatToDXT5HPass2(const BRGBAFImage& srcImage, const BRGBAImage& unpackedAlphaImage, BRGBAImage& compImage, float hdrScale, float maxHDRScale);
   static float packFloatToDXT5HImage(const BRGBAFImage& image, BByteArray& outStream, float hdrScale = -1.0f, float maxHDRScale = 64.0f);
   
   static void unpackDXT5HToFloatImage(const BRGBAImage& sourceImage, float hdrScale, BRGBAFImage& outImage);
   static void unpackDXT5HToFloatImage(const BConstDataBuffer& sourceData, uint width, uint height, float hdrScale, BRGBAFImage& outImage);
};


// File: hdrUtils.h
// rg [6/9/06] - Created
#pragma once

#include "rgbaImage.h"
#include "utils\dataBuffer.h"

class BHDRUtils
{
public:
   static float getLargestComponent(const BRGBAFImage& srcImage, uint numComps = 4);
   
   static void packFloatToHalfImage(const BRGBAFImage& srcImage, BRGBA16Image& dstImage, uint numComps = 4, bool littleEndian = true);
   static void unpackHalfFloatImage(const BRGBA16Image& srcImage, BRGBAFImage& dstImage, uint numComps = 4, bool littleEndian = true);
   
   static void convertRGB8ToFloatImage(const BRGBAImage& srcImage, BRGBAFImage& dstImage, float gamma = 2.2f);
   static void convertFloatToRGB8Image(const BRGBAFImage& srcImage, BRGBAImage& dstImage, float gamma = 2.2f, float exposure = 0.0f);
   static void convertRGB8ToHalfFloatImage(const BRGBAImage& srcImage, BRGBA16Image& dstImage, float gamma = 2.2f, bool littleEndian = true);
   static void convertHalfFloatToRGB8Image(const BRGBA16Image& srcImage, BRGBAImage& dstImage, float gamma = 2.2f, float exposure = 0.0f, bool littleEndian = true);
};
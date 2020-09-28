// File: ImageResample.h
#pragma once

class BImageResampler
{
public:
   BImageResampler(float sourceGamma = 1.7f);

   bool resample(const BRGBAImage& sourceImage, BRGBAImage& destImage, uint newWidth, uint newHeight, uint channelsToResample = 4, bool wrapAddressing = true, bool srgbFiltering = true, const char* pFilter = "kaiser", float filterScale = .75f);
   
   bool resample(const BRGBAFImage& sourceImage, BRGBAFImage& destImage, uint newWidth, uint newHeight, uint channelsToResample = 4, bool wrapAddressing = true, const char* pFilter = "kaiser", float filterScale = .75f, float lowValue = 0.0f, float highValue = 65535.0f);

private:
   static float gSRGBToLinearLightTable[256];

   // Must be at least 2^10!
   enum { LinearToSRGBTableSize = 2048 };
   static uchar gLinearToSRGBTable[LinearToSRGBTableSize];

   static void initLinearLightConversionTables(float sourceGamma);
   static float srgbToLinearLight(int x);
   static uint linearLightToSRGB(float x);
   static DWORD round(float x);
   static DWORD clamp(DWORD x, DWORD min, DWORD max);
   static DWORD floatToScaledByte(float x);
};



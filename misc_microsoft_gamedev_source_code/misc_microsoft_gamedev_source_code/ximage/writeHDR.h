// File: writeHDR.h
#pragma once
#include "colorUtils.h"
#include "stream\stream.h"

class BHDRWriter
{
public:
   BHDRWriter();
   ~BHDRWriter();

   // gamma is currently ignored unless the input is cInputRGB.
   bool open(BStream& stream, uint width, uint height, bool xFlipped = false, bool yFlipped = false, float gamma = 2.2f);
   bool close(void);

   enum cInputType
   {
      cInputRGB,     // BRGBAColor
      cInputRGBE,    // BRGBAColor in Radiance RGBE format
      cInputRGBF     // BRGBAColorF
   };
   
   bool writeLine(const void* pScanLine, cInputType inputType);
   
private:
   uint mWidth;
   uint mHeight;
   float mGamma;
   
   BStream* mpStream;
   
   BDynamicArray<BRGBAColor> mTempBuf;
   BDynamicArray<float> mGammaCurve;
   
   typedef uchar COLR[4];
   bool packScanline(const COLR* pScanLine, int len);
};
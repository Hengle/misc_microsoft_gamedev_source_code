//--------------------------------------------------------------------------------------------------------------------------------------------
// File: imageUtils.h
//--------------------------------------------------------------------------------------------------------------------------------------------
#pragma once
#include "RGBAImage.h"
#include "writeTGA.h"

class BPixelFormat;

class BImageUtils
{
public:
   static void ditherImage(BRGBAImage& ditheredImage, const BRGBAImage& image, int channel = -1);

   static bool readTGA(BStream& stream, BRGBAImage& image, BPixelFormat* pTGAFormat);
   static bool writeTGA(BStream& stream, const BRGBAImage& image, BTGAImageType type = cTGAImageTypeBGRA, int channel = -1);   
   
   static bool writeHDR(BStream& stream, const BRGBAFImage& image);
   static bool writeHDR(BStream& stream, const BRGBA16Image& image);
   static bool readHDR(BStream& stream, BRGBAFImage& image);
   
   static bool readTiff(BStream& stream, BRGBAFImage& image);
   
   struct BErrorMetrics
   {
      BErrorMetrics() 
      {
         clear();
      }
      
      void clear(void)
      {
         mMaxError = 0;
         mMeanError = 0.0f;
         mMSE = 0.0f;
         mRMSE = 0.0f;
         mPSNR = 0.0f;
      }         
      
      uint mMaxError;
      float mMeanError;
      float mMSE;
      float mRMSE;
      float mPSNR;
   };
   
   static bool computeErrorMetrics(BErrorMetrics& errorMetrics, const BRGBAImage& a, const BRGBAImage& b, uint firstChannel, uint numChannels, bool lumaOnly);
   
   static void swizzleAR(BRGBAImage& dst, const BRGBAImage& src);

   static void renormalizeImage(BRGBAImage& dst, const BRGBAImage& src, bool fixZeroVectors = false);
};   

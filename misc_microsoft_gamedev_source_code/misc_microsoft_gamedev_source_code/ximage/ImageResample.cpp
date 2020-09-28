// File: ImageResample.cpp
#pragma once
#include "ximage.h"
#include "colorutils.h"
#include "containers\dynamicarray.h"
#include "RGBAImage.h"
#include "ImageResample.h"
#include "resampler.h"

float BImageResampler::gSRGBToLinearLightTable[256];

// Must be at least 2^10!
uchar BImageResampler::gLinearToSRGBTable[BImageResampler::LinearToSRGBTableSize];

BImageResampler::BImageResampler(float sourceGamma)
{
   initLinearLightConversionTables(sourceGamma);
}

bool BImageResampler::resample(
   const BRGBAImage& sourceImage, 
   BRGBAImage& destImage, 
   uint newWidth, uint newHeight, 
   uint channelsToResample, 
   bool wrapAddressing,
   bool srgbFiltering,
   const char* pFilter,
   float filterScale)
{
   const uint srcX = sourceImage.getWidth();
   const uint srcY = sourceImage.getHeight();

   if ((newWidth < 1) || (newHeight < 1) || (channelsToResample < 1) || (channelsToResample > 4))
      return false;
   
   destImage.setSize(newWidth, newHeight);

   const char* filterName   = pFilter;
   const float filterXScale = filterScale;//0.75f;
   const float filterYScale = filterXScale;
   
   Resampler* pResamplers[4] = { NULL, NULL, NULL, NULL };
   BDynamicArray<Resample_Real> channels[4];

   for (uint i = 0; i < channelsToResample; ++i)
   {
      pResamplers[i] = new Resampler(srcX, srcY, newWidth, newHeight, wrapAddressing ? Resampler::BOUNDARY_WRAP : Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, filterName, NULL, NULL, filterXScale, filterYScale);
      channels[i].resize(srcX);
   }

   uint curDestY = 0;
   for (uint curY = 0; curY < srcY; ++curY)
   {
      const BRGBAColor* pSrc = &sourceImage(0, curY);
      
      if (srgbFiltering)
      {
         for (uint curX = 0; curX < srcX; ++curX, ++pSrc)
            for (uint i = 0; i < channelsToResample; ++i)
               channels[i][curX] = srgbToLinearLight((*pSrc)[i]);
      }
      else
      {
         for (uint curX = 0; curX < srcX; ++curX, ++pSrc)
            for (uint i = 0; i < channelsToResample; ++i)
               channels[i][curX] = (*pSrc)[i] * (1.0f / 255.0f);
      }               

      for (uint i = 0; i < channelsToResample; ++i)
         pResamplers[i]->put_line(&channels[i][0]); 

      for ( ; ; )
      {
         const Resampler::Sample* pResampledData[4];

         uint i;
         for (i = 0; i < channelsToResample; ++i)
         {
            pResampledData[i] = pResamplers[i]->get_line();
            if (!pResampledData[i])
               break;
         }
         if (i < channelsToResample)
            break;

         BRGBAColor* pDst = &destImage(0, curDestY);
         if (srgbFiltering)
         {
            for (uint curDestX = 0; curDestX < newWidth; ++curDestX, ++pDst)
            {
               for (uint i = 0; i < channelsToResample; ++i)
                  (*pDst)[i] = static_cast<uchar>(linearLightToSRGB((float)pResampledData[i][curDestX]));
               
               if (channelsToResample < 4)
                  (*pDst)[3] = 255;                  
            }                  
         }
         else
         {
            for (uint curDestX = 0; curDestX < newWidth; ++curDestX, ++pDst)
            {
               for (uint i = 0; i < channelsToResample; ++i)
               {
                  int c = Math::FloatToIntRound((float)pResampledData[i][curDestX] * 255.0f);
                  if (c & 0xFFFFFF00)
                  {
                     if (c < 0) 
                        c = 0; 
                     else if (c > 255) 
                        c = 255;
                  }
                  
                  (*pDst)[i] = static_cast<uchar>(c);
               }
               
               if (channelsToResample < 4)
                  (*pDst)[3] = 255;
            }               
         }
         
         //-- increment our destination row
         curDestY++;
      }
   } // curY

   for (uint i = 0; i < channelsToResample; ++i)
      delete pResamplers[i];

   return true;      
}

//----------------------------------------------------------------------------
// BImageResampler::resample
//----------------------------------------------------------------------------
bool BImageResampler::resample(
   const BRGBAFImage& sourceImage, 
   BRGBAFImage& destImage, 
   uint newWidth, uint newHeight, 
   uint channelsToResample, 
   bool wrapAddressing,
   const char* pFilter,
   float filterScale,
   float lowValue, 
   float highValue)
{
   const uint srcX = sourceImage.getWidth();
   const uint srcY = sourceImage.getHeight();

   if ((newWidth < 1) || (newHeight < 1) || (channelsToResample < 1) || (channelsToResample > 4))
      return false;
   
   destImage.setSize(newWidth, newHeight);

   const char* filterName   = pFilter;
   const float filterXScale = filterScale;//0.75f;
   const float filterYScale = filterXScale;
   
   Resampler* pResamplers[4] = { NULL, NULL, NULL, NULL };
   BDynamicArray<Resample_Real> channels[4];

   for (uint i = 0; i < channelsToResample; ++i)
   {
      pResamplers[i] = new Resampler(srcX, srcY, newWidth, newHeight, wrapAddressing ? Resampler::BOUNDARY_WRAP : Resampler::BOUNDARY_CLAMP, lowValue, highValue, filterName, NULL, NULL, filterXScale, filterYScale);
      channels[i].resize(srcX);
   }

   uint curDestY = 0;
   for (uint curY = 0; curY < srcY; ++curY)
   {
      const BRGBAColorF* pSrc = &sourceImage(0, curY);
      
      for (uint curX = 0; curX < srcX; ++curX, ++pSrc)
         for (uint i = 0; i < channelsToResample; ++i)
            channels[i][curX] = (*pSrc)[i];
      
      for (uint i = 0; i < channelsToResample; ++i)
         pResamplers[i]->put_line(&channels[i][0]); 

      for ( ; ; )
      {
         const Resampler::Sample* pResampledData[4];

         uint i;
         for (i = 0; i < channelsToResample; ++i)
         {
            pResampledData[i] = pResamplers[i]->get_line();
            if (!pResampledData[i])
               break;
         }
         if (i < channelsToResample)
            break;

         BRGBAColorF* pDst = &destImage(0, curDestY);
         
         for (uint curDestX = 0; curDestX < newWidth; ++curDestX, ++pDst)
         {
            for (uint i = 0; i < channelsToResample; ++i)
               (*pDst)[i] = static_cast<float>(pResampledData[i][curDestX]);
               
            if (channelsToResample < 4)
               (*pDst)[3] = 1.0f;               
         }
         
         //-- increment our destination row
         curDestY++;
      }
   } // curY

   for (uint i = 0; i < channelsToResample; ++i)
      delete pResamplers[i];

   return true;      
}

//----------------------------------------------------------------------------
// BImageResampler::initLinearLightConversionTables
//----------------------------------------------------------------------------
void BImageResampler::initLinearLightConversionTables(float sourceGamma)
{
   if (gSRGBToLinearLightTable[1])
      return;

   for (int i = 0; i < 256; ++i)
      gSRGBToLinearLightTable[i] = (float) pow(i * cOneOver255, sourceGamma);

   const float oneOverLinearToSRGBTableSize = 1.0f / LinearToSRGBTableSize;
   const float oneOverSourceGamma = 1.0f / sourceGamma;

   for (int i = 0; i < LinearToSRGBTableSize; ++i)
      gLinearToSRGBTable[i] = (uchar)floatToScaledByte((float) pow(i * oneOverLinearToSRGBTableSize, oneOverSourceGamma));
}

//----------------------------------------------------------------------------
// Converts sRGB samples to linear light.
// Must call initLinearLightConversionTables() first!
//----------------------------------------------------------------------------
float BImageResampler::srgbToLinearLight(int x)
{

   BASSERT(x>=0);
   BASSERT(x<=255);

   const float linearLight = gSRGBToLinearLightTable[x];
   return linearLight;
}

//----------------------------------------------------------------------------
// Converts linear light samples to sRGB.
// Must call initLinearLightConversionTables() first!
//----------------------------------------------------------------------------
uint BImageResampler::linearLightToSRGB(float x)
{
   int y = Math::FloatToIntRound(x * LinearToSRGBTableSize);
   if (y < 0)
      y = 0;
   else if (y > (LinearToSRGBTableSize - 1))
      y = (LinearToSRGBTableSize - 1);

   const uint sRGBLight = gLinearToSRGBTable[y];
   return sRGBLight;
}

DWORD BImageResampler::round(float x)
{
   return (DWORD)floor(x + 0.5f);
}

DWORD BImageResampler::clamp(DWORD x, DWORD min, DWORD max)
{
   if (x < min)
      x = min;
   else if (x > max)
      x = max;

   return x;
}

DWORD BImageResampler::floatToScaledByte(float x)
{      
   return clamp(round(x * 255.0f), 0, 255);
}

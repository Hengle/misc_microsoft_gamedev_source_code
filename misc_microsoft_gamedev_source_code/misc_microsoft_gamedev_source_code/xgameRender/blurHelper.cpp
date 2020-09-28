//==============================================================================
//
// File: blurHelper.cpp
//
// Copyright (c) 2006 Ensemble Studios
//
//==============================================================================
#include "xgameRender.h"
#include "blurHelper.h"

//==============================================================================
// BBlurHelper::BBlurHelper
//==============================================================================
BBlurHelper::BBlurHelper()
{
}

//==============================================================================
// BBlurHelper::~BBlurHelper
//==============================================================================
BBlurHelper::~BBlurHelper()
{
}

//==============================================================================
// BBlurHelper::computeGaussianSample
//==============================================================================
float BBlurHelper::computeGaussianSample(float x, float sigma) 
{   
   return exp(-x*x/(2.0f*sigma*sigma)) / (sigma*sqrt(2.0f*cPi));
}

//==============================================================================
// BToneMapManager::computeGaussianWeights
//==============================================================================
float* BBlurHelper::computeGaussianWeights(float* pDst, int maxDst, float sigma, float intensity, uint maxSamples, int& filterWidth, int& sampleCount)
{
   filterWidth = static_cast<int>(floor(3.0f*sigma)-1);
   if (filterWidth < 0)
      filterWidth = 0;

   int maxFilterWidth = (maxSamples-1)/2;
   if (filterWidth > maxFilterWidth)
      filterWidth = maxFilterWidth;

   sampleCount = filterWidth*2+1;
   BDEBUG_ASSERT(sampleCount <= (int)maxSamples);

   BDEBUG_ASSERT(sampleCount <= maxDst);
   float* pWeights = pDst; //new float[sampleCount];
   memset(pWeights, 0, sizeof(float)*sampleCount);

   const float minAcceptableWeight = 0.000125f;

   int   actualSampleCount = 0;
   int x;
   for(x=0; x<sampleCount; x++) 
   {
      float weight = computeGaussianSample((float) x-filterWidth, sigma);
      if (weight < minAcceptableWeight)
         continue;

      pWeights[actualSampleCount] = weight;      
      ++actualSampleCount;
   }

   float totalWeight = 0.0;
   for(x=0; x<actualSampleCount; x++) 
      totalWeight += pWeights[x];

   //-- normalize the weights
   for(x=0; x<actualSampleCount; x++) 
   {
      pWeights[x] /= totalWeight;
      pWeights[x] *= intensity;
   }

   //-- store our actual sample count b4 we pass them back
   sampleCount = actualSampleCount;
   return pWeights;
}

//==============================================================================
// BToneMapManager::computeBilinearBloomSamples
//==============================================================================
int BBlurHelper::computeBilinearBloomSamples(
   float sigma, 
   float intensity, 
   BVec4* pWeights, 
   BVec4* pHorizontal, 
   BVec4* pVertical, 
   uint maxSamples, 
   uint textureWidth, 
   uint textureHeight)
{
   BDEBUG_ASSERT(pWeights);
   BDEBUG_ASSERT(pHorizontal);
   BDEBUG_ASSERT(pVertical);

   int weightCount = 0;
   int filterWidth = 0;

   const int cMaxTempFloats = 128;
   float temp[cMaxTempFloats];

   float* pTempWeights = computeGaussianWeights(temp, cMaxTempFloats, sigma, intensity, maxSamples * 2, filterWidth, weightCount);
   if (!pTempWeights || weightCount <= 0)
      return 0;

   //-- calculate new set of weights and offsets
   int sampleCount = (int) ceilf(weightCount/2.0f);

   if (sampleCount < 0)
      sampleCount = 0;

   BDEBUG_ASSERT(sampleCount <= (int)maxSamples);

   for(int i=0; i<sampleCount; i++) 
   {
      float a = pTempWeights[i*2];
      float b;
      if ((i*2+1) > weightCount-1)
         b = 0;
      else
      {
         BDEBUG_ASSERT((i*2+1) < weightCount);
         b = pTempWeights[i*2+1];
      }

      pWeights[i].getX() = a + b;
      float offset = b / (a + b);

      float yOffset = ((i*2)-filterWidth + offset);
      float xOffset = ((i*2)-filterWidth + offset);

      xOffset /= textureWidth;
      yOffset /= textureHeight;

      pHorizontal[i].getX() = xOffset;
      pHorizontal[i].getY() = 0.0f;

      pVertical[i].getX() = 0.0f;
      pVertical[i].getY() = yOffset;
   }

   return sampleCount;
}
//==============================================================================
//
// File: blurHelper.h
//
// Copyright (c) 2006 Ensemble Studios
//
//==============================================================================
#pragma once
#include "math\generalVector.h"

//==============================================================================
// class BBlurHelper
//==============================================================================
class BBlurHelper
{
public:
   BBlurHelper();
   ~BBlurHelper();

   int computeBilinearBloomSamples(
      float sigma, 
      float intensity, 
      BVec4* pWeights, 
      BVec4* pHorizontal, 
      BVec4* pVertical, 
      uint maxSamples, 
      uint textureWidth, 
      uint textureHeight);

private:
   float computeGaussianSample(float x, float sigma);
   float* computeGaussianWeights(float* pDst, int maxDst, float sigma, float intensity, uint maxSamples, int& filterWidth, int& sampleCount);
};
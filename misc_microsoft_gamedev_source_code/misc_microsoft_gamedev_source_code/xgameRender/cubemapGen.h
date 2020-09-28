//==============================================================================
// File: cubemapGen.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once
#include "resampler.h"
#include "RGBAImage.h"
#include "math\sphericalHarmonic.h"

//==============================================================================
// class BCubemapGen
//==============================================================================
class BCubemapGen
{
public:
   BCubemapGen();
   ~BCubemapGen();
   
   void init(uint srcWidth, uint srcHeight, uint dim);
   void deinit(void);
   
   void captureFace(IDirect3DTexture9* pCaptureTex, uint faceIndex);
   
   bool saveToHDRFile(long dirID, const char* pFilename);
         
   void calculateSHCoefficients(SphericalHarmonic::Vec9Vector& coeffs);
   
   // The SH coefficients should be premultiplied by the irradiance convolution coefficients if diffuse lighting is the intended usage.
   void unprojectSHCoefficients(const SphericalHarmonic::Vec9Vector& coeffs, uint dim);
   
   uint getCrossCubemapDimension(void) const { return mCubeDim; }
   const BRGBA16Image& getCrossCubemapImage(void) const { return mCubemap; }

private:
   enum { cNumResamplers = 3 };
   Resampler* mpResamplers[cNumResamplers];
   
   uint mSrcWidth; 
   uint mSrcHeight;
   uint mCubeDim;

   BRGBA16Image mCubemap;
};


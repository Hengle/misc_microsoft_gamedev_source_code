// =============================================================================
// RGBHistogram.cpp
//
// Copyright (c) 2002 Ensemble Studios
//
// created by Matt Pritchard 7/2002
// =============================================================================


#include "xsystem.h"
#include "RGBHistogram.h"
#include "image.h"





// =============================================================================
// BRGBHistogram::Constructor
// =============================================================================
BRGBHistogram::BRGBHistogram(long numBits) :
   mNumBits(0),
   mColorScale(0),
   mCubeSize(0),
   mHistogramSize(0),
   mRedScale(0),
   mGreenScale(0),
   mBlueScale(0),   
   mHistData(NULL)
{
   if (numBits < 2 || numBits > 8)
   {
      BASSERT(0);
      return;
   }

   // Setup Histogram Info

   mNumBits       = numBits;
   mColorScale    = 8 - mNumBits;            // 8 bits minus the number of precision bits we have

   mCubeSize      = 1 << numBits;

   mHistogramSize = mCubeSize * mCubeSize * mCubeSize;
   
   mRedScale      = mCubeSize * mCubeSize;
   mGreenScale    = mCubeSize;
   mBlueScale        = 1;                     // Included so it's easy to swap axis

   // Allocate & Init Histogram

   mHistData = new DWORD[mHistogramSize];
   if (mHistData == NULL)
   {
      BASSERT(0);
      return;
   }

   reset();


}


// =============================================================================
// BRGBHistogram::Destructor
// =============================================================================
BRGBHistogram::~BRGBHistogram(void)
{
   if (mHistData != NULL)
   {
      delete[] mHistData;
   }
}


// =============================================================================
// BRGBHistogram::Operator= - copies a RGBHistogram
// =============================================================================
BRGBHistogram& BRGBHistogram::operator=(BRGBHistogram &SourceHistogram)
{

   if (mHistData != NULL)
   {
      delete[] mHistData;
   }

   // Copy Members

   mNumBits          = SourceHistogram.mNumBits;        
   mColorScale       = SourceHistogram.mColorScale;
   mCubeSize         = SourceHistogram.mCubeSize;       
   mHistogramSize    = SourceHistogram.mHistogramSize;     

   mRedScale         = SourceHistogram.mRedScale;       
   mGreenScale       = SourceHistogram.mGreenScale;     
   mBlueScale        = SourceHistogram.mBlueScale;      

   // Copy Histogram Data
   
   mHistData = new DWORD[mHistogramSize];
   if (mHistData == NULL)
   {
      BASSERT(0);
   }
   else
   {
      memcpy(mHistData, SourceHistogram.mHistData, mHistogramSize * sizeof(*mHistData));
   }

   return(*this);
}


// =============================================================================
// BRGBHistogram::reset
// =============================================================================
void BRGBHistogram::reset(void)
{
   if (mHistData != NULL)
   {
      memset(mHistData, 0L, mHistogramSize * sizeof(*mHistData));
   }
}

// =============================================================================
// BRGBHistogram::generateFromImage
// =============================================================================
bool BRGBHistogram::generateFromImage(const BImage *theImage)
{

   if (theImage == NULL || !theImage->getValid())
   {
      BASSERT(0);
      return(false);
   }

   reset();

   // Fill out the data
   DWORD* data = theImage->getRawData();

   for (uint i = 0; i < theImage->getWidth() * theImage->getHeight(); i++)
   {
      DWORD dataVal = data[i];
      long  red     = (IMG_GETR(dataVal)) >> mColorScale;
      long  green   = (IMG_GETG(dataVal)) >> mColorScale;
      long  blue    = (IMG_GETB(dataVal)) >> mColorScale;

      long  index   = red * mRedScale + green * mGreenScale + blue * mBlueScale;

      mHistData[index]++;
   }
   
   return(true);
}

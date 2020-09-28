// =============================================================================
// RGBHistogram.h
//
// Copyright (c) 2002 Ensemble Studios
//
// created by Matt Pritchard 7/2002
// =============================================================================

#pragma once 

#ifndef _RGBHISTOGRAM_H_
#define _RGBHISTOGRAM_H_


// =============================================================================
// Forward declarations
// =============================================================================

class BImage;

                     
// =============================================================================
// BRGBHistogram Class
//
// Provedes services for managine a 3D Histogram of RGB frequencies
// =============================================================================
class BRGBHistogram
{                    
   public: 
      long              mNumBits;               // Number of bits of precision
      long              mColorScale;            // Number of bits to shift off of 8-bit R,G,B values to scale for the Histogram

      long              mCubeSize;              // size of each axis in the RGB array
      long              mHistogramSize;         // size of the entire Histogram Array

      long              mRedScale;              // Amount to scale Red Index values by
      long              mGreenScale;            // Amount to scale Green Index values by
      long              mBlueScale;             // Amount to scale Blue Index values by
      
      DWORD*            mHistData;              // The Histogram Data
                     
   public:           
      explicit          BRGBHistogram(const long numBits);
                       ~BRGBHistogram(void);

      BRGBHistogram&    operator= (BRGBHistogram &SourceHistogram);

      void              reset();                // Clears Histogram

      bool              generateFromImage(const BImage *theImage);      // Creates Histogram from a BImage

      DWORD*            getData(void)           {return (mHistData);}
      long              getCubeSize(void)       {return (mCubeSize);}
      
};




#endif // _RGBHISTOGRAM_H_

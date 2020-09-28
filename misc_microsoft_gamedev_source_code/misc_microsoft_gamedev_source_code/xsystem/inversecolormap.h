// =============================================================================
// inverseColorMap.h
//
// Copyright (c) 1999-2002 Ensemble Studios
// =============================================================================

#pragma once 

#ifndef _INVERSECOLORMAP_H_
#define _INVERSECOLORMAP_H_


// =============================================================================
// Forward declarations
// =============================================================================

class BPalette;
class BImage;
struct BRGBQuad;



// =============================================================================
// BInverseColorMap Class
// =============================================================================

class BInverseColorMap
{
      
	private:		// Vars used in the ICM generation process
		long 	         mRcenter, mBcenter, mGcenter;
		long 	         mGdist, mRdist, mCdist;
		long 	         mcRinc, mcGinc, mcBinc;
		DWORD          *mGdp, *mRdp, *mCdp;
		BYTE	         *mGrgbp, *mRrgbp, *mCrgbp;
		long	         mGstride, mRstride;
		long	         mXsqr;
      long           mColorMax;              // size of the ICM on each axis, aka the maximum scaled R,G,B lookup value
      long           mCubeSize;              // numbe of ICM / Distance Buffer entries
		long 	         mCindex;

	private:
		void 	         init();

		void 	         computeICMap(long colors, BRGBQuad* colormap, long bits, DWORD *dist_buf, BYTE *rgbmap);
		long 	         redLoop(void);
		long 	         greenLoop(long restart);
		long 	         blueLoop(long restart);

      void           clearDistanceBuffer(void);

	protected:
   	bool	         mValidICM;				// Inverse Color map data is valid
      long	         mBits;					// # of significant bits of Resoultion in the ICM

      long	         mRedScale;				// Scale factor for red value in ICM lookup
      long	         mGreenScale;			// Scale factor for green value in ICM lookup
      long	         mBlueScale;				// Scale factor for blue value in ICM lookup

      DWORD	         *mDistanceBuffer;		// Distance buffer (used during ICM calculation)
      BYTE	         *mRGBData;				// the Color

      enum {
                     cDefaultICMSize = 6  // default ICM size is 6 bits (aka 2^6) 64 elements per axis
      };

	public:  // Data members

 	public:
   	explicit       BInverseColorMap(void);
   	explicit       BInverseColorMap(long numBits);

                    ~BInverseColorMap(void);

     bool            generateICMforPalette(BPalette &thePalette);
     long            getIndexedColor(long Red, long Green, long Blue);

      // Accessor functions

	  bool            getValid(void)           {return (mValidICM);};
     long            getResolution(void)     {return(mBits);};

     BYTE*           getColorTable(void)     {return (mRGBData);};

     long	         getRedScale(void)       {return (mRedScale);};
     long	         getGreenScale(void)     {return (mGreenScale);};
     long	         getBlueScale(void)      {return (mBlueScale);};


};

                     




#endif _INVERSECOLORMAP_H_
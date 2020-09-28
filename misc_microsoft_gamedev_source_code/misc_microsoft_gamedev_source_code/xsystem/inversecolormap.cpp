// =============================================================================
// inverseColorMap.cpp
//
// Copyright (c) 1999-2002 Ensemble Studios
// =============================================================================

#include "xsystem.h"
#include "inverseColorMap.h"
#include "palette.h"



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// BInverseColorMap - device that handles palette to RGB conversion
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//#define INSTRUMENT_IT

#ifdef INSTRUMENT_IT
	long outercount = 0;
	long innercount = 0;
#endif


// =============================================================================
// BInverseColorMap::BInverseColorMap - Consturctor
// =============================================================================
BInverseColorMap::BInverseColorMap()
{
	init();
}


// =============================================================================
// BInverseColorMap::BInverseColorMap - Resolution Consturctor
// =============================================================================
BInverseColorMap::BInverseColorMap(long numBits)
{
	init();
   if ((numBits >= 4) && (numBits <= 8))
	   mBits = numBits;
}


// =============================================================================
// BInverseColorMap::~BInverseColorMap - Destructor
// =============================================================================
BInverseColorMap::~BInverseColorMap()
{
	if (mDistanceBuffer != NULL)
   {
   	delete[] mDistanceBuffer;
      mDistanceBuffer = NULL;
   }

   if (mRGBData != NULL)
   {
   	delete[] mRGBData;
      mRGBData = NULL;
   }

}


// =============================================================================
// BInverseColorMap::init - setup object
// =============================================================================
void BInverseColorMap::init()
{
	mValidICM = false;
   mBits     = cDefaultICMSize;            

   mDistanceBuffer = NULL;
   mRGBData        = NULL;

   mRedScale   = 0;
   mGreenScale = 0;
   mBlueScale  = 0;

}


// =============================================================================
// BInverseColorMap::computeICMap - generate an ICM for a Palette
// =============================================================================
void BInverseColorMap::computeICMap(long numColors, BRGBQuad *colormap, long bits, DWORD *dist_buf, BYTE *rgbMap)
{
	bool  ColorsProcessed = false;
	long 	nbits, x;

	nbits       = 8 - bits;
   mColorMax   = 1 << bits;
   x           = 1 << nbits;
   mXsqr       = 1 << (2 * nbits);

   // Compute "strides" for accessing the arrays. 

   mGstride = mColorMax;
   mRstride = mColorMax * mColorMax;

#ifdef INSTRUMENT_IT
	outercount = 0;
	innercount = 0;
#endif

   // Reset Distance Buffer

	clearDistanceBuffer();

   for (mCindex = 0; mCindex < numColors; mCindex++)
   {
      //
      // Distance formula is
      // (red - map[0])^2 + (green - map[1])^2 + (blue - map[2])^2
      //
      // Because of quantization, we will measure from the center of
      // each quantized "cube", so blue distance is
      // 	(blue + x/2 - map[2])^2,
      // where x = 2^(8 - bits).
      // The step size is x, so the blue increment is
      // 	2*x*blue - 2*x*map[2] + 2*x^2
      //
      // Now, b in the code below is actually blue/x, so our
      // increment will be 2*(b*x^2 + x^2 - x*map[2]).  For
      // efficiency, we will maintain this quantity in a separate variable
      // that will be updated incrementally by adding 2*x^2 each time.
      //

      // The initial position is the cell containing the colormap
      // entry.  We get this by quantizing the colormap values.

      // RG: Special case added when bits = 5. This code seems to achieve a
      // more visually pleasing mapping than Spencer's original code.  Don't
      // ask for a mathematical basis for the following, as these
      // modifications where stumbled upon experimentally.

		if (nbits == 3)		// 5 bits? (8-3 = 5)
		{
  			int r, g, b;

		   x = 2;
		   mXsqr = 4;

			r = mRcenter = colormap[mCindex].Red;
	  		g = mGcenter = colormap[mCindex].Green;
  			b = mBcenter = colormap[mCindex].Blue;

			r >>= 2;        // 6-bits of precision for original entry 
    		g >>= 2;
    		b >>= 2;

    		mRcenter >>= 3;  // 5-bits of precision for quantized entry
    		mGcenter >>= 3;
    		mBcenter >>= 3;

			mRdist = r - ((mRcenter << 1) + 1);
    		mGdist = g - ((mGcenter << 1) + 1);
    		mCdist = b - ((mBcenter << 1) + 1);
    		mCdist = mRdist * mRdist + mGdist * mGdist + mCdist * mCdist;

    		mcRinc = 4 - (4 * r) + (8 * mRcenter);
    		mcGinc = 4 - (4 * g) + (8 * mGcenter);
    		mcBinc = 4 - (4 * b) + (8 * mBcenter);
		}
		else
		{
  			mRcenter = (int)colormap[mCindex].Red >> nbits;
	  		mGcenter = (int)colormap[mCindex].Green >> nbits;
  			mBcenter = (int)colormap[mCindex].Blue >> nbits;

	  		mRdist = (int)colormap[mCindex].Red - (mRcenter * x + x/2);
  			mGdist = (int)colormap[mCindex].Green - (mGcenter * x + x/2);
	  		mCdist = (int)colormap[mCindex].Blue - (mBcenter * x + x/2);
  			mCdist = mRdist*mRdist + mGdist*mGdist + mCdist*mCdist;

	  		mcRinc = 2 * ((mRcenter + 1) * mXsqr - ((int)colormap[mCindex].Red * x));
  			mcGinc = 2 * ((mGcenter + 1) * mXsqr - ((int)colormap[mCindex].Green * x));
	  		mcBinc = 2 * ((mBcenter + 1) * mXsqr - ((int)colormap[mCindex].Blue * x));
  		}

		// Array starting points
		mCdp = dist_buf + mRcenter * mRstride + mGcenter * mGstride + mBcenter;
		mCrgbp = rgbMap + mRcenter * mRstride + mGcenter * mGstride + mBcenter;

		redLoop();

      ColorsProcessed = true;
	}

   // Were no colors marked as used?  if so, create a null ICM

   if (!ColorsProcessed)
   {
      long nDist = 1 << mBits;
		mCubeSize = nDist * nDist * nDist;
   	memset(mRGBData, 0, mCubeSize * sizeof(mRGBData[0]));
	}

	#ifdef INSTRUMENT_IT
   	 fprintf( stderr, "K = %d, N = %d, outer count = %ld, inner count = %ld\n",
	   	  numColors, mColorMax, outercount, innercount );
	#endif
}


// =============================================================================
// BInverseColorMap::redLoop -- loop up and down from red center.
// =============================================================================
long BInverseColorMap::redLoop()
{
   int   detect;
   int   r;
   // int i = mCindex;
   int   first;
   int   txsqr = mXsqr + mXsqr;

   // static int here, min, max;
   static int rxx;

   detect = 0;

   // Basic loop up 
   for ( r = mRcenter, mRdist = mCdist, rxx = mcRinc,
	  		mRdp = mCdp, mRrgbp = mCrgbp, first = 1;
			r < mColorMax;
			r++, mRdp += mRstride, mRrgbp += mRstride,
	  		mRdist += rxx, rxx += txsqr, first = 0 )
   {
		if ( greenLoop( first ) )
      {
		   detect = 1;
      }
		else if ( detect )
      {
	    	break;
      }
   }

   // Basic loop down
   for ( r = mRcenter - 1, rxx = mcRinc - txsqr, mRdist = mCdist - rxx,
		   mRdp = mCdp - mRstride, mRrgbp = mCrgbp - mRstride, first = 1;
	  		r >= 0;
	  		r--, mRdp -= mRstride, mRrgbp -= mRstride,
	  		rxx -= txsqr, mRdist -= rxx, first = 0 )
   {
		if ( greenLoop( first ) )
      {
	   	detect = 1;
      }
      else if ( detect )
      {
	   	break;
      }
   }

   return(detect);
}


// =============================================================================
// BInverseColorMap::greenLop -- loop up and down from green center.
// =============================================================================
long BInverseColorMap::greenLoop(long restart)
{
   int   detect;
   int   g;
   // int i = mCindex;
   int   first;
   int   txsqr = mXsqr + mXsqr;
   static int here, min, max;

#ifdef MINMAX_TRACK
   static int prevmax, prevmin;
   int thismax, thismin;
#endif

   static int ginc, gxx, gcdist;		// "gc" variables maintain correct 
   static DWORD *gcdp;					//  values for mBcenter position, 
   static BYTE	*gcrgbp;					//  despite modifications by blueloop 
												//  to mGdist, mGdp, mGrgbp. 

   if ( restart )
   {
		here = mGcenter;
		min = 0;
		max = mColorMax - 1;
		ginc = mcGinc;

		#ifdef MINMAX_TRACK
			prevmax = 0;
			prevmin = mColorMax;
		#endif
   }

	#ifdef MINMAX_TRACK
   	thismin = min;
    	thismax = max;
	#endif

   detect = 0;

   // Basic loop up
   for ( g = here, gcdist = mGdist = mRdist, gxx = ginc,
		  	gcdp = mGdp = mRdp, gcrgbp = mGrgbp = mRrgbp, first = 1;
	  		g <= max;
	  		g++, mGdp += mGstride, gcdp += mGstride, mGrgbp += mGstride, gcrgbp += mGstride,
	  		mGdist += gxx, gcdist += gxx, gxx += txsqr, first = 0 )
   {
		if ( blueLoop( first ) )
		{
	    	if ( !detect )
	    	{
				// Remember here and associated data!
				if ( g > here )
				{
		    		here = g;
		    		mRdp = gcdp;
		    		mRrgbp = gcrgbp;
		    		mRdist = gcdist;
		    		ginc = gxx;

					#ifdef MINMAX_TRACK
		    			thismin = here;
					#endif
				}
				detect = 1;
	    	}
		}
		else if ( detect )
		{
			#ifdef MINMAX_TRACK
	    		thismax = g - 1;
			#endif
	    	break;
      }
   }

   // Basic loop down
   for ( g = here - 1, gxx = ginc - txsqr, gcdist = mGdist = mRdist - gxx,
	  		gcdp = mGdp = mRdp - mGstride, gcrgbp = mGrgbp = mRrgbp - mGstride,
	  		first = 1;
	  		g >= min;
	  		g--, mGdp -= mGstride, gcdp -= mGstride, mGrgbp -= mGstride, gcrgbp -= mGstride,
	  		gxx -= txsqr, mGdist -= gxx, gcdist -= gxx, first = 0 )
   {
		if ( blueLoop( first ) )
		{
	   	if ( !detect )
	    	{
				// Remember here! 
				here = g;
				mRdp = gcdp;
				mRrgbp = gcrgbp;
				mRdist = gcdist;
				ginc = gxx;

				#ifdef MINMAX_TRACK
					thismax = here;
				#endif
				detect = 1;
	    	}
		}
		else if ( detect )
		{
			#ifdef MINMAX_TRACK
	    		thismin = g + 1;
			#endif
	    	break;
		}
   }

	#ifdef MINMAX_TRACK
   	// If we saw something, update the edge trackers.  For now, only
     	// tracks edges that are "shrinking" (min increasing, max decreasing)

   	if ( detect )
    	{
			if ( thismax < prevmax )
	    		max = thismax;

			prevmax = thismax;

			if ( thismin > prevmin )
	   		min = thismin;

			prevmin = thismin;
    	}
	#endif

   return detect;
}

// =============================================================================
// BInverseColorMap::blueLoop -- loop up and down from blue center.
// =============================================================================
long BInverseColorMap::blueLoop(long restart)
{
   int detect;
   DWORD	*dp;
   BYTE *rgbp;
   DWORD bdist, bxx;
   int 	b, i = mCindex;
   int 	txsqr = mXsqr + mXsqr;
   int 	lim;
   static int here, min, max;

	#ifdef MINMAX_TRACK
   	static int prevmin, prevmax;
    	int thismin, thismax;
	#endif // MINMAX_TRACK 

   static int binc;

   if ( restart )
   {
		here = mBcenter;
		min = 0;
		max = mColorMax - 1;
		binc = mcBinc;

		#ifdef MINMAX_TRACK
			prevmin = mColorMax;
			prevmax = 0;
		#endif // MINMAX_TRACK 
   }

   detect = 0;

   #ifdef MINMAX_TRACK
    	thismin = min;
    	thismax = max;
	#endif

   // Basic loop up. 
   // First loop just finds first applicable cell.

   for ( b = here, bdist = mGdist, bxx = binc, dp = mGdp, rgbp = mGrgbp, lim = max;
	  		b <= lim;
	  		b++, dp++, rgbp++,
	  		bdist += bxx, bxx += txsqr )
   {

		#ifdef INSTRUMENT_IT
			outercount++;
		#endif

		if ( *dp > bdist )
		{
	   	// Remember new 'here' and associated data!
	    	if ( b > here )
	    	{
				here = b;
				mGdp = dp;
				mGrgbp = rgbp;
				mGdist = bdist;
				binc = bxx;

				#ifdef MINMAX_TRACK
					thismin = here;
				#endif
	    	}
		   detect = 1;

			#ifdef INSTRUMENT_IT
				outercount--;
			#endif
	    	break;
		}
   }

   // Second loop fills in a run of closer cells.
   for ( ;
	  		b <= lim;
	  		b++, dp++, rgbp++,
	  		bdist += bxx, bxx += txsqr )
   {
		#ifdef INSTRUMENT_IT
			outercount++;
		#endif

		if ( *dp > bdist )
		{
			#ifdef INSTRUMENT_IT
	    		innercount++;
			#endif

	    	*dp = bdist;
	    	*rgbp = (BYTE) (i & 0x00FF);
		}
		else
		{
			#ifdef MINMAX_TRACK
	    		thismax = b - 1;
			#endif

	    	break;
		}
   }

   // Basic loop down. 
   // Do initializations here, since the 'find' loop might not get executed.

   lim = min;
   b = here - 1;
   bxx = binc - txsqr;
   bdist = mGdist - bxx;
   dp = mGdp - 1;
   rgbp = mGrgbp - 1;

   // The 'find' loop is executed only if we didn't already find something.
   
   if ( !detect )
   {
		for ( ;
	      	b >= lim;
	      	b--, dp--, rgbp--,
	      	bxx -= txsqr, bdist -= bxx )
      {
			#ifdef INSTRUMENT_IT
	    		outercount++;
			#endif

	    	if ( *dp > bdist )
	    	{
				// Remember here! 
				// No test for b against here necessary because b <
		 		// here by definition.
		 		 
				here = b;
				mGdp = dp;
				mGrgbp = rgbp;
				mGdist = bdist;
				binc = bxx;

				#ifdef MINMAX_TRACK
					thismax = here;
				#endif

				detect = 1;

				#ifdef INSTRUMENT_IT
					outercount--;
				#endif
				break;
	    	}
		}
   }

   // The 'update' loop.
   for ( ;
	  		b >= lim;
	  		b--, dp--, rgbp--,
	  		bxx -= txsqr, bdist -= bxx )
   {

		#ifdef INSTRUMENT_IT
			outercount++;
		#endif

		if ( *dp > bdist )
		{
			#ifdef INSTRUMENT_IT
	    		innercount++;
			#endif
	    	*dp = bdist;
	    	*rgbp = (BYTE) (i & 0x00FF);
		}
		else
		{
			#ifdef MINMAX_TRACK
	    		thismin = b + 1;
			#endif
	    	break;
		}
   }

	// If we saw something, update the edge trackers.

#ifdef MINMAX_TRACK
   if ( detect )
   {
		// Only tracks edges that are "shrinking" (min increasing, max decreasing)
	 	
		if ( thismax < prevmax )
	   	max = thismax;

		if ( thismin > prevmin )
	    	min = thismin;

		// Remember the min and max values
		prevmax = thismax;
		prevmin = thismin;
   }
#endif // MINMAX_TRACK 

    return detect;
}

// =============================================================================
// BInverseColorMap::maxfill - fills in the buffer
// =============================================================================
void BInverseColorMap::clearDistanceBuffer(void)
{
   if (mDistanceBuffer == NULL || mCubeSize <= 0)
   {
      BASSERT(0);
      return;
   }

   memset(mDistanceBuffer, 0L, mCubeSize * sizeof(mDistanceBuffer[0]));

}


// =============================================================================
// BInverseColorMap::GenerateICMforPalette - Creates an ICM for a palette
// returns false if an error occurs
// =============================================================================
bool BInverseColorMap::generateICMforPalette(BPalette &thePalette)
{
   // Make sure we are initialized

	if (mBits <= 0)
   {
      BASSERT(0);
      return(false);
   }

   // Compue sizes and scales for an ICM with mBits
   // of precision

   long  nDist    = 1 << mBits;
   mCubeSize = nDist * nDist * nDist;

   // amount to scale R, G, B values to find ICM index

   mBlueScale  = 1;
   mGreenScale = nDist;
   mRedScale   = nDist * nDist;

   // Allocate buffers, delete any previous buffers

   if (mRGBData != NULL)
   {
      delete[] mRGBData;
   }

   mDistanceBuffer   = new DWORD[mCubeSize];
   mRGBData          = new BYTE[mCubeSize];

   // Build the ICM for the supplied palette

	computeICMap(thePalette.getSize(), &thePalette.getPaletteData()[0], mBits, mDistanceBuffer, mRGBData);

   // Deallocate temp working stuff

   delete[]  mDistanceBuffer;
   mDistanceBuffer = NULL;

   // This Inverse Colormap now has valid entries

   mValidICM = true;

	return(true);
}


// =============================================================================
// BInverseColorMap::GetIndexedColor - lookup for when we are not in a hurry
// returns -1 if an error occurs
// =============================================================================
long BInverseColorMap::getIndexedColor(long Red, long Green, long Blue)
{
	long  index;
   long	Shift = 8-mBits;

   // Make sure we have a valid ICM

	if (!mValidICM || (mRGBData == NULL))
   {
      return(-1);
   }

   // Compute 3-d index into ICM

   index = ((Red >> Shift) * mRedScale) + ((Green >> Shift) * mGreenScale) + ((Blue >> Shift) * mBlueScale);

   // and return the pallete index for the given (R,G,B) color values

   return ( (long) mRGBData[index]);
}










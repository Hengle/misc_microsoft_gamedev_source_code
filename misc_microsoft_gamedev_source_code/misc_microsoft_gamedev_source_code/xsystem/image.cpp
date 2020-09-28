//==============================================================================
// image.cpp
//
// Copyright (c) 1999 - 2002 Ensemble Studios
//==============================================================================

#include "xsystem.h"
#include "mathutil.h"
#ifndef XBOX
   #include "libjpg\jpeglib.h"
#endif   
#include "readTGA.h"
#include "writeTGA.h"

#include "inverseColorMap.h"
#include "palette.h"
#include "RGBHistogram.h"


#include "image.h"

//==============================================================================
// BImage Static Data members
//==============================================================================
BQuantizeBox** BImage::mHeap = NULL;               // priority queue
long           BImage::mHeap_size = 0;
BQuantizeBox*  BImage::mBoxes = NULL;              // variance box list
long           BImage::mNum_boxes = 0;
DWORD*         BImage::mHistogramData = NULL;      // current histogram data

// Defines for quantization

#define HIST_BIT   (6)
#define HIST_MAX   (1 << HIST_BIT)
#define R_STRIDE   (HIST_MAX * HIST_MAX)
#define G_STRIDE   (HIST_MAX)
#define HIST_SHIFT (8 - HIST_BIT)


//==============================================================================
// BImage::BImage
//==============================================================================
BImage::BImage( void ) :
   mValidImage(false),
   mWrapped(false),
   mData(0),
   mWidth(0),
   mHeight(0)
{
   // Nothing to do 
}

//==============================================================================
// BImage::~BImage
//==============================================================================
BImage::~BImage( void )
{
   deInit();
}

//==============================================================================
// BImage::allocateData
//==============================================================================
bool BImage::allocateData( long w, long h, bool bfill /* = true */) 
{
   // We can't reallocate if we're wrapping data we don't own.
   if(mWrapped)
   {
      BFAIL("BImage::allocateData cannot be performed when wrapping ane external buffer.");
      return(false);
   }


   BASSERT (w > 0 && h > 0);

   deInit();   

   mData = new DWORD[w*h];

   if (!mData)
   {
      BASSERT(0);
      return false;
   }

   if (bfill)
   {
      memset(mData, 0, sizeof(DWORD) * w * h);
   }

   mWidth=w;
   mHeight=h;
   mValidImage = true;

   return (true);
}


void BImage::wrapBuffer(long width, long height, DWORD *data)
{
   // Clean up anything old.
   deInit();

   // Copy stuff over.
   mWidth=width;
   mHeight=height;
   mData=data;

   // Mark this as a wrapped buffer so we know not to try reallocating it, etc.
   mWrapped=true;

   // Also mark it as valid.
   mValidImage = true;
}


//==============================================================================
// BImage::deInit
//==============================================================================
void BImage::deInit(void)
{
   if (mData != NULL)
   {
      // If we aren't wrapping some other buffer, delete the memory,
      if(!mWrapped)
         delete[] mData;

      mData = NULL;
   }


   mValidImage = false;
   mWrapped=false;
  
   mWidth  = 0;
   mHeight = 0;

}
                              
//==============================================================================
// BImage::destroyButNotReleaseData
// Used to hand off an Bimage's contents to another image without copying
//==============================================================================
void BImage::destroyButNotReleaseData(BImage *image)  
{
   image->mData = NULL;
   delete image;
}


//==============================================================================
// BImage::copy
//==============================================================================
bool BImage::copy(BImage *image)
{
   if(image == NULL || !image->getValid())
   {
      BFAIL("Tried to copy an invalid image!");
      return(false);
   }

   // Size to soruce image dimensions.
   bool ok=allocateData(image->getWidth(), image->getHeight());
   if(!ok)
   {
      BASSERT(0);
      return(false);
   }

   // Copy bits.
   memcpy(mData, image->getRawData(), mWidth*mHeight*sizeof(*mData));

   // Success.
   return(true);
}



//==============================================================================
// BImage::copy from BImage8
//         Takes an Index Image and creates a 32-RGBA version
//==============================================================================
bool BImage::copy(BImage8 *image)
{
   if(image == NULL || !image->getValid())
   {
      BASSERT(0);
      return(false);
   }

   // Size to soruce image dimensions.
   bool ok=allocateData(image->getWidth(), image->getHeight());
   if(!ok)
   {
      BASSERT(0);
      return(false);
   }

   // Convert Pixels as we go...

   BYTE* src = image->getData();
   DWORD* palData=image->getPalette()->getPaletteRGBA();

   for (uint i = 0; i < mWidth * mHeight; i++)
   {
      mData[i] = palData[(long) src[i]];
   }


   // Success.
   return(true);
}

//==============================================================================
class Contributor
{
   public:
      long  pixel;
      float weight;
};

//==============================================================================
class ContributorList
{
   public:
      ContributorList(void): mNumber(0), mContributors(NULL) {}
      ~ContributorList(void) {delete []mContributors;}

      void allocateContributors(long num) 
      {
         if(mContributors)
            delete []mContributors;
         mNumber = 0;
          
         // Sanity.  
         if(num<0)
         {
            BFAIL("Negative num passed to allocateContributors.");
            return;
         }
            
         mContributors  = new Contributor[num];
         memset(mContributors, 0, sizeof(*mContributors)*num);
      }

      long           getNumber(void) {return(mNumber);}
      void           setNumber(long number) {mNumber = number;}
      void           incrementNumber(void) {mNumber++;}
      Contributor *  getContributors(void) {return(mContributors);}

   protected:
      long           mNumber;
      Contributor *  mContributors;
};

//==============================================================================
struct FloatARGB
{
   float a, r, g, b;
};



//==============================================================================
// sinc
//==============================================================================
float sinc(float x)
{
   const float cEpsilon=0.0000001f;

   x *= cPi;
   if(fabs(x) > cEpsilon)
      return((float)sin(x) / x);
   else
      return(1.0f);
} // sinc

//==============================================================================
// filter - Lanczos3 filter
//==============================================================================
/*
const float cFilterWidth=3.0f;
float filter(float t)         
{
   if(t < 0)
      t = -t;
   if(t <= 3.0f)
      return(sinc(t) * sinc(t/3.0f));
   return(0.0f);
} // filter
*/

/*
const float cFilterWidth=2.0f;
//==============================================================================
// filter - Mitchell filter
//==============================================================================
float filter(float t)         
{
   static const float cOneSixth=1.0f/6.0f;
   static const float B=1.0f/3.0f;
   static const float C=1.0f/3.0f;

   float tt = t*t;
   if(t<0.0f)
      t=-t;
   if(t<1.0f)
      return(cOneSixth*(((12.0f-9.0f*B-6.0f*C)*(t*tt)) + ((-18.0f+12.0f*B+6.0f*C)*tt) + (6.0f-2.0f*B)));
   else if(t<2.0f)
      return(cOneSixth*(((-1.0f*B - 6.0f*C)*(t*tt)) + ((6.0f*B+30.0f*C)*tt) + ((-12.0f*B-48.0f*C)*t) + (8.0f*B+24.0f*C)));

   return(0.0f);
} // filter
*/

const float cFilterWidth=0.5f;
//==============================================================================
// filter - Box filter
//==============================================================================
float filter(float t)         
{
   if((t>-0.5f) && (t<=0.5f)) 
      return(1.0f);
   else
      return(0.0f);
} // filter

/*
const float cFilterWidth=2.0f;
//==============================================================================
// filter - B-spline filter
//==============================================================================
float filter(float t)         
{
   static const float cOneSixth=1.0f/6.0f;
   static const float cTwoThirds=2.0f/3.0f;

   if(t<0.0f)
      t=-t;
   if(t<1.0f)
   {
      float tt = t*t;
      return(((0.5f*tt*t) - tt + cTwoThirds));
   }
   else if(t<2.0f)
   {
      t=2.0f-t;
      return(cOneSixth*t*t*t);
   }

   return(0.0f);
} // filter
*/

//==============================================================================
// BImage::resampleImage
// 
// Resamples the given image down by the given factors.  Returns 
// a pointer to an image object holding the resampled image.  The caller is
// resposible for deleting the object.  xFactor and yFactor must be between
// 0 and 1 (i.e scaling up is not allowed).
//==============================================================================
BImage *BImage::resampleImage(float xFactor, float yFactor)
{
   // Allocate a final image
   BImage *newImage = new BImage;
   if(!newImage)
   {
#ifdef _BANG
      {setBlogError(4045); blogerror("BImage::resampleImage --  failed to allocate new image");}
#endif
      return(NULL);
   }

   long newXSize = (long)((float)getWidth()*xFactor+0.5f);
   long newYSize = (long)((float)getHeight()*yFactor+0.5f);

   if(!newImage->allocateData(newXSize, newYSize))
   {
#ifdef _BANG
      {setBlogError(4046); blogerror("BImage::resampleImage --  failed to allocate new image data ");}
#endif
      delete newImage;
      return(NULL);
   }

   if (!resampleImage(newImage, xFactor, yFactor))
   {
#ifdef _BANG
      {setBlogError(4047); blogerror("BImage::resampleImage --  failed to resample image ");}
#endif
      delete newImage;
      return(NULL);
   }

   return(newImage);
}


//==============================================================================
// BImage::resampleImage
//==============================================================================
bool BImage::resampleImage(BImage *dst, float xFactor, float yFactor)
{
   const float gamma=1.7f;
   const float recipGamma=1.0f/gamma;

   if(!dst)
   {
#ifdef _BANG
      {setBlogError(4048); blogerror("BImage::resampleImage -- bad dst image");}
#endif
      return(false);
   }

   // allocate a temp image struct to hold the results of a scale in the x direction
   long oldXSize = getWidth();
   long newXSize = (long)((float)getWidth()*xFactor+0.5f);
   FloatARGB *temp = new FloatARGB[newXSize*getHeight()];
   if(!temp)
   {
#ifdef _BANG
      {setBlogError(4049); blogerror("BImage::resampleImage -- failed to allocate temp image");}
#endif
      return(false);
   }

   //Precalc filter weights for a row
   float scale = xFactor;
   float fscale = 1.0f / scale;
   float width = cFilterWidth*fscale;
   ContributorList *contributorLists = new ContributorList[newXSize];

   if (!contributorLists)
   {
#ifdef _BANG
      {setBlogError(4050); blogerror("BImage::resampleImage -- failed to allocate contributor list");}
#endif
      delete [] temp;
      return(false);
   }

   if(xFactor < 1.0f)
   {
      long maxContributors = (long)(width*2.0f+1.0f);
      float offset = 0.5f*(fscale-1);
      for(long i=0; i<newXSize; i++)
      {
         contributorLists[i].allocateContributors(maxContributors);
      
         float center = offset+(float)i * fscale;
         long left = (long)ceil(center-width);
         long right = (long)floor(center+width);

         for(long j=left; j<=right; j++)
         {
            float t = (center-(float)j)*scale;
            float weight = filter(t)*scale;
            long n=j;
            /*if(j<0)
               n = -j;
            else if(j >= oldSize)
               n = (oldSize-j) + oldSize-1;
            else
               n = j;*/
            while(n<0 || n>=oldXSize)
            {
               if(n<0)
                  n+=oldXSize;
               if(n>=oldXSize)
                  n-=oldXSize;
            }
         
            long index = contributorLists[i].getNumber();
            contributorLists[i].getContributors()[index].pixel=n;            
            contributorLists[i].getContributors()[index].weight=weight;
            contributorLists[i].incrementNumber();
         }
      }
   }
   else
   {
      long maxContributors = (long)(cFilterWidth*2.0f+1.0f);
      for(long i=0; i<newXSize; i++)
      {
         contributorLists[i].allocateContributors(maxContributors);
      
         float center = (float)i * fscale;
         long left = (long)ceil(center-cFilterWidth);
         long right = (long)floor(center+cFilterWidth);

         for(long j=left; j<=right; j++)
         {
            float t = center-(float)j;
            float weight = filter(t);
            long n=j;
            /*if(j<0)
               n = -j;
            else if(j >= oldSize)
               n = (oldSize-j) + oldSize-1;
            else
               n = j;*/
            while(n<0 || n>=oldXSize)
            {
               if(n<0)
                  n+=oldXSize;
               if(n>=oldXSize)
                  n-=oldXSize;
            }
         
            long index = contributorLists[i].getNumber();
            contributorLists[i].getContributors()[index].pixel=n;            
            contributorLists[i].getContributors()[index].weight=weight;
            contributorLists[i].incrementNumber();
         }
      }
   }
 
   // Apply the filter to shrink in the x direction (into temp)
   for(uint y=0; y<getHeight(); y++)
   {
      for(uint x=0; x<(uint)newXSize; x++)
      {
         FloatARGB *curr = &(temp[y*newXSize+x]);
         curr->a = 0.0f;
         curr->r = 0.0f;
         curr->g = 0.0f;
         curr->b = 0.0f;

         // Get total weight -- this should theoretically be 1.0, but in practice
         // can be slightly off.
         float totalWeight=0.0f;
         for(long j=0; j<contributorLists[x].getNumber(); j++)
         {
            float weight = contributorLists[x].getContributors()[j].weight;
            totalWeight+=weight;
         }
         // Get normalization factor based on the weight.
         float normalFactor=1.0f/totalWeight;

         for(long j=0; j<contributorLists[x].getNumber(); j++)
         {
            float weight = normalFactor*contributorLists[x].getContributors()[j].weight;
            //float weight = contributorLists[x].getContributors()[j].weight;
            long pixelIndex = contributorLists[x].getContributors()[j].pixel;
            DWORD pixel = getPixel(pixelIndex, y); //getData()[y*getWidth()+pixelIndex];
            curr->a += weight*float(pow(float((pixel&0xff000000) >> 24), gamma));
            curr->r += weight*float(pow(float((pixel&0x00ff0000) >> 16), gamma));
            curr->g += weight*float(pow(float((pixel&0x0000ff00) >> 8), gamma));
            curr->b += weight*float(pow(float(pixel&0x000000ff), gamma));
         }
      }
   }

   long oldYSize = getHeight();
   long newYSize = (long)((float)getHeight()*yFactor+0.5f);

   //Precalc filter weights for a column
   delete []contributorLists;
   scale = yFactor;
   fscale = 1.0f / scale;
   width = cFilterWidth*fscale;
   contributorLists = new ContributorList[newYSize];

   if (!contributorLists)
   {
#ifdef _BANG
      {setBlogError(4051); blogerror("BImage::resampleImage -- failed to allocate contributor lists");}
#endif
      delete []temp;
      delete []contributorLists;
      return(false);
   }

   if(yFactor<1.0f)
   {
      float offset = 0.5f*(fscale-1);
      long maxContributors = (long)(width*2.0f+1.0f);
      for(int i=0; i<newYSize; i++)
      {
         contributorLists[i].allocateContributors(maxContributors);
      
         float center = offset+(float)i * fscale;
         long left = (long)ceil(center-width);
         long right = (long)floor(center+width);

         for(long j=left; j<=right; j++)
         {
            float t = (center-(float)j)*scale;
            float weight = filter(t)*scale;
            long n=j;
            /*if(j<0)
               n = -j;
            else if(j >= oldSize)
               n = (oldSize-j) + oldSize-1;
            else
               n = j;*/
            while(n<0 || n>=oldYSize)
            {
               if(n<0)
                  n+=oldYSize;
               if(n>=oldYSize)
                  n-=oldYSize;
            }
         
            long index = contributorLists[i].getNumber();
            contributorLists[i].getContributors()[index].pixel=n;            
            contributorLists[i].getContributors()[index].weight=weight;
            contributorLists[i].incrementNumber();
         }
      }
   }
   else
   {
      long maxContributors = (long)(cFilterWidth*2.0f+1.0f);
      for(int i=0; i<newYSize; i++)
      {
         contributorLists[i].allocateContributors(maxContributors);
      
         float center = (float)i * fscale;
         long left = (long)ceil(center-cFilterWidth);
         long right = (long)floor(center+cFilterWidth);

         for(long j=left; j<=right; j++)
         {
            float t = center-(float)j;
            float weight = filter(t);
            long n=j;
            /*if(j<0)
               n = -j;
            else if(j >= oldSize)
               n = (oldSize-j) + oldSize-1;
            else
               n = j;*/
            while(n<0 || n>=oldYSize)
            {
               if(n<0)
                  n+=oldYSize;
               if(n>=oldYSize)
                  n-=oldYSize;
            }
         
            long index = contributorLists[i].getNumber();
            contributorLists[i].getContributors()[index].pixel=n;            
            contributorLists[i].getContributors()[index].weight=weight;
            contributorLists[i].incrementNumber();
         }
      }
   }

   // Apply the filter to shrink in the y direction (into dst)
   for(uint x=0; x<dst->getWidth(); x++)
   {
      for(uint y=0; y<dst->getHeight(); y++)
      {
         float newA = 0.0f;
         float newR = 0.0f;
         float newG = 0.0f;
         float newB = 0.0f;

         // Get total weight -- this should theoretically be 1.0, but in practice
         // can be slightly off.
         float totalWeight=0.0f;
         for(long j=0; j<contributorLists[y].getNumber(); j++)
         {
            float weight = contributorLists[y].getContributors()[j].weight;
            totalWeight+=weight;
         }
         // Get normalization factor based on the weight.
         float normalFactor=1.0f/totalWeight;

         for(long j=0; j<contributorLists[y].getNumber(); j++)
         {
            float weight = normalFactor*contributorLists[y].getContributors()[j].weight;
            //float weight = contributorLists[y].getContributors()[j].weight;
            long pixelIndex = contributorLists[y].getContributors()[j].pixel;
            FloatARGB *curr = &(temp[pixelIndex*newXSize+x]);
            newA += weight*curr->a;
            newR += weight*curr->r;
            newG += weight*curr->g;
            newB += weight*curr->b;
         }
         long a = (long)(pow(newA+0.5f, recipGamma));
         a = max(0,min(255, a));
         long r = (long)(pow(newR+0.5f, recipGamma));
         r = max(0,min(255, r));
         long g = (long)(pow(newG+0.5f, recipGamma));
         g = max(0,min(255, g));
         long b = (long)(pow(newB+0.5f, recipGamma));
         b = max(0,min(255, b));

		   dst->setPixel(x,y, ARGBToDWORD(a,r,g,b));
      }
   }


   delete []temp;
   delete []contributorLists;

   return(true);
} // resampleImage


//==============================================================================
// fastDownSample
//==============================================================================
/*
bool fastDownSample(BImage &srcImage, BImage *destImages, long levels)
{
   // Check that destImages is non-null.
   if(!destImages)
      return(false);

   // Check that the source is square.
   if(srcImage.getWidth() != srcImage.getHeight())
      return(false);

   // Check if levels is too big or non power of two.
   long maxLevels=logBase2WithError((WORD)srcImage.getWidth());
   if(maxLevels<=0 || levels>maxLevels)
      return(false);

   // Allocate float scratch space.
   long levelWidth=srcImage.getWidth();
   float *img = new float[levelWidth*levelWidth*4];
   if(!img)
      return(false);

   // Convert to float.
   DWORD *srcData=srcImage.getData();
   float *imgData=img;
   for(long y=0; y<levelWidth; y++)
   {
      for(long x=0; x<levelWidth; x++, srcData++)
      {
         *imgData=(float)((*srcData&0xff000000) >> 24);
         imgData++;
         *imgData=(float)((*srcData&0x00ff0000) >> 16);
         imgData++;
         *imgData=(float)((*srcData&0x0000ff00) >> 8);
         imgData++;
         *imgData=(float)(*srcData&0x000000ff);
         imgData++;
      }
   }

   for(long level=0; level<levels; level++)
   {
      // Get width for this level.
      levelWidth/=2;

      // Set up image.
      destImages[level].allocateData(levelWidth, levelWidth);
      DWORD *destData=destImages[level].getData();
      if(!destData)
         return(false);

      float *imgSrcData=img;
      float *imgDestData=img;
      long colOffset=levelWidth*8;

      // Average previous level to this level and convert.
      for(y=0; y<levelWidth; y++, imgSrcData+=colOffset)
      {
         for(long x=0; x<levelWidth; x++, destData++, imgSrcData+=4)
         {
            // Average the four texels.
            for(long channel=0; channel<4; channel++, imgSrcData++, imgDestData++)
               *imgDestData=0.25f*(imgSrcData[0] + imgSrcData[4] + imgSrcData[levelWidth*8] + imgSrcData[levelWidth*8+4]);

            // Pack up ARGB data.
            *destData = (((long)imgDestData[-4])<<cAlphaShift) | (((long)imgDestData[-3])<<cRedShift) | 
               (((long)imgDestData[-2])<<cGreenShift) | (((long)imgDestData[-1]));
         }
      }
   }

   delete []img;

   return(true);
}
*/

//==============================================================================
// BImage::fastDownSample
//==============================================================================
bool BImage::fastDownSample(BImage *destImages, long levels) const
{
   // Check that destImages is non-null.
   if(!destImages)
      return(false);

   // Check that the source is square.
   if(getWidth() != getHeight())
      return(false);

   // Check if levels is too big or non power of two.
   long maxLevels=logBase2WithError((WORD)getWidth());
   if(maxLevels<=0 || levels>maxLevels)
      return(false);

   // Allocate float scratch space.
   uint levelWidth=getWidth();

   BYTE *srcData;
   long srcWidth;
   BYTE *destData;

   for(long level=0; level<levels; level++)
   {
      // Get width for this level.
//      levelWidth/=2;
      levelWidth = levelWidth >> 1;

      // Set up image. - allocate only is size doesn't match
      if ((levelWidth != destImages[level].getWidth()) ||
          (levelWidth != destImages[level].getHeight()))
         destImages[level].allocateData(levelWidth, levelWidth, false);

      destData = (BYTE *) destImages[level].getRawData();

      if(!destData)
         return(false);

      if (!level)
      {
         srcData = (BYTE *) getRawData();
         srcWidth = 4 * getWidth();
      }
      else
      {
         srcData = (BYTE *) destImages[level - 1].getRawData();
         srcWidth = 4 * destImages[level - 1].getWidth();
      }
#pragma warning (disable: 4244)

      // Average previous level to this level and convert.
      for(uint y=0; y<levelWidth; y++, srcData+=srcWidth)
      {
         for(uint x=0; x<levelWidth; x++, destData += 4, srcData += 8)
         {
            *destData = (srcData[0] + srcData[4] + srcData[srcWidth + 0] + srcData[srcWidth + 4]) >> 2;
            destData[1] = (srcData[1] + srcData[5] + srcData[srcWidth + 1] + srcData[srcWidth + 5]) >> 2;
            destData[2] = (srcData[2] + srcData[6] + srcData[srcWidth + 2] + srcData[srcWidth + 6]) >> 2;
            destData[3] = (srcData[3] + srcData[7] + srcData[srcWidth + 3] + srcData[srcWidth + 7]) >> 2;
         }
      }

#pragma warning (default: 4244)
   }

   return(true);

} // fastDownSample


//==============================================================================
// BImage::displacementToNormal
//==============================================================================
BImage *BImage::displacementToNormal(void)
{
   static const float cHeightScale=10.0f/255.0f;

   // Allocate a final image
   BImage *normalMap = new BImage;
   if(!normalMap)
   {
#ifdef _BANG
      {setBlogError(4052); blogerror("BImage::displacementToNormal -- failed to allocate image");}
#endif
      return(NULL);
   }

   if(!normalMap->allocateData(getWidth(), getHeight()))
   {
#ifdef _BANG
      {setBlogError(4053); blogerror("BImage::displacementToNormal -- failed to allocate data ");}
#endif
      delete normalMap;
      return(NULL);
   }

   // Rip through pixels of dest image.
   BVector total;
   BVector tangent[8];
   for(uint y=0; y<normalMap->getHeight(); y++)
   {
      // Get up pixel Y value -- assume wrapping.
      uint upY=y+1;         
      if(upY>=normalMap->getHeight())
         upY=normalMap->getHeight()-1;
      
      // Get down pixel Y value -- assume wrapping.
      long downY=y-1;         
      if(downY<0)
         downY=normalMap->getHeight()-1;

      for(uint x=0; x<normalMap->getWidth(); x++)
      {
         // Get right pixel X value -- assume wrapping.
         uint rightX=x+1;         
         if(rightX>=normalMap->getWidth())
            rightX=normalMap->getWidth()-1;
      
         // Get right pixel X value -- assume wrapping.
         long leftX=x-1;         
         if(leftX<0)
            leftX=0;
         
         // Just pull green value -- we could compute actual "intensity" and be cooler and slower I suppose...
         float thisHeight=(float)IMG_GETG(getPixel(x, y));

         // Get tangents.
         tangent[0].set(0.0f, ((float)IMG_GETG(getPixel(x, upY))-thisHeight)*cHeightScale, 1.0f);
         tangent[1].set(1.0f, ((float)IMG_GETG(getPixel(rightX, upY))-thisHeight)*cHeightScale, 1.0f);
         tangent[2].set(1.0f, ((float)IMG_GETG(getPixel(rightX, y))-thisHeight)*cHeightScale, 0.0f);
         tangent[3].set(1.0f, ((float)IMG_GETG(getPixel(rightX, downY))-thisHeight)*cHeightScale, -1.0f);
         tangent[4].set(0.0f, ((float)IMG_GETG(getPixel(x, downY))-thisHeight)*cHeightScale, -1.0f);
         tangent[5].set(-1.0f, ((float)IMG_GETG(getPixel(leftX, downY))-thisHeight)*cHeightScale, -1.0f);
         tangent[6].set(-1.0f, ((float)IMG_GETG(getPixel(leftX, y))-thisHeight)*cHeightScale, 0.0f);
         tangent[7].set(-1.0f, ((float)IMG_GETG(getPixel(leftX, upY))-thisHeight)*cHeightScale, 1.0f);

         // Total starts at 0.
         total=cOriginVector;

         // Sum normals.
         for(long i=0; i<6; i++)
            total+=tangent[i].cross(tangent[i+1]);

         // Normalize it.
         total.normalize();

         // Convert color color.
         DWORD color=vectorToRGBA(total);

         // Set pixel.
         normalMap->setPixel(x, y, color);
      }
   }

   return(normalMap);
}


//==============================================================================
// swap
//==============================================================================
void swap(DWORD &val1, DWORD &val2)
{
   DWORD temp;
   temp=val1;
   val1=val2;
   val2=temp;
}


//==============================================================================
// BImage::flipHorizontal
//==============================================================================
void BImage::flipHorizontal(void)
{
   // Skip if no data.
   if(!mData)
      return;

   // Compute num to swap.
   long numToSwap=mWidth/2;

   // For each row.
   for(uint y=0; y<mHeight; y++)
   {
      long srcX=mWidth-1;
      for(long dstX=0; dstX<numToSwap; dstX++, srcX--)
      {
         swap(mData[y*mWidth+dstX], mData[y*mWidth+srcX]);
      }
   }
}


//==============================================================================
// BImage::flipVertical
//==============================================================================
void BImage::flipVertical(void)
{
   // Skip if no data.
   if(!mData)
      return;

   // Compute num to swap.
   long numToSwap=mHeight/2;

   // For each column.
   for(uint x=0; x<mWidth; x++)
   {
      long srcY=mHeight-1;
      for(long dstY=0; dstY<numToSwap; dstY++, srcY--)
      {
         swap(mData[dstY*mWidth+x], mData[srcY*mWidth+x]);
      }
   }
}




//==============================================================================
// BImage::readTGA
//==============================================================================
bool BImage::loadTGA(long dirID, const BCHAR_T *filename)
{
   // Valid op?
   if (filename == NULL)
   {
      BASSERT(0);
      return (false);
   }

   // Can't do this on wrapped buffer since it could involve resizing.  TBD: allow this if size if exactly the same?
   if(mWrapped)
   {
      BFAIL("loadTGA not allowed on wrapped buffer.");
      return(false);
   }

   // reset our contents

   deInit();  

   // Load the Targa

   BImage* tempImage = ::readTGA(dirID, filename);

   if (tempImage == NULL)
   {
      BASSERT(0);
      return(false);
   }

   // Now, assume the identity of the allocated image
   // And save the copying of the image data

   BASSERT(tempImage->getValid());

   mHeight     = tempImage->getHeight();
   mWidth      = tempImage->getWidth();
   mData       = tempImage->getRawData();
   mValidImage = true;

   destroyButNotReleaseData(tempImage);

   return(true);

}


//==============================================================================
// BImage::writeTGA
//==============================================================================
bool BImage::saveTGA(long dirID, const BCHAR_T *filename)
{
   // Valid op?
   dirID;

   if (!mValidImage || filename == NULL)
   {
      BASSERT(0);
      return (false);
   }

   return(writeTGA(dirID, filename, this));

}


//==============================================================================
// BImage::saveAlphaToBMP
//==============================================================================
bool BImage::saveAlphaToBMP(long dirID, const BCHAR_T *filename)
{

   // Valid op?

   if (!mValidImage || filename == NULL)
   {
      BASSERT(0);
      return (false);
   }

   // Create Temp 8-bit image

   BImage8 *tempImage = new BImage8();
   if (tempImage == NULL)
   {
      BASSERT(0);
      return (false);
   }

   tempImage->copyAlphaChannel(this);

   bool success = tempImage->saveBMP(dirID, filename);

   delete tempImage;

   return(success);
  
}

//==============================================================================
// BImage::loadAlphaFromBMP
//==============================================================================
bool BImage::loadAlphaFromBMP(long dirID, const BCHAR_T *filename)
{

   // Valid op?

   if (!mValidImage || filename == NULL)
   {
      BASSERT(0);
      return (false);
   }

   // Can't do this on wrapped buffer since it could involve resizing.  TBD: allow this if size if exactly the same?
   if(mWrapped)
   {
      BFAIL("loadAlphaFromBMP not allowed on wrapped buffer.");
      return(false);
   }


   // Create Temp 8-bit image
   BImage8 *tempImage = new BImage8();
   if (tempImage == NULL)
   {
      BASSERT(0);
      return (false);
   }

   // Load up Alpha channel Info

   if (!tempImage->loadBMP(dirID, filename))
   {
      BASSERT(0);
      return (false);
   }

   // Verify sizes match

   if (tempImage->getValid() == false || tempImage->getWidth() != mWidth || tempImage->getHeight() != mHeight)
   {
      #ifdef _BANG
      {setBlogError(4054); blogerror("error - BImage::loadAlphaFromBMP('%s') - Alpha Surface does not match image size", BStrConv::toA(filename));}
      #endif

      BASSERT(0);
      return (false);
   }
   
   // Copy over Alpha Channel
   BYTE alphaValue = 0;
   BYTE a,r,g,b;
   for (uint x = 0; x < mWidth; x++)
      for (uint y = 0; y < mHeight; y++)
      {
         alphaValue = tempImage->getPixel(x,y);
         getPixel(x,y,a,r,g,b);
         setPixel(x,y,alphaValue, r, g, b);
      }
   // cleanup

   delete tempImage;

   return(true);

}


//==============================================================================
// BImage::saveBMP
//==============================================================================
bool BImage::saveBMP(long dirID, const BCHAR_T *filename, bool saveAlphaChannel)
{
#ifdef XBOX
   // rg [6/10/05] - FIXME, probably use D3DX
   return (false);
#else   
   if (filename == NULL)
   {
      BASSERT(0);
      return(false);
   }

   BFile file;
   bool ok=file.openWriteable(dirID, filename);
   if(!ok)
      return(false);

   BITMAPFILEHEADER   bmFile;
   BITMAPINFOHEADER   bmInfo;

   long headerSize = sizeof(bmFile) + sizeof(bmInfo);

   bmFile.bfType = 'MB';
   bmFile.bfReserved1 = 0;
   bmFile.bfReserved2 = 0;
   bmFile.bfOffBits = headerSize;

   bmInfo.biSize = sizeof(bmInfo);
   bmInfo.biWidth = mWidth;
   bmInfo.biHeight = mHeight;                      // Height is Positive, this is a "Bottom Up" BMP
   bmInfo.biPlanes = 1;

   bmInfo.biCompression = BI_RGB;                  // Uncompressed Image 

   if (saveAlphaChannel)
   {
      bmInfo.biBitCount = 32;                          //  RGBA image
      bmInfo.biSizeImage = mWidth * mHeight * 4;
   }
   else
   {
      bmInfo.biBitCount = 24;                          // RGB image
      bmInfo.biSizeImage = mWidth * mHeight * 3;
   }
   bmInfo.biXPelsPerMeter = 0;
   bmInfo.biYPelsPerMeter = 0;
   bmInfo.biClrUsed = 0;                           
   bmInfo.biClrImportant = 0;

   long fileSize = headerSize + bmInfo.biSizeImage ;
   bmFile.bfSize = fileSize;


   file.write(&bmFile, sizeof(bmFile));
   file.write(&bmInfo, sizeof(bmInfo));

   if (saveAlphaChannel)
   {
      for (int y = mHeight-1; y>= 0; y--)
      {
         file.write(&mData[y*mWidth], mWidth*4);
      }
   }
   else
   {
      for (int y = mHeight-1; y>= 0; y--)
      {
         for (uint x = 0; x < mWidth; x++)
         {
            file.write(&mData[y*mWidth+x], 3);
         }
      }
   }



   return(true);
#endif
}


//==============================================================================
// BImage::loadBMP
//==============================================================================
bool BImage::loadBMP(long dirID, const BCHAR_T *filename)
{
#ifdef XBOX
   // rg [6/10/05] - FIXME, probably use D3DX
   return (false);
#else
   bool  success = false;
   bool  topDown = false;
   uint   y;                      // These are up front because of the goto's (complier no like!)
   long  width, height;
   DWORD headerSize, fileSize;

   BITMAPFILEHEADER   bmFile;
   BITMAPINFOHEADER   bmInfo;

   // Empty current image data

   deInit();

   // open the file

   BFile theFile;
   bool ok = theFile.openReadOnly(dirID, filename);

   // bail out if the open failed

   if(!ok)
   {
      #ifdef BANG
      {setBlogError(4055); blogerror("error - BImage::readBMP: unable to open file for read '%s'", BStrConv::toA(filename));}
      #endif

      goto error_out;
   }

   // Read Headers


   if (!theFile.read(&bmFile, sizeof(bmFile)))
      goto error_out;

   if (!theFile.read(&bmInfo, sizeof(bmInfo)))
      goto error_out;


   // validate Header info
   DWORD size;
   theFile.getSize(size);

   if (bmFile.bfType != 'MB' || bmInfo.biSize != sizeof(bmInfo) || bmInfo.biPlanes != 1 || 
       bmFile.bfSize != size || bmFile.bfReserved1 != 0 || bmFile.bfReserved2 != 0)
   {
      #ifdef BANG
      {setBlogError(4056); blogerror("error - BMP File header corrupt");}
      #endif

      goto error_out;
   }

   // Valid Image type?

   if (!(bmInfo.biBitCount == 24 || bmInfo.biBitCount == 32 || bmInfo.biBitCount == 8))
   {
      #ifdef BANG
      {setBlogError(4057); blogerror("error - BMP File is not supported format");}
      #endif

      goto error_out;
   }


   if (bmInfo.biCompression != BI_RGB)
   {
      #ifdef BANG
      {setBlogError(4058); blogerror("error - BMP File is compressed (not suported)");}
      #endif

      goto error_out;
   }


   // ======================================================================
   // Special case - load an 8-bit image
   // Just load a Bimage 8 and copy
   // ======================================================================

   if (bmInfo.biBitCount == 8)
   {
      #ifdef BANG
      setBlogWarning(29);
      blogwarning("warning - loading BImage from ('%s') requires 8 to 24 bit conversion", filename);
      #endif

      BImage8 *tempImage = new BImage8();
      if (tempImage == NULL)
      {
         BASSERT(0);
         goto error_out;
      }

      success = tempImage->loadBMP(dirID, filename);
      if (success)
      {
         copy(tempImage);
      }

      delete tempImage;
      goto error_out;
   }

   headerSize = sizeof(bmFile) + sizeof(bmInfo);

   if (bmFile.bfOffBits != headerSize)
   {
      #ifdef BANG
      {setBlogError(4059); blogerror("error - BMP File in bmFile.bfOffBits");}
      #endif

      goto error_out;
   }
   
   width = bmInfo.biWidth;
   height = bmInfo.biHeight;
   
   if (height < 0)
   {
      height = -height;
      topDown = true;
   }

   fileSize = headerSize + (width * height * 1);

   if (width <= 0 || height <= 0 || (long) bmInfo.biSize !=  (width*height) || fileSize != size)
   {
      #ifdef BANG
      {setBlogError(4060); blogerror("error - BMP File image dimension error");}
      #endif

      goto error_out;
   }

   // Everything checks out, lets allocate the image

   allocateData(width, height);

   // Now read in the image data (line @ a time)

   if (bmInfo.biBitCount == 32)           // 32-bit (24+Alpha) .BMP
   {
      if (topDown)
      {
         for (y = 0; y < mHeight; y++)
         {
            theFile.read(&mData[y*mWidth], mWidth * 4);
         }
      }
      else     // Bottom Up .BMP
      {
         for (y = mHeight-1; y>= 0; y--)
         {
            theFile.read(&mData[y*mWidth], mWidth * 4);
         }
      }
   }
   else if (bmInfo.biBitCount == 24)         // 24-bit .BMP (no Alpha)
   {
      if (topDown)
      {
         for (y = 0; y < mHeight; y++)
         {
            for (uint x = 0; x < mWidth; x++)
            {
               theFile.read(&mData[y*mWidth + x], 3);
            }
         }
      }
      else     // Bottom up 24-bit .BMP
      {
         for (y = mHeight-1; y>= 0; y--)
         {
            for (uint x = 0; x < mWidth; x++)
            {
               theFile.read(&mData[y*mWidth + x], 3);
            }
         }
      }
   }
   else  // Unsupported bit-depth
   {
      BASSERT(0);
      goto error_out;
   }

   // We did it!  We loaded a BMP!

   success = true;


error_out:

   if (!success)
   {
      #ifdef BANG
      {setBlogError(4061); blogerror("error - BImage8::readBMP('%s') - I/O during image load", BStrConv::toA(filename));}
      #endif
   }

   return(success);
#endif
}


//==============================================================================
// BImage::writeJPG
//==============================================================================
bool BImage::writeJPG(long dirID, const BCHAR_T *filename, long quality)
{
#ifdef XBOX
   // rg [6/10/05] - FIXME, probably use D3DX
   return(false);
#else   
   // Bail if not initialized, or bad filename
   if(!mValidImage || filename == NULL)
   {
      BASSERT(0);
      return(false);
   }

   // Fix up quality param if it's out of range.
   if(quality<0)
   {
      quality=0;
   }
   else if(quality>100)
   {
      quality=100;
   }

   // Open the file.
   BFile file;
   bool ok=file.openWriteable(dirID, filename);
   if(!ok)
      return(false);

   // Temp array to convert scanlines into.
   JSAMPLE *tempLine=new JSAMPLE[mWidth*3];
   if(!tempLine)
   {
      BASSERT(0);
      return(false);
   }
   JSAMPROW tempRow[1] = {tempLine};

   // Set up.
   struct jpeg_compress_struct jpegInfo;
	struct jpeg_error_mgr jpegErr;

   jpegInfo.err = jpeg_std_error(&jpegErr);
   jpegInfo.err->trace_level=-1;
   jpeg_create_compress(&jpegInfo);
   
   jpeg_stdio_dest(&jpegInfo, file.getHandle());

   // Set up input params.
   jpegInfo.image_width      = mWidth;
   jpegInfo.image_height     = mHeight;
   jpegInfo.input_components = 3;
   jpegInfo.in_color_space   = JCS_RGB;

   // Set default compression params.
   jpeg_set_defaults(&jpegInfo);
   jpeg_set_quality(&jpegInfo, quality, false);

   // Start 'er up.
   jpeg_start_compress(&jpegInfo, true);

   // Rip out scanlines.
   for(uint y=0; y<mHeight; y++)
   {
      JSAMPLE *curr=tempLine;
      for(uint x=0; x<mWidth; x++)
      {
         DWORD pixel=getPixel(x, y);
         *curr=(BYTE)IMG_GETR(pixel);
         curr++;
         *curr=(BYTE)IMG_GETG(pixel);
         curr++;
         *curr=(BYTE)IMG_GETB(pixel);
         curr++;
      }

      // Write out line.
      jpeg_write_scanlines(&jpegInfo, tempRow, 1);
   }

   // Clean up.
   jpeg_finish_compress(&jpegInfo);
   jpeg_destroy_compress(&jpegInfo);
   delete []tempLine;

   return(true);
#endif   
}

//==============================================================================
// BImage::copyData
//==============================================================================
bool BImage::copyData(BImage *srcImage, long sourceX, long sourceY, long destX, long destY, long xPixels, long yPixels)
{
   // Make sure both are initialized.
   if(!getValid() || !srcImage->getValid())
   {
      BASSERT(0);
      return(false);
   }

   // Set up.

   long lastSourceX  = sourceX + xPixels-1;
   long lastSourceY  = sourceY + yPixels-1;
   long lastDestX    = destX   + xPixels-1;
   long lastDestY    = destY   + yPixels-1;

   // Check params.

   if(xPixels < 0 || yPixels < 0)
   {
      return(false);
   }
   if(sourceX < 0 || lastSourceX >= (long)srcImage->getWidth() || sourceY < 0 || lastSourceY >= (long)srcImage->getHeight())
   {
      return(false);
   }
   if(destX < 0 || lastDestX >= (long)getWidth() || destY < 0 || lastDestY >= (long)getHeight())
   {
      return(false);
   }

   // Copy.       
   for( ; sourceY <= lastSourceY; sourceY++, destY++)
   {
      memcpy(getRawData()+destX+getWidth()*destY, srcImage->getRawData()+sourceX+srcImage->getWidth()*sourceY, xPixels*sizeof(DWORD));
   }

   // Success.
   return(true);
}


//==============================================================================
// getPixelWithWrap
// c helper
//==============================================================================
DWORD getPixelWithWrap(DWORD *data, long x, long y, long width, long height)
{
   // Wrap x.
   while(x<0 || x>=width)
   {
      if(x<0)
         x+=width;
      if(x>=width)
         x-=width;
   }
   
   // Wrap y.
   while(y<0 || y>=height)
   {
      if(y<0)
         y+=height;
      if(y>=height)
         y-=height;
   }
   
   // Get pixel
   return(data[x + (y * width)]);
}

//==============================================================================
// getPixelWithClamp
// c helper
//==============================================================================
DWORD getPixelWithClamp(DWORD *data, long x, long y, long width, long height)
{
   if(x<0)
      x=0;
   if(x>=width)
      x=width-1;
   if(y<0)
      y=0;
   if(y>=height)
      y=height-1;
   
   // Get pixel
   return(data[x + (y * width)]);
}

//==============================================================================
// BImage::sharpen
//==============================================================================
void BImage::sharpen(void)
{
   // Make sure we have some real data.
   if(!mValidImage)
   {
      BASSERT(0);
      return;
   }
      
   // Allocate space for temp data.
   long numPixels=mWidth*mHeight;
   DWORD *temp=new DWORD[numPixels];
   if(!temp)
   {
      BASSERT(0);
      return;
   }

   // Copy the original image data.      
   memcpy(temp, mData, numPixels*sizeof(DWORD));
   
   // Our convolution kernal.
   static const float filter[3][3] = {-1.0f, -2.0f, -1.0f,
                                      -2.0f, 28.0f, -2.0f,
                                      -1.0f, -2.0f, -1.0f};
   static const float recipWeight=1.0f/16.0f;

   // Run through each pixel.      
   for(uint y=0; y<mHeight; y++)
   {
      for(uint x=0; x<mWidth; x++)
      {
         float a=0.0f;
         float r=0.0f;
         float g=0.0f;
         float b=0.0f;
         
         // Check +/- one pixel in each direction and apply filter weights.
         for(long dy=-1; dy<2; dy++)
         {
            for(long dx=-1; dx<2; dx++)
            {
               //DWORD pixel=getPixelWithWrap(temp, x+dx, y+dy, mWidth, mHeight);
               DWORD pixel=getPixelWithClamp(temp, x+dx, y+dy, mWidth, mHeight);
               a+=filter[dy+1][dx+1]*IMG_GETA(pixel);
               r+=filter[dy+1][dx+1]*IMG_GETR(pixel);
               g+=filter[dy+1][dx+1]*IMG_GETG(pixel);
               b+=filter[dy+1][dx+1]*IMG_GETB(pixel);
            }
         }
         
         // Assemble output pixel.
         DWORD outPixel=0;
         IMG_SETA(outPixel, min(max(0, long(recipWeight*a+0.5f)), 255));
         IMG_SETR(outPixel, min(max(0, long(recipWeight*r+0.5f)), 255));
         IMG_SETG(outPixel, min(max(0, long(recipWeight*g+0.5f)), 255));
         IMG_SETB(outPixel, min(max(0, long(recipWeight*b+0.5f)), 255));
         
         // Poke it back into the image.  This is cool to update as we go since we've
         // saved the original data in temp.
         setPixel(x, y, outPixel);
      }
   }
   
   delete []temp;
}




//==============================================================================
// BImage::buildPalette
//==============================================================================
bool BImage::buildPalette(BPalette &palette, long numColors) const
{
   // Create RGB Histogram from our image
   BRGBHistogram* histogram = new BRGBHistogram(6);
   if (!histogram)
   {
      BFAIL("Could not allocate rgb histogram.");
      return(false);
   }

   histogram->generateFromImage(this);

   // Now we get to work.....
   long maxColors = numColors;
   palette.setSize(0);

   mHeap = NULL;
   mHeap_size = 0;

   mBoxes = NULL;
   mNum_boxes = 0;

   mHistogramData = histogram->mHistData;

   if (initQuantization(maxColors))
   {
      BQuantizeBox *p;
      while (mNum_boxes < maxColors)
      {
         if ((p = getWorstQuantizeBox()) == NULL)
         {
            break;
         }

         splitQuantizeBox(p);
      }

      fillOutPalette(&palette);
   }

   // Delete quantization processing working data

   delete histogram;

   delete[] mHeap;
   delete[] mBoxes;

   mHeap = NULL;
   mHeap_size = 0;

   mBoxes = NULL;
   mNum_boxes = 0;

   return(true);
}


//==============================================================================
// BImage::quantizeImage
//  creates an index (8-bit) image from the TGA, reduced to the 
//  specified number of colors
//==============================================================================
BImage8*  BImage::quantizeImage(long numColors) const
{
   // Validate everything

   if (!mValidImage || numColors < 2 || numColors > 256)
   {
      BASSERT(0);
      return(NULL);
   }

   BPalette *palette=new BPalette;
   if(!palette)
   {
      BFAIL("Failed to allocate palette.");
      return(NULL);
   }
   buildPalette(*palette);

   // Copy to 8-bit image

   // How many colors did we actually find?

   long actualColors = palette->getSize();

   BImage8*  indexedImage = NULL;

   if (actualColors > 0)
   {
      indexedImage = new BImage8();

      // Size the indexed image

      indexedImage->init(mWidth, mHeight);

      // Assign the palette

      *(indexedImage->getPalette()) = *palette;

      // Create Inverse Color Map for the palette

      //BInverseColorMap *ICM = new BInverseColorMap(5);         // 6-bit precision ICM
      //ICM->generateICMforPalette(*palette);

      // Now copy the pixels

      // BYTE  *ICMData = ICM->getColorTable();
      DWORD *src     = mData;
      BYTE  *dst     = indexedImage->getData();

      for (uint i = 0; i < mWidth * mHeight; i++)
      {
         DWORD    rgbPix = src[i];
         // long     idx = ((IMG_GETR(rgbPix) >> 2) << 12) | ((IMG_GETG(rgbPix) >> 2) << 6) | (IMG_GETB(rgbPix) >> 2);
         //dst[i] = (BYTE) ICM->getIndexedColor(IMG_GETR(rgbPix), IMG_GETG(rgbPix), IMG_GETB(rgbPix));

         BYTE red = (BYTE)IMG_GETR(rgbPix);
         BYTE green = (BYTE)IMG_GETG(rgbPix);
         BYTE blue  = (BYTE)IMG_GETB(rgbPix);
         BYTE color;

         if (indexedImage->getPalette()->findNearestColor(color, red, green, blue))
         {
            dst[i] = (BYTE)  color;
            BASSERT(color < indexedImage->getPalette()->getSize());
         }
         else
         {
            BASSERT(0);
         }

         //dst[i] = ICMData[idx];
      }

      //delete ICM;
   }

   // Clean up
   
   delete palette;   

   return (indexedImage);

}


//==============================================================================
// BImage::initQuantization
// Create initial box, initialize heap
//==============================================================================
bool BImage::initQuantization(long numColors) const
{
   DWORD  total_weight;
   DWORD  tt_sum, t_ur, t_ug, t_ub;
   long   ir, ig, ib, jr, jg, jb;

   mHeap = new BQuantizeBox*[numColors+1];
   if (mHeap == NULL)
   {
      BASSERT(0);
      return(false);
   }

   mBoxes = new BQuantizeBox[numColors];
   if (mBoxes == NULL)
   {
      BASSERT(0);
      return(false);
   }

   // shrink initial box to minimum possible size
   shrinkQuantizeBox(0, 0, 0, HIST_MAX - 1, HIST_MAX - 1, HIST_MAX - 1, &ir, &ig, &ib, &jr, &jg, &jb);

   // calculate the initial box's statistics */
   sumQuantizeData(ir, ig, ib, jr, jg, jb, &total_weight, &tt_sum, &t_ur, &t_ug, &t_ub);

   mBoxes->total_weight = total_weight;
   mBoxes->variance     = 1;
   mBoxes->tt_sum       = tt_sum;
   mBoxes->t_ur         = t_ur;
   mBoxes->t_ug         = t_ug;
   mBoxes->t_ub         = t_ub;
   mBoxes->ir           = ir;
   mBoxes->ig           = ig;
   mBoxes->ib           = ib;
   mBoxes->jr           = jr;
   mBoxes->jg           = jg;
   mBoxes->jb           = jb;

   // enter box into heap if it's splittable

   mNum_boxes           = 1;
   mHeap_size           = 0;

   if ((jr - ir) + (jg - ig) + (jb - ib))
   {
      mHeap[1] = mBoxes;
      mHeap_size = 1;
   }

   return(true);
}



//==============================================================================
// BImage::shrinkQuantizeBox
// Shrinks box to minimum possible size
//==============================================================================
void BImage::shrinkQuantizeBox(long ir, long ig, long ib, long jr, long jg, long jb,
                       long *lr, long *lg, long *lb, long *hr, long *hg, long *hb) const
{
   long   r, g, b;
   DWORD  *rp, *gp, *bp;
   DWORD  *s;

   s = mHistogramData + (ir * R_STRIDE + ig * G_STRIDE + ib);

   rp = s;

   for (r = ir; r <= jr; r++)
   {
      gp = rp;
      for (g = ig; g <= jg; g++)
      {
         bp = gp;
         for (b = ib; b <= jb; b++)
         {
            if (*bp++) 
            {
               *lr = r;
               goto lr_done;
            }
         }
         gp += G_STRIDE;
      }
      rp += R_STRIDE;
   }

lr_done:

   gp = s;

   for (g = ig; g <= jg; g++)
   {
      rp = gp;
      for (r = ir; r <= jr; r++)
      {
         bp = rp;
         for (b = ib; b <= jb; b++)
         {
            if (*bp++)
            {
               *lg = g;
               goto lg_done; 
            }
         }
         rp += R_STRIDE;
      }
      gp += G_STRIDE;
   }

lg_done:

   bp = s;

   for (b = ib; b <= jb; b++)
   {
      rp = bp;
      for (r = ir; r <= jr; r++)
      {
         gp = rp;
         for (g = ig; g <= jg; g++, gp += G_STRIDE)
         {
            if (*gp)
            {
               *lb = b;
               goto lb_done;
            }
         }
         rp += R_STRIDE;
      }
      bp++;
   }

lb_done:

   s = mHistogramData + (jr * R_STRIDE + jg * G_STRIDE + jb);

   rp = s;

   for (r = jr; r >= ir; r--)
   {
      gp = rp;
      for (g = jg; g >= ig; g--)
      {
         bp = gp;
         for (b = jb; b >= ib; b--)
         {
            if (*bp--) 
            {
               *hr = r;
               goto hr_done;
            }
         }
         gp -= G_STRIDE;
      }
      rp -= R_STRIDE;
   }

hr_done:

   gp = s;

   for (g = jg; g >= ig; g--)
   {
      rp = gp;
      for (r = jr; r >= ir; r--)
      {
         bp = rp;
         for (b = jb; b >= ib; b--)
         {
            if (*bp--) 
            {
               *hg = g;
               goto hg_done;
            }
         }
         rp -= R_STRIDE;
      }
      gp -= G_STRIDE;
   }

hg_done:

   bp = s;

   for (b = jb; b >= ib; b--)
   {
      gp = bp;
      for (g = jg; g >= ig; g--)
      {
         rp = gp;
         for (r = jr; r >= ir; r--, rp -= R_STRIDE)
         {
            if (*rp)
            { 
               *hb = b; 
               goto hb_done;
            }
         }
         gp -= G_STRIDE;
      }
      bp--;
   }

hb_done:

  return;
}


//==============================================================================
// BImage::sumQuantizeData
// Calculate statistics over the specified box. This is an implementation of
// the "brute force" method of gathering statistics described earlier
//==============================================================================
void BImage::sumQuantizeData(long ir, long ig, long ib, long jr, long jg, long jb,
                DWORD *total_weight, DWORD *tt_sum, DWORD *t_ur, DWORD *t_ug, DWORD *t_ub) const
{
   long     i, j, r, g, b;
   DWORD    rs, ts;
   DWORD    w, tr, tg, tb;
   DWORD    *rp, *gp, *bp;

   j = 0;

   tr = tg = tb = i = 0;

   rp = mHistogramData + ((ir * R_STRIDE) + (ig * G_STRIDE) + ib);

   for (r = ir; r <= jr; r++)
   {
      rs = r * r;
      gp = rp;

      for (g = ig; g <= jg; g++)
      {
         ts = rs + (g * g);
         bp = gp;

         for (b = ib; b <= jb; b++)
         {
            if (*bp++)                 // was this cell used at all?
            {
               w   = *(bp - 1);        // update statistics
               j  += w;
               tr += r * w;
               tg += g * w;
               tb += b * w;
               i  += (ts + b * b) * w;
            }
         }
         gp += G_STRIDE;
      }
      rp += R_STRIDE;
   }

   *total_weight = j;
   *tt_sum       = i;
   *t_ur         = tr;
   *t_ug         = tg;
   *t_ub         = tb;
}


//==============================================================================
// BImage::getWorstQuantizeBox
// Returns "worst" box, or NULL if no more splittable boxes remain. The worst 
// box is the box with the largest variance.
//==============================================================================
BQuantizeBox* BImage::getWorstQuantizeBox(void) const
{
   if (mHeap_size == 0)
   {
      return(NULL);
   }
   else
   {
      return(mHeap[1]);
   }
}



//==============================================================================
// BImage::splitQuantizeBox
//
// Splits box along the axis which will minimize the two new box's overall
// variance. A search on each axis is used to locate the optimum split point.
//==============================================================================
void BImage::splitQuantizeBox(BQuantizeBox *old_box) const
{
   long     i;
   BQuantizeBox *new_box;

   DWORD    total_weight;
   DWORD    tt_sum, t_ur, t_ug, t_ub;
   long     ir, ig, ib, jr, jg, jb;
            
   DWORD    total_weight1;
   DWORD    tt_sum1, t_ur1, t_ug1, t_ub1;
   long     ir1, ig1, ib1, jr1, jg1, jb1;
            
   DWORD    total_weight2;
   DWORD    tt_sum2, t_ur2, t_ug2, t_ub2;
   long     ir2, ig2, ib2, jr2, jg2, jb2;
            
   DWORD    total_weight3;
   DWORD    tt_sum3, t_ur3, t_ug3, t_ub3;
            
   DWORD    lowest_variance, variance_r, variance_g, variance_b;
   long     pick_r = -1, pick_g = -1, pick_b =-1;
         
   // Create new Box at end of list

   new_box = mBoxes + mNum_boxes;
   mNum_boxes++;

   // setup vars 

   total_weight          = old_box->total_weight;
   tt_sum                = old_box->tt_sum;
   t_ur                  = old_box->t_ur;
   t_ug                  = old_box->t_ug;
   t_ub                  = old_box->t_ub;
   ir                    = old_box->ir;
   ig                    = old_box->ig;
   ib                    = old_box->ib;
   jr                    = old_box->jr;
   jg                    = old_box->jg;
   jb                    = old_box->jb;

   // left box's initial statistics

   total_weight1         = 0;
   tt_sum1               = 0;
   t_ur1                 = 0;
   t_ug1                 = 0;
   t_ub1                 = 0;

   // right box's initial statistics

   total_weight2         = total_weight;
   tt_sum2               = tt_sum;
   t_ur2                 = t_ur;
   t_ug2                 = t_ug;
   t_ub2                 = t_ub;

  // Note: One useful optimization has been purposefully omitted from the
  // following loops. The getVariance function is always called twice per
  // iteration to calculate the new total variance. This is a waste of time
  // in the possibly common case when the new split point did not shift any
  // new points from one box into the other. A simple test can be added to
  // remove this inefficiency.

  // locate optimum split point on red axis */

   variance_r = 0xFFFFFFFF;

   for (i = ir; i < jr; i++)
   {
      DWORD total_variance;

      // calculate the statistics for the area being taken
      // away from the right box and given to the left box

      sumQuantizeData(i, ig, ib, i, jg, jb,   &total_weight3, &tt_sum3, &t_ur3, &t_ug3, &t_ub3);

#ifdef DEBUGGING_QUANT
      if (total_weight3 > total_weight)
         BASSERT(false);
#endif

      // update left and right box's statistics

      total_weight1 += total_weight3;
      tt_sum1       += tt_sum3;
      t_ur1         += t_ur3;
      t_ug1         += t_ug3;
      t_ub1         += t_ub3;

      total_weight2 -= total_weight3;
      tt_sum2       -= tt_sum3;
      t_ur2         -= t_ur3;
      t_ug2         -= t_ug3;
      t_ub2         -= t_ub3;

#ifdef DEBUGGING_QUANT
      if ((total_weight1 + total_weight2) != total_weight)
         BASSERT(false);
#endif

      // calculate left and right box's overall variance

      total_variance = getVariance(total_weight1, tt_sum1, t_ur1, t_ug1, t_ub1) +
                       getVariance(total_weight2, tt_sum2, t_ur2, t_ug2, t_ub2);

      // found better split point? if so, remember it

      if (total_variance < variance_r)
      {
         variance_r = total_variance;
         pick_r = i;
      }
   }

   // left box's initial statistics

   total_weight1         = 0;
   tt_sum1               = 0;
   t_ur1                 = 0;
   t_ug1                 = 0;
   t_ub1                 = 0;

   // right box's initial statistics

   total_weight2         = total_weight;
   tt_sum2               = tt_sum;
   t_ur2                 = t_ur;
   t_ug2                 = t_ug;
   t_ub2                 = t_ub;

   // locate optimum split point on green axis */

   variance_g = 0xFFFFFFFF;

   for (i = ig; i < jg; i++)
   {
      DWORD total_variance;

      // calculate the statistics for the area being taken
      // away from the right box and given to the left box

      sumQuantizeData(ir, i, ib, jr, i, jb,   &total_weight3, &tt_sum3, &t_ur3, &t_ug3, &t_ub3);

#ifdef DEBUGGING_QUANT
      if (total_weight3 > total_weight)
         BASSERT(false);
#endif

      // update left and right box's statistics 

      total_weight1 += total_weight3;
      tt_sum1       += tt_sum3;
      t_ur1         += t_ur3;
      t_ug1         += t_ug3;
      t_ub1         += t_ub3;

      total_weight2 -= total_weight3;
      tt_sum2       -= tt_sum3;
      t_ur2         -= t_ur3;
      t_ug2         -= t_ug3;
      t_ub2         -= t_ub3;

#ifdef DEBUGGING_QUANT
      if ((total_weight1 + total_weight2) != total_weight)
         BASSERT(false)
#endif

      // calculate left and right box's overall variance */

      total_variance = getVariance(total_weight1, tt_sum1, t_ur1, t_ug1, t_ub1) +
                       getVariance(total_weight2, tt_sum2, t_ur2, t_ug2, t_ub2);

      // found better split point? if so, remember it */

      if (total_variance < variance_g)
      {
         variance_g = total_variance;
         pick_g = i;
      }
   }

   // left box's initial statistics

   total_weight1         = 0;
   tt_sum1               = 0;
   t_ur1                 = 0;
   t_ug1                 = 0;
   t_ub1                 = 0;

   // right box's initial statistics

   total_weight2         = total_weight;
   tt_sum2               = tt_sum;
   t_ur2                 = t_ur;
   t_ug2                 = t_ug;
   t_ub2                 = t_ub;

   variance_b = 0xFFFFFFFF;

   // locate optimum split point on blue axis

   for (i = ib; i < jb; i++)
   {
      DWORD total_variance;

      // calculate the statistics for the area being taken
      // away from the right box and given to the left box

      sumQuantizeData(ir, ig, i, jr, jg, i,  &total_weight3, &tt_sum3, &t_ur3, &t_ug3, &t_ub3);

#ifdef DEBUGGING_QUANT
      if (total_weight3 > total_weight)
         BASSERT(false);
#endif

      // update left and right box's statistics

      total_weight1 += total_weight3;
      tt_sum1       += tt_sum3;
      t_ur1         += t_ur3;
      t_ug1         += t_ug3;
      t_ub1         += t_ub3;

      total_weight2 -= total_weight3;
      tt_sum2       -= tt_sum3;
      t_ur2         -= t_ur3;
      t_ug2         -= t_ug3;
      t_ub2         -= t_ub3;

#ifdef DEBUGGING_QUANT
      if ((total_weight1 + total_weight2) != total_weight)
         BASSERT(false);
#endif

      // calculate left and right box's overall variance

      total_variance = getVariance(total_weight1, tt_sum1, t_ur1, t_ug1, t_ub1) +
                       getVariance(total_weight2, tt_sum2, t_ur2, t_ug2, t_ub2);

      // found better split point? if so, remember it

      if (total_variance < variance_b)
      {
         variance_b = total_variance;
         pick_b = i;
      }
   }
         
   // now find out which axis should be split

   lowest_variance = variance_r;
   i = 0;

   if (variance_g < lowest_variance)
   {
      lowest_variance = variance_g;
      i = 1;
   }

   if (variance_b < lowest_variance)
   {
      lowest_variance = variance_b;
      i = 2;
   }

   // split box on the selected axis

   ir1 = ir; ig1 = ig; ib1 = ib;
   jr2 = jr; jg2 = jg; jb2 = jb;

   switch (i)
   {
      case 0:
      {
         jr1 = pick_r + 0; jg1 = jg; jb1 = jb;
         ir2 = pick_r + 1; ig2 = ig; ib2 = ib;
         break;
      }
      case 1:
      {
         jr1 = jr; jg1 = pick_g + 0; jb1 = jb;
         ir2 = ir; ig2 = pick_g + 1; ib2 = ib;
         break;
      }
      case 2:
      {
         jr1 = jr; jg1 = jg; jb1 = pick_b + 0;
         ir2 = ir; ig2 = ig; ib2 = pick_b + 1;
         break;
      }
   }

   // shrink the new boxes to their minimum possible sizes

   shrinkQuantizeBox(ir1, ig1, ib1, jr1, jg1, jb1,    &ir1, &ig1, &ib1, &jr1, &jg1, &jb1);

   shrinkQuantizeBox(ir2, ig2, ib2, jr2, jg2, jb2,    &ir2, &ig2, &ib2, &jr2, &jg2, &jb2);

   // update statistics 

   sumQuantizeData(ir1, ig1, ib1, jr1, jg1, jb1,  &total_weight1, &tt_sum1, &t_ur1, &t_ug1, &t_ub1);

   total_weight2         = total_weight - total_weight1;
   tt_sum2               = tt_sum - tt_sum1;
   t_ur2                 = t_ur - t_ur1;
   t_ug2                 = t_ug - t_ug1;
   t_ub2                 = t_ub - t_ub1;

   // create the new boxes

   old_box->variance     = getVariance(total_weight1, tt_sum1, t_ur1, t_ug1, t_ub1);
   old_box->total_weight = total_weight1;
   old_box->tt_sum       = tt_sum1;
   old_box->t_ur         = t_ur1;
   old_box->t_ug         = t_ug1;
   old_box->t_ub         = t_ub1;
   old_box->ir           = ir1;
   old_box->ig           = ig1;
   old_box->ib           = ib1;
   old_box->jr           = jr1;
   old_box->jg           = jg1;
   old_box->jb           = jb1;

   new_box->variance     = getVariance(total_weight2, tt_sum2, t_ur2, t_ug2, t_ub2);
   new_box->total_weight = total_weight2;
   new_box->tt_sum       = tt_sum2;
   new_box->t_ur         = t_ur2;
   new_box->t_ug         = t_ug2;
   new_box->t_ub         = t_ub2;
   new_box->ir           = ir2;
   new_box->ig           = ig2;
   new_box->ib           = ib2;
   new_box->jr           = jr2;
   new_box->jg           = jg2;
   new_box->jb           = jb2;

  // enter all splittable boxes into the priory queue

   i = 0;
   if ((jr1 - ir1) + (jg1 - ig1) + (jb1 - ib1)) i = 2;
   if ((jr2 - ir2) + (jg2 - ig2) + (jb2 - ib2)) i++;

   switch (i)
   {
      case 0:
      {
         mHeap[1] = mHeap[mHeap_size];
         mHeap_size--;

         if (mHeap_size)
            downQuantHeap();

         break;   
      }

      case 1:
      {
         mHeap[1] = new_box;
         downQuantHeap();

         break;
      }

      case 2:
      {
         downQuantHeap();

         break;
      }

      case 3:
      {
         downQuantHeap();
         insertQuantHeap(new_box);

         break;
      }
   }
}



//==============================================================================
// BImage::getVariance
//==============================================================================
DWORD BImage::getVariance(DWORD tw, DWORD tt_sum, DWORD t_ur, DWORD t_ug, DWORD t_ub) const
{
   double temp;

   // the following calculations can be performed in fixed point
   // if needed - just be sure to preserve enough precision!

   temp  = (double)t_ur * (double)t_ur;
   temp += (double)t_ug * (double)t_ug;
   temp += (double)t_ub * (double)t_ub;
   temp /= (double)tw;

   return ((DWORD)((double)tt_sum - temp));
}




//==============================================================================
// BImage::downQuantHeap
// Standard binary tree based priorty queue manipulation function
//==============================================================================
void BImage::downQuantHeap(void) const
{
   DWORD i, j, q;
   BQuantizeBox *p;

   p = mHeap[1];
   q = p->variance;

   for (i = 1; ; )
   {
      if ((j = i << 1) > (DWORD) mHeap_size)
      {
         break;
      }

      if (j < (DWORD) mHeap_size)
      {
         if (mHeap[j]->variance < mHeap[j + 1]->variance)
         j++;
      }

      if (q >= mHeap[j]->variance)
      {
         break;
      }

      mHeap[i] = mHeap[j];

      i = j;
   }

   mHeap[i] = p;
}

//==============================================================================
// BImage::insertQuantHeap
// Standard binary tree based priorty queue manipulation function
//==============================================================================
void BImage::insertQuantHeap(BQuantizeBox *p) const
{
   DWORD i, j, q;

   q = p->variance;
   j = ++mHeap_size;

   for ( ; ; )
   {
      if ((i = j >> 1) == 0)
      {
         break;
      }

      if (mHeap[i]->variance >= q)
      {
         break;
      }

      mHeap[j] = mHeap[i];

      j = i;
   }

   mHeap[j] = p;
}



//==============================================================================
// BImage::fillOutPalette
//       Initializes a palette with the quantized data
//==============================================================================
void BImage::fillOutPalette(BPalette *thePallete) const
{
   int          i;
   BQuantizeBox *p;
   DWORD        total_weight;

   thePallete->setSize(mNum_boxes);

   p = mBoxes;

   for (i = 0; i < mNum_boxes; i++, p++)
   {
      total_weight = p->total_weight;

      long red   = (long) (((p->t_ur << HIST_SHIFT) + (total_weight >> 1)) / total_weight);
      long green = (long) (((p->t_ug << HIST_SHIFT) + (total_weight >> 1)) / total_weight);
      long blue  = (long) (((p->t_ub << HIST_SHIFT) + (total_weight >> 1)) / total_weight);

      // Interpolate colors back up to 8-bit color , copy upper bits into lower to remove fenceposting
      
      red = red | (red >> HIST_BIT);
      green = green | (green >> HIST_BIT);
      blue = blue | (blue >> HIST_BIT);

      // Record Color

      thePallete->setColor(i, red, green, blue, 0);
   }
}








//==============================================================================
//==============================================================================
//==============================================================================
//
//
//
//
//
//
//
//
//
//==============================================================================
//==============================================================================
//==============================================================================








//==============================================================================
// BImage8::BImage8
//==============================================================================
BImage8::BImage8( void ) :
   mValidImage(false),
   mData(0),
   mWidth(0),
   mHeight(0),
   mPalette(NULL)
{
   // Nothing to do 
}

//==============================================================================
// BImage8::~BImage8
//==============================================================================
BImage8::~BImage8( void )
{
   deInit(true);
}

//==============================================================================
// BImage8::init
//==============================================================================
bool BImage8::init(long width, long height, bool clear) 
{
   BASSERT (width > 0 && height > 0);

   deInit();   

   // Allocate Image Data

   mData = new BYTE[width*height];

   if (!mData)
   {
      BASSERT(0);
      return false;
   }

   if (clear)
   {
      memset(mData, 0, sizeof(BYTE) * width * height);
   }

   // Allocate Palette if one doesn't exist

   if (mPalette == NULL)
   {
      mPalette    = new BPalette();
   }
   
   mWidth      = width;
   mHeight     = height;
   mValidImage = true;

   return (true);
}

//==============================================================================
// BImage8::deInit
//==============================================================================
void BImage8::deInit(bool destroyAll)
{
   mValidImage = false;
   
   if (mData != NULL)
   {
      delete[] mData;
      mData = NULL;
   }
   
   if (mPalette != NULL)
   {
      if (destroyAll)
      {
         delete mPalette;
         mPalette = NULL;
      }
      else
      {
         mPalette->reset();
      }
   }

   mWidth  = 0;
   mHeight = 0;

}


//==============================================================================
// BImage8::operator= 
//==============================================================================
BImage8& BImage8::operator=(BImage8 &SourceImg)
{
   deInit();

   // Return uninitialized image if source is inavlid

   if (!SourceImg.getValid())
   {
      return(*this);
   }

   // Config the same

   init(SourceImg.getWidth(), SourceImg.getHeight());

   // Copy Image data
   
   memcpy(mData, SourceImg.getData(), sizeof(*mData) * mWidth * mHeight);

   // Copy Palette

   *mPalette = *(SourceImg.getPalette());

   return(*this);
}




//==============================================================================
// BImage8::operator= 
//==============================================================================
bool BImage8::copyAlphaChannel(BImage* SourceImg)
{

   deInit();

   // Return uninitialized image if source is inavlid

   if (!SourceImg->getValid())
   {
      return(false);
   }

   // Config the same

   init(SourceImg->getWidth(), SourceImg->getHeight());

   // GrayScale Palette

   mPalette->initGreyScale();

   // Copy Image data & extract Alpha Channel info

   DWORD *srcPix = SourceImg->getRawData();

   for (uint z = 0; z < mHeight * mWidth; z++)
   {
      mData[z] = (BYTE) IMG_GETA(srcPix[z]);
   }

   return(true);

}

//==============================================================================
// BImage8::saveBMP
//==============================================================================
bool BImage8::saveBMP(long dirID, const BCHAR_T *filename)
{
#ifdef XBOX
   // rg [6/10/05] - FIXME
   return (false);
#else   
   if (filename == NULL)
   {
      BASSERT(0);
      return(false);
   }

   BFile file;
   bool ok=file.openReadOnly(dirID, filename);
   if(!ok)
      return(false);

   BITMAPFILEHEADER   bmFile;
   BITMAPINFOHEADER   bmInfo;

   RGBQUAD palData;

   long headerSize = sizeof(bmFile) + sizeof(bmInfo) + 256* sizeof(BRGBQuad);
   long fileSize = headerSize + (mWidth * mHeight * 1);

   bmFile.bfType = 'MB';
   bmFile.bfSize = fileSize;
   bmFile.bfReserved1 = 0;
   bmFile.bfReserved2 = 0;
   bmFile.bfOffBits = headerSize;

   bmInfo.biSize = sizeof(bmInfo);
   bmInfo.biWidth = mWidth;
   bmInfo.biHeight = mHeight;
   bmInfo.biPlanes = 1;
   bmInfo.biBitCount = 8;                          // 8-bit image
   bmInfo.biCompression = BI_RGB;                  // Uncompressed Image 
   bmInfo.biSizeImage = mWidth * mHeight * 1;
   bmInfo.biXPelsPerMeter = 0;
   bmInfo.biYPelsPerMeter = 0;
   bmInfo.biClrUsed = 0;                           // All 256 colors used
   bmInfo.biClrImportant = 0;

   file.write(&bmFile, sizeof(bmFile));
   file.write(&bmInfo, sizeof(bmInfo));

   for (int z = 0; z < 256; z++)
   {
      palData.rgbBlue  = mPalette->getPaletteData()[z].Blue;
      palData.rgbGreen = mPalette->getPaletteData()[z].Green;
      palData.rgbRed   = mPalette->getPaletteData()[z].Red;
      palData.rgbReserved = 0;
      file.write(&palData, sizeof(palData));
   }

   //for (int y = mHeight-1; y>= 0; y--)
   for (uint y = 0; y < mHeight; y++)
   {
      file.write(mData+(y*mWidth), mWidth);
   }

   return(true);
#endif   
}




//==============================================================================
// BImage8::loadBMP
//==============================================================================
bool BImage8::loadBMP(long dirID, const BCHAR_T *filename)
{
#ifdef XBOX
   // rg [6/10/05] - FIXME, probably use D3DX
   return (false);
#else   
   bool  success = false;
   uint  y;                   // These are up front because of the goto's (complier no like!)
   long  z;
   long  width, height, numColors;
   DWORD headerSize, fileSize;

   BITMAPFILEHEADER   bmFile;
   BITMAPINFOHEADER   bmInfo;
   RGBQUAD            palData;

   // Empty current image data

   deInit();

   // open the file

   BFile theFile;
   bool ok = theFile.openReadOnly(dirID, filename);

   // bail out if the open failed

   if(!ok)
   {
#ifdef _BANG
      {setBlogError(4062); blogerror("error - BImage8::readBMP: unable to open file for read '%s'", BStrConv::toA(filename));}
#endif
      goto error_out;
   }

   // Read Headers


   if (!theFile.read(&bmFile, sizeof(bmFile)))
      goto error_out;

   if (!theFile.read(&bmInfo, sizeof(bmInfo)))
      goto error_out;


   // validate Header info
   DWORD fileLength;
   theFile.getSize(fileLength);

   if (bmFile.bfType != 'MB' || bmInfo.biSize != sizeof(bmInfo) || bmInfo.biPlanes != 1 || 
       bmFile.bfSize != fileLength || bmFile.bfReserved1 != 0 || bmFile.bfReserved2 != 0)
   {
#ifdef _BANG
      {setBlogError(4063); blogerror("error - BMP File header corrupt");}
#endif
      goto error_out;
   }

   // Valid Image type?

   if (bmInfo.biBitCount != 8)
   {
#ifdef _BANG
      {setBlogError(4064); blogerror("error - BMP File is not 8-bit indexed image");}
#endif
      goto error_out;
   }


   if (bmInfo.biCompression != BI_RGB)
   {
#ifdef _BANG
      {setBlogError(4065); blogerror("error - BMP File is compressed (not suported)");}
#endif
      goto error_out;
   }

   numColors = bmInfo.biClrUsed;
   if (numColors == 0)
   {
      numColors = 256;
   }

   headerSize = sizeof(bmFile) + sizeof(bmInfo) + numColors * sizeof(BRGBQuad);

   if (bmFile.bfOffBits != headerSize)
   {
#ifdef _BANG
      {setBlogError(4066); blogerror("error - BMP File in bmFile.bfOffBits");}
#endif
      goto error_out;
   }
   
   width = bmInfo.biWidth;
   height = bmInfo.biHeight;
   fileSize = headerSize + (width * height * 1);

   if (width <= 0 || height <= 0 || (long) bmInfo.biSize !=  (width*height) || fileSize != fileLength)
   {
#ifdef _BANG
      {setBlogError(4067); blogerror("error - BMP File image dimension error");}
#endif
      goto error_out;
   }

   // Everything checks out, lets allocate the image

   init(width, height);

   // let's read in the palette

   for (z = 0; z < numColors; z++)
   {
      theFile.read(&palData, sizeof(palData));
      mPalette->setColor(z, palData.rgbRed, palData.rgbGreen, palData.rgbBlue, true);
   }

   // Now read in the image data (line @ a time)

   for (y = 0; y < mHeight; y++)
   {
      theFile.read(mData+(y*mWidth), mWidth);
   }

   // We did it!  We loaded a BMP!

   success = true;


error_out:

   if (!success)
   {
#ifdef _BANG
      {setBlogError(4068); blogerror("error - BImage8::readBMP('%s') - I/O during image load");}
#endif
   }

   return(success);
#endif

}


//==============================================================================
// eof: image.cpp
//==============================================================================


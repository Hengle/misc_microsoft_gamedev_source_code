//==============================================================================
// image.h
//
// Copyright (c) 1999-2001 Ensemble Studios
//==============================================================================

// rg [2/10/06] - Please avoid using this class on Xbox. Internally, BImage is
// always little endian. 

#pragma once 

#ifndef _IMAGE_H_
#define _IMAGE_H_

#define ARGBToDWORD(a, r, g, b) DWORD((a<<24)|(r<<16)|(g<<8)|(b))
#define MCLAMP(color) { if (color & 0xFFFFFF00) color = 255; } 

#define DWORDToARGB(dw, a, r, g, b) \
{ \
   (a) = ((dw) & cAlphaMask) >> cAlphaShift; \
   (r) = ((dw) & cRedMask) >> cRedShift; \
   (g) = ((dw) & cGreenMask) >> cGreenShift; \
   (b) = (dw) & cBlueMask; \
}

// image data access & mutation
#define IMG_GETA(pixel) (((pixel) & 0xff000000) >> 24)
#define IMG_GETR(pixel) (((pixel) & 0x00ff0000) >> 16)
#define IMG_GETG(pixel) (((pixel) & 0x0000ff00) >> 8)
#define IMG_GETB(pixel) (((pixel) & 0x000000ff) )


// these rely on you to be in the range of 0x00 to 0xFF yourself!
#define IMG_SETA(pixel,val) ((pixel) = ((pixel) & 0x00FFFFFF) | ((val) << 24))
#define IMG_SETR(pixel,val) ((pixel) = ((pixel) & 0xFF00FFFF) | ((val) << 16))
#define IMG_SETG(pixel,val) ((pixel) = ((pixel) & 0xFFFF00FF) | ((val) << 8))
#define IMG_SETB(pixel,val) ((pixel) = ((pixel) & 0xFFFFFF00) | ((val)))

//==============================================================================
// Forward references
//==============================================================================

class BPalette;
class BImage8;

//==============================================================================
// BQuantizeBox - Structure to hold quantization working data set
//==============================================================================
class BQuantizeBox
{
   public:
      DWORD     variance;           // weighted variance 
      DWORD     total_weight;       // total weight 
      DWORD     tt_sum;             // tt_sum += r*r+g*g+b*b*weight over entire box 
      DWORD     t_ur;               // t_ur += r*weight over entire box 
      DWORD     t_ug;               // t_ug += g*weight over entire box 
      DWORD     t_ub;               // t_ub += b*weight over entire box 
      long      ir, ig, ib;          // upper and lower bounds 
      long      jr, jg, jb;
};






//==============================================================================
// BImage Class - This class provides support for handing RGBA images in
//                an easy to access (unpacked) format.
//
//==============================================================================
class BImage
{

//==============================================================================

   public:
      explicit                BImage( void );         // Mp 7/11/02 - made explicit
                             ~BImage( void );

      bool                    allocateData( long w, long h, bool bfill = true);
      void                    wrapBuffer(long width, long height, DWORD *data);
      void                    deInit(void);
                              
      static void             destroyButNotReleaseData(BImage *image);  // Used to hand off image contents without copying

      // Accessor Functions

#ifndef BUILD_FINAL
      DWORD                   getPixel( uint x, uint y) const           { 
                                                                           BDEBUG_ASSERT( (x < mWidth) && (y < mHeight) ); 
                                                                           DWORD color = 0L;
                                                                           if (mValidImage)
                                                                           {                                                                           
                                                                              const BYTE * const pSrc = reinterpret_cast<const BYTE *>(&(mData[x +(y*mWidth)]));
                                                                              color = ARGBToDWORD(pSrc[3], pSrc[2], pSrc[1], pSrc[0]);
                                                                           }
                                                                           return color;
                                                                        }

      void                   getPixel(uint x, uint y, BYTE& a, BYTE& r, BYTE& g, BYTE& b)
                                                                        {
                                                                           BDEBUG_ASSERT( (x < mWidth) && (y < mHeight) );
                                                                           if (mValidImage)
                                                                           {                                                                           
                                                                              const BYTE * const pSrc = reinterpret_cast<const BYTE *>(&(mData[x +(y*mWidth)]));
                                                                              a = pSrc[3];
                                                                              r = pSrc[2];
                                                                              g = pSrc[1];
                                                                              b = pSrc[0];
                                                                           }
                                                                        }
      void                    setPixel( uint x, uint y, DWORD pix)      {
                                                                          BDEBUG_ASSERT( (x < mWidth) && (y < mHeight) );
                                                                          if (mValidImage)
                                                                          { 
                                                                             BYTE* pDest = reinterpret_cast<BYTE*>(&(mData[x +(y*mWidth)]));                                                                          
                                                                             pDest[0] = static_cast<BYTE>((pix & 0xFF));
                                                                             pDest[1] = static_cast<BYTE>((pix >> 8)  & 0xFF);
                                                                             pDest[2] = static_cast<BYTE>((pix >> 16) & 0xFF);
                                                                             pDest[3] = static_cast<BYTE>((pix >> 24) & 0xFF);
                                                                          }                                                                          
                                                                        }
      void                    setPixel(uint x, uint y, BYTE a, BYTE r, BYTE g, BYTE b)
                                                                        {
                                                                           BDEBUG_ASSERT( (x < mWidth) && (y < mHeight) );
                                                                           if (mValidImage)
                                                                           {                                                                           
                                                                              DWORD pixel = ARGBToDWORD(a,r,g,b);
                                                                              setPixel(x,y,pixel);
                                                                           }
                                                                        }                                                                             
#else
      DWORD                  getPixel( uint x, uint y)                 {  
                                                                           const BYTE * const pSrc = reinterpret_cast<const BYTE *>(&(mData[x +(y*mWidth)]));
                                                                           DWORD color = ARGBToDWORD(pSrc[3], pSrc[2], pSrc[1], pSrc[0]);
                                                                           return color;
                                                                        }
      void                   getPixel(uint x, uint y, BYTE& a, BYTE& r, BYTE& g, BYTE& b)
                                                                        {  
                                                                           const BYTE * const pSrc = reinterpret_cast<const BYTE *>(&(mData[x +(y*mWidth)]));
                                                                           a = pSrc[3];
                                                                           r = pSrc[2];
                                                                           g = pSrc[1];
                                                                           b = pSrc[0];
                                                                        }
      void                    setPixel(uint x, uint y, DWORD pix)      { BYTE* pDest = reinterpret_cast<BYTE*>(&(mData[x +(y*mWidth)]));                                                                          
                                                                          pDest[0] = static_cast<BYTE>((pix & 0xFF));
                                                                          pDest[1] = static_cast<BYTE>((pix >> 8)  & 0xFF);
                                                                          pDest[2] = static_cast<BYTE>((pix >> 16) & 0xFF);
                                                                          pDest[3] = static_cast<BYTE>((pix >> 24) & 0xFF);
                                                                        }
      void                    setPixel(uint x, uint y, BYTE a, BYTE r, BYTE g, BYTE b)
                                                                        {
                                                                           DWORD pixel = ARGBToDWORD(a,r,g,b);
                                                                           setPixel(x,y,pixel);
                                                                        }      
#endif

#ifndef BUILD_FINAL
   void                       setPixelNoSwap( const uint x, const uint y, const DWORD pix)
                                                                        {
                                                                        #if defined(XBOX)
                                                                           BDEBUG_ASSERT( (x < mWidth) && (y < mHeight) );
                                                                           if (mValidImage)
                                                                              mData[x +(y*mWidth)] = pix;
                                                                        #else
                                                                           setPixel(x, y, pix);
                                                                        #endif
                                                                        }
   void                       setPixelNoSwap( DWORD &data, const DWORD pix)
                                                                        {
                                                                           if (mValidImage)
                                                                              data = pix;
                                                                        }
#else
   void                       setPixelNoSwap( const uint x, const uint y, const DWORD pix)
                                                                        {
                                                                        #if defined(XBOX)
                                                                           mData[x +(y*mWidth)] = pix;
                                                                        #else
                                                                           setPixel(x, y, pix);
                                                                        #endif
                                                                        }
   void                       setPixelNoSwap( DWORD &data, const DWORD pix)
                                                                        {
                                                                           data = pix;
                                                                        }
#endif

      void                    setPixelSafe(uint x, uint y, DWORD pix)      {if(x<mWidth && y<mHeight) setPixel(x,y, pix);}

      //-- Changed this function name b/c we need to be aware of endianess for all platforms
      //-- If you want to munge colorchannels use the accessors unless you are aware of the 
      //-- PC / Xenon endian differences.
      DWORD*                  getRawData( void ) const      { return (mData); }
      DWORD*                  getRawData( uint y ) const    { BDEBUG_ASSERT(y < mHeight); return (&mData[y*mWidth]);}
      uint                    getWidth( void ) const     { return (mWidth); }
      uint                    getHeight( void ) const    { return (mHeight); }
      bool                    getValid( void ) const     { return (mValidImage); }


      // Copy functions

      bool                    copy(BImage *image);
      bool                    copy(BImage8 *image);

      bool                    copyData(BImage *srcImage, long sourceX, long sourceY, long destX, long destY, long xPixels, long yPixels);

      // Image Processing

      BImage                  *resampleImage(float xFactor, float yFactor);            // Rescale Image w/ Axis independant scales
      bool                    fastDownSample(BImage *destImages, long levels) const;
      BImage                  *displacementToNormal(void);

      void                    flipHorizontal(void);
      void                    flipVertical(void);

      void                    sharpen(void);

      class BImage8*          quantizeImage(long numColors = 256) const;
      bool                    buildPalette(BPalette &palette, long numColors=256) const;

      // Image to File operatons

      bool                    loadTGA(long dirID, const BCHAR_T *filename);           // Load Image from .TGA file
      bool                    saveTGA(long dirID, const BCHAR_T *filename);           // Save Image to a .TGA file

      bool                    writeJPG(long dirID, const BCHAR_T *filename, long quality);   // Save Image to a .JPG file

      bool                    saveAlphaToBMP(long dirID, const BCHAR_T *filename);
      bool                    loadAlphaFromBMP(long dirID, const BCHAR_T *filename);

      bool                    saveBMP(long dirID, const BCHAR_T *filename, bool saveAlphaChannel = false);
      bool                    loadBMP(long dirID, const BCHAR_T *filename);
      

   protected:
      bool                    resampleImage(BImage *dst, float xFactor, float yFactor);

      bool                    mValidImage;         // Indicates the image contents are valid
      bool                    mWrapped;            // Indicates we're wrapping data we don't own.

      DWORD                  *mData;               // Actual Image data of DWORD pixels
      uint                    mWidth;              // Width of Image in Pixels
      uint                    mHeight;             // Height of Image in Pixels

      // Data needed for image quantization

      static BQuantizeBox*   *mHeap;               // priority queue
      static long             mHeap_size;
      static BQuantizeBox*    mBoxes;              // variance box list
      static long             mNum_boxes;
      static DWORD*           mHistogramData;      // current histogram data
      
      // quantization functions
      
      bool                    initQuantization(long numColors) const;
      void                    shrinkQuantizeBox(long ir, long ig, long ib, long jr, long jg, long jb,
                                                long *lr, long *lg, long *lb, long *hr, long *hg, long *hb) const;

      void                    sumQuantizeData(long ir, long ig, long ib, long jr, long jg, long jb,
                                                DWORD *total_weight, DWORD *tt_sum, DWORD *t_ur, DWORD *t_ug, DWORD *t_ub) const;

      BQuantizeBox*           getWorstQuantizeBox(void) const;
      void                    splitQuantizeBox(BQuantizeBox *old_box) const;

      DWORD                   getVariance(DWORD tw, DWORD tt_sum, DWORD t_ur, DWORD t_ug, DWORD t_ub) const;

      void                    downQuantHeap(void) const;
      void                    insertQuantHeap(BQuantizeBox *p) const;

      void                    fillOutPalette(BPalette *thePallete) const;


}; // BImage


//==============================================================================




//==============================================================================
// BImage Class - This class provides support for handing RGBA images in
//                an easy to access (unpacked) format.
//
//==============================================================================
class BImage8
{
   public:
      explicit                BImage8(void);
                             ~BImage8(void);

      bool                    init(long width, long height, bool clear = true);
      void                    deInit(bool destroyAll = false);
                              
      // Accessor Functions

#ifndef BUILD_FINAL
      BYTE                    getPixel(long x, long y)                  { return (mValidImage ? (BYTE) mData[x + (y * mWidth)] : (BYTE) 0); }
      void                    setPixel(long x, long y, BYTE pix)        { if (mValidImage) mData[x + (y * mWidth)] = pix; }
#else                                                                   
      BYTE                    getPixel(long x, long y)                  { return mData[x + (y * mWidth)]; }
      void                    setPixel(long x, long y, BYTE pix)        { mData[x + (y * mWidth)] = pix; }
#endif

      
      BYTE*                   getData(void) const                       { return (mData); }
      uint                    getWidth(void) const                      { return (mWidth); }
      uint                    getHeight(void) const                     { return (mHeight); }
      bool                    getValid(void) const                      { return (mValidImage); }

      BPalette*               getPalette(void) const                    { return (mPalette);}

      // Copy functions

      BImage8&                operator= (BImage8 &SourceImg);

      bool                    copyAlphaChannel(BImage* SourceImg);
      
      bool                    saveBMP(long dirID, const BCHAR_T *filename);
      bool                    loadBMP(long dirID, const BCHAR_T *filename);


   protected:

      bool                    mValidImage;         // Indicates the image contents are valid

      BYTE*                   mData;               // Actual Image data of Indexed pixels
      uint                    mWidth;              // Width of Image in Pixels
      uint                    mHeight;             // Height of Image in Pixels

      BPalette*               mPalette;            // The color Palette


}; // BImage8








#endif // _IMAGE_H_

//==============================================================================
// eof: image.h
//==============================================================================

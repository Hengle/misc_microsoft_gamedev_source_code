// ========================================================================
// $File: //jeffr/granny/rt/granny_bink.cpp $
// $DateTime: 2007/09/20 15:05:27 $
// $Change: 16033 $
// $Revision: #9 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_BINK_H)
#include "granny_bink.h"
#endif

#if !defined(GRANNY_BINK0_COMPRESSION_H)
#include "granny_bink0_compression.h"
#endif

#if !defined(GRANNY_BINK1_COMPRESSION_H)
#include "granny_bink1_compression.h"
#endif

#if !defined(GRANNY_PIXEL_LAYOUT_H)
#include "granny_pixel_layout.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

#define radmemcpy(Dest, Source, Size) Copy(Size, Source, Dest)
#define USING_GL_RGB

/* ========================================================================
   From yuvrgb.c (Jeff's code)
   ======================================================================== */
static void * memset16( void * ptr, int16 val, uint32 count ) // count in 16-bit words
{
  uint32 c;
  int16* p;

  c = count;
  p = ( int16* ) ptr;

  while ( c-- )
  {
    *p++ = val;
  }

  return( p );
}

// convert into a YUV-ish colorspace
void RGBtoYUV( int16* yp, int16* up, int16* vp, int16* ap, int32 width, int32 height,
               void const* input, int32 inpitch, int32 inwidth, int32 inheight )
{
  uint32 x, y;
  uint32 * i = ( uint32* ) input;

  for( y = inheight ; y-- ; )
  {
    for( x = inwidth ; x-- ; )
    {
      int16 r,g,b,a;

      // this could be a *lot* faster
      #ifdef USING_GL_RGB
      r = (int16)   ( i[ 0 ] & 255 );
      g = (int16) ( ( i[ 0 ] >> 8 ) & 255 );
      b = (int16) ( ( i[ 0 ] >> 16 ) & 255 );
      a = (int16)   ( i[ 0 ] >> 24 );
      #else
      b = (int16)   ( i[ 0 ] & 255 );
      g = (int16) ( ( i[ 0 ] >> 8 ) & 255 );
      r = (int16) ( ( i[ 0 ] >> 16 ) & 255 );
      a = (int16)   ( i[ 0 ] >> 24 );
      #endif

      *yp++ = (int16) ( ( ( uint32 ) (r + g + g + b ) ) / 4 );
      *up++ = (int16) ( r - g );
      *vp++ = (int16) ( b - g );
      *ap++ = a;

      ++i;
    }

    // pad to out width from input width
    yp = (int16 *)memset16( yp, yp[ -1 ], width - inwidth );
    up = (int16 *)memset16( up, up[ -1 ], width - inwidth );
    vp = (int16 *)memset16( vp, vp[ -1 ], width - inwidth );
    ap = (int16 *)memset16( ap, ap[ -1 ], width - inwidth );

    i = (uint32*) ( ( ( char* ) i ) + ( inpitch - ( inwidth * 4 ) ) );
  }

  // pad to out height from input height
  for( y = height - inheight ; y-- ; )
  {
    radmemcpy( yp, yp - width, width * 2 );
    radmemcpy( up, up - width, width * 2 );
    radmemcpy( vp, vp - width, width * 2 );
    radmemcpy( ap, ap - width, width * 2 );
    yp += width;
    up += width;
    vp += width;
    ap += width;
  }

}

void RGBShiftUp( int16* yp, int16* up, int16* vp, int16* ap, int32 width, int32 height,
                 void const* input, int32 inpitch, int32 inwidth, int32 inheight )
{
  uint32 x, y;
  uint32 * i = ( uint32* ) input;

  for( y = inheight ; y-- ; )
  {
    for( x = inwidth ; x-- ; )
    {
      // this could be a *lot* faster
      *yp++ = (int16) (( i[ 0 ] & 255 ) << 2);
      *up++ = (int16) ((( i[ 0 ] >> 8 ) & 255) << 2);
      *vp++ = (int16) ((( i[ 0 ] >> 16 ) & 255 ) << 2);
      *ap++ = (int16) (( i[ 0 ] >> 24 ) << 2);

      ++i;
    }

    // pad to out width from input width
    yp = (int16 *)memset16( yp, yp[ -1 ], width - inwidth );
    up = (int16 *)memset16( up, up[ -1 ], width - inwidth );
    vp = (int16 *)memset16( vp, vp[ -1 ], width - inwidth );
    ap = (int16 *)memset16( ap, ap[ -1 ], width - inwidth );

    i = (uint32*) ( ( ( char* ) i ) + ( inpitch - ( inwidth * 4 ) ) );
  }

  // pad to out height from input height
  for( y = height - inheight ; y-- ; )
  {
    radmemcpy( yp, yp - width, width * 2 );
    radmemcpy( up, up - width, width * 2 );
    radmemcpy( vp, vp - width, width * 2 );
    radmemcpy( ap, ap - width, width * 2 );
    yp += width;
    up += width;
    vp += width;
    ap += width;
  }

}

// convert back to RGB from a YUV-ish colorspace
void YUVtoRGB( void * output, int16 const * yp, int16 const* up, int16 const* vp,
               int16 const* ap, int32 outpitch, int32 width, int32 height )
{
  uint32 x, y;
  uint8 * o = ( uint8* ) output;
  uint32 adj;

  adj = ( outpitch - ( width * 4 ) );

  for( y = height ; y-- ; )
  {
    for( x = width ; x-- ; )
    {
      int32 r,g,b,a;

      // this also could be much, much faster
      r = *up++;
      g = *yp++;
      b = *vp++;
      a = *ap++;

      g -= ( ( r + b ) / 4 );
      r += g;
      b += g;

      // clamp
      if ( a < 0 ) a = 0; else if ( a > 255 ) a = 255;
      if ( r < 0 ) r = 0; else if ( r > 255 ) r = 255;
      if ( g < 0 ) g = 0; else if ( g > 255 ) g = 255;
      if ( b < 0 ) b = 0; else if ( b > 255 ) b = 255;

      #ifdef USING_GL_RGB
      *o++ = (uint8)r;
      *o++ = (uint8)g;
      *o++ = (uint8)b;
      *o++ = (uint8)a;
      #else
      *o++ = (uint8)b;
      *o++ = (uint8)g;
      *o++ = (uint8)r;
      *o++ = (uint8)a;
      #endif
    }
    o += adj ;
  }
}


void RGBShiftDown( void * output, int16 const * yp, int16 const* up, int16 const* vp,
                   int16 const* ap, int32 outpitch, int32 width, int32 height )
{
  uint32 x, y;
  uint8 * o = ( uint8* ) output;
  uint32 adj;

  adj = ( outpitch - ( width * 4 ) );

  for( y = height ; y-- ; )
  {
    for( x = width ; x-- ; )
    {
      int32 r,g,b,a;

      // this also could be much, much faster
      r = *yp++ >> 2;
      g = *up++ >> 2;
      b = *vp++ >> 2;
      a = *ap++ >> 2;

      // clamp
      if ( a < 0 ) a = 0; else if ( a > 255 ) a = 255;
      if ( r < 0 ) r = 0; else if ( r > 255 ) r = 255;
      if ( g < 0 ) g = 0; else if ( g > 255 ) g = 255;
      if ( b < 0 ) b = 0; else if ( b > 255 ) b = 255;

      *o++ = (uint8)r;
      *o++ = (uint8)g;
      *o++ = (uint8)b;
      *o++ = (uint8)a;
    }
    o += adj ;
  }
}


// convert a signed 16-bit plane into RGB output
void PtoRGB( uint32* output, int16 const* plane, int32 width, int32 height )
{
  int32 x, y;
  int16 const* p;
  uint32* o;
  int32 min, max, range;

  // figure out the min and max in case they are outside of 8-bit range
  min = 0;
  max = 255;
  p = plane;
  for( x = (width*height); x-- ; )
  {
    if ( (*p) < min )
      min = (*p);
    else if ( (*p) > max )
      max = (*p);
    ++p;
  }

  range = max - min + 1;

  // copy it over
  p = plane;
  o = output;
  for(y=0;y<height;y++)
  {
    for(x=0;x<width;x++)
    {
      int32 val;

      val = ( ( (*p) - min ) * 256 ) / range;

      *o++ = val | ( val << 8 ) | ( val << 16 );
      ++p;
    }
  }
}

static int32x
RGBABytes(bool Alpha)
{
    return(Alpha ? 4 : 3);
}

static int32x
GetCompressTo(int32x Width, int32x Height, bool Alpha, int32x Compression)
{
    if(Compression < 1)
    {
        Compression = 1;
    }

    return((Width * Height * RGBABytes(Alpha)) / Compression);
}

static bool
ShouldBink(int32x &Width, int32x &Height, int32x BinkVersion)
{
    if((Width * Height) > 256)
    {
        uint32 WidthTemp = Width;
        uint32 HeightTemp = Height;
        switch(BinkVersion)
        {
            case 0: BinkTCCheckSizes0(&WidthTemp, &HeightTemp); break;
            case 1: BinkTCCheckSizes1(&WidthTemp, &HeightTemp); break;
        }
        Width = (int32x)WidthTemp;
        Height = (int32x)HeightTemp;

        return(true);
    }
    else
    {
        return(false);
    }
}

pixel_layout const &GRANNY
GetBinkPixelLayout(bool Alpha)
{
    return(Alpha ? RGBA8888PixelFormat : RGB888PixelFormat);
}

static int32x
GetBinkVersion(uint32x Flags)
{
    return((Flags & BinkUseBink1) ? 1 : 0);
}

int32x GRANNY
GetMaximumBinkImageSize(int32x Width, int32x Height,
                        uint32x Flags, int32x Compression)
{
    bool Alpha = Flags & BinkEncodeAlpha;
    int32x BinkVersion = GetBinkVersion(Flags);

    if(ShouldBink(Width, Height, BinkVersion))
    {
        switch(BinkVersion)
        {
            case 0:
                return(ToBinkTCOutputMem0(
                           Width, Height, RGBABytes(Alpha),
                           GetCompressTo(Width, Height, Alpha, Compression)));
            case 1:
                return(ToBinkTCOutputMem1(
                           Width, Height, RGBABytes(Alpha),
                           GetCompressTo(Width, Height, Alpha, Compression)));
            default:
                Assert ( false );
                return -1;
        }
    }
    else
    {
        return(Width * Height * RGBABytes(Alpha));
    }
}

int32x GRANNY
BinkCompressTexture(int32x SourceWidth, int32x SourceHeight,
                    int32x SourceStride, void const *Source,
                    uint32x Flags, int32x Compression, void *Dest)
{
    int32x ResultSize = 0;

    int32x Width = SourceWidth;
    int32x Height = SourceHeight;

    bool Alpha = Flags & BinkEncodeAlpha;

    int32x BinkVersion = GetBinkVersion(Flags);

    if(ShouldBink(Width, Height, BinkVersion))
    {
        int16 *Planes[4];  // Y U V A
        real32 Weights[4] = {6.0f, 1.0f, 1.0f, 1.0f};
        int32x PlaneCount = RGBABytes(Alpha);

        int32x TempSize = 0;
        switch(BinkVersion)
        {
            case 0: TempSize = ToBinkTCTempMem0(Width, Height); break;
            case 1: TempSize = ToBinkTCTempMem1(Width, Height); break;
            default:
                Assert ( false );
                break;
        }
        void *Temp = AllocateSize(TempSize);

        int32x CompressTo = GetCompressTo(Width, Height, Alpha, Compression);
        {for(int32x PlaneIndex = 0;
             PlaneIndex < 4;
             ++PlaneIndex)
        {
            Planes[PlaneIndex] = AllocateArray(Width*Height, int16);
        }}

        if(Flags & BinkUseScaledRGBInsteadOfYUV)
        {
            Weights[0] = 1.0f;
            RGBShiftUp(Planes[0], Planes[1], Planes[2], Planes[3],
                       Width, Height, Source,
                       SourceStride, SourceWidth, SourceHeight);
        }
        else
        {
            RGBtoYUV(Planes[0], Planes[1], Planes[2], Planes[3],
                     Width, Height, Source,
                     SourceStride, SourceWidth, SourceHeight);
        }

        switch(BinkVersion)
        {
            case 0:
                ResultSize = ToBinkTC0(Dest, CompressTo, Planes, Weights, PlaneCount,
                                       Width, Height, Temp, TempSize);
                break;
            case 1:
                ResultSize = ToBinkTC1(Dest, CompressTo, Planes, Weights, PlaneCount,
                                       Width, Height, Temp, TempSize);
                break;
        }

        // TODO: The following is a hack to make sure we work on little-endian
        // machines.  What should probably happen in the (far) future
        // is that Binks should be put in the 32-bit reversal section
        // of the file.
        if(PROCESSOR_BIG_ENDIAN)
        {
            Reverse32(ResultSize, Dest);
        }

        {for(int32x PlaneIndex = 0;
             PlaneIndex < 4;
             ++PlaneIndex)
        {
            Deallocate(Planes[PlaneIndex]);
        }}

        Deallocate(Temp);
    }
    else
    {
        ConvertPixelFormat(Width, Height, RGBA8888PixelFormat,
                           SourceStride, Source,
                           Alpha ? RGBA8888PixelFormat : RGB888PixelFormat,
                           Width * RGBABytes(Alpha), Dest);
        ResultSize = Width * Height * RGBABytes(Alpha);
    }

    return(ResultSize);
}

void GRANNY
BinkDecompressTexture(int32x DestWidth, int32x DestHeight, uint32x Flags,
                      int32x SourceSize, void const *Source,
                      pixel_layout const &DestLayout,
                      int32x DestStride, void *Dest)
{
    bool Alpha = Flags & BinkEncodeAlpha;

    int32x BinkVersion = (Flags & BinkUseBink1) ? 1 : 0;

    int32x Width = DestWidth;
    int32x Height = DestHeight;
    if(ShouldBink(Width, Height, BinkVersion))
    {
        int16 *Planes[4];  // Y U V A
        int32x PlaneCount = RGBABytes(Alpha);

        // TODO: The following is a hack to make sure we work on little-endian
        // machines.  What should probably happen in the (far) future
        // is that Binks should be put in the 32-bit reversal section
        // of the file.
        if(BinkVersion == 0)
        {
            if(PROCESSOR_BIG_ENDIAN)
            {
                Reverse32(SourceSize, (void *)Source);
            }
        }

        int32x TempSize = 0;
        switch(BinkVersion)
        {
            case 0: TempSize = FromBinkTCTempMem0(Source); break;
            case 1: TempSize = FromBinkTCTempMem1(Source); break;
            default:
                Assert ( false );
                break;
        }
        void *Temp = AllocateSize(TempSize);

        void *TempPixels = AllocateSize(Width*Height*4);

        {for(int32x PlaneIndex = 0;
             PlaneIndex < 4;
             ++PlaneIndex)
        {
            Planes[PlaneIndex] = AllocateArray(Width*Height, int16);
        }}

        switch(BinkVersion)
        {
            case 0: FromBinkTC0(Planes, PlaneCount, Source, Width, Height, Temp, TempSize); break;
            case 1: FromBinkTC1(Planes, PlaneCount, Source, Width, Height, Temp, TempSize); break;
        }

        if(Flags & BinkUseScaledRGBInsteadOfYUV)
        {
            RGBShiftDown(TempPixels, Planes[0], Planes[1], Planes[2], Planes[3],
                         Width*4, Width, Height);
        }
        else
        {
            YUVtoRGB(TempPixels, Planes[0], Planes[1], Planes[2], Planes[3],
                     Width*4, Width, Height);
        }

        ConvertPixelFormat(DestWidth, DestHeight,
                           RGBA8888PixelFormat,
                           Width*4, TempPixels,
                           DestLayout, DestStride, Dest);

        {for(int32x PlaneIndex = 0;
             PlaneIndex < 4;
             ++PlaneIndex)
        {
            Deallocate(Planes[PlaneIndex]);
        }}

        Deallocate(TempPixels);
        Deallocate(Temp);
    }
    else
    {
        ConvertPixelFormat(Width, Height,
                           Alpha ? RGBA8888PixelFormat : RGB888PixelFormat,
                           Width * RGBABytes(Alpha), Source,
                           DestLayout, DestStride, Dest);
    }
}

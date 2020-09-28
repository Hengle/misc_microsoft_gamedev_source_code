// ========================================================================
// $File: //jeffr/granny/rt/granny_image_operations.cpp $
// $DateTime: 2007/09/20 13:01:50 $
// $Change: 16027 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_IMAGE_OPERATIONS_H)
#include "granny_image_operations.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode TextureLogMessage

USING_GRANNY_NAMESPACE;

typedef real32 SCALEFLOAT;
typedef uint32 u32;
typedef int32 s32;
typedef uint16 u16;
typedef int16 s16;
typedef uint8 u8;
typedef int8 s8;

#define ceil Ceiling
#define floor Floor
#define radmalloc AllocateSize
#define radfree Deallocate

typedef u32 (*RADScaleGetPixel32)(void* scanline,u32 xoff);

typedef struct
{
  SCALEFLOAT    weight;
  int32   pixel;
} CONTRIB;

typedef struct RADSCALE
{
    uint32 destwidth;
    uint32 destheight;

    uint32 srcwidth;
    uint32 srcheight;

    int32* w_contrib_counts;
    CONTRIB* w_contribs;

    int32* h_contrib_counts;
    CONTRIB* h_contribs;

    int32 w_contrib_max;
    int32 h_contrib_max;

    SCALEFLOAT tmp[0];
} RADSCALE;

RADSCALE* RADScaleOpen(int32 filter_type, uint32 destwidth,uint32 destheight,
                       uint32 srcwidth, uint32 srcheight);
void RADScale32(RADSCALE* rs, void* dest, s32 destpitch, void const * src, s32 srcpitch, RADScaleGetPixel32 gp);
void RADScaleClose(RADSCALE* rs);

static u32
GetPixelStub(void *ScanLine, u32 Offset)
{
    return(*((uint32 *)ScanLine + Offset));
}

void GRANNY
ScaleImage(pixel_filter_type FilterType,
           int32x SourceWidth, int32x SourceHeight, int32x SourceStride,
           uint8 const *SourcePixels,
           int32x DestWidth, int32x DestHeight, int32x DestStride,
           uint8 *DestPixels)
{
    if(FilterType < OnePastLastPixelFilterType)
    {
        RADSCALE *Scaler = RADScaleOpen(FilterType, DestWidth, DestHeight,
                                        SourceWidth, SourceHeight);
        RADScale32(Scaler, DestPixels, DestStride, SourcePixels, SourceStride,
                   GetPixelStub);
        RADScaleClose(Scaler);
    }
    else
    {
        Log1(ErrorLogMessage, TextureLogMessage,
             "Invalid pixel filter specified (%d)", FilterType);
    }
}

// adapted from Graphics Gems III

typedef SCALEFLOAT (*filterft)(SCALEFLOAT t);

#define box_support     (0.5)

SCALEFLOAT box_filter(SCALEFLOAT t)
{
  if ((t > -0.5) && (t <= 0.5))
    return(1.0);
  return(0.0);
}

#define linear_support  (1.0)

SCALEFLOAT linear_filter( SCALEFLOAT t)
{
  if (t < 0.0)
    t = -t;
  if(t < 1.0)
    return((SCALEFLOAT)(1.0 - t));
  return(0.0);
}

#define mcubic_support  (2.0)

SCALEFLOAT mcubic_filter(SCALEFLOAT t)
{
  SCALEFLOAT tt;

  tt = t * t;

  if (t < 0)
    t = -t;

  if(t < 1.0)
  {
    return((SCALEFLOAT)(  ((7.0/6.0)*t*tt)+
                          (-2.0*tt)+
                          (8.0/9.0)));
  }
  else
  {
    if (t<2.0)
    {
      return((SCALEFLOAT)(  ((-7.0/18.0)*t*tt)+
                            (2.0*tt)+
                            ((-10.0/3.0)*t)+
                            (16.0/9.0)));
    }
  }
  return(0);
}

static void calc_contribs(u32 dest,u32 src,s32** ret_contrib_counts,CONTRIB** ret_contribs,s32* ret_contrib_max,filterft filterf,SCALEFLOAT fwidth)
{
  SCALEFLOAT scale;
  s32* contrib_counts=0;
  CONTRIB* contribs=0;
  s32 contrib_max=0;
  s32 i,j;

  if ( ( src != 1 ) && ( dest != 1 ) )
    scale = ( SCALEFLOAT ) ( dest - 1 ) / ( SCALEFLOAT ) ( src - 1 );
  else
    scale = ( SCALEFLOAT ) dest / ( SCALEFLOAT ) src;

  // pre-calculate filter contributions

  if (scale < 1.0)
  {
    // shrinking the area
    SCALEFLOAT width = fwidth / scale;

    contrib_max=((s32)ceil(width * 2 + 1));
    contrib_counts=(s32 *)radmalloc( (sizeof(s32)+
                                      sizeof(CONTRIB)*contrib_max)*dest );
    if (contrib_counts==0)
      goto dump;

    contribs=(CONTRIB*)(contrib_counts+dest);

    for (i = 0; i < (s32)dest; ++i)
    {
        SCALEFLOAT center;
       s32 right;
       CONTRIB* cur=contribs+i*contrib_max;
       SCALEFLOAT total_weight = 0.0F;
       s32 total_count = 0;

       center = (SCALEFLOAT) i / scale;
       right = (s32)floor(center + width);

       for (j =(s32)ceil(center - width); j <= right; ++j)
       {
         if (j<0)
         {
           cur->pixel = ( -j ) % src;
         }
         else if (j>=(s32)src)
         {
           cur->pixel = (src - 1 ) - ( ( j - ( src - 1 ) ) % src );
         }
         else
         {
           cur->pixel = j;
         }
         cur->weight=(*filterf)((center - (SCALEFLOAT) j) * scale) * scale;
         total_weight += cur->weight;
         ++total_count;
         cur++;
       }

       //
       // Normalize the weights...
       //

       total_weight = ( 1.0f / total_weight );
       cur=contribs+i*contrib_max;
       while( total_count-- )
       {
         cur->weight *= total_weight;
         cur++;
       }

       CheckConvertToInt32(contrib_counts[i], cur-(contribs+i*contrib_max), return);
    }
  }
  else
  {
    // zooming the area
    contrib_max=(TruncateReal32ToInt32(fwidth * 2 + 1));
    contrib_counts=(s32 *)radmalloc( (sizeof(s32)+
                                      sizeof(CONTRIB)*contrib_max)*dest );
    if (contrib_counts==0)
      goto dump;

    contribs=(CONTRIB*)(contrib_counts+dest);

    for(i = 0; i < (s32)dest; ++i)
    {
        SCALEFLOAT center;
       s32 right;
       CONTRIB* cur=contribs+i*contrib_max;
       SCALEFLOAT total_weight = 0.0F;
       s32 total_count = 0;

       center = (SCALEFLOAT) i / scale;
       right = (s32)floor(center + fwidth);
       for (j =(s32)ceil(center - fwidth); j <= right; ++j)
       {
         if (j<0)
         {
           cur->pixel = ( -j ) % src;
         }
         else if (j>=(s32)src)
         {
           cur->pixel = (src - 1 ) - ( ( j - ( src - 1 ) ) % src );
         }
         else
         {
           cur->pixel = j;
         }
         cur->weight=(*filterf)(center - (SCALEFLOAT) j);
         total_weight += cur->weight;
         ++total_count;
         cur++;
       }

       //
       // Normalize the weights...
       //

       total_weight = ( 1.0f / total_weight );
       cur=contribs+i*contrib_max;
       while( total_count-- )
       {
         cur->weight *= total_weight;
         cur++;
       }

       CheckConvertToInt32(contrib_counts[i], cur-(contribs+i*contrib_max), return);
    }
  }
 dump:
  *ret_contrib_counts=contrib_counts;
  *ret_contribs=contribs;
  *ret_contrib_max=contrib_max;
}


static SCALEFLOAT filter_support[]={mcubic_support,linear_support,box_support};
static filterft filter_function[]={mcubic_filter,linear_filter,box_filter};

RADSCALE* RADScaleOpen(s32 filter_type, u32 destwidth,u32 destheight,u32 srcwidth, u32 srcheight)
{
  RADSCALE* ret;
  SCALEFLOAT fwidth;
  filterft filterf;

  ret=(RADSCALE*)radmalloc(sizeof(RADSCALE)+(srcheight * sizeof(SCALEFLOAT)*4));
  if (ret==0)
    return(0);

  fwidth=filter_support[filter_type];
  filterf=filter_function[filter_type];

  ret->destwidth=destwidth;
  ret->destheight=destheight;
  ret->srcwidth=srcwidth;
  ret->srcheight=srcheight;

  calc_contribs(destwidth,srcwidth,&ret->w_contrib_counts,&ret->w_contribs,&ret->w_contrib_max,filterf,fwidth);
  if (ret->w_contrib_counts==0)
  {
   err:
    radfree(ret);
    return(0);
  }

  calc_contribs(destheight,srcheight,&ret->h_contrib_counts,&ret->h_contribs,&ret->h_contrib_max,filterf,fwidth);
  if (ret->h_contrib_counts==0)
  {
    radfree(ret->w_contrib_counts);
    goto err;
  }

  return(ret);
}

void RADScaleClose(RADSCALE* rs)
{
  radfree(rs->w_contrib_counts);
  radfree(rs->h_contrib_counts);
  radfree(rs);
}

void RADScale32(RADSCALE* rs, void* dest, s32 destpitch, void const * src, s32 srcpitch, RADScaleGetPixel32 gp)
{
  s32 xx,i, j;          /* loop variables */
  SCALEFLOAT weight;
  SCALEFLOAT weighta,weightr,weightg,weightb;   /* filter calculation variables */
  CONTRIB* cur;

  for(xx = 0; xx < (s32)rs->destwidth; xx++)
  {

    /* Apply horz filter to make dst column in tmp. */
    SCALEFLOAT* colu=rs->tmp;
    u8* scanline=((u8*)src);

    for(i = 0; i < (s32)rs->srcheight; ++i)
    {
      weighta = 0.0;
      weightr = 0.0;
      weightg = 0.0;
      weightb = 0.0;

      cur=rs->w_contribs+xx*rs->w_contrib_max;
      for(j = 0; j < rs->w_contrib_counts[xx]; ++j)
      {
        s32 pel = gp(scanline,cur[j].pixel);
        weight = cur[j].weight;
//#define TEST
#ifdef TEST
        weighta += 0 * weight;
        weightr += 255 * weight;
        weightg += 255 * weight;
        weightb += 255 * weight;
#else
        weighta += ((u32)pel>>24) * weight;
        weightr += ((pel>>16)&255) * weight;
        weightg += ((pel>>8)&255) * weight;
        weightb += (pel&255) * weight;
#endif
      }

      colu[0] = weighta;
      colu[1] = weightr;
      colu[2] = weightg;
      colu[3] = weightb;

      colu+=4;
      scanline+=srcpitch;
    }


    /* The temp column has been built. Now stretch it
         vertically into dst column. */
    scanline=((u8*)dest)+(xx*4);
    for(i = 0; i < (s32)rs->destheight; ++i)
    {
      weighta = 0.0;
      weightr = 0.0;
      weightg = 0.0;
      weightb = 0.0;

      cur=rs->h_contribs+i*rs->h_contrib_max;
      for(j = 0; j < rs->h_contrib_counts[i]; ++j)
      {
        colu=rs->tmp+(cur[j].pixel*4);
        weight = cur[j].weight;

        weighta += colu[0] * weight;
        weightr += colu[1] * weight;
        weightg += colu[2] * weight;
        weightb += colu[3] * weight;
      }

      if (weighta>0.0)
        j=(weighta>=254.5)?(255<<24):((RoundReal32ToInt32(weighta))<<24);
      else
        j=0;
      if (weightr>0.0)
        j|=(weightr>=254.5)?(255<<16):((RoundReal32ToInt32(weightr))<<16);
      if (weightg>0.0)
        j|=(weightg>=254.5)?(255<<8):((RoundReal32ToInt32(weightg))<<8);
      if (weightb>0.0)
        j|=(weightb>=254.5)?255:((RoundReal32ToInt32(weightb)));

      *((u32*)scanline)=j;

      scanline+=destpitch;
    }
  }
}

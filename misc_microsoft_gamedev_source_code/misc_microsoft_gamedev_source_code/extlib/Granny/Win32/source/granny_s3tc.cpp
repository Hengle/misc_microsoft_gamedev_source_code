// ========================================================================
// $File: //jeffr/granny/rt/granny_s3tc.cpp $
// $DateTime: 2007/11/06 15:40:58 $
// $Change: 16454 $
// $Revision: #9 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_S3TC_H)
#include "granny_s3tc.h"
#endif

#if !defined(GRANNY_PIXEL_LAYOUT_H)
#include "granny_pixel_layout.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

static char const *S3TCTextureFormatNames[] =
{
    "S3TC RGB555 (DXT1)",
    "S3TC RGBA5551 (DXT1 with alpha)",
    "S3TC RGBA8888, mapped alpha (DXT2/3)",
    "S3TC RGBA8888, interpolated alpha (DXT4/5)",
};

pixel_layout const &GRANNY
GetS3TCPixelLayout(s3tc_texture_format Format)
{
    switch(Format)
    {
        case S3TCBGR565:
        {
            return(BGR565PixelFormat);
        } //break;

        case S3TCBGRA5551:
        {
            return(BGRA5551PixelFormat);
        } //break;

        default:
        {
            return(BGRA8888PixelFormat);
        } //break;
    }
}

char const *GRANNY
GetS3TCTextureFormatName(int32x Format)
{
    if(Format < OnePastLastS3TCTextureFormat)
    {
        return(S3TCTextureFormatNames[Format]);
    }
    else
    {
        return("Unknown S3TC texture format");
    }
}

//=========================================================================
// S3TC routines - Jeff Roberts 6/28/00
//=========================================================================

// set this define for GL_RGBA input (vs. GL_BGRA which is the default)
#define USING_GL_RGB

//=========================================================================
// lookup tables
static int setup=0;
static unsigned int div3[256];
static unsigned int absl[512];
static unsigned int alpha4to8[16];
static unsigned int alpha4to88[16];
static unsigned int r5to8[32];
static unsigned int g6to8[64];
static unsigned int b5to8[32];
#define Quick8Abs(val) (absl[511&(val)]) // can do -255 to 255
#define Quick3Div(val) (div3[(val)])     // can divide values up to a max of 255
//=========================================================================


static void setuplookups(void);
static unsigned int findcolor4x4(unsigned short* output,
                                 const unsigned int* input,unsigned int ip);
static unsigned int findcolor4x4a(unsigned short* output,
                                  const unsigned int* input,unsigned int ip);
static unsigned int findalpha4x4(unsigned short* output,
                                 const unsigned char* input,unsigned int ip);

static bool MakePaddedImage(void const* original,
                            unsigned int orig_stride,
                            unsigned int orig_width,
                            unsigned int orig_height,
                            void*& padded,
                            unsigned int& padded_stride,
                            unsigned int& padded_width,
                            unsigned int& padded_height)
{
    Assert(orig_width < 4 || orig_height < 4);  // Otherwise, we shouldn't be here...
    Assert(orig_width > 0 || orig_height > 0);  // something weird is going on...

    padded_width  = orig_width  < 4 ? 4 : orig_width;
    padded_height = orig_height < 4 ? 4 : orig_height;
    padded_stride = padded_width * 4;

    uint8* PaddedImage = AllocateArray(padded_width * padded_height * 4, uint8);
    padded = PaddedImage;

    for (unsigned int y = 0; y < padded_height; ++y)
    {
        int source_y = y % orig_height;

        uint8 const* orig_row   = (uint8 const*)original + (source_y * orig_stride);
        uint8*       padded_row = PaddedImage + (y * padded_stride);

        for (unsigned int x = 0; x < padded_stride; x += orig_stride)
        {
            unsigned int copy_bytes = padded_stride - x;
            if (copy_bytes > orig_stride)
                copy_bytes = orig_stride;

            Copy(copy_bytes, orig_row, padded_row + x);
        }
    }

    return true;
}


//=========================================================================
// output needs to be 4-bits per pixel, input needs to be 32-bits per pixel
//   alpha is ignored in this routine (use to_S3TC1a for alpha)
//=========================================================================

unsigned int GRANNY
to_S3TC1(void* output,const void* input, unsigned int instride, unsigned int width, unsigned int height)
{
    Assert(width > 0 && height > 0);

    if (width < 4 || height < 4)
    {
        void* padded;
        unsigned int pad_stride, pad_width, pad_height;
        if (MakePaddedImage(input, instride, width, height,
                            padded, pad_stride, pad_width, pad_height))
        {
            unsigned int err = to_S3TC1(output, padded, pad_stride, pad_width, pad_height);
            Deallocate(padded);

            return err;
        }
        else
        {
            InvalidCodePath("Failed to make padded image");
            return (unsigned int)-1;
        }
    }
    Assert((instride % 4) == 0);
    Assert((width % 4) == 0);
    Assert((height % 4) == 0);

    int x,y;
    unsigned int inadj=((instride*4)-(width*4))/4;
    unsigned int* i=(unsigned int*)input;
    unsigned short* o=(unsigned short*)output;
    unsigned int ip=instride/4;
    unsigned int iw=width/4;
    unsigned int err=0;

    // setup a couple of lookup tables
    setuplookups();

    for(y=(unsigned int)(height/4);y;y--)
    {
        for(x=(unsigned int)iw;x;x--)
        {
            err+=findcolor4x4(o,i,ip);
            o+=4;
            i+=4;
        }
        i+=inadj;
    }

    return(err);
}


//=========================================================================
// output needs to be 4-bits per pixel, input needs to be 32-bits per pixel
//   alpha is performed in this routine (use to_S3TC1 for no alpha)
//=========================================================================

unsigned int GRANNY
to_S3TC1a(void* output,const void* input, unsigned int instride, unsigned int width, unsigned int height)
{
    if (width < 4 || height < 4)
    {
        void* padded;
        unsigned int pad_stride, pad_width, pad_height;
        if (MakePaddedImage(input, instride, width, height,
                            padded, pad_stride, pad_width, pad_height))
        {
            unsigned int err = to_S3TC1a(output, padded, pad_stride, pad_width, pad_height);
            Deallocate(padded);

            return err;
        }
        else
        {
            InvalidCodePath("Failed to make padded image");
            return (unsigned int)-1;
        }
    }
    Assert((instride % 4) == 0);
    Assert((width % 4) == 0);
    Assert((height % 4) == 0);

    int x,y;
    unsigned int inadj=((instride*4)-(width*4))/4;
    unsigned int* i=(unsigned int*)input;
    unsigned short* o=(unsigned short*)output;
    unsigned int ip=instride/4;
    unsigned int iw=width/4;
    unsigned int err=0;

    // setup a couple of lookup tables
    setuplookups();

    for(y=(unsigned int)(height/4);y;y--)
    {
        for(x=(unsigned int)iw;x;x--)
        {
            err+=findcolor4x4a(o,i,ip);
            o+=4;
            i+=4;
        }
        i+=inadj;
    }

    return(err);
}


//=========================================================================
// output needs to be 8-bits per pixel, input needs to be 32-bits per pixel
//   instride, width and height must all be divisible by 4
//   alpha is performed in this routine (use to_S3TC1 for no alpha)
//=========================================================================

unsigned int GRANNY
to_S3TC2or3(void* output,const void* input, unsigned int instride, unsigned int width, unsigned int height)
{
    if (width < 4 || height < 4)
    {
        void* padded;
        unsigned int pad_stride, pad_width, pad_height;
        if (MakePaddedImage(input, instride, width, height,
                            padded, pad_stride, pad_width, pad_height))
        {
            unsigned int err = to_S3TC2or3(output, padded, pad_stride, pad_width, pad_height);
            Deallocate(padded);

            return err;
        }
        else
        {
            InvalidCodePath("Failed to make padded image");
            return (unsigned int)-1;
        }
    }
    Assert((instride % 4) == 0);
    Assert((width % 4) == 0);
    Assert((height % 4) == 0);

    int x,y;
    unsigned int inadj=((instride*4)-(width*4))/4;
    unsigned int* i=(unsigned int*)input;
    unsigned short* o=(unsigned short*)output;
    unsigned int ip=instride/4;
    unsigned int iw=width/4;
    unsigned int err=0;

    // setup a couple of lookup tables
    setuplookups();

    for(y=(unsigned int)(height/4);y;y--)
    {
        for(x=(unsigned int)iw;x;x--)
        {
            // quantize the alpha down to 4 bits
            o[0]=(unsigned short)((i[0]>>28)|((i[1]>>28)<<4|((i[2]>>28)<<8)|((i[3]>>28)<<12)));
            err+=(Quick8Abs((i[0]>>24)-alpha4to88[(i[0]>>28)])+Quick8Abs((i[1]>>24)-alpha4to88[(i[1]>>28)])+Quick8Abs((i[2]>>24)-alpha4to88[(i[2]>>28)])+Quick8Abs((i[3]>>24)-alpha4to88[(i[3]>>28)]));
            i+=ip;
            o[1]=(unsigned short)((i[0]>>28)|((i[1]>>28)<<4|((i[2]>>28)<<8)|((i[3]>>28)<<12)));
            err+=(Quick8Abs((i[0]>>24)-alpha4to88[(i[0]>>28)])+Quick8Abs((i[1]>>24)-alpha4to88[(i[1]>>28)])+Quick8Abs((i[2]>>24)-alpha4to88[(i[2]>>28)])+Quick8Abs((i[3]>>24)-alpha4to88[(i[3]>>28)]));
            i+=ip;
            o[2]=(unsigned short)((i[0]>>28)|((i[1]>>28)<<4|((i[2]>>28)<<8)|((i[3]>>28)<<12)));
            err+=(Quick8Abs((i[0]>>24)-alpha4to88[(i[0]>>28)])+Quick8Abs((i[1]>>24)-alpha4to88[(i[1]>>28)])+Quick8Abs((i[2]>>24)-alpha4to88[(i[2]>>28)])+Quick8Abs((i[3]>>24)-alpha4to88[(i[3]>>28)]));
            i+=ip;
            o[3]=(unsigned short)((i[0]>>28)|((i[1]>>28)<<4|((i[2]>>28)<<8)|((i[3]>>28)<<12)));
            err+=(Quick8Abs((i[0]>>24)-alpha4to88[(i[0]>>28)])+Quick8Abs((i[1]>>24)-alpha4to88[(i[1]>>28)])+Quick8Abs((i[2]>>24)-alpha4to88[(i[2]>>28)])+Quick8Abs((i[3]>>24)-alpha4to88[(i[3]>>28)]));

            i-=(ip*3);
            o+=4;
            err+=findcolor4x4(o,i,ip);
            o+=4;
            i+=4;
        }
        i+=inadj;
    }

    return(err);
}


//=========================================================================
// output needs to be 8-bits per pixel, input needs to be 32-bits per pixel
//   instride, width and height must all be divisible by 4
//   alpha is performed in this routine (use to_S3TC1 for no alpha)
//=========================================================================

unsigned int GRANNY
to_S3TC4or5(void* output,const void* input, unsigned int instride, unsigned int width, unsigned int height)
{
    if (width < 4 || height < 4)
    {
        void* padded;
        unsigned int pad_stride, pad_width, pad_height;
        if (MakePaddedImage(input, instride, width, height,
                            padded, pad_stride, pad_width, pad_height))
        {
            unsigned int err = to_S3TC4or5(output, padded, pad_stride, pad_width, pad_height);
            Deallocate(padded);

            return err;
        }
        else
        {
            InvalidCodePath("Failed to make padded image");
            return (unsigned int)-1;
        }
    }
    Assert((instride % 4) == 0);
    Assert((width % 4) == 0);
    Assert((height % 4) == 0);

    int x,y;
    unsigned int inadj=((instride*4)-(width*4))/4;
    unsigned int* i=(unsigned int*)input;
    unsigned short* o=(unsigned short*)output;
    unsigned int ip=instride/4;
    unsigned int iw=width/4;
    unsigned int err=0;

    // setup a couple of lookup tables
    setuplookups();

    for(y=(unsigned int)(height/4);y;y--)
    {
        for(x=(unsigned int)iw;x;x--)
        {
            err+=findalpha4x4(o,((unsigned char*)i)+3,ip*4);
            o+=4;
            err+=findcolor4x4(o,i,ip);
            o+=4;
            i+=4;
        }
        i+=inadj;
    }

    return(err);
}


//=========================================================================
// output needs to be 8-bits per pixel, input needs to be 32-bits per pixel
//   instride, width and height must all be divisible by 4
//   alpha is performed in this routine (use to_S3TC1 for no alpha)
//   type returns 2 for S3TC2 or S3TC3 format or 4 for S3TC4 or S3TC5 format
//=========================================================================

unsigned int GRANNY
to_S3TC2or3vs4or5(void* output,const void* input, unsigned int instride, unsigned int width, unsigned int height,unsigned int* type)
{
    if (width < 4 || height < 4)
    {
        void* padded;
        unsigned int pad_stride, pad_width, pad_height;
        if (MakePaddedImage(input, instride, width, height,
                            padded, pad_stride, pad_width, pad_height))
        {
            unsigned int err = to_S3TC2or3vs4or5(output, padded, pad_stride, pad_width, pad_height, type);
            Deallocate(padded);

            return err;
        }
        else
        {
            InvalidCodePath("Failed to make padded image");
            return (unsigned int)-1;
        }
    }
    Assert((instride % 4) == 0);
    Assert((width % 4) == 0);
    Assert((height % 4) == 0);

    int x,y;
    unsigned int inadj=((instride*4)-(width*4))/4;
    unsigned int* i=(unsigned int*)input;
    unsigned short* o=(unsigned short*)output;
    unsigned int ip=instride/4;
    unsigned int iw=width/4;
    unsigned int err=0,aerr2=0,aerr4=0;

    // setup a couple of lookup tables
    setuplookups();

    // try linear
    for(y=(unsigned int)(height/4);y;y--)
    {
        for(x=(unsigned int)iw;x;x--)
        {
            // quantize the alpha down to 4 bits
            o[0]=(unsigned short)((i[0]>>28)|((i[1]>>28)<<4|((i[2]>>28)<<8)|((i[3]>>28)<<12)));
            aerr2+=(Quick8Abs((i[0]>>24)-alpha4to88[(i[0]>>28)])+Quick8Abs((i[1]>>24)-alpha4to88[(i[1]>>28)])+Quick8Abs((i[2]>>24)-alpha4to88[(i[2]>>28)])+Quick8Abs((i[3]>>24)-alpha4to88[(i[3]>>28)]));
            i+=ip;
            o[1]=(unsigned short)((i[0]>>28)|((i[1]>>28)<<4|((i[2]>>28)<<8)|((i[3]>>28)<<12)));
            aerr2+=(Quick8Abs((i[0]>>24)-alpha4to88[(i[0]>>28)])+Quick8Abs((i[1]>>24)-alpha4to88[(i[1]>>28)])+Quick8Abs((i[2]>>24)-alpha4to88[(i[2]>>28)])+Quick8Abs((i[3]>>24)-alpha4to88[(i[3]>>28)]));
            i+=ip;
            o[2]=(unsigned short)((i[0]>>28)|((i[1]>>28)<<4|((i[2]>>28)<<8)|((i[3]>>28)<<12)));
            aerr2+=(Quick8Abs((i[0]>>24)-alpha4to88[(i[0]>>28)])+Quick8Abs((i[1]>>24)-alpha4to88[(i[1]>>28)])+Quick8Abs((i[2]>>24)-alpha4to88[(i[2]>>28)])+Quick8Abs((i[3]>>24)-alpha4to88[(i[3]>>28)]));
            i+=ip;
            o[3]=(unsigned short)((i[0]>>28)|((i[1]>>28)<<4|((i[2]>>28)<<8)|((i[3]>>28)<<12)));
            aerr2+=(Quick8Abs((i[0]>>24)-alpha4to88[(i[0]>>28)])+Quick8Abs((i[1]>>24)-alpha4to88[(i[1]>>28)])+Quick8Abs((i[2]>>24)-alpha4to88[(i[2]>>28)])+Quick8Abs((i[3]>>24)-alpha4to88[(i[3]>>28)]));

            i-=(ip*3);
            o+=4;
            err+=findcolor4x4(o,i,ip);
            o+=4;
            i+=4;
        }
        i+=inadj;
    }

    // handle perfect error (usually this means there's no alpha, btw)
    if (aerr2==0)
    {
        *type=2;
        return(err);
    }

    // now try interpolated
    i=(unsigned int*)input;
    o=(unsigned short*)output;
    for(y=(unsigned int)(height/4);y;y--)
    {
        for(x=(unsigned int)iw;x;x--)
        {
            aerr4+=findalpha4x4(o,((unsigned char*)i)+3,ip*4);

            o+=8;
            i+=4;
        }
        i+=inadj;
    }

    if (aerr4<=aerr2)
    {
        *type=4;
        return(err+aerr4);
    }


    // linear won, so recalc it
    i=(unsigned int*)input;
    o=(unsigned short*)output;
    for(y=(unsigned int)(height/4);y;y--)
    {
        for(x=(unsigned int)iw;x;x--)
        {
            // quantize the alpha down to 4 bits
            o[0]=(unsigned short)((i[0]>>28)|((i[1]>>28)<<4|((i[2]>>28)<<8)|((i[3]>>28)<<12)));
            i+=ip;
            o[1]=(unsigned short)((i[0]>>28)|((i[1]>>28)<<4|((i[2]>>28)<<8)|((i[3]>>28)<<12)));
            i+=ip;
            o[2]=(unsigned short)((i[0]>>28)|((i[1]>>28)<<4|((i[2]>>28)<<8)|((i[3]>>28)<<12)));
            i+=ip;
            o[3]=(unsigned short)((i[0]>>28)|((i[1]>>28)<<4|((i[2]>>28)<<8)|((i[3]>>28)<<12)));

            i-=(ip*3);
            o+=8;
            i+=4;
        }
        i+=inadj;
    }

    *type=2;
    return(err+aerr2);
}


//=========================================================================
// input is S3T1 formatted data - output is 32-bit pixels
//   this does both alpha and non-alpha S3TC1 surfaces
//=========================================================================

void GRANNY
from_S3TC1(unsigned int* outbuf,unsigned short* s3buf, unsigned int width,unsigned int height )
{
    unsigned int x,y,iw,onext,oadj;
    unsigned int* o=outbuf;

    // setup a couple of lookup tables
    setuplookups();

    iw=width/4;
    onext=(width*3)-4;
    oadj=width*3;

    for(y=(height/4);y;y--)
    {
        for(x=iw;x;x--)
        {
            unsigned int c[4];
            unsigned int bits;

            // get the two base colors
            c[0]=s3buf[0];
            c[1]=s3buf[1];

            // expand to 8-bit range
            c[0]=(r5to8[c[0]>>11]     |
                  g6to8[(c[0]>>5)&63] |
                  b5to8[c[0]&31]      |
                  0xff000000);
            c[1]=(r5to8[c[1]>>11]     |
                  g6to8[(c[1]>>5)&63] |
                  b5to8[c[1]&31]      |
                  0xff000000);

            bits=*((unsigned int*)&s3buf[2]);

            // get the interpolated colors
            {
                unsigned int r0 = (c[0] >> (16 + 3)) & 0x1f;
                unsigned int r1 = (c[1] >> (16 + 3)) & 0x1f;
                unsigned int g0 = (c[0] >> ( 8 + 2)) & 0x3f;
                unsigned int g1 = (c[1] >> ( 8 + 2)) & 0x3f;
                unsigned int b0 = (c[0] >> ( 0 + 3)) & 0x1f;
                unsigned int b1 = (c[1] >> ( 0 + 3)) & 0x1f;

                if (c[0]>c[1])
                {
                    c[2] = (r5to8[(2*r0 + r1 + 1) / 3] |
                            g6to8[(2*g0 + g1 + 1) / 3] |
                            b5to8[(2*b0 + b1 + 1) / 3] |
                            0xff000000);
                    c[3] = (r5to8[(r0 + 2*r1 + 1) / 3] |
                            g6to8[(g0 + 2*g1 + 1) / 3] |
                            b5to8[(b0 + 2*b1 + 1) / 3] |
                            0xff000000);
                }
                else
                {
                    c[2] = (r5to8[(r0 + r1) / 2] |
                            g6to8[(g0 + g1) / 2] |
                            b5to8[(b0 + b1) / 2] |
                            0xff000000);

                    // the third color is just transparent, so it doesn't matter
                    c[3]=0;
                }
            }

            // do first row
            o[0]=c[bits&3];
            bits>>=2;
            o[1]=c[bits&3];
            bits>>=2;
            o[2]=c[bits&3];
            bits>>=2;
            o[3]=c[bits&3];
            bits>>=2;
            o+=width;

            // do second row
            o[0]=c[bits&3];
            bits>>=2;
            o[1]=c[bits&3];
            bits>>=2;
            o[2]=c[bits&3];
            bits>>=2;
            o[3]=c[bits&3];
            bits>>=2;
            o+=width;

            // do third row
            o[0]=c[bits&3];
            bits>>=2;
            o[1]=c[bits&3];
            bits>>=2;
            o[2]=c[bits&3];
            bits>>=2;
            o[3]=c[bits&3];
            bits>>=2;
            o+=width;

            // do forth row
            o[0]=c[bits&3];
            bits>>=2;
            o[1]=c[bits&3];
            bits>>=2;
            o[2]=c[bits&3];
            bits>>=2;
            o[3]=c[bits&3];

            s3buf+=4;
            o-=onext;
        }
        o+=oadj;
    }
}



//=========================================================================
// input is S3T2 or S3T3 formatted data - output is 32-bit pixels
//=========================================================================

void GRANNY
from_S3TC2or3(unsigned int* outbuf,unsigned short* s3buf, unsigned int width,unsigned int height )
{
    unsigned int x,y,iw,onext,oadj;
    unsigned int* o=outbuf;

    // setup a couple of lookup tables
    setuplookups();

    iw=width/4;
    onext=(width*3)-4;
    oadj=width*3;

    for(y=(height/4);y;y--)
    {
        for(x=iw;x;x--)
        {
            unsigned int c[4];
            unsigned int bits,alpha;

            alpha=*((unsigned int*)&s3buf[0]);

            // get the two base colors
            c[0]=s3buf[4];
            c[1]=s3buf[5];

            // expand to 8-bit range
            c[0]=r5to8[c[0]>>11] |
                g6to8[(c[0]>>5)&63] |
                b5to8[c[0]&31];
            c[1]=r5to8[c[1]>>11] |
                g6to8[(c[1]>>5)&63] |
                b5to8[c[1]&31];

            bits=*((unsigned int*)&s3buf[6]);

            // get the interpolated colors
            if (c[0]>c[1])
            {
                c[2]=(((((c[0]>>16)&0xff)*2+((c[1]>>16)&0xff))/3)<<16)|
                    (((((c[0]>> 8)&0xff)*2+((c[1]>> 8)&0xff))/3)<<8) |
                    (((((c[0]    )&0xff)*2+((c[1]    )&0xff))/3))    ;
                c[3]=(((((c[1]>>16)&0xff)*2+((c[0]>>16)&0xff))/3)<<16)|
                    (((((c[1]>> 8)&0xff)*2+((c[0]>> 8)&0xff))/3)<<8) |
                    (((((c[1]    )&0xff)*2+((c[0]    )&0xff))/3))    ;
            }
            else
            {
                c[2]=(((((c[0]>>16)&0xff)+((c[1]>>16)&0xff))/2)<<16)|
                    (((((c[0]>> 8)&0xff)+((c[1]>> 8)&0xff))/2)<<8) |
                    (((((c[0]    )&0xff)+((c[1]    )&0xff))/2))    ;
                c[3]=0xffff;
            }

            // first row
            o[0]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o[1]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o[2]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o[3]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o+=width;

            // second row
            o[0]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o[1]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o[2]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o[3]=c[bits&3]|alpha4to8[alpha];
            alpha=*((unsigned int*)&s3buf[2]);
            bits>>=2;
            o+=width;

            // third row
            o[0]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o[1]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o[2]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o[3]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o+=width;

            // forth row
            o[0]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o[1]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o[2]=c[bits&3]|alpha4to8[alpha&15];
            bits>>=2;
            alpha>>=4;
            o[3]=c[bits&3]|alpha4to8[alpha];

            s3buf+=8;
            o-=onext;
        }
        o+=oadj;
    }
}


//=========================================================================
// input is S3T4 or S3T5 formatted data - output is 32-bit pixels
//=========================================================================

void GRANNY
from_S3TC4or5(unsigned int* outbuf,unsigned short* s3buf, unsigned int width,unsigned int height )
{
    unsigned int x,y,iw,onext,oadj;
    unsigned int* o=outbuf;

    // setup a couple of lookup tables
    setuplookups();

    iw=width/4;
    onext=(width*3)-4;
    oadj=width*3;

    for(y=(height/4);y;y--)
    {
        for(x=iw;x;x--)
        {
            unsigned int c[4],a[8];
            unsigned int bits,alpha;

            // get the two base alphas
            a[0]=((unsigned char*)s3buf)[0];
            a[1]=((unsigned char*)s3buf)[1];

            alpha=*((unsigned int*)&s3buf[1]);

            // get the two base colors
            c[0]=s3buf[4];
            c[1]=s3buf[5];

            // expand to 8-bit range
            c[0]=r5to8[c[0]>>11] |
                g6to8[(c[0]>>5)&63] |
                b5to8[c[0]&31] ;
            c[1]=r5to8[c[1]>>11] |
                g6to8[(c[1]>>5)&63] |
                b5to8[c[1]&31] ;

            bits=*((unsigned int*)&s3buf[6]);

            // get the interpolated colors
            if (c[0]>c[1])
            {
                c[2]=(((((c[0]>>16)&0xff)*2+((c[1]>>16)&0xff))/3)<<16)|
                    (((((c[0]>> 8)&0xff)*2+((c[1]>> 8)&0xff))/3)<<8) |
                    (((((c[0]    )&0xff)*2+((c[1]    )&0xff))/3))    ;
                c[3]=(((((c[1]>>16)&0xff)*2+((c[0]>>16)&0xff))/3)<<16)|
                    (((((c[1]>> 8)&0xff)*2+((c[0]>> 8)&0xff))/3)<<8) |
                    (((((c[1]    )&0xff)*2+((c[0]    )&0xff))/3))    ;
            }
            else
            {
                c[2]=(((((c[0]>>16)&0xff)+((c[1]>>16)&0xff))/2)<<16)|
                    (((((c[0]>> 8)&0xff)+((c[1]>> 8)&0xff))/2)<<8) |
                    (((((c[0]    )&0xff)+((c[1]    )&0xff))/2))    ;
                c[3]=0xffff;
            }

            // get the interpolated alphas
            if (a[0]>a[1])
            {
                a[2]=((6*a[0]+a[1])/7)<<24;
                a[3]=((5*a[0]+2*a[1])/7)<<24;
                a[4]=((4*a[0]+3*a[1])/7)<<24;
                a[5]=((3*a[0]+4*a[1])/7)<<24;
                a[6]=((2*a[0]+5*a[1])/7)<<24;
                a[7]=((a[0]+6*a[1])/7)<<24;
            }
            else
            {
                a[2]=((4*a[0]+a[1])/5)<<24;
                a[3]=((3*a[0]+2*a[1])/5)<<24;
                a[4]=((2*a[0]+3*a[1])/5)<<24;
                a[5]=((a[0]+4*a[1])/5)<<24;
                a[6]=0;
                a[7]=(unsigned)255<<24;
            }

            // shift base alphas into position
            a[0]<<=24;
            a[1]<<=24;

            // first row
            o[0]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o[1]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o[2]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o[3]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o+=width;

            // second row
            o[0]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o[1]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o[2]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o[3]=c[bits&3]|a[alpha&7];

            // read the next 16 bits of alpha bits (48 bits total)
            alpha=(alpha>>3)|((unsigned int)s3buf[3])<<8;
            bits>>=2;
            o+=width;

            // third row
            o[0]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o[1]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o[2]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o[3]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o+=width;

            // forth row
            o[0]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o[1]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o[2]=c[bits&3]|a[alpha&7];
            bits>>=2;
            alpha>>=3;
            o[3]=c[bits&3]|a[alpha];

            s3buf+=8;
            o-=onext;
        }
        o+=oadj;
    }
}




//=========================================================================
//  internal routines
//=========================================================================


static void setuplookups(void)
{
    if (!setup)
    {
        int i;
        setup=1;

        // linear alpha convertor
        for(i=0;i<16;i++)
        {
            unsigned int v=(i*255)/15;
            alpha4to88[i]=v;
            alpha4to8[i]=v<<24;
        }

        // red and blue 565 to 888 position converter
        for(i=0;i<32;i++)
        {
            unsigned int v= (i << 3) | (i >> 2);
            r5to8[i]=v<<16;
            b5to8[i]=v;
        }

        // green 565 to 888 position converter
        for(i=0;i<64;i++)
        {
            unsigned int v= (i << 2) | (i >> 4);
            g6to8[i]=v<<8;
        }

        // setup a divide by 3 table
        for (i=0;i<256;i++)
            div3[i]=i/3;

        // setup an 8-bit (9-bit with sign) absolute value table
        for (i=-255;i<0;i++)
            absl[((unsigned int)i)&511]=-i;
        for (;i<256;i++)
            absl[((unsigned int)i)&511]=i;
    }
}

// finds the best colors from two extremes for a single color gradient
#define SEARCHRANGE 4  // defines how far around the end points that we'll search
static void findbest(unsigned char* unique,unsigned int* counts,unsigned int tot,unsigned int* col, unsigned int max)
{
    unsigned int besterr,best;
    int i,j;
    unsigned int p,d1,d2;

    // set the final value
    if ((col[1]-col[0])<3)
    {
        if ((col[1]-col[0])<2)
            return;

        if (col[0]>=(max-3))
        {
            col[0]=max-3;
            col[SEARCHRANGE-1]=max;
        }
        else
        {
            col[SEARCHRANGE-1]=col[0]+3;
        }
    }
    else
        col[SEARCHRANGE-1]=col[1];

    // set the middle values
    for(i=0;i<((SEARCHRANGE-2)/2);i++)
    {
        col[i+1]=col[0]+i+1;
        col[SEARCHRANGE-1-(i+1)]=col[SEARCHRANGE-1]-i-1;
    }

    // setup the other points that we'll try
    besterr=0xffffffff;
    best=1;

    // try all combination of points
    for(i=0;i<SEARCHRANGE;i++)
    {
        for(j=i+1;j<SEARCHRANGE;j++)
        {
            unsigned int err;
            unsigned int c2,c3;

            c2=Quick3Div(col[i]*2+col[j] + 1);
            c3=Quick3Div(col[j]*2+col[i] + 1);

            err=0;
            for(p=0;p<tot;p++)
            {
                unsigned int u=unique[p*4];

                // calculate the differences
                if (u>=c3)
                {
                    d1=u-c3;
                    d2=Quick8Abs(col[j]-u);
                    err+=(((d1<=d2)?d1:d2)*counts[p]);
                }
                else
                {
                    if (u>=c2)
                    {
                        d1=u-c2;
                        d2=c3-u;
                        err+=(((d1<=d2)?d1:d2)*counts[p]);
                    }
                    else
                    {
                        d1=Quick8Abs(col[i]-u);
                        d2=c2-u;
                        err+=(((d1<=d2)?d1:d2)*counts[p]);
                    }
                }

                if (err>=besterr)
                    goto skip;
            }

            if (besterr>err)
            {
                besterr=err;
                best=(i<<16)+j;
                if (besterr==0)
                    goto perfect;
            }
    skip:;
        }
    }
perfect:;

    col[0]=col[best>>16];
    col[1]=col[best&0xffff];
}

static unsigned int getblockerr(unsigned int const* unique,
                                unsigned int const* counts,
                                int tot,
                                unsigned int r1, unsigned int r2,
                                unsigned int g1, unsigned int g2,
                                unsigned int b1, unsigned int b2,
                                unsigned int besterr)
{
    unsigned int err = 0;
    unsigned int r3=Quick3Div(2*r1+r2 + 1);
    unsigned int g3=Quick3Div(2*g1+g2 + 1);
    unsigned int b3=Quick3Div(2*b1+b2 + 1);

    unsigned int r4=Quick3Div(2*r2+r1 + 1);
    unsigned int g4=Quick3Div(2*g2+g1 + 1);
    unsigned int b4=Quick3Div(2*b2+b1 + 1);

    err=0;
    for(int p=0;p<tot;p++)
    {
        unsigned int d1=(Quick8Abs(b1-((unique[p]>> 0)&0xff))+
                         Quick8Abs(g1-((unique[p]>> 8)&0xff))+
                         Quick8Abs(r1-((unique[p]>>16)&0xff)));
        unsigned int d2=(Quick8Abs(b2-((unique[p]>> 0)&0xff))+
                         Quick8Abs(g2-((unique[p]>> 8)&0xff))+
                         Quick8Abs(r2-((unique[p]>>16)&0xff)));
        unsigned int d3=(Quick8Abs(b3-((unique[p]>> 0)&0xff))+
                         Quick8Abs(g3-((unique[p]>> 8)&0xff))+
                         Quick8Abs(r3-((unique[p]>>16)&0xff)));
        unsigned int d4=(Quick8Abs(b4-((unique[p]>> 0)&0xff))+
                         Quick8Abs(g4-((unique[p]>> 8)&0xff))+
                         Quick8Abs(r4-((unique[p]>>16)&0xff)));

        if ((d1<=d2) && (d1<=d3) && (d1<=d4))
        {
            err+=(d1*counts[p]);
        }
        else if ((d2<=d1) && (d2<=d3) && (d2<=d4))
        {
            err+=(d2*counts[p]);
        }
        else if ((d3<=d1) && (d3<=d2) && (d3<=d4))
        {
            err+=(d3*counts[p]);
        }
        else
        {
            err+=(d4*counts[p]);
        }

        if (err >= besterr)
            return 0xffffffff;
    }

    return err;
}


#define COLOR_1(c0, c3) ((2*(c0) + (c3) + 1)/3)
#define COLOR_2(c0, c3) (((c0) + 2*(c3) + 1)/3)

unsigned int find3from012(int c0, int c1, int c2)
{
    int c3 = (3 * c2 - 1 - c0) / 2;

    // Rounding can put this off by one in either direction
    if (c3 >= 0 && (COLOR_1(c0, c3) == c1 && COLOR_2(c0, c3) == c2))
        return c3;
    else if (c3 > 0 && (COLOR_1(c0, c3-1) == c1 && COLOR_2(c0, c3-1) == c2))
        return c3-1;
    else if (c3+1 >= 0 && (COLOR_1(c0, c3+1) == c1 && COLOR_2(c0, c3+1) == c2))
        return c3+1;

    // May not come from an S3 encoding
	return c3 < 0 ? 0 : c3;
}

unsigned int find0from123(int c1, int c2, int c3)
{
    int c0 = (3 * c1 - 1 - c3) / 2;

    // Rounding can put this off by one in either direction
    if (c0 >= 0 && (COLOR_1(c0, c3) == c1 && COLOR_2(c0, c3) == c2))
        return c0;
    else if (c0 > 0 && (COLOR_1(c0-1, c3) == c1 && COLOR_2(c0-1, c3) == c2))
        return c0-1;
    else if (c0+1 >= 0 && (COLOR_1(c0+1, c3) == c1 && COLOR_2(c0+1, c3) == c2))
        return c0+1;

    // May not come from an S3 encoding
	return c0 < 0 ? 0 : c0;
}

// quick search for the best a 4x4 color block.  Returns the error
static unsigned int findcolor4x4(unsigned short* output,const unsigned int* input,unsigned int ip)
{
    unsigned int unique[16];  // unique colors
    unsigned int invert[16];  // all colors stored in inverted order
    unsigned int counts[16];

    unsigned int r[SEARCHRANGE];
    unsigned int g[SEARCHRANGE];
    unsigned int b[SEARCHRANGE];

    unsigned int p,tot;
    int i,j;
    unsigned int best,besterr=0xffffffff;
    unsigned int r1,g1,b1,r2,g2,b2,r3,g3,b3,r4,g4,b4;
    unsigned int ro,go,bo;

    // figure out the unique colors and fill invert
    tot=0;
    best=0;
    for(i=12;i>=0;i-=4)
    {
        for(j=3;j>=0;j--)
        {
            unsigned int v=input[j^3];

#ifdef USING_GL_RGB
            v=(((v>>(0+0+3))&31)<<16) |
                (((v>>(0+8+2))&63)<<8) |
                (((v>>(8+8+3))&31)) ;
#else
            v=(((v>>(0+0+3))&31)) |
                (((v>>(0+8+2))&63)<<8) |
                (((v>>(8+8+3))&31)<<16) ;
#endif

            invert[i+j]=v;

            for(p=0;p<tot;p++)
            {
                if (v==unique[p])
                {
                    counts[p]++;
                    goto skip;
                }
            }

            unique[tot]=v;
            counts[tot]=1;
            ++tot;

    skip:;

        }
        input+=ip;
    }

    // early out on a single color
    if (tot==1)
    {
        best=((unique[0]&0x1f0000)>>5)|((unique[0]&0x3f00)>>3)|(unique[0]&0x1f);
        if (best>0)
        {
            output[0]=(unsigned short)best;
            output[1]=0;
            output[2]=0;
            output[3]=0;
        }
        else
        {
            output[0]=0xffff;
            output[1]=(unsigned short)best;
            output[2]=0x5555;
            output[3]=0x5555;
        }

        return(0);
    }

    // early out on two colors
    if (tot==2)
    {
        if (unique[0]<unique[1])
        {
            p=unique[0];
            unique[0]=unique[1];
            unique[1]=p;
        }

        r1=(unique[0]>>16)&0xff;
        g1=(unique[0]>>8)&0xff;
        b1=(unique[0])&0xff;
        r2=(unique[1]>>16)&0xff;
        g2=(unique[1]>>8)&0xff;
        b2=(unique[1])&0xff;

        tot=0;
        for(p=0;p<16;p++)
        {
            unsigned int d1=(Quick8Abs(b1-(invert[p]&0xff))+
                             Quick8Abs(g1-((invert[p]>>8)&0xff))+
                             Quick8Abs(r1-((invert[p]>>16)&0xff)));
            unsigned int d2=(Quick8Abs(b2-(invert[p]&0xff))+
                             Quick8Abs(g2-((invert[p]>>8)&0xff))+
                             Quick8Abs(r2-((invert[p]>>16)&0xff)));
            if (d1<=d2)
            {
                tot=(tot<<2);
            }
            else
            {
                tot=(tot<<2)|1;
            }
        }

        output[0]=(unsigned short)((r1<<11)|(g1<<5)|(b1));
        output[1]=(unsigned short)((r2<<11)|(g2<<5)|(b2));
        *((unsigned int*)(&output[2]))=tot;

        return(0);
    }

    unsigned int best_r1=0, best_r2=0;
    unsigned int best_g1=0, best_g2=0;
    unsigned int best_b1=0, best_b2=0;

#define COPY_OUT_BEST(err)                      \
    do {                                        \
        best_r1 = r1;                           \
        best_r2 = r2;                           \
        best_g1 = g1;                           \
        best_g2 = g2;                           \
        best_b1 = b1;                           \
        best_b2 = b2;                           \
        besterr = (err);                        \
    } while (false)

    if (tot == 3)
    {
        // Special case for 3 colors.  We may be able to extract this exactly if it was a
        // block encoded already with s3tc
        for (i=0; i < 3*3 && besterr != 0; i++)
        {
            int first_color  = i / 3;
            int second_color = i % 3;
            int other_color = -1;
            if (first_color != 0 && second_color != 0) other_color = 0;
            if (first_color != 1 && second_color != 1) other_color = 1;
            if (first_color != 2 && second_color != 2) other_color = 2;
            Assert(other_color != -1);

            r1=(unique[first_color] >>16)&0xff;
            r2=(unique[second_color]>>16)&0xff;
            ro=(unique[other_color] >>16)&0xff;
            g1=(unique[first_color] >> 8)&0xff;
            g2=(unique[second_color]>> 8)&0xff;
            go=(unique[other_color] >> 8)&0xff;
            b1=(unique[first_color] >> 0)&0xff;
            b2=(unique[second_color]>> 0)&0xff;
            bo=(unique[other_color] >> 0)&0xff;

            if (!((r1>r2) || ((r1==r2) && (g1>g2)) || ((r1==r2) && (g1==g2) && (b1>b2))))
            {
                if (getblockerr(unique, counts, tot,
                                r1, r2, g1, g2, b1, b2, 1) == 0)
                {
                    COPY_OUT_BEST(0); break;
                }
            }

            // Try assuming that first and second color represent the 0 and 2 colors of
            // the 4 possibilities
            // *1, *2 still valid
            r2 = find3from012(r1, ro, r2);
            g2 = find3from012(g1, go, g2);
            b2 = find3from012(b1, bo, b2);
            if (!((r1>r2) || ((r1==r2) && (g1>g2)) || ((r1==r2) && (g1==g2) && (b1>b2))))
            {
                if (r2 < 0x1f && g2 < 0x3f && b2 < 0x1f)
                {
                    if (getblockerr(unique, counts, tot,
                                    r1, r2, g1, g2, b1, b2, 1) == 0)
                    {
                        COPY_OUT_BEST(0); break;
                    }
                }
            }

            // Try assuming that first and second color represent the 1 and 3 colors of
            // the 4 possibilities
            // we overwrote *2 above...
            r2 = (unique[second_color]>>16)&0xff;
            g2 = (unique[second_color]>> 8)&0xff;
            b2 = (unique[second_color]>> 0)&0xff;
            r1 = find0from123(r1, ro, r2);
            g1 = find0from123(g1, go, g2);
            b1 = find0from123(b1, bo, b2);
            if (!((r1>r2) || ((r1==r2) && (g1>g2)) || ((r1==r2) && (g1==g2) && (b1>b2))))
            {
                if (r2 < 0x1f && g2 < 0x3f && b2 < 0x1f)
                {
                    if (getblockerr(unique, counts, tot,
                                    r1, r2, g1, g2, b1, b2, 1) == 0)
                    {
                        COPY_OUT_BEST(0); break;
                    }
                }
            }
        }

    }

    if (tot == 4)
    {
        // Special case for 4 colors.  We may be able to extract this exactly if it was a
        // block encoded already with s3tc.  Just try each of the colors as the extremal
        // values in turn.  Note that we only want to take this if we can find a perfect
        // match.  Otherwise, we should still run the fit process.
        for (i=0; i < 4*4 && besterr != 0; i++)
        {
            int first_color  = i / 4;
            int second_color = i % 4;

            r1=(unique[first_color] >>16)&0xff;
            r2=(unique[second_color]>>16)&0xff;
            g1=(unique[first_color] >> 8)&0xff;
            g2=(unique[second_color]>> 8)&0xff;
            b1=(unique[first_color] >> 0)&0xff;
            b2=(unique[second_color]>> 0)&0xff;

            if (!((r1>r2) || ((r1==r2) && (g1>g2)) || ((r1==r2) && (g1==g2) && (b1>b2))))
            {
                if (getblockerr(unique, counts, tot,
                                r1, r2, g1, g2, b1, b2,
                                1) == 0)
                {
                    COPY_OUT_BEST(0);
                }
            }
        }
    }

    if (besterr != 0)
    {
        // find the mins and maxs
        r[0]=31; r[1]=0;
        g[0]=63; g[1]=0;
        b[0]=31; b[1]=0;
        for(p=0;p<tot;p++)
        {
            best=((unique[p]>>16)&0x1f);
            if (best<r[0])
                r[0]=best;
            if (best>r[1])
                r[1]=best;
            best=((unique[p]>>8)&0x3f);
            if (best<g[0])
                g[0]=best;
            if (best>g[1])
                g[1]=best;
            best=(unique[p]&0x1f);
            if (best<b[0])
                b[0]=best;
            if (best>b[1])
                b[1]=best;
        }

        // find the best end points single dimensionally
        findbest(((unsigned char*)unique)+2,counts,tot,r,0x1f);
        findbest(((unsigned char*)unique)+1,counts,tot,g,0x3f);
        findbest(((unsigned char*)unique),counts,tot,b,0x1f);

        // try all colors in range
        besterr=0xffffffff;

        for(i=0;i<64;i++)
        {
            unsigned int err;

            r1=r[i&1];
            r2=r[(i>>1)&1];
            g1=g[(i>>2)&1];
            g2=g[(i>>3)&1];
            b1=b[(i>>4)&1];
            b2=b[(i>>5)&1];

            if ((r1>r2) || ((r1==r2) && (g1>g2)) || ((r1==r2) && (g1==g2) && (b1>b2)))
                continue;

            err=getblockerr(unique, counts, tot,
                            r1, r2, g1, g2, b1, b2,
                            besterr);

            if (err<besterr)
            {
                COPY_OUT_BEST(err);
                if (err==0)
                {
                    goto perfect;
                }
            }
        }
    }

perfect:

    // finally, calculate the bit pattern
    r2=best_r1;
    r1=best_r2;
    g2=best_g1;
    g1=best_g2;
    b2=best_b1;
    b1=best_b2;

    r3=Quick3Div(2*r1+r2 + 1);
    g3=Quick3Div(2*g1+g2 + 1);
    b3=Quick3Div(2*b1+b2 + 1);

    r4=Quick3Div(2*r2+r1 + 1);
    g4=Quick3Div(2*g2+g1 + 1);
    b4=Quick3Div(2*b2+b1 + 1);

    tot=0;
    for(p=0;p<16;p++)
    {
        unsigned int d1=(Quick8Abs(b1-(invert[p]&0xff))+
                         Quick8Abs(g1-((invert[p]>>8)&0xff))+
                         Quick8Abs(r1-((invert[p]>>16)&0xff)));
        unsigned int d2=(Quick8Abs(b2-(invert[p]&0xff))+
                         Quick8Abs(g2-((invert[p]>>8)&0xff))+
                         Quick8Abs(r2-((invert[p]>>16)&0xff)));
        unsigned int d3=(Quick8Abs(b3-(invert[p]&0xff))+
                         Quick8Abs(g3-((invert[p]>>8)&0xff))+
                         Quick8Abs(r3-((invert[p]>>16)&0xff)));
        unsigned int d4=(Quick8Abs(b4-(invert[p]&0xff))+
                         Quick8Abs(g4-((invert[p]>>8)&0xff))+
                         Quick8Abs(r4-((invert[p]>>16)&0xff)));

        if ((d1<=d2) && (d1<=d3) && (d1<=d4))
        {
            tot=tot<<2;
        }
        else if ((d2<=d1) && (d2<=d3) && (d2<=d4))
        {
            tot=(tot<<2)|1;
        }
        else if ((d3<=d1) && (d3<=d2) && (d3<=d4))
        {
            tot=(tot<<2)|2;
        }
        else
        {
            tot=(tot<<2)|3;
        }
    }

    output[0]=(unsigned short)((r1<<11)|(g1<<5)|b1);
    output[1]=(unsigned short)((r2<<11)|(g2<<5)|b2);
    *((unsigned int*)(&output[2]))=tot;

    return(besterr);
}


// quick search for the best a 4x4 color block with alpha.  Returns the error
static unsigned int findcolor4x4a(unsigned short* output,const unsigned int* input,unsigned int ip)
{
    unsigned int unique[16];  // unique colors
    unsigned int invert[16];  // all colors stored in inverted order
    unsigned int counts[16];

    unsigned int r[SEARCHRANGE];
    unsigned int g[SEARCHRANGE];
    unsigned int b[SEARCHRANGE];

    unsigned int p,tot;
    int i,j,alpha;
    unsigned int best,besterr,aerr;
    unsigned int r1,g1,b1,r2,g2,b2,r3,g3,b3;
    const unsigned int* inp=input;

    // figure out the unique colors and fill invert
    alpha=0;
    aerr=0;
    tot=0;
    best=0;
    unique[0]=0;
    for(i=12;i>=0;i-=4)
    {
        for(j=3;j>=0;j--)
        {
            unsigned int v=inp[j^3];

#ifdef USING_GL_RGB
            v=(((v>>(0+0+3))&31)<<16) |
                (((v>>(0+8+2))&63)<<8) |
                (((v>>(8+8+3))&31)) | (v&0x80000000);
#else
            v=(((v>>(0+0+3))&31)) |
                (((v>>(0+8+2))&63)<<8) |
                (((v>>(8+8+3))&31)<<16) | (v&0x80000000);
#endif

            invert[i+j]=v;

            if ((v&0x80000000)==0)
            {
                aerr+=(v>>24);
                alpha=1;
                goto skip;
            }
            else
                aerr+=(255-(v>>24));

            for(p=0;p<tot;p++)
            {
                if (v==unique[p])
                {
                    counts[p]++;
                    goto skip;
                }
            }

            unique[tot]=v;
            counts[tot]=1;
            ++tot;

    skip:;

        }
        inp+=ip;
    }

    // if there's no alpha - use the standard color version (we get 4 colors then)
    if (!alpha)
        return( findcolor4x4(output,input,ip) );

    // early out on a single color (or no colors)
    if (tot<=1)
    {
        best=((unique[0]&0x1f0000)>>5)|((unique[0]&0x3f00)>>3)|(unique[0]&0x1f);
        if (best>0)
        {
            output[0]=0;
            output[1]=(unsigned short)best;
            j=1;
        }
        else
        {
            output[0]=(unsigned short)best;
            output[1]=0xffff;
            j=0;
        }

        tot=0;
        for(p=0;p<16;p++)
        {
            if (invert[p]&0x80000000)
            {
                tot=(tot<<2)|j;
            }
            else
            {
                tot=(tot<<2)|3;
            }
        }
        *((unsigned int*)(&output[2]))=tot;

        return(aerr);
    }

    // early out on two colors
    if (tot==2)
    {

        if (unique[0]>unique[1])
        {
            p=unique[0];
            unique[0]=unique[1];
            unique[1]=p;
        }

        r1=(unique[0]>>16)&0xff;
        g1=(unique[0]>>8)&0xff;
        b1=(unique[0])&0xff;
        r2=(unique[1]>>16)&0xff;
        g2=(unique[1]>>8)&0xff;
        b2=(unique[1])&0xff;

        tot=0;
        for(p=0;p<16;p++)
        {
            if ((invert[p]&0x80000000)==0)
            {
                tot=(tot<<2)|3;
            }
            else
            {
                unsigned int d1=(Quick8Abs(b1-(invert[p]&0xff))+
                                 Quick8Abs(g1-((invert[p]>>8)&0xff))+
                                 Quick8Abs(r1-((invert[p]>>16)&0xff)));
                unsigned int d2=(Quick8Abs(b2-(invert[p]&0xff))+
                                 Quick8Abs(g2-((invert[p]>>8)&0xff))+
                                 Quick8Abs(r2-((invert[p]>>16)&0xff)));
                if (d1<=d2)
                {
                    tot=(tot<<2);
                }
                else
                {
                    tot=(tot<<2)|1;
                }
            }
        }

        output[0]=(unsigned short)((r1<<11)|(g1<<5)|(b1));
        output[1]=(unsigned short)((r2<<11)|(g2<<5)|(b2));
        *((unsigned int*)(&output[2]))=tot;

        return(aerr);
    }

    // find the mins and maxs
    r[0]=31; r[1]=0;
    g[0]=63; g[1]=0;
    b[0]=31; b[1]=0;
    for(p=0;p<tot;p++)
    {
        best=((unique[p]>>16)&0x1f);
        if (best<r[0])
            r[0]=best;
        if (best>r[1])
            r[1]=best;
        best=((unique[p]>>8)&0x3f);
        if (best<g[0])
            g[0]=best;
        if (best>g[1])
            g[1]=best;
        best=(unique[p]&0x1f);
        if (best<b[0])
            b[0]=best;
        if (best>b[1])
            b[1]=best;
    }

    // build ranges single dimensionally
    findbest(((unsigned char*)unique)+2,counts,tot,r,0x1f);
    findbest(((unsigned char*)unique)+1,counts,tot,g,0x3f);
    findbest(((unsigned char*)unique),counts,tot,b,0x1f);

    // try all colors in range
    besterr=0xffffffff;

    for(i=0;i<64;i++)
    {
        unsigned int err;

        r1=r[i&1];
        r2=r[(i>>1)&1];
        g1=g[(i>>2)&1];
        g2=g[(i>>3)&1];
        b1=b[(i>>4)&1];
        b2=b[(i>>5)&1];

        r3=(r1+r2)/2;
        g3=(g1+g2)/2;
        b3=(b1+b2)/2;

        if ((r1>r2) || ((r1==r2) && (g1>g2)) || ((r1==r2) && (g1==g2) && (b1>b2)))
            continue;

        err=0;
        for(p=0;p<tot;p++)
        {
            unsigned int d1=(Quick8Abs(b1-(unique[p]&0xff))+
                             Quick8Abs(g1-((unique[p]>>8)&0xff))+
                             Quick8Abs(r1-((unique[p]>>16)&0xff)));
            unsigned int d2=(Quick8Abs(b2-(unique[p]&0xff))+
                             Quick8Abs(g2-((unique[p]>>8)&0xff))+
                             Quick8Abs(r2-((unique[p]>>16)&0xff)));
            unsigned int d3=(Quick8Abs(b3-(unique[p]&0xff))+
                             Quick8Abs(g3-((unique[p]>>8)&0xff))+
                             Quick8Abs(r3-((unique[p]>>16)&0xff)));

            if ((d1<=d2) && (d1<=d3))
            {
                err+=(d1*counts[p]);
            }
            else if ((d2<=d1) && (d2<=d3))
            {
                err+=(d2*counts[p]);
            }
            else
            {
                err+=(d3*counts[p]);
            }

            if (err>=besterr)
                goto next;
        }

        if (err<besterr)
        {
            besterr=err;
            best=i;
            if (err==0)
            {
                goto perfect;
            }
        }
next:;
    }

perfect:

    // finally, calculate the bit pattern

    r1=r[best&1];
    r2=r[(best>>1)&1];
    g1=g[(best>>2)&1];
    g2=g[(best>>3)&1];
    b1=b[(best>>4)&1];
    b2=b[(best>>5)&1];

    r3=(r1+r2)/2;
    g3=(g1+g2)/2;
    b3=(b1+b2)/2;

    tot=0;
    for(p=0;p<16;p++)
    {
        if ((invert[p]&0x80000000)==0)
        {
            tot=(tot<<2)|3;
        }
        else
        {
            unsigned int d1=(Quick8Abs(b1-(invert[p]&0xff))+
                             Quick8Abs(g1-((invert[p]>>8)&0xff))+
                             Quick8Abs(r1-((invert[p]>>16)&0xff)));
            unsigned int d2=(Quick8Abs(b2-(invert[p]&0xff))+
                             Quick8Abs(g2-((invert[p]>>8)&0xff))+
                             Quick8Abs(r2-((invert[p]>>16)&0xff)));
            unsigned int d3=(Quick8Abs(b3-(invert[p]&0xff))+
                             Quick8Abs(g3-((invert[p]>>8)&0xff))+
                             Quick8Abs(r3-((invert[p]>>16)&0xff)));

            if ((d1<=d2) && (d1<=d3))
            {
                tot=tot<<2;
            }
            else if ((d2<=d1) && (d2<=d3))
            {
                tot=(tot<<2)|1;
            }
            else
            {
                tot=(tot<<2)|2;
            }
        }
    }

    output[0]=(unsigned short)((r1<<11)|(g1<<5)|b1);
    output[1]=(unsigned short)((r2<<11)|(g2<<5)|b2);
    *((unsigned int*)(&output[2]))=tot;

    return(besterr+aerr);
}


static unsigned int findalpha4x4(unsigned short* output,const unsigned char* input,unsigned int ip)
{
    unsigned char unique[16];  // unique colors
    unsigned char invert[16];  // all colors stored in inverted order
    unsigned char counts[16];

    unsigned int amin,amax,atop,mmin,mmax;
    unsigned int p,tot,d1,d2,pv;
    int i,j;
    unsigned int best,besterr,a1,a2;
    unsigned int c[8];

    // figure out the unique colors and fill invert
    tot=0;
    best=0;
    amin=255;
    amax=0;
    mmin=255;
    mmax=0;
    unique[0]=0;
    for(i=12;i>=0;i-=4)
    {
        for(j=3;j>=0;j--)
        {
            unsigned char v=input[(j^3)*4];

            invert[i+j]=v;

            // mins and maxs
            if (v<amin)
                amin=v;
            if (v>amax)
                amax=v;

            // mins and maxs excluding 0,1,254,255
            if (v>1)
                if (v<mmin)
                    mmin=v;
            if (v<254)
                if (v>mmax)
                    mmax=v;

            for(p=0;p<tot;p++)
            {
                if (v==unique[p])
                {
                    counts[p]++;
                    goto skip;
                }
            }

            unique[tot]=v;
            counts[tot]=1;
            ++tot;

    skip:;

        }
        input+=ip;
    }

    // early out on a single color (or no colors)
    if (tot<=1)
    {
        ((unsigned char*)output)[0]=unique[0];
        ((unsigned char*)output)[1]=0;
        output[1]=0;
        output[2]=0;
        output[3]=0;
        return(0);
    }

    // early out on two colors
    if (tot==2)
    {
        ((unsigned char*)output)[0]=unique[0];
        ((unsigned char*)output)[1]=unique[1];

        tot=0;
        for(p=0;p<16;p++)
        {
            unsigned int d1=Quick8Abs((unsigned int)unique[0]-(unsigned int)invert[p]);
            unsigned int d2=Quick8Abs((unsigned int)unique[1]-(unsigned int)invert[p]);
            if (d1<=d2)
            {
                tot=(tot<<3);
            }
            else
            {
                tot=(tot<<3)|1;
            }

            if (p==7)
            {
                ((unsigned char*)output)[5]=(unsigned char)tot;
                output[3]=(unsigned short)(tot>>8);
            }
        }

        output[1]=(unsigned short)tot;
        ((unsigned char*)output)[4]=(unsigned char)(tot>>16);

        return(0);
    }

    besterr=0xffffffff;

    // try the 8 level block

    // start one below the min
    if (amin>=1)
    {
        amin--;
        if (amin>=254)
            atop=255;
        else
            atop=amin+2;
    } else
        atop=1;


    // try one below and one over
    for(a1=amin;a1<=atop;a1++)
    {
        a2=1;
        do
        {
            unsigned int err;

            c[0]=a1;
            c[7]=a1+(a2*4); // use a range of 4 times a2 (about half of 7)

            if (c[7]>255)
                goto next;

            c[1]=(6*c[0]+c[7])/7;
            c[2]=(5*c[0]+2*c[7])/7;
            c[3]=(4*c[0]+3*c[7])/7;
            c[4]=(3*c[0]+4*c[7])/7;
            c[5]=(2*c[0]+5*c[7])/7;
            c[6]=(c[0]+6*c[7])/7;

            err=0;
            for(p=0;p<tot;p++)
            {
                pv=unique[p];

                // binary search to find the closest of the 8 alpha
                if (pv>=c[4])
                {
                    if (pv>=c[6])
                    {
                        d1=pv-c[6];
                        d2=Quick8Abs(c[7]-pv);
                    }
                    else
                    {
                        if (pv>=c[5])
                        {
                            d1=pv-c[5];
                            d2=c[6]-pv;
                        }
                        else
                        {
                            d1=pv-c[4];
                            d2=c[5]-pv;
                        }
                    }
                }
                else
                {
                    if (pv>=c[2])
                    {
                        if (pv>=c[3])
                        {
                            d1=pv-c[3];
                            d2=c[4]-pv;
                        }
                        else
                        {
                            d1=pv-c[2];
                            d2=c[3]-pv;
                        }
                    }
                    else
                    {
                        if (pv>=c[1])
                        {
                            d1=pv-c[1];
                            d2=c[2]-pv;
                        }
                        else
                        {
                            d1=Quick8Abs(c[0]-pv);
                            d2=c[1]-pv;
                        }
                    }
                }
                err+=((d1<=d2)?d1:d2);

                if (err>=besterr)
                    goto next;
            }
            if (besterr>err)
            {
                besterr=err;
                best=(c[0]<<8)|c[7];
                if (err==0)
                    goto perfect;
            }
    next:
            a2++;
        } while (c[7]<=amax);
    }

    // see if 6 level alpha is better
    if ((besterr) && (mmin<mmax))
    {

        // start one below the min
        if (mmin>=1)
        {
            mmin--;
            if (mmin>=254)
                atop=255;
            else
                atop=mmin+2;
        } else
            atop=1;

        c[0]=0;
        c[7]=255;
        for(a1=mmin;a1<=atop;a1++)
        {
            a2=1;
            do
            {
                unsigned int err;

                c[1]=a1;
                c[6]=a1+(a2*4); // use a range of 3 times a2 (half of 6)

                if (c[6]>255)
                    goto next2;

                c[2]=(4*c[1]+c[6])/5;
                c[3]=(3*c[1]+2*c[6])/5;
                c[4]=(2*c[1]+3*c[6])/5;
                c[5]=(c[1]+4*c[6])/5;

                err=0;
                for(p=0;p<tot;p++)
                {
                    pv=unique[p];

                    // binary search to find the closest alpha
                    if (pv>=c[4])
                    {
                        if (pv>=c[6])
                        {
                            d1=pv-c[6];
                            d2=Quick8Abs(c[7]-pv);
                        }
                        else
                        {
                            if (pv>=c[5])
                            {
                                d1=pv-c[5];
                                d2=c[6]-pv;
                            }
                            else
                            {
                                d1=pv-c[4];
                                d2=c[5]-pv;
                            }
                        }
                    }
                    else
                    {
                        if (pv>=c[2])
                        {
                            if (pv>=c[3])
                            {
                                d1=pv-c[3];
                                d2=c[4]-pv;
                            }
                            else
                            {
                                d1=pv-c[2];
                                d2=c[3]-pv;
                            }
                        }
                        else
                        {
                            if (pv>=c[1])
                            {
                                d1=pv-c[1];
                                d2=c[2]-pv;
                            }
                            else
                            {
                                d1=Quick8Abs(c[0]-pv);
                                d2=c[1]-pv;
                            }
                        }
                    }
                    err+=((d1<=d2)?d1:d2);

                    if (err>=besterr)
                        goto next2;
                }
                if (besterr>err)
                {
                    besterr=err;
                    best=(c[6]<<8)|c[1];
                    if (err==0)
                        goto perfect;
                }
        next2:
                a2++;
            } while (c[6]<=mmax);
        }


    }
perfect:

    c[0]=best&0xff;
    c[1]=(best>>8)&0xff;

    // save the base colors
    ((unsigned char*)output)[0]=(unsigned char)(c[0]);
    ((unsigned char*)output)[1]=(unsigned char)(c[1]);

    if (c[0]>=c[1])
    {
        // do 8 level alpha
        c[2]=(6*c[0]+c[1])/7;
        c[3]=(5*c[0]+2*c[1])/7;
        c[4]=(4*c[0]+3*c[1])/7;
        c[5]=(3*c[0]+4*c[1])/7;
        c[6]=(2*c[0]+5*c[1])/7;
        c[7]=(c[0]+6*c[1])/7;

        tot=0;
        for(p=0;p<16;p++)
        {
            pv=invert[p];

            // binary search to find the closest (the colors are out of index order, though)
            if (pv>=c[4])
            {
                if (pv>=c[2])
                {
                    d1=pv-c[2];
                    d2=Quick8Abs(c[0]-pv);
                    tot=(tot<<3)|((d1<=d2)?2:0);
                }
                else
                {
                    if (pv>=c[3])
                    {
                        d1=pv-c[3];
                        d2=c[2]-pv;
                        tot=(tot<<3)|((d1<=d2)?3:2);
                    }
                    else
                    {
                        d1=pv-c[4];
                        d2=c[3]-pv;
                        tot=(tot<<3)|((d1<=d2)?4:3);
                    }
                }
            }
            else
            {
                if (pv>=c[6])
                {
                    if (pv>=c[5])
                    {
                        d1=pv-c[5];
                        d2=c[4]-pv;
                        tot=(tot<<3)|((d1<=d2)?5:4);
                    }
                    else
                    {
                        d1=pv-c[6];
                        d2=c[5]-pv;
                        tot=(tot<<3)|((d1<=d2)?6:5);
                    }
                }
                else
                {
                    if (pv>=c[7])
                    {
                        d1=pv-c[7];
                        d2=c[6]-pv;
                        tot=(tot<<3)|((d1<=d2)?7:6);
                    }
                    else
                    {
                        d1=Quick8Abs(c[1]-pv);
                        d2=c[7]-pv;
                        tot=(tot<<3)|((d1<=d2)?1:7);
                    }
                }
            }

            if (p==7)
            {
                ((unsigned char*)output)[5]=(unsigned char)tot;
                output[3]=(unsigned short)(tot>>8);
            }
        }

        output[1]=(unsigned short)tot;
        ((unsigned char*)output)[4]=(unsigned char)(tot>>16);

    }
    else
    {
        // do 6 level alpha
        c[2]=(4*c[0]+c[1])/5;
        c[3]=(3*c[0]+2*c[1])/5;
        c[4]=(2*c[0]+3*c[1])/5;
        c[5]=(c[0]+4*c[1])/5;
        c[6]=0;
        c[7]=255;

        tot=0;
        for(p=0;p<16;p++)
        {
            pv=invert[p];

            // binary search to find the closest (the colors are out of index order, though)
            if (pv>=c[4])
            {
                if (pv>=c[1])
                {
                    d1=pv-c[1];
                    d2=Quick8Abs(c[7]-pv);
                    tot=(tot<<3)|((d1<=d2)?1:7);
                }
                else
                {
                    if (pv>=c[5])
                    {
                        d1=pv-c[5]-pv;
                        d2=c[1]-pv;
                        tot=(tot<<3)|((d1<=d2)?5:1);
                    }
                    else
                    {
                        d1=pv-c[4]-pv;
                        d2=c[5]-pv;
                        tot=(tot<<3)|((d1<=d2)?4:5);
                    }
                }
            }
            else
            {
                if (pv>=c[2])
                {
                    if (pv>=c[3])
                    {
                        d1=pv-c[3];
                        d2=c[4]-pv;
                        tot=(tot<<3)|((d1<=d2)?3:4);
                    }
                    else
                    {
                        d1=pv-c[2];
                        d2=c[3]-pv;
                        tot=(tot<<3)|((d1<=d2)?2:3);
                    }
                }
                else
                {
                    if (pv>=c[0])
                    {
                        d1=pv-c[0];
                        d2=c[2]-pv;
                        tot=(tot<<3)|((d1<=d2)?0:2);
                    }
                    else
                    {
                        d1=Quick8Abs(c[6]-pv);
                        d2=c[0]-pv;
                        tot=(tot<<3)|((d1<=d2)?6:0);
                    }
                }
            }

            if (p==7)
            {
                ((unsigned char*)output)[5]=(unsigned char)tot;
                output[3]=(unsigned short)(tot>>8);
            }
        }

        output[1]=(unsigned short)tot;
        ((unsigned char*)output)[4]=(unsigned char)(tot>>16);

    }

    return(besterr);
}

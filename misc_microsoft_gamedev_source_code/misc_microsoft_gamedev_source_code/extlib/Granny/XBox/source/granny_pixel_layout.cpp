// ========================================================================
// $File: //jeffr/granny/rt/granny_pixel_layout.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_PIXEL_LAYOUT_H)
#include "granny_pixel_layout.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition PixelLayoutType[] =
{
    {Int32Member, "BytesPerPixel"},
    {Int32Member, "ShiftForComponent", 0, 4},
    {Int32Member, "BitsForComponent", 0, 4},

    {EndMember},
};

pixel_layout const RGB555PixelFormat =
{
    2, {0, 5, 10, 0}, {5, 5, 5, 0},
};

pixel_layout const RGB565PixelFormat =
{
    2, {0, 5, 11, 0}, {5, 6, 5, 0},
};

pixel_layout const RGBA5551PixelFormat =
{
    2, {0, 5, 10, 15}, {5, 5, 5, 1},
};

pixel_layout const RGBA4444PixelFormat =
{
    2, {0, 4, 8, 12}, {4, 4, 4, 4},
};

pixel_layout const RGB888PixelFormat =
{
    3, {0, 8, 16, 0}, {8, 8, 8, 0},
};

pixel_layout const RGBA8888PixelFormat =
{
    4, {0, 8, 16, 24}, {8, 8, 8, 8},
};

pixel_layout const ARGB8888PixelFormat =
{
    4, {8, 16, 24, 0}, {8, 8, 8, 8},
};

pixel_layout const BGR555PixelFormat =
{
    2, {10, 5, 0, 0}, {5, 5, 5, 0},
};

pixel_layout const BGR565PixelFormat =
{
    2, {11, 5, 0, 0}, {5, 6, 5, 0},
};

pixel_layout const BGRA5551PixelFormat =
{
    2, {10, 5, 0, 15}, {5, 5, 5, 1},
};

pixel_layout const BGRA4444PixelFormat =
{
    2, {8, 4, 0, 12}, {4, 4, 4, 4},
};

pixel_layout const BGR888PixelFormat =
{
    3, {16, 8, 0, 0}, {8, 8, 8, 0},
};

pixel_layout const BGRA8888PixelFormat =
{
    4, {16, 8, 0, 24}, {8, 8, 8, 8},
};

END_GRANNY_NAMESPACE;

bool GRANNY
PixelLayoutsAreEqual(pixel_layout const &Operand0,
                     pixel_layout const &Operand1)
{
    return((Operand0.BytesPerPixel == Operand1.BytesPerPixel) &&
           (Operand0.ShiftForComponent[0] == Operand1.ShiftForComponent[0]) &&
           (Operand0.ShiftForComponent[1] == Operand1.ShiftForComponent[1]) &&
           (Operand0.ShiftForComponent[2] == Operand1.ShiftForComponent[2]) &&
           (Operand0.ShiftForComponent[3] == Operand1.ShiftForComponent[3]) &&
           (Operand0.BitsForComponent[0] == Operand1.BitsForComponent[0]) &&
           (Operand0.BitsForComponent[1] == Operand1.BitsForComponent[1]) &&
           (Operand0.BitsForComponent[2] == Operand1.BitsForComponent[2]) &&
           (Operand0.BitsForComponent[3] == Operand1.BitsForComponent[3]));
}

bool GRANNY
PixelLayoutHasAlpha(pixel_layout const &Layout)
{
    return(Layout.BitsForComponent[3] != 0);
}

void GRANNY
SetStockSpecification(pixel_layout &Layout,
                      int32 const *BitsForComponent,
                      int32 const *ComponentPlacement)
{
    uint32 ComponentSum = 0;
    {for(int32x ComponentIndexIndex = 0;
         ComponentIndexIndex < 4;
         ++ComponentIndexIndex)
    {
        uint32 const ComponentIndex = ComponentPlacement[ComponentIndexIndex];

        Layout.BitsForComponent[ComponentIndex] =
            BitsForComponent[ComponentIndex];
        Layout.ShiftForComponent[ComponentIndex] = ComponentSum;

        ComponentSum += BitsForComponent[ComponentIndex];
    }}

    Layout.BytesPerPixel = (ComponentSum + 7) & ~7;
    Assert((Layout.BytesPerPixel % 8) == 0);
    Layout.BytesPerPixel /= 8;
}

void GRANNY
SetStockRGBASpecification(pixel_layout &Layout,
                          int32x RedBits, int32x GreenBits,
                          int32x BlueBits, int32x AlphaBits)
{
    int32x const BitArray[4] = {RedBits, GreenBits, BlueBits, AlphaBits};
    int32x const PlacementArray[4] = {0, 1, 2, 3};
    SetStockSpecification(Layout, BitArray, PlacementArray);
}

void GRANNY
SetStockBGRASpecification(pixel_layout &Layout,
                          int32x RedBits, int32x GreenBits,
                          int32x BlueBits, int32x AlphaBits)
{
    int32x const BitArray[4] = {RedBits, GreenBits, BlueBits, AlphaBits};
    int32x const PlacementArray[4] = {2, 1, 0, 3};
    SetStockSpecification(Layout, BitArray, PlacementArray);
}

static void
Swap(int32 A, int32 B)
{
    int32 const Temp = A;

    A = B;
    B = Temp;
}

void GRANNY
SwapRGBAToBGRA(pixel_layout &Layout)
{
    Swap(Layout.ShiftForComponent[0], Layout.ShiftForComponent[2]);
    Swap(Layout.BitsForComponent[0], Layout.BitsForComponent[2]);
}

static uint32
GenerateMask(int32x const FirstBit,
             int32x const OnePastLastBit)
{
    // Build a mask that has a consecutive batch of 1's in some part
    // of the word.
    uint32 Mask = 0;
    {for(int32x BitIndex = FirstBit;
         BitIndex < OnePastLastBit;
         ++BitIndex)
    {
        Mask |= 1 << BitIndex;
    }}

    return(Mask);
}

static uint32
ShiftMore(uint32 const Value, int32 const Shift)
{
    return((Shift > 0) ? (Value<<Shift) : (Value>>(-Shift)));
}

static uint32
ConvertPixel(int32x *Shifts, uint32 *Masks,
             int32x InBytesPerPixel, int32x OutBytesPerPixel,
             uint32 Pixel)
{
    if(PROCESSOR_BIG_ENDIAN)
    {
        Pixel = Reverse32(Pixel);
    }

    Pixel = ((ShiftMore(Pixel, Shifts[0]) & Masks[0]) +
             (ShiftMore(Pixel, Shifts[1]) & Masks[1]) +
             (ShiftMore(Pixel, Shifts[2]) & Masks[2]) +
             (ShiftMore(Pixel, Shifts[3]) & Masks[3]));

    if(PROCESSOR_BIG_ENDIAN)
    {
        if(OutBytesPerPixel > 2)
        {
            Pixel = Reverse32(Pixel);
        }
        else
        {
            Pixel <<= 16;
        }
    }

    return(Pixel);
}

void GRANNY
ConvertPixelFormat(int32x Width, int32x Height,
                   pixel_layout const &SourceLayout,
                   int32x SourceStride, void const *SourceBitsInit,
                   pixel_layout const &DestLayout,
                   int32x DestStride, void *DestBitsInit)
{
    uint8 const *SourceBits = (uint8 const *)SourceBitsInit;
    uint8 *DestBits = (uint8 *)DestBitsInit;

    // Determine the shifts and masks
    int32x Shifts[4];
    uint32 Masks[4];
    {for(int32x ComponentIndex = 0;
         ComponentIndex < 4;
         ++ComponentIndex)
    {
        int32x const FromShift = SourceLayout.ShiftForComponent[ComponentIndex];
        int32x const FromCount = SourceLayout.BitsForComponent[ComponentIndex];

        int32x const ToShift = DestLayout.ShiftForComponent[ComponentIndex];
        int32x const ToCount = DestLayout.BitsForComponent[ComponentIndex];

        // Determine the shift
        int32x const RelativeShift = (ToShift + ToCount) - (FromShift + FromCount);

        // Determine the mask
        int32x const RelativeFromShift = FromShift + RelativeShift;
        uint32 Mask = 0;
        if(ToCount > 0)
        {
            Mask = GenerateMask((RelativeFromShift > ToShift) ?
                                RelativeFromShift : ToShift,
                                RelativeFromShift + FromCount);
        }

        // Store the shift and mask
        Shifts[ComponentIndex] = RelativeShift;
        Masks[ComponentIndex] = Mask;
    }}

    int32x InBytesPerPixel = SourceLayout.BytesPerPixel;
    int32x OutBytesPerPixel = DestLayout.BytesPerPixel;

    // Prepare the iterators
    int32x const SourceGap = SourceStride - (Width * SourceLayout.BytesPerPixel);
    int32x const DestGap = DestStride - (Width * DestLayout.BytesPerPixel);

    // First, copy everything but the last scanline
    {for(int32x Y = Height;
         Y;
         --Y)
    {
        // Copy everything in the last scan line but the last pixel
        {for(int32x X = (Width - 1);
             X;
             --X)
        {
            // Get the source pixel and advance the source pointer
            uint32 const Pixel = *((uint32*)SourceBits);
            SourceBits += SourceLayout.BytesPerPixel;

            // Write the destination pixel
            *((uint32*)DestBits) = ConvertPixel(Shifts, Masks,
                                                InBytesPerPixel,
                                                OutBytesPerPixel,
                                                Pixel);

            // Advance the destination pointer
            DestBits += DestLayout.BytesPerPixel;
        }}

        // Copy the last pixel a byte at a time.  This is necessary
        // because otherwise we might over-read or over-write by one or two
        // bytes, depending on the bytes-per-pixel of the source and dest.
        uint32 Pixel = 0;
        uint8 *BytePointer = (uint8 *)&Pixel;
        int32x BytesPerPixel = SourceLayout.BytesPerPixel;
        while(BytesPerPixel--)
        {
            *BytePointer++ = *SourceBits++;
        }

        uint32 DestTemp = ConvertPixel(Shifts, Masks,
                                       InBytesPerPixel,
                                       OutBytesPerPixel,
                                       Pixel);

        BytePointer = (uint8 *)&DestTemp;
        BytesPerPixel = DestLayout.BytesPerPixel;
        while(BytesPerPixel--)
        {
            *DestBits++ = *BytePointer++;
        }

        // Account for the difference in the strides and the widths
        SourceBits += SourceGap;
        DestBits += DestGap;
    }}
}

//
// assumes ARGB linear in memory as: A, R, B, G (this is the fastest one)
//
void GRANNY
ARGB8888SwizzleNGC(uint32 Width, uint32 Height, uint32 SourceStride,
                   void *SourceBits, void *DestBits)
{
    uint32 * rgb0 = (uint32*) SourceBits;
    uint32 * rgb1 = (uint32*) ( ( (uint8*) SourceBits ) + SourceStride );
    uint32 * rgb2 = (uint32*) ( ( (uint8*) SourceBits ) + SourceStride + SourceStride );
    uint32 * rgb3 = (uint32*) ( ( (uint8*) SourceBits ) + SourceStride + SourceStride + SourceStride );
    uint32 * ngc = (uint32*) DestBits;
    uint32 adj = ( SourceStride * 4 ) - ( Width * 4 );
    uint32 y;

    for( y = ( Height / 4 ) ; y-- ; )
    {
        uint32 x;
        for( x = ( Width / 4 ) ; x-- ; )
        {
            register uint32 p00;
            register uint32 p01;
            register uint32 p02;
            register uint32 p03;
            register uint32 p10;
            register uint32 p11;
            register uint32 p12;
            register uint32 p13;
            register uint32 p20;
            register uint32 p21;
            register uint32 p22;
            register uint32 p23;
            register uint32 p30;
            register uint32 p31;
            register uint32 p32;
            register uint32 p33;

            register uint32 out0;
#if 1
#define out8 p00
#define out1 p01
#define out9 p02
#define out2 p03
#define outa p10
#define out3 p11
#define outb p12
#define out4 p13
#define outc p20
#define out5 p21
#define outd p22
#define out6 p23
#define oute p30
#define out7 p31
#define outf p32
#else
            register uint32 out8;
            register uint32 out1;
            register uint32 out9;
            register uint32 out2;
            register uint32 outa;
            register uint32 out3;
            register uint32 outb;
            register uint32 out4;
            register uint32 outc;
            register uint32 out5;
            register uint32 outd;
            register uint32 out6;
            register uint32 oute;
            register uint32 out7;
            register uint32 outf;
#endif

            p00 = rgb0[ 0 ];
            p01 = rgb0[ 1 ];
            p02 = rgb0[ 2 ];
            p03 = rgb0[ 3 ];

            p10 = rgb1[ 0 ];
            p11 = rgb1[ 1 ];
            p12 = rgb1[ 2 ];
            p13 = rgb1[ 3 ];

            p20 = rgb2[ 0 ];
            p21 = rgb2[ 1 ];
            p22 = rgb2[ 2 ];
            p23 = rgb2[ 3 ];

            p30 = rgb3[ 0 ];
            p31 = rgb3[ 1 ];
            p32 = rgb3[ 2 ];
            p33 = rgb3[ 3 ];

            out0 = ( p00 & 0xffff0000 ) | ( ( p01 >> 16 ) & 0xffff );
            out8 = ( ( p00 << 16 ) & 0xffff0000 ) | ( ( p01 ) & 0xffff );

            out1 = ( p02 & 0xffff0000 ) | ( ( p03 >> 16 ) & 0xffff );
            out9 = ( ( p02 << 16 ) & 0xffff0000 ) | ( ( p03 ) & 0xffff );

            out2 = ( p10 & 0xffff0000 ) | ( ( p11 >> 16 ) & 0xffff );
            outa = ( ( p10 << 16 ) & 0xffff0000 ) | ( ( p11 ) & 0xffff );

            out3 = ( p12 & 0xffff0000 ) | ( ( p13 >> 16 ) & 0xffff );
            outb = ( ( p12 << 16 ) & 0xffff0000 ) | ( ( p13 ) & 0xffff );

            out4 = ( p20 & 0xffff0000 ) | ( ( p21 >> 16 ) & 0xffff );
            outc = ( ( p20 << 16 ) & 0xffff0000 ) | ( ( p21 ) & 0xffff );

            out5 = ( p22 & 0xffff0000 ) | ( ( p23 >> 16 ) & 0xffff );
            outd = ( ( p22 << 16 ) & 0xffff0000 ) | ( ( p23 ) & 0xffff );

            out6 = ( p30 & 0xffff0000 ) | ( ( p31 >> 16 ) & 0xffff );
            oute = ( ( p30 << 16 ) & 0xffff0000 ) | ( ( p31 ) & 0xffff );

            out7 = ( p32 & 0xffff0000 ) | ( ( p33 >> 16 ) & 0xffff );
            outf = ( ( p32 << 16 ) & 0xffff0000 ) | ( ( p33 ) & 0xffff );

#if 1
            ngc[  0 ] = out0;
            ngc[  1 ] = out1;
            ngc[  2 ] = out2;
            ngc[  3 ] = out3;
            ngc[  4 ] = out4;
            ngc[  5 ] = out5;
            ngc[  6 ] = out6;
            ngc[  7 ] = out7;
            ngc[  8 ] = out8;
            ngc[  9 ] = out9;
            ngc[ 10 ] = outa;
            ngc[ 11 ] = outb;
            ngc[ 12 ] = outc;
            ngc[ 13 ] = outd;
            ngc[ 14 ] = oute;
            ngc[ 15 ] = outf;
#else
            ngc[  0 ] =
                ngc[  1 ] =
                ngc[  2 ] =
                ngc[  3 ] =
                ngc[  4 ] =
                ngc[  5 ] =
                ngc[  6 ] =
                ngc[  7 ] = 0xFF00FF00;
            //                AARRAARR
            ngc[  8 ] =
                ngc[  9 ] =
                ngc[ 10 ] =
                ngc[ 11 ] =
                ngc[ 12 ] =
                ngc[ 13 ] =
                ngc[ 14 ] =
                ngc[ 15 ] = 0xFF00FF00;
            //                GGBBGGBB
#endif

            rgb0 += 4;
            rgb1 += 4;
            rgb2 += 4;
            rgb3 += 4;
            ngc += 16;
        }
        rgb0 = (uint32*) ( ( (uint8*) rgb0 ) + adj );
        rgb1 = (uint32*) ( ( (uint8*) rgb1 ) + adj );
        rgb2 = (uint32*) ( ( (uint8*) rgb2 ) + adj );
        rgb3 = (uint32*) ( ( (uint8*) rgb3 ) + adj );
    }
}

// 16-bit pixel swizzler
// this format doesn't care about the format of the 16-bit pixels, so the
// input can be anything (555, 565, 4444, etc). it just leaves the pixels
// in the same format.
void GRANNY
All16SwizzleNGC(uint32 Width, uint32 Height, uint32 SourceStride,
                void *SourceBits, void *DestBits)
{
    real64x * rgb0 = (real64x*) SourceBits;
    real64x * rgb1 = (real64x*) ( ( (uint8*) SourceBits ) + SourceStride );
    real64x * rgb2 = (real64x*) ( ( (uint8*) SourceBits ) + SourceStride + SourceStride );
    real64x * rgb3 = (real64x*) ( ( (uint8*) SourceBits ) + SourceStride + SourceStride + SourceStride );
    real64x * ngc = (real64x*) DestBits;
    uint32 adj = ( SourceStride * 4 ) - ( Width * 2 );
    uint32 y;

    for( y = ( Height / 4 ) ; y-- ; )
    {
        uint32 x;

        for( x = ( Width / 4 ) ; x-- ; )
        {
            register real64x p0;
            register real64x p1;
            register real64x p2;
            register real64x p3;

            p0 = rgb0[ 0 ];
            p1 = rgb1[ 0 ];
            p2 = rgb2[ 0 ];
            p3 = rgb3[ 0 ];

            ngc[ 0 ] = p0;
            ngc[ 1 ] = p1;
            ngc[ 2 ] = p2;
            ngc[ 3 ] = p3;

            ++rgb0;
            ++rgb1;
            ++rgb2;
            ++rgb3;
            ngc += 4;
        }
        rgb0 = (real64x*) ( ( (uint8*) rgb0 ) + adj );
        rgb1 = (real64x*) ( ( (uint8*) rgb1 ) + adj );
        rgb2 = (real64x*) ( ( (uint8*) rgb2 ) + adj );
        rgb3 = (real64x*) ( ( (uint8*) rgb3 ) + adj );
    }
}

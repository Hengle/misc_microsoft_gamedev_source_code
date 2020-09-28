#if !defined(GRANNY_PIXEL_LAYOUT_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_pixel_layout.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(PixelLayoutGroup);

EXPTYPE struct pixel_layout
{
    int32 BytesPerPixel;

    // These are for R, G, B, A respectively
    int32 ShiftForComponent[4];
    int32 BitsForComponent[4];
};
EXPCONST EXPGROUP(pixel_layout) extern data_type_definition PixelLayoutType[];

EXPAPI GS_SAFE bool PixelLayoutsAreEqual(pixel_layout const &Operand0,
                                         pixel_layout const &Operand1);
EXPAPI GS_SAFE bool PixelLayoutHasAlpha(pixel_layout const &Layout);

// The SetStockSpecification() family of functions take a texture
// specification and set the ShiftForComponent and BitsForComponent
// fields to be consistent with simple, packed RGBA textures.  For
// example, calling SetStockRGBASpecification() with RGBA bit
// counts of (5, 6, 5, 0) would result in a standard RGB565 format.
EXPAPI GS_SAFE void SetStockSpecification(pixel_layout &Layout,
                                          int32 const *BitsForComponent,
                                          int32 const *ComponentPlacement);
EXPAPI GS_SAFE void SetStockRGBASpecification(pixel_layout &Layout,
                                              int32x RedBits, int32x GreenBits,
                                              int32x BlueBits, int32x AlphaBits);
EXPAPI GS_SAFE void SetStockBGRASpecification(pixel_layout &Layout,
                                              int32x RedBits, int32x GreenBits,
                                              int32x BlueBits, int32x AlphaBits);

EXPAPI GS_PARAM void SwapRGBAToBGRA(pixel_layout &Layout);

EXPAPI GS_PARAM void ConvertPixelFormat(int32x Width, int32x Height,
                                        pixel_layout const &SourceLayout,
                                        int32x SourceStride, void const *SourceBits,
                                        pixel_layout const &DestLayout,
                                        int32x DestStride, void *DestBits);

EXPAPI GS_PARAM void ARGB8888SwizzleNGC(uint32 Width, uint32 Height,
                                        uint32 SourceStride,
                                        void *SourceBits, void *DestBits);
EXPAPI GS_PARAM void All16SwizzleNGC(uint32 Width, uint32 Height,
                                     uint32 SourceStride,
                                     void *SourceBits, void *DestBits);

EXPGROUP(pixel_layout)
EXPCONST extern pixel_layout const RGB555PixelFormat;
EXPCONST extern pixel_layout const RGB565PixelFormat;
EXPCONST extern pixel_layout const RGBA5551PixelFormat;
EXPCONST extern pixel_layout const RGBA4444PixelFormat;
EXPCONST extern pixel_layout const RGB888PixelFormat;
EXPCONST extern pixel_layout const RGBA8888PixelFormat;
EXPCONST extern pixel_layout const ARGB8888PixelFormat;
EXPCONST extern pixel_layout const BGR555PixelFormat;
EXPCONST extern pixel_layout const BGR565PixelFormat;
EXPCONST extern pixel_layout const BGRA5551PixelFormat;
EXPCONST extern pixel_layout const BGRA4444PixelFormat;
EXPCONST extern pixel_layout const BGR888PixelFormat;
EXPCONST extern pixel_layout const BGRA8888PixelFormat;
EXPGROUP(PixelLayoutGroup)

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_PIXEL_LAYOUT_H
#endif

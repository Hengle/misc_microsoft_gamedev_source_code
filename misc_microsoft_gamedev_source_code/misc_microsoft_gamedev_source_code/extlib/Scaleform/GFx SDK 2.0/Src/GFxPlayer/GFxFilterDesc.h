/**********************************************************************

Filename    :   GFxFilterDesc.h
Content     :   
Created     :   
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxFilterDesc_H
#define INC_GFxFilterDesc_H

#include "GTypes.h"

class GFxStream;

//------------------------------------------------------------------------
struct GFxFilterDesc
{
    enum { MaxFilters = 4 };

    enum FilterType
    {
        Filter_DropShadow   = 0,
        Filter_Blur         = 1,
        Filter_Glow         = 2,
        Filter_Bevel        = 3,
        Filter_GradientGlow = 4,
        Filter_Convolution  = 5,
        Filter_AdjustColor  = 6,
        Filter_GradientBevel= 7,
    };

    enum FlagsType
    {
        KnockOut   = 0x20,
        HideObject = 0x40,
        FineBlur   = 0x80
    };

    UInt8   Flags;      // 0..3 - FilterType, 4..7 - Flags (GFxGlyphParam::Flags)
    UInt8   BlurX;      // Fixed point 4.4, unsigned
    UInt8   BlurY;      // Fixed point 4.4, unsigned
    UInt8   Strength;   // Fixed point 4.4, unsigned
    SInt16  Angle;      // In 1/10 degree
    SInt16  Distance;   // In Twips
    GColor  Color;      // Color with alpha

    void Clear()
    {
        Flags = BlurX = BlurY = 0;
        Strength = 1 << 4;
        Angle = 0;
        Distance = 0;
        Color = 0;
    }
};


//------------------------------------------------------------------------
struct GFxTextFilter : public GNewOverrideBase
{
    UInt8                           BlurX;            // Fixed point 4.4, unsigned
    UInt8                           BlurY;            // Fixed point 4.4, unsigned
    UInt8                           BlurStrength;     // Fixed point 4.4, unsigned
    UInt8                           ShadowFlags;      // Corresponds to GFxGlyphParam::Flags
    UInt8                           ShadowBlurX;      // Fixed point 4.4, unsigned
    UInt8                           ShadowBlurY;      // Fixed point 4.4, unsigned
    UInt8                           ShadowStrength;   // Fixed point 4.4, unsigned
    UInt8                           ShadowAlpha;
    SInt16                          ShadowAngle;      // In 1/10 degree
    SInt16                          ShadowDistance;   // In Twips
    SInt16                          ShadowOffsetX;    // In Twips, default 57 twips (cos(45) * 4 * 20)
    SInt16                          ShadowOffsetY;    // In Twips, default 57 twips (sin(45) * 4 * 20)
    GColor                          ShadowColor;      // Color with alpha

    GFxTextFilter()
    {
        SetDefaultShadow();
    }

    void SetDefaultShadow()
    {
        BlurX = 0;
        BlurY = 0;
        BlurStrength   = 1 << 4;
        ShadowFlags    = GFxFilterDesc::FineBlur;
        ShadowBlurX    = 4 << 4;
        ShadowBlurY    = 4 << 4;
        ShadowStrength = 1 << 4;
        ShadowAlpha    = 255;
        ShadowAngle    = 450;
        ShadowDistance = 4*20;
        ShadowOffsetX  = 57;  // In Twips, default 57 twips (cos(45) * 4 * 20)
        ShadowOffsetY  = 57;  // In Twips, default 57 twips (sin(45) * 4 * 20)
        ShadowColor    = 0;   // Color with alpha
    }

    static UInt8 FloatToFixed44(Float v) { return UInt8(GTL::gclamp(UInt(v * 16.0f + 0.5f), UInt(0), UInt(255))); }
    static Float Fixed44toFloat(UInt8 v) { return Float(v) / 16.0f; }

    void UpdateShadowOffset()
    {
        Float a = Float(ShadowAngle) * 3.1415926535897932384626433832795f / 1800.0f;
        ShadowOffsetX = SInt16(cosf(a) * ShadowDistance);
        ShadowOffsetY = SInt16(sinf(a) * ShadowDistance);
    }
};

// Helper function used to load filters.
UInt GFx_LoadFilters(GFxStream* pin, GFxFilterDesc* filters);

#endif

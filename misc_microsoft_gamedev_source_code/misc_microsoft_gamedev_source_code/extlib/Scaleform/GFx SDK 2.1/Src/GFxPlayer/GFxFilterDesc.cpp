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

#include "GFxStream.h"
#include "GFxFilterDesc.h"


// ***** Tag Loaders implementation


/*
    struct swf_filter_drop_shadow 
    {
        swf_filter_type     f_type;     // 0
        swf_rgba            f_rgba;
        signed long fixed   f_blur_horizontal;
        signed long fixed   f_blur_vertical;
        signed long fixed   f_radian_angle;
        signed long fixed   f_distance;
        signed short fixed  f_strength;
        unsigned            f_inner_shadow     : 1;
        unsigned            f_knock_out        : 1;
        unsigned            f_composite_source : 1;
        unsigned            f_reserved1        : 1;
        unsigned            f_reserved2        : 4;
    };


    struct swf_filter_blur 
    {
        swf_filter_type     f_type;     // 1 
        unsigned long fixed f_blur_horizontal;
        unsigned long fixed f_blur_vertical;
        unsigned            f_passes   : 5;
        unsigned            f_reserved : 3;
    };


    struct swf_filter_glow 
    {
        swf_filter_type     f_type;     // 2
        swf_rgba            f_rgba;
        signed long fixed   f_blur_horizontal;
        signed long fixed   f_blur_vertical;
        signed short fixed  f_strength;
        unsigned            f_inner_shadow     : 1;
        unsigned            f_knock_out        : 1;
        unsigned            f_composite_source : 1;
        unsigned            f_reserved1        : 1;
        unsigned            f_reserved2        : 4;
    };
*/


//------------------------------------------------------------------------
static inline UInt8 GFx_Fixed1616toFixed44U(UInt32 v)
{
    UInt vi = v >> 16;
    if (vi > 15) vi = 15;
    return UInt8((vi << 4) | ((v >> 12) & 0xF));
}

//------------------------------------------------------------------------
static inline UInt8 GFx_Fixed88toFixed44U(UInt16 v)
{
    UInt vi = v >> 8;
    if (vi > 15) vi = 15;
    return UInt8((vi << 4) | ((v >> 4) & 0xF));
}

//------------------------------------------------------------------------
static inline SInt16 GFx_Radians1616toAngle(SInt32 v)
{
    Float vf = Float(v) / 65536.0f;
    return SInt16(fmodf(vf * 1800.0f / 3.1415926535897932384626433832795f, 3600.0f));
}

//------------------------------------------------------------------------
static inline SInt16 GFx_Fixed1616toTwips(SInt32 v)
{
    Float vf = Float(v) / 65536.0f;
    return SInt16(vf * 20.0f);
}

//------------------------------------------------------------------------
static inline UInt8 GFx_ParseFilterFlags(UInt8 f)
{
    UInt8 flags = 0;
    if  (f & 0x40)       flags |= GFxFilterDesc::KnockOut;
    if ((f & 0x20) == 0) flags |= GFxFilterDesc::HideObject;
    if ((f & 0x0F) > 1)  flags |= GFxFilterDesc::FineBlur;
    return flags;
}

// Helper function used to load filters.
//------------------------------------------------------------------------
UInt GFx_LoadFilters(GFxStream* pin, GFxFilterDesc* filters)
{
    UByte filterCount = pin->ReadU8();
    pin->LogParse("  filter count: %d\n", (SInt)filterCount);

    GFxFilterDesc filterDesc;
    UInt numFilters = 0;

    while(filterCount--)
    {
        GFxFilterDesc::FilterType filter = (GFxFilterDesc::FilterType)pin->ReadU8();
        UInt numBytes = 0;

        filterDesc.Clear();
        filterDesc.Flags = UInt8(filter);

        switch(filter)
        {
        case GFxFilterDesc::Filter_DropShadow: // 23 bytes
            pin->ReadRgba(&filterDesc.Color);
            filterDesc.BlurX    = GFx_Fixed1616toFixed44U(pin->ReadU32());
            filterDesc.BlurY    = GFx_Fixed1616toFixed44U(pin->ReadU32());
            filterDesc.Angle    = GFx_Radians1616toAngle(pin->ReadU32());
            filterDesc.Distance = GFx_Fixed1616toTwips(pin->ReadU32());
            filterDesc.Strength = GFx_Fixed88toFixed44U(pin->ReadU16());
            filterDesc.Flags   |= GFx_ParseFilterFlags(pin->ReadU8());
            if (filters && numFilters < GFxFilterDesc::MaxFilters) 
                filters[numFilters++] = filterDesc;
            break;

        case GFxFilterDesc::Filter_Blur: // 9 bytes
            filterDesc.BlurX  = GFx_Fixed1616toFixed44U(pin->ReadU32());
            filterDesc.BlurY  = GFx_Fixed1616toFixed44U(pin->ReadU32());
            filterDesc.Flags |= ((pin->ReadU8() >> 3) > 1) ? GFxFilterDesc::FineBlur : 0;
            if (filters && numFilters < GFxFilterDesc::MaxFilters) 
                filters[numFilters++] = filterDesc;
            break;

        case GFxFilterDesc::Filter_Glow: // 15 bytes
            pin->ReadRgba(&filterDesc.Color);
            filterDesc.BlurX    = GFx_Fixed1616toFixed44U(pin->ReadU32());
            filterDesc.BlurY    = GFx_Fixed1616toFixed44U(pin->ReadU32());
            filterDesc.Strength = GFx_Fixed88toFixed44U(pin->ReadU16());
            filterDesc.Flags   |= GFx_ParseFilterFlags(pin->ReadU8());
            if (filters && numFilters < GFxFilterDesc::MaxFilters) 
                filters[numFilters++] = filterDesc;
            break;

        case GFxFilterDesc::Filter_Bevel:
            numBytes = 27; 
            break;

        case GFxFilterDesc::Filter_GradientGlow:
            numBytes = 19 + pin->ReadU8()*5; 
            break;

        case GFxFilterDesc::Filter_Convolution:
            {
                UInt cols = pin->ReadU8();
                UInt rows = pin->ReadU8();
                numBytes = 4 + 4 + 4*cols*rows + 4 + 1;
            }
            break;

        case GFxFilterDesc::Filter_AdjustColor:
            numBytes = 80; 
            break;

        case GFxFilterDesc::Filter_GradientBevel:
            numBytes = 19 + pin->ReadU8()*5; 
            break;

        default:
            pin->LogParse("Unknown filter code %d\n", filter);
        }

        // Skip filter or the rest of the filter
        while(numBytes--)
            pin->ReadU8();
    }
    return numFilters;
}


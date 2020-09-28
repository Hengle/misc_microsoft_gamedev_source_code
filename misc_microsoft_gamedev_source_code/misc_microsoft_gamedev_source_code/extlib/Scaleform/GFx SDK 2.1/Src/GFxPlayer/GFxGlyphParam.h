/**********************************************************************

Filename    :   GFxGlyphParam.h
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

#ifndef INC_GFxGlyphParam_H
#define INC_GFxGlyphParam_H

#include <math.h>
#include "GTypes.h"
#include "GFxFilterDesc.h"

class GFxFontResource;

//------------------------------------------------------------------------
struct GFxGlyphParam
{
    GFxFontResource* pFont;
    UInt16           GlyphIndex;

    UInt8            FontSize;     // 255 is the max size for raster glyphs
    UInt8            Flags;        // See enum FlagsType
    UInt8            BlurX;        // fixed point 4.4, unsigned
    UInt8            BlurY;        // fixed point 4.4, unsigned
    UInt8            BlurStrength; // fixed point 4.4, unsigned
    SInt8            Outline;      // fixed point 5.3, signed

    enum FlagsType
    {
        OptRead    = 0x01,
        AutoFit    = 0x02,
        Stretch    = 0x04,
        FauxBold   = 0x08,
        FauxItalic = 0x10,
        KnockOut   = 0x20,
        HideObject = 0x40,
        FineBlur   = 0x80
    };

    static Float GetMaxBlur()    { return  15.75f; }
    static Float GetMaxOutline() { return  15.75f; }
    static Float GetMinOutline() { return -15.75f; }

    void Clear()
    {
        pFont=0; GlyphIndex=0; FontSize=0; Flags=0;
        BlurX=0; BlurY=0; BlurStrength=0; Outline=0;
    }

    UInt  GetFontSize()     const { return FontSize; }
    bool  IsOptRead()       const { return (Flags & OptRead) != 0; }
    bool  IsAutoFit()       const { return (Flags & AutoFit) != 0; }
    UInt  GetStretch()      const { return (Flags & Stretch) ? 3 : 1; }
    bool  IsFauxBold()      const { return (Flags & FauxBold) != 0; }
    bool  IsFauxItalic()    const { return (Flags & FauxItalic) != 0; }
    bool  IsKnockOut()      const { return (Flags & KnockOut) != 0; }
    bool  IsHiddenObject()  const { return (Flags & HideObject) != 0; }
    bool  IsFineBlur()      const { return (Flags & FineBlur) != 0; }
    Float GetBlurX()        const { return Float(BlurX) / 16.0f; }
    Float GetBlurY()        const { return Float(BlurY) / 16.0f; }
    Float GetBlurStrength() const { return Float(BlurStrength) / 16.0f; }
    Float GetOutline()      const { return Float(Outline) / 8.0f; }

    void SetFontSize(UInt s)      { FontSize = UInt8(s); }
    void SetOptRead(bool f)       { if(f) Flags |= OptRead;    else Flags &= ~OptRead; }
    void SetAutoFit(bool f)       { if(f) Flags |= AutoFit;    else Flags &= ~AutoFit; }
    void SetStretch(bool f)       { if(f) Flags |= Stretch;    else Flags &= ~Stretch; }
    void SetFauxBold(bool f)      { if(f) Flags |= FauxBold;   else Flags &= ~FauxBold; }
    void SetFauxItalic(bool f)    { if(f) Flags |= FauxItalic; else Flags &= ~FauxItalic; }
    void SetKnockOut(bool f)      { if(f) Flags |= KnockOut;   else Flags &= ~KnockOut; }
    void SetHideObject(bool f)    { if(f) Flags |= HideObject; else Flags &= ~HideObject; }
    void SetFineBlur(bool f)      { if(f) Flags |= FineBlur;   else Flags &= ~FineBlur; }
    void SetBlurX(Float v)        { BlurX = UInt8(v * 16.0f + 0.5f); }
    void SetBlurY(Float v)        { BlurY = UInt8(v * 16.0f + 0.5f); }
    void SetBlurStrength(Float v) { BlurStrength = UInt8(v * 16.0f + 0.5f); }
    void SetOutline(Float v)      { Outline = SInt8(floorf(v * 8.0f + 0.5f)); }

    size_t operator()(const GFxGlyphParam& key) const
    {
        return (((size_t)key.pFont) >> 6) ^ (size_t)key.pFont ^ 
                 (size_t)key.GlyphIndex ^
                 (size_t)key.FontSize ^
                 (size_t)key.Flags ^
                 (size_t)key.BlurX ^     
                ((size_t)key.BlurY << 1) ^
                 (size_t)key.BlurStrength ^
               (((size_t)key.Outline) & 0xFF);
    }

    bool operator == (const GFxGlyphParam& key) const 
    { 
        return  pFont        == key.pFont && 
                GlyphIndex   == key.GlyphIndex &&
                FontSize     == key.FontSize &&
                Flags        == key.Flags &&
                BlurX        == key.BlurX &&     
                BlurY        == key.BlurY &&
                BlurStrength == key.BlurStrength &&
                Outline      == key.Outline;
    }
    
    bool operator != (const GFxGlyphParam& key) const
    {
        return !(*this == key);
    }

};




//------------------------------------------------------------------------
struct GFxTextFieldParam
{
    enum { ShadowDisabled = 0x01 };

    GFxGlyphParam TextParam;
    GFxGlyphParam ShadowParam;
    GColor        ShadowColor;
    SInt16        ShadowOffsetX; // In Twips
    SInt16        ShadowOffsetY; // In Twips

    void Clear()
    {
        TextParam.Clear();
        ShadowParam.Clear();
        ShadowColor = 0; ShadowOffsetX = 0; ShadowOffsetY = 0;
    }

    GFxTextFieldParam() { Clear(); }

    bool operator == (const GFxTextFieldParam& key) const 
    { 
        return TextParam     == key.TextParam &&
               ShadowParam   == key.ShadowParam &&
               ShadowColor   == key.ShadowColor &&
               ShadowOffsetX == key.ShadowOffsetX &&
               ShadowOffsetY == key.ShadowOffsetY;
    }

    bool operator != (const GFxTextFieldParam& key) const
    {
        return !(*this == key);
    }

    void LoadFromTextFilter(const GFxTextFilter& filter)
    {
        TextParam.BlurX        = filter.BlurX;
        TextParam.BlurY        = filter.BlurY;
        TextParam.Flags        = GFxGlyphParam::FineBlur;
        TextParam.BlurStrength = filter.BlurStrength;
      //TextParam.Outline      = ...;

        if ((filter.ShadowFlags & ShadowDisabled) == 0)
        {
            ShadowParam.Flags        = UInt8(filter.ShadowFlags & ~ShadowDisabled);
            ShadowParam.BlurX        = filter.ShadowBlurX;
            ShadowParam.BlurY        = filter.ShadowBlurY;
            ShadowParam.BlurStrength = filter.ShadowStrength;
          //ShadowParam.Outline      = ...;
            ShadowColor              = filter.ShadowColor;
            ShadowOffsetX            = filter.ShadowOffsetX;
            ShadowOffsetY            = filter.ShadowOffsetY;
        }
    }

};



#endif

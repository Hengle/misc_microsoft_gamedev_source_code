/**********************************************************************

Filename    :   GFxFontProviderPacked.h
Content     :   Packed Fonts wrapper (External Font Provider) 
Created     :   6/21/2007
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxFontProviderPacked_H
#define INC_GFxFontProviderPacked_H

#include "GFxFont.h"
#include "GFxLoader.h"


class GFxFontCompactor;
class GFxCompactedFont;

class GFxShapeNoStylesDef;

//------------------------------------------------------------------------
class GFxExternalFontPacked : public GFxFont
{
    enum { FontHeight = 1024, ShapePageSize = 256-2 - 8-4 };
public:
    GFxExternalFontPacked(GFxFontCompactor* fc, const GFxCompactedFont* font,
                          UInt fontFlags);
    virtual ~GFxExternalFontPacked();
    
    virtual GFxTextureGlyphData*    GetTextureGlyphData() const { return 0; }

    virtual int                     GetGlyphIndex(UInt16 code);
    virtual bool                    IsHintedVectorGlyph(UInt, UInt) const { return false; }
    virtual bool                    IsHintedRasterGlyph(UInt, UInt) const { return false; }
    virtual GFxShapeCharacterDef*   GetGlyphShape(UInt glyphIndex, UInt glyphSize);
    virtual GFxGlyphRaster*         GetGlyphRaster(UInt glyphIndex, UInt glyphSize);
    virtual Float                   GetAdvance(UInt glyphIndex) const;
    virtual Float                   GetKerningAdjustment(UInt lastCode, UInt thisCode) const;

    virtual Float                   GetGlyphWidth(UInt glyphIndex) const;
    virtual Float                   GetGlyphHeight(UInt glyphIndex) const;
    virtual GRectF&                 GetGlyphBounds(UInt glyphIndex, GRectF* prect) const;

    virtual const char*             GetName() const;

private:
    Float norm(Float v) const { return v * FontHeight / NominalSize; }
    SInt  norm(SInt v)  const { return v * FontHeight / NominalSize; }

    GPtr<GFxFontCompactor>          pFontData;
    const GFxCompactedFont*         pFont;
    GPtr<GFxShapeNoStylesDef>      pShape;
    SInt                            NominalSize;
};



//------------------------------------------------------------------------
class GFxFontProviderPacked : public GFxFontProvider
{
public:
    GFxFontProviderPacked();
    virtual ~GFxFontProviderPacked();
    
    bool LoadFonts(GFxFileOpener* fo, const char* fileName);

    virtual GFxFont*    CreateFont(const char* name, UInt fontFlags);    

private:
    GPtr<GFxFontCompactor> pFontData;
    bool                   FontsAcquired;
};



#endif



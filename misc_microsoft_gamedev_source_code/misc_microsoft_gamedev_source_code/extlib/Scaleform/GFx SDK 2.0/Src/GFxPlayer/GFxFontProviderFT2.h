/**********************************************************************

Filename    :   GFxFontProviderFT2.h
Content     :   FreeType2 font provider
Created     :   6/21/2007
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

----------------------------------------------------------------------
The code of these classes was taken from the Anti-Grain Geometry
Project and modified for the use by Scaleform. 
Permission to use without restrictions is hereby granted to 
Scaleform Corporation by the author of Anti-Grain Geometry Project.
See http://antigtain.com for details.
**********************************************************************/

#ifndef INC_GFxFontProviderFT2_H
#define INC_GFxFontProviderFT2_H


#include <ft2build.h>
#include FT_FREETYPE_H

#include "GFxFont.h"
#include "GFxLoader.h"
#include "GFxString.h"

//------------------------------------------------------------------------
class GFxShapeCache;
class GFxShapeCacheNode;
class GFxFontProviderFT2;

class GFxExternalFontFT2 : public GFxFont
{
    enum { FontHeight = 1024, ShapePageSize = 256-2 - 8-4 };
public:
    GFxExternalFontFT2(GFxFontProviderFT2* pprovider, 
                       FT_Library lib, 
                       const GFxString& fontName, 
                       UInt fontFlags,
                       const char* fileName, 
                       UInt faceIndex);

    GFxExternalFontFT2(GFxFontProviderFT2* pprovider, 
                       FT_Library lib, 
                       const GFxString& fontName, 
                       UInt fontFlags,
                       const char* fontMem, 
                       UInt fontMemSize, 
                       UInt faceIndex);

    virtual ~GFxExternalFontFT2();

    bool    IsValid() const { return Face != 0; }
    
    virtual GFxTextureGlyphData*    GetTextureGlyphData() const { return 0; }

    virtual int                     GetGlyphIndex(UInt16 code);
    virtual GFxShapeCharacterDef*   GetGlyphShape(UInt glyphIndex);
    virtual Float                   GetAdvance(UInt glyphIndex) const;
    virtual Float                   GetKerningAdjustment(UInt lastCode, UInt thisCode) const;

    virtual Float                   GetGlyphWidth(UInt glyphIndex) const;
    virtual Float                   GetGlyphHeight(UInt glyphIndex) const;
    virtual GRectF&                 GetGlyphBounds(UInt glyphIndex, GRectF* prect) const;

    virtual const char*             GetName() const { return &Name[0]; }

private:
    void    setFontMetrics();
    bool    decomposeGlyphOutline(const FT_Outline& outline, GFxShapeCharacterDef* shape);

    struct GlyphType
    {
        UInt                    FtIndex;
        Float                   Advance;
        GRectF                  Bounds;
        GFxShapeCacheNode*      pShapeNode;
        UInt                    Stamp;
    };

    struct KerningPairType
    {           
        UInt16  Char0, Char1;
        bool    operator==(const KerningPairType& k) const
            { return Char0 == k.Char0 && Char1 == k.Char1; }
    };

    // AddRef for font provider since it contains our cache.
    GPtr<GFxFontProviderFT2>            pFontProvider;    
    GFxString                           Name;
    FT_Face                             Face;
    GTL::garray<GlyphType>              Glyphs;
    GTL::ghash_identity<UInt16, UInt>   CodeTable;
    GTL::ghash<KerningPairType, Float>  KerningPairs;
};



//------------------------------------------------------------------------
class GFxFontProviderFT2 : public GFxFontProvider
{
public:
    GFxFontProviderFT2(FT_Library lib=0, UInt maxNumShapes=4000);
    virtual ~GFxFontProviderFT2();

    FT_Library GetFT_Library() { return Lib; }

    void MapFontToFile  (const char* fontName, UInt fontFlags, const char* fileName, UInt faceIndex=0);
    void MapFontToMemory(const char* fontName, UInt fontFlags, const char* fontData, UInt dataSize, UInt faceIndex=0);


    virtual GFxFont*    CreateFont(const char* name, UInt fontFlags);

    GFxShapeCache*      GetShapeCache() const { return pCache; }

private:
    struct FontType
    {
        GFxString    FontName;
        UInt         FontFlags;
        GFxString    FileName;
        const char*  FontData;
        UInt         FontDataSize;
        UInt         FaceIndex;
    };

    GFxExternalFontFT2* createFont(FontType* font, const GFxString& fontName, UInt fontFlags);

    GFxShapeCache*        pCache;
    FT_Library            Lib;
    bool                  ExtLibFlag;
    GTL::garray<FontType> Fonts;
    UInt                  NamesEndIdx;
};




#endif

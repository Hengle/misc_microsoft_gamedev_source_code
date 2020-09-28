/**********************************************************************

Filename    :   GFxFontProviderWin32.h
Content     :   Win32 API Font provider (GetGlyphOutline)
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

#include "GFxFontProviderWin32.h"
#include "GFxString.h"
#include "GFxShape.h"
#include "GStd.h"

//------------------------------------------------------------------------
struct GFxGdiSelectObjectGuard
{
    HDC     WinDC;
    HGDIOBJ WinObj;
    GFxGdiSelectObjectGuard(HDC dc, HGDIOBJ obj) : WinDC(dc), WinObj(::SelectObject(dc, obj)) {}
   ~GFxGdiSelectObjectGuard() { ::SelectObject(WinDC, WinObj); }
};

//------------------------------------------------------------------------
GFxExternalFontWin32::~GFxExternalFontWin32()
{
    if (HintedFont)
        ::DeleteObject(HintedFont);

    if (MasterFont)
        ::DeleteObject(MasterFont);
}

//------------------------------------------------------------------------
GFxExternalFontWin32::GFxExternalFontWin32(GFxFontProviderWin32 *pprovider,
                                           GFxFontSysDataWin32* sysData, 
                                           const char* name, 
                                           UInt fontFlags)
    : GFxFont(fontFlags),
      pFontProvider(pprovider),
      pSysData(sysData),
      MasterFont(0), HintedFont(0), LastHintedFontSize(0),
      Scale1024(1024.0f / Float(GGO_FontHeight))
      
{
    Name.resize(gfc_strlen(name) + 1);
    gfc_strcpy(&Name[0], gfc_strlen(name) + 1, name);
   
    // Font name is encoded as UTF8, so unpack.
    NameW.resize(GUTF8Util::GetLength(name) + 1);
    GUTF8Util::DecodeString(&NameW[0], name);

    MasterFont = ::CreateFontW(-GGO_FontHeight,             // height of font
                                0,                          // average character width
                                0,                          // angle of escapement
                                0,                          // base-line orientation angle
                                IsBold() ? FW_BOLD : FW_NORMAL, // font weight
                                IsItalic(),                 // italic attribute option
                                0,                          // underline attribute option
                                0,                          // strikeout attribute option
                                DEFAULT_CHARSET,            // character set identifier
                                OUT_DEFAULT_PRECIS,         // output precision
                                CLIP_DEFAULT_PRECIS,        // clipping precision
                                ANTIALIASED_QUALITY,        // output quality
                                DEFAULT_PITCH,              // pitch and family
                                &NameW[0]);                 // typeface name

    if(MasterFont)
    {
        TEXTMETRICW tm;
        GFxGdiSelectObjectGuard g1(pSysData->WinHDC, MasterFont);
        ::GetTextMetricsW(pSysData->WinHDC, &tm);
        SetFontMetrics(Float(tm.tmExternalLeading) * Scale1024, 
                       Float(tm.tmAscent) * Scale1024, 
                       Float(tm.tmDescent) * Scale1024);
        loadKerningPairs();
    }
}

//------------------------------------------------------------------------
void GFxExternalFontWin32::SetHinting(const GFxFont::NativeHintingType& pnh)
{
    Hinting.MaxRasterHintedSize = pnh.MaxRasterHintedSize;
    Hinting.MaxVectorHintedSize = pnh.MaxVectorHintedSize;
    Hinting.RasterRange         = pnh.RasterRange;
    Hinting.VectorRange         = pnh.VectorRange;
}

//------------------------------------------------------------------------
void GFxExternalFontWin32::loadKerningPairs()
{
    GTL::garray<KERNINGPAIR> pairs;
    UInt size = ::GetKerningPairsW(pSysData->WinHDC, 0, 0);
    if(size)
    {
        pairs.resize(size);
        ::GetKerningPairsW(pSysData->WinHDC, size, &pairs[0]);
    }
    KerningPairs.clear();
    for(UInt i = 0; i < pairs.size(); ++i)
    {
        KerningPairType pair;
        pair.Char0 = pairs[i].wFirst;
        pair.Char1 = pairs[i].wSecond;
        KerningPairs.add(pair, Float(pairs[i].iKernAmount) * Scale1024);
    }
}

//------------------------------------------------------------------------
void GFxExternalFontWin32::decomposeGlyphOutline(const UByte* data, 
                                                 UInt size, 
                                                 GFxShapeNoStylesDef* shape)
{
    const UByte* curGlyph = data;
    const UByte* endGlyph = data + size;
    bool  hinted = shape->GetHintedGlyphSize() != 0;
    
    GFxPathPacker path;

    while(curGlyph < endGlyph)
    {
        const TTPOLYGONHEADER* th = (TTPOLYGONHEADER*)curGlyph;
        
        const UByte* endPoly = curGlyph + th->cb;
        const UByte* curPoly = curGlyph + sizeof(TTPOLYGONHEADER);

        path.Reset();
        path.SetFill0(1);
        path.SetFill1(0);
        if (hinted)
            path.SetMoveTo(FxToTwips(th->pfxStart.x), -FxToTwips(th->pfxStart.y));
        else
            path.SetMoveTo(FxToS1024(th->pfxStart.x), -FxToS1024(th->pfxStart.y));

        while(curPoly < endPoly)
        {
            const TTPOLYCURVE* pc = (const TTPOLYCURVE*)curPoly;
            
            if (pc->wType == TT_PRIM_LINE)
            {
                int i;
                for (i = 0; i < pc->cpfx; i++)
                {
                    if (hinted)
                        path.LineToAbs(FxToTwips(pc->apfx[i].x), -FxToTwips(pc->apfx[i].y));
                    else
                        path.LineToAbs(FxToS1024(pc->apfx[i].x), -FxToS1024(pc->apfx[i].y));
                }
            }
            
            if (pc->wType == TT_PRIM_QSPLINE)
            {
                int u;
                for (u = 0; u < pc->cpfx - 1; u++)  // Walk through points in spline
                {
                    POINTFX pntB = pc->apfx[u];     // B is always the current point
                    POINTFX pntC = pc->apfx[u+1];
                    
                    if (u < pc->cpfx - 2)           // If not on last spline, compute C
                    {
                        // midpoint (x,y)
                        *(int*)&pntC.x = (*(int*)&pntB.x + *(int*)&pntC.x) / 2;
                        *(int*)&pntC.y = (*(int*)&pntB.y + *(int*)&pntC.y) / 2;
                    }
                    if (hinted)
                        path.CurveToAbs(FxToTwips(pntB.x), -FxToTwips(pntB.y),
                                        FxToTwips(pntC.x), -FxToTwips(pntC.y));
                    else
                        path.CurveToAbs(FxToS1024(pntB.x), -FxToS1024(pntB.y),
                                        FxToS1024(pntC.x), -FxToS1024(pntC.y));
                }
            }
            curPoly += sizeof(WORD) * 2 + sizeof(POINTFX) * pc->cpfx;
        }
        curGlyph += th->cb;
        path.ClosePath();
        shape->AddPath(&path);
    }
}

//------------------------------------------------------------------------
class GFxBitsetIterator
{
public:
    GFxBitsetIterator(const UByte* bits, unsigned offset = 0) :
        Bits(bits + (offset >> 3)),
        Mask(UByte(0x80 >> (offset & 7)))
    {}

    void operator ++ ()
    {
        Mask >>= 1;
        if(Mask == 0)
        {
            ++Bits;
            Mask = 0x80;
        }
    }

    unsigned GetBit() const
    {
        return (*Bits) & Mask;
    }

private:
    const UByte* Bits;
    UByte        Mask;
};

//------------------------------------------------------------------------
void GFxExternalFontWin32::decomposeGlyphBitmap(const UByte* data, 
                                                int w, int h,
                                                int x, int y,
                                                GFxGlyphRaster* raster)
{
    raster->Width  = w;
    raster->Height = h;
    raster->OriginX = -x;
    raster->OriginY =  y;
    raster->Raster.resize(w * h);

    int i;
    int pitch = ((w + 31) >> 5) << 2;

    const UByte* src = data;
          UByte* dst = &raster->Raster[0];
    for(i = 0; i < h; i++)
    {
        GFxBitsetIterator bits(src, 0);
        int j;
        for(j = 0; j < w; j++)
        {
            *dst++ = bits.GetBit() ? 255 : 0;
            ++bits;
        }
        src += pitch;
    }
}

//------------------------------------------------------------------------
int GFxExternalFontWin32::GetGlyphIndex(UInt16 code)
{
    if (MasterFont)
    {
        UInt* indexPtr = CodeTable.get(code);
        if (indexPtr)
            return *indexPtr;

        GFxGdiSelectObjectGuard g1(pSysData->WinHDC, MasterFont);
        GLYPHMETRICS gm;
        MAT2 im = { {0,1}, {0,0}, {0,0}, {0,1} };
        int ret = ::GetGlyphOutlineW(pSysData->WinHDC,
                                     code,
                                     GGO_METRICS,
                                     &gm,
                                     0, 0,
                                     &im);

        if (ret == GDI_ERROR)
            return -1;

        GlyphType glyph;
        glyph.Code          =  code;
        glyph.Advance       =  Float(gm.gmCellIncX) * Scale1024;
        glyph.Bounds.Left   =  Float(gm.gmptGlyphOrigin.x) * Scale1024;
        glyph.Bounds.Top    = -Float(gm.gmptGlyphOrigin.y) * Scale1024;
        glyph.Bounds.Right  =  glyph.Bounds.Left + Float(gm.gmBlackBoxX) * Scale1024;
        glyph.Bounds.Bottom =  glyph.Bounds.Top  + Float(gm.gmBlackBoxY) * Scale1024;

        Glyphs.push_back(glyph);
        CodeTable.add(code, (UInt)Glyphs.size()-1);
        return (UInt)Glyphs.size()-1;
    }
    return -1;
}

//------------------------------------------------------------------------
bool GFxExternalFontWin32::IsHintedVectorGlyph(UInt glyphIndex, UInt glyphSize) const
{
    if (glyphIndex == (UInt)-1 ||
        Hinting.VectorRange == DontHint ||
        glyphSize > Hinting.MaxVectorHintedSize)
    {
        return false;
    }

    if (Hinting.VectorRange == HintAll)
        return true;

    return IsCJK(UInt16(Glyphs[glyphIndex].Code));
}

//------------------------------------------------------------------------
bool GFxExternalFontWin32::IsHintedRasterGlyph(UInt glyphIndex, UInt glyphSize) const
{
    if (glyphIndex == (UInt)-1 ||
        Hinting.RasterRange == DontHint ||
        glyphSize > Hinting.MaxRasterHintedSize)
    {
        return false;
    }

    if (Hinting.RasterRange == HintAll)
        return true;

    return IsCJK(UInt16(Glyphs[glyphIndex].Code));
}

//------------------------------------------------------------------------
GFxShapeCharacterDef* GFxExternalFontWin32::GetGlyphShape(UInt glyphIndex,
                                                          UInt glyphSize)
{
    if (glyphIndex == (UInt)-1)
        return 0;

    if (!IsHintedVectorGlyph(glyphIndex, glyphSize))
        glyphSize = 0;

    GlyphType& glyph = Glyphs[glyphIndex];

    HFONT hFont = MasterFont;
    if (glyphSize)
    {
        if (glyphSize != LastHintedFontSize)
        {
            if (HintedFont)
                ::DeleteObject(HintedFont);

            HintedFont = ::CreateFontW(-int(glyphSize),             // height of font
                                        0,                          // average character width
                                        0,                          // angle of escapement
                                        0,                          // base-line orientation angle
                                        IsBold() ? FW_BOLD : FW_NORMAL, // font weight
                                        IsItalic(),                 // italic attribute option
                                        0,                          // underline attribute option
                                        0,                          // strikeout attribute option
                                        DEFAULT_CHARSET,            // character set identifier
                                        OUT_DEFAULT_PRECIS,         // output precision
                                        CLIP_DEFAULT_PRECIS,        // clipping precision
                                        ANTIALIASED_QUALITY,        // output quality
                                        DEFAULT_PITCH,              // pitch and family
                                        &NameW[0]);                 // typeface name
            LastHintedFontSize = glyphSize;
        }
        hFont = HintedFont;
    }

    GFxGdiSelectObjectGuard g1(pSysData->WinHDC, hFont);

    if (pSysData->GlyphBuffer.size() == 0)
        pSysData->GlyphBuffer.resize(GFxFontSysDataWin32::BufSizeInc);

    #ifndef GGO_UNHINTED         // For compatibility with old SDKs.
    #define GGO_UNHINTED 0x0100
    #endif

    GLYPHMETRICS gm;
    int totalSize = 0;
    MAT2 im = { {0,1}, {0,0}, {0,0}, {0,1} };
    totalSize = ::GetGlyphOutlineW(pSysData->WinHDC,
                                   glyph.Code,
                                   GGO_NATIVE,
                                   &gm,
                                   (DWORD)pSysData->GlyphBuffer.size(),
                                   &pSysData->GlyphBuffer[0],
                                   &im);

    if (totalSize == GDI_ERROR ||
        totalSize > (int)pSysData->GlyphBuffer.size())
    {
        // An error has occured. Most probably it's because
        // of not enough room in the buffer. So, request the buffer size,
        // reallocate, and try again. The second call may fail for
        // some other reason (no glyph); in this case there's nothing 
        // else to do, just return NULL. 
        // The buffer is reallocated with adding BufSizeInc in order
        // to reduce the number of reallocations.
        //----------------------------------
        totalSize = ::GetGlyphOutlineW(pSysData->WinHDC,
                                       glyph.Code,
                                       GGO_NATIVE,
                                       &gm, 0, 0, &im);

        if(totalSize == GDI_ERROR) return 0;

        pSysData->GlyphBuffer.resize(totalSize + GFxFontSysDataWin32::BufSizeInc);
        totalSize = ::GetGlyphOutlineW(pSysData->WinHDC,
                                       glyph.Code,
                                       GGO_NATIVE,
                                       &gm,
                                       (DWORD)pSysData->GlyphBuffer.size(),
                                       &pSysData->GlyphBuffer[0],
                                       &im);
        if (totalSize == GDI_ERROR ||
            totalSize > (int)pSysData->GlyphBuffer.size())
            return 0;
    }

    pShape = 0;
    if (totalSize)
    {
        pShape = *new GFxShapeNoStylesDef(ShapePageSize);
        pShape->SetHintedGlyphSize(glyphSize);
        decomposeGlyphOutline(&pSysData->GlyphBuffer[0], totalSize, pShape);
    }

    return pShape;
}

//------------------------------------------------------------------------
GFxGlyphRaster* GFxExternalFontWin32::GetGlyphRaster(UInt glyphIndex, UInt glyphSize)
{
    if (!IsHintedRasterGlyph(glyphIndex, glyphSize))
        return 0;

    GlyphType& glyph = Glyphs[glyphIndex];
    if (glyphSize != LastHintedFontSize)
    {
        if (HintedFont)
            ::DeleteObject(HintedFont);

        HintedFont = ::CreateFontW(-int(glyphSize),             // height of font
                                    0,                          // average character width
                                    0,                          // angle of escapement
                                    0,                          // base-line orientation angle
                                    IsBold() ? FW_BOLD : FW_NORMAL, // font weight
                                    IsItalic(),                 // italic attribute option
                                    0,                          // underline attribute option
                                    0,                          // strikeout attribute option
                                    DEFAULT_CHARSET,            // character set identifier
                                    OUT_DEFAULT_PRECIS,         // output precision
                                    CLIP_DEFAULT_PRECIS,        // clipping precision
                                    ANTIALIASED_QUALITY,        // output quality
                                    DEFAULT_PITCH,              // pitch and family
                                    &NameW[0]);                 // typeface name
        LastHintedFontSize = glyphSize;
    }

    GFxGdiSelectObjectGuard g1(pSysData->WinHDC, HintedFont);

    if (pSysData->GlyphBuffer.size() == 0)
        pSysData->GlyphBuffer.resize(GFxFontSysDataWin32::BufSizeInc);

    GLYPHMETRICS gm;
    int totalSize = 0;
    MAT2 im = { {0,1}, {0,0}, {0,0}, {0,1} };
    totalSize = ::GetGlyphOutlineW(pSysData->WinHDC,
                                   glyph.Code,
                                   GGO_BITMAP,
                                   &gm,
                                   (DWORD)pSysData->GlyphBuffer.size(),
                                   &pSysData->GlyphBuffer[0],
                                   &im);

    if (totalSize == GDI_ERROR ||
        totalSize > (int)pSysData->GlyphBuffer.size())
    {
        // An error has occured. Most probably it's because
        // of not enough room in the buffer. So, request the buffer size,
        // reallocate, and try again. The second call may fail for
        // some other reason (no glyph); in this case there's nothing 
        // else to do, just return NULL. 
        // The buffer is reallocated with adding BufSizeInc in order
        // to reduce the number of reallocations.
        //----------------------------------
        totalSize = ::GetGlyphOutlineW(pSysData->WinHDC,
                                       glyph.Code,
                                       GGO_BITMAP,
                                       &gm, 0, 0, &im);

        if(totalSize == GDI_ERROR) return 0;

        pSysData->GlyphBuffer.resize(totalSize + GFxFontSysDataWin32::BufSizeInc);
        totalSize = ::GetGlyphOutlineW(pSysData->WinHDC,
                                       glyph.Code,
                                       GGO_BITMAP,
                                       &gm,
                                       (DWORD)pSysData->GlyphBuffer.size(),
                                       &pSysData->GlyphBuffer[0],
                                       &im);
        if (totalSize == GDI_ERROR ||
            totalSize > (int)pSysData->GlyphBuffer.size())
            return 0;
    }

    if (pRaster.GetPtr() == 0)
        pRaster = *new GFxGlyphRaster;

    if (totalSize)
    {
        decomposeGlyphBitmap(&pSysData->GlyphBuffer[0],
                             gm.gmBlackBoxX,
                             gm.gmBlackBoxY,
                             gm.gmptGlyphOrigin.x,
                             gm.gmptGlyphOrigin.y,
                             pRaster);
        return pRaster;
    }
    else
    {
        pRaster->Width  = 1;
        pRaster->Height = 1;
        pRaster->Raster.resize(1);
        pRaster->Raster[0] = 0;
    }

    return pRaster;
}

//------------------------------------------------------------------------
Float GFxExternalFontWin32::GetAdvance(UInt glyphIndex) const
{
    if (glyphIndex == (UInt)-1)
        return GetDefaultGlyphWidth();

    return Glyphs[glyphIndex].Advance;
}

//------------------------------------------------------------------------
Float GFxExternalFontWin32::GetKerningAdjustment(UInt lastCode, UInt thisCode) const
{
    Float   adjustment;
    KerningPairType k;
    k.Char0 = (UInt16)lastCode;
    k.Char1 = (UInt16)thisCode;
    if (KerningPairs.get(k, &adjustment))
    {
        return adjustment;
    }
    return 0;
}

//------------------------------------------------------------------------
Float GFxExternalFontWin32::GetGlyphWidth(UInt glyphIndex) const
{
    if (glyphIndex == (UInt)-1)
        return GetDefaultGlyphWidth();

    const GRectF& r = Glyphs[glyphIndex].Bounds;
    return r.Width();
}

//------------------------------------------------------------------------
Float GFxExternalFontWin32::GetGlyphHeight(UInt glyphIndex) const
{
    if (glyphIndex == (UInt)-1)
        return GetDefaultGlyphHeight();

    const GRectF& r = Glyphs[glyphIndex].Bounds;
    return r.Height();
}

//------------------------------------------------------------------------
GRectF& GFxExternalFontWin32::GetGlyphBounds(UInt glyphIndex, GRectF* prect) const
{
    if (glyphIndex == (UInt)-1)
        prect->SetRect(GetDefaultGlyphWidth(), GetDefaultGlyphHeight());
    else
        *prect = Glyphs[glyphIndex].Bounds;
    return *prect;
}

//------------------------------------------------------------------------
const char*   GFxExternalFontWin32::GetName() const
{
    return &Name[0];
}




//------------------------------------------------------------------------
GFxFontProviderWin32::GFxFontProviderWin32(HDC dc): 
    SysData(dc)
{
    GFxFont::NativeHintingType nhAllFonts;
    nhAllFonts.RasterRange = GFxFont::HintCJK;
    nhAllFonts.VectorRange = GFxFont::DontHint;
    nhAllFonts.MaxRasterHintedSize = 24;
    nhAllFonts.MaxVectorHintedSize = 24;
    NativeHinting.push_back(nhAllFonts);
}

//------------------------------------------------------------------------
GFxFontProviderWin32::~GFxFontProviderWin32()
{    
}

//------------------------------------------------------------------------
GFxFont::NativeHintingType* GFxFontProviderWin32::findNativeHinting(const char* name)
{
    UInt i;
    for (i = 0; i < NativeHinting.size(); ++i)
    {
        if (NativeHinting[i].Typeface.CompareNoCase(name) == 0)
            return &NativeHinting[i];
    }
    return 0;
}

//------------------------------------------------------------------------
void GFxFontProviderWin32::SetHinting(const char* name, 
                                      GFxFont::NativeHintingRange vectorRange, 
                                      GFxFont::NativeHintingRange rasterRange, 
                                      UInt maxVectorHintedSize,
                                      UInt maxRasterHintedSize)
{
    GFxFont::NativeHintingType* pnh = findNativeHinting(name);
    if (pnh == 0)
    {
        GFxFont::NativeHintingType nh = NativeHinting[0];
        nh.Typeface = name;
        NativeHinting.push_back(nh);
        pnh = &NativeHinting[NativeHinting.size()-1];
    }
    pnh->VectorRange         = vectorRange;
    pnh->RasterRange         = rasterRange;
    pnh->MaxVectorHintedSize = maxVectorHintedSize;
    pnh->MaxRasterHintedSize = maxRasterHintedSize;
}

//------------------------------------------------------------------------
void GFxFontProviderWin32::SetHintingAllFonts(GFxFont::NativeHintingRange vectorRange, 
                                              GFxFont::NativeHintingRange rasterRange, 
                                              UInt maxVectorHintedSize,
                                              UInt maxRasterHintedSize)
{
    NativeHinting[0].VectorRange         = vectorRange;
    NativeHinting[0].RasterRange         = rasterRange;
    NativeHinting[0].MaxVectorHintedSize = maxVectorHintedSize;
    NativeHinting[0].MaxRasterHintedSize = maxRasterHintedSize;
}

//------------------------------------------------------------------------
GFxFont* GFxFontProviderWin32::CreateFont(const char* name, UInt fontFlags)
{
//if (fontFlags & GFxFont::FF_BoldItalic) return 0; // DBG

    // Mask flags to be safe.
    fontFlags &= GFxFont::FF_CreateFont_Mask;
    fontFlags |= GFxFont::FF_DeviceFont | GFxFont::FF_NativeHinting;

    // Return a newly created font with a default RefCount of 1.
    // It is users responsibility to cache the font and release
    // it when it is no longer necessary.
    GFxExternalFontWin32* pnewFont =
        new GFxExternalFontWin32(this, &SysData, name, fontFlags);
    if (pnewFont && !pnewFont->IsValid())
    {
        pnewFont->Release();
        return 0;
    }

    GFxFont::NativeHintingType* pnh = findNativeHinting(name);
    if (pnh)
        pnewFont->SetHinting(*pnh);
    else
        pnewFont->SetHinting(NativeHinting[0]);

    return pnewFont;
}



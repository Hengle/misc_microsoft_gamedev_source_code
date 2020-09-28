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
#include "GFxShapeCache.h"
#include "GFxString.h"
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
    if (WinHFont)
        ::DeleteObject(WinHFont);
}

//------------------------------------------------------------------------
GFxExternalFontWin32::GFxExternalFontWin32(GFxFontProviderWin32 *pprovider,
                                           GFxFontSysDataWin32* sysData, 
                                           const char* name, 
                                           UInt fontFlags)
    : GFxFont(fontFlags),
      pFontProvider(pprovider),
      pSysData(sysData)
{
    Name.resize(gfc_strlen(name) + 1);
    gfc_strcpy(&Name[0], gfc_strlen(name) + 1, name);
   
    // Font name is encoded as UTF8, so unpack.
    GTL::garray<WCHAR> nameW;
    nameW.resize(GUTF8Util::GetLength(name) + 1);
    GUTF8Util::DecodeString(&nameW[0], name);

    WinHFont = ::CreateFontW(-FontHeight,                 // height of font
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
                              &nameW[0]);                 // typeface name

    if(WinHFont)
    {
        TEXTMETRICW tm;
        GFxGdiSelectObjectGuard g1(pSysData->WinHDC, WinHFont);
        ::GetTextMetricsW(pSysData->WinHDC, &tm);
        SetFontMetrics(Float(tm.tmExternalLeading), 
                       Float(tm.tmAscent), 
                       Float(tm.tmDescent));
        loadKerningPairs();
    }
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
    if(pairs.size() > 1)
    {
        // Check to see if the kerning pairs are sorted and
        // sort them if necessary.
        //----------------
        for(UInt i = 1; i < pairs.size(); ++i)
        {
            KerningPairType pair;
            pair.Char0 = pairs[i].wFirst;
            pair.Char1 = pairs[i].wSecond;
            KerningPairs.add(pair, Float(pairs[i].iKernAmount));
        }
    }
}

//------------------------------------------------------------------------
void GFxExternalFontWin32::decomposeGlyphOutline(const UByte* data, 
                                                 UInt size, 
                                                 GFxShapeCharacterDef* shape)
{
    const UByte* curGlyph = data;
    const UByte* endGlyph = data + size;
    
    GFxPathPacker path;

    while(curGlyph < endGlyph)
    {
        const TTPOLYGONHEADER* th = (TTPOLYGONHEADER*)curGlyph;
        
        const UByte* endPoly = curGlyph + th->cb;
        const UByte* curPoly = curGlyph + sizeof(TTPOLYGONHEADER);

        path.Reset();
        path.SetFill0(1);
        path.SetFill1(0);
        path.SetMoveTo(th->pfxStart.x.value, -th->pfxStart.y.value);

        while(curPoly < endPoly)
        {
            const TTPOLYCURVE* pc = (const TTPOLYCURVE*)curPoly;
            
            if (pc->wType == TT_PRIM_LINE)
            {
                int i;
                for (i = 0; i < pc->cpfx; i++)
                {
                    path.LineToAbs(pc->apfx[i].x.value, -pc->apfx[i].y.value);
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
                    path.CurveToAbs(pntB.x.value, -pntB.y.value,
                                    pntC.x.value, -pntC.y.value);
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
int GFxExternalFontWin32::GetGlyphIndex(UInt16 code)
{
    if (WinHFont)
    {
        UInt* indexPtr = CodeTable.get(code);
        if (indexPtr)
            return *indexPtr;

        GFxGdiSelectObjectGuard g1(pSysData->WinHDC, WinHFont);
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
        glyph.Advance       =  Float(gm.gmCellIncX);
        glyph.Bounds.Left   =  Float(gm.gmptGlyphOrigin.x);
        glyph.Bounds.Top    = -Float(gm.gmptGlyphOrigin.y);
        glyph.Bounds.Right  =  glyph.Bounds.Left + Float(gm.gmBlackBoxX);
        glyph.Bounds.Bottom =  glyph.Bounds.Top  + Float(gm.gmBlackBoxY);
        glyph.pShapeNode    =  0;
        glyph.Stamp         =  0;

        Glyphs.push_back(glyph);
        CodeTable.add(code, (UInt)Glyphs.size()-1);
        return (UInt)Glyphs.size()-1;
    }
    return -1;
}

//------------------------------------------------------------------------
GFxShapeCharacterDef* GFxExternalFontWin32::GetGlyphShape(UInt glyphIndex)
{
    if (glyphIndex == (UInt)-1)
        return 0;

    GlyphType& glyph = Glyphs[glyphIndex];

    if (glyph.pShapeNode &&
        glyph.pShapeNode->pShape.GetPtr() &&
        glyph.pShapeNode->Stamp == glyph.Stamp)
    {
        pFontProvider->GetShapeCache()->SendToBack(glyph.pShapeNode);
        return glyph.pShapeNode->pShape;
    }

    glyph.pShapeNode = pFontProvider->GetShapeCache()->GetNewShapeNode();

    GFxGdiSelectObjectGuard g1(pSysData->WinHDC, WinHFont);

    if (pSysData->GlyphBuffer.size() == 0)
        pSysData->GlyphBuffer.resize(GFxFontSysDataWin32::BufSizeInc);

    #ifndef GGO_UNHINTED         // For compatibility with old SDKs.
    #define GGO_UNHINTED 0x0100
    #endif

    GLYPHMETRICS gm;
    int totalSize = 0;
    MAT2 im = { {0,1}, {0,0}, {0,0}, {0,1} };

    // Removed GGO_UNHINTED because it causes some Asian fonts, such
    // as "DFKai-SB" and "PMingLiU", to be seriously corrupted. Hinting
    // should not make a difference anyway at this large size.
    const UINT    format = GGO_NATIVE; // GGO_UNHINTED

    totalSize = ::GetGlyphOutlineW(pSysData->WinHDC,
                                   glyph.Code,
                                   format,
                                   &gm,
                                   (DWORD)pSysData->GlyphBuffer.size(),
                                   &pSysData->GlyphBuffer[0],
                                   &im);

    if (totalSize == GDI_ERROR ||
        totalSize > (int)pSysData->GlyphBuffer.size())
    {
        // An error has occured. Most probably it's because
        // of not enough room in the buffer. So, request for the buffer size,
        // reallocate, and try again. The second call may fail for
        // some other reason (no glyph); in this case there's nothing 
        // else to do, just return -1. 
        // The buffer is reallocated with adding BufSizeInc in order
        // to reduce the number of reallocations when requesting other glyphs.
        //----------------------------------
        totalSize = ::GetGlyphOutlineW(pSysData->WinHDC,
                                       glyph.Code,
                                       format,
                                       &gm, 0, 0, &im);

        if(totalSize == GDI_ERROR) return 0;

        pSysData->GlyphBuffer.resize(totalSize + GFxFontSysDataWin32::BufSizeInc);
        totalSize = ::GetGlyphOutlineW(pSysData->WinHDC,
                                       glyph.Code,
                                       format,
                                       &gm,
                                       (DWORD)pSysData->GlyphBuffer.size(),
                                       &pSysData->GlyphBuffer[0],
                                       &im);
        if (totalSize == GDI_ERROR ||
            totalSize > (int)pSysData->GlyphBuffer.size())
            return 0;
    }

    if (totalSize)
    {
        glyph.pShapeNode->pShape = *new GFxShapeCharacterDef(ShapePageSize);
        glyph.Stamp = glyph.pShapeNode->Stamp;
        decomposeGlyphOutline(&pSysData->GlyphBuffer[0], totalSize, glyph.pShapeNode->pShape);
    }
    return glyph.pShapeNode->pShape;
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
GFxFontProviderWin32::GFxFontProviderWin32(HDC dc, UInt maxNumShapes): 
    SysData(dc), pCache(new GFxShapeCache(maxNumShapes))
{
}

//------------------------------------------------------------------------
GFxFontProviderWin32::~GFxFontProviderWin32()
{    
    delete pCache;
}

//------------------------------------------------------------------------
GFxFont* GFxFontProviderWin32::CreateFont(const char* name, UInt fontFlags)
{
    // Mask flags to be safe.
    fontFlags &= GFxFont::FF_CreateFont_Mask;
    fontFlags |= GFxFont::FF_DeviceFont;

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
    return pnewFont;
}



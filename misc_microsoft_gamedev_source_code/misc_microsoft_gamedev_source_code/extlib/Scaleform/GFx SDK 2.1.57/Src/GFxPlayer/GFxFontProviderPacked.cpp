/**********************************************************************

Filename    :   GFxFontProviderPacked.cpp
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

#include "GFxFontProviderPacked.h"
#include "GFxFontCompactor.h"
#include "GFxShape.h"
#include "GFile.h"




//------------------------------------------------------------------------
GFxExternalFontPacked::GFxExternalFontPacked(GFxFontCompactor* fc, 
                                             const GFxCompactedFont* font,
                                             UInt fontFlags) :
    GFxFont(fontFlags | FF_DeviceFont),
    pFontData(fc), pFont(font)
{
    NominalSize = pFont->GetNominalSize();
    SetFontMetrics(norm(pFont->GetLeading()), 
                   norm(pFont->GetAscent()), 
                   norm(pFont->GetDescent()));
}

//------------------------------------------------------------------------
GFxExternalFontPacked::~GFxExternalFontPacked()
{
}

//------------------------------------------------------------------------
int GFxExternalFontPacked::GetGlyphIndex(UInt16 code)
{
    return pFont->GetGlyphIndex(code);
}

//------------------------------------------------------------------------
GFxShapeCharacterDef* GFxExternalFontPacked::GetGlyphShape(UInt glyphIndex, UInt)
{
    if (glyphIndex == (UInt)-1)
        return 0;

    pShape = *new GFxShapeNoStylesDef(ShapePageSize);

    GFxGlyphPathIterator glyph;
    pFont->GetGlyphShape(glyphIndex, &glyph);

    GFxPathPacker path;

    while(!glyph.IsFinished())
    {
        path.Reset();
        path.SetFill0(1);
        path.SetFill1(0);
        path.SetMoveTo(norm(glyph.GetMoveX()), norm(glyph.GetMoveY()));

        while(!glyph.IsPathFinished())
        {
            SInt edge[5];
            glyph.ReadEdge(edge);
            if (edge[0] == GFxPathDataStorage::Edge_Line)
                path.LineToAbs(norm(edge[1]), norm(edge[2]));
            else
                path.CurveToAbs(norm(edge[1]), norm(edge[2]), 
                                norm(edge[3]), norm(edge[4]));
        }

        path.ClosePath();
        pShape->AddPath(&path);

        glyph.AdvancePath();
    }
    return pShape;
}

//------------------------------------------------------------------------
GFxGlyphRaster* GFxExternalFontPacked::GetGlyphRaster(UInt, UInt)
{
    return 0;
}

//------------------------------------------------------------------------
Float GFxExternalFontPacked::GetAdvance(UInt glyphIndex) const
{
    if (glyphIndex == (UInt)-1)
        return GetDefaultGlyphWidth();

    return norm(pFont->GetAdvance(glyphIndex));
}

//------------------------------------------------------------------------
Float GFxExternalFontPacked::GetKerningAdjustment(UInt lastCode, UInt thisCode) const
{
    return norm(pFont->GetKerningAdjustment(lastCode, thisCode));
}

//------------------------------------------------------------------------
Float GFxExternalFontPacked::GetGlyphWidth(UInt glyphIndex) const
{
    if (glyphIndex == (UInt)-1)
        return GetDefaultGlyphWidth();

    return norm(pFont->GetGlyphWidth(glyphIndex));
}

//------------------------------------------------------------------------
Float GFxExternalFontPacked::GetGlyphHeight(UInt glyphIndex) const
{
    if (glyphIndex == (UInt)-1)
        return GetDefaultGlyphHeight();

    return norm(pFont->GetGlyphHeight(glyphIndex));
}

//------------------------------------------------------------------------
GRectF& GFxExternalFontPacked::GetGlyphBounds(UInt glyphIndex, GRectF* prect) const
{
    if (glyphIndex == (UInt)-1)
        prect->SetRect(GetDefaultGlyphWidth(), GetDefaultGlyphHeight());
    else
        pFont->GetGlyphBounds(glyphIndex, prect);
    prect->Left   = norm(prect->Left);
    prect->Top    = norm(prect->Top);
    prect->Right  = norm(prect->Right);
    prect->Bottom = norm(prect->Bottom);
    return *prect;
}

//------------------------------------------------------------------------
const char* GFxExternalFontPacked::GetName() const
{
    return pFont->GetName();
}




//------------------------------------------------------------------------
GFxFontProviderPacked::GFxFontProviderPacked() :
    FontsAcquired(false)
{
}

//------------------------------------------------------------------------
GFxFontProviderPacked::~GFxFontProviderPacked()
{
}

//------------------------------------------------------------------------
bool GFxFontProviderPacked::LoadFonts(GFxFileOpener* fo, const char* fileName)
{
    GPtr<GFile> file = *fo->OpenFile(fileName);
    if (file.GetPtr() == 0)
        return false;

    if (pFontData.GetPtr() == 0)
        pFontData = *new GFxFontCompactor;

    GPodVector<UByte> buf;
    buf.allocate(4096);

    SInt bytes;
    bool firstBlock = true;
    do
    {
        bytes = file->Read(&buf[0], 4096);
        if (bytes > 0)
        {
            const UByte* dataPtr  = &buf[0];
            UInt         dataSize =  bytes;
            if (pFontData->GetDataSize() && firstBlock)
            {
                if (dataSize < 5 ||
                    dataPtr[0] != 'G' || 
                    dataPtr[1] != 'F' ||
                    dataPtr[2] != 'x' ||
                    dataPtr[3] != 'F')
                {
                    return false;
                }
                dataPtr  += 4;
                dataSize -= 4;
            }
            firstBlock = false;
            pFontData->Deserialize(dataPtr, dataSize);
        }
    }
    while(bytes == 4096);
    return true;
}

//------------------------------------------------------------------------
GFxFont* GFxFontProviderPacked::CreateFont(const char* name, UInt fontFlags)
{
    if (pFontData.GetPtr() == 0)
        return 0;

    if (!FontsAcquired)
        if (!pFontData->AcquireFonts())
            return 0;

    FontsAcquired = true;

    fontFlags &= GFxFont::FF_CreateFont_Mask;

    UInt i;
    for (i = 0; i < pFontData->GetNumFonts(); ++i)
    {
        const GFxCompactedFont& font = pFontData->GetFont(i);
        if (font.MatchFont(name, fontFlags))
            return new GFxExternalFontPacked(pFontData, &pFontData->GetFont(i), fontFlags);
    }
    return 0;
}

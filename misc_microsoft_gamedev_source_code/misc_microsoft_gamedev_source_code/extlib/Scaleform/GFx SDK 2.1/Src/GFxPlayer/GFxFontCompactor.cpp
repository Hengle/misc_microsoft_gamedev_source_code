/**********************************************************************

Filename    :   GFxFontCompactor.cpp
Content     :   
Created     :   2007
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   Compact font data storage

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

For information regarding Commercial License Agreements go to:
online - http://www.scaleform.com/licensing.html or
email  - sales@scaleform.com 

**********************************************************************/

#include "GMath2D.h"
#include "GFxFontCompactor.h"


// Font collection data structure:
//------------------------------------------------------------------------
//
// Font Collection:
//
//      Char                MagicCode[4];        // "GFxF"
//      FontType            Font[];              // while(pos < size)
//
// FontType:
//      Char                Name[];              // Null-terminated, possibly UTF-8 encoded
//      UInt30              Flags;
//      UInt15              nominalSize;
//      SInt30              Ascent;
//      SInt30              Descent;
//      SInt30              Leading;
//      UInt32fixlen        NumGlyphs;
//      UInt32fixlen        TotalGlyphBytes;     // To quckly jump to the tables
//      GlyphType           Glyphs[NumGlyphs];
//      GlyphInfoType       GlyphInfoTable[NumGlyphs];
//      UInt30              KerningTableSize;
//      KerningPairType     KerningPairs[KerningTableSize]; // Must be sorted by Code1,Code2
//      
// GlyphType:
//      SInt15              BoundingBox[4];
//      UInt15              NumContours;
//      ContourType         Contours[NumContours];
//      
// ContourType:
//      SInt15              MoveToX;
//      SInt15              MoveToY;
//      UInt30              NumEdges OR Reference;
//          if  (NumEdges & 1) Go To (NumEdges >> 1) and read edges
//          else NumEdges >>= 1
//      EdgeType            Edges[NumEdges];    // No edgess in case of reference.
//      
// EdgeType:
//      See GFxPathDataStorage.cpp, edge description
//
// GlyphInfoType:
//      UInt16fixlen        GlyphCode;
//      SInt16fixlen        AdvanceX;
//      UInt32fixlen        GlobalOffset;       // Starting from BoundingBox
//
// KerningPair:
//      UInt16fixlen        Char1;
//      UInt16fixlen        Char2;
//      SInt16fixlen        Adjustment;
//
//------------------------------------------------------------------------





//------------------------------------------------------------------------
const SInt GFxFontCompactor_CollinearCurveEpsilon = 5;

//------------------------------------------------------------------------
GFxFontCompactor::GFxFontCompactor() : pContourHash(0), pGlyphHash(0)
{
}

//------------------------------------------------------------------------
GFxFontCompactor::~GFxFontCompactor()
{
    ResetContourHash();
}

//------------------------------------------------------------------------
void GFxFontCompactor::ResetContourHash() 
{ 
    if (pContourHash)
    {
        pContourHash->clear();
        delete pContourHash;
        pContourHash = 0;
    }
    if (pGlyphHash)
    {
        pGlyphHash->clear();
        delete pGlyphHash;
        pGlyphHash = 0;
    }
}

//------------------------------------------------------------------------
void GFxFontCompactor::RemoveAll()
{
    FontData.RemoveAll();
    Glyphs.removeAll();
    TmpVertices.removeAll();
    TmpContours.removeAll();
    GlyphCodes.clear();
    GlyphInfoTable.removeAll();
    KerningTable.removeAll();
    ResetContourHash();
}

//------------------------------------------------------------------------
void GFxFontCompactor::normalizeLastContour()
{
    ContourType& c = TmpContours.last();
    VertexType v2 = TmpVertices.last();
    VertexType v1;
    if ((v2.x & 1) == 0)
    {
        v1 = TmpVertices[c.DataStart];
        if (v1.x == v2.x && v1.y == v2.y)
        {
            c.DataSize--;
            TmpVertices.removeLast();
        }
    }
    if (c.DataSize < 3)
    {
        TmpVertices.cutAt(c.DataStart);
        TmpContours.removeLast();
        return;
    }

    UInt start = 0;
    SInt minX = TmpVertices[c.DataStart].x >> 1;
    SInt minY = TmpVertices[c.DataStart].y;
    UInt i;
    for (i = 1; i < c.DataSize; i++)
    {
        v1 = TmpVertices[c.DataStart + i];
        if (v1.x & 1)
        {
            v1 = TmpVertices[++i + c.DataStart];
        }
        else
        {
            if (v1.y < minY)
            {
                minY = v1.y;
                start = i;
            }
            else
            if (v1.y == minY && (v1.x >> 1) < minX)
            {
                minX = v1.x;
                start = i;
            }
        }
    }

    if (start == 0)
        return;

    TmpContour.removeAll();

    v1 = TmpVertices[c.DataStart + start];
    v1.x &= ~1;
    TmpContour.add(v1);
    for (i = 1; i < c.DataSize; i++)
    {
        ++start;
        v1 = TmpVertices[c.DataStart + start % c.DataSize];
        if (v1.x & 1)
        {
            ++i;
            ++start;
            v2 = TmpVertices[c.DataStart + start % c.DataSize];
            TmpContour.add(v1);
            TmpContour.add(v2);
        }
        else
        {
            if ((v1.x >> 1) != (TmpContour.last().x >> 1) ||
                 v1.y       !=  TmpContour.last().y)
            {
                TmpContour.add(v1);
            }
        }
    }

    TmpVertices.cutAt(c.DataStart);
    for (i = 0; i < TmpContour.size(); i++)
        TmpVertices.add(TmpContour[i]);

    c.DataSize = TmpContour.size();
}

//------------------------------------------------------------------------
inline void GFxFontCompactor::extendBounds(SInt* x1, SInt* y1, SInt* x2, SInt* y2, 
                                           SInt  x,  SInt  y)
{
    if (x < *x1) *x1 = x;
    if (y < *y1) *y1 = y;
    if (x > *x2) *x2 = x;
    if (y > *y2) *y2 = y;
}

//------------------------------------------------------------------------
void GFxFontCompactor::computeBounds(SInt* x1, SInt* y1, SInt* x2, SInt* y2) const
{
    *x1 =  16383;
    *y1 =  16383;
    *x2 = -16383;
    *y2 = -16383;

    UInt i, j;
    for (i = 0; i < TmpContours.size(); ++i)
    {
        const ContourType& c = TmpContours[i];
        VertexType v1 = TmpVertices[c.DataStart];
        v1.x >>= 1;
        extendBounds(x1, y1, x2, y2, v1.x, v1.y);
        for (j = 1; j < c.DataSize; ++j)
        {
            const VertexType& v2 = TmpVertices[c.DataStart + j];
            if(v2.x & 1)
            {
                const VertexType& v3 = TmpVertices[++j + c.DataStart];

                Float t, x, y;
                t = GMath2D::CalcQuadCurveExtremum(Float(v1.x), Float(v2.x >> 1), Float(v3.x >> 1));
                if (t > 0 && t < 1)
                {
                    GMath2D::CalcPointOnQuadCurve(Float(v1.x),      Float(v1.y),
                                                  Float(v2.x >> 1), Float(v2.y),
                                                  Float(v3.x >> 1), Float(v3.y),
                                                  t, &x, &y);
                    extendBounds(x1, y1, x2, y2, SInt(floorf(x) + 0.5f), SInt(floorf(y) + 0.5f));
                }
                t = GMath2D::CalcQuadCurveExtremum(Float(v1.y), Float(v2.y), Float(v3.y));
                if (t > 0 && t < 1)
                {
                    GMath2D::CalcPointOnQuadCurve(Float(v1.x),      Float(v1.y),
                                                  Float(v2.x >> 1), Float(v2.y),
                                                  Float(v3.x >> 1), Float(v3.y),
                                                  t, &x, &y);
                    extendBounds(x1, y1, x2, y2, SInt(floorf(x) + 0.5f), SInt(floorf(y) + 0.5f));
                }
                v1 = v3;
            }
            else
                v1 = v2;

            v1.x >>= 1;
            extendBounds(x1, y1, x2, y2, v1.x, v1.y);
        }
    }
}


//------------------------------------------------------------------------
void GFxFontCompactor::StartFont(const char* name, UInt flags, UInt nominalSize, 
                                 SInt ascent, SInt descent, SInt leading)
{
    if (FontData.GetSize() == 0)
    {
        FontData.WriteChar('G');
        FontData.WriteChar('F');
        FontData.WriteChar('x');
        FontData.WriteChar('F');
    }
    while(*name) 
        FontData.WriteChar(*name++);
    FontData.WriteChar(0);
    FontData.WriteUInt30(flags);
    FontData.WriteUInt15(nominalSize);
    FontData.WriteSInt30(ascent);
    FontData.WriteSInt30(descent);
    FontData.WriteSInt30(leading);
    FontNumGlyphs       = 0;
    FontTotalGlyphBytes = 0;
    FontStartGlyphs     = FontData.GetSize();
    FontData.WriteUInt32fixlen(0); // NumGlyphs
    FontData.WriteUInt32fixlen(0); // TotalGlyphBytes

    GlyphCodes.clear();
    GlyphInfoTable.removeAll();
    KerningTable.removeAll();
}

//------------------------------------------------------------------------
void GFxFontCompactor::StartGlyph()
{
    TmpVertices.removeAll();
    TmpContours.removeAll();
}

//------------------------------------------------------------------------
void GFxFontCompactor::MoveTo(SInt16 x, SInt16 y)
{
    if (TmpContours.size())
        normalizeLastContour();

    ContourType c;
    c.DataStart = TmpVertices.size();
    c.DataSize  = 1;
    TmpContours.add(c);

    VertexType v;
    v.x = x << 1;
    v.y = y;
    TmpVertices.add(v);
}

//------------------------------------------------------------------------
void GFxFontCompactor::LineTo(SInt16 x, SInt16 y)
{
    ContourType& c = TmpContours.last();
    VertexType v;
    if (c.DataSize)
    {
        v = TmpVertices.last(); 
        if (x == (v.x >> 1) && y == v.y)
        {
            c.DataSize--;
            TmpVertices.removeLast();
        }
    }
    v.x = x << 1;
    v.y = y;
    TmpVertices.add(v);
    TmpContours.last().DataSize++;
}

//------------------------------------------------------------------------
void GFxFontCompactor::QuadTo(SInt16 cx, SInt16 cy, SInt16 ax, SInt16 ay)
{
    ContourType& c = TmpContours.last();
    VertexType v;
    if (c.DataSize)
    {
        v = TmpVertices.last(); 
        v.x >>= 1;
        SInt cp = SInt(cx - ax) * SInt(ay - v.y) - SInt(cy - ay) * SInt(ax - v.x);
        if (cp < 0) cp = -cp;
        if (cp <= GFxFontCompactor_CollinearCurveEpsilon)
        {
            LineTo(ax, ay);
            return;
        }
    }
    v.x = (cx << 1) | 1;
    v.y = cy;
    TmpVertices.add(v);
    v.x = (ax << 1) | 1;
    v.y = ay;
    TmpVertices.add(v);
    TmpContours.last().DataSize += 2;
}

//------------------------------------------------------------------------
UInt GFxFontCompactor::navigateToEndGlyph(UInt pos) const
{
    SInt t1;
    UInt numContours;
    pos += FontData.ReadSInt15(pos, &t1);
    pos += FontData.ReadSInt15(pos, &t1);
    pos += FontData.ReadSInt15(pos, &t1);
    pos += FontData.ReadSInt15(pos, &t1);
    pos += FontData.ReadUInt15(pos, &numContours);
    while (numContours--)
    {
        UInt numEdges;
        pos += FontData.ReadSInt15(pos, &t1);
        pos += FontData.ReadSInt15(pos, &t1);
        pos += FontData.ReadUInt30(pos, &numEdges);
        if ((numEdges & 1) == 0)
        {
            numEdges >>= 1;
            while(numEdges--)
            {
                UInt8 edge[10];
                pos += FontData.ReadRawEdge(pos, edge);
            }
        }
    }
    return pos;
}

//------------------------------------------------------------------------
UInt32 GFxFontCompactor::computeGlyphHash(UInt pos) const
{
    UInt32 h = 0;
    UInt   end = navigateToEndGlyph(pos);

    for (; pos < end; ++pos)
        h = ((h << 5) + h) ^ UInt8(FontData.ReadChar(pos));

    return h;
}

//------------------------------------------------------------------------
bool GFxFontCompactor::GlyphsEqual(UInt pos, const GFxFontCompactor& cmpFont, UInt cmpPos) const
{
    UInt end1 =         navigateToEndGlyph(pos);
    UInt end2 = cmpFont.navigateToEndGlyph(cmpPos);

    if (end1 - pos != end2 - cmpPos)
        return false;

    for (; pos < end1; ++pos, ++cmpPos)
    {
        if (UInt8(FontData.ReadChar(pos)) != UInt8(cmpFont.FontData.ReadChar(cmpPos)))
            return false;
    }
    return true;
}

//------------------------------------------------------------------------
void GFxFontCompactor::EndGlyph(bool mergeContours)
{
    GlyphInfoType glyphInfo;
    glyphInfo.GlyphCode     = UInt16(FontNumGlyphs);
    glyphInfo.AdvanceX      = 0;
    glyphInfo.GlobalOffset  = FontData.GetSize();

    if (TmpContours.size())
        normalizeLastContour();

    SInt x1, y1, x2, y2;
    computeBounds(&x1, &y1, &x2, &y2);
    FontData.WriteSInt15(x1);
    FontData.WriteSInt15(y1);
    FontData.WriteSInt15(x2);
    FontData.WriteSInt15(y2);

    FontData.WriteUInt15(TmpContours.size());

    bool newShapesAdded = false;
    if (TmpContours.size())
    {
        UInt i, j;
        SInt x, y, cx, cy, ax, ay;
        for (i = 0; i < TmpContours.size(); ++i)
        {
            const ContourType& c = TmpContours[i];
            const VertexType* v1;

            UInt numEdges = 0;
            for (j = 1; j < c.DataSize; ++j)
            {
                ++numEdges;
                v1 = &TmpVertices[c.DataStart + j];
                if(v1->x & 1)
                    ++j;
            }

            v1 = &TmpVertices[c.DataStart];
            x = v1->x >> 1;
            y = v1->y;
            FontData.WriteSInt15(x);
            FontData.WriteSInt15(y);
            UInt startPath = FontData.GetSize();
            FontData.WriteUInt30(numEdges << 1);

            for (j = 1; j < c.DataSize; ++j)
            {
                v1 = &TmpVertices[c.DataStart + j];
                if(v1->x & 1)
                {
                    const VertexType* v2 = &TmpVertices[++j + c.DataStart];
                    cx = v1->x >> 1;
                    cy = v1->y;
                    ax = v2->x >> 1;
                    ay = v2->y;
                    FontData.WriteQuad(cx-x, cy-y, ax-cx, ay-cy);
                    x = ax;
                    y = ay;
                }
                else
                {
                    ax = v1->x >> 1;
                    ay = v1->y;
                    if (ax == x)
                        FontData.WriteVLine(ay-y);
                    else
                    if (ay == y)
                        FontData.WriteHLine(ax-x);
                    else
                        FontData.WriteLine(ax-x, ay-y);
                    x = ax;
                    y = ay;
                }
            }
            if (mergeContours)
            {
                if (pContourHash == 0)
                    pContourHash = new ContourHashType;

                UInt32 hash = FontData.ComputePathHash(startPath);
                ContourKeyType key(&FontData, hash, startPath);
                const ContourKeyType* found = pContourHash->get(key);
                if (found)
                {
                    FontData.CutAt(startPath);
                    FontData.WriteUInt30((found->DataStart << 1) | 1);
                }
                else
                {
                    pContourHash->add(key);
                    newShapesAdded = true;
                }
            }
        }
    }
    ++FontNumGlyphs;

    UInt startGlyph = glyphInfo.GlobalOffset;
    if (mergeContours && !newShapesAdded)
    {
        if (pGlyphHash == 0)
            pGlyphHash = new GlyphHashType;

        UInt32 hash = computeGlyphHash(glyphInfo.GlobalOffset);

        GlyphKeyType key(this, hash, glyphInfo.GlobalOffset);
        const GlyphKeyType* found = pGlyphHash->get(key);
        if (found)
        {
            FontData.CutAt(glyphInfo.GlobalOffset);
            glyphInfo.GlobalOffset = found->DataStart;
        }
        else
        {
            pGlyphHash->add(key);
        }
    }

    FontTotalGlyphBytes += FontData.GetSize() - startGlyph;
    FontData.UpdateUInt32fixlen(FontStartGlyphs,   FontNumGlyphs);
    FontData.UpdateUInt32fixlen(FontStartGlyphs+4, FontTotalGlyphBytes);

    GlyphInfoTable.add(glyphInfo);
}

//------------------------------------------------------------------------
void GFxFontCompactor::AssignGlyphInfo(UInt glyphIndex, UInt glyphCode, SInt advanceX)
{
    if (glyphIndex < GlyphInfoTable.size())
    {
        GlyphInfoType& glyphInfo = GlyphInfoTable[glyphIndex];
        glyphInfo.GlyphCode = UInt16(glyphCode);
        glyphInfo.AdvanceX  = SInt16(advanceX);
        if (GlyphCodes.get(UInt16(glyphCode)) == 0)
            GlyphCodes.add(UInt16(glyphCode));
    }
}

//------------------------------------------------------------------------
void GFxFontCompactor::EndGlyph(UInt glyphCode, SInt advanceX, bool mergeContours)
{
    EndGlyph(mergeContours);
    AssignGlyphInfo(GlyphInfoTable.size() - 1, glyphCode, advanceX);
}

//------------------------------------------------------------------------
void GFxFontCompactor::AddKerningPair(UInt char1, UInt char2, SInt adjustment)
{
    if (GlyphCodes.get(UInt16(char1)) && GlyphCodes.get(UInt16(char2)))
    {
        KerningPairType kp;
        kp.Char1        = UInt16(char1);
        kp.Char2        = UInt16(char2);
        kp.Adjustment   = adjustment;
        KerningTable.add(kp);
    }
}

//------------------------------------------------------------------------
void GFxFontCompactor::EndFont()
{
    UInt i;

    for (i = 0; i < GlyphInfoTable.size(); ++i)
    {
        const GlyphInfoType& glyphInfo = GlyphInfoTable[i];
        FontData.WriteUInt16fixlen(glyphInfo.GlyphCode);
        FontData.WriteSInt16fixlen(glyphInfo.AdvanceX);
        FontData.WriteUInt32fixlen(glyphInfo.GlobalOffset);
    }

    GAlg::QuickSort(KerningTable, cmpKerningPairs);

    FontData.WriteUInt30(KerningTable.size());
    for (i = 0; i < KerningTable.size(); ++i)
    {
        const KerningPairType& kerningPair = KerningTable[i];
        FontData.WriteUInt16fixlen(kerningPair.Char1);
        FontData.WriteUInt16fixlen(kerningPair.Char2);
        FontData.WriteSInt32fixlen(kerningPair.Adjustment);
    }
}

//------------------------------------------------------------------------
bool GFxFontCompactor::AcquireFonts()
{
    enum { MinFontDataSize = 15 };

    Fonts.clear();
    if (FontData.GetSize() < MinFontDataSize+4)
        return false;

    char magicCode[4];
    magicCode[0] = FontData.ReadChar(0);
    magicCode[1] = FontData.ReadChar(1);
    magicCode[2] = FontData.ReadChar(2);
    magicCode[3] = FontData.ReadChar(3);
    if (magicCode[0] != 'G' || magicCode[1] != 'F' ||
        magicCode[2] != 'x' || magicCode[3] != 'F')
        return false;


    GPodVector<char> name;

    UInt pos = 4;
    UInt i;
    while(pos + MinFontDataSize < FontData.GetSize())
    {
        GPtr<GFxCompactedFont> font = *new GFxCompactedFont(FontData);

        // Read font name
        //--------------------
        i = 0;
        while(FontData.ReadChar(pos + i)) 
            ++i;

        name.allocate(i + 1);
        for(i = 0; i < name.size(); ++i)
            name[i] = FontData.ReadChar(pos + i);

        font->SetName(&name[0]);
        pos += name.size();
        //---------------------

        // Read attributes and sizes;
        UInt flags, nominalSize;
        SInt ascent, descent, leading;
        UInt numGlyphs;
        UInt totalGlyphBytes;
        UInt glyphInfoTablePos;
        UInt kerningTableSize;
        UInt kerningTablePos;
        pos += FontData.ReadUInt30(pos, &flags);
        pos += FontData.ReadUInt15(pos, &nominalSize);
        pos += FontData.ReadSInt30(pos, &ascent);
        pos += FontData.ReadSInt30(pos, &descent);
        pos += FontData.ReadSInt30(pos, &leading);
        numGlyphs       = FontData.ReadUInt32fixlen(pos); pos += 4;
        totalGlyphBytes = FontData.ReadUInt32fixlen(pos); pos += 4;

        // Navigate to glyph info table (code, advanceX, globalOffset)
        pos += totalGlyphBytes;
        glyphInfoTablePos = pos;

        // Navigate to kerning table
        pos += numGlyphs * (2+2+4);
        pos += FontData.ReadUInt30(pos, &kerningTableSize);
        kerningTablePos = pos;

        // Navigate to next font
        pos += kerningTableSize * (2+2+2);

        font->SetAttributes(flags, nominalSize, Float(ascent), Float(descent), Float(leading), 
                            numGlyphs, glyphInfoTablePos, kerningTableSize, kerningTablePos);

        // Fill CodeTable
        for (i = 0; i < numGlyphs; i++)
        {
            UInt code = FontData.ReadUInt16fixlen(glyphInfoTablePos); 
            glyphInfoTablePos += 2+2+4;
            font->AddGlyphCode(code, i);
        }
        Fonts.push_back(font);
    }
    return true;
}


//------------------------------------------------------------------------
void GFxGlyphPathIterator::readPathHeader()
{
    if (NumContours)
    {
        Pos += pDataStorage->ReadSInt15(Pos, &MoveX);
        Pos += pDataStorage->ReadSInt15(Pos, &MoveY);
        Pos += pDataStorage->ReadUInt30(Pos, &NumEdges);

        EdgePos = Pos;
        JumpToPos = true;
        if (NumEdges & 1)
        {
            // Go to the referenced contour
            EdgePos   = NumEdges >> 1;
            EdgePos  += pDataStorage->ReadUInt30(EdgePos, &NumEdges);
            JumpToPos = false;
        }
        NumEdges >>= 1;
    }
}


//------------------------------------------------------------------------
void GFxGlyphPathIterator::ReadEdge(SInt* edge)
{
    EdgePos += pDataStorage->ReadEdge(EdgePos, edge);
    switch(edge[0])
    {
    case GFxPathDataStorage::Edge_HLine:
        MoveX += edge[1];
        edge[0] = GFxPathDataStorage::Edge_Line;
        edge[1] = MoveX;
        edge[2] = MoveY;
        break;

    case GFxPathDataStorage::Edge_VLine:
        MoveY += edge[1];
        edge[0] = GFxPathDataStorage::Edge_Line;
        edge[1] = MoveX;
        edge[2] = MoveY;
        break;

    case GFxPathDataStorage::Edge_Line:
        MoveX += edge[1];
        MoveY += edge[2];
        edge[1] = MoveX;
        edge[2] = MoveY;
        break;

    case GFxPathDataStorage::Edge_Quad:
        MoveX += edge[1];
        MoveY += edge[2];
        edge[1] = MoveX;
        edge[2] = MoveY;
        MoveX += edge[3];
        MoveY += edge[4];
        edge[3] = MoveX;
        edge[4] = MoveY;
        break;
    }
    if (NumEdges)
        --NumEdges;

    if(NumEdges == 0 && JumpToPos)
        Pos = EdgePos;
}

//------------------------------------------------------------------------
Float GFxCompactedFont::GetAdvance(UInt glyphIndex) const
{
    UInt pos = GlyphInfoTablePos + glyphIndex * (2+2+4) + 2;
    return (Float)FontData.ReadSInt16fixlen(pos);
}

//------------------------------------------------------------------------
Float GFxCompactedFont::GetKerningAdjustment(UInt lastCode, UInt thisCode) const
{
    int end = (int)KerningTableSize - 1;
    int beg = 0;
    while(beg <= end)
    {
        int  mid   = (end + beg) / 2;
        UInt pos   = GlyphInfoTablePos + mid * (2+2+2);
        UInt char1 = FontData.ReadUInt16fixlen(pos);
        UInt char2 = FontData.ReadUInt16fixlen(pos + 2);
        if (char1 == lastCode && char2 == thisCode)
        {
            return Float(FontData.ReadSInt16fixlen(pos + 4));
        }
        else
        {
            bool pairLess = (lastCode != char1) ? (lastCode < char1) : (thisCode < char2);
            if (pairLess) end = mid - 1;
            else          beg = mid + 1;
        }
    }
    return 0;
}

//------------------------------------------------------------------------
Float GFxCompactedFont::GetGlyphWidth(UInt glyphIndex) const
{
    GFxGlyphPathIterator it;
    it.ReadBounds(FontData, getGlyphPos(glyphIndex));
    return Float(it.GetWidth());
}

//------------------------------------------------------------------------
Float GFxCompactedFont::GetGlyphHeight(UInt glyphIndex) const
{
    GFxGlyphPathIterator it;
    it.ReadBounds(FontData, getGlyphPos(glyphIndex));
    return Float(it.GetHeight());
}

//------------------------------------------------------------------------
GRectF& GFxCompactedFont::GetGlyphBounds(UInt glyphIndex, GRectF* prect) const
{
    GFxGlyphPathIterator it;
    it.ReadBounds(FontData, getGlyphPos(glyphIndex));
    return it.GetBounds(prect);
}

//------------------------------------------------------------------------
void GFxCompactedFont::GetGlyphShape(UInt glyphIndex, GFxGlyphPathIterator* glyph) const
{
    glyph->StartGlyph(FontData, getGlyphPos(glyphIndex));
}

//------------------------------------------------------------------------
bool GFxCompactedFont::MatchFont(const char* name, UInt flags) const
{
    return Name.CompareNoCase(name) == 0 && flags == Flags;
}


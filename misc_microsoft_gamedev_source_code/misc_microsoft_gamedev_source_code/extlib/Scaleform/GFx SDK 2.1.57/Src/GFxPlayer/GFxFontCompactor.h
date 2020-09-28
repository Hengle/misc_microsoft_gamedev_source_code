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

#ifndef INC_GFxFontCompactor_H
#define INC_GFxFontCompactor_H

#include "GTLTypes.h"
#include "GTypes2DF.h"
#include "GContainers.h"
#include "GRefCount.h"
#include "GFxFont.h"
#include "GFxPathDataStorage.h"



//------------------------------------------------------------------------
class GFxGlyphPathIterator
{
public:
    void ReadBounds(const GFxPathDataStorage& ds, UInt pos)
    {
        SInt t;
        pDataStorage = &ds;
        Pos          = pos;
        Pos += pDataStorage->ReadSInt15(Pos, &t); XMin = SInt16(t);
        Pos += pDataStorage->ReadSInt15(Pos, &t); YMin = SInt16(t);
        Pos += pDataStorage->ReadSInt15(Pos, &t); XMax = SInt16(t);
        Pos += pDataStorage->ReadSInt15(Pos, &t); YMax = SInt16(t);
    }

    void StartGlyph(const GFxPathDataStorage& ds, UInt pos)
    {
        ReadBounds(ds, pos);
        Pos += pDataStorage->ReadUInt15(Pos, &NumContours);
        readPathHeader();
    }

    GRectF& GetBounds(GRectF* r) const { *r = GRectF(XMin, YMin, XMax, YMax); return *r; }
    SInt    GetWidth()   const { return XMax - XMin; }
    SInt    GetHeight()  const { return YMax - YMin; }

    bool    IsFinished() const { return NumContours == 0; }
    void    AdvancePath() { --NumContours; readPathHeader(); }

    SInt    GetMoveX() const { return MoveX; }
    SInt    GetMoveY() const { return MoveY; }

    bool    IsPathFinished() const { return NumEdges == 0; }
    void    ReadEdge(SInt* edge);

private:
    void readPathHeader();

    const GFxPathDataStorage* pDataStorage;
    UInt   Pos;
    SInt16 XMin, YMin, XMax, YMax;
    SInt   MoveX, MoveY;
    UInt   NumContours;
    UInt   NumEdges;
    UInt   EdgePos;
    bool   JumpToPos;
};




//------------------------------------------------------------------------
class GFxFontCompactor;
class GFxCompactedFont : public GRefCountBase<GFxCompactedFont>
{
public:
    GFxCompactedFont(const GFxPathDataStorage& fontData) : FontData(fontData) {}

    int                 GetGlyphIndex(UInt16 code) const;
    Float               GetAdvance(UInt glyphIndex) const;
    Float               GetKerningAdjustment(UInt lastCode, UInt thisCode) const;
    Float               GetGlyphWidth(UInt glyphIndex) const;
    Float               GetGlyphHeight(UInt glyphIndex) const;
    GRectF&             GetGlyphBounds(UInt glyphIndex, GRectF* prect) const;
    void                GetGlyphShape(UInt glyphIndex, GFxGlyphPathIterator* glyph) const;

    inline  UInt        GetNumGlyphs() const    { return NumGlyphs; }
    inline  Float       GetAscent()    const    { return Ascent;  }
    inline  Float       GetDescent()   const    { return Descent; }
    inline  Float       GetLeading()   const    { return Leading; }
    
    inline  UInt        GetNominalSize() const  { return NominalSize; }
    inline  const char* GetName() const         { return Name; }
    inline  UInt        GetFontFlags() const    { return Flags; }

    bool                MatchFont(const char* name, UInt flags) const;

    // Interface for acquiring the font data. Used from GFxFontCompactor
    void                SetName(const char* name) { Name = name; }
    void                SetAttributes(UInt flags, UInt nominalSize,
                                      Float ascent, Float descent, Float leading,
                                      UInt numGlyphs, UInt glyphInfoTablePos,
                                      UInt kerningTableSize, UInt kerningTablePos);
    void                AddGlyphCode(UInt code, UInt index);

private:
    GFxCompactedFont(const GFxCompactedFont&);
    const GFxCompactedFont& operator = (const GFxCompactedFont&);

    UInt getGlyphPos(UInt glyphIndex) const
    {
        return FontData.ReadUInt32fixlen(GlyphInfoTablePos + 
                                         glyphIndex * (2+2+4) + 2+2);
    }

    const GFxPathDataStorage&           FontData;
    UInt                                NumGlyphs;
    UInt                                GlyphInfoTablePos;
    UInt                                KerningTableSize;
    UInt                                KerningTablePos;
    GTL::ghash_identity<UInt16, UInt16> CodeTable;

    GFxString                           Name;
    UInt                                Flags;
    UInt                                NominalSize;
    Float                               Ascent;
    Float                               Descent;
    Float                               Leading;
};

//------------------------------------------------------------------------
inline int GFxCompactedFont::GetGlyphIndex(UInt16 code) const
{
    UInt16 glyphIndex;
    if (CodeTable.get(code, &glyphIndex))
        return glyphIndex;
    return -1;
}

//------------------------------------------------------------------------
inline void GFxCompactedFont::SetAttributes(UInt flags, UInt nominalSize,
                                            Float ascent, Float descent, Float leading,
                                            UInt numGlyphs, UInt glyphInfoTablePos,
                                            UInt kerningTableSize, UInt kerningTablePos)
{
    Flags             = flags;
    NominalSize       = nominalSize;
    Ascent            = ascent;
    Descent           = descent;
    Leading           = leading;
    NumGlyphs         = numGlyphs;
    GlyphInfoTablePos = glyphInfoTablePos;
    KerningTableSize  = kerningTableSize;
    KerningTablePos   = kerningTablePos;
}

//------------------------------------------------------------------------
inline void GFxCompactedFont::AddGlyphCode(UInt code, UInt index)
{
    if (CodeTable.get(UInt16(code)) == 0)
        CodeTable.add(UInt16(code), UInt16(index));
}




//------------------------------------------------------------------------
class GFxFontCompactor : public GRefCountBase<GFxFontCompactor>
{
public:
     GFxFontCompactor();
    ~GFxFontCompactor();

    void RemoveAll();
    void ResetContourHash();

    // Font creation and packing
    //-----------------------
    void StartFont(const char* name, UInt flags, UInt nominalSize, 
                   SInt ascent, SInt descent, SInt leading);

    void StartGlyph();
    void MoveTo(SInt16 x, SInt16 y);
    void LineTo(SInt16 x, SInt16 y);
    void QuadTo(SInt16 cx, SInt16 cy, SInt16 ax, SInt16 ay);
    void EndGlyph(bool mergeContours);
    void EndGlyph(UInt glyphCode, SInt advanceX, bool mergeContours);

    void AssignGlyphInfo(UInt glyphIndex, UInt glyphCode, SInt advanceX);

    void AddKerningPair(UInt char1, UInt char2, SInt adjustment);

    void EndFont();
    //-----------------------

    // Serialization
    //-----------------------
    UInt GetDataSize() const { return FontData.GetSize(); }
    void Serialize(void* ptr, UInt start, UInt size) const
    {
        FontData.Serialize(ptr, start, size);
    }

    // Deserealization
    //-----------------------
    void Deserialize(const void* ptr, UInt size)
    {
         FontData.Deserialize(ptr, size);
    }

    // Acquiring fonts. Call after last EndFont() if necessary, or
    // after last Deserialize()
    //-----------------------
    bool AcquireFonts();

    // Access to acquired fonts
    //-----------------------
    UInt  GetNumFonts()                     const { return (UInt)Fonts.size(); }
    const GFxCompactedFont& GetFont(UInt i) const { return *(Fonts[i]);  }

    bool GlyphsEqual(UInt pos, const GFxFontCompactor& cmpFont, UInt cmpPos) const;

private:
    struct VertexType
    {
        SInt16 x,y;
    };

    struct ContourType
    {
        UInt DataStart;
        UInt DataSize;
    };

    struct GlyphInfoType
    {
        UInt16 GlyphCode;
        SInt16 AdvanceX;
        UInt   GlobalOffset;
    };

    struct ContourKeyType
    {
        const GFxPathDataStorage* pCoord;
        UInt32                    HashValue;
        UInt                      DataStart;

        ContourKeyType() : pCoord(0), HashValue(0), DataStart(0) {}
        ContourKeyType(const GFxPathDataStorage* coord, UInt32 hash, UInt start) :
            pCoord(coord), HashValue(hash), DataStart(start) {}

        size_t operator()(const ContourKeyType& data) const
        {
            return (size_t)data.HashValue;
        }

        bool operator== (const ContourKeyType& cmpWith) const 
        {
            return pCoord->PathsEqual(DataStart, *cmpWith.pCoord, cmpWith.DataStart);
        }
    };

    struct GlyphKeyType
    {
        const GFxFontCompactor* pFont;
        UInt32                  HashValue;
        UInt                    DataStart;

        GlyphKeyType() : pFont(0), HashValue(0), DataStart(0) {}
        GlyphKeyType(const GFxFontCompactor* font, UInt32 hash, UInt start) :
            pFont(font), HashValue(hash), DataStart(start) {}

        size_t operator()(const GlyphKeyType& data) const
        {
            return (size_t)data.HashValue;
        }

        bool operator== (const GlyphKeyType& cmpWith) const 
        {
            return pFont->GlyphsEqual(DataStart, *cmpWith.pFont, cmpWith.DataStart);
        }
    };

    struct KerningPairType
    {
        UInt16 Char1, Char2;
        SInt   Adjustment;
    };

    static bool cmpKerningPairs(KerningPairType& a, KerningPairType& b)
    {
        if (a.Char1 != b.Char1) return a.Char1 < b.Char1;
        return a.Char2 < b.Char2;
    }

    void normalizeLastContour();
    static void extendBounds(SInt* x1, SInt* y1, SInt* x2, SInt* y2, SInt x, SInt y);
    void computeBounds(SInt* x1, SInt* y1, SInt* x2, SInt* y2) const;
    UInt navigateToEndGlyph(UInt pos) const;
    UInt32 computeGlyphHash(UInt pos) const;


    typedef GTL::ghash_set<ContourKeyType, ContourKeyType> ContourHashType;
    typedef GTL::ghash_set<GlyphKeyType,   GlyphKeyType>   GlyphHashType;

    GFxPathDataStorage              FontData;
    GPodBVector<UInt>               Glyphs;
    ContourHashType*                pContourHash;
    GlyphHashType*                  pGlyphHash;
    GPodBVector<VertexType>         TmpVertices;
    GPodBVector<ContourType>        TmpContours;
    GPodBVector<VertexType>         TmpContour;

    GTL::ghash_set<UInt16>          GlyphCodes;
    GPodBVector<GlyphInfoType>      GlyphInfoTable;
    GPodBVector<KerningPairType>    KerningTable;

    UInt                            FontNumGlyphs;
    UInt                            FontTotalGlyphBytes;
    UInt                            FontStartGlyphs;

    GTL::garray<GPtr<GFxCompactedFont> > Fonts;
};














#endif



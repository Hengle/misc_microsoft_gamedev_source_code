/**********************************************************************

Filename    :   GFxFontResource.cpp
Content     :   Representation of SWF/GFX font data and resources.
Created     :   
Authors     :   

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxFontResource.h"
#include "GFxStream.h"
#include "GFxLog.h"
#include "GFxShape.h"
#include "GMath.h"
#include "GStd.h"
#include "GFile.h"

#include "GFxLoadProcess.h"


// ***** GFxTextureGlyph

GImageInfoBase* GFxTextureGlyph::GetImageInfo(const GFxResourceBinding *pbinding) const
{
    GFxImageResource* pimageRes = (*pbinding)[pImage];
    return pimageRes ? pimageRes->GetImageInfo() : 0;
}


// ***** GFxTextureGlyphData


// Return a pointer to a GFxTextureGlyph struct Corresponding to
// the given GlyphIndex, if we have one.  Otherwise return NULL.
const GFxTextureGlyph&  GFxTextureGlyphData::GetTextureGlyph(UInt glyphIndex) const
{
    if (glyphIndex >= TextureGlyphs.size())
    {
        static const GFxTextureGlyph dummyTextureGlyph;
        return dummyTextureGlyph;
    }
    return TextureGlyphs[glyphIndex];
}


// Register some texture info for the glyph at the specified
// index.  The GFxTextureGlyph can be used later to render the glyph.
void    GFxTextureGlyphData::AddTextureGlyph(UInt glyphIndex, const GFxTextureGlyph& glyph)
{
    GASSERT(glyphIndex != (UInt)-1);

    // Resize texture glyphs so that the index will fit.
    if (TextureGlyphs.size() <= glyphIndex)
    {
        TextureGlyphs.resize(glyphIndex+1);
        if (TextureGlyphs.size() <= glyphIndex)
            return;
    }
    
    GASSERT(glyph.HasImageResource());
    GASSERT(TextureGlyphs[glyphIndex].HasImageResource() == false);
    TextureGlyphs[glyphIndex] = glyph;
}


// Delete all our texture glyph info.
void    GFxTextureGlyphData::WipeTextureGlyphs()
{
    //GASSERT(TextureGlyphs.size() == Glyphs.size());

    // Replace with default (empty) glyph info.
    GFxTextureGlyph defaultTg;
    for (UPInt i = 0, n = TextureGlyphs.size(); i < n; i++)
        TextureGlyphs[i] = defaultTg;
}


void GFxTextureGlyphData::VisitTextureGlyphs(TextureGlyphVisitor* pvisitor)
{
    GASSERT(pvisitor);

    for(UPInt i = 0, n = TextureGlyphs.size(); i < n; ++i)    
        pvisitor->Visit((UInt)i, &TextureGlyphs[i]);    
}

Float GFxTextureGlyphData::GetTextureGlyphScale() const
{
    return GFxFontPackParams::GlyphBoundBox / PackTextureConfig.NominalSize;
}

void GFxTextureGlyphData::AddTexture(GFxResourceId textureId, GFxImageResource* pimageRes)
{
    GFxResourcePtr<GFxImageResource> pres = pimageRes;
    GlyphsTextures.set(textureId, pres);
}
void GFxTextureGlyphData::AddTexture(GFxResourceId textureId, const GFxResourceHandle &rh)
{
    GFxResourcePtr<GFxImageResource> pres;
    pres.SetFromHandle(rh);
    GlyphsTextures.set(textureId, pres);
}


void GFxTextureGlyphData::VisitTextures(TexturesVisitor* pvisitor, GFxResourceBinding* pbinding)
{
    GASSERT(pvisitor);

    ImageResourceHash::const_iterator iter = GlyphsTextures.begin();
    for (; iter != GlyphsTextures.end(); ++iter)
    {
        GFxResource* pres = (*pbinding) [iter->second];
        GASSERT(pres->GetResourceType() == GFxResource::RT_Image);        
        pvisitor->Visit(iter->first, (GFxImageResource*) pres);
    }
}



// ***** GFxFontData


GFxFontData::GFxFontData()   
   : GFxFont(), Name(NULL)
{
}

GFxFontData::GFxFontData(const char* name, UInt fontFlags)
    : GFxFont(fontFlags), Name(NULL)
{
    Name = (char*)GALLOC(gfc_strlen(name) + 1);
    if (Name)
        gfc_strcpy(Name, gfc_strlen(name) + 1, name);

    SetHasLayout(1);
}


GFxFontData::~GFxFontData()
{
    Glyphs.resize(0);

    // Delete the name string.
    if (Name)
    {
        GFREE(Name);
        Name = NULL;
    }
}


void    GFxFontData::Read(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT(tagInfo.TagType == GFxTag_DefineFont ||
            tagInfo.TagType == GFxTag_DefineFont2 ||
            tagInfo.TagType == GFxTag_DefineFont3);

    GFxStream * pin = p->GetStream();
    bool        isLoadingFontShapes = p->IsLoadingFontShapes();

    // No AddRef() here, to avoid cycle.
    // OwningMovie is our owner, so it has a ref to us.
    //OwningMovie = p->GetDataDef();    

    if (tagInfo.TagType == GFxTag_DefineFont)
    {
        pin->LogParse("reading DefineFont\n");

        int tableBase = pin->Tell();

        // Read the glyph offsets.  Offsets
        // are measured from the start of the
        // offset table.
        GTL::garray<int>    offsets;
        offsets.push_back(pin->ReadU16());
        pin->LogParse("offset[0] = %d\n", offsets[0]);
        int count = offsets[0] >> 1;
        for (int i = 1; i < count; i++)
        {
            UInt16 off = pin->ReadU16();
            if (off == 0)
            {
                // special case for tag with stripped font shapes
                isLoadingFontShapes = false;
                break;
            }
            offsets.push_back(off);
            pin->LogParse("offset[%d] = %d\n", i, offsets[i]);
        }

        Glyphs.resize(count);
      //  TextureGlyphs.resize(Glyphs.size());

        if (isLoadingFontShapes)
        {
            // Read the glyph shapes.
            for (int i = 0; i < count; i++)
            {
                // Seek to the start of the shape data.
                int newPos = tableBase + offsets[i];
                pin->SetPosition(newPos);
                
                SPInt shapeDataLen;
                if (i + 1 < count)
                    shapeDataLen = offsets[i + 1] - offsets[i];
                else
                    shapeDataLen = tagInfo.TagDataOffset + tagInfo.TagLength - newPos;

                // Create & read the shape.
                GPtr<GFxConstShapeNoStylesDef> s = *new GFxConstShapeNoStylesDef;
                s->Read(p, GFxTag_DefineShape, (UInt)shapeDataLen, false);

                Glyphs[i] = s;
            }
        }
        else
            SetGlyphShapesStripped();
    }
    else if ((tagInfo.TagType == GFxTag_DefineFont2) || (tagInfo.TagType == GFxTag_DefineFont3))
    {
        if (tagInfo.TagType == GFxTag_DefineFont2)
            pin->LogParse("reading DefineFont2: ");
        else
            pin->LogParse("reading DefineFont3: ");

        bool    hasLayout = (pin->ReadUInt(1) != 0);
        SetHasLayout(hasLayout);

        bool shiftJisFlag        = (pin->ReadUInt(1) != 0);
        bool pixelAlignedChars   = (pin->ReadUInt(1) != 0);
        bool ansiFlag            = (pin->ReadUInt(1) != 0);

        if (shiftJisFlag)  SetCodePage(FF_CodePage_ShiftJis);
        else if (ansiFlag) SetCodePage(FF_CodePage_Ansi);
        else               SetCodePage(FF_CodePage_Unicode);
        
        GUNUSED(pixelAlignedChars);       
        
        bool    wideOffsets = (pin->ReadUInt(1) != 0);
        SetWideCodes(pin->ReadUInt(1) != 0);
        SetItalic(pin->ReadUInt(1) != 0);
        SetBold(pin->ReadUInt(1) != 0);
        UByte   langCode = pin->ReadU8(); // Language code

        // Inhibit warning.
        langCode = langCode;

        Name = pin->ReadStringWithLength();

        int glyphCount = pin->ReadU16();

        if (pin->IsVerboseParse())
        {
            pin->LogParse("  Name = %s, %d glyphs\n", Name ? Name : "(none)", glyphCount);

            char* pcodePage = "Unicode";
            if (GetCodePage() == FF_CodePage_ShiftJis)
                pcodePage = "ShiftJIS";
            else if (GetCodePage() == FF_CodePage_Ansi)
                pcodePage = "ANSI";

            pin->LogParse("  HasLayout = %d, CodePage = %s, Italic = %d, Bold = %d\n", 
                (int)hasLayout, pcodePage, IsItalic(), IsBold());
            pin->LogParse("  LangCode = %d\n", langCode);
        }

        
        int tableBase = pin->Tell();

        // Read the glyph offsets.  Offsets
        // are measured from the start of the
        // offset table.
        GTL::garray<int>    offsets;
        int fontCodeOffset;

        int offsetsCount = glyphCount;
        if (glyphCount > 0)
        {
            // check for the first offset. If it is 0 then shapes
            // were stripped from the tag.
            UInt off0 = (wideOffsets) ? (UInt)pin->ReadU32() : (UInt)pin->ReadU16();
            if (off0 == 0)
            {
                // special case for tags with stripped font shapes
                isLoadingFontShapes = false;
                offsetsCount = 0;
            }
            else
                offsets.push_back(off0);
        }
        if (wideOffsets)
        {
            // 32-bit offsets.
            for (int i = 1; i < offsetsCount; i++)
            {
                offsets.push_back(pin->ReadU32());
            }
            fontCodeOffset = pin->ReadU32();
        }
        else
        {
            // 16-bit offsets.
            for (int i = 1; i < offsetsCount; i++)
            {
                offsets.push_back(pin->ReadU16());
            }
            fontCodeOffset = pin->ReadU16();
        }

        Glyphs.resize(glyphCount);
    //    TextureGlyphs.resize(Glyphs.size());

        if (isLoadingFontShapes)
        {
            GFxTagType fontGlyphShapeTag = (tagInfo.TagType == GFxTag_DefineFont2) ?
                                           GFxTag_DefineShape2 : tagInfo.TagType;

            if (hasLayout)
            {
                AdvanceTable.resize(glyphCount);
            }

            // Read the glyph shapes.
            for (int i = 0; i < glyphCount; i++)
            {
                // Seek to the start of the shape data.
                int newPos = tableBase + offsets[i];
                // if we're seeking backwards, then that looks like a bug.
                GASSERT(newPos >= pin->Tell());
                pin->SetPosition(newPos);

                SPInt shapeDataLen;
                if (i + 1 < glyphCount)
                    shapeDataLen = offsets[i + 1] - offsets[i];
                else
                    shapeDataLen = fontCodeOffset - offsets[i];
                GASSERT(shapeDataLen >= 0);

                // Create & read the shape.
                GPtr<GFxConstShapeNoStylesDef> s = *new GFxConstShapeNoStylesDef;
                s->Read(p, fontGlyphShapeTag, (UInt)shapeDataLen, false);

                Glyphs[i] = s;

                if (hasLayout)
                {
                    GRectF bounds;
                    s->ComputeBound(&bounds);
                    AdvanceEntry& ge = AdvanceTable[i];
                    if (bounds.IsNormal())
                    {
                        Float left = bounds.Left;
                        Float top  = bounds.Top;
                        Float w = bounds.Width();
                        Float h = bounds.Height();
                        /*
                        GASSERT(left >= -32768./20 && left < 32767./20 && 
                            top  >= -32768./20 && top < 32767./20 && 
                            w < 65535./20 && h < 65535./20);
                            */
                        ge.Left    = (SInt16)(left * 20.f);
                        ge.Top     = (SInt16)(top * 20.f);
                        ge.Width   = (UInt16)(w * 20.f);
                        ge.Height  = (UInt16)(h * 20.f);
                    }
                    else
                    {
                        ge.Left  = ge.Top = 0;
                        ge.Width = ge.Height = 0;
                    }
                }
            }

            int currentPosition = pin->Tell();
            if (fontCodeOffset + tableBase != currentPosition)
            {
                // Bad offset!  Don't try to read any more.
                return;
            }
        }
        else
        {
            // Skip the shape data.
            int newPos = tableBase + fontCodeOffset;
            if (newPos >= pin->GetTagEndPosition())
            {
                // No layout data!
                return;
            }

            pin->SetPosition(newPos);
            SetGlyphShapesStripped();
        }

        ReadCodeTable(pin);

        // Read layout info for the glyphs.
        if (hasLayout)
        {
            // Multiplier used for scaling.
            // In SWF 8, font shape constant resolution (tag 75) was increased by 20.
            Float sfactor = (tagInfo.TagType == GFxTag_DefineFont3)  ? 1.0f / 20.0f : 1.0f;

            Ascent = (Float) pin->ReadS16() * sfactor;
            Descent = (Float) pin->ReadS16() * sfactor;
            Leading = (Float) pin->ReadS16() * sfactor;
            
            if (pin->IsVerboseParse())
            {
                pin->LogParse("  Ascent = %d, Descent = %d, Leading = %d\n", 
                    (int)Ascent, (int)Descent, (int)Leading);
            }

            // Advance table; i.E. how wide each character is.
            if (AdvanceTable.size() != Glyphs.size())
                AdvanceTable.resize(Glyphs.size());
            for (UPInt i = 0, n = AdvanceTable.size(); i < n; i++)
            {
                AdvanceEntry& ge = AdvanceTable[i];
                ge.Advance = (Float) pin->ReadU16() * sfactor;
            }

            // Bounds table.
            //BoundsTable.resize(Glyphs.size());    // kill
            GRectF  dummyRect;
            {for (UPInt i = 0, n = Glyphs.size(); i < n; i++)
            {
                //BoundsTable[i].Read(pin);  // kill
                pin->ReadRect(&dummyRect);                   
            }}

            // Kerning pairs.
            int kerningCount = pin->ReadU16();
            if (pin->IsVerboseParse())
            {
                pin->LogParse("  KerningCount = %d\n", (int)kerningCount);
            }
            {for (int i = 0; i < kerningCount; i++)
            {
                UInt16  char0, char1;
                if (AreWideCodes())
                {
                    char0 = pin->ReadU16();
                    char1 = pin->ReadU16();
                }
                else
                {
                    char0 = pin->ReadU8();
                    char1 = pin->ReadU8();
                }
                Float   adjustment = (Float) pin->ReadS16() * sfactor;

                KerningPair k;
                k.Char0 = char0;
                k.Char1 = char1;

                if (pin->IsVerboseParse())
                    pin->LogParse("     Pair: %d - %d,\tadj = %d\n", (int)char0, (int)char1, (int)adjustment);

                // Remember this adjustment; we can look it up quickly
                // later using the character pair as the key.
                KerningPairs.add(k, adjustment);
            }}
        }
    }
}


// Read additional information about this GFxFontData, from a
// DefineFontInfo tag.  The caller has already read the tag
// type and GFxFontData id.
void    GFxFontData::ReadFontInfo(GFxStream* in, GFxTagType tagType)
{
    if (Name)
    {
        GFREE(Name);
        Name = NULL;
    }

    Name = in->ReadStringWithLength();

    UByte flags = in->ReadU8();

    UByte langCode = 0;
    if (tagType == GFxTag_DefineFontInfo2)
    {
        langCode = in->ReadU8(); // Language code
        langCode = langCode; // suppress warning
    }

    bool pixelAlignedChars  =  ((flags & 0x20) != 0);
    bool shiftJisFlag        = ((flags & 0x10) != 0);
    bool ansiFlag            = ((flags & 0x08) != 0);

    if (shiftJisFlag)  SetCodePage(FF_CodePage_ShiftJis);
    else if (ansiFlag) SetCodePage(FF_CodePage_Ansi);
    else               SetCodePage(FF_CodePage_Unicode);
    GUNUSED(pixelAlignedChars);
    
    SetItalic       ((flags & 0x04) != 0);
    SetBold         ((flags & 0x02) != 0);
    SetWideCodes    ((flags & 0x01) != 0);

    if (in->IsVerboseParse())
    {
        if (tagType == GFxTag_DefineFontInfo)        
            in->LogParse("reading DefineFontInfo\n");
        else
            in->LogParse("reading DefineFontInfo2\n");
        in->LogParse("  Name = %s\n", Name ? Name : "(none)");

        char* pcodePage = "Unicode";
        if (GetCodePage() == FF_CodePage_ShiftJis)
            pcodePage = "ShiftJIS";
        else if (GetCodePage() == FF_CodePage_Ansi)
            pcodePage = "ANSI";

        in->LogParse("  CodePage = %s, Italic = %d, Bold = %d\n", 
            pcodePage, IsItalic(), IsBold());
        if (tagType == GFxTag_DefineFontInfo2)
            in->LogParse("  LangCode = %d\n", langCode);
    }

    ReadCodeTable(in);
}


// Read the table that maps from glyph indices to character codes.
void    GFxFontData::ReadCodeTable(GFxStream* in)
{
    in->LogParse("reading code table at offset %d\n", in->Tell());

    GASSERT(CodeTable.is_empty());

    if (AreWideCodes())
    {
        // Code table is made of UInt16's.
        for (UPInt i = 0, n = Glyphs.size(); i < n; i++)            
            CodeTable.add(in->ReadU16(), (UInt)i);            
    }
    else
    {
        // Code table is made of bytes.
        for (UPInt i = 0, n = Glyphs.size(); i < n; i++)
            CodeTable.add(in->ReadU8(), (UInt)i); 
    }
}

int GFxFontData::GetGlyphIndex(UInt16 code)
{
    UInt glyphIndex;
    if (CodeTable.get(code, &glyphIndex))
    {
        return glyphIndex;
    }
    return -1;
}


GFxShapeCharacterDef*   GFxFontData::GetGlyphShape(UInt index, UInt)
{
    if (index < Glyphs.size())
    {
        return Glyphs[index].GetPtr();
    }
    else
    {
        return NULL;
    }
}

Float   GFxFontData::GetAdvance(UInt GlyphIndex) const
{
    // how could this be if GlyphIndex is unsigned?
    if (GlyphIndex == (UInt)-1)
        return GetDefaultGlyphWidth();

    if (AdvanceTable.size() == 0)
    {
        // No layout info for this GFxFontData!!!
        static bool Logged = false;
        if (Logged == false)
        {
            Logged = true;
            GFC_DEBUG_ERROR1(1, "Empty advance table in font %s", GetName());
        }
        
        // MA: It is better to return default advance, make char substitution show up better
        return GetDefaultGlyphWidth();        
    }

    if (GlyphIndex < AdvanceTable.size())
    {
        GASSERT(GlyphIndex != (UInt)-1 );
        return AdvanceTable[GlyphIndex].Advance;
    }
    else
    {
        // Bad glyph index.  Due to bad data file?
        GASSERT(0);
        return 0;
    }
}

Float   GFxFontData::GetGlyphWidth(UInt glyphIndex) const
{
    if (glyphIndex == (UInt)-1)
        return GetDefaultGlyphWidth();
    
    if (glyphIndex < AdvanceTable.size())
    {
        GASSERT(glyphIndex != (UInt)-1 );
        Float w = Float(AdvanceTable[glyphIndex].Width)/20.f;
        if (w != 0)
            return w;
    }
    return GetAdvance(glyphIndex); 
}

Float   GFxFontData::GetGlyphHeight(UInt glyphIndex) const
{
    // How could this be if GlyphIndex is unsigned?
    if (glyphIndex == (UInt)-1 || AdvanceTable.size() == 0)    
        return GetDefaultGlyphHeight();

    if (glyphIndex < AdvanceTable.size())
    {
        GASSERT(glyphIndex != (UInt)-1 );
        return Float(AdvanceTable[glyphIndex].Height)/20.f;
    }
    else
    {
        // Bad glyph index.  Due to bad data file?
        // This is possible, if font doesn't have layout (for static text).
        // But AdvanceTable should be empty in this case.
        GASSERT(AdvanceTable.size() == 0);
        return 0;
    }
}

GRectF& GFxFontData::GetGlyphBounds(UInt glyphIndex, GRectF* prect) const
{
    if (glyphIndex == (UInt)-1)
    {
        prect->Left = prect->Top = 0;
        prect->SetWidth(GetGlyphWidth(glyphIndex));
        prect->SetHeight(GetGlyphHeight(glyphIndex));
    }
    else if (glyphIndex < AdvanceTable.size())
    {
        GASSERT(glyphIndex != (UInt)-1 );
        const AdvanceEntry& e = AdvanceTable[glyphIndex];
        Float w = Float(e.Width)/20.f;
        if (w == 0)
            w = e.Advance;
        Float h = Float(e.Height)/20.f;
        prect->Left = Float(e.Left)/20.f;
        prect->Top  = Float(e.Top)/20.f;
        prect->SetWidth(w);
        prect->SetHeight(h);
    }
    else
    {
        // Bad glyph index.  Due to bad data file?
        // This is possible, if font doesn't have layout (for static text).
        // But AdvanceTable should be empty in this case.
        //GASSERT(AdvanceTable.size() == 0);
        GFC_DEBUG_ERROR3((AdvanceTable.size() != 0), 
            "Glyph index %d exceeds advance table (size = %d), font = %s", 
            glyphIndex, (int)AdvanceTable.size(), GetName());

        prect->Left = prect->Top = 0;
        prect->SetWidth(0);
        prect->SetHeight(0);

        // so try to calculate the bounds
        if (glyphIndex < Glyphs.size())
        {
            GFxShapeCharacterDef* s = Glyphs[glyphIndex];
            if (s)
            {
                GRectF bounds;
                s->ComputeBound(&bounds);
                if (bounds.IsNormal())
                {
                    Float left = bounds.Left;
                    Float top  = bounds.Top;
                    Float w = bounds.Width();
                    Float h = bounds.Height();
                    prect->Left    = left;
                    prect->Top     = top;
                    prect->SetWidth(w);
                    prect->SetHeight(h);
                }
            }
        }
    }
    return *prect;
}

// Return the adjustment in advance between the given two
// characters.  Normally this will be 0; i.E. the 
Float   GFxFontData::GetKerningAdjustment(UInt LastCode, UInt code) const
{
    Float   adjustment;
    KerningPair k;
    k.Char0 = (UInt16)LastCode;
    k.Char1 = (UInt16)code;
    if (KerningPairs.get(k, &adjustment))
    {
        return adjustment;
    }
    return 0;
}

static GFxFontResourceCreator static_inst;
GFxResourceData    GFxFontData::CreateFontResourceData()
{
    return GFxResourceData(&static_inst, this);
}


// ***** GFxFontResource

GFxFontResource::GFxFontResource(GFxFont *pfont, GFxResourceBinding* pbinding)
    : pBinding(pbinding)
{    
    pFont           = pfont;
    pTGData         = pfont ? pfont->GetTextureGlyphData() : 0;
    HandlerArrayFlag= 0;
    pHandler        = 0;
    LowerCaseTop    = 0;
    UpperCaseTop    = 0;
}

GFxFontResource::GFxFontResource(GFxFont *pfont, const GFxResourceKey& key)
    : pBinding(0)
{    
    pFont           = pfont;
    pTGData         = pfont ? pfont->GetTextureGlyphData() : 0;
    HandlerArrayFlag= 0;
    pHandler        = 0;
    FontKey         = key;
    LowerCaseTop    = 0;
    UpperCaseTop    = 0;
}


UInt16 GFxFontResource::calcTopBound(UInt16 code)
{
    GFxShapeCharacterDef* shape;
    GRectF bounds;

    int idx = pFont->GetGlyphIndex(code);
    if (idx != -1)
    {
        shape = GetGlyphShape(idx, 0);
        if (shape)
        {
            shape->ComputeBound(&bounds);
            return UInt16(-bounds.Top);
        }
    }
    return 0;
}


void GFxFontResource::calcLowerUpperTop(GFxLog* log)
{
    SInt16 lowerCaseTop = 0;
    SInt16 upperCaseTop = 0;
    if (pFont && LowerCaseTop == 0 && UpperCaseTop == 0)
    {
        const UByte upperCaseCandidates[] = "HEFTUVWXZ";
        const UByte lowerCaseCandidates[] = "zxvwy";
        
        const UByte* p;
        for (p = upperCaseCandidates; *p; ++p)
        {
            upperCaseTop = calcTopBound(*p);
            if (upperCaseTop)
                break;
        }

        if (upperCaseTop)
        {
            for (p = lowerCaseCandidates; *p; ++p)
            {
                lowerCaseTop = calcTopBound(*p);
                if (lowerCaseTop)
                    break;
            }
        }
    }
    if (lowerCaseTop && upperCaseTop)
    {
        LowerCaseTop = lowerCaseTop;
        UpperCaseTop = upperCaseTop;
    }
    else
    {
        if (log)
            log->LogWarning("Warning: Font '%s%s%s': No hinting chars "
                            "(any of 'HEFTUVWXZ' and 'zxvwy'). Auto-Hinting is disabled\n", 
                            GetName(), 
                            IsBold() ? " Bold" : "", 
                            IsItalic() ? " Italic" : "");
        LowerCaseTop = -1;
        UpperCaseTop = -1;
    }
}


GFxFontResource::~GFxFontResource()
{
    if (HandlerArrayFlag)
    {
        for (UInt i=0; i<pHandlerArray->size(); i++)
            (*pHandlerArray)[i]->OnDispose(this);
    }
    else if (pHandler)
    {
        pHandler->OnDispose(this);
    }
}


// Add/Remove notification
void GFxFontResource::AddDisposeHandler(DisposeHandler *handler)
{
    if (pHandler)
    {
        if (!HandlerArrayFlag)              
        {
            DisposeHandler *oldHandler = handler;
            if ((pHandlerArray = new GTL::garray<DisposeHandler*>)!=0)
            {
                pHandlerArray->push_back(oldHandler);
                HandlerArrayFlag = 1;
            }
            else
                return;
        }
        pHandlerArray->push_back(handler);
        return;
    }
    // If no array, just store handler
    pHandler = handler;
}

void GFxFontResource::RemoveDisposeHandler(DisposeHandler* handler)
{
    if (HandlerArrayFlag)
    {
        for (UInt i=0; i<pHandlerArray->size(); i++)
        {
            if ((*pHandlerArray)[i] == handler)
            {
                // Handler found -> remove
                pHandlerArray->remove(i);
                if (pHandlerArray->size() == 1)
                {                       
                    DisposeHandler *oldHandler = (*pHandlerArray)[0];
                    delete pHandlerArray;
                    pHandler = oldHandler;
                    HandlerArrayFlag = 0;
                    return;
                }
            }           
        }
    }
    else if (pHandler == handler)
    {
        pHandler = 0;
    }
}



// *** System FontProvider resource key

// We store system fonts obtained from GFxFontProvider is ResourceLib so
// that they are re-used it requested several times.

class GFxSystemFontResourceKey : public GRefCountBase<GFxSystemFontResourceKey>
{
    // Image States.  
    GPtr<GFxFontProvider>   pFontProvider;
    GFxString               FontName;
    UInt                    CreateFontFlags;
public:

    GFxSystemFontResourceKey(const char* pname, UInt fontFlags,
                             GFxFontProvider* pfontProvider)
    {
        SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
        FontName        = GFxString(pname).ToLower(); // Font names are case-insensitive.   
        CreateFontFlags = fontFlags & GFxFont::FF_Style_Mask;
        pFontProvider   = pfontProvider;
    }

    bool operator == (GFxSystemFontResourceKey& other) const
    {
        return (FontName == other.FontName &&
                pFontProvider == other.pFontProvider &&
                CreateFontFlags == other.CreateFontFlags);
    }

    size_t  GetHashCode() const
    {
        size_t hashCode = GFxString::BernsteinHashFunctionCIS(
                            FontName.ToCStr(), FontName.GetSize());
        return CreateFontFlags ^ hashCode ^            
            ((size_t)pFontProvider.GetPtr()) ^ (((size_t)pFontProvider.GetPtr()) >> 7);
    }
};

class GFxSystemFontResourceKeyInterface : public GFxResourceKey::KeyInterface
{
public:
    typedef GFxResourceKey::KeyHandle KeyHandle;    

    // GFxResourceKey::KeyInterface implementation.    
    virtual void                AddRef(KeyHandle hdata)     { ((GFxSystemFontResourceKey*) hdata)->AddRef(); }
    virtual void                Release(KeyHandle hdata)    { ((GFxSystemFontResourceKey*) hdata)->Release(); }
    
    virtual GFxResourceKey::KeyType GetKeyType(KeyHandle hdata) const
    {
        GUNUSED(hdata);
        return GFxResourceKey::Key_None;
    }
    virtual size_t              GetHashCode(KeyHandle hdata) const
    {
        return ((GFxSystemFontResourceKey*) hdata)->GetHashCode();
    }

    virtual bool                KeyEquals(KeyHandle hdata, const GFxResourceKey& other);
};

static GFxSystemFontResourceKeyInterface GFxSystemFontResourceKeyInterface_Instance;

bool    GFxSystemFontResourceKeyInterface::KeyEquals(KeyHandle hdata, const GFxResourceKey& other)
{
    if (this != other.GetKeyInterface())
        return 0;

    GFxSystemFontResourceKey* pthisData = (GFxSystemFontResourceKey*) hdata;
    GFxSystemFontResourceKey* potherData = (GFxSystemFontResourceKey*) other.GetKeyData();
    GASSERT(pthisData && potherData);    
    return (*pthisData == *potherData);
}


GFxResourceKey GFxFontResource::CreateFontResourceKey(const char* pname, UInt fontFlags,
                                                      GFxFontProvider* pfontProvider)
{
    GPtr<GFxSystemFontResourceKey> pdata =
        *new GFxSystemFontResourceKey(pname, fontFlags, pfontProvider);

    return GFxResourceKey(&GFxSystemFontResourceKeyInterface_Instance,
                          (GFxResourceKey::KeyHandle)pdata.GetPtr() );
}



// Static helper function used to lookup and/or create a font resource from provider.
GFxFontResource* GFxFontResource::CreateFontResource(const char* pname, UInt fontFlags,
                                                     GFxFontProvider* pprovider,
                                                     GFxResourceWeakLib *plib)
{
    GASSERT(pname && pprovider && plib);

    // Create a search key.
    GFxResourceKey              fontKey = CreateFontResourceKey(pname, fontFlags, pprovider);
    GFxResourceLib::BindHandle  bh;
    GFxFontResource*            pfontRes = 0;

    if (plib->BindResourceKey(&bh, fontKey) == GFxResourceLib::RS_NeedsResolve)
    {
        // If hot found, create the font object.
        GPtr<GFxFont> pexternalFont = *pprovider->CreateFont(pname, fontFlags);
        if (pexternalFont)        
            pfontRes = new GFxFontResource(pexternalFont, fontKey);
        
        if (pfontRes)
        {
            bh.ResolveResource(pfontRes);
        }
        else
        {   // No error messages are logged for failed system font creation.
            bh.CancelResolve("");
        }
    }
    else
    {
        // If Available, wait for resource to become available.
        pfontRes = (GFxFontResource*)bh.WaitForResolve();
        GASSERT(pfontRes->GetResourceType() == RT_Font);        
    }

    return pfontRes;
}



bool GFxFont::IsCJK(UInt16 code)
{
    static const UInt16 ranges[] = 
    {
        0x1100, 0x11FF, // Hangul Jamo                          U+1100 - U+11FF

        0x2E80, 0x2FDF, // CJK Radicals Supplement,             U+2E80 - U+2EFF
                        // KangXi Radicals                      U+2F00 - U+2FDF

        0x2FF0, 0x4DB5, // Ideographic Description Characters   U+2FF0 - U+2FFF
                        // CJK Symbols and Punctuation          U+3000 - U+303F
                        // Hiragana                             U+3040 - U+309F
                        // Katakana                             U+30A0 - U+30FF
                        // Bopomofo                             U+3100 - U+312F
                        // Hangul Compatibility Jamo            U+3130 - U+318F
                        // Kanbun                               U+3190 - U+319F
                        // Bopomofo Extended                    U+31A0 - U+31BF
                        // CJK Strokes                          U+31C0 - U+31EF
                        // Katakana Phonetic Extensions         U+31F0 - U+31FF
                        // Enclosed CJK Letters and Months      U+3200 - U+32FF
                        // CJK Compatibility                    U+3300 - U+33FF
                        // CJK Unified Ideographs Extension A   U+3400 - U+4DB5

        0x4DC0, 0x9FBB, // Yijing Hexagram Symbols              U+4DC0 - U+4DFF
                        // CJK Unified Ideographs               U+4E00 - U+9FBB


        0xAC00, 0xD7A3, // Hangul Syllables                     U+AC00 - U+D7A3

        0xF900, 0xFAFF, // CJK Compatibility Ideographs         U+F900 - U+FAFF

        0xFF62, 0xFFDC, // Halfwidth and Fullwidth Forms        U+FF00 - U+FFEF (Ideographic part)

        0, 0
    };
    for (UInt i = 0; ranges[i]; i += 2)
    {
        if (code >= ranges[i] && code <= ranges[i + 1])
            return true;
    }
    return false;
}

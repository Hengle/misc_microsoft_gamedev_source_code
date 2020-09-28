/**********************************************************************

Filename    :   GFxStaticText.cpp
Content     :   Static text field character implementation
Created     :   May, 2007
Authors     :   Artem Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#include "GUTF8Util.h"

#include "GRenderer.h"

#include "GFxCharacter.h"
#include "GFxShape.h"
#include "GFxStream.h"
#include "GFxLog.h"
#include "GFxFontResource.h"
#include "GFxText.h"
#include "GFxStyledText.h"

#include "GFxNumber.h"

// For Root()->GetRenderer
#include "GFxPlayerImpl.h"
#include "GFxSprite.h"

#include "GFxDisplayContext.h"
#include "GFxLoadProcess.h"

//
// TextCharacter
//

//////////////////////////////////////////////////////////////////////////
// GFxStaticTextRecord
//
GFxStaticTextRecord::GFxStaticTextRecord() :
    Offset(0.0f),       
    TextHeight(1.0f),
    FontId(0)
{
}

void    GFxStaticTextRecord::Read(GFxStream* in, int glyphCount, int glyphBits, int advanceBits)
{
    Glyphs.resize(glyphCount);
    for (int i = 0; i < glyphCount; i++)
    {
        Glyphs[i].GlyphIndex = in->ReadUInt(glyphBits);
        Glyphs[i].GlyphAdvance = (Float) in->ReadSInt(advanceBits);
    }
}

// Obtains cumulative advance value.
Float   GFxStaticTextRecord::GetCumulativeAdvance() const
{
    Float advance = 0;
    for (UPInt i = 0, glyphCount = Glyphs.size(); i < glyphCount; i++)
        advance += Glyphs[i].GlyphAdvance;
    return advance;
}
//////////////////////////////////////////////////////////////////////////
// GFxStaticTextRecordList
//
void GFxStaticTextRecordList::Clear()
{
    for (UPInt i=0, sz = Records.size(); i < sz; i++)
        if (Records[i]) delete Records[i];
    Records.clear();
}

GFxStaticTextRecord* GFxStaticTextRecordList::AddRecord()
{
    GFxStaticTextRecord* precord = new GFxStaticTextRecord;
    if (precord)    
        Records.push_back(precord);     
    return precord;
}

//////////////////////////////////////////////////////////////////////////
// GFxStaticTextCharacterDef
//
GFxStaticTextCharacterDef::GFxStaticTextCharacterDef()
 : Flags(0)
{
}

void    GFxStaticTextCharacterDef::Read(GFxLoadProcess* p, GFxTagType tagType)
{                
    GASSERT(tagType == GFxTag_DefineText || tagType == GFxTag_DefineText2);
    GFxStream *in = p->GetStream();

    in->ReadRect(&TextRect);

    in->LogParse("  TextRect = { l: %f, t: %f, r: %f, b: %f }\n", 
        (float)TextRect.Left, (float)TextRect.Top, (float)TextRect.Right, (float)TextRect.Bottom);

    in->ReadMatrix(&MatrixPriv);

    in->LogParse("  mat:\n");
    in->LogParseClass(MatrixPriv);

    int GlyphBits = in->ReadU8();
    int AdvanceBits = in->ReadU8();

    in->LogParse("begin text records\n");

    bool    lastRecordWasStyleChange = false;
    
    GPointF offset;
    GColor color;
    Float textHeight    = 0;
    UInt16  fontId      = 0;
    GFxResourcePtr<GFxFontResource> pfont;

    // Style offset starts at 0 and is overwritten when specified.
    offset.SetPoint(0.0f);

    for (;;)
    {
        int FirstByte = in->ReadU8();
        
        if (FirstByte == 0)
        {
            // This is the end of the text records.
            in->LogParse("end text records\n");
            break;
        }

        // Style changes and glyph records just alternate.
        // (Contrary to what most SWF references say!)
        if (lastRecordWasStyleChange == false)
        {
            // This is a style change.

            lastRecordWasStyleChange = true;

            bool    hasFont     = (FirstByte >> 3) & 1;
            bool    hasColor    = (FirstByte >> 2) & 1;
            bool    hasYOffset  = (FirstByte >> 1) & 1;
            bool    hasXOffset  = (FirstByte >> 0) & 1;

            in->LogParse("  text style change\n");

            if (hasFont)
            {
                fontId = in->ReadU16();
                in->LogParse("  HasFont: font id = %d\n", fontId);
                // Create a font resource handle.
                GFxResourceHandle hres;
                p->GetResourceHandle(&hres, GFxResourceId(fontId));
                pfont.SetFromHandle(hres);
            }
            if (hasColor)
            {
                if (tagType == 11)
                {
                    in->ReadRgb(&color);                          
                }
                else
                {
                    GASSERT(tagType == 33);                         
                    in->ReadRgba(&color);
                }
                in->LogParse("  HasColor\n");
            }
            if (hasXOffset)
            {                   
                offset.x = in->ReadS16();
                in->LogParse("  XOffset = %g\n", offset.x);
            }
            else
            {
                // Leave carry-over from last record.                   
            }
            if (hasYOffset)
            {                   
                offset.y = in->ReadS16();
                in->LogParse("  YOffset = %g\n", offset.y);
            }
            else
            {
                // Leave carry-over from last record.
            }
            if (hasFont)
            {
                textHeight = in->ReadU16();
                in->LogParse("  TextHeight = %g\n", textHeight);
            }
        }
        else
        {
            // Read the glyph record.
            lastRecordWasStyleChange = false;

            int GlyphCount = FirstByte;
            // Don't mask the top GlyphCount bit; the first record is allowed to have > 127 glyphs.

            GFxStaticTextRecord* precord = TextRecords.AddRecord();
            if (precord)
            {
                precord->Offset     = offset;
                precord->pFont      = pfont;
                precord->TextHeight = textHeight;
                precord->Color      = color;
                precord->FontId     = fontId;
                precord->Read(in, GlyphCount, GlyphBits, AdvanceBits);

                // Add up advances and adjust offset.
                offset.x += precord->GetCumulativeAdvance();
            }

            in->LogParse("  GlyphRecords: count = %d\n", GlyphCount);
        }
    }
}

bool    GFxStaticTextCharacterDef::DefPointTestLocal(const GPointF &pt, bool testShape, const GFxCharacter* pinst) const
{
    GUNUSED2(testShape, pinst);
    // Flash only uses the bounding box for text - even if shape flag is used
    return TextRect.Contains(pt);
}

//////////////////////////////////////////////////////////////////////////
// GFxStaticTextCharacter
//
class GFxStaticTextCharacter : public GFxCharacter
{
public:
    enum
    {
        Flags_FontsResolved = 1,
        Flags_NextFrame     = 2
    };

    GFxStaticTextCharacterDef*  pDef;
    GFxTextLineBuffer           TextGlyphRecords;
    UInt8                       Flags;
    GFxTextFilter               Filter;

    GFxStaticTextCharacter(GFxStaticTextCharacterDef* def, GFxMovieDefImpl *pbindingDefImpl, 
        GFxASCharacter* parent, GFxResourceId id)
        :
        GFxCharacter(parent, id),
        pDef(def)
    {
        Matrix invm = pDef->MatrixPriv;
        invm.Invert();
        GRectF r = invm.EncloseTransform(pDef->TextRect);

        for (UInt i = 0, n = pDef->TextRecords.GetSize(); i < n; ++i)
        {
            const GFxStaticTextRecord* prec = pDef->TextRecords.Records[i];

            // resolve font
            GASSERT(prec->FontId != 0);

            GFxResourceBindData fontData = pbindingDefImpl->GetResourceBinding().GetResourceData(prec->pFont);
            GFxFontResource* pfont = (GFxFontResource*)fontData.pResource.GetPtr();
            if (!pfont)
            {
                GFxLog* plog = pbindingDefImpl->GetLog();
                if (plog)
                    plog->LogError("Error: text style with undefined font; FontId = %d\n", prec->FontId);
                return;
            }

            if (!def->HasInstances())
            {
                // do this check only once per def
                GFxMovieDataDef* pdataDef = pbindingDefImpl->GetDataDef();
                GFxImportData* pimport = pdataDef->GetFirstImport();
                for(; pimport != 0; pimport = pimport->pNext)
                {
                    for (UInt j = 0; j<pimport->Imports.size(); j++)
                    {
                        if (pimport->Imports[j].BindIndex == prec->pFont.GetBindIndex())
                        {
                            // found import table entry, display warning.
                            // It is highly not recommended to use imported fonts for static texts
                            // since imported font might be substituted by the localization stuff. 
                            // Unfortunately, Flash may use imported font implicitly, for example, 
                            // when static text uses font "Arial" and imported font "$Font" represents
                            // also the font "Arial" (see Properties window for imported font). In this 
                            // case, Flash won't create a local "Arial" font for the static text, it
                            // will use the "$Font" instead. Since, static text does not reformat
                            // the text for the different fonts, replacing the "$Font" by another 
                            // font may screw up the static text because of different advance values.
                            GFxLog* plog = pbindingDefImpl->GetLog();
                            if (plog)
                                plog->LogWarning("Warning: static text uses imported font! FontId = %d, import name = %s\n", 
                                    prec->FontId, pimport->Imports[j].SymbolName.ToCStr());
                        }
                    }
                }
            }

            // It could be more efficient to obtain GFxFontHandle from manager, however,
            // this is a resource id based lookup (not font name based).
            GPtr<GFxFontHandle> pfontHandle = *new GFxFontHandle(NULL, pfont);

            GFxTextLineBuffer::Line* pline;
            UInt glyphsCount = (UInt)prec->Glyphs.size();
            if (glyphsCount <= 255)
            {
                // convert line to short form
                pline = TextGlyphRecords.InsertNewLine(i, glyphsCount, 2, GFxTextLineBuffer::Line8);
            }
            else
            {
                pline = TextGlyphRecords.InsertNewLine(i, glyphsCount, 2, GFxTextLineBuffer::Line32);
            }
            GASSERT(pline);
            GPointF p =  prec->Offset;
            p -= r.TopLeft();
            pline->SetOffset(p);

            pline->SetDimensions(0, 0);
            GFxTextLineBuffer::GlyphInserter gins(pline->GetGlyphs(), glyphsCount, pline->GetFormatData());

            for (UInt i = 0; i < glyphsCount; ++i, ++gins)
            {
                GFxTextLineBuffer::GlyphEntry& glyph = gins.GetGlyph();
                glyph.SetIndex(prec->Glyphs[i].GlyphIndex);
                glyph.SetAdvance((SInt)prec->Glyphs[i].GlyphAdvance);
                glyph.SetFontSize((UInt)TwipsToPixels(prec->TextHeight));
                if (i == 0)
                {
                    gins.AddFont(pfontHandle);
                    gins.AddColor(prec->Color);
                }
            }
        }
        TextGlyphRecords.SetVisibleRect(r);
        TextGlyphRecords.SetStaticText();
        Filter.SetDefaultShadow();
        def->SetHasInstances();
    }


    virtual void    AdvanceFrame(bool nextFrame, Float framePos)
    {
        GUNUSED(framePos);
        if (nextFrame)
            Flags |= Flags_NextFrame;
        else
            Flags &= ~Flags_NextFrame;
    }

    virtual void    SetFilters(const GFxTextFilter& f)
    {
        Filter = f;
    }


    void    Display(GFxDisplayContext &context, StackData stackData)   
    {
        // And displays records.
        //GRenderer::Matrix m = GetWorldMatrix();

		// Pass matrix on the stack
		GRenderer::Matrix m = stackData.stackMatrix * this->Matrix_1;
		GRenderer::Cxform cx = stackData.stackColor * this->ColorTransform;		


        // Do viewport culling if bounds are available.
        if (!GetMovieRoot()->GetVisibleFrameRectInTwips().Intersects(m.EncloseTransform(pDef->TextRect)))
            if (!(context.GetRenderFlags() & GFxRenderConfig::RF_NoViewCull))
                return;

        GFxTextFieldParam param;
        param.LoadFromTextFilter(Filter);

        if (pDef->IsAAForReadability())
        {
            param.TextParam.SetOptRead(true);
            param.TextParam.SetAutoFit(true);
            param.ShadowParam.SetOptRead(true);
            param.ShadowParam.SetAutoFit(true);
        }

        m *= pDef->MatrixPriv;
        TextGlyphRecords.Display(context, m, cx, 
                                 (Flags & Flags_NextFrame) != 0,
                                 param);
        Flags &= ~Flags_NextFrame;
    }

    // Obtains character definition relying on us. Must be overridden.
    GFxCharacterDef* GetCharacterDef() const { return pDef; }

    virtual GRectF GetBounds(const Matrix &t) const
    {
        return t.EncloseTransform(pDef->GetBoundsLocal());
    }

};

GFxCharacter*   GFxStaticTextCharacterDef::CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId id,
                                                                   GFxMovieDefImpl *pbindingImpl)
{
    GFxStaticTextCharacter*   ch = new GFxStaticTextCharacter(this, pbindingImpl, parent, id);
    return ch;
}

void    GSTDCALL GFx_DefineTextLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)  // Read a DefineText tag.
{
    GASSERT(tagInfo.TagType == GFxTag_DefineText || tagInfo.TagType == GFxTag_DefineText2);

    UInt16  characterId = p->ReadU16();
    
    GPtr<GFxStaticTextCharacterDef>   pch = *new GFxStaticTextCharacterDef();
    p->LogParse("TextCharacter, id = %d\n", characterId);
    pch->Read(p, tagInfo.TagType);

    // Log print some parse stuff...

    p->AddResource(GFxResourceId(characterId), pch);
}



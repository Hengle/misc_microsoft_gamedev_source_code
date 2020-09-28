/**********************************************************************

Filename    :   GFxStyledText.cpp
Content     :   Static and dynamic text field character implementation
Created     :   Feb-May, 2007
Authors     :   Artem Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include "GUTF8Util.h"
#include "GStd.h"
#include "GRenderer.h"
#include "GFxDisplayContext.h"

#include "GFxCharacter.h"
#include "GFxShape.h"
#include "GFxStream.h"
#include "GFxStyledText.h"

#if defined(GFC_OS_SYMBIAN) || defined(GFC_OS_WII) || defined(GFC_CC_RENESAS)
#define iswspace(x) ((x) < 128 ? isspace((x)) : 0)
#else
#include <wctype.h>
#endif

#define GFX_BULLET_CHAR_CODE        0x25CF
#define GFX_BULLET_CHAR_ADVANCE     15
#define GFX_BULLET_PARA_INDENT      20
#define GFX_LINE_ALLOC_DELTA        100

#define GFX_EDIT_HSCROLL_DELTA PixelsToTwips(60)

#ifdef GFC_BUILD_DEBUG
//#define DEBUG_CURSOR_POS
#endif //GFC_BUILD_DEBUG

template <typename T>
SInt RoundTwips(T v)
{
    return SInt(v + 0.5)/20*20;
}

template <typename T>
SInt AlignTwipsToPixel(T v)
{
    // Flash has kinda strange rounding rule, at least in text area. 
    //return SInt(v + 20*0.6)/20*20;
    //return SInt(v + 20*0.5)/20*20;
    return SInt(v);//???
}

#ifdef GFC_BUILD_DEBUG
template <class T>
void _dump(GRangeDataArray<T>& ranges)
{
    printf("-----\n");
    typename GRangeDataArray<T>::Iterator it = ranges.Begin();
    for(; !it.IsFinished(); ++it)
    {
        GRangeData<T>& r = *it;
        printf ("i = %d, ei = %d\n", r.Index, r.NextIndex());
    }
    printf("-----\n");
}
#endif

//////////////////////////////////
// GFxTextFormat
//
void GFxTextFormat::SetFontName(const char* pfontName, UPInt  fontNameSz)
{
    GASSERT(pfontName && fontNameSz > 0);
    if (fontNameSz == GFC_MAX_UPINT)
        fontNameSz = gfc_strlen(pfontName);

    if (IsFontHandleSet())
    {
        // if font handle is set and name being set is different - reset the handle
        if (gfc_strlen(FontName) != fontNameSz || GFxString::CompareNoCase(pfontName, FontName, fontNameSz) != 0)
            ClearFontHandle();
    }
    fontNameSz = GTL::gpmin<UPInt>(sizeof(FontName) - 1, fontNameSz);
    memcpy(FontName, pfontName, fontNameSz);
    FontName[fontNameSz] = 0;
    PresentMask |= PresentMask_FontName; 
}

void GFxTextFormat::SetFontName(const wchar_t* pfontName, UPInt fontNameSz)
{
    GASSERT(pfontName && fontNameSz > 0);
    if (fontNameSz == GFC_MAX_UPINT)
        fontNameSz = wcslen(pfontName);

    if (IsFontHandleSet())
    {
        // if font handle is set and name being set is different - reset the handle
        if (gfc_strlen(FontName) != fontNameSz)
        {
            UPInt i;
            for (i = 0; i < fontNameSz && FontName[i] != 0; ++i)
            {
                if (FontName[i] != (char)pfontName[i])
                {
                    ClearFontHandle();
                    break;
                }
            }
        }
    }
    fontNameSz = GTL::gpmin<UPInt>(sizeof(FontName) - 1, fontNameSz);
    for (UPInt i = 0; i < fontNameSz; ++i)
    {
        FontName[i] = (char)pfontName[i];
    }
    FontName[fontNameSz] = 0;
    PresentMask |= PresentMask_FontName; 
}

void GFxTextFormat::SetFontHandle(GFxFontHandle* pfontHandle) 
{ 
    pFontHandle = pfontHandle;
    PresentMask |= PresentMask_FontHandle; 
}

void GFxTextFormat::SetUrl(const char* purl, UPInt urlSz)
{
    GASSERT(purl && urlSz > 0);
    if (urlSz == GFC_MAX_UPINT)
        urlSz = gfc_strlen(purl);
    Url.Resize(0);
    Url.AppendString(purl, urlSz);
    PresentMask |= PresentMask_Url; 
}

void GFxTextFormat::SetUrl(const wchar_t* purl, UPInt urlSz)
{
    GASSERT(purl && urlSz > 0);
    if (urlSz == GFC_MAX_UPINT)
        urlSz = wcslen(purl);
    Url.Resize(0);
    Url.AppendString(purl, urlSz);
    PresentMask |= PresentMask_Url; 
}

void GFxTextFormat::SetUrl(const GFxString& url)
{
    Url = url;
    PresentMask |= PresentMask_Url; 
}


void GFxTextFormat::SetBold(bool bold)
{
    if (IsFontHandleSet())
    {
        // if font handle is set and boldness being set is different - reset the handle
        if (IsBold() != bold)
            ClearFontHandle();
    }
    if (bold) 
    {
        FormatFlags |= Format_Bold; 
    }
    else
    {
        FormatFlags &= ~Format_Bold; 
    }
    PresentMask |= PresentMask_Bold; 
}

void GFxTextFormat::SetItalic(bool italic)
{
    if (IsFontHandleSet())
    {
        // if font handle is set and italicness being set is different - reset the handle
        if (IsItalic() != italic)
            ClearFontHandle();
    }
    if (italic) 
    {
        FormatFlags |= Format_Italic; 
    }
    else
    {
        FormatFlags &= ~Format_Italic; 
    }
    PresentMask |= PresentMask_Italic; 
}

void GFxTextFormat::SetUnderline(bool underline)
{
    if (underline) 
    {
        FormatFlags |= Format_Underline; 
    }
    else
    {
        FormatFlags &= ~Format_Underline; 
    }
    PresentMask |= PresentMask_Underline; 
}

void GFxTextFormat::SetKerning(bool kerning)
{
    if (kerning) 
    {
        FormatFlags |= Format_Kerning; 
    }
    else
    {
        FormatFlags &= ~Format_Kerning; 
    }
    PresentMask |= PresentMask_Kerning; 
}

bool GFxTextFormat::IsFontSame(const GFxTextFormat& fmt) const 
{ 
    return (((IsFontNameSet() && fmt.IsFontNameSet() && GFxString::CompareNoCase(FontName, fmt.FontName) == 0) ||
            (IsFontHandleSet() && fmt.IsFontHandleSet() && pFontHandle == fmt.pFontHandle)) &&
           IsBold() == fmt.IsBold() && IsItalic() == fmt.IsItalic()); 
}

GFxTextFormat GFxTextFormat::Merge(const GFxTextFormat& fmt) const
{
    GFxTextFormat result(*this);
    if (fmt.IsBoldSet())
        result.SetBold(fmt.IsBold());
    if (fmt.IsItalicSet())
        result.SetItalic(fmt.IsItalic());
    if (fmt.IsUnderlineSet())
        result.SetUnderline(fmt.IsUnderline());
    if (fmt.IsKerningSet())
        result.SetKerning(fmt.IsKerning());
    if (fmt.IsColorSet())
        result.SetColor(fmt.GetColor());
    if (fmt.IsAlphaSet())
        result.SetAlpha(fmt.GetAlpha());
    if (fmt.IsLetterSpacingSet())
        result.SetLetterSpacingInTwips(fmt.GetLetterSpacingInTwips()); // avoid extra conversions
    if (fmt.IsFontSizeSet())
        result.SetFontSize(fmt.GetFontSize());
    if (fmt.IsFontNameSet())
        result.SetFontName(fmt.GetFontName());
    if (fmt.IsFontHandleSet())
        result.SetFontHandle(fmt.GetFontHandle());
    if (fmt.IsUrlSet())
        result.SetUrl(fmt.GetUrl());
    if (fmt.IsImageDescSet())
        result.SetImageDesc(fmt.GetImageDesc());
    return result;
}

GFxTextFormat GFxTextFormat::Intersection(const GFxTextFormat& fmt) const
{
    GFxTextFormat result;
    if (IsBoldSet() && fmt.IsBoldSet() && IsBold() == fmt.IsBold())
        result.SetBold(fmt.IsBold());
    if (IsItalicSet() && fmt.IsItalicSet() && IsItalic() == fmt.IsItalic())
        result.SetItalic(fmt.IsItalic());
    if (IsUnderlineSet() && fmt.IsUnderlineSet() && IsUnderline() == fmt.IsUnderline())
        result.SetUnderline(fmt.IsUnderline());
    if (IsKerningSet() && fmt.IsKerningSet() && IsKerning() == fmt.IsKerning())
        result.SetKerning(fmt.IsKerning());
    if (IsColorSet() && fmt.IsColorSet() && GetColor() == fmt.GetColor())
        result.SetColor(fmt.GetColor());
    if (IsAlphaSet() && fmt.IsAlphaSet() && GetAlpha() == fmt.GetAlpha())
        result.SetAlpha(fmt.GetAlpha());
    if (IsLetterSpacingSet() && fmt.IsLetterSpacingSet() && GetLetterSpacing() == fmt.GetLetterSpacing())
        result.SetLetterSpacingInTwips(fmt.GetLetterSpacingInTwips()); // avoid extra conversions
    if (IsFontSizeSet() && fmt.IsFontSizeSet() && GetFontSize() == fmt.GetFontSize())
        result.SetFontSize(fmt.GetFontSize());
    if (IsFontNameSet() && fmt.IsFontNameSet() && GFxString::CompareNoCase(GetFontName(), fmt.GetFontName()) == 0)
        result.SetFontName(fmt.GetFontName());
    if (IsFontHandleSet() && fmt.IsFontHandleSet() && GetFontHandle() == fmt.GetFontHandle())
        result.SetFontHandle(fmt.GetFontHandle());
    if (IsUrlSet() && fmt.IsUrlSet() && Url.CompareNoCase(fmt.Url) == 0)
        result.SetUrl(fmt.GetUrl());
    if (IsImageDescSet() && fmt.IsImageDescSet() && GetImageDesc() == fmt.GetImageDesc())
        result.SetImageDesc(fmt.GetImageDesc());
    return result;
}

void GFxTextFormat::InitByDefaultValues()
{
    SetColor(0);
    SetFontName("Times New Roman");
    SetFontSize(12);
    SetBold(false);
    SetItalic(false);
    SetUnderline(false);
    SetKerning(false);
    ClearAlpha();
    ClearLetterSpacing();
    ClearUrl();

}

//////////////////////////////////
// GFxTextParagraphFormat
//
GFxTextParagraphFormat GFxTextParagraphFormat::Merge(const GFxTextParagraphFormat& fmt) const
{
    GFxTextParagraphFormat result(*this);
    if (fmt.IsAlignmentSet())
        result.SetAlignment(fmt.GetAlignment());
    if (fmt.IsBulletSet())
        result.SetBullet(fmt.IsBullet());
    if (fmt.IsBlockIndentSet())
        result.SetBlockIndent(fmt.GetBlockIndent());
    if (fmt.IsIndentSet())
        result.SetIndent(fmt.GetIndent());
    if (fmt.IsLeadingSet())
        result.SetLeading(fmt.GetLeading());
    if (fmt.IsLeftMarginSet())
        result.SetLeftMargin(fmt.GetLeftMargin());
    if (fmt.IsRightMarginSet())
        result.SetRightMargin(fmt.GetRightMargin());
    if (fmt.IsTabStopsSet())
        result.SetTabStops(fmt.GetTabStops());
    return result;
}

GFxTextParagraphFormat GFxTextParagraphFormat::Intersection(const GFxTextParagraphFormat& fmt) const
{
    GFxTextParagraphFormat result;
    if (IsAlignmentSet() && fmt.IsAlignmentSet() && GetAlignment() == fmt.GetAlignment())
        result.SetAlignment(fmt.GetAlignment());
    if (IsBulletSet() && fmt.IsBulletSet() && IsBullet() == fmt.IsBullet())
        result.SetBullet(fmt.IsBullet());
    if (IsBlockIndentSet() && fmt.IsBlockIndentSet() && GetBlockIndent() == fmt.GetBlockIndent())
        result.SetBlockIndent(fmt.GetBlockIndent());
    if (IsIndentSet() && fmt.IsIndentSet() && GetIndent() == fmt.GetIndent())
        result.SetIndent(fmt.GetIndent());
    if (IsLeadingSet() && fmt.IsLeadingSet() && GetLeading() == fmt.GetLeading())
        result.SetLeading(fmt.GetLeading());
    if (IsLeftMarginSet() && fmt.IsLeftMarginSet() && GetLeftMargin() == fmt.GetLeftMargin())
        result.SetLeftMargin(fmt.GetLeftMargin());
    if (IsRightMarginSet() && fmt.IsRightMarginSet() && GetRightMargin() == fmt.GetRightMargin())
        result.SetRightMargin(fmt.GetRightMargin());
    if (IsTabStopsSet() && fmt.IsTabStopsSet() && GetTabStops() == fmt.GetTabStops())
        result.SetTabStops(fmt.GetTabStops());
    return result;
}

void GFxTextParagraphFormat::InitByDefaultValues()
{
    SetAlignment(Align_Left);
    ClearBlockIndent();
    ClearBullet();
    ClearIndent();
    ClearLeading();
    ClearLeftMargin();
    ClearRightMargin();
    ClearTabStops();
}

void GFxTextParagraphFormat::SetTabStops(UInt num, ...)
{
    if (num > 0)
    {
        if (!pTabStops || pTabStops[0] != num)
        {
            FreeTabStops();
            AllocTabStops(num);
        }
        va_list vl;
        va_start(vl, num);

        for (UInt i = 0; i < num; ++i)
        {
            UInt arg = va_arg(vl, UInt);
            pTabStops[i + 1] = arg;
        }
        va_end(vl);
        PresentMask |= PresentMask_TabStops;
    }
    else
        ClearTabStops();
}

void  GFxTextParagraphFormat::SetTabStops(const UInt* psrcTabStops)
{
    if (psrcTabStops && psrcTabStops[0] > 0)
    {
        FreeTabStops();
        CopyTabStops(psrcTabStops);
        PresentMask |= PresentMask_TabStops;
    }
    else
        ClearTabStops();
}

const UInt* GFxTextParagraphFormat::GetTabStops(UInt* pnum) const
{
    if (pTabStops)
    {
        UInt cnt = pTabStops[0];
        if (pnum)
            *pnum = cnt;
        return pTabStops + 1;
    }
    return NULL;
}

void GFxTextParagraphFormat::AllocTabStops(UInt num)
{
    FreeTabStops();
    pTabStops = (UInt*)GALLOC_ALIGN((num + 1) * sizeof(UInt), sizeof(UInt));
    pTabStops[0] = num;
}

void GFxTextParagraphFormat::FreeTabStops()
{
    GFREE_ALIGN(pTabStops);
    pTabStops = NULL;
}

bool GFxTextParagraphFormat::TabStopsEqual(const UInt* psrcTabStops) const
{
    if (pTabStops == psrcTabStops)
        return true;
    if (pTabStops && psrcTabStops)
    {
        UInt c1 = pTabStops[0];
        UInt c2 = psrcTabStops[0];
        if (c1 == c2)
        {
            return (memcmp(pTabStops + 1, psrcTabStops + 1, sizeof(UInt) * c1) == 0);
        }
    }
    return false;
}

void GFxTextParagraphFormat::CopyTabStops(const UInt* psrcTabStops)
{
    if (psrcTabStops)
    {
        UInt n = psrcTabStops[0];
        pTabStops = (UInt*)GALLOC_ALIGN((n + 1) * sizeof(UInt), sizeof(UInt));
        memcpy(pTabStops, psrcTabStops, (n + 1) * sizeof(UInt));
    }
    else
        pTabStops = NULL;
}

//////////////////////////////////////////////////////////////////////////
// GFxTextAllocator
//
GFxTextParagraph* GFxTextAllocator::AllocateParagraph()
{
    return new GFxTextParagraph(this);
}

//////////////////////////////////
// GFxWideStringBuffer
//
GFxWideStringBuffer::~GFxWideStringBuffer()
{
    pAllocator->FreeText(pText);
}

void GFxWideStringBuffer::SetString(const char* putf8Str, UPInt utf8length)
{
    if (utf8length == GFC_MAX_UPINT)
        utf8length = GUTF8Util::GetLength(putf8Str, gfc_strlen(putf8Str));
    if (Allocated < utf8length)
    {
        pText = pAllocator->ReallocText(pText, Allocated, utf8length);
        GASSERT(pText);
        Allocated = utf8length;
    }
    if (utf8length > 0)
    {
        GASSERT(pText);
        GUTF8Util::DecodeString(pText, putf8Str, utf8length);
    }
    Length = utf8length;
}

void GFxWideStringBuffer::SetString(const wchar_t* pstr, UPInt length)
{
    if (length == GFC_MAX_UPINT)
        length = StrLen(pstr);
    if (Allocated < length)
    {
        pText = pAllocator->ReallocText(pText, Allocated, length);
        GASSERT(pText);
        Allocated = length;
    }
    if (length > 0)
    {
        GASSERT(pText);
        memcpy(pText, pstr, length * sizeof(wchar_t));
    }
    Length = length;
}

wchar_t* GFxWideStringBuffer::CreatePosition(UPInt pos, UPInt length)
{
    GASSERT(pos <= Length);

    if (Allocated < Length + length)        
    {
        pText = pAllocator->ReallocText(pText, Allocated, Length + length);
        GASSERT(pText);
        Allocated = Length + length;
    }
    if (Length - pos > 0)
    {
        GASSERT(pText);
        memmove(pText + pos + length, pText + pos, (Length - pos)*sizeof(wchar_t));
    }
    Length += length;
    GASSERT(pText != NULL || pos == 0);
    return pText + pos;
}

void GFxWideStringBuffer::Remove(UPInt pos, UPInt length)
{
    if (pos < Length)
    {
        if (pos + length >= Length)
            Length = pos;
        else
        {
            memmove(pText + pos, pText + pos + length, (Length - (pos + length))*sizeof(wchar_t));
            Length -= length;
        }
    }
}

SPInt GFxWideStringBuffer::StrChr(const wchar_t* ptext, UPInt length, wchar_t c)
{
    for (UPInt i = 0; i < length; ++i)
    {
        GASSERT(ptext);
        if (ptext[i] == c)
            return (SPInt)i;
    }
    return -1;
}

UPInt GFxWideStringBuffer::StrLen(const wchar_t* ptext)
{
    GASSERT(ptext);
    UPInt i = 0;
    while (ptext[i] != 0)
        ++i;
    return i;
}

void GFxWideStringBuffer::StripTrailingNewLines()
{
    int len = int(Length);
    // check, is the content already null terminated
    if (len > 0 && pText[len -1] == 0)
        --len; //if yes, skip the '\0'
    for (int i = len - 1; i >= 0 && (pText[i] == '\n' || pText[i] == '\r'); --i)
    {
        --Length;
        pText[i] = 0;
    }
}

//////////////////////////////////
// GFxTextParagraph
//
void GFxTextParagraph::SetText(const wchar_t* pstring, UPInt length)
{
    GASSERT(length <= GFC_MAX_SPINT);
    Text.SetString(pstring, length);
    ++ModCounter;
}

void GFxTextParagraph::SetTextFormat(const GFxTextFormat& fmt, UPInt startPos, UPInt endPos)
{
    FormatRunIterator it = GetIteratorAt(startPos);
    if (endPos < startPos) endPos = startPos;
    SPInt length = (endPos == GFC_MAX_UPINT) ? GFC_MAX_SPINT : endPos - startPos;
    while(length > 0 && !it.IsFinished())
    {
        const StyledTextRun& run = *it;
        const UPInt runIndex = run.Index, runLength = run.Length;
        SPInt curIndex;
        if (startPos > runIndex)
            curIndex = startPos;
        else
            curIndex = runIndex;
        GFxTextFormat format;
        GPtr<GFxTextFormat> pfmt;
        if (run.pFormat)
        {
            format = run.pFormat->Merge(fmt);
            pfmt = GetAllocator()->AllocateTextFormat(format);
        }
        else
        {
            pfmt = GetAllocator()->AllocateTextFormat(fmt);
        }

        UPInt newLen = GTL::gpmin(runLength - (curIndex - runIndex), (UPInt)length);
        FormatInfo.SetRange(curIndex, newLen, pfmt);
        length -= newLen;
        //++it;
        it.SetTextPos(runIndex + runLength);
    }

    //_dump(FormatInfo);

    ++ModCounter;
}

GFxTextFormat GFxTextParagraph::GetTextFormat(UPInt startPos, UPInt endPos) const
{
    FormatRunIterator it = GetIteratorAt(startPos);
    if (endPos < startPos) endPos = startPos;
    SPInt length = (endPos == GFC_MAX_UPINT) ? GFC_MAX_SPINT : endPos - startPos;
    GFxTextFormat finalTextFmt;
    UInt i = 0;
    while(length > 0 && !it.IsFinished())
    {
        const StyledTextRun& run = *it;

        if (run.pFormat)
        {
            if (i++ == 0)
                finalTextFmt = *run.pFormat;
            else
                finalTextFmt = run.pFormat->Intersection(finalTextFmt);
        }

        length -= run.Length;
        ++it;
    }
    return finalTextFmt;
}

// returns the actual pointer on text format at the specified position.
// Will return NULL, if there is no text format at the "pos"
GFxTextFormat* GFxTextParagraph::GetTextFormatPtr(UPInt startPos) const
{
    FormatRunIterator it = GetIteratorAt(startPos);
    GFxTextFormat* pfmt = NULL;
    if (!it.IsFinished())
    {
        const StyledTextRun& run = *it;

        if (run.pFormat)
        {
            pfmt = run.pFormat;
        }
    }
    return pfmt;
}

void GFxTextParagraph::SetFormat(const GFxTextParagraphFormat& fmt)
{
    GPtr<GFxTextParagraphFormat> pfmt;
    if (pFormat)
        pfmt = GetAllocator()->AllocateParagraphFormat(pFormat->Merge(fmt));
    else
        pfmt = GetAllocator()->AllocateParagraphFormat(fmt);
    pFormat = pfmt;
    ++ModCounter;
}

GFxTextParagraph::FormatRunIterator GFxTextParagraph::GetIterator() const
{
    return FormatRunIterator(FormatInfo, Text);
}

GFxTextParagraph::FormatRunIterator GFxTextParagraph::GetIteratorAt(UPInt index) const
{
    return FormatRunIterator(FormatInfo, Text, index);
}

wchar_t* GFxTextParagraph::CreatePosition(UPInt pos, UPInt length)
{
    wchar_t* p = Text.CreatePosition(pos, length);
    FormatInfo.ExpandRange(pos, length);
    ++ModCounter;
    return p;
}

void GFxTextParagraph::InsertString(const wchar_t* pstr, UPInt pos, UPInt length, const GFxTextFormat* pnewFmt)
{
    if (length > 0)
    {
        if (length == GFC_MAX_UPINT)
            length = GFxWideStringBuffer::StrLen(pstr);
        wchar_t* p = CreatePosition(pos, length);
        if (pnewFmt)
        {
            FormatInfo.SetRange(pos, length, const_cast<GFxTextFormat*>(pnewFmt));
        }
        else
        {
            if (pos > 0)
                FormatInfo.ExpandRange(pos - 1, length);
            else
                FormatInfo.ExpandRange(pos, length);
        }

        memcpy(p, pstr, length * sizeof(wchar_t));
        ++ModCounter;

        //_dump(FormatInfo);
    }
}

void GFxTextParagraph::AppendPlainText(const wchar_t* pstr, UPInt length)
{
    if (length > 0)
    {
        if (length == GFC_MAX_UPINT)
            length = GFxWideStringBuffer::StrLen(pstr);
        wchar_t* p = CreatePosition(GetLength(), length);

        memcpy(p, pstr, length * sizeof(wchar_t));
        ++ModCounter;

        //_dump(FormatInfo);
    }
}

void GFxTextParagraph::AppendPlainText(const char* putf8str, UPInt utf8StrSize)
{
    if (utf8StrSize > 0)
    {
        UPInt length;
        if (utf8StrSize == GFC_MAX_UPINT)
            length = (UPInt)GUTF8Util::GetLength(putf8str);
        else
            length = (UPInt)GUTF8Util::GetLength(putf8str, (SPInt)utf8StrSize);
        wchar_t* p = CreatePosition(GetLength(), length);

        GUTF8Util::DecodeString(p, putf8str, length);

        ++ModCounter;

        //_dump(FormatInfo);
    }
}

void GFxTextParagraph::Remove(UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    UPInt length = (endPos == GFC_MAX_UPINT) ? GFC_MAX_UPINT : (UPInt)(endPos - startPos);
    if (length > 0)
    {
        Text.Remove(startPos, length);
        //_dump(FormatInfo);
        FormatInfo.RemoveRange(startPos, length);
        //_dump(FormatInfo);
        ++ModCounter;
    }
}

// copies text and formatting from psrcPara paragraph to this one, starting from startSrcIndex in
// source paragraph and startDestIndex in destination one. The "length" specifies the number
// of positions to copy.
void GFxTextParagraph::Copy(const GFxTextParagraph& psrcPara, UPInt startSrcIndex, UPInt startDestIndex, UPInt length)
{
    if (length > 0)
    {
        InsertString(psrcPara.GetText() + startSrcIndex, startDestIndex, length);
        // copy format info
        FormatRunIterator fmtIt = psrcPara.GetIteratorAt(startSrcIndex);
        for(; !fmtIt.IsFinished(); ++fmtIt)
        {
            const GFxTextParagraph::StyledTextRun& run = *fmtIt;
            if (run.pFormat)
            {
                SPInt idx;
                UPInt len;
                if (run.Index < SPInt(startSrcIndex))
                {
                    idx = 0;
                    len = run.Length - (startSrcIndex - run.Index);
                }
                else
                {
                    idx = run.Index - startSrcIndex;
                    len = run.Length;
                }
                FormatInfo.SetRange(startDestIndex + idx, len, run.pFormat);
            }
        }
        ++ModCounter;
    }
}

// Shrinks the paragraph's length by the "delta" value.
void GFxTextParagraph::Shrink(UPInt delta)
{
    UPInt len = Text.GetLength();
    delta = GTL::gpmin(delta, len);
    Remove(len - delta, len);
}

#ifdef GFC_BUILD_DEBUG
void GFxTextParagraph::CheckIntegrity() const
{
    const_cast<GFxTextParagraph*>(this)->FormatInfo.CheckIntegrity();
}
#endif

//////////////////////////////////////////////////////////////////////////
// Paragraphs' iterators
GFxTextParagraph::FormatRunIterator::FormatRunIterator(const TextFormatArrayType& fmts, const GFxWideStringBuffer& textHandle) : 
    pFormatInfo(&fmts), FormatIterator(fmts.Begin()), pText(&textHandle),  
    CurTextIndex(0)
{
}

GFxTextParagraph::FormatRunIterator::FormatRunIterator(const TextFormatArrayType& fmts, const GFxWideStringBuffer& textHandle, UPInt index) : 
    pFormatInfo(&fmts), FormatIterator(pFormatInfo->GetIteratorByNearestIndex(index)),
    pText(&textHandle), CurTextIndex(0)
{
    if (!FormatIterator.IsFinished())
    {
        if (!FormatIterator->Contains((SPInt)index))
        {
            if ((SPInt)index > FormatIterator->Index)
            {
                CurTextIndex = (UPInt)FormatIterator->Index;
                CurTextIndex += FormatIterator->Length;
                ++FormatIterator;
            }
        }
        else
        {
            CurTextIndex = (UPInt)FormatIterator->Index;    
        }
    }
}
GFxTextParagraph::FormatRunIterator::FormatRunIterator(const FormatRunIterator& orig) :
    pFormatInfo(orig.pFormatInfo), FormatIterator(orig.FormatIterator), pText(orig.pText),
    CurTextIndex(orig.CurTextIndex)
{
}

GFxTextParagraph::FormatRunIterator& GFxTextParagraph::FormatRunIterator::operator=(const GFxTextParagraph::FormatRunIterator& orig)
{
    FormatIterator    = orig.FormatIterator;
    pFormatInfo       = orig.pFormatInfo;
    pText             = orig.pText;
    CurTextIndex      = orig.CurTextIndex;
    return *this;
}

const GFxTextParagraph::StyledTextRun& GFxTextParagraph::FormatRunIterator::operator* () 
{ 
    if (!FormatIterator.IsFinished())
    {
        const TextFormatRunType& fmtRange = *FormatIterator; //(*pFormatInfo)[CurFormatIndex];
        GASSERT(fmtRange.Index >= 0);
        if (CurTextIndex < (UPInt)fmtRange.Index)
        {
            // if index in text is lower than current range index - 
            // just return a text w/o format.

            // determine the length of text as fmtRange.Index - CurTextIndex
            return PlaceHolder.Set(pText->ToWStr() + CurTextIndex, CurTextIndex, fmtRange.Index - CurTextIndex, NULL);
        }
        else
        {
            GASSERT(fmtRange.Index >= 0 && UPInt(fmtRange.Index) == CurTextIndex);
            return PlaceHolder.Set(pText->ToWStr() + fmtRange.Index, fmtRange.Index, fmtRange.Length, fmtRange.GetData());
        }
    }
    return PlaceHolder.Set(pText->ToWStr() + CurTextIndex, CurTextIndex, pText->GetLength() - CurTextIndex, NULL); 
}

void GFxTextParagraph::FormatRunIterator::operator++() 
{ 
    //if (CurFormatIndex < pFormatInfo->Count())
    if (!FormatIterator.IsFinished())
    {
        const TextFormatRunType& fmtRange = *FormatIterator;
        GASSERT(fmtRange.Index >= 0);
        if (CurTextIndex < UPInt(fmtRange.Index))
        {
            CurTextIndex = fmtRange.Index;
        }
        else
        {
            GASSERT(UPInt(fmtRange.Index) == CurTextIndex);
            CurTextIndex += fmtRange.Length;
            //++CurFormatIndex;
            ++FormatIterator;
        }
    }
    else
        CurTextIndex = pText->GetLength();
}

void GFxTextParagraph::FormatRunIterator::SetTextPos(UPInt newTextPos)
{
    while(!IsFinished())
    {
        const GFxTextParagraph::StyledTextRun& stRun = *(*this); 
        //GASSERT(fmtRange.Index >= 0);
        if (stRun.Index >= (SPInt)newTextPos)
            break;
        operator++();
    }
}

//////////////////////////////////////////////////////////////////////////
GFxTextParagraph::CharactersIterator::CharactersIterator(const GFxTextParagraph* pparagraph) : 
    pFormatInfo(&pparagraph->FormatInfo), FormatIterator(pFormatInfo->Begin()), pText(&pparagraph->Text),  
    CurTextIndex(0)
{
}

GFxTextParagraph::CharactersIterator::CharactersIterator(const GFxTextParagraph* pparagraph, UPInt index) : 
    pFormatInfo(&pparagraph->FormatInfo), FormatIterator(pFormatInfo->GetIteratorByNearestIndex(index)),
    pText(&pparagraph->Text), CurTextIndex(index)
{
    if (!FormatIterator.IsFinished())
    {
        if (!FormatIterator->Contains(index) && (SInt)index > FormatIterator->Index)
        {
            ++FormatIterator;
        }
    }
}

GFxTextParagraph::CharacterInfo& GFxTextParagraph::CharactersIterator::operator*() 
{ 
    if (!IsFinished())
    {
        PlaceHolder.Character = *(pText->ToWStr() + CurTextIndex);
        PlaceHolder.Index     = CurTextIndex;
        if (!FormatIterator.IsFinished())
        {
            const TextFormatRunType& fmtRange = *FormatIterator; 
            GASSERT(fmtRange.Index >= 0);
            if (CurTextIndex < (UPInt)fmtRange.Index)
            {
                // if index in text is lower than current range index - 
                // just return a text w/o format.
                PlaceHolder.pFormat = NULL;
            }
            else
            {
                GASSERT(fmtRange.Contains(CurTextIndex));
                PlaceHolder.pFormat = fmtRange.GetData();
            }
        }
        else
            PlaceHolder.pFormat = NULL;
    }
    else
    {
        PlaceHolder.Character   = 0;
        PlaceHolder.Index       = CurTextIndex;
        PlaceHolder.pFormat     = NULL;
    }
    return PlaceHolder; 
}

void GFxTextParagraph::CharactersIterator::operator++() 
{ 
    if (!IsFinished())
    {
        ++CurTextIndex;
        if (!FormatIterator.IsFinished())
        {
            const TextFormatRunType& fmtRange = *FormatIterator; 
            GASSERT(fmtRange.Index >= 0);
            if (CurTextIndex >= UPInt(fmtRange.NextIndex()))
            {
                ++FormatIterator;
            }
        }
    }
    else
        CurTextIndex = pText->GetLength();
}

void GFxTextParagraph::CharactersIterator::operator+=(UPInt n) 
{
    for(UPInt i = 0; i < n; ++i)
    {
        operator++();
    }
}

const wchar_t* GFxTextParagraph::CharactersIterator::GetRemainingTextPtr(UPInt* plen) const 
{ 
    if (!IsFinished())
    {
        if (plen) *plen = pText->GetLength() - CurTextIndex;
        return pText->ToWStr() + CurTextIndex;
    }
    *plen = 0;
    return NULL;
}

//////////////////////////////////
// GFxStyledText
//
GFxStyledText::GFxStyledText():Paragraphs(), RTFlags(0) 
{
    pDefaultParagraphFormat = *new GFxTextParagraphFormat;
    pDefaultTextFormat = *new GFxTextFormat;
}

GFxStyledText::GFxStyledText(GFxTextAllocator* pallocator) : 
    pTextAllocator(pallocator),Paragraphs(), RTFlags(0)
{
    GASSERT(pallocator);
    pDefaultParagraphFormat = pallocator->AllocateParagraphFormat(GFxTextParagraphFormat());
    pDefaultTextFormat = pallocator->AllocateTextFormat(GFxTextFormat());
}

GFxTextParagraph* GFxStyledText::AppendNewParagraph(const GFxTextParagraphFormat* pdefParaFmt)
{
    UPInt sz = Paragraphs.size();
    UPInt nextPos = 0;
    if (sz > 0)
    {
        GFxTextParagraph* ppara = Paragraphs[sz - 1];
        GASSERT(ppara);
        nextPos = ppara->GetStartIndex() + ppara->GetLength();
    }

    Paragraphs.append(GetAllocator()->AllocateParagraph());

    sz = Paragraphs.size();
    GFxTextParagraph* ppara = Paragraphs[sz - 1];
    GASSERT(ppara);
    ppara->SetFormat((pdefParaFmt == NULL) ? *pDefaultParagraphFormat : *pdefParaFmt);
    ppara->SetStartIndex(nextPos);
    return ppara;
}

GFxTextParagraph* GFxStyledText::InsertNewParagraph(GFxStyledText::ParagraphsIterator& iter, 
                                                    const GFxTextParagraphFormat* pdefParaFmt)
{
    if (iter.IsFinished())
        return AppendNewParagraph(pdefParaFmt);
    else
    {
        UPInt index = UPInt(iter.get_index());
        UPInt nextPos = 0;
        if (index > 0)
        {
            GFxTextParagraph* ppara = Paragraphs[index - 1];
            GASSERT(ppara);
            nextPos = ppara->GetStartIndex() + ppara->GetLength();
        }

        Paragraphs.insert(index, GetAllocator()->AllocateParagraph());

        GFxTextParagraph* ppara = Paragraphs[index];
        GASSERT(ppara);
        ppara->SetFormat((pdefParaFmt == NULL) ? *pDefaultParagraphFormat : *pdefParaFmt);
        ppara->SetStartIndex(nextPos);
        return ppara;
    }
}

GFxTextParagraph* GFxStyledText::GetLastParagraph()
{
    ParagraphsIterator paraIter = Paragraphs.End();
    if (!paraIter.IsFinished())
    {
        return *paraIter;
    }
    return NULL;
}

void GFxStyledText::AppendString(const char* putf8String, UPInt stringSize)
{
    GASSERT(GetDefaultTextFormat() && GetDefaultParagraphFormat());
    AppendString(putf8String, stringSize, GetDefaultTextFormat(), GetDefaultParagraphFormat());
}

void GFxStyledText::AppendString(const char* putf8String, UPInt stringSize,
                                 const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt)
{
    GASSERT(pdefTextFmt && pdefParaFmt);
    // parse text, and form paragraphs
    const char* pbegin = putf8String;
    if (stringSize == GFC_MAX_UPINT)
        stringSize = gfc_strlen(putf8String);
    const char* pend = pbegin + stringSize;

    // parse text, and form paragraphs
    UInt32 uniChar = 0;

    UPInt curOffset;
    GFxTextParagraph* ppara = GetLastParagraph();
    if (!ppara)
        curOffset = 0;
    else
        curOffset = ppara->GetStartIndex();
    UInt i = 0;
    do
    {
        UPInt posInPara;
        UPInt startOffset = curOffset;
        if (i++ > 0 || ppara == NULL)
        {
            // need to create new paragraph
            ppara = AppendNewParagraph(pdefParaFmt);
            posInPara = 0;
            ppara->SetStartIndex(startOffset);
        }
        else
        {
            // use existing paragraph
            posInPara = ppara->GetLength();
        }
        GASSERT(ppara);
        UPInt paraLength = 0;
        const char* pstr = pbegin;
        while (pstr < pend && (uniChar = GUTF8Util::DecodeNextChar(&pstr)) != 0)
        {
            if (uniChar == '\n' || uniChar == '\r')
                break;
            ++paraLength;
        }
        if (uniChar == '\n' || uniChar == '\r')
            ++paraLength; 

        wchar_t* pwstr = ppara->CreatePosition(posInPara, paraLength);

        pstr = pbegin;
        while (pstr < pend && (uniChar = GUTF8Util::DecodeNextChar(&pstr)) != 0)
        {
            if (uniChar == '\r' || uniChar == '\n')  // replace '\r' by '\n'
                uniChar = NewLineChar();
            *pwstr++ = (wchar_t)uniChar;
            if (uniChar == NewLineChar())
                break;
        }
        ppara->SetTextFormat(*pdefTextFmt, posInPara, GFC_MAX_UPINT);
        pbegin = pstr;
        curOffset += paraLength + posInPara;
    } while(pbegin < pend && uniChar != 0);

    // check the last char. If it is \n - create the empty paragraph
    if (uniChar == NewLineChar())
        AppendNewParagraph(pdefParaFmt);

    if (pdefTextFmt->IsUrlSet())
        SetMayHaveUrl();

    CheckIntegrity();
}

void GFxStyledText::AppendString(const wchar_t* pstr, UPInt length)
{
    GASSERT(GetDefaultTextFormat() && GetDefaultParagraphFormat());
    AppendString(pstr, length, GetDefaultTextFormat(), GetDefaultParagraphFormat());
}

void GFxStyledText::AppendString(const wchar_t* pstr, UPInt length,
                                 const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt)
{
    GASSERT(pdefTextFmt && pdefParaFmt);

    // parse text, and form paragraphs
    UInt32 uniChar = 0;
    if (length == GFC_MAX_UPINT)
        length = wcslen(pstr);
    const wchar_t* pend = pstr + length;

    UPInt curOffset;
    GFxTextParagraph* ppara = GetLastParagraph();
    if (!ppara)
        curOffset = 0;
    else
        curOffset = ppara->GetStartIndex();
    UInt i = 0;
    do
    {
        UPInt posInPara;
        UPInt startOffset = curOffset;
        if (i++ > 0 || ppara == NULL)
        {
            // need to create new paragraph
            ppara = AppendNewParagraph(pdefParaFmt);
            posInPara = 0;
            ppara->SetStartIndex(startOffset);
        }
        else
        {
            // use existing paragraph
            posInPara = ppara->GetLength();
        }
        UPInt paraLength = 0;
        for(; paraLength < length; ++paraLength)
        {
            uniChar = pstr[paraLength];
            if (uniChar == '\n' || uniChar == '\r' || uniChar == '\0')
                break;
        }
        if (uniChar == '\n' || uniChar == '\r')
            ++paraLength; // skip \n

        wchar_t* pwstr = ppara->CreatePosition(posInPara, paraLength);
        memcpy(pwstr, pstr, paraLength * sizeof(wchar_t));
        pstr += paraLength;
        length -= paraLength;
        GASSERT(((SPInt)length) >= 0);

        if ((uniChar == '\n' || uniChar == '\r') && uniChar != NewLineChar())
        {
            // need to add '\n' instead of '\r'
            UPInt l = ppara->GetLength();
            if (l > 0)
            {
                wchar_t* p = ppara->GetText();
                p[l - 1] = NewLineChar();
            }
            uniChar = NewLineChar();
        }
        ppara->SetTextFormat(*pdefTextFmt, posInPara, GFC_MAX_UPINT);
        curOffset += paraLength + posInPara;
    } while(pstr < pend && uniChar != '\0');

    // check the last char. If it is \n - create the empty paragraph
    if (uniChar == NewLineChar())
        AppendNewParagraph(pdefParaFmt);

    CheckIntegrity();
}

void GFxStyledText::SetText(const char* putf8String, UPInt stringSize)
{
    Clear();
    AppendString(putf8String, stringSize);
}

void GFxStyledText::SetText(const wchar_t* pstr, UPInt length)
{
    Clear();
    AppendString(pstr, length);
}

GFxStyledText::ParagraphsIterator GFxStyledText::GetParagraphByIndex(UPInt index, UPInt* pindexInParagraph)
{
    static ParagraphComparator cmp;
    SPInt i = GTL::find_index_in_sorted_array(Paragraphs, index, cmp);
    if (i != -1)
    {
        ParagraphsIterator it(Paragraphs.Begin() + (int)i);
        if (pindexInParagraph)
            *pindexInParagraph = index - it->GetPtr()->GetStartIndex();
        return it;    
    }
    return ParagraphsIterator();
}

GFxStyledText::ParagraphsIterator GFxStyledText::GetNearestParagraphByIndex(UPInt index, UPInt* pindexInParagraph)
{
    static ParagraphComparator cmp;
    SPInt i = GTL::find_nearest_index_in_sorted_array(Paragraphs, index, cmp);
    if (i != -1)
    {
        ParagraphsIterator it(Paragraphs.Begin() + (int)i);
        if (pindexInParagraph)
        {
            *pindexInParagraph = index - (*it)->GetStartIndex();
        }
        return it;    
    }
    return ParagraphsIterator();
}

void GFxStyledText::SetTextFormat(const GFxTextFormat& fmt, UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    UPInt indexInPara, endIndexInPara;
    UPInt runLen = endPos - startPos;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    while(runLen > 0 && !paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt paraLength = ppara->GetLength();
        if (runLen > paraLength)
            endIndexInPara = indexInPara + paraLength - indexInPara;
        else
            endIndexInPara = indexInPara + runLen;

        OnParagraphTextFormatChanging(*ppara, indexInPara, endIndexInPara, fmt);
        ppara->SetTextFormat(fmt, indexInPara, endIndexInPara);
        OnParagraphTextFormatChanged(*ppara, indexInPara, endIndexInPara);

        GASSERT(runLen >= (endIndexInPara - indexInPara));
        runLen -= (endIndexInPara - indexInPara);
        indexInPara = 0;
        ++paraIter;
    }
    if (fmt.IsUrlSet())
        SetMayHaveUrl();
    CheckIntegrity();
}

void GFxStyledText::GetTextAndParagraphFormat
    (GFxTextFormat* pdestTextFmt, GFxTextParagraphFormat* pdestParaFmt, UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    GASSERT(pdestParaFmt || pdestTextFmt);

    UPInt indexInPara;
    UPInt runLen = endPos - startPos + 1;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    GFxTextFormat finalTextFmt;
    GFxTextParagraphFormat finalParaFmt;
    int i = 0;
    while(runLen > 0 && !paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt lengthInPara = GTL::gpmin(runLen, ppara->GetLength());
        if (i++ == 0)
            finalTextFmt = ppara->GetTextFormat(indexInPara, indexInPara + lengthInPara);
        else
            finalTextFmt = ppara->GetTextFormat(indexInPara, indexInPara + lengthInPara).Intersection(finalTextFmt);
        if (indexInPara == 0)
        {
            const GFxTextParagraphFormat* ppfmt = ppara->GetFormat();
            if (ppfmt)
                finalParaFmt = ppfmt->Intersection(finalParaFmt);
        }
        GASSERT(runLen >= lengthInPara);
        runLen -= lengthInPara;
        ++paraIter;
    }
    if (pdestTextFmt)
        *pdestTextFmt = finalTextFmt;
    if (pdestParaFmt)
        *pdestParaFmt = finalParaFmt;
}

bool GFxStyledText::GetTextAndParagraphFormat
    (const GFxTextFormat** ppdestTextFmt, const GFxTextParagraphFormat** ppdestParaFmt, UPInt pos)
{
    GASSERT(ppdestParaFmt || ppdestTextFmt);

    UPInt indexInPara;
    ParagraphsIterator paraIter = GetParagraphByIndex(pos, &indexInPara);
    const GFxTextParagraphFormat* pparaFmt = NULL;
    const GFxTextFormat* ptextFmt = NULL;
    bool rv = false;
    if (!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        ptextFmt = ppara->GetTextFormatPtr(indexInPara);
        pparaFmt = ppara->GetFormat();
        rv = true;
    }
    if (ptextFmt == NULL)
        ptextFmt = pDefaultTextFormat;
    if (pparaFmt == NULL)
        pparaFmt = pDefaultParagraphFormat;
    if (ppdestTextFmt)
        *ppdestTextFmt = ptextFmt;
    if (ppdestParaFmt)
        *ppdestParaFmt = pparaFmt;
    return rv;
}

void GFxStyledText::SetParagraphFormat(const GFxTextParagraphFormat& fmt, UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    UPInt indexInPara, endIndexInPara;
    UPInt runLen = endPos - startPos;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    while(runLen > 0 && !paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        if (indexInPara == 0)
        {
            OnParagraphFormatChanging(*ppara, fmt);
            ppara->SetFormat(fmt);
            OnParagraphFormatChanged(*ppara);
        }
        UPInt paraLength = ppara->GetLength();
        if (runLen > paraLength)
            endIndexInPara = indexInPara + paraLength - indexInPara;
        else
            endIndexInPara = indexInPara + runLen;
        GASSERT(runLen >= (endIndexInPara - indexInPara));
        runLen -= (endIndexInPara - indexInPara);
        indexInPara = 0;
        ++paraIter;
    }

    CheckIntegrity();
}

void GFxStyledText::SetDefaultTextFormat(const GFxTextFormat& defaultTextFmt)
{
    if (defaultTextFmt.GetImageDesc())
    {
        GFxTextFormat textfmt = defaultTextFmt;
        textfmt.SetImageDesc(NULL);
        pDefaultTextFormat = GetAllocator()->AllocateTextFormat(textfmt);
    }
    else 
        pDefaultTextFormat = GetAllocator()->AllocateTextFormat(defaultTextFmt);
    GASSERT(pDefaultTextFormat);
}

void GFxStyledText::SetDefaultParagraphFormat(const GFxTextParagraphFormat& defaultParagraphFmt)
{
    pDefaultParagraphFormat = GetAllocator()->AllocateParagraphFormat(defaultParagraphFmt);
    GASSERT(pDefaultParagraphFormat);
}

void GFxStyledText::SetDefaultTextFormat(const GFxTextFormat* pdefaultTextFmt)
{
    GASSERT(pdefaultTextFmt);
    if (pdefaultTextFmt->GetImageDesc())
    {
        SetDefaultTextFormat(*pdefaultTextFmt);
    }
    else
    {
        pDefaultTextFormat = const_cast<GFxTextFormat*>(pdefaultTextFmt);
    }
}

void GFxStyledText::SetDefaultParagraphFormat(const GFxTextParagraphFormat* pdefaultParagraphFmt)
{
    GASSERT(pdefaultParagraphFmt);
    pDefaultParagraphFormat = const_cast<GFxTextParagraphFormat*>(pdefaultParagraphFmt);
}


GFxString GFxStyledText::GetHtml() const
{
    GFxString retStr;
    ParagraphsIterator paraIter = GetParagraphIterator();
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        retStr += "<TEXTFORMAT";
        GASSERT(ppara->GetFormat());
        const GFxTextParagraphFormat& paraFormat = *ppara->GetFormat();
        if (paraFormat.IsIndentSet())
        {
            retStr += " INDENT=\"";
            char buf[20];
            gfc_sprintf(buf, 20, "%d\"", paraFormat.GetIndent());
            retStr += buf;
        }
        if (paraFormat.IsBlockIndentSet())
        {
            retStr += " BLOCKINDENT=\"";
            char buf[20];
            gfc_sprintf(buf, 20, "%d\"", paraFormat.GetBlockIndent());
            retStr += buf;
        }
        if (paraFormat.IsLeftMarginSet())
        {
            retStr += " LEFTMARGIN=\"";
            char buf[20];
            gfc_sprintf(buf, 20, "%d\"", paraFormat.GetLeftMargin());
            retStr += buf;
        }
        if (paraFormat.IsRightMarginSet())
        {
            retStr += " RIGHTMARGIN=\"";
            char buf[20];
            gfc_sprintf(buf, 20, "%d\"", paraFormat.GetRightMargin());
            retStr += buf;
        }
        if (paraFormat.IsLeadingSet())
        {
            retStr += " LEADING=\"";
            char buf[20];
            gfc_sprintf(buf, 20, "%d\"", paraFormat.GetLeading());
            retStr += buf;
        }
        if (paraFormat.IsTabStopsSet())
        {
            // TODO!
        }
        retStr += "><";
        if (!paraFormat.IsBullet())
            retStr += "P";
        else
            retStr += "LI";
        retStr += " ALIGN=\"";
        switch(paraFormat.GetAlignment())
        {
        case GFxTextParagraphFormat::Align_Left: retStr += "LEFT"; break;
        case GFxTextParagraphFormat::Align_Right: retStr += "RIGHT"; break;
        case GFxTextParagraphFormat::Align_Center: retStr += "CENTER"; break;
        case GFxTextParagraphFormat::Align_Justify: retStr += "JUSTIFY"; break;
        }
        retStr += "\">";
        GFxTextParagraph::FormatRunIterator it = ppara->GetIterator();
        for(; !it.IsFinished(); ++it)
        {
            const GFxTextParagraph::StyledTextRun& run = *it;
            bool isImageTag = false;
            if (run.pFormat)
            {
                if (run.pFormat->IsImageDescSet())
                {
                    GPtr<GFxTextHTMLImageTagDesc> pimage = run.pFormat->GetImageDesc();
                    if (pimage)
                    {
                        isImageTag = true;
                        retStr += "<IMG SRC=\"";

                        retStr += pimage->Url;
                        retStr += "\"";
                        if (pimage->ScreenWidth > 0)
                        {
                            retStr += " WIDTH=\"";
                            char buf[20];
                            gfc_sprintf(buf, 20, "%d\"", TwipsToPixels(pimage->ScreenWidth));
                            retStr += buf;
                        }
                        if (pimage->ScreenHeight > 0)
                        {
                            retStr += " HEIGHT=\"";
                            char buf[20];
                            gfc_sprintf(buf, 20, "%d\"", TwipsToPixels(pimage->ScreenHeight));
                            retStr += buf;
                        }
                        if (pimage->VSpace != 0)
                        {
                            retStr += " VSPACE=\"";
                            char buf[20];
                            gfc_sprintf(buf, 20, "%d\"", TwipsToPixels(pimage->VSpace));
                            retStr += buf;
                        }
                        if (pimage->HSpace != 0)
                        {
                            retStr += " HSPACE=\"";
                            char buf[20];
                            gfc_sprintf(buf, 20, "%d\"", TwipsToPixels(pimage->HSpace));
                            retStr += buf;
                        }
                        if (!pimage->Id.IsEmpty())
                        {
                            retStr += " ID=\"";
                            retStr += pimage->Id;
                            retStr += "\"";
                        }
                        retStr += " ALIGN=\"";
                        switch(pimage->Alignment)
                        {
                        case GFxStyledText::HTMLImageTagInfo::Align_Left:
                            retStr += "left";
                            break;
                        case GFxStyledText::HTMLImageTagInfo::Align_Right:
                            retStr += "right";
                            break;
                        case GFxStyledText::HTMLImageTagInfo::Align_BaseLine:
                            retStr += "baseline";
                            break;
                        default: break;
                        }
                        retStr += "\">";
                    }
                }
                else
                {
                    retStr += "<FONT";
                    if (run.pFormat->IsFontNameSet())
                    {
                        retStr += " FACE=\"";
                        const char* pfontName = run.pFormat->GetFontName();
                        retStr += pfontName;
                        retStr += "\"";
                    }
                    if (run.pFormat->IsFontSizeSet())
                    {
                        retStr += " SIZE=\"";
                        char buf[20];
                        gfc_sprintf(buf, 20, "%d\"", run.pFormat->GetFontSize());
                        retStr += buf;
                    }
                    if (run.pFormat->IsColorSet())
                    {
                        retStr += " COLOR=\"#";
                        char buf[20];
                        gfc_sprintf(buf, 20, "%.6X\"", (run.pFormat->GetColor() & 0xFFFFFFu));
                        retStr += buf;
                    }
                    if (run.pFormat->IsLetterSpacingSet())
                    {
                        retStr += " LETTERSPACING=\"";
                        char buf[20];
                        gfc_sprintf(buf, 20, "%f\"", run.pFormat->GetLetterSpacing());
                        retStr += buf;
                    }
                    if (run.pFormat->IsAlphaSet())
                    {
                        retStr += " ALPHA=\"#";
                        char buf[20];
                        gfc_sprintf(buf, 20, "%.2X\"", (run.pFormat->GetAlpha() & 0xFFu));
                        retStr += buf;
                    }
                    retStr += " KERNING=\"";
                    retStr += (run.pFormat->IsKerning()) ? "1" : "0";
                    retStr += "\"";

                    retStr += ">";
                    if (run.pFormat->IsUrlSet())
                    {
                        retStr += "<A HREF=\"";
                        retStr += run.pFormat->GetUrl();
                        retStr += "\">";
                    }

                    if (run.pFormat->IsBold())
                        retStr += "<B>";
                    if (run.pFormat->IsItalic())
                        retStr += "<I>";
                    if (run.pFormat->IsUnderline())
                        retStr += "<U>";
                }
            }

            if (!isImageTag)
            {
                for (UPInt i = 0; i < run.Length; ++i)
                {
                    if (run.pText[i] == NewLineChar())
                        continue;
                    retStr.AppendChar(run.pText[i]);
                }
                if (run.pFormat)
                {
                    if (run.pFormat->IsUnderline())
                        retStr += "</U>";
                    if (run.pFormat->IsItalic())
                        retStr += "</I>";
                    if (run.pFormat->IsBold())
                        retStr += "</B>";
                    if (run.pFormat->IsUrlSet())
                        retStr += "</A>";
                    retStr += "</FONT>";
                }
            }
        }

        retStr += "</";
        if (!paraFormat.IsBullet())
            retStr += "P";
        else
            retStr += "LI";
        retStr += "></TEXTFORMAT>";
    }
    return retStr;
}

UPInt GFxStyledText::GetLength() const
{
    UPInt length = 0;
    ParagraphsIterator paraIter = GetParagraphIterator();
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        length += ppara->GetLength();
    }
    return length;
}

GFxString GFxStyledText::GetText() const
{
    GFxString retStr;
    ParagraphsIterator paraIter = GetParagraphIterator();
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        const wchar_t* pstr = ppara->GetText();
        retStr.AppendString(pstr, ppara->GetLength());
    }
    return retStr;
}

void GFxStyledText::GetText(GFxWideStringBuffer* pBuffer) const
{
    GASSERT(pBuffer);

    pBuffer->Clear();
    ParagraphsIterator paraIter = GetParagraphIterator();
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        const wchar_t* pstr = ppara->GetText();
        pBuffer->CreatePosition(pBuffer->GetLength(),  ppara->GetLength());
        memcpy(pBuffer->ToWStr(), pstr, sizeof(wchar_t) * ppara->GetLength());
    }
	UPInt end = pBuffer->GetLength();
	pBuffer->CreatePosition(end, 1);
	(pBuffer->ToWStr())[end] = 0;
}

wchar_t* GFxStyledText::CreatePosition(UPInt pos, UPInt length)
{
    GUNUSED2(pos, length);
    return 0;
}

void GFxStyledText::InsertString(const wchar_t* pstr, UPInt pos, UPInt length)
{
    InsertString(pstr, pos, length, GetDefaultTextFormat(), GetDefaultParagraphFormat());
}

void GFxStyledText::InsertString(const wchar_t* pstr, UPInt pos, UPInt length, 
                                 const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt)
{
    if (length == 0)
        return;
    if (pos > GetLength())
        pos = GetLength();

    // Insert plain string at the position. Style will be inherited from the current style
    // at the insertion position. If there is no style at the insertion position, then
    // text will remain plain. New paragraph will be created after each CR.
    if (length == GFC_MAX_UPINT)
        length = GFxWideStringBuffer::StrLen(pstr);
    UPInt indexInPara = 0;
    UPInt remainingSrcStrLen = length;
    SPInt newLineIdx;
    ParagraphsIterator paraIter = GetNearestParagraphByIndex(pos, &indexInPara);

    UPInt insLineLen;
    UPInt nextParaStartingPos = (!paraIter.IsFinished()) ? (*paraIter)->GetStartIndex() : 0;

    do
    {
        if (paraIter.IsFinished())
        {
            // ok, there is no more (or none) paragraphs - create a new one
            AppendNewParagraph(pdefParaFmt);
            paraIter = GetParagraphIterator();
            indexInPara = 0;
        }
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);

        // look for a new-line char in the line being inserted.
        newLineIdx = GFxWideStringBuffer::StrChr(pstr, remainingSrcStrLen, NewLineChar());
        if (newLineIdx != -1)
        {
            insLineLen = UPInt(newLineIdx + 1); // including '\n'
        }
        else
        {
            insLineLen = remainingSrcStrLen;
        }

        // was there a new-line symbol?
        if (newLineIdx != -1)
        {
            // yes, make another paragraph first
            ParagraphsIterator newParaIter = paraIter;
            ++newParaIter;
            GFxTextParagraph& newPara = *InsertNewParagraph(newParaIter, pdefParaFmt);

            // new paragraph should inherit style from the previous ppara->
            const GFxTextParagraphFormat* pparaFmt = ppara->GetFormat();
            if (pparaFmt)
                newPara.SetFormat(*pparaFmt);

            // now, copy the text to the new para
            newPara.Copy(*ppara, indexInPara, 0, ppara->GetLength() - indexInPara);

            UPInt remLen = ppara->GetLength();
            GASSERT(remLen >= indexInPara);
            remLen -= indexInPara;

            // now, we can insert the sub-string in source para
            OnParagraphTextInserting(*ppara, indexInPara, insLineLen);
            ppara->InsertString(pstr, indexInPara, insLineLen, pdefTextFmt);
            OnParagraphTextInserted(*ppara, indexInPara, indexInPara + insLineLen, pstr);

            if (remLen > 0)
            {
                // now, make source paragraph shorter
                OnParagraphTextRemoving(*ppara, indexInPara + insLineLen, remLen);
                ppara->Shrink(remLen);
                OnParagraphTextRemoved(*ppara, indexInPara + insLineLen, remLen);
            }
        }
        else
        {
            // the string being inserted doesn't contain any '\n'
            // so, insert it as a substring
            OnParagraphTextInserting(*ppara, indexInPara, insLineLen);
            ppara->InsertString(pstr, indexInPara, insLineLen, pdefTextFmt);
            OnParagraphTextInserted(*ppara, indexInPara, indexInPara + insLineLen, pstr);
        }

        GASSERT(remainingSrcStrLen >= insLineLen);
        remainingSrcStrLen -= insLineLen;
        pstr += insLineLen;
        indexInPara = 0;
        ppara->SetStartIndex(nextParaStartingPos);
        nextParaStartingPos += ppara->GetLength();
        ++paraIter;
    } while(remainingSrcStrLen > 0);

    // correct the starting indices for remaining paragraphs
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        ppara->SetStartIndex(nextParaStartingPos);
        nextParaStartingPos += ppara->GetLength();
    }

    if (pdefTextFmt->IsUrlSet())
        SetMayHaveUrl();
    CheckIntegrity();
}

void GFxStyledText::Remove(UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    UPInt length;
    if (endPos == GFC_MAX_UPINT)
        length = GetLength();
    else
        length = endPos - startPos;
    UPInt indexInPara;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    GFxTextParagraph* pprevPara = NULL;

    UPInt remainingLen = length;
    bool needUniteParas = false;
    // first stage - remove part of this para
    if (!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);

        UPInt lengthInPara = GTL::gpmin(remainingLen, ppara->GetLength() - indexInPara);
        if (lengthInPara <= ppara->GetLength())
        {
            needUniteParas = (indexInPara + lengthInPara >= ppara->GetLength());
            pprevPara = *paraIter;
            OnParagraphTextRemoving(*ppara, indexInPara, lengthInPara);
            ppara->Remove(indexInPara, indexInPara + lengthInPara);
            OnParagraphTextRemoved(*ppara, indexInPara, lengthInPara);
            remainingLen -= lengthInPara;
            ++paraIter;
        }
    }

    // second stage - remove all paragraphs we can remove completely
    while(!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt paraLen = ppara->GetLength();
        if (remainingLen >= paraLen)
        {
            OnParagraphRemoving(*ppara);
            paraIter.remove();
            remainingLen -= paraLen;
        }
        else if (pprevPara)
        {
            if (needUniteParas)
            {
                // 3rd stage: do a copy of remaining part of the last para
                UPInt insPos = pprevPara->GetLength();
                UPInt insLen = paraLen - remainingLen;
                OnParagraphTextInserting(*pprevPara, insPos, insLen);
                pprevPara->Copy(*ppara, remainingLen, insPos, insLen);
                OnParagraphTextInserted(*pprevPara, insPos, insLen, NULL);

                OnParagraphRemoving(*ppara);
                paraIter.remove();
            }
            break;
        }
        else  
            break; //???
        if (remainingLen == 0) break;
    }
    // correct the starting indices for remaining paragraphs
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        ppara->SetStartIndex(ppara->GetStartIndex() - length);
    }
    CheckIntegrity();
}

void GFxStyledText::OnParagraphTextFormatChanging(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const GFxTextFormat& formatToBeSet)
{
    GUNUSED(para);
    GUNUSED3(startPos, endPos, formatToBeSet);
}
void GFxStyledText::OnParagraphTextFormatChanged(const GFxTextParagraph& para, UPInt startPos, UPInt endPos)
{
    GUNUSED(para);
    GUNUSED2(startPos, endPos);
}

void GFxStyledText::OnParagraphFormatChanging(const GFxTextParagraph& para, const GFxTextParagraphFormat& formatToBeSet)
{
    GUNUSED(para);
    GUNUSED(formatToBeSet);
}
void GFxStyledText::OnParagraphFormatChanged(const GFxTextParagraph& para)
{
    GUNUSED(para);
}

void GFxStyledText::OnParagraphTextInserting(const GFxTextParagraph& para, UPInt insertionPos, UPInt insertingLen)
{
    GUNUSED(para);
    GUNUSED2(insertionPos, insertingLen);
}

void GFxStyledText::OnParagraphTextInserted(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const wchar_t* ptextInserted)
{
    GUNUSED(para);
    GUNUSED3(startPos, endPos, ptextInserted);
}

void GFxStyledText::OnParagraphTextRemoving(const GFxTextParagraph& para, UPInt removingPos, UPInt removingLen)
{
    GUNUSED(para);
    GUNUSED2(removingPos, removingLen);
}

void GFxStyledText::OnParagraphTextRemoved(const GFxTextParagraph& para, UPInt removedPos, UPInt removedLen)
{
    GUNUSED(para);
    GUNUSED2(removedPos, removedLen);
}

void GFxStyledText::OnParagraphRemoving(const GFxTextParagraph& para)
{
    GUNUSED(para);
}

#ifdef GFC_BUILD_DEBUG
void GFxStyledText::CheckIntegrity() const
{
    ParagraphsIterator paraIter = GetParagraphIterator();
    UPInt pos = 0;
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        ppara->CheckIntegrity();
        GASSERT(pos == ppara->GetStartIndex());
        pos += ppara->GetLength();
    }
}
#endif

//////////////////////////////////////////////////////////////////////////
// GFxStyledText's iterators
//
GFxStyledText::CharactersIterator::CharactersIterator(GFxStyledText* ptext) : 
    Paragraphs(ptext->GetParagraphIterator()), pText(ptext), FirstCharInParagraphIndex(0)
{
    if (!Paragraphs.IsFinished())
    {
        GFxTextParagraph* ppara = *Paragraphs;
        GASSERT(ppara);
        FirstCharInParagraphIndex = ppara->GetStartIndex();
        Characters = ppara->GetCharactersIterator();
    }
}
GFxStyledText::CharactersIterator::CharactersIterator(GFxStyledText* ptext, SInt index) : pText(ptext)
{
    UPInt indexInPara;
    Paragraphs = ptext->GetParagraphByIndex(index, &indexInPara);
    if (!Paragraphs.IsFinished())
    {
        GFxTextParagraph* ppara = *Paragraphs;
        GASSERT(ppara);
        FirstCharInParagraphIndex = ppara->GetStartIndex();
        Characters = ppara->GetCharactersIterator(indexInPara);
    }
}

GFxStyledText::CharacterInfo& GFxStyledText::CharactersIterator::operator*() 
{
    GFxTextParagraph::CharacterInfo& chInfo = *Characters;
    CharInfoPlaceHolder.Character = chInfo.Character;
    CharInfoPlaceHolder.Index     = chInfo.Index + FirstCharInParagraphIndex;
    CharInfoPlaceHolder.pOriginalFormat = chInfo.pFormat;
    CharInfoPlaceHolder.pParagraph = *Paragraphs;
    return CharInfoPlaceHolder;
}

void GFxStyledText::CharactersIterator::operator++() 
{ 
    ++Characters;
    if (Characters.IsFinished())
    {
        ++Paragraphs;
        if (!Paragraphs.IsFinished())
        {
            GFxTextParagraph* ppara = *Paragraphs;
            GASSERT(ppara);
            FirstCharInParagraphIndex = ppara->GetStartIndex();
            Characters = ppara->GetCharactersIterator();
        }
    }
}

void GFxStyledText::Clear()
{
    Paragraphs.resize(0);
    ClearMayHaveUrl();
}

//////////////////////////////////////////////////////////////////////////
// HTML parsing routine
//
enum GFxHTMLElementsEnum
{
    GFxHTML_A,
    GFxHTML_B,
    GFxHTML_BR,
    GFxHTML_FONT,
    GFxHTML_I,
    GFxHTML_IMG,
    GFxHTML_LI,
    GFxHTML_P,
    GFxHTML_TAB,
    GFxHTML_TEXTFORMAT,
    GFxHTML_U,

    GFxHTML_ALIGN,
    GFxHTML_ALPHA,
    GFxHTML_BASELINE,
    GFxHTML_BLOCKINDENT,
    GFxHTML_COLOR,
    GFxHTML_FACE,
    GFxHTML_HEIGHT,
    GFxHTML_HREF,
    GFxHTML_HSPACE,
    GFxHTML_ID,
    GFxHTML_INDENT,
    GFxHTML_KERNING,
    GFxHTML_LEADING,
    GFxHTML_LEFTMARGIN,
    GFxHTML_LETTERSPACING,
    GFxHTML_RIGHTMARGIN,
    GFxHTML_SRC,
    GFxHTML_SIZE,
    GFxHTML_TARGET,
    GFxHTML_TABSTOPS,
    GFxHTML_VSPACE,
    GFxHTML_WIDTH,

    GFxHTML_LEFT,
    GFxHTML_RIGHT,
    GFxHTML_CENTER,
    GFxHTML_JUSTIFY
};

struct GFxSGMLElementDesc
{
    const char*         ElemName;
    GFxHTMLElementsEnum ElemId;
    bool                ReqEndElement;
    bool                EmptyElement;

    template <class Char>
    struct Comparable
    {
        const Char* Str;
        UPInt      Size;

        Comparable(const Char* p, UPInt sz):Str(p), Size(sz) {}
    };

    template <class Char>
    struct Comparator
    {
        int Compare(const GFxSGMLElementDesc& p1, const Comparable<Char>& p2) const
        {
            return -GFxSGMLCharIter<Char>::StrCompare(p2.Str, p1.ElemName, p2.Size);
        }
    };
    template <class Char>
    static const GFxSGMLElementDesc* FindElem
        (const Char* lookForElemName, UPInt nameSize, const GFxSGMLElementDesc* ptable, int tableSize)
    {
        static Comparator<Char> cmp;
        Comparable<Char> e(lookForElemName, nameSize);
        SPInt i = GTL::find_index_in_sorted_primitive_array(ptable, tableSize, e, cmp);
        if (i != -1)
        {
            return &ptable[i];
        }
        return NULL;
    }
};

template <class Char>
struct GFxSGMLStackElemDesc
{
    const Char*             pElemName;
    UPInt                  ElemNameSize;
    const GFxSGMLElementDesc* pElemDesc;
    UPInt                   StartPos;
    GFxTextFormat           TextFmt;
    GFxTextParagraphFormat  ParaFmt;

    GFxSGMLStackElemDesc():pElemName(NULL),ElemNameSize(0),pElemDesc(NULL) {}
    GFxSGMLStackElemDesc(const Char* pname, UPInt sz, const GFxSGMLElementDesc* pelemDesc, UPInt startPos) : 
    pElemName(pname), ElemNameSize(sz), pElemDesc(pelemDesc), StartPos(startPos) {}
};

template <class Char>
bool GFxStyledText::ParseHtml(const Char* phtml, UPInt htmlSize, GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr, bool multiline, bool condenseWhite)
{
    static const GFxSGMLElementDesc elementsTable[] = 
    {  // must be sorted by name!
        { "A",      GFxHTML_A,      true, false },
        { "B",      GFxHTML_B,      true, false },
        { "BR",     GFxHTML_BR,     false, true },
        { "FONT",   GFxHTML_FONT,   true, false },
        { "I",      GFxHTML_I,      true, false },
        { "IMG",    GFxHTML_IMG,    false, true },
        { "LI",     GFxHTML_LI,     false, false },
        { "P",      GFxHTML_P,      false, false },
        { "SBR",    GFxHTML_BR,     false, true },
        { "TAB",    GFxHTML_TAB,     false, true },
        { "TEXTFORMAT", GFxHTML_TEXTFORMAT, true, false },
        { "U",      GFxHTML_U,      true, false }
    };

    static const GFxSGMLElementDesc attributesTable[] = 
    {  // must be sorted by name!
        { "ALIGN",      GFxHTML_ALIGN },
        { "ALPHA",      GFxHTML_ALPHA },
        { "BASELINE",   GFxHTML_BASELINE },
        { "BLOCKINDENT",GFxHTML_BLOCKINDENT },
        { "CENTER",     GFxHTML_CENTER },
        { "COLOR",      GFxHTML_COLOR },
        { "FACE",       GFxHTML_FACE },
        { "HEIGHT",     GFxHTML_HEIGHT },
        { "HREF",       GFxHTML_HREF },
        { "HSPACE",     GFxHTML_HSPACE },
        { "ID",         GFxHTML_ID },
        { "INDENT",     GFxHTML_INDENT },
        { "JUSTIFY",    GFxHTML_JUSTIFY },
        { "KERNING",    GFxHTML_KERNING },
        { "LEADING",    GFxHTML_LEADING },
        { "LEFT",       GFxHTML_LEFT },
        { "LEFTMARGIN", GFxHTML_LEFTMARGIN },
        { "LETTERSPACING", GFxHTML_LETTERSPACING },
        { "RIGHT",      GFxHTML_RIGHT },
        { "RIGHTMARGIN", GFxHTML_RIGHTMARGIN },
        { "SIZE",       GFxHTML_SIZE },
        { "SRC",        GFxHTML_SRC },
        { "TABSTOPS",   GFxHTML_TABSTOPS },
        { "TARGET",     GFxHTML_TARGET },
        { "VSPACE",     GFxHTML_VSPACE },
        { "WIDTH",      GFxHTML_WIDTH }
    };
    
    GTL::garray<GFxSGMLStackElemDesc<Char> > elementsStack;

    GFxSGMLParser<Char> parser(phtml, htmlSize);
    if (condenseWhite)
        parser.SetCondenseWhite();

    int type;
    GFxTextParagraph* pcurPara = NULL;
    const GFxSGMLElementDesc* plastElemDesc = NULL;
    int p_elem_is_opened = 0;
    
    GFxTextFormat defaultTextFmt            = *GetDefaultTextFormat();
    GFxTextParagraphFormat defaultParaFmt   = *GetDefaultParagraphFormat();

    while((type = parser.GetNext()) != SGMLPS_FINISHED && type != SGMLPS_ERROR)
    {
        switch(type)
        {
        case SGMLPS_START_ELEMENT:
            {
                const Char* elemName;
                UPInt elemLen;

                parser.ParseStartElement(&elemName, &elemLen);

                const GFxSGMLElementDesc* pelemDesc = GFxSGMLElementDesc::FindElem<Char>
                    (elemName, elemLen, elementsTable, sizeof(elementsTable)/sizeof(elementsTable[0]));
                plastElemDesc = pelemDesc;

                if (pelemDesc)
                {
                    int prevStackIndex = int(elementsStack.size()-1);

                    if (pelemDesc->EmptyElement)
                    {
                        switch(pelemDesc->ElemId)
                        {
                        case GFxHTML_BR:
                            // new paragraph
                            if (multiline)
                                AppendString(NewLineStr(), 1);
                            break;
                        case GFxHTML_TAB:
                            AppendString("\t", 1);
                            break;
                        case GFxHTML_IMG:
                            {
                                if (pimgInfoArr)
                                {
                                    const Char* pattrName;
                                    UPInt      attrSz;
                                    pimgInfoArr->push_back(HTMLImageTagInfo());
                                    HTMLImageTagInfo&  imgInfo = (*pimgInfoArr)[pimgInfoArr->size() - 1];
                                    pcurPara = GetLastParagraph();
                                    GFxTextFormat imgCharFmt;
                                    imgInfo.ParaId = pcurPara->GetId();
                                    while(parser.GetNextAttribute(&pattrName, &attrSz))
                                    {
                                        const GFxSGMLElementDesc* pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                            (pattrName, attrSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                        if (pattrDesc)
                                        {
                                            const Char* pattrVal;
                                            UPInt      attrValSz;                                        
                                            if (parser.GetNextAttributeValue(&pattrVal, &attrValSz))
                                            {
                                                switch(pattrDesc->ElemId)
                                                {
                                                case GFxHTML_SRC:
                                                    imgInfo.Url.AppendString(pattrVal, attrValSz);
                                                    break;
                                                case GFxHTML_ID:
                                                    imgInfo.Id.AppendString(pattrVal, attrValSz);
                                                    break;
                                                case GFxHTML_ALIGN:
                                                    pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                                        (pattrVal, attrValSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                                    if (pattrDesc)
                                                    {
                                                        switch(pattrDesc->ElemId)
                                                        {
                                                        case GFxHTML_LEFT:
                                                            imgInfo.Alignment = HTMLImageTagInfo::Align_Left;
                                                            break;
                                                        case GFxHTML_RIGHT:
                                                            imgInfo.Alignment = HTMLImageTagInfo::Align_Right;
                                                            break;
                                                        case GFxHTML_BASELINE:
                                                            imgInfo.Alignment = HTMLImageTagInfo::Align_BaseLine;
                                                            break;
                                                        default: break;
                                                        }
                                                    }
                                                    break;
                                                case GFxHTML_WIDTH:
                                                    {
                                                        int v;
                                                        if (parser.ParseInt(&v, pattrVal, attrValSz) && v >= 0)
                                                        {
                                                            imgInfo.Width = PixelsToTwips((UInt)v);
                                                        }
                                                    }
                                                    break;
                                                case GFxHTML_HEIGHT:
                                                    {
                                                        int v;
                                                        if (parser.ParseInt(&v, pattrVal, attrValSz) && v >= 0)
                                                        {
                                                            imgInfo.Height = PixelsToTwips((UInt)v);
                                                        }
                                                    }
                                                    break;
                                                case GFxHTML_VSPACE:
                                                    {
                                                        int v;
                                                        if (parser.ParseInt(&v, pattrVal, attrValSz))
                                                        {
                                                            imgInfo.VSpace = PixelsToTwips(v);
                                                        }
                                                    }
                                                    break;
                                                case GFxHTML_HSPACE:
                                                    {
                                                        int v;
                                                        if (parser.ParseInt(&v, pattrVal, attrValSz))
                                                        {
                                                            imgInfo.HSpace = PixelsToTwips(v);
                                                        }
                                                    }
                                                    break;
                                                default: break;
                                                }
                                            }
                                        }
                                    }
                                    GPtr<GFxTextHTMLImageTagDesc> ptextImageDesc = *new GFxTextHTMLImageTagDesc;
                                    imgCharFmt.SetImageDesc(ptextImageDesc);
                                    ptextImageDesc->ScreenWidth     = imgInfo.Width;
                                    ptextImageDesc->ScreenHeight    = imgInfo.Height;
                                    ptextImageDesc->VSpace          = imgInfo.VSpace;
                                    ptextImageDesc->HSpace          = imgInfo.HSpace;
                                    ptextImageDesc->Url             = imgInfo.Url;
                                    ptextImageDesc->Id              = imgInfo.Id;
                                    ptextImageDesc->Alignment       = imgInfo.Alignment;
                                    imgInfo.pTextImageDesc = ptextImageDesc;
                                    GFxTextFormat* ptextFmt;
                                    const GFxTextParagraphFormat* pparaFmt;
                                    if (prevStackIndex >= 0)
                                    {
                                        ptextFmt = &elementsStack[prevStackIndex].TextFmt;
                                        pparaFmt = &elementsStack[prevStackIndex].ParaFmt;
                                    }
                                    else
                                    {
                                        ptextFmt = &defaultTextFmt;
                                        pparaFmt = &defaultParaFmt;
                                    }
                                    AppendString(" ", 1, &imgCharFmt, pparaFmt);
                                    ptextFmt->SetImageDesc(NULL);
                                    SetDefaultTextFormat(*ptextFmt);
                                }
                            }
                            break;
                        default: break;
                        }
                    }
                    else
                    {
                        UPInt curPos = GetLength();
                        elementsStack.push_back(GFxSGMLStackElemDesc<Char>(elemName, elemLen, pelemDesc, 0));
                        GFxSGMLStackElemDesc<Char>& stackElem = elementsStack[elementsStack.size()-1];
                        stackElem.StartPos = curPos;
                        if (prevStackIndex >= 0)
                        {
                            stackElem.TextFmt = elementsStack[prevStackIndex].TextFmt;
                            stackElem.ParaFmt = elementsStack[prevStackIndex].ParaFmt;
                        }
                        else
                        {
                            stackElem.TextFmt = defaultTextFmt;
                            stackElem.ParaFmt = defaultParaFmt;
                        }

                        switch(pelemDesc->ElemId)
                        {
                        case GFxHTML_A:
                            const Char* pattrName;
                            UPInt      attrSz;
                            while(parser.GetNextAttribute(&pattrName, &attrSz))
                            {
                                const GFxSGMLElementDesc* pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                    (pattrName, attrSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                if (pattrDesc)
                                {
                                    const Char* pattrVal;
                                    size_t      attrValSz;                                        
                                    if (parser.GetNextAttributeValue(&pattrVal, &attrValSz))
                                    {
                                        switch(pattrDesc->ElemId)
                                        {
                                        case GFxHTML_HREF:
                                            stackElem.TextFmt.SetUrl(pattrVal, attrValSz);
                                            break;
                                        default: break;
                                        }
                                    }
                                }
                            }
                            break;
                        case GFxHTML_LI:
                            stackElem.ParaFmt.SetBullet();
                            pcurPara = GetLastParagraph();
                            if (pcurPara == NULL)
                                pcurPara = AppendNewParagraph(&stackElem.ParaFmt);
                            else if (pcurPara->GetLength() == 0)
                                pcurPara->SetFormat(stackElem.ParaFmt);
                            else
                            {
                                if (multiline)
                                    AppendString(NewLineStr(), 1);
                                pcurPara = GetLastParagraph();
                                pcurPara->SetFormat(stackElem.ParaFmt);
                            }
                            break;
                        case GFxHTML_P:
                            if ((pcurPara = GetLastParagraph()) == NULL)
                            {
                                // no paragraphs yet
                                pcurPara = AppendNewParagraph();
                            }
                            else if (p_elem_is_opened)
                            {
                                if (multiline)
                                    AppendString(NewLineStr(), 1);
                                pcurPara = GetLastParagraph();
                            }
                            if (pcurPara->GetLength() == 0)
                            {
                                // new paragraph
                                const Char* pattrName;
                                UPInt       attrSz;
                                while(parser.GetNextAttribute(&pattrName, &attrSz))
                                {
                                    const GFxSGMLElementDesc* pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                        (pattrName, attrSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                    if (pattrDesc)
                                    {
                                        const Char* pattrVal;
                                        UPInt       attrValSz;                                        
                                        if (parser.GetNextAttributeValue(&pattrVal, &attrValSz))
                                        {
                                            switch(pattrDesc->ElemId)
                                            {
                                            case GFxHTML_ALIGN:
                                                {
                                                    pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                                        (pattrVal, attrValSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                                    if (pattrDesc)
                                                    {
                                                        switch(pattrDesc->ElemId)
                                                        {
                                                        case GFxHTML_LEFT:
                                                            stackElem.ParaFmt.SetAlignment(GFxTextParagraphFormat::Align_Left);
                                                            break;
                                                        case GFxHTML_RIGHT:
                                                            stackElem.ParaFmt.SetAlignment(GFxTextParagraphFormat::Align_Right);
                                                            break;
                                                        case GFxHTML_CENTER:
                                                            stackElem.ParaFmt.SetAlignment(GFxTextParagraphFormat::Align_Center);
                                                            break;
                                                        case GFxHTML_JUSTIFY:
                                                            stackElem.ParaFmt.SetAlignment(GFxTextParagraphFormat::Align_Justify);
                                                            break;
                                                        default: break;
                                                        }
                                                    }
                                                }
                                                break;
                                            default: break;
                                            }
                                        }
                                    }
                                }
                                pcurPara->SetFormat(stackElem.ParaFmt);
                            }
                            ++p_elem_is_opened;
                            break;
                        case GFxHTML_B:
                            stackElem.TextFmt.SetBold(true);
                            break;
                        case GFxHTML_I:
                            stackElem.TextFmt.SetItalic(true);
                            break;
                        case GFxHTML_U:
                            stackElem.TextFmt.SetUnderline(true);
                            break;
                        case GFxHTML_FONT:
                            {
                                const Char* pattrName;
                                UPInt       attrSz;
                                while(parser.GetNextAttribute(&pattrName, &attrSz))
                                {
                                    const GFxSGMLElementDesc* pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                        (pattrName, attrSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                    if (pattrDesc)
                                    {
                                        const Char* pattrVal;
                                        UPInt       attrValSz;                                        
                                        if (parser.GetNextAttributeValue(&pattrVal, &attrValSz))
                                        {
                                            switch(pattrDesc->ElemId)
                                            {
                                            case GFxHTML_COLOR:
                                                // parse color: #RRGGBB
                                                if (*pattrVal == '#')
                                                {
                                                    UInt32 color;
                                                    if (parser.ParseHexInt(&color, pattrVal+1, attrValSz-1))
                                                    {
                                                        stackElem.TextFmt.SetColor(color);    
                                                    }
                                                }
                                                break;
                                            case GFxHTML_FACE:
                                                stackElem.TextFmt.SetFontName(pattrVal, attrValSz);
                                                break;
                                            case GFxHTML_SIZE:
                                                // parse size
                                                int sz;
                                                if (parser.ParseInt(&sz, pattrVal, attrValSz) && sz >= 0)
                                                {
                                                    stackElem.TextFmt.SetFontSize((UInt)sz);
                                                }
                                                break;
                                            case GFxHTML_KERNING:
                                                // parse kerning
                                                int kern;
                                                if (parser.ParseInt(&kern, pattrVal, attrValSz) && kern >= 0)
                                                {
                                                    if (kern)
                                                        stackElem.TextFmt.SetKerning();
                                                    else
                                                        stackElem.TextFmt.ClearKerning();
                                                }
                                                break;
                                            case GFxHTML_LETTERSPACING:
                                                // parse letterspacing
                                                Float ls;
                                                if (parser.ParseFloat(&ls, pattrVal, attrValSz))
                                                {
                                                    stackElem.TextFmt.SetLetterSpacing(ls);
                                                }
                                                break;
                                            // extension
                                            case GFxHTML_ALPHA:
                                                // parse alpha: #AA
                                                if (*pattrVal == '#')
                                                {
                                                    UInt32 alpha;
                                                    if (parser.ParseHexInt(&alpha, pattrVal+1, attrValSz-1))
                                                    {
                                                        stackElem.TextFmt.SetAlpha(UInt8(alpha));    
                                                    }
                                                }
                                                break;
                                            default: break;
                                            }
                                        }
                                    }
                                }
                            }

                            break;
                        case GFxHTML_TEXTFORMAT:
                            {
                                const Char* pattrName;
                                UPInt       attrSz;
                                while(parser.GetNextAttribute(&pattrName, &attrSz))
                                {
                                    const GFxSGMLElementDesc* pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                        (pattrName, attrSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                    if (pattrDesc)
                                    {
                                        const Char* pattrVal;
                                        UPInt       attrValSz;                                        
                                        if (parser.GetNextAttributeValue(&pattrVal, &attrValSz))
                                        {
                                            switch(pattrDesc->ElemId)
                                            {
                                            case GFxHTML_INDENT:
                                                {
                                                    // parse indent
                                                    int indent;
                                                    if (parser.ParseInt(&indent, pattrVal, attrValSz))
                                                    {
                                                        stackElem.ParaFmt.SetIndent(indent);
                                                    }
                                                    break;
                                                }
                                            case GFxHTML_BLOCKINDENT:
                                                {
                                                    // parse indent
                                                    int indent;
                                                    if (parser.ParseInt(&indent, pattrVal, attrValSz) && indent >= 0)
                                                    {
                                                        stackElem.ParaFmt.SetBlockIndent((UInt)indent);
                                                    }
                                                    break;
                                                }
                                            case GFxHTML_LEADING:
                                                {
                                                    // parse indent
                                                    int leading;
                                                    if (parser.ParseInt(&leading, pattrVal, attrValSz))
                                                    {
                                                        stackElem.ParaFmt.SetLeading(leading);
                                                    }
                                                    break;
                                                }
                                            case GFxHTML_LEFTMARGIN:
                                                {
                                                    // parse margin
                                                    int margin;
                                                    if (parser.ParseInt(&margin, pattrVal, attrValSz) && margin >= 0)
                                                    {
                                                        stackElem.ParaFmt.SetLeftMargin((UInt)margin);
                                                    }
                                                    break;
                                                }
                                            case GFxHTML_RIGHTMARGIN:
                                                {
                                                    // parse margin
                                                    int margin;
                                                    if (parser.ParseInt(&margin, pattrVal, attrValSz) && margin >= 0)
                                                    {
                                                        stackElem.ParaFmt.SetRightMargin((UInt)margin);
                                                    }
                                                    break;
                                                }
                                            default: break;
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        default: break;
                        }
                    }
                }
                else
                {
                    //elementsStack.push_back(GFxSGMLStackElemDesc<Char>(elemName, elemLen, NULL, GFC_MAX_UPINT));
                }
            }
            break;
        case SGMLPS_CONTENT:
            {
                const Char* content;
                UPInt  contentSize;
                if (parser.ParseContent(&content, &contentSize))
                {
                    if (contentSize > 0)
                    {
                        pcurPara = GetLastParagraph();
                        // do not add space at the beginning of paragraph, if condenseWhite
                        if (!condenseWhite || contentSize > 1 || content[0] != ' ' || pcurPara->GetLength() > 0)
                        {
                            if (elementsStack.size() > 0)
                            {
                                GFxSGMLStackElemDesc<Char>& stackElem = elementsStack[elementsStack.size()-1];
                                AppendString(content, contentSize, &stackElem.TextFmt, &stackElem.ParaFmt);
                            }
                            else
                            {
                                AppendString(content, contentSize);
                            }
                        }
                    }
                }
            }
            break;
        case SGMLPS_END_ELEMENT:
        case SGMLPS_EMPTY_ELEMENT_FINISHED:
            {
                const Char* elemName;
                UPInt  elemLen;

                UPInt startPos;
                const GFxSGMLStackElemDesc<Char>* pstackElem = NULL;
                if (type == SGMLPS_END_ELEMENT)
                {
                    if (elementsStack.size() > 0)
                        pstackElem = &elementsStack[elementsStack.size()-1];
                    parser.ParseEndElement(&elemName, &elemLen);
                }
                else
                {
                    // ignore end elements for BR & TAB
                    if (plastElemDesc)
                    {
                        if (plastElemDesc->EmptyElement)
                            break;
                    }
                    if (elementsStack.size() > 0)
                        pstackElem = &elementsStack[elementsStack.size()-1];
                    elemName = pstackElem->pElemName;
                    elemLen  = pstackElem->ElemNameSize;
                }
                if (pstackElem)
                {
                    // verify matching of end-element and start-element
                    if (parser.StrCompare(elemName, elemLen, pstackElem->pElemName, pstackElem->ElemNameSize) != 0)
                    {
                        // mismatch!
                        // just ignore such an element for now. Do we need to print
                        // kinda warning?
                        const GFxSGMLElementDesc* pelemDesc = GFxSGMLElementDesc::FindElem<Char>
                            (elemName, elemLen, elementsTable, sizeof(elementsTable)/sizeof(elementsTable[0]));
                        if (pelemDesc && p_elem_is_opened && (pelemDesc->ElemId == GFxHTML_P || pelemDesc->ElemId == GFxHTML_LI))
                        {
                            // looks like, if </p> end tag is used w/o closing all tags nested to it
                            // then it should close all opened tags till <p>
                            while(elementsStack.size() > 0)
                            {
                                const GFxSGMLStackElemDesc<Char>& stackElem = elementsStack[elementsStack.size()-1];
                                if (stackElem.pElemDesc && pelemDesc->ElemId == stackElem.pElemDesc->ElemId)
                                    break;
                                elementsStack.pop_back();
                            }
                            if (elementsStack.size() > 0)
                                pstackElem = &elementsStack[elementsStack.size()-1];
                            else
                                break;
                        }
                        else
                            break;
                    }
                    plastElemDesc = pstackElem->pElemDesc;
                    startPos = pstackElem->StartPos;
                    elementsStack.pop_back();

                    if (plastElemDesc)
                    {
                        switch(plastElemDesc->ElemId)
                        {
                        case GFxHTML_P:
                            --p_elem_is_opened;
                            // Empty <P/> element doesn't produce a new paragraph in Flash
                            if (type == SGMLPS_EMPTY_ELEMENT_FINISHED)
                                break;
                        case GFxHTML_LI:
                            if (multiline)
                                AppendString(NewLineStr(), 1);
                            break;
                        default: break;
                        }
                    }
                }
                else
                {
                    // wrong end elem, ignore...
                    break;
                }
            }
            break;
        default: break;
        }
    }
    return true;

}


//////////////////////////////////
// GFxTextDocView
//
GFxTextDocView::GFxTextDocView(GFxFontManager* pfontMgr) : 
    pFontManager(pfontMgr) 
{
    pDocument = *new DocumentText(this);
    FormatCounter = 1;
    Alignment = Align_Left;
    Flags = RTFlags = 0;
    ViewRect.Clear();
    pImageSubstitutor = NULL;

    SetAutoFit(); // on by default
    SetDefaultShadow();
}

GFxTextDocView::~GFxTextDocView()
{
    Close();
    ClearImageSubstitutor();
}


struct GFxImageSubstCmp
{
    struct Comparable
    {
        const wchar_t*  Str;
        UPInt           MaxSize;

        Comparable(const wchar_t* p, UPInt  sz):Str(p), MaxSize(sz) {}
    };
    struct Comparator
    {
        bool Insertion;
        Comparator(bool ins = false):Insertion(ins) {}
        
        int StrCompare(const wchar_t* dst, UPInt  dstlen, const wchar_t* src, UPInt  srclen) const
        {
            if (dstlen)
            {
                int f,l;
                int slen = (int)srclen, dlen = (int)dstlen;
                do {
                    f = (int)(*(dst++));
                    l = (int)(*(src++));
                } while (--dstlen && f && (f == l) && --srclen);

                if (f == l && srclen != 0 && (!Insertion || dstlen != 0))
                {
                    return dlen - slen;
                }
                return f - l;
            }
            else
                return 0-(int)srclen;
        }
        int Compare(const GFxTextDocView::ImageSubstitutor::Element& p1, const Comparable& p2) const
        {
            return -StrCompare(p2.Str, p2.MaxSize, p1.SubString, p1.SubStringLen);
        }
    };
};

void GFxTextDocView::ImageSubstitutor::AddImageDesc(const GFxTextDocView::ImageSubstitutor::Element& elem)
{
    if (FindImageDesc(elem.SubString, elem.SubStringLen))
        return; // already added
    /*printf("----------\n");
    for(UPInt i = 0; i < Elements.size(); ++i)
        printf("%S\n", Elements[i].SubString);
    printf("----------\n");*/

    static GFxImageSubstCmp::Comparator cmp(true);
    GFxImageSubstCmp::Comparable e(elem.SubString, elem.SubStringLen);
    SPInt i = GTL::find_insertion_index_in_sorted_array(Elements, e, cmp);
    GASSERT(i >= 0);
    Elements.insert((UPInt)i, elem);
}

GFxTextImageDesc* GFxTextDocView::ImageSubstitutor::FindImageDesc(const wchar_t* pstr, UPInt  maxlen, UPInt * ptextLen)
{
    static GFxImageSubstCmp::Comparator cmp;
    GFxImageSubstCmp::Comparable e(pstr, maxlen);
    SPInt i = GTL::find_index_in_sorted_array(Elements, e, cmp);
    if (i != -1)
    {
        if (ptextLen) *ptextLen = Elements[(UPInt)i].SubStringLen;
        return Elements[(UPInt)i].pImageDesc;
    }
    return NULL;
}

void GFxTextDocView::ImageSubstitutor::RemoveImageDesc(GFxTextImageDesc* pimgDesc)
{
    for(UPInt i = 0, n = Elements.size(); i < n;)
    {
        if (Elements[i].pImageDesc == pimgDesc)
            Elements.remove(i);
        else
            ++i;
    }
}

void GFxTextDocView::SetText(const char* putf8String, UPInt stringSize)
{
    pDocument->SetText(putf8String, stringSize);
    OnDocumentChanged(ViewNotify_SignificantTextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::SetText(const wchar_t* pstring, UPInt length)
{
    pDocument->SetText(pstring, length);
    OnDocumentChanged(ViewNotify_SignificantTextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::SetTextFormat(const GFxTextFormat& fmt, UPInt startPos, UPInt endPos)
{
    pDocument->SetTextFormat(fmt, startPos, endPos);
    OnDocumentChanged(ViewNotify_FormatChange);
}

void GFxTextDocView::SetParagraphFormat(const GFxTextParagraphFormat& fmt, UPInt startPos, UPInt endPos)
{
    pDocument->SetParagraphFormat(fmt, startPos, endPos);
    OnDocumentChanged(ViewNotify_FormatChange);
}

void GFxTextDocView::NotifyParagraphRemoving(const GFxTextParagraph& para)
{
    OnDocumentParagraphRemoving(para);
}

void GFxTextDocView::DocumentText::OnParagraphRemoving(const GFxTextParagraph& para)
{
    pDocument->NotifyParagraphRemoving(para);
}

void GFxTextDocView::DocumentText::OnParagraphTextInserted(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const wchar_t* ptextInserted)
{
    GUNUSED4(para, startPos, endPos, ptextInserted);
    pDocument->OnDocumentChanged(ViewNotify_TextChange);
}

void GFxTextDocView::DocumentText::OnParagraphTextRemoved(const GFxTextParagraph& para, UPInt removedPos, UPInt removedLen)
{
    GUNUSED3(para, removedPos, removedLen);
    pDocument->OnDocumentChanged(ViewNotify_TextChange);
}

void GFxTextDocView::EditCommand(GFxTextDocView::CommandType cmdId, const void* command)
{
    switch(cmdId)
    {
    case InsertChar:
        {
            GASSERT(command);
            const InsertCharCommand& cmd = *reinterpret_cast<const InsertCharCommand*>(command);
            pDocument->InsertString(&cmd.CharCode, cmd.PosAt, 1);
            break;
        }
    case DeleteChar:
        {
            GASSERT(command);
            const DeleteCharCommand& cmd = *reinterpret_cast<const DeleteCharCommand*>(command);
            pDocument->Remove(cmd.PosAt, cmd.PosAt + 1);
            break;
        }

    default: break;
    }
}

GFxString GFxTextDocView::GetText() const
{
    return pDocument->GetText();
}

GFxString GFxTextDocView::GetHtml() const
{
    return pDocument->GetHtml();
}

void GFxTextDocView::ParseHtml(const char* putf8Str, UPInt utf8Len, bool condenseWhite, GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr)
{
    pDocument->Clear();
    if (utf8Len == GFC_MAX_UPINT)
        utf8Len = gfc_strlen(putf8Str); // actually, this is the SIZE, not length. TODO
    pDocument->ParseHtml<char>(putf8Str, utf8Len, pimgInfoArr, IsMultiline(), condenseWhite);
    OnDocumentChanged(ViewNotify_SignificantTextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::ParseHtml(const wchar_t* pwStr, UPInt strLen, bool condenseWhite, GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr)
{
    pDocument->Clear();
    if (strLen == GFC_MAX_UPINT)
        strLen = wcslen(pwStr); 
    pDocument->ParseHtml<wchar_t>(pwStr, strLen, pimgInfoArr, IsMultiline(), condenseWhite);
    OnDocumentChanged(ViewNotify_SignificantTextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::AppendText(const wchar_t* pwStr, UPInt strLen)
{
    const GFxTextFormat* ptxtfmt;
    const GFxTextParagraphFormat* pparafmt;
    UPInt len = pDocument->GetLength();
    if (len > 0 && pDocument->GetTextAndParagraphFormat(&ptxtfmt, &pparafmt, len - 1))
        pDocument->AppendString(pwStr, strLen, ptxtfmt, pparafmt);
    else
        pDocument->AppendString(pwStr, strLen);
    OnDocumentChanged(ViewNotify_TextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::AppendText(const char* putf8Str, UPInt utf8Len)
{
    const GFxTextFormat* ptxtfmt;
    const GFxTextParagraphFormat* pparafmt;
    UPInt len = pDocument->GetLength();
    if (len > 0 && pDocument->GetTextAndParagraphFormat(&ptxtfmt, &pparafmt, len - 1))
        pDocument->AppendString(putf8Str, utf8Len, ptxtfmt, pparafmt);
    else
        pDocument->AppendString(putf8Str, utf8Len);
    OnDocumentChanged(ViewNotify_TextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::AppendHtml(const wchar_t* pwStr, UPInt strLen, bool condenseWhite)
{
    if (strLen == GFC_MAX_UPINT)
        strLen = wcslen(pwStr); 
    // at the moment, image tags are not supported by AppendHtml. TODO.
    pDocument->ParseHtml<wchar_t>(pwStr, strLen, NULL, IsMultiline(), condenseWhite);
    OnDocumentChanged(ViewNotify_TextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::AppendHtml(const char* putf8Str, UPInt utf8Len, bool condenseWhite)
{
    if (utf8Len == GFC_MAX_UPINT)
        utf8Len = gfc_strlen(putf8Str); // actually, this is the SIZE, not length. TODO
    // at the moment, image tags are not supported by AppendHtml. TODO.
    pDocument->ParseHtml<char>(putf8Str, utf8Len, NULL, IsMultiline(), condenseWhite);
    OnDocumentChanged(ViewNotify_TextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::InsertText(const wchar_t* pstr, UPInt startPos)
{
    pDocument->InsertString(pstr, startPos, GFC_MAX_UPINT);
}

void GFxTextDocView::ReplaceText(const wchar_t* pstr, UPInt startPos, UPInt endPos)
{
    pDocument->Remove(startPos, endPos);
    pDocument->InsertString(pstr, startPos, GFC_MAX_UPINT);
    //OnDocumentChanged(ViewNotify_TextChange);
}

void GFxTextDocView::RemoveText(UPInt startPos, UPInt endPos)
{
    pDocument->Remove(startPos, endPos);
    //OnDocumentChanged(ViewNotify_TextChange);
}

void GFxTextDocView::Close()
{
    if (pDocument)
    {
        pDocument = NULL;
        
        SetDocumentListener(NULL);
        pEditorKit = NULL;
        LineBuffer.ClearLines();
    }
}

void GFxTextDocView::SetViewRect(const GRectF& rect)
{
    if (rect != ViewRect)
    {
        ViewRect = rect;
        GRectF textRect = ViewRect;
        textRect.Contract(GFX_TEXT_GUTTER);
        LineBuffer.SetVisibleRect(textRect);
        if (IsWordWrap())
            SetCompleteReformatReq();
    }
}

/*void GFxTextDocView::OnDocumentChanging(UPInt startPos, UPInt endPos) 
{
    GFxStyledText* pstyledText = pDocument->GetStyledText();
    UPInt indexInPara;
    GFxStyledText::ParagraphsIterator paraIter = pstyledText->GetParagraphByIndex(startPos, &indexInPara);
    for(;!paraIter.IsFinished(); ++paraIter)
    {
        GPtr<GFxTextParagraph> ppara = paraIter->GetData();
        ChangedParagraphs.set(ppara);
    }
}*/

void GFxTextDocView::OnDocumentChanged(UInt notifyMask) 
{
    if (notifyMask & GFxTextDocView::ViewNotify_SignificantMask)
        SetCompleteReformatReq();
    else
        SetReformatReq();
    
    // reset scroll params if necessary
    if (notifyMask & GFxTextDocView::ViewNotify_ScrollingParamsChange)
    {
        SetHScrollOffset(0);
        SetVScrollOffset(0);
    }
//    if (pEditorKit)
//        pEditorKit->OnDocumentChanged(notifyMask);
}

void GFxTextDocView::OnDocumentParagraphRemoving(const GFxTextParagraph& para)
{
    // find lines for this paragraph in LineBuffer and mark them to be removed.
    GFxTextLineBuffer::Iterator linesIt = LineBuffer.Begin();
    for(bool found = false; !linesIt.IsFinished(); ++linesIt)
    {
        GFxTextLineBuffer::Line& curLine = *linesIt;
        if (para.GetId() == curLine.GetParagraphId())
        {
            found = true;
            curLine.SetTextPos(~0u); // indicator of line to be removed
        }
        else if (found)
            break;
    }
    SetReformatReq();
}

GFxFontHandle* GFxTextDocView::FindFont(FindFontInfo* pfontInfo)
{
    GASSERT(pfontInfo);
    GASSERT(pfontInfo->pCurrentFormat);

    // check if the font is same as in previous textFormat - don't need to check cache.
    if (!pfontInfo->pCurrentFont || !pfontInfo->pPrevFormat ||
        (!pfontInfo->pCurrentFormat->IsFontSame(*pfontInfo->pPrevFormat)))
    {
        // if font is not the same - check the cache first
        GPtr<GFxFontHandle>* ppcurrentFont = (pfontInfo->pFontCache) ?
            pfontInfo->pFontCache->get(pfontInfo->pCurrentFormat) : NULL; // no cache installed.
        if (ppcurrentFont)
            pfontInfo->pCurrentFont = *ppcurrentFont;
        else
        {
            if (pfontInfo->pCurrentFormat->IsFontHandleSet())
            {
                // font handle is set in TextFormat if font was referenced by id
                pfontInfo->pCurrentFont = pfontInfo->pCurrentFormat->GetFontHandle();
            }
            else
            {
                // if still not found - resolve the font via FontManager.
                GFxFontManager* pfontMgr   = GetFontManager();

                // font name always should be set in merged text style
                if (pfontInfo->pCurrentFormat->IsFontNameSet())
                {
                    pfontInfo->pCurrentFont = *pfontMgr->CreateFontHandle(pfontInfo->pCurrentFormat->GetFontName(), 
                                                                        pfontInfo->pCurrentFormat->IsBold(), 
                                                                        pfontInfo->pCurrentFormat->IsItalic(),
                                                                        DoesUseDeviceFont());
                }
                if (!pfontInfo->pCurrentFont)
                {
                    // font is not found! do default substitution here (TODO)
                    // if font is still not found, use a default font
                    pfontInfo->pCurrentFont = *pfontMgr->CreateAnyFontHandle("Arial");//??
                    GASSERT(pfontInfo->pCurrentFont);
                }
            }
            if (pfontInfo->pFontCache)
            {
                // add to cache
                pfontInfo->pFontCache->set(pfontInfo->pCurrentFormat, pfontInfo->pCurrentFont);
            }
        }
    }
    pfontInfo->pPrevFormat = pfontInfo->pCurrentFormat;
    return pfontInfo->pCurrentFont;
}

struct GFxLineCursor
{
    GFxTextLineBuffer::GlyphEntry*  pPrevGrec;                  // used for kerning
    GPtr<GFxFontHandle>         pLastFont;                      // used for kerning and line width correction
    UInt                        LastCharCode;                   // used for kerning

    UInt                        LastGlyphIndex;                 // used to correct line width
    Float                       LastAdvance;                    // used to correct line width
    SInt                        LastGlyphWidth;                 // used to correct line width
    UInt32                      LastColor;

    GFxTextDocView*             pDocView;
    const GFxTextParagraph*     pParagraph;
    SInt                        LineWidth;                      // accumulated line's width
    SInt                        LineWidthWithoutTrailingSpaces;
    UInt                        LineLength;                     // accumulated line's length in chars
    Float                       MaxFontAscent;
    Float                       MaxFontDescent;
    Float                       MaxFontLeading;
    GFxTextParagraph::CharactersIterator CharIter;
    GFxTextParagraph::CharacterInfo      CharInfoHolder;
    SInt                        Indent;
    SInt                        LeftMargin;
    SInt                        RightMargin;
    GFxTextLineBuffer::GlyphInserter    GlyphIns;
    UInt                        NumOfSpaces;
    UInt                        NumOfTrailingSpaces;
    bool                        LastKerning;
    bool                        LineHasNewLine;

    GFxLineCursor(): pPrevGrec(NULL), pLastFont(NULL), LastCharCode(0), 
        LastGlyphIndex(0), LastAdvance(0), LastGlyphWidth(0), LastColor(0), pDocView(NULL), pParagraph(NULL), 
        LineWidth(0), LineWidthWithoutTrailingSpaces(0), LineLength(0), 
        MaxFontAscent(0), MaxFontDescent(0), MaxFontLeading(0), Indent(0), 
        LeftMargin(0), RightMargin(0), NumOfSpaces(0), NumOfTrailingSpaces(0),
        LastKerning(false), LineHasNewLine(false)
    { }
    GFxLineCursor(GFxTextDocView* pview, const GFxTextParagraph* ppara, const GFxTextParagraph::CharactersIterator& chIter):
        pPrevGrec(NULL), pLastFont(NULL), LastCharCode(0), LastGlyphIndex(0), LastAdvance(0), 
        LastGlyphWidth(0), LastColor(0), pDocView(pview), pParagraph(ppara), LineWidth(0), 
        LineWidthWithoutTrailingSpaces(0), LineLength(0),
        MaxFontAscent(0), MaxFontDescent(0), MaxFontLeading(0),
        CharIter(chIter), Indent(0), LeftMargin(0), RightMargin(0), NumOfSpaces(0), 
        NumOfTrailingSpaces(0), LastKerning(false), 
        LineHasNewLine(false)
    { }

    bool IsFinished() const { return CharIter.IsFinished(); }
    void operator++() { ++CharIter; }
    void operator+=(UInt n) { CharIter += n; }
    const GFxTextParagraph::CharacterInfo& operator*() 
    { 
        if (pDocView->IsPasswordMode())
        {
            CharInfoHolder.Index = CharIter->Index;
            CharInfoHolder.Character = '*';
            CharInfoHolder.pFormat = CharIter->pFormat;
            return CharInfoHolder;
        }

        return *CharIter; 
    }

    Float GetPrevGlyphAdvanceAdj(wchar_t c) const 
    {
        if (pPrevGrec)
        {
            GASSERT(pLastFont);
            return pLastFont->GetFont()->GetKerningAdjustment(LastCharCode, c);
        }
        return 0;
    }
    void Reset()
    {
        pPrevGrec     = NULL;
        MaxFontAscent = MaxFontDescent = MaxFontLeading = 0;
        LastAdvance   = 0;
        LastKerning   = LineHasNewLine = false;
        LineWidthWithoutTrailingSpaces = 0;
        LineLength    = 0;
        NumOfSpaces   = NumOfTrailingSpaces = 0;
    }

    void TrackFontParams(GFxFontResource* pfont, Float scale)
    {
        Float ascent = pfont->GetAscent();
        Float descent = pfont->GetDescent();
        // if ascent is 0, then use 1024 instead.
        if (ascent == 0)
            ascent = 960.f;
        if (descent == 0)
            descent = 1024.f - 960.f;
        MaxFontAscent  = GTL::gmax(MaxFontAscent,  ascent * scale);
        MaxFontDescent = GTL::gmax(MaxFontDescent, descent * scale);
        MaxFontLeading = GTL::gmax(MaxFontLeading, pfont->GetLeading() * scale);
    }
};

void GFxTextDocView::FinalizeLine(GFxTextLineBuffer::Line* ptempLine, 
                                  FormatParagraphInfo* pinfo, 
                                  const GFxTextParagraphFormat& paraFormat, 
                                  GFxLineCursor& lineCursor)
{
    SInt lastAdvance = AlignTwipsToPixel(lineCursor.LastAdvance);
    if (lineCursor.pPrevGrec)
    {
        lineCursor.pPrevGrec->SetAdvance(lastAdvance);
    }

    // correct the line width by removing the advance and adding the real width of the last char
    if (lineCursor.pLastFont)
        lineCursor.LineWidth += lineCursor.LastGlyphWidth;
    else
        lineCursor.LineWidth += lastAdvance;

    if (lineCursor.pParagraph->GetLength() == 0) // was: pcurLine->GetNumGlyphs() == 0
    {
        // a special case when paragraph is empty.
        // in this case we need to calculate at least the height of the line.
        // To do this, we need to have MaxFontAscent & Descent set correctly.
        FindFontInfo findFontInfo(pinfo->pFontCache);

        findFontInfo.pCurrentFormat = lineCursor.pDocView->GetDefaultTextFormat();
        GASSERT(findFontInfo.pCurrentFormat);
        GPtr<GFxFontHandle> pfontHandle = lineCursor.pDocView->FindFont(&findFontInfo);
        GASSERT(pfontHandle);

        Float scale = PixelsToTwips(findFontInfo.pCurrentFormat->GetFontSize()) / 1024.0f; // the EM square is 1024 x 1024   
        lineCursor.TrackFontParams(pfontHandle->GetFont(), scale);
    }

    Float fleading   = (paraFormat.IsLeadingSet()) ? PixelsToTwips(paraFormat.GetLeading()) : lineCursor.MaxFontLeading; 
    //SInt lineHeight = RoundTwips(lineCursor.MaxFontAscent + lineCursor.MaxFontDescent + fleading);
    SInt lineHeight = RoundTwips(lineCursor.MaxFontAscent + lineCursor.MaxFontDescent + 0);
    SInt leading = RoundTwips(fleading);

    GFxTextLineBuffer::Line* pcurLine;
    // so, here we need to allocate a "real" (non temporary) line and transfer
    // all data to it.
    // First, we need to decide, can we allocate Line8 or not...
    // The criteria for Line8 are as follows:
    // Dimensions = [-32768..32767] (i.e. -1638.4 .. 1638.35 pts), 
    // TextLen <= 255, GlyphsCount <= 255, Leading <= 255 (i.e. 12.75 pt), TextPos < 2^24
    UInt glyphsCount         = lineCursor.GlyphIns.GetGlyphIndex();
    UInt formatDataElemCount = lineCursor.GlyphIns.GetFormatDataElementsCount();
    if (lineCursor.LineLength <= 255 && glyphsCount <= 255 && (leading >= -256 && leading <= 255) &&
        (lineHeight >= -32768 && lineHeight < 32768) && 
        (lineCursor.LineWidth >= -32768 && lineCursor.LineWidth < 32768))
    {
        // convert line to short form
        pcurLine = pinfo->pLinesIter->InsertNewLine(glyphsCount, formatDataElemCount, GFxTextLineBuffer::Line8);
    }
    else
    {
        pcurLine = pinfo->pLinesIter->InsertNewLine(glyphsCount, formatDataElemCount, GFxTextLineBuffer::Line32);
    }
    pcurLine->SetParagraphId(lineCursor.pParagraph->GetId());
    pcurLine->SetParagraphModId(lineCursor.pParagraph->GetModCounter());

    // copy glyph and format data
    memcpy(pcurLine->GetGlyphs(), ptempLine->GetGlyphs(), 
        glyphsCount*sizeof(GFxTextLineBuffer::GlyphEntry));
    memcpy(pcurLine->GetFormatData(), ptempLine->GetFormatData(), 
        formatDataElemCount*sizeof(GFxTextLineBuffer::GFxFormatDataEntry));

    pcurLine->SetTextPos(ptempLine->GetTextPos());
    pcurLine->SetTextLength(lineCursor.LineLength);

    pcurLine->SetBaseLineOffset(lineCursor.MaxFontAscent);

    // check for justify.
    // do not do justification for the last line in para, or for the empty one
    if (!lineCursor.LineHasNewLine && IsWordWrap() && 
        lineCursor.NumOfSpaces - lineCursor.NumOfTrailingSpaces > 0 && paraFormat.IsJustifyAlignment())
    {
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            // do one more pass over the line and set appropriate advance values for spaces
        // to accomplish justification. 
        SInt deltaW = (SInt(GetTextRect().Width()) - lineCursor.RightMargin) - 
            (lineCursor.LineWidthWithoutTrailingSpaces + lineCursor.Indent + lineCursor.LeftMargin);
        if (deltaW > 0) //? could it be < 0?
        {
            SInt advDelta = deltaW / (lineCursor.NumOfSpaces - lineCursor.NumOfTrailingSpaces);

            GFxTextLineBuffer::GlyphIterator git = pcurLine->Begin();
            for(; !git.IsFinished(); ++git)
            {
                GFxTextLineBuffer::GlyphEntry& ge = git.GetGlyph();
                if (ge.IsCharInvisible() && !ge.IsNewLineChar() && ge.GetLength() > 0)
                    ge.SetAdvance(ge.GetAdvance() + advDelta);
            }
            lineCursor.LineWidth += deltaW;
        }
    }

    // calculate the bounds of the line
    SInt lineOffsetX = lineCursor.Indent + lineCursor.LeftMargin;
    pcurLine->SetOffset(GPointF((Float)lineOffsetX, pinfo->NextOffsetY));
    pcurLine->SetDimensions(lineCursor.LineWidth, lineHeight);
    pcurLine->SetLeading(leading);

    switch(paraFormat.GetAlignment())
    {
    case GFxTextParagraphFormat::Align_Right:   
        pcurLine->SetAlignment(GFxTextLineBuffer::Line::Align_Right); 

        // align line accordingly to ViewRect
        GASSERT(lineCursor.pDocView);
        pcurLine->SetOffsetX(GTL::gmax(0.f, lineCursor.pDocView->GetTextRect().Width() - pcurLine->GetWidth()));

        if (lineCursor.pDocView->IsAutoSizeX())
        {
            pinfo->NeedRecenterLines = true;
        }
        break;
    case GFxTextParagraphFormat::Align_Center:  
        pcurLine->SetAlignment(GFxTextLineBuffer::Line::Align_Center); 

        // align line accordingly to ViewRect
        GASSERT(lineCursor.pDocView);
        pcurLine->SetOffsetX (GTL::gmax(0.f, lineCursor.pDocView->GetTextRect().Width() / 2 - pcurLine->GetWidth() / 2));

        if (lineCursor.pDocView->IsAutoSizeX())
        {
            pinfo->NeedRecenterLines = true;
        }
        break;
    default:                                    
        pcurLine->SetAlignment(GFxTextLineBuffer::Line::Align_Left); 
        break;
    }

    pinfo->ParaWidth    = GTL::gmax(pinfo->ParaWidth, lineOffsetX + lineCursor.LineWidth);
    pinfo->ParaHeight   = SInt(pinfo->NextOffsetY + lineHeight - pinfo->ParaYOffset);
    pinfo->NextOffsetY  += lineHeight + leading;
    lineCursor.Indent   = 0; // reset indent
    lineCursor.GlyphIns.Reset();
    lineCursor.LastColor = 0;
    lineCursor.pLastFont = NULL;
}

// Format the paragraph into temporary line buffer
void GFxTextDocView::FormatParagraph(const GFxTextParagraph& paragraph, FormatParagraphInfo* pinfo)
{
    FindFontInfo findFontInfo(pinfo->pFontCache);
    UInt currentLineNum = 0;

    GASSERT(paragraph.GetFormat());
    const GFxTextParagraphFormat& paraFormat = *paragraph.GetFormat();
    GFxLineCursor lineCursor(this, &paragraph, pinfo->CharIter), wordWrapPoint;

    // try to estimate how much memory would be necessary for temporary line
    // Assume, glyphCount <= paragraph.GetLength()
    //         fmtDataElemCount <= paragraph.GetLength()*2*paragraph.GetLength()
    UPInt maxGlyphCount = paragraph.GetLength();
    if (paraFormat.IsBullet())
        ++maxGlyphCount;
    UInt linesz = GFxTextLineBuffer::CalcLineSize((UInt)maxGlyphCount, (UInt)maxGlyphCount*2, GFxTextLineBuffer::Line32);
    GFxTextLineBuffer::Line* ptempLine = NULL;
    if (linesz < sizeof(pinfo->TempLineBuff))
    {
        ptempLine = reinterpret_cast<GFxTextLineBuffer::Line*>(pinfo->TempLineBuff);
        ptempLine->SetMemSize(linesz);
    }
    else if (pinfo->pDynLine)
    {
        if (linesz >= pinfo->pDynLine->GetMemSize())
        {
            LineBuffer.LineAllocator.FreeLine(pinfo->pDynLine);
            pinfo->pDynLine = LineBuffer.LineAllocator.AllocLine(linesz + GFX_LINE_ALLOC_DELTA, GFxTextLineBuffer::Line32);
        }
        ptempLine = pinfo->pDynLine;
    }
    else
        ptempLine = pinfo->pDynLine = LineBuffer.LineAllocator.AllocLine(linesz + GFX_LINE_ALLOC_DELTA, GFxTextLineBuffer::Line32);
    ptempLine->InitLine32();
    ptempLine->ClearInitialized(); // to avoid tempLine->Release to be invoked
    ptempLine->SetNumGlyphs((UInt)maxGlyphCount);

    GASSERT(ptempLine);
    if (CanBeEditable())
        ptempLine->SetTextPos(UInt(paragraph.GetStartIndex()));
    else
        ptempLine->SetTextPos(0);

    lineCursor.GlyphIns = GFxTextLineBuffer::GlyphInserter(ptempLine->GetGlyphs(), ptempLine->GetNumGlyphs(), ptempLine->GetFormatData());

    pinfo->ParaHeight  = pinfo->ParaWidth = 0;
    pinfo->ParaLines   = 0;

    GFxTextParagraph::CharactersIterator savedCharIter;
    GRectF glyphBounds;

    if (paraFormat.IsBullet())
    {
        // add bullet symbol.
        GFxTextLineBuffer::GlyphEntry& grec = lineCursor.GlyphIns.GetGlyph();

        const GFxTextParagraph::CharacterInfo& chInfo = *lineCursor;
        findFontInfo.pCurrentFormat = chInfo.pFormat;
        GPtr<GFxFontHandle> pfontHandle = FindFont(&findFontInfo);
        GASSERT(pfontHandle);

        int glyphIndex = pfontHandle->GetFont()->GetGlyphIndex(GFX_BULLET_CHAR_CODE);
        grec.SetIndex(glyphIndex);
        grec.SetAdvance(PixelsToTwips(GFX_BULLET_CHAR_ADVANCE));
        UInt fontSize = findFontInfo.pCurrentFormat->GetFontSize()*2/3;
        grec.SetFontSize(fontSize);
        grec.SetLength(0);

        UInt32 color = findFontInfo.pCurrentFormat->GetColor();
        lineCursor.GlyphIns.AddFont(pfontHandle);
        lineCursor.GlyphIns.AddColor(color);
        ++lineCursor.GlyphIns;

        lineCursor.pLastFont    = pfontHandle;
        lineCursor.LastColor    = color;
        lineCursor.Indent       = PixelsToTwips(GFX_BULLET_PARA_INDENT);
        lineCursor.LineWidth    = PixelsToTwips(GFX_BULLET_CHAR_ADVANCE);
    }
    else
        lineCursor.Indent = PixelsToTwips(paraFormat.GetIndent());
    lineCursor.LeftMargin  = PixelsToTwips(paraFormat.GetLeftMargin());
    lineCursor.RightMargin = PixelsToTwips(paraFormat.GetRightMargin());

    UInt deltaText = 1;
    for(;!lineCursor.IsFinished(); lineCursor += deltaText)
    {
        const GFxTextParagraph::CharacterInfo& chInfo = *lineCursor;
        deltaText = 1;
        
        GPtr<GFxTextImageDesc> pimgDesc = NULL;
        if (pImageSubstitutor)
        {
            UPInt  remlen;
            const wchar_t* premainingStr = lineCursor.CharIter.GetRemainingTextPtr(&remlen);
            UPInt  textLen;
            pimgDesc = pImageSubstitutor->FindImageDesc(premainingStr, remlen, &textLen);
            if (pimgDesc)
            {
                deltaText = (UInt)textLen;
            }
        }
        else
        {
            GASSERT(chInfo.pFormat);
            if (chInfo.pFormat->IsImageDescSet())
            {
                pimgDesc = chInfo.pFormat->GetImageDesc();
            }
        }

        if (IsWordWrap() && !pimgDesc)
        {
            if (chInfo.Character == '-' || iswspace(chInfo.Character)) // '-' also is a "post-word" separator
            {
                // '-' shouldn't be omitted! So, save an iterator here
                savedCharIter = lineCursor.CharIter;
            }
        }
        GFxFontResource* pfont = NULL;
        GPtr<GFxFontHandle> pfontHandle;
        Float   scale = 1.f;
        int     glyphIndex;
        Float   glyphAdvance;
        SInt    lastAdvance;
        SInt    glyphWidth;
        SInt    adjLineWidth;
        bool    isSpace;
        if (!pimgDesc)
        {
            if (findFontInfo.pCurrentFont && findFontInfo.pCurrentFormat == chInfo.pFormat) // is the format ptr same as previous one?
            {
                // if yes, just use findFontInfo.CurrentFormat, since the default text format is same.
                pfontHandle = findFontInfo.pCurrentFont;
            }
            else
            {
                findFontInfo.pCurrentFormat = chInfo.pFormat;
                
                pfontHandle = FindFont(&findFontInfo);
                GASSERT(pfontHandle);
                if (!pfontHandle) 
                    continue;
            }
            pfont = pfontHandle->GetFont();
            scale = PixelsToTwips(findFontInfo.pCurrentFormat->GetFontSize()) / 1024.0f; // the EM square is 1024 x 1024   
            if (chInfo.Character == NewLineChar())
            {
                // ok, the end of the paragraph and the line. Add a fake glyph with 
                // half of space's width to the buffer
                glyphIndex = pfont->GetGlyphIndex(' ');
                glyphAdvance = pfont->GetAdvance(glyphIndex) / 2 * scale;
            }
            else
            {
                glyphIndex = pfont->GetGlyphIndex((UInt16)chInfo.Character);
                if (glyphIndex == -1)
                {
                    // error -- missing glyph!
                    if (pinfo->pLog)
                    {
                        // Log an error, but don't log too many times.
                        static int  LogCount = 0;
                        if (LogCount < 10)
                        {
                            LogCount++;
                            pinfo->pLog->LogError("GFxTextDocView::Format() - missing glyph %d. "
                                "Make sure that SWF file includes character shapes for \"%s\" font.\n",
                                chInfo.Character,
                                pfont->GetName());
                        }

                        // Drop through and use index == -1; this will display
                        // using the empty-box glyph
                    }
                }

                glyphAdvance = pfont->GetAdvance(glyphIndex) * scale;
            }

            // if kerning for previous glyph was on - modify its advance and the width of line
            if (lineCursor.pPrevGrec)
            {
                Float prevGlyphAdvanceAdj = 0;
                if (lineCursor.LastKerning)
                    prevGlyphAdvanceAdj = lineCursor.GetPrevGlyphAdvanceAdj(chInfo.Character) * scale;
                //lastAdvance = AlignTwipsToPixel(lineCursor.LastAdvance + prevGlyphAdvanceAdj);
                lastAdvance = AlignTwipsToPixel(lineCursor.LastAdvance + prevGlyphAdvanceAdj);
            }
            else
            {
                //lastAdvance = AlignTwipsToPixel(lineCursor.LastAdvance);
                lastAdvance = AlignTwipsToPixel(lineCursor.LastAdvance);
            }
            glyphWidth   = AlignTwipsToPixel(GTL::gmax(
                pfont->GetGlyphBounds(glyphIndex, &glyphBounds).Right * scale + 20, glyphAdvance));

            // the space shouldn't be taken into account when checking the lineWidth for
            // intersection with viewrect. Though, the space's width still should be added
            // to the lineWidth, even if it is a "word-wrapping" point.
            isSpace = (iswspace(chInfo.Character) != 0);
            adjLineWidth = (isSpace) ? 0 : glyphWidth;
        }
        else
        {
            findFontInfo.pCurrentFormat = chInfo.pFormat;
            glyphWidth      = AlignTwipsToPixel(pimgDesc->GetScreenWidth());
            glyphAdvance    = glyphWidth + 40.f;
            lastAdvance     = AlignTwipsToPixel(lineCursor.LastAdvance);
            adjLineWidth    = glyphWidth;
            glyphIndex      = ~0u;
            isSpace         = false;
        }

        SInt newLineWidth = lineCursor.LineWidth + lastAdvance;

        // Do word-wrapping?
        // use a real width of last glyph to test for intersection with viewrect
        if (IsWordWrap() && !isSpace && 
            newLineWidth + adjLineWidth + lineCursor.Indent + lineCursor.LeftMargin > 
            GetTextRect().Width() - lineCursor.RightMargin)
        {
            // create a new line by incrementing 'currentLineNum'
            ++currentLineNum;
            newLineWidth = 0;
            
            bool wordWrap = false;
            UPInt newTextPos;

            // if wordWrap - cut the last word
            if (!wordWrapPoint.IsFinished())
            {
                GFxTextLineBuffer::GlyphInserter ins = wordWrapPoint.GlyphIns;
                lineCursor.GlyphIns.ResetTo(ins);

                lineCursor = wordWrapPoint;
                wordWrapPoint = GFxLineCursor();
                wordWrap = true;
                newTextPos = paragraph.GetStartIndex() + (*lineCursor).Index + 1;
            }
            else
                newTextPos = paragraph.GetStartIndex() + (*lineCursor).Index;

            // make the separator char at the end of the previous line zero-length
            // Though, if wordwrapping happened not at the separator...
            if (lineCursor.pPrevGrec)
            {
                lineCursor.pPrevGrec->SetLength(0);
            }

            // finalize the line
            FinalizeLine(ptempLine, pinfo, paraFormat, lineCursor);

            // reset values for the next line
            lineCursor.Reset();

            //!!ptempLine = GFxTextLineBuffer::GetLine(currentLineNum, *pinfo->pLines, &LineBuffer.GlyphsStorage);
            GASSERT(ptempLine);
            
            //InitLine(this, ptempLine, paragraph);
            if (CanBeEditable())
                ptempLine->SetTextPos((UInt)newTextPos);
            else
                ptempLine->SetTextPos(0);

            if (wordWrap)
            {
                lineCursor.LineWidth = 0;
                continue;
            }
        }
        if (lineCursor.pPrevGrec)
        {
            lineCursor.pPrevGrec->SetAdvance(lastAdvance);
        }

        GFxTextLineBuffer::GlyphEntry& grec = lineCursor.GlyphIns.GetGlyph();
        grec.SetIndex(glyphIndex);
        // Advance will be set at the next iteration or in Finalize, taking
        // kerning into account.
        //grec.SetAdvance(glyphAdvance);

        if (pimgDesc)
        {
            lineCursor.GlyphIns.AddImage(pimgDesc);
            grec.SetLength(deltaText);
            lineCursor.LastKerning = false;

            GPointF p(0,0);
            p = pimgDesc->Matrix.Transform(p);
            lineCursor.MaxFontAscent  = GTL::gmax(lineCursor.MaxFontAscent,  -p.y);
            lineCursor.MaxFontDescent = GTL::gmax(lineCursor.MaxFontDescent, pimgDesc->GetScreenHeight() - -p.y);
        }
        else
        {
            UInt32 color  = findFontInfo.pCurrentFormat->GetColor();
            UInt fontSize = findFontInfo.pCurrentFormat->GetFontSize();
            grec.SetFontSize(fontSize);

            if (pfontHandle != lineCursor.pLastFont)
                lineCursor.GlyphIns.AddFont(pfontHandle);
            if (color != lineCursor.LastColor)
                lineCursor.GlyphIns.AddColor(color);

            if (chInfo.Character == NewLineChar())
            {
                // special symbol at the end of paragraph. Set its length to 0.
                // it also shouldn't participate in textWidth's calculation.
                // but it should be taken into account when drawing background 
                // highlighting.
                grec.SetLength(0);
                grec.SetInvisibleChar();
                grec.SetNewLineChar();
                glyphWidth = 0;
                lineCursor.LineHasNewLine = true;
            }
            else
            {
                if (iswspace(chInfo.Character))
                {
                    grec.SetInvisibleChar();
                    ++lineCursor.NumOfSpaces;
                    ++lineCursor.NumOfTrailingSpaces;
                }
                else
                {
                    lineCursor.NumOfTrailingSpaces = 0;
                    lineCursor.LineWidthWithoutTrailingSpaces = newLineWidth + adjLineWidth;
                }
                grec.SetLength(1);
            }
            if (findFontInfo.pCurrentFormat->IsUnderline())
                grec.SetUnderline();
            else
                grec.ClearUnderline();
            if (findFontInfo.pCurrentFormat->IsUrlSet())
                grec.SetUrl();
            else
                grec.ClearUrl();

            // gather dimensions for the line
            lineCursor.TrackFontParams(pfont, scale);

            lineCursor.pLastFont        = pfontHandle;
            lineCursor.LastColor        = color;
            lineCursor.LastKerning      = findFontInfo.pCurrentFormat->IsKerning();
        }

        lineCursor.LineLength += grec.GetLength();

        lineCursor.pPrevGrec        = &grec;
        lineCursor.LastCharCode     = chInfo.Character;
        lineCursor.LineWidth        = newLineWidth;

        lineCursor.LastGlyphIndex   = glyphIndex;
        lineCursor.LastAdvance      = glyphAdvance;
        if (findFontInfo.pCurrentFormat->IsLetterSpacingSet())
            lineCursor.LastAdvance  += PixelsToTwips(findFontInfo.pCurrentFormat->GetLetterSpacing());
        lineCursor.LastGlyphWidth   = glyphWidth;

        ++lineCursor.GlyphIns;
        if (IsWordWrap() && !pimgDesc)
        {
            if (chInfo.Character == '-' || iswspace(chInfo.Character)) // also a word separator
            {
                wordWrapPoint = lineCursor;
                wordWrapPoint.CharIter = savedCharIter;
                //lastSpaceGlyphInserter = lineCursor.GlyphIns;
            }
        }
    }

    if (ptempLine)
    {
        // finalize the last line
        FinalizeLine(ptempLine, pinfo, paraFormat, lineCursor);
    }
}

void GFxTextDocView::Format(GFxLog* plog)
{
    GFxStyledText* pstyledText = GetStyledText();
    FontCache fontCache;

    GFxStyledText::ParagraphsIterator paraIter = pstyledText->GetParagraphIterator();
    GFxTextLineBuffer::Iterator linesIt = LineBuffer.Begin();

    //LineBuffer.InvalidateCache(); //??AB: should it happen unconditionally (this will cause batch rebuild)?
    FormatParagraphInfo paraInfo;
    paraInfo.pFontCache         = &fontCache;
    paraInfo.ParaYOffset        = 0;
    paraInfo.NextOffsetY        = 0;
    paraInfo.NeedRecenterLines  = false;
    paraInfo.pLog               = plog;
    SInt textWidth = 0, textHeight = 0;

    while(!paraIter.IsFinished())
    {
        const GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        if (!linesIt.IsFinished())
        {
            GFxTextLineBuffer::Line& curLine = *linesIt;

            if (!IsCompleteReformatReq() && ppara->GetId() == curLine.GetParagraphId() && ppara->GetModCounter() == curLine.GetParagraphModId())
            {
                // if paragraph is not changed, then just update yoffset & textpos of each line
                UPInt pos = ppara->GetStartIndex();
                for(; !linesIt.IsFinished(); ++linesIt)
                {
                    GFxTextLineBuffer::Line& curLine = *linesIt;
                    if (ppara->GetId() != curLine.GetParagraphId())
                        break;
                    curLine.SetTextPos((UInt)pos);
                    SInt oldX = (SInt)curLine.GetOffsetX();
                    SInt oldY = (SInt)curLine.GetOffsetY();

                    pos += curLine.GetTextLength();
                    curLine.SetOffsetY(paraInfo.NextOffsetY);

                    textWidth  =  GTL::gmax(textWidth, curLine.GetWidth());
                    textHeight =  SInt(paraInfo.NextOffsetY + curLine.GetHeight());

                    paraInfo.NextOffsetY += curLine.GetHeight() + curLine.GetLeading();

                    // recenter line, if necessary
                    if (IsAutoSizeX())
                    {
                        switch(curLine.GetAlignment())
                        {
                        case GFxTextLineBuffer::Line::Align_Right:
                            curLine.SetOffsetX(GTL::gmax(0.f, GetTextRect().Width() - curLine.GetWidth()));
                            
                            // still need to set NeedRecenterLines to true, in the case if view rect width will be changed
                            paraInfo.NeedRecenterLines = true; 
                            break;
                        case GFxTextLineBuffer::Line::Align_Center:
                            curLine.SetOffsetX(GTL::gmax(0.f, GetTextRect().Width() / 2 - curLine.GetWidth() / 2));

                            // still need to set NeedRecenterLines to true, in the case if view rect width will be changed
                            paraInfo.NeedRecenterLines = true;
                            break;

                        default: break;
                        }
                    }
                    if (oldX != (SInt)curLine.GetOffsetX() || oldY != (SInt)curLine.GetOffsetY())
                        LineBuffer.InvalidateCache(); //??AB: should it happen unconditionally (this will cause batch rebuild)?
                }
                ++paraIter;
                continue;
            }
            else // para was changed, removed or inserted
            {
                bool paraWasRemoved = (curLine.GetTextPos() == ~0u);
                // do we need to remove lines for this paragraph?
                if (paraWasRemoved) // para was removed
                {
                    UInt l = 0;
                    UInt32 paraId = curLine.GetParagraphId();
                    for(GFxTextLineBuffer::Iterator it = linesIt; !it.IsFinished(); ++l, ++it)
                    {
                        GFxTextLineBuffer::Line& curLine = *it;
                        if (curLine.GetParagraphId() != paraId)
                            break;
                    }
                    if (l > 0)
                    {
                        linesIt.Remove(l);
                        LineBuffer.InvalidateCache(); // force batching to be rebuilt
                    }
                    // was the paragraph removed completely? don't do formatting then
                    continue;
                }
                else if ((ppara->GetId() == curLine.GetParagraphId() && ppara->GetModCounter() != curLine.GetParagraphModId())) // para was changed
                {
                    // remove all lines by one call
                    UInt l = 0;
                    for(GFxTextLineBuffer::Iterator it = linesIt; !it.IsFinished(); ++l, ++it)
                    {
                        GFxTextLineBuffer::Line& curLine = *it;
                        if (curLine.GetParagraphId() != ppara->GetId())
                            break;
                    }
                    linesIt.Remove(l);
                    LineBuffer.InvalidateCache(); // force batching to be rebuilt
                }
            }
        }
        // else, no lines to modify - just add formatting for the paragraph

        // format paragraph
        paraInfo.pLinesIter     = &linesIt;
        paraInfo.CharIter       = ppara->GetCharactersIterator(0);
        paraInfo.ParaYOffset    = paraInfo.NextOffsetY;
        FormatParagraph(*ppara, &paraInfo);
        LineBuffer.InvalidateCache(); // force batching to be rebuilt

        textWidth  =  GTL::gmax(textWidth, paraInfo.ParaWidth);
        textHeight = SInt(paraInfo.ParaYOffset + paraInfo.ParaHeight);

        ++paraIter;
    }
    // now need to remove all remaining lines in line buffer
    while(!linesIt.IsFinished())
    {
        linesIt.Remove();
    }

    // assign new values for TextWidth & TextHeight
    GASSERT(textWidth >= 0 && textHeight >= 0);
    TextWidth = (UInt32)textWidth;
    TextHeight = (UInt32)textHeight;

    //    LineBuffer.Dump();

    ++FormatCounter; // force textWidth, etc format dependent values to be recalculated

    ClearReformatReq();

    // handle autosize
    if (IsAutoSizeX() || IsAutoSizeY())
    {
        GRectF viewRect = GetViewRect();
        GRectF newViewRect = viewRect;
        if (IsAutoSizeX())
        {
            Float newW = GetTextWidth() + GFX_TEXT_GUTTER*2;

            switch(Alignment)
            {
            case Align_Center:
                newViewRect.Left += newViewRect.Width()/2 - newW/2;
                break;
            case Align_Right:
                newViewRect.Left = newViewRect.Right - newW;
                break;

            default: break;
            }
            newViewRect.SetWidth(newW);
        }
        if (IsAutoSizeY())
        {
            Float newH = GetTextHeight() + GFX_TEXT_GUTTER*2;
            newViewRect.SetHeight(newH);
        }
        SetViewRect(newViewRect);

        if (paraInfo.NeedRecenterLines && newViewRect != viewRect)
        {
            // do we need to recenter lines? This might be required if AutoSizeX is on
            // and the width of view rect was changed because of formatting.
            GFxTextLineBuffer::Iterator linesIt = LineBuffer.Begin();
            for(; !linesIt.IsFinished(); ++linesIt)
            {
                GFxTextLineBuffer::Line& curLine = *linesIt;
                switch(curLine.GetAlignment())
                {
                case GFxTextLineBuffer::Line::Align_Right:
                    curLine.SetOffsetX(GTL::gmax(0.f, GetTextRect().Width() - curLine.GetWidth()));
                    break;
                case GFxTextLineBuffer::Line::Align_Center:
                    curLine.SetOffsetX(GTL::gmax(0.f, GetTextRect().Width() / 2 - curLine.GetWidth() / 2));
                    break;

                default: break;
                }
            }
        }
    }

    // also, we need to correct scrolling values, if they are out of range after re-formatting
    UInt maxHScroll = GetMaxHScroll();
    if(GetHScrollOffset() > maxHScroll)
    {
        SetHScrollOffset(maxHScroll);
    }
    UInt maxVScroll = GetMaxVScroll();
    if(GetVScrollOffset() > maxVScroll)
    {
        SetVScrollOffset(maxVScroll);
    }
    LineBuffer.CheckIntegrity();
}

void    GFxTextDocView::Display(GFxDisplayContext &context,
                             const Matrix& mat, const Cxform& cx,
                             bool nextFrame,
                             GFxLog* plog)
{
    if (IsReformatReq())
    {
        Format(plog);
    }

    // TO DO: Review and copy shadow and blur
    GFxTextFieldParam param;
    param.LoadFromTextFilter(Filter);

    if (IsAAForReadability())
    {
        param.TextParam.SetOptRead(true);
        param.ShadowParam.SetOptRead(true);
    }
    if (IsAutoFit())
    {
        param.TextParam.SetAutoFit(true);
        param.ShadowParam.SetAutoFit(true);
    }

    LineBuffer.Display(context, mat, cx, nextFrame, param);
}

bool    GFxTextDocView::SetHScrollOffset(UInt hscroll)
{
    bool rv = false;
    if (hscroll <= GetMaxHScroll() || hscroll < GetHScrollOffset())
    {
        if (LineBuffer.GetHScrollOffset() != hscroll)
        {
            LineBuffer.SetHScrollOffset(hscroll);
            if (GetDocumentListener())
            {
                GetDocumentListener()->View_OnHScroll(*this, hscroll);
            }
            rv = true;
        }
    }
    return rv;
}

bool    GFxTextDocView::SetVScrollOffset(UInt vscroll)
{
    bool rv = false;
    if (vscroll <= GetMaxVScroll())
    {
        if (LineBuffer.GetFirstVisibleLineIndex() != vscroll)
        {
            LineBuffer.SetFirstVisibleLine(vscroll);
            if (GetDocumentListener())
            {
                GetDocumentListener()->View_OnVScroll(*this, vscroll);
            }
            rv = true;
        }
    }
    return rv;
}

Float   GFxTextDocView::GetTextWidth()
{
    // check if reformatting is pending. If yes - force reformat first
    if (IsReformatReq())
    {
        Format();
        ClearReformatReq();
    }
    if (TextWidth > 0)
        return Float(TextWidth + GFX_TEXT_GUTTER*0);
    return 0;
}
Float   GFxTextDocView::GetTextHeight()
{
    // check if reformatting is pending. If yes - force reformat first
    if (IsReformatReq())
    {
        Format();
        ClearReformatReq();
    }
    if (TextHeight > 0)
        return Float(TextHeight + GFX_TEXT_GUTTER*0);
    return 0;
}

UInt   GFxTextDocView::GetLinesCount()
{
    // check if reformatting is pending. If yes - force reformat first
    if (IsReformatReq())
    {
        Format();
        ClearReformatReq();
    }
    return LineBuffer.size();
}

UInt GFxTextDocView::GetMaxHScroll()
{
    // check if reformatting is pending. If yes - force reformat first
    if (IsReformatReq())
    {
        Format();
        ClearReformatReq();
    }
    SInt editorDelta = (HasEditorKit()) ? GFX_EDIT_HSCROLL_DELTA : 0;
    UInt v = UInt(GTL::gmax(0.f, GetTextWidth() - GetTextRect().Width() + editorDelta));
    return v;
}

UInt  GFxTextDocView::GetMaxVScroll()
{
    // check if reformatting is pending. If yes - force reformat first
    if (IsReformatReq())
    {
        Format();
        ClearReformatReq();
    }
    if (!MaxVScroll.IsValid(FormatCounter))
    {
        if (LineBuffer.size() == 0)
            MaxVScroll.SetValue(0, FormatCounter);
        else
        {
            Float h = 0;
            UInt l = 0;
            GFxTextLineBuffer::Iterator linesIt = LineBuffer.End();
            for(; !linesIt.IsFinished(); --linesIt, ++l)
            {
                const GFxTextLineBuffer::Line& line = *linesIt;
                if (l > 0 && h + line.GetHeight() + line.GetNonNegLeading() > GetTextRect().Height())
                    break;
                h += line.GetHeight() + line.GetNonNegLeading();
            }
            // MaxVScroll is zero based.
            MaxVScroll.SetValue(LineBuffer.size() - l, FormatCounter);
        }
    }
    return MaxVScroll;
}

void GFxTextDocView::SetAutoSizeX()     
{ 
    if (!IsAutoSizeX())
    {
        Flags |= Flags_AutoSizeX; 
        SetCompleteReformatReq();
    }
}

void GFxTextDocView::SetAutoSizeY()     
{ 
    if (!IsAutoSizeY())
    {
        Flags |= Flags_AutoSizeY; 
        SetCompleteReformatReq();
    }
}

void GFxTextDocView::SetWordWrap()      
{ 
    if (!IsWordWrap())
    {
        Flags |= Flags_WordWrap; 
        SetCompleteReformatReq();
    }
}

void GFxTextDocView::ClearWordWrap()    
{ 
    if (IsWordWrap())
    {
        Flags &= (~Flags_WordWrap); 
        SetCompleteReformatReq();
    }
}

bool GFxTextDocView::ForceReformat()
{
    if (IsReformatReq())
    {
        Format();
        ClearReformatReq();
        return true;
    }
    return false;
}

const GRectF& GFxTextDocView::GetViewRect() 
{ 
    ForceReformat();
    return ViewRect; 
}

void GFxTextDocView::SetAlignment(GFxTextDocView::ViewAlignment alignment)
{
    Alignment = alignment;
}

UPInt GFxTextDocView::GetCharIndexAtPoint(Float x, Float y)
{
    ForceReformat();
    x -= LineBuffer.Geom.VisibleRect.Left - ViewRect.Left;
    y -= LineBuffer.Geom.VisibleRect.Top - ViewRect.Top;
    GFxTextLineBuffer::Iterator it = LineBuffer.FindLineAtYOffset(y + LineBuffer.GetVScrollOffsetInTwips());
    if (!it.IsFinished())
    {
        GFxTextLineBuffer::Line& line = *it;

        Float lineOffX = line.GetOffsetX();
        if (x >= lineOffX && x <= lineOffX + Float(line.GetWidth()))
        {
            Float xoffInLine = x - lineOffX;
            SInt xoffset = 0;

            // now need to find the glyph entry. Do the exhaustive search for now
            GFxTextLineBuffer::GlyphIterator git = line.Begin();
            UInt i = 0;
            for(; !git.IsFinished(); ++git)
            {
                const GFxTextLineBuffer::GlyphEntry& ge = git.GetGlyph();
                xoffset += ge.GetAdvance();

                if (xoffset > xoffInLine)
                    break;

                i += ge.GetLength();
            }

            return line.GetTextPos() + i;
        }
    }
    return GFC_MAX_UPINT;
}

// x, y - coord on the screen
UPInt GFxTextDocView::GetCursorPosAtPoint(Float x, Float y)
{
    ForceReformat();
    x -= LineBuffer.Geom.VisibleRect.Left - ViewRect.Left;
    y -= LineBuffer.Geom.VisibleRect.Top - ViewRect.Top;
    GFxTextLineBuffer::Iterator it = LineBuffer.FindLineAtYOffset(y + LineBuffer.GetVScrollOffsetInTwips());
    if (it.IsFinished())
    {
        // if it.IsFinished, then try to get iterator pointed to the last line
        it = LineBuffer.Begin() + LineBuffer.size() - 1;
    }
    if (!it.IsFinished())
    {
        GFxTextLineBuffer::Line& line = *it;

        //?? if (x >= line.Offset.x && x <= line.Offset.x + Float(line.Width()))
        {
            Float xoffInLine = x - line.GetOffsetX() + LineBuffer.GetHScrollOffset();
            SInt xoffset = 0;

            // now need to find the glyph entry. Do the exhaustive search for now
            GFxTextLineBuffer::GlyphIterator git = line.Begin();
            UInt i = 0;
            for(; !git.IsFinished(); ++git)
            {
                const GFxTextLineBuffer::GlyphEntry& ge = git.GetGlyph();
                UInt advance = ge.GetAdvance();

                if (xoffset + advance > xoffInLine)
                {
                    if (xoffInLine - xoffset > advance / 2)
                    {
                        i += ge.GetLength();
                    }
                    break;
                }

                xoffset += advance;
                i += ge.GetLength();
            }

            return line.GetTextPos() + i;
        }
    }
    return (LineBuffer.size() == 0) ? 0 : GFC_MAX_UPINT;
}

// x, y - coord on the screen
bool GFxTextDocView::IsUrlAtPoint(Float x, Float y)
{
    ForceReformat();
    x -= LineBuffer.Geom.VisibleRect.Left - ViewRect.Left;
    y -= LineBuffer.Geom.VisibleRect.Top - ViewRect.Top;
    GFxTextLineBuffer::Iterator it = LineBuffer.FindLineAtYOffset(y + LineBuffer.GetVScrollOffsetInTwips());
    bool rv = false;
    if (!it.IsFinished())
    {
        GFxTextLineBuffer::Line& line = *it;

        Float lineOffX = line.GetOffsetX();
        if (x >= lineOffX && x <= lineOffX + Float(line.GetWidth()))
        {
            Float xoffInLine = x - lineOffX + LineBuffer.GetHScrollOffset();
            SInt xoffset = 0;

            // now need to find the glyph entry. Do the exhaustive search for now
            GFxTextLineBuffer::GlyphIterator git = line.Begin();
            UInt i = 0;
            for(; !git.IsFinished(); ++git)
            {
                const GFxTextLineBuffer::GlyphEntry& ge = git.GetGlyph();
                UInt advance = ge.GetAdvance();

                if (xoffset + advance > xoffInLine)
                {
                    rv = ge.IsUrl();
                    break;
                }

                xoffset += advance;
                i += ge.GetLength();
            }
        }
    }
    return rv;
}

// Returns the zero-based index value of the line at the point specified by the x  and y parameters.
UInt GFxTextDocView::GetLineIndexAtPoint(Float x, Float y)
{
    if (!CanBeEditable())
        return ~0u;
    GUNUSED(x);
    ForceReformat();
    GFxTextLineBuffer::Iterator it = LineBuffer.FindLineAtYOffset(y + LineBuffer.GetVScrollOffsetInTwips());
    if (!it.IsFinished())
    {
        return it.GetIndex();
    }
    return ~0u;
}

// Returns the zero-based index value of the line containing the character specified by the indexOfChar parameter.
UInt GFxTextDocView::GetLineIndexOfChar(UPInt indexOfChar)
{
    if (!CanBeEditable())
        return ~0u;
    ForceReformat();
    GFxTextLineBuffer::Iterator it = LineBuffer.FindLineByTextPos(indexOfChar);
    if (!it.IsFinished())
    {
        return it.GetIndex();
    }
    return ~0u;
}

// Returns the character index of the first character in the line that the lineIndex parameter specifies.
UPInt GFxTextDocView::GetLineOffset(UInt lineIndex)
{
    if (!CanBeEditable())
        return GFC_MAX_UPINT;
    ForceReformat();
    GFxTextLineBuffer::Iterator it = LineBuffer.Begin() + lineIndex;
    if (!it.IsFinished())
    {
        //const GFxTextLineBuffer::Line& line = *it;
        return it->GetTextPos();
    }
    return GFC_MAX_UPINT;
}

// Returns the number of characters in a specific text line.
UPInt GFxTextDocView::GetLineLength(UInt lineIndex)
{
    if (!CanBeEditable())
        return GFC_MAX_UPINT;
    ForceReformat();
    GFxTextLineBuffer::Iterator it = LineBuffer.Begin() + lineIndex;
    if (!it.IsFinished())
    {
        //const GFxTextLineBuffer::Line& line = *it;
        return it->GetTextLength();
    }
    return GFC_MAX_UPINT;
}

// Returns a rectangle that is the bounding box of the character.
// This version returns the same rectangle as Flash 9 does, based on advance. For newline
// symbols it returns false.
bool GFxTextDocView::GetCharBoundaries(GRectF* pCharRect, UPInt indexOfChar)
{
    GASSERT(pCharRect);
    if (!pCharRect || indexOfChar >= pDocument->GetLength()) return false;
    ForceReformat();

    GFxTextLineBuffer::Iterator it = LineBuffer.FindLineByTextPos(indexOfChar);
    bool rv = false;
    if (!it.IsFinished())
    {
        GFxTextLineBuffer::Line& line = *it;
        UPInt indexInLine = indexOfChar - line.GetTextPos();
        GFxTextLineBuffer::GlyphIterator git = line.Begin();
        SInt advance = 0;
        for(UInt i = 0; !git.IsFinished(); ++git, ++i)
        {
            const GFxTextLineBuffer::GlyphEntry& glyph = git.GetGlyph();
            if (i == indexInLine)
            {
                if (glyph.IsNewLineChar()) // do not return rect for newline, use GetExactCharBoundaries
                    break;
                rv = true;
                GFxFontResource* pfont = git.GetFont();
                Float scale = PixelsToTwips(glyph.GetFontSize()) / 1024.0f; // the EM square is 1024 x 1024   

                /////
                pCharRect->Clear();
                pCharRect->Right += Float(glyph.GetAdvance());
                ////

                // Flash 9 calculates boundary box using advance only
                // Though, it is not precise, to get precise boundary
                // need to use GetGlyphBounds instead of 2 lines of code above.
                //pfont->GetGlyphBounds(glyph.GetIndex(), pCharRect);

                Float textHeight = (pfont->GetAscent() + pfont->GetDescent()) * scale;
                //pCharRect->Left     *= scale;
                //pCharRect->Right    *= scale;
                pCharRect->Top      = GFX_TEXT_GUTTER;
                pCharRect->Bottom   = pCharRect->Top + textHeight;
                (*pCharRect) += GPointF(Float(advance) + GFX_TEXT_GUTTER, line.GetOffsetY());
                break;
            }
            advance += glyph.GetAdvance();
        }
    }
    return rv;
}

// Returns a rectangle that is the bounding box of the character.
// This method returns more exact rectangle, based on glyph width. For
// newline symbols it returns true and a rectangle with half width of a space.
bool GFxTextDocView::GetExactCharBoundaries(GRectF* pCharRect, UPInt indexOfChar)
{
    GASSERT(pCharRect);
    UPInt len = pDocument->GetLength();
    if (!pCharRect || indexOfChar > len) return false;
    ForceReformat();

    // a special case - simulate we have a very last fake char - EOF
    if (indexOfChar == len)
    {
        GFxTextLineBuffer::Iterator it = LineBuffer.FindLineByTextPos(indexOfChar);
        if (!it.IsFinished())
        {
            GFxTextLineBuffer::Line& line = *it;
            pCharRect->Left     = pCharRect->Top = 0;
            pCharRect->Right    = pCharRect->Left + PixelsToTwips(2.0f);
            pCharRect->Bottom   = pCharRect->Top + line.GetHeight();
            (*pCharRect) += GPointF(line.GetOffsetX() + line.GetWidth() + GFX_TEXT_GUTTER, line.GetOffsetY() + GFX_TEXT_GUTTER);
        }
        return true;
    }

    GFxTextLineBuffer::Iterator it = LineBuffer.FindLineByTextPos(indexOfChar);
    bool rv = false;
    if (!it.IsFinished())
    {
        GFxTextLineBuffer::Line& line = *it;
        UPInt indexInLine = indexOfChar - line.GetTextPos();
        GFxTextLineBuffer::GlyphIterator git = line.Begin();
        SInt advance = 0;
        for(UInt i = 0; !git.IsFinished(); ++git, ++i)
        {
            const GFxTextLineBuffer::GlyphEntry& glyph = git.GetGlyph();
            if (i == indexInLine)
            {
                rv = true;
                GFxFontResource* pfont = git.GetFont();
                Float scale = PixelsToTwips(glyph.GetFontSize()) / 1024.0f; // the EM square is 1024 x 1024   

                // Flash 9 calculates boundary box using advance only
                // Though, it is not precise, to get precise boundary
                // need to use GetGlyphBounds instead of 2 lines of code above.
                pfont->GetGlyphBounds(glyph.GetIndex(), pCharRect);
                if (glyph.IsNewLineChar())
                    pCharRect->SetWidth(pCharRect->Width()/2);

                Float textHeight = (pfont->GetAscent() + pfont->GetDescent()) * scale;
                pCharRect->Left     *= scale;
                pCharRect->Right    *= scale;
                pCharRect->Top      = GFX_TEXT_GUTTER;
                pCharRect->Bottom   = pCharRect->Top + textHeight;
                (*pCharRect) += GPointF(Float(advance) + GFX_TEXT_GUTTER, line.GetOffsetY());
                break;
            }
            advance += glyph.GetAdvance();
        }
    }
    return rv;
}
UPInt GFxTextDocView::GetFirstCharInParagraph(UPInt indexOfChar)
{
    UPInt indexInPara = GFC_MAX_UPINT;
    GFxStyledText::ParagraphsIterator pit = pDocument->GetParagraphByIndex(indexOfChar, &indexInPara);
    if (!pit.IsFinished())
    {
        return indexOfChar - indexInPara;
    }
    return GFC_MAX_UPINT;
}

UInt GFxTextDocView::GetBottomVScroll()
{
    ForceReformat();
    GFxTextLineBuffer::Iterator it = LineBuffer.BeginVisible(-Float(LineBuffer.GetVScrollOffsetInTwips()));
    UInt index = 0;
    for(; it.IsVisible(); ++it)
    {
        index = it.GetIndex();
    }
    return index;
}

bool GFxTextDocView::SetBottomVScroll(UInt newBottomMostLine)
{
    Float h = 0;
    UInt l = newBottomMostLine;
    GFxTextLineBuffer::Iterator linesIt = LineBuffer.Begin() + newBottomMostLine;
    for(; !linesIt.IsFinished(); --linesIt)
    {
        const GFxTextLineBuffer::Line& line = *linesIt;
        if (h + line.GetHeight() + line.GetNonNegLeading() > GetTextRect().Height())
            break;
        if (h != 0)
            --l; // do not decrement at first iteration
        h += line.GetHeight() + line.GetNonNegLeading();
    }
    return SetVScrollOffset(l);
}

GFxTextEditorKit* GFxTextDocView::CreateEditorKit()
{
    if (!pEditorKit)
        pEditorKit = *new GFxTextEditorKit(this);
    return pEditorKit;
}

GFxTextEditorKit* GFxTextDocView::GetEditorKit()
{
    return pEditorKit;
}

////////////////////////////////////////////
// GFxTextEditorKit
//
GFxTextEditorKit::GFxTextEditorKit(GFxTextDocView* pdocview):pDocView(pdocview), Flags(0)
{
    CursorTimer     = 0.0f;
    CursorPos       = 0;
    CursorColor     = 0xFF000000; // black, opaque
    LastAdvanceTime = 0.0f;
    CursorRect->Clear();
}

GFxTextEditorKit::~GFxTextEditorKit()
{

}

void GFxTextEditorKit::SetCursorPos(UPInt pos)
{
    if (!pDocView->CanBeEditable())
        return;
    if (pos != GFC_MAX_UPINT)
    {
        UPInt len = pDocView->pDocument->GetLength();
        if (pos > len)
            pos = len;
        
        ResetBlink(1);
        
        if (pDocView->pImageSubstitutor)
        {
            // if image substitution is on, then we need to check that new cursor pos
            // is not inside the image. If so - correct the cursor position.
            UPInt textPos;
            const GFxTextLineBuffer::GlyphEntry* pge = GetGlyphEntryAtIndex(pos, &textPos);
            if (pge && pge->GetLength() != 1 && textPos != pos)
            {
                // determine the vector of cursor changing
                if (pos >= CursorPos)
                    pos = textPos + pge->GetLength();
                else
                    pos = textPos;
            }
        }
    }

    CursorPos = pos;
    CursorRect.Invalidate();

    if (CursorPos != GFC_MAX_UPINT)
    {
        ScrollToCursor();

        const GFxTextParagraphFormat* pparaFmt = NULL;
        const GFxTextFormat* ptextFmt = NULL;
        GFxTextDocView* pdoc = GetDocument();
        GFxStyledText* ptext = pdoc->GetStyledText();

        UPInt indexInPara = pdoc->GetFirstCharInParagraph(CursorPos);
        if (indexInPara != GFC_MAX_UPINT)
        {
            bool res;
            if (indexInPara != CursorPos)
                res = ptext->GetTextAndParagraphFormat(&ptextFmt, &pparaFmt, CursorPos - 1);
            else
                res = ptext->GetTextAndParagraphFormat(&ptextFmt, &pparaFmt, CursorPos);
            if (res)
            {
                pDocView->SetDefaultParagraphFormat(pparaFmt);
                pDocView->SetDefaultTextFormat(ptextFmt);
            }
        }
    }
#ifdef DEBUG_CURSOR_POS
    printf("CursorPos = %d\n", CursorPos);
#endif // DEBUG_CURSOR_POS
}

UPInt GFxTextEditorKit::GetCursorPos() const
{
    return CursorPos;
}

void GFxTextEditorKit::Advance(Float timer)
{
    if (HasCursor())
    {
        Float delta = timer - LastAdvanceTime;
        LastAdvanceTime = timer;

        if (CursorTimer+delta > 0.5)
        {
            if (!IsCursorTimerBlocked())
            {
                if (IsCursorBlink())
                    ClearCursorBlink();
                else
                    SetCursorBlink();
            }
            CursorTimer = 0.0f;
            ClearCursorTimerBlocked();
        }
        else
            CursorTimer += delta;
    }
}

bool GFxTextEditorKit::CalcCursorRectPosInLineBuffer(UPInt charIndex, GPointF* ptopLeft, UInt* plineIndex, UInt* pglyphIndex)
{
    if (!pDocView->CanBeEditable())
        return false;
    GASSERT(ptopLeft);    
    UInt lineIndex = pDocView->GetLineIndexOfChar(charIndex);
    //pDocView->LineBuffer.Dump();
    
    if (lineIndex != ~0u)
    {
        GFxTextLineBuffer::Line& line = pDocView->LineBuffer[lineIndex];

        UPInt posInLine = CursorPos - line.GetTextPos();
        SInt xoffset = 0;

        // now need to find the glyph entry. Do the exhaustive search for now
        GFxTextLineBuffer::GlyphIterator git = line.Begin();
        UInt nGlyph = 0;

        // skip all leading "empty" glyphs (with length == 0)
        for(;!git.IsFinished(); ++git)
        {
            const GFxTextLineBuffer::GlyphEntry& ge = git.GetGlyph();
            if (ge.GetLength() != 0 || ge.IsNewLineChar())
                break;
            xoffset += ge.GetAdvance();
            ++nGlyph;
        }
        for(UInt i = 0;i < posInLine && !git.IsFinished(); ++git)
        {
            const GFxTextLineBuffer::GlyphEntry& ge = git.GetGlyph();
            xoffset += ge.GetAdvance();

            i += ge.GetLength();
            ++nGlyph;
        }
        ptopLeft->x = line.GetOffsetX() + Float(xoffset);
        ptopLeft->y = line.GetOffsetY();
        if (plineIndex)
            *plineIndex = lineIndex;
        if (pglyphIndex)
            *pglyphIndex = nGlyph;
        return true;
    }
    return false;
}

bool GFxTextEditorKit::CalcCursorRectPosOnScreen(UPInt charIndex, GPointF* ptopLeft, UInt* plineIndex, UInt* pglyphIndex)
{
    bool rv = CalcCursorRectPosInLineBuffer(charIndex, ptopLeft, plineIndex, pglyphIndex);
    if (rv)
    {
        GASSERT(ptopLeft);
        ptopLeft->x -= Float(pDocView->LineBuffer.GetHScrollOffset());
        ptopLeft->y -= Float(pDocView->LineBuffer.GetVScrollOffsetInTwips());
        const GRectF& rect = pDocView->GetViewRect();
        *ptopLeft = pDocView->LineBuffer.BufferToView(rect, *ptopLeft);
        ptopLeft->x += rect.Left;
        ptopLeft->y += rect.Top;
    }
    return rv;
}

const GFxTextLineBuffer::GlyphEntry* GFxTextEditorKit::GetGlyphEntryAtIndex(UPInt charIndex, UPInt* ptextPos)
{
    if (!pDocView->CanBeEditable())
        return false;
    UInt lineIndex = pDocView->GetLineIndexOfChar(charIndex);
    if (lineIndex != ~0u)
    {
        GFxTextLineBuffer::Line* pline = pDocView->LineBuffer.GetLine(lineIndex);
        if (!pline)
            return NULL;
        UInt textPos = pline->GetTextPos();
        UPInt posInLine = charIndex - textPos;

        // now need to find the glyph entry. 
        GFxTextLineBuffer::GlyphIterator git = pline->Begin();
        const GFxTextLineBuffer::GlyphEntry* pge = NULL;

        // skip all leading "empty" glyphs (with length == 0)
        for(;!git.IsFinished(); ++git)
        {
            const GFxTextLineBuffer::GlyphEntry& ge = git.GetGlyph();
            if (ge.GetLength() != 0 || ge.IsNewLineChar())
                break;
        }
        UInt prevLen = 0;
        for(UInt i = 0; i <= posInLine; ++git)
        {
            textPos += prevLen;
            if (git.IsFinished())
                break;
            pge = &git.GetGlyph();

            prevLen = pge->GetLength();
            i += prevLen;
        }
        if (ptextPos)
            *ptextPos = textPos;
        return pge;
    }
    return NULL;
}


// Draws cursor, if necessary
void GFxTextEditorKit::Display(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx)
{
    GRenderer*  prenderer       = context.GetRenderer();

    // Draw the cursor
    if (IsCursorBlink() && !IsReadOnly())
    {
        // check is CursorRect valid
        UInt lineIndex = GFC_MAX_UINT;
        if (!CursorRect.IsValid(pDocView->FormatCounter))
        {
            // not valid, calculate
            GRectF newCursorRect;
            GPointF topLeft;
            UInt glyphIndex;

            if (CalcCursorRectPosInLineBuffer(CursorPos, &topLeft, &lineIndex, &glyphIndex))
            {
                const GFxTextLineBuffer::Line& line = pDocView->LineBuffer[lineIndex];
                newCursorRect.SetRect(topLeft, GSizeF(0.f, Float(line.GetHeight())));

                // determine the cursor's color
                const GFxTextFormat* pdefFormat = pDocView->GetDefaultTextFormat();

                if (pdefFormat && pdefFormat->IsColorSet())
                    CursorColor = pdefFormat->GetColor();
            }
            else
            {
                // a special case, if line buffer is empty.
                // we need to calculate height of cursor by getting a default format,
                // get the font from it and get height from it.
                GFxTextDocView::FindFontInfo findFontInfo;

                findFontInfo.pCurrentFormat = pDocView->GetDefaultTextFormat();
                GPtr<GFxFontHandle> pfontHandle = pDocView->FindFont(&findFontInfo);
                GASSERT(pfontHandle);
                GFxFontResource* pfont = pfontHandle->GetFont();

                if (findFontInfo.pCurrentFormat->IsColorSet())
                    CursorColor = findFontInfo.pCurrentFormat->GetColor();

                Float scale = PixelsToTwips(findFontInfo.pCurrentFormat->GetFontSize()) / 1024.0f; // the EM square is 1024 x 1024   
                Float h = (pfont->GetAscent() + pfont->GetDescent()) * scale;
                newCursorRect.SetRect(GPointF(0, 0), GSizeF(0.f, h));
            }

            // CursorRect is in text area coordinate space, where topLeft corner is (0,0)
            // Later, calculating actualCursorRect we will need to translate them to textRect's
            // coordinate space, where topLeft corner may have any coordinates, different from (0,0).
            CursorRect.SetValue(newCursorRect, pDocView->FormatCounter);
        }

        GRectF actualCursorRect = *CursorRect;
        const GRectF& textRect = pDocView->GetTextRect();
        actualCursorRect -= GPointF(Float(pDocView->LineBuffer.GetHScrollOffset()), Float(pDocView->LineBuffer.GetVScrollOffsetInTwips()));
        actualCursorRect += textRect.TopLeft();

        // sometimes, cursor rect may be higher than the actual viewRect, especially in the case
        // of single line edit with big leading value. So, correct the bottom of cursor rect here
        if (actualCursorRect.Bottom > textRect.Bottom)
        {
            if (lineIndex == GFC_MAX_UINT)
                lineIndex = pDocView->GetLineIndexOfChar(CursorPos);
            if (pDocView->LineBuffer.IsLineVisible(lineIndex))
                actualCursorRect.Bottom = textRect.Bottom;
        }

        if (textRect.Contains(actualCursorRect))
        {
            actualCursorRect.Right = actualCursorRect.Left + 20.0f;

            Matrix m(mat);
            GRectF newRect;
            GFx_RecalculateRectToFit16Bit(m, actualCursorRect, &newRect);
            prenderer->SetMatrix(m);

            prenderer->SetCxform(cx);

            // snap to pixel the x value
            Matrix mm(m); 
            mm.Append(context.ViewportMatrix);
            Matrix invmm(mm);
            invmm.Invert();

            GPointF p1 = mm.Transform(newRect.TopLeft());
            p1.x = floorf(p1.x + 0.5f);
            newRect.Left = invmm.Transform(p1).x;

            // calculate the width of one pixel in twips
            ++p1.x;
            p1 = invmm.Transform(p1);
            newRect.Right = p1.x;

            GColor  transformedColor    = cx.Transform(CursorColor);

            // Force the size of the cursor width
            // TODO: clamp to viewport to prevent cursor from becoming less than 1 pixel

            GPointF         coords[4];
            static const UInt16   indices[6] = { 0, 1, 2, 2, 1, 3 };

            coords[0] = newRect.TopLeft();
            coords[1] = newRect.TopRight();
            coords[2] = newRect.BottomLeft();
            coords[3] = newRect.BottomRight();

            GRenderer::VertexXY16i icoords[4];
            icoords[0].x = (SInt16) coords[0].x;
            icoords[0].y = (SInt16) coords[0].y;
            icoords[1].x = (SInt16) coords[1].x;
            icoords[1].y = (SInt16) coords[1].y;
            icoords[2].x = (SInt16) coords[2].x;
            icoords[2].y = (SInt16) coords[2].y;
            icoords[3].x = (SInt16) coords[3].x;
            icoords[3].y = (SInt16) coords[3].y;

            // Draw the cursor rect
            prenderer->FillStyleColor(transformedColor);
            prenderer->SetVertexData(icoords, 4, GRenderer::Vertex_XY16i);            
            prenderer->SetIndexData(indices, 6, GRenderer::Index_16);
            prenderer->DrawIndexedTriList(0, 0, 6, 0, 2);
            // Done
            prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
            prenderer->SetIndexData(0, 0, GRenderer::Index_None);
        }
    }
}

void    GFxTextEditorKit::ResetBlink(bool state, bool blocked)
{
    if (!IsReadOnly())
    {
        if (state)
            SetCursorBlink();
        else
            ClearCursorBlink();
    }
    else
    {
        ClearCursorBlink();
    }
    CursorTimer = 0.0f;
    if (blocked)
        SetCursorTimerBlocked();
    // do not clear "blocked" flag; it will be cleared at the next advance
    //else
    //    ClearCursorTimerBlocked();
}

// scrolls view to make cursor visible
bool GFxTextEditorKit::ScrollToCursor()
{
    if (!pDocView->CanBeEditable())
        return false;
    GPointF topLeft;
    UInt lineIndex;
    bool rv = false;

    if (CalcCursorRectPosOnScreen(CursorPos, &topLeft, &lineIndex))
    {
        const GRectF& textRect = pDocView->GetTextRect();
        
        GRectF newCursorRect;
        const GFxTextLineBuffer::Line& line = pDocView->LineBuffer[lineIndex];
        newCursorRect.SetRect(topLeft, GSizeF(20.f, Float(line.GetHeight())));

        if (!textRect.Contains(newCursorRect))
        {
            // cursor is invisible, calculate scrolling
            // hscroll first
            SInt hscroll = (SInt)pDocView->LineBuffer.GetHScrollOffset();
            SInt prevHScroll = hscroll;
            if (topLeft.x > textRect.Right)
            {
                hscroll += SInt(topLeft.x - textRect.Right) + GFX_EDIT_HSCROLL_DELTA;
            }
            else if (topLeft.x < textRect.Left)
            {
                hscroll -= SInt(textRect.Left - topLeft.x) + GFX_EDIT_HSCROLL_DELTA;
                hscroll = GTL::gmax(0, hscroll);
            }
            GASSERT(hscroll >= 0);
    
            // do not scroll left horizontally, if word-wrapping is on
            if (!pDocView->IsWordWrap() || hscroll < prevHScroll)
                rv = pDocView->SetHScrollOffset(UInt(hscroll));

            // calculate new vscroll
            UInt fvline = pDocView->GetVScrollOffset();
            if (lineIndex < fvline)
            {
                rv |= pDocView->SetVScrollOffset(lineIndex);
            }
            else
            {
                UInt bottomV = pDocView->GetBottomVScroll();
                if (lineIndex > bottomV)
                {
                    rv |= pDocView->SetBottomVScroll(lineIndex);
                }
            }
        }
    }
    return rv;
}

void    GFxTextEditorKit::OnMouseDown(Float x, Float y, int buttons)
{
    if (buttons & 1) // left mouse button is down
    {
        Float ax = x;// + Float(pDocView->LineBuffer.GetHScrollOffset());
        Float ay = y;// + Float(pDocView->LineBuffer.GetVScrollOffsetInTwips()) - pDocView->LineBuffer.GetVScrollOffsetInTwips();
        const GRectF& r = pDocView->GetViewRect();
        ax -= r.Left;
        ay -= r.Top;

        UPInt pos = pDocView->GetCursorPosAtPoint(ax, ay);
        if (pos != GFC_MAX_UPINT && !IsReadOnly())
        {
            SetCursorPos(pos);
        }
    }
}

bool GFxTextEditorKit::OnKeyDown(int keyCode, const GFxSpecialKeysState& specKeysState)
{
    if (!pDocView->CanBeEditable())
        return false;
    GFxTextDocView* pdocument = GetDocument();
    UPInt len = pdocument->GetStyledText()->GetLength();
    bool rv = false;
    UPInt newPos = CursorPos;

    if (pdocument->GetDocumentListener())
    {
        keyCode = pdocument->GetDocumentListener()->Editor_OnKey(*this, keyCode);
    }
    bool needChangeNotify = false;
    switch(keyCode)
    {
        // Left arrow key
        case GFxKey::Left:
        {
            if (newPos == GFC_MAX_UPINT)
                newPos = len;
            if (newPos > 0)
                newPos -= 1;
            break;
        }
        // Right arrow key
        case GFxKey::Right:
        {
            if (newPos == GFC_MAX_UPINT)
                newPos = len;
            else if (newPos < len)
                newPos += 1;
            break;
        }
        // Home key
        case GFxKey::Home:
        {
            if (specKeysState.IsCtrlPressed()) // Ctrl-Home
            {
                newPos = 0;
            }
            else
            {
                UInt lineIndex = pDocView->GetLineIndexOfChar(CursorPos);
                if (lineIndex != ~0u)
                {
                    newPos = pDocView->GetLineOffset(lineIndex);
                    GASSERT(newPos != GFC_MAX_UPINT);
                }
            }
            break;
        }
        // End key
        case GFxKey::End:
        {
            if (specKeysState.IsCtrlPressed()) // Ctrl-End
            {
                newPos = len;
            }
            else
            {
                UInt lineIndex = pDocView->GetLineIndexOfChar(CursorPos);
                if (lineIndex != ~0u)
                {
                    GASSERT(pDocView->GetLineOffset(lineIndex) != GFC_MAX_UPINT && pDocView->GetLineLength(lineIndex) != GFC_MAX_UPINT);
                    newPos = pDocView->GetLineOffset(lineIndex) + pDocView->GetLineLength(lineIndex);
                }
            }
            break;
        }
        case GFxKey::PageUp:
            {
                GPointF topLeft;
                UInt lineIndex;

                if (specKeysState.IsCtrlPressed()) // Ctrl-PageUp - goto to the beginning of current page
                {
                    lineIndex = pDocView->GetVScrollOffset();
                    newPos = pDocView->GetLineOffset(lineIndex);
                    GASSERT(newPos != GFC_MAX_UPINT);
                }
                else
                {
                    if (CalcCursorRectPosOnScreen(CursorPos, &topLeft, &lineIndex))
                    {
                        UInt bottomV = pDocView->GetBottomVScroll();
                        UInt numLinesOnScreen = bottomV - pDocView->GetVScrollOffset() + 1;
                        if (lineIndex < numLinesOnScreen)
                            lineIndex = 0;
                        else
                            lineIndex -= numLinesOnScreen;
                        const GFxTextLineBuffer::Line& line = pDocView->LineBuffer[lineIndex];
                        GPointF lineOffInView = pDocView->LineBuffer.BufferToView(pDocView->GetViewRect(), line.GetOffset());

                        newPos = pDocView->GetCursorPosAtPoint(topLeft.x, lineOffInView.y - pDocView->LineBuffer.GetVScrollOffsetInTwips());
                        GASSERT(newPos != GFC_MAX_UPINT);
                    }
                }
                break;
            }
        case GFxKey::PageDown:
            {
                GPointF topLeft;
                UInt lineIndex;

                if (specKeysState.IsCtrlPressed()) // Ctrl-PageDown - goto to the bottom of current page
                {
                    lineIndex = pDocView->GetBottomVScroll();
                    newPos = pDocView->GetLineOffset(lineIndex) + pDocView->GetLineLength(lineIndex);
                }
                else
                {
                    if (CalcCursorRectPosOnScreen(CursorPos, &topLeft, &lineIndex))
                    {
                        UInt bottomV = pDocView->GetBottomVScroll();
                        UInt numLinesOnScreen = bottomV - pDocView->GetVScrollOffset() + 1;
                        lineIndex += numLinesOnScreen;
                        if (lineIndex >= pDocView->GetLinesCount())
                            lineIndex = pDocView->GetLinesCount() - 1;
                        const GFxTextLineBuffer::Line& line = pDocView->LineBuffer[lineIndex];
                        GPointF lineOffInView = pDocView->LineBuffer.BufferToView(pDocView->GetViewRect(), line.GetOffset());

                        newPos = pDocView->GetCursorPosAtPoint(topLeft.x, lineOffInView.y - pDocView->LineBuffer.GetVScrollOffsetInTwips());
                        GASSERT(newPos != GFC_MAX_UPINT);
                    }
                }
                break;
            }
        case GFxKey::Up:
        {
            GPointF topLeft;
            UInt lineIndex;

            if (CalcCursorRectPosOnScreen(CursorPos, &topLeft, &lineIndex))
            {
                if (lineIndex > 0)
                {
                    --lineIndex;
                    const GFxTextLineBuffer::Line& line = pDocView->LineBuffer[lineIndex];
                    GPointF lineOffInView = pDocView->LineBuffer.BufferToView(pDocView->GetViewRect(), line.GetOffset());

                    newPos = pDocView->GetCursorPosAtPoint(topLeft.x, lineOffInView.y - pDocView->LineBuffer.GetVScrollOffsetInTwips());
                    GASSERT(newPos != GFC_MAX_UPINT);
                }
            }
            break;
        }
        case GFxKey::Down:
        {
            GPointF topLeft;
            UInt lineIndex;

            if (CalcCursorRectPosOnScreen(CursorPos, &topLeft, &lineIndex))
            {
                if (lineIndex + 1 < pDocView->GetLinesCount())
                {
                    ++lineIndex;
                    const GFxTextLineBuffer::Line& line = pDocView->LineBuffer[lineIndex];
                    GPointF lineOffInView = pDocView->LineBuffer.BufferToView(pDocView->GetViewRect(), line.GetOffset());

                    newPos = pDocView->GetCursorPosAtPoint(topLeft.x, lineOffInView.y - pDocView->LineBuffer.GetVScrollOffsetInTwips()); // <=!
                    GASSERT(newPos != GFC_MAX_UPINT);
                }
            }
            break;
        }
#ifdef GFC_BUILD_DEBUG
        case 19: // pause
            {
                pDocView->LineBuffer.Dump();
                break;
            }
        /*case 192:
            {
                pDocView->pDocument->GetStyledText()->Remove(1, 20);
            }
            break;*/
        /*case 192: // `
            //pDocView->pDocument->GetStyledText()->InsertString(L"AAAA\nBBBB\n334", 10, ~0u);
            pDocView->pDocument->GetStyledText()->InsertString(L"AAAA\nBBBB\nTheEND! ", 0, ~0u);
            break;*/
#endif
        case GFxKey::Backspace:
        {
            if (!IsReadOnly())
            {
                if (1) // TODO: if (noActiveSelection)
                {
                    if (CursorPos > 0)
                    {
                        GFxTextDocView::DeleteCharCommand cmd(--newPos);
                        pDocView->EditCommand(GFxTextDocView::DeleteChar, &cmd);

                        needChangeNotify = true;
                    }
                }
                else
                {
                    //TODO
                    //GFxTextDocView::DeleteCommand cmd(StartSelection, EndSelection);
                    //pDocView->EditCommand(GFxTextDocView::Delete, &cmd);
                }
            }
            break;
        }
        case GFxKey::Delete:
        {
            if (!IsReadOnly())
            {
                if (1) // TODO: if (noActiveSelection)
                {
                    if (CursorPos < len)
                    {
                        GFxTextDocView::DeleteCharCommand cmd(newPos);
                        pDocView->EditCommand(GFxTextDocView::DeleteChar, &cmd);
                        needChangeNotify = true;
                    }
                }
                else
                {
                    //TODO
                    //GFxTextDocView::DeleteCommand cmd(StartSelection, EndSelection);
                    //pDocView->EditCommand(GFxTextDocView::Delete, &cmd);
                }
            }
            break;
        }
        case GFxKey::Return:
        {
            if (!IsReadOnly())
            {
                if (pDocView->IsMultiline())
                {
                    GFxTextDocView::InsertCharCommand cmd(CursorPos, pDocView->NewLineChar());
                    pDocView->EditCommand(GFxTextDocView::InsertChar, &cmd);
                    ++newPos;
                    needChangeNotify = true;
                }
            }
            break;
        }
        default:
            break;
    }

    if (CursorPos != newPos)
    {
        SetCursorPos(newPos);
        rv = true;
    }
    if (needChangeNotify)
    {
        if (pdocument->GetDocumentListener())
        {
            pdocument->GetDocumentListener()->Editor_OnChanged(*this);
        }
    }
    return rv;
}

bool GFxTextEditorKit::OnChar(UInt32 wcharCode)
{
    if (!wcharCode)
        return false;
    if (!pDocView->CanBeEditable())
        return false;
    GFxTextDocView* pdocument = GetDocument();
    bool rv = false;
    UPInt newPos = CursorPos;

    bool needChangeNotify = false;
    if (!IsReadOnly())
    {
        wchar_t wc;
        wc = (wchar_t)wcharCode;
        if (wc >= 32)
        {
            if (wc && pdocument->GetDocumentListener())
            {
                wc = pdocument->GetDocumentListener()->Editor_OnCharacter(*this, wc);
            }
            if (wc)
            {
                GFxTextDocView::InsertCharCommand cmd(CursorPos, wc);
                pDocView->EditCommand(GFxTextDocView::InsertChar, &cmd);
                ++newPos;
                needChangeNotify = true;
            }
        }
    }

    if (CursorPos != newPos)
    {
        SetCursorPos(newPos);
        rv = true;
    }
    if (needChangeNotify)
    {
        if (pdocument->GetDocumentListener())
        {
            pdocument->GetDocumentListener()->Editor_OnChanged(*this);
        }
    }
    return rv;
}

void GFxTextEditorKit::OnDocumentChanged(UInt notifyMask)
{
    if (notifyMask & GFxTextDocView::ViewNotify_SignificantTextChange)
    {
        if (!IsReadOnly() || IsSelectable())
        {
            //if (!pDocView->IsMultiline())
            //    SetCursorPos(pDocView->pDocument->GetLength());
            //else
                SetCursorPos(0);
        }
    }
    else
    {
        if (CursorPos > pDocView->pDocument->GetLength())
            SetCursorPos(pDocView->pDocument->GetLength());
    }
}


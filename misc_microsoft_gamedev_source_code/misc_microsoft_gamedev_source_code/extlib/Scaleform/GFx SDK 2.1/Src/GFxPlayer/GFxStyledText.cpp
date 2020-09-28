/**********************************************************************

Filename    :   GFxStyledText.cpp
Content     :   Static and dynamic text field character implementation
Created     :   Feb-May, 2007
Authors     :   Artem Bolgar

Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

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

#define GFX_BULLET_CHAR_CODE        0x25CF
#define GFX_BULLET_CHAR_ADVANCE     15
#define GFX_BULLET_PARA_INDENT      20
#define GFX_LINE_ALLOC_DELTA        100
#define GFX_NBSP_CHAR_CODE          160
#define GFX_TEXT_JUSTIFY_CORRECTION 30

#define GFX_EDIT_HSCROLL_DELTA PixelsToTwips(60)

#define GFX_MIN_LINE_HEIGHT_FOR_AUTOSIZE PixelsToTwips(6)

#define GFX_DEFAULT_COMPOSSTR_TEXTCOLOR 0xFFFF0000
#define GFX_DEFAULT_CLAUSE_TEXTCOLOR    0xFF0000FF

#ifdef GFC_BUILD_DEBUG
//#define DEBUG_CURSOR_POS
//#define GFX_DEBUG_DISPLAY_INVISIBLE_CHARS
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
        fontNameSz = gfc_wcslen(pfontName);

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
        urlSz = gfc_wcslen(purl);
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
        result.SetFontSizeInTwips(fmt.GetFontSizeInTwips());
    if (fmt.IsFontNameSet())
        result.SetFontName(fmt.GetFontName());
    if (fmt.IsFontHandleSet())
        result.SetFontHandle(fmt.GetFontHandle());
    if (fmt.IsUrlCleared())
        result.ClearUrl();
    else if (fmt.IsUrlSet())
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
    if (IsFontSizeSet() && fmt.IsFontSizeSet() && GetFontSizeInTwips() == fmt.GetFontSizeInTwips())
        result.SetFontSizeInTwips(fmt.GetFontSizeInTwips());
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
    SetColor32(0);
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

size_t GFxTextFormat::HashFunctor::operator()(const GFxTextFormat& data) const
{
    UPInt v[] = { 0, 0, 0, 0};
    if (data.IsColorSet() || data.IsAlphaSet()) v[0] |= data.Color;
    if (data.IsLetterSpacingSet())  v[1] |= data.LetterSpacing;
    if (data.IsFontSizeSet())       v[1] |= ((UPInt)data.FontSize << 16);
    v[0] |= UPInt(data.FormatFlags) << 24;
    v[1] |= UPInt(data.PresentMask) << 24;
    if (data.IsFontHandleSet())
        v[2] = (UPInt)data.pFontHandle.GetPtr();
    if (data.IsImageDescSet())
        v[3] = (UPInt)data.pImageDesc.GetPtr();
    size_t hash = GTL::gfixed_size_hash<int>::sdbm_hash(&v, sizeof(v));
    if (data.IsFontNameSet())
    {
        size_t size = gfc_strlen(data.FontName);
        hash ^= GFxString::BernsteinHashFunctionCIS(data.FontName, size);           
    }
    if (data.IsUrlSet())
    {
        hash ^= GFxString::BernsteinHashFunctionCIS(data.Url.ToCStr(), data.Url.GetSize());
    }
    return hash;
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
    if (IsTabStopsSet() && fmt.IsTabStopsSet() && TabStopsEqual(fmt.GetTabStops()))
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

void GFxTextParagraphFormat::SetTabStopsElement(UInt idx, UInt val)
{
    if (pTabStops && idx < pTabStops[0])
    {
        pTabStops[idx + 1] = val;
    }
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
        if (!pTabStops || pTabStops[0] != n)
        {
            AllocTabStops(n);
        }
        memcpy(pTabStops + 1, psrcTabStops + 1, n * sizeof(UInt));
    }
    else
        FreeTabStops();
}

GFxTextParagraphFormat& GFxTextParagraphFormat::operator=(const GFxTextParagraphFormat& src)
{
    BlockIndent     = src.BlockIndent;
    Indent          = src.Indent;
    Leading         = src.Leading;
    LeftMargin      = src.LeftMargin;
    RightMargin     = src.RightMargin;
    Align           = src.Align;
    Bullet          = src.Bullet;
    PresentMask     = src.PresentMask;
    CopyTabStops(src.pTabStops);
    return *this;
}

size_t  GFxTextParagraphFormat::HashFunctor::operator()(const GFxTextParagraphFormat& data) const
{
    size_t hash = 0;
    if (data.IsTabStopsSet() && data.pTabStops)
        hash ^= GTL::gfixed_size_hash<UInt>::sdbm_hash(data.pTabStops, (data.pTabStops[0] + 1) * sizeof(UInt));
    if (data.IsBlockIndentSet())
        hash ^= data.GetBlockIndent();
    if (data.IsIndentSet())
        hash ^= ((size_t)data.GetIndent()) << 8;
    if (data.IsLeadingSet())
        hash ^= ((size_t)data.GetLeading()) << 12;
    if (data.IsLeftMarginSet())
        hash ^= ((size_t)data.GetLeftMargin()) << 16;
    if (data.IsRightMarginSet())
        hash ^= ((size_t)data.GetRightMargin()) << 18;
    hash ^= ((((size_t)data.PresentMask) << 9) | (((size_t)data.GetAlignment()) << 1) | (size_t)data.IsBullet());
    return hash;
}

//////////////////////////////////////////////////////////////////////////
// GFxTextAllocator
//
GFxTextParagraph* GFxTextAllocator::AllocateParagraph()
{
    return new GFxTextParagraph(this);
}

GFxTextParagraph* GFxTextAllocator::AllocateParagraph(const GFxTextParagraph& srcPara)
{
    return new GFxTextParagraph(srcPara);
}

//////////////////////////////////
// GFxWideStringBuffer
//
GFxWideStringBuffer::~GFxWideStringBuffer()
{
    pAllocator->FreeText(pText);
}

const wchar_t* GFxWideStringBuffer::GetCharPtrAt(UPInt pos) const
{
    if (!pText || pos >= Size)
        return NULL;
    return &pText[pos];
}

void GFxWideStringBuffer::SetString(const char* putf8Str, UPInt utf8length)
{
    UPInt len;
    if (utf8length == GFC_MAX_UPINT)
        len = GUTF8Util::GetLength(putf8Str, gfc_strlen(putf8Str));
    else
        len = utf8length;
    if (Allocated < len)
    {
        pText = pAllocator->ReallocText(pText, Allocated, len);
        GASSERT(pText);
        Allocated = len;
    }
    if (len > 0)
    {
        GASSERT(pText);
        GUTF8Util::DecodeString(pText, putf8Str, len);
    }
    Size = len;
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
    Size = length;
}

wchar_t* GFxWideStringBuffer::CreatePosition(UPInt pos, UPInt length)
{
    GASSERT(pos <= Size);

    if (Allocated < Size + length)        
    {
        pText = pAllocator->ReallocText(pText, Allocated, Size + length);
        GASSERT(pText);
        Allocated = Size + length;
    }
    if (Size - pos > 0)
    {
        GASSERT(pText);
        memmove(pText + pos + length, pText + pos, (Size - pos)*sizeof(wchar_t));
    }
    Size += length;
    GASSERT(pText != NULL || pos == 0);
    return pText + pos;
}

void GFxWideStringBuffer::Remove(UPInt pos, UPInt length)
{
    if (pos < Size)
    {
        if (pos + length >= Size)
            Size = pos;
        else
        {
            memmove(pText + pos, pText + pos + length, (Size - (pos + length))*sizeof(wchar_t));
            Size -= length;
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
    int len = int(Size);
    // check, is the content already null terminated
    if (len > 0 && pText[len -1] == 0)
        --len; //if yes, skip the '\0'
    for (int i = len - 1; i >= 0 && (pText[i] == '\n' || pText[i] == '\r'); --i)
    {
        --Size;
        pText[i] = 0;
    }
}

void GFxWideStringBuffer::StripTrailingNull()
{
    if (Size > 0)
    {
        GASSERT(pText);
        if (pText[Size - 1] == '\0')
            --Size;
    }
}

void GFxWideStringBuffer::AppendTrailingNull()
{
    if (Size > 0 && Size < Allocated)
    {
        GASSERT(pText);
        pText[Size++] = '\0';
    }
    else
    {
        wchar_t* p = CreatePosition(Size, 1); 
        GASSERT(p);
        if (p)
            *p  = '\0';
    }
}

UPInt GFxWideStringBuffer::GetLength() const
{
    int len = int(Size);
    // check, is the content already null terminated
    if (len > 0 && pText[len - 1] == '\0')
        return Size - 1;

    return Size;
}

//////////////////////////////////
// GFxTextParagraph
//
GFxTextParagraph::GFxTextParagraph(GFxTextAllocator* ptextAllocator):
    Text(ptextAllocator), StartIndex(0), ModCounter(0) 
{
    GASSERT(ptextAllocator);
    UniqueId = ptextAllocator->AllocateParagraphId();
}

GFxTextParagraph::GFxTextParagraph(const GFxTextParagraph& o) : Text(o.Text), 
    pFormat(o.pFormat), FormatInfo(o.FormatInfo), StartIndex(o.StartIndex), ModCounter(0)
{
    GASSERT(GetAllocator());
    UniqueId = GetAllocator()->AllocateParagraphId();
}

void GFxTextParagraph::Clear()
{
    Text.Clear();
    FormatInfo.RemoveAll();
    ++ModCounter;
}

void GFxTextParagraph::SetText(const wchar_t* pstring, UPInt length)
{
    GASSERT(length <= GFC_MAX_UPINT);
    if (length != GFC_MAX_UPINT)
    {
        // check if string contains '\0' inside. If yes - correct
        // the length.
        for (SPInt i = (SPInt)length - 1; i >= 0; --i)
        {
            if (pstring[i] == '\0')
            {
                length = (UPInt)i;
                break;
            }
        }
    }
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
            pfmt = *GetAllocator()->AllocateTextFormat(format);
        }
        else
        {
            pfmt = *GetAllocator()->AllocateTextFormat(fmt);
        }

        UPInt newLen = GTL::gpmin(UPInt(runLength - (curIndex - runIndex)), (UPInt)length);
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

void GFxTextParagraph::ClearTextFormat(UPInt startPos, UPInt endPos)
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

        UPInt newLen = GTL::gpmin(UPInt(runLength - (curIndex - runIndex)), (UPInt)length);
        FormatInfo.ClearRange(curIndex, newLen);
        length -= newLen;
        //++it;
        it.SetTextPos(runIndex + runLength);
    }

    //_dump(FormatInfo);

    ++ModCounter;
}


void GFxTextParagraph::SetFormat(const GFxTextParagraphFormat& fmt)
{
    GPtr<GFxTextParagraphFormat> pfmt;
    if (pFormat)
        pfmt = *GetAllocator()->AllocateParagraphFormat(pFormat->Merge(fmt));
    else
        pfmt = *GetAllocator()->AllocateParagraphFormat(fmt);
    pFormat = pfmt;
    ++ModCounter;
}

void GFxTextParagraph::SetFormat(const GFxTextParagraphFormat* pfmt)
{
    pFormat = const_cast<GFxTextParagraphFormat*>(pfmt);
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
    if (length == 0)
        return NULL;
    wchar_t* p = Text.CreatePosition(pos, length);
    GASSERT(p);
    FormatInfo.ExpandRange(pos, length);
    ++ModCounter;
    return p;
}

void GFxTextParagraph::SetTermNullFormat()
{
    if (HasTermNull())
    {
        UPInt len = GetLength();
        FormatInfo.ExpandRange(len, 1);
        FormatInfo.RemoveRange(len + 1, 1);
    }
}

void GFxTextParagraph::InsertString(const wchar_t* pstr, UPInt pos, UPInt length, const GFxTextFormat* pnewFmt)
{
    if (length > 0)
    {
        if (length == GFC_MAX_UPINT)
            length = GFxWideStringBuffer::StrLen(pstr);
        wchar_t* p = CreatePosition(pos, length);
        GASSERT(p);
        if (p)
        {
            memcpy(p, pstr, length * sizeof(wchar_t));
            if (pnewFmt)
            {
                //if (HasTermNull() && pos + length == GetLength())
                //    ++length; // need to expand format info to the termination null symbol
                FormatInfo.SetRange(pos, length, const_cast<GFxTextFormat*>(pnewFmt));
            }

            SetTermNullFormat();
            ++ModCounter;
        }
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
        GASSERT(p);

        if (p)
        {
            memcpy(p, pstr, length * sizeof(wchar_t));
            ++ModCounter;
        }
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
        GASSERT(p);

        if (p)
        {
            GUTF8Util::DecodeString(p, putf8str, length);
            ++ModCounter;
        }
        //_dump(FormatInfo);
    }
}

void GFxTextParagraph::AppendTermNull(const GFxTextFormat* pdefTextFmt)
{
    if (!HasTermNull())
    {
        UPInt pos = GetLength();
        wchar_t* p = CreatePosition(pos, 1);
        GASSERT(p);
        if (p)
        {
            *p = '\0';
            GASSERT(FormatInfo.Count() || pdefTextFmt);
            if (FormatInfo.Count() == 0 && pdefTextFmt)
            {
                GPtr<GFxTextFormat> pfmt = *GetAllocator()->AllocateTextFormat(*pdefTextFmt);
                FormatInfo.SetRange(pos, 1, pfmt);
            }
        }
    }
}

void GFxTextParagraph::RemoveTermNull()
{
    if (HasTermNull())
    {
        FormatInfo.RemoveRange(GetLength(), 1);
        Text.StripTrailingNull();
    }
}

bool GFxTextParagraph::HasTermNull() const
{
    UPInt l = Text.GetSize();
    return (l > 0 && *Text.GetCharPtrAt(l - 1) == '\0');
}

bool GFxTextParagraph::HasNewLine() const
{
    UPInt l = Text.GetSize();
    if (l > 0)
    {
        wchar_t c = *Text.GetCharPtrAt(l - 1);
        return (c == '\r' || c == '\n');
    }
    return false;
}

// returns true if paragraph is empty (no chars or only termination null)
UPInt GFxTextParagraph::GetLength() const
{
    UPInt len = GetSize();
    if (len > 0 && HasTermNull())
        --len;
    return len;
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
        SetTermNullFormat();
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
        UPInt remainingLen = length;
        for(; !fmtIt.IsFinished() && remainingLen > 0; ++fmtIt)
        {
            const GFxTextParagraph::StyledTextRun& run = *fmtIt;

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
            len = GTL::gpmin(len, remainingLen);
            if (run.pFormat)
            {
                FormatInfo.SetRange(startDestIndex + idx, len, run.pFormat);
            }
            remainingLen -= len;
        }
        SetTermNullFormat();
        ++ModCounter;
    }
}

// Shrinks the paragraph's length by the "delta" value.
void GFxTextParagraph::Shrink(UPInt delta)
{
    UPInt len = Text.GetSize();
    delta = GTL::gpmin(delta, len);
    Remove(len - delta, len);
}

#ifdef GFC_BUILD_DEBUG
void GFxTextParagraph::CheckIntegrity() const
{
    const_cast<GFxTextParagraph*>(this)->FormatInfo.CheckIntegrity();

    // check the size and length are correctly set
    if (GetLength() != GetSize())
        GASSERT(HasTermNull());
    else
        GASSERT(!HasTermNull());

    // check if the null or newline chars are inside of the paragraph. 
    // Should be only at the end of it.
    for(UPInt i = 0, n = GetLength(); i < n; ++i)
    {
        GASSERT(Text.GetCharPtrAt(i));
        wchar_t c = *Text.GetCharPtrAt(i);
        GASSERT(c != 0);
        if (i + 1 < n)
            GASSERT(c != '\r' && c != '\n');
    }

    // also need to verify there are no term null and \n - \r symbols together
    for(UPInt i = 0, n = GetSize(), m = 0; i < n; ++i)
    {
        GASSERT(Text.GetCharPtrAt(i));
        wchar_t c = *Text.GetCharPtrAt(i);
        if (c == '\r')      { GASSERT(m == 0); m |= 1; }
        else if (c == '\n') { GASSERT(m == 0); m |= 2; }
        else if (c == 0)    { GASSERT(m == 0); m |= 4; }

        // also check, that every char is covered by format data
        GASSERT(GetTextFormatPtr(i));
    }

    // ensure, format info range array does not exceed the text size
    TextFormatArrayType::Iterator it = const_cast<GFxTextParagraph*>(this)->FormatInfo.Last();
    if (!it.IsFinished())
    {
        TextFormatRunType& r = *it; 
        SPInt ni = r.NextIndex();
        UPInt sz = GetSize();
        GASSERT(ni >= 0 && ((UPInt)ni) <= sz);
    }

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
    return PlaceHolder.Set(pText->ToWStr() + CurTextIndex, CurTextIndex, pText->GetSize() - CurTextIndex, NULL); 
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
        CurTextIndex = pText->GetSize();
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
        CurTextIndex = pText->GetSize();
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
        if (plen) *plen = pText->GetSize() - CurTextIndex;
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
    pDefaultTextFormat      = *new GFxTextFormat;
}

GFxStyledText::GFxStyledText(GFxTextAllocator* pallocator) : 
    pTextAllocator(pallocator),Paragraphs(), RTFlags(0)
{
    GASSERT(pallocator);
    pDefaultParagraphFormat = *pallocator->AllocateParagraphFormat(GFxTextParagraphFormat());
    pDefaultTextFormat      = *pallocator->AllocateTextFormat(GFxTextFormat());
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

GFxTextParagraph* GFxStyledText::AppendCopyOfParagraph(const GFxTextParagraph& srcPara)
{
    UPInt sz = Paragraphs.size();
    UPInt nextPos = 0;
    if (sz > 0)
    {
        GFxTextParagraph* ppara = Paragraphs[sz - 1];
        GASSERT(ppara);
        nextPos = ppara->GetStartIndex() + ppara->GetLength();
    }

    Paragraphs.append(GetAllocator()->AllocateParagraph(srcPara));

    sz = Paragraphs.size();
    GFxTextParagraph* ppara = Paragraphs[sz - 1];
    GASSERT(ppara);
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

GFxTextParagraph* GFxStyledText::InsertCopyOfParagraph(GFxStyledText::ParagraphsIterator& iter, 
                                                       const GFxTextParagraph& srcPara)
{
    if (iter.IsFinished())
        return AppendCopyOfParagraph(srcPara);
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

        Paragraphs.insert(index, GetAllocator()->AllocateParagraph(srcPara));

        GFxTextParagraph* ppara = Paragraphs[index];
        GASSERT(ppara);
        ppara->SetStartIndex(nextPos);
        return ppara;
    }
}

GFxTextParagraph* GFxStyledText::GetLastParagraph()
{
    ParagraphsIterator paraIter = Paragraphs.Last();
    if (!paraIter.IsFinished())
    {
        return paraIter->GetPtr();
    }
    return NULL;
}

const GFxTextParagraph* GFxStyledText::GetLastParagraph() const
{
    ParagraphsConstIterator paraIter = Paragraphs.Last();
    if (!paraIter.IsFinished())
    {
        return paraIter->GetPtr();
    }
    return NULL;
}

UPInt GFxStyledText::AppendString(const char* putf8String, UPInt stringSize,
                                  NewLinePolicy newLinePolicy)
{
    GASSERT(GetDefaultTextFormat() && GetDefaultParagraphFormat());
    return AppendString(putf8String, stringSize, newLinePolicy, 
                        GetDefaultTextFormat(), GetDefaultParagraphFormat());
}

UPInt GFxStyledText::AppendString(const char* putf8String, UPInt stringSize,
                                  NewLinePolicy newLinePolicy,
                                  const GFxTextFormat* pdefTextFmt, 
                                  const GFxTextParagraphFormat* pdefParaFmt)
{
    GASSERT(pdefTextFmt && pdefParaFmt);
    // parse text, and form paragraphs
    const char* pbegin = putf8String;
    if (stringSize == GFC_MAX_UPINT)
        stringSize = gfc_strlen(putf8String);
    const char* pend = pbegin + stringSize;

    // parse text, and form paragraphs
    UInt32 uniChar = 0;

    UPInt totalAppenededLen = 0;
    UPInt curOffset;
    GFxTextParagraph* ppara = GetLastParagraph();
    if (!ppara)
        curOffset = 0;
    else
        curOffset = ppara->GetStartIndex();

    OnTextInserting(curOffset, stringSize, putf8String);

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
            ppara->RemoveTermNull();
            posInPara = ppara->GetLength();
        }
        GASSERT(ppara);
        UPInt paraLength = 0;
        const char* pstr = pbegin;
        UInt32 prevUniChar = uniChar;
        for (uniChar = ~0u; pstr < pend && uniChar != 0; )
        {
            UInt32 newUniChar = GUTF8Util::DecodeNextChar(&pstr);
            if (newLinePolicy == NLP_CompressCRLF && prevUniChar == '\r' && paraLength == 0)
            {
                prevUniChar = ~0u;
                if (newUniChar == '\n')
                {
                    ++pbegin;
                    continue; // skip extra newline
                }
            }
            uniChar = newUniChar;
            if (uniChar == '\n' || uniChar == '\r')
                break;
            ++paraLength;
        }
        if (uniChar == '\n' || uniChar == '\r')
            ++paraLength; 

        if (paraLength > 0)
        {
            wchar_t* pwstr = ppara->CreatePosition(posInPara, paraLength);

            pstr = pbegin;
            for (uniChar = ~0u; pstr < pend && uniChar != 0; )
            {
                uniChar = GUTF8Util::DecodeNextChar(&pstr);
                if (uniChar == '\r' || uniChar == '\n')  // replace '\r' by '\n'
                    uniChar = NewLineChar();
                *pwstr++ = (wchar_t)uniChar;
                if (uniChar == NewLineChar())
                    break;
            }
            ppara->SetTextFormat(*pdefTextFmt, posInPara, GFC_MAX_UPINT);
            pbegin = pstr;
            curOffset += paraLength + posInPara;
            totalAppenededLen += paraLength;
        }
    } while(pbegin < pend && uniChar != 0);

    // check the last char. If it is \n - create the empty paragraph
    if (uniChar == NewLineChar())
        ppara = AppendNewParagraph(pdefParaFmt);

    ppara->AppendTermNull(pdefTextFmt);

    if (pdefTextFmt->IsUrlSet())
        SetMayHaveUrl();

    CheckIntegrity();
    return totalAppenededLen;
}

UPInt GFxStyledText::AppendString(const wchar_t* pstr, UPInt length, NewLinePolicy newLinePolicy)
{
    GASSERT(GetDefaultTextFormat() && GetDefaultParagraphFormat());
    return AppendString(pstr, length, newLinePolicy, GetDefaultTextFormat(), GetDefaultParagraphFormat());
}

UPInt GFxStyledText::AppendString(const wchar_t* pstr, UPInt length,
                                  NewLinePolicy newLinePolicy,
                                  const GFxTextFormat* pdefTextFmt, 
                                  const GFxTextParagraphFormat* pdefParaFmt)
{
    GASSERT(pdefTextFmt && pdefParaFmt);

    // parse text, and form paragraphs
    UInt32 uniChar = 0;
    if (length == GFC_MAX_UPINT)
        length = gfc_wcslen(pstr);
    const wchar_t* pend = pstr + length;

    UPInt curOffset;
    GFxTextParagraph* ppara = GetLastParagraph();
    if (!ppara)
        curOffset = 0;
    else
        curOffset = ppara->GetStartIndex();

    OnTextInserting(curOffset, length, pstr);

    UPInt totalAppenededLen = 0;
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
            ppara->RemoveTermNull();
            posInPara = ppara->GetLength();
        }
        UPInt paraLength = 0;
        // check, do we need to skip extra new line char in the case if "\r\n" EOL
        // is used.
        if (newLinePolicy == NLP_CompressCRLF && uniChar == '\r' && pstr[0] == '\n')
        {
            ++pstr;
            --length;
            if (length == 0)
                break; // it is possible that that was the last char.
        }
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
        totalAppenededLen += paraLength;
    } while(pstr < pend && uniChar != '\0');

    // check the last char. If it is \n - create the empty paragraph
    if (uniChar == NewLineChar())
        ppara = AppendNewParagraph(pdefParaFmt);

    ppara->AppendTermNull(pdefTextFmt);

    CheckIntegrity();
    return totalAppenededLen;
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

void GFxStyledText::EnsureTermNull()
{
    GFxTextParagraph* ppara = GetLastParagraph();
    if (ppara && !ppara->HasNewLine())
    {
        ppara->AppendTermNull(pDefaultTextFormat);
    }
}

void GFxStyledText::SetTextFormat(const GFxTextFormat& fmt, UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    UPInt indexInPara, endIndexInPara;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    UPInt runLen = endPos - startPos;
    while(!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt paraLength = ppara->GetLength();
        if (indexInPara + runLen > paraLength)
            endIndexInPara = indexInPara + paraLength - indexInPara;
        else
            endIndexInPara = indexInPara + runLen;

        if (endIndexInPara == paraLength && ppara->HasTermNull())
        {
            ++endIndexInPara; // include the termination null symbol
            if (runLen != GFC_MAX_UPINT) // preventing overflowing
                ++runLen;
        }

        ppara->SetTextFormat(fmt, indexInPara, endIndexInPara);

        GASSERT(runLen >= (endIndexInPara - indexInPara));
        runLen -= (endIndexInPara - indexInPara);
        indexInPara = 0;
        ++paraIter;
    }
    if (fmt.IsUrlSet())
        SetMayHaveUrl();
    CheckIntegrity();
}

void GFxStyledText::SetParagraphFormat(const GFxTextParagraphFormat& fmt, UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    UPInt indexInPara, endIndexInPara;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    UPInt runLen = endPos - startPos;
    while(!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        if (indexInPara == 0)
        {
            ppara->SetFormat(fmt);
        }
        if (runLen == 0)
            break;
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

void GFxStyledText::ClearTextFormat(UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    UPInt indexInPara, endIndexInPara;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    UPInt runLen = endPos - startPos;
    while(!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt paraLength = ppara->GetLength();
        if (indexInPara + runLen > paraLength)
            endIndexInPara = indexInPara + paraLength - indexInPara;
        else
            endIndexInPara = indexInPara + runLen;

        if (endIndexInPara == paraLength && ppara->HasTermNull())
        {
            ++endIndexInPara; // include the termination null symbol
            if (runLen != GFC_MAX_UPINT) // preventing overflowing
                ++runLen;
        }

        ppara->ClearTextFormat(indexInPara, endIndexInPara);

        GASSERT(runLen >= (endIndexInPara - indexInPara));
        runLen -= (endIndexInPara - indexInPara);
        indexInPara = 0;
        ++paraIter;
    }
}

void GFxStyledText::GetTextAndParagraphFormat
    (GFxTextFormat* pdestTextFmt, GFxTextParagraphFormat* pdestParaFmt, UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    GASSERT(pdestParaFmt || pdestTextFmt);

    UPInt indexInPara;
    UPInt runLen = endPos - startPos;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    GFxTextFormat finalTextFmt;
    GFxTextParagraphFormat finalParaFmt;
    int i = 0, pi = 0;
    while(runLen > 0 && !paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt lengthInPara = GTL::gpmin(runLen, ppara->GetLength());
        if (lengthInPara == 0)
            break; // the only empty paragraph could be the last one, so, we good to break
        if (i++ == 0)
            finalTextFmt = ppara->GetTextFormat(indexInPara, indexInPara + lengthInPara);
        else
            finalTextFmt = ppara->GetTextFormat(indexInPara, indexInPara + lengthInPara).Intersection(finalTextFmt);
        if (indexInPara == 0)
        {
            const GFxTextParagraphFormat* ppfmt = ppara->GetFormat();
            if (ppfmt)
            {
                if (pi++ == 0)
                    finalParaFmt = *ppfmt;
                else
                    finalParaFmt = ppfmt->Intersection(finalParaFmt);
            }
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

void GFxStyledText::SetDefaultTextFormat(const GFxTextFormat& defaultTextFmt)
{
    if (defaultTextFmt.GetImageDesc())
    {
        GFxTextFormat textfmt = defaultTextFmt;
        textfmt.SetImageDesc(NULL);
        pDefaultTextFormat = *GetAllocator()->AllocateTextFormat(textfmt);
    }
    else 
        pDefaultTextFormat = *GetAllocator()->AllocateTextFormat(defaultTextFmt);
    GASSERT(pDefaultTextFormat);
}

void GFxStyledText::SetDefaultParagraphFormat(const GFxTextParagraphFormat& defaultParagraphFmt)
{
    pDefaultParagraphFormat = *GetAllocator()->AllocateParagraphFormat(defaultParagraphFmt);
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
    for(UPInt np = 0, nn = Paragraphs.size(); !paraIter.IsFinished(); ++paraIter, ++np)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        if (np + 1 == nn && ppara->GetLength() == 0)
            continue; // this is the last empty paragraph - ignore it
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
            retStr += " TABSTOPS=\"";
            UInt n;
            const UInt *ptabStops = paraFormat.GetTabStops(&n);
            for (UInt i = 0; i < n; ++i)
            {
                char buf[20];
                if (i > 0)
                    retStr += ",";
                gfc_sprintf(buf, 20, "%d", ptabStops[i]);
                retStr += buf;
            }
            retStr += "\"";
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
            if (run.Length == 1 && run.pText[0] == NewLineChar())
                break; // a spec case - do not include format for newline char into HTML,
                       // if this format is different from rest of the text.
            bool isImageTag = false;
            if (run.pFormat)
            {
                GPtr<GFxTextHTMLImageTagDesc> pimage = run.pFormat->GetImageDesc();
                GASSERT(!pimage || run.pFormat->IsImageDescSet());
                if (pimage)
                {
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
                        gfc_sprintf(buf, 20, "%d\"", (UInt)run.pFormat->GetFontSize());
                        retStr += buf;
                    }
                    if (run.pFormat->IsColorSet())
                    {
                        retStr += " COLOR=\"#";
                        char buf[20];
                        gfc_sprintf(buf, 20, "%.6X\"", (run.pFormat->GetColor32() & 0xFFFFFFu));
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
                    if (run.pText[i] == NewLineChar() || run.pText[i] == '\0')
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
        UPInt oldSz = pBuffer->GetSize();
        pBuffer->CreatePosition(oldSz,  ppara->GetLength());
        memcpy(pBuffer->ToWStr() + oldSz, pstr, sizeof(wchar_t) * ppara->GetLength());
    }
    pBuffer->AppendTrailingNull();
}

void GFxStyledText::GetText(GFxWideStringBuffer* pBuffer,UPInt startPos, UPInt endPos) const
{
    GASSERT(endPos >= startPos);
    GASSERT(pBuffer);

    if (endPos == GFC_MAX_UPINT)
        endPos = GetLength();
    UPInt len = endPos - startPos;

    pBuffer->Clear();
    UPInt indexInPara = 0, remainedLen = len;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    for(; !paraIter.IsFinished() && remainedLen > 0; ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt lenInPara = ppara->GetLength() - indexInPara;
        if (lenInPara > remainedLen)
            lenInPara = remainedLen;
        const wchar_t* pstr = ppara->GetText();

        UPInt oldSize = pBuffer->GetSize();
        pBuffer->CreatePosition(oldSize,  lenInPara);
        memcpy(pBuffer->ToWStr() + oldSize, pstr + indexInPara, sizeof(wchar_t) * lenInPara);

        indexInPara = 0;
        remainedLen -= lenInPara;
        GASSERT(((SPInt)remainedLen) >= 0);
    }
    pBuffer->AppendTrailingNull();
}

GFxStyledText* GFxStyledText::CopyStyledText(UPInt startPos, UPInt endPos) const
{
    GFxStyledText* pstyledText = new GFxStyledText(GetAllocator());
    CopyStyledText(pstyledText, startPos, endPos);
    return pstyledText;
}

void GFxStyledText::CopyStyledText(GFxStyledText* pdest, UPInt startPos, UPInt endPos) const
{
    GASSERT(endPos >= startPos);

    if (endPos == GFC_MAX_UPINT)
        endPos = GetLength();
    UPInt len = endPos - startPos;
    UPInt indexInPara = 0, remainedLen = len;
    pdest->Clear();
    pdest->OnTextInserting(startPos, len, "");
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    if (!paraIter.IsFinished())
    {
        // if we need to take the only part of the first paragraph then
        // do Copy for this part. Otherwise, whole paragraph will be copied later.
        if (indexInPara != 0)
        {
            GFxTextParagraph* psrcPara = *paraIter;
            GFxTextParagraph* pdestPara = pdest->AppendNewParagraph(psrcPara->GetFormat());
            UPInt lenToCopy = psrcPara->GetLength() - indexInPara;
            lenToCopy = GTL::gpmin(lenToCopy, len);
            pdestPara->Copy(*psrcPara, indexInPara, 0, lenToCopy);
            remainedLen -= lenToCopy;
            ++paraIter;
        }
        for(; !paraIter.IsFinished() && remainedLen > 0; ++paraIter)
        {
            GFxTextParagraph* ppara = *paraIter;
            GASSERT(ppara);
            UPInt lenInPara = ppara->GetLength();
            if (lenInPara > remainedLen)
            {
                // copy the part of the last paragraph
                GFxTextParagraph* psrcPara = *paraIter;
                GFxTextParagraph* pdestPara = pdest->AppendNewParagraph(psrcPara->GetFormat());
                pdestPara->Copy(*psrcPara, 0, 0, remainedLen);
                break;
            }
            pdest->AppendCopyOfParagraph(*ppara);
            remainedLen -= lenInPara;
            GASSERT(((SPInt)remainedLen) >= 0);
        }
    }
    // check the last char. If it is \n - create the empty paragraph
    GFxTextParagraph* plastPara = pdest->GetLastParagraph();
    if (plastPara && plastPara->HasNewLine())
    {
        pdest->AppendNewParagraph(plastPara->GetFormat());
    }

    pdest->EnsureTermNull();
    if (MayHaveUrl())
        pdest->SetMayHaveUrl();
    pdest->CheckIntegrity();
}

wchar_t* GFxStyledText::CreatePosition(UPInt pos, UPInt length)
{
    GUNUSED2(pos, length);
    return 0;
}

UPInt GFxStyledText::InsertString(const wchar_t* pstr, UPInt pos, UPInt length, NewLinePolicy newLinePolicy)
{
    return InsertString(pstr, pos, length, newLinePolicy, GetDefaultTextFormat(), GetDefaultParagraphFormat());
}

UPInt GFxStyledText::InsertString(const wchar_t* pstr, UPInt pos, UPInt length, 
                                  NewLinePolicy newLinePolicy,
                                  const GFxTextFormat* pdefTextFmt, 
                                  const GFxTextParagraphFormat* pdefParaFmt)
{
    if (length == 0)
        return 0;
    if (pos > GetLength())
        pos = GetLength();

    // Insert plain string at the position. Style will be inherited from the current style
    // at the insertion position. If there is no style at the insertion position, then
    // text will remain plain. New paragraph will be created after each CR.
    if (length == GFC_MAX_UPINT)
        length = GFxWideStringBuffer::StrLen(pstr);

    OnTextInserting(pos, length, pstr);

    UPInt indexInPara = 0;
    UPInt remainingSrcStrLen = length;
    SPInt newLineIdx;
    ParagraphsIterator paraIter = GetNearestParagraphByIndex(pos, &indexInPara);

    UPInt insLineLen;
    UPInt nextParaStartingPos = (!paraIter.IsFinished()) ? (*paraIter)->GetStartIndex() : 0;
    wchar_t uniChar = 0;
    UPInt totalInsertedLen = 0;

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
        if (ppara->IsEmpty())
        {
            // if paragraph is empty we can apply the pdefParaFmt
            ppara->SetFormat(*pdefParaFmt);
        }

        insLineLen = 0;
        newLineIdx = -1;
        // check, do we need to skip extra new line char in the case if "\r\n" EOL
        // is used.
        if (newLinePolicy == NLP_CompressCRLF && uniChar == '\r' && pstr[insLineLen] == '\n')
        {
            ++pstr;
            --remainingSrcStrLen;
            if (remainingSrcStrLen == 0)
                break; // it is possible that that was the last char.
        }
        // look for a new-line char in the line being inserted.
        for(; insLineLen < remainingSrcStrLen; ++insLineLen)
        {
            uniChar = pstr[insLineLen];
            if (uniChar == '\n' || uniChar == '\r')
            {
                newLineIdx = insLineLen;
                break;
            }
            else if (uniChar == '\0')
                break;
        }
        if (uniChar == '\n' || uniChar == '\r')
            ++insLineLen; // skip \n

        // was there a new-line symbol?
        if (newLineIdx != -1)
        {
            // yes, make another paragraph first
            ParagraphsIterator newParaIter = paraIter;
            ++newParaIter;
            GFxTextParagraph& newPara = *InsertNewParagraph(newParaIter, pdefParaFmt);

            // new paragraph should inherit style from the previous ppara->
            newPara.SetFormat(ppara->GetFormat());
            GASSERT(newPara.GetFormat());

            // now, copy the text to the new para
            newPara.Copy(*ppara, indexInPara, 0, ppara->GetSize() - indexInPara);

            UPInt remLen = ppara->GetSize();
            GASSERT(remLen >= indexInPara);
            remLen -= indexInPara;

            // now, we can insert the sub-string in source para
            ppara->InsertString(pstr, indexInPara, insLineLen, pdefTextFmt);

            if (remLen > 0)
            {
                // now, make source paragraph shorter
                ppara->Shrink(remLen);
            }
        }
        else
        {
            // the string being inserted doesn't contain any '\n'
            // so, insert it as a substring
            ppara->InsertString(pstr, indexInPara, insLineLen, pdefTextFmt);
        }
        // do we need to replace the newline char?
        if ((uniChar == '\r' || uniChar == '\n') && uniChar != NewLineChar())
        {
            wchar_t* ptxt = ppara->GetText();
            GASSERT(ptxt[indexInPara + insLineLen - 1] == '\r' || 
                    ptxt[indexInPara + insLineLen - 1] == '\n');
            ptxt[indexInPara + insLineLen - 1] = NewLineChar();
        }

        GASSERT(remainingSrcStrLen >= insLineLen);
        remainingSrcStrLen -= insLineLen;
        pstr += insLineLen;
        totalInsertedLen += insLineLen;
        indexInPara = 0;
        ppara->SetStartIndex(nextParaStartingPos);
        nextParaStartingPos += ppara->GetSize();
        ++paraIter;
    } while(remainingSrcStrLen > 0 && uniChar != '\0');

    // correct the starting indices for remaining paragraphs
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        ppara->SetStartIndex(nextParaStartingPos);
        nextParaStartingPos += ppara->GetSize();
    }

    EnsureTermNull();

    if (pdefTextFmt->IsUrlSet())
        SetMayHaveUrl();
    CheckIntegrity();
    return totalInsertedLen;
}

UPInt GFxStyledText::InsertStyledText(const GFxStyledText& text, UPInt pos, UPInt length)
{
    if (length == GFC_MAX_UPINT)
        length = text.GetLength();
    if (length == 0)
        return 0;
    OnTextInserting(pos, length, ""); //??

    UPInt indexInPara = 0;
    ParagraphsIterator destParaIter = GetNearestParagraphByIndex(pos, &indexInPara);

    if (destParaIter.IsFinished())
    {
        // ok, there is no paragraphs - create a new one
        AppendNewParagraph();
        destParaIter = GetParagraphIterator();
        indexInPara = 0;
    }

    UPInt nextParaStartingPos = (!destParaIter.IsFinished()) ? (*destParaIter)->GetStartIndex() : 0;
    if (text.Paragraphs.size() == 1)
    {
        // special case if there is only one para to insert
        GFxTextParagraph* pdestPara = *destParaIter;
        const GFxTextParagraph* psrcPara = text.Paragraphs[0];
        GASSERT(psrcPara && pdestPara);
        pdestPara->Copy(*psrcPara, 0, indexInPara, psrcPara->GetLength());            
        if (indexInPara == 0)
            pdestPara->SetFormat(psrcPara->GetFormat());

        nextParaStartingPos += pdestPara->GetSize();
        ++destParaIter;
    }
    else
    {   
        UPInt remainedLen = length;
        ParagraphsIterator srcParaIter = text.GetParagraphIterator();
        GASSERT(!srcParaIter.IsFinished());

        // divide the target paragraph on two parts
        GFxTextParagraph* ppara = *destParaIter;
        ParagraphsIterator newParaIter = destParaIter;
        ++newParaIter;
        GFxTextParagraph& newPara = *InsertNewParagraph(newParaIter, ppara->GetFormat());

        // now, copy the text to the new para
        newPara.Copy(*ppara, indexInPara, 0, ppara->GetSize() - indexInPara);

        UPInt remLen = ppara->GetSize();
        GASSERT(remLen >= indexInPara);
        remLen -= indexInPara;

        // now, we can insert the first source paragraph
        GFxTextParagraph* psrcPara = *srcParaIter;
        GASSERT(psrcPara);
        ppara->Copy(*psrcPara, 0, indexInPara, psrcPara->GetLength());
        remainedLen -= psrcPara->GetLength();
        GASSERT(((SPInt)remainedLen) >= 0);
        if (indexInPara == 0)
            ppara->SetFormat(psrcPara->GetFormat());

        if (remLen > 0)
        {
            // now, make source paragraph shorter
            ppara->Shrink(remLen);
        }
        nextParaStartingPos += ppara->GetLength();

        ++destParaIter;
        ++srcParaIter;
        // now we can insert remaining paras
        for(; !srcParaIter.IsFinished() && remainedLen > 0; ++srcParaIter, ++destParaIter)
        {
            const GFxTextParagraph* psrcPara = *srcParaIter;
            GASSERT(psrcPara);
            UPInt lenInPara = psrcPara->GetLength();
            if (lenInPara >= remainedLen)
            {
                // copy the part of the last paragraph
                newPara.Copy(*psrcPara, 0, 0, lenInPara);
                newPara.SetFormat(psrcPara->GetFormat());
                newPara.SetStartIndex(nextParaStartingPos);
                nextParaStartingPos += newPara.GetLength();
                ++destParaIter;
                break;
            }
            InsertCopyOfParagraph(destParaIter, *psrcPara);
            remainedLen -= lenInPara;
            nextParaStartingPos += lenInPara;
            GASSERT(((SPInt)remainedLen) >= 0);
        }
    }

    // correct the starting indices for remaining paragraphs
    for(; !destParaIter.IsFinished(); ++destParaIter)
    {
        GFxTextParagraph* ppara = *destParaIter;
        GASSERT(ppara);
        if (ppara->GetStartIndex() == nextParaStartingPos)
            break;
        ppara->SetStartIndex(nextParaStartingPos);
        nextParaStartingPos += ppara->GetSize();
    }
    EnsureTermNull();
    if (text.MayHaveUrl())
        SetMayHaveUrl();
    CheckIntegrity();
    return length;
}

void GFxStyledText::Remove(UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    UPInt length;
    if (endPos == GFC_MAX_UPINT)
        length = GetLength();
    else
        length = endPos - startPos;

    OnTextRemoving(startPos, length);

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

        UPInt lengthInPara = GTL::gpmin(remainingLen, ppara->GetSize() - indexInPara);
        if (lengthInPara <= ppara->GetSize())
        {
            needUniteParas = (indexInPara + lengthInPara >= ppara->GetSize());
            pprevPara = *paraIter;
            ppara->Remove(indexInPara, indexInPara + lengthInPara);
            remainingLen -= lengthInPara;
            ++paraIter;
        }
    }

    // second stage - remove all paragraphs we can remove completely
    while(!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt paraLen = ppara->GetSize();
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
                UPInt insPos = pprevPara->GetSize();
                UPInt insLen = paraLen - remainingLen;
                pprevPara->Copy(*ppara, remainingLen, insPos, insLen);

                OnParagraphRemoving(*ppara);
                paraIter.remove();
                needUniteParas = false;
            }
            break;
        }
        else  
            break; //???
        if (remainingLen == 0) 
            break;
    }
    if (!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        if (ppara->GetSize() == 0)
        {
            OnParagraphRemoving(*ppara);
            paraIter.remove();
        }
        else if (pprevPara && needUniteParas)
        {
            // 3rd stage, if remainingLen == 0: do a copy of remaining part of the last para
            UPInt paraLen = ppara->GetSize();
            UPInt insPos = pprevPara->GetSize();
            UPInt insLen = paraLen;
            pprevPara->Copy(*ppara, 0, insPos, insLen);

            OnParagraphRemoving(*ppara);
            paraIter.remove();
        }
    }

    // correct the starting indices for remaining paragraphs
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        ppara->SetStartIndex(ppara->GetStartIndex() - length);
    }
    EnsureTermNull();
    CheckIntegrity();
}

void GFxStyledText::OnTextInserting(UPInt startPos, UPInt length, const wchar_t* ptxt)
{
    GUNUSED3(startPos, length, ptxt);
}

void GFxStyledText::OnTextRemoving(UPInt startPos, UPInt length)
{
    GUNUSED2(startPos, length);
}

/*
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
*/
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

        // only the last paragraph MUST have the terminal null
        GASSERT(GetLastParagraph() != ppara || ppara->HasTermNull());
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
    const Char*                 pElemName;
    UPInt                       ElemNameSize;
    const GFxSGMLElementDesc*   pElemDesc;
    UPInt                       StartPos;
    GFxTextFormat               TextFmt;
    GFxTextParagraphFormat      ParaFmt;

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
    if (!defaultTextFmt.IsUrlSet())
    {
        // This is necessary to prevent URL-style propagation after
        // closing </A> tag.
        defaultTextFmt.SetUrl("");
        SetDefaultTextFormat(defaultTextFmt);
    }
    GFxTextFormat lastFormat = defaultTextFmt;

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
                                    if (pcurPara == NULL)
                                        pcurPara = AppendNewParagraph(&defaultParaFmt);
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
                                                        stackElem.TextFmt.SetColor32(color);    
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
                                                    stackElem.TextFmt.SetFontSize((Float)sz);
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
                                            case GFxHTML_TABSTOPS:
                                                {
                                                    // parse tabstops as [xx,xx,xx]
                                                    // calculate number of tabstops first
                                                    UPInt i = 0;
                                                    while(i < attrValSz && GFxSGMLCharIter<Char>::IsSpace(pattrVal[i]))
                                                        ++i;
                                                    if (pattrVal[i] == '[')
                                                        ++i; // skip the [
                                                    UInt n = 1;
                                                    UPInt j;
                                                    for (j = i; j < attrValSz && pattrVal[j] != ']'; ++j)
                                                    {
                                                        if (pattrVal[j] == ',')
                                                            ++n;
                                                    }
                                                    stackElem.ParaFmt.SetTabStopsNum(n);
                                                    UInt idx = 0;
                                                    for (j = i; j < attrValSz && pattrVal[j] != ']'; )
                                                    {
                                                        UInt v = 0;
                                                        for (; j < attrValSz && GFxSGMLCharIter<Char>::IsDigit(pattrVal[j]); ++j)
                                                        {
                                                            v *= 10;
                                                            v += (pattrVal[j] - '0');
                                                        }
                                                        while(j < attrValSz && GFxSGMLCharIter<Char>::IsSpace(pattrVal[j]))
                                                            ++j;
                                                        stackElem.ParaFmt.SetTabStopsElement(idx, v);
                                                        ++idx;
                                                        if (pattrVal[j] == ',')
                                                        {
                                                            ++j;
                                                            while(j < attrValSz && GFxSGMLCharIter<Char>::IsSpace(pattrVal[j]))
                                                                ++j;
                                                        }
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
                        default: break;
                        }
                    }
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
                                lastFormat = stackElem.TextFmt;
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
    if (elementsStack.size() > 0)
    {
        GFxSGMLStackElemDesc<Char>& stackElem = elementsStack[elementsStack.size()-1];
        SetDefaultTextFormat(stackElem.TextFmt);
        SetDefaultParagraphFormat(stackElem.ParaFmt);
    }
    else
    {
        SetDefaultTextFormat(defaultTextFmt);
        SetDefaultParagraphFormat(defaultParaFmt);
    }
    EnsureTermNull();
    
    // Flash treats empty paragraphs at the end of the specific way. For example, if 
    // HTML code is as follows: 
    // "<p align="left"><font face="Arial" size="20" color="#ffffff" letterSpacing="5" kerning="1">Test 12345</font></p><p align="left"></p>"
    // then the last paragraph should be empty w/o any format data (<p align="left"></p>). Though,
    // Flash propagates the last known text format on the empty paragraphs. The code below does
    // this in GFx.
    GFxTextParagraph* ppara = GetLastParagraph();
    if (ppara && ppara->GetLength() == 0)
    {
        ParagraphsIterator paraIter = Paragraphs.Last();
        while(!paraIter.IsFinished())
        {
            GFxTextParagraph* pcurPara = paraIter->GetPtr();
            if (pcurPara->GetLength() <= 1)
            {
                pcurPara->SetTextFormat(lastFormat, 0, GFC_MAX_UPINT);
            }
            else
                break;
            --paraIter;
        }
    }

    CheckIntegrity();
    return true;
}


//////////////////////////////////
// GFxTextDocView
//
GFxTextDocView::GFxTextDocView(GFxFontManager* pfontMgr) : 
    pFontManager(pfontMgr), pHighlight(NULL)
{
    pDocument = *new DocumentText(this);
    FormatCounter = 1;
    BeginSelection = EndSelection = GFC_MAX_UPINT;
    SetAlignment(Align_Left);
    SetVAlignment(VAlign_None);
    SetTextAutoSize(TAS_None);
    Flags = RTFlags = 0;
    ViewRect.Clear();
    pImageSubstitutor = NULL;
    SetFontScaleFactor(1);
    MaxLength = 0;

    SetAutoFit(); // on by default
    SetDefaultShadow();
    TextWidth = TextHeight = 0;
}

GFxTextDocView::~GFxTextDocView()
{
    Close();
    ClearImageSubstitutor();
    delete pHighlight;
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

void GFxTextDocView::DocumentText::OnParagraphRemoving(const GFxTextParagraph& para)
{
    pDocument->OnDocumentParagraphRemoving(para);
}

void GFxTextDocView::DocumentText::OnTextInserting(UPInt startPos, UPInt length, const wchar_t* ptextInserting)
{
    GUNUSED3(startPos, length, ptextInserting);
    pDocument->OnDocumentChanged(ViewNotify_TextChange);
}

void GFxTextDocView::DocumentText::OnTextInserting(UPInt startPos, UPInt length, const char* ptextInserting)
{
    GUNUSED3(startPos, length, ptextInserting);
    pDocument->OnDocumentChanged(ViewNotify_TextChange);
}

void GFxTextDocView::DocumentText::OnTextRemoving(UPInt startPos, UPInt length)
{
    GUNUSED2(startPos, length);
    pDocument->OnDocumentChanged(ViewNotify_TextChange);
}

UPInt GFxTextDocView::EditCommand(GFxTextDocView::CommandType cmdId, const void* command)
{
    UPInt res = 0;
    switch(cmdId)
    {
    case Cmd_InsertChar:
        {
            GASSERT(command);
            const InsertCharCommand& cmd = *reinterpret_cast<const InsertCharCommand*>(command);
            // check for maxlength
            if (!HasMaxLength() || GetLength() + 1 <= GetMaxLength())
                res = pDocument->InsertString(&cmd.CharCode, cmd.PosAt, 1);
            break;
        }
    case Cmd_InsertPlainText:
        {
            GASSERT(command);
            const InsertPlainTextCommand& cmd = *reinterpret_cast<const InsertPlainTextCommand*>(command);
            UPInt len    = cmd.Length;
            UPInt curLen = GetLength();
            // check for maxlength
            if (HasMaxLength() && curLen + len > GetMaxLength())
                len = GetMaxLength() - curLen;
            res = pDocument->InsertString(cmd.pStr, cmd.PosAt, len, GFxStyledText::NLP_CompressCRLF);
            break;
        }
    case Cmd_InsertStyledText:
        {
            GASSERT(command);
            const InsertStyledTextCommand& cmd = *reinterpret_cast<const InsertStyledTextCommand*>(command);
            GASSERT(cmd.pText);

            UPInt len = GFC_MAX_UPINT;
            // check for maxlength
            if (HasMaxLength())
            {
                UPInt curDstLen = GetLength();   
                UPInt curSrcLen = cmd.pText->GetLength();
                if (curDstLen + curSrcLen > GetMaxLength())
                   len = GetMaxLength() - curDstLen;
            }
            res = pDocument->InsertStyledText(*cmd.pText, cmd.PosAt, len);
            break;
        }
    case Cmd_ReplaceTextByChar:
        {
            GASSERT(command);
            const ReplaceTextByCharCommand& cmd = *reinterpret_cast<const ReplaceTextByCharCommand*>(command);
            UPInt stPos, endPos;
            if (cmd.BeginPos > cmd.EndPos)
            {
                stPos = cmd.EndPos;
                endPos= cmd.BeginPos;
            }
            else
            {
                stPos = cmd.BeginPos;
                endPos= cmd.EndPos;
            }
            UPInt len = 1;                        
            // check for maxlength
            if (HasMaxLength())
            {
                UPInt curDstLen = GetLength() - (endPos - stPos);   
                if (curDstLen + 1 > GetMaxLength())
                    len = 0;
            }
            if (len > 0)
            {
                res = pDocument->InsertString(&cmd.CharCode, stPos, 1, GFxStyledText::NLP_CompressCRLF);
                pDocument->Remove(stPos + 1, endPos + 1);
            }
            break;
        }
    case Cmd_ReplaceTextByPlainText:
        {
            GASSERT(command);
            const ReplaceTextByPlainTextCommand& cmd = *reinterpret_cast<const ReplaceTextByPlainTextCommand*>(command);
            UPInt stPos, endPos;
            if (cmd.BeginPos > cmd.EndPos)
            {
                stPos = cmd.EndPos;
                endPos= cmd.BeginPos;
            }
            else
            {
                stPos = cmd.BeginPos;
                endPos= cmd.EndPos;
            }
            UPInt len = cmd.Length;
            // check for maxlength
            if (HasMaxLength())
            {
                UPInt curDstLen = GetLength() - (endPos - stPos);   
                if (curDstLen + cmd.Length > GetMaxLength())
                    len = GetMaxLength() - curDstLen;
            }
            res = pDocument->InsertString(cmd.pStr, stPos, len, GFxStyledText::NLP_CompressCRLF);
            pDocument->Remove(stPos + res, endPos + res);
            break;
        }
    case Cmd_ReplaceTextByStyledText:
        {
            GASSERT(command);
            const ReplaceTextByStyledTextCommand& cmd = *reinterpret_cast<const ReplaceTextByStyledTextCommand*>(command);
            UPInt stPos, endPos;
            if (cmd.BeginPos > cmd.EndPos)
            {
                stPos = cmd.EndPos;
                endPos= cmd.BeginPos;
            }
            else
            {
                stPos = cmd.BeginPos;
                endPos= cmd.EndPos;
            }
            UPInt len = GFC_MAX_UPINT;
            // check for maxlength
            if (HasMaxLength())
            {
                UPInt curDstLen = GetLength() - (endPos - stPos);   
                if (curDstLen + cmd.pText->GetLength() > GetMaxLength())
                    len = GetMaxLength() - curDstLen;
            }
            res = pDocument->InsertStyledText(*cmd.pText, stPos, len);
            pDocument->Remove(stPos + res, endPos + res);
            break;
        }
    case Cmd_BackspaceChar:
        {
            GASSERT(command);
            res = 1;
            const DeleteCharCommand& cmd = *reinterpret_cast<const DeleteCharCommand*>(command);
            // need to check a special case, if paragraph has indent or bullet and cursor
            // is at the beginning of para - remove these attributes first, before continuing 
            // removing chars
            UPInt indexInPara = 0;
            GFxStyledText::ParagraphsIterator paraIter = 
                pDocument->GetParagraphByIndex(cmd.PosAt, &indexInPara);
            if (!paraIter.IsFinished() && indexInPara == 0)
            {
                GFxTextParagraph* ppara = *paraIter;
                GASSERT(ppara);
                const GFxTextParagraphFormat* pparaFmt = ppara->GetFormat();
                if (pparaFmt)
                {
                    if (pparaFmt->IsBullet())
                    {
                        GFxTextParagraphFormat newFmt = *pparaFmt;
                        newFmt.SetBullet(false);
                        ppara->SetFormat(newFmt);
                        OnDocumentChanged(ViewNotify_TextChange);
                        res = 0;
                    }
                    else if (pparaFmt->GetIndent() != 0 || pparaFmt->GetBlockIndent() != 0)
                    {
                        GFxTextParagraphFormat newFmt = *pparaFmt;
                        newFmt.SetIndent(0);
                        newFmt.SetBlockIndent(0);
                        ppara->SetFormat(newFmt);
                        OnDocumentChanged(ViewNotify_TextChange);
                        res = 0;
                    }
                }
            }
            if (res == 1 && cmd.PosAt > 0) // res wasn't zeroed - means we need to remove char
            {
                pDocument->Remove(cmd.PosAt - 1, cmd.PosAt);
            }
            else
                res = 0;
            break;
        }
    case Cmd_DeleteChar:
        {
            GASSERT(command);
            const DeleteCharCommand& cmd = *reinterpret_cast<const DeleteCharCommand*>(command);
            pDocument->Remove(cmd.PosAt, cmd.PosAt + 1);
            res = 1;
            break;
        }
    case Cmd_DeleteText:
        {
            GASSERT(command);
            const DeleteTextCommand& cmd = *reinterpret_cast<const DeleteTextCommand*>(command);
            UPInt stPos, endPos;
            if (cmd.BeginPos > cmd.EndPos)
            {
                stPos = cmd.EndPos;
                endPos= cmd.BeginPos;
            }
            else
            {
                stPos = cmd.BeginPos;
                endPos= cmd.EndPos;
            }
            //?SetDefaultTextAndParaFormat(stPos);
            pDocument->Remove(stPos, endPos);
            res = endPos - stPos;
            break;
        }
    default: break;
    }
    return res;
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
        strLen = gfc_wcslen(pwStr); 
    pDocument->ParseHtml<wchar_t>(pwStr, strLen, pimgInfoArr, IsMultiline(), condenseWhite);
    OnDocumentChanged(ViewNotify_SignificantTextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::AppendText(const wchar_t* pwStr, UPInt strLen)
{
    const GFxTextFormat* ptxtfmt;
    const GFxTextParagraphFormat* pparafmt;
    UPInt len = pDocument->GetLength();
    if (len > 0 && pDocument->GetTextAndParagraphFormat(&ptxtfmt, &pparafmt, len - 1))
        pDocument->AppendString(pwStr, strLen, GetNewLinePolicy(), ptxtfmt, pparafmt);
    else
        pDocument->AppendString(pwStr, strLen, GetNewLinePolicy());
    OnDocumentChanged(ViewNotify_TextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::AppendText(const char* putf8Str, UPInt utf8Len)
{
    const GFxTextFormat* ptxtfmt;
    const GFxTextParagraphFormat* pparafmt;
    UPInt len = pDocument->GetLength();
    if (len > 0 && pDocument->GetTextAndParagraphFormat(&ptxtfmt, &pparafmt, len - 1))
        pDocument->AppendString(putf8Str, utf8Len, GetNewLinePolicy(), ptxtfmt, pparafmt);
    else
        pDocument->AppendString(putf8Str, utf8Len, GetNewLinePolicy());
    OnDocumentChanged(ViewNotify_TextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::AppendHtml(const wchar_t* pwStr, UPInt strLen, bool condenseWhite)
{
    if (strLen == GFC_MAX_UPINT)
        strLen = gfc_wcslen(pwStr); 
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

UPInt GFxTextDocView::InsertText(const wchar_t* pstr, UPInt startPos, UPInt strLen)
{
    return pDocument->InsertString(pstr, startPos, strLen);
}

UPInt GFxTextDocView::ReplaceText(const wchar_t* pstr, UPInt startPos, UPInt endPos, UPInt strLen)
{
    pDocument->Remove(startPos, endPos);
    UPInt len = pDocument->InsertString(pstr, startPos, strLen);
    //OnDocumentChanged(ViewNotify_TextChange);
    return len;
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

void GFxTextDocView::SetViewRect(const GRectF& rect, UseType ut)
{
    if (rect != ViewRect)
    {
        UInt oldW = (UInt)ViewRect.Width();
        ViewRect = rect;
        GRectF textRect = ViewRect;
        textRect.Contract(GFX_TEXT_GUTTER);
        LineBuffer.SetVisibleRect(textRect);
        if (ut == UseExternally)
        {
            // if WordWrap is on and width has been changed then we need 
            // reformat. Otherwise, we just need to update the 
            // maxhscroll and maxvscroll values
            if (IsWordWrap() && oldW != (UInt)ViewRect.Width())
                SetCompleteReformatReq();
            else
            {
                ++FormatCounter; // invalidate cached values to force recalc
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
            }
        }
    }
}

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
                //pfontInfo->pCurrentFont->FontScaleFactor = 1.3f;
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

                    //pfontInfo->pCurrentFont->FontScaleFactor = 1.3f;
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

class GFxLineCursor
{
public:
    GFxTextLineBuffer::GlyphEntry*  pPrevGrec;                  // used for kerning
    GPtr<GFxFontHandle>         pLastFont;                      // used for kerning and line width correction
    UInt                        LastCharCode;                   // used for kerning

    UInt                        LastGlyphIndex;                 // used to correct line width
    Float                       LastAdvance;                    // used to correct line width
    SInt                        LastGlyphWidth;                 // used to correct line width
    UInt32                      LastColor;

#ifndef GFC_NO_IME_SUPPORT
    // composition string
    GPtr<GFxTextCompositionString>  pComposStr;
#endif
    UPInt                           ComposStrPosition;
    UPInt                           ComposStrLength;
    UPInt                           ComposStrCurPos;

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
    Float                       FontScaleFactor;                // contains current font scale factor
    bool                        LastKerning;
    bool                        LineHasNewLine;

    UPInt                       NumChars; // num chars processed

    GFxLineCursor(): pPrevGrec(NULL), pLastFont(NULL), LastCharCode(0), 
        LastGlyphIndex(0), LastAdvance(0), LastGlyphWidth(0), LastColor(0), 
        ComposStrPosition(GFC_MAX_UPINT), ComposStrLength(0),ComposStrCurPos(0),
        pDocView(NULL), pParagraph(NULL), 
        LineWidth(0), LineWidthWithoutTrailingSpaces(0), LineLength(0), 
        MaxFontAscent(0), MaxFontDescent(0), MaxFontLeading(0), Indent(0), 
        LeftMargin(0), RightMargin(0), NumOfSpaces(0), NumOfTrailingSpaces(0),
        FontScaleFactor(1.f), LastKerning(false), LineHasNewLine(false), NumChars(0)
    { }
    GFxLineCursor(GFxTextDocView* pview, const GFxTextParagraph* ppara, const GFxTextParagraph::CharactersIterator& chIter):
        pPrevGrec(NULL), pLastFont(NULL), LastCharCode(0), LastGlyphIndex(0), LastAdvance(0), 
        LastGlyphWidth(0), LastColor(0), 
        ComposStrPosition(GFC_MAX_UPINT), ComposStrLength(0), ComposStrCurPos(0),
        pDocView(pview), pParagraph(ppara), LineWidth(0), 
        LineWidthWithoutTrailingSpaces(0), LineLength(0),
        MaxFontAscent(0), MaxFontDescent(0), MaxFontLeading(0),
        CharIter(chIter), Indent(0), LeftMargin(0), RightMargin(0), NumOfSpaces(0), 
        NumOfTrailingSpaces(0), FontScaleFactor(1.f), 
        LastKerning(false), LineHasNewLine(false), NumChars(0)
    { }

    bool IsFinished() const { return CharIter.IsFinished(); }
    
    void operator++() 
    { 
        (*this) += 1;
    }
    void operator+=(UInt n) 
    { 
#ifndef GFC_NO_IME_SUPPORT
        if (pComposStr && pComposStr->GetLength() > 0)
        {
            UPInt newN = n;
            UPInt curTextPos = CharIter.GetCurTextIndex() + pParagraph->GetStartIndex();
            if (curTextPos     <= pComposStr->GetPosition() && 
                curTextPos + n >= pComposStr->GetPosition())
            {
                // position of composition string is in the range
                newN = GTL::gpmin(pComposStr->GetPosition() - curTextPos, (UPInt)n);

                if (ComposStrCurPos + (n - newN) > pComposStr->GetLength())
                {
                    newN += ComposStrCurPos + (n - newN) - pComposStr->GetLength();
                    ComposStrCurPos = pComposStr->GetLength();
                    NumChars        += ComposStrCurPos;
                }
                else
                {
                    ComposStrCurPos += (n - newN);
                    NumChars        += (n - newN);
                }
                n = (UInt)newN;
            }
        }
#endif //#ifndef GFC_NO_IME_SUPPORT
        if (n > 0)
        {
            CharIter += n; 
            NumChars += n;
        }
    }
    bool IsInsideComposStr() const
    {
#ifndef GFC_NO_IME_SUPPORT
        if (pComposStr && pComposStr->GetLength() > 0)
        {
            UPInt curTextPos = CharIter->Index + pParagraph->GetStartIndex();
            if (curTextPos >= pComposStr->GetPosition())
            {
                if (ComposStrCurPos < pComposStr->GetLength())
                {
                    return true;
                }
            }
        }
#endif //#ifndef GFC_NO_IME_SUPPORT
        return false;
    }
    const GFxTextParagraph::CharacterInfo& operator*() 
    { 
        CharInfoHolder.Index   = CharIter->Index;
#ifndef GFC_NO_IME_SUPPORT
        if (pComposStr && pComposStr->GetLength() > 0)
        {
            UPInt curTextPos = CharInfoHolder.Index + pParagraph->GetStartIndex();
            if (curTextPos >= pComposStr->GetPosition())
            {
                if (curTextPos == pComposStr->GetPosition() && ComposStrCurPos < pComposStr->GetLength())
                {
                    CharInfoHolder.Index = CharIter->Index + ComposStrCurPos;
                    CharInfoHolder.Character = pComposStr->GetText()[ComposStrCurPos];
                    CharInfoHolder.pFormat = *pComposStr->GetAllocator()->AllocateTextFormat
                        (CharIter->pFormat->Merge(*pComposStr->GetTextFormat(ComposStrCurPos)));
                    return CharInfoHolder;
                }
                else
                {
                    CharInfoHolder.Index = CharIter->Index + pComposStr->GetLength();    
                }
            }
        }
#endif //#ifndef GFC_NO_IME_SUPPORT

        CharInfoHolder.pFormat = CharIter->pFormat;
        if (pDocView->IsPasswordMode() && CharIter->Character != '\0')
            CharInfoHolder.Character = '*';
        else
            CharInfoHolder.Character = CharIter->Character;

        return CharInfoHolder;
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
        NumChars      = 0;
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

inline Float GFxTextDocView::GetActualFontSize(const GFxTextDocView::FindFontInfo& findFontInfo, GFxTextDocView::FormatParagraphInfo* pinfo)
{
    Float fontSize = findFontInfo.pCurrentFormat->GetFontSize();
    if (findFontInfo.pCurrentFont->FontScaleFactor != 1.0f)
    {
        fontSize *= findFontInfo.pCurrentFont->FontScaleFactor;
        if (!IsMultiline())
            pinfo->ForceVerticalCenterAutoSize = true;
    }
    if (HasFontScaleFactor())
        fontSize *= GetFontScaleFactor();
    return fontSize;
}

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

    if (paraFormat.IsRightAlignment() || paraFormat.IsCenterAlignment())
    {
        // if text is right or center aligned and RightMargin is not zero - 
        // add the margin to line width.
        lineCursor.LineWidth += lineCursor.RightMargin;
    }

    /*if (lineCursor.pParagraph->GetLength() == 0) // not necessary anymore since we always have at least one char (EOF) DEL!
    {
        // a special case when paragraph is empty.
        // in this case we need to calculate at least the height of the line.
        // To do this, we need to have MaxFontAscent & Descent set correctly.
        FindFontInfo findFontInfo(pinfo->pFontCache);

        findFontInfo.pCurrentFormat = lineCursor.pDocView->GetDefaultTextFormat();
        GASSERT(findFontInfo.pCurrentFormat);
        GPtr<GFxFontHandle> pfontHandle = lineCursor.pDocView->FindFont(&findFontInfo);
        GASSERT(pfontHandle);

        Float fontSize = GetActualFontSize(findFontInfo, pinfo); 
        Float scale = PixelsToTwips(fontSize) / 1024.0f; // the EM square is 1024 x 1024   
        lineCursor.TrackFontParams(pfontHandle->GetFont(), scale);
    }*/

    Float fleading   = (paraFormat.IsLeadingSet()) ? PixelsToTwips(paraFormat.GetLeading()) : lineCursor.MaxFontLeading; 
    Float lineHeight = lineCursor.MaxFontAscent + lineCursor.MaxFontDescent;
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
    if (GetTextAutoSize() == TAS_None && // if auto text fit is on - always use long form
        lineCursor.LineLength <= 255 && glyphsCount <= 255 && (leading >= -128 && leading <= 127) &&
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
        SInt deltaW = (SInt(GetTextRect().Width() - GFX_TEXT_JUSTIFY_CORRECTION) - lineCursor.RightMargin) - 
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
    pcurLine->SetDimensions(lineCursor.LineWidth, SInt(lineHeight));
    pcurLine->SetLeading(leading);

    switch(paraFormat.GetAlignment())
    {
    case GFxTextParagraphFormat::Align_Right:   
        pcurLine->SetAlignment(GFxTextLineBuffer::Line::Align_Right); 

        // align line accordingly to ViewRect
        pcurLine->SetOffsetX(GTL::gmax(0.f, GetTextRect().Width() - lineCursor.LineWidthWithoutTrailingSpaces));

        if (IsAutoSizeX())
        {
            pinfo->NeedRecenterLines = true;
        }
        break;
    case GFxTextParagraphFormat::Align_Center:  
    {
        pcurLine->SetAlignment(GFxTextLineBuffer::Line::Align_Center); 

        // align line accordingly to ViewRect and LeftMargin. RightMargin is already counted
        // at the top of this func.
        Float adjTextRect = GetTextRect().Width()  - Float(lineCursor.LeftMargin);
        pcurLine->SetOffsetX (GTL::gmax(0.f, adjTextRect / 2 - pcurLine->GetWidth() / 2) + lineCursor.LeftMargin);

        if (IsAutoSizeX())
        {
            pinfo->NeedRecenterLines = true;
        }
        break;
    }
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

    GASSERT(paragraph.GetFormat());
    const GFxTextParagraphFormat& paraFormat = *paragraph.GetFormat();
    GFxLineCursor lineCursor(this, &paragraph, pinfo->CharIter);
    GFxLineCursor wordWrapPoint;

    lineCursor.FontScaleFactor = GetFontScaleFactor();

    // Check, do we have a composition string somewhere in the paragraph
    if (HasEditorKit() && GetEditorKit()->HasCompositionString())
    {
#ifndef GFC_NO_IME_SUPPORT
        lineCursor.pComposStr           = GetEditorKit()->GetCompositionString();
        lineCursor.ComposStrPosition    = lineCursor.pComposStr->GetPosition();
        lineCursor.ComposStrLength      = lineCursor.pComposStr->GetLength();
#endif //#ifndef GFC_NO_IME_SUPPORT
    }

    // try to estimate how much memory would be necessary for temporary line
    // Assume, glyphCount <= paragraph.GetLength()
    //         fmtDataElemCount <= paragraph.GetLength()*2*paragraph.GetLength()
    UPInt maxGlyphCount = paragraph.GetSize() + lineCursor.ComposStrLength;
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
    ptempLine->SetTextPos(UInt(TextPos2GlyphOffset(paragraph.GetStartIndex())));

    lineCursor.GlyphIns = GFxTextLineBuffer::GlyphInserter(ptempLine->GetGlyphs(), ptempLine->GetNumGlyphs(), ptempLine->GetFormatData());

    pinfo->ParaHeight  = pinfo->ParaWidth = 0;
    pinfo->ParaLines   = 0;

    //GFxTextParagraph::CharactersIterator savedCharIter;
    GRectF glyphBounds;

    if (paraFormat.IsBullet())
    {
        // add bullet symbol.
        GFxTextLineBuffer::GlyphEntry& grec = lineCursor.GlyphIns.GetGlyph();

        const GFxTextParagraph::CharacterInfo& chInfo = *lineCursor;
        findFontInfo.pCurrentFormat = chInfo.pFormat;
        if (findFontInfo.pCurrentFormat)
        {
            // it is possible that there is no format info if paragraph
            // is empty.
            GPtr<GFxFontHandle> pfontHandle = FindFont(&findFontInfo);
            GASSERT(pfontHandle);

            int glyphIndex = pfontHandle->GetFont()->GetGlyphIndex(GFX_BULLET_CHAR_CODE);
            grec.SetIndex(glyphIndex);
            grec.SetAdvance(PixelsToTwips(GFX_BULLET_CHAR_ADVANCE));
            Float fontSize = GetActualFontSize(findFontInfo, pinfo)*2/3; 
            grec.SetFontSize(fontSize);
            grec.SetLength(0);

            UInt32 color = findFontInfo.pCurrentFormat->GetColor32();
            lineCursor.GlyphIns.AddFont(pfontHandle);
            lineCursor.GlyphIns.AddColor(color);
            ++lineCursor.GlyphIns;

            lineCursor.pLastFont    = pfontHandle;
            lineCursor.LastColor    = color;
        }
        lineCursor.Indent       = PixelsToTwips(GFX_BULLET_PARA_INDENT);
        lineCursor.LineWidth    = PixelsToTwips(GFX_BULLET_CHAR_ADVANCE);
    }
    else
        lineCursor.Indent = PixelsToTwips(paraFormat.GetIndent());
    lineCursor.LeftMargin  = PixelsToTwips(paraFormat.GetLeftMargin() + paraFormat.GetBlockIndent());
    lineCursor.RightMargin = PixelsToTwips(paraFormat.GetRightMargin());
    UInt tabStopsNum = 0, tabStopsIndex = 0;
    const UInt* const ptabStops = paraFormat.GetTabStops(&tabStopsNum);

    UInt deltaText  = 1;
    GPtr<GFxTextImageDesc> pimgDesc;
    bool    isSpace = false;
    for(;!lineCursor.IsFinished(); lineCursor += deltaText)
    {
        if (IsWordWrap() && !pimgDesc)
        {
            if (lineCursor.LastCharCode == '-' || isSpace) // also a word separator
            {
                wordWrapPoint = lineCursor;
            }
        }
        const GFxTextParagraph::CharacterInfo& chInfo = *lineCursor;
        deltaText = 1;

        pimgDesc = NULL;
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
        
        if (!pimgDesc)
        {
            GASSERT(chInfo.pFormat);
            if (chInfo.pFormat && chInfo.pFormat->IsImageDescSet() && chInfo.Character != 0)
            {
                pimgDesc = chInfo.pFormat->GetImageDesc();
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
        Float   fontSize = -1;
        bool    isNbsp;
        if (!pimgDesc)
        {
            isNbsp  = (chInfo.Character == GFX_NBSP_CHAR_CODE);
            isSpace = (chInfo.Character == 0 || (!isNbsp && gfc_iswspace(chInfo.Character) != 0));

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
            fontSize = GetActualFontSize(findFontInfo, pinfo); 
            scale = PixelsToTwips(fontSize) / 1024.0f; // the EM square is 1024 x 1024   
            if (chInfo.Character == NewLineChar() || chInfo.Character == '\0')
            {
                // ok, the end of the paragraph and the line. Add a fake glyph with 
                // half of space's width to the buffer
                glyphIndex = pfont->GetGlyphIndex(' ');
                #ifdef GFX_DEBUG_DISPLAY_INVISIBLE_CHARS
                if (chInfo.Character == '\0')
                    glyphIndex = pfont->GetGlyphIndex(0xA4);
                else
                    glyphIndex = pfont->GetGlyphIndex(0xB6);
                #endif
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
                
                lastAdvance = AlignTwipsToPixel(lineCursor.LastAdvance + prevGlyphAdvanceAdj);
            }
            else
            {
                lastAdvance = AlignTwipsToPixel(lineCursor.LastAdvance);
            }
            glyphWidth   = AlignTwipsToPixel(GTL::gmax(
                pfont->GetGlyphBounds(glyphIndex, &glyphBounds).Right * scale + 20, glyphAdvance));

            // the space shouldn't be taken into account when checking the lineWidth for
            // intersection with viewrect. Though, the space's width still should be added
            // to the lineWidth, even if it is a "word-wrapping" point.
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
            isNbsp          = false;
        }

        SInt newLineWidth = lineCursor.LineWidth + lastAdvance;

        // Do word-wrapping?
        // use a real width of last glyph to test for intersection with viewrect
        if ((IsWordWrap() && !isSpace && 
            newLineWidth + adjLineWidth + lineCursor.Indent + lineCursor.LeftMargin > 
            GetTextRect().Width() - lineCursor.RightMargin))
        {
            newLineWidth = 0;
            
            bool wordWrap = false;
            UPInt newTextPos;

            // if wordWrap - cut the last word
            if (!wordWrapPoint.IsFinished())
            {
                GFxTextLineBuffer::GlyphInserter ins = wordWrapPoint.GlyphIns;
                lineCursor.GlyphIns.ResetTo(ins);

                lineCursor              = wordWrapPoint;
                lineCursor.LastCharCode = 0;
                isSpace                 = false;
                wordWrapPoint           = GFxLineCursor();
                wordWrap                = true;
                deltaText               = 0;
            }
            newTextPos = paragraph.GetStartIndex() + (*lineCursor).Index;

            // mark the separator char at the end of the previous line
            if (lineCursor.pPrevGrec)
            {
                lineCursor.pPrevGrec->SetWordWrapSeparator();
            }

            // finalize the line
            FinalizeLine(ptempLine, pinfo, paraFormat, lineCursor);

            // reset values for the next line
            lineCursor.Reset();
            tabStopsIndex = 0; // reset tabStops index. Actually Flash 8 doesn't do this
                               // Flash 8 applies tabStops only to the first line.
                               // Flash 9 resets tabStops index for each line.

            GASSERT(ptempLine);
            
            ptempLine->SetTextPos((UInt)newTextPos);

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
        if (lineCursor.IsInsideComposStr())
            grec.SetComposStrGlyph();
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

            lineCursor.NumOfTrailingSpaces = 0;
            lineCursor.LineWidthWithoutTrailingSpaces = newLineWidth + adjLineWidth;
        }
        else
        {
            UInt32 color  = findFontInfo.pCurrentFormat->GetColor32();
            //Float fontSize = findFontInfo.pCurrentFormat->GetFontSize(lineCursor.FontScaleFactor);
            GASSERT(fontSize >= 0);
            grec.SetFontSize(fontSize);

            if (pfontHandle != lineCursor.pLastFont)
                lineCursor.GlyphIns.AddFont(pfontHandle);
            if (color != lineCursor.LastColor)
                lineCursor.GlyphIns.AddColor(color);

            if (chInfo.Character == NewLineChar() || chInfo.Character == '\0')
            {
                // special symbol at the end of paragraph. Set its length to 0.
                // it also shouldn't participate in textWidth's calculation.
                // but it should be taken into account when drawing background 
                // highlighting.
                if (chInfo.Character == '\0')
                    grec.SetLength(0);
                else
                    grec.SetLength(1); //?????????????? //!
                grec.SetInvisibleChar();
                #ifdef GFX_DEBUG_DISPLAY_INVISIBLE_CHARS
                grec.ClearInvisibleChar();
                #endif
                grec.SetNewLineChar();
                glyphWidth = 0;
                lineCursor.LineHasNewLine = true;

                // gather dimensions for the line, ONLY if this is the first char
                if (chInfo.Index == 0)
                    lineCursor.TrackFontParams(pfont, scale);

            }
            else
            {
                if (isSpace || isNbsp)
                    grec.SetInvisibleChar();
                if (isSpace)
                {
                    ++lineCursor.NumOfSpaces;
                    ++lineCursor.NumOfTrailingSpaces;
                }
                else
                {
                    lineCursor.NumOfTrailingSpaces = 0;
                    lineCursor.LineWidthWithoutTrailingSpaces = newLineWidth + adjLineWidth;
                }
                grec.SetLength(1);

                // gather dimensions for the line
                lineCursor.TrackFontParams(pfont, scale);

            }
            if (findFontInfo.pCurrentFormat->IsUnderline())
                grec.SetUnderline();
            else
                grec.ClearUnderline();
            if (findFontInfo.pCurrentFormat->IsUrlSet())
                grec.SetUrl();
            else
                grec.ClearUrl();

            lineCursor.pLastFont        = pfontHandle;
            lineCursor.LastColor        = color;
            lineCursor.LastKerning      = findFontInfo.pCurrentFormat->IsKerning();
        }

        // check for tabs
        if (tabStopsIndex < tabStopsNum && chInfo.Character == '\t')
        {
            SInt nextTabStopPos = (SInt)PixelsToTwips(ptabStops[tabStopsIndex]);
            if (nextTabStopPos > newLineWidth)
            {
                glyphAdvance = Float(nextTabStopPos - newLineWidth);
            }
            ++tabStopsIndex;
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

    UInt oldMaxHScroll = GetMaxHScrollValue();
    UInt oldMaxVScroll = GetMaxVScrollValue();

    // if auto text fit feature is on, then we need to reformat everything
    // to be able to scale it correctly.
    if (GetTextAutoSize() != TAS_None)
        SetCompleteReformatReq();

    GFxStyledText::ParagraphsIterator paraIter = pstyledText->GetParagraphIterator();
    GFxTextLineBuffer::Iterator linesIt = LineBuffer.Begin();

    FormatParagraphInfo paraInfo;
    paraInfo.pFontCache         = &fontCache;
    paraInfo.ParaYOffset        = 0;
    paraInfo.NextOffsetY        = 0;
    paraInfo.NeedRecenterLines  = false;
    paraInfo.ForceVerticalCenterAutoSize = false;
    paraInfo.pLog               = plog;
    SInt textWidth = 0, textHeight = 0;

    while(!paraIter.IsFinished())
    {
        const GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        if (!linesIt.IsFinished())
        {
            GFxTextLineBuffer::Line& curLine = *linesIt;

            if (!IsCompleteReformatReq() && 
                 ppara->GetId() == curLine.GetParagraphId() && 
                 ppara->GetModCounter() == curLine.GetParagraphModId())
            {
                // if paragraph is not changed, then just update yoffset & textpos of each line
                //UPInt pos = ppara->GetStartIndex();
                UPInt pos = TextPos2GlyphOffset(ppara->GetStartIndex());
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
                    // In Flash, the last empty line doesn't participate in textHeight,
                    // unless this is editable text.
                    if (ppara->GetLength() != 0 || (HasEditorKit() && !GetEditorKit()->IsReadOnly()))
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

        // In Flash, the last empty line doesn't participate in textHeight,
        // unless this is editable text.
        if (ppara->GetLength() != 0 || (HasEditorKit() && !GetEditorKit()->IsReadOnly()))
            textHeight = SInt(paraInfo.ParaYOffset + paraInfo.ParaHeight);

        ++paraIter;
    }
    // now need to remove all remaining lines in line buffer
    if (!linesIt.IsFinished())
    {
        linesIt.Remove(LineBuffer.size() - linesIt.GetIndex());
        LineBuffer.InvalidateCache();
    }

    // assign new values for TextWidth & TextHeight
    GASSERT(textWidth >= 0 && textHeight >= 0);
    TextWidth = (UInt32)textWidth;
    TextHeight = (UInt32)textHeight;

    //    LineBuffer.Dump();

    ++FormatCounter; // force textWidth, etc format dependent values to be recalculated

    ClearReformatReq();

    // handle autosize
    bool valignApplied = false;
    ViewVAlignment valign = GetVAlignment();
    if (IsAutoSizeX() || IsAutoSizeY() || paraInfo.ForceVerticalCenterAutoSize)
    {
        GRectF newViewRect = ViewRect;
        if (IsAutoSizeX())
        {
            Float newW = TextWidth + GFX_TEXT_GUTTER*2;

            switch(GetAlignment())
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

        if (IsAutoSizeY() || paraInfo.ForceVerticalCenterAutoSize)
        {
            Float newH = TextHeight + GFX_TEXT_GUTTER*2;
            if ((valign == VAlign_None && paraInfo.ForceVerticalCenterAutoSize) || valign == VAlign_Center)
            {
                newViewRect.Top += newViewRect.Height()/2 - newH/2;
            }
            else if (valign == VAlign_Bottom)
            {
                newViewRect.Top = newViewRect.Bottom - newH;
            }
            newViewRect.SetHeight(newH);
            valignApplied = true;
        }

        if (newViewRect != ViewRect)
        {
            SetViewRect(newViewRect, UseInternally);

            if (paraInfo.NeedRecenterLines)
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
    }

    // handle auto text fit here. Calculate the scale and recalculate 
    // all dimensions and advances accordingly.
    ViewTextAutoSize tas;
    if ((tas = GetTextAutoSize()) != TAS_None)
    { 
        GRectF maxTextRect = ViewRect;
        maxTextRect.Contract(GFX_TEXT_GUTTER);

        if (!IsMultiline() && valign == VAlign_None)
        {
            // if valign is not set but textAutoSize is specified for single line edit - 
            // use "center" by default. This behavior might be overridden by explicit
            // setting of valign property.
            valign = VAlign_Center;
        }
        Float scale = 1.f;
        Float hscale = 1.f, vscale = 1.f;
        if (tas == TAS_Shrink)
        {
            // try to fit the text into the text rect
            if (textWidth > maxTextRect.Width())
            {
                hscale = maxTextRect.Width()/textWidth;
            }
            if (textHeight > maxTextRect.Height())
            {
                vscale = maxTextRect.Height()/textHeight;
            }
            scale = GTL::gmin(vscale, hscale);
        }
        else if (tas == TAS_Fit)
        {
            // need either expand or shrink the text to fill the text rect
            hscale = maxTextRect.Width()/textWidth;
            vscale = maxTextRect.Height()/textHeight;
            scale = GTL::gmin(vscale, hscale);
        }
        if (scale != 1.f)
        {
            // check limits for scaling
            SInt minLineH = LineBuffer.GetMinLineHeight();
            if (minLineH > 0)
            {
                Float fmlh = Float(minLineH);
                if (fmlh*scale < GFX_MIN_LINE_HEIGHT_FOR_AUTOSIZE)
                    scale = GFX_MIN_LINE_HEIGHT_FOR_AUTOSIZE/fmlh;
            }
            TextWidth  = (UInt32)(textWidth  * scale);
            TextHeight = (UInt32)(textHeight * scale);
            LineBuffer.Scale(scale);
        }
    }
    // handle vertical alignment. If vertical auto size is not on, but
    // the vertical alignment is specified then align the line buffer INSIDE the
    // existing view rectangle w/o changing view rectangle (vertical auto size
    // changes both view and text rectangles)
    if (!valignApplied && valign != VAlign_None && !IsAutoSizeY())
    {
        GRectF maxTextRect = ViewRect;
        maxTextRect.Contract(GFX_TEXT_GUTTER);

        GRectF newTextRect = GetTextRect();
        Float textHeight = Float(TextHeight);
        if (textHeight < maxTextRect.Height())
        {
            switch(valign)
            {
            case VAlign_Center:
                newTextRect.Top = maxTextRect.Height()/2 - textHeight/2;
                break;
            case VAlign_Bottom:
                newTextRect.Top = maxTextRect.Bottom - textHeight;
                break;
            default: break;
            }
            LineBuffer.SetVisibleRect(newTextRect);
        }
        else
        {
            LineBuffer.SetVisibleRect(maxTextRect);
        }
    }
    GASSERT(!IsReformatReq());

    // also, we need to correct scrolling values, if they are out of range after re-formatting
    UInt maxHScroll = GetMaxHScroll();
    bool needOnScroller = false;
    if(GetHScrollOffset() > maxHScroll)
    {
        SetHScrollOffset(maxHScroll);
    }
    else if (maxHScroll != oldMaxHScroll)
    {
        needOnScroller = true;   
    }
    UInt maxVScroll = GetMaxVScroll();
    if(GetVScrollOffset() > maxVScroll)
    {
        SetVScrollOffset(maxVScroll);
    }
    else if (maxVScroll != oldMaxVScroll)
    {
        needOnScroller = true;   
    }
    if (pDocumentListener)
    {   
        if (needOnScroller && pDocumentListener->DoesHandleOnMaxScrollChanged())
            pDocumentListener->View_OnMaxScrollChanged(*this);
        if (pDocumentListener->DoesHandleOnViewChanged())
            pDocumentListener->View_OnChanged(*this);
    }

    GASSERT(!IsReformatReq());
    LineBuffer.CheckIntegrity();

    if (pHighlight)
    {
        pHighlight->HighlightManager.UpdateGlyphIndices
            ((HasEditorKit()) ? GetEditorKit()->GetCompositionString() : NULL);
    }
}

void    GFxTextDocView::PreDisplay(GFxDisplayContext &context,
                                   const Matrix& mat, const Cxform& cx,
                                   bool focused)
{
    if (HasEditorKit() && focused)
        GetEditorKit()->PreDisplay(context, mat, cx);
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
    if (pHighlight)
    {
        pHighlight->DrawBackground(*this, context, mat, cx);
        LineBuffer.Display(context, mat, cx, nextFrame, param, &pHighlight->HighlightManager);
    }
    else
    {
        LineBuffer.Display(context, mat, cx, nextFrame, param);
    }
}

void    GFxTextDocView::PostDisplay(GFxDisplayContext &context,
                                    const Matrix& mat, const Cxform& cx,
                                    bool focused)
{
    GUNUSED4(context, mat, cx, focused);
    if (pHighlight)
    {
        pHighlight->HighlightManager.Validate();
    }
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
        return Float(TextWidth);
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
        return Float(TextHeight);
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

UInt GFxTextDocView::GetMaxHScrollValue() const
{
    if (IsWordWrap())
        return 0;
    SInt editorDelta = (HasEditorKit()) ? GFX_EDIT_HSCROLL_DELTA : 0;
    UInt v = UInt(GTL::gmax(0.f, ((TextWidth > 0) ? Float(TextWidth) : 0) - 
        GetTextRect().Width() + editorDelta));
    return v;
}

UInt GFxTextDocView::GetMaxVScrollValue() const
{
    return MaxVScroll.Value;
}

UInt GFxTextDocView::GetMaxHScroll()
{
    // check if reformatting is pending. If yes - force reformat first
    if (IsReformatReq())
    {
        Format();
        ClearReformatReq();
    }
    return GetMaxHScrollValue();
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
            UInt l = 0;
            GFxTextLineBuffer::Iterator linesIt = LineBuffer.Last();
            GASSERT(!linesIt.IsFinished());
            const GRectF& textRect = GetTextRect();

            const GFxTextLineBuffer::Line& bottomLine = *linesIt;
            SInt lineH = bottomLine.GetHeight() + bottomLine.GetNonNegLeading();
            Float top = textRect.Top + (bottomLine.GetOffsetY() + lineH - textRect.Bottom);

            for(; !linesIt.IsFinished(); --linesIt, ++l)
            {
                const GFxTextLineBuffer::Line& line = *linesIt;

                if (l > 0 && line.GetOffsetY() < top)
                    break;
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

UPInt GFxTextDocView::GetCursorPosInLine(UInt lineIndex, Float x)
{
    if (lineIndex >= LineBuffer.size()) 
        return GFC_MAX_UPINT;
    ForceReformat();
    //?x -= LineBuffer.Geom.VisibleRect.Left - ViewRect.Left;
    return GetCursorPosInLineByOffset(lineIndex, x);
}

UPInt GFxTextDocView::GetCursorPosInLineByOffset(UInt lineIndex, Float relativeOffsetX)
{
    if (lineIndex >= LineBuffer.size()) 
        return GFC_MAX_UPINT;
    GFxTextLineBuffer::Line& line = LineBuffer[lineIndex];

    //?? if (relativeOffsetX >= line.Offset.x && relativeOffsetX <= line.Offset.x + Float(line.Width()))
    {
        Float xoffInLine = relativeOffsetX - line.GetOffsetX() + LineBuffer.GetHScrollOffset();
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
            if (!ge.IsNewLineChar())
                i += ge.GetLength();
        }

        return line.GetTextPos() + i;
    }
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
        if (y <= 0)
            it = LineBuffer.Begin();
        else
            // if it.IsFinished, then try to get iterator pointed to the last line
            it = LineBuffer.Begin() + LineBuffer.size() - 1;
    }
    if (!it.IsFinished())
    {
        return GetCursorPosInLineByOffset(it.GetIndex(), x);
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

void GFxTextDocView::SetFontScaleFactor(Float f) 
{ 
    if (f == 1) 
    {
        RTFlags &= ~RTFlags_HasFontScaleFactor;
        FontScaleFactor = PixelsToTwips(UInt16(1));
    }
    else
    {
        RTFlags |= RTFlags_HasFontScaleFactor;
        FontScaleFactor = (UInt16)PixelsToTwips(f); 
    }
}

// Returns the zero-based index value of the line at the point specified by the x  and y parameters.
UInt GFxTextDocView::GetLineIndexAtPoint(Float x, Float y)
{
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
    ForceReformat();
    GFxTextLineBuffer::Iterator it = LineBuffer.Begin() + lineIndex;
    if (!it.IsFinished())
    {
        return it->GetTextPos();
    }
    return GFC_MAX_UPINT;
}

// Returns the number of characters in a specific text line.
UPInt GFxTextDocView::GetLineLength(UInt lineIndex, bool* phasNewLine)
{
    ForceReformat();
    GFxTextLineBuffer::Iterator it = LineBuffer.Begin() + lineIndex;
    if (!it.IsFinished())
    {
        if (phasNewLine)
            *phasNewLine = it->HasNewLine();
        return it->GetTextLength();
    }
    if (phasNewLine)
        *phasNewLine = false;
    return GFC_MAX_UPINT;
}

// Returns the text of the line specified by the lineIndex parameter.
const wchar_t* GFxTextDocView::GetLineText(UInt lineIndex, UPInt* plen)
{
    if (!plen)
        return NULL;
    ForceReformat();
    GFxTextLineBuffer::Iterator it = LineBuffer.Begin() + lineIndex;
    if (!it.IsFinished())
    {
        UInt pos = it->GetTextPos();
        UPInt indexInPara = 0;
        GFxStyledText::ParagraphsIterator pit = pDocument->GetParagraphByIndex(pos, &indexInPara);
        if (!pit.IsFinished())
        {
            *plen = it->GetTextLength();
            return pit->GetPtr()->GetText() + indexInPara;
        }
    }
    return NULL;
}

// Returns metrics information about a given text line.
bool GFxTextDocView::GetLineMetrics(UInt lineIndex, GFxTextDocView::LineMetrics* pmetrics)
{
    if (!pmetrics)
        return false;
    ForceReformat();
    GFxTextLineBuffer::Iterator it = LineBuffer.Begin() + lineIndex;
    if (!it.IsFinished())
    {
        GFxTextLineBuffer::Line& line = *it;
        pmetrics->Ascent        = (UInt)line.GetAscent();
        pmetrics->Descent       = (UInt)line.GetDescent();
        pmetrics->Width         = (UInt)line.GetWidth();
        pmetrics->Height        = (UInt)line.GetHeight();
        pmetrics->Leading       = line.GetLeading();
        pmetrics->FirstCharXOff = (SInt)line.GetOffsetX();
        return true;
    }
    return false;
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

                pCharRect->Clear();
                pCharRect->Right += Float(glyph.GetAdvance());
                pCharRect->Top      = GFX_TEXT_GUTTER;
                pCharRect->Bottom   = Float(line.GetHeight());
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

/*  // not necessary anymore since we always have at least one char (EOF) DEL!
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
*/
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
                // need to use GetGlyphBounds instead
                pfont->GetGlyphBounds(glyph.GetIndex(), pCharRect);
                if (glyph.IsNewLineChar())
                    pCharRect->SetWidth(pCharRect->Width()/3);

                Float textHeight = (pfont->GetAscent() + pfont->GetDescent()) * scale;
                pCharRect->Left     *= scale;
                pCharRect->Right    *= scale;
                pCharRect->Top      = line.GetBaseLineOffset() - pfont->GetAscent() * scale + GFX_TEXT_GUTTER;
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
    if (newBottomMostLine >= LineBuffer.size())
        newBottomMostLine = LineBuffer.size() - 1;
    SPInt l = newBottomMostLine;
    GFxTextLineBuffer::Iterator linesIt = LineBuffer.Begin() + newBottomMostLine;
    if (!linesIt.IsFinished())
    {
        const GFxTextLineBuffer::Line& bottomLine = *linesIt;
        const GRectF& textRect = GetTextRect();
        SInt lineH = bottomLine.GetHeight() + bottomLine.GetNonNegLeading();
        Float top = textRect.Top + (bottomLine.GetOffsetY() + lineH - textRect.Bottom);
        UInt topMostLine = newBottomMostLine;
        for(; l >= 0 && !linesIt.IsFinished(); --linesIt)
        {
            const GFxTextLineBuffer::Line& line = *linesIt;
            if (line.GetOffsetY() < top)
                break;

            topMostLine = (UInt)l;
            --l;
        }
        return SetVScrollOffset(topMostLine);
    }
    return false;
}

GFxTextEditorKit* GFxTextDocView::CreateEditorKit()
{
    if (!pEditorKit)
        pEditorKit = *new GFxTextEditorKit(this);
    return pEditorKit;
}

void GFxTextDocView::HighlightDesc::DrawBackground(
    GFxTextDocView& document, GFxDisplayContext &context, const Matrix& mat, const Cxform& cx)
{
    GRenderer*  prenderer       = context.GetRenderer();
    GUNUSED4(context, mat, cx, prenderer);

    Float newHScrollOffset = Float(document.GetHScrollOffset());
    Float newVScrollOffset = Float(document.LineBuffer.GetVScrollOffsetInTwips());

    if (!IsValid())
    {
        HighlightManager.UpdateGlyphIndices
            ((document.HasEditorKit()) ? document.GetEditorKit()->GetCompositionString() : NULL);
    }

    // Draw highlight backgrounds
    if (!HighlightManager.IsEmpty() && 
        (!IsValid() || HScrollOffset != newHScrollOffset || VScrollOffset != newVScrollOffset || 
         FormatCounter != document.FormatCounter))
    {
        HScrollOffset = newHScrollOffset;
        VScrollOffset = newVScrollOffset;
        FormatCounter = document.FormatCounter;
        BackgroundDrawing.Clear();

        UPInt firstVisibleChar = document.GetLineOffset(document.GetVScrollOffset());

        UInt32 oldColor = 0;
        BackgroundDrawing.Clear();
        BackgroundDrawing.SetNonZeroFill(true);
        const GRectF& textRect = document.GetTextRect();
        GFxTextHighlighterRangeIterator it = HighlightManager.GetRangeIterator(firstVisibleChar);
        for (; !it.IsFinished(); ++it)
        {
            GFxTextHighlightDesc hdesc = *it;

            GFxTextLineBuffer::Iterator lit = document.LineBuffer.FindLineByTextPos(hdesc.GlyphIndex);
            bool rv = false;
            
            GRectF lineRect, prevLineRect;
            lineRect.Clear();
            prevLineRect.Clear();
            
            UPInt lenRemaining = hdesc.GlyphNum;
            for (;!lit.IsFinished() && lenRemaining > 0; ++lit)
            {
                GFxTextLineBuffer::Line& line = *lit;
                if (!document.LineBuffer.IsLineVisible(lit.GetIndex()))
                    continue;
                UPInt indexInLine;
                if (line.GetTextPos() < hdesc.GlyphIndex)
                    indexInLine = hdesc.GlyphIndex + (hdesc.GlyphNum - lenRemaining) - line.GetTextPos();
                else
                {
                    indexInLine = 0;
                    // correct the remaining len, if there is a gap between lines in characters positions
                    lenRemaining -= line.GetTextPos() - (hdesc.GlyphIndex + (hdesc.GlyphNum - lenRemaining));
                }
                GFxTextLineBuffer::GlyphIterator git = line.Begin();
                SInt advance = 0;
                lineRect.Clear();
                for(UInt i = 0; !git.IsFinished() && lenRemaining > 0; ++git)
                {
                    const GFxTextLineBuffer::GlyphEntry& glyph = git.GetGlyph();
                    GRectF glyphRect;
                    glyphRect.Clear();
                    if (i >= indexInLine)
                    {
                        rv = true;

                        glyphRect.Right = Float(glyph.GetAdvance());

                        glyphRect.Top      = 0;//GFX_TEXT_GUTTER;
                        glyphRect.Bottom   = Float(line.GetHeight() + line.GetNonNegLeading());

                        glyphRect += GPointF(Float(advance), 0);
                        lenRemaining -= glyph.GetLength();

                        if (i == indexInLine)
                            lineRect = glyphRect;
                        else
                            lineRect.Union(glyphRect);

                    }
                    advance += glyph.GetAdvance();
                    if (glyph.GetLength() > 0)
                        ++i; // do not increase the index, if glyph is lengthless
                }
                lineRect.Offset(-HScrollOffset, -VScrollOffset);
                lineRect.Offset(line.GetOffsetX(), line.GetOffsetY());
                lineRect.Offset(textRect.TopLeft());
                if (!prevLineRect.IsEmpty())
                {
                    // correct the bottom of previous rect to be the same as top of the current one
                    // to avoid holes between selections
                    prevLineRect.Bottom = lineRect.Top;
                    prevLineRect.Intersect(textRect);
                    if (!prevLineRect.IsEmpty())
                    {
                        UInt32 c = hdesc.Info.GetBackgroundColor().ToColor32();
                        if (c != oldColor)
                            BackgroundDrawing.SetFill(oldColor = c);
                        BackgroundDrawing.MoveTo(prevLineRect.Left,  prevLineRect.Top);
                        BackgroundDrawing.LineTo(prevLineRect.Right, prevLineRect.Top);
                        BackgroundDrawing.LineTo(prevLineRect.Right, prevLineRect.Bottom);
                        BackgroundDrawing.LineTo(prevLineRect.Left, prevLineRect.Bottom);
                        BackgroundDrawing.LineTo(prevLineRect.Left, prevLineRect.Top);
                        BackgroundDrawing.AddPath();
                    }
                }
                prevLineRect = lineRect;
            }
            if (!lineRect.IsEmpty())
            {
                lineRect.Intersect(textRect);
                if (!lineRect.IsEmpty())
                {
                    UInt32 c = hdesc.Info.GetBackgroundColor().ToColor32();
                    if (c != oldColor)
                        BackgroundDrawing.SetFill(oldColor = c);
                    BackgroundDrawing.MoveTo(lineRect.Left, lineRect.Top);
                    BackgroundDrawing.LineTo(lineRect.Right, lineRect.Top);
                    BackgroundDrawing.LineTo(lineRect.Right, lineRect.Bottom);
                    BackgroundDrawing.LineTo(lineRect.Left, lineRect.Bottom);
                    BackgroundDrawing.LineTo(lineRect.Left, lineRect.Top);
                    BackgroundDrawing.AddPath();
                }
            }
        }
    }
    if (!BackgroundDrawing.IsEmpty())
        BackgroundDrawing.Display(context, mat, cx, GRenderer::Blend_None, false);
}

GFxTextHighlighter* GFxTextDocView::CreateHighlighterManager() 
{
    if (!pHighlight)
        pHighlight = new HighlightDesc;
    return &pHighlight->HighlightManager;
}

bool GFxTextDocView::AddHighlight(GFxTextHighlightDesc* pdesc)
{
    if (!pHighlight)
        pHighlight = new HighlightDesc;
    return (pHighlight->HighlightManager.CreateNewHighlighter(pdesc) != NULL);
}

bool GFxTextDocView::RemoveHighlight(int id)
{
    if (pHighlight)
        return pHighlight->HighlightManager.FreeHighlighter(id);
    return false;
}

void GFxTextDocView::UpdateHighlight(const GFxTextHighlightDesc& desc)
{
    GUNUSED(desc); //?
    if (!pHighlight)
        return;
    pHighlight->Invalidate();
}

GFxTextHighlightDesc* GFxTextDocView::GetSelectionHighlighterDesc() const
{
    if (!pHighlight)
        return NULL;
    GFxTextHighlightDesc* pdesc = NULL;
    if ((pdesc = pHighlight->HighlightManager.GetHighlighterPtr(GFX_TOPMOST_HIGHLIGHTING_INDEX)) == NULL)
    {
        GFxTextHighlightDesc desc;
        desc.StartPos = 0;
        desc.Length   = 0;
        desc.Id = GFX_TOPMOST_HIGHLIGHTING_INDEX;
        desc.Info.SetBackgroundColor(GFX_ACTIVE_SEL_BKCOLOR);
        desc.Info.SetTextColor(GFX_ACTIVE_SEL_TEXTCOLOR);
        pdesc = pHighlight->HighlightManager.CreateHighlighter(desc);
        GASSERT(pdesc);
    }
    return pdesc;
}

GFxTextHighlightDesc* GFxTextDocView::GetSelectionHighlighterDesc()
{
    if (!pHighlight)
        pHighlight = new HighlightDesc;
    return const_cast<const GFxTextDocView*>(this)->GetSelectionHighlighterDesc();
}

void GFxTextDocView::SetSelection(UPInt startPos, UPInt endPos)
{
    BeginSelection = startPos;
    EndSelection   = endPos;

    if (!pHighlight)
        pHighlight = new HighlightDesc;
    if (endPos < startPos)
    {
        UPInt t = endPos;
        endPos = startPos;
        startPos = t;
    }
    GFxTextHighlightDesc* pdesc = GetSelectionHighlighterDesc();
    GASSERT(pdesc);

    UPInt oldLen = 1;
    UPInt newLen = endPos - startPos;
    if (pdesc->StartPos != startPos || pdesc->Length != newLen)
    {
        oldLen = pdesc->Length;
        pdesc->StartPos = startPos;
        pdesc->Length   = newLen;

        pHighlight->Invalidate();
    }
}

/*void GFxTextDocView::GetSelection(UPInt* pstartPos, UPInt* pendPos)
{
    GASSERT(pstartPos && pendPos);
    *pstartPos = 0;
    *pendPos   = 0;
    if (!pHighlight)
        return;
    GFxTextHighlightDesc* pdesc;
    if ((pdesc = pHighlight->HighlightManager.GetHighlighterPtr(GFX_TOPMOST_HIGHLIGHTING_INDEX)) != NULL)
    {
        *pstartPos = pdesc->StartPos;
        *pendPos   = pdesc->StartPos + pdesc->Length;
    }
}*/

void GFxTextDocView::SetSelectionBackgroundColor(UInt32 color)
{
    GFxTextHighlightDesc* pdesc = GetSelectionHighlighterDesc();
    GASSERT(pdesc);
    if (pdesc->Info.GetBackgroundColor() != color)
    {
        pdesc->Info.SetBackgroundColor(color);
        pHighlight->Invalidate();
    }
}

void GFxTextDocView::SetSelectionTextColor(UInt32 color)
{
    GFxTextHighlightDesc* pdesc = GetSelectionHighlighterDesc();
    GASSERT(pdesc);
    if (pdesc->Info.GetTextColor() != color)
    {
        pdesc->Info.SetTextColor(color);
        pHighlight->Invalidate();
    }
}

UInt32 GFxTextDocView::GetSelectionBackgroundColor() const
{
    GFxTextHighlightDesc* pdesc = GetSelectionHighlighterDesc();
    if (pdesc)
        return pdesc->Info.GetBackgroundColor().ToColor32();
    return GFX_ACTIVE_SEL_BKCOLOR;
}

UInt32 GFxTextDocView::GetSelectionTextColor() const
{
    GFxTextHighlightDesc* pdesc = GetSelectionHighlighterDesc();
    if (pdesc)
        return pdesc->Info.GetBackgroundColor().ToColor32();
    return GFX_ACTIVE_SEL_TEXTCOLOR;
}

void GFxTextDocView::SetDefaultTextAndParaFormat(UPInt cursorPos)
{
    const GFxTextParagraphFormat* pparaFmt = NULL;
    const GFxTextFormat* ptextFmt = NULL;

    UPInt indexInPara = GetFirstCharInParagraph(cursorPos);
    if (indexInPara != GFC_MAX_UPINT)
    {
        bool res;
        if (indexInPara != cursorPos)
            res = pDocument->GetTextAndParagraphFormat(&ptextFmt, &pparaFmt, cursorPos - 1);
        else
            res = pDocument->GetTextAndParagraphFormat(&ptextFmt, &pparaFmt, cursorPos);
        if (res)
        {
            SetDefaultParagraphFormat(pparaFmt);
            SetDefaultTextFormat(ptextFmt);
        }
    }
}

UPInt GFxTextDocView::TextPos2GlyphOffset(UPInt textPos)
{
    if (pEditorKit)
        return pEditorKit->TextPos2GlyphOffset(textPos);
    return textPos;
}

UPInt GFxTextDocView::TextPos2GlyphPos(UPInt textPos)
{
    if (pEditorKit)
        return pEditorKit->TextPos2GlyphPos(textPos);
    return textPos;
}

UPInt GFxTextDocView::GlyphPos2TextPos(UPInt glyphPos)
{
    if (pEditorKit)
        return pEditorKit->GlyphPos2TextPos(glyphPos);
    return glyphPos;
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
    LastHorizCursorPos = -1;

    CursorRect->Clear();
    ActiveSelectionBkColor      = GFX_ACTIVE_SEL_BKCOLOR;
    ActiveSelectionTextColor    = GFX_ACTIVE_SEL_TEXTCOLOR;
    InactiveSelectionBkColor    = GFX_INACTIVE_SEL_BKCOLOR;
    InactiveSelectionTextColor  = GFX_INACTIVE_SEL_TEXTCOLOR;
}

GFxTextEditorKit::~GFxTextEditorKit()
{
}

void GFxTextEditorKit::SetCursorPos(UPInt pos, bool selectionAllowed)
{
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
    LastHorizCursorPos = -1;

    if (CursorPos != GFC_MAX_UPINT)
    {
        ScrollToCursor();

        GetDocument()->SetDefaultTextAndParaFormat(CursorPos);
    }
    if (IsSelectable())
    {
        if (selectionAllowed)
        {
            // handle selection here
            if (IsShiftPressed() || IsMouseCaptured())
            {
                if (pDocView->EndSelection != CursorPos)
                    pDocView->SetEndSelection(CursorPos);            
            }
            else
            {
                if (pDocView->BeginSelection != CursorPos || pDocView->EndSelection != CursorPos)
                    pDocView->SetSelection(CursorPos, CursorPos);
            }
        }
        else
        {
            pDocView->SetSelection(CursorPos, CursorPos);
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
    Float delta = timer - LastAdvanceTime;
    LastAdvanceTime = timer;

    // handle cursor blinking
    if (HasCursor())
    {
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
    // handle auto-scrolling, if mouse is captured
    if (IsMouseCaptured())
    {
        //printf("%f t = %f, b = %f\n", LastMousePos.y, pDocView->GetViewRect().Top, pDocView->GetViewRect().Bottom);
        if (LastMousePos.y <= pDocView->GetViewRect().Top)
        {
            // scroll up
            UInt lineIndex = pDocView->GetVScrollOffset();

            if (lineIndex > 0)
            {
                --lineIndex;
                UPInt newPos = pDocView->GetCursorPosInLine(lineIndex, LastMousePos.y);
                GASSERT(newPos != GFC_MAX_UPINT);
                if (newPos != CursorPos)
                    SetCursorPos(newPos);
            }
        }
        else if (LastMousePos.y >= pDocView->GetViewRect().Bottom)
        {
            // scroll down
            UInt lineIndex = pDocView->GetBottomVScroll();
            ++lineIndex;

            if (lineIndex < pDocView->GetLinesCount())
            {
                UPInt newPos = pDocView->GetCursorPosInLine(lineIndex, LastMousePos.y);
                GASSERT(newPos != GFC_MAX_UPINT);
                if (newPos != CursorPos)
                    SetCursorPos(newPos);
            }
        }
    }
}

bool GFxTextEditorKit::CalcCursorRectInLineBuffer
    (UPInt charIndex, GRectF* pcursorRect, UInt* plineIndex, UInt* pglyphIndex, bool avoidComposStr)
{
    GASSERT(pcursorRect);    
    pDocView->ForceReformat();
    UInt lineIndex = pDocView->GetLineIndexOfChar(charIndex);
    //pDocView->LineBuffer.Dump();
    
    if (lineIndex != ~0u)
    {
        GFxTextLineBuffer::Line& line = pDocView->LineBuffer[lineIndex];

        UPInt posInLine = charIndex - line.GetTextPos();
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
            
            if (!avoidComposStr || !ge.IsComposStrGlyph())
                i += ge.GetLength();
            ++nGlyph;
        }
        Float w = 0;
        if (!git.IsFinished())
        {
            w = Float(git.GetGlyph().GetAdvance());
        }
        pcursorRect->Left = line.GetOffsetX() + Float(xoffset);
        pcursorRect->Top  = line.GetOffsetY();
        pcursorRect->Right  = pcursorRect->Left + w;
        pcursorRect->Bottom = pcursorRect->Top  + line.GetHeight();
        if (plineIndex)
            *plineIndex = lineIndex;
        if (pglyphIndex)
            *pglyphIndex = nGlyph;
        return true;
    }
    return false;
}

bool GFxTextEditorKit::CalcCursorRectOnScreen
    (UPInt charIndex, GRectF* pcursorRect, UInt* plineIndex, UInt* pglyphIndex, bool avoidComposStr)
{
    bool rv = CalcCursorRectInLineBuffer(charIndex, pcursorRect, plineIndex, pglyphIndex, avoidComposStr);
    if (rv)
    {
        pcursorRect->Offset(GPointF(-Float(pDocView->LineBuffer.GetHScrollOffset()), 
                                    -Float(pDocView->LineBuffer.GetVScrollOffsetInTwips())));
        const GRectF& rect = pDocView->GetViewRect();
        pcursorRect->SetTopLeft(pDocView->LineBuffer.BufferToView(rect, pcursorRect->TopLeft()));
        pcursorRect->SetBottomRight(pDocView->LineBuffer.BufferToView(rect, pcursorRect->BottomRight()));
        pcursorRect->Offset(GPointF(rect.Left, rect.Top));
    }
    return rv;
}

const GFxTextLineBuffer::GlyphEntry* GFxTextEditorKit::GetGlyphEntryAtIndex(UPInt charIndex, UPInt* ptextPos)
{
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

void GFxTextEditorKit::PreDisplay(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx)
{
    GUNUSED3(context, mat, cx);

    // handle wide cursor. Wide cursor is drawing in overwrite mode by using highlighter.
    if (HasCursor() && IsWideCursor())
    {
        bool cursorBlinkState;
        GFxTextHighlighter* phighlighter = pDocView->CreateHighlighterManager();
        GASSERT(phighlighter);

        GFxTextHighlightDesc* pdesc = NULL;
        if ((pdesc = phighlighter->GetHighlighterPtr(GFX_WIDECURSOR_HIGHLIGHTING_INDEX)) == NULL)
        {
            GFxTextHighlightDesc desc;
            desc.StartPos = 0;
            desc.Length   = 0;
            desc.Offset   = 0;
            desc.Id = GFX_WIDECURSOR_HIGHLIGHTING_INDEX;
            desc.Info.SetBackgroundColor(GFX_ACTIVE_SEL_BKCOLOR);
            desc.Info.SetTextColor(GFX_ACTIVE_SEL_TEXTCOLOR);
            pdesc = phighlighter->CreateHighlighter(desc);
            GASSERT(pdesc);
            cursorBlinkState = false;
        }
        else
            cursorBlinkState = (pdesc->Length == 0) ? 0 : 1;
        if (pdesc->StartPos != CursorPos || cursorBlinkState != IsCursorBlink())
        {
            pdesc->StartPos = CursorPos;
            pdesc->Length   = IsCursorBlink() ? 1 : 0;
            pDocView->UpdateHighlight(*pdesc);
        }
    }
}

void GFxTextEditorKit::ClearWideCursor()   
{ 
    Flags &= (~Flags_WideCursor); 

    GFxTextHighlighter* phighlighter = pDocView->GetHighlighterManager();
    if (phighlighter)
    {
        GFxTextHighlightDesc* pdesc = NULL;
        if ((pdesc = phighlighter->GetHighlighterPtr(GFX_WIDECURSOR_HIGHLIGHTING_INDEX)) != NULL)
        {
            if (pdesc->Length)
            {
                pdesc->Length = 0;
                pDocView->UpdateHighlight(*pdesc);
            }
        }
    }
}

// Draws cursor, if necessary
void GFxTextEditorKit::Display(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx)
{
    GRenderer*  prenderer       = context.GetRenderer();
    
    // Draw the cursor
    if (IsCursorBlink() && !IsReadOnly() && !IsWideCursor())
    {
        // check is CursorRect valid
        UInt lineIndex = GFC_MAX_UINT;
        if (!CursorRect.IsValid(pDocView->FormatCounter))
        {
            // not valid, calculate
            GRectF newCursorRect;
            UInt glyphIndex;
            UPInt cursorPos = CursorPos;

#ifndef GFC_NO_IME_SUPPORT
            // check if composition string is active. If so, check, does it require
            // cursor to be drawn inside the composition string.
            if (HasCompositionString())
            {
                cursorPos += pComposStr->GetCursorPosition();
            }
#endif //#ifndef GFC_NO_IME_SUPPORT

            if (CalcAbsCursorRectInLineBuffer(cursorPos, &newCursorRect, &lineIndex, &glyphIndex))
            {
                //const GFxTextLineBuffer::Line& line = pDocView->LineBuffer[lineIndex];
                //newCursorRect.SetRect(topLeft, GSizeF(0.f, Float(line.GetHeight())));
                newCursorRect.Right = newCursorRect.Left; // set width to 0

                // determine the cursor's color
                const GFxTextFormat* pdefFormat = pDocView->GetDefaultTextFormat();

                if (pdefFormat && pdefFormat->IsColorSet())
                    CursorColor = pdefFormat->GetColor();

                // replace the cursor pos by the highlighted text color, if exists
                GFxTextHighlighter* phighlighter;
                if ((phighlighter = pDocView->GetHighlighterManager()) != NULL)
                {
                    GFxTextHighlighterPosIterator it = phighlighter->GetPosIterator(CursorPos);
                    const GFxTextHighlightDesc& desc = *it;
                    if (desc.Info.HasTextColor())
                        CursorColor = desc.Info.GetTextColor();
                }
            }
            else
            {
                // not necessary anymore since we always have at least one char (EOF) DEL!
                newCursorRect.Clear();
                GASSERT(0); // shouldn't happen anymore!
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
            icoords[0].x = (SInt16) floorf(coords[0].x + 0.5f);
            icoords[0].y = (SInt16) coords[0].y;
            icoords[1].x = (SInt16) floorf(coords[1].x + 0.5f);
            icoords[1].y = (SInt16) coords[1].y;
            icoords[2].x = (SInt16) floorf(coords[2].x + 0.5f);
            icoords[2].y = (SInt16) coords[2].y;
            icoords[3].x = (SInt16) floorf(coords[3].x + 0.5f);
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
bool GFxTextEditorKit::ScrollToPosition(UPInt pos, bool avoidComposStr, bool wideCursor)
{
    GRectF newCursorRect;
    UInt lineIndex;
    bool rv = false;

    if (CalcCursorRectOnScreen(pos, &newCursorRect, &lineIndex, NULL, avoidComposStr))
    {
        const GRectF& textRect = pDocView->GetTextRect();
        
        if (!wideCursor)
            newCursorRect.SetWidth(20.f);

        if (!textRect.Contains(newCursorRect))
        {
            // cursor is invisible, calculate scrolling
            // hscroll first
            SInt hscroll = (SInt)pDocView->LineBuffer.GetHScrollOffset();
            SInt prevHScroll = hscroll;
            if (newCursorRect.Right > textRect.Right)
            {
                // make sure whole cursor is visible.
                hscroll += SInt(newCursorRect.Right - textRect.Right + GFX_EDIT_HSCROLL_DELTA - 20.f);
            }
            else if (newCursorRect.Left < textRect.Left)
            {
                hscroll -= SInt(textRect.Left - newCursorRect.Left) + GFX_EDIT_HSCROLL_DELTA;
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
        Float ax = x;
        Float ay = y;
        const GRectF& r = pDocView->GetViewRect();
        ax -= r.Left;
        ay -= r.Top;
        LastMousePos.x = ax;
        LastMousePos.y = ay;

        UPInt pos = pDocView->GetCursorPosAtPoint(ax, ay);
        if (pos != GFC_MAX_UPINT)
        {
            SetCursorPos(pos);
            if (IsSelectable() && !IsMouseCaptured())
            {
                SetMouseCaptured();
                if (!IsShiftPressed())
                    pDocView->SetSelection(pos, pos);
            }
        }
    }
}

void    GFxTextEditorKit::OnMouseUp(Float x, Float y, int buttons)
{
    GUNUSED2(x, y);
    if (!(buttons & 1)) // left mouse button is up
    {
        if (IsSelectable() && IsMouseCaptured())
        {
            ClearMouseCaptured();
            //ClearShiftPressed();
        }
    }
}

void    GFxTextEditorKit::OnMouseMove(Float x, Float y)
{
    if (IsMouseCaptured()) 
    {
        Float ax = x;// + Float(pDocView->LineBuffer.GetHScrollOffset());
        Float ay = y;// + Float(pDocView->LineBuffer.GetVScrollOffsetInTwips()) - pDocView->LineBuffer.GetVScrollOffsetInTwips();
        const GRectF& r = pDocView->GetViewRect();
        ax -= r.Left;
        ay -= r.Top;
        LastMousePos.x = ax;
        LastMousePos.y = ay;

        UPInt pos = pDocView->GetCursorPosAtPoint(ax, ay);
        if (pos != GFC_MAX_UPINT)
        {
            //printf("%d x %f y %f\n", pos, (float)ax, (float)ay);
            SetCursorPos(pos);
            if (IsSelectable())
                pDocView->SetEndSelection(pos);
        }
    }
}

bool GFxTextEditorKit::OnKeyUp(int keyCode, const GFxSpecialKeysState& specKeysState)
{
    GUNUSED(specKeysState);

    if (pKeyMap)
    {
        const GFxTextKeyMap::KeyMapEntry* pke = pKeyMap->Find(keyCode, specKeysState, GFxTextKeyMap::State_Up);
        if (pke)
        {
            switch(pke->Action)
            {
            case GFxTextKeyMap::KeyAct_LeaveSelectionMode:
                if (IsSelectable() && IsShiftPressed())
                {
                    ClearShiftPressed();
                }
                break;
            default: break;
            }
        }
    }
    return true;
}

bool GFxTextEditorKit::OnKeyDown(int keyCode, const GFxSpecialKeysState& specKeysState)
{
    GFxTextDocView* pdocument = GetDocument();
    UPInt len = pdocument->GetStyledText()->GetLength();
    bool rv = false;
    UPInt newPos = CursorPos;

    if (pdocument->GetDocumentListener())
    {
        keyCode = pdocument->GetDocumentListener()->Editor_OnKey(*this, keyCode);
    }
    bool  needChangeNotify = false, noActions = false, isShiftPressed = IsShiftPressed();
    Float lastHorizCursorPos = -1;
    UPInt beginSel = pDocView->BeginSelection, 
          endSel   = pDocView->EndSelection;

    if (pKeyMap)
    {
        const GFxTextKeyMap::KeyMapEntry* pke = pKeyMap->Find(keyCode, specKeysState, GFxTextKeyMap::State_Down);
        if (!pke)
            noActions = true;
        else
        {
            switch(pke->Action)
            {
            case GFxTextKeyMap::KeyAct_EnterSelectionMode:
                if (IsSelectable())
                {
                    isShiftPressed = true;
                }
                noActions = true;
                break;
            case GFxTextKeyMap::KeyAct_Left:
                {
                    if (newPos == GFC_MAX_UPINT)
                        newPos = len;
                    if (newPos > 0)
                        newPos -= 1;
                    if (!IsShiftPressed())
                    {
                        // not selecting but BegSel != EndSel? Jump to the lowest pos
                        if (beginSel != endSel)
                            newPos = GTL::gpmin(beginSel, endSel);
                    }
                    break;
                }
                // Right arrow key
            case GFxTextKeyMap::KeyAct_Right:
                {
                    if (newPos == GFC_MAX_UPINT)
                        newPos = len;
                    else if (newPos < len)
                        newPos += 1;
                    if (!IsShiftPressed())
                    {
                        // not selecting but BegSel != EndSel? Jump to the greatest pos
                        if (beginSel != endSel)
                            newPos = GTL::gpmax(beginSel, endSel);
                    }
                    break;
                }
            case GFxTextKeyMap::KeyAct_LineHome:
                {
                    UInt lineIndex = pDocView->GetLineIndexOfChar(CursorPos);
                    if (lineIndex != ~0u)
                    {
                        newPos = pDocView->GetLineOffset(lineIndex);
                        GASSERT(newPos != GFC_MAX_UPINT);
                    }
                    break;
                }
            case GFxTextKeyMap::KeyAct_LineEnd:
                {
                    UInt lineIndex = pDocView->GetLineIndexOfChar(CursorPos);
                    if (lineIndex != ~0u)
                    {
                        GASSERT(pDocView->GetLineOffset(lineIndex) != GFC_MAX_UPINT && pDocView->GetLineLength(lineIndex) != GFC_MAX_UPINT);
                        bool hasNewLine = false;
                        UPInt len = pDocView->GetLineLength(lineIndex, &hasNewLine);
                        if (hasNewLine)
                            --len;
                        newPos = pDocView->GetLineOffset(lineIndex) + len;
                    }
                }
                break;

            case GFxTextKeyMap::KeyAct_DocHome:
                newPos = 0;
                break;

            case GFxTextKeyMap::KeyAct_DocEnd:
                newPos = len;
                break;

            case GFxTextKeyMap::KeyAct_PageHome:
                {
                    UInt lineIndex = pDocView->GetVScrollOffset();
                    newPos = pDocView->GetLineOffset(lineIndex);
                    GASSERT(newPos != GFC_MAX_UPINT);
                    break;
                }
            case GFxTextKeyMap::KeyAct_PageEnd:
                {
                    UInt lineIndex = pDocView->GetBottomVScroll();
                    bool hasNewLine = false;
                    UPInt len = pDocView->GetLineLength(lineIndex, &hasNewLine);
                    if (hasNewLine)
                        --len;
                    newPos = pDocView->GetLineOffset(lineIndex) + len;
                    break;
                }
            case GFxTextKeyMap::KeyAct_PageUp:
                {
                    GRectF cursorRect;
                    UInt lineIndex;

                    if (CalcCursorRectOnScreen(CursorPos, &cursorRect, &lineIndex))
                    {
                        if (LastHorizCursorPos < 0)
                            lastHorizCursorPos = cursorRect.Left;
                        else
                            lastHorizCursorPos = LastHorizCursorPos;

                        UInt bottomV = pDocView->GetBottomVScroll();
                        UInt numLinesOnScreen = bottomV - pDocView->GetVScrollOffset() + 1;
                        if (lineIndex < numLinesOnScreen)
                            newPos = 0;
                        else
                        {
                            lineIndex -= numLinesOnScreen;
                            newPos = pDocView->GetCursorPosInLine(lineIndex, lastHorizCursorPos);
                        }
                        GASSERT(newPos != GFC_MAX_UPINT);
                    }
                    break;
                }
            case GFxTextKeyMap::KeyAct_PageDown:
                {
                    GRectF cursorRect;
                    UInt lineIndex;

                    if (CalcCursorRectOnScreen(CursorPos, &cursorRect, &lineIndex))
                    {
                        if (LastHorizCursorPos < 0)
                            lastHorizCursorPos = cursorRect.Left;
                        else
                            lastHorizCursorPos = LastHorizCursorPos;

                        UInt bottomV = pDocView->GetBottomVScroll();
                        UInt numLinesOnScreen = bottomV - pDocView->GetVScrollOffset() + 1;
                        lineIndex += numLinesOnScreen;
                        if (lineIndex >= pDocView->GetLinesCount())
                            newPos = len;
                        else
                            newPos = pDocView->GetCursorPosInLine(lineIndex, lastHorizCursorPos);
                        GASSERT(newPos != GFC_MAX_UPINT);
                    }
                    break;
                }

            case GFxTextKeyMap::KeyAct_Up:
                {
                    GRectF cursorRect;
                    UInt lineIndex;

                    if (!IsShiftPressed() && beginSel != endSel)
                    {
                        // not selecting but BegSel != EndSel? Jump to the lowest pos
                        newPos = GTL::gpmin(beginSel, endSel);
                    }
                    else if (CalcCursorRectOnScreen(CursorPos, &cursorRect, &lineIndex))
                    {
                        if (lineIndex > 0)
                        {
                            if (LastHorizCursorPos < 0)
                                lastHorizCursorPos = cursorRect.Left;
                            else
                                lastHorizCursorPos = LastHorizCursorPos;

                            --lineIndex;
                            newPos = pDocView->GetCursorPosInLine(lineIndex, lastHorizCursorPos);
                            GASSERT(newPos != GFC_MAX_UPINT);
                        }
                        else
                            newPos = 0;
                    }
                    break;
                }
            case GFxTextKeyMap::KeyAct_Down:
                {
                    GRectF cursorRect;
                    UInt lineIndex;

                    if (!IsShiftPressed() && beginSel != endSel)
                    {
                        // not selecting but BegSel != EndSel? Jump to the greatest pos
                        newPos = GTL::gpmax(beginSel, endSel);
                    }
                    else if (CalcCursorRectOnScreen(CursorPos, &cursorRect, &lineIndex))
                    {
                        if (lineIndex + 1 < pDocView->GetLinesCount())
                        {
                            if (LastHorizCursorPos < 0)
                                lastHorizCursorPos = cursorRect.Left;
                            else
                                lastHorizCursorPos = LastHorizCursorPos;

                            ++lineIndex;
                            newPos = pDocView->GetCursorPosInLine(lineIndex, lastHorizCursorPos);
                            GASSERT(newPos != GFC_MAX_UPINT);
                        }
                        else
                            newPos = len;
                    }
                    break;
                }

            case GFxTextKeyMap::KeyAct_Backspace:
                {
                    if (!IsReadOnly())
                    {
                        if (beginSel == endSel)
                        {
                            GFxTextDocView::DeleteCharCommand cmd(newPos);
                            newPos -= pDocView->EditCommand(GFxTextDocView::Cmd_BackspaceChar, &cmd);
                            GASSERT(SPInt(newPos) >= 0);
                            needChangeNotify = true;
                        }
                        else
                        {
                            GFxTextDocView::DeleteTextCommand cmd(beginSel, endSel);
                            pDocView->EditCommand(GFxTextDocView::Cmd_DeleteText, &cmd);
                            newPos = GTL::gpmin(beginSel, endSel);
                            needChangeNotify = true;
                        }
                    }
                    break;
                }
            case GFxTextKeyMap::KeyAct_Delete:
                {
                    if (!IsReadOnly())
                    {
                        if (beginSel == endSel)
                        {
                            if (CursorPos < len)
                            {
                                GFxTextDocView::DeleteCharCommand cmd(newPos);
                                pDocView->EditCommand(GFxTextDocView::Cmd_DeleteChar, &cmd);
                                needChangeNotify = true;
                            }
                        }
                        else
                        {
                            GFxTextDocView::DeleteTextCommand cmd(beginSel, endSel);
                            pDocView->EditCommand(GFxTextDocView::Cmd_DeleteText, &cmd);
                            newPos = GTL::gpmin(beginSel, endSel);
                            needChangeNotify = true;
                        }
                    }
                    break;
                }
            case GFxTextKeyMap::KeyAct_Return:
                {
                    if (!IsReadOnly())
                    {
                        if (pDocView->IsMultiline())
                        {
                            ClearShiftPressed();
                            if (beginSel == endSel)
                            {
                                GFxTextDocView::InsertCharCommand cmd(CursorPos, pDocView->NewLineChar());
                                pDocView->EditCommand(GFxTextDocView::Cmd_InsertChar, &cmd);
                                ++newPos;
                                needChangeNotify = true;
                            }
                            else
                            {
                                // replace the selection by the input
                                GFxTextDocView::ReplaceTextByCharCommand cmd(beginSel, endSel, pDocView->NewLineChar());
                                pDocView->EditCommand(GFxTextDocView::Cmd_ReplaceTextByChar, &cmd);
                                newPos = GTL::gpmin(beginSel, endSel) + 1;
                                needChangeNotify = true;
                            }
                        }
                    }
                    break;
                }

            case GFxTextKeyMap::KeyAct_SelectAll:
                {
                    beginSel = 0;
                    endSel   = newPos = len;
                    SetCursorPos(newPos);
                    pDocView->SetSelection(beginSel, endSel);
                    noActions = true;
                }
                break;
            case GFxTextKeyMap::KeyAct_Copy:
                ClearShiftPressed();
                CopyToClipboard(beginSel, endSel, DoesUseRichClipboard());
                noActions = true;
                break;
            case GFxTextKeyMap::KeyAct_Cut:
                if (pClipboard)
                {
                    ClearShiftPressed();
                    CutToClipboard(beginSel, endSel, DoesUseRichClipboard());
                    newPos = GTL::gpmin(beginSel, endSel);
                    needChangeNotify = true;
                }
                break;
            case GFxTextKeyMap::KeyAct_Paste:
                {
                    UPInt _newPos = PasteFromClipboard(beginSel, endSel, DoesUseRichClipboard());
                    if (_newPos != GFC_MAX_UPINT)
                    {
                        newPos = _newPos;
                        needChangeNotify = true;
                    }
                }
                break;
                
            default: noActions = true; break;
            }
        }
    }
    else
        noActions = true;

#ifdef GFC_BUILD_DEBUG
    if (noActions)
    {
        switch(keyCode)
        {
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
        default: break;
        }
    }
#endif

    if (!noActions)
    {
        if (CursorPos != newPos)
        {
            SetCursorPos(newPos);
            LastHorizCursorPos = lastHorizCursorPos;
            rv = true;
        }
        else if ((!IsShiftPressed() && beginSel != endSel))
        {
            SetCursorPos(newPos);
            rv = true;
        }

        if (needChangeNotify)
        {
            if (!rv)
                pDocView->SetDefaultTextAndParaFormat(CursorPos);

            if (pdocument->GetDocumentListener())
            {
                pdocument->GetDocumentListener()->Editor_OnChanged(*this);
            }
        }
    }
    if (isShiftPressed) // set selecting mode back
        SetShiftPressed();
    return rv;
}

bool GFxTextEditorKit::OnChar(UInt32 wcharCode)
{
    if (!wcharCode || IsMouseCaptured())
        return false;
    GFxTextDocView* pdocument = GetDocument();
    bool rv = false;
    UPInt newPos = CursorPos;

    UPInt beginSel = pDocView->BeginSelection, 
          endSel   = pDocView->EndSelection;

    bool needChangeNotify = false;
    if (!IsReadOnly())
    {
        wchar_t wc;
        wc = (wchar_t)wcharCode;
        if (wc >= 32)
        {
            //@TODO - remove
            //if (wc && pdocument->GetDocumentListener())
            //{
            //    wc = pdocument->GetDocumentListener()->Editor_OnCharacter(*this, wc);
            //}
            //if (wc)
            {
                //ClearShiftPressed();
                if (beginSel == endSel && IsOverwriteMode())
                {
                    ++endSel;
                }
                if (beginSel == endSel)
                {
                    // regular input
                    GFxTextDocView::InsertCharCommand cmd(CursorPos, wc);
                    newPos += pDocView->EditCommand(GFxTextDocView::Cmd_InsertChar, &cmd);
                    needChangeNotify = true;
                }
                else
                {
                    // replace the selection by the input
                    GFxTextDocView::ReplaceTextByCharCommand cmd(beginSel, endSel, wc);
                    UPInt res = pDocView->EditCommand(GFxTextDocView::Cmd_ReplaceTextByChar, &cmd);
                    newPos = GTL::gpmin(beginSel, endSel) + res;
                    needChangeNotify = true;
                }
            }
            if (CursorPos != newPos || beginSel != newPos || endSel != newPos)
            {
                SetCursorPos(newPos, false);
                rv = true;
            }
            if (needChangeNotify)
            {
                //if (IsSelectable())
                //    pDocView->SetSelection(beginSel, endSel);
                if (pdocument->GetDocumentListener())
                {
                    pdocument->GetDocumentListener()->Editor_OnChanged(*this);
                }
            }
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

void GFxTextEditorKit::OnSetFocus()
{
    pDocView->SetSelectionTextColor(ActiveSelectionTextColor);
    pDocView->SetSelectionBackgroundColor(ActiveSelectionBkColor);
}

void GFxTextEditorKit::OnKillFocus()
{
    pDocView->SetSelectionTextColor(InactiveSelectionTextColor);
    pDocView->SetSelectionBackgroundColor(InactiveSelectionBkColor);
    ClearShiftPressed();
    ClearMouseCaptured();
}

void GFxTextEditorKit::SetSelection(UPInt startPos, UPInt endPos)
{
    if (!IsReadOnly() || IsSelectable())
    {
        SetCursorPos(endPos);
    }
    pDocView->SetSelection(startPos, endPos);
}

void GFxTextEditorKit::CopyToClipboard(UPInt startPos, UPInt endPos, bool useRichClipboard)
{
    if (pClipboard && !pDocView->IsPasswordMode())
    {
        if (endPos < startPos)
        {
            UPInt t = endPos;
            endPos = startPos;
            startPos = t;
        }
        // copy to clipboard
        GFxWideStringBuffer wbuf;
        pDocView->GetStyledText()->GetText(&wbuf, startPos, endPos);
        if (useRichClipboard)
        {
            GPtr<GFxStyledText> ptext = 
                *pDocView->GetStyledText()->CopyStyledText(startPos, endPos);
            pClipboard->SetTextAndStyledText(wbuf.ToWStr(), wbuf.GetLength(), ptext);
        }
        else
            pClipboard->SetText(wbuf.ToWStr(), wbuf.GetLength());
    }
}

void GFxTextEditorKit::CutToClipboard(UPInt startPos, UPInt endPos, bool useRichClipboard)
{
    if (pClipboard)
    {
        if (endPos < startPos)
        {
            UPInt t = endPos;
            endPos = startPos;
            startPos = t;
        }
        CopyToClipboard(startPos, endPos, useRichClipboard);
        pDocView->RemoveText(startPos, endPos);
    }
}

// if successful, returns new projected cursor position; GFC_MAX_UPINT otherwise.
UPInt GFxTextEditorKit::PasteFromClipboard(UPInt startPos, UPInt endPos, bool useRichClipboard)
{
    UPInt newPos = GFC_MAX_UPINT;
    if (!IsReadOnly() && pClipboard)
    {
        if (endPos < startPos)
        {
            UPInt t = endPos;
            endPos = startPos;
            startPos = t;
        }
        // paste from clipboard
        if (useRichClipboard && pClipboard->ContainsRichText())
        {
            // rich text
            GPtr<GFxStyledText> pstr = pClipboard->GetStyledText();
            if (pstr && pstr->GetLength() > 0)
            {
                ClearShiftPressed();
                if (startPos == endPos)
                {
                    GFxTextDocView::InsertStyledTextCommand cmd(startPos, pstr);
                    UPInt len = pDocView->EditCommand(GFxTextDocView::Cmd_InsertStyledText, &cmd);
                    newPos = startPos + len;
                }
                else
                {
                    // replace the selection by the input
                    GFxTextDocView::ReplaceTextByStyledTextCommand cmd(startPos, endPos, pstr);
                    UPInt len = pDocView->EditCommand(GFxTextDocView::Cmd_ReplaceTextByStyledText, &cmd);
                    newPos = startPos + len;
                }
            }
        }
        else
        {
            // plain text
            const GFxWStringBuffer& str = pClipboard->GetText();
            if (str.GetLength() > 0)
            {
                ClearShiftPressed();
                if (startPos == endPos)
                {
                    GFxTextDocView::InsertPlainTextCommand cmd(startPos, str.ToWStr(), str.GetLength());
                    UPInt len = pDocView->EditCommand(GFxTextDocView::Cmd_InsertPlainText, &cmd);
                    newPos = startPos + len;
                }
                else
                {
                    // replace the selection by the input
                    GFxTextDocView::ReplaceTextByPlainTextCommand cmd
                        (startPos, endPos, str.ToWStr(), str.GetLength());
                    UPInt len = pDocView->EditCommand(GFxTextDocView::Cmd_ReplaceTextByPlainText, &cmd);
                    newPos = startPos + len;
                }
            }
        }
    }
    return newPos;
}

#ifndef GFC_NO_IME_SUPPORT
GFxTextCompositionString* GFxTextEditorKit::CreateCompositionString()
{
    pComposStr = *new GFxTextCompositionString(this);
    return pComposStr;
}

void GFxTextEditorKit::ReleaseCompositionString()
{
    if (pComposStr)
    {
        GPtr<GFxTextCompositionString> pcs = pComposStr;
        pComposStr = NULL;
        pcs->Reformat(); // schedule reformatting of src paragraph
    }
}
#endif //#ifndef GFC_NO_IME_SUPPORT

UPInt GFxTextEditorKit::TextPos2GlyphOffset(UPInt textPos)
{
#ifndef GFC_NO_IME_SUPPORT
    if (HasCompositionString())
    {
        if (textPos > pComposStr->GetPosition())
        {
            return textPos + pComposStr->GetLength();
        }
    }
#endif //#ifndef GFC_NO_IME_SUPPORT
    return textPos;
}

UPInt GFxTextEditorKit::TextPos2GlyphPos(UPInt textPos)
{
#ifndef GFC_NO_IME_SUPPORT
    if (HasCompositionString())
    {
        if (textPos >= pComposStr->GetPosition())
        {
            return textPos + pComposStr->GetLength();
        }
    }
#endif //#ifndef GFC_NO_IME_SUPPORT
    return textPos;
}

UPInt GFxTextEditorKit::GlyphPos2TextPos(UPInt glyphPos)
{
#ifndef GFC_NO_IME_SUPPORT
    if (HasCompositionString())
    {
        if (glyphPos <= pComposStr->GetPosition())
            return glyphPos;
        if (glyphPos >= pComposStr->GetPosition() + pComposStr->GetLength())
        {
            return glyphPos - pComposStr->GetLength();
        }
        else
            return pComposStr->GetPosition();
    }
#endif //#ifndef GFC_NO_IME_SUPPORT
    return glyphPos;
}

#ifndef GFC_NO_IME_SUPPORT
//////////////////////////////////////////////////////////////////////////
// GFxTextCompositionString
//
GFxTextCompositionString::GFxTextCompositionString(GFxTextEditorKit* peditorKit) :
    pEditorKit(peditorKit), String(peditorKit->GetDocument()->GetAllocator()),
    CursorPos(0), HasHighlightingFlag(false)
{
    GFxTextFormat fmt; // create empty text format
    const GFxTextFormat* pdefFmt = peditorKit->GetDocument()->GetDefaultTextFormat();
    if (!pdefFmt)
        pdefFmt = &fmt;
    pDefaultFormat = *GetAllocator()->AllocateTextFormat(*pdefFmt);

    String.AppendTermNull(pDefaultFormat);

    // create highlighting for composition string
    GFxTextHighlighter* phighlighter = pEditorKit->GetDocument()->CreateHighlighterManager();
    GASSERT(phighlighter);
    GFxTextHighlightDesc* pdesc;
    if ((pdesc = phighlighter->GetHighlighterPtr(GFX_COMPOSSTR_HIGHLIGHTING_INDEX)) == NULL)
    {
        GFxTextHighlightDesc desc;
        desc.StartPos = 0;
        desc.Length   = 0;
        desc.Id = GFX_COMPOSSTR_HIGHLIGHTING_INDEX;

        //desc.Info.SetTextColor(GFX_DEFAULT_COMPOSSTR_TEXTCOLOR);
        
        //@DBG       
        //desc.Info.SetBackgroundColor(0xFF903376);
        //desc.Info.SetUnderlineColor(0xFF33FF03);
        //desc.Info.SetDoubleUnderline();
        //desc.Info.SetDottedUnderline();
        //desc.Info.SetDitheredSingleUnderline();
        pdesc = phighlighter->CreateHighlighter(desc);
        GASSERT(pdesc);
    }
    if ((pdesc = phighlighter->GetHighlighterPtr(GFX_CLAUSE_HIGHLIGHTING_INDEX)) == NULL)
    {
        GFxTextHighlightDesc desc;
        desc.StartPos = 0;
        desc.Length   = 0;
        desc.Id = GFX_CLAUSE_HIGHLIGHTING_INDEX;
        //desc.Info.SetTextColor(GFX_DEFAULT_CLAUSE_TEXTCOLOR);

        //@DBG       
        //desc.Info.SetBackgroundColor(0xFF769911);
        //desc.Info.SetTextColor(0xFF0000FF);
        //desc.Info.SetUnderlineColor(0xFF33FF03);
        //desc.Info.SetDoubleUnderline();
        //desc.Info.SetDitheredThickUnderline();
        pdesc = phighlighter->CreateHighlighter(desc);
        GASSERT(pdesc);
    }

    // set default styles
    Styles = GetDefaultStyles();
}

GFxTextCompositionString::~GFxTextCompositionString()
{

}

GFxTextIMEStyle GFxTextCompositionString::GetDefaultStyles()
{
    GFxTextIMEStyle styles;
    styles.SetElement(GFxTextIMEStyle::SC_CompositionSegment, 
        GFxTextHighlightInfo(GFxTextHighlightInfo::Underline_Dotted));
    styles.SetElement(GFxTextIMEStyle::SC_ClauseSegment,      
        GFxTextHighlightInfo(GFxTextHighlightInfo::Underline_Thick));
    styles.SetElement(GFxTextIMEStyle::SC_ConvertedSegment,   
        GFxTextHighlightInfo(GFxTextHighlightInfo::Underline_Single));
    GFxTextHighlightInfo plainfo;
    plainfo.SetBackgroundColor(GFX_ACTIVE_SEL_BKCOLOR);
    plainfo.SetTextColor(GFX_ACTIVE_SEL_TEXTCOLOR);
    styles.SetElement(GFxTextIMEStyle::SC_PhraseLengthAdj, plainfo);
    GFxTextHighlightInfo lcsinfo(GFxTextHighlightInfo::Underline_Single);
    styles.SetElement(GFxTextIMEStyle::SC_LowConfSegment, lcsinfo);
    return styles;
}

GFxTextParagraph* GFxTextCompositionString::GetSourceParagraph()
{
    UPInt pos = GetPosition();
    GFxStyledText::ParagraphsIterator it = 
        pEditorKit->GetDocument()->GetStyledText()->GetParagraphByIndex(pos, NULL);
    if (!it.IsFinished())
        return *it;
    return NULL;
}

void GFxTextCompositionString::Reformat()
{
    GFxTextParagraph* ppara = GetSourceParagraph();
    if (ppara)
    {
        ppara->MarkToReformat();
        pEditorKit->GetDocument()->SetReformatReq();
    }
}

void GFxTextCompositionString::SetText(const wchar_t* pwstr, UPInt nchars)
{
    String.Clear();
    String.SetText(pwstr, nchars);
    String.SetTextFormat(*pDefaultFormat, 0, String.GetSize());
    String.AppendTermNull(pDefaultFormat);
    
    String.CheckIntegrity();
    Reformat();
 
    if (HasHighlighting())
    {
        GFxTextHighlighter* phighlighter = pEditorKit->GetDocument()->CreateHighlighterManager();
        GASSERT(phighlighter);
        GFxTextHighlightDesc* pdesc = phighlighter->GetHighlighterPtr(GFX_COMPOSSTR_HIGHLIGHTING_INDEX);
        GASSERT(pdesc);
        pdesc->StartPos = GetPosition();
        pdesc->Offset = 0;
        pdesc->Length = String.GetLength();
        pEditorKit->GetDocument()->UpdateHighlight(*pdesc);
    }
    HighlightText(0, 0, GFxTextIMEStyle::SC_CompositionSegment);
}

void GFxTextCompositionString::SetText(const char* pstr, UPInt nchars)
{
    GUNUSED2(pstr, nchars);
    GASSERT(0);
    //String.SetText(pstr, nchars);
}

void GFxTextCompositionString::SetPosition(UPInt pos)
{
    String.SetStartIndex(pos);

    Reformat();
}

void GFxTextCompositionString::SetCursorPosition(UPInt pos)
{
    CursorPos = pos;
    pEditorKit->CursorRect.Invalidate();
    pEditorKit->ResetBlink(1);
    pEditorKit->ScrollToPosition(GetPosition() + CursorPos, false, pEditorKit->IsWideCursor());
}

void GFxTextCompositionString::HighlightText(UPInt pos, UPInt len, GFxTextIMEStyle::Category styleCategory)
{
    /*String.SetTextFormat(*pDefaultFormat);
    if (len > 0)
    {
        String.SetTextFormat(*pClauseFormat, pos, pos + len);
    }*/

    GFxTextHighlighter* phighlighter = pEditorKit->GetDocument()->CreateHighlighterManager();
    GASSERT(phighlighter);
    GFxTextHighlightDesc* pdesc = phighlighter->GetHighlighterPtr(GFX_CLAUSE_HIGHLIGHTING_INDEX);
    GASSERT(pdesc);
    pdesc->StartPos = GetPosition();
    pdesc->Offset = pos;
    pdesc->Length = len;
    pdesc->Info = Styles.GetElement(styleCategory);
    pEditorKit->GetDocument()->UpdateHighlight(*pdesc);

    //Reformat();

    // we also need to make sure that the highlighted clause is visible
    if (len > 0)
    {
        pEditorKit->ScrollToPosition(GetPosition() + pos + len, false, pEditorKit->IsWideCursor());
        pEditorKit->ScrollToPosition(GetPosition() + pos, false, pEditorKit->IsWideCursor());
    }
    String.CheckIntegrity();
}

void  GFxTextCompositionString::SetStyle(GFxTextIMEStyle::Category styleCategory)
{
    GFxTextHighlighter* phighlighter = pEditorKit->GetDocument()->CreateHighlighterManager();
    GASSERT(phighlighter);
    GFxTextHighlightDesc* pdesc = phighlighter->GetHighlighterPtr(GFX_COMPOSSTR_HIGHLIGHTING_INDEX);
    GASSERT(pdesc);
    pdesc->Info = Styles.GetElement(styleCategory);
}

void  GFxTextCompositionString::UseStyles(const GFxTextIMEStyle& styles)
{
    Styles.Unite(styles);
}

void  GFxTextCompositionString::SetNoHighlighting(bool v) 
{ 
    HasHighlightingFlag = !v; 

    if (!HasHighlightingFlag)
    {
        GFxTextHighlighter* phighlighter = pEditorKit->GetDocument()->CreateHighlighterManager();
        GASSERT(phighlighter);
        GFxTextHighlightDesc* pdesc = phighlighter->GetHighlighterPtr(GFX_COMPOSSTR_HIGHLIGHTING_INDEX);
        GASSERT(pdesc);
        pdesc->StartPos = 0;
        pdesc->Length   = 0;
    }
}
#endif //#ifndef GFC_NO_IME_SUPPORT

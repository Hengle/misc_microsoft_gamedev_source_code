/**********************************************************************

Filename    :   GFxStyledText.h
Content     :   TextFormat object functinality
Created     :   April 17, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXSTYLEDTEXT_H
#define INC_GFXSTYLEDTEXT_H

#include "GUTF8Util.h"
#include "GTLTypes.h"
#include "GRange.h"
#include "GRenderer.h"
#include "GFxStyles.h"
#include "GFxFontResource.h"
#include "GFxFontManager.h"
#include "GFxLog.h"
#include "GFxTextLineBuffer.h"
#include "GFxTextHighlight.h"
#include "GFxGlyphParam.h"
#include "GFxEvent.h"

#include "GFxSGMLParser.h"

#define MAX_FONT_NAME_LEN       50

class GFxTextParagraph;

struct GFxTextHTMLImageTagDesc : public GFxTextImageDesc
{
    GFxString Url;
    GFxString Id;
    SInt      VSpace, HSpace; // in twips
    UInt      ParaId;
    enum
    {
        Align_BaseLine,
        Align_Right,
        Align_Left
    };
    UByte     Alignment;

    GFxTextHTMLImageTagDesc() : VSpace(0), HSpace(0), 
        ParaId(~0u), Alignment(Align_BaseLine) {}
};

// Format descriptor, applying to text
class GFxTextFormat : public GRefCountBase<GFxTextFormat>
{
    // Ranges:
    // fontSize         [0..2500] pts
    // letterSpacing    [-60..60] pts
public:
    friend class HashFunctor;
    struct HashFunctor
    {
        size_t  operator()(const GFxTextFormat& data) const
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
            size_t hash = GTL::gfixed_size_hash<int>::sdbm_hash(&v, sizeof(v) * sizeof(v[0]));
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
    };
protected:
    GFxString   Url;
    GPtr<GFxTextHTMLImageTagDesc>   pImageDesc;
    GPtr<GFxFontHandle>             pFontHandle; // required if font was referenced by id

    UInt32      Color; // format AA RR GG BB.
    SInt16      LetterSpacing; // in twips!
    UInt16      FontSize;      // in pixels!

    enum
    {
        Format_Bold =       0x1,
        Format_Italic =     0x2,
        Format_Underline =  0x4,
        Format_Kerning =    0x8
    };
    UInt8       FormatFlags;

    char        FontName[MAX_FONT_NAME_LEN + 1];

    enum
    {
        PresentMask_Color         = 1,
        PresentMask_LetterSpacing = 2,
        PresentMask_FontName      = 4,
        PresentMask_FontSize      = 8,
        PresentMask_Bold          = 0x10,
        PresentMask_Italic        = 0x20,
        PresentMask_Underline     = 0x40,
        PresentMask_Kerning       = 0x80,
        PresentMask_Url           = 0x100,
        PresentMask_ImageDesc     = 0x200,
        PresentMask_Alpha         = 0x400,
        PresentMask_FontHandle    = 0x800,
    };
    UInt16       PresentMask;

//    void InvalidateCachedFont() { pCachedFont = 0; }
public:

    GFxTextFormat(): Color(0xFF000000u), LetterSpacing(0), FontSize(0), FormatFlags(0), PresentMask(0) { FontName[0] = 0; }
    GFxTextFormat(const GFxTextFormat& srcfmt) : Url(srcfmt.Url), pImageDesc(srcfmt.pImageDesc), 
        pFontHandle(srcfmt.pFontHandle), Color(srcfmt.Color), LetterSpacing(srcfmt.LetterSpacing),
        FontSize(srcfmt.FontSize), FormatFlags(srcfmt.FormatFlags), PresentMask(srcfmt.PresentMask)
    {
        memcpy(FontName, srcfmt.FontName, sizeof(FontName));
    }
    ~GFxTextFormat() 
    {
    }

    void SetBold(bool bold = true);
    void ClearBold()    { FormatFlags &= ~Format_Bold; PresentMask &= ~PresentMask_Bold; }
    bool IsBold() const { return (FormatFlags & Format_Bold) != 0; }

    void SetItalic(bool italic = true);
    void ClearItalic()    { FormatFlags &= ~Format_Italic; PresentMask &= ~PresentMask_Italic; }
    bool IsItalic() const { return (FormatFlags & Format_Italic) != 0; }

    void SetUnderline(bool underline = true);
    void ClearUnderline()    { FormatFlags &= ~Format_Underline; PresentMask &= ~PresentMask_Underline; }
    bool IsUnderline() const { return (FormatFlags & Format_Underline) != 0; }

    void SetKerning(bool kerning = true);
    void ClearKerning()    { FormatFlags &= ~Format_Kerning; PresentMask &= ~PresentMask_Kerning; }
    bool IsKerning() const { return (FormatFlags & Format_Kerning) != 0; }

    // This method sets only RGB values of color
    void SetColor(UInt32 color) { Color = (color & 0xFFFFFF) | (Color & 0xFF000000u); PresentMask |= PresentMask_Color; }
    // This method sets whole color, including the alpha
    void SetColor(const GColor& color) { Color = color.ToColor32(); PresentMask |= PresentMask_Color; }
    UInt32 GetColor() const { return Color; }
    void ClearColor()       { Color = 0xFF000000u; PresentMask &= ~PresentMask_Color; }

    void SetAlpha(UInt8 alpha) { Color = (Color & 0xFFFFFFu) | (UInt32(alpha)<<24); PresentMask |= PresentMask_Alpha; }
    UInt8 GetAlpha() const  { return UInt8((Color >> 24) & 0xFF); }
    void ClearAlpha()       { Color |= 0xFF000000u; PresentMask &= ~PresentMask_Alpha; }

    // letterSpacing is specified in pixels, e.g.: 2.6 pixels, but stored in twips
    void SetLetterSpacing(Float letterSpacing) 
    {
        GASSERT(PixelsToTwips(letterSpacing) >= -32768 && PixelsToTwips(letterSpacing) < 32768); 
        LetterSpacing = (SInt16)PixelsToTwips(letterSpacing); PresentMask |= PresentMask_LetterSpacing; 
    }
    Float GetLetterSpacing() const             { return TwipsToPixels(Float(LetterSpacing)); }
    void SetLetterSpacingInTwips(SInt letterSpacing) 
    { 
        GASSERT(letterSpacing >= -32768 && letterSpacing < 32768); 
        LetterSpacing = (SInt16)letterSpacing; PresentMask |= PresentMask_LetterSpacing; 
    }
    SInt16 GetLetterSpacingInTwips() const     { return LetterSpacing; }
    void ClearLetterSpacing()                  { LetterSpacing = 0; PresentMask &= ~PresentMask_LetterSpacing; }

    void SetFontSize(UInt fontSize) 
    { 
        FontSize = (fontSize <= 65536) ? (UInt16)fontSize : (UInt16)65535u; 
        PresentMask |= PresentMask_FontSize; 
    }
    UInt GetFontSize() const        { return FontSize; }
    void ClearFontSize()            { FontSize = 0; PresentMask &= ~PresentMask_FontSize; }

    void SetFontName(const char* pfontName, UPInt  fontNameSz = GFC_MAX_UPINT);
    void SetFontName(const wchar_t* pfontName, UPInt  fontNameSz = GFC_MAX_UPINT);
    const char* GetFontName() const         { return (IsFontNameSet()) ? FontName : ""; }
    void ClearFontName()                    { PresentMask &= ~PresentMask_FontName; }

    void SetFontHandle(GFxFontHandle* pfontHandle); 
    GFxFontHandle* GetFontHandle() const { return (IsFontHandleSet()) ? pFontHandle : NULL; }
    void ClearFontHandle() { pFontHandle = NULL; PresentMask &= ~PresentMask_FontHandle; }

    void SetImageDesc(GFxTextHTMLImageTagDesc* pimage) { pImageDesc = pimage; PresentMask |= PresentMask_ImageDesc;}
    GFxTextHTMLImageTagDesc* GetImageDesc() const { return (IsImageDescSet()) ? pImageDesc : NULL; }
    void ClearImageDesc() { pImageDesc = NULL; PresentMask &= ~PresentMask_ImageDesc; }

    void SetUrl(const char* purl, UPInt  urlSz = GFC_MAX_UPINT);
    void SetUrl(const wchar_t* purl, UPInt  urlSz = GFC_MAX_UPINT);
    void SetUrl(const GFxString& url);
    const GFxString& GetUrl() const    { return Url; }
    const char* GetUrlCStr() const     { return (IsUrlSet()) ? Url.ToCStr() : ""; }
    void ClearUrl()                    { Url.Resize(0); PresentMask &= ~PresentMask_Url; }

    bool IsBoldSet() const      { return (PresentMask & PresentMask_Bold) != 0; }
    bool IsItalicSet() const    { return (PresentMask & PresentMask_Italic) != 0; }
    bool IsUnderlineSet() const { return (PresentMask & PresentMask_Underline) != 0; }
    bool IsKerningSet() const   { return (PresentMask & PresentMask_Kerning) != 0; }
    bool IsFontSizeSet() const  { return (PresentMask & PresentMask_FontSize) != 0; }
    bool IsFontNameSet() const  { return (PresentMask & PresentMask_FontName) != 0; }
    bool IsFontHandleSet() const{ return (PresentMask & PresentMask_FontHandle) != 0; }
    bool IsImageDescSet() const { return (PresentMask & PresentMask_ImageDesc) != 0; }
    bool IsColorSet() const     { return (PresentMask & PresentMask_Color) != 0; }
    bool IsAlphaSet() const     { return (PresentMask & PresentMask_Alpha) != 0; }
    bool IsLetterSpacingSet() const { return (PresentMask & PresentMask_LetterSpacing) != 0; }
    bool IsUrlSet() const       { return (PresentMask & PresentMask_Url) != 0; }

    GFxTextFormat Merge(const GFxTextFormat& fmt) const;
    GFxTextFormat Intersection(const GFxTextFormat& fmt) const;

    bool operator == (const GFxTextFormat& f) const
    {
        return (PresentMask == f.PresentMask && FormatFlags == f.FormatFlags && Color == f.Color && 
            FontSize == f.FontSize && 
            (!IsFontNameSet() || GFxString::CompareNoCase(FontName, f.FontName) == 0) && LetterSpacing == f.LetterSpacing &&
            (!IsFontHandleSet() || pFontHandle == f.pFontHandle) &&
            (!IsUrlSet() || Url.CompareNoCase(f.Url) == 0) && 
            (pImageDesc == f.pImageDesc));
    }

    bool IsFontSame(const GFxTextFormat& fmt) const; 
    void InitByDefaultValues();
};

template <class T>
class GFxTextFormatPtrWrapper
{
public:
    GPtr<T> pFormat;

    GFxTextFormatPtrWrapper() {}
    GFxTextFormatPtrWrapper(GPtr<T>& ptr) : pFormat(ptr) {}
    GFxTextFormatPtrWrapper(const GPtr<T>& ptr) : pFormat(ptr) {}
    GFxTextFormatPtrWrapper(T* ptr) : pFormat(ptr) {}
    GFxTextFormatPtrWrapper(const T* ptr) : pFormat(const_cast<T*>(ptr)) {}
    GFxTextFormatPtrWrapper(T& ptr) : pFormat(ptr) {}
    GFxTextFormatPtrWrapper(const GFxTextFormatPtrWrapper& orig) : pFormat(orig.pFormat) {}

    GPtr<T>& operator*() { return pFormat; }
    const GPtr<T>& operator*() const { return pFormat; }

    operator GFxTextFormat*() { return pFormat; }
    operator const GFxTextFormat*() const { return pFormat; }

    T* operator-> () { return pFormat; }
    const T* operator-> () const { return pFormat; }

    T* GetPtr() { return pFormat; }
    const T* GetPtr() const { return pFormat; }

    GFxTextFormatPtrWrapper<T>& operator=(const GFxTextFormatPtrWrapper<T>& p)
    {
        pFormat = (*p).GetPtr();
        return *this;
    }
    bool operator==(const GFxTextFormatPtrWrapper<T>& p) const
    {
        return *pFormat == *p.pFormat;
    }
    bool operator==(const T* p) const
    {
        return *pFormat == *p;
    }
    bool operator==(const GPtr<T> p) const
    {
        return *pFormat == *p;
    }

    // hash functor
    struct HashFunctor
    {
        size_t  operator()(const GFxTextFormatPtrWrapper<T>& data) const
        {
            typename T::HashFunctor h;
            return h.operator()(*data.pFormat);
        }
    };
};

typedef GFxTextFormatPtrWrapper<GFxTextFormat> GFxTextFormatPtr;

// Format descriptor, applying to whole paragraph
class GFxTextParagraphFormat : public GRefCountBase<GFxTextParagraphFormat>
{
    // ranges:
    // indent   [-720..720] px
    // leading  [-360..720] px
    // margins  [0..720] px
public:
    enum AlignType
    {
        Align_Left      = 0,
        Align_Right     = 1,
        Align_Justify   = 2,
        Align_Center    = 3
    };
protected:
    UInt*  pTabStops;   // First elem is total num of tabstops, followed by tabstops in twips
    UInt16 BlockIndent;
    SInt16 Indent;
    SInt16 Leading;
    UInt16 LeftMargin;
    UInt16 RightMargin;
    AlignType Align;
    bool Bullet;
    enum
    {
        PresentMask_Alignment     = 0x01,
        PresentMask_BlockIndent   = 0x02,
        PresentMask_Indent        = 0x04,
        PresentMask_Leading       = 0x08,
        PresentMask_LeftMargin    = 0x10,
        PresentMask_RightMargin   = 0x20,
        PresentMask_TabStops      = 0x40,
        PresentMask_Bullet        = 0x80
    };
    UInt8 PresentMask;
public:
    GFxTextParagraphFormat():pTabStops(NULL), BlockIndent(0), Indent(0), Leading(0), 
        LeftMargin(0), RightMargin(0), Align(Align_Left), Bullet(0), PresentMask(0) 
    {}
    
    GFxTextParagraphFormat(const GFxTextParagraphFormat& src) : pTabStops(NULL), 
        BlockIndent(src.BlockIndent), Indent(src.Indent), Leading(src.Leading), 
        LeftMargin(src.LeftMargin), RightMargin(src.RightMargin), 
        Align(src.Align), Bullet(src.Bullet), PresentMask(src.PresentMask)
    {
        CopyTabStops(src.pTabStops);
    }
    ~GFxTextParagraphFormat()
    {
        FreeTabStops();
    }

    void SetAlignment(AlignType align) { Align = align; PresentMask |= PresentMask_Alignment; }
    void ClearAlignment()              { Align = Align_Left; PresentMask &= ~PresentMask_Alignment; }
    bool IsAlignmentSet() const        { return (PresentMask & PresentMask_Alignment) != 0; }
    bool IsLeftAlignment() const       { return IsAlignmentSet() && Align == Align_Left; }
    bool IsRightAlignment() const      { return IsAlignmentSet() && Align == Align_Right; }
    bool IsCenterAlignment() const     { return IsAlignmentSet() && Align == Align_Center; }
    bool IsJustifyAlignment() const    { return IsAlignmentSet() && Align == Align_Justify; }
    AlignType GetAlignment() const     { return Align; }

    void SetBullet(bool bullet = true) { Bullet = bullet; PresentMask |= PresentMask_Bullet; }
    void ClearBullet()                 { Bullet = false; PresentMask &= ~PresentMask_Bullet; }
    bool IsBulletSet() const           { return (PresentMask & PresentMask_Bullet) != 0; }
    bool IsBullet() const              { return IsBulletSet() && Bullet; }

    void SetBlockIndent(UInt value)    { GASSERT(value < 65536); BlockIndent = (UInt16)value; PresentMask |= PresentMask_BlockIndent; }
    UInt GetBlockIndent() const        { return BlockIndent; }
    void ClearBlockIndent()            { BlockIndent = 0; PresentMask &= ~PresentMask_BlockIndent; }
    bool IsBlockIndentSet() const      { return (PresentMask & PresentMask_BlockIndent) != 0; }

    void SetIndent(SInt value)    { GASSERT(value >= -32768 && value < 32768); Indent = (SInt16)value; PresentMask |= PresentMask_Indent; }
    SInt GetIndent() const        { return Indent; }
    void ClearIndent()            { Indent = 0; PresentMask &= ~PresentMask_Indent; }
    bool IsIndentSet() const      { return (PresentMask & PresentMask_Indent) != 0; }

    void SetLeading(SInt value)    { GASSERT(value >= -32768 && value < 32768); Leading = (SInt16)value; PresentMask |= PresentMask_Leading; }
    SInt GetLeading() const        { return Leading; }
    void ClearLeading()            { Leading = 0; PresentMask &= ~PresentMask_Leading; }
    bool IsLeadingSet() const      { return (PresentMask & PresentMask_Leading) != 0; }

    void SetLeftMargin(UInt value)    { GASSERT(value < 65536); LeftMargin = (UInt16)value; PresentMask |= PresentMask_LeftMargin; }
    UInt GetLeftMargin() const        { return LeftMargin; }
    void ClearLeftMargin()            { LeftMargin = 0; PresentMask &= ~PresentMask_LeftMargin; }
    bool IsLeftMarginSet() const      { return (PresentMask & PresentMask_LeftMargin) != 0; }

    void SetRightMargin(UInt value)    { GASSERT(value < 65536); RightMargin = (UInt16)value; PresentMask |= PresentMask_RightMargin; }
    UInt GetRightMargin() const        { return RightMargin; }
    void ClearRightMargin()            { RightMargin = 0; PresentMask &= ~PresentMask_RightMargin; }
    bool IsRightMarginSet() const      { return (PresentMask & PresentMask_RightMargin) != 0; }

    void SetTabStops(UInt num, ...);
    const UInt* GetTabStops(UInt* pnum) const;
    void  SetTabStops(const UInt* psrcTabStops);
    const UInt* GetTabStops() const { return pTabStops; }
    void ClearTabStops()            { FreeTabStops(); PresentMask &= ~PresentMask_TabStops; }
    bool IsTabStopsSet() const      { return (PresentMask & PresentMask_TabStops) != 0; }

    GFxTextParagraphFormat Merge(const GFxTextParagraphFormat& fmt) const;
    GFxTextParagraphFormat Intersection(const GFxTextParagraphFormat& fmt) const;

    bool operator == (const GFxTextParagraphFormat& f) const
    {
        return (PresentMask == f.PresentMask && BlockIndent == f.BlockIndent && Indent == f.Indent && 
            Leading == f.Leading && LeftMargin == f.LeftMargin &&
            RightMargin == f.RightMargin && TabStopsEqual(f.pTabStops) && Align == f.Align && Bullet == f.Bullet);
    }

    void InitByDefaultValues();
protected:
    void AllocTabStops(UInt num);
    void FreeTabStops();
    bool TabStopsEqual(const UInt* psrcTabStops) const;
    void CopyTabStops(const UInt* psrcTabStops);
};

typedef GFxTextFormatPtrWrapper<GFxTextParagraphFormat> GFxTextParagraphFormatPtr;

// Text allocator. Allocates text
class GFxTextAllocator : public GRefCountBase<GFxTextAllocator>
{
    typedef GTL::ghash_set<GFxTextFormatPtr, GFxTextFormatPtr::HashFunctor> TextFormatStorageType;
    typedef GTL::ghash_set<GFxTextParagraphFormatPtr> ParagraphFormatStorageType;

    TextFormatStorageType           TextFormatStorage;
    ParagraphFormatStorageType      ParagraphFormatStorage;
    UInt32                          NewParagraphId;
public:
    GFxTextAllocator() : NewParagraphId(1) {}
    ~GFxTextAllocator() {}

    wchar_t* AllocText(UPInt numChar)
    {
        return (wchar_t*)GALLOC(numChar * sizeof(wchar_t));
    }
    wchar_t* ReallocText(wchar_t* pstr, UPInt oldLength, UPInt newLength)
    {
        GUNUSED(oldLength);
        return (wchar_t*)GREALLOC(pstr, newLength*sizeof(wchar_t));
    }
    void FreeText(wchar_t* pstr)
    {
        GFREE(pstr);
    }

    GFxTextFormat* AllocateTextFormat(const GFxTextFormat& srcfmt)
    {
        GFxTextFormatPtr* ppfmt = TextFormatStorage.get(&srcfmt);
        if (ppfmt)
            return ppfmt->GetPtr();
        GPtr<GFxTextFormat> pfmt = *new GFxTextFormat(srcfmt);
        TextFormatStorage.set(pfmt);
        return pfmt;
    }

    GFxTextParagraphFormat* AllocateParagraphFormat(const GFxTextParagraphFormat& srcfmt)
    {
        GFxTextParagraphFormatPtr* ppfmt = ParagraphFormatStorage.get(&srcfmt);
        if (ppfmt)
            return ppfmt->GetPtr();
        GPtr<GFxTextParagraphFormat> pfmt = *new GFxTextParagraphFormat(srcfmt);
        ParagraphFormatStorage.set(pfmt);
        return pfmt;
    }

    GFxTextParagraph* AllocateParagraph();

    UInt32  AllocateParagraphId() { return NewParagraphId++; }
    UInt32  GetNextParagraphId()  { return NewParagraphId; }
};

class GFxWideStringBuffer
{
    friend class GFxTextAllocator;

    GPtr<GFxTextAllocator> pAllocator;
    wchar_t* pText;
    UPInt    Length;
    UPInt    Allocated;

public:
    GFxWideStringBuffer(): pText(0), Length(0), Allocated(0) {}
    GFxWideStringBuffer(GFxTextAllocator* pallocator):pAllocator(pallocator), pText(0), Length(0), Allocated(0)
    {
        GASSERT(pAllocator);
    }
    GFxWideStringBuffer(const GFxWideStringBuffer& o):pAllocator(o.pAllocator), pText(0), Length(0), Allocated(0)
    {
        if (o.Length > 0)
        {
            SetString(o.pText, o.Length);
        }
    }
    ~GFxWideStringBuffer();

    wchar_t* ToWStr() const
    {
        return pText;
    }
    UPInt GetLength() const { return Length; }

    void SetString(const char* putf8Str, UPInt utf8length = GFC_MAX_UPINT);
    void SetString(const wchar_t* pstr, UPInt length = GFC_MAX_UPINT);
    wchar_t* CreatePosition(UPInt pos, UPInt length);
    void Remove(UPInt pos, UPInt length);
    void Clear() { Length = 0; } // doesn't free mem!

    SPInt StrChr(wchar_t c) { return StrChr(pText, Length, c); }
    void StripTrailingNewLines();


    static SPInt StrChr(const wchar_t* ptext, UPInt length, wchar_t c);
    static UPInt StrLen(const wchar_t* ptext);

    GFxTextAllocator* GetAllocator() const { return pAllocator; }
};

// Represents a paragraph of text. Paragraph - is the text line terminating by new line symbol or '\0'
class GFxTextParagraph : public GNewOverrideBase
{
    friend class GFxStyledText;

    typedef GRangeDataArray<GPtr<GFxTextFormat> >   TextFormatArrayType;
    typedef GRangeData<GPtr<GFxTextFormat> >        TextFormatRunType;

    GFxWideStringBuffer             Text;
    GPtr<GFxTextParagraphFormat>    pFormat;
    TextFormatArrayType             FormatInfo;
    UPInt                           StartIndex;
    UInt32                          UniqueId;
    UInt16                          ModCounter;

    GFxTextAllocator*   GetAllocator() const  { return Text.GetAllocator(); }

    void SetStartIndex(UPInt i) { StartIndex = i; }

    // returns the actual pointer on text format at the specified position.
    // Will return NULL, if there is no text format at the "pos"
    GFxTextFormat* GetTextFormatPtr(UPInt pos) const;

public:
    // this struct is returned by FormatRunIterator::operator*
    struct StyledTextRun
    {
        const wchar_t*  pText;
        SPInt           Index;
        UPInt           Length;
        GPtr<GFxTextFormat>  pFormat;

        StyledTextRun():pText(0), Index(0), Length(0) {}
        StyledTextRun(const wchar_t* ptext, SPInt index, UPInt len, GFxTextFormat* pfmt)
        { Set(ptext, index, len, pfmt); }

        StyledTextRun& Set(const wchar_t* ptext, SPInt index, UPInt len, GFxTextFormat* pfmt)
        {
            pText   = ptext;
            Index   = index;
            Length  = len;
            pFormat = pfmt;
            return *this;
        }
    };

    // Iterates through all format ranges in the paragraph, returning both
    // format structure and text chunks as StyledTextRun
    class FormatRunIterator
    {
        StyledTextRun                       PlaceHolder;
        const TextFormatArrayType*          pFormatInfo;
        TextFormatArrayType::ConstIterator  FormatIterator;
        const GFxWideStringBuffer*          pText;

        UPInt                               CurTextIndex;
    public:
        FormatRunIterator(const TextFormatArrayType& fmts, const GFxWideStringBuffer& textHandle);
        FormatRunIterator(const TextFormatArrayType& fmts, const GFxWideStringBuffer& textHandle, UPInt index);
        FormatRunIterator(const FormatRunIterator& orig);

        FormatRunIterator& operator=(const FormatRunIterator& orig);

        inline bool IsFinished() const  { return CurTextIndex >= pText->GetLength(); }

        const StyledTextRun& operator* ();
        void operator++();
        inline void operator++(int) { operator++(); }

        void SetTextPos(UPInt newTextPos);
    };

    struct CharacterInfo
    {
        GPtr<GFxTextFormat> pFormat;
        UPInt               Index;
        wchar_t             Character;

        CharacterInfo() : pFormat(NULL), Index(0), Character(0) {}
        CharacterInfo(wchar_t c, UPInt index, GFxTextFormat* pf) : pFormat(pf), Index(index), Character(c) {}
    };
    class CharactersIterator
    {
        CharacterInfo                       PlaceHolder;
        const TextFormatArrayType*          pFormatInfo;
        TextFormatArrayType::ConstIterator  FormatIterator;
        const GFxWideStringBuffer*          pText;

        UPInt                               CurTextIndex;
    public:
        CharactersIterator() : pFormatInfo(NULL), pText(NULL), CurTextIndex(0) {}
        CharactersIterator(const GFxWideStringBuffer& buf) : pFormatInfo(NULL), pText(&buf), CurTextIndex(0) {}
        CharactersIterator(const GFxTextParagraph* pparagraph);
        CharactersIterator(const GFxTextParagraph* pparagraph, UPInt index);

        inline bool IsFinished() const { return pText == NULL || CurTextIndex >= pText->GetLength(); }

        CharacterInfo& operator*();
        inline CharacterInfo* operator->() { return &operator*(); }

        void operator++();
        void operator++(int) { operator++(); }
        void operator+=(UPInt n);

        const wchar_t* GetRemainingTextPtr(UPInt * plen) const; 
    };
    CharactersIterator GetCharactersIterator() const            { return CharactersIterator(this); }
    CharactersIterator GetCharactersIterator(UPInt index) const { return CharactersIterator(this, index); }
public:
    GFxTextParagraph(GFxTextAllocator* ptextAllocator):
      Text(ptextAllocator), StartIndex(0), ModCounter(0) 
    {
        GASSERT(ptextAllocator);
        UniqueId = ptextAllocator->AllocateParagraphId();
    }
    GFxTextParagraph() {  } // shouldn't be used! 
    GFxTextParagraph(const GFxTextParagraph& o) : Text(o.Text), 
        StartIndex(o.StartIndex), UniqueId(o.UniqueId), ModCounter(o.ModCounter)
    {}
    ~GFxTextParagraph() {}

    FormatRunIterator GetIterator() const;
    FormatRunIterator GetIteratorAt(UPInt index) const;

    UPInt GetStartIndex() const { return StartIndex; }
    UPInt GetLength() const { return Text.GetLength(); }
    UPInt GetNextIndex() const { return StartIndex + GetLength(); }

    const wchar_t* GetText() const  { return Text.ToWStr(); }
    wchar_t*       GetText()        { return Text.ToWStr(); }

    wchar_t* CreatePosition(UPInt pos, UPInt length = 1);
    void InsertString(const wchar_t* pstr, UPInt pos, UPInt length, const GFxTextFormat* pnewFmt);
    void InsertString(const wchar_t* pstr, UPInt pos, UPInt length = GFC_MAX_UPINT)
    {
        InsertString(pstr, pos, length, NULL);
    }
    // AppendPlainText methods doesn't expand format ranges
    void AppendPlainText(const wchar_t* pstr, UPInt length = GFC_MAX_UPINT);
    void AppendPlainText(const char* putf8str, UPInt utf8StrSize = GFC_MAX_UPINT);

    void Remove(UPInt startPos, UPInt endPos = GFC_MAX_UPINT);

    void SetText(const wchar_t* pstring, UPInt length = GFC_MAX_UPINT);
    void SetTextFormat(const GFxTextFormat& fmt, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);
    GFxTextFormat GetTextFormat(UPInt startPos, UPInt endPos = GFC_MAX_UPINT) const;

    void SetFormat(const GFxTextParagraphFormat& fmt);
    const GFxTextParagraphFormat* GetFormat() const { return pFormat.GetPtr(); }

    UInt32 GetId() const        { return UniqueId; }
    UInt16 GetModCounter() const{ return ModCounter; }

    // copies text and formatting from psrcPara paragraph to this one, starting from startSrcIndex in
    // source paragraph and startDestIndex in destination one. The "length" specifies the number
    // of positions to copy.
    void Copy(const GFxTextParagraph& psrcPara, UPInt startSrcIndex, UPInt startDestIndex, UPInt length);

    // Shrinks the paragraph's length by the "delta" value.
    void Shrink(UPInt delta);

#ifdef GFC_BUILD_DEBUG
    void CheckIntegrity() const;
#else
    void CheckIntegrity() const {}
#endif
};

class GFxStyledText : public GRefCountBase<GFxStyledText>
{
    friend class GFxTextDocView;
public:
    class ParagraphPtrWrapper
    {
        mutable GFxTextParagraph* pPara;
    public:

        ParagraphPtrWrapper():pPara(NULL) {}
        ParagraphPtrWrapper(GFxTextParagraph* ptr) : pPara(ptr) {}
        ParagraphPtrWrapper(const GFxTextParagraph* ptr) : pPara(const_cast<GFxTextParagraph*>(ptr)) {}

        ParagraphPtrWrapper(const ParagraphPtrWrapper& ptr) : pPara(ptr.pPara) { ptr.pPara = NULL; }
        ~ParagraphPtrWrapper() { delete pPara; }

        GFxTextParagraph& operator*() { return *pPara; }
        const GFxTextParagraph& operator*() const { return *pPara; }

        GFxTextParagraph* operator-> () { return pPara; }
        const GFxTextParagraph* operator-> () const { return pPara; }

        GFxTextParagraph* GetPtr() { return pPara; }
        const GFxTextParagraph* GetPtr() const { return pPara; }

        operator GFxTextParagraph*()                { return pPara; }
        operator const GFxTextParagraph*() const    { return pPara; }

        ParagraphPtrWrapper& operator=(const GFxTextParagraph* p)
        {
            if (pPara != const_cast<GFxTextParagraph*>(p))
                delete pPara;
            pPara = const_cast<GFxTextParagraph*>(p);
            return *this;
        }
        ParagraphPtrWrapper& operator=(GFxTextParagraph* p)
        {
            if (pPara != p)
                delete pPara;
            pPara = p;
            return *this;
        }
        ParagraphPtrWrapper& operator=(const ParagraphPtrWrapper& p)
        {
            if (pPara != p.pPara)
                delete pPara;
            pPara = p.pPara;
            p.pPara = NULL;
            return *this;
        }
        bool operator==(const ParagraphPtrWrapper& p) const
        {
            return pPara == p.pPara;
        }
        bool operator==(const GFxTextParagraph* p) const
        {
            return pPara == p;
        }
    };
    struct HTMLImageTagInfo
    {
        GPtr<GFxTextImageDesc> pTextImageDesc;
        GFxString Url;
        GFxString Id;
        UInt      Width, Height; // in twips
        SInt      VSpace, HSpace; // in twips
        UInt      ParaId;
        enum
        {
            Align_BaseLine,
            Align_Right,
            Align_Left
        };
        UByte     Alignment;

        HTMLImageTagInfo() : Width(0), Height(0), VSpace(0), HSpace(0), 
            ParaId(~0u), Alignment(Align_BaseLine) {}
    };

    typedef GTL::garray<HTMLImageTagInfo>       HTMLImageTagInfoArray;
    typedef GTL::garray<ParagraphPtrWrapper>    ParagraphArrayType;
    typedef ParagraphArrayType::Iterator        ParagraphsIterator;

    struct CharacterInfo
    {
        GPtr<GFxTextFormat> pOriginalFormat;
        GFxTextParagraph*   pParagraph;
        UPInt               Index;
        wchar_t             Character;

        CharacterInfo() : Index(0), Character(0) {}
    };

    class CharactersIterator
    {
        ParagraphsIterator                      Paragraphs;
        GFxTextParagraph::CharactersIterator    Characters;
        GPtr<GFxStyledText>                     pText;
        UPInt                                   FirstCharInParagraphIndex;
        CharacterInfo                           CharInfoPlaceHolder;
    public:
        CharactersIterator(GFxStyledText* ptext);
        CharactersIterator(GFxStyledText* ptext, SInt index);
        inline bool IsFinished() const { return Characters.IsFinished() && Paragraphs.IsFinished(); }

        CharacterInfo& operator*();
        void operator++();
        inline void operator++(int) { operator++(); }
    };
private:
    struct ParagraphComparator
    {
        int Compare(const GFxTextParagraph* pp1, UPInt index) const
        {
            UPInt si1 = pp1->GetStartIndex();
            if (index >= si1 && index < (si1 + pp1->GetLength()))
                return 0; 
            return (int)(si1 - index);
        }
    };

public:
    GFxStyledText();
    GFxStyledText(GFxTextAllocator* pallocator);

    void Clear();

    UPInt GetLength() const;

    void SetText(const char* putf8String, UPInt stringSize = GFC_MAX_UPINT);
    void SetText(const wchar_t* pstring, UPInt length = GFC_MAX_UPINT);
    
    GFxString GetText() const;
    void GetText(GFxWideStringBuffer* pBuffer) const;

    void SetHtml(const GFxString&);
    void SetHtml(const wchar_t* pstring, UPInt length = GFC_MAX_UPINT);
    GFxString GetHtml() const;

    void GetTextAndParagraphFormat(GFxTextFormat* pdestTextFmt, GFxTextParagraphFormat* pdestParaFmt, UPInt startPos, UPInt endPos = GFC_MAX_UPINT);
    bool GetTextAndParagraphFormat(const GFxTextFormat** ppdestTextFmt, const GFxTextParagraphFormat** pdestParaFmt, UPInt pos);
    void SetTextFormat(const GFxTextFormat& fmt, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);
    void SetParagraphFormat(const GFxTextParagraphFormat& fmt, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);

    // assign pdefaultTextFmt as a default text format
    void SetDefaultTextFormat(const GFxTextFormat* pdefaultTextFmt);
    // makes a copy of text format and assign
    void SetDefaultTextFormat(const GFxTextFormat& defaultTextFmt);
    // assign pdefaultTextFmt as a default text format
    void SetDefaultParagraphFormat(const GFxTextParagraphFormat* pdefaultParagraphFmt);
    // makes a copy of text format and assign
    void SetDefaultParagraphFormat(const GFxTextParagraphFormat& defaultParagraphFmt);

    const GFxTextFormat*          GetDefaultTextFormat() const 
        { GASSERT(pDefaultTextFormat); return pDefaultTextFormat; }
    const GFxTextParagraphFormat* GetDefaultParagraphFormat() const 
        { GASSERT(pDefaultParagraphFormat); return pDefaultParagraphFormat; }

    void SetParagraphFormatByIndex(const GFxTextParagraph& fmt, UInt paragraphIndex);

    wchar_t* CreatePosition(UPInt pos, UPInt length = 1);
    void AppendString(const char* putf8String, UPInt stringSize = GFC_MAX_UPINT);
    void AppendString(const char* putf8String, UPInt stringSize, 
                      const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt);
    void AppendString(const wchar_t* pstr, UPInt length = ~0u);
    void AppendString(const wchar_t* pstr, UPInt length,
                      const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt);

    void InsertString(const wchar_t* pstr, UPInt pos, UPInt length = GFC_MAX_UPINT);
    void InsertString(const wchar_t* pstr, UPInt pos, UPInt length, 
        const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt);
    void Remove(UPInt startPos, UPInt endPos);

    CharactersIterator GetCharactersIterator() { return CharactersIterator(this); }

    void SetNewLine0D()        { RTFlags |= RTFlags_NewLine0D; }
    void ClearNewLine0D()      { RTFlags &= (~(RTFlags_NewLine0D)); }
    bool IsNewLine0D() const   { return (RTFlags & (RTFlags_NewLine0D)) != 0; }
    unsigned char NewLineChar() const { return IsNewLine0D() ? '\r' : '\n'; }
    const char*   NewLineStr()  const { return IsNewLine0D() ? "\r" : "\n"; }

#ifdef GFC_BUILD_DEBUG
    void CheckIntegrity() const;
#else
    void CheckIntegrity() const {}
#endif
protected:
    GFxTextAllocator* GetAllocator() const
    {
        if (!pTextAllocator)
            pTextAllocator = *new GFxTextAllocator();
        return pTextAllocator;
    }

    void SetMayHaveUrl()        { RTFlags |= RTFlags_MayHaveUrl; }
    void ClearMayHaveUrl()      { RTFlags &= (~(RTFlags_MayHaveUrl)); }
    bool MayHaveUrl() const     { return (RTFlags & (RTFlags_MayHaveUrl)) != 0; }

    ParagraphsIterator GetParagraphByIndex(UPInt index, UPInt* pindexInParagraph = NULL);
    ParagraphsIterator GetNearestParagraphByIndex(UPInt index, UPInt* pindexInParagraph = NULL);
    ParagraphsIterator GetParagraphIterator() { return Paragraphs.Begin(); }
    ParagraphsIterator GetParagraphIterator() const { return const_cast<GFxStyledText*>(this)->Paragraphs.Begin(); } //!AB need to use const iter!

    GFxTextParagraph* AppendNewParagraph(const GFxTextParagraphFormat* pdefParaFmt = NULL);
    GFxTextParagraph* InsertNewParagraph(GFxStyledText::ParagraphsIterator& iter, 
                                         const GFxTextParagraphFormat* pdefParaFmt = NULL);

    GFxTextParagraph* GetLastParagraph();

    template <class Char>
    bool ParseHtml(const Char* phtml, UPInt  htmlSize, HTMLImageTagInfoArray* pimgInfoArr = NULL, 
                   bool multiline = true, bool condenseWhite = false);

    // atomic callbacks. These methods might be invoked multiple times 
    // from inside of the InsertString or Remove methods. Client SHOULD NOT do
    // anything "heavy" in these handlers, like re-formatting (though, the flag 
    // "re-formatting is neeeded" might be set).
    virtual void OnParagraphTextFormatChanging(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const GFxTextFormat& formatToBeSet);
    virtual void OnParagraphTextFormatChanged(const GFxTextParagraph& para, UPInt startPos, UPInt endPos);
    virtual void OnParagraphFormatChanging(const GFxTextParagraph& para, const GFxTextParagraphFormat& formatToBeSet);
    virtual void OnParagraphFormatChanged(const GFxTextParagraph& para);
    virtual void OnParagraphTextInserting(const GFxTextParagraph& para, UPInt insertionPos, UPInt insertingLen);
    virtual void OnParagraphTextInserted(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const wchar_t* ptextInserted);
    virtual void OnParagraphTextRemoving(const GFxTextParagraph& para, UPInt removingPos, UPInt removingLen);
    virtual void OnParagraphTextRemoved(const GFxTextParagraph& para, UPInt removedPos, UPInt removedLen);
    virtual void OnParagraphRemoving(const GFxTextParagraph& para);

    // This callback will be fired at the end of big change, like InsertString, Remove, etc
    virtual void OnDocumentChanged() {}

protected: //data
    mutable GPtr<GFxTextAllocator>  pTextAllocator;
    ParagraphArrayType              Paragraphs;
    GPtr<GFxTextParagraphFormat>    pDefaultParagraphFormat;
    GPtr<GFxTextFormat>             pDefaultTextFormat;

    enum 
    {
        RTFlags_MayHaveUrl = 0x1,
        RTFlags_NewLine0D  = 0x2 // use '\r' as internal new-line char
    };
    UByte                           RTFlags;
};

class GFxTextEditorKit;
struct GFxLineCursor;

template <typename T>
struct CachedPrimValue
{
    mutable T       Value;
    mutable UInt16  FormatCounter;

    CachedPrimValue():FormatCounter(0) {}
    CachedPrimValue(T v):Value(v), FormatCounter(0) {}
    CachedPrimValue(T v, UInt16 counter):Value(v), FormatCounter(counter) {}

    void SetValue(T v, UInt16 counter)
    {
        Value = v; FormatCounter = counter;
    }
    operator T() const { return Value; }
    bool IsValid(UInt16 fmtCnt)
    {
        return FormatCounter == fmtCnt;
    }
};

template <typename T>
struct CachedValue
{
    mutable T       Value;
    mutable UInt16  FormatCounter;

    CachedValue():FormatCounter(0) {}
    CachedValue(const T& v):Value(v), FormatCounter(0) {}
    CachedValue(const T& v, UInt16 counter):Value(v), FormatCounter(counter) {}

    void SetValue(const T& v, UInt16 counter)
    {
        Value = v; FormatCounter = counter;
    }
    T& GetValue() const { return Value; }
    T& operator*() const { return Value; }
    T* operator->() const { return &Value; }
    //operator T&() const { return Value; }
    bool IsValid(UInt16 fmtCnt)
    {
        return FormatCounter == fmtCnt;
    }
    void Invalidate() { --FormatCounter; }
};


class GFxTextDocView : public GRefCountBase<GFxTextDocView>
{
    friend class GFxTextEditorKit;

    typedef GRenderer::BitmapDesc   BitmapDesc;
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;
    typedef GTL::ghash_identity<const GFxTextFormat*, GPtr<GFxFontHandle> > FontCache;
public:
    enum CommandType
    {
        InsertChar,
        InsertSubstr,
        DeleteChar,
        DeleteSubstr,
        Replace
    };
    struct InsertCharCommand
    {
        UPInt   PosAt;
        wchar_t CharCode;

        InsertCharCommand(UPInt pos, wchar_t c):PosAt(pos), CharCode(c) {}
    };
    struct InsertSubstrCommand
    {
        UPInt   PosAt;
        const wchar_t* pStr;
        UPInt   Length;

        InsertSubstrCommand(UPInt pos, const wchar_t* pstr, UPInt len):PosAt(pos), pStr(pstr), Length(len) {}
    };
    struct DeleteCharCommand
    {
        UPInt   PosAt;

        DeleteCharCommand(UPInt pos):PosAt(pos) {}
    };
    class DocumentListener : public GRefCountBase<DocumentListener>
    {
    public:
        // view-related events
        virtual void    View_OnHScroll(GFxTextDocView& , UInt newScroll) { GUNUSED(newScroll); }
        virtual void    View_OnVScroll(GFxTextDocView& , UInt newScroll) { GUNUSED(newScroll); }

        // editor-related events
        virtual UInt    Editor_OnKey(GFxTextEditorKit& , UInt keyCode) { return keyCode; }
        virtual wchar_t Editor_OnCharacter(GFxTextEditorKit& , wchar_t srcChar) { return srcChar; }
        virtual void    Editor_OnChanged(GFxTextEditorKit& ) {  }
    };

    // View's part
    enum ViewAlignment
    {
        Align_Left   = 0,
        Align_Right  = 1,
        Align_Center = 2
    };

    struct ImageSubstitutor : public GNewOverrideBase
    {
        struct Element
        {
            wchar_t                 SubString[20];
            GPtr<GFxTextImageDesc>  pImageDesc;
            UByte                   SubStringLen;
        };
        GTL::garray<Element>        Elements;

        GFxTextImageDesc* FindImageDesc(const wchar_t* pstr, UPInt  maxlen, UPInt * ptextLen = NULL);
        void AddImageDesc(const Element& elem);
        void RemoveImageDesc(GFxTextImageDesc*);
    };
protected:
    // Document's part
    class DocumentText : public GFxStyledText
    {
        GFxTextDocView* pDocument;
    public:
        DocumentText(GFxTextDocView* pdoc) : pDocument(pdoc) {}
        virtual void OnParagraphRemoving(const GFxTextParagraph& para);
        virtual void OnParagraphTextInserted(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const wchar_t* ptextInserted);
        virtual void OnParagraphTextRemoved(const GFxTextParagraph& para, UPInt removedPos, UPInt removedLen);
    };
    GPtr<DocumentText>              pDocument;
    GPtr<GFxFontManager>            pFontManager;
    GPtr<DocumentListener>          pDocumentListener;

    // View's part
    ImageSubstitutor*               pImageSubstitutor;
    GFxTextLineBuffer               LineBuffer;
    GRectF                          ViewRect; // total rectangle occupied by the view
    UInt                            TextWidth;  // in twips
    UInt                            TextHeight; // in twips
    CachedPrimValue<UInt>           MaxVScroll; 

    GPtr<GFxTextEditorKit>          pEditorKit;
    UInt16                          FormatCounter; // being incremented each Format call
    ViewAlignment                   Alignment;

    enum
    {
        Flags_AutoSizeX = 0x1,
        Flags_AutoSizeY = 0x2,
        Flags_Multiline = 0x4,
        Flags_WordWrap  = 0x8,
        Flags_PasswordMode  = 0x10,
        Flags_UseDeviceFont = 0x20,
        Flags_AAReadability = 0x40,
        Flags_AutoFit       = 0x80
    };
    UInt8                           Flags;
    enum
    {
        RTFlags_ReformatReq         = 0x1,
        RTFlags_CompleteReformatReq = 0x2,
        RTFlags_CantBeEditable      = 0x4 // set if Translate was used
    };
    UInt8                           RTFlags; // run-time flags

    GFxTextFilter                   Filter;

    void ClearReformatReq()   { RTFlags &= (~(RTFlags_ReformatReq | RTFlags_CompleteReformatReq)); }
    bool IsReformatReq() const{ return (RTFlags & (RTFlags_ReformatReq | RTFlags_CompleteReformatReq)) != 0; }

    void ClearCompleteReformatReq()   { RTFlags &= (~RTFlags_CompleteReformatReq); }
    bool IsCompleteReformatReq() const{ return (RTFlags & RTFlags_CompleteReformatReq) != 0; }

    void SetCantBeEditable()     { RTFlags |= RTFlags_CantBeEditable; }
    void ClearCantBeEditable()   { RTFlags &= (~RTFlags_CantBeEditable); }
    bool CanBeEditable() const   { return (RTFlags & RTFlags_CantBeEditable) == 0; }

    struct FindFontInfo
    {
        FontCache*              pFontCache;
        const GFxTextFormat*    pCurrentFormat;
        const GFxTextFormat*    pPrevFormat;
        GPtr<GFxFontHandle>     pCurrentFont;

        FindFontInfo():pFontCache(NULL) { init(); }
        explicit FindFontInfo(FontCache* pfc):pFontCache(pfc) { init(); }

        inline void init() { pCurrentFormat = pPrevFormat = NULL; pCurrentFont = NULL; }
    };

    struct FormatParagraphInfo
    {
        GFxTextLineBuffer           LineBuffer;
        // in-out values
        GFxTextLineBuffer::Iterator* pLinesIter;
        FontCache*                  pFontCache;
        UByte                       TempLineBuff[1024];
        GFxTextLineBuffer::Line*    pDynLine;
        Float                       NextOffsetY;

        // in values
        Float                       ParaYOffset;
        GFxTextParagraph::CharactersIterator CharIter;
        GFxLog*                     pLog;

        // out values
        SInt                        ParaWidth;
        SInt                        ParaHeight;
        UInt                        ParaLines;
        bool                        NeedRecenterLines;

        FormatParagraphInfo()  { pDynLine = NULL; }
        ~FormatParagraphInfo() { LineBuffer.LineAllocator.FreeLine(pDynLine); }
    };

    GFxFontHandle* FindFont(FindFontInfo* pfontInfo);
    void FormatParagraph(const GFxTextParagraph& paragraph, FormatParagraphInfo* pinfo);

    void FinalizeLine(GFxTextLineBuffer::Line* pcurLine, 
        FormatParagraphInfo* pinfo, const GFxTextParagraphFormat& paraFormat, 
        GFxLineCursor& lineCursor);
public:
    // types of view notifications, used for NotifyViewsChanged/OnDocumentChanged
    enum ViewNotificationMasks
    {
        ViewNotify_SignificantMask          = 0x100,
        ViewNotify_FormatChange             = 0x1,
        ViewNotify_TextChange               = 0x2,
        ViewNotify_ScrollingParamsChange    = 0x4,

        ViewNotify_SignificantFormatChange  = ViewNotify_SignificantMask | ViewNotify_FormatChange,
        ViewNotify_SignificantTextChange    = ViewNotify_SignificantMask | ViewNotify_TextChange,
    };

protected:
    void NotifyParagraphRemoving(const GFxTextParagraph& para);
public:
    GFxTextDocView(GFxFontManager* pfontMgr);
    ~GFxTextDocView();

    UPInt GetLength() const { return pDocument->GetLength(); }
    GFxString GetText() const;
    GFxString GetHtml() const;

    GFxStyledText* GetStyledText() const { return pDocument; }
    GFxFontManager* GetFontManager() const { return pFontManager; }

    void SetText(const char* putf8String, UPInt stringSize = GFC_MAX_UPINT);
    void SetText(const wchar_t* pstring, UPInt length = GFC_MAX_UPINT);

    void ParseHtml(const GFxString& str, bool condenseWhite, 
                   GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr = NULL)
    {
        ParseHtml(str.ToCStr(), str.GetLength(), condenseWhite, pimgInfoArr);
    }
    void ParseHtml(const char* putf8Str, UPInt utf8Len, bool condenseWhite, 
                   GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr = NULL);
    void ParseHtml(const char* putf8Str, UPInt utf8Len = GFC_MAX_UPINT, 
                   GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr = NULL)
    {
        ParseHtml(putf8Str, utf8Len, false, pimgInfoArr);
    }
    void ParseHtml(const wchar_t* pwStr, UPInt strLen = GFC_MAX_UPINT, bool condenseWhite = false, 
                   GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr = NULL);

    bool MayHaveUrl() const { return pDocument->MayHaveUrl(); }

    // formatting
    void SetTextFormat(const GFxTextFormat& fmt, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);
    void SetParagraphFormat(const GFxTextParagraphFormat& fmt, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);
    void GetTextAndParagraphFormat
        (GFxTextFormat* pdestTextFmt, 
         GFxTextParagraphFormat* pdestParaFmt, 
         UPInt startPos, UPInt endPos = GFC_MAX_UPINT)
    {
        pDocument->GetTextAndParagraphFormat(pdestTextFmt, pdestParaFmt, startPos, endPos);
    }
    bool GetTextAndParagraphFormat(const GFxTextFormat** ppdestTextFmt, const GFxTextParagraphFormat** pdestParaFmt, UPInt pos)
    {
        return pDocument->GetTextAndParagraphFormat(ppdestTextFmt, pdestParaFmt, pos);
    }

    // assign pdefaultTextFmt as a default text format
    void SetDefaultTextFormat(const GFxTextFormat* pdefaultTextFmt)
        { pDocument->SetDefaultTextFormat(pdefaultTextFmt); }
    // makes a copy of text format and assign
    void SetDefaultTextFormat(const GFxTextFormat& defaultTextFmt)
        { pDocument->SetDefaultTextFormat(defaultTextFmt); }
    // assign pdefaultTextFmt as a default text format
    void SetDefaultParagraphFormat(const GFxTextParagraphFormat* pdefaultParagraphFmt)
        { pDocument->SetDefaultParagraphFormat(pdefaultParagraphFmt); }
    // makes a copy of text format and assign
    void SetDefaultParagraphFormat(const GFxTextParagraphFormat& defaultParagraphFmt)
        { pDocument->SetDefaultParagraphFormat(defaultParagraphFmt); }

    const GFxTextFormat*          GetDefaultTextFormat() const { return pDocument->GetDefaultTextFormat(); }
    const GFxTextParagraphFormat* GetDefaultParagraphFormat() const { return pDocument->GetDefaultParagraphFormat(); }

    void SetDocumentListener(DocumentListener* pdocumentListener) { pDocumentListener = pdocumentListener; }
    DocumentListener* GetDocumentListener() const { return pDocumentListener; }

    // text modification
    void AppendText(const wchar_t* pwStr, UPInt strLen = GFC_MAX_UPINT);
    void AppendText(const char* putf8Str, UPInt utf8Len = GFC_MAX_UPINT);

    void AppendHtml(const wchar_t* pwStr, UPInt strLen = GFC_MAX_UPINT, bool condenseWhite = false);
    void AppendHtml(const char* putf8Str, UPInt utf8Len = GFC_MAX_UPINT, bool condenseWhite = false);

    void InsertText(const wchar_t* pstr, UPInt startPos);
    void ReplaceText(const wchar_t* pstr, UPInt startPos, UPInt endPos);
    void RemoveText(UPInt startPos, UPInt endPos);

    // edit
    void EditCommand(CommandType cmdId, const void* command);

    // view
    ImageSubstitutor* CreateImageSubstitutor()
    {
        if (!pImageSubstitutor)
            pImageSubstitutor = new ImageSubstitutor;
        return pImageSubstitutor;
    }
    ImageSubstitutor* GetImageSubstitutor() const { return pImageSubstitutor; }
    void ClearImageSubstitutor() { delete pImageSubstitutor; pImageSubstitutor = NULL; }
    void Close();

    // notifications from document
    // Notifies the view the document was updated. "notifyMask" represents
    // the combination of GFxTextDocView::ViewNotificationMasks
    virtual void OnDocumentChanged(UInt notifyMask);
    virtual void OnDocumentParagraphRemoving(const GFxTextParagraph& para);

    void SetReformatReq()           { RTFlags |= RTFlags_ReformatReq; }
    void SetCompleteReformatReq()   { RTFlags |= RTFlags_CompleteReformatReq; }

    void Format(GFxLog* plog = 0);

    void Display(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx, bool nextFrame, GFxLog* plog);

    void SetAutoSizeX();
    void ClearAutoSizeX()   { Flags &= (~Flags_AutoSizeX); }
    bool IsAutoSizeX() const{ return (Flags & Flags_AutoSizeX); }

    void SetAutoSizeY();
    void ClearAutoSizeY()   { Flags &= (~Flags_AutoSizeY); }
    bool IsAutoSizeY() const{ return (Flags & Flags_AutoSizeY) != 0; }

    void SetMultiline()     { Flags |= Flags_Multiline; }
    void ClearMultiline()   { Flags &= (~Flags_Multiline); }
    bool IsMultiline() const{ return (Flags & Flags_Multiline) != 0; }

    void SetWordWrap();
    void ClearWordWrap();
    bool IsWordWrap() const { return (Flags & Flags_WordWrap) != 0 && !IsAutoSizeX(); }

    void SetPasswordMode()     { Flags |= Flags_PasswordMode; }
    void ClearPasswordMode()   { Flags &= (~Flags_PasswordMode); }
    bool IsPasswordMode() const{ return (Flags & Flags_PasswordMode) != 0; }

    void SetUseDeviceFont()     { Flags |= Flags_UseDeviceFont; }
    void ClearUseDeviceFont()   { Flags &= (~Flags_UseDeviceFont); }
    bool DoesUseDeviceFont() const{ return (Flags & Flags_UseDeviceFont) != 0; }

    void SetAAForReadability()     { Flags |= Flags_AAReadability; }
    void ClearAAForReadability()   { Flags &= (~Flags_AAReadability); }
    bool IsAAForReadability() const{ return (Flags & Flags_AAReadability) != 0; }

    void SetAutoFit()     { Flags |= Flags_AutoFit; }
    void ClearAutoFit()   { Flags &= (~Flags_AutoFit); }
    bool IsAutoFit() const{ return (Flags & Flags_AutoFit) != 0; }

    void SetFilters(const GFxTextFilter& f) { Filter = f; }
    void SetDefaultShadow()  { Filter.SetDefaultShadow(); }
    void EnableSoftShadow()  { Filter.ShadowFlags &= ~GFxTextFieldParam::ShadowDisabled; }
    void DisableSoftShadow() { Filter.ShadowFlags |=  GFxTextFieldParam::ShadowDisabled; }

    void  SetBlurX(Float v) { Filter.BlurX = Filter.FloatToFixed44(v); }
    Float GetBlurX() const  { return Filter.Fixed44toFloat(Filter.BlurX); }

    void  SetBlurY(Float v) { Filter.BlurY = Filter.FloatToFixed44(v); }
    Float GetBlurY() const  { return Filter.Fixed44toFloat(Filter.BlurY); }

    void  SetBlurStrength(Float v) { Filter.BlurStrength = Filter.FloatToFixed44(v); }
    Float GetBlurStrength() const  { return Filter.Fixed44toFloat(Filter.BlurStrength); }

    void  SetShadowBlurX(Float v) { Filter.ShadowBlurX = Filter.FloatToFixed44(v); }
    Float GetShadowBlurX() const  { return Filter.Fixed44toFloat(Filter.ShadowBlurX); }

    void  SetShadowBlurY(Float v) { Filter.ShadowBlurY = Filter.FloatToFixed44(v); }
    Float GetShadowBlurY() const  { return Filter.Fixed44toFloat(Filter.ShadowBlurY); }

    void  SetShadowStrength(Float v) { Filter.ShadowStrength = Filter.FloatToFixed44(v); }
    Float GetShadowStrength() const  { return Filter.Fixed44toFloat(Filter.ShadowStrength); }

    void  SetKnockOut(bool f) { if(f) Filter.ShadowFlags |= GFxGlyphParam::KnockOut; else Filter.ShadowFlags &= ~GFxGlyphParam::KnockOut; }
    bool  IsKnockOut()  const { return (Filter.ShadowFlags & GFxGlyphParam::KnockOut) != 0; }

    void  SetHideObject(bool f) { if(f) Filter.ShadowFlags |= GFxGlyphParam::HideObject; else Filter.ShadowFlags &= ~GFxGlyphParam::HideObject; }
    bool  IsHiddenObject()  const { return (Filter.ShadowFlags & GFxGlyphParam::HideObject) != 0; }

    void  SetShadowQuality(UInt v) { if(v > 1) Filter.ShadowFlags |= GFxGlyphParam::FineBlur; else Filter.ShadowFlags &= ~GFxGlyphParam::FineBlur; }
    UInt  GetShadowQuality() const { return (Filter.ShadowFlags & GFxGlyphParam::FineBlur) ? 2 : 1; } 

    void  SetShadowAngle(Float a) { Filter.ShadowAngle = SInt16(fmodf(a,360) * 10); Filter.UpdateShadowOffset(); }
    Float GetShadowAngle() const  { return Float(Filter.ShadowAngle) / 10.0f; }

    void  SetShadowDistance(Float d) { Filter.ShadowDistance = SInt16(d * 20); Filter.UpdateShadowOffset(); }
    Float GetShadowDistance() const  { return Float(Filter.ShadowDistance) / 20.0f; }

    void  SetShadowColor(UInt c) { Filter.ShadowColor = (c & 0xFFFFFFu) | Filter.ShadowAlpha << 24; }
    UInt  GetShadowColor() const { return Filter.ShadowColor.ToColor32() & 0xFFFFFFu; }

    void  SetShadowAlpha(Float a) { Filter.ShadowAlpha = UByte(GTL::gclamp(a * 255.0f, 0.0f, 255.0f)); Filter.ShadowColor.SetAlpha(Filter.ShadowAlpha); }
    Float GetShadowAlpha() const  { return Float(Filter.ShadowAlpha) / 255.0f; }

    void SetViewRect(const GRectF& rect);
    const GRectF& GetViewRect();

    // rectangle used for painting text (ViewRect - GUTTER)
    const GRectF& GetTextRect() const { return LineBuffer.Geom.VisibleRect; }

    bool SetHScrollOffset(UInt hscroll);    
    bool SetVScrollOffset(UInt vscroll);    

    UInt GetHScrollOffset() const { return LineBuffer.GetHScrollOffset(); }    
    UInt GetVScrollOffset() const { return LineBuffer.GetFirstVisibleLineIndex(); }    

    Float GetTextWidth();
    Float GetTextHeight();
    UInt  GetLinesCount();

    UInt  GetMaxHScroll();
    UInt  GetMaxVScroll();

    // Returns true, if reformat actually occurred
    bool ForceReformat();

    void SetAlignment(ViewAlignment alignment);
    ViewAlignment GetAlignment() const { return Alignment; }

    bool IsUrlAtPoint(Float x, Float y);

    // Returns the zero-based index value of the character at the point specified by the x  and y parameters.
    UPInt GetCharIndexAtPoint(Float x, Float y);
    // Returns the zero-based index value of the cursor at the point specified by the x  and y parameters.
    UPInt GetCursorPosAtPoint(Float x, Float y);
    // Given a character index, returns the index of the first character in the same paragraph. 
    UPInt GetFirstCharInParagraph(UPInt indexOfChar);
    // Returns the zero-based index value of the line at the point specified by the x  and y parameters.
    UInt GetLineIndexAtPoint(Float x, Float y);
    // Returns the zero-based index value of the line containing the character specified by the indexOfChar parameter.
    UInt GetLineIndexOfChar(UPInt indexOfChar);
    // Returns the character index of the first character in the line that the lineIndex parameter specifies.
    UPInt GetLineOffset(UInt lineIndex);
    // Returns the number of characters in a specific text line.
    UPInt GetLineLength(UInt lineIndex);
    
    // Retrieve a rectangle (in twips) that is the bounding box of the character.
    // X and Y coordinates are in textfield's coordinate space and they does not reflect
    // current scrolling settings. Thus, if the character belongs to the line with large index
    // its Y-coordinate will be cumulative value of all prior line's heights, even if
    // it is scrolled and currently visible.
    bool GetCharBoundaries(GRectF* pCharRect, UPInt indexOfChar);

    // Retrieve a rectangle (in twips) that is the bounding box of the character.
    // X and Y coordinates are in textfield's coordinate space and they does not reflect
    // current scrolling settings. Thus, if the character belongs to the line with large index
    // its Y-coordinate will be cumulative value of all prior line's heights, even if
    // it is scrolled and currently visible.
    // This method returns better rectangle than GetCharBoundaries does. It calculates
    // the width using the glyph's width rather than advance.
    bool GetExactCharBoundaries(GRectF* pCharRect, UPInt indexOfChar);

    // returns the bottommost line index that is currently visible
    UInt GetBottomVScroll();
    bool SetBottomVScroll(UInt newBottomMostLine);

    GFxTextEditorKit* CreateEditorKit();
    GFxTextEditorKit* GetEditorKit();
    bool HasEditorKit() const { return (pEditorKit.GetPtr() != NULL); }

    unsigned char NewLineChar() const { return pDocument->NewLineChar(); }
    const char*   NewLineStr()  const { return pDocument->NewLineStr(); }
};

class GFxTextEditorKit : public GRefCountBase<GFxTextEditorKit>
{
    friend class GFxTextDocView;
protected:
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;

    enum 
    {
        Flags_ReadOnly              = 1,
        Flags_Selectable            = 2,
        Flags_CursorBlink           = 4,
        Flags_CursorTimerBlocked    = 8
    };
    GPtr<GFxTextDocView>    pDocView;
    GFxTextHighlighter      HighlightManager;
    UPInt                   CursorPos;
    GColor                  CursorColor;
    int                     SelectPos;
    CachedValue<GRectF>     CursorRect;
    Float                   CursorTimer;
    Float                   LastAdvanceTime;
    UInt8                   Flags;

    void SetCursorBlink()     { Flags |= Flags_CursorBlink; }
    void ClearCursorBlink()   { Flags &= (~Flags_CursorBlink); }
    bool IsCursorBlink() const{ return (Flags & Flags_CursorBlink) != 0; }

    void SetCursorTimerBlocked()     { Flags |= Flags_CursorTimerBlocked; }
    void ClearCursorTimerBlocked()   { Flags &= (~Flags_CursorTimerBlocked); }
    bool IsCursorTimerBlocked() const{ return (Flags & Flags_CursorTimerBlocked) != 0; }

    bool CalcCursorRectPosInLineBuffer(UPInt charIndex, GPointF* ptopLeft, UInt* plineIndex = NULL, UInt* pglyphIndex = NULL);
    bool CalcCursorRectPosOnScreen(UPInt charIndex, GPointF* ptopLeft, UInt* plineIndex = NULL, UInt* pglyphIndex = NULL);

    // "notifyMask" represents
    // the combination of GFxTextDocView::ViewNotificationMasks
    virtual void OnDocumentChanged(UInt notifyMask);

    const GFxTextLineBuffer::GlyphEntry* GetGlyphEntryAtIndex(UPInt charIndex, UPInt* ptextPos);

    GFxTextEditorKit(GFxTextDocView* pdocview);
    ~GFxTextEditorKit();
public:
    void SetReadOnly()     { Flags |= Flags_ReadOnly; }
    void ClearReadOnly()   { Flags &= (~Flags_ReadOnly); }
    bool IsReadOnly() const{ return (Flags & Flags_ReadOnly) != 0; }

    void SetSelectable()     { Flags |= Flags_Selectable; }
    void ClearSelectable()   { Flags &= (~Flags_Selectable); }
    bool IsSelectable() const{ return (Flags & Flags_Selectable) != 0; }

    void Advance(Float timer);
    void Display(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx);
    void ResetBlink(bool state = 1, bool blocked = 0);

    bool HasCursor() const { return !IsReadOnly(); }

    void SetCursorPos(UPInt pos);
    UPInt GetCursorPos() const;
    bool ScrollToCursor();

    // event handlers
    void OnMouseDown(Float x, Float y, int buttons);
    bool OnKeyDown(int code, const GFxSpecialKeysState& specKeysState);
    bool OnChar(UInt32 wcharCode);

    GFxTextDocView* GetDocument() const { return pDocView; }
};


#endif //INC_GFXSTYLEDTEXT_H

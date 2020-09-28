/**********************************************************************

Filename    :   GFxTextCore.h
Content     :   Core text definitions
Created     :   April 29, 2008
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_TEXT_GFXTEXTCORE_H
#define INC_TEXT_GFXTEXTCORE_H

#define GFC_NO_CSS_SUPPORT

#include "GUTF8Util.h"
#include "GTLTypes.h"
#include "GRange.h"
#include "Text/GFxTextLineBuffer.h"

/*
#include "GFxStyles.h"
#include "GFxFontResource.h"
#include "GFxFontManager.h"
#include "GFxLog.h"
#include "GFxTextHighlight.h"
#include "GFxGlyphParam.h"
#include "GFxEvent.h"*/

#define MAX_FONT_NAME_LEN           50
#define GFX_ACTIVE_SEL_BKCOLOR      0xFF000000u
#define GFX_INACTIVE_SEL_BKCOLOR    0xFF808080u
#define GFX_ACTIVE_SEL_TEXTCOLOR    0xFFFFFFFFu
#define GFX_INACTIVE_SEL_TEXTCOLOR  0xFFFFFFFFu

class GFxTextParagraph;

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
    
    size_t GetHash() const;

    bool operator == (const GFxTextHTMLImageTagDesc& f) const
    {
        return Url == f.Url && Id == f.Id && VSpace == f.VSpace && 
               HSpace == f.HSpace && ParaId == f.ParaId && Alignment == f.Alignment;
    }
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
        size_t  operator()(const GFxTextFormat& data) const;
    };
protected:
    GFxString   FontList; // comma separated list of font names
    GFxString   Url;
    GPtr<GFxTextHTMLImageTagDesc>   pImageDesc;
    GPtr<GFxFontHandle>             pFontHandle; // required if font was referenced by id

    UInt32      Color; // format AA RR GG BB.
    SInt16      LetterSpacing; // in twips!
    UInt16      FontSize;      // in twips!

    enum
    {
        Format_Bold =       0x1,
        Format_Italic =     0x2,
        Format_Underline =  0x4,
        Format_Kerning =    0x8
    };
    UInt8       FormatFlags;

    enum
    {
        PresentMask_Color         = 1,
        PresentMask_LetterSpacing = 2,
        PresentMask_FontList      = 4,
        PresentMask_FontSize      = 8,
        PresentMask_Bold          = 0x10,
        PresentMask_Italic        = 0x20,
        PresentMask_Underline     = 0x40,
        PresentMask_Kerning       = 0x80,
        PresentMask_Url           = 0x100,
        PresentMask_ImageDesc     = 0x200,
        PresentMask_Alpha         = 0x400,
        PresentMask_FontHandle    = 0x800,
        PresentMask_SingleFontName= 0x1000 // indicating that the single font name is stored in FontList
    };
    UInt16       PresentMask;

    //    void InvalidateCachedFont() { pCachedFont = 0; }
public:

    GFxTextFormat(): Color(0xFF000000u), LetterSpacing(0), FontSize(0), FormatFlags(0), PresentMask(0) 
    { 
        SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
    }
    GFxTextFormat(const GFxTextFormat& srcfmt) : FontList(srcfmt.FontList),
        Url(srcfmt.Url), pImageDesc(srcfmt.pImageDesc), 
        pFontHandle(srcfmt.pFontHandle), Color(srcfmt.Color), LetterSpacing(srcfmt.LetterSpacing),
        FontSize(srcfmt.FontSize), FormatFlags(srcfmt.FormatFlags), PresentMask(srcfmt.PresentMask)
    {
        SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
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

    // This method sets whole color, including the alpha
    void   SetColor(const GColor& color) { Color = color.ToColor32(); PresentMask |= PresentMask_Color; }
    GColor GetColor() const              { return GColor(Color); }
    // This method sets only RGB values of color
    void SetColor32(UInt32 color) { Color = (color & 0xFFFFFF) | (Color & 0xFF000000u); PresentMask |= PresentMask_Color; }
    UInt32 GetColor32() const     { return Color; }
    void ClearColor()             { Color = 0xFF000000u; PresentMask &= ~PresentMask_Color; }

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

    void SetFontSizeInTwips(UInt fontSize) 
    { 
        FontSize = (fontSize <= 65536) ? (UInt16)fontSize : (UInt16)65535u; 
        PresentMask |= PresentMask_FontSize; 
    }
    UInt GetFontSizeInTwips() const { return FontSize; }
    void SetFontSize(Float fontSize) 
    { 
        FontSize = (fontSize < TwipsToPixels(65536.0)) ? (UInt16)PixelsToTwips(fontSize) : (UInt16)65535u; 
        PresentMask |= PresentMask_FontSize; 
    }
    Float GetFontSize(Float scaleFactor) const { return TwipsToPixels((Float)FontSize)*scaleFactor; }
    Float GetFontSize() const       { return TwipsToPixels((Float)FontSize); }
    void ClearFontSize()            { FontSize = 0; PresentMask &= ~PresentMask_FontSize; }

    void SetFontName(const GFxString& fontName);
    void SetFontName(const char* pfontName, UPInt  fontNameSz = GFC_MAX_UPINT);
    void SetFontName(const wchar_t* pfontName, UPInt fontNameSz = GFC_MAX_UPINT);
    void SetFontList(const GFxString& fontList);
    void SetFontList(const char* pfontList, UPInt  fontListSz = GFC_MAX_UPINT);
    void SetFontList(const wchar_t* pfontList, UPInt  fontListSz = GFC_MAX_UPINT);
    const GFxString& GetFontList() const;
    const GFxString& GetFontName() const { return GetFontList(); }
    void ClearFontList() { PresentMask &= ~(PresentMask_FontList|PresentMask_SingleFontName); }
    void ClearFontName() { ClearFontList(); }

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
    bool IsFontListSet() const  { return (PresentMask & PresentMask_FontList) != 0; }
    bool IsFontNameSet() const  { return IsFontListSet(); }
    bool IsSingleFontNameSet() const { return (PresentMask & PresentMask_SingleFontName) != 0; }
    bool IsFontHandleSet() const{ return (PresentMask & PresentMask_FontHandle) != 0; }
    bool IsImageDescSet() const { return (PresentMask & PresentMask_ImageDesc) != 0; }
    bool IsColorSet() const     { return (PresentMask & PresentMask_Color) != 0; }
    bool IsAlphaSet() const     { return (PresentMask & PresentMask_Alpha) != 0; }
    bool IsLetterSpacingSet() const { return (PresentMask & PresentMask_LetterSpacing) != 0; }
    bool IsUrlSet() const       { return (PresentMask & PresentMask_Url) != 0 && Url.GetLength() != 0; }
    bool IsUrlCleared() const   { return (PresentMask & PresentMask_Url) != 0 && Url.GetLength() == 0; }

    GFxTextFormat Merge(const GFxTextFormat& fmt) const;
    GFxTextFormat Intersection(const GFxTextFormat& fmt) const;

    bool operator == (const GFxTextFormat& f) const
    {
        return (PresentMask == f.PresentMask && FormatFlags == f.FormatFlags && Color == f.Color && 
            FontSize == f.FontSize && 
            (IsFontListSet() == f.IsFontListSet() && (!IsFontListSet() || FontList.CompareNoCase(f.FontList) == 0)) && 
            LetterSpacing == f.LetterSpacing &&
            (IsFontHandleSet() == f.IsFontHandleSet() && (!IsFontHandleSet() || pFontHandle == f.pFontHandle || 
            (pFontHandle && f.pFontHandle && *pFontHandle == *f.pFontHandle))) &&
            (IsUrlSet() == f.IsUrlSet() && (!IsUrlSet() || Url.CompareNoCase(f.Url) == 0)) && 
            ((pImageDesc && f.pImageDesc && *pImageDesc == *f.pImageDesc) || (pImageDesc == f.pImageDesc)));
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
public:
    friend class HashFunctor;
    struct HashFunctor
    {
        size_t  operator()(const GFxTextParagraphFormat& data) const;
    };

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
    enum DisplayType
    {
        Display_Inline = 0,
        Display_Block  = 1,
        Display_None   = 2
    };
protected:
    UInt*  pTabStops;   // First elem is total num of tabstops, followed by tabstops in twips
    UInt16 BlockIndent;
    SInt16 Indent;
    SInt16 Leading;
    UInt16 LeftMargin;
    UInt16 RightMargin;
    AlignType   Align;
    DisplayType Display;
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
        PresentMask_Bullet        = 0x80,
        PresentMask_Display       = 0x100
    };
    UInt16 PresentMask;
public:
    GFxTextParagraphFormat():pTabStops(NULL), BlockIndent(0), Indent(0), Leading(0), 
        LeftMargin(0), RightMargin(0), Align(Align_Left), Bullet(0), PresentMask(0) 
    {}

    GFxTextParagraphFormat(const GFxTextParagraphFormat& src) : pTabStops(NULL), 
        BlockIndent(src.BlockIndent), Indent(src.Indent), Leading(src.Leading), 
        LeftMargin(src.LeftMargin), RightMargin(src.RightMargin), 
        Align(src.Align), Display(src.Display), Bullet(src.Bullet), PresentMask(src.PresentMask)
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
    void SetTabStopsNum(UInt num)   { AllocTabStops(num); PresentMask |= PresentMask_TabStops; }
    void SetTabStopsElement(UInt idx, UInt val);
    void ClearTabStops()            { FreeTabStops(); PresentMask &= ~PresentMask_TabStops; }
    bool IsTabStopsSet() const      { return (PresentMask & PresentMask_TabStops) != 0; }

    void SetDisplay(DisplayType display) { Display = display; PresentMask |= PresentMask_Display; }
    void ClearDisplay()                  { Display = Display_Inline; PresentMask &= ~PresentMask_Display; }
    bool IsDisplaySet() const            { return (PresentMask & PresentMask_Display) != 0; }
    DisplayType GetDisplay() const       { return Display; }

    GFxTextParagraphFormat Merge(const GFxTextParagraphFormat& fmt) const;
    GFxTextParagraphFormat Intersection(const GFxTextParagraphFormat& fmt) const;

    GFxTextParagraphFormat& operator=(const GFxTextParagraphFormat& src);

    bool operator == (const GFxTextParagraphFormat& f) const
    {
        return (PresentMask == f.PresentMask && BlockIndent == f.BlockIndent && Indent == f.Indent && 
            Leading == f.Leading && LeftMargin == f.LeftMargin &&
            RightMargin == f.RightMargin && TabStopsEqual(f.pTabStops) && Align == f.Align && 
            Bullet == f.Bullet && Display == f.Display);
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
    typedef GTL::ghash_set<GFxTextFormatPtr, GFxTextFormatPtr::HashFunctor>                   TextFormatStorageType;
    typedef GTL::ghash_set<GFxTextParagraphFormatPtr, GFxTextParagraphFormatPtr::HashFunctor> ParagraphFormatStorageType;

    TextFormatStorageType           TextFormatStorage;
    ParagraphFormatStorageType      ParagraphFormatStorage;
    UInt32                          NewParagraphId;
public:
    GFxTextAllocator() : NewParagraphId(1) {}
    ~GFxTextAllocator() { GASSERT(1); }

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
    void* AllocRaw(UPInt numBytes)
    {
        return GALLOC(numBytes);
    }
    void FreeRaw(void* pstr)
    {
        GFREE(pstr);
    }

    GFxTextFormat* AllocateTextFormat(const GFxTextFormat& srcfmt)
    {
        GFxTextFormatPtr* ppfmt = TextFormatStorage.get(&srcfmt);
        if (ppfmt)
        {
            ppfmt->GetPtr()->AddRef();
            return ppfmt->GetPtr();
        }
        GFxTextFormat* pfmt = new GFxTextFormat(srcfmt);
        TextFormatStorage.set(pfmt);
        return pfmt;
    }

    GFxTextParagraphFormat* AllocateParagraphFormat(const GFxTextParagraphFormat& srcfmt)
    {
        GFxTextParagraphFormatPtr* ppfmt = ParagraphFormatStorage.get(&srcfmt);
        if (ppfmt)
        {
            ppfmt->GetPtr()->AddRef();
            return ppfmt->GetPtr();
        }
        GFxTextParagraphFormat* pfmt = new GFxTextParagraphFormat(srcfmt);
        ParagraphFormatStorage.set(pfmt);
        return pfmt;
    }

    GFxTextParagraph* AllocateParagraph();
    GFxTextParagraph* AllocateParagraph(const GFxTextParagraph& srcPara);

    UInt32  AllocateParagraphId() { return NewParagraphId++; }
    UInt32  GetNextParagraphId()  { return NewParagraphId; }
};

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

#endif //INC_TEXT_GFXTEXTCORE_H

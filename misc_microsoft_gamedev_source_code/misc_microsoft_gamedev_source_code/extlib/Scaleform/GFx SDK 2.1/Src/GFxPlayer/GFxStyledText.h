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

#define MAX_FONT_NAME_LEN           50
#define GFX_ACTIVE_SEL_BKCOLOR      0xFF000000u
#define GFX_INACTIVE_SEL_BKCOLOR    0xFF808080u
#define GFX_ACTIVE_SEL_TEXTCOLOR    0xFFFFFFFFu
#define GFX_INACTIVE_SEL_TEXTCOLOR  0xFFFFFFFFu

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
        size_t  operator()(const GFxTextFormat& data) const;
    };
protected:
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

    GFxTextFormat(): Color(0xFF000000u), LetterSpacing(0), FontSize(0), FormatFlags(0), PresentMask(0) 
    { 
        SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
        FontName[0] = 0; 
    }
    GFxTextFormat(const GFxTextFormat& srcfmt) : Url(srcfmt.Url), pImageDesc(srcfmt.pImageDesc), 
        pFontHandle(srcfmt.pFontHandle), Color(srcfmt.Color), LetterSpacing(srcfmt.LetterSpacing),
        FontSize(srcfmt.FontSize), FormatFlags(srcfmt.FormatFlags), PresentMask(srcfmt.PresentMask)
    {
        SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
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
    bool IsUrlSet() const       { return (PresentMask & PresentMask_Url) != 0 && Url.GetLength() != 0; }
    bool IsUrlCleared() const   { return (PresentMask & PresentMask_Url) != 0 && Url.GetLength() == 0; }

    GFxTextFormat Merge(const GFxTextFormat& fmt) const;
    GFxTextFormat Intersection(const GFxTextFormat& fmt) const;

    bool operator == (const GFxTextFormat& f) const
    {
        return (PresentMask == f.PresentMask && FormatFlags == f.FormatFlags && Color == f.Color && 
            FontSize == f.FontSize && 
            (IsFontNameSet() == f.IsFontNameSet() && (!IsFontNameSet() || GFxString::CompareNoCase(FontName, f.FontName) == 0)) && 
             LetterSpacing == f.LetterSpacing &&
            (IsFontHandleSet() == f.IsFontHandleSet() && (!IsFontHandleSet() || pFontHandle == f.pFontHandle)) &&
            (IsUrlSet() == f.IsUrlSet() && (!IsUrlSet() || Url.CompareNoCase(f.Url) == 0)) && 
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
    void SetTabStopsNum(UInt num)   { AllocTabStops(num); PresentMask |= PresentMask_TabStops; }
    void SetTabStopsElement(UInt idx, UInt val);
    void ClearTabStops()            { FreeTabStops(); PresentMask &= ~PresentMask_TabStops; }
    bool IsTabStopsSet() const      { return (PresentMask & PresentMask_TabStops) != 0; }

    GFxTextParagraphFormat Merge(const GFxTextParagraphFormat& fmt) const;
    GFxTextParagraphFormat Intersection(const GFxTextParagraphFormat& fmt) const;

    GFxTextParagraphFormat& operator=(const GFxTextParagraphFormat& src);

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
    typedef GTL::ghash_set<GFxTextFormatPtr, GFxTextFormatPtr::HashFunctor>                   TextFormatStorageType;
    typedef GTL::ghash_set<GFxTextParagraphFormatPtr, GFxTextParagraphFormatPtr::HashFunctor> ParagraphFormatStorageType;

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

class GFxWideStringBuffer
{
    friend class GFxTextAllocator;

    GPtr<GFxTextAllocator> pAllocator;
    wchar_t* pText;
    UPInt    Size;
    UPInt    Allocated;

public:
    GFxWideStringBuffer(): pText(0), Size(0), Allocated(0) {}
    GFxWideStringBuffer(GFxTextAllocator* pallocator):pAllocator(pallocator), pText(0), Size(0), Allocated(0)
    {
        GASSERT(pAllocator);
    }
    GFxWideStringBuffer(const GFxWideStringBuffer& o):pAllocator(o.pAllocator), pText(0), Size(0), Allocated(0)
    {
        if (o.Size > 0)
        {
            SetString(o.pText, o.Size);
        }
    }
    ~GFxWideStringBuffer();

    wchar_t* ToWStr() const
    {
        return pText;
    }
    UPInt GetSize() const { return Size; }
    UPInt GetLength() const;
    const wchar_t* GetCharPtrAt(UPInt pos) const;

    void SetString(const char* putf8Str, UPInt utf8length = GFC_MAX_UPINT);
    void SetString(const wchar_t* pstr, UPInt length = GFC_MAX_UPINT);
    wchar_t* CreatePosition(UPInt pos, UPInt length);
    void Remove(UPInt pos, UPInt length);
    void Clear() { Size = 0; } // doesn't free mem!

    SPInt StrChr(wchar_t c) { return StrChr(pText, GetLength(), c); }
    void StripTrailingNewLines();
    void StripTrailingNull();
    void AppendTrailingNull();

    static SPInt StrChr(const wchar_t* ptext, UPInt length, wchar_t c);
    static UPInt StrLen(const wchar_t* ptext);

    GFxTextAllocator* GetAllocator() const { return pAllocator; }
};

// Represents a paragraph of text. Paragraph - is the text line terminating by new line symbol or '\0'
class GFxTextParagraph : public GNewOverrideBase
{
    friend class GFxStyledText;
    friend class GFxTextCompositionString;

    typedef GRangeDataArray<GPtr<GFxTextFormat> >   TextFormatArrayType;
    typedef GRangeData<GPtr<GFxTextFormat> >        TextFormatRunType;

    GFxTextAllocator*   GetAllocator() const  { return Text.GetAllocator(); }

    void SetStartIndex(UPInt i) { StartIndex = i; }

    // returns the actual pointer on text format at the specified position.
    // Will return NULL, if there is no text format at the "pos"
    GFxTextFormat* GetTextFormatPtr(UPInt pos) const;
    void SetFormat(const GFxTextParagraphFormat* pfmt);

    void AppendTermNull(const GFxTextFormat* pdefTextFmt);
    void RemoveTermNull();
    bool HasTermNull() const;
    bool HasNewLine() const;
    void SetTermNullFormat();

    // returns true if paragraph is empty (no chars or only termination null)
    bool IsEmpty() const { return GetLength() == 0; }

    void MarkToReformat() { ++ModCounter; }
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

        inline bool IsFinished() const  { return CurTextIndex >= pText->GetSize(); }

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

        inline bool IsFinished() const { return pText == NULL || CurTextIndex >= pText->GetSize(); }

        CharacterInfo&              operator*();
        const CharacterInfo&        operator*() const
        {
            return const_cast<CharactersIterator*>(this)->operator*();
        }
        inline CharacterInfo*       operator->() { return &operator*(); }
        inline const CharacterInfo* operator->() const { return &operator*(); }

        void operator++();
        void operator++(int) { operator++(); }
        void operator+=(UPInt n);

        const wchar_t* GetRemainingTextPtr(UPInt * plen) const;
        UPInt GetCurTextIndex() const { return CurTextIndex; }
    };
    CharactersIterator GetCharactersIterator() const            { return CharactersIterator(this); }
    CharactersIterator GetCharactersIterator(UPInt index) const { return CharactersIterator(this, index); }
public:
    GFxTextParagraph(GFxTextAllocator* ptextAllocator);
    GFxTextParagraph() {  } // shouldn't be used! 
    GFxTextParagraph(const GFxTextParagraph& o);
    ~GFxTextParagraph() {}

    FormatRunIterator GetIterator() const;
    FormatRunIterator GetIteratorAt(UPInt index) const;

    UPInt GetStartIndex() const { return StartIndex; }
    // returns length, not including terminal null (if exists)
    UPInt GetLength() const;
    // returns length, including terminal null (if exists)
    UPInt GetSize() const       { return Text.GetSize(); }
    UPInt GetNextIndex() const  { return StartIndex + GetLength(); }

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
    void Clear();

    void SetText(const wchar_t* pstring, UPInt length = GFC_MAX_UPINT);
    void SetTextFormat(const GFxTextFormat& fmt, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);
    GFxTextFormat GetTextFormat(UPInt startPos, UPInt endPos = GFC_MAX_UPINT) const;

    void ClearTextFormat(UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);

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

private: // data
    GFxWideStringBuffer             Text;
    GPtr<GFxTextParagraphFormat>    pFormat;
    TextFormatArrayType             FormatInfo;
    UPInt                           StartIndex;
    UInt32                          UniqueId;
    UInt16                          ModCounter;
};

class GFxStyledText : public GRefCountBase<GFxStyledText>
{
    friend class GFxTextDocView;
    friend class GFxTextCompositionString;
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
    typedef ParagraphArrayType::ConstIterator   ParagraphsConstIterator;

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
            if (index >= si1 && index < (si1 + pp1->GetSize()))
                return 0; 
            return (int)(si1 - index);
        }
    };
    void EnsureTermNull();
public:
    GFxStyledText();
    GFxStyledText(GFxTextAllocator* pallocator);

    void Clear();

    UPInt GetLength() const;

    void SetText(const char* putf8String, UPInt stringSize = GFC_MAX_UPINT);
    void SetText(const wchar_t* pstring, UPInt length = GFC_MAX_UPINT);
    
    GFxString GetText() const;
    void GetText(GFxWideStringBuffer* pBuffer) const;
    void GetText(GFxWideStringBuffer* pBuffer,UPInt startPos, UPInt endPos) const;
    void CopyStyledText(GFxStyledText* pdest, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT) const;
    GFxStyledText* CopyStyledText(UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT) const;

    void SetHtml(const GFxString&);
    void SetHtml(const wchar_t* pstring, UPInt length = GFC_MAX_UPINT);
    GFxString GetHtml() const;

    void GetTextAndParagraphFormat(GFxTextFormat* pdestTextFmt, GFxTextParagraphFormat* pdestParaFmt, UPInt startPos, UPInt endPos = GFC_MAX_UPINT);
    bool GetTextAndParagraphFormat(const GFxTextFormat** ppdestTextFmt, const GFxTextParagraphFormat** pdestParaFmt, UPInt pos);
    void SetTextFormat(const GFxTextFormat& fmt, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);
    void SetParagraphFormat(const GFxTextParagraphFormat& fmt, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);

    // wipe out text format for the specified range. Might be useful, if need to set
    // format without merging with existing one.
    void ClearTextFormat(UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);

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

    // Insert/append funcs
    enum NewLinePolicy
    {
        NLP_CompressCRLF, // CR LF will be compressed into one EOL
        NLP_ReplaceCRLF   // each CR and/or LF will be replaced by EOL
    };
    UPInt AppendString(const char* putf8String, UPInt stringSize = GFC_MAX_UPINT, 
                       NewLinePolicy newLinePolicy = NLP_ReplaceCRLF);
    UPInt AppendString(const char* putf8String, UPInt stringSize, 
                       NewLinePolicy newLinePolicy,  
                       const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt);
    UPInt AppendString(const char* putf8String, UPInt stringSize, 
                       const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt)
    {
        return AppendString(putf8String, stringSize, NLP_ReplaceCRLF, pdefTextFmt, pdefParaFmt);
    }
    UPInt AppendString(const wchar_t* pstr, UPInt length = ~0u,
                       NewLinePolicy newLinePolicy = NLP_ReplaceCRLF);
    UPInt AppendString(const wchar_t* pstr, UPInt length,
                       NewLinePolicy newLinePolicy,
                       const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt);
    UPInt AppendString(const wchar_t* pstr, UPInt length,
                       const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt)
    {
        return AppendString(pstr, length, NLP_ReplaceCRLF, pdefTextFmt, pdefParaFmt);
    }

    UPInt InsertString(const wchar_t* pstr, UPInt pos, UPInt length = GFC_MAX_UPINT,
                       NewLinePolicy newLinePolicy = NLP_ReplaceCRLF);
    UPInt InsertString(const wchar_t* pstr, UPInt pos, UPInt length, 
                       NewLinePolicy newLinePolicy,
                       const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt);
    UPInt InsertString(const wchar_t* pstr, UPInt pos, UPInt length, 
                       const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt)
    {
        return InsertString(pstr, pos, length, NLP_ReplaceCRLF, pdefTextFmt, pdefParaFmt);
    }
    // insert styled text
    UPInt InsertStyledText(const GFxStyledText& text, UPInt pos, UPInt length = GFC_MAX_UPINT);

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
    ParagraphsIterator GetParagraphByIndex(UPInt index, UPInt* pindexInParagraph = NULL) const
    {
        return const_cast<GFxStyledText*>(this)->GetParagraphByIndex(index, pindexInParagraph);
    }
    ParagraphsIterator GetNearestParagraphByIndex(UPInt index, UPInt* pindexInParagraph = NULL);
    ParagraphsIterator GetNearestParagraphByIndex(UPInt index, UPInt* pindexInParagraph = NULL) const
    {
        return const_cast<GFxStyledText*>(this)->GetNearestParagraphByIndex(index, pindexInParagraph);
    }
    ParagraphsIterator GetParagraphIterator() { return Paragraphs.Begin(); }
    ParagraphsIterator GetParagraphIterator() const { return const_cast<GFxStyledText*>(this)->Paragraphs.Begin(); }

    GFxTextParagraph* AppendNewParagraph(const GFxTextParagraphFormat* pdefParaFmt = NULL);
    GFxTextParagraph* InsertNewParagraph(GFxStyledText::ParagraphsIterator& iter, 
                                         const GFxTextParagraphFormat* pdefParaFmt = NULL);
    GFxTextParagraph* AppendCopyOfParagraph(const GFxTextParagraph& srcPara);
    GFxTextParagraph* InsertCopyOfParagraph(GFxStyledText::ParagraphsIterator& iter, 
                                            const GFxTextParagraph& srcPara);

    GFxTextParagraph*       GetLastParagraph();
    const GFxTextParagraph* GetLastParagraph() const;

    template <class Char>
    bool ParseHtml(const Char* phtml, UPInt  htmlSize, HTMLImageTagInfoArray* pimgInfoArr = NULL, 
                   bool multiline = true, bool condenseWhite = false);

    // atomic callbacks. These methods might be invoked multiple times 
    // from inside of the InsertString or Remove methods. Client SHOULD NOT do
    // anything "heavy" in these handlers, like re-formatting (though, the flag 
    // "re-formatting is neeeded" might be set).
    //virtual void OnParagraphTextFormatChanging(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const GFxTextFormat& formatToBeSet);
    //virtual void OnParagraphTextFormatChanged(const GFxTextParagraph& para, UPInt startPos, UPInt endPos);
    //virtual void OnParagraphFormatChanging(const GFxTextParagraph& para, const GFxTextParagraphFormat& formatToBeSet);
    //virtual void OnParagraphFormatChanged(const GFxTextParagraph& para);
    //virtual void OnParagraphTextInserting(const GFxTextParagraph& para, UPInt insertionPos, UPInt insertingLen);
    //virtual void OnParagraphTextInserted(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const wchar_t* ptextInserted);
    //virtual void OnParagraphTextRemoving(const GFxTextParagraph& para, UPInt removingPos, UPInt removingLen);
    //virtual void OnParagraphTextRemoved(const GFxTextParagraph& para, UPInt removedPos, UPInt removedLen);
    virtual void OnTextInserting(UPInt startPos, UPInt length, const wchar_t* ptxt);
    virtual void OnTextInserting(UPInt startPos, UPInt length, const char* ptxt)
        { GUNUSED3(startPos, length, ptxt); };
    virtual void OnTextRemoving(UPInt startPos, UPInt length);
    virtual void OnTextInserted(UPInt startPos, UPInt length, const wchar_t* ptxt) 
        { GUNUSED3(startPos, length, ptxt); };
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
class GFxLineCursor;

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
    friend class GFxTextCompositionString;
    friend class GFxLineCursor;

    typedef GRenderer::BitmapDesc   BitmapDesc;
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;
    typedef GTL::ghash_identity<const GFxTextFormat*, GPtr<GFxFontHandle> > FontCache;
public:
    // editor's commands
    enum CommandType
    {
        Cmd_InsertChar,
        Cmd_InsertPlainText,
        Cmd_InsertStyledText,
        Cmd_DeleteChar,
        Cmd_DeleteText,
        Cmd_ReplaceTextByChar,
        Cmd_ReplaceTextByPlainText,
        Cmd_ReplaceTextByStyledText,
        Cmd_BackspaceChar
    };
    struct InsertCharCommand
    {
        UPInt   PosAt;
        wchar_t CharCode;

        InsertCharCommand(UPInt pos, wchar_t c):PosAt(pos), CharCode(c) {}
    };
    struct InsertPlainTextCommand
    {
        UPInt           PosAt;
        const wchar_t*  pStr;
        UPInt           Length;

        InsertPlainTextCommand(UPInt pos, const wchar_t* pstr, UPInt len):PosAt(pos), pStr(pstr), Length(len) {}
    };
    struct InsertStyledTextCommand
    {
        UPInt                   PosAt;
        const GFxStyledText*    pText;

        InsertStyledTextCommand(UPInt pos, const GFxStyledText* pstr):PosAt(pos), pText(pstr) {}
    };
    struct DeleteCharCommand
    {
        UPInt   PosAt;

        DeleteCharCommand(UPInt pos):PosAt(pos) {}
    };
    struct DeleteTextCommand
    {
        UPInt   BeginPos;
        UPInt   EndPos;

        DeleteTextCommand(UPInt startPos, UPInt endPos):
            BeginPos(startPos), EndPos(endPos) {}
    };
    struct ReplaceTextByCharCommand
    {
        UPInt   BeginPos;
        UPInt   EndPos;
        wchar_t CharCode;

        ReplaceTextByCharCommand(UPInt startPos, UPInt endPos, wchar_t c):
            BeginPos(startPos), EndPos(endPos), CharCode(c) {}
    };
    struct ReplaceTextByPlainTextCommand
    {
        UPInt           BeginPos;
        UPInt           EndPos;
        const wchar_t*  pStr;
        UPInt           Length;

        ReplaceTextByPlainTextCommand(UPInt startPos, UPInt endPos,  const wchar_t* pstr, UPInt len):
        BeginPos(startPos), EndPos(endPos), pStr(pstr), Length(len) {}
    };
    struct ReplaceTextByStyledTextCommand
    {
        UPInt                   BeginPos;
        UPInt                   EndPos;
        const GFxStyledText*    pText;

        ReplaceTextByStyledTextCommand(UPInt startPos, UPInt endPos, const GFxStyledText* pstr):
        BeginPos(startPos), EndPos(endPos), pText(pstr) {}
    };

    // structure used for DocumentListener::Format_OnLineFormat. All widths are in twips.
    struct LineFormatDesc
    {
        const wchar_t*  pParaText;              // [in] paragraph text
        Float*          pCharWidths;            // [in] array of char widths, size = NumCharsInLine
        UPInt           LineStartPos;           // [in] text position in paragraph of first char in line
        UPInt           NumCharsInLine;         // [in] count of chars currently in the line        
        Float           VisibleRectWidth;       // [in] width of client rect
        Float           CurrentLineWidth;       // [in] current line width
        Float           LineWidthBeforeWordWrap;// [in] line width till ProposedWordWrapPoint
        Float           DashSymbolWidth;        // [in] may be used to calculate hyphenation

        UPInt           ProposedWordWrapPoint;  // [in,out] text index of proposed word wrap pos,
                                                //          callback may change it to move wordwrap point
        bool            UseHyphenation;         // [out]    callback may set it to indicate to use hyphenation
    };

    // document listener to pass events outside of the docview
    class DocumentListener : public GRefCountBase<DocumentListener>
    {
    protected:
        enum
        {
            Mask_OnLineFormat       = 0x01,
            Mask_OnScroll           = 0x02,
            Mask_OnMaxScrollChanged = 0x04,
            Mask_OnViewChanged      = 0x08,

            Mask_All = (Mask_OnLineFormat | Mask_OnScroll | Mask_OnMaxScrollChanged | Mask_OnViewChanged)
        };
        UInt8           HandlersMask;           
    public:
        DocumentListener(UInt8 handlersMask = 0):HandlersMask(handlersMask) {}
        bool DoesHandleLineFormat() const  { return (HandlersMask & Mask_OnLineFormat) != 0; }
        bool DoesHandleOnScroll() const    { return (HandlersMask & Mask_OnScroll) != 0; }
        bool DoesHandleOnMaxScrollChanged() const { return (HandlersMask & Mask_OnMaxScrollChanged) != 0; }
        bool DoesHandleOnViewChanged() const { return (HandlersMask & Mask_OnViewChanged) != 0; }

        // fired if Mask_OnLineFormat bit is set and line is formatted and 
        // ready to be committed. Might be used to implement custom word wrapping
        // or hyphenation.
        virtual bool    View_OnLineFormat(GFxTextDocView&, LineFormatDesc&) { return false; }

        // view-related events
        virtual void    View_OnHScroll(GFxTextDocView& , UInt newScroll) { GUNUSED(newScroll); }
        virtual void    View_OnVScroll(GFxTextDocView& , UInt newScroll) { GUNUSED(newScroll); }
        virtual void    View_OnMaxScrollChanged(GFxTextDocView&) {  }
        // fired, if view needs to be repainted, for example if formatting is changed,
        // selection truned on/off, etc. Fired only if Mask_OnViewChanged bit is set.
        virtual void    View_OnChanged(GFxTextDocView&)                  {}

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
        Align_Center = 2,
    };
    enum ViewVAlignment
    {
        VAlign_None   = 0,
        VAlign_Top    = 1,
        VAlign_Bottom = 2,
        VAlign_Center = 3,
    };
    enum ViewTextAutoSize
    {
        TAS_None   = 0,
        TAS_Shrink = 1,
        TAS_Fit    = 2
    };
    enum 
    {
        Align_Mask   = 0x3,
        Align_Shift  = 0,

        VAlign_Mask   = 0x3,
        VAlign_Shift  = 2,

        TAS_Mask  = 0x3,
        TAS_Shift = 4
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
    GFxTextAllocator* GetAllocator() { return pDocument->GetAllocator(); }

    void ClearReformatReq()   { RTFlags &= (~(RTFlags_ReformatReq | RTFlags_CompleteReformatReq)); }
    bool IsReformatReq() const{ return (RTFlags & (RTFlags_ReformatReq | RTFlags_CompleteReformatReq)) != 0; }

    void ClearCompleteReformatReq()   { RTFlags &= (~RTFlags_CompleteReformatReq); }
    bool IsCompleteReformatReq() const{ return (RTFlags & RTFlags_CompleteReformatReq) != 0; }

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
        bool                        ForceVerticalCenterAutoSize;    // force vertical centered autoSize

        FormatParagraphInfo()  { pDynLine = NULL; }
        ~FormatParagraphInfo() { LineBuffer.LineAllocator.FreeLine(pDynLine); }
    };

    GFxFontHandle* FindFont(FindFontInfo* pfontInfo);
    void FormatParagraph(const GFxTextParagraph& paragraph, FormatParagraphInfo* pinfo);

    inline Float GetActualFontSize(const FindFontInfo& findFontInfo, FormatParagraphInfo* pinfo);
    void FinalizeLine(GFxTextLineBuffer::Line* pcurLine, 
        FormatParagraphInfo* pinfo, const GFxTextParagraphFormat& paraFormat, 
        GFxLineCursor& lineCursor);

    UPInt GetCursorPosInLineByOffset(UInt lineIndex, Float relativeOffsetX);
    void  SetDefaultTextAndParaFormat(UPInt cursorPos);

    GFxStyledText::NewLinePolicy GetNewLinePolicy() const
    {
        return (DoesCompressCRLF()) ? GFxStyledText::NLP_CompressCRLF: GFxStyledText::NLP_ReplaceCRLF;
    }

    enum UseType { UseInternally, UseExternally };
    // if ut == UseInternally it will not try to reformat the text
    void SetViewRect(const GRectF& rect, UseType ut);

    GFxTextHighlightDesc* GetSelectionHighlighterDesc();
    GFxTextHighlightDesc* GetSelectionHighlighterDesc() const;

    // convert glyph pos (taken from LineBuffer to-from text pos (as in StyledText)
    UPInt TextPos2GlyphOffset(UPInt textPos);
    UPInt TextPos2GlyphPos(UPInt textPos);
    UPInt GlyphPos2TextPos(UPInt glyphPos);

    // Return current values of maxHScroll/maxVScroll values without
    // recalculations of them.
    UInt  GetMaxHScrollValue() const;
    UInt  GetMaxVScrollValue() const;
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
    void ClearAndSetTextFormat(const GFxTextFormat& fmt, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT)
    {
        pDocument->ClearTextFormat(startPos, endPos);
        pDocument->SetTextFormat(fmt, startPos, endPos);
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

    UPInt InsertText(const wchar_t* pstr, UPInt startPos, UPInt strLen = GFC_MAX_UPINT);
    UPInt ReplaceText(const wchar_t* pstr, UPInt startPos, UPInt endPos, UPInt strLen = GFC_MAX_UPINT);
    void  RemoveText(UPInt startPos, UPInt endPos);

    // edit
    // returns number of inserted or deleted chars
    UPInt EditCommand(CommandType cmdId, const void* command);

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

    void PreDisplay(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx, bool focused);
    void Display(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx, bool nextFrame, GFxLog* plog);
    void PostDisplay(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx, bool focused);

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
    bool IsWordWrap() const { return (Flags & Flags_WordWrap) != 0 /*&& !IsAutoSizeX()*/; }

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

    void SetViewRect(const GRectF& rect) { SetViewRect(rect, UseExternally); }
    const GRectF& GetViewRect();

    // rectangle used for painting text (ViewRect - GUTTER)
    const GRectF& GetTextRect() const { return LineBuffer.Geom.VisibleRect; }

    bool SetHScrollOffset(UInt hscroll);    
    bool SetVScrollOffset(UInt vscroll);    

    UInt GetHScrollOffset() const { return LineBuffer.GetHScrollOffset(); }    
    UInt GetVScrollOffset() const { return LineBuffer.GetFirstVisibleLineIndex(); }    
    UInt GetVScrollOffsetInTwips() const { return LineBuffer.GetVScrollOffsetInTwips(); }    

    Float GetTextWidth();
    Float GetTextHeight();
    UInt  GetLinesCount();

    UInt  GetMaxHScroll();
    UInt  GetMaxVScroll();

    void  SetMaxLength(UPInt maxLen) { MaxLength = UInt(maxLen); }
    UPInt GetMaxLength() const       { return MaxLength; }
    bool  HasMaxLength() const       { return MaxLength != 0; }

    // Returns true, if reformat actually occurred
    bool ForceReformat();

    void SetAlignment(ViewAlignment alignment) 
    { 
        AlignProps = (UByte)((AlignProps & ~(Align_Mask << Align_Shift)) | ((alignment & Align_Mask) << Align_Shift));
        SetReformatReq();
    }
    ViewAlignment GetAlignment() const { return ViewAlignment((AlignProps >> Align_Shift) & Align_Mask); }

    void SetVAlignment(ViewVAlignment valignment)
    { 
        AlignProps = (UByte)((AlignProps & ~(VAlign_Mask << VAlign_Shift)) | ((valignment & VAlign_Mask) << VAlign_Shift));
        SetReformatReq();
    }
    ViewVAlignment GetVAlignment() const { return ViewVAlignment((AlignProps >> VAlign_Shift) & VAlign_Mask); }
    
    void SetTextAutoSize(ViewTextAutoSize valignment)
    { 
        AlignProps = (UByte)((AlignProps & ~(TAS_Mask << TAS_Shift)) | ((valignment & TAS_Mask) << TAS_Shift));
        SetReformatReq();
    }
    ViewTextAutoSize GetTextAutoSize() const { return ViewTextAutoSize((AlignProps >> TAS_Shift) & TAS_Mask); }

    bool IsUrlAtPoint(Float x, Float y);

    void  SetFontScaleFactor(Float f); 
    Float GetFontScaleFactor() const { return TwipsToPixels((Float)FontScaleFactor); }
    bool  HasFontScaleFactor() const { return (RTFlags & RTFlags_HasFontScaleFactor) != 0; }

    // Returns the zero-based index value of the character at the point specified by the x  and y parameters.
    UPInt GetCharIndexAtPoint(Float x, Float y);
    // Returns the zero-based index value of the cursor at the point specified by the x  and y parameters.
    UPInt GetCursorPosAtPoint(Float x, Float y);
    // Returns the zero-based index value of the cursor at the x-offset of line at lineIndex
    UPInt GetCursorPosInLine(UInt lineIndex, Float x);
    // Given a character index, returns the index of the first character in the same paragraph. 
    UPInt GetFirstCharInParagraph(UPInt indexOfChar);
    // Returns the zero-based index value of the line at the point specified by the x  and y parameters.
    UInt GetLineIndexAtPoint(Float x, Float y);
    // Returns the zero-based index value of the line containing the character specified by the indexOfChar parameter.
    UInt GetLineIndexOfChar(UPInt indexOfChar);
    // Returns the character index of the first character in the line that the lineIndex parameter specifies.
    UPInt GetLineOffset(UInt lineIndex);
    // Returns the number of characters in a specific text line.
    UPInt GetLineLength(UInt lineIndex, bool* phasNewLine = NULL);
    // Returns the text of the line specified by the lineIndex parameter.
    const wchar_t* GetLineText(UInt lineIndex, UPInt* plen);
    
    struct LineMetrics
    {   // everything is in twips
        UInt Width, Height;
        UInt Ascent, Descent;
        SInt FirstCharXOff;
        SInt Leading;
    };
    // Returns metrics information about a given text line.
    bool GetLineMetrics(UInt lineIndex, LineMetrics* pmetrics);
    
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
    GFxTextEditorKit* GetEditorKit()  { return pEditorKit; };
    bool HasEditorKit() const { return (pEditorKit.GetPtr() != NULL); }

    // new line related
    unsigned char NewLineChar() const { return pDocument->NewLineChar(); }
    const char*   NewLineStr()  const { return pDocument->NewLineStr(); }
    void SetCompressCRLF()     { RTFlags |= RTFlags_CompressCRLF; }
    void ClearCompressCRLF()   { RTFlags &= (~RTFlags_CompressCRLF); }
    bool DoesCompressCRLF() const { return (RTFlags & RTFlags_CompressCRLF) != 0; }

    // highlight
    bool AddHighlight(GFxTextHighlightDesc* pdesc);
    bool RemoveHighlight(int id);
    GFxTextHighlighter* GetHighlighterManager() const { return (pHighlight) ? &pHighlight->HighlightManager : NULL; } //?
    void UpdateHighlight(const GFxTextHighlightDesc& desc);

protected:
    GFxTextHighlighter* CreateHighlighterManager();

    // primitive selection support, should be used by editor kit
    void SetSelection(UPInt startPos, UPInt endPos);
    void ClearSelection() { SetSelection(0, 0); }
    //void GetSelection(UPInt* pstartPos, UPInt* pendPos);
    UPInt GetBeginSelection() const      { return GTL::gpmin(BeginSelection, EndSelection); }
    UPInt GetEndSelection()   const      { return GTL::gpmax(BeginSelection, EndSelection); }
    void SetBeginSelection(UPInt begSel) { SetSelection(begSel, EndSelection); }
    void SetEndSelection(UPInt endSel)   { SetSelection(BeginSelection, endSel); }
    void SetSelectionTextColor(UInt32 color);
    void SetSelectionBackgroundColor(UInt32 color);
    UInt32 GetSelectionTextColor() const;
    UInt32 GetSelectionBackgroundColor() const;

protected:
    // Document's part
    class DocumentText : public GFxStyledText
    {
        GFxTextDocView* pDocument;
    public:
        DocumentText(GFxTextDocView* pdoc) : pDocument(pdoc) {}
        virtual void OnParagraphRemoving(const GFxTextParagraph& para);
        virtual void OnTextInserting(UPInt startPos, UPInt length, const wchar_t* ptxt);
        virtual void OnTextInserting(UPInt startPos, UPInt length, const char* ptxt);
        virtual void OnTextRemoving(UPInt startPos, UPInt length);
        //virtual void OnParagraphTextInserted(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const wchar_t* ptextInserted);
        //virtual void OnParagraphTextRemoved(const GFxTextParagraph& para, UPInt removedPos, UPInt removedLen);
    };
    GPtr<DocumentText>              pDocument;
    GPtr<GFxFontManager>            pFontManager;
    GPtr<DocumentListener>          pDocumentListener;

    struct HighlightDesc : public GNewOverrideBase
    {
        GFxTextHighlighter          HighlightManager;
        GFxDrawingContext           BackgroundDrawing;
        Float                       HScrollOffset;
        Float                       VScrollOffset;
        UInt16                      FormatCounter; 

        HighlightDesc() : HScrollOffset(-1), VScrollOffset(-1), 
            FormatCounter(0) {}

        void DrawBackground(
            GFxTextDocView& document,  
            GFxDisplayContext &context, const Matrix& mat, const Cxform& cx);
        void Invalidate()       { HighlightManager.Invalidate(); }
        bool IsValid() const    { return HighlightManager.IsValid(); }
    } *pHighlight;

    // View's part
    ImageSubstitutor*               pImageSubstitutor;
    UPInt                           BeginSelection, EndSelection;
    GFxTextLineBuffer               LineBuffer;
    GRectF                          ViewRect; // total rectangle occupied by the view
    UInt                            TextWidth;  // in twips
    UInt                            TextHeight; // in twips
    UInt                            MaxLength;
    CachedPrimValue<UInt>           MaxVScroll; 

    GPtr<GFxTextEditorKit>          pEditorKit;
    UInt16                          FormatCounter; // being incremented each Format call
    UInt16                          FontScaleFactor; // in twips, 0 .. 1000.0
    UInt8                           AlignProps; // combined H- and V- alignments and TAS_<>

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
        RTFlags_HasFontScaleFactor  = 0x4,
        RTFlags_CompressCRLF        = 0x8
    };
    UInt8                           RTFlags; // run-time flags

    GFxTextFilter                   Filter;
};

struct GFxTextIMEStyle : public GNewOverrideBase
{
    enum Category
    {
        SC_CompositionSegment   = 0,
        SC_ClauseSegment        = 1,
        SC_ConvertedSegment     = 2,
        SC_PhraseLengthAdj      = 3,
        SC_LowConfSegment       = 4,

        SC_MaxNum
    };
    GFxTextHighlightInfo HighlightStyles[SC_MaxNum];
    UInt8                PresenceMask;

    GFxTextIMEStyle() : PresenceMask(0) {}

    const GFxTextHighlightInfo& GetElement(UPInt n) const
    {
        return HighlightStyles[n];
    }
    bool HasElement(UPInt n) const 
    { 
        GASSERT(n < sizeof(PresenceMask)*8); 
        return (PresenceMask & (1 << n)) != 0; 
    }

    void SetElement(UPInt n, const GFxTextHighlightInfo& hinfo) 
    { 
        GASSERT(n < sizeof(PresenceMask)*8); 
        PresenceMask |= (1 << n);
        HighlightStyles[n] = hinfo;
    }
    void Unite(const GFxTextIMEStyle& st)
    {
        for (UInt i = 0; i < SC_MaxNum; ++i)
        {
            if (st.HasElement(i))
                SetElement(i, st.GetElement(i));
        }
    }
};

#ifndef GFC_NO_IME_SUPPORT
class GFxTextCompositionString : public GRefCountBase<GFxTextCompositionString>
{
    friend class GFxTextEditorKit;

    GPtr<GFxTextFormat> pDefaultFormat;
    GFxTextEditorKit*   pEditorKit;
    GFxTextParagraph    String;
    UPInt               CursorPos;
    bool                HasHighlightingFlag;
    GFxTextIMEStyle     Styles;

    GFxTextParagraph*   GetSourceParagraph();
    void                Reformat();
public:
    GFxTextCompositionString(GFxTextEditorKit* peditorKit);
    ~GFxTextCompositionString();

    GFxTextAllocator*   GetAllocator() const  { return String.GetAllocator(); }

    void SetText(const wchar_t*, UPInt nchars = GFC_MAX_UPINT);
    void SetText(const char*, UPInt nchars = GFC_MAX_UPINT);
    const wchar_t* GetText() const { return String.GetText(); }

    void SetPosition(UPInt pos);
    UPInt GetPosition() const { return String.GetStartIndex(); }

    UPInt GetLength() const { return String.GetLength(); }

    GFxTextParagraph& GetParagraph() { return String; }
    GFxTextFormat* GetTextFormat(UPInt pos) { return String.GetTextFormatPtr(pos); }
    GFxTextFormat* GetDefaultTextFormat() const { return pDefaultFormat; }

    void  SetCursorPosition(UPInt pos);
    UPInt GetCursorPosition() const { return CursorPos; }

    void  SetStyle(GFxTextIMEStyle::Category styleCategory);
    void  HighlightText(UPInt pos, UPInt len, GFxTextIMEStyle::Category styleCategory);

    void  UseStyles(const GFxTextIMEStyle&);

    void  SetNoHighlighting(bool v = true);
    bool  HasHighlighting() const { return HasHighlightingFlag; }

    static GFxTextIMEStyle GetDefaultStyles();
};
#endif //#ifndef GFC_NO_IME_SUPPORT

class GFxTextEditorKit : public GRefCountBase<GFxTextEditorKit>
{
    friend class GFxTextDocView;
    friend class GFxTextCompositionString;
protected:
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;

    enum 
    {
        Flags_ReadOnly              = 1,
        Flags_Selectable            = 2,
        Flags_UseRichClipboard      = 4,
        Flags_CursorBlink           = 8,
        Flags_CursorTimerBlocked    = 0x10,
        Flags_MouseCaptured         = 0x20,
        Flags_ShiftPressed          = 0x40,
        Flags_OverwriteMode         = 0x80,
        Flags_WideCursor            = 0x100
    };
    GPtr<GFxTextDocView>    pDocView;
    GPtr<GFxTextClipboard>  pClipboard;
    GPtr<GFxTextKeyMap>     pKeyMap;
#ifndef GFC_NO_IME_SUPPORT
    GPtr<GFxTextCompositionString> pComposStr;
#endif //#ifndef GFC_NO_IME_SUPPORT
    UPInt                   CursorPos;
    GColor                  CursorColor;
    int                     SelectPos;
    CachedValue<GRectF>     CursorRect;
    Float                   CursorTimer;
    Float                   LastAdvanceTime;
    Float                   LastHorizCursorPos;
    GPointF                 LastMousePos;
    UInt32                  ActiveSelectionBkColor;
    UInt32                  ActiveSelectionTextColor;
    UInt32                  InactiveSelectionBkColor;
    UInt32                  InactiveSelectionTextColor;
    UInt16                  Flags;

    void SetCursorBlink()     { Flags |= Flags_CursorBlink; }
    void ClearCursorBlink()   { Flags &= (~Flags_CursorBlink); }
    bool IsCursorBlink() const{ return (Flags & Flags_CursorBlink) != 0; }

    void SetCursorTimerBlocked()     { Flags |= Flags_CursorTimerBlocked; }
    void ClearCursorTimerBlocked()   { Flags &= (~Flags_CursorTimerBlocked); }
    bool IsCursorTimerBlocked() const{ return (Flags & Flags_CursorTimerBlocked) != 0; }

    void SetMouseCaptured()     { Flags |= Flags_MouseCaptured; }
    void ClearMouseCaptured()   { Flags &= (~Flags_MouseCaptured); }

    void SetShiftPressed()     { Flags |= Flags_ShiftPressed; }
    void ClearShiftPressed()   { Flags &= (~Flags_ShiftPressed); }
    bool IsShiftPressed() const{ return (Flags & Flags_ShiftPressed) != 0; }

    // "notifyMask" represents
    // the combination of GFxTextDocView::ViewNotificationMasks
    virtual void OnDocumentChanged(UInt notifyMask);

    const GFxTextLineBuffer::GlyphEntry* GetGlyphEntryAtIndex(UPInt charIndex, UPInt* ptextPos);

    // convert glyph pos (taken from LineBuffer to-from text pos (as in StyledText)
    UPInt TextPos2GlyphOffset(UPInt textPos);
    UPInt TextPos2GlyphPos(UPInt textPos);
    UPInt GlyphPos2TextPos(UPInt glyphPos);

    GFxTextEditorKit(GFxTextDocView* pdocview);
    ~GFxTextEditorKit();
public:
    void SetReadOnly()     { Flags |= Flags_ReadOnly; }
    void ClearReadOnly()   { Flags &= (~Flags_ReadOnly); }
    bool IsReadOnly() const{ return (Flags & Flags_ReadOnly) != 0; }

    void SetSelectable()     { Flags |= Flags_Selectable; }
    void ClearSelectable()   { Flags &= (~Flags_Selectable); }
    bool IsSelectable() const{ return (Flags & Flags_Selectable) != 0; }

    void SetUseRichClipboard()     { Flags |= Flags_UseRichClipboard; }
    void ClearUseRichClipboard()   { Flags &= (~Flags_UseRichClipboard); }
    bool DoesUseRichClipboard() const{ return (Flags & Flags_UseRichClipboard) != 0; }

    void SetOverwriteMode()     { Flags |= Flags_OverwriteMode; }
    void ClearOverwriteMode()   { Flags &= (~Flags_OverwriteMode); }
    bool IsOverwriteMode() const{ return (Flags & Flags_OverwriteMode) != 0; }

    void SetWideCursor()     { Flags |= Flags_WideCursor; }
    void ClearWideCursor();
    bool IsWideCursor() const{ return (Flags & Flags_WideCursor) != 0; }

    bool IsMouseCaptured() const{ return (Flags & Flags_MouseCaptured) != 0; }

    void Advance(Float timer);
    void PreDisplay(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx);
    void Display(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx);
    void PostDisplay(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx) { GUNUSED3(context, mat, cx); }
    void ResetBlink(bool state = 1, bool blocked = 0);

    bool HasCursor() const { return !IsReadOnly(); }

    void SetCursorPos(UPInt pos, bool selectionAllowed);
    void SetCursorPos(UPInt pos) { SetCursorPos(pos, IsSelectable()); }
    UPInt GetCursorPos() const;
    // scrolls the text to the position. If wideCursor is true, it will scroll
    // to make whole wide cursor visible.
    bool ScrollToPosition(UPInt pos, bool avoidComposStr, bool wideCursor = false);
    bool ScrollToCursor()
    {
        return ScrollToPosition(CursorPos, true, IsWideCursor());
    }

    // event handlers
    void OnMouseDown(Float x, Float y, int buttons);
    void OnMouseUp(Float x, Float y, int buttons);
    void OnMouseMove(Float x, Float y);
    bool OnKeyDown(int code, const GFxSpecialKeysState& specKeysState);
    bool OnKeyUp(int code, const GFxSpecialKeysState& specKeysState);
    bool OnChar(UInt32 wcharCode);
    void OnSetFocus();
    void OnKillFocus();

    GFxTextDocView* GetDocument() const { return pDocView; }
    void SetClipboard(GFxTextClipboard* pclipbrd) { pClipboard = pclipbrd; }
    void SetKeyMap(GFxTextKeyMap* pkeymap)        { pKeyMap = pkeymap; }

    bool CalcCursorRectInLineBuffer
        (UPInt charIndex, GRectF* pcursorRect, UInt* plineIndex = NULL, UInt* pglyphIndex = NULL, bool avoidComposStr = true);
    bool CalcAbsCursorRectInLineBuffer(UPInt charIndex, GRectF* pcursorRect, UInt* plineIndex = NULL, UInt* pglyphIndex = NULL)
    {
        return CalcCursorRectInLineBuffer(charIndex, pcursorRect, plineIndex, pglyphIndex, false);
    }
    bool CalcCursorRectOnScreen
        (UPInt charIndex, GRectF* pcursorRect, UInt* plineIndex = NULL, UInt* pglyphIndex = NULL, bool avoidComposStr = true);
    bool CalcAbsCursorRectOnScreen(UPInt charIndex, GRectF* pcursorRect, UInt* plineIndex = NULL, UInt* pglyphIndex = NULL)
    {
        return CalcCursorRectOnScreen(charIndex, pcursorRect, plineIndex, pglyphIndex, false);
    }

    // Selection, public API
    void SetSelection(UPInt startPos, UPInt endPos);
    void ClearSelection()                                  { pDocView->ClearSelection(); }
    UPInt GetBeginSelection() const                        { return pDocView->GetBeginSelection(); }
    UPInt GetEndSelection()   const                        { return pDocView->GetEndSelection(); }
    void SetBeginSelection(UPInt begSel)                   { SetSelection(begSel, GetEndSelection()); }
    void SetEndSelection(UPInt endSel)                     { SetSelection(GetBeginSelection(), endSel); }
    void SetActiveSelectionTextColor(UInt32 color)         { ActiveSelectionTextColor = color; }
    void SetActiveSelectionBackgroundColor(UInt32 color)   { ActiveSelectionBkColor   = color; }
    void SetInactiveSelectionTextColor(UInt32 color)       { InactiveSelectionTextColor = color; }
    void SetInactiveSelectionBackgroundColor(UInt32 color) { InactiveSelectionBkColor   = color; }
    UInt32 GetActiveSelectionTextColor() const             { return ActiveSelectionTextColor; }
    UInt32 GetActiveSelectionBackgroundColor() const       { return ActiveSelectionBkColor; }
    UInt32 GetInactiveSelectionTextColor() const           { return InactiveSelectionTextColor; }
    UInt32 GetInactiveSelectionBackgroundColor() const     { return InactiveSelectionBkColor; }

    // clipboard operations
    void CopyToClipboard(UPInt startPos, UPInt endPos, bool useRichClipboard);
    void CutToClipboard(UPInt startPos, UPInt endPos, bool useRichClipboard);
    // if successful, returns new projected cursor position; GFC_MAX_UPINT otherwise.
    UPInt PasteFromClipboard(UPInt startPos, UPInt endPos, bool useRichClipboard);

#ifndef GFC_NO_IME_SUPPORT
    // Composition string
    GFxTextCompositionString* CreateCompositionString();
    void ReleaseCompositionString();
    bool HasCompositionString() const { return (pComposStr && pComposStr->GetLength() > 0); }
    GFxTextCompositionString* GetCompositionString() { return pComposStr; }
#else
    bool HasCompositionString() const { return false; }
    GFxTextCompositionString* GetCompositionString() { return NULL; }
#endif //#ifndef GFC_NO_IME_SUPPORT
};


#endif //INC_GFXSTYLEDTEXT_H

/**********************************************************************

Filename    :   GFxTextLineBuffer.h
Content     :   Text line buffer
Created     :   May 31, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXTEXTLINEBUFFER_H
#define INC_GFXTEXTLINEBUFFER_H

#include "GUTF8Util.h"
#include "GTLTypes.h"
#include "GRange.h"
#include "GRenderer.h"
#include "GFxStyles.h"
#include "GFxFontResource.h"
#include "GFxLog.h"
#include "GFxDisplayContext.h"
#include "GFxFontManager.h"
#include "GFxShape.h"
#include "GFxCharacter.h"
#include "GFxDrawingContext.h"
#include "GFxGlyphParam.h"
#include "GFxTextHighlight.h"

struct GFxBatchPackage;
class  GFxFontCacheManagerImpl;

#define GFX_TEXT_GUTTER         40.0f

struct GFxTextImageDesc : public GRefCountBase<GFxTextImageDesc>
{
    typedef GRenderer::Matrix   MatrixType;

    GPtr<GFxShapeWithStylesDef> pImageShape;
    GPtr<GFxCharacter>          pSpriteShape;
    SInt                        BaseLineX, BaseLineY;
    UInt                        ScreenWidth, ScreenHeight;
    MatrixType                  Matrix;                      

    GFxTextImageDesc():BaseLineX(0), BaseLineY(0), ScreenWidth(0), ScreenHeight(0) {}

    // in twips
    Float GetScreenWidth()  const { return (Float)ScreenWidth; }
    Float GetScreenHeight() const { return (Float)ScreenHeight; }

    Float GetBaseLineX() const { return (Float)BaseLineX; }
    Float GetBaseLineY() const { return (Float)BaseLineY; }
};

struct GFxLineBufferGeometry
{
    typedef GRenderer::Matrix Matrix;

    GFxLineBufferGeometry() : 
        FirstVisibleLinePos(0), 
        VisibleRect(0,0,0,0),
        HScrollOffset(0),
        SourceTextMatrix(0,0,0,0,0,0),
        ViewportMatrix(0,0,0,0,0,0),
        HeightRatio(-1), 
        Translation(0,0),
        BlurX(0),
        BlurY(0),
        SteadyCount(0),
        Flags(0)
    {}

    UInt     FirstVisibleLinePos;
    GRectF   VisibleRect;
    UInt     HScrollOffset;
    Matrix   SourceTextMatrix;  // Original source text matrix (from SWF)
    Matrix   ViewportMatrix;    // World->Screen viewport matrix
    Float    HeightRatio;       // Text height scale deduced from SourceTextMatrix
    GPointF  Translation;
    Float    BlurX;
    Float    BlurY;
    UInt     SteadyCount;
    enum
    {
        Flag_InvalidCache       = 0x01,
        Flag_BatchHasUnderline  = 0x02,
        Flag_StaticText         = 0x04,
        Flag_Readability        = 0x08,
        Flag_CheckPreciseScale  = 0x10
    };
    UInt8    Flags;

    // helper methods
    void InvalidateCache()          { Flags |= Flag_InvalidCache; }
    void ValidateCache()            { Flags &= ~Flag_InvalidCache; }
    bool IsCacheInvalid() const     { return (Flags & Flag_InvalidCache) != 0; }

    void SetBatchHasUnderline()     { Flags |= Flag_BatchHasUnderline; }
    void ClearBatchHasUnderline()   { Flags &= ~Flag_BatchHasUnderline; }
    bool HasBatchUnderline() const  { return (Flags & Flag_BatchHasUnderline) != 0; }

    void SetStaticText()            { Flags |= Flag_StaticText; }
    bool IsStaticText() const       { return (Flags & Flag_StaticText) != 0; }

    void SetReadability()           { Flags |= Flag_Readability; }
    void ClearReadability()         { Flags &= ~Flag_Readability; }
    bool IsReadability() const      { return (Flags & Flag_Readability) != 0; }

    void SetCheckPreciseScale()     { Flags |= Flag_CheckPreciseScale; }
    void ClearCheckPreciseScale()   { Flags &= ~Flag_CheckPreciseScale; }
    bool NeedsCheckPreciseScale() const { return (Flags & Flag_CheckPreciseScale) != 0; }
};


class GFxTextLineBuffer
{
public:
    typedef GRenderer::BitmapDesc   BitmapDesc;
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;

    enum LineType
    {
        Line8,
        Line32
    };

    class GlyphEntry
    {
        // sizeof(GlyphEntry) == 8 bytes
    protected:
        UInt16  Index;
        UInt16  Advance;  // advance to the next glyph. Might be negative (in static text), see Flags_NegAdvance
        UInt16  LenAndFontSize; // len - how many chars this record represents (4 m.s.bits)

        enum
        {
            Mask_Length     = 0xF,
            Shift_Length    = 12,
            Mask_FontSize   = 0xFFF,
            Shift_FontSize  = 0,

            Flags_InBatch       = 0x8000,
            Flags_NextFormat    = 0x4000,
            Flags_FmtHasFont    = 0x2000,
            Flags_FmtHasColor   = 0x1000,
            Flags_FmtHasImage   = 0x0800,

            Flags_Underline     = 0x0400,
            Flags_InvisibleChar = 0x0200,
            Flags_NewLineChar   = 0x0100,
            Flags_Url           = 0x0080,
            Flags_NegAdvance    = 0x0040, // used to indicate advance is negative
            Flags_ShadowInBatch = 0x0020,
            Flags_FractionalFontSize = 0x0010,
            Flags_WordWrapSeparator  = 0x0008,
            Flags_ComposStrGlyph = 0x0004
        };
        UInt16  Flags;

        void SetNegAdvance()      { Flags |= Flags_NegAdvance; }
        void ClearNegAdvance()    { Flags &= ~Flags_NegAdvance; }
        bool IsNegAdvance() const { return (Flags & Flags_NegAdvance) != 0; }
    public:
        GlyphEntry() { Init(); }

        void Init() { Flags = 0; }
        void SetIndex(UInt index)
        {
            GASSERT(index < 65536 || index == ~0u);
            Index = (UInt16)index;
        }
        UInt GetIndex() const { return (Index < 65535) ? Index : ~0u; }

        UInt GetLength() const   { return (LenAndFontSize >> Shift_Length) & Mask_Length; }
        void SetLength(UInt len) 
        { 
            GASSERT(len <= Mask_Length); 
            LenAndFontSize = (UInt16)
                ((LenAndFontSize & (~(Mask_Length << Shift_Length))) | 
                 ((len & Mask_Length) << Shift_Length));
        }

        void SetFontSize(UInt fs)
        {
            GASSERT(fs <= Mask_FontSize);
            LenAndFontSize = (UInt16)
                ((LenAndFontSize & (~(Mask_FontSize << Shift_FontSize))) | 
                 ((fs & Mask_FontSize) << Shift_FontSize));
        }
        void SetFontSize(Float fs) 
        {
            if (fs < 256.f)
            {
                UInt fxpSize = (UInt)(fs * 16.f);
                if (fxpSize & 0xF)
                {
                    Flags |= Flags_FractionalFontSize;
                    SetFontSize((UInt)fxpSize);
                    return;
                }
            }
            Flags &= ~Flags_FractionalFontSize;
            SetFontSize((UInt)fs); 
        } 
        void SetFontSizeInTwips(UInt fs) 
        {
            if (fs < PixelsToTwips(256u))
            {
                UInt fxpSize = (fs << 4) / 20;
                if (fxpSize & 0xF)
                {
                    Flags |= Flags_FractionalFontSize;
                    SetFontSize((UInt)fxpSize);
                    return;
                }
            }
            Flags &= ~Flags_FractionalFontSize;
            SetFontSize((UInt)fs); 
        }
        Float GetFontSize() const 
        {
            UInt fsz = (LenAndFontSize >> Shift_FontSize) & Mask_FontSize; 
            if (!(Flags & Flags_FractionalFontSize))
                return Float(fsz); 
            return Float(fsz)/16.f; // make fractional font size
        }

        void SetInBatch()       { Flags |= Flags_InBatch; }
        void ClearInBatch()     { Flags &= ~Flags_InBatch; }
        bool IsInBatch() const  { return (Flags & Flags_InBatch) != 0; }

        void SetShadowInBatch()       { Flags |= Flags_ShadowInBatch; }
        void ClearShadowInBatch()     { Flags &= ~Flags_ShadowInBatch; }
        bool IsShadowInBatch() const  { return (Flags & Flags_ShadowInBatch) != 0; }

        void SetComposStrGlyph()       { Flags |= Flags_ComposStrGlyph; }
        void ClearComposStrGlyph()     { Flags &= ~Flags_ComposStrGlyph; }
        bool IsComposStrGlyph() const  { return (Flags & Flags_ComposStrGlyph) != 0; }

        void SetNextFormat()       { Flags |= Flags_NextFormat; }
        void ClearNextFormat()     { Flags &= ~Flags_NextFormat; }
        bool IsNextFormat() const  { return (Flags & Flags_NextFormat) != 0; }

        void SetFmtHasFont()       { Flags |= Flags_FmtHasFont; }
        void ClearFmtHasFont()     { Flags &= ~Flags_FmtHasFont; }
        bool IsFmtHasFont() const  { return (Flags & Flags_FmtHasFont) != 0; }

        void SetFmtHasColor()       { Flags |= Flags_FmtHasColor; }
        void ClearFmtHasColor()     { Flags &= ~Flags_FmtHasColor; }
        bool IsFmtHasColor() const  { return (Flags & Flags_FmtHasColor) != 0; }

        void SetFmtHasImage()       { Flags |= Flags_FmtHasImage; }
        void ClearFmtHasImage()     { Flags &= ~Flags_FmtHasImage; }
        bool IsFmtHasImage() const  { return (Flags & Flags_FmtHasImage) != 0; }

        void SetUnderline()      { Flags |= Flags_Underline; }
        void ClearUnderline()    { Flags &= ~Flags_Underline; }
        bool IsUnderline() const { return (Flags & Flags_Underline) != 0; }

        void SetInvisibleChar()       { Flags |= Flags_InvisibleChar; }
        void ClearInvisibleChar()     { Flags &= ~Flags_InvisibleChar; }
        bool IsCharInvisible() const  { return (Flags & Flags_InvisibleChar) != 0; }

        void SetNewLineChar()       { Flags |= Flags_NewLineChar; }
        void ClearNewLineChar()     { Flags &= ~Flags_NewLineChar; }
        bool IsNewLineChar() const  { return (Flags & Flags_NewLineChar) != 0; }
        bool IsEOFChar() const      { return IsNewLineChar() && GetLength() == 0; }

        void SetUrl()      { Flags |= Flags_Url; }
        void ClearUrl()    { Flags &= ~Flags_Url; }
        bool IsUrl() const { return (Flags & Flags_Url) != 0; }

        void SetWordWrapSeparator()       { Flags |= Flags_WordWrapSeparator; }
        void ClearWordWrapSeparator()     { Flags &= ~Flags_WordWrapSeparator; }
        bool IsWordWrapSeparator() const  { return (Flags & Flags_WordWrapSeparator) != 0; }

        // set advance value, in twips
        void SetAdvance(SInt v) 
        { 
            GASSERT(v >= -65536 && v < 65536);
            if (v >= 0)
            {
                Advance = (UInt16)v;
                ClearNegAdvance();
            }
            else
            {
                Advance = (UInt16)GTL::gabs(v);
                SetNegAdvance();
            }
        }
        // returns advance, in twips
        SInt GetAdvance() const { return (!IsNegAdvance()) ? Advance : -(SInt)(Advance); }

        void ClearFlags() { Flags = 0; }
    };

    union GFxFormatDataEntry
    {
        GFxFontHandle*      pFont;
        GFxTextImageDesc*   pImage;
        UInt32              Color;
    };

    class GlyphIterator
    {
        GlyphEntry*                     pGlyphs;
        GlyphEntry*                     pEndGlyphs;
        GFxFormatDataEntry*             pNextFormatData;
        GFxTextHighlighterPosIterator   HighlighterIter;
        UInt32                          Color, OrigColor, UnderlineColor;
        GPtr<GFxFontHandle>             pFontHandle;
        GPtr<GFxTextImageDesc>          pImage;
        // a bit mask, see GFxTextHighlightInfo::Flag_<>Underline:
        // Flag_SingleUnderline, Flag_DoubleUnderline, Flag_DottedUnderline
        GFxTextHighlightInfo::UnderlineStyle UnderlineStyle; 

        void UpdateDesc();
    public:
        GlyphIterator(void* pglyphs, UInt glyphsCount, GFxFormatDataEntry* pfmtData) : 
            Color(0), OrigColor(0), UnderlineColor(0), UnderlineStyle(GFxTextHighlightInfo::Underline_None)
        {
            pGlyphs         = reinterpret_cast<GlyphEntry*>(pglyphs);
            pEndGlyphs      = pGlyphs + glyphsCount;
            pNextFormatData = pfmtData;
            UpdateDesc();
        }
        GlyphIterator(void* pglyphs, UInt glyphsCount, GFxFormatDataEntry* pfmtData, 
                      const GFxTextHighlighter& highlighter, UPInt lineStartPos) : 
            Color(0), OrigColor(0), UnderlineColor(0), UnderlineStyle(GFxTextHighlightInfo::Underline_None)
        {
            HighlighterIter = highlighter.GetPosIterator(lineStartPos);
            pGlyphs         = reinterpret_cast<GlyphEntry*>(pglyphs);
            pEndGlyphs      = pGlyphs + glyphsCount;
            pNextFormatData = pfmtData;
            UpdateDesc();
        }

        bool IsFinished() const { return !pGlyphs || pGlyphs >= pEndGlyphs; }
        void operator++()
        {
            if (!IsFinished())
            {
                if (!HighlighterIter.IsFinished())
                {
                    HighlighterIter += pGlyphs->GetLength();
                }
                ++pGlyphs;
                UpdateDesc();
            }
        }
        GFxFontResource*    GetFont()  const { return (pFontHandle) ? pFontHandle->GetFont() : NULL; }
        bool                IsFauxBold() const      { return (pFontHandle) ? pFontHandle->IsFauxBold() : false; }
        bool                IsFauxItalic() const    { return (pFontHandle) ? pFontHandle->IsFauxItalic() : false; }
        bool                IsFauxBoldItalic() const{ return (pFontHandle) ? pFontHandle->IsFauxBoldItalic() : false; }
        UInt                GetFauxFontStyle()      { return (pFontHandle) ? pFontHandle->GetFauxFontStyle() : 0;  }
        GColor              GetColor() const { return GColor(Color); }
        GlyphEntry&         GetGlyph() const { return *pGlyphs; }
        GFxTextImageDesc*   GetImage() const { return pImage; }
        bool                HasImage() const { return !!pImage; }
        bool                IsUnderline() const { return UnderlineStyle != GFxTextHighlightInfo::Underline_None; }
        bool                IsSingleUnderline() const 
            { return (UnderlineStyle == GFxTextHighlightInfo::Underline_Single); }
        bool                IsThickUnderline() const 
            { return (UnderlineStyle == GFxTextHighlightInfo::Underline_Thick); }
        bool                IsDottedUnderline() const 
            { return (UnderlineStyle == GFxTextHighlightInfo::Underline_Dotted); }
        bool                IsDitheredSingleUnderline() const 
            { return (UnderlineStyle == GFxTextHighlightInfo::Underline_DitheredSingle); }
        bool                IsDitheredThickUnderline() const 
            { return (UnderlineStyle == GFxTextHighlightInfo::Underline_DitheredThick); }
        GColor              GetUnderlineColor() const { return GColor(UnderlineColor); }
        GFxTextHighlightInfo::UnderlineStyle GetUnderlineStyle() const { return UnderlineStyle; }
    };
    class GlyphInserter
    {
        GlyphEntry*         pGlyphs;
        GFxFormatDataEntry* pNextFormatData;
        UInt                GlyphIndex;
        UInt                GlyphsCount;
        UInt                FormatDataIndex;
    public:
        GlyphInserter():pGlyphs(NULL), pNextFormatData(NULL), GlyphIndex(0),
            GlyphsCount(0), FormatDataIndex(0) {}
        GlyphInserter(GlyphEntry* pglyphs, UInt glyphsCount, GFxFormatDataEntry* pfmtData) :
            pGlyphs(reinterpret_cast<GlyphEntry*>(pglyphs)), pNextFormatData(pfmtData), 
            GlyphIndex(0), GlyphsCount(glyphsCount), FormatDataIndex(0) 
        {}
        GlyphInserter(const GlyphInserter& gi)
        {
            operator=(gi);
        }
        GlyphInserter& operator=(const GlyphInserter& gi)
        {
            pGlyphs         = gi.pGlyphs;
            pNextFormatData = gi.pNextFormatData;
            GlyphIndex      = gi.GlyphIndex;
            GlyphsCount     = gi.GlyphsCount;
            FormatDataIndex = gi.FormatDataIndex;
            return *this;
        }
        void Reset()
        {
            GlyphIndex = FormatDataIndex = 0;
        }
        bool IsFinished() const { return !pGlyphs || GlyphIndex >= GlyphsCount; }
        void operator++()
        {
            if (!IsFinished())
            {
                ++GlyphIndex;
            }
        }
        void operator++(int)
        {
            if (!IsFinished())
            {
                ++GlyphIndex;
            }
        }
        GlyphEntry& GetGlyph() 
        { 
            GlyphEntry& pg = pGlyphs[GlyphIndex];
            pg.Init();
            return pg; 
        }
        void AddFont(GFxFontHandle* pfont)
        {
            pGlyphs[GlyphIndex].SetNextFormat();
            GASSERT(!pGlyphs[GlyphIndex].IsFmtHasFont());
            GASSERT(!pGlyphs[GlyphIndex].IsFmtHasColor()); // font should be set BEFORE color
            GASSERT(!pGlyphs[GlyphIndex].IsFmtHasImage()); // image cannot be set with font
            pGlyphs[GlyphIndex].SetFmtHasFont();
            pNextFormatData[FormatDataIndex++].pFont = pfont;
            pfont->AddRef();
        }
        void AddColor(GColor color)
        {
            pGlyphs[GlyphIndex].SetNextFormat();
            GASSERT(!pGlyphs[GlyphIndex].IsFmtHasColor());
            GASSERT(!pGlyphs[GlyphIndex].IsFmtHasImage()); // image cannot be set with color
            pGlyphs[GlyphIndex].SetFmtHasColor();
            pNextFormatData[FormatDataIndex++].Color = color.ToColor32();
        }
        void AddImage(GFxTextImageDesc* pimage)
        {
            pGlyphs[GlyphIndex].SetNextFormat();
            GASSERT(!pGlyphs[GlyphIndex].IsFmtHasFont()); // image cannot be set with font
            GASSERT(!pGlyphs[GlyphIndex].IsFmtHasColor());// image cannot be set with color
            GASSERT(!pGlyphs[GlyphIndex].IsFmtHasImage());
            pGlyphs[GlyphIndex].SetFmtHasImage();
            pNextFormatData[FormatDataIndex++].pImage = pimage;
            pimage->AddRef();
        }
        UInt GetFormatDataElementsCount() const { return FormatDataIndex; }
        UInt GetGlyphIndex() const { return GlyphIndex; }

        void ResetTo(const GlyphInserter& savedPos);

        GlyphEntry* GetReadyGlyphs(UInt* pnumGlyphs)
        {
            *pnumGlyphs = GetGlyphIndex();
            return pGlyphs;
        }
    };
    struct LineData32 // 38 bytes
    {
        UInt32                  GlyphsCount;
        UInt32                  TextPos;        // index of the first char in the text
        SInt32                  OffsetX, OffsetY;         // offset of TopLeft corner, in twips
        UInt32                  Width, Height;     // dimensions, in twips; (W,H = 0) for static text;
        UInt32                  ParagraphId;
        UInt32                  TextLength; 
        UInt16                  ParagraphModId;
        UInt16                  BaseLineOffset; // Y-offset from Bounds.Top to baseline, in twips
        SInt16                  Leading;

        void Init()
        {
            GlyphsCount = 0;
            TextPos = 0;
            BaseLineOffset = 0;
            Leading = 0;
            OffsetX = OffsetY = 0;
            Width = Height = 0;
            TextLength = 0;
        }
        static UInt GetLineStructSize() 
        { 
            struct _a { UInt32 b; UInt16 a; };
            return sizeof(LineData32) - (sizeof(_a) - (sizeof(UInt32) + sizeof(UInt16))); 
        }

        UInt GetTextPos() const { return TextPos; }
        void SetTextPos(UInt tp) { TextPos = tp; }

        UInt GetNumGlyphs() const { return GlyphsCount; }
        void SetNumGlyphs(UInt tp) { GlyphsCount = tp; }

        GlyphEntry* GetGlyphs() const { return reinterpret_cast<GlyphEntry*>(((UByte*)this) + GetLineStructSize()); }

        void SetTextLength(UInt len) 
        { 
            TextLength = (UInt32)len;
        }
        UInt GetTextLength() const { return TextLength; }

        void  SetBaseLineOffset(Float baseLine) { BaseLineOffset = (UInt16)baseLine; }
        Float GetBaseLineOffset() const         { return Float(BaseLineOffset); }

        void  SetLeading(SInt leading) { GASSERT(leading >= -32768 && leading < 32768); Leading = (SInt16)leading; }
        SInt  GetLeading() const       { return SInt(Leading); }
        SInt  GetNonNegLeading() const { return Leading > 0 ? SInt(Leading) : 0; }

        void SetBounds(const GRectF& bounds) 
        { 
            SetOffset(bounds.TopLeft()); 
            SetDimensions((SInt)bounds.Width(), (SInt)bounds.Height());
        }
        // set width & height, specified in twips
        void SetDimensions(SInt w, SInt h)
        {
            GASSERT(w >= 0 && h >= 0 && w < INT_MAX && h < INT_MAX);
            Width = UInt32(w);
            Height = UInt32(h);
        }
        SInt GetWidth() const  { return SInt(Width);  }
        SInt GetHeight() const { return SInt(Height); }

        void    SetOffset(const GPointF& p) { OffsetX = SInt(p.x); OffsetY = SInt(p.y); }
        GPointF GetOffset() const { return GPointF(Float(OffsetX), Float(OffsetY)); }
        Float GetOffsetX() const { return Float(OffsetX); }
        Float GetOffsetY() const { return Float(OffsetY); }
        void SetOffsetX(Float x) { OffsetX = (SInt32)x; }
        void SetOffsetY(Float y) { OffsetY = (SInt32)y; }

        UInt GetParagraphId() const    { return ParagraphId; }
        void SetParagraphId(UInt id)   { ParagraphId = id; }
        UInt16 GetParagraphModId() const { return ParagraphModId; }
        void SetParagraphModId(UInt16 id){ ParagraphModId = id; }
    };
    // 26 bytes data. Limitations:
    // Dimensions = [-32768..32767] (i.e. -1638.4 .. 1638.35 pts), 
    // TextLen <= 255, GlyphsCount <= 255, Leading <= 255 (i.e. 12.75 pt), TextPos < 2^24
    struct LineData8
    {
        UInt32                  ParagraphId;
        UInt32                  TextPosAndLength;  // index of the first char in the text / length (1 most signif. octet)
        SInt32                  OffsetX, OffsetY;  // offset of TopLeft corner, in twips
        UInt16                  Width, Height;     // dimensions, in twips; (W,H = 0) for static text;
        UInt16                  ParagraphModId;
        UInt16                  BaseLineOffset; // Y-offset from Bounds.Top to baseline, in twips
        UInt8                   GlyphsCount;
        SInt8                   Leading;
        enum
        {
            Mask_TextPos = 0xFFFFFFu,
            Shift_TextPos = 0,

            Mask_TextLength = 0xFF,
            Shift_TextLength = 24
        };

        void Init()
        {
            GlyphsCount = 0;
            TextPosAndLength = 0;
            BaseLineOffset = 0;
            Leading = 0;
            OffsetX = OffsetY = 0;
            Width = Height = 0;
        }
        static UInt GetLineStructSize() 
        { 
            struct _a { UInt32 b; UInt16 a; };
            return sizeof(LineData8) - (sizeof(_a) - (sizeof(UInt32) + sizeof(UInt16))); 
        }

        UInt GetTextPos() const 
        { 
            UInt v = (TextPosAndLength >> Shift_TextPos) & Mask_TextPos;
            return (v == Mask_TextPos) ? ~0u : v; 
        }
        void SetTextPos(UInt tp) 
        { 
            GASSERT(tp <= Mask_TextPos || tp == ~0u);
            TextPosAndLength = 
                (TextPosAndLength & (~(Mask_TextPos << Shift_TextPos))) | 
                ((tp & Mask_TextPos) << Shift_TextPos);
        }

        UInt GetNumGlyphs() const { return GlyphsCount; }
        void SetNumGlyphs(UInt tp) { GASSERT(tp < 256); GlyphsCount = (UInt8)tp; }

        GlyphEntry* GetGlyphs() const { return reinterpret_cast<GlyphEntry*>(((UByte*)this) + GetLineStructSize()); }

        void SetTextLength(UInt len) 
        { 
            GASSERT(len < 256);
            TextPosAndLength = 
                (TextPosAndLength & (~(Mask_TextLength << Shift_TextLength))) | 
                ((len & Mask_TextLength) << Shift_TextLength);
        }
        UInt GetTextLength() const { return (TextPosAndLength >> Shift_TextLength) & Mask_TextLength; }

        void  SetBaseLineOffset(Float baseLine) { BaseLineOffset = (UInt16)baseLine; }
        Float GetBaseLineOffset() const         { return Float(BaseLineOffset); }

        void  SetLeading(SInt leading) { GASSERT(leading >= -128 && leading < 128); Leading = (SInt8)leading; }
        SInt  GetLeading() const       { return SInt(Leading); }
        SInt  GetNonNegLeading() const { return Leading > 0 ? SInt(Leading) : 0; }

        void SetBounds(const GRectF& bounds) 
        { 
            SetOffset(bounds.TopLeft()); 
            SetDimensions((SInt)bounds.Width(), (SInt)bounds.Height());
        }
        // set width & height, specified in twips
        void SetDimensions(SInt w, SInt h)
        {
            GASSERT(w >= 0 && h >= 0 && w < 65536 && h < 65536);
            Width = UInt16(w);
            Height = UInt16(h);
        }
        SInt GetWidth() const  { return SInt(Width);  }
        SInt GetHeight() const { return SInt(Height); }

        void    SetOffset(const GPointF& p) 
        { 
            OffsetX = SInt32(p.x); OffsetY = SInt32(p.y); 
        }
        GPointF GetOffset() const { return GPointF(Float(OffsetX), Float(OffsetY)); }
        Float GetOffsetX() const { return Float(OffsetX); }
        Float GetOffsetY() const { return Float(OffsetY); }
        void SetOffsetX(Float x) { OffsetX = (SInt32)x; }
        void SetOffsetY(Float y) { OffsetY = (SInt32)y; }

        UInt GetParagraphId() const    { return ParagraphId; }
        void SetParagraphId(UInt id)   { ParagraphId = id; }
        UInt16 GetParagraphModId() const { return ParagraphModId; }
        void SetParagraphModId(UInt16 id){ ParagraphModId = id; }
    };

    class Line
    {
        UInt32 MemSize;
        enum Flags
        {
            Flags_Data8         = 0x80000000u, // if set, Data8 is in use.
            Flags_Initialized   = 0x40000000u,

            Mask_MemSize        = 0x0FFFFFFFu,
            Shift_MemSize       = 0,

            Mask_Alignment      = 0x3,
            Shift_Alignment     = 28
        };
        union 
        {
            LineData32  Data32;
            LineData8   Data8;
        };

    public:
        enum Alignment
        {
            Align_Left   = 0,
            Align_Right  = 1,
            Align_Center = 2
        };
    public:
        Line():MemSize(0) {}

        void InitLine8()  { MemSize &= Mask_MemSize; MemSize |= Flags_Data8; MemSize |= Flags_Initialized;  Data8.Init(); }
        void InitLine32() { MemSize &= Mask_MemSize; MemSize |= Flags_Initialized; Data32.Init(); }
        void ClearInitialized()    { MemSize &= ~Flags_Initialized; }
        bool IsInitialized() const { return (MemSize & Flags_Initialized) != 0; }
        static UInt GetLineStructSize(LineType lineType) 
        { 
            return sizeof(UInt32) + (lineType == Line8) ? LineData8::GetLineStructSize() : LineData32::GetLineStructSize(); 
        }
        void Release();

        inline bool IsData8() const  { return (MemSize & Flags_Data8) != 0; }
        inline bool IsData32() const { return !IsData8(); }

        void SetMemSize(UInt len) 
        { 
            GASSERT(len <= Mask_MemSize); 
            MemSize = 
                (MemSize & (~(Mask_MemSize << Shift_MemSize))) | 
                ((len & Mask_MemSize) << Shift_MemSize);
        }
        UInt GetMemSize() const { return (MemSize >> Shift_MemSize) & Mask_MemSize; }

        void SetAlignment(Alignment a)
        {
            MemSize = 
                (MemSize & (~(Mask_Alignment << Shift_Alignment))) | 
                ((a & Mask_Alignment) << Shift_Alignment);

        }
        Alignment GetAlignment() const { return (Alignment)((MemSize >> Shift_Alignment) & Mask_Alignment);}
        bool IsLeftAlignment() const   { return (GetAlignment() == Align_Left); }
        bool IsRightAlignment() const  { return (GetAlignment() == Align_Right); }
        bool IsCenterAlignment() const { return (GetAlignment() == Align_Center); }

        UInt GetTextPos() const  { return (IsData8()) ? Data8.GetTextPos() : Data32.GetTextPos(); }
        void SetTextPos(UInt tp) { (IsData8()) ? Data8.SetTextPos(tp) : Data32.SetTextPos(tp); }

        UInt GetNumGlyphs() const { return (IsData8()) ? Data8.GetNumGlyphs() : Data32.GetNumGlyphs(); }
        void SetNumGlyphs(UInt tp) { (IsData8()) ? Data8.SetNumGlyphs(tp) : Data32.SetNumGlyphs(tp); }

        GlyphEntry* GetGlyphs() const { return (IsData8()) ? Data8.GetGlyphs() : Data32.GetGlyphs(); }
        GFxFormatDataEntry* GetFormatData() const;

        void SetTextLength(UInt len) { (IsData8()) ? Data8.SetTextLength(len) : Data32.SetTextLength(len); }
        UInt GetTextLength() const   { return (IsData8()) ? Data8.GetTextLength() : Data32.GetTextLength(); }

        void  SetBaseLineOffset(Float baseLine) { (IsData8()) ? Data8.SetBaseLineOffset(baseLine) : Data32.SetBaseLineOffset(baseLine); }
        Float GetBaseLineOffset() const         { return (IsData8()) ? Data8.GetBaseLineOffset() : Data32.GetBaseLineOffset(); }

        Float GetAscent() const  { return GetBaseLineOffset(); }
        Float GetDescent() const { return GetHeight() - GetBaseLineOffset(); }

        void  SetLeading(SInt leading) { (IsData8()) ? Data8.SetLeading(leading) : Data32.SetLeading(leading); }
        SInt  GetLeading() const       { return (IsData8()) ? Data8.GetLeading() : Data32.GetLeading(); }
        SInt  GetNonNegLeading() const { return (IsData8()) ? Data8.GetNonNegLeading() : Data32.GetNonNegLeading(); }

        void SetBounds(const GRectF& bounds) { (IsData8()) ? Data8.SetBounds(bounds) : Data32.SetBounds(bounds); }
        void SetDimensions(SInt w, SInt h)   { (IsData8()) ? Data8.SetDimensions(w, h) : Data32.SetDimensions(w, h); }
        SInt GetWidth() const  { return (IsData8()) ? Data8.GetWidth() : Data32.GetWidth(); }
        SInt GetHeight() const { return (IsData8()) ? Data8.GetHeight() : Data32.GetHeight(); }

        void SetOffset(const GPointF& p) { (IsData8()) ? Data8.SetOffset(p) : Data32.SetOffset(p); }
        GPointF GetOffset() const        { return (IsData8()) ? Data8.GetOffset() : Data32.GetOffset(); }
        Float GetOffsetX() const { return (IsData8()) ? Data8.GetOffsetX() : Data32.GetOffsetX(); }
        Float GetOffsetY() const { return (IsData8()) ? Data8.GetOffsetY() : Data32.GetOffsetY(); }
        void SetOffsetX(Float x) { (IsData8()) ? Data8.SetOffsetX(x) : Data32.SetOffsetX(x); }
        void SetOffsetY(Float y) { (IsData8()) ? Data8.SetOffsetY(y) : Data32.SetOffsetY(y); }

        UInt GetParagraphId() const    { return (IsData8()) ? Data8.GetParagraphId() : Data32.GetParagraphId(); }
        UInt16 GetParagraphModId() const { return (IsData8()) ? Data8.GetParagraphModId() : Data32.GetParagraphModId(); }
        void SetParagraphId(UInt id)   { (IsData8()) ? Data8.SetParagraphId(id) : Data32.SetParagraphId(id); }
        void SetParagraphModId(UInt16 id){ (IsData8()) ? Data8.SetParagraphModId(id) : Data32.SetParagraphModId(id); }

        GlyphIterator Begin() { return GlyphIterator(GetGlyphs(), GetNumGlyphs(), GetFormatData()); }
        GlyphIterator Begin(const GFxTextHighlighter* phighlighter) 
        {
            if (phighlighter)
                return GlyphIterator(GetGlyphs(), GetNumGlyphs(), GetFormatData(), *phighlighter, GetTextPos()); 
            else
                return Begin();
        }

        bool HasNewLine() const;
    };

private:
    struct LineIndexComparator
    {
        int Compare(const Line* p1, int index) const
        {
            GASSERT(p1);
            int si1 = p1->GetTextPos();
            if (index >= si1 && index < (int)(si1 + p1->GetTextLength()))
                return 0; 
            return si1 - index;
        }
    };

    struct LineYOffsetComparator
    {
        int Compare(const Line* p1, Float yoffset) const
        {
            GASSERT(p1);
            Float si1 = p1->GetOffsetY();
            if (yoffset >= si1 && yoffset < si1 + p1->GetHeight() + p1->GetLeading())
                return 0; 
            return int(si1 - yoffset);
        }
    };

    static void ReleasePartOfLine(GlyphEntry* pglyphs, UInt n, GFxFormatDataEntry* pnextFormatData);

public:
    GFxTextLineBuffer();
    ~GFxTextLineBuffer();

    Line& operator[](UInt pos) 
    {
        return *Lines[pos];
    }
    const Line& operator[](UInt pos) const
    {
        return *Lines[pos];
    }
    UInt size() const { return (UInt)Lines.size(); }

    void ResetCache(); 

    void ClearLines()
    {
        InvalidateCache();
        RemoveLines(0, (UInt)Lines.size());
    }

    friend class Iterator;
    friend class ConstIterator;

    class ConstIterator
    {
        friend class GFxTextLineBuffer;

        const GFxTextLineBuffer*  pLineBuffer;
        const GFxTextHighlighter* pHighlight;
        UInt                      CurrentPos;
        Float                     YOffset;    // negative, if vertically scrolled. Used for IsVisible.
        bool                      StaticText; // for static text, all lines are visible

        ConstIterator(const GFxTextLineBuffer& lb):
        pLineBuffer(&lb), CurrentPos(0), YOffset(0), StaticText(lb.IsStaticText()) {}
    public:
        ConstIterator():pLineBuffer(NULL), CurrentPos(0), YOffset(0), StaticText(0) {}
        ConstIterator(const ConstIterator& it):
        pLineBuffer(it.pLineBuffer), CurrentPos(it.CurrentPos), YOffset(it.YOffset), StaticText(it.StaticText) {}
        ConstIterator(const GFxTextLineBuffer& lb, UInt pos, Float yoffset = 0):
        pLineBuffer(&lb), CurrentPos(pos), YOffset(yoffset), StaticText(lb.IsStaticText()) {}

        bool IsFinished() const 
        { 
            return !pLineBuffer || CurrentPos >= pLineBuffer->Lines.size() || (SInt)CurrentPos < 0; 
        }
        bool IsVisible() const 
        { 
            if (!IsFinished())
            {
                if (StaticText) return true; // for static text we reckon all lines are visible.
                // otherwise, we need to sort all static text lines by Y
                return pLineBuffer->IsLineVisible(CurrentPos, YOffset);
            }
            return false;
        }

        ConstIterator& operator++() 
        { 
            GASSERT(pLineBuffer);
            if (CurrentPos < pLineBuffer->Lines.size())
                ++CurrentPos; 
            return *this; 
        }    
        ConstIterator operator++(int) 
        { 
            GASSERT(pLineBuffer);
            ConstIterator it(*this); 
            if (CurrentPos < pLineBuffer->Lines.size())
                ++CurrentPos; 
            return it; 
        }   
        ConstIterator& operator--() 
        { 
            GASSERT(pLineBuffer);
            if ((SInt)CurrentPos >= 0)
                --CurrentPos; 
            return *this; 
        }    
        ConstIterator operator--(int) 
        { 
            GASSERT(pLineBuffer);
            ConstIterator it(*this); 
            if ((SInt)CurrentPos >= 0)
                --CurrentPos; 
            return it; 
        }   
        ConstIterator operator+(UInt delta) const
        {
            return ConstIterator(*pLineBuffer, CurrentPos + delta, YOffset);
        }
        ConstIterator operator-(UInt delta) const
        {
            return ConstIterator(*pLineBuffer, CurrentPos - delta, YOffset);
        }
        const Line& operator*() const
        {
            GASSERT(pLineBuffer);
            return (*pLineBuffer)[CurrentPos];
        }
        const Line* operator->() const
        {
            GASSERT(pLineBuffer);
            return &(*pLineBuffer)[CurrentPos];
        }
        const Line* GetPtr() const
        {
            GASSERT(pLineBuffer);
            return &(*pLineBuffer)[CurrentPos];
        }
        UInt GetIndex() const { return ((SInt)CurrentPos >= 0) ? UInt(CurrentPos) : ~0u; }

        void SetHighlighter(const GFxTextHighlighter* ph) { pHighlight = ph; }
        const GFxTextHighlighter* GetGighlighter() const  { return pHighlight; }
    };

    class Iterator
    {
        friend class GFxTextLineBuffer;

        GFxTextLineBuffer*        pLineBuffer;
        const GFxTextHighlighter* pHighlight;
        UInt                      CurrentPos;
        Float                     YOffset;    // negative, if vertically scrolled. Used for IsVisible.
        bool                      StaticText; // for static text, all lines are visible

        Iterator(GFxTextLineBuffer& lb):
            pLineBuffer(&lb), pHighlight(NULL), CurrentPos(0), YOffset(0), StaticText(lb.IsStaticText()) {}
        Iterator(GFxTextLineBuffer& lb, UInt pos, Float yoffset = 0):
            pLineBuffer(&lb), pHighlight(NULL), CurrentPos(pos), YOffset(yoffset), StaticText(lb.IsStaticText()) {}
    public:
        Iterator():pLineBuffer(NULL), pHighlight(NULL), CurrentPos(0), YOffset(0), StaticText(false) {}
        Iterator(const Iterator& it):
            pLineBuffer(it.pLineBuffer), pHighlight(it.pHighlight), CurrentPos(it.CurrentPos), YOffset(it.YOffset), 
            StaticText(it.StaticText) {}

        bool IsFinished() const 
        { 
            return !pLineBuffer || CurrentPos >= pLineBuffer->Lines.size() || (SInt)CurrentPos < 0; 
        }
        bool IsVisible() const 
        { 
            if (!IsFinished())
            {
                if (StaticText) return true; // for static text we reckon all lines are visible.
                                             // otherwise, we need to sort all static text lines by Y
                return pLineBuffer->IsLineVisible(CurrentPos, YOffset);
            }
            return false;
        }

        Iterator& operator++() 
        { 
            GASSERT(pLineBuffer);
            if (CurrentPos < pLineBuffer->Lines.size())
                ++CurrentPos; 
            return *this; 
        }    
        Iterator operator++(int) 
        { 
            GASSERT(pLineBuffer);
            Iterator it(*this); 
            if (CurrentPos < pLineBuffer->Lines.size())
                ++CurrentPos; 
            return it; 
        }   
        Iterator& operator--() 
        { 
            GASSERT(pLineBuffer);
            if ((SInt)CurrentPos >= 0)
                --CurrentPos; 
            return *this; 
        }    
        Iterator operator--(int) 
        { 
            GASSERT(pLineBuffer);
            Iterator it(*this); 
            if ((SInt)CurrentPos >= 0)
                --CurrentPos; 
            return it; 
        }   
        Line& operator*() const
        {
            GASSERT(pLineBuffer);
            return (*pLineBuffer)[CurrentPos];
        }
        Line* operator->() const
        {
            GASSERT(pLineBuffer);
            return &(*pLineBuffer)[CurrentPos];
        }
        Line* GetPtr() const
        {
            GASSERT(pLineBuffer);
            return &(*pLineBuffer)[CurrentPos];
        }
        void Remove(UInt num = 1)
        {
            if(!IsFinished())
            {
                pLineBuffer->RemoveLines(CurrentPos, num);
            }
        }
        Iterator operator+(UInt delta) const
        {
            return Iterator(*pLineBuffer, CurrentPos + delta, YOffset);
        }
        Iterator operator-(UInt delta) const
        {
            return Iterator(*pLineBuffer, CurrentPos - delta, YOffset);
        }
        Line* InsertNewLine(UInt glyphCount, UInt formatDataElementsCount, LineType lineType)
        {
            GASSERT(pLineBuffer);
            UInt pos = GetIndex();
            if (pos == ~0u)
                pos = (UInt)pLineBuffer->Lines.size();
            Line* pline = pLineBuffer->InsertNewLine(pos, glyphCount, formatDataElementsCount, lineType);
            ++CurrentPos;
            return pline;
        }
        UInt GetIndex() const { return ((SInt)CurrentPos >= 0) ? UInt(CurrentPos) : ~0u; }

        void SetHighlighter(const GFxTextHighlighter* ph) { pHighlight = ph; }
        const GFxTextHighlighter* GetHighlighter() const  { return pHighlight; }

        GFxTextLineBuffer::ConstIterator GetConstIterator() const;
    };

    Iterator Begin()                        { return Iterator(*this); }
    Iterator BeginVisible(Float yoffset)    { return Iterator(*this, Geom.FirstVisibleLinePos, yoffset); }
    Iterator Last()                         { return Iterator(*this, (UInt)Lines.size() - 1); }
    Iterator FindLineByTextPos(UPInt textPos); 
    Iterator FindLineAtYOffset(Float yoff);

    ConstIterator Begin() const             { return ConstIterator(*this); }
    ConstIterator BeginVisible(Float yoffset) const { return ConstIterator(*this, Geom.FirstVisibleLinePos, yoffset); }
    ConstIterator Last() const              { return ConstIterator(*this, (UInt)Lines.size() - 1); }
    ConstIterator FindLineByTextPos(UPInt textPos) const
    { 
        Iterator iter = const_cast<GFxTextLineBuffer*>(this)->FindLineByTextPos(textPos);
        return iter.GetConstIterator();
    }
    ConstIterator FindLineAtYOffset(Float yoff) const
    { 
        return const_cast<GFxTextLineBuffer*>(this)->FindLineAtYOffset(yoff).GetConstIterator();
    }

    void     SetVisibleRect(const GRectF& bnd) { Geom.VisibleRect = bnd; }

    Line*        GetLine(UInt lineIdx);
    const Line*  GetLine(UInt lineIdx) const;
    Line*        InsertNewLine(UInt lineIdx, UInt glyphCount, UInt formatDataElementsCount, LineType lineType);
    void         RemoveLines(UInt lineIdx, UInt num);
    static UInt  CalcLineSize(UInt glyphCount, UInt formatDataElementsCount, LineType lineType);

    // sets/gets the index of first visible line, 0-based
    void    SetFirstVisibleLine(UInt line);
    UInt    GetFirstVisibleLineIndex() const { return Geom.FirstVisibleLinePos; }

    // gets Y-offset of the first visible line, in twips, in line buffer's coord space (0-based)
    UInt    GetVScrollOffsetInTwips() const;

    // gets/sets hscroll offset of the line buffer, in twips
    void    SetHScrollOffset(UInt offset);
    UInt    GetHScrollOffset() const { return Geom.HScrollOffset; }

    static bool ApplyMask (GFxDisplayContext &context, const GRenderer::Matrix& mat, const GRectF& rect);
    static void RemoveMask(GFxDisplayContext &context, const GRenderer::Matrix& mat, const GRectF& rect);
    static bool DrawMask  (GFxDisplayContext &context, const GRenderer::Matrix& mat, const GRectF& rect, 
                           GRenderer::SubmitMaskMode maskMode);

    void DrawUnderline
        (Float offx, Float offy, Float singleUnderlineHeight, Float singlePointWidth, const GRectF& hscrolledViewRect, 
         SInt lineLength, GFxTextHighlightInfo::UnderlineStyle style, UInt32 color,
         const Matrix& globalMatrixDir, const Matrix& globalMatrixInv);

    void Display(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx, 
                 bool nextFrame, const GFxTextFieldParam& param, const GFxTextHighlighter* phighlighter = NULL);

    template <class Pnt, class Rect>
    Pnt BufferToView(const Rect& viewRect, const Pnt& p)
    {
        Pnt dp(p);
        dp.x += Geom.VisibleRect.Left - viewRect.Left;
        dp.y += Geom.VisibleRect.Top - viewRect.Top;
        return dp;
    }
    template <class Pnt, class Rect>
    Pnt ViewToBuffer(const Rect& viewRect, const Pnt& p)
    {
        Pnt dp(p);
        dp.x -= Geom.VisibleRect.Left - viewRect.Left;
        dp.y -= Geom.VisibleRect.Top - viewRect.Top;
        return dp;
    }

    void InvalidateCache()          { Geom.InvalidateCache(); }
    void ValidateCache()            { Geom.ValidateCache(); }
    bool IsCacheInvalid() const     { return Geom.IsCacheInvalid(); }

    void SetBatchHasUnderline()     { Geom.SetBatchHasUnderline(); }
    void ClearBatchHasUnderline()   { Geom.ClearBatchHasUnderline(); }
    bool HasBatchUnderline() const  { return Geom.HasBatchUnderline(); }

    void SetStaticText()            { Geom.SetStaticText(); }
    bool IsStaticText() const       { return Geom.IsStaticText(); }

    bool IsLineVisible(UInt lineIndex) const
    {
        Float yOffset = -Float(GetVScrollOffsetInTwips());
        return IsLineVisible(lineIndex, yOffset);
    }
    bool IsLineVisible(UInt lineIndex, Float yOffset) const;
    bool IsPartiallyVisible(Float yOffset) const;

    // scale the whole line buffer
    void Scale(Float scaleFactor);
    // returns the minimal height of all lines in the buffer (in twips)
    SInt GetMinLineHeight() const;
#ifdef GFC_BUILD_DEBUG
    void Dump() const;
    void CheckIntegrity() const;
#else
    void Dump() const {}
    void CheckIntegrity() const {}
#endif

public:
    GTL::garray<Line*>          Lines;

    GFxBatchPackage*            pBatchPackage;
    GFxFontCacheManagerImpl*    pCacheManager;

    GFxLineBufferGeometry       Geom;
    GFxTextFieldParam           Param;

    GFxFillStyle                DummyStyle; // used to pass a color on to ShapeCharacter::Display()
  //GFxLineStyle                DummyLineStyle;

    class TextLineAllocator
    {
        //TODO: GTL::garray<Line*>  FreeLinesPool;
    public:
        Line* AllocLine(UInt size, GFxTextLineBuffer::LineType lineType);
        void FreeLine(Line* ptr);
    } LineAllocator;

    GPtr<GFxDrawingContext>     pUnderlineDrawing;
    UInt                        LastHScrollOffset;

};

inline GFxTextLineBuffer::ConstIterator GFxTextLineBuffer::Iterator::GetConstIterator() const
{
    ConstIterator citer(*pLineBuffer, CurrentPos, YOffset);
    citer.SetHighlighter(pHighlight);
    return citer;
}

inline bool GFxTextLineBuffer::ApplyMask(GFxDisplayContext &context, const GRenderer::Matrix& mat, const GRectF& rect)
{
    return GFxTextLineBuffer::DrawMask(context, mat, rect, (context.MaskStackIndex == 0) ?
        GRenderer::Mask_Clear : GRenderer::Mask_Increment);
}

inline void GFxTextLineBuffer::RemoveMask(GFxDisplayContext &context, const GRenderer::Matrix& mat, const GRectF& rect)
{
    if (context.MaskStackIndex == 0)
        context.GetRenderer()->DisableMask();
    else
        GFxTextLineBuffer::DrawMask(context, mat, rect, GRenderer::Mask_Decrement);
}

// recalculate the srcRect and the matrix to fit into 16-bit vertex format.
void GFx_RecalculateRectToFit16Bit(GRenderer::Matrix& matrix, const GRectF& srcRect, GRectF* pdestRect);


#endif //INC_GFXTEXTLINEBUFFER_H

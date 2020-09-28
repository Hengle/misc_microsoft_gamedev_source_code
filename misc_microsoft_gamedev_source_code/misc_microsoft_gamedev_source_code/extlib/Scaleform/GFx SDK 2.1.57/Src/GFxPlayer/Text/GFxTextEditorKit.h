/**********************************************************************

Filename    :   GFxTextEditorKit.h
Content     :   Editor implementation
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

#ifndef INC_TEXT_GFXEDITORKIT_H
#define INC_TEXT_GFXEDITORKIT_H

#include "Text/GFxTextDocView.h"

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

#if 0//ndef GFC_NO_IME_SUPPORT
class GFxTextCompositionString : public GRefCountBase<GFxTextCompositionString>
{
    friend class GFxTextEditorKit;

    GPtr<GFxTextFormat> pDefaultFormat;
    GFxTextEditorKit*   pEditorKit;
    GPtr<GFxTextAllocator> pAllocator;
    GFxTextParagraph    String;
    UPInt               CursorPos;
    bool                HasHighlightingFlag;
    GFxTextIMEStyle     Styles;
    UInt                HighlightIds[GFxTextIMEStyle::SC_MaxNum*2];
    UInt8               HighlightIdsUsed; // number of highlight ids used

    GFxTextParagraph*   GetSourceParagraph();
    void                Reformat();
public:
    GFxTextCompositionString(GFxTextEditorKit* peditorKit);
    ~GFxTextCompositionString();

    GFxTextAllocator*   GetAllocator() const { return pAllocator; }

    void SetText(const wchar_t*, UPInt nchars = GFC_MAX_UPINT);
    const wchar_t* GetText() const { return String.GetText(); }

    void SetPosition(UPInt pos);
    UPInt GetPosition() const { return String.GetStartIndex(); }

    UPInt GetLength() const { return String.GetLength(); }

    GFxTextParagraph& GetParagraph() { return String; }
    GFxTextFormat* GetTextFormat(UPInt pos) { return String.GetTextFormatPtr(pos); }
    GFxTextFormat* GetDefaultTextFormat() const { return pDefaultFormat; }

    void  SetCursorPosition(UPInt pos);
    UPInt GetCursorPosition() const { return CursorPos; }

    void  HighlightText(UPInt pos, UPInt len, GFxTextIMEStyle::Category styleCategory);

    void  UseStyles(const GFxTextIMEStyle&);

    void  ClearHighlighting();
    bool  HasHighlighting() const { return HasHighlightingFlag; }

    static GFxTextIMEStyle GetDefaultStyles();
};
#endif //#ifndef GFC_NO_IME_SUPPORT


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

    GFxTextAllocator*   GetAllocator() const;

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

#endif //INC_TEXT_GFXEDITORKIT_H

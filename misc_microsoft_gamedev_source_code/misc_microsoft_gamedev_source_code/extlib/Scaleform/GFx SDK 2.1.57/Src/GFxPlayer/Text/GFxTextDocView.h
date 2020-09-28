/**********************************************************************

Filename    :   GFxTextDocView.h
Content     :   Document-view implementation
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

#ifndef INC_TEXT_GFXTEXTDOCVIEW_H
#define INC_TEXT_GFXTEXTDOCVIEW_H

#include "GRange.h"
#include "Text/GFxStyledText.h"

class GFxTextEditorKit;
class GFxTextStyleManager;

#define GFX_EDIT_HSCROLL_DELTA PixelsToTwips(60)

class GFxTextDocView : public GRefCountBase<GFxTextDocView>
{
    friend class GFxTextEditorKit;
    friend class GFxTextCompositionString;
    friend class GFxLineCursor;
    friend class GFxParagraphFormatter;

    typedef GRenderer::BitmapDesc   BitmapDesc;
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;
public:
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
        UPInt           ParaTextLen;            // [in] length, in chars
        Float*          pWidths;                // [in] arr of line widths before char at the index, size = NumCharsInLine + 1
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

        virtual GFxString GetCharacterPath() { return GFxString(); }
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
    struct FindFontInfo
    {
        FontCache*              pFontCache;
        const GFxTextFormat*    pCurrentFormat;
        const GFxTextFormat*    pPrevFormat;
        GPtr<GFxFontHandle>     pCurrentFont;

        //FindFontInfo():pFontCache(NULL) { init(); }
        explicit FindFontInfo(FontCache* pfc = NULL) : pFontCache(pfc) { init(); }

        inline void init() { pCurrentFormat = pPrevFormat = NULL; pCurrentFont = NULL; }
    };

protected:
    GFxTextAllocator* GetAllocator() { return pDocument->GetAllocator(); }

    void ClearReformatReq()   { RTFlags &= (~(RTFlags_ReformatReq | RTFlags_CompleteReformatReq)); }
    bool IsReformatReq() const{ return (RTFlags & (RTFlags_ReformatReq | RTFlags_CompleteReformatReq)) != 0; }

    void ClearCompleteReformatReq()   { RTFlags &= (~RTFlags_CompleteReformatReq); }
    bool IsCompleteReformatReq() const{ return (RTFlags & RTFlags_CompleteReformatReq) != 0; }

    GFxFontHandle* FindFont(FindFontInfo* pfontInfo);

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

    UInt GetHScrollOffsetValue() const { return LineBuffer.GetHScrollOffset(); }    
    UInt GetVScrollOffsetValue() const { return LineBuffer.GetFirstVisibleLineIndex(); }    
    UInt GetVScrollOffsetValueInTwips() const { return LineBuffer.GetVScrollOffsetInTwips(); }    

    // returns true, if at least one of the paragraphs contains non-left alignment.
    bool ContainsNonLeftAlignment() const;
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
    GFxTextDocView(GFxFontManager* pfontMgr, GFxLog* plog);
    ~GFxTextDocView();

    UPInt GetLength() const { return pDocument->GetLength(); }
    GFxString GetText() const;
    GFxString GetHtml() const;

    GFxStyledText* GetStyledText() const { return pDocument; }
    GFxFontManager* GetFontManager() const { return pFontManager; }

    void SetText(const char* putf8String, UPInt stringSize = GFC_MAX_UPINT);
    void SetText(const wchar_t* pstring, UPInt length = GFC_MAX_UPINT);

    void ParseHtml(const GFxString& str, bool condenseWhite, 
        GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr = NULL, const GFxTextStyleManager* pstyleMgr = NULL)
    {
        ParseHtml(str.ToCStr(), str.GetLength(), condenseWhite, pimgInfoArr, pstyleMgr);
    }
    void ParseHtml(const char* putf8Str, UPInt utf8Len, bool condenseWhite, 
        GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr = NULL, const GFxTextStyleManager* pstyleMgr = NULL);
    void ParseHtml(const char* putf8Str, UPInt utf8Len = GFC_MAX_UPINT, 
        GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr = NULL, const GFxTextStyleManager* pstyleMgr = NULL)
    {
        ParseHtml(putf8Str, utf8Len, false, pimgInfoArr, pstyleMgr);
    }
    void ParseHtml(const wchar_t* pwStr, UPInt strLen = GFC_MAX_UPINT, bool condenseWhite = false, 
        GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr = NULL, const GFxTextStyleManager* pstyleMgr = NULL);

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
    void  RemoveText(UPInt startPos, UPInt endPos)
    {
        UPInt len = (endPos >= startPos) ? endPos - startPos : 0;
        pDocument->Remove(startPos, len);
    }

    // insert styled text
    UPInt InsertStyledText(const GFxStyledText& text, UPInt startPos, UPInt strLen = GFC_MAX_UPINT)
    {
        return pDocument->InsertStyledText(text, startPos, strLen);
    }

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

    void Format();

    void PreDisplay(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx, bool focused);
    void Display(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx, bool nextFrame);
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

    UInt GetHScrollOffset();
    UInt GetVScrollOffset();

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

    bool IsUrlAtPoint(Float x, Float y, GRange* purlPosRange = NULL);

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
    GFxTextEditorKit* GetEditorKit()             { return pEditorKit; };
    const GFxTextEditorKit* GetEditorKit() const { return pEditorKit; };
    bool HasEditorKit() const                    { return (pEditorKit.GetPtr() != NULL); }

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

#ifdef GFC_BUILD_DEBUG
    void Dump() { ForceReformat(); LineBuffer.Dump(); }
#endif
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
        DocumentText(GFxTextDocView* pdoc, GFxTextAllocator* pallocator) : 
          GFxStyledText(pallocator), pDocument(pdoc) {}
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
    GFxTextFilter                   Filter;
    GPtr<GFxLog>                    pLog;
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
        RTFlags_CompressCRLF        = 0x8,
        RTFlags_FontErrorDetected   = 0x10
    };
    UInt8                           RTFlags; // run-time flags
};

#endif //INC_TEXT_GFXTEXTDOCVIEW_H

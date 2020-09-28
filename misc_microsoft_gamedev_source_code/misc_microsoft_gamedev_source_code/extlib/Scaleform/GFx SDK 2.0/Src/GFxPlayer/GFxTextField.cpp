/**********************************************************************

Filename    :   GFxTextField.cpp
Content     :   Dynamic and input text field character implementation
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
#include "GFxTextFormat.h"
#include "GFxAsBroadcaster.h"

#include "GFxNumber.h"
#include "GFxRectangle.h"
#include "GFxMouse.h"

// For Root()->GetRenderer
#include "GFxPlayerImpl.h"
#include "GFxSprite.h"

#include "GFxDisplayContext.h"
#include "GFxLoadProcess.h"

// For image substitution
#include "GFxBitmapData.h"
#include "GFxShape.h"
#include "GFxArray.h"


//
// GFxEditTextCharacterDef
//
GFxEditTextCharacterDef::GFxEditTextCharacterDef(GFxMovieDataDef* RootDef)
:
    RootDef(RootDef),
    FontId(0),
    TextHeight(1.0f),
    MaxLength(0),
    LeftMargin(0.0f),
    RightMargin(0.0f),
    Indent(0.0f),
    Leading(0.0f),
    Flags(0),
    Alignment(ALIGN_LEFT)
{
    GASSERT(RootDef);

    Color.SetColor(0, 0, 0, 255);
    TextRect.Clear();
}
GFxEditTextCharacterDef::~GFxEditTextCharacterDef()
{
}

void    GFxEditTextCharacterDef::Read(GFxLoadProcess* p, GFxTagType tagType)
{
    GUNUSED(tagType);        
    GASSERT(tagType == GFxTag_DefineEditText);
    GFxStream* in = p->GetStream();

    in->ReadRect(&TextRect);

    in->LogParse("  TextRect = { l: %f, t: %f, r: %f, b: %f }\n", 
        (float)TextRect.Left, (float)TextRect.Top, (float)TextRect.Right, (float)TextRect.Bottom);

    in->Align();
    bool    hasText = in->ReadUInt(1) ? true : false;
    SetWordWrap(in->ReadUInt(1) != 0);
    SetMultiline(in->ReadUInt(1) != 0);
    SetPassword(in->ReadUInt(1) != 0);
    SetReadOnly(in->ReadUInt(1) != 0);

    in->LogParse("  WordWrap = %d, Multiline = %d, Password = %d, ReadOnly = %d\n", 
        (int)IsWordWrap(), (int)IsMultiline(), (int)IsPassword(), (int)IsReadOnly());

    bool    hasColor = in->ReadUInt(1) ? true : false;
    bool    hasMaxLength = in->ReadUInt(1) ? true : false;
    bool    hasFont = in->ReadUInt(1) ? true : false;

    in->ReadUInt(1);    // reserved
    SetAutoSize(in->ReadUInt(1) != 0);
    bool    hasLayout = in->ReadUInt(1) ? true : false;
    SetSelectable(in->ReadUInt(1) == 0);
    SetBorder(in->ReadUInt(1) != 0);
    in->ReadUInt(1);    // reserved

    // In SWF 8 text is *ALWAYS* marked as HTML.
    // Correction, AB: no, only if kerning is ON
    // In SWF <= 7, that is not the case.
    SetHtml(in->ReadUInt(1) != 0);
    SetUseDeviceFont(in->ReadUInt(1) == 0);

    in->LogParse("  AutoSize = %d, Selectable = %d, Border = %d, Html = %d, UseDeviceFont = %d\n", 
        (int)IsAutoSize(), (int)IsSelectable(), (int)IsBorder(), (int)IsHtml(), (int)DoesUseDeviceFont());

    if (hasFont)
    {
        FontId = GFxResourceId(in->ReadU16());
        TextHeight = (Float) in->ReadU16();
        in->LogParse("  HasFont: font id = %d\n", FontId.GetIdIndex());

        // Create a font resource handle.
        GFxResourceHandle hres;
        p->GetResourceHandle(&hres, FontId);
        pFont.SetFromHandle(hres);
    }

    if (hasColor)
    {               
        in->ReadRgba(&Color);
        in->LogParse("  HasColor\n");
    }

    if (hasMaxLength)
    {
        MaxLength = in->ReadU16();
        in->LogParse("  HasMaxLength: len = %d\n", MaxLength);
    }

    if (hasLayout)
    {
        SetHasLayout();
        Alignment = (alignment) in->ReadU8();
        LeftMargin = (Float) in->ReadU16();
        RightMargin = (Float) in->ReadU16();
        Indent = (Float) in->ReadS16();
        Leading = (Float) in->ReadS16();
        in->LogParse("  HasLayout: alignment = %d, leftmarg = %f, rightmarg = %f, indent = %f, leading = %f\n", 
            (int)Alignment, LeftMargin, RightMargin, Indent, Leading);
    }

    char*   name = in->ReadString();
    VariableName = name;
    GFREE(name);

    if (hasText)
    {
        char*   str = in->ReadString();
        DefaultText = str;
        GFREE(str);
    }
    in->LogParse("EditTextChar, varname = %s, text = %s\n",
        VariableName.ToCStr(), DefaultText.ToCStr());
}

void GFxEditTextCharacterDef::InitEmptyTextDef()
{
    Color = GColor((UByte)0, (UByte)0, (UByte)0, (UByte)255);
    TextHeight = PixelsToTwips(12.f);
    SetReadOnly();
    SetSelectable();
    SetUseDeviceFont();
    SetAAForReadability();
    // font name?
}

//
// GFxEditTextCharacter
//


class GFxEditTextCharacter : public GASTextField
{
    GFxEditTextCharacterDef*    pDef;
    GPtr<GFxTextDocView>        pDocument;
    GFxResourceBinding*         pBinding; //?AB: is there any other way to get GFxFontResource*?

    enum
    {
        Flags_AutoSize      = 0x1,
        Flags_Html          = 0x2,
        Flags_Password      = 0x4,
        Flags_NoTranslate   = 0x8,
        Flags_CondenseWhite = 0x10,
        Flags_HandCursor    = 0x20,
        Flags_NextFrame     = 0x40,
        Flags_MouseWheelEnabled = 0x80
    };

    GFxEditTextCharacterDef::alignment  Alignment;
    GColor      BackgroundColor;
    GColor      BorderColor;
    int         MaxLength;

    GASString   VariableName;

    GASValue    VariableVal;
    UInt8       Flags;

    void SetAutoSize(bool v = true) { (v) ? Flags |= Flags_AutoSize : Flags &= (~Flags_AutoSize); }
    void ClearAutoSize()            { SetAutoSize(false); }
    bool IsAutoSize() const         { return (Flags & Flags_AutoSize) != 0; }

    void SetHtml(bool v = true) { (v) ? Flags |= Flags_Html : Flags &= (~Flags_Html); }
    void ClearHtml()            { SetHtml(false); }
    bool IsHtml() const         { return (Flags & Flags_Html) != 0; }

    void SetPassword(bool v = true) { (v) ? Flags |= Flags_Password : Flags &= (~Flags_Password); }
    void ClearPassword()            { SetPassword(false); }
    bool IsPassword() const         { return (Flags & Flags_Password) != 0; }

    void SetCondenseWhite(bool v = true) { (v) ? Flags |= Flags_CondenseWhite : Flags &= (~Flags_CondenseWhite); }
    void ClearCondenseWhite()            { SetCondenseWhite(false); }
    bool IsCondenseWhite() const         { return (Flags & Flags_CondenseWhite) != 0; }

    void SetHandCursor(bool v = true) { (v) ? Flags |= Flags_HandCursor : Flags &= (~Flags_HandCursor); }
    void ClearHandCursor()            { SetHandCursor(false); }
    bool IsHandCursor() const         { return (Flags & Flags_HandCursor) != 0; }

    void SetMouseWheelEnabled(bool v = true) { (v) ? Flags |= Flags_MouseWheelEnabled : Flags &= (~Flags_MouseWheelEnabled); }
    void ClearMouseWheelEnabled()            { SetMouseWheelEnabled(false); }
    bool IsMouseWheelEnabled() const         { return (Flags & Flags_MouseWheelEnabled) != 0; }

    void SetSelectable(bool v = true) 
    { 
        GASSERT(pDocument);
        GFxTextEditorKit* peditor = pDocument->GetEditorKit();
        if (v)
        {
            peditor = CreateEditorKit();
            peditor->SetSelectable();
        }
        else
        {
            if (peditor)
                peditor->ClearSelectable();
        }
    }
    void ClearSelectable()            { SetSelectable(false); }
    bool IsSelectable() const         { return (pDocument->GetEditorKit()) ? pDocument->GetEditorKit()->IsSelectable() : pDef->IsSelectable(); }

    void SetNoTranslate(bool v = true) { (v) ? Flags |= Flags_NoTranslate : Flags &= (~Flags_NoTranslate); }
    void ClearNoTranslate()            { SetNoTranslate(false); }
    bool IsNoTranslate() const         { return (Flags & Flags_NoTranslate) != 0; }

    bool IsReadOnly() const { return (pDocument->GetEditorKit()) ? pDocument->GetEditorKit()->IsReadOnly() : pDef->IsReadOnly(); }

    GFxTextEditorKit* CreateEditorKit()
    {
        GASSERT(pDocument);
        GFxTextEditorKit* peditor = pDocument->GetEditorKit();
        if (peditor == NULL)
        {
            peditor = pDocument->CreateEditorKit();
            if (pDef->IsReadOnly())
                peditor->SetReadOnly();
            if (pDef->IsSelectable())
                peditor->SetSelectable();
        }
        return peditor;
    }

    class TextDocumentListener : public GFxTextDocView::DocumentListener
    {
        GFxEditTextCharacter* GetTextField()
        {
            return (GFxEditTextCharacter*)(((UByte*)this) - 
                   ((UByte*)&(((GFxEditTextCharacter*)this)->TextDocListener) - 
                    (UByte*)((GFxEditTextCharacter*)this)) );
        }
    public:
        TextDocumentListener() {}

        void OnScroll(GFxTextDocView&, UInt newScroll)
        {
            GUNUSED(newScroll);
            GFxEditTextCharacter* ptextField = GetTextField();
            GASEnvironment* penv = ptextField->GetASEnvironment();
            penv->Push(GASValue(ptextField));
            GASAsBroadcaster::BroadcastMessage(penv, ptextField, penv->CreateConstString("onScroller"), 1, penv->GetTopIndex());
            penv->Drop1();
        }

        // events
        virtual void View_OnHScroll(GFxTextDocView& view, UInt newScroll)
        {
            OnScroll(view, newScroll);
        }
        virtual void View_OnVScroll(GFxTextDocView& view, UInt newScroll)
        {
            OnScroll(view, newScroll);
        }

        // editor events
        virtual wchar_t Editor_OnCharacter(GFxTextEditorKit& editor, wchar_t srcChar) 
        { 
            GFxEditTextCharacter* peditChar = GetTextField();
            if (peditChar->MaxLength && editor.GetDocument()->GetLength() + 1 > UInt(peditChar->MaxLength))
                return 0;
            return srcChar;
        }
        virtual void    Editor_OnChanged(GFxTextEditorKit& editor) 
        { 
            GUNUSED(editor); 
            GFxEditTextCharacter* peditChar = GetTextField();
            peditChar->NotifyChanged();
        }
    } TextDocListener;

    // Shadow structure - used to keep track of shadow color and offsets, allocated only
    // if those were assigned through the shadowStyle and shadowColor properties.
    struct ShadowParams : public GNewOverrideBase
    {
        GColor                  ShadowColor;
        GASString               ShadowStyleStr;
        GTL::garray<GPointF>    ShadowOffsets;
        GTL::garray<GPointF>    TextOffsets;        

        ShadowParams(GASStringContext* psc)
            : ShadowStyleStr(psc->GetBuiltin(GASBuiltin_empty_))
        {
            ShadowColor = GColor(0,0,0,255);
        }
    };

    ShadowParams             *pShadow;
    GPtr<GASTextFieldObject> ASTextFieldObj;
    GFxStringHash<GPtr<GFxTextImageDesc> >* pImageDescAssoc;

    void ProceedImageSubstitution(const GASFnCall& fn, int idx, const GASValue* pve);
public:

    static void SetTextFormat(const GASFnCall& fn);
    static void SetNewTextFormat(const GASFnCall& fn);
    static void GetTextFormat(const GASFnCall& fn);
    static void GetNewTextFormat(const GASFnCall& fn);
    static void ReplaceText(const GASFnCall& fn);
    static void RemoveTextField(const GASFnCall& fn);

    // extensions
    static void GetCharBoundaries(const GASFnCall& fn);
    static void GetCharIndexAtPoint(const GASFnCall& fn);
    static void GetExactCharBoundaries(const GASFnCall& fn);
    static void GetLineIndexAtPoint(const GASFnCall& fn);
    static void GetLineIndexOfChar(const GASFnCall& fn);
    static void GetLineOffset(const GASFnCall& fn);
    static void GetLineLength(const GASFnCall& fn);
    static void GetLineMetrics(const GASFnCall& fn);
    static void GetLineText(const GASFnCall& fn);
    static void GetFirstCharInParagraph(const GASFnCall& fn);
    static void SetImageSubstitutions(const GASFnCall& fn);
    static void UpdateImageSubstitution(const GASFnCall& fn);
    static void AppendText(const GASFnCall& fn);
    static void AppendHtml(const GASFnCall& fn);

    GFxEditTextCharacter(GFxEditTextCharacterDef* def, GFxMovieDefImpl *pbindingDefImpl, 
                         GFxASCharacter* parent, GFxResourceId id)
        :
        GASTextField(pbindingDefImpl, parent, id),
        pDef(def),
        VariableName(parent->GetASEnvironment()->CreateString(def->VariableName))
    {
        GASSERT(parent);
        GASSERT(pDef);
        pImageDescAssoc = NULL;
        pBinding = &pbindingDefImpl->ResourceBinding;

        Flags = 0;

        //TextColor   = def->Color;
        Alignment   = def->Alignment;
        MaxLength   = def->MaxLength;
        SetPassword(def->IsPassword());
        SetHtml(def->IsHtml());
        SetMouseWheelEnabled(true);
        pShadow     = 0;

        BackgroundColor = GColor(255, 255, 255, 0);
        BorderColor     = GColor(0,0,0,0);
        if (def->IsBorder())
        {
            BackgroundColor.SetAlpha(255);
            BorderColor.SetAlpha(255);
        }

        // initialize built-in members by default values
        ASTextFieldObj = *new GASTextFieldObject(GetGC(), this);
        // let the base class know what's going on
        pProto = ASTextFieldObj->Get__proto__();

        // add itself as a listener, the Flash behavior
        GASAsBroadcaster::InitializeInstance(parent->GetASEnvironment()->GetSC(), this);
        GASAsBroadcaster::AddListener(parent->GetASEnvironment(), this, this);
        
        pDocument = *new GFxTextDocView(parent->GetFontManager());
        pDocument->SetDocumentListener(&TextDocListener);
        pDocument->GetStyledText()->SetNewLine0D(); // Flash uses '\x0D' ('\r', #13) as a new-line char
        
        SetInitialFormatsAsDefault();

        pDocument->SetViewRect(def->TextRect);
        if (def->DoesUseDeviceFont())
        {
            pDocument->SetUseDeviceFont();
            pDocument->SetAAForReadability();
        }
        if (def->IsMultiline())
            pDocument->SetMultiline();
        else
            pDocument->ClearMultiline();
        const bool  autoSizeByX = (def->IsAutoSize() && !(def->IsWordWrap() && def->IsMultiline()));
        const bool  autoSizeByY = (def->IsAutoSize() /*&& Multiline*/);
        if (autoSizeByX)
            pDocument->SetAutoSizeX();
        if (autoSizeByY)
            pDocument->SetAutoSizeY();
        if (def->IsWordWrap())
            pDocument->SetWordWrap();
        if (IsPassword())
            pDocument->SetPasswordMode();
        if (pDef->IsAAForReadability())
            pDocument->SetAAForReadability();
        if (!IsReadOnly() || pDef->IsSelectable())
        {
            CreateEditorKit();
        }

        if (pDef->DefaultText.GetLength() > 0)
        {
            bool varExists = false;
            // check, does variable exist or not. If it doesn't exist, we
            // will need to call UpdateVariable in order to create this variable.
            if (!VariableName.IsEmpty())
            {
                GASEnvironment* penv = GetASEnvironment();
                if (penv)
                {
                    GASValue val;
                    varExists = penv->GetVariable(VariableName, &val);
                }
            }
            if (!varExists)
            {
                SetTextValue(pDef->DefaultText.ToCStr(), IsHtml());
                UpdateVariable();
            }
        }
        else
        {
            // This assigns both Text and HtmlText vars.
            SetTextValue("", IsHtml());
        }

        //DummyStyle.push_back(GFxFillStyle());

        // update text, if variable is used
        UpdateTextFromVariable();

        pDocument->Format(GetLog());
    }

    ~GFxEditTextCharacter()
    {
        ClearIdImageDescAssoc();
        pDocument->Close();
        if (pShadow)
            delete pShadow;
    }


    virtual void SetFilters(const GFxTextFilter& f)
    { 
        if (pDocument)
            pDocument->SetFilters(f);
    }

    virtual GFxCharacterDef* GetCharacterDef() const        { return pDef; }

    void GetInitialFormats(GFxTextFormat* ptextFmt, GFxTextParagraphFormat* pparaFmt)
    {
        ptextFmt->InitByDefaultValues();
        pparaFmt->InitByDefaultValues();
        if (pDef->FontId.GetIdIndex())
        {
            // resolve font name, if possible
            GFxResourceBindData fontData = pBinding->GetResourceData(pDef->pFont);
            GFxFontResource* pfont = (GFxFontResource*)fontData.pResource.GetPtr();
            if (pfont)
            {
                ptextFmt->SetFontName(pfont->GetName());
                ptextFmt->SetBold(pfont->IsBold());
                ptextFmt->SetItalic(pfont->IsItalic());
                if (!pDef->DoesUseDeviceFont())
                {
                    GPtr<GFxFontHandle> pfontHandle = *new GFxFontHandle(NULL, pfont);
                    ptextFmt->SetFontHandle(pfontHandle);
                }
            }
        }
        ptextFmt->SetFontSize((UInt)TwipsToPixels(pDef->TextHeight));
        ptextFmt->SetColor(pDef->Color.ToColor32());

        GFxTextParagraphFormat defaultParagraphFmt;
        switch (pDef->Alignment)
        {
        case GFxEditTextCharacterDef::ALIGN_LEFT: 
            pparaFmt->SetAlignment(GFxTextParagraphFormat::Align_Left);
            break;
        case GFxEditTextCharacterDef::ALIGN_RIGHT: 
            pparaFmt->SetAlignment(GFxTextParagraphFormat::Align_Right);
            break;
        case GFxEditTextCharacterDef::ALIGN_CENTER: 
            pparaFmt->SetAlignment(GFxTextParagraphFormat::Align_Center);
            break;
        case GFxEditTextCharacterDef::ALIGN_JUSTIFY: 
            pparaFmt->SetAlignment(GFxTextParagraphFormat::Align_Justify);
            break;
        }
        if (pDef->HasLayout())
        {
            pparaFmt->SetLeftMargin((UInt)TwipsToPixels(pDef->LeftMargin));
            pparaFmt->SetRightMargin((UInt)TwipsToPixels(pDef->RightMargin));
            pparaFmt->SetIndent((SInt)TwipsToPixels(pDef->Indent));
            pparaFmt->SetLeading((SInt)TwipsToPixels(pDef->Leading));
        }
    }

    void SetInitialFormatsAsDefault()
    {
        GFxTextFormat defaultTextFmt;
        GFxTextParagraphFormat defaultParagraphFmt;
        GetInitialFormats(&defaultTextFmt, &defaultParagraphFmt);
        pDocument->SetDefaultTextFormat(defaultTextFmt);
        pDocument->SetDefaultParagraphFormat(defaultParagraphFmt);
    }

    bool UpdateTextFromVariable() 
    {
        if (!VariableName.IsEmpty()) 
        {   
            // Variable name can be a path, so use GetVariable().
            GASEnvironment* penv = GetASEnvironment();
            if (penv)
            {
                GASValue val;
                if (penv->GetVariable(VariableName, &val)) 
                {        
                    if (!val.IsEqual(penv, VariableVal))
                    {
                        VariableVal = val;
                        GASString str = val.ToString(penv);
                        SetTextValue(str.ToCStr(), false);
                        return true;
                    }
                }
                else
                {
                    // if variable name is specified but no variable found (were deleted, for example)
                    // then just set empty string
                    SetTextValue("", false);
                }
            }
            return false; // can't update the text for some reasons
        }
        return true; // do not need update anything, so, status of operation is OK.
    }

    
    // Override AdvanceFrame so that variable dependencies can be updated.
    virtual void    AdvanceFrame(bool nextFrame, Float framePos)
    {
        GUNUSED(framePos);
        if (nextFrame)
        {
            UpdateTextFromVariable();
            Flags |= Flags_NextFrame;
        }
        else
        {
            Flags &= ~Flags_NextFrame;
        }
        
        if (pDocument->HasEditorKit())
        {
            GFxMovieRoot* proot = GetMovieRoot();
            if (proot->IsFocused(this))
            {
                Float timer = proot->GetTimer();
                pDocument->GetEditorKit()->Advance(timer);
            }
        }
    }

    void ProcessImageTags(GFxStyledText::HTMLImageTagInfoArray& imageInfoArray);

    // Set our text to the given string. pnewText can have HTML or
    // regular formatting based on argument.
    void    SetTextValue(const char* pnewText, bool html)
    {
        bool doNotifyChanged = false;
        bool translated = false;
        if (!IsNoTranslate())
        {
            GPtr<GFxTranslator> ptrans = GetMovieRoot()->GetTranslator();
            if (ptrans)
            {
                GFxWStringBuffer::Reserve<512> res1, res2;
                GFxWStringBuffer wbuf(res1);
                GFxWStringBuffer wkeybuf(res2);

                if (!html || ptrans->CanReceiveHtml())
                {
                    // convert html or plain text to unicode w/o parsing
                    int nchars = (int)GUTF8Util::GetLength(pnewText);
                    wkeybuf.Resize(nchars + 1);
                    GUTF8Util::DecodeString(wkeybuf.GetBuffer(), pnewText);
                    translated = ptrans->Translate(&wbuf, wkeybuf.ToWStr());
                }
                else
                {
                    // if html, but need to pass plain text
                    SetInitialFormatsAsDefault();
                    pDocument->ParseHtml(pnewText, GFC_MAX_UPINT, IsCondenseWhite());
                    GFxWideStringBuffer wstrippedHtmlBuf;
                    pDocument->GetStyledText()->GetText(&wstrippedHtmlBuf);
                    if (ptrans->NeedStripNewLines())
                        wstrippedHtmlBuf.StripTrailingNewLines();
                    translated = ptrans->Translate(&wbuf, wstrippedHtmlBuf.ToWStr());
                }
                if (translated)
                {
                    if (ptrans->CanReturnHtml())
                    {
                        SetInitialFormatsAsDefault();
                        pDocument->ParseHtml(wbuf.ToWStr(), GFC_MAX_UPINT, IsCondenseWhite());
                    }
                    else
                    {
                        pDocument->SetText(wbuf.ToWStr());
                    }
                    doNotifyChanged = true;
                }
            }
        }

        if (!translated)
        {
            if (html)
            {
                SetInitialFormatsAsDefault();
                GFxStyledText::HTMLImageTagInfoArray imageInfoArray;
                pDocument->ParseHtml(pnewText, GFC_MAX_UPINT, IsCondenseWhite(), &imageInfoArray);
                if (imageInfoArray.size() > 0)
                {
                    ProcessImageTags(imageInfoArray);
                }
            }
            else
            {
                pDocument->SetText(pnewText);        
            }
        }
        if (pDocument->HasEditorKit())
        {
            if (!IsReadOnly()/* || IsSelectable()*/)
            {
                if (!pDocument->IsMultiline())
                    pDocument->GetEditorKit()->SetCursorPos(pDocument->GetLength());
                else
                    pDocument->GetEditorKit()->SetCursorPos(0);
            }
        }
        
        //!AB MaxLength is available only for InputText; though, if
        // you set text by variable the maxlength is ignored.
        if (doNotifyChanged)
            NotifyChanged();
    }

    // For ActionScript toString.
    virtual const char* GetTextValue(GASEnvironment* =0) const
    {       
        return "";//NULL;//pDocument->GetText()
    }

    virtual ObjectType  GetObjectType() const
    {
        return Object_TextField;
    }

    // GFxASCharacter override to indicate which standard members are handled for us.
    virtual UInt32  GetStandardMemberBitMask() const
    {       
        return  M_BitMask_PhysicalMembers |
                M_BitMask_CommonMembers |                               
                (1 << M_target) |
                (1 << M_url) |
                (1 << M_filters) |          
                (1 << M_tabEnabled) |
                (1 << M_tabIndex) |         
                (1 << M_quality) |
                (1 << M_highquality) |
                (1 << M_soundbuftime) |
                (1 << M_xmouse) |
                (1 << M_ymouse)
                ;
        // MA Verified: _lockroot does not exist/carry over from text fields, so don't include it.
        // If a movie is loaded into TextField, local _lockroot state is lost.
    }
    
    GASObject*  GetASObject () { return ASTextFieldObj; }
    GASObject*  GetASObject () const { return ASTextFieldObj; }

    bool    SetStandardMember(StandardMember member, const GASValue& val, bool opcodeFlag)
    {
        // _width and _height assignment work differently from movieclip:
        // it just changes the textrect.
        switch(member)
        {
        case M_x:
            {
                Double newX = PixelsToTwips(val.ToNumber(GetASEnvironment()));
                GeomDataType geomData;
                newX = newX - pDocument->GetViewRect().Left;
                GASValue val;
                val.SetNumber(TwipsToPixels(newX));

                if (GASTextField::SetStandardMember(member, val, 0))
                    return true;
                return false;
            }
        case M_y:
            {
                Double newY = PixelsToTwips(val.ToNumber(GetASEnvironment()));
                GeomDataType geomData;
                newY = newY - pDocument->GetViewRect().Top;
                GASValue val;
                val.SetNumber(TwipsToPixels(newY));

                if (GASTextField::SetStandardMember(member, val, 0))
                    return true;
                return false;
            }
        case M_width:
            {
                Float newWidth = Float(PixelsToTwips(val.ToNumber(GetASEnvironment())));
                GRectF viewRect = pDocument->GetViewRect();
                viewRect.Right = viewRect.Left + newWidth;
                pDocument->SetViewRect(viewRect);
                
                return true;
            }
        case M_height:
            {   
                Float newHeight = Float(PixelsToTwips(val.ToNumber(GetASEnvironment())));
                GRectF viewRect = pDocument->GetViewRect();
                viewRect.Bottom = viewRect.Top + newHeight;
                pDocument->SetViewRect(viewRect);

                return true;
            }
        default:
            break;
        }

        if (GASTextField::SetStandardMember(member, val, opcodeFlag))
            return true;
        return false;
    }

    bool    SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags())
    {
        StandardMember member = GetStandardMemberConstant(name);
        
        if (SetStandardMember(member, val, false))
            return true;

        // Handle TextField-specific "standard" members.
        switch(member)
        {
            case M_html:
                SetHtml(val.ToBool(penv));
                return true; 
                
            case M_htmlText:
                {
                    GASString str(val.ToStringVersioned(penv, GetVersion()));
                    SetTextValue(str.ToCStr(), true);
                    return true;
                }

            case M_text:
                {
                    ResetBlink(1);
                    GASString str(val.ToStringVersioned(penv, GetVersion()));
                    SetTextValue(str.ToCStr(), false);
                    return true;
                }

            case M_textColor:
                {
                    // The arg is 0xRRGGBB format.
                    UInt32  rgb = (UInt32) val.ToNumber(penv);
                    
                    GFxTextFormat colorTextFmt;
                    colorTextFmt.SetColor(rgb);

                    pDocument->SetTextFormat(colorTextFmt);

                    colorTextFmt = *pDocument->GetDefaultTextFormat();
                    colorTextFmt.SetColor(rgb);
                    pDocument->SetDefaultTextFormat(colorTextFmt);
                    return true;
                }

            case M_autoSize:
                {
                    GASString asStr(val.ToString(penv));
                    GFxString str = (val.IsBoolean()) ? 
                        ((val.ToBool(penv)) ? "left":"none") : asStr.ToCStr();

                    GFxTextDocView::ViewAlignment oldAlignment = pDocument->GetAlignment();
                    bool oldAutoSize = IsAutoSize();
                    if (str == "none")
                    {
                        ClearAutoSize();
                        pDocument->SetAlignment(GFxTextDocView::Align_Left);
                    }
                    else 
                    {
                        SetAutoSize();
                        if (str == "left")
                            pDocument->SetAlignment(GFxTextDocView::Align_Left);
                        else if (str == "right")
                            pDocument->SetAlignment(GFxTextDocView::Align_Right);
                        else if (str == "center")
                            pDocument->SetAlignment(GFxTextDocView::Align_Center);
                    }
                    if ((oldAlignment != pDocument->GetAlignment()) || (oldAutoSize != IsAutoSize()))
                    {
                        const bool  autoSizeByX = (IsAutoSize() && !(pDocument->IsWordWrap() && pDocument->IsMultiline()));
                        const bool  autoSizeByY = (IsAutoSize() /*&& Multiline*/);
                        if (autoSizeByX)
                            pDocument->SetAutoSizeX();
                        if (autoSizeByY)
                            pDocument->SetAutoSizeY();
                    }
                    return true;
                }

            case M_wordWrap:
                {
                    bool wordWrap = val.ToBool(penv);
                    if (wordWrap)
                        pDocument->SetWordWrap();
                    else
                        pDocument->ClearWordWrap();
                    return true;
                }

            case M_multiline:
                {
                    bool oldML = pDocument->IsMultiline();
                    bool newMultiline = val.ToBool(penv);
                    if (oldML != newMultiline)
                    {
                        if (newMultiline)
                            pDocument->SetMultiline();
                        else
                            pDocument->ClearMultiline();
                    }
                    return true;
                }

            case M_border:
                {
                    bool bc = val.ToBool(penv);
                    if (bc)
                        BorderColor.SetAlpha(255);
                    else
                        BorderColor.SetAlpha(0);
                    return true;
                }

            case M_variable:
                {
                    //if (val.IsNull() || val.IsUndefined())
                    //    VariableName = "";
                    //!AB: even if we set variable name pointing to invalid or non-exisiting variable
                    // we need to save it and try to update text using it. If we can't get a value 
                    // (variable doesn't exist), then text shouldn't be changed.
                    {                        
                        VariableName = val.ToString(penv);
                        UpdateTextFromVariable();
                    }
                    return true;
                }

            case M_selectable:
                {
                    SetSelectable(val.ToBool(penv));
                    return true;
                }

            case M_embedFonts:
                {
                    if (val.ToBool(penv))
                        pDocument->ClearUseDeviceFont();
                    else
                        pDocument->SetUseDeviceFont();
                    return true;
                }

            case M_password:
                {
                    bool pswd = IsPassword();
                    bool password = val.ToBool(penv);
                    if (pswd != password)
                    {
                        SetPassword(password);
                        if (password)
                            pDocument->SetPasswordMode();
                        else
                            pDocument->ClearPasswordMode();
                        pDocument->SetCompleteReformatReq();
                    }
                    return true;
                }

            case M_hscroll:
                {
                    SInt scrollVal = (SInt)val.ToNumber(penv);
                    if (scrollVal < 0)
                        scrollVal = 0;
                    pDocument->SetHScrollOffset((UInt)PixelsToTwips(scrollVal));    
                    return true;
                }

            case M_scroll:
                {
                    SInt scrollVal = (SInt)val.ToNumber(penv);
                    if (scrollVal < 1)
                        scrollVal = 1;
                    pDocument->SetVScrollOffset((UInt)(scrollVal - 1));    
                    return true;
                }
            case M_background:
                {
                    bool bc = val.ToBool(penv);
                    if (bc)
                        BackgroundColor.SetAlpha(255);
                    else
                        BackgroundColor.SetAlpha(0);
                    return true;
                }
            case M_backgroundColor:
                {
                    GASNumber bc = val.ToNumber(penv);
                    if (!GASNumberUtil::IsNaN(bc))
                    {
                        UInt32 c = UInt32(bc);
                        BackgroundColor = (BackgroundColor & 0xFF000000u) | (c & 0xFFFFFFu);
                    }
                    return true;
                }
            case M_borderColor:
                {
                    GASNumber bc = val.ToNumber(penv);
                    if (!GASNumberUtil::IsNaN(bc))
                    {
                        UInt32 c = UInt32(bc);
                        BorderColor = (BorderColor & 0xFF000000u) | (c & 0xFFFFFFu);
                    }
                    return true;
                }
            case M_type:
                {
                    GASString typeStr = val.ToString(penv);
                    if (penv->GetSC()->CompareConstString_CaseCheck(typeStr, "dynamic"))
                    {
                        if (pDocument->HasEditorKit())
                        {
                            pDocument->GetEditorKit()->SetReadOnly();
                        }
                    }
                    else if (penv->GetSC()->CompareConstString_CaseCheck(typeStr, "input"))
                    {
                        GFxTextEditorKit* peditor = CreateEditorKit();
                        peditor->ClearReadOnly();
                    }
                    return true;
                }
            case M_maxChars:
                {
                    GASNumber ml = val.ToNumber(penv);
                    if (!GASNumberUtil::IsNaN(ml))
                    {
                        MaxLength = (int)ml;
                    }
                    return true;
                }
            case M_condenseWhite:
                {
                    SetCondenseWhite(val.ToBool(penv));
                    return true;
                }
            case M_antiAliasType:
                {
                    GASString typeStr = val.ToString(penv);
                    if (penv->GetSC()->CompareConstString_CaseCheck(typeStr, "normal"))
                    {
                        pDocument->ClearAAForReadability();
                    }
                    else if (penv->GetSC()->CompareConstString_CaseCheck(typeStr, "advanced"))
                    {
                        pDocument->SetAAForReadability();                    
                    }
                    return true;
                }

            case M_mouseWheelEnabled:
                {
                    SetMouseWheelEnabled(val.ToBool(penv));
                    return true;
                }

            // extension
            case M_shadowStyle:
                if (!penv->CheckExtensions())
                    break;

                {
                    GASString   styleStr = val.ToString(penv);
                    const char *pstr = styleStr.ToCStr();

                    if (!pShadow)
                        if ((pShadow = new ShadowParams(penv->GetSC())) == 0)
                            return true;

                    // The arg is 0xRRGGBB format.
                    UInt32  rgb = pDocument->GetShadowColor();
                    pShadow->ShadowColor.SetBlue((UByte)rgb);
                    pShadow->ShadowColor.SetGreen((UByte)(rgb >> 8));
                    pShadow->ShadowColor.SetRed((UByte)(rgb >> 16));

                    // When using the old fasioned shadow we have to disable the new one.
                    pDocument->DisableSoftShadow();

                reset:
                    pShadow->ShadowOffsets.clear();
                    pShadow->TextOffsets.clear();

                    // Parse shadow style text to generate the offset arrays.
                    // Format is similar to "s{1,0.5}{-1,0.5}t{0,0}".
                    const char *p = pstr;
                    GTL::garray<GPointF> *offsets = 0;
                    while (*p)
                    {
                        if (*p == 's' || *p == 'S')
                        {
                            offsets = &pShadow->ShadowOffsets;
                            p++;
                        }
                        else if (*p == 't' || *p == 'T')
                        {
                            offsets = &pShadow->TextOffsets;
                            p++;
                        }
                        else if (*p == '{' && offsets) // {x,y}
                        {
                            char pn[24];
                            p++;
                            const char *q = p;
                            while (*q && *q != ',') q++;
                            if (!*q || q-p > 23) { pstr = pShadow->ShadowStyleStr.ToCStr(); goto reset; }
                            memcpy(pn,p,q-p);
                            pn[q-p] = 0;
                            Float x = (Float)((GASValue(penv->CreateString(pn))).ToNumber(penv) * 20.0);
                            q++;
                            p = q;

                            while (*q && *q != '}') q++;
                            if (!*q || q-p > 23) { pstr = pShadow->ShadowStyleStr.ToCStr(); goto reset; }
                            memcpy(pn,p,q-p);
                            pn[q-p] = 0;
                            Float y = (Float)((GASValue(penv->CreateString(pn))).ToNumber(penv) * 20.0);
                            q++;
                            p = q;

                            offsets->push_back(GPointF(x,y));
                        }
                        else
                        {
                            pstr = pShadow->ShadowStyleStr.ToCStr();
                            goto reset;
                        }
                    }

                    if (*pstr)
                        pShadow->ShadowStyleStr = penv->CreateString(pstr);
                    return true;
                }

            case M_shadowColor:
                if (penv->CheckExtensions())
                {
                    pDocument->SetShadowColor((UInt32)val.ToNumber(penv));
                    if (pShadow)
                    {
                        // The arg is 0xRRGGBB format.
                        UInt32  rgb = pDocument->GetShadowColor();
                        pShadow->ShadowColor.SetBlue((UByte)rgb);
                        pShadow->ShadowColor.SetGreen((UByte)(rgb >> 8));
                        pShadow->ShadowColor.SetRed((UByte)(rgb >> 16));
                    }
                    return true;
                }
                break;

            case M_hitTestDisable:
                if (penv->CheckExtensions())
                {
                    HitTestDisable = val.ToBool(GetASEnvironment());
                    return 1;
                }
                break;

            case M_noTranslate:
                if (penv->CheckExtensions())
                {
                    SetNoTranslate(val.ToBool(GetASEnvironment()));
                    pDocument->SetCompleteReformatReq(); 
                    return 1;
                }
                break;

            case M_autoFit:
                if (penv->CheckExtensions())
                {
                    if (val.ToBool(penv))
                        pDocument->SetAutoFit();
                    else
                        pDocument->ClearAutoFit();
                    return true;
                }
                break;

            case M_blurX:
                if (penv->CheckExtensions())
                {
                    pDocument->SetBlurX((Float)val.ToNumber(penv));
                    return true;
                }
                break;

            case M_blurY:
                if (penv->CheckExtensions())
                {
                    pDocument->SetBlurY((Float)val.ToNumber(penv));
                    return true;
                }
                break;

            case M_blurStrength:
                if (penv->CheckExtensions())
                {
                    pDocument->SetBlurStrength((Float)val.ToNumber(penv));
                    return true;
                }
                break;

            case M_outline:
                // Not implemented
                //if (GetASEnvironment()->CheckExtensions())
                //{
                //    return true;
                //}
                break;

            case M_shadowAlpha:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowAlpha((Float)val.ToNumber(penv));
                    return true;
                }
                break;

            case M_shadowAngle:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowAngle((Float)val.ToNumber(penv));
                    return true;
                }
                break;

            case M_shadowBlurX:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowBlurX((Float)val.ToNumber(penv));
                    return true;
                }
                break;

            case M_shadowBlurY:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowBlurY((Float)val.ToNumber(penv));
                    return true;
                }
                break;

            case M_shadowDistance:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowDistance((Float)val.ToNumber(penv));
                    return true;
                }
                break;

            case M_shadowHideObject:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetHideObject(val.ToBool(penv));
                    return true;
                }
                break;

            case M_shadowKnockOut:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetKnockOut(val.ToBool(penv));
                    return true;
                }
                break;

            case M_shadowQuality:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowQuality((UInt)val.ToNumber(penv));
                    return true;
                }
                break;

            case M_shadowStrength:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowStrength((Float)val.ToNumber(penv));
                    return true;
                }
                break;

            case M_shadowOutline:
                // Not implemented
                //if (GetASEnvironment()->CheckExtensions())
                //{
                //    return true;
                //}
                break;

            default:
                break;
        }
        if (ASTextFieldObj)
        {
            return ASTextFieldObj->SetMember(penv, name, val, flags);
        }
        return false;
    }


    bool    GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval)
    {
        StandardMember member = GetStandardMemberConstant(name);
        
        // Handle TextField-specific "standard" members.
        // We put this before GetStandardMember so that we can handle _width
        // and _height in a custom way.
        switch(member)
        {
            case M_x:
                {
                    GeomDataType geomData;
                    pval->SetNumber(TwipsToPixels(Double(GetGeomData(geomData).X)) + TwipsToPixels(pDocument->GetViewRect().Left));
                    return true;
                }

            case M_y:
                {
                    GeomDataType geomData;
                    pval->SetNumber(TwipsToPixels(Double(GetGeomData(geomData).Y)) + TwipsToPixels(pDocument->GetViewRect().Top));
                    return true;
                }

            case M_html:
                {               
                    pval->SetBool(IsHtml());
                    return true;
                }

            case M_htmlText:
                {
                    // For non-HTML field, non-HTML text is returned.
                    if (IsHtml())
                        pval->SetString(penv->CreateString(pDocument->GetHtml()));
                    else
                        pval->SetString(penv->CreateString(pDocument->GetText()));
                    return true;
                }

            case M_text:
                {                   
                    pval->SetString(penv->CreateString(pDocument->GetText()));
                    return true;
                }

            case M_textColor:
                {
                    // Return color in 0xRRGGBB format
                    const GFxTextFormat* ptf = pDocument->GetDefaultTextFormat();
                    GASSERT(ptf);
                    GColor c = ptf->GetColor();
                    UInt textColor = (((UInt32)c.GetRed()) << 16) | (((UInt32)c.GetGreen()) << 8) | ((UInt32)c.GetBlue());
                    pval->SetInt(textColor);
                    return true;
                }

            case M_textWidth:           
                {
                    // Return the width, in pixels, of the text as laid out.
                    // (I.E. the actual text content, not our defined
                    // bounding box.)
                    //
                    // In local coords.  Verified against Macromedia Flash.
                    pval->SetNumber((GASNumber)int(TwipsToPixels(pDocument->GetTextWidth())));
                    return true;
                }

            case M_textHeight:              
                {
                    // Return the height, in pixels, of the text as laid out.
                    // (I.E. the actual text content, not our defined
                    // bounding box.)
                    //
                    // In local coords.  Verified against Macromedia Flash.
                    pval->SetNumber((GASNumber)int(TwipsToPixels(pDocument->GetTextHeight())));
                    return true;
                }

            case M_length:
                {
                    pval->SetNumber((GASNumber)pDocument->GetLength());
                    return true;
                }

            case M_autoSize:
                {
                    if (!IsAutoSize())
                        pval->SetString(penv->CreateConstString("none"));
                    else
                    {
                        switch(pDocument->GetAlignment())
                        {
                        case GFxTextDocView::Align_Left:
                                pval->SetString(penv->CreateConstString("left")); 
                                break;
                        case GFxTextDocView::Align_Right:
                                pval->SetString(penv->CreateConstString("right")); 
                                break;
                        case GFxTextDocView::Align_Center:
                                pval->SetString(penv->CreateConstString("center")); 
                                break;
                        default: pval->SetUndefined();
                        }
                    }
                    return true;
                }
            case M_wordWrap:
                {
                    pval->SetBool(pDocument->IsWordWrap());
                    return true;
                }
            case M_multiline:
                {
                    pval->SetBool(pDocument->IsMultiline());
                    return true;
                }
            case M_border:
                {
                    pval->SetBool(BorderColor.GetAlpha() != 0);
                    return true;
                }
            case M_variable:
                {
                    if (!VariableName.IsEmpty())
                        pval->SetString(VariableName);
                    else
                        pval->SetNull();
                    return true;
                }
            case M_selectable:
                {
                    pval->SetBool(IsSelectable());
                    return true;
                }
            case M_embedFonts:
                {
                    pval->SetBool(!pDocument->DoesUseDeviceFont());
                    return true;
                }
            case M_password:
                {
                    pval->SetBool(IsPassword());
                    return true;
                }

            case M_shadowStyle:
                {
                    if (!penv->CheckExtensions())
                        break;

                    pval->SetString(pShadow ? pShadow->ShadowStyleStr : penv->GetBuiltin(GASBuiltin_empty_));
                    return true;
                }
            case M_shadowColor:
                if (penv->CheckExtensions())
                {
                    pval->SetInt(pDocument->GetShadowColor());
                    return true;
                }
                break;

            case M_hscroll:
                {
                    pval->SetNumber((GASNumber)TwipsToPixels(pDocument->GetHScrollOffset()));    
                    return 1;
                }
            case M_scroll:
                {
                    pval->SetNumber((GASNumber)(pDocument->GetVScrollOffset() + 1));    
                    return 1;
                }
            case M_maxscroll:
                {
                    pval->SetNumber((GASNumber)(pDocument->GetMaxVScroll() + 1));    
                    return 1;
                }
            case M_maxhscroll:
                {
                    pval->SetNumber((GASNumber)(TwipsToPixels(pDocument->GetMaxHScroll())));    
                    return 1;
                }
            case M_background:
                {
                    pval->SetBool(BackgroundColor.GetAlpha() != 0);
                    return 1;
                }
            case M_backgroundColor:
                {
                    pval->SetNumber(GASNumber(BackgroundColor.ToColor32() & 0xFFFFFFu));
                    return 1;
                }
            case M_borderColor:
                {
                    pval->SetNumber(GASNumber(BorderColor.ToColor32() & 0xFFFFFFu));
                    return 1;
                }
            case M_bottomScroll:
                {
                    pval->SetNumber((GASNumber)(pDocument->GetBottomVScroll()));
                    return 1;
                }
            case M_type:
                {
                    pval->SetString(IsReadOnly() ? 
                        penv->CreateConstString("dynamic") : penv->CreateConstString("input")); 
                    return 1;
                }
            case M_maxChars:
                {
                    if (MaxLength <= 0)
                        pval->SetNull();
                    else
                        pval->SetNumber(GASNumber(MaxLength));
                    return 1;
                }
            case M_condenseWhite:
                {
                    pval->SetBool(IsCondenseWhite());
                    return 1;
                }

            case M_antiAliasType:
                {
                    pval->SetString(pDocument->IsAAForReadability() ? 
                        penv->CreateConstString("advanced") : penv->CreateConstString("normal"));
                    return 1;
                }
            case M_mouseWheelEnabled:
                {
                    pval->SetBool(IsMouseWheelEnabled());
                    return true;
                }

             // extension
            case M_hitTestDisable:
                if (GetASEnvironment()->CheckExtensions())
                {
                    pval->SetBool(HitTestDisable);
                    return 1;
                }
                break;

            case M_noTranslate:
                if (GetASEnvironment()->CheckExtensions())
                {
                    pval->SetBool(IsNoTranslate());
                    return 1;
                }
                break;

            case M_caretIndex:
                if (penv->CheckExtensions())
                {
                    if (pDocument->HasEditorKit())
                    {
                        pval->SetNumber(GASNumber(pDocument->GetEditorKit()->GetCursorPos()));
                        return 1;
                    }
                }
                break;
            case M_autoFit:
                if (penv->CheckExtensions())
                {
                    pval->SetBool(pDocument->IsAutoFit());
                    return true;
                }
                break;

            case M_blurX:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetBlurX()));
                    return true;
                }
                break;

            case M_blurY:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetBlurY()));
                    return true;
                }
                break;

            case M_blurStrength:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetBlurStrength()));
                    return true;
                }
                break;

            case M_outline:
                // Not implemented
                //if (penv->CheckExtensions())
                //{
                //    return true;
                //}
                break;

            case M_shadowAlpha:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowAlpha()));
                    return true;
                }
                break;

            case M_shadowAngle:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowAngle()));
                    return true;
                }
                break;

            case M_shadowBlurX:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowBlurX()));
                    return true;
                }
                break;

            case M_shadowBlurY:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowBlurY()));
                    return true;
                }
                break;

            case M_shadowDistance:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowDistance()));
                    return true;
                }
                break;

            case M_shadowHideObject:
                if (penv->CheckExtensions())
                {
                    pval->SetBool(pDocument->IsHiddenObject());
                    return true;
                }
                break;

            case M_shadowKnockOut:
                if (penv->CheckExtensions())
                {
                    pval->SetBool(pDocument->IsKnockOut());
                    return true;
                }
                break;

            case M_shadowQuality:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowQuality()));
                    return true;
                }
                break;

            case M_shadowStrength:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowStrength()));
                    return true;
                }
                break;

            case M_shadowOutline:
                // Not implemented
                //if (penv->CheckExtensions())
                //{
                //    return true;
                //}
                break;

            default: 
                if (GetStandardMember(member, pval, 0))
                    return true;
                break;
        }

        if (ASTextFieldObj)
        {
            return ASTextFieldObj->GetMember(penv, name, pval);
        }
        // looks like _global is accessable from any character
        if (name == "_global" && penv)
        {
            pval->SetAsObject(penv->GetGC()->pGlobal);
            return true;
        }
        return false;
    }

    // Return the topmost entity that the given point covers.  NULL if none.
    // I.E. check against ourself.
    virtual GFxASCharacter* GetTopMostMouseEntity(const GPointF &pt, bool testAll = false)
    {
        if (HitTestDisable || !GetVisible())
            return 0;

        GRenderer::Matrix   m = GetMatrix();
        GPointF             p;
        m.TransformByInverse(&p, pt);

        if ((ClipDepth == 0) && PointTestLocal(p, 1))
        {
            if (testAll || IsSelectable())
                return this;
            else if (!IsSelectable() && IsHtml() && 
                     pDocument->MayHaveUrl() && pDocument->IsUrlAtPoint(p.x, p.y))
            {
                // if not selectable, will need to check for url under the
                // mouse cursor. Return "this", if there is an url under 
                // the mouse cursor.
                return this;
            }
            else
            {
                GFxASCharacter* pparent = GetParent();
                while (pparent && pparent->IsSprite())
                {
                    // Parent sprite would have to be in button mode to grab attention.
                    GFxSprite * psprite = (GFxSprite*)pparent;
                    if (testAll || psprite->ActsAsButton())
                        return psprite;
                    pparent = pparent->GetParent ();
                }
            }
        }

        return NULL;
    }

    bool    IsTabable() const
    {
        return GetVisible() && !IsReadOnly() && !TabEnabled.IsFalse();
    }

    bool    IsFocusRectEnabled() const { return false; }

    bool    IsFocusEnabled() const { return !IsReadOnly(); }

    UInt    GetCursorType() const 
    {
        if (IsHandCursor())
            return GFxMouseCursorEvent::HAND;
        if (IsSelectable())
            return GFxMouseCursorEvent::IBEAM;
        return GFxASCharacter::GetCursorType();
    }

    bool IsUrlUnderMouseCursor(GPointF* pPnt = NULL)
    {
        // Local coord of mouse IN PIXELS.
        GFxMovieRoot* proot = GetMovieRoot();
        int x, y, buttons;          
        proot->GetMouseState(&x, &y, &buttons);

        Matrix  m = GetWorldMatrix();
        GPointF a(PixelsToTwips(Float(x)), PixelsToTwips(Float(y)));
        GPointF b;

        m.TransformByInverse(&b, a);
        if (pPnt) *pPnt = b;

        return pDocument->IsUrlAtPoint(b.x, b.y);
    }

    void    PropagateMouseEvent(const GFxEventId& id)
    {
        GFxMovieRoot* proot = GetMovieRoot();
        if (proot->MouseButtonState.TopmostEntity == this)
        {
            if (GetVisible() == false)  
                return;
            switch(id.Id)
            {
            case GFxEventId::Event_MouseMove:
                {
                    if (IsHtml() && pDocument->MayHaveUrl())
                    {
                        SetHandCursor(IsUrlUnderMouseCursor());
                        GASMouseCtorFunction::SetCursorType(proot, GetCursorType());
                    }
                    else if (IsHandCursor())
                    {
                        ClearHandCursor();
                        GASMouseCtorFunction::SetCursorType(proot, GetCursorType());
                    }
                }
                break;
            default: break;
            }
        }
        GFxASCharacter::PropagateMouseEvent(id);
    }

    // handle Event_MouseDown to grab focus
    virtual bool OnButtonEvent(const GFxEventId& event)
    { 
        if (event.Id == GFxEventId::Event_Press)
        {
            GetMovieRoot()->SetFocusTo(this);

            // if editor exists - re-translate event to it
            if (pDocument->HasEditorKit())
            {
                // Local coord of mouse IN PIXELS.
                int x, y, buttons;          
                GetMovieRoot()->GetMouseState(&x, &y, &buttons);

                Matrix  m = GetWorldMatrix();
                GPointF a(GFC_PIXELS_TO_TWIPS(x), GFC_PIXELS_TO_TWIPS(y));
                GPointF b;

                m.TransformByInverse(&b, a);
                pDocument->GetEditorKit()->OnMouseDown(b.x, b.y, buttons);
            }

            // if url is clicked - execute the action
            if (IsHtml() && pDocument->MayHaveUrl())
            {
                GPointF p;
                if (IsUrlUnderMouseCursor(&p))
                {
                    UPInt pos = pDocument->GetCharIndexAtPoint(p.x, p.y);
                    if (!GTL::IsMax(pos))
                    {
                        const GFxTextFormat* ptextFmt;
                        if (pDocument->GetTextAndParagraphFormat(&ptextFmt, NULL, pos))
                        {
                            if (ptextFmt->IsUrlSet())
                            {
                                const char* url = ptextFmt->GetUrl();
                                
                                // url should represent link in asfunction protocol:
                                // "asfunction:funcName, parametersString"
                                // NOTE: parametersString is not parsed.

                                if (GFxString::CompareNoCase(url, "asfunction:", sizeof("asfunction:") - 1) == 0)
                                {
                                    url += sizeof("asfunction:") - 1;
                                    // now get the function's name
                                    const char* pnameEnd = strchr(url, ',');
                                    GFxString funcName;
                                    GFxValue param;
                                    UInt paramCnt = 0;
                                    if (pnameEnd)
                                    {
                                        funcName.AppendString(url, pnameEnd - url);
                                        ++pnameEnd;
                                        paramCnt = 1;
                                        param.SetString(pnameEnd);
                                    }
                                    else
                                        funcName = url;
                                    GFxValue result;
                                    GetMovieRoot()->Invoke(funcName, &result, &param, paramCnt);
                                }
                                //printf ("%s\n", url);
                            }
                        }
                    }
                }
            }

            return true;
        }
        return false; 
    }

    void OnFocus(FocusEventType event, GFxASCharacter* oldOrNewFocusCh)
    {
        if (!IsReadOnly() || IsSelectable())
        {
            if (event == GFxASCharacter::SetFocus)
            {
                ResetBlink(1, 1);
            }

            GFxASCharacter::OnFocus(event, oldOrNewFocusCh);
        }
    }

    // invoked when focused item is about to lose mouse input (clicked on another item, for example)
    void OnLosingMouseInput() 
    {
        GetMovieRoot()->SetFocusTo(0);
    }

    void UpdateVariable()
    {
        if (!VariableName.IsEmpty())
        {
            GASEnvironment* penv = GetASEnvironment();
            if (penv)
            {
                VariableVal.SetString(penv->CreateString(pDocument->GetText()));
                penv->SetVariable(VariableName, VariableVal);
            }
        }
    }

    void NotifyChanged()
    {
        // if Variable is set - set new value into it
        if (!VariableName.IsEmpty())
            UpdateVariable();
        GASEnvironment* penv = GetASEnvironment();

        penv->Push(GASValue(this));
        GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onChanged"), 1, penv->GetTopIndex());
        penv->Drop1();
    }

    virtual GRectF  GetBounds(const Matrix &t) const
    {
        return t.EncloseTransform(pDocument->GetViewRect());
    }

    virtual bool    PointTestLocal(const GPointF &pt, bool testShape = 0) const
    {
        GUNUSED(testShape);
        if (HitTestDisable)
            return false;
        // Flash only uses the bounding box for text - even if shape flag is used
        return pDocument->GetViewRect().Contains(pt);
    }

    // Draw the dynamic string.
    void    Display(GFxDisplayContext &context, StackData stackData)   
    {        
        GRenderer* prenderer = context.GetRenderer();
        if (!prenderer)
            return;

        //UpdateTextFromVariable (); // this is necessary for text box with binded variables. (??)
          		
		// Pass Matrix on the stack
		GRenderer::Matrix mat = stackData.stackMatrix * this->Matrix_1;
		GRenderer::Cxform cx = stackData.stackColor * this->ColorTransform;		

        //GRenderer::Matrix   mat = GetWorldMatrix();         
        //GRenderer::Cxform   cx = GetWorldCxform();

        // Do viewport culling if bounds are available.
        if (!GetMovieRoot()->GetVisibleFrameRectInTwips().Intersects(mat.EncloseTransform(pDocument->GetViewRect())))
            if (!(context.GetRenderFlags() & GFxRenderConfig::RF_NoViewCull))
                return;
        // If Alpha is Zero, don't draw text.        
        if ((fabs(cx.M_[3][0]) < 0.001f) &&
            (fabs(cx.M_[3][1]) < 1.0f) )
            return;

        // Show white background + black bounding box.
        if (BorderColor.GetAlpha() > 0 || BackgroundColor.GetAlpha() > 0)
        {        
            static const UInt16   indices[6] = { 0, 1, 2, 2, 1, 3 };

            // Show white background + black bounding box.
            prenderer->SetCxform(cx);
            Matrix m(mat);
            GRectF newRect;
            GFx_RecalculateRectToFit16Bit(m, pDocument->GetViewRect(), &newRect);
            prenderer->SetMatrix(m);

            GPointF         coords[4];
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

            const SInt16  linecoords[10] = 
            {
                // outline
                (SInt16) coords[0].x, (SInt16) coords[0].y,
                (SInt16) coords[1].x, (SInt16) coords[1].y,
                (SInt16) coords[3].x, (SInt16) coords[3].y,
                (SInt16) coords[2].x, (SInt16) coords[2].y,
                (SInt16) coords[0].x, (SInt16) coords[0].y,
            };
            
            prenderer->FillStyleColor(BackgroundColor);
            prenderer->SetVertexData(icoords, 4, GRenderer::Vertex_XY16i);
            
            // Fill the inside
            prenderer->SetIndexData(indices, 6, GRenderer::Index_16);
            prenderer->DrawIndexedTriList(0, 0, 6, 0, 2);
            prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
            // And draw outline
            prenderer->SetVertexData(linecoords, 5, GRenderer::Vertex_XY16i);
            prenderer->LineStyleColor(BorderColor);
            prenderer->DrawLineStrip(0, 4);
            // Done
            prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
            prenderer->SetIndexData(0, 0, GRenderer::Index_None);
        }

        bool nextFrame = (Flags & Flags_NextFrame) != 0;
        Flags &= ~Flags_NextFrame;

        if (pShadow)
        {
            for (UInt i = 0; i < pShadow->ShadowOffsets.size(); i++)
            {
                GMatrix2D   shm(mat);
                GRenderer::Cxform c(cx);
                c.M_[0][0] = c.M_[1][0] = c.M_[2][0] = 0;
                c.M_[0][1] = (Float)(pShadow->ShadowColor.GetRed());   // R
                c.M_[1][1] = (Float)(pShadow->ShadowColor.GetGreen()); // G
                c.M_[2][1] = (Float)(pShadow->ShadowColor.GetBlue());  // B

                shm.PrependTranslation(pShadow->ShadowOffsets[i].x, pShadow->ShadowOffsets[i].y);
                pDocument->Display(context, shm, c, nextFrame, GetLog());
            }
            
            // Draw our actual text.
            if (pShadow->TextOffsets.size() == 0)
                pDocument->Display(context, mat, cx, nextFrame, GetLog());
            else
            {
                for (UInt j = 0; j < pShadow->TextOffsets.size(); j++)
                {
                    GMatrix2D   shm(mat);

                    shm.PrependTranslation(pShadow->TextOffsets[j].x, pShadow->TextOffsets[j].y);
                    pDocument->Display(context, shm, cx, nextFrame, GetLog());
                }
            }
        }
        else
        {
            pDocument->Display(context, mat, cx, nextFrame, GetLog());
        }
        if (pDocument->HasEditorKit() && GetMovieRoot()->IsFocused(this))
            pDocument->GetEditorKit()->Display(context, mat, cx);

        DoDisplayCallback();
    }


    void    ResetBlink(bool state = 1, bool blocked = 0)
    {
        if (pDocument->HasEditorKit())        
            pDocument->GetEditorKit()->ResetBlink(state, blocked);
    }

    void SetSelection(int beginIndex, int endIndex)
    {
        GUNUSED(beginIndex);
        // at the moment only cursor positioning works

        if (!IsReadOnly() || IsSelectable())
        {
            if (pDocument->HasEditorKit())
            {
                pDocument->GetEditorKit()->SetCursorPos(endIndex);
            }
        }
    }

    int GetCaretIndex()
    {
        if (!IsReadOnly() || IsSelectable())
        {
            if (pDocument->HasEditorKit())
            {
                return (int)pDocument->GetEditorKit()->GetCursorPos();
            }
        }
        return -1;
    }

    virtual bool    OnKeyEvent(const GFxEventId& id, int* pkeyMask)
    {
        GUNUSED(pkeyMask);

        if (!((*pkeyMask) & GFxCharacter::KeyMask_FocusedItemHandled))
        {
            if (GetMovieRoot()->IsFocused(this) && id.Id == GFxEventId::Event_KeyDown) 
            {
                // cursor positioning is available for selectable readonly textfields
                if (!IsReadOnly() || IsSelectable())
                {
                    if (pDocument->HasEditorKit())
                        pDocument->GetEditorKit()->OnKeyDown(id.KeyCode, id.SpecialKeysState);
                }
                (*pkeyMask) |= GFxCharacter::KeyMask_FocusedItemHandled;
                return true;
            }
        }
        return false;
    }

    virtual bool    OnCharEvent(UInt32 wcharCode)
    {
        // cursor positioning is available for selectable readonly textfields
        if (!IsReadOnly() || IsSelectable())
        {
            if (pDocument->HasEditorKit())
                pDocument->GetEditorKit()->OnChar(wcharCode);
        }
        return true;
    }

    // Special event handler; mouse wheel support
    bool            OnMouseWheelEvent(int mwDelta)
    { 
        GUNUSED(mwDelta); 
        if (IsMouseWheelEnabled() && IsSelectable())
        {
            SInt vscroll = (SInt)pDocument->GetVScrollOffset();
            vscroll -= mwDelta;
            if (vscroll < 0)
                vscroll = 0;
            if (vscroll > (SInt)pDocument->GetMaxVScroll())
                vscroll = (SInt)pDocument->GetMaxVScroll();
            pDocument->SetVScrollOffset((UInt)vscroll);
            return true;
        }
        return false; 
    }

    void AddIdImageDescAssoc(const char* idStr, GFxTextImageDesc* pdesc)
    {
        if (!pImageDescAssoc)
            pImageDescAssoc = new GFxStringHash<GPtr<GFxTextImageDesc> >;
        pImageDescAssoc->set(idStr, pdesc);
    }
    void RemoveIdImageDescAssoc(const char* idStr)
    {
        if (pImageDescAssoc)
        {
            pImageDescAssoc->remove(idStr);
        }
    }
    void ClearIdImageDescAssoc()
    {
        delete pImageDescAssoc;
        pImageDescAssoc = NULL;
    }
};


GFxCharacter*   GFxEditTextCharacterDef::CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId id,
                                                                 GFxMovieDefImpl *pbindingImpl)
{
    GFxEditTextCharacter*   ch = new GFxEditTextCharacter(this, pbindingImpl, parent, id);
    return ch;
}

// Read a DefineText tag.
void    GSTDCALL GFx_DefineEditTextLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT(tagInfo.TagType == GFxTag_DefineEditText);

    UInt16  characterId = p->ReadU16();

    GPtr<GFxEditTextCharacterDef>   pch = *new GFxEditTextCharacterDef(p->GetDataDef());
    p->LogParse("EditTextChar, id = %d\n", characterId);
    pch->Read(p, tagInfo.TagType);

    p->AddResource(GFxResourceId(characterId), pch);
}

void GFxEditTextCharacter::SetTextFormat(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs == 1)
        {
            // my_textField.setTextFormat(textFormat:TextFormat)
            GASObject* pobjVal = fn.Arg(0).ToObject();
            if (pobjVal->GetObjectType() == Object_TextFormat)
            {
                GASTextFormatObject* ptextFormatObj = static_cast<GASTextFormatObject*>(pobjVal);
                pthis->pDocument->SetTextFormat(ptextFormatObj->TextFormat);
                pthis->pDocument->SetParagraphFormat(ptextFormatObj->ParagraphFormat);
            }
        }
        else if (fn.NArgs == 2)
        {
            // my_textField.setTextFormat(beginIndex:Number, textFormat:TextFormat)
            GASObject* pobjVal = fn.Arg(1).ToObject();
            if (pobjVal->GetObjectType() == Object_TextFormat)
            {
                GASTextFormatObject* ptextFormatObj = static_cast<GASTextFormatObject*>(pobjVal);
                pthis->pDocument->SetTextFormat(ptextFormatObj->TextFormat, (UInt)fn.Arg(0).ToNumber(fn.Env));
                pthis->pDocument->SetParagraphFormat(ptextFormatObj->ParagraphFormat, (UInt)fn.Arg(0).ToNumber(fn.Env));
            }
        }
        else if (fn.NArgs >= 3)
        {
            // my_textField.setTextFormat(beginIndex:Number, endIndex:Number, textFormat:TextFormat)
            GASObject* pobjVal = fn.Arg(2).ToObject();
            if (pobjVal->GetObjectType() == Object_TextFormat)
            {
                GASTextFormatObject* ptextFormatObj = static_cast<GASTextFormatObject*>(pobjVal);
                pthis->pDocument->SetTextFormat(ptextFormatObj->TextFormat, 
                    (UInt)fn.Arg(0).ToNumber(fn.Env), (UInt)fn.Arg(1).ToNumber(fn.Env));
                pthis->pDocument->SetParagraphFormat(ptextFormatObj->ParagraphFormat, 
                    (UInt)fn.Arg(0).ToNumber(fn.Env), (UInt)fn.Arg(1).ToNumber(fn.Env));
            }
        }
    }
}

void GFxEditTextCharacter::GetTextFormat(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        UPInt beginIndex = 0, endIndex = GFC_MAX_UPINT;
        if (fn.NArgs >= 1)
        {
            // beginIndex
            GASNumber v = fn.Arg(0).ToNumber(fn.Env);
            if (v >= 0)
                beginIndex = (UInt)v;
        }
        if (fn.NArgs >= 2)
        {
            // endIndex
            GASNumber v = fn.Arg(1).ToNumber(fn.Env);
            if (v >= 0)
                endIndex = (UInt)v;
        }
        GFxTextFormat textFmt;
        GFxTextParagraphFormat paraFmt;
        pthis->pDocument->GetTextAndParagraphFormat(&textFmt, &paraFmt, beginIndex, endIndex);

        GPtr<GASTextFormatObject> pasTextFmt = *new GASTextFormatObject(fn.Env);
        pasTextFmt->SetTextFormat(fn.Env->GetSC(), textFmt);
        pasTextFmt->SetParagraphFormat(fn.Env->GetSC(), paraFmt);
        fn.Result->SetAsObject(pasTextFmt);
    }
    else
        fn.Result->SetUndefined();
}

void GFxEditTextCharacter::GetNewTextFormat(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        const GFxTextFormat* ptextFmt = pthis->pDocument->GetDefaultTextFormat();
        const GFxTextParagraphFormat* pparaFmt = pthis->pDocument->GetDefaultParagraphFormat();

        GPtr<GASTextFormatObject> pasTextFmt = *new GASTextFormatObject(fn.Env);
        if (ptextFmt)
            pasTextFmt->SetTextFormat(fn.Env->GetSC(), *ptextFmt);
        if (pparaFmt)
            pasTextFmt->SetParagraphFormat(fn.Env->GetSC(), *pparaFmt);
        fn.Result->SetAsObject(pasTextFmt);
   }
    else
        fn.Result->SetUndefined();
}

void GFxEditTextCharacter::SetNewTextFormat(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            GASObject* pobjVal = fn.Arg(0).ToObject();
            if (pobjVal->GetObjectType() == Object_TextFormat)
            {
                GASTextFormatObject* ptextFormatObj = static_cast<GASTextFormatObject*>(pobjVal);

                const GFxTextFormat* ptextFmt = pthis->pDocument->GetDefaultTextFormat();
                const GFxTextParagraphFormat* pparaFmt = pthis->pDocument->GetDefaultParagraphFormat();
                pthis->pDocument->SetDefaultTextFormat(ptextFmt->Merge(ptextFormatObj->TextFormat));
                pthis->pDocument->SetDefaultParagraphFormat(pparaFmt->Merge(ptextFormatObj->ParagraphFormat));
            }
        }
    }
}

void GFxEditTextCharacter::ReplaceText(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 3)
        {
            GASNumber start = fn.Arg(0).ToNumber(fn.Env);
            GASNumber end   = fn.Arg(1).ToNumber(fn.Env);
            GASString str = fn.Arg(2).ToString(fn.Env);

            const GFxTextFormat* ptextFmt = pthis->pDocument->GetDefaultTextFormat();
            const GFxTextParagraphFormat* pparaFmt = pthis->pDocument->GetDefaultParagraphFormat();

            UInt len = str.GetLength();
            UInt startPos = UInt(start);
            UInt endPos   = UInt(end);
            wchar_t buf[1024];
            if (len < sizeof(buf)/sizeof(buf[0]))
            {
                GUTF8Util::DecodeString(buf, str.ToCStr());
                pthis->pDocument->ReplaceText(buf, startPos, endPos);
            }
            else
            {
                wchar_t* pbuf = (wchar_t*)GALLOC((len + 1) * sizeof(wchar_t));
                GUTF8Util::DecodeString(pbuf, str.ToCStr());
                pthis->pDocument->ReplaceText(pbuf, startPos, endPos);
                GFREE(pbuf);
            }
            if (pthis->pDocument->HasEditorKit())
                pthis->pDocument->GetEditorKit()->SetCursorPos(startPos + len);
            if (pparaFmt)
                pthis->pDocument->SetParagraphFormat(*pparaFmt, startPos, startPos + len);
            if (ptextFmt)
                pthis->pDocument->SetTextFormat(*ptextFmt, startPos, startPos + len);
        }
    }
}

void GFxEditTextCharacter::AppendText(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            GASString str = fn.Arg(0).ToString(fn.Env);
            pthis->pDocument->AppendText(str.ToCStr());
        }
    }
}

void GFxEditTextCharacter::AppendHtml(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            GASString str = fn.Arg(0).ToString(fn.Env);
            pthis->pDocument->AppendHtml(str.ToCStr());
        }
    }
}

void GFxEditTextCharacter::RemoveTextField(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (pthis->GetDepth() < 16384)
        {
            pthis->LogScriptWarning("%s.removeMovieClip() failed - depth must be >= 0\n",
                pthis->GetName().ToCStr());
            return;
        }
        pthis->RemoveDisplayObject();
    }
}

void GFxEditTextCharacter::GetCharBoundaries(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            UInt charIndex = (UInt)fn.Arg(0).ToNumber(fn.Env);
            GRectF charBounds;
            charBounds.Clear();
            if (pthis->pDocument->GetCharBoundaries(&charBounds, charIndex))
            {
                GPtr<GASRectangleObject> prect = *new GASRectangleObject(fn.Env);
                fn.Result->SetAsObject(prect.GetPtr());

                GASRect gasRect;
                gasRect.Left    = TwipsToPixels(GASNumber(charBounds.Left));
                gasRect.Top     = TwipsToPixels(GASNumber(charBounds.Top));
                gasRect.Right   = TwipsToPixels(GASNumber(charBounds.Right));
                gasRect.Bottom  = TwipsToPixels(GASNumber(charBounds.Bottom));
                prect->SetProperties(fn.Env, gasRect);
            }
            else
                fn.Result->SetNull();
        }
    }
}

void GFxEditTextCharacter::GetExactCharBoundaries(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            UInt charIndex = (UInt)fn.Arg(0).ToNumber(fn.Env);
            GRectF charBounds;
            charBounds.Clear();
            if (pthis->pDocument->GetExactCharBoundaries(&charBounds, charIndex))
            {
                GPtr<GASRectangleObject> prect = *new GASRectangleObject(fn.Env);
                fn.Result->SetAsObject(prect.GetPtr());

                GASRect gasRect;
                gasRect.Left    = TwipsToPixels(GASNumber(charBounds.Left));
                gasRect.Top     = TwipsToPixels(GASNumber(charBounds.Top));
                gasRect.Right   = TwipsToPixels(GASNumber(charBounds.Right));
                gasRect.Bottom  = TwipsToPixels(GASNumber(charBounds.Bottom));
                prect->SetProperties(fn.Env, gasRect);
            }
            else
                fn.Result->SetNull();
        }
    }
}

void GFxEditTextCharacter::GetCharIndexAtPoint(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 2)
        {
            GASNumber x = fn.Arg(0).ToNumber(fn.Env);
            GASNumber y = fn.Arg(1).ToNumber(fn.Env);
            UPInt pos = pthis->pDocument->GetCharIndexAtPoint(Float(PixelsToTwips(x)), Float(PixelsToTwips(y)));
            if (!GTL::IsMax(pos))
                fn.Result->SetNumber(GASNumber(pos));
            else
                fn.Result->SetNumber(-1);
        }
    }
}

void GFxEditTextCharacter::GetLineIndexAtPoint(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 2)
        {
            GASNumber x = fn.Arg(0).ToNumber(fn.Env);
            GASNumber y = fn.Arg(1).ToNumber(fn.Env);
            UInt pos = pthis->pDocument->GetLineIndexAtPoint(Float(PixelsToTwips(x)), Float(PixelsToTwips(y)));
            if (!GTL::IsMax(pos))
                fn.Result->SetNumber(GASNumber(pos));
            else
                fn.Result->SetNumber(-1);
        }
    }
}

void GFxEditTextCharacter::GetLineOffset(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            SInt lineIndex = SInt(fn.Arg(0).ToNumber(fn.Env));
            if (lineIndex < 0)
                fn.Result->SetNumber(-1);
            else
            {
                UPInt off = pthis->pDocument->GetLineOffset(UInt(lineIndex));
                if (!GTL::IsMax(off))
                    fn.Result->SetNumber(GASNumber(off));
                else
                    fn.Result->SetNumber(-1);
            }
        }
    }
}

void GFxEditTextCharacter::GetLineLength(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            SInt lineIndex = SInt(fn.Arg(0).ToNumber(fn.Env));
            if (lineIndex < 0)
                fn.Result->SetNumber(-1);
            else
            {
                UPInt len = pthis->pDocument->GetLineLength(UInt(lineIndex));
                if (!GTL::IsMax(len))
                    fn.Result->SetNumber(GASNumber(len));
                else
                    fn.Result->SetNumber(-1);
            }
        }
    }
}

void GFxEditTextCharacter::GetLineMetrics(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            SInt lineIndex = SInt(fn.Arg(0).ToNumber(fn.Env));
            if (lineIndex < 0)
                fn.Result->SetNumber(-1);
            else
            {
                GASSERT(0);
                pthis = pthis;
                UInt len = ~0u;//TODO. pthis->pDocument->GetLineLength(UInt(lineIndex));
                if (len != ~0u)
                    fn.Result->SetNumber(GASNumber(len));
                else
                    fn.Result->SetNumber(-1);
            }
        }
    }
}

void GFxEditTextCharacter::GetLineText(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            SInt lineIndex = SInt(fn.Arg(0).ToNumber(fn.Env));
            if (lineIndex < 0)
                fn.Result->SetNumber(-1);
            else
            {
                GASSERT(0);
                pthis = pthis;
                UInt len = ~0u;//TODO. pthis->pDocument->GetLineLength(UInt(lineIndex));
                if (len != ~0u)
                    fn.Result->SetNumber(GASNumber(len));
                else
                    fn.Result->SetNumber(-1);
            }
        }
    }
}


void GFxEditTextCharacter::GetFirstCharInParagraph(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            SInt charIndex = SInt(fn.Arg(0).ToNumber(fn.Env));
            if (charIndex < 0)
                fn.Result->SetNumber(-1);
            else
            {
                UPInt off = pthis->pDocument->GetFirstCharInParagraph(UInt(charIndex));
                if (!GTL::IsMax(off))
                    fn.Result->SetNumber(GASNumber(off));
                else
                    fn.Result->SetNumber(-1);
            }
        }
    }
}

void GFxEditTextCharacter::GetLineIndexOfChar(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            SInt charIndex = SInt(fn.Arg(0).ToNumber(fn.Env));
            if (charIndex < 0)
                fn.Result->SetNumber(-1);
            else
            {
                UInt off = pthis->pDocument->GetLineIndexOfChar(UInt(charIndex));
                if (!GTL::IsMax(off))
                    fn.Result->SetNumber(GASNumber(off));
                else
                    fn.Result->SetNumber(-1);
            }
        }
    }
}

void GFxEditTextCharacter::ProceedImageSubstitution(const GASFnCall& fn, int idx, const GASValue* pve)
{
    if (pve && pve->IsObject())
    {
        GASObject* peobj = pve->ToObject();
        GASValue val;
        GFxTextDocView::ImageSubstitutor* pimgSubst = pDocument->CreateImageSubstitutor();
        if (!pimgSubst)
            return;
        GFxTextDocView::ImageSubstitutor::Element isElem;
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "subString", &val))
        {
            GASString str = val.ToString(fn.Env);
            UPInt wstrLen = str.GetLength();
            wstrLen = GTL::gpmin(wstrLen, (UPInt)(sizeof(isElem.SubString) - 1));

            GUTF8Util::DecodeString(isElem.SubString, str.ToCStr(), wstrLen + 1);
            isElem.SubStringLen = (UByte)wstrLen;
        }
        else
        {
            // subString is mandatory!
            LogScriptWarning("%s.setImageSubstitutions() failed for #%d element - subString should be specified\n",
                GetName().ToCStr(), idx);
            return;
        }
        GPtr<GFxShapeCharacterDef> pimgShape;
        Float screenWidth = 0, screenHeight = 0;
        Float origWidth = 0, origHeight = 0;
        Float baseLineX = 0, baseLineY = 0;
        const char* idStr = NULL;
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "image", &val))
        {
            GASObject* piobj = val.ToObject();
            if (piobj && piobj->GetObjectType() == Object_BitmapData)
            {
                GASBitmapData* pbmpData = static_cast<GASBitmapData*>(piobj);
                GFxImageResource* pimgRes = pbmpData->GetImage();
                pimgShape = *new GFxShapeCharacterDef();
                pimgShape->SetToImage(pimgRes, true);

                screenWidth  = origWidth  = (Float)PixelsToTwips(pimgRes->GetWidth());
                screenHeight = origHeight = (Float)PixelsToTwips(pimgRes->GetHeight());
                if (origWidth == 0 || origHeight == 0)
                {
                    LogScriptWarning("%s.setImageSubstitutions() failed for #%d element - image has one zero dimension\n",
                        GetName().ToCStr(), idx);
                    return;
                }
            }
        }
        if (!pimgShape)
        {
            LogScriptWarning("%s.setImageSubstitutions() failed for #%d element - 'image' is not specified or not a BitmapData\n",
                GetName().ToCStr(), idx);
            return;
        }
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "width", &val))
        {
            GASNumber v = val.ToNumber(fn.Env);
            screenWidth = (Float)PixelsToTwips(v);
        }
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "height", &val))
        {
            GASNumber v = val.ToNumber(fn.Env);
            screenHeight = (Float)PixelsToTwips(v);
        }
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "baseLineX", &val))
        {
            GASNumber v = val.ToNumber(fn.Env);
            baseLineX = (Float)PixelsToTwips(v);
        }
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "baseLineY", &val))
        {
            GASNumber v = val.ToNumber(fn.Env);
            baseLineY = (Float)PixelsToTwips(v);
        }
        else
        {
            // if baseLineY is not specified, then use (origHeight - 1) pts instead
            baseLineY = origHeight - PixelsToTwips(1.0f);
        }
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "id", &val))
        {
            idStr = val.ToString(fn.Env).ToCStr();
        }

        isElem.pImageDesc = *new GFxTextImageDesc;
        GASSERT(isElem.pImageDesc);
        isElem.pImageDesc->pImageShape  = pimgShape;
        isElem.pImageDesc->BaseLineX    = (SInt)baseLineX;
        isElem.pImageDesc->BaseLineY    = (SInt)baseLineY;
        isElem.pImageDesc->ScreenWidth  = (UInt)screenWidth;
        isElem.pImageDesc->ScreenHeight = (UInt)screenHeight;
        if (idStr)
            AddIdImageDescAssoc(idStr, isElem.pImageDesc);
        isElem.pImageDesc->Matrix.AppendTranslation(-baseLineX, -baseLineY);
        isElem.pImageDesc->Matrix.AppendScaling(screenWidth/origWidth, screenHeight/origHeight);

        pimgSubst->AddImageDesc(isElem);
        pDocument->SetCompleteReformatReq();
    }
}


void GFxEditTextCharacter::SetImageSubstitutions(const GASFnCall& fn)
{
    fn.Result->SetBool(false);
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            if (fn.Arg(0).IsNull())
            {
                // clear all substitutions
                pthis->ClearIdImageDescAssoc();
                pthis->pDocument->ClearImageSubstitutor();
                pthis->pDocument->SetCompleteReformatReq();
            }
            else
            {
                GASObject* pobj = fn.Arg(0).ToObject();
                if (pobj)
                {
                    // if array is specified as a parameter, proceed it; otherwise
                    // if an object is specified - proceed it as single element.
                    if (pobj->GetObjectType() == Object_Array)
                    {
                        GASArrayObject* parr = static_cast<GASArrayObject*>(pobj);
                        for (int i = 0, n = parr->GetSize(); i < n; ++i)
                        {
                            GASValue* pve = parr->GetElementPtr(i);
                            pthis->ProceedImageSubstitution(fn, i, pve);
                        }
                    }
                    else
                    {
                        const GASValue& ve = fn.Arg(0);
                        pthis->ProceedImageSubstitution(fn, 0, &ve);
                    }
                }
                else
                {
                    pthis->LogScriptWarning("%s.setImageSubstitutions() failed: parameter should be either 'null', object or array\n",
                        pthis->GetName().ToCStr());
                }
            }
        }
    }
}

void GFxEditTextCharacter::UpdateImageSubstitution(const GASFnCall& fn)
{
    fn.Result->SetBool(false);
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            GASString idStr = fn.Arg(0).ToString(fn.Env);
            if (pthis->pImageDescAssoc)
            {
                GPtr<GFxTextImageDesc>* ppimgDesc = pthis->pImageDescAssoc->get(idStr.ToCStr());
                if (ppimgDesc)
                {
                    GFxTextImageDesc* pimageDesc = ppimgDesc->GetPtr();
                    GASSERT(pimageDesc);
                    if (fn.NArgs >= 2)
                    {
                        if (fn.Arg(1).IsNull() || fn.Arg(1).IsUndefined())
                        {
                            // if null or undefined - remove the substitution and reformat
                            GFxTextDocView::ImageSubstitutor* pimgSubst = pthis->pDocument->CreateImageSubstitutor();
                            if (pimgSubst)
                            {
                                pimgSubst->RemoveImageDesc(pimageDesc);
                                pthis->pDocument->SetCompleteReformatReq();
                                pthis->RemoveIdImageDescAssoc(idStr.ToCStr());
                            }
                        }
                        else
                        {
                            GASObject* piobj = fn.Arg(1).ToObject();
                            if (piobj && piobj->GetObjectType() == Object_BitmapData)
                            {
                                GASBitmapData* pbmpData = static_cast<GASBitmapData*>(piobj);
                                GFxImageResource* pimgRes = pbmpData->GetImage();
                                GPtr<GFxShapeCharacterDef> pimgShape = *new GFxShapeCharacterDef();
                                pimgShape->SetToImage(pimgRes, true);

                                pimageDesc->pImageShape = pimgShape;
                            }
                        }
                    }
                }
            }
        }
    }
}

void GFxEditTextCharacter::ProcessImageTags(GFxStyledText::HTMLImageTagInfoArray& imageInfoArray)
{
    GASEnvironment* penv = GetASEnvironment();
    for (UPInt i = 0, n = imageInfoArray.size(); i < n; ++i)
    {
        GFxStyledText::HTMLImageTagInfo& imgTagInfo = imageInfoArray[i];
        GASSERT(imgTagInfo.pTextImageDesc);
        // TODO: Url should be used to load image by the same way as loadMovie does.
        // At the moment there is no good way to get a callback when movie is loaded.
        // Therefore, at the moment, Url is used as export name only. "imgps://" also
        // may be used.
       
        GFxResourceBindData resBindData;
        GPtr<GFxMovieDefImpl> md = penv->GetTarget()->GetResourceMovieDef();
        if (md) // can it be NULL?
        {
            if (!md->GetExportedResource(&resBindData, imgTagInfo.Url))
            {
                penv->LogScriptWarning("ProcessImageTags: can't find a resource for export name '%s'\n", imgTagInfo.Url.ToCStr());
                continue;
            }
            GASSERT(resBindData.pResource.GetPtr() != 0);
            // Must check resource type, since users can theoretically pass other resource ids.
            if (resBindData.pResource->GetResourceType() == GFxResource::RT_Image)
            {
                // bitmap
                GPtr<GASBitmapData> pbmpData = *GASBitmapData::LoadBitmap(penv, imgTagInfo.Url);
                if (!pbmpData)
                {
                    penv->LogScriptWarning("ProcessImageTags: can't load the image '%s'\n", imgTagInfo.Url.ToCStr());
                    continue;
                }
                GFxImageResource* pimgRes = pbmpData->GetImage();
                GASSERT(pimgRes);
                GPtr<GFxShapeCharacterDef> pimgShape = *new GFxShapeCharacterDef();
                GASSERT(pimgShape);
                pimgShape->SetToImage(pimgRes, true);
                imgTagInfo.pTextImageDesc->pImageShape = pimgShape;

                Float origWidth = 0, origHeight = 0;
                Float screenWidth = 0, screenHeight = 0;
                screenWidth  = origWidth  = (Float)PixelsToTwips(pimgRes->GetWidth());
                screenHeight = origHeight = (Float)PixelsToTwips(pimgRes->GetHeight());
                if (imgTagInfo.Width)
                    screenWidth = Float(imgTagInfo.Width);
                if (imgTagInfo.Height)
                    screenHeight = Float(imgTagInfo.Height);

                Float baseLineY = origHeight - PixelsToTwips(1.0f);
                baseLineY += imgTagInfo.VSpace;
                imgTagInfo.pTextImageDesc->ScreenWidth  = (UInt)screenWidth;
                imgTagInfo.pTextImageDesc->ScreenHeight = (UInt)screenHeight;
                imgTagInfo.pTextImageDesc->Matrix.AppendTranslation(0, -baseLineY);
                imgTagInfo.pTextImageDesc->Matrix.AppendScaling(screenWidth/origWidth, screenHeight/origHeight);

                pDocument->SetCompleteReformatReq();
            }
            if (resBindData.pResource->GetResourceType() == GFxResource::RT_SpriteDef)
            {
                // sprite
                //GFxSpriteDef* psprRes = (GFxSpriteDef*)resBindData.pResource.GetPtr();
            }
        }
    }
}

//////////////////////////////////////////
static const GASNameFunction GAS_TextFieldFunctionTable[] = 
{
    { "getDepth",           &GFxASCharacter::CharacterGetDepth },
    { "getTextFormat",      &GFxEditTextCharacter::GetTextFormat },
    { "getNewTextFormat",   &GFxEditTextCharacter::GetNewTextFormat },
    { "setTextFormat",      &GFxEditTextCharacter::SetTextFormat },
    { "setNewTextFormat",   &GFxEditTextCharacter::SetNewTextFormat },
    { "replaceText",        &GFxEditTextCharacter::ReplaceText },
    { "removeTextField",    &GFxEditTextCharacter::RemoveTextField },

    { 0, 0 }
};

static const GASNameFunction GAS_TextFieldExtFunctionTable[] = 
{
    { "appendText",             &GFxEditTextCharacter::AppendText },
    { "appendHtml",             &GFxEditTextCharacter::AppendHtml },
    { "getCharBoundaries",      &GFxEditTextCharacter::GetCharBoundaries },
    { "getCharIndexAtPoint",    &GFxEditTextCharacter::GetCharIndexAtPoint },
    { "getExactCharBoundaries", &GFxEditTextCharacter::GetExactCharBoundaries },
    { "getFirstCharInParagraph",&GFxEditTextCharacter::GetFirstCharInParagraph },
    { "getLineIndexAtPoint",    &GFxEditTextCharacter::GetLineIndexAtPoint },
    { "getLineIndexOfChar",     &GFxEditTextCharacter::GetLineIndexOfChar },
    { "getLineOffset",          &GFxEditTextCharacter::GetLineOffset },
    { "getLineMetrics",         &GFxEditTextCharacter::GetLineMetrics },
    { "getLineText",            &GFxEditTextCharacter::GetLineText },
    { "getLineLength",          &GFxEditTextCharacter::GetLineLength },
    { "setImageSubstitutions",  &GFxEditTextCharacter::SetImageSubstitutions },
    { "updateImageSubstitution",&GFxEditTextCharacter::UpdateImageSubstitution },

    { 0, 0 }
};

GASTextFieldProto::GASTextFieldProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor) :
    GASPrototype<GASTextFieldObject>(psc, prototype, constructor)
{
    GASAsBroadcaster::InitializeProto(psc, this);
    InitFunctionMembers(psc, GAS_TextFieldFunctionTable, prototype);
    SetConstMemberRaw(psc, "scroll", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "hscroll", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "maxscroll", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "maxhscroll", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);

    SetConstMemberRaw(psc, "background", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "backgroundColor", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "border", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "borderColor", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "bottomScroll", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "mouseWheelEnabled", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);

    // init extensions
    InitFunctionMembers(psc, GAS_TextFieldExtFunctionTable, NULL);
}

void GASTextFieldProto::GlobalCtor(const GASFnCall& fn) 
{
    fn.Result->SetAsObject(GPtr<GASObject>(*new GASTextFieldObject()));
}

// Read CSMTextSettings tag
void    GSTDCALL GFx_CSMTextSettings(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);        
    GASSERT(tagInfo.TagType == GFxTag_CSMTextSettings);

    UInt16  characterId = p->ReadU16();

    GFxStream*  pin = p->GetStream();    

    UInt  flagType  = pin->ReadUInt(2);
    UInt  gridFit   = pin->ReadUInt(3);
    Float thickness = pin->ReadFloat();
    Float sharpness = pin->ReadFloat();

    if (pin->IsVerboseParse())
    {
        static const char* const gridfittypes[] = { "None", "Pixel", "LCD" };
        p->LogParse("CSMTextSettings, id = %d\n", characterId);
        p->LogParse("  FlagType = %s, GridFit = %s\n", 
            (flagType == 0) ? "System" : "Internal", gridfittypes[gridFit] );
        p->LogParse("  Thinkness = %f, Sharpnesss = %f\n", 
            (float)thickness, (float)sharpness);
    }

    GFxResourceHandle handle;
    if (p->GetResourceHandle(&handle, GFxResourceId(characterId)))
    {
        GFxResource* rptr = handle.GetResourcePtr();
        if (rptr)
        { 
            if (rptr->GetResourceType() == GFxResource::RT_EditTextDef)
            {
                GFxEditTextCharacterDef* pch = static_cast<GFxEditTextCharacterDef*>(rptr);
                pch->SetAAForReadability();
            }
            else if (rptr->GetResourceType() == GFxResource::RT_TextDef)
            {
                GFxStaticTextCharacterDef* pch = static_cast<GFxStaticTextCharacterDef*>(rptr);
                pch->SetAAForReadability();
            }
        }
    }
}


/**********************************************************************

Filename    :   GFxTextDocView.cpp
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

#include "Text/GFxTextDocView.h"
#include "Text/GFxTextEditorKit.h"

#define GFX_BULLET_CHAR_CODE        0x25CF
#define GFX_BULLET_CHAR_ADVANCE     15
#define GFX_BULLET_PARA_INDENT      20
#define GFX_LINE_ALLOC_DELTA        100
#define GFX_TEXT_JUSTIFY_CORRECTION 30

#define GFX_MIN_LINE_HEIGHT_FOR_AUTOSIZE PixelsToTwips(6)

#ifdef GFC_BUILD_DEBUG
//#define DEBUG_CURSOR_POS
//#define GFX_DEBUG_DISPLAY_INVISIBLE_CHARS
#endif //GFC_BUILD_DEBUG

//////////////////////////////////
// GFxTextDocView
//
GFxTextDocView::GFxTextDocView(GFxFontManager* pfontMgr, GFxLog* plog) : 
pFontManager(pfontMgr), pHighlight(NULL), pLog(plog)
{
    GPtr<GFxTextAllocator> pallocator = *new GFxTextAllocator();
    pDocument = *new DocumentText(this, pallocator);
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
    RTFlags &= ~RTFlags_FontErrorDetected; //?
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
                RemoveText(stPos + 1, endPos + 1);
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
            RemoveText(stPos + res, endPos + res);
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
            RemoveText(stPos + res, endPos + res);
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
                        ppara->SetFormat(pDocument->GetAllocator(), newFmt);
                        OnDocumentChanged(ViewNotify_TextChange);
                        res = 0;
                    }
                    else if (pparaFmt->GetIndent() != 0 || pparaFmt->GetBlockIndent() != 0)
                    {
                        GFxTextParagraphFormat newFmt = *pparaFmt;
                        newFmt.SetIndent(0);
                        newFmt.SetBlockIndent(0);
                        ppara->SetFormat(pDocument->GetAllocator(), newFmt);
                        OnDocumentChanged(ViewNotify_TextChange);
                        res = 0;
                    }
                }
            }
            if (res == 1 && cmd.PosAt > 0) // res wasn't zeroed - means we need to remove char
            {
                RemoveText(cmd.PosAt - 1, cmd.PosAt);
            }
            else
                res = 0;
            break;
        }
    case Cmd_DeleteChar:
        {
            GASSERT(command);
            const DeleteCharCommand& cmd = *reinterpret_cast<const DeleteCharCommand*>(command);
            RemoveText(cmd.PosAt, cmd.PosAt + 1);
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
            RemoveText(stPos, endPos);
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

void GFxTextDocView::ParseHtml(const char* putf8Str, UPInt utf8Len, bool condenseWhite, 
                               GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr,
                               const GFxTextStyleManager* pstyleMgr)
{
    pDocument->Clear();
    if (utf8Len == GFC_MAX_UPINT)
        utf8Len = gfc_strlen(putf8Str); // actually, this is the SIZE, not length. TODO
    pDocument->ParseHtml(putf8Str, utf8Len, pimgInfoArr, IsMultiline(), condenseWhite, pstyleMgr);
    OnDocumentChanged(ViewNotify_SignificantTextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::ParseHtml(const wchar_t* pwStr, UPInt strLen, bool condenseWhite, 
                               GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr,
                               const GFxTextStyleManager* pstyleMgr)
{
    pDocument->Clear();
    if (strLen == GFC_MAX_UPINT)
        strLen = gfc_wcslen(pwStr); 
    pDocument->ParseHtml(pwStr, strLen, pimgInfoArr, IsMultiline(), condenseWhite, pstyleMgr);
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
    pDocument->ParseHtml(pwStr, strLen, NULL, IsMultiline(), condenseWhite);
    OnDocumentChanged(ViewNotify_TextChange | ViewNotify_ScrollingParamsChange);
}

void GFxTextDocView::AppendHtml(const char* putf8Str, UPInt utf8Len, bool condenseWhite)
{
    if (utf8Len == GFC_MAX_UPINT)
        utf8Len = gfc_strlen(putf8Str); // actually, this is the SIZE, not length. TODO
    // at the moment, image tags are not supported by AppendHtml. TODO.
    pDocument->ParseHtml(putf8Str, utf8Len, NULL, IsMultiline(), condenseWhite);
    OnDocumentChanged(ViewNotify_TextChange | ViewNotify_ScrollingParamsChange);
}

UPInt GFxTextDocView::InsertText(const wchar_t* pstr, UPInt startPos, UPInt strLen)
{
    return pDocument->InsertString(pstr, startPos, strLen);
}

UPInt GFxTextDocView::ReplaceText(const wchar_t* pstr, UPInt startPos, UPInt endPos, UPInt strLen)
{
    RemoveText(startPos, endPos);
    UPInt len = pDocument->InsertString(pstr, startPos, strLen);
    //OnDocumentChanged(ViewNotify_TextChange);
    return len;
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

bool GFxTextDocView::ContainsNonLeftAlignment() const
{
    for (UInt i = 0, n = pDocument->GetParagraphsCount(); i < n; ++i)
    {
        const GFxTextParagraph* ppara = pDocument->GetParagraph(i);
        GASSERT(ppara);
        if (ppara->GetFormat()->GetAlignment() != GFxTextParagraphFormat::Align_Left)
            return true;
    }
    return false;
}

void GFxTextDocView::SetViewRect(const GRectF& rect, UseType ut)
{
    if (rect != ViewRect)
    {
        UInt oldW = (UInt)ViewRect.Width();
        UInt oldH = (UInt)ViewRect.Height();
        ViewRect = rect;
        GRectF textRect = ViewRect;
        textRect.Contract(GFX_TEXT_GUTTER);
        LineBuffer.SetVisibleRect(textRect);
        if (ut == UseExternally)
        {
            // if WordWrap is on and width has been changed then we need 
            // reformat. Otherwise, we just need to update the 
            // maxhscroll and maxvscroll values, unless we have set vertical
            // alignment, horizontal alignment other than "left", text auto size is on,
            // or one of the paragraph has alignment other than "left".
            if (GetTextAutoSize() != TAS_None ||
                (oldW != (UInt)ViewRect.Width() && (IsWordWrap() || GetAlignment() != Align_Left || ContainsNonLeftAlignment())) ||
                (oldH != (UInt)ViewRect.Height() && (GetVAlignment() != VAlign_None && GetVAlignment() != VAlign_Top)))
            {
                SetCompleteReformatReq();
            }
            else
            {
                ++FormatCounter; // invalidate cached values to force recalc
                UInt maxHScroll = GetMaxHScroll();
                if(GetHScrollOffsetValue() > maxHScroll)
                {
                    SetHScrollOffset(maxHScroll);
                }
                UInt maxVScroll = GetMaxVScroll();
                if(GetVScrollOffsetValue() > maxVScroll)
                {
                    SetVScrollOffset(maxVScroll);
                }
                LineBuffer.InvalidateCache(); // need to force batches rebuild
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
    GFxLineCursor(GFxTextDocView* pview, const GFxTextParagraph* ppara):
    pPrevGrec(NULL), pLastFont(NULL), LastCharCode(0), LastGlyphIndex(0), LastAdvance(0), 
        LastGlyphWidth(0), LastColor(0), 
        ComposStrPosition(GFC_MAX_UPINT), ComposStrLength(0), ComposStrCurPos(0),
        pDocView(pview), pParagraph(ppara), LineWidth(0), 
        LineWidthWithoutTrailingSpaces(0), LineLength(0),
        MaxFontAscent(0), MaxFontDescent(0), MaxFontLeading(0),
        CharIter(ppara->GetCharactersIterator(0)), Indent(0), LeftMargin(0), RightMargin(0), NumOfSpaces(0), 
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
        LastCharCode  = 0;
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

class GFxParagraphFormatter
{
    GFxTextDocView*                         pDocView;
    const GFxTextParagraph*                 pParagraph;
    const GFxTextParagraphFormat*           pParaFormat;
    GFxTextLineBuffer::Line*                pTempLine;
    GFxLineCursor                           LineCursor;
    GFxTextDocView::FontCache               FontCache;
    GFxTextDocView::FindFontInfo            FindFontInfo;
    GFxLineCursor                           StartPoint, HalfPoint, WordWrapPoint;
    Float                                   TextRectWidth;

    UInt                                    TabStopsNum, TabStopsIndex;
    UInt                                    DeltaText;
    int                                     Pass;
    bool                                    HyphenationRequested;
    UInt                                    RequestedWordWrapPos;     // used only if Pass == 2
    bool                                    HasLineFormatHandler;

    // custom wordwrapping
    wchar_t                                 TextBufForCustomFormat[256];
    wchar_t*                                pTextBufForCustomFormat;
    UPInt                                   TextBufLen;

    // local vars
    GFxFontResource*                        pFont;
    GPtr<GFxFontHandle>                     pFontHandle;
    Float                                   Scale;
    int                                     GlyphIndex;
    Float                                   GlyphAdvance;
    SInt                                    LastAdvance;
    SInt                                    GlyphWidth;
    SInt                                    AdjLineWidth;
    SInt                                    NewLineWidth;
    Float                                   FontSize;
    bool                                    isSpace;
    bool                                    isNbsp;
public:
    // in-out values
    GFxTextLineBuffer::Iterator*            pLinesIter;
    UByte                                   TempLineBuff[1024];
    GFxTextLineBuffer::Line*                pDynLine;
    Float                                   NextOffsetY;

    // in values
    Float                                   ParaYOffset;
    GFxLog*                                 pLog;

    // out values
    SInt                                    ParaWidth;
    SInt                                    ParaHeight;
    UInt                                    ParaLines;
    bool                                    NeedRecenterLines;
    bool                                    ForceVerticalCenterAutoSize;    // force vertical centered autoSize


    void FinalizeLine();
    void InitParagraph(const GFxTextParagraph& paragraph);
    void InitCustomWordWrapping();
    Float GetActualFontSize();
    bool CheckWordWrap();
    bool HandleCustomWordWrap();
public:
    GFxParagraphFormatter(GFxTextDocView*, GFxLog* plog);
    ~GFxParagraphFormatter();
    void Format(const GFxTextParagraph& paragraph);
};

GFxParagraphFormatter::GFxParagraphFormatter
(GFxTextDocView* pdoc, GFxLog* plog) :
pDocView(pdoc), pParagraph(NULL), pParaFormat(NULL), pTempLine(NULL),
FindFontInfo(&FontCache),
pDynLine(NULL)
{
    ParaYOffset        = 0;
    NextOffsetY        = 0;
    NeedRecenterLines  = false;
    ForceVerticalCenterAutoSize = false;
    pLog               = plog;

}

GFxParagraphFormatter::~GFxParagraphFormatter()
{
    if (pDynLine)
        pDocView->LineBuffer.LineAllocator.FreeLine(pDynLine);
}

void GFxParagraphFormatter::InitParagraph(const GFxTextParagraph& paragraph)
{
    pParagraph = &paragraph;
    pParaFormat = pParagraph->GetFormat();
    GASSERT(pParagraph && pParaFormat);
    LineCursor = GFxLineCursor(pDocView, &paragraph);
    StartPoint = HalfPoint = WordWrapPoint = GFxLineCursor();
    LineCursor.FontScaleFactor = pDocView->GetFontScaleFactor();

    // Check, do we have a composition string somewhere in the paragraph
    if (pDocView->HasEditorKit() && pDocView->GetEditorKit()->HasCompositionString())
    {
#ifndef GFC_NO_IME_SUPPORT
        LineCursor.pComposStr           = pDocView->GetEditorKit()->GetCompositionString();
        LineCursor.ComposStrPosition    = LineCursor.pComposStr->GetPosition();
        LineCursor.ComposStrLength      = LineCursor.pComposStr->GetLength();
#endif //#ifndef GFC_NO_IME_SUPPORT
    }

    InitCustomWordWrapping();

    // try to estimate how much memory would be necessary for temporary line
    // Assume, glyphCount <= paragraph.GetLength()
    //         fmtDataElemCount <= paragraph.GetLength()*2*paragraph.GetLength()
    UPInt maxGlyphCount = paragraph.GetSize() + LineCursor.ComposStrLength;
    if (pParaFormat->IsBullet())
        ++maxGlyphCount;
    UInt linesz = GFxTextLineBuffer::CalcLineSize
        ((UInt)maxGlyphCount, (UInt)maxGlyphCount*2, GFxTextLineBuffer::Line32);
    if (linesz < sizeof(TempLineBuff))
    {
        pTempLine = reinterpret_cast<GFxTextLineBuffer::Line*>(TempLineBuff);
        pTempLine->SetMemSize(linesz);
    }
    else if (pDynLine)
    {
        if (linesz >= pDynLine->GetMemSize())
        {
            pDocView->LineBuffer.LineAllocator.FreeLine(pDynLine);
            pDynLine = pDocView->LineBuffer.LineAllocator.AllocLine
                (linesz + GFX_LINE_ALLOC_DELTA, GFxTextLineBuffer::Line32);
        }
        pTempLine = pDynLine;
    }
    else
        pTempLine = pDynLine = pDocView->LineBuffer.LineAllocator.AllocLine
        (linesz + GFX_LINE_ALLOC_DELTA, GFxTextLineBuffer::Line32);
    pTempLine->InitLine32();
    pTempLine->ClearInitialized(); // to avoid tempLine->Release to be invoked
    pTempLine->SetNumGlyphs((UInt)maxGlyphCount);

    GASSERT(pTempLine);
    pTempLine->SetTextPos(UInt(pDocView->TextPos2GlyphOffset(paragraph.GetStartIndex())));

    LineCursor.GlyphIns = GFxTextLineBuffer::GlyphInserter
        (pTempLine->GetGlyphs(), pTempLine->GetNumGlyphs(), pTempLine->GetFormatData());

    ParaHeight              = ParaWidth = 0;
    ParaLines               = 0;

    DeltaText               = 1;
    Pass                    = 1;
    HyphenationRequested    = false;
    RequestedWordWrapPos    = 0;     // used only if Pass == 2
    isSpace                 = false;
    TextRectWidth           = pDocView->GetTextRect().Width();
    TabStopsNum = 0, TabStopsIndex = 0;
}

void GFxParagraphFormatter::InitCustomWordWrapping()
{
    HasLineFormatHandler = (pDocView->pDocumentListener && pDocView->pDocumentListener->DoesHandleLineFormat());
    pTextBufForCustomFormat = NULL;

    // make a copy of paragraph text with insertion of composition string
    // to use it for custom formatting handler.
    if (HasLineFormatHandler)
    {
        // check if we can use buffer on stack. If not - allocate temporary one.
        UPInt reqBufSize = pParagraph->GetLength() + LineCursor.ComposStrLength;
        wchar_t* pbuf;
        if (reqBufSize < sizeof(TextBufForCustomFormat)/sizeof(TextBufForCustomFormat[0]))
            pbuf = TextBufForCustomFormat;
        else
            pbuf = pDocView->GetAllocator()->AllocText(reqBufSize + 1);

        // copy text of whole paragraph + composition string into the buffer
        const wchar_t* paraText = pParagraph->GetText();
        if (LineCursor.ComposStrLength > 0)
        {
#ifndef GFC_NO_IME_SUPPORT
            memcpy(pbuf, paraText, LineCursor.ComposStrPosition * sizeof(wchar_t));
            paraText += LineCursor.ComposStrPosition;
            memcpy(pbuf + LineCursor.ComposStrPosition, LineCursor.pComposStr->GetText(), 
                LineCursor.ComposStrLength * sizeof(wchar_t));
            memcpy(pbuf + LineCursor.ComposStrPosition + LineCursor.ComposStrLength, 
                paraText, (pParagraph->GetLength() - LineCursor.ComposStrPosition) * sizeof(wchar_t));
#endif
        }
        else
        {
            memcpy(pbuf, paraText, reqBufSize * sizeof(wchar_t));
        }
        pbuf[reqBufSize]        = 0;
        pTextBufForCustomFormat = pbuf;
        TextBufLen              = reqBufSize;
    }
}

Float GFxParagraphFormatter::GetActualFontSize()
{
    Float fontSize = FindFontInfo.pCurrentFormat->GetFontSize();
    // Not supp for ensemble
    //if (FindFontInfo.pCurrentFont->GetFontScaleFactor() != 1.0f)
    //{
    //    fontSize *= FindFontInfo.pCurrentFont->GetFontScaleFactor();
    //    //        if (!pDocView->IsMultiline())
    //    //            ForceVerticalCenterAutoSize = true;
    //}
    if (pDocView->HasFontScaleFactor())
        fontSize *= pDocView->GetFontScaleFactor();
    return fontSize;
}

void GFxParagraphFormatter::FinalizeLine()
{
    SInt lastAdvance = AlignTwipsToPixel(LineCursor.LastAdvance);
    if (LineCursor.pPrevGrec)
    {
        LineCursor.pPrevGrec->SetAdvance(lastAdvance);
    }

    // correct the line width by removing the advance and adding the real width of the last char
    if (LineCursor.pLastFont)
        LineCursor.LineWidth += LineCursor.LastGlyphWidth;
    else
        LineCursor.LineWidth += lastAdvance;

    if (pParaFormat->IsRightAlignment() || pParaFormat->IsCenterAlignment())
    {
        // if text is right or center aligned and RightMargin is not zero - 
        // add the margin to line width.
        LineCursor.LineWidth += LineCursor.RightMargin;
        LineCursor.LineWidthWithoutTrailingSpaces   += LineCursor.RightMargin;
    }

    Float fleading   = (pParaFormat->IsLeadingSet()) ? PixelsToTwips(pParaFormat->GetLeading()) : LineCursor.MaxFontLeading; 
    Float lineHeight = LineCursor.MaxFontAscent + LineCursor.MaxFontDescent;
    SInt leading = RoundTwips(fleading);

    GFxTextLineBuffer::Line* pcurLine;
    // so, here we need to allocate a "real" (non temporary) line and transfer
    // all data to it.
    // First, we need to decide, can we allocate Line8 or not...
    // The criteria for Line8 are as follows:
    // Dimensions = [-32768..32767] (i.e. -1638.4 .. 1638.35 pts), 
    // TextLen <= 255, GlyphsCount <= 255, Leading <= 255 (i.e. 12.75 pt), TextPos < 2^24
    UInt glyphsCount         = LineCursor.GlyphIns.GetGlyphIndex();
    UInt formatDataElemCount = LineCursor.GlyphIns.GetFormatDataElementsCount();
    if (pDocView->GetTextAutoSize() == GFxTextDocView::TAS_None && // if auto text fit is on - always use long form
        LineCursor.LineLength <= 255 && glyphsCount <= 255 && (leading >= -128 && leading <= 127) &&
        (lineHeight >= -32768 && lineHeight < 32768) && 
        (LineCursor.LineWidth >= -32768 && LineCursor.LineWidth < 32768))
    {
        // convert line to short form
        pcurLine = pLinesIter->InsertNewLine(glyphsCount, formatDataElemCount, GFxTextLineBuffer::Line8);
    }
    else
    {
        pcurLine = pLinesIter->InsertNewLine(glyphsCount, formatDataElemCount, GFxTextLineBuffer::Line32);
    }
    pcurLine->SetParagraphId(LineCursor.pParagraph->GetId());
    pcurLine->SetParagraphModId(LineCursor.pParagraph->GetModCounter());

    // copy glyph and format data
    memcpy(pcurLine->GetGlyphs(), pTempLine->GetGlyphs(), 
        glyphsCount*sizeof(GFxTextLineBuffer::GlyphEntry));
    memcpy(pcurLine->GetFormatData(), pTempLine->GetFormatData(), 
        formatDataElemCount*sizeof(GFxTextLineBuffer::GFxFormatDataEntry));

    pcurLine->SetTextPos(pTempLine->GetTextPos());
    pcurLine->SetTextLength(LineCursor.LineLength);

    pcurLine->SetBaseLineOffset(LineCursor.MaxFontAscent);

    // check for justify.
    // do not do justification for the last line in para, or for the empty one
    if (!LineCursor.LineHasNewLine && pDocView->IsWordWrap() && 
        LineCursor.NumOfSpaces - LineCursor.NumOfTrailingSpaces > 0 && pParaFormat->IsJustifyAlignment())
    {
        // do one more pass over the line and set appropriate advance values for spaces
        // to accomplish justification. 
        SInt deltaW = (SInt(TextRectWidth - GFX_TEXT_JUSTIFY_CORRECTION) - LineCursor.RightMargin) - 
            (LineCursor.LineWidthWithoutTrailingSpaces + LineCursor.Indent + LineCursor.LeftMargin);
        if (deltaW > 0) //? could it be < 0?
        {
            SInt advDelta = deltaW / (LineCursor.NumOfSpaces - LineCursor.NumOfTrailingSpaces);

            GFxTextLineBuffer::GlyphIterator git = pcurLine->Begin();
            for(; !git.IsFinished(); ++git)
            {
                GFxTextLineBuffer::GlyphEntry& ge = git.GetGlyph();
                if (ge.IsCharInvisible() && !ge.IsNewLineChar() && ge.GetLength() > 0)
                    ge.SetAdvance(ge.GetAdvance() + advDelta);
            }
            LineCursor.LineWidth += deltaW;
        }
    }

    // calculate the bounds of the line
    SInt lineOffsetX = LineCursor.Indent + LineCursor.LeftMargin;
    pcurLine->SetOffset(GPointF((Float)lineOffsetX, NextOffsetY));
    pcurLine->SetDimensions(LineCursor.LineWidth, SInt(lineHeight));
    pcurLine->SetLeading(leading);

    switch(pParaFormat->GetAlignment())
    {
    case GFxTextParagraphFormat::Align_Right:   
        pcurLine->SetAlignment(GFxTextLineBuffer::Line::Align_Right); 

        // align line accordingly to ViewRect
        pcurLine->SetOffsetX(GTL::gmax(0.f, TextRectWidth - LineCursor.LineWidthWithoutTrailingSpaces));

        if (pDocView->IsAutoSizeX() || pDocView->GetTextAutoSize() != GFxTextDocView::TAS_None)
        {
            NeedRecenterLines = true;
        }
        break;
    case GFxTextParagraphFormat::Align_Center:  
        {
            pcurLine->SetAlignment(GFxTextLineBuffer::Line::Align_Center); 

            // align line accordingly to ViewRect and LeftMargin. RightMargin is already counted
            // at the top of this func.
            Float adjTextRect = TextRectWidth  - Float(LineCursor.LeftMargin);
            pcurLine->SetOffsetX (GTL::gmax(0.f, adjTextRect / 2 - 
                LineCursor.LineWidthWithoutTrailingSpaces / 2) + LineCursor.LeftMargin);

            if (pDocView->IsAutoSizeX() || pDocView->GetTextAutoSize() != GFxTextDocView::TAS_None)
            {
                NeedRecenterLines = true;
            }
            break;
        }
    default:                                    
        pcurLine->SetAlignment(GFxTextLineBuffer::Line::Align_Left); 
        break;
    }

    ParaWidth    = GTL::gmax(ParaWidth, lineOffsetX + LineCursor.LineWidth);
    ParaHeight   = SInt(NextOffsetY + lineHeight - ParaYOffset);
    NextOffsetY  += lineHeight + leading;
    LineCursor.Indent   = 0; // reset indent
    LineCursor.GlyphIns.Reset();
    LineCursor.LastColor = 0;
    LineCursor.pLastFont = NULL;
}

bool GFxParagraphFormatter::HandleCustomWordWrap()
{
    // if this is the first pass and document listener handles OnLineFormat
    // then invoke this handler and run the second path if necessary.
    if (Pass == 1 && HasLineFormatHandler)
    {
        Float lwBeforeWW;
        GFxFontResource* pfont;
        UPInt wordWrapPos;
        if (WordWrapPoint.IsFinished())
        {
            lwBeforeWW  = (Float)LineCursor.LineWidth;
            pfont       = LineCursor.pLastFont->pFont;
            wordWrapPos = LineCursor.NumChars; 
        }
        else
        {
            lwBeforeWW  = (Float)WordWrapPoint.LineWidth;
            pfont       = WordWrapPoint.pLastFont->pFont;
            wordWrapPos = WordWrapPoint.NumChars; 
        }
        // first of all we need to fill out LineFormatDesc structure.
        GFxTextDocView::LineFormatDesc desc;
        desc.pParaText      = pTextBufForCustomFormat;
        desc.ParaTextLen    = TextBufLen;
        desc.LineStartPos   = pTempLine->GetTextPos() - pParagraph->GetStartIndex();
        desc.NumCharsInLine = LineCursor.NumChars;

        desc.VisibleRectWidth = (Float)TextRectWidth;  // [in] width of client rect
        desc.CurrentLineWidth = (Float)LineCursor.LineWidth;   // [in] current line width
        desc.LineWidthBeforeWordWrap = lwBeforeWW;// [in] line width till ProposedWordWrapPoint
        int dashGlyphIndex    = pfont->GetGlyphIndex('-');
        if (dashGlyphIndex > 0)
            desc.DashSymbolWidth  = pfont->GetGlyphWidth(dashGlyphIndex);                      // [in] may be used to calculate hyphenation

        desc.ProposedWordWrapPoint  = wordWrapPos;  // [in,out] text index of proposed word wrap pos
        Float widths[256], *pwidths = widths;
        if (desc.NumCharsInLine + 1 > sizeof(widths)/sizeof(widths[0]))
        {
            pwidths = (Float*)pDocView->GetAllocator()->AllocRaw(sizeof(Float)*(desc.NumCharsInLine + 1));
        }
        UInt i = 0;
        UInt numGlyphs;
        GFxTextLineBuffer::GlyphEntry* pglyphs = LineCursor.GlyphIns.GetReadyGlyphs(&numGlyphs);
        Float deltaW = 0;
        // Skip leading zero-length chars and calc delta width
        for (; i < numGlyphs; ++i)
        {
            if (pglyphs[i].GetLength() == 0)
                deltaW += pglyphs[i].GetAdvance();
            else
                break;

        }
        desc.pWidths                    = pwidths;
        desc.VisibleRectWidth           -= deltaW;
        desc.CurrentLineWidth           -= deltaW;
        desc.LineWidthBeforeWordWrap    -= deltaW;
        // fill pwidths array with widths of every char
        Float curW = 0;
        UPInt p;
        for (p = 0; i < numGlyphs; ++i)
        {
            pwidths[p] = curW;
            curW += (Float)pglyphs[i].GetAdvance();
            UInt l = pglyphs[i].GetLength();
            if (l > 1)
            {
                for (UInt j = 0; j < l; ++j)
                {
                    pTextBufForCustomFormat[p + j] = GFX_NBSP_CHAR_CODE;
                    pwidths[p + j] = 0;
                }
            }
            p += l;
        }
        pwidths[p] = curW;
        desc.UseHyphenation = false;
        bool handlerSucceeded = pDocView->pDocumentListener->View_OnLineFormat(*pDocView, desc);
        if (handlerSucceeded)
        {
            //GASSERT(desc.ProposedWordWrapPoint > pTempLine->GetTextPos());
            //GASSERT(desc.ProposedWordWrapPoint < pTempLine->GetTextPos() + LineCursor.NumChars);

            HyphenationRequested = desc.UseHyphenation;
            // if returned true then format was handled and need to update
            // format data according to desc data.
            if (desc.ProposedWordWrapPoint != wordWrapPos)
            {
                if (!WordWrapPoint.IsFinished() && desc.ProposedWordWrapPoint > WordWrapPoint.NumChars)
                {
                    LineCursor.GlyphIns.ResetTo(WordWrapPoint.GlyphIns);
                    LineCursor = WordWrapPoint;
                }
                else if (!HalfPoint.IsFinished() && desc.ProposedWordWrapPoint > HalfPoint.NumChars)
                {
                    LineCursor.GlyphIns.ResetTo(HalfPoint.GlyphIns);
                    LineCursor = HalfPoint;
                }
                else
                {
                    LineCursor.GlyphIns.ResetTo(StartPoint.GlyphIns);
                    LineCursor = StartPoint;
                }
                Pass = 2;
                RequestedWordWrapPos = (UInt)desc.ProposedWordWrapPoint;
                DeltaText = 0; //don't increment
            }
        }
        if (pwidths != widths)
            pDocView->GetAllocator()->FreeRaw(pwidths);
        StartPoint = HalfPoint = GFxLineCursor();

        // if View_OnLineFormat returned true this means that word wrap position has been
        // moved and the second Pass is required. Unless, the new word wrap point is equal
        // to the proposed earlier.
        if (handlerSucceeded && Pass == 2)
            return true;
    }
    else if (Pass == 2)
    {
        WordWrapPoint = GFxLineCursor();
        Pass = 1;
    }

    // add hyphenation mark
    if (HyphenationRequested)
    {
        GFxTextLineBuffer::GlyphEntry& grec = LineCursor.GlyphIns.GetGlyph();
        SInt lastAdvance = AlignTwipsToPixel(LineCursor.LastAdvance);
        if (LineCursor.pPrevGrec)
        {
            LineCursor.pPrevGrec->SetAdvance(lastAdvance);
        }
        int dashGlyphIndex = LineCursor.pLastFont->pFont->GetGlyphIndex('-');
        grec.SetIndex(dashGlyphIndex);
        grec.SetLength(0);
        grec.SetFontSize(FontSize);
        //grec.SetAdvance(LineCursor.pLastFont->GetAdvance(dashGlyphIndex))
        //grec.SetAdvance((SInt)(LineCursor.pLastFont->pFont->GetAdvance(dashGlyphIndex) * scale));
        LineCursor.LineWidth        = NewLineWidth;
        Float dashGlyphAdvance = LineCursor.pLastFont->pFont->GetAdvance(dashGlyphIndex) * Scale;
        GRectF dashGlyphBounds;
        SInt  dashGlyphWidth   = AlignTwipsToPixel(GTL::gmax(
            pFont->GetGlyphBounds(dashGlyphIndex, &dashGlyphBounds).Right * Scale + 20, dashGlyphAdvance));
        LineCursor.LastGlyphWidth   = (SInt)(LineCursor.pLastFont->pFont->GetGlyphWidth(dashGlyphIndex) * Scale);
        LineCursor.LastAdvance      = (Float)dashGlyphWidth;
        LineCursor.LastGlyphWidth   = dashGlyphWidth;
        LineCursor.LastGlyphIndex   = dashGlyphIndex;
        LineCursor.NumOfTrailingSpaces = 0;
        LineCursor.LineWidthWithoutTrailingSpaces = NewLineWidth + dashGlyphWidth;
        HyphenationRequested = false;
        LineCursor.pPrevGrec = &grec;
        ++LineCursor.GlyphIns;
    }
    return false;
}

bool GFxParagraphFormatter::CheckWordWrap()
{
    // Do word-wrapping?
    // use a real width of last glyph to test for intersection with viewrect
    if ((Pass == 1 && (pDocView->IsWordWrap() && !isSpace && 
        NewLineWidth + AdjLineWidth + LineCursor.Indent + LineCursor.LeftMargin > 
        TextRectWidth - LineCursor.RightMargin)) || 
        (Pass == 2 && LineCursor.NumChars == RequestedWordWrapPos))
    {
        if (HandleCustomWordWrap())
            return true;

        // create a new line by incrementing 'currentLineNum'
        NewLineWidth = 0;

        UPInt newTextPos;
        bool wordWrap = false;

        // if wordWrap - cut the last word
        if (!WordWrapPoint.IsFinished())
        {
            GFxTextLineBuffer::GlyphInserter ins = WordWrapPoint.GlyphIns;
            LineCursor.GlyphIns.ResetTo(ins);

            LineCursor              = WordWrapPoint;
            isSpace                 = false;
            wordWrap                = true;
            DeltaText               = 0;
        }
        WordWrapPoint = GFxLineCursor();
        newTextPos = pParagraph->GetStartIndex() + (*LineCursor).Index;

        // mark the separator char at the end of the previous line
        if (LineCursor.pPrevGrec)
        {
            LineCursor.pPrevGrec->SetWordWrapSeparator();
        }

        // finalize the line
        FinalizeLine();

        // reset values for the next line
        LineCursor.Reset();
        TabStopsIndex = 0; // reset tabStops index. Actually Flash 8 doesn't do this
        // Flash 8 applies tabStops only to the first line.
        // Flash 9 resets tabStops index for each line.

        GASSERT(pTempLine);

        pTempLine->SetTextPos((UInt)newTextPos);

        if (wordWrap)
        {
            LineCursor.LineWidth = 0;
            return true;
        }
    }
    return false;
}

// Format the paragraph into temporary line buffer
void GFxParagraphFormatter::Format(const GFxTextParagraph& paragraph)
{
    InitParagraph(paragraph);

    if (pParaFormat->IsBullet())
    {
        // add bullet symbol.
        GFxTextLineBuffer::GlyphEntry& grec = LineCursor.GlyphIns.GetGlyph();

        const GFxTextParagraph::CharacterInfo& chInfo = *LineCursor;
        FindFontInfo.pCurrentFormat = chInfo.pFormat;
        if (FindFontInfo.pCurrentFormat)
        {
            // it is possible that there is no format info if paragraph
            // is empty.
            GPtr<GFxFontHandle> pfontHandle = pDocView->FindFont(&FindFontInfo);
            GASSERT(pfontHandle);

            int glyphIndex = pfontHandle->GetFont()->GetGlyphIndex(GFX_BULLET_CHAR_CODE);
            grec.SetIndex(glyphIndex);
            grec.SetAdvance(PixelsToTwips(GFX_BULLET_CHAR_ADVANCE));
            Float fontSize = GetActualFontSize()*2/3; 
            grec.SetFontSize(fontSize);
            grec.SetLength(0);

            UInt32 color = FindFontInfo.pCurrentFormat->GetColor32();
            LineCursor.GlyphIns.AddFont(pfontHandle);
            LineCursor.GlyphIns.AddColor(color);
            ++LineCursor.GlyphIns;

            LineCursor.pLastFont    = pfontHandle;
            LineCursor.LastColor    = color;
        }
        LineCursor.Indent       = PixelsToTwips(GFX_BULLET_PARA_INDENT);
        LineCursor.LineWidth    = PixelsToTwips(GFX_BULLET_CHAR_ADVANCE);
    }
    else
        LineCursor.Indent  = PixelsToTwips(pParaFormat->GetIndent());
    LineCursor.LeftMargin  = PixelsToTwips(pParaFormat->GetLeftMargin() + pParaFormat->GetBlockIndent());
    LineCursor.RightMargin = PixelsToTwips(pParaFormat->GetRightMargin());
    const UInt* const ptabStops = pParaFormat->GetTabStops(&TabStopsNum);

    GPtr<GFxTextImageDesc> pimgDesc;
    for(;!LineCursor.IsFinished(); LineCursor += DeltaText)
    {
        if (Pass == 1 && pDocView->IsWordWrap() && !pimgDesc)
        {
            if (LineCursor.LastCharCode == '-' || isSpace) // also a word separator
            {
                WordWrapPoint = LineCursor;
                //GASSERT(WordWrapPoint.LastCharCode != 5760); //@DBG
            }
        }
        // save control points, if necessary
        if (Pass == 1 && HasLineFormatHandler)
        {
            Float w = TextRectWidth;
            if (LineCursor.LineWidth == 0 && !StartPoint.pDocView)
                StartPoint = LineCursor;
            if (LineCursor.LineWidth > w/2) 
            {
                if (!HalfPoint.pDocView)  
                    HalfPoint = LineCursor;
            }
            GASSERT(!StartPoint.IsFinished() || StartPoint.NumChars == 0);
        }

        const GFxTextParagraph::CharacterInfo& chInfo = *LineCursor;
        DeltaText = 1;
        //GASSERT(chInfo.Character != 6158); //@DBG

        pimgDesc = NULL;
        if (pDocView->pImageSubstitutor)
        {
            UPInt  remlen;
            const wchar_t* premainingStr = LineCursor.CharIter.GetRemainingTextPtr(&remlen);
            UPInt  textLen;
            pimgDesc = pDocView->pImageSubstitutor->FindImageDesc(premainingStr, remlen, &textLen);
            if (pimgDesc)
            {
                DeltaText = (UInt)textLen;
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

        pFont = NULL;
        Scale = 1.f;
        FontSize = -1;
        if (!pimgDesc)
        {
            isNbsp  = (chInfo.Character == GFX_NBSP_CHAR_CODE);
            isSpace = (chInfo.Character == 0 || (!isNbsp && gfc_iswspace(chInfo.Character) != 0));

            if (FindFontInfo.pCurrentFont && FindFontInfo.pCurrentFormat == chInfo.pFormat) // is the format ptr same as previous one?
            {
                // if yes, just use FindFontInfo.CurrentFormat, since the default text format is same.
                pFontHandle = FindFontInfo.pCurrentFont;
            }
            else
            {
                FindFontInfo.pCurrentFormat = chInfo.pFormat;

                pFontHandle = pDocView->FindFont(&FindFontInfo);
                GASSERT(pFontHandle);
                if (!pFontHandle) 
                    continue;
            }
            pFont = pFontHandle->GetFont();
            FontSize = GetActualFontSize(); 
            Scale = PixelsToTwips(FontSize) / 1024.0f; // the EM square is 1024 x 1024   
            if (chInfo.Character == pDocView->NewLineChar() || chInfo.Character == '\0')
            {
                // ok, the end of the paragraph and the line. Add a fake glyph with 
                // half of space's width to the buffer
                GlyphIndex = pFont->GetGlyphIndex(' ');
#ifdef GFX_DEBUG_DISPLAY_INVISIBLE_CHARS
                if (chInfo.Character == '\0')
                    GlyphIndex = pFont->GetGlyphIndex(0xA4);
                else
                    GlyphIndex = pFont->GetGlyphIndex(0xB6);
#endif
                GlyphAdvance = pFont->GetAdvance(GlyphIndex) / 2 * Scale;
            }
            else
            {
                GlyphIndex = pFont->GetGlyphIndex((UInt16)chInfo.Character);
                if (GlyphIndex == -1)
                {
                    // error -- missing glyph!
                    if (pLog && !(pDocView->RTFlags & GFxTextDocView::RTFlags_FontErrorDetected))
                    {
                        // Log an error, but don't log too many times.
                        static int  LogCount = 0;
                        if (LogCount < 10)
                        {
                            LogCount++;
                            pLog->LogError("GFxTextDocView::Format() - missing glyph %d. "
                                "Make sure that SWF file includes character shapes for \"%s\" font.\n",
                                chInfo.Character,
                                pFont->GetName());
                        }
                        //GFxFontManager::FontSearchPathInfo searchInfo(1);
                        //GPtr<GFxFontHandle> h = *pDocView->GetFontManager()->
                        //    CreateFontHandle(FindFontInfo.pCurrentFormat->GetFontList(), 
                        //        FindFontInfo.pCurrentFormat->IsBold(), 
                        //        FindFontInfo.pCurrentFormat->IsItalic(),
                        //        pDocView->DoesUseDeviceFont(), !FindFontInfo.pCurrentFormat->IsSingleFontNameSet(), &searchInfo);
                        //GFxString charPath = pDocView->GetDocumentListener()? pDocView->GetDocumentListener()->GetCharacterPath(): GFxString();
                        //GFxString charRanges = h->GetFont()->GetFont()->GetCharRanges();
                        //UInt glyphsCount = h->GetFont()->GetFont()->GetGlyphShapeCount();
                        //pLog->LogError("Missing \"%s\" glyph \'%c\' (0x%x) in \"%s\".\n"
                        //    "Font has %u glyphs, ranges %s.\nSearch log: \n%s",
                        //    FindFontInfo.pCurrentFormat->GetFontList().ToCStr(), (char)chInfo.Character, chInfo.Character, 
                        //    charPath.ToCStr(), glyphsCount,
                        //    charRanges.ToCStr(), searchInfo.Info.ToCStr());
                        pDocView->RTFlags |= GFxTextDocView::RTFlags_FontErrorDetected;

                        // Drop through and use index == -1; this will display
                        // using the empty-box glyph
                    }
                }

                GlyphAdvance = pFont->GetAdvance(GlyphIndex) * Scale;
            }

            // if kerning for previous glyph was on - modify its advance and the width of line
            if (LineCursor.pPrevGrec)
            {
                Float prevGlyphAdvanceAdj = 0;
                if (LineCursor.LastKerning)
                    prevGlyphAdvanceAdj = LineCursor.GetPrevGlyphAdvanceAdj(chInfo.Character) * Scale;

                LastAdvance = AlignTwipsToPixel(LineCursor.LastAdvance + prevGlyphAdvanceAdj);
            }
            else
            {
                LastAdvance = AlignTwipsToPixel(LineCursor.LastAdvance);
            }
            GlyphWidth   = AlignTwipsToPixel(GlyphAdvance);
            //!AB: The code below is replaced by the code above, because using scaled right
            // boundary as a width of glyph may cause incorrect text positioning, especially
            // in the case of right-aligned text.
            //GRectF glyphBounds;
            //GlyphWidth   = AlignTwipsToPixel(GTL::gmax(
            //    pFont->GetGlyphBounds(GlyphIndex, &glyphBounds).Right * Scale + 20, GlyphAdvance));

            // the space shouldn't be taken into account when checking the lineWidth for
            // intersection with viewrect. Though, the space's width still should be added
            // to the lineWidth, even if it is a "word-wrapping" point.
            AdjLineWidth = (isSpace) ? 0 : GlyphWidth;
        }
        else
        {
            FindFontInfo.pCurrentFormat = chInfo.pFormat;
            GlyphWidth      = AlignTwipsToPixel(pimgDesc->GetScreenWidth());
            GlyphAdvance    = GlyphWidth + 40.f;
            LastAdvance     = AlignTwipsToPixel(LineCursor.LastAdvance);
            AdjLineWidth    = GlyphWidth;
            GlyphIndex      = ~0u;
            isSpace         = false;
            isNbsp          = false;
        }

        NewLineWidth = LineCursor.LineWidth + LastAdvance;

        // Do word-wrapping?
        if (CheckWordWrap())
            continue;

        if (LineCursor.pPrevGrec)
        {
            LineCursor.pPrevGrec->SetAdvance(LastAdvance);
        }

        GFxTextLineBuffer::GlyphEntry& grec = LineCursor.GlyphIns.GetGlyph();
        grec.SetIndex(GlyphIndex);
        if (LineCursor.IsInsideComposStr())
            grec.SetComposStrGlyph();
        // Advance will be set at the next iteration or in Finalize, taking
        // kerning into account.
        //grec.SetAdvance(GlyphAdvance);

        if (pimgDesc)
        {
            LineCursor.GlyphIns.AddImage(pimgDesc);
            grec.SetLength(DeltaText);
            LineCursor.LastKerning = false;

            GPointF p(0,0);
            p = pimgDesc->Matrix.Transform(p);
            LineCursor.MaxFontAscent  = GTL::gmax(LineCursor.MaxFontAscent,  -p.y);
            LineCursor.MaxFontDescent = GTL::gmax(LineCursor.MaxFontDescent, pimgDesc->GetScreenHeight() - -p.y);

            LineCursor.NumOfTrailingSpaces = 0;
            LineCursor.LineWidthWithoutTrailingSpaces = NewLineWidth + AdjLineWidth;
        }
        else
        {
            UInt32 color  = FindFontInfo.pCurrentFormat->GetColor32();
            //Float FontSize = FindFontInfo.pCurrentFormat->GetFontSize(LineCursor.FontScaleFactor);
            GASSERT(FontSize >= 0);
            grec.SetFontSize(FontSize);

            if (pFontHandle != LineCursor.pLastFont)
                LineCursor.GlyphIns.AddFont(pFontHandle);
            if (color != LineCursor.LastColor)
                LineCursor.GlyphIns.AddColor(color);

            if (chInfo.Character == pDocView->NewLineChar() || chInfo.Character == '\0')
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
                GlyphWidth = 0;
                LineCursor.LineHasNewLine = true;

                // gather dimensions for the line, ONLY if this is the first char
                if (chInfo.Index == 0)
                    LineCursor.TrackFontParams(pFont, Scale);

            }
            else
            {
                if (isSpace || isNbsp)
                    grec.SetInvisibleChar();
                if (isSpace)
                {
                    ++LineCursor.NumOfSpaces;
                    ++LineCursor.NumOfTrailingSpaces;
                }
                else
                {
                    LineCursor.NumOfTrailingSpaces = 0;
                    LineCursor.LineWidthWithoutTrailingSpaces = NewLineWidth + AdjLineWidth;
                }
                grec.SetLength(1);

                // gather dimensions for the line
                LineCursor.TrackFontParams(pFont, Scale);

            }
            if (FindFontInfo.pCurrentFormat->IsUnderline())
                grec.SetUnderline();
            else
                grec.ClearUnderline();
            if (FindFontInfo.pCurrentFormat->IsUrlSet())
                grec.SetUrl();
            else
                grec.ClearUrl();

            LineCursor.pLastFont        = pFontHandle;
            LineCursor.LastColor        = color;
            LineCursor.LastKerning      = FindFontInfo.pCurrentFormat->IsKerning();
        }

        // check for tabs
        if (TabStopsIndex < TabStopsNum && chInfo.Character == '\t')
        {
            SInt nextTabStopPos = (SInt)PixelsToTwips(ptabStops[TabStopsIndex]);
            if (nextTabStopPos > NewLineWidth)
            {
                GlyphAdvance = Float(nextTabStopPos - NewLineWidth);
            }
            ++TabStopsIndex;
        }

        LineCursor.LineLength += grec.GetLength();

        LineCursor.pPrevGrec        = &grec;
        LineCursor.LastCharCode     = chInfo.Character;
        LineCursor.LineWidth        = NewLineWidth;

        LineCursor.LastGlyphIndex   = GlyphIndex;
        LineCursor.LastAdvance      = GlyphAdvance;
        if (FindFontInfo.pCurrentFormat->IsLetterSpacingSet())
            LineCursor.LastAdvance  += PixelsToTwips(FindFontInfo.pCurrentFormat->GetLetterSpacing());
        LineCursor.LastGlyphWidth   = GlyphWidth;

        ++LineCursor.GlyphIns;
    }

    if (pTempLine)
    {
        // finalize the last line
        FinalizeLine();
    }
    if (pTextBufForCustomFormat && pTextBufForCustomFormat != TextBufForCustomFormat)
        pDocView->GetAllocator()->FreeText(pTextBufForCustomFormat);
}

void GFxTextDocView::Format()
{
    GFxStyledText* pstyledText = GetStyledText();

    UInt oldMaxHScroll = GetMaxHScrollValue();
    UInt oldMaxVScroll = GetMaxVScrollValue();

    // if auto text fit feature is on, then we need to reformat everything
    // to be able to scale it correctly.
    if (GetTextAutoSize() != TAS_None)
        SetCompleteReformatReq();

    GFxStyledText::ParagraphsIterator paraIter = pstyledText->GetParagraphIterator();
    GFxTextLineBuffer::Iterator linesIt = LineBuffer.Begin();

    GFxParagraphFormatter formatter(this, pLog);
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
                    curLine.SetOffsetY(formatter.NextOffsetY);

                    textWidth  =  GTL::gmax(textWidth, curLine.GetWidth());
                    // In Flash, the last empty line doesn't participate in textHeight,
                    // unless this is editable text.
                    if (ppara->GetLength() != 0 || (HasEditorKit() && !GetEditorKit()->IsReadOnly()))
                        textHeight =  SInt(formatter.NextOffsetY + curLine.GetHeight());

                    formatter.NextOffsetY += curLine.GetHeight() + curLine.GetLeading();

                    // recenter line, if necessary
                    if (IsAutoSizeX())
                    {
                        switch(curLine.GetAlignment())
                        {
                        case GFxTextLineBuffer::Line::Align_Right:
                            curLine.SetOffsetX(GTL::gmax(0.f, GetTextRect().Width() - curLine.GetWidth()));

                            // still need to set NeedRecenterLines to true, in the case if view rect width will be changed
                            formatter.NeedRecenterLines = true; 
                            break;
                        case GFxTextLineBuffer::Line::Align_Center:
                            curLine.SetOffsetX(GTL::gmax(0.f, GetTextRect().Width() / 2 - curLine.GetWidth() / 2));

                            // still need to set NeedRecenterLines to true, in the case if view rect width will be changed
                            formatter.NeedRecenterLines = true;
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
        formatter.pLinesIter     = &linesIt;
        formatter.ParaYOffset    = formatter.NextOffsetY;
        formatter.Format(*ppara);
        LineBuffer.InvalidateCache(); // force batching to be rebuilt

        textWidth  =  GTL::gmax(textWidth, formatter.ParaWidth);

        // In Flash, the last empty line doesn't participate in textHeight,
        // unless this is editable text.
        if (ppara->GetLength() != 0 || (HasEditorKit() && !GetEditorKit()->IsReadOnly()))
            textHeight = SInt(formatter.ParaYOffset + formatter.ParaHeight);

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
    if (IsAutoSizeX() || IsAutoSizeY() || formatter.ForceVerticalCenterAutoSize)
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

        if (IsAutoSizeY() || formatter.ForceVerticalCenterAutoSize)
        {
            Float newH = TextHeight + GFX_TEXT_GUTTER*2;
            if ((valign == VAlign_None && formatter.ForceVerticalCenterAutoSize) || valign == VAlign_Center)
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
    // check if we need to recenter lines. Usually we need to recenter lines in two cases:
    // 1) Text has alignment "center" or "right" and autoSizeX is true;
    // 2) Text has alignment "center" or "right" and textAutoSize is used.
    if (formatter.NeedRecenterLines)
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

    GASSERT(!IsReformatReq());

    // also, we need to correct scrolling values, if they are out of range after re-formatting
    UInt maxHScroll = GetMaxHScroll();
    bool needOnScroller = false;
    if(GetHScrollOffsetValue() > maxHScroll)
    {
        SetHScrollOffset(maxHScroll);
    }
    else if (maxHScroll != oldMaxHScroll)
    {
        needOnScroller = true;   
    }
    UInt maxVScroll = GetMaxVScroll();
    if(GetVScrollOffsetValue() > maxVScroll)
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
                                bool nextFrame)
{
    if (IsReformatReq())
    {
        Format();
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
    if (hscroll <= GetMaxHScroll() || hscroll < GetHScrollOffsetValue())
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

UInt    GFxTextDocView::GetHScrollOffset()
{
    // check if reformatting is pending. If yes - force reformat first
    if (IsReformatReq())
    {
        Format();
        ClearReformatReq();
    }
    return GetHScrollOffsetValue();
}

UInt    GFxTextDocView::GetVScrollOffset()
{
    // check if reformatting is pending. If yes - force reformat first
    if (IsReformatReq())
    {
        Format();
        ClearReformatReq();
    }
    return GetVScrollOffsetValue();
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
    SInt editorDelta = (HasEditorKit() && !GetEditorKit()->IsReadOnly()) ? GFX_EDIT_HSCROLL_DELTA : 0;
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

            const GFxTextLineBuffer::Line* pbottomLine = linesIt.GetPtr();
            if (pbottomLine->GetTextLength() == 0) // Flash doesn't count the last empty line
            {
                --linesIt;
                ++l;
            }
            if (!linesIt.IsFinished())
            {
                pbottomLine = linesIt.GetPtr(); // it might be changed above
                SInt lineH = pbottomLine->GetHeight() + pbottomLine->GetNonNegLeading();
                Float top = textRect.Top + (pbottomLine->GetOffsetY() + lineH - textRect.Bottom);

                for(; !linesIt.IsFinished(); --linesIt, ++l)
                {
                    const GFxTextLineBuffer::Line& line = *linesIt;

                    if (l > 0 && line.GetOffsetY() < top)
                        break;
                }
                // MaxVScroll is zero based.
                MaxVScroll.SetValue(LineBuffer.size() - l, FormatCounter);
            }
            else
                MaxVScroll.SetValue(0, FormatCounter);
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
bool GFxTextDocView::IsUrlAtPoint(Float x, Float y, GRange* purlPosRange)
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
            if (rv && purlPosRange)
            {
                purlPosRange->Index = 0;
                purlPosRange->Length = 0;
                // retrieve position range for the url
                // searching from the position (i + line.GetTextPos()) in both directions
                UPInt posInDoc = i + line.GetTextPos();
                UPInt indexInPara = 0;
                GFxStyledText::ParagraphsIterator paraIter = pDocument->GetParagraphByIndex(posInDoc, &indexInPara);
                if (!paraIter.IsFinished())
                {
                    GFxTextParagraph* ppara = *paraIter;
                    GASSERT(ppara);
                    GFxTextFormat formatAtThePoint = ppara->GetTextFormat(indexInPara, indexInPara+1);
                    GASSERT(formatAtThePoint.IsUrlSet());
                    GFxTextParagraph::FormatRunIterator it = ppara->GetIterator();
                    for (; !it.IsFinished(); ++it)
                    {
                        UPInt indexInDoc = (UPInt)it->Index + ppara->GetStartIndex();
                        if (it->pFormat->IsUrlSet() && it->pFormat->GetUrl() == formatAtThePoint.GetUrl())
                        {
                            if ((UPInt)purlPosRange->Index + purlPosRange->Length < indexInDoc)
                            {
                                if (indexInDoc > posInDoc)
                                    break;
                                purlPosRange->Index  = indexInDoc;
                                purlPosRange->Length = it->Length;
                            }
                            else
                            {
                                purlPosRange->Length += it->Length;
                            }
                        }
                        else
                        {
                            if (indexInDoc > posInDoc)
                                break;
                            purlPosRange->Index = 0;
                            purlPosRange->Length = 0;
                        }
                    }
                }
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

    Float newHScrollOffset = Float(document.GetHScrollOffsetValue());
    Float newVScrollOffset = Float(document.GetVScrollOffsetValueInTwips());

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

        UPInt firstVisibleChar = document.GetLineOffset(document.GetVScrollOffsetValue());

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


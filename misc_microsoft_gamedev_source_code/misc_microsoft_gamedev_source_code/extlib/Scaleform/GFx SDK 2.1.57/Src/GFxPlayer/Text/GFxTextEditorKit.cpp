/**********************************************************************

Filename    :   GFxTextEditorKit.cpp
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

#include "Text/GFxTextEditorKit.h"

#define GFX_CLAUSE_HIGHLIGHTING_INDEX       (GFX_TOPMOST_HIGHLIGHTING_INDEX - 2)

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
            UInt lineIndex = pDocView->GetVScrollOffsetValue();

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

        // Check, is the cursor fully visible or not. If not - scroll horizontally. 
        // Note, if text auto size mode is on, then scrolling is forbidden.
        if (!textRect.Contains(newCursorRect) && pDocView->GetTextAutoSize() == GFxTextDocView::TAS_None)
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
            UInt fvline = pDocView->GetVScrollOffsetValue();
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
                    UInt lineIndex = pDocView->GetVScrollOffsetValue();
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
                        UInt numLinesOnScreen = bottomV - pDocView->GetVScrollOffsetValue() + 1;
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
                        UInt numLinesOnScreen = bottomV - pDocView->GetVScrollOffsetValue() + 1;
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
            case GFxTextKeyMap::KeyAct_Cut:
                if (pClipboard)
                {
                    if (IsReadOnly() || pke->Action == GFxTextKeyMap::KeyAct_Copy)
                    {
                        ClearShiftPressed();
                        CopyToClipboard(beginSel, endSel, DoesUseRichClipboard());
                        noActions = true;
                    }
                    else
                    {
                        ClearShiftPressed();
                        CutToClipboard(beginSel, endSel, DoesUseRichClipboard());
                        newPos = GTL::gpmin(beginSel, endSel);
                        needChangeNotify = true;
                    }
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
        GFxWStringBuffer wbuf;
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
        if (!IsReadOnly())
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

#if 0 //ndef GFC_NO_IME_SUPPORT
//////////////////////////////////////////////////////////////////////////
// GFxTextCompositionString
//
GFxTextCompositionString::GFxTextCompositionString(GFxTextEditorKit* peditorKit) :
    pEditorKit(peditorKit), pAllocator(pEditorKit->GetDocument()->GetAllocator()), 
    String(peditorKit->GetDocument()->GetAllocator()),
    CursorPos(0), HasHighlightingFlag(false), HighlightIdsUsed(0)
{
    GFxTextFormat fmt; // create empty text format
    const GFxTextFormat* pdefFmt = peditorKit->GetDocument()->GetDefaultTextFormat();
    if (!pdefFmt)
        pdefFmt = &fmt;
    pDefaultFormat = *GetAllocator()->AllocateTextFormat(*pdefFmt);

    String.AppendTermNull(GetAllocator(), pDefaultFormat);

    // create highlighting for composition string

    GFxTextHighlighter* phighlighter = pEditorKit->GetDocument()->CreateHighlighterManager();
    GASSERT(phighlighter);
    for (UPInt i = 0; i < sizeof(HighlightIds)/sizeof(HighlightIds[0]); i++)
    {
        GFxTextHighlightDesc* pdesc;
        GFxTextHighlightDesc desc;
        desc.StartPos = 0;
        desc.Offset   = 0;
        desc.Length   = 0;
        pdesc = phighlighter->CreateNewHighlighter(&desc);
        GASSERT(pdesc);
        HighlightIds[i] = pdesc->Id;
    }

    // set default styles
    Styles = GetDefaultStyles();
}

GFxTextCompositionString::~GFxTextCompositionString()
{
    GFxTextHighlighter* phighlighter = pEditorKit->GetDocument()->CreateHighlighterManager();
    GASSERT(phighlighter);
    for (UPInt i = 0; i < sizeof(HighlightIds)/sizeof(HighlightIds[0]); i++)
    {
        phighlighter->FreeHighlighter(HighlightIds[i]);
    }
    String.FreeText(GetAllocator());
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
    lcsinfo.SetUnderlineColor(GColor(0,255,0));
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
    ClearHighlighting();
    String.Clear();
    String.SetText(GetAllocator(), pwstr, nchars);
    String.SetTextFormat(GetAllocator(), *pDefaultFormat, 0, String.GetSize());
    String.AppendTermNull(GetAllocator(), pDefaultFormat);

    String.CheckIntegrity();
    Reformat();
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
    HasHighlightingFlag = true;
    GASSERT(HighlightIdsUsed < sizeof(HighlightIds)/sizeof(HighlightIds[0]));
    if (HighlightIdsUsed < sizeof(HighlightIds)/sizeof(HighlightIds[0]))
    {
        GFxTextHighlighter* phighlighter = pEditorKit->GetDocument()->CreateHighlighterManager();
        GASSERT(phighlighter);
        GFxTextHighlightDesc* pdesc = phighlighter->GetHighlighterPtr(HighlightIds[HighlightIdsUsed++]);
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
}

void  GFxTextCompositionString::UseStyles(const GFxTextIMEStyle& styles)
{
    Styles.Unite(styles);
}

void  GFxTextCompositionString::ClearHighlighting() 
{ 
    HasHighlightingFlag = false; 

    GFxTextHighlighter* phighlighter = pEditorKit->GetDocument()->CreateHighlighterManager();
    GASSERT(phighlighter);
    for (UPInt i = 0; i < HighlightIdsUsed; i++)
    {
        GFxTextHighlightDesc* pdesc = phighlighter->GetHighlighterPtr(HighlightIds[i]);
        GASSERT(pdesc);
        pdesc->StartPos = 0;
        pdesc->Length   = 0;
        pEditorKit->GetDocument()->UpdateHighlight(*pdesc);
    }
    HighlightIdsUsed = 0;
}
#endif //#ifndef GFC_NO_IME_SUPPORT


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

    String.AppendTermNull(GetAllocator(), pDefaultFormat);

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

GFxTextAllocator*   GFxTextCompositionString::GetAllocator() const  
{ 
    return pEditorKit->GetDocument()->GetAllocator(); 
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
    String.SetText(GetAllocator(), pwstr, nchars);
    String.SetTextFormat(GetAllocator(), *pDefaultFormat, 0, String.GetSize());
    String.AppendTermNull(GetAllocator(), pDefaultFormat);

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

/*********************************************************************

Filename    :   GFxTextLineBuffer.cpp
Content     :   Text line buffer
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
#include "GFxDisplayContext.h"
#include "GFxFontCacheManager.h"
#include "GFxTextLineBuffer.h"

#ifdef GFC_BUILD_DEBUG
//#define DEBUG_MASK
//#define DEBUG_LINE_WIDTHS
//#define DEBUG_VIS_RECT
#endif //GFC_BUILD_DEBUG

GFxTextLineBuffer::Line* GFxTextLineBuffer::TextLineAllocator::AllocLine(UInt size, GFxTextLineBuffer::LineType lineType)
{
    //TODO: look for already allocated line in FreeLinesPool first
    Line* pline = (Line*)GALLOC(size);
    pline->SetMemSize(size);
    if (lineType == Line8)
        pline->InitLine8();
    else
        pline->InitLine32();
    return pline;
}

void GFxTextLineBuffer::TextLineAllocator::FreeLine(GFxTextLineBuffer::Line* ptr)
{
    //TODO: return line being freed in FreeLinesPool
    if (ptr)
    {
        ptr->Release();
        GFREE(ptr);
    }
}

void GFxTextLineBuffer::ReleasePartOfLine(GlyphEntry* pglyphs, UInt n, GFxFormatDataEntry* pnextFormatData)
{
    for(UInt i = 0; i < n; ++i, ++pglyphs)
    {
        GlyphEntry* pglyph = pglyphs;
        if (pglyph->IsNextFormat())
        {   
            if (pglyph->IsFmtHasFont())
            {
                GFxFontHandle* pfont = pnextFormatData->pFont;
                pfont->Release();
                ++pnextFormatData;
            }
            if (pglyph->IsFmtHasColor())
            {
                ++pnextFormatData;
            }
            if (pglyph->IsFmtHasImage())
            {
                GFxTextImageDesc *pimage = pnextFormatData->pImage;
                pimage->Release();
                ++pnextFormatData;
            }
        }
    }
}

void GFxTextLineBuffer::GlyphInserter::ResetTo(const GlyphInserter& savedPos)
{
    GASSERT(savedPos.GlyphIndex <= GlyphIndex);
    if (savedPos.GlyphIndex < GlyphIndex && GlyphsCount > 0)
    {
        // first of all, we need to release all fonts and image descriptors
        // beyond the savedPos.GlyphIndex; otherwise memory leak is possible.
        
        GlyphEntry* pglyphs = savedPos.pGlyphs + savedPos.GlyphIndex;
        UInt n = GlyphIndex - savedPos.GlyphIndex;
        GFxFormatDataEntry* pnextFormatData 
            = reinterpret_cast<GFxFormatDataEntry*>(savedPos.pNextFormatData + savedPos.FormatDataIndex);
        ReleasePartOfLine(pglyphs, n, pnextFormatData);
    }
    operator=(savedPos);
}

void GFxTextLineBuffer::GlyphIterator::UpdateDesc()
{
    pImage = NULL;
    if (!IsFinished())
    {
        if (pGlyphs->IsNextFormat())
        {   
            if (pGlyphs->IsFmtHasFont())
            {
                pFontHandle = pNextFormatData->pFont;
                ++pNextFormatData;
            }
            if (pGlyphs->IsFmtHasColor())
            {
                OrigColor = Color = pNextFormatData->Color;
                ++pNextFormatData;
            }
            if (pGlyphs->IsFmtHasImage())
            {
                pImage = pNextFormatData->pImage;
                ++pNextFormatData;
            }
        }
        if (pGlyphs->IsUnderline())
        {
            UnderlineStyle  = GFxTextHighlightInfo::Underline_Single;
            UnderlineColor  = Color;
        }
        else
            UnderlineStyle = GFxTextHighlightInfo::Underline_None;

        if (!HighlighterIter.IsFinished())
        {
            // get data from the highlighter
            const GFxTextHighlightDesc& hd = *HighlighterIter;
            
            Color           = OrigColor;
            if ((pGlyphs->GetLength() > 0 || pGlyphs->IsWordWrapSeparator()))
            {
                if (hd.Info.HasTextColor())
                    Color = hd.Info.GetTextColor().ToColor32();

                if (hd.Info.HasUnderlineStyle())
                    UnderlineStyle = hd.Info.GetUnderlineStyle();
                if (hd.Info.HasUnderlineColor())
                    UnderlineColor = hd.Info.GetUnderlineColor().ToColor32();
                else
                    UnderlineColor  = Color;
            }
        }
        else
        {
            if (pGlyphs->IsUnderline())
            {
                UnderlineColor = Color;
                UnderlineStyle  = GFxTextHighlightInfo::Underline_Single;
            }
        }
    }
}

GFxTextLineBuffer::GFxFormatDataEntry* GFxTextLineBuffer::Line::GetFormatData() const 
{ 
    UByte* p = reinterpret_cast<UByte*>(GetGlyphs() + GetNumGlyphs());
    // align to boundary of GFxFormatDataEntry
    size_t pi = ((size_t)p);
    size_t delta = ((pi + sizeof(GFxFormatDataEntry*) - 1) & ~(sizeof(GFxFormatDataEntry*) - 1)) - pi;
    return reinterpret_cast<GFxFormatDataEntry*>(p + delta);
}

void GFxTextLineBuffer::Line::Release()
{
    if (!IsInitialized()) return;

    GlyphEntry* pglyphs = GetGlyphs();
    UInt n = GetNumGlyphs();
    GFxFormatDataEntry* pnextFormatData = GetFormatData();
    ReleasePartOfLine(pglyphs, n, pnextFormatData);
    SetNumGlyphs(0);
#ifdef GFC_BUILD_DEBUG
    memset(((UByte*)this) + sizeof(MemSize), 0xf0, GetMemSize() - sizeof(MemSize));
#endif
}

bool GFxTextLineBuffer::Line::HasNewLine() const
{
    UInt n = GetNumGlyphs();
    if (n > 0)
    {
        GlyphEntry* pglyphs = GetGlyphs();
        return pglyphs[n - 1].IsNewLineChar() && !pglyphs[n - 1].IsEOFChar();
    }
    return false;
}

//////////////////////////////////
// GFxTextLineBuffer
//
GFxTextLineBuffer::GFxTextLineBuffer()
{
    DummyStyle.SetColor(0xFFFFFFFF); // White, modified by CXForm.
    Geom.VisibleRect.Clear();
    pBatchPackage = 0;
    pCacheManager = 0;
    LastHScrollOffset = ~0u;
}

GFxTextLineBuffer::~GFxTextLineBuffer()
{
    if (pCacheManager)
        pCacheManager->ReleaseBatchPackage(pBatchPackage);
    ClearLines();
}

GFxTextLineBuffer::Line* GFxTextLineBuffer::GetLine(UInt lineIdx)
{
    if (lineIdx >= Lines.size())
        return NULL;
    GFxTextLineBuffer::Line* pline = Lines[lineIdx];
    InvalidateCache();
    return pline;
}

const GFxTextLineBuffer::Line* GFxTextLineBuffer::GetLine(UInt lineIdx) const
{
    if (lineIdx >= Lines.size())
        return NULL;
    const GFxTextLineBuffer::Line* pline = Lines[lineIdx];
    return pline;
}

bool GFxTextLineBuffer::DrawMask(GFxDisplayContext &context, 
                                 const GRenderer::Matrix& mat, 
                                 const GRectF& rect, 
                                 GRenderer::SubmitMaskMode maskMode)
{
    GRenderer*  prenderer = context.GetRenderer();

    // Only clear stencil if no masks were applied before us; otherwise
    // no clear is necessary because we are building a cumulative mask.
#ifndef DEBUG_MASK
    prenderer->BeginSubmitMask(maskMode);
#else
    GUNUSED(maskMode);
#endif

    GPointF                 coords[4];
    static const UInt16     indices[6] = { 0, 1, 2, 2, 1, 3 };

    // Show white background + black bounding box.
    //prenderer->SetCxform(cx); //?AB: do we need this for drawing mask?
#ifdef DEBUG_MASK
    GRenderer::Cxform colorCxform;
    GColor color(255, 0, 0, 128);
    colorCxform.M_[0][0] = (1.0f / 255.0f) * color.GetRed();
    colorCxform.M_[1][0] = (1.0f / 255.0f) * color.GetGreen();
    colorCxform.M_[2][0] = (1.0f / 255.0f) * color.GetBlue();
    colorCxform.M_[3][0] = (1.0f / 255.0f) * color.GetAlpha();
    prenderer->SetCxform(colorCxform); //?AB: do we need this for drawing mask?
#endif
    Matrix m(mat);
    GRectF newRect;
    GFx_RecalculateRectToFit16Bit(m, rect, &newRect);
    prenderer->SetMatrix(m);

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

    prenderer->FillStyleColor(GColor(255, 255, 255, 255));
    prenderer->SetVertexData(icoords, 4, GRenderer::Vertex_XY16i);

    // Fill the inside
    prenderer->SetIndexData(indices, 6, GRenderer::Index_16);
    prenderer->DrawIndexedTriList(0, 0, 6, 0, 2);

    // Done
    prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
    prenderer->SetIndexData(0, 0, GRenderer::Index_None);

    // Done rendering mask.
#ifndef DEBUG_MASK
    prenderer->EndSubmitMask();
#endif
    return true;
}

UInt GFxTextLineBuffer::CalcLineSize
    (UInt glyphCount, UInt formatDataElementsCount, GFxTextLineBuffer::LineType lineType)
{
    UInt sz;
    if (lineType == Line8)
        sz = LineData8::GetLineStructSize();
    else
        sz = LineData32::GetLineStructSize();
    sz += sizeof(UInt32); // MemSize
    sz += sizeof(GlyphEntry) * glyphCount;
    // add alignment here
    sz = (sz + sizeof(GFxFormatDataEntry) - 1) & ~(sizeof(GFxFormatDataEntry) - 1);
    sz += sizeof(GFxFormatDataEntry) * formatDataElementsCount;
    return sz;
}


GFxTextLineBuffer::Line* GFxTextLineBuffer::InsertNewLine
    (UInt lineIdx, UInt glyphCount, UInt formatDataElementsCount, GFxTextLineBuffer::LineType lineType)
{
    UInt sz = CalcLineSize(glyphCount, formatDataElementsCount, lineType);
    Line* pline = LineAllocator.AllocLine(sz, lineType);
    if (!pline) return NULL;

    pline->SetNumGlyphs(glyphCount);
    Lines.insert(lineIdx, pline);
    return pline;
}

void GFxTextLineBuffer::RemoveLines(UInt lineIdx, UInt num)
{
    Iterator it = Begin() + lineIdx;
    for(UInt i = 0; i < num && !it.IsFinished(); ++it, ++i)
    {
        LineAllocator.FreeLine(it.GetPtr());
    }
    Lines.removeMultiple(lineIdx, num);
}

UInt   GFxTextLineBuffer::GetVScrollOffsetInTwips() const
{
    Float yOffset = 0;
    if (Geom.FirstVisibleLinePos != 0)
    {
        // calculate y-offset of first visible line
        ConstIterator visIt   = BeginVisible(0);
        ConstIterator linesIt = Begin();
        if (!visIt.IsFinished() && !linesIt.IsFinished())
        {
            yOffset = visIt->GetOffsetY() - linesIt->GetOffsetY();
            GASSERT(yOffset >= 0);
        }
    }
    return UInt(yOffset);
}

void GFxTextLineBuffer::ResetCache() 
{ 
    if (pCacheManager)
        pCacheManager->ReleaseBatchPackage(pBatchPackage);
    pBatchPackage = 0;
}


template<class Matrix>
static Float GFx_CalcHeightRatio(const Matrix& matrix)
{
    Matrix mat2(matrix);
    mat2.M_[0][2] = 0;
    mat2.M_[1][2] = 0;
    GPointF p01 = mat2.Transform(GPointF(0,1));
    GPointF p10 = mat2.Transform(GPointF(1,0));
    return fabsf(GMath2D::CalcLinePointDistance(0, 0, p10.x, p10.y, p01.x, p01.y));
}

void    GFxTextLineBuffer::DrawUnderline
    (Float offx, Float offy, Float singleUnderlineHeight, Float singlePointWidth, const GRectF& hscrolledViewRect, 
     SInt lineLength, GFxTextHighlightInfo::UnderlineStyle style, UInt32 color,
     const Matrix& globalMatrixDir, const Matrix& globalMatrixInv)
{
    // clip offx and lineLength by VisibleRect, since mask might be off
    GPointF topLeft     = hscrolledViewRect.ClampPoint(GPointF(offx, offy));
    GPointF bottomRight;
    if (style == GFxTextHighlightInfo::Underline_Thick || 
        style == GFxTextHighlightInfo::Underline_Thick)
    {
        bottomRight = hscrolledViewRect.ClampPoint(
            GPointF(offx + lineLength, offy + singleUnderlineHeight*2));
    }
    else
        bottomRight = hscrolledViewRect.ClampPoint(
            GPointF(offx + lineLength, offy + singleUnderlineHeight));
    bottomRight.x = floorf(bottomRight.x + 0.5f);
    bottomRight.y = floorf(bottomRight.y + 0.5f);

    if (!pUnderlineDrawing)
        pUnderlineDrawing = *new GFxDrawingContext;
    pUnderlineDrawing->SetFill(color);
    Float dashLen = (style == GFxTextHighlightInfo::Underline_Dotted) ? 2.f : 1.f;
    switch (style)
    {
    case GFxTextHighlightInfo::Underline_Single:
    case GFxTextHighlightInfo::Underline_Thick:
        pUnderlineDrawing->MoveTo(topLeft.x, topLeft.y);
        pUnderlineDrawing->LineTo(bottomRight.x, topLeft.y);
        pUnderlineDrawing->LineTo(bottomRight.x, bottomRight.y);
        pUnderlineDrawing->LineTo(topLeft.x, bottomRight.y);
        pUnderlineDrawing->LineTo(topLeft.x, topLeft.y);
        pUnderlineDrawing->AddPath();
        break;
    case GFxTextHighlightInfo::Underline_Dotted:
    case GFxTextHighlightInfo::Underline_DitheredSingle:
    {
        for (Float x = topLeft.x; x < bottomRight.x; x += singlePointWidth*dashLen*2)
        {
            // for dotted or dithered underlining we need to snap to pixel every pixel
            // since otherwise it doesn't work correctly.
            GPointF p(x, bottomRight.y);
            p   = globalMatrixDir.Transform(p);
            p.x = floorf(p.x + 0.5f);
            p.y = floorf(p.y + 0.5f);
            p    = globalMatrixInv.Transform(p);
            p.x = floorf(p.x + 0.5f);
            p.y = floorf(p.y + 0.5f);
            x = p.x;

            pUnderlineDrawing->MoveTo(p.x, topLeft.y);
            pUnderlineDrawing->LineTo(p.x + singlePointWidth*dashLen, topLeft.y);
            pUnderlineDrawing->LineTo(p.x + singlePointWidth*dashLen, bottomRight.y);
            pUnderlineDrawing->LineTo(p.x, bottomRight.y);
            pUnderlineDrawing->LineTo(p.x, topLeft.y);
            pUnderlineDrawing->AddPath();
        }
        break;
    }
    case GFxTextHighlightInfo::Underline_DitheredThick:
        // X X X X X X X X
        //  X X X X X X X X
        for (int i = 0; i < 2; ++i)
        {
            for (Float x = topLeft.x + i*singlePointWidth; x < bottomRight.x; x += singlePointWidth*2)
            {
                // for dotted or dithered underlining we need to snap to pixel every pixel
                // since otherwise it doesn't work correctly.
                GPointF p(x, topLeft.y);
                p   = globalMatrixDir.Transform(p);
                p.x = floorf(p.x + 0.5f);
                p.y = floorf(p.y + 0.5f);
                p    = globalMatrixInv.Transform(p);
                p.x = floorf(p.x + 0.5f);
                p.y = floorf(p.y + 0.5f);
                x = p.x;

                pUnderlineDrawing->MoveTo(p.x, p.y);
                pUnderlineDrawing->LineTo(p.x + singlePointWidth, p.y);
                pUnderlineDrawing->LineTo(p.x + singlePointWidth, p.y + singleUnderlineHeight);
                pUnderlineDrawing->LineTo(p.x, p.y + singleUnderlineHeight);
                pUnderlineDrawing->LineTo(p.x, p.y);
                pUnderlineDrawing->AddPath();
            }
            topLeft.y += singleUnderlineHeight;
        }
        break;
    default: GASSERT(0); // not supported
    }
}

#if defined(GFC_BUILD_DEBUG) && (defined(DEBUG_LINE_WIDTHS) || defined(DEBUG_VIS_RECT))
static void DrawRect(GRenderer*  prenderer, const GRenderer::Matrix& mat, const GRectF& rect, GColor color)
{
    GRenderer::Cxform colorCxform;
    colorCxform.M_[0][0] = (1.0f / 255.0f) * color.GetRed();
    colorCxform.M_[1][0] = (1.0f / 255.0f) * color.GetGreen();
    colorCxform.M_[2][0] = (1.0f / 255.0f) * color.GetBlue();
    colorCxform.M_[3][0] = (1.0f / 255.0f) * color.GetAlpha();
    prenderer->SetCxform(colorCxform); //?AB: do we need this for drawing mask?

    GPointF                 coords[4];
    static const UInt16     indices[6] = { 0, 1, 2, 2, 1, 3 };

    GRenderer::Matrix m(mat);
    GRectF newRect;
    GFx_RecalculateRectToFit16Bit(m, rect, &newRect);
    prenderer->SetMatrix(m);

    coords[0] = newRect.TopLeft();
    coords[1] = newRect.TopRight();
    coords[2] = newRect.BottomLeft();
    coords[3] = newRect.BottomRight();

    const SInt16  icoords[18] = 
    {
        // Strip (fill in)
        (SInt16) coords[0].x, (SInt16) coords[0].y,
        (SInt16) coords[1].x, (SInt16) coords[1].y,
        (SInt16) coords[2].x, (SInt16) coords[2].y,
        (SInt16) coords[3].x, (SInt16) coords[3].y
    };

    prenderer->FillStyleColor(GColor(255, 255, 255, 255));
    prenderer->SetVertexData(icoords, 9, GRenderer::Vertex_XY16i);

    // Fill the inside
    prenderer->SetIndexData(indices, 6, GRenderer::Index_16);
    prenderer->DrawIndexedTriList(0, 0, 6, 0, 2);

    // Done
    prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
    prenderer->SetIndexData(0, 0, GRenderer::Index_None);
}
#endif

void    GFxTextLineBuffer::Display(GFxDisplayContext &context,
                                   const Matrix& matrix, const Cxform& cxform,
                                   bool nextFrame,
                                   const GFxTextFieldParam& textFieldParam,
                                   const GFxTextHighlighter* phighlighter)
{
    GFxTextFieldParam param(textFieldParam);

// DBG
//param.ShadowColor = 0xC0FF0000;
//param.ShadowParam.SetBlurX(2.f);
//param.ShadowParam.SetBlurY(2.f);
//param.ShadowParam.SetBlurStrength(5.0f);
//param.ShadowParam.SetFineBlur(false);
//param.ShadowParam.SetKnockOut(true);

    GRenderer*  prenderer = context.GetRenderer();

    bool dynamicCacheEnabled = false; 
    bool hasUnderline = false;
                
    Float maxRasterScale = 1.25f;
    UInt  steadyCount = 0;
    GFxFontCacheManager* fm = context.pFontCacheManager;
    if (fm)
    {
        pCacheManager       = fm->GetImplementation();
        dynamicCacheEnabled = fm->IsDynamicCacheEnabled();
        maxRasterScale      = fm->GetMaxRasterScale();
        steadyCount         = fm->GetSteadyCount();
    }
    else
    {
        pCacheManager = 0;
    }

    Matrix mat(matrix);
    mat.PrependTranslation(-Float(Geom.HScrollOffset), 0);

    Float yOffset = -Float(GetVScrollOffsetInTwips());

    Float heightRatio = GFx_CalcHeightRatio(matrix) * 
                        PixelsToTwips(context.ViewportMatrix.M_[1][1]);

    if (heightRatio < 0.001f)
        heightRatio = 0.001f; // To prevent from Div by Zero

    if (steadyCount && !param.TextParam.IsOptRead())
    {
        bool animation = (matrix != Geom.SourceTextMatrix);
        if (animation)
        {
            Geom.SteadyCount = 0;
            if (Geom.IsReadability())
            {
                InvalidateCache();
                Geom.ClearReadability();
            }
        }
        else
        {
            if (nextFrame)
            {
                Geom.SteadyCount++;
                if (Geom.SteadyCount > steadyCount) 
                    Geom.SteadyCount = steadyCount;
            }
        }
        if (Geom.SteadyCount >= steadyCount)
        {
            param.TextParam.SetOptRead(true);
            param.ShadowParam.SetOptRead(true);
            if(!Geom.IsReadability())
            {
                InvalidateCache();
                Geom.SetReadability();
            }
        }
    }

    Matrix displayMatrix = matrix;
    bool   matrixChanged = false;
    if (param.TextParam.IsOptRead())
    {
        // We have to check the 2x2 matrix. Theoretically it would be enough 
        // to check for the heightRatio only, but it may result in using 
        // a degenerate matrix for the first calculation, and so, 
        // introducing huge inaccuracy. 
        //------------------------------
        matrixChanged = Geom.SourceTextMatrix.M_[0][0] != matrix.M_[0][0] ||
                        Geom.SourceTextMatrix.M_[0][1] != matrix.M_[0][1] ||
                        Geom.SourceTextMatrix.M_[1][0] != matrix.M_[1][0] ||
                        Geom.SourceTextMatrix.M_[1][1] != matrix.M_[1][1] ||
                        Geom.ViewportMatrix != context.ViewportMatrix;

        if (!matrixChanged)
        {
            GPointF transl(GPointF(matrix.M_[0][2] - Geom.Translation.x, 
                                   matrix.M_[1][2] - Geom.Translation.y));
            Float kx = context.ViewportMatrix.M_[0][0];
            Float ky = context.ViewportMatrix.M_[1][1];
            displayMatrix.M_[0][2] = Geom.Translation.x + floorf(transl.x * kx + 0.5f) / kx;
            displayMatrix.M_[1][2] = Geom.Translation.y + floorf(transl.y * ky + 0.5f) / ky;
        }
    }
    else
    {
        matrixChanged = Geom.ViewportMatrix != context.ViewportMatrix;
        if (Geom.NeedsCheckPreciseScale() && heightRatio != Geom.HeightRatio)
        {
            matrixChanged = true;
        }
    }

    Geom.HeightRatio      = heightRatio;
    Geom.SourceTextMatrix = matrix;
    Geom.ViewportMatrix   = context.ViewportMatrix;

    if (phighlighter)
    {
        hasUnderline = phighlighter->HasUnderlineHighlight();
        if (!phighlighter->IsValid())
            InvalidateCache(); // if highligher was invalidated then invalidate the cache too
    }

    if (IsCacheInvalid() ||
        matrixChanged || 
        LastHScrollOffset != Geom.HScrollOffset)
    {
        pUnderlineDrawing = NULL;
    }

    if (IsCacheInvalid() || 
        param != Param ||
        matrixChanged || 
       !pBatchPackage || !pCacheManager ||
        context.MaskRenderCount > 0 || // if text is used as a mask - don't use batches (!AB)
        LastHScrollOffset != Geom.HScrollOffset ||
       !pCacheManager->VerifyBatchPackage(pBatchPackage, 
                                          context, 
                                          param.TextParam.IsOptRead() ? 0 : heightRatio))
    {
        //printf("\nInvalidate%d matrixChanged%d Readability%d\n", InvalidCache, matrixChanged, GFxTextParam::GetOptRead(textParam));
        ResetCache();
        ValidateCache();
    }

    LastHScrollOffset = Geom.HScrollOffset;

    // Prepare a batch for optimized rendering.
    // if text is used as a mask - don't use batches (!AB)
    UInt numGlyphsInBatch = 0;
    if (!pBatchPackage && context.MaskRenderCount == 0)
    {      
        ClearBatchHasUnderline();
        Iterator     linesIt = BeginVisible(yOffset);
        Geom.ClearCheckPreciseScale();
        // set or clear "in-batch" flags for each glyph
        for(; linesIt.IsVisible(); ++linesIt)
        {
            GFxTextLineBuffer::Line& line = *linesIt;
            GFxTextLineBuffer::GlyphIterator glyphIt = line.Begin();
            for (; !glyphIt.IsFinished(); ++glyphIt)
            {
                GFxTextLineBuffer::GlyphEntry& glyph = glyphIt.GetGlyph();
                glyph.ClearInBatch();

                if (glyphIt.IsUnderline())
                    SetBatchHasUnderline();

                if (glyph.GetIndex() == ~0u || glyph.IsCharInvisible() || glyphIt.HasImage())
                    continue;

                GFxFontResource* presolvedFont = glyphIt.GetFont();
                GASSERT(presolvedFont);

                UInt fontSize = (UInt)glyph.GetFontSize(); //?AB, support for fractional font size(?)
                if (fontSize == 0) 
                    continue;

                const GFxTextureGlyphData* tgd = presolvedFont->GetTextureGlyphData();
                bool useGlyphTextures = false;
                if (tgd)
                {
                    Float textScreenHeight = heightRatio * fontSize;
                    Float maxGlyphHeight   = GFxFontPackParams::GetTextureGlyphMaxHeight(presolvedFont);
                    useGlyphTextures       = ((textScreenHeight <= maxGlyphHeight * maxRasterScale) || 
                                               presolvedFont->GlyphShapesStripped());
                    Geom.SetCheckPreciseScale();
                }
                else
                if (dynamicCacheEnabled && pCacheManager)
                {
                    GRectF b;
                    useGlyphTextures = 
                        pCacheManager->GlyphFits(presolvedFont->GetGlyphBounds(glyph.GetIndex(), &b),
                                                 fontSize, 
                                                 heightRatio,
                                                 param.TextParam,
                                                 maxRasterScale);
                }

                if (useGlyphTextures)
                {
                    glyph.SetInBatch();
                    ++numGlyphsInBatch;
                }
                else
                    Geom.SetCheckPreciseScale();
            }
        }

        Param = param;

        linesIt = BeginVisible(yOffset);
        GPointF offset = Geom.VisibleRect.TopLeft();
        offset.y += yOffset;

        // Create batches.
        // Value numGlyphsInBatch is just a hint to the allocator that
        // helps avoid extra reallocs. 
        if (pCacheManager)
        {
            linesIt.SetHighlighter(phighlighter);
            pBatchPackage = pCacheManager->CreateBatchPackage(pBatchPackage, 
                                                              linesIt, 
                                                              context, 
                                                              offset,
                                                              &Geom,
                                                              param,
                                                              numGlyphsInBatch);
        }
        Geom.Translation.x = matrix.M_[0][2];
        Geom.Translation.y = matrix.M_[1][2];
    }

    bool maskApplied = false;

#ifdef DEBUG_VIS_RECT
    DrawRect(prenderer, mat, Geom.VisibleRect, GColor(0, 0, 233, 128));
#endif

#ifdef DEBUG_LINE_WIDTHS
    // Draw bounds around every line
    {Iterator linesIt = BeginVisible(yOffset);
    for(; linesIt.IsVisible(); ++linesIt)
    {
        const Line& line = *linesIt;
        GPointF offset = line.GetOffset();
        offset += Geom.VisibleRect.TopLeft();
        offset.y += yOffset;

        GRectF rect(offset.x, offset.y, offset.x + line.GetWidth(), offset.y + line.GetHeight());
        DrawRect(prenderer, mat, rect, GColor(0, 222, 0, 128));
    }}
#endif //DEBUG_LINE_WIDTHS

    // Display batches
    if (pCacheManager)
    {
        pCacheManager->DisplayBatchPackage(pBatchPackage, 
                                           context, 
                                           displayMatrix,
                                           cxform);
    }

    if (!pCacheManager || 
         pCacheManager->IsVectorRenderingRequired(pBatchPackage))
    {
        // check for partially visible (vertically) line
        // if there is one - apply mask right here
        if (IsPartiallyVisible(yOffset))
        {
            maskApplied = ApplyMask(context, matrix, Geom.VisibleRect);
        }

        // We apply textColor through CXForm because that works for EdgeAA too.
        GRenderer::Cxform finalCxform = cxform;
        GColor prevColor(0u);
        Iterator linesIt = BeginVisible(yOffset);
        GRectF glyphBounds;
        for(; linesIt.IsVisible(); ++linesIt)
        {
            Line& line = *linesIt;
            GPointF offset = line.GetOffset();
            offset += Geom.VisibleRect.TopLeft();
            offset.y += yOffset + line.GetBaseLineOffset();
            SInt advance = 0;
            GlyphIterator glyphIt = line.Begin(phighlighter);
            for (; !glyphIt.IsFinished(); ++glyphIt, offset.x += advance)
            {
                const GlyphEntry& glyph = glyphIt.GetGlyph();

                advance = glyph.GetAdvance();
                if (glyph.IsInBatch() || (glyph.IsCharInvisible() && !glyphIt.IsUnderline() && !glyphIt.HasImage()))
                    continue;

                Float scale = 1.0f;
                Float approxSymW = 0;
                UInt  index = glyph.GetIndex();
                GFxTextImageDesc* pimage = NULL;
                GFxFontResource* presolvedFont = NULL;
                GFxShapeCharacterDef* pglyphShape = NULL; 

                if (glyphIt.HasImage())
                {
                    pimage = glyphIt.GetImage();
                    approxSymW = pimage->GetScreenWidth();
                }
                else
                {
                    // render glyph as shape
                    Float fontSize = (Float)PixelsToTwips(glyph.GetFontSize());
                    scale = fontSize / 1024.0f; // the EM square is 1024 x 1024   

                    presolvedFont = glyphIt.GetFont();
                    GASSERT(presolvedFont);

                    if (pCacheManager)
                    {
                        if (index != ~0u)
                            pglyphShape = pCacheManager->GetGlyphShape(presolvedFont, index, 0,
                                                                       glyphIt.IsFauxBold(), 
                                                                       glyphIt.IsFauxItalic(),
                                                                       context.pLog);
                        if (pglyphShape)
                            approxSymW = pglyphShape->GetBound().Right * scale;
                    }
                    else
                    {
                        if (index != ~0u)
                            pglyphShape = presolvedFont->GetGlyphShape(index, 0);

                        if (!IsStaticText()) // no mask or partial visibility test for static text
                        {
                            approxSymW = presolvedFont->GetGlyphBounds(index, &glyphBounds).Right * scale;
                        }
                        else
                        {
                            // For static text GetGlyphBounds is expensive, and since we do not 
                            // need it, just don't get it for static text.
                            approxSymW = 0;
                        }
                    }
                }
                Float adjox = offset.x - Geom.HScrollOffset;

                // check for invisibility/partial visibility of the glyph in order to determine
                // necessity of setting mask.
                if (!IsStaticText()) // no mask or partial visibility test for static text
                {
                    // test for complete invisibility, left side
                    if (adjox + approxSymW < Geom.VisibleRect.Left)
                        continue;

                    // test for complete invisibility, right side
                    if (adjox >= Geom.VisibleRect.Right)
                        break; // we can finish here
                }
                if (glyphIt.IsUnderline())
                {
                    if (!glyph.IsNewLineChar())
                        hasUnderline = true;
                    if (glyph.IsCharInvisible())
                        continue;
                }

                // test for partial visibility
                if (!maskApplied && !IsStaticText() &&
                    ((adjox < Geom.VisibleRect.Left && adjox + approxSymW > Geom.VisibleRect.Left) || 
                    (adjox < Geom.VisibleRect.Right && adjox + approxSymW > Geom.VisibleRect.Right)))
                {
                    maskApplied = ApplyMask(context, matrix, Geom.VisibleRect);
                }

                Matrix  m(mat);
                m.PrependTranslation(offset.x, offset.y);

                if (pimage)
                {
                    if (pimage->pImageShape)
                    {
                        m *= pimage->Matrix;
                        // draw image
                        GFxDisplayParams params(context, m, cxform, GRenderer::Blend_None);
                        pimage->pImageShape->GetFillAndLineStyles(&params);
                        pimage->pImageShape->Display(params, false, 0);
                    }
                    continue;
                }

                GColor color(glyphIt.GetColor());
                if (color != prevColor)
                {
                    GRenderer::Cxform   colorCxform;
                    colorCxform.M_[0][0] = (1.0f / 255.0f) * color.GetRed();
                    colorCxform.M_[1][0] = (1.0f / 255.0f) * color.GetGreen();
                    colorCxform.M_[2][0] = (1.0f / 255.0f) * color.GetBlue();
                    colorCxform.M_[3][0] = (1.0f / 255.0f) * color.GetAlpha();

                    finalCxform = cxform;
                    finalCxform.Concatenate(colorCxform);
                }

                m.PrependScaling(scale);

                if (index == ~0u)
                {
                    // Invalid glyph; render it as an empty box.
                    prenderer->SetCxform(cxform);
                    prenderer->SetMatrix(m);
                    GColor transformedColor = cxform.Transform(glyphIt.GetColor());
                    prenderer->LineStyleColor(transformedColor);

                    // The EM square is 1024x1024, but usually isn't filled up.
                    // We'll use about half the width, and around 3/4 the height.
                    // Values adjusted by eye.
                    // The Y baseline is at 0; negative Y is up.
                    static const SInt16 EmptyCharBox[5 * 2] =
                    {
                        32,     32,
                        480,    32,
                        480,    -656,
                        32,     -656,
                        32,     32
                    };

                    prenderer->SetVertexData(EmptyCharBox, 5, GRenderer::Vertex_XY16i);
                    prenderer->DrawLineStrip(0, 4);
                    prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
                }
                else
                {               
                    // Draw the GFxCharacter using the filled outline.
                    // MA TBD: Is 0 ok for mask in characters?
                    if (pglyphShape)
                    {
                        // Blend arg doesn't matter because no stroke is used for glyphs.
                        pglyphShape->SetNonZeroFill(true);
                        GFxDisplayParams params(context, m, finalCxform, GRenderer::Blend_None);
                        params.FillStylesNum = 1;
                        params.pFillStyles = &DummyStyle;

                        pglyphShape->Display(params, false, 0);
                        //pglyphShape->Display(context, m, finalCxform, GRenderer::Blend_None,
                        //    0, DummyStyle, DummyLineStyle, 0);
                    }
                }
            }
        }
    }

    // draw underline if necessary
    if ((hasUnderline || HasBatchUnderline()) && !pUnderlineDrawing)
    {
        Iterator linesIt = BeginVisible(yOffset);
        //linesIt.SetHighlighter(phighlighter);

        // calculate the "hscrolled" view rect for clipping
        GRectF hscrolledViewRect(Geom.VisibleRect);
        hscrolledViewRect.OffsetX(Float(Geom.HScrollOffset));

        for(; linesIt.IsVisible(); ++linesIt)
        {
            Line& line = *linesIt;
            GPointF offset = line.GetOffset();
            offset += Geom.VisibleRect.TopLeft();

            bool newLine = true;

            Float offx = 0, offy = 0, singleUnderlineHeight = 0, singlePointWidth = 0;
            UInt32 lastColor = 0;
            SInt lineLength = 0;
            UInt lastIndex = ~0u;
            GFxTextHighlightInfo::UnderlineStyle lastStyle = GFxTextHighlightInfo::Underline_None;

            Matrix globalMatrixDir;
            Matrix globalMatrixInv;
            SInt advance = 0;
            GlyphIterator glyphIt = line.Begin(phighlighter);
            for (UInt i = 0; !glyphIt.IsFinished(); ++glyphIt, offset.x += advance, ++i)
            {
                const GlyphEntry& glyph = glyphIt.GetGlyph();

                advance = glyph.GetAdvance();
                if (glyphIt.IsUnderline() && !glyphIt.HasImage() && !glyph.IsNewLineChar())
                {
                    Float adjox = offset.x - Geom.HScrollOffset;

                    // check for invisibility/partial visibility of the glyph in order to determine
                    // necessity of setting mask.

                    // test for complete invisibility, right side
                    if (adjox >= Geom.VisibleRect.Right)
                        break; // we can finish here

                    // test for complete invisibility, left side
                    if (adjox + advance < Geom.VisibleRect.Left)
                        continue;

                    // if this is the first visible underlined char in the line - 
                    // do some init stuff
                    if (newLine)
                    {
                        Float descent = Float(line.GetHeight() - line.GetBaseLineOffset());
                        offset.y += yOffset + line.GetBaseLineOffset() + descent/2;

                        // snap to pixel the x and y values
                        //globalMatrixDir = Geom.SourceTextMatrix;

                        globalMatrixDir = mat;

                        globalMatrixDir.Append(context.ViewportMatrix);
                        globalMatrixInv = globalMatrixDir;
                        globalMatrixInv.Invert();

                        GPointF offset1   = globalMatrixDir.Transform(offset);
                        offset1.x = floorf(offset1.x + 0.5f);
                        offset1.y = floorf(offset1.y + 0.5f);
                        offset    = globalMatrixInv.Transform(offset1);
                        offset.x = floorf(offset.x + 0.5f);
                        offset.y = floorf(offset.y + 0.5f);

                        offset1.x += 1;
                        offset1.y += 1;
                        offset1    = globalMatrixInv.Transform(offset1);
                        offset1.x = floorf(offset1.x + 0.5f);
                        offset1.y = floorf(offset1.y + 0.5f);

                        singleUnderlineHeight = offset1.y - offset.y;
                        singlePointWidth      = offset1.x - offset.x;
                        offx = offset.x;
                        offy = offset.y;

                        newLine = false;
                    }

                    UInt32 curColor = glyphIt.GetUnderlineColor().ToColor32();
                    GFxTextHighlightInfo::UnderlineStyle curStyle = glyphIt.GetUnderlineStyle();
                    if (lineLength != 0 && 
                       (lastColor != curColor || lastIndex + 1 != i || curStyle != lastStyle))
                    {
                        DrawUnderline(offx, offy, singleUnderlineHeight, singlePointWidth, hscrolledViewRect, 
                            lineLength, lastStyle, lastColor, globalMatrixDir, globalMatrixInv);

                        lineLength = 0;
                        offx = offset.x;
                    }
                    lineLength += advance;
                    lastColor = curColor;
                    lastIndex = i;
                    lastStyle = curStyle;
                }
            }
            if (lineLength != 0)
            {
                DrawUnderline(offx, offy, singleUnderlineHeight, singlePointWidth, hscrolledViewRect, 
                    lineLength, lastStyle, lastColor, globalMatrixDir, globalMatrixInv);
            }
        }
    }
    if (pUnderlineDrawing)
        pUnderlineDrawing->Display(context, mat, cxform, GRenderer::Blend_None, false);

    // done DisplayRecord
    if (maskApplied)
    {
#ifndef DEBUG_MASK
        RemoveMask(context, matrix, Geom.VisibleRect);
#endif
    }
}

void    GFxTextLineBuffer::SetFirstVisibleLine(UInt line)
{
    Geom.FirstVisibleLinePos = line;
    InvalidateCache();
}

void    GFxTextLineBuffer::SetHScrollOffset(UInt offset)
{
    Geom.HScrollOffset = offset;
}

GFxTextLineBuffer::Iterator GFxTextLineBuffer::FindLineByTextPos(UPInt textPos)
{
    static LineIndexComparator cmp;
    SPInt i = GTL::find_nearest_index_in_sorted_array(Lines, (UInt)textPos, cmp);
    if (i != -1)
    {
        Line* pline = Lines[(UInt)i];
        UInt textPos = pline->GetTextPos();
        if (textPos >= textPos && textPos <= textPos + pline->GetTextLength())
            return Iterator(*this, (UInt)i);
    }
    return Iterator();
}

GFxTextLineBuffer::Iterator GFxTextLineBuffer::FindLineAtYOffset(Float yoff)
{
    //yoff += Float(GetVScrollOffsetInTwips());
    LineYOffsetComparator cmp;
    SPInt i = GTL::find_nearest_index_in_sorted_array(Lines, yoff, cmp);
    if (i != -1)
    {
        Line* pline = Lines[(UInt)i];
        if (yoff >= pline->GetOffsetY() && yoff < pline->GetOffsetY() + pline->GetHeight() + pline->GetLeading())
        {
            return Iterator(*this, (UInt)i);
        }
    }
    return Iterator();
}

bool GFxTextLineBuffer::IsLineVisible(UInt lineIndex, Float yOffset) const
{
    const Line& line = *Lines[lineIndex];
    if (lineIndex == Geom.FirstVisibleLinePos)
    {
        // a special case for the first VISIBLE line: display it even if it is only
        // partially visible
        return (line.GetOffsetY() + yOffset <= Geom.VisibleRect.Height() + GFX_TEXT_GUTTER/2); 
    }
    // subtract leading of the line before checking its complete visibility
    return lineIndex > Geom.FirstVisibleLinePos  && 
        (line.GetOffsetY() + line.GetHeight() + yOffset <= Geom.VisibleRect.Height() + GFX_TEXT_GUTTER/2); 
}

bool GFxTextLineBuffer::IsPartiallyVisible(Float yOffset) const
{
    if (Geom.FirstVisibleLinePos < size())
    {
        const Line& line = *Lines[Geom.FirstVisibleLinePos];
        if (line.GetWidth() != 0 && line.GetHeight() != 0)
        {
            Float ly = line.GetOffsetY();
            if ((ly + yOffset <= Geom.VisibleRect.Height() + GFX_TEXT_GUTTER/2) &&
                (ly + line.GetHeight() + yOffset > Geom.VisibleRect.Height() + GFX_TEXT_GUTTER/2))
                return true;
        }
    }
    return false;
}

void GFxTextLineBuffer::Scale(Float scaleFactor)
{
    Iterator it = Begin();
    for (;!it.IsFinished(); ++it)
    {
        Line& line = *it;
        GASSERT(line.IsData32());

        Float newLeading    = Float(line.GetLeading())*scaleFactor;
        Float newW          = Float(line.GetWidth())*scaleFactor;
        Float newH          = Float(line.GetHeight())*scaleFactor;
        line.SetLeading(SInt(newLeading));
        line.SetDimensions(SInt(newW), SInt(newH));
        line.SetBaseLineOffset(line.GetBaseLineOffset()*scaleFactor);
        line.SetOffsetX(line.GetOffsetX()*scaleFactor);
        line.SetOffsetY(line.GetOffsetY()*scaleFactor);

        GlyphIterator git = line.Begin();
        for(; !git.IsFinished(); ++git)
        {
            GlyphEntry& ge = git.GetGlyph();
            Float newAdv = Float(ge.GetAdvance())*scaleFactor;
            ge.SetAdvance(SInt(newAdv));
            ge.SetFontSize(ge.GetFontSize()*scaleFactor);
        }
    }
    InvalidateCache();
}

SInt GFxTextLineBuffer::GetMinLineHeight() const
{
    if (size() == 0)
        return 0;
    ConstIterator it = Begin();
    SInt minH = GFC_MAX_SINT;
    for (;!it.IsFinished(); ++it)
    {
        const Line& line = *it;
        SInt h = line.GetHeight();
        if (h < minH)
            minH = h;
    }
    return minH;
}

#ifdef GFC_BUILD_DEBUG
void GFxTextLineBuffer::Dump() const
{
    printf("Dumping lines...\n");
    printf("VisibleRect: { %f, %f, {%f, %f}}\n", 
        Geom.VisibleRect.Left, Geom.VisibleRect.Top, Geom.VisibleRect.Width(), Geom.VisibleRect.Height());
    ConstIterator linesIt = Begin();
    UInt i = 0;
    for(; !linesIt.IsFinished(); ++linesIt)
    {
        const Line& line = *linesIt;
        printf("Line[%d]\n", i++);
        printf("   TextPos = %d, NumGlyphs = %d, Len = %d\n", line.GetTextPos(), line.GetNumGlyphs(), line.GetTextLength());
        printf("   Bounds = { %f, %f, {%f, %f}}\n", line.GetOffsetX(), line.GetOffsetY(), Float(line.GetWidth()), Float(line.GetHeight()));
    }
    printf("...end\n\n");
}

void GFxTextLineBuffer::CheckIntegrity() const
{
    ConstIterator linesIt = Begin();
    UInt nextpos = 0;
    for(; !linesIt.IsFinished(); ++linesIt)
    {
        const Line& line = *linesIt;
        UInt l = 0;
        GlyphIterator git = const_cast<Line&>(line).Begin();
        for (; !git.IsFinished(); ++git)
        {
            const GFxTextLineBuffer::GlyphEntry& glyph = git.GetGlyph();
            l += glyph.GetLength();
        }
        if (!(line.GetTextPos() == nextpos || line.GetTextPos() == nextpos + 1) )
        {
            Dump();
            GASSERT(0);
        }
        nextpos = line.GetTextPos() + l;
    }
}
#endif

void GFx_RecalculateRectToFit16Bit(GRenderer::Matrix& matrix, const GRectF& srcRect, GRectF* pdestRect)
{
    matrix.PrependTranslation(srcRect.Left, srcRect.Top);

    Float xscale = 1, width = srcRect.Width();
    if (width > 32767)
    {
        xscale = width / 32767;
        width = 32767;
    }
    Float yscale = 1, height = srcRect.Height();
    if (height > 32767)
    {
        yscale = height / 32767;
        height = 32767;
    }
    matrix.PrependScaling(xscale, yscale);
    GASSERT(pdestRect);
    pdestRect->Top = pdestRect->Left = 0;
    pdestRect->SetWidth(width);
    pdestRect->SetHeight(height);
}



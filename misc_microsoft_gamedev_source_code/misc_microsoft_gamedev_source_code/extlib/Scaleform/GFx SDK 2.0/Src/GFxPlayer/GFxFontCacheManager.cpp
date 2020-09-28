/**********************************************************************

Filename    :   GFxFontCacheManager.cpp
Content     :   
Created     :   
Authors     :   Maxim Shemanarev, Artem Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxFontCacheManager.h"
#include "GFxLoader.h"

//------------------------------------------------------------------------
GFxFontCacheManager::GFxFontCacheManager(bool enableDynamicCache):
    GFxState(State_FontCacheManager),
    DynamicCacheEnabled(enableDynamicCache),
    MaxRasterScale(1.25),
    pCache(new GFxFontCacheManagerImpl())
{
    SetTextureConfig(TextureConfig());
}

GFxFontCacheManager::GFxFontCacheManager(const TextureConfig& config, bool enableDynamicCache):
    GFxState(State_FontCacheManager),
    DynamicCacheEnabled(enableDynamicCache),
    MaxRasterScale(1.25),
    pCache(new GFxFontCacheManagerImpl())
{
    SetTextureConfig(config);
}

GFxFontCacheManager::~GFxFontCacheManager()
{
    delete pCache;
}

void GFxFontCacheManager::SetTextureConfig(const TextureConfig& config)
{
    CacheTextureConfig = config;
    pCache->Init(config.TextureWidth,
                 config.TextureHeight,
                 config.MaxNumTextures,
                 config.MaxSlotHeight,
                 config.SlotPadding,
                 config.TexUpdWidth,
                 config.TexUpdHeight);
}


#ifndef GFC_NO_GLYPH_CACHE
//UInt GFxFontCacheManagerImpl::FontSizeRamp[] = { 0,1,2,4,6,8,11,16,22,32,40,50,64,76,90,108,128,147,168,194,222,256,0};
UInt GFxFontCacheManagerImpl::FontSizeRamp[] = { 0,1,2,4,6,8,12,16,18,22,26,32,36,40,45,50,56,64,72,80,90,108,128,147,168,194,222,256,0};
//UInt GFxFontCacheManagerImpl::FontSizeRamp[] = { 0,1,2,4,8,16,32,64,128,256,0};
#endif


//------------------------------------------------------------------------
GFxFontCacheManagerImpl::GFxFontCacheManagerImpl():
    pRenderer(0)
{
    FontDisposer.Bind(this);
#ifndef GFC_NO_GLYPH_CACHE
    UInt i;
    UInt ramp = 0;
    for (i = 0; i < 256; ++i)
    {
        if (i > FontSizeRamp[ramp + 1])
            ++ramp;
        FontSizeMap[i] = (UByte)ramp;
    }
#endif
}

//------------------------------------------------------------------------
GFxFontCacheManagerImpl::~GFxFontCacheManagerImpl()
{
    RemoveAll();
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::RemoveAll()
{
    InvalidateAll();
    BatchPackageQueue.RemoveAll();
    BatchPackageStorage.FreeAll();
    FontSetType::iterator it = KnownFonts.begin();    
    for (; it != KnownFonts.end(); ++it)
    {
        (*it)->RemoveDisposeHandler(&FontDisposer);
    }
    KnownFonts.clear();
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::InvalidateAll()
{
    GFxBatchPackage* bp = BatchPackageQueue.GetFirst();
    while(!BatchPackageQueue.IsNull(bp))
    {
        delete bp->Package;
        bp->Package = 0;
        bp = bp->pNext;
    }
#ifndef GFC_NO_GLYPH_CACHE
    Cache.RemoveAll();
#endif
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::CleanUpFont(GFxFontResource* font)
{
    GFxFontResource** knownFont = KnownFonts.get(font);
    if (knownFont)
    {
#ifndef GFC_NO_GLYPH_CACHE
        Cache.CleanUpFont(font);
#endif
        font->RemoveDisposeHandler(&FontDisposer);
        KnownFonts.remove(font);
        GFxBatchPackage* bp = BatchPackageQueue.GetFirst();
        while(!BatchPackageQueue.IsNull(bp))
        {
            bool removeFlag = false;
            if (bp->Package)
            {
                for (UInt i = 0; i < bp->Package->BatchVerifier.size(); ++i)
                {
                    if (bp->Package->BatchVerifier[i].GlyphParam.pFont == font)
                    {
                        removeFlag = true;
                        break;
                    }
                }
            }
            if (removeFlag)
            {
                // Invalidate BatchPackage.
                delete bp->Package;
                bp->Package = 0;
            }
            bp = bp->pNext;
        }
    }
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::Init(UInt texWidth, UInt texHeight, UInt maxNumTextures, 
                                   UInt maxSlotHeight, UInt slotPadding,
                                   UInt texUpdWidth, UInt texUpdHeight)
{
#ifndef GFC_NO_GLYPH_CACHE
    Cache.Init(texWidth, texHeight, maxNumTextures, 
               maxSlotHeight, slotPadding,
               texUpdWidth, texUpdHeight);
#else
    GUNUSED(texWidth);      GUNUSED(texHeight); GUNUSED(maxNumTextures);
    GUNUSED(maxSlotHeight); GUNUSED(slotPadding);
    GUNUSED(texUpdWidth);   GUNUSED(texUpdHeight);
#endif
}

//------------------------------------------------------------------------
inline UInt GFxFontCacheManagerImpl::snapFontSizeToRamp(UInt fontSize) const
{
#ifndef GFC_NO_GLYPH_CACHE
    fontSize = fontSize + ((fontSize + 3) >> 2);
    fontSize =(fontSize <= 255) ? FontSizeRamp[FontSizeMap[fontSize] + 1] : 255;
#endif
    return fontSize;
}

//------------------------------------------------------------------------
bool GFxFontCacheManagerImpl::GlyphFits(const GRectF& bounds, 
                                        UInt  fontSize, 
                                        Float heightRatio, 
                                        const GFxGlyphParam& param) const
{
    if (param.BlurX || param.BlurY || param.Outline)
         return true;

    fontSize = UInt(heightRatio * fontSize + 0.5f);
    if (!param.IsOptRead())
        fontSize = snapFontSizeToRamp(fontSize);

    UInt h = (UInt)ceilf (bounds.Bottom * fontSize / 1024.0f) - 
             (UInt)floorf(bounds.Top    * fontSize / 1024.0f) + 1;
    return h < GetTextureGlyphMaxHeight();
}


//------------------------------------------------------------------------
bool GFxFontCacheManagerImpl::VerifyBatchPackage(const GFxBatchPackage* bp, 
                                                 GFxDisplayContext &context,
                                                 Float heightRatio)
{
    // Verify the renderer for consistency
    GRenderer* ren = context.GetRenderer();
    if (pRenderer == 0) 
        pRenderer = ren;
    else
    if (ren != pRenderer)
    {
        InvalidateAll();
        pRenderer = ren;
    }

    // Verify the pachage and the owner
    if (!bp || bp->Owner != this || !bp->Package)
        return false;

#ifndef GFC_NO_GLYPH_CACHE
    // Verify glyphs
    const GFxBatchPackageData* bpd = bp->Package;

    UInt i;
    bool verifySize = heightRatio != 0;
    for (i = 0; i < bpd->BatchVerifier.size(); ++i)
    {
        const GFxBatchPackageData::GlyphVerifier& gv = bpd->BatchVerifier[i];

        if (verifySize)
        {
            UInt newSize = snapFontSizeToRamp(UInt(gv.FontSize * heightRatio + 0.5f));
            if (newSize != gv.GlyphParam.GetFontSize())
                return false;
        }
        if (gv.pGlyph && !Cache.VerifyGlyphAndSendBack(gv.GlyphParam, gv.pGlyph))
            return false;
    }
#else
    GUNUSED(heightRatio);
#endif

    return true;
}

//------------------------------------------------------------------------
GFxBatchPackage* 
GFxFontCacheManagerImpl::CreateBatchPackage(GFxBatchPackage* bp, 
                                            const GFxTextLineBuffer::Iterator& linesIt, 
                                            GFxDisplayContext &context, 
                                            const GPointF& lnOffset,
                                            GFxLineBufferGeometry* geom,
                                            const GFxTextFieldParam& param,
                                            UInt numGlyphsInBatch)
{
    if (bp && bp->Owner == this)
    {
        //BatchPackageQueue.SendToBack(bp);
        if (bp->Package)
            if (numGlyphsInBatch)
                bp->Package->RemoveAll();
        else
            bp->Package = new GFxBatchPackageData;
    }
    else
    {
        if (bp) 
            delete bp->Package;

        bp = BatchPackageStorage.Allocate();
        bp->Owner   = this;
        bp->Package = new GFxBatchPackageData;
        BatchPackageQueue.PushBack(bp);
    }

    fillBatchPackage(bp->Package, 
                     linesIt, 
                     context, 
                     lnOffset, 
                     geom, 
                     param, 
                     numGlyphsInBatch);
#ifndef GFC_NO_GLYPH_CACHE
    Cache.UpdateTextures(context.GetRenderer());
#endif
    return bp;
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::ReleaseBatchPackage(GFxBatchPackage* bp)
{
    if (bp)
    {
        if (bp->Owner != this)
        {
            delete bp->Package;
            bp->Package = 0;
        }
        else
        {
            delete bp->Package;
            BatchPackageQueue.Remove(bp);
            BatchPackageStorage.Free(bp);
        }
    }
}

//------------------------------------------------------------------------
bool GFxFontCacheManagerImpl::DisplayBatchPackage(GFxBatchPackage* bp, 
                                                  GFxDisplayContext &context, 
                                                  const GFxLineBufferGeometry& geom,
                                                  const GFxTextFieldParam& param,
                                                  const Matrix& displayMatrix,
                                                  const Cxform& cx, 
                                                  bool maskApplied)
{
    if (bp && bp->Owner == this && bp->Package)
    {
        //BatchPackageQueue.SendToBack(bp);
        GFxBatchPackageData* pd = bp->Package;

        if (pd->Batch.size() > 0)
        {
            GRenderer*  prenderer = context.GetRenderer();
            Matrix mat(displayMatrix);

            Float offset = Float(geom.HScrollOffset);
            if (param.TextParam.IsOptRead())
            {
                Float k = geom.HeightRatio / PixelsToTwips(1);
                offset = floorf(geom.HScrollOffset * k + 0.5f) / k;
            }

            mat.PrependTranslation(-offset, 0); 
            prenderer->SetCxform(cx);

            // Render the batch piece by piece.
            GRenderer::CacheProvider cacheProvider(&pd->BatchCache);

            GRectF visibleRect = geom.VisibleRect;

            visibleRect.Left   -= geom.BlurX * 20;
            visibleRect.Top    -= geom.BlurY * 20;
            visibleRect.Right  += geom.BlurX * 20;
            visibleRect.Bottom += geom.BlurY * 20;

            if (param.ShadowColor.GetAlpha() != 0)
            {
                visibleRect.Left   += (Float)param.ShadowOffsetX;
                visibleRect.Top    += (Float)param.ShadowOffsetY;
                visibleRect.Right  += (Float)param.ShadowOffsetX;
                visibleRect.Bottom += (Float)param.ShadowOffsetY;
                visibleRect.Union(geom.VisibleRect);
            }

            for (UInt layer = 0; layer < 2; ++layer)
            {
                GFxBatchPackageData::BatchDescHash::iterator it = pd->BatchDesc.begin();
                UInt batchIndex;
                for (batchIndex = 0; it != pd->BatchDesc.end(); ++it)
                {
                    if (it->first.Layer != layer)
                        continue;

	                if (!geom.IsStaticText()) // no mask or partial visibility tests for static text
    	            {
	                    // check the batch is at least partially in the VisibleRect
    	                Float adjL = it->second.Bounds.Left - geom.HScrollOffset;
        	            Float adjR = it->second.Bounds.Right - geom.HScrollOffset;
            	        if (adjL > visibleRect.Right || adjR < visibleRect.Left)
                	        continue; // batch is completely out of visible rect

                    	// apply mask, if hscroll is not 0
	                    if (!maskApplied && (geom.HScrollOffset != 0 || adjR > visibleRect.Right + GFX_TEXT_GUTTER/2))
	                        //!AB, just check the right bound. Left bound might be slightly out of VisibleRect because
    	                    // of pad pixels. Not sure what to do with this.
        	            {
            	            maskApplied = GFxTextLineBuffer::ApplyMask(context, geom.SourceTextMatrix, visibleRect);
                	    }
					}

                    prenderer->DrawBitmaps(&pd->Batch[0], (int)pd->Batch.size(),
                        it->second.Index, it->second.Count,
                        it->second.pTexture, mat, &cacheProvider);
                }
            }
        }
    }
    return maskApplied;
}

//------------------------------------------------------------------------
inline void
GFxFontCacheManagerImpl::resolveTextureGlyph(GFxBatchPackageData::GlyphVerifier* gv, 
                                             const GFxGlyphParam& param,
                                             GFxDisplayContext &context)
{
    gv->GlyphParam    = param;
#ifndef GFC_NO_GLYPH_CACHE
    gv->pGlyph        = 0;
#endif
    gv->pTexture      = 0;
    gv->TextureWidth  = 0;
    gv->TextureHeight = 0;
    GFxTextureGlyphData* tgData = param.pFont->GetTextureGlyphData();
    if (tgData)
    {
        const GFxTextureGlyph& tg = tgData->GetTextureGlyph(param.GlyphIndex);
        GImageInfoBase* pimage = tg.GetImageInfo(param.pFont->GetBinding());
        if (pimage)
        {
            gv->pTexture      = pimage->GetTexture(context.GetRenderer());
            gv->TextureWidth  = (UInt16)pimage->GetWidth();
            gv->TextureHeight = (UInt16)pimage->GetHeight();
        }
    }
#ifndef GFC_NO_GLYPH_CACHE
    else
    {
        gv->pGlyph = Cache.GetGlyph(context.GetRenderer(), param, gv->FontSize, context.pLog);
        if (gv->pGlyph)
        {
            gv->pTexture = Cache.GetGlyphTexture(gv->pGlyph);
            Cache.LockGlyph(gv->pGlyph);
        }
    }
#endif
}


//------------------------------------------------------------------------
template<class Matrix>
static bool GFx_DetectMatrixFreeRotation(const Matrix& matrix)
{
    Matrix mat2(matrix);
    mat2.M_[0][2] = 0;
    mat2.M_[1][2] = 0;
    GPointF p10 = mat2.Transform(GPointF(1,0));
    const Float epsilon = 1e-6f;
    return fabsf(p10.x) > epsilon && fabsf(p10.y) > epsilon;
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::fillBatchPackage(GFxBatchPackageData* pd, 
                                               const GFxTextLineBuffer::Iterator& linesIter, 
                                               GFxDisplayContext &context, 
                                               const GPointF& lnOffset, 
                                               GFxLineBufferGeometry* geom,
                                               const GFxTextFieldParam& textFieldParam,
                                               UInt numGlyphsInBatch)
{
// DBG
//printf("C");

#ifdef GFC_NO_GLYPH_CACHE
    GUNUSED(geom); 
#endif

    GFxFontResource* pprevFont = NULL;
    GFxResourceBindData fontData;

    if (textFieldParam.ShadowColor != 0)
        numGlyphsInBatch *= 2;

    // Value numGlyphsInBatch is just a hint to the allocator that
    // helps avoid extra reallocs. 
    pd->BatchVerifier.resize(0);    // Avoid possible extra copying
    pd->BatchVerifier.reserve(numGlyphsInBatch);

    pd->VectorRenderingRequired = false;

#ifndef GFC_NO_GLYPH_CACHE
    Matrix globalMatrixDir;
    Matrix globalMatrixInv;
    if (textFieldParam.TextParam.IsOptRead())
    {
        globalMatrixDir = geom->SourceTextMatrix;
        globalMatrixDir.Append(context.ViewportMatrix);
        globalMatrixInv = globalMatrixDir;
        globalMatrixInv.Invert();
    }

    // This flag is used to snap the glyphs to pixels in the X direction.
    // If the text is not oriented along the axes (freely rotated) it 
    // does not make sense to snap it.
    bool freeRotation = GFx_DetectMatrixFreeRotation(geom->SourceTextMatrix);
#endif

    UInt pass;
    enum { PassShadow = 0, PassText = 1 };
    UInt passStart = PassText;
    UInt passEnd   = PassText;

    if (textFieldParam.ShadowColor != 0)
    {
        passStart = PassShadow;
        if (textFieldParam.ShadowParam.IsKnockOut() || 
            textFieldParam.ShadowParam.IsHiddenObject())
        {
            passEnd = PassShadow;
        }
    }

    geom->BlurX = 0;
    geom->BlurY = 0;
    UInt layer;

    GFxTextLineBuffer::Iterator linesIt;
    for (pass = passStart; pass <= passEnd; ++pass)
    {
        const GFxGlyphParam& textParam = (pass == PassText)? 
            textFieldParam.TextParam: 
            textFieldParam.ShadowParam;

        // When using a single texture it will be a single batch. 
        // If there are many textures and text has a shadow, it is 
        // possible than the shadow will be drawn after text. So that, 
        // it is necessary to break the batches so that they would have
        // separately shadow and glyph bitmaps
        //-----------------------
        layer = passStart;
#ifndef GFC_NO_GLYPH_CACHE
        if (Cache.GetMaxNumTextures() > 1)
            layer = pass;
#endif

        for(linesIt = linesIter; linesIt.IsVisible(); ++linesIt)
        {
            GFxTextLineBuffer::Line& line = *linesIt;
            GFxTextLineBuffer::GlyphIterator glyphIt = line.Begin();
            for (; !glyphIt.IsFinished(); ++glyphIt)
            {
                GFxTextLineBuffer::GlyphEntry& glyph = glyphIt.GetGlyph();

                if (pass == PassShadow)
                    glyph.ClearShadowInBatch();

                if (glyph.GetIndex() == ~0u)
                {
                    pd->VectorRenderingRequired = true;
                    continue;
                }

                if (glyph.IsCharInvisible())
                    continue;

                if (!glyph.IsInBatch() && pass == PassText)
                {
                    pd->VectorRenderingRequired = true;
                    continue;
                }

                GFxFontResource* presolvedFont = glyphIt.GetFont();
                GASSERT(presolvedFont);

                // Create a BatchDesc entry for each different image.
                if (presolvedFont != pprevFont)
                {
                    //fontData = context.pResourceBinding->GetResourceData(presolvedFont);
                    pprevFont = presolvedFont;
                    if (!KnownFonts.get(presolvedFont))
                    {
                        KnownFonts.add(presolvedFont);
                        presolvedFont->AddDisposeHandler(&FontDisposer);
                    }
                }

                Float originalFontSize = geom->HeightRatio * glyph.GetFontSize();
                UInt  snappedFontSize  = UInt(originalFontSize + 0.5f);
                GFxGlyphParam glyphParam;

                glyphParam.Clear();
                glyphParam.pFont      = presolvedFont;
                glyphParam.GlyphIndex = UInt16(glyph.GetIndex());

                GFxBatchPackageData::GlyphVerifier gv;

#ifndef GFC_NO_GLYPH_CACHE
                GRectF bounds;
                presolvedFont->GetGlyphBounds(glyph.GetIndex(), &bounds);
                if (!textParam.IsOptRead())
                    snappedFontSize = snapFontSizeToRamp(snappedFontSize);

                Float outline = textParam.GetOutline() * originalFontSize * GFx_OutlineRatio;
                Float blurX   = textParam.GetBlurX() + outline;
                Float blurY   = textParam.GetBlurY() + outline;
                if (blurX > geom->BlurX) geom->BlurX = blurX;
                if (blurY > geom->BlurY) geom->BlurY = blurY;

                Cache.CalcGlyphParam(originalFontSize,
                                     snappedFontSize,
                                     geom->HeightRatio,
                                     bounds,
                                     textParam,
                                     &glyphParam,
                                     &gv.FontSize,
                                     &gv.GlyphScale);

                if (textParam.IsOptRead())
                {
                    if (textParam.BlurX == 0)
                    {
                        Float glyphWidth = (bounds.Right - bounds.Left) * glyphParam.GetFontSize() / 1024.0f;
                        if (3*glyphWidth < Cache.GetMaxGlyphHeight())
                        {
                            glyphParam.SetStretch(true);
                        }
                    }
                }
#endif
                resolveTextureGlyph(&gv, glyphParam, context);

                if (gv.pTexture)
                {
                    if (pass == PassShadow)
                        glyph.SetShadowInBatch();
                    
                    pd->BatchVerifier.push_back(gv);

                    // Find the batch with the same texture and color.
                    GFxBatchPackageData::BatchInfoKey bik(gv.pTexture, layer);
                    GFxBatchPackageData::BatchInfo* pbi = pd->BatchDesc.get(bik);
                    if (pbi == NULL)
                    {
                        // Not found? Add new record.
                        GFxBatchPackageData::BatchInfo bi;
                        bi.Clear();
                        bi.pTexture = bik.pTexture;
                        bi.ImageUseCount = 1;
                        pd->BatchDesc.add(bik, bi);
                    }
                    else
                        ++pbi->ImageUseCount;
                }
                else
                {
                    if (pass == PassText)
                        glyph.ClearInBatch();

                    pd->VectorRenderingRequired = true;
                    ++geom->NumVectorGlyphs;
                }
            }
        }
    }  // for(pass...)

    // Assign start indices to batches.
    GFxBatchPackageData::BatchDescHash::iterator it = pd->BatchDesc.begin();
    UInt batchIndex;
    for (batchIndex = 0; it != pd->BatchDesc.end(); ++it)
    {
        it->second.Index = batchIndex;
        batchIndex += it->second.ImageUseCount;
    }

    // Allocate bitmap coordinate batch.
    pd->Batch.resize(0);            // Avoid possible extra copying
    pd->Batch.resize(pd->BatchVerifier.size());

    // Assign texture coordinates to right slots in the batch.
    UInt gvIdx = 0;
    for (pass = passStart; pass <= passEnd; ++pass)
    {
        const GFxGlyphParam& textParam = (pass == PassText)? 
            textFieldParam.TextParam: 
            textFieldParam.ShadowParam;

        layer = passStart;
#ifndef GFC_NO_GLYPH_CACHE
        if (Cache.GetMaxNumTextures() > 1)
            layer = pass;
#endif

        for(linesIt = linesIter; linesIt.IsVisible(); ++linesIt)
        {
            GFxTextLineBuffer::Line& line = *linesIt;
            GPointF offset = line.GetOffset();

            offset += lnOffset;
            offset.y += line.GetBaseLineOffset();
            SInt advance = 0;

#ifndef GFC_NO_GLYPH_CACHE
            if (textParam.IsOptRead())
            {
                offset   = globalMatrixDir.Transform(offset);
                offset.x = floorf(offset.x + 0.5f);
                offset.y = floorf(offset.y + 0.5f);
                offset   = globalMatrixInv.Transform(offset);
            }
#endif

            GFxTextLineBuffer::GlyphIterator glyphIt = line.Begin();
            for (; !glyphIt.IsFinished(); ++glyphIt, offset.x += advance)
            {
                GFxTextLineBuffer::GlyphEntry& glyph = glyphIt.GetGlyph();
                advance = glyph.GetAdvance();

                if (glyph.GetIndex() == ~0u)
                {
                    pd->VectorRenderingRequired = true;
                    continue;
                }

                if (glyph.IsCharInvisible())
                    continue;

                if (pass == PassShadow)
                {
                    if (!glyph.IsShadowInBatch())
                        continue;
                }
                else
                {
                    if (!glyph.IsInBatch())
                    {
                        pd->VectorRenderingRequired = true;
                        continue;
                    }
                }

                // do batching
                GFxBatchPackageData::GlyphVerifier& gv = pd->BatchVerifier[gvIdx++];

                // Find the batch to which the glyph belongs to.
                GFxBatchPackageData::BatchInfoKey bik(gv.pTexture, layer);
                GFxBatchPackageData::BatchInfo* pbi = pd->BatchDesc.get(bik);
                GASSERT(pbi != NULL);

                GFxBatchPackageData::BitmapDesc& bd = pd->Batch[pbi->Index + pbi->Count];
                Float scale;

#ifndef GFC_NO_GLYPH_CACHE
                bool snapX = false;
                if (gv.pGlyph)
                {
                    Cache.UnlockGlyph(gv.pGlyph);
                    const GFxGlyphRect&   r = gv.pGlyph->Rect;
                    const GPoint<SInt16>& o = gv.pGlyph->Origin;

                    bd.TextureCoords.Left   = (r.x + 1) * Cache.GetScaleU();
                    bd.TextureCoords.Top    = (r.y + 1) * Cache.GetScaleV();
                    bd.TextureCoords.Right  =  bd.TextureCoords.Left + (r.w - 2) * Cache.GetScaleU();
                    bd.TextureCoords.Bottom =  bd.TextureCoords.Top  + (r.h - 2) * Cache.GetScaleV();

                    UInt stretch = gv.GlyphParam.GetStretch();
                    bd.Coords.Left   = (r.x + 1 - o.x) * 20.0f / stretch;
                    bd.Coords.Top    = (r.y + 1 - o.y) * 20.0f;
                    bd.Coords.Right  = bd.Coords.Left + (r.w - 2) * 20.0f / stretch;
                    bd.Coords.Bottom = bd.Coords.Top  + (r.h - 2) * 20.0f;
                    snapX = stretch == 1 && !freeRotation && gv.GlyphParam.BlurX == 0;

                    if (gv.GlyphParam.IsOptRead())
                    {
                        scale = 1.0f / (Float(gv.GlyphScale) * geom->HeightRatio / 256.0f);
                    }
                    else
                    {
                        scale = Float(glyph.GetFontSize() * SubpixelSizeScale) / Float(gv.FontSize);
                        snapX = false;
                    }

                    bd.Coords.Left   *= scale;
                    bd.Coords.Top    *= scale;
                    bd.Coords.Right  *= scale;
                    bd.Coords.Bottom *= scale;

                    if (pass == PassShadow)
                    {
                        bd.Coords.Left   += (Float)textFieldParam.ShadowOffsetX;
                        bd.Coords.Top    += (Float)textFieldParam.ShadowOffsetY;
                        bd.Coords.Right  += (Float)textFieldParam.ShadowOffsetX;
                        bd.Coords.Bottom += (Float)textFieldParam.ShadowOffsetY;
                    }
                }
                else
#endif
                {
                    const GFxTextureGlyphData *gd = gv.GlyphParam.pFont->GetTextureGlyphData();
                    const GFxTextureGlyph& tg = gd->GetTextureGlyph(glyph.GetIndex());

                    // Scale from uv coords to the 1024x1024 glyph square.
                    // @@ need to factor this out!
                    scale = PixelsToTwips(glyph.GetFontSize()) / 1024.0f; // the EM square is 1024 x 1024   
                    Float textureXScale = gd->GetTextureGlyphScale() * scale * gv.TextureWidth;
                    Float textureYScale = gd->GetTextureGlyphScale() * scale * gv.TextureHeight;
                    bd.Coords         = tg.UvBounds;
                    bd.TextureCoords  = tg.UvBounds;
                    bd.Coords        -= tg.UvOrigin;
                    bd.Coords.Left   *= textureXScale;
                    bd.Coords.Top    *= textureYScale;
                    bd.Coords.Right  *= textureXScale;
                    bd.Coords.Bottom *= textureYScale;
                }

                gv.FontSize = (UInt16)glyph.GetFontSize();
                GPointF p(offset);

#ifndef GFC_NO_GLYPH_CACHE
                if (snapX)
                {
                    p  = globalMatrixDir.Transform(p);
                    p.x = floorf(p.x + 0.5f);
                    p  = globalMatrixInv.Transform(p);
                }
#endif
                bd.Coords += p;
                bd.Color   = (pass == PassText) ? glyphIt.GetColor() : textFieldParam.ShadowColor;
// DBG
//bd.Color   = (pass == PassText) ? 0xFFFFFFFF : textFieldParam.ShadowColor;

                if (pbi->Bounds.IsEmpty())
                    pbi->Bounds.SetRect(bd.Coords);
                else
                    pbi->Bounds.Union(bd.Coords);

                pbi->Count++;
            }
        }
    } // for(pass...)
}


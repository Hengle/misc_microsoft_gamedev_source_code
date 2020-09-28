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

//------------------------------------------------------------------------
GFxFontCacheManager::GFxFontCacheManager(bool enableDynamicCache):
    GFxState(State_FontCacheManager),
    DynamicCacheEnabled(enableDynamicCache),
    MaxRasterScale(1),
    MaxVectorCacheSize(512),
    FauxItalicAngle(0.28f),
    FauxBoldRatio(0.045f),
    pCache(new GFxFontCacheManagerImpl())
{
    SetTextureConfig(TextureConfig());
}

GFxFontCacheManager::GFxFontCacheManager(const TextureConfig& config, bool enableDynamicCache):
    GFxState(State_FontCacheManager),
    DynamicCacheEnabled(enableDynamicCache),
    MaxRasterScale(1),
    MaxVectorCacheSize(512),
    FauxItalicAngle(0.3f),
    FauxBoldRatio(0.055f),
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
    pCache->Init(config);
    pCache->SetMaxVectorCacheSize(MaxVectorCacheSize);
    pCache->SetFauxItalicAngle(FauxItalicAngle);
    pCache->SetFauxBoldRatio(FauxBoldRatio);
}

void  GFxFontCacheManager::SetMaxVectorCacheSize(UInt n) 
{ 
    pCache->SetMaxVectorCacheSize(MaxVectorCacheSize = n);
}

void  GFxFontCacheManager::SetFauxItalicAngle(Float a) 
{ 
    pCache->SetFauxItalicAngle(FauxItalicAngle = a); 
}

void  GFxFontCacheManager::SetFauxBoldRatio(Float r) 
{ 
    pCache->SetFauxBoldRatio(FauxBoldRatio = r);
}

void GFxFontCacheManager::InitTextures(GRenderer* ren)
{
    pCache->InitTextures(ren);
}


#ifndef GFC_NO_GLYPH_CACHE
//UInt GFxFontCacheManagerImpl::FontSizeRamp[] = { 0,1,2,4,6,8,11,16,22,32,40,50,64,76,90,108,128,147,168,194,222,256,0};
UInt GFxFontCacheManagerImpl::FontSizeRamp[] = { 0,0,2,4,6,8,12,16,18,22,26,32,36,40,45,50,56,64,72,80,90,108,128,147,168,194,222,256,0};
//UInt GFxFontCacheManagerImpl::FontSizeRamp[] = { 0,1,2,4,8,16,32,64,128,256,0};
#endif


//------------------------------------------------------------------------
GFxFontCacheManagerImpl::GFxFontCacheManagerImpl():
    pRenderer(0),
    LockAllInFrame(false),
    RasterCacheWarning(true),
    VectorCacheWarning(true),
    MaxVectorCacheSize(512),
    FauxItalicAngle(0.3f),
    FauxBoldRatio(0.055f)
{
    ItalicMtx = GMatrix2D::Shearing(0, -FauxItalicAngle);

    FontDisposer.Bind(this);
    FrameHandler.Bind(this);
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
    GLock::Locker lock(&StateLock);
    InvalidateAll();
    BatchPackageQueue.RemoveAll();
    BatchPackageStorage.FreeAll();
    FontSetType::iterator it = KnownFonts.begin();    
    for (; it != KnownFonts.end(); ++it)
    {
        (*it)->RemoveDisposeHandler(&FontDisposer);
    }
    KnownFonts.clear();
    VectorGlyphShape* sh = VectorGlyphShapeList.GetFirst();
    while(!VectorGlyphShapeList.IsNull(sh))
    {
        sh->pShape->Release();
        sh = sh->pNext;
    }
    VectorGlyphShapeStorage.FreeAll();
    VectorGlyphCache.clear();
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
    if (pRenderer && LockAllInFrame) 
    {
        pRenderer->RemoveEventHandler(&FrameHandler);
        pRenderer = NULL;
    }
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::CleanUpFont(GFxFontResource* font)
{
    GLock::Locker lock(&StateLock);
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
    VectorGlyphShape* sh = VectorGlyphShapeList.GetFirst();
    while(!VectorGlyphShapeList.IsNull(sh))
    {
        VectorGlyphShape* next = sh->pNext;
        if (sh->pFont == font)
        {

            sh->pShape->Release();
            VectorGlyphCache.remove(VectorGlyphKey(sh->pFont, sh->GlyphIndex, sh->HintedGlyphSize, sh->Flags));
            VectorGlyphShapeList.Remove(sh);
            VectorGlyphShapeStorage.Free(sh);
        }
        sh = next;

    }
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::Init(const GFxFontCacheManager::TextureConfig& config)
{
#ifndef GFC_NO_GLYPH_CACHE
    Cache.Init(config.TextureWidth,
               config.TextureHeight,
               config.MaxNumTextures,
               config.MaxSlotHeight,
               config.SlotPadding,
               config.TexUpdWidth,
               config.TexUpdHeight);
#else
    GUNUSED(config);
#endif
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::SetFauxItalicAngle(Float a) 
{ 
    FauxItalicAngle = a;
    ItalicMtx = GMatrix2D::Shearing(0, -FauxItalicAngle); 
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
                                        const GFxGlyphParam& param,
                                        Float maxRasterScale) const
{
    if (param.BlurX || param.BlurY || param.Outline)
         return true;

    fontSize = UInt(heightRatio * fontSize + 0.5f);
    if (!param.IsOptRead())
        fontSize = snapFontSizeToRamp(fontSize);

    UInt h = (UInt)ceilf (bounds.Bottom * fontSize / 1024.0f) - 
             (UInt)floorf(bounds.Top    * fontSize / 1024.0f) + 1;
    return h < GetTextureGlyphMaxHeight() * maxRasterScale;
}


//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::setRenderer(GRenderer* ren)
{
    if (pRenderer != ren)
    {
        if (pRenderer)
            InvalidateAll();

        pRenderer = ren;
        LockAllInFrame = false;

        GRenderer::RenderCaps caps;
        pRenderer->GetRenderCaps(&caps);
        if (caps.CapBits & (GRenderer::Cap_KeepVertexData | GRenderer::Cap_NoTexOverwrite))
        {
            LockAllInFrame = true;
            pRenderer->AddEventHandler(&FrameHandler);
        }
    }
}

//------------------------------------------------------------------------
bool GFxFontCacheManagerImpl::VerifyBatchPackage(const GFxBatchPackage* bp, 
                                                 GFxDisplayContext &context,
                                                 Float heightRatio)
{
    setRenderer(context.GetRenderer());

    // Verify the package and the owner
    if (!bp || bp->Owner != this || !bp->Package || bp->Package->FailedGlyphs)
        return false;

#ifndef GFC_NO_GLYPH_CACHE
    // Verify glyphs
    const GFxBatchPackageData* bpd = bp->Package;

    UInt i;
    bool verifySize = heightRatio != 0;
    for (i = 0; i < bpd->BatchVerifier.size(); ++i)
    {
        const GFxBatchPackageData::GlyphVerifier& gv = bpd->BatchVerifier[i];

        if (verifySize && gv.pGlyph != 0)
        {
            UInt oldSize = gv.GlyphParam.GetFontSize();
            if (oldSize)
            {
                UInt newSize = snapFontSizeToRamp(UInt(gv.FontSize * heightRatio + 0.5f));
                if (newSize != oldSize)
                    return false;
            }
        }
        if (gv.pGlyph && !Cache.VerifyGlyphAndSendBack(gv.GlyphParam, gv.pGlyph))
            return false;
    }
#else
    GUNUSED(heightRatio);
#endif
//printf("*"); // DBG
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
    GLock::Locker lock(&StateLock);
    setRenderer(context.GetRenderer());

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
    GLock::Locker lock(&StateLock);
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
void GFxFontCacheManagerImpl::DisplayBatchPackage(GFxBatchPackage* bp, 
                                                  GFxDisplayContext &context, 
                                                  const Matrix& displayMatrix,
                                                  const Cxform& cx)
{
    if (bp && bp->Owner == this && bp->Package)
    {
        GFxBatchPackageData* pd = bp->Package;

        if (pd->Batch.size() > 0)
        {
            GRenderer*  prenderer = context.GetRenderer();
            prenderer->SetCxform(cx);

            // Render the batch piece by piece.
            GRenderer::CacheProvider cacheProvider(&pd->BatchCache);

            for (UInt layer = 0; layer < 2; ++layer)
            {
                GFxBatchPackageData::BatchDescHash::iterator it = pd->BatchDesc.begin();
                UInt batchIndex;
                for (batchIndex = 0; it != pd->BatchDesc.end(); ++it)
                {
                    if (it->first.Layer == layer)
                    {
                        prenderer->DrawBitmaps(&pd->Batch[0], (int)pd->Batch.size(),
                            it->second.Index, it->second.Count,
                            it->second.pTexture, displayMatrix, &cacheProvider);
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------
inline bool
GFxFontCacheManagerImpl::resolveTextureGlyph(GFxBatchPackageData::GlyphVerifier* gv, 
                                             const GFxGlyphParam& gp,
                                             bool canUseRaster,
                                             const GFxShapeCharacterDef* shape,
                                             GFxDisplayContext &context,
                                             bool canUseTgd)
{
    gv->GlyphParam    = gp;
#ifndef GFC_NO_GLYPH_CACHE
    gv->pGlyph        = 0;
#else
    GUNUSED(canUseRaster);
#endif
    gv->pTexture      = 0;
    gv->TextureWidth  = 0;
    gv->TextureHeight = 0;
    GFxTextureGlyphData* tgData = 0;
    
    if (canUseTgd || shape == 0)
        tgData = gp.pFont->GetTextureGlyphData();

    if (tgData)
    {
#ifndef GFC_NO_GLYPH_CACHE
        gv->FontSize      = 0;
        gv->GlyphScale    = 256;
#endif
        const GFxTextureGlyph& tg = tgData->GetTextureGlyph(gp.GlyphIndex);
        GImageInfoBase* pimage = tg.GetImageInfo(gp.pFont->GetBinding());
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
        gv->pGlyph = Cache.GetGlyph(context.GetRenderer(), 
                                    gp, 
                                    canUseRaster,
                                    shape, 
                                    gv->FontSize, 
                                    context.pLog);
        if (gv->pGlyph)
        {
            gv->pTexture = Cache.GetGlyphTexture(gv->pGlyph);
            Cache.LockGlyph(gv->pGlyph);
        }
        else
        {
            if (shape != 0 && context.pLog && RasterCacheWarning)
                context.pLog->LogWarning("Warning: Increase raster glyph cache capacity - TextureConfig.\n");
            RasterCacheWarning = false;
            return false;
        }
    }
#endif
    return true;
}


//------------------------------------------------------------------------
static bool GFx_ClipBitmapDesc(GFxBatchPackageData::BitmapDesc* bd, 
                               const GRectF& visibleRect)
{
    GRectF clipped = bd->Coords;
    clipped.Intersect(visibleRect);
    if (clipped.IsEmpty())
    {
        bd->Coords = visibleRect;
        bd->Coords.Right = bd->Coords.Left;
        bd->Coords.Bottom = bd->Coords.Top;
        bd->TextureCoords.Right = bd->TextureCoords.Left;
        bd->TextureCoords.Bottom = bd->TextureCoords.Top;
        return false;
    }

    if (clipped != bd->Coords)
    {
        GRectF clippedTexture = bd->TextureCoords;

        if (bd->Coords.Left != clipped.Left)
            clippedTexture.Left = bd->TextureCoords.Left + 
                                 (clipped.Left - bd->Coords.Left) * 
                                  bd->TextureCoords.Width() / 
                                  bd->Coords.Width();

        if (bd->Coords.Top != clipped.Top)
            clippedTexture.Top = bd->TextureCoords.Top + 
                                (clipped.Top - bd->Coords.Top) * 
                                 bd->TextureCoords.Height() / 
                                 bd->Coords.Height();

        if (bd->Coords.Right != clipped.Right)
            clippedTexture.Right = bd->TextureCoords.Right - 
                                  (bd->Coords.Right - clipped.Right) * 
                                   bd->TextureCoords.Width() / 
                                   bd->Coords.Width();

        if (bd->Coords.Bottom != clipped.Bottom)
            clippedTexture.Bottom = bd->TextureCoords.Bottom - 
                                   (bd->Coords.Bottom - clipped.Bottom) * 
                                    bd->TextureCoords.Height() / 
                                    bd->Coords.Height();
        bd->Coords = clipped;
        bd->TextureCoords = clippedTexture;
    }
    return true;    
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
    pd->FailedGlyphs = false;

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
    bool freeRotation = geom->SourceTextMatrix.IsFreeRotation();
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
        // When using a single texture it will be a single batch. 
        // If there are many textures and text has a shadow, it is 
        // possible than the shadow will be drawn after text. So that, 
        // it is necessary to break the batches so that they would have
        // separately shadow and glyph bitmaps
        //-----------------------
        layer = passStart;
#ifndef GFC_NO_GLYPH_CACHE
        const GFxGlyphParam& textParam = (pass == PassText)? 
            textFieldParam.TextParam: 
            textFieldParam.ShadowParam;

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

                GFxGlyphParam glyphParam;

                glyphParam.Clear();
                glyphParam.pFont      = presolvedFont;
                glyphParam.GlyphIndex = UInt16(glyph.GetIndex());

                GFxBatchPackageData::GlyphVerifier gv;
                const GFxShapeCharacterDef* shape = 0;
                bool canUseRaster = false;

#ifndef GFC_NO_GLYPH_CACHE
                Float screenFontSize  = geom->HeightRatio * glyph.GetFontSize();
                if (screenFontSize < 1)
                    screenFontSize = 1;
                UInt  snappedFontSize = UInt(screenFontSize + 0.5f);

                if (glyphParam.pFont->GetTextureGlyphData() == 0)// || pass == PassShadow)
                {
                    canUseRaster = 
                        textParam.IsOptRead() && 
                        textParam.GetBlurX() == 0 && 
                        textParam.GetBlurY() == 0 &&
                        pass == PassText &&
                        glyphParam.pFont->IsHintedRasterGlyph(glyphParam.GlyphIndex, glyphParam.FontSize);

                    shape = GetGlyphShape_NoLock(glyphParam.pFont, 
                                                 glyphParam.GlyphIndex, 
                                                 (textParam.IsOptRead() && pass == PassText)? 
                                                                             snappedFontSize : 0,
                                                  glyphIt.IsFauxBold(), 
                                                  glyphIt.IsFauxItalic(),
                                                  context.pLog);
                }

                if (shape)
                {
                    GRectF bounds;
                    if (shape->HasValidBounds())
                        bounds = shape->GetBound();
                    else
                        presolvedFont->GetGlyphBounds(glyph.GetIndex(), &bounds);

                    if (!textParam.IsOptRead())
                        snappedFontSize = snapFontSizeToRamp(snappedFontSize);

                    Float outline = textParam.GetOutline() * screenFontSize * GFx_OutlineRatio;
                    Float blurX   = textParam.GetBlurX() + outline;
                    Float blurY   = textParam.GetBlurY() + outline;
                    if (blurX > geom->BlurX) geom->BlurX = blurX;
                    if (blurY > geom->BlurY) geom->BlurY = blurY;

                    Cache.CalcGlyphParam(screenFontSize,
                                         snappedFontSize,
                                         geom->HeightRatio,
                                         bounds.Bottom - bounds.Top,
                                         textParam,
                                         &glyphParam,
                                         &gv.FontSize,
                                         &gv.GlyphScale);

                    if (textParam.IsOptRead() && 
                        shape->GetHintedGlyphSize() == 0 &&
                        !canUseRaster &&
                        textParam.BlurX == 0)
                    {
                        Float glyphWidth = (bounds.Right - bounds.Left) * glyphParam.FontSize / 1024.0f;
                        if (3*glyphWidth < Cache.GetMaxGlyphHeight())
                        {
                            glyphParam.SetStretch(true);
                        }
                    }
                }
                else
                {
                    glyphParam.SetFontSize(snappedFontSize);
                    glyphParam.Flags = textParam.Flags;
                    gv.FontSize      = UInt16(snappedFontSize * SubpixelSizeScale);
                    gv.GlyphScale    = 256;
                }
#endif

                glyphParam.SetFauxBold  (glyphIt.IsFauxBold());
                glyphParam.SetFauxItalic(glyphIt.IsFauxItalic());
                if (!resolveTextureGlyph(&gv, glyphParam, canUseRaster, shape, context, true))//pass == PassText))
                    pd->FailedGlyphs = true;

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
                    geom->SetCheckPreciseScale();
                }
            }
        }
    }  // for(pass...)


    GRectF visibleRect = geom->VisibleRect;

    visibleRect.Left   -= geom->BlurX * 20;
    visibleRect.Top    -= geom->BlurY * 20;
    visibleRect.Right  += geom->BlurX * 20;
    visibleRect.Bottom += geom->BlurY * 20;

    if (textFieldParam.ShadowColor.GetAlpha() != 0)
    {
        visibleRect.Left   += (Float)textFieldParam.ShadowOffsetX;
        visibleRect.Top    += (Float)textFieldParam.ShadowOffsetY;
        visibleRect.Right  += (Float)textFieldParam.ShadowOffsetX;
        visibleRect.Bottom += (Float)textFieldParam.ShadowOffsetY;
        visibleRect.Union(geom->VisibleRect);
    }

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
        layer = passStart;
#ifndef GFC_NO_GLYPH_CACHE
        const GFxGlyphParam& textParam = (pass == PassText)? 
            textFieldParam.TextParam: 
            textFieldParam.ShadowParam;

        if (Cache.GetMaxNumTextures() > 1)
            layer = pass;
#endif

        for(linesIt = linesIter; linesIt.IsVisible(); ++linesIt)
        {
            GFxTextLineBuffer::Line& line = *linesIt;
            GPointF offset = line.GetOffset();

            offset += lnOffset;
            offset.x -= Float(geom->HScrollOffset);
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
            GFxTextLineBuffer::GlyphIterator glyphIt = line.Begin(linesIt.GetHighlighter());
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
                bool snapX  = false;
                if (gv.pGlyph)
                {
                    if (!LockAllInFrame)
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
                    gv.FontSize = (UInt16)glyph.GetFontSize();
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

                GFx_ClipBitmapDesc(&bd, visibleRect);
                pbi->Count++;
            }
        }
    } // for(pass...)
}


//------------------------------------------------------------------------
bool GFxFontCacheManagerImpl::isOuterContourCW(const GCompoundShape& shape) const
{
    UInt i, j;

    Float minX1 =  1e10f;
    Float minY1 =  1e10f;
    Float maxX1 = -1e10f;
    Float maxY1 = -1e10f;
    Float minX2 =  1e10f;
    Float minY2 =  1e10f;
    Float maxX2 = -1e10f;
    Float maxY2 = -1e10f;
    bool cw = true;

    for(i = 0; i < shape.GetNumPaths(); ++i)
    {
        const GCompoundShape::SPath& path = shape.GetPath(i);
        if(path.GetNumVertices() > 2)
        {
            GPointType v1 = path.GetVertex(path.GetNumVertices() - 1);
            Float sum = 0;
            for(j = 0; j < path.GetNumVertices(); ++j)
            {
                const GPointType& v2 = path.GetVertex(j);
                if(v2.x < minX1) minX1 = v2.x;
                if(v2.y < minY1) minY1 = v2.y;
                if(v2.x > maxX1) maxX1 = v2.x;
                if(v2.y > maxY1) maxY1 = v2.y;
                sum += v1.x * v2.y - v1.y * v2.x;
                v1 = v2;
            }

            if(minX1 < minX2 || minY1 < minY2 || maxX1 > maxX2 || maxY1 > maxY2)
            {
                minX2 = minX1;
                minY2 = minY1;
                maxX2 = maxX1;
                maxY2 = maxY1;
                cw = sum > 0;
            }
        }
    }
    return cw;
}


//------------------------------------------------------------------------
const GRectF& GFxFontCacheManagerImpl::AdjustBounds(GRectF* bounds, 
                                                    bool fauxBold, bool fauxItalic) const
{
    if (fauxBold)
    {
        Float fauxBoldWidth = FauxBoldRatio * 1024.0f;
        bounds->Left  -= fauxBoldWidth;
        bounds->Right += fauxBoldWidth;
    }
    if (fauxItalic)
    {
        GPointF p1, p2;
        p1.x = bounds->Left;
        p1.y = bounds->Top;
        p2.x = bounds->Right;
        p2.y = bounds->Bottom;
        p1 = ItalicMtx.Transform(p1);
        p2 = ItalicMtx.Transform(p2);
        bounds->Left  = p1.x;
        bounds->Right = p2.x;
    }
    return *bounds;
}


//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::copyAndTransformShape(GFxShapeNoStylesDef* dst, 
                                                    const GFxShapeCharacterDef* src,
                                                    GRectF* bbox,
                                                    bool fauxBold, bool fauxItalic)
{
    if (!src)
        return;
    GFxPathPacker path;
    Float fauxBoldWidth = FauxBoldRatio * 1024.0f;
    GPtr<GFxShapeNoStylesDef> fauxBoldShape;
    if (fauxBold)
    {
        fauxBoldShape = *new GFxShapeNoStylesDef(ShapePageSize * 4);
        src->MakeCompoundShape(&TmpShape1, 10);
        TmpShape1.ScaleAndTranslate(1, 1000, 0, 0);
        Stroker.SetWidth(isOuterContourCW(TmpShape1) ? fauxBoldWidth : -fauxBoldWidth);
        Stroker.SetLineJoin(GStrokerTypes::MiterJoin);
        TmpShape2.RemoveAll();
        Stroker.GenerateContour(TmpShape1, -1, TmpShape2);
        UInt i, j;
        for(i = 0; i < TmpShape2.GetNumPaths(); ++i)
        {
            const GCompoundShape::SPath& path2 = TmpShape2.GetPath(i);
            if (path2.GetNumVertices() > 2)
            {
                const GPointType& p = path2.GetVertex(0);
                path.Reset();
                path.SetFill0(1);
                path.SetFill1(0);
                path.SetMoveTo(SInt(p.x), SInt(p.y * 0.001f));
                for(j = 1; j < path2.GetNumVertices(); ++j)
                {
                    const GPointType& p = path2.GetVertex(j);
                    path.LineToAbs(SInt(p.x), SInt(p.y * 0.001f));
                }
                fauxBoldShape->AddPath(&path);
            }
        }
        src = fauxBoldShape;
    }

    GPtr<GFxShapeCharacterDef::PathsIterator> ppathsIt = *src->GetPathsIterator();
    GFxShapeCharacterDef::PathsIterator::StateInfo info;

    GPointF p1, p2;

    while(ppathsIt->GetNext(&info))
    {
        switch (info.State)
        {
        case GFxShapeCharacterDef::PathsIterator::StateInfo::St_Setup:
            {
                Float ax = info.Setup.MoveX, ay = info.Setup.MoveY;

                path.Reset();
                path.SetFill0(1);
                path.SetFill1(0);

                p1.x = ax;
                p1.y = ay;
                if (fauxItalic)
                    p1 = ItalicMtx.Transform(p1);
                path.SetMoveTo(SInt(p1.x), SInt(p1.y));
            }
            break;
        case GFxShapeCharacterDef::PathsIterator::StateInfo::St_Edge:
            if (info.Edge.Curve)
            {
                p1.x = info.Edge.Cx;
                p1.y = info.Edge.Cy;
                p2.x = info.Edge.Ax;
                p2.y = info.Edge.Ay;
                if (fauxItalic)
                {
                    p1 = ItalicMtx.Transform(p1);
                    p2 = ItalicMtx.Transform(p2);
                }
                path.CurveToAbs(SInt(p1.x), SInt(p1.y), SInt(p2.x), SInt(p2.y));
            }
            else
            {
                p1.x = info.Edge.Ax;
                p1.y = info.Edge.Ay;
                if (fauxItalic)
                    p1 = ItalicMtx.Transform(p1);
                path.LineToAbs(SInt(p1.x), SInt(p1.y));
            }
            break;
        case GFxShapeCharacterDef::PathsIterator::StateInfo::St_NewPath:
        case GFxShapeCharacterDef::PathsIterator::StateInfo::St_NewShape:
            dst->AddPath(&path);
            break;
        default:;
        }
    }

    if (fauxBold || fauxItalic)
    {
        dst->ComputeBound(bbox);
    }
}

//------------------------------------------------------------------------
GFxShapeCharacterDef* 
GFxFontCacheManagerImpl::GetGlyphShape(GFxFontResource* font, UInt index, UInt size, 
                                       bool fauxBold, bool fauxItalic,
                                       GFxLog* log)
{
//return font->GetGlyphShape(index, 0); // DBG
    GLock::Locker guard(&StateLock);
    return GetGlyphShape_NoLock(font,index, size, fauxBold, fauxItalic, log);
}
GFxShapeCharacterDef* 
GFxFontCacheManagerImpl::GetGlyphShape_NoLock(GFxFontResource* font, UInt index, UInt size, 
                                              bool fauxBold, bool fauxItalic,
                                              GFxLog* log)
{
//return font->GetGlyphShape(index, 0); // DBG
    if (!KnownFonts.get(font))
    {
        KnownFonts.add(font);
        font->AddDisposeHandler(&FontDisposer);
    }

    // At this point the font size makes sense only if the font
    // supports native hinting. Otherwise it will be just a waste 
    // of cache capacity. Also, check for the maximum hinted size.
    if (!font->HasNativeHinting() ||
        !font->IsHintedVectorGlyph(index, size))
    {
        size = 0;
    }

    UInt flags = (fauxBold   ? GFxFont::FF_Bold : 0) |
                 (fauxItalic ? GFxFont::FF_Italic : 0);

    VectorGlyphShape** shapePtr = VectorGlyphCache.get(VectorGlyphKey(font, index, size, flags));
    if (shapePtr)
    {
        VectorGlyphShapeList.SendToBack(*shapePtr);
        if (LockAllInFrame)
            (*shapePtr)->Flags |= VectorGlyphShape::LockGlyphFlag;
        return (*shapePtr)->pShape;
    }

    GFxShapeCharacterDef* srcShapePtr = font->GetGlyphShape(index, size);
    if (srcShapePtr == 0)
        return 0;

    VectorGlyphShape* shape;
    if (VectorGlyphCache.size() >= MaxVectorCacheSize)
    {
        shape = VectorGlyphShapeList.GetFirst();
        if (shape->Flags & VectorGlyphShape::LockGlyphFlag)
        {
            if (log && VectorCacheWarning)
                log->LogWarning("Warning: Increase vector glyph cache capacity - SetMaxVectorCacheSize().\n");
            VectorCacheWarning = false;
            return 0;
        }

        shape->pShape->Release();
        VectorGlyphCache.remove(VectorGlyphKey(shape->pFont, 
                                               shape->GlyphIndex, 
                                               shape->HintedGlyphSize,
                                               shape->Flags));
        VectorGlyphShapeList.Remove(shape);
        VectorGlyphShapeStorage.Free(shape);
    }

    shape = VectorGlyphShapeStorage.Allocate();
    shape->pFont            = font;
    shape->GlyphIndex       = (UInt16)index;
    shape->HintedGlyphSize  = (UInt8)size;
    shape->Flags            = (UInt8)flags;
    shape->pShape           = new GFxShapeNoStylesDef(ShapePageSize);

    GRectF bbox;

    if (srcShapePtr->GetHintedGlyphSize())
        srcShapePtr->ComputeBound(&bbox);
    else
        font->GetGlyphBounds(index, &bbox);

    copyAndTransformShape(shape->pShape, srcShapePtr, &bbox, fauxBold, fauxItalic);
    shape->pShape->SetBound(bbox);
    shape->pShape->SetValidBoundsFlag(true);
    shape->pShape->SetNonZeroFill(true);
    shape->pShape->SetHintedGlyphSize(srcShapePtr->GetHintedGlyphSize());

    VectorGlyphShapeList.PushBack(shape);
    VectorGlyphCache.add(VectorGlyphKey(font, index, size, flags), shape);

    if (LockAllInFrame)
        shape->Flags |= VectorGlyphShape::LockGlyphFlag;

    return shape->pShape;
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::UnlockAllGlyphs()
{
    GLock::Locker guard(&StateLock);
    if (LockAllInFrame)
    {
        VectorGlyphShape* shape = VectorGlyphShapeList.GetFirst();
        while(!VectorGlyphShapeList.IsNull(shape))
        {
            shape->Flags &= ~VectorGlyphShape::LockGlyphFlag;
            shape = VectorGlyphShapeList.GetNext(shape);
        }
#ifndef GFC_NO_GLYPH_CACHE
        Cache.UnlockAllGlyphs();
#endif
    }
    RasterCacheWarning = true;
    VectorCacheWarning = true;
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::RenEventHandler::OnEvent(GRenderer* prenderer, GRenderer::EventHandler::EventType eventType)
{
    pSelf->UnlockAllGlyphs();
    if (eventType == GRenderer::EventHandler::Event_RendererReleased)
    {
        if (pSelf->pRenderer && pSelf->pRenderer == prenderer)
        {
            pSelf->pRenderer->RemoveEventHandler(&pSelf->FrameHandler);
            pSelf->pRenderer = NULL;
        }
    }
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::InitTextures(GRenderer* ren)
{
    setRenderer(ren);
#ifndef GFC_NO_GLYPH_CACHE
    Cache.InitTextures(ren);
#endif
}


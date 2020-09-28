/**********************************************************************

Filename    :   GFxFontGlyphPacker.cpp
Content     :   GFxFontGlyphPacker implementation
Created     :   6/14/2007
Authors     :   Maxim Shemanarev, Artyom Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxFontGlyphPacker.h"


#ifndef GFC_NO_FONT_GLYPH_PACKER

// The font glyphs are cached into textures; these textures are rendered 
// in software using shape rasterization with anti-aliasing. A packing
// algorithm is used to pack individual glyph images into textures.

//------------------------------------------------------------------------
GFxFontGlyphPacker::GFxFontGlyphPacker(GFxFontPackParams* params,
                                       GFxImageCreator *pimageCreator,
                                       GFxRenderConfig *prenderConfig,
                                       GFxLog* plog,
                                       GFxResourceId* ptextureIdGen,
                                       bool threadedLoading):
    pFontPackParams(params),
    pTextureIdGen(ptextureIdGen),
    pImageCreator(pimageCreator),
    pRenderConfig(prenderConfig),
    pLog(plog),
    ThreadedLoading(threadedLoading)
{
    if (pFontPackParams)
        pFontPackParams->GetTextureConfig(&PackTextureConfig);
    Packer.SetWidth (PackTextureConfig.TextureWidth);
    Packer.SetHeight(PackTextureConfig.TextureHeight);
}

//------------------------------------------------------------------------
GFxFontGlyphPacker::~GFxFontGlyphPacker()
{
}

//------------------------------------------------------------------------
void GFxFontGlyphPacker::generateGlyphInfo(GTL::garray<GlyphInfo>* glyphs, 
                                           GFxFontResource* f)
{
    GASSERT(glyphs != 0 && f != 0);

    if (f->GetGlyphShapeCount() == 0)
        return;

    // The font must not have texture glyph data yet; if it does, as might happen
    // for loaded pre-generated textures, it is filtered out by our caller function.       
    GASSERT(f->GetTextureGlyphData() == 0);

    GPtr<GFxTextureGlyphData> ptextureGlyphData = 
                        *new GFxTextureGlyphData(f->GetGlyphShapeCount());
    ptextureGlyphData->SetTextureConfig(PackTextureConfig);
    f->SetTextureGlyphData(ptextureGlyphData);

    Float glyphToPix = PackTextureConfig.NominalSize / GFxFontPackParams::GlyphBoundBox;
    UInt i;
    UInt n = f->GetGlyphShapeCount();

    for (i = 0; i < n; i++)
    {
        const GFxTextureGlyph& tg = ptextureGlyphData->GetTextureGlyph(i);

        // If image has not yet been initialized
        if (!tg.HasImageResource())
        {
            GFxShapeCharacterDef*   sh = f->GetGlyphShape(i, 0);
            if (sh)
            {
                GRectF  glyphBounds;
                sh->ComputeBound(&glyphBounds);

                if (glyphBounds.Width() > 0 && glyphBounds.Height() > 0)
                {
                    // Add a glyph.
                    GlyphInfo gi;
                    gi.PixBounds.Left   = int(floorf(glyphBounds.Left   * glyphToPix)) - PackTextureConfig.PadPixels;
                    gi.PixBounds.Top    = int(floorf(glyphBounds.Top    * glyphToPix)) - PackTextureConfig.PadPixels;
                    gi.PixBounds.Right  = int(ceilf (glyphBounds.Right  * glyphToPix)) + PackTextureConfig.PadPixels;
                    gi.PixBounds.Bottom = int(ceilf (glyphBounds.Bottom * glyphToPix)) + PackTextureConfig.PadPixels;
                    gi.PixOrigin.x      = 0;
                    gi.PixOrigin.y      = 0;

                    if (gi.PixBounds.Width() > 0 && gi.PixBounds.Height() > 0)
                    {
                        gi.pFont = f;
                        gi.GlyphIndex  = i;
                        gi.GlyphReuse  = ~0U;  
                        gi.TextureIdx  = ~0U;

                        // Try to reuse the glyph
                        GlyphGeometryKey key(f, sh, sh->ComputeGeometryHash());
                        const UInt*      val = GlyphGeometryHash.get(key);
                        if (val)
                        {
                            gi.GlyphReuse = *val;
                        }
                        else
                        {
                            UInt glyphIdx = (UInt)glyphs->size();
                            GlyphGeometryHash.add(key, glyphIdx);
                        }
                        glyphs->push_back(gi);
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------
UInt GFxFontGlyphPacker::packGlyphRects(GTL::garray<GlyphInfo>* glyphs, 
                                        UInt start, UInt end, UInt texIdx)
{
    Packer.RemoveAll();
    UInt i, j, w, h;
    for (i = start; i < end; i++)
    {
        const GlyphInfo& gi = (*glyphs)[i];
        if(gi.GlyphReuse == ~0U)
        {
            w = gi.PixBounds.Right - gi.PixBounds.Left;
            h = gi.PixBounds.Bottom - gi.PixBounds.Top;
            Packer.AddRect(w, h, i);
        }
    }
    Packer.Pack();
    for (i = 0; i < Packer.GetNumPacks(); i++)
    {
        const GRectPacker::PackType& pack = Packer.GetPack(i);
        for(j = 0; j < pack.NumRects; j++)
        {
            const GRectPacker::RectType& rect = Packer.GetRect(pack, j);
            GlyphInfo& gi = (*glyphs)[rect.Id];
            w = gi.PixBounds.Right - gi.PixBounds.Left;
            h = gi.PixBounds.Bottom - gi.PixBounds.Top;
            gi.PixOrigin.x      = int(rect.x) - gi.PixBounds.Left;
            gi.PixOrigin.y      = int(rect.y) - gi.PixBounds.Top;
            gi.PixBounds.Left   = int(rect.x);
            gi.PixBounds.Top    = int(rect.y);
            gi.PixBounds.Right  = int(rect.x + w);
            gi.PixBounds.Bottom = int(rect.y + h);
            gi.TextureIdx = texIdx + i;
        }
    }
    return texIdx + Packer.GetNumPacks();
}

//------------------------------------------------------------------------
UInt GFxFontGlyphPacker::packGlyphRects(GTL::garray<GlyphInfo>* glyphs)
{
    UInt numTextures = 0;
    if(pFontPackParams->GetUseSeparateTextures())
    {
        UInt i;
        UInt start = 0;
        for (i = 1; i < glyphs->size(); i++)
        {
            if ((*glyphs)[i-1].pFont != (*glyphs)[i].pFont)
            {
                numTextures = packGlyphRects(glyphs, start, i, numTextures);
                start = i;
            }
        }
        numTextures = packGlyphRects(glyphs, start, (UInt)glyphs->size(), numTextures);
    }
    else
    {
        numTextures = packGlyphRects(glyphs, 0, (UInt)glyphs->size(), 0);
    }
    return numTextures;
}

//------------------------------------------------------------------------
void GFxFontGlyphPacker::rasterizeGlyph(GImage* texImage, GlyphInfo* gi)
{
    GFxShapeCharacterDef* sh = gi->pFont->GetGlyphShape(gi->GlyphIndex, 0);
    if (sh)
    {
        sh->MakeCompoundShape(&CompoundShape, 
            GFxFontPackParams::GlyphBoundBox / PackTextureConfig.NominalSize * 0.5f);

        Rasterizer.Reset();
        Rasterizer.AddShape(CompoundShape, 
                            PackTextureConfig.NominalSize / 
                            GFxFontPackParams::GlyphBoundBox);

        if(Rasterizer.SortCells()) // If there is anything to sweep...
        {
            // Sweep the raster writing the scan lines to the image.
            UInt h = Rasterizer.GetMaxY() - Rasterizer.GetMinY() + 1;
            int  x = gi->PixBounds.Left + PackTextureConfig.PadPixels;
            int  y = gi->PixBounds.Top  + PackTextureConfig.PadPixels;
            for (UInt i = 0; i < h; i++)
            {
                Rasterizer.SweepScanline(i, texImage->GetScanline(y+i) + x);
            }
        }
    }
}

//------------------------------------------------------------------------
void GFxFontGlyphPacker::generateTextures(GTL::garray<GlyphInfo>* glyphs, UInt numTextures)
{
    UInt i, j;

    for (i = 0; i < numTextures; i++)
    {
        UInt maxWidth  = 0;
        UInt maxHeight = 0;
        for(j = 0; j < glyphs->size(); j++)
        {
            const GlyphInfo& gi = (*glyphs)[j];
            if (gi.TextureIdx == i)
            {
                if (gi.PixBounds.Right > (int)maxWidth)
                    maxWidth = gi.PixBounds.Right;

                if (gi.PixBounds.Bottom > (int)maxHeight)
                    maxHeight = gi.PixBounds.Bottom;
            }
        }
    
        UInt texWidth  = PackTextureConfig.TextureWidth;
        UInt texHeight = PackTextureConfig.TextureHeight;
        if (maxWidth <= (UInt)PackTextureConfig.TextureWidth / 2)
        {
            texWidth = 1;
            while (texWidth < maxWidth) texWidth <<= 1;
        }
        if (maxHeight <= (UInt)PackTextureConfig.TextureHeight / 2)
        {
            texHeight = 1;
            while (texHeight < maxHeight) texHeight <<= 1;
        }


        GImage* texImage = new GImage(GImage::Image_A_8,
                                      texWidth, 
                                      texHeight);
        memset(texImage->pData, 0, texWidth * texHeight);

        for(j = 0; j < glyphs->size(); j++)
        {
            GlyphInfo& gi = (*glyphs)[j];
            if (gi.TextureIdx == i)
            {
                rasterizeGlyph(texImage, &gi);
            }
        }

        Float pixToU = 1.0f / texImage->Width;
        Float pixToV = 1.0f / texImage->Height;
        GFxResourceId          textureId = pTextureIdGen->GenerateNextId();
        GFxImageCreateInfo     icreateInfo(texImage, GFxResource::Use_FontTexture,
                                           pRenderConfig);
        icreateInfo.ThreadedLoading = ThreadedLoading;
        GPtr<GImageInfoBase>   pimageInfo = *pImageCreator->CreateImage(icreateInfo);
        GPtr<GFxImageResource> pimageRes  = *new GFxImageResource(pimageInfo.GetPtr(), 
                                                                  GFxResource::Use_FontTexture);
        texImage->Release();

        for(j = 0; j < glyphs->size(); j++)
        {
            GlyphInfo gi = (*glyphs)[j];
            if (gi.GlyphReuse != ~0U)
            {
                // If the glyph is reused switch to the original.
                const GlyphInfo& gi2 = (*glyphs)[gi.GlyphReuse];
                GASSERT(gi.pFont == gi2.pFont); // Font must be the same
                gi.TextureIdx = gi2.TextureIdx;
                gi.PixBounds  = gi2.PixBounds;
                gi.PixOrigin  = gi2.PixOrigin;
            }
            if (gi.TextureIdx == i)
            {
                GPtr<GFxTextureGlyph> tg = *new GFxTextureGlyph();
                tg->pImage          = pimageRes;
                tg->UvBounds.Left   = Float(gi.PixBounds.Left)   * pixToU;
                tg->UvBounds.Top    = Float(gi.PixBounds.Top)    * pixToV;
                tg->UvBounds.Right  = Float(gi.PixBounds.Right)  * pixToU;
                tg->UvBounds.Bottom = Float(gi.PixBounds.Bottom) * pixToV;
                tg->UvOrigin.x      = Float(gi.PixOrigin.x)      * pixToU;
                tg->UvOrigin.y      = Float(gi.PixOrigin.y)      * pixToV;

                GFxTextureGlyphData* tgd = gi.pFont->GetTextureGlyphData();
                tg->SetImageResource(pimageRes.GetPtr());

                // The texture is only added to font data.
                // This means that VisitFonts will need to collect fonts,
                // matching them through a hash-table if necessary.
                tgd->AddTextureGlyph(gi.GlyphIndex, *tg);

                // add textureId -> texture assoc into the font
                tgd->AddTexture(textureId, pimageRes);
            }
        }
    }
}

//#include <time.h>

//------------------------------------------------------------------------
void GFxFontGlyphPacker::GenerateFontBitmaps(const GTL::garray<GFxFontResource*>& fonts)
{
//clock_t cl = clock();
    UInt i;
    UInt totalNumGlyphs = 0;
    for (i = 0; i < fonts.size(); i++)
    {
        if (fonts[i]->GetTextureGlyphData() != 0)
            continue;

        if (pFontPackParams->GetGlyphCountLimit() && 
            (int)fonts[i]->GetGlyphShapeCount() > pFontPackParams->GetGlyphCountLimit())
            continue;

        totalNumGlyphs += fonts[i]->GetGlyphShapeCount();
    }

    GTL::garray<GlyphInfo> glyphInfo;
    glyphInfo.reserve(totalNumGlyphs);
    GlyphGeometryHash.clear();
    for (i = 0; i < fonts.size(); i++)
    {
        if (fonts[i]->GetTextureGlyphData() != 0)
            continue;

        if (pFontPackParams->GetGlyphCountLimit() && 
            (int)fonts[i]->GetGlyphShapeCount() > pFontPackParams->GetGlyphCountLimit())
            continue;

        generateGlyphInfo(&glyphInfo, fonts[i]);
    }

    UInt numTextures = packGlyphRects(&glyphInfo);
    generateTextures(&glyphInfo, numTextures);
//printf("%d %f\n", glyphInfo.size(), double(clock() - cl) / CLOCKS_PER_SEC);
}

#endif //GFC_NO_FONT_GLYPH_PACKER






///////////////////////////////////////////////////////////
//
// ***** GFxFontPackParams
//


// Size (in TWIPS) of the box that the glyph should stay within.
// this *should* be 1024, but some glyphs in some fonts exceed it!
//------------------------------------------------------------------------
const Float GFxFontPackParams::GlyphBoundBox = 1536.0f;


//------------------------------------------------------------------------
void GFxFontPackParams::SetTextureConfig(const TextureConfig& config)
{
    PackTextureConfig = config;

    const int    minSize = 4;
    const int    maxSize = PackTextureConfig.TextureHeight / 2;

    if (PackTextureConfig.NominalSize < minSize)
    {
        GFC_DEBUG_WARNING2(1, "SetTextureConfig - NominalSize (%d) too small, clamping to %d\n",
                              PackTextureConfig.NominalSize, minSize);
        PackTextureConfig.NominalSize = minSize;
    }
    else if (PackTextureConfig.NominalSize > maxSize)
    {
        GFC_DEBUG_WARNING2(1, "SetTextureConfig - NominalSize (%d) too large, clamping to %d\n",
                              PackTextureConfig.NominalSize, maxSize);
        PackTextureConfig.NominalSize = maxSize;
    }

    if (config.PadPixels < 1)
    {
        GFC_DEBUG_WARNING2(1, "SetTextureConfig - PadPixels (%d) too small, clamping to %d\n",
                              PackTextureConfig.PadPixels, 1);
        PackTextureConfig.PadPixels = minSize;
    }
}


//------------------------------------------------------------------------
#ifdef GFC_ASSERT_ON_FONT_BITMAP_GEN
#define GASSERT_ON_FONT_BITMAP_GEN GASSERT(0)
#else
#define GASSERT_ON_FONT_BITMAP_GEN ((void)0)
#endif //GFC_ASSERT_ON_FONT_BITMAP_GEN



// Build cached textures from glyph outlines.
//------------------------------------------------------------------------
void    GFx_GenerateFontBitmaps(GFxFontPackParams *params,
                                const GTL::garray<GFxFontResource*>& fonts,
                                GFxImageCreator *pimageCreator,
                                GFxRenderConfig *prenderConfig,
                                GFxLog* plog,
                                GFxResourceId* pidGenerator,
                                bool threadedLoading)
{
    if (!params)
    {
        // User should not be calling us with no font params. Null params are checked
        // by caller during movieDef binding, ensuring that correct warning can be issued
        // in case when there is also no dynamic cache.
        return;
    }
    if (!pimageCreator)
    {
        GFC_DEBUG_WARNING(1, "Bitmap font texture gen failed - GFxImageCreator not installed");
        return;
    }

    GASSERT(pidGenerator != 0);

  //if (powner->IsRenderingFonts())
    {
        GASSERT_ON_FONT_BITMAP_GEN;
#ifndef GFC_NO_FONT_GLYPH_PACKER
        GFxFontGlyphPacker  packer(params, pimageCreator, prenderConfig,
                                   plog, pidGenerator, threadedLoading);
        packer.GenerateFontBitmaps(fonts);
#else
        GUNUSED4(fonts, prenderConfig, plog, pidGenerator);        
        GUNUSED(threadedLoading);        
#endif//GFC_NO_FONT_GLYPH_PACKER
    }
}

//------------------------------------------------------------------------
Float   GFxFontPackParams::GetDrawGlyphScale(int nominalGlyphHeight)
{
    // Scale from uv coords to the 1024x1024 glyph square.
    // This equation used to be in GFxFontLibImpl::DrawGlyph
    return PackTextureConfig.TextureHeight * GFxFontPackParams::GlyphBoundBox / nominalGlyphHeight;
}

//------------------------------------------------------------------------
Float   GFxFontPackParams::GetTextureGlyphMaxHeight(const GFxFontResource* f)
{
    TextureConfig conf;
    f->GetTextureConfig(&conf);
    return 1024.0f / GFxFontPackParams::GlyphBoundBox * conf.NominalSize; 
}


/**********************************************************************

Filename    :   GFxFontCacheManager.h
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

#ifndef INC_GFxFontCacheManager_H
#define INC_GFxFontCacheManager_H

#include "GTLTypes.h"
#include "GFxTextLineBuffer.h" 
#include "GRenderer.h"
#include "GContainers.h"

#ifndef GFC_NO_GLYPH_CACHE
#include "GFxGlyphCache.h"
#endif


//------------------------------------------------------------------------
struct GFxBatchPackageData : public GNewOverrideBase
{
    typedef GRenderer::BitmapDesc   BitmapDesc;

    // Batching
    //--------------------------------------------------------------------
    struct BatchInfo
    {   
        UInt                    Index;          // Index of first BitmapDesc
        UInt                    ImageUseCount;  // Expected image use count
        UInt                    Count;          // Running count        
        const GTexture*         pTexture;
        GRectF                  Bounds;

        void Clear()
        { 
            Count = ImageUseCount = Index = 0; 
            pTexture = 0; 
            Bounds.Clear(); 
        }
    };

    //--------------------------------------------------------------------
    struct GlyphVerifier
    {
        GFxGlyphParam    GlyphParam;
#ifndef GFC_NO_GLYPH_CACHE
        GFxGlyphNode*    pGlyph;
        UInt16           FontSize;
        UInt16           GlyphScale;
#endif
        const GTexture*  pTexture;
        UInt16           TextureWidth;
        UInt16           TextureHeight;
    };

    //--------------------------------------------------------------------
    struct BatchInfoKey
    {
    public:
        const GTexture* pTexture;
        UInt            Layer;      // 0=Shadow, 1=Text

        BatchInfoKey(): pTexture(0) {}
        BatchInfoKey(const GTexture* pt, UInt layer) : pTexture(pt), Layer(layer) {}

        size_t  operator()(const BatchInfoKey& data) const
        {
            return (((size_t)data.pTexture) >> 6) ^ 
                     (size_t)data.pTexture ^
                     (size_t)data.Layer;
        }
        bool operator== (const BatchInfoKey& bik) const 
        { 
            return pTexture == bik.pTexture && Layer == bik.Layer; 
        }
    };

    typedef GTL::ghash<BatchInfoKey, BatchInfo, BatchInfoKey> BatchDescHash;

    //--------------------------------------------------------------------
    BatchDescHash               BatchDesc;
    GTL::garray<BitmapDesc>     Batch;
    GTL::garray<GlyphVerifier>  BatchVerifier;
    GRenderer::CachedData       BatchCache;
    bool                        VectorRenderingRequired;

    void RemoveAll() 
    { 
        BatchDesc.clear();
        Batch.resize(0);
        BatchVerifier.resize(0);
        BatchCache.ReleaseData(GRenderer::Cached_BitmapList);
        VectorRenderingRequired = false;
    }

    GINLINE ~GFxBatchPackageData()
    {
        BatchCache.ReleaseData(GRenderer::Cached_BitmapList);
    }
};




//------------------------------------------------------------------------
struct GFxBatchPackage : public GPodDListNode<GFxBatchPackage>
{
    GFxBatchPackageData*     Package;
    GFxFontCacheManagerImpl* Owner;
};

//------------------------------------------------------------------------
struct GFxFontHashOp
{
    size_t operator()(const GFxFontResource* ptr)
    {
        return (((size_t)ptr) >> 6) ^ (size_t)ptr;
    }
};

//------------------------------------------------------------------------
class GFxFontCacheManagerImpl : public GNewOverrideBase
{
    enum { SubpixelSizeScale = GFxGlyphRasterCache::SubpixelSizeScale };

    class FontDisposeHandler : public GFxFontResource::DisposeHandler
    {
    public:
        void Bind(GFxFontCacheManagerImpl* cache) { pCache = cache; }
        virtual void OnDispose(GFxFontResource* font)
        {
            pCache->CleanUpFont(font);
        }

    private:
        GFxFontCacheManagerImpl* pCache;
    };


public:
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;
    typedef GRenderer::BitmapDesc   BitmapDec;

    GFxFontCacheManagerImpl();
   ~GFxFontCacheManagerImpl();

    void                Init(UInt texWidth, UInt texHeight, UInt maxNumTextures, 
                             UInt maxSlotHeight, UInt slotPadding,
                             UInt texUpdWidth, UInt texUpdHeight);

    void                InvalidateAll();
    void                RemoveAll();

    bool                VerifyBatchPackage(const GFxBatchPackage* bp, 
                                           GFxDisplayContext &context,
                                           Float heightRatio);

    GFxBatchPackage*    CreateBatchPackage(GFxBatchPackage* bp, 
                                           const GFxTextLineBuffer::Iterator& linesIt, 
                                           GFxDisplayContext &context, 
                                           const GPointF& lnOffset,
                                           GFxLineBufferGeometry* geom,
                                           const GFxTextFieldParam& param,
                                           UInt numGlyphsInBatch);

    void                ReleaseBatchPackage(GFxBatchPackage* bp);
    void                CleanUpFont(GFxFontResource* font);

    bool                DisplayBatchPackage(GFxBatchPackage* bp, 
                                            GFxDisplayContext &context, 
                                            const GFxLineBufferGeometry& geom,
                                            const GFxTextFieldParam& textFieldParam,
                                            const Matrix& displayMatrix,
                                            const Cxform& cx, 
                                            bool maskApplied);

    bool                GlyphFits(const GRectF& bounds, 
                                  UInt  fontSize, 
                                  Float heightRatio, 
                                  const GFxGlyphParam& param) const;

    bool                IsVectorRenderingRequired(GFxBatchPackage* bp)
    {
        return !bp || (bp->Package && bp->Package->VectorRenderingRequired);
    }

    UInt GetTextureGlyphMaxHeight() const
    {
#ifndef GFC_NO_GLYPH_CACHE
        return Cache.GetMaxGlyphHeight(); 
#else
        return 0;
#endif
    }

private:
    // Prohibit copying
    GFxFontCacheManagerImpl(const GFxFontCacheManagerImpl&);
    const GFxFontCacheManagerImpl& operator = (const GFxFontCacheManagerImpl&);

    typedef GTL::ghash_set<GFxFontResource*, GFxFontHashOp> FontSetType;

    void resolveTextureGlyph(GFxBatchPackageData::GlyphVerifier* gv, 
                             const GFxGlyphParam& param,
                             GFxDisplayContext &context);

    void fillBatchPackage(GFxBatchPackageData* pd, 
                          const GFxTextLineBuffer::Iterator& linesIt, 
                          GFxDisplayContext &context, 
                          const GPointF& lnOffset,
                          GFxLineBufferGeometry* geom,
                          const GFxTextFieldParam& textFieldParam,
                          UInt numGlyphsInBatch);

    inline UInt snapFontSizeToRamp(UInt fontSize) const;

    typedef GPodStructAllocator<GFxBatchPackage> BatchPackageStorageType;
    typedef GPodDList<GFxBatchPackage>           BatchPackageListType;


    GRenderer*                  pRenderer;
    FontDisposeHandler          FontDisposer;
    BatchPackageStorageType     BatchPackageStorage;
    BatchPackageListType        BatchPackageQueue;
#ifndef GFC_NO_GLYPH_CACHE
    GFxGlyphRasterCache         Cache;
    static UInt                 FontSizeRamp[];
    UByte                       FontSizeMap[256];
#endif
    FontSetType                 KnownFonts;
};


#endif

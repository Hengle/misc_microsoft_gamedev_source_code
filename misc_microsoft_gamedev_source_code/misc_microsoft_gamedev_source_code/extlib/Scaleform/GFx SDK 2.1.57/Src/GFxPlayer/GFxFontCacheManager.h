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
#include "GStroker.h"

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

        void Clear()
        { 
            Count = ImageUseCount = Index = 0; 
            pTexture = 0; 
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
    bool                        FailedGlyphs;

    void RemoveAll() 
    { 
        BatchCache.ReleaseData(GRenderer::Cached_BitmapList);
        BatchDesc.clear();
        Batch.resize(0);
        BatchVerifier.resize(0);
        VectorRenderingRequired = false;
        FailedGlyphs = false;
    }

    GFxBatchPackageData() : 
        VectorRenderingRequired(false), FailedGlyphs(false) {}

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
    enum 
    { 
#ifndef GFC_NO_GLYPH_CACHE
        SubpixelSizeScale   = GFxGlyphRasterCache::SubpixelSizeScale,
#endif
        ShapePageSize       = 256-2 - 8-4
    };

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

    struct RenEventHandler : public GRenderer::EventHandler
    {
        RenEventHandler() {}
        void Bind(GFxFontCacheManagerImpl* self) { pSelf = self; }
        virtual void OnEvent(GRenderer* prenderer, GRenderer::EventHandler::EventType changeType);
        GFxFontCacheManagerImpl* pSelf;
    };

    struct VectorGlyphShape : public GPodDListNode<VectorGlyphShape>
    {
        enum { LockGlyphFlag = 0x80 };

        GFxFontResource*      pFont;
        UInt16                GlyphIndex;
        UInt8                 HintedGlyphSize;
        UInt8                 Flags;
        GFxShapeNoStylesDef*  pShape;
    };

    struct VectorGlyphKey
    {
        GFxFontResource* pFont;
        UInt16           GlyphIndex;
        UInt8            HintedGlyphSize;
        UInt8            Flags;

        VectorGlyphKey() {}
        VectorGlyphKey(GFxFontResource* font, UInt glyphIndex, UInt hintedGlyphSize, UInt flags):
            pFont(font), GlyphIndex((UInt16)glyphIndex), 
            HintedGlyphSize((UInt8)hintedGlyphSize), 
            Flags((UInt8)flags & (GFxFont::FF_Bold | GFxFont::FF_Italic)) {}

        size_t operator()(const VectorGlyphKey& key) const
        {
            return (((size_t)key.pFont) >> 6) ^ (size_t)key.pFont ^ 
                     (size_t)key.GlyphIndex ^
                     (size_t)key.HintedGlyphSize ^
                     (size_t)key.Flags;
        }

        bool operator == (const VectorGlyphKey& key) const 
        { 
            return  pFont           == key.pFont && 
                    GlyphIndex      == key.GlyphIndex &&
                    HintedGlyphSize == key.HintedGlyphSize &&
                    Flags           == key.Flags;
        }
    };

public:
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;
    typedef GRenderer::BitmapDesc   BitmapDec;

    GFxFontCacheManagerImpl();
   ~GFxFontCacheManagerImpl();

    void                Init(const GFxFontCacheManager::TextureConfig& config);
    void                SetMaxVectorCacheSize(UInt n) { MaxVectorCacheSize = n; }
    void                SetFauxItalicAngle(Float a);
    void                SetFauxBoldRatio(Float r)     { FauxBoldRatio = r; }

    // Init glyph textures. In some cases it may be desirable to initialize
    // the textures in advance. The function may be called once before the 
    // main display loop. Without this call the textures will be 
    // initialized on demand as necessary.
    void                InitTextures(GRenderer* ren);

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

    void                DisplayBatchPackage(GFxBatchPackage* bp, 
                                            GFxDisplayContext &context, 
                                            const Matrix& displayMatrix,
                                            const Cxform& cx);

    bool                GlyphFits(const GRectF& bounds, 
                                  UInt  fontSize, 
                                  Float heightRatio, 
                                  const GFxGlyphParam& param,
                                  Float maxRasterScale) const;

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

    GFxShapeCharacterDef* GetGlyphShape(GFxFontResource* font, UInt index, UInt glyphSize,
                                        bool fauxBold, bool fauxItalic,
                                        GFxLog* log);
    
    GFxShapeCharacterDef* GetGlyphShape_NoLock(GFxFontResource* font, UInt index, UInt glyphSize,
                                              bool fauxBold, bool fauxItalic,
                                              GFxLog* log);

    const GRectF& AdjustBounds(GRectF* bounds, bool fauxBold, bool fauxItalic) const;

    void  UnlockAllGlyphs();

private:
    // Prohibit copying
    GFxFontCacheManagerImpl(const GFxFontCacheManagerImpl&);
    const GFxFontCacheManagerImpl& operator = (const GFxFontCacheManagerImpl&);

    typedef GTL::ghash_set<GFxFontResource*, GFxFontHashOp> FontSetType;

    void setRenderer(GRenderer* pRenderer);
    bool resolveTextureGlyph(GFxBatchPackageData::GlyphVerifier* gv, 
                             const GFxGlyphParam& gp,
                             bool  canUseRaster,
                             const GFxShapeCharacterDef* shape,
                             GFxDisplayContext &context,
                             bool canUseTgd);

    void fillBatchPackage(GFxBatchPackageData* pd, 
                          const GFxTextLineBuffer::Iterator& linesIt, 
                          GFxDisplayContext &context, 
                          const GPointF& lnOffset,
                          GFxLineBufferGeometry* geom,
                          const GFxTextFieldParam& textFieldParam,
                          UInt numGlyphsInBatch);

    bool isOuterContourCW(const GCompoundShape& shape) const;
    void copyAndTransformShape(GFxShapeNoStylesDef* dst, 
                               const GFxShapeCharacterDef* src,
                               GRectF* bbox,
                               bool fauxBold, bool fauxItalic);

    inline UInt snapFontSizeToRamp(UInt fontSize) const;

    typedef GPodStructAllocator<GFxBatchPackage> BatchPackageStorageType;
    typedef GPodDList<GFxBatchPackage>           BatchPackageListType;
    typedef GTL::ghash<VectorGlyphKey, VectorGlyphShape*, VectorGlyphKey> VectorGlyphCacheType;


    GRenderer*                              pRenderer;
    FontDisposeHandler                      FontDisposer;
    RenEventHandler                         FrameHandler;
    bool                                    LockAllInFrame;
    bool                                    RasterCacheWarning;
    bool                                    VectorCacheWarning;
    BatchPackageStorageType                 BatchPackageStorage;
    BatchPackageListType                    BatchPackageQueue;
#ifndef GFC_NO_GLYPH_CACHE
    GFxGlyphRasterCache                     Cache;
    static UInt                             FontSizeRamp[];
    UByte                                   FontSizeMap[256];
#endif
    FontSetType                             KnownFonts;
    GPodStructAllocator<VectorGlyphShape>   VectorGlyphShapeStorage;
    GPodDList<VectorGlyphShape>             VectorGlyphShapeList;
    VectorGlyphCacheType                    VectorGlyphCache;
    GStroker                                Stroker;
    GCompoundShape                          TmpShape1;
    GCompoundShape                          TmpShape2;

    UInt                                    MaxVectorCacheSize;
    Float                                   FauxItalicAngle;
    Float                                   FauxBoldRatio;
    GMatrix2D                               ItalicMtx;
    GLock                                   StateLock;
};


#endif

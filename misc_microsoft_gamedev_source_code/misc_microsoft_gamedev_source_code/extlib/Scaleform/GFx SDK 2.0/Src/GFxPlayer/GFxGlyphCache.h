/**********************************************************************

Filename    :   GFxGlyphCache.h
Content     :   
Created     :   
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxGlyphCache_H
#define INC_GFxGlyphCache_H

#ifndef GFC_NO_GLYPH_CACHE
#include "GTLTypes.h"
#include "GRenderer.h"
#include "GContainers.h"
#include "GImage.h"
#include "GRasterizer.h"
#include "GCompoundShape.h"
#include "GFxGlyphFitter.h"
#include "GFxGlyphParam.h"
#include "GFxLog.h"

class GFxFontResource;


//------------------------------------------------------------------------
struct GFxGlyphRect
{
    UInt16 x,y,w,h;
    GFxGlyphRect() {}
    GFxGlyphRect(UInt x_, UInt y_, UInt w_, UInt h_):
        x(UInt16(x_)), y(UInt16(y_)), w(UInt16(w_)), h(UInt16(h_)) {}
};


struct GFxGlyphNode;
struct GFxGlyphBand;

//------------------------------------------------------------------------
struct GFxGlyphDynaSlot : GPodDListNode<GFxGlyphDynaSlot>
{
    enum 
    { 
        LockFlag = 0x8000U,
        FullFlag = 0x4000U,
        Mask     = 0xC000U
    };

    GFxGlyphNode*     pRoot;
    GFxGlyphDynaSlot* pPrevInBand;
    GFxGlyphDynaSlot* pNextInBand;
    GFxGlyphDynaSlot* pPrevActive;
    GFxGlyphDynaSlot* pNextActive;
    GFxGlyphBand*     pBand;
    UInt16            TextureId;   // Index of the texture in the array. May have LockFlag.
    UInt16            x,w;
    UInt16            Failures;
};


//------------------------------------------------------------------------
struct GFxGlyphDynaSlot_Band
{
inline static void SetPrev(GFxGlyphDynaSlot* self, GFxGlyphDynaSlot* what)  { self->pPrevInBand = what; }
inline static void SetNext(GFxGlyphDynaSlot* self, GFxGlyphDynaSlot* what)  { self->pNextInBand = what; }
inline static const GFxGlyphDynaSlot* GetPrev(const GFxGlyphDynaSlot* self) { return self->pPrevInBand; }
inline static const GFxGlyphDynaSlot* GetNext(const GFxGlyphDynaSlot* self) { return self->pNextInBand; }
inline static       GFxGlyphDynaSlot* GetPrev(GFxGlyphDynaSlot* self)       { return self->pPrevInBand; }
inline static       GFxGlyphDynaSlot* GetNext(GFxGlyphDynaSlot* self)       { return self->pNextInBand; }
};

//------------------------------------------------------------------------
struct GFxGlyphDynaSlot_Active
{
inline static void SetPrev(GFxGlyphDynaSlot* self, GFxGlyphDynaSlot* what)  { self->pPrevActive = what; }
inline static void SetNext(GFxGlyphDynaSlot* self, GFxGlyphDynaSlot* what)  { self->pNextActive = what; }
inline static const GFxGlyphDynaSlot* GetPrev(const GFxGlyphDynaSlot* self) { return self->pPrevActive; }
inline static const GFxGlyphDynaSlot* GetNext(const GFxGlyphDynaSlot* self) { return self->pNextActive; }
inline static       GFxGlyphDynaSlot* GetPrev(GFxGlyphDynaSlot* self)       { return self->pPrevActive; }
inline static       GFxGlyphDynaSlot* GetNext(GFxGlyphDynaSlot* self)       { return self->pNextActive; }
};

//------------------------------------------------------------------------
struct GFxGlyphNode
{
    GFxGlyphParam     Param;
    GFxGlyphDynaSlot* pSlot;
    GFxGlyphNode*     pNext;       // pNext plays the role of Child[0]
    GFxGlyphNode*     pNex2;       // pNex2 plays the role of Child[1]
    GFxGlyphRect      Rect;
    GPoint<SInt16>    Origin;
};

//------------------------------------------------------------------------
struct GFxGlyphBand
{
    UInt16 TextureId;
    UInt16 y,h;
    UInt16 RightSpace;
    GPodDList2<GFxGlyphDynaSlot, GFxGlyphDynaSlot_Band> Slots;
};

//------------------------------------------------------------------------
class GFxGlyphScanlineFilter
{
public:
    GFxGlyphScanlineFilter(Float prim, Float second, Float tert)
    {
        Float norm = 1.0f / (prim + second*2 + tert*2);
        prim   *= norm;
        second *= norm;
        tert   *= norm;
        for(UInt i = 0; i < 256; i++)
        {
            Primary[i]   = (UByte)floorf(prim   * i);
            Secondary[i] = (UByte)floorf(second * i);
            Tertiary[i]  = (UByte)floorf(tert   * i);
        }
    }

    void Filter(const UByte* src, UByte* dst) const
    {
        UByte v = src[2];
        dst[0] = UByte(dst[0] + Tertiary [v]);
        dst[1] = UByte(dst[1] + Secondary[v]);
        dst[2] = UByte(dst[2] + Primary  [v]);
        dst[3] = UByte(dst[3] + Secondary[v]);
        dst[4] = UByte(dst[4] + Tertiary [v]);
    }

private:
    UByte Primary[256];
    UByte Secondary[256];
    UByte Tertiary[256];
};


//------------------------------------------------------------------------
class GFxTextureUpdatePacker
{
public:
    GFxTextureUpdatePacker(UInt width, UInt height) : 
        Width(width), Height(height), 
        LastX(0), LastY(0), LastMaxHeight(0) {}


    UInt GetWidth()  const { return Width; }
    UInt GetHeight() const { return Height; }

    void Reset()
    {
        LastX = 0; 
        LastY = 0;
        LastMaxHeight = 0;
    }

    bool Allocate(UInt w, UInt h, UInt* x, UInt* y)
    {
        if (LastX + w <= Width && LastY + h <= Height)
        {
            *x = LastX;
            *y = LastY;
            LastX += w;
            if (h > LastMaxHeight)
                LastMaxHeight = h;
            return true;
        }

        LastY += LastMaxHeight;
        if (LastY + h <= Height)
        {
            *x = 0;
            *y = LastY;
            LastX = w;
            LastMaxHeight = h;
            return true;
        }
        return false;
    }

private:
    UInt Width, Height, LastX, LastY, LastMaxHeight;
};


//------------------------------------------------------------------------
class GFxGlyphSlotQueue
{
public:
    enum 
    { 
        // The maximal number of failed attempts, after which 
        // the slot is removed from the active list
        MaxSlotFailures = 16
    };

    GFxGlyphSlotQueue();

    void RemoveAll();
    void Init(UInt firstTexture, UInt numTextures, 
              UInt textureWidth, UInt textureHeight, 
              UInt maxSlotHeight);

    UInt GetMaxSlotHeight() const { return MaxSlotHeight; }

    UInt GetNumPacked() const { return (UInt)GlyphHTable.size(); }

    static void LockSlot  (GFxGlyphDynaSlot* slot) { slot->TextureId |=  GFxGlyphDynaSlot::LockFlag; }
    static void UnlockSlot(GFxGlyphDynaSlot* slot) { slot->TextureId &= ~GFxGlyphDynaSlot::LockFlag; }

    static void LockGlyph  (GFxGlyphNode* r) { LockSlot(r->pSlot);   }
    static void UnlockGlyph(GFxGlyphNode* r) { UnlockSlot(r->pSlot); }

    void UnlockAllGlyphs();
    void CleanUpTexture(UInt textureId);
    void CleanUpFont(const GFxFontResource* font);

    void SendGlyphToBack(GFxGlyphNode* r) { SlotQueue.SendToBack(r->pSlot); }

    GFxGlyphNode* FindGlyph(const GFxGlyphParam& gp);
    GFxGlyphNode* AllocateGlyph(const GFxGlyphParam& gp, UInt w, UInt h);

    UInt ComputeUsedArea() const;

private:
    typedef GTL::ghash<GFxGlyphParam, GFxGlyphNode*, GFxGlyphParam> CachedGlyphsType;
    typedef GPodDList2<GFxGlyphDynaSlot, GFxGlyphDynaSlot_Active> ActiveSlotsType;

    GFxGlyphNode*       packGlyph(UInt w, UInt h, GFxGlyphNode* glyph);
    GFxGlyphNode*       packGlyph(UInt w, UInt h, GFxGlyphDynaSlot* slot);
    GFxGlyphNode*       findSpaceInSlots(UInt w, UInt h);
    GFxGlyphDynaSlot*   initNewSlot(GFxGlyphBand* band, UInt x, UInt w);
    GFxGlyphNode*       allocateNewSlot(UInt w, UInt h);
    void                splitSlot(GFxGlyphDynaSlot* slot, UInt w);
    void                splitGlyph(GFxGlyphDynaSlot* slot, bool left, UInt w);
    bool                checkDistance(GFxGlyphDynaSlot* slot) const;
    GFxGlyphDynaSlot*   mergeSlotWithNeighbor(GFxGlyphDynaSlot* slot);
    void                mergeSlots(GFxGlyphDynaSlot* from, GFxGlyphDynaSlot* to, UInt w);
    GFxGlyphNode*       extrudeOldSlot(UInt w, UInt h);
    void                releaseGlyphTree(GFxGlyphNode* glyph);
    void                releaseSlot(GFxGlyphDynaSlot* slot);
    void                computeGlyphArea(const GFxGlyphNode* glyph, UInt* used) const;
    static const GFxGlyphNode* findFontInSlot(GFxGlyphNode* glyph, const GFxFontResource* font);

    UInt                                    MinSlotSpace;
    UInt                                    FirstTexture;
    UInt                                    NumTextures;
    UInt                                    TextureWidth;
    UInt                                    TextureHeight;
    UInt                                    MaxSlotHeight;
    UInt                                    NumBandsInTexture;

    GPodStructAllocator<GFxGlyphDynaSlot>   Slots;
    GPodDList<GFxGlyphDynaSlot>             SlotQueue;
    UInt                                    SlotQueueSize;
    ActiveSlotsType                         ActiveSlots;
    GPodStructAllocator<GFxGlyphNode>       Glyphs;
    GPodVector<GFxGlyphBand>                Bands;
    CachedGlyphsType                        GlyphHTable;

    UInt                                    NumUsedBands;
};


const Float GFx_OutlineRatio = 0.1f;

//------------------------------------------------------------------------
class GFxGlyphRasterCache : public GNewOverrideBase
{
    enum 
    { 
        TexturePoolSize      = 32,
        MissingGlyph         = 0xFFFF
    };

    class TextureEventHandler : public GTexture::ChangeHandler
    {
    public:
        TextureEventHandler() : pCache(0), TextureId(~0U) {}
        void Bind(GFxGlyphRasterCache* cache, UInt textureId)
        {
            pCache = cache;
            TextureId = textureId & ~GFxGlyphDynaSlot::LockFlag;
        }
        virtual void OnChange(GRenderer*, EventType changeType)
        {
            if (changeType == Event_DataLost)
                pCache->CleanUpTexture(TextureId, false);
            else if (changeType == Event_RendererReleased)
                pCache->CleanUpTexture(TextureId, true);
        }
    
        virtual bool Recreate(GRenderer*)
        {
            return pCache->RecreateTexture(TextureId);
        }

    private:
        GFxGlyphRasterCache* pCache;
        UInt                 TextureId;
    };

public:
    enum { SubpixelSizeScale = 16 };

    GFxGlyphRasterCache();
   ~GFxGlyphRasterCache();

    void RemoveAll();
    void Init(UInt texWidth, UInt texHeight, UInt maxNumTextures, 
              UInt maxSlotHeight, UInt slotPadding,
              UInt texUpdWidth, UInt texUpdHeight);

    Float GetScaleU()         const { return ScaleU; }
    Float GetScaleV()         const { return ScaleV; }
    UInt  GetMaxSlotHeight()  const { return MaxSlotHeight; }
    UInt  GetMaxGlyphHeight() const { return MaxSlotHeight - 2*SlotPadding; }
    UInt  GetMaxNumTextures() const { return MaxNumTextures; }

    void          CleanUpTexture(UInt textureId, bool release);
    bool          RecreateTexture(UInt textureId);
    void          CleanUpFont(const GFxFontResource* font);
    void          UpdateTextures(GRenderer* ren);

    void          CalcGlyphParam(Float originalFontSize,
                                 UInt  snappedFontSize, 
                                 Float heightRatio,
                                 const GRectF& bounds,
                                 const GFxGlyphParam& textParam,
                                 GFxGlyphParam* glyphParam,
                                 UInt16* subpixelSize,
                                 UInt16* glyphScale) const;

    GFxGlyphNode* GetGlyph(GRenderer* ren, 
                           const GFxGlyphParam& gp,
                           UInt subpixelSize,
                           GFxLog* log);
    GTexture*     GetGlyphTexture(const GFxGlyphNode* glyph) const
    {
        return Textures[glyph->pSlot->TextureId & ~GFxGlyphDynaSlot::Mask].pTexture;
    }
    static UInt   GetTextureId(const GFxGlyphNode* glyph)
    {
        return glyph->pSlot->TextureId & ~GFxGlyphDynaSlot::Mask;
    }

    bool VerifyGlyphAndSendBack(const GFxGlyphParam& gp, 
                                GFxGlyphNode* glyph)
    {
        if (gp == glyph->Param)
        {
            SlotQueue.SendGlyphToBack(glyph);
            return true;
        }
        return false;
    }

    static void LockGlyph(GFxGlyphNode* glyph) 
    { 
        GFxGlyphSlotQueue::LockGlyph(glyph);
    }

    static void UnlockGlyph(GFxGlyphNode* glyph)
    {
        GFxGlyphSlotQueue::UnlockGlyph(glyph);
    }

    void UnlockAllGlyphs()
    {
        SlotQueue.UnlockAllGlyphs();
    }

private:
    // Prohibit copying
    GFxGlyphRasterCache(const GFxGlyphRasterCache&);
    const GFxGlyphRasterCache& operator = (const GFxGlyphRasterCache&);

    struct TextureType
    {
        GTexture*           pTexture;
        TextureEventHandler Handler;
        UInt                NumGlyphsToUpdate;
    };

    struct GlyphUpdateType
    {
        GTexture::UpdateRect Rect;
        UInt                 TextureId;
    };

    void                releaseAllTextures();
    bool                initTextureIfRequired(GRenderer* ren, UInt textureId);
    void                filterScanline(const UByte* src, UByte* dst, UInt len);
    void                stackBlur(GImage& img, UInt sx, UInt sy, UInt w, UInt h, UInt rx, UInt ry);
    void                recursiveBlur(GImage& img, UInt sx, UInt sy, UInt w, UInt h, Float rx, Float ry);
    void                strengthenImage(GImage& img, UInt sx, UInt  sy, UInt  w,  UInt  h, Float ratio, SInt bias);
    void                makeKnockOutCopy(const GImage& img, UInt sx, UInt  sy, UInt  w,  UInt  h);
    void                knockOut(GImage& img, UInt sx, UInt  sy, UInt  w,  UInt  h);

    GFxGlyphNode*       rasterizeAndPack(GRenderer* ren, 
                                         const GFxGlyphParam& gp,
                                         UInt subpixelSize,
                                         GFxLog* log);

    UInt                                TextureWidth;
    UInt                                TextureHeight;
    UInt                                MaxNumTextures;
    UInt                                MaxSlotHeight;
    UInt                                SlotPadding;
    Float                               ScaleU;
    Float                               ScaleV;
    GPodBVector<GlyphUpdateType>        GlyphsToUpdate;
    GPodVector<GTexture::UpdateRect>    RectsToUpdate;
    TextureType                         Textures[TexturePoolSize];

    GPtr<GImage>                        TexUpdBuffer;
    GFxTextureUpdatePacker              TexUpdPacker;
    GPtr<GImage>                        KnockOutCopy;
    GPodVector<UInt8>                   BlurStack;
    GPodVector<Float>                   BlurSum;
    GCompoundShape                      Shape;
    GRasterizer                         Rasterizer;
    GFxGlyphScanlineFilter              ScanlineFilter;
    GFxGlyphFitter                      Fitter;
    GFxGlyphSlotQueue                   SlotQueue;
};

#endif // GFC_NO_GLYPH_CACHE

#endif


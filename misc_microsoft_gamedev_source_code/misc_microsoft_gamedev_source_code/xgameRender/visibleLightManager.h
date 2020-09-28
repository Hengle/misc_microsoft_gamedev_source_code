//============================================================================
//
// File: visibleLightManager.h
//
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "sceneLightManager.h"
#include "containers\hashMap.h"
#include "effectFileLoader.h"
#include "math\vectorInterval.h"

// Enough for 255 shadow casting lights, each influencing ~60 objects or 15k objects total.
typedef BFixedBlockAllocator<128, 255> BLightObjectArrayAllocator;
typedef BLinkedArrayManager<short, BLightObjectArrayAllocator, true, false> BLightObjectLinkedArrayManager;
typedef BLightObjectLinkedArrayManager::BLinkedArrayBlock BLightObjectLinkedArray;

//============================================================================
// class BVisibleLightManager
//============================================================================
#pragma warning(push)
#pragma warning(disable:4324)
__declspec(align(128))
class BVisibleLightManager : public BRenderCommandListener
{
public:
   BVisibleLightManager();
   ~BVisibleLightManager();

   void init(void);
   void deinit(void);
   
   // Render thread only
   
   void setLocalLightingEnabled(bool enabled) { mLocalLightingEnabled = enabled; }
   bool getLocalLightingEnabled() const { return mLocalLightingEnabled; }
   
   void setLightBufferingEnabled(bool enabled) { mLightBufferingEnabled = enabled; }
   bool getLightBufferingEnabled() const { return mLightBufferingEnabled; }

   void update(float worldMinY, float worldMaxY);
   void updateLightTextures(void);

   // Returns active light indices by default, unless visibleLightIndices is set.
   void findLights(BSceneLightManager::BActiveLightIndexArray& activeLightIndices, BSceneLightManager::BGridRect gridRect, XMVECTOR min, XMVECTOR max, bool refine = true, bool visibleLightIndices = false);

   // Translates active to visible light indices.         
   enum { cInvalidVisibleLightIndex = USHRT_MAX };
   typedef uint BVisibleLightIndex;
   BVisibleLightIndex getVisibleLightIndex(BSceneLightManager::BActiveLightIndex activeLightIndex) const { return mActiveToVisibleLights[activeLightIndex]; }

   // Returns the number of visible local lights in the view frustum.
   uint getNumVisibleLights(void) const { return mVisibleLights.size(); }
   BSceneLightManager::BActiveLightIndex getVisibleLight(BVisibleLightIndex visibleLightIndex) const { return mVisibleLights[visibleLightIndex]; }
   
   uint getNumVisibleLightBufferedLights(void) { return mLightBufferedLights.size(); }

   uint getNumShadowedVisibleLights(void) const { return mNumShadowedLights; }

   bool getVisibleLightShadows(BVisibleLightIndex visibleLightIndex) const { return mVisibleLightShadows.get(visibleLightIndex) != FALSE; }

   struct BVisibleLightState
   {
      ushort mCircleRadius;
      uchar mLightFadeFract;
      uchar mShadowFadeFract;
   };

   const BVisibleLightState& getVisibleLightInfo(BVisibleLightIndex visibleLightIndex) const { return mVisibleLightState[visibleLightIndex]; }

   // The visible light texels point to write combined physical memory unless cachedReadOnly is true.
   enum { cTexelsPerLight = 8 };
   const XMFLOAT4* getVisibleLightTexels(bool cachedReadOnly = false) const { return (XMFLOAT4*)(cachedReadOnly ? GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS(mpVisibleLightTexels) : mpVisibleLightTexels); }

   const BLightObjectLinkedArrayManager& getLightObjectLinkedArrayManager(void) const  { return mLightObjectLinkedArrayManager; }
         BLightObjectLinkedArrayManager& getLightObjectLinkedArrayManager(void)        { return mLightObjectLinkedArrayManager; }
   
   bool addToShadowedLightObjectList(BVisibleLightIndex visibleLightIndex, int objectIndex);
   BLightObjectLinkedArray* getShadowedLightObjectList(BVisibleLightIndex visibleLightIndex);
         
   void drawDebugInfo(void);
   
   enum { cLightBufferColorTexture = 0, cLightBufferVectorTexture = 1 };
   IDirect3DVolumeTexture9* getLightBuffer(uint index) const;
   float getLightBufferIntensityScale(void) const;
   const XMMATRIX& getWorldToLightBuffer(void) const;
   
private:
   __declspec(align(128)) BLightObjectArrayAllocator  mLightObjectArrayAllocator;
   
   IDirect3DTexture9*                                 mpLightTexture;

   BSceneLightManager::BActiveLightIndexArray         mVisibleLights;
   BDynamicArray<ushort, 128>                         mActiveToVisibleLights;
   BBitArray2D<1>                                     mVisibleLightShadows;
   
   struct BLightBufferedLight
   {
      WORD                 mActiveLightIndex;
      BVisibleLightState   mState;
   };
   BDynamicArray<BLightBufferedLight, 4, BDynamicArrayRenderHeapAllocator> mLightBufferedLights;

   BDynamicArray<BVisibleLightState>                  mVisibleLightState;

   float                                              mWorldMinY;
   float                                              mWorldMaxY;
   
   XMVECTOR                                           mWorldFocusMin, mWorldFocusMax;
   
   const XMFLOAT4*                                    mpVisibleLightTexels;

   BLightObjectLinkedArrayManager                     mLightObjectLinkedArrayManager;
   // Change this to ushort if you increase the # of max items in BLightObjectLinkedArrayManager to > 255.
   // Indexed by visible light indices;
   BDynamicArray<uchar>                               mShadowedLightObjectsArray;

   uint                                               mNumShadowedLights;
         
   BFXLEffectFileLoader*                              mpLightBufferEffect;
   
   enum { cNumLightBuffers = 2 };
   IDirect3DVolumeTexture9*                           mpLightBuffers[cNumLightBuffers];
   AABB                                               mLightBufferBounds;
   XMMATRIX                                           mWorldToLightBuffer;
   XMMATRIX                                           mLightBufferToWorld;
   
   BFXLEffectTechnique                                mAccumLightTechnique;
   BFXLEffectTechnique                                mRenormTechnique;
   BFXLEffectParam                                    mSlicePlaneParam;
   BFXLEffectParam                                    mLightToWorldParam;
   BFXLEffectParam                                    mWorldToLightParam;
   BFXLEffectParam                                    mLightBufValues0Param;
   BFXLEffectParam                                    mLightBufValues1Param;
   BFXLEffectParam                                    mLightBufferVectorSampler;
   IDirect3DVertexDeclaration9*                       mpAccumLightVertexDecl;
   IDirect3DVertexDeclaration9*                       mpRenormVertexDecl;
   
   bool                                               mLocalLightingEnabled : 1;
   bool                                               mLightBufferingEnabled : 1;
   bool                                               mInitialized : 1;
      
   void clear(void);   
   void findVisibleLights(void);
         
   bool calcSceneFocusRegion();
   
   void updateLightBufferTexture(void);
   void updateVisibleLightTexture(void);
   void updateLightBufferParams(void);
   void createLightBufferTextures(void);
   void freeLightBufferTextures(void);
               
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
   
   virtual void beginLevelLoad(void);
   virtual void endLevelLoad(void);
};

extern BVisibleLightManager gVisibleLightManager;
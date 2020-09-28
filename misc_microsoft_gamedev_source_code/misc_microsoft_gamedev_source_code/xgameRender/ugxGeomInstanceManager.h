//==============================================================================
//
// File: ugxGeomInstanceManager.h
//
// Copyright (c) 2006 Ensemble Studios
//
//==============================================================================
#pragma once

#include "threading\eventDispatcher.h"
#include "renderThread.h"
#include "ugxGeomRenderTypes.h"
#include "volumeCuller.h"
#include "math\vectorInterval.h"
#include "containers\hashMap.h"

//==============================================================================
// class BUGXGeomInstanceManager
//==============================================================================
class BUGXGeomInstanceManager : public BRenderCommandListener
{
public:
   BUGXGeomInstanceManager();
   ~BUGXGeomInstanceManager();

   // rg [4/6/7] - FIXME: Make sure we have enough frame storage for >3072 instances!
   enum { cMaxExpectedInstances = 5120 };
   
   void init(void);
   void deinit(void);
   void reset();
            
   void simSetInstances(
      uint numInstances, const BUGXGeomRenderInstanceAttributes* pCPUFrameStorageInstanceAttributes, 
      void* pGPUFrameStorageBones, uint GPUFrameStorageSize);
      
   void renderUpdate(double gameTime);
      
   void renderCreateSortableAttributes(void);
   
   void renderSort(void);
         
   // Updates the local shadow status of all instances.
   void renderUpdateLocalShadowStatus(void);

   // Updates the local reflection status of all instances.
   void renderUpdateLocalReflectionStatus(void);
   
   void renderUpdatePackedTextures(void);
   
   enum eRenderFlushFlags
   {
      cRFFSetStencilRefToTeamColor        = 1,
      cRFFSetCommandBufferRunPredication  = 2
   };
   
   // renderFlushUpdateIntrinsics() MUST be called before any of the flush API - this updates the mesh renderer's intrinsic pool with the current global intrinsics and sets them to the device.
   void renderFlushUpdateIntrinsics(void);

   // Renders all instances with no culling.
   void renderFlushAll(eUGXGeomPass pass, uint layerFlags, DWORD renderFlushFlags = 0);

   // Renders all instances that cast shadows from the specified local light.
   void renderFlushLocalLightInstances(eUGXGeomPass pass, uint layerFlags, uint localLightShadowPassIndex, DWORD renderFlushFlags = 0);
         
   // Renders all instances with conservative culling. pVolumeCuller may be NULL to disable culling.
   void renderFlush(eUGXGeomPass pass, uint layerFlags, const BVolumeCuller* pVolumeCuller, bool farLayer = false, bool nearLayer = false, DWORD renderFlushFlags = 0);

   // Sorts the instances into discrete layers with culling. pVolumeCuller may be NULL to disable culling.
   // Volume culler must remain valid until all group rendering is complete.
   void renderFlushVisiblePrep(const BVolumeCuller* pVolumeCuller);
   
   enum eGroupIndex
   {
      cGroupOpaque,
      cGroupOverallAlpha,
      cGroupAdditive,
      cGroupOver,
      cGroupFar,
      cGroupNear,
      cGroupObscurable
   };
   
   enum eObscurableFilter
   {
      cOFAll,
      cOFObscurable,
      cOFNotObscurable
   };
   
   enum eBelowDecalsFilter
   {
      cBDAll,
      cBDBelowDecals,
      cBDAboveDecals
   };
   
   void renderFlushVisibleGroup(
      eGroupIndex group, 
      eUGXGeomPass pass, 
      uint layerFlags, 
      eObscurableFilter obscurableFilter = cOFAll, 
      eBelowDecalsFilter belowDecalsFilter = cBDAll,
      DWORD renderFlushFlags = cRFFSetStencilRefToTeamColor);
      
   void renderFlushDCBBeginFork(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDepthStencilSurf, bool halfPixelOffset);
   IDirect3DDevice9* renderFlushDCBForkCallbackBegin(void);
   void renderFlushDCBForkCallbackEnd(void);
   void renderFlushDCBEndFork(void);
   void renderFlushDCBEnd(void);
   IDirect3DCommandBuffer9* renderFlushGetDCB(void) { return mTotalDCBDrawCalls ? mpCommandBuffer : NULL; }
   void renderFlushDCBRun(uint tileIndex);
      
   uint renderCalculateViewExtents(XMMATRIX worldToView, XMMATRIX viewToProj, const BVolumeCuller& volumeCuller, const BVec3& worldMin, const BVec3& worldMax, AABB& viewBounds);
   uint renderCalculateProjZExtents(XMMATRIX worldToView, XMMATRIX viewToProj, const BVolumeCuller& volumeCuller, const BVec3& worldMin, const BVec3& worldMax, BInterval& zExtent);
         
   struct BSceneIterateParams
   {
      BSceneIterateParams() : 
         mpVolumeCuller(NULL), 
         mShadowCastersOnly(false)
      {
      }
      
      const BVolumeCuller* mpVolumeCuller;
      bool mShadowCastersOnly;
   };
   
   // Returns false to stop iteration.
   typedef bool (*BSceneIteratorCallbackFunc)(const BSceneIterateParams& params, uint instanceIndex, XMVECTOR worldMin, XMVECTOR worldMax, void* pData);
   
   uint renderSceneIterate(const BSceneIterateParams& params, BSceneIteratorCallbackFunc pIteratorFunc, void* pData);

   long getNumNearLayerInstances() const { return mCulledNearLayerInstanceIndices.getNumber(); }
   
   // Only obscurable instances will update the player color tracking variables.
   enum { cMaxPlayerColorIndex = 16 };
   uint getMinPlayerColorIndex(void) const { return mMinPlayerColorIndex; }
   uint getMaxPlayerColorIndex(void) const { return mMaxPlayerColorIndex; }
   BOOL getPlayerColorIndexUsed(uint playerColorIndex) const { BDEBUG_ASSERT(playerColorIndex <= cMaxPlayerColorIndex); return mPlayerColorIndexUsed[playerColorIndex]; }

   uint getNumObscurableInstances(void) const { return mNumObscurableInstances; }
   
   // Union of layer flags for all instances - updated by renderCreateSortableAttributes().
   uint getSceneLayerFlags(void) const { return mSceneLayerFlags; }
         
#ifndef BUILD_FINAL
   struct BStats
   {
      BStats() {  clear(); }
      
      uint mTotalFlushes;
      uint mTotalNonEmptyFlushes;
      uint mTotalInstances;
      uint mTotalModelBatches;
      
      void clear(void)
      {
         Utils::ClearObj(*this);
      }
   };
   
   BStats& getStats() { return mStats; }
#endif   
               
private:
   struct BInstanceSortableRenderAttributes
   {
      // All attributes that could break a batch should be here.
      DWORD mTintColor;
      float mEmissiveIntensity;
      float mHighlightIntensity;
      
      // mpExtendedAttributes must point to CPU frame storage, and may be NULL.
      IExtendedVisualRenderAttributes* mpExtendedAttributes;
      
      ushort mModelIndex;
      ushort mBatchIndex;
      
      uchar mTileFlags;
      
      uchar mNumPixelLights;
                  
      uchar mLayerFlags;
      
      uchar mPlayerColorIndex;
      
      uchar mMultiframeTextureIndex;
            
      bool mGlobalLighting       : 1;
      bool mLocalLighting        : 1;
      bool mShadowReceiver       : 1;
      bool mDirShadows           : 1;
      bool mLocalShadows         : 1;
      bool mFarLayer             : 1;
      bool mNearLayer            : 1;
      
      // mShadowCaster takes into account whether or not the model has any opaque or over operator sections.
      bool mShadowCaster         : 1;

      bool mLocalReflection      : 1;
      bool mSampleBlackmap       : 1;
                  
      bool operator< (const BInstanceSortableRenderAttributes& rhs) const;
      static bool compareVisual(const BInstanceSortableRenderAttributes& lhs, const BInstanceSortableRenderAttributes& rhs);
      static bool compareShadowGen(const BInstanceSortableRenderAttributes& lhs, const BInstanceSortableRenderAttributes& rhs);
   };
   
   BDynamicRenderArray<BInstanceSortableRenderAttributes> mInstanceSortableAttributes;
   
   struct BInstanceLightState
   {
      // Visible light indices.
      
      enum { cMaxPixelLights = 16 };
      short mPixelLights[cMaxPixelLights];
   };
   
   BDynamicRenderArray<BInstanceLightState> mInstanceLightState;
         
   enum 
   {
      cRenderCommandSetInstances = 0,
   };
   
   DWORD                                     mNextBatchIndex;
   
   BDynamicRenderArray<ushort>               mSortedInstanceIndices;
   BDynamicRenderArray<ushort>               mOriginalToSortedInstanceIndices;
   
   BDynamicRenderArray<ushort>               mCulledOpaqueInstanceIndices;
   BDynamicRenderArray<ushort>               mCulledAdditiveInstanceIndices;
   BDynamicRenderArray<ushort>               mCulledOverInstanceIndices;
   BDynamicRenderArray<ushort>               mCulledOverallAlphaInstanceIndices;
   BDynamicRenderArray<ushort>               mCulledFarLayerInstanceIndices;
   BDynamicRenderArray<ushort>               mCulledNearLayerInstanceIndices;
      
   BDynamicRenderArray<ushort>               mLocalLightInstanceIndices;
   
   BDynamicRenderArray<ushort>               mTempIndices;
   
   BDynamicRenderArray<BUGXGeomRenderPerInstanceData, 16> mPerInstanceDataArray;
         
   struct BInstanceData
   {
      uint mNumInstances;
      const BUGXGeomRenderInstanceAttributes* mpCPUFrameStorageInstanceAttributes;
      void* mpGPUFrameStorageBones; // This is a write combined pointer!
      uint mGPUFrameStorageSize;
   }; 
   
   BInstanceData                             mInstanceData;
   
   // low 16-bits are the model index, and high 16-bits are the multiframetexture index
   typedef BHashMap<uint, float, BHasher<uint>, BEqualTo<uint>, true, BRenderFixedHeapAllocator> BModelHashMap;
   BModelHashMap                             mUniqueModels;
         
   const BVolumeCuller*                      mpGroupVolumeCuller;
   
   double                                    mGameTime;
   
   uint                                      mTotalInstancesFlushed;
   
   uint                                      mNumObscurableInstances;
   uint                                      mMinPlayerColorIndex;
   uint                                      mMaxPlayerColorIndex;
      
#ifndef BUILD_FINAL
   BStats mStats;
#endif   

   uint                                      mSceneLayerFlags;
   
   IDirect3DCommandBuffer9*                  mpCommandBuffer;
   uint                                      mTotalDCBDrawCalls;
      
   DWORD                                     mSavedFillMode;
   DWORD                                     mSavedColorWriteEnable;
   bool                                      mSavedHalfPixelOffset;
   IDirect3DSurface9                         mSavedRenderTargetSurf;
   IDirect3DSurface9                         mSavedDepthStencilSurf;
      
   bool                                      mPlayerColorIndexUsed[cMaxPlayerColorIndex + 1];
   
   bool                                      mInitialized : 1;
   
   bool                                      mDCBRenderingActive : 1;
               
   void renderUpdate(void);
      
   void renderSetInstances(const BRenderCommandHeader& header, const uchar* pData);
     
   void renderFlushInstances(eUGXGeomPass pass, uint layerFlags, const BDynamicRenderArray<ushort>* pSortedInstances, const BVolumeCuller* pVolumeCuller, DWORD renderFlushFlags);
   void renderAllInstances(eUGXGeomPass pass, uint layerFlags, const BDynamicRenderArray<ushort>& indices, uint startIndex, uint endIndex, const BVolumeCuller* pVolumeCuller, DWORD renderFlushFlags);
      
   static bool instanceCompareFunc(uint i, uint j);

   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);

   XMVECTOR getLocalReflectionPlane(uint32 instanceIndex) const;
};

extern BUGXGeomInstanceManager gUGXGeomInstanceManager;
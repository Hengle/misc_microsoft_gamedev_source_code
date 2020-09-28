//============================================================================
//
//  TerrainRender.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

// local
#include "terrain.h"

// xrender
#include "rendercommand.h"
#include "effect.h"
#include "D3DTexture.h"
#include "effectIntrinsicManager.h"

// xgameRender
#include "visibleLightManager.h"
#include "worldVisibility.h"

// xcore
#include "threading\eventDispatcher.h"

class BTerrainTexturingRenderData;
class BFXLEffectFileLoader;
 
//-----------------------------------
class BTerrainRenderPacket
{
public:
   BTerrainTexturingRenderData      *mTexturingData;
   BTerrain3DSimpleVisualPacket     *mVisualData;
};

//-----------------------------------
class BTerrainRender : public BRenderCommandListenerInterface, BEventReceiverInterface
{
public:
   BTerrainRender();
   ~BTerrainRender();
      

   bool           init();
   bool           deinit();

   void           allocateTempResources(void);
   void           releaseTempResources(void);

   // Rendering calls
   void           beginRender(eTRenderPhase renderPhase);
   void           render(const BTerrainQuadNode& quadNode, const BTerrainRenderPacket* packet);
   void           renderSkirt(const BTerrainQuadNode::skirtNodeInfo& skirtInfo);
   void           endRender();
           
   // This methods override the rendering process to create DCBs
   // if deferToThreads = false, the DCBs will be created on the calling thread.
   // otherwise, this functions will spawn off on the thread pool and calculate the DCB's there.
   // this functions require an explicit 'FLUSH' operation which must be called.
   // In the case of threaded DCB creation, flush will wait on the events before flushing the DCBs.
   // NOTE this function composites both visible and skirt nodes into one DCB list. 
   //  This is due to some restrictions with how effects and DCBs work
   void           beginDCBRender(const BTerrainQuadNodePtrArray* pVisibleNodeInstances, const BTerrainQuadNodePtrArray* pVisibleSkirtNodeInstances, bool deferToThreads);
   void           joinDCBWork();
   void           flushDCBs(uint tileIndex);
   void           endDCBRender();
         
   void           setAODiffuseIntensity(float aoDI) { mAODiffuseIntensity = aoDI; } 
   float          getAODiffuseIntensity() { return mAODiffuseIntensity; }
   
   void           toggleUsingDCB() { mDCBRenderingToggle = true; }
   bool           isUsingDCBs() { return mDCBRenderingEnabled; }
   
   const BBlackmapParams&           getBlackmapParams() { return mBlackmapParams;}
   void           setBlackmapParams(const BBlackmapParams& params) { mBlackmapParams = params; }
   void           clearBlackmapParams() { mBlackmapParams.clear(); }

private:
   BEventReceiverHandle            mEventHandle;

   enum 
   {
      cRenderPassAN                    = 0,
      cRenderPassANS                   = 1,
      cRenderPassANSE                  = 2,
      cRenderPassANSR                  = 3,
      cRenderPassFull                  = 4,
      cRenderPassSmallChunk            = 5,

      cRenderPassAN_NoLocalLighting    = 6,
      cRenderPassANS_NoLocalLighting   = 7,
      cRenderPassANSE_NoLocalLighting  = 8,
      cRenderPassANSR_NoLocalLighting  = 9,
      cRenderPassFull_NoLocalLighting  = 10,
      cRenderPassSmallChunk_NoLocalLighting = 11,

      cRenderPassSkirtBatched          = 12,   //for skirt chunks (batched, large albedo textures)
      cRenderPassSkirtBatchedFog       = 13,   //for skirt chunks (batched, uses pure fog color)
      cRenderPassSkirtHiTex            = 14,   //for skirt chunks (non-batched, uses texture from parent)
      cRenderPassSkirtLowTex          = 15,   //for skirt chunks (non-batched, uses large albedo texture)
      cRenderPassSkirtNonBatchedFog   = 16,   //for skirt chunks (non-batched, uses pure fog color)
      


      cRenderPassVis                   = 17,      //visualization pass
      cRenderPassNumTexturePass        = 6
   };
   
   //FX File and FX Handles
   BFXLEffectFileLoader*           mpEffectLoader;
   BFXLEffect                      mTerrainGPUShader;
            
   //ugg.. i hate duplicating this.. but it does speed things up...(i think...)
   BFXLEffectTechnique             mCurrTechnique;

   //compressed visrep
   

   BFXLEffectParam                 mPosComprMin;
   BFXLEffectParam                 mPosComprRange;
   BFXLEffectParam                 mShaderDataValsHandle;
   BFXLEffectParam                 mShaderDataVals2Handle;
   BFXLEffectParam                 mShaderRcpMapSizeHandle;

   BFXLEffectParam                 mShaderPositionTextureHandle;
   BFXLEffectParam                 mShaderNormalTextureHandle;
   BFXLEffectParam                 mShaderAOTextureHandle;
   BFXLEffectParam                 mShaderAlphaTextureHandle;
   BFXLEffectParam                 mShaderLightingTextureHandle;

   BFXLEffectParam                 mShaderDynamicAlphaTextureHandle;
   BFXLEffectParam                 mShaderDynamicAlphaTextureWidth;

   //texturing
   BFXLEffectParam                 mShaderUniqueAlbedoTexHandle;
   BFXLEffectParam                 mShaderUniqueNormalTexHandle;
   BFXLEffectParam                 mShaderUniqueSpecularTexHandle;
   BFXLEffectParam                 mShaderUniqueEnvMaskTexHandle;
   BFXLEffectParam                 mShaderUniqueSelfTexHandle;
   BFXLEffectParam                 mShaderUniqueSelfTexPacked;
   
   

   BFXLEffectParam                 mShaderEnvMapTexHandle;
   BFXLEffectParam                 mShaderEnvMapHDRHandle;

   BFXLEffectParam                 mShaderSpecPowerHandle;
   BFXLEffectParam                 mShaderBumpPowerHandle;

   BFXLEffectParam                 mShaderSpecOnlyDir_IntHandle;
   BFXLEffectParam                 mShaderSpecOnlyCol_ShadHandle;

   BFXLEffectParam                 mShaderAODiffuseIntensityHandle;


   BFXLEffectParam                 mShaderNumLayers;
   BFXLEffectParam                 mShaderNumActiveTextures; 
   

   BFXLEffectParam                 mShaderDistFromCameraScalar;

   BFXLEffectParam                 mShaderPatchOffsetHandle;
   

   BFXLEffectParam                 mLocalLightingEnabled;
   BFXLEffectParam                 mLocalShadowingEnabled;
   BFXLEffectParam                 mLightData;

   BFXLEffectParam                 mShaderWVPMat;
   BFXLEffectParam                 mShaderQuadrantMatrixHandle;
   BFXLEffectParam                 mShadeNumBatchedXChunks;
   
   BFXLEffectParam                 mExtendedLocalLightSampler;
   
   BFXLEffectParam                 mBlackmapEnabled;
   BFXLEffectParam                 mBlackmapSampler;
   BFXLEffectParam                 mBlackmapUnexploredSampler;
   BFXLEffectParam                 mBlackmapParams0;
   BFXLEffectParam                 mBlackmapParams1;
   BFXLEffectParam                 mBlackmapParams2;
   BFXLEffectParam                 mEnableLightBuffering;
   BFXLEffectParam                 mWorldToLightBuf;
   BFXLEffectParam                 mLightBufferColorSampler;
   BFXLEffectParam                 mLightBufferVectorSampler;
   
   void                            setVisControlRegs(void);
   int                             mNumILayers;
   int                             mNumLayers;
   
   float                           mTempLayerIndexes[16];  //16*4
   float                           mTempLayerUVs[32];  //16*2
   float                           mTempHDRScales[16];  //16*2

   float                            mAODiffuseIntensity;
   //dummy VB's and IB's
   LPDIRECT3DVERTEXBUFFER9          mVertexBuffer;
 //  IDirect3DVertexDeclaration9   *msVertexDecl;
   int                              mVertexMemSize;

   //our current pass# for the shader
   eTRenderPhase                    mPhase;
   
   BSceneLightManager::BActiveLightIndexArray mCurVisibleLightIndices;
   
   IDirect3DTexture9*               mpLightAttribTexture;
   uint                             mNextLightAttribTexelIndex;
   
   BCommandListenerHandle           mCommandListenerHandle;
   
   BBlackmapParams                  mBlackmapParams;

   bool                             mRendering : 1;
   
   void                             beginRenderInternal(eTRenderPhase renderPhase);

   // these are the direct instances to draw an array of quadnode instances
   void                             renderNodeInstances(const BTerrainQuadNodePtrArray* visibleNodeInstances);
   void                             renderSkirtNodeInstances(const BTerrainQuadNodePtrArray* nodeInstances);
   
   inline void                      render3DPacketTesselated(BTerrain3DSimpleVisualPacket *visDat, const BTerrainQuadNodeDesc& desc,int techniquePassNum, int quadrant);
   inline int                       setupTexturing(BTerrainTexturingRenderData *mTexturingData,bool isSmallNode);
   
   int                             setupLocalLighting(const BTerrainQuadNodeDesc& desc);
   XMFLOAT4*                        computeLightAttribTexelPtr(XMFLOAT4* pDstTexture, uint texelIndex);
   uint                             fillLightAttribTexture(const BSceneLightManager::BActiveLightIndexArray& lights, uint firstIndex, uint& outNumLights);

   enum 
   {
      cEventClassTerrainRenderReload = cEventClassFirstUser
   };
   
   void                       loadEffect(void);
   void                       tickEffect(void);
   void                       initEffectConstants(void);
   
   void                       setRenderPhase(eTRenderPhase phase);
      
   //BRenderCommandListenerInterface
   virtual void               processCommand(const BRenderCommandHeader& header, const unsigned char* pData);  
   virtual void               frameBegin(void);
   virtual void               frameEnd(void);
   virtual void               initDeviceData(void);
   virtual void               deinitDeviceData(void);
   
   //BEventReceiverInterface
   virtual bool               receiveEvent(const BEvent& event, BThreadIndex threadIndex);

   // DCBs
      
   D3DDevice*                          mpUsingDevice;    //overloaded to the CB device when creating DCBs

   D3DDevice*                          mpCommandBufferDevice;
   
   D3DCommandBuffer*                   mpCommandBuffer;
      
   BFXLEffectIntrinsicPool             mIntrinsicPool;
      
   D3DVIEWPORT9                        mSavedViewPort;
   IDirect3DSurface9                   mSavedRenderTargetSurf;
   IDirect3DSurface9                   mSavedDepthStencilSurf;
      
   bool                                mDCBRenderingEnabled : 1;
   bool                                mDCBRenderingToggle : 1;  //we can only swap the flag at the end of a render cycle.. so queue it.
   bool                                mDCBRenderingActive : 1;
   bool                                mThreadedNodeDCBCreationIssued : 1;
   bool                                mPredicatedRendering : 1;
   
   static void                         beginDCBRenderCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);
      
   void                                beginDCBRenderInternal(const BTerrainQuadNodePtrArray* pVisibleNodeInstances, const BTerrainQuadNodePtrArray* pVisibleSkirtNodeInstances);

   void                                setDCBPredicationData(const BTerrainQuadNode* pNode);
   struct BCalcNodeDCBsCallbackData
   {
      BTerrainQuadNodePtrArray   mNodeList;
      BTerrainQuadNodePtrArray   mSkirtNodeList;
      DWORD                      mColorWriteEnable;
      DWORD                      mFillMode;
      DWORD                      mHalfPixelOffset;
   };

   BCalcNodeDCBsCallbackData           mCalcNodeDCBsCallbackData;
   
   BWin32Event                         mNodeDCBCreationEvent;
};
//-----------------------------------
extern BTerrainRender gTerrainRender;
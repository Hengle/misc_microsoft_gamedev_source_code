//==============================================================================
// worldVisibility.h
//
// The world visibility renderer.
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "threading\eventDispatcher.h"
#include "renderThread.h"
#include "effect.h"
#include "effectFileLoader.h"
#include "utils\heapSingleton.h"
#include "D3DTextureManager.h"

struct BBlackmapParams
{
   BBlackmapParams() { clear(); }

   IDirect3DTexture9* mpTexture;
   IDirect3DTexture9* mpUnexploredTexture;
   BVec4              mParams[3];

   void clear() { Utils::ClearObj(*this); }
};

//==============================================================================
// Class definition
//==============================================================================
class BWorldVisibility : public BRenderCommandListener, public BHeapSingleton<BWorldVisibility, BRenderFixedHeapAllocator>
#ifdef ENABLE_RELOAD_MANAGER
   , public BEventReceiver
#endif
{
   BWorldVisibility(const BWorldVisibility&);
   BWorldVisibility& operator= (const BWorldVisibility&);
   
public:
   BWorldVisibility();
   ~BWorldVisibility();
   
   void init(long dataDirIR, uint visibilityTextureDim = 512);
   void deinit(void);
      
   void setWorldSize(float worldSize) { mWorldSize = worldSize; }
   
   void setExploredBrightness(float value) { mExploredBrightness = value; }
   void setUnexploredBrightness(float value) { mUnexploredBrightness = value; }
   void setEdgeOfWorldTransition(float value) { mEdgeOfWorldTransition = value; }
      
   // pVisibilityTexture will be passed to the render thread! Don't delete it until you are sure the renderer is idle.
   void updateVisibility(bool enabled, uint viewportIndex, const BVec4& playableBounds, IDirect3DTexture9* pVisibilityTexture);
   
protected:
   struct BPTVertex
   {
      XMFLOAT2 position;
      XMHALF2 uv;
   };

   struct BGenerateVisibilityPacket
   {
      IDirect3DTexture9*   mpVisibilityTexture;
      uint                 mViewportIndex;
      float                mWorldSize;
      float                mExploredBrightness;
      float                mUnexploredBrightness;
      float                mEdgeOfWorldTransition;
      float                mUnexploredMaskUVMultiplier;
      BVec4                mPlayableBounds;
      bool                 mEnabled;
   };

   enum 
   {
      cMMCommandFilterVisibility = 0,
      
      cMMCommandMax
   };
      
   // BRenderCommandListener interface
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader &header, const uchar *pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
   
   // Worker thread functions
   void tickEffect(void);
   
   void workerRenderVisibility(const BGenerateVisibilityPacket* pVisibilityData);
   void workerFilterVisibility(const BGenerateVisibilityPacket* pVisibilityData);
   void workerUpdateVisibility(const BGenerateVisibilityPacket* pVisibilityData);
   void workerDrawQuad(uint width, uint height);
         
   BFXLEffectFileLoader*         mpEffectLoader;
      
   BFXLEffectParam               mVisibilityTextureParam;
      
   BFXLEffectTechnique           mBlurVisibilityXTechnique;
   BFXLEffectTechnique           mBlurVisibilityYTechnique;
   
   BFXLEffectParam               mPrevVisibilityTextureParam;
   BFXLEffectParam               mBlurVisibilityOffsetsParam;
   BFXLEffectParam               mBlurVisibilityWeightsParam;

   BFXLEffectParam               mBlackMapTintParam;
   
   IDirect3DVertexDeclaration9*  mBPTVertexDecl;
   
   uint                          mVisibilityTextureDim;
   
   float                         mWorldSize;
   float                         mExploredBrightness;
   float                         mUnexploredBrightness;
   float                         mEdgeOfWorldTransition;
   float                         mUnexploredMaskUVMultiplier;
   long                          mDataDirID;
   
   enum { cMaxViewports = 2 };         
   IDirect3DTexture9*            mpPrevVisibilityTexture[cMaxViewports];
   
   BD3DTextureManager::BManagedTexture* mpUnexploredTexture;
   
   void                          setDefaultConfig(void);
   bool                          loadConfig(void);
#ifdef ENABLE_RELOAD_MANAGER
   virtual bool                  receiveEvent(const BEvent& event, BThreadIndex threadIndex);
#endif
};


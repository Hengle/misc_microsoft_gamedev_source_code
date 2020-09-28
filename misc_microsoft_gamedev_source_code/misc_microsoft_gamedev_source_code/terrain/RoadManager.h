//============================================================================
//
//  RoadManager.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once
// xrender
#include "rendercommand.h"
#include "effect.h"
#include "renderThread.h"

// xcore
#include "threading\eventDispatcher.h"

// xgameRender
#include "visibleLightManager.h"

//terrain
#include "TerrainTexturing.h"
#include "TerrainQuadNode.h"

class BFXLEffectFileLoader;
class BTerrainRoadManager;
//--------------------------------------------
class BTerrainRoadQNChunk
{
public:
   BTerrainRoadQNChunk():
      mpPhysicalMemoryPointer(0),
         mSegmentVB(0),
         mNumVerts(0),
         mQNParentIndex(0)
      {

      }
      ~BTerrainRoadQNChunk()
      {
         freeDeviceData();
         if (mpPhysicalMemoryPointer)
         {
            XPhysicalFree(mpPhysicalMemoryPointer);
            mpPhysicalMemoryPointer = NULL;
         }
      }



      LPDIRECT3DVERTEXBUFFER9 mSegmentVB;
      int mNumVerts;
      int mNumPrims;

      int mQNParentIndex;
private:
   void freeDeviceData()
   {
      if(mSegmentVB)
      {
         delete mSegmentVB;
         mSegmentVB=NULL;
      }
   }

   void* mpPhysicalMemoryPointer;

   friend class BTerrainIOLoader;
};
//--------------------------------------------
class BTerrainRoad
{
public:
   BTerrainRoad():
            mRoadChunks(0)
   {
    
      
   }
   ~BTerrainRoad()
   {
      freeDeviceData();

      for(uint i=0;i<mRoadChunks.size();i++)
      {
         if(mRoadChunks[i])
         {
            delete mRoadChunks[i];
            mRoadChunks[i] =NULL;
         }
      }
      mRoadChunks.clear();
   }

    void setRoadTexture(int dirID, const char *filename);


private:


   void freeDeviceData()
   {
      for(int i=0;i<cTextureTypeMax;i++)
         mTexture[i].release();
   }

   BDynamicArray<BTerrainRoadQNChunk*> mRoadChunks;

   void     loadRoadTextureCallback(void *pData);
  


   BD3DTexture                   mTexture[cTextureTypeMax];

   friend class BTerrainIOLoader;
   friend class BTerrainRoadManager;
};
//--------------------------------------------
class BTerrainRoadManager : public BRenderCommandListener, BEventReceiverInterface
{
public:
   BTerrainRoadManager();
   ~BTerrainRoadManager();

   bool           init();
   bool           deinit();

   void           destroy();

   void           render(const BTerrainQuadNode *terrainQuadGrid, int tileIndex);


   void           allocateTempResources(void);
   void           releaseTempResources(void);
private:
   enum 
   {
      cRenderPassANS =0,
      cRenderPassANSE =1,
      cRenderPassANSR =2,
      cRenderPassFull =3,

      cRenderPassVis =4,      //visualization pass
   };
   void                       setVisControlRegs();
   int                       setupTexturing(int roadIndex);

   BDynamicArray<BTerrainRoad*>   mRoads;    //array

   //FX File and FX Handles
   BFXLEffectFileLoader*           mpEffectLoader;
   BFXLEffect                      mTerrainRoadShader;

   //ugg.. i hate duplicating this.. but it does speed things up...(i think...)
   BFXLEffectTechnique             mCurrTechnique;

   BFXLEffectParam                 mPosComprMin;
   BFXLEffectParam                 mPosComprRange;
   BFXLEffectParam                 mShaderDataValsHandle;
   
   BFXLEffectParam                 mShaderPositionTextureHandle;
   BFXLEffectParam                 mShaderNormalTextureHandle;

   BFXLEffectParam                 mShaderRoadTextureHandle[cTextureTypeMax];

   //LIGHTING
   BFXLEffectParam                 mLocalLightingEnabled;
   BFXLEffectParam                 mLocalShadowingEnabled;
   BFXLEffectParam                 mLightData;
   BSceneLightManager::BActiveLightIndexArray mCurVisibleLightIndices;

   IDirect3DTexture9*          mpLightAttribTexture;
   uint                        mNextLightAttribTexelIndex;
   void                       setupLighting(const BTerrainQuadNodeDesc& desc);
   XMFLOAT4*                  computeLightAttribTexelPtr(XMFLOAT4* pDstTexture, uint texelIndex);
   uint                       fillLightAttribTexture(const BSceneLightManager::BActiveLightIndexArray& lights, uint firstIndex, uint& outNumLights);

   

private://BRenderCommandListenerInterface
   BEventReceiverHandle            mEventHandle;


   enum
   {
      cTRM_Destroy=0,
   };
   bool                       destroyInternal();
   //BRenderCommandListenerInterface
   virtual void               processCommand(const BRenderCommandHeader& header, const unsigned char* pData);  
   virtual void               frameBegin(void);
   virtual void               frameEnd(void);
   virtual void               initDeviceData(void);
   virtual void               deinitDeviceData(void);


   enum 
   {
      cEventClassTerrainRenderReload = cEventClassFirstUser
   };
   void                       loadEffect(void);
   void                       initEffectConstants(void);
   void                       tickEffect(void);
   //BEventReceiverInterface
   virtual bool               receiveEvent(const BEvent& event, BThreadIndex threadIndex);


   friend class BTerrainIOLoader;
   friend class BTerrainRender;
   friend class BTerrain;
};

//--------------------------------------------
extern BTerrainRoadManager  gRoadManager;  

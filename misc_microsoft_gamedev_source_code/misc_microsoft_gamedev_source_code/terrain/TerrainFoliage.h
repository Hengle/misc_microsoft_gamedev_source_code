//============================================================================
//
//  TerrainFoliage.h
//  
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma  once

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


class BTerrainFoliageManager;
//-------------------------------------------------------------
class BTerrainFoliageQNChunk
{
public:
   BTerrainFoliageQNChunk():
      mQNParentIndex(0),
         mNumSets(0),
         mSetIndexes(0),
         mSetPolyCount(0),
         mSetIBs(0),
         mpPhysicalMemoryPointer(0)
   {

   }
   ~BTerrainFoliageQNChunk()
   {
      if(mSetIndexes)
      {
         delete []mSetIndexes;
         mSetIndexes=NULL;
      }
      if(mSetPolyCount)
      {
         delete []mSetPolyCount;
         mSetPolyCount=NULL;
      }
      if(mSetIBs)
      {
         for(uint i=0;i<mNumSets;i++)
         {
            delete mSetIBs[i];
            mSetIBs[i]=NULL;
         }
         delete []mSetIBs;
         mSetIBs=NULL;
      }
      if(mpPhysicalMemoryPointer)
      {
         XPhysicalFree(mpPhysicalMemoryPointer);
         mpPhysicalMemoryPointer=NULL;
      }
   }
   uint                    mQNParentIndex;
   uint                    mNumSets;
   int                     *mSetIndexes;
   int                     *mSetPolyCount;
   LPDIRECT3DINDEXBUFFER9  *mSetIBs;
   void                    *mpPhysicalMemoryPointer;
};
//-------------------------------------------------------------
class BTerrainFoliageSet: BEventReceiver
{
public:
   BTerrainFoliageSet():
      mPositionsTexture(0),
      mVertNormalsTexture(0),
      mReloadTextures(0),
      mReloadXML(0),
      mBacksideShadowScalar(1.0f)
   {

   };
   ~BTerrainFoliageSet()
   {
      freeDeviceData();
      deinit();
   }




   bool init()
   {
      eventReceiverInit(cThreadIndexRender);
      return true;
   }
   bool deinit()
   {
      gReloadManager.deregisterClient(mEventHandle);

      if (mEventHandle != cInvalidEventReceiverHandle)
      {
         gEventDispatcher.removeClientDeferred(mEventHandle, true);
         mEventHandle = cInvalidEventReceiverHandle;
      }
      eventReceiverDeinit();
      return true;
   }
 

   void loadSet(const char *texNameRoot);

private:
   void loadPositionsXML();
   void loadTextures();
   void setTexturesCallback(void *pData);
   

   void freeDeviceData()
   {

      mAlbedoTexture.release();
      mNormalTexture.release();
      mSpecularTexture.release();
      mOpacityTexture.release();

      if(mPositionsTexture)
      {
         mPositionsTexture->Release();
         mPositionsTexture=NULL;
      }
      if(mVertNormalsTexture)
      {
         mVertNormalsTexture->Release();
         mVertNormalsTexture=NULL;
      }

      
   }

   
   BFixedString256   mFilename;
   int               mNumTypesInSet;
   float             mBacksideShadowScalar;
   BD3DTexture       mAlbedoTexture;
   BD3DTexture       mNormalTexture;
   BD3DTexture       mSpecularTexture;
   BD3DTexture       mOpacityTexture;
   

   D3DLineTexture   *mPositionsTexture;
   D3DLineTexture   *mVertNormalsTexture;

private:
   enum
   {
      cFoliageSetReloadFileEvent = cEventClassFirstUser
   };

   //BEventReceiverInterface
   virtual bool               receiveEvent(const BEvent& event, BThreadIndex threadIndex)
   {
      ASSERT_THREAD(cThreadIndexRender);

      switch (event.mEventClass)
      {
      case cFoliageSetReloadFileEvent:
         {
            BReloadManager::BNotificationPayload* pPayload = static_cast<BReloadManager::BNotificationPayload*>(event.mpPayload);

            if (strstr(pPayload->mPath.getPtr(), ".ddx") != NULL)
               mReloadTextures = true;
            else if (strstr(pPayload->mPath.getPtr(), ".xml") != NULL)
               mReloadXML = true;
            break;            
         }
      }

      return false;
   }
   bool needReload()
   {
      return mReloadTextures | mReloadXML;
   }
   void reload()
   {
      if(mReloadTextures)
      {
         loadTextures();
         mReloadTextures=false;
      }
      if(mReloadXML)
      {
         loadPositionsXML();
         mReloadXML=false;
      }
   }
   bool mReloadTextures : 1;
   bool mReloadXML : 1;


    friend class BTerrainFoliageManager;
    friend class BTerrainIOLoader;
};
//-------------------------------------------------------------
class BTerrainFoliageManager : public BRenderCommandListener
{
public:
   BTerrainFoliageManager();
   ~BTerrainFoliageManager();

   bool           init();
   bool           deinit();

   void           destroy();

   void           update();

   void           render(const BTerrainQuadNode *terrainQuadGrid, int tileIndex);
   void           renderCustom(eTRenderPhase renderPhase,const BTerrainQuadNodePtrArray* pNodes,const BTerrainQuadNode *terrainQuadGrid);
   void           allocateTempResources(void);
   void           releaseTempResources(void);

   static const int      cNumXFoliagePerChunk=64;
   static const int      cNumZFoliagePerChunk=64;
   static const int      cNumVertsPerBlade=10;

   void    newSet(const char *ifilename);

private:
   BDynamicArray<BTerrainFoliageSet*> mFoliageSets;
   BDynamicArray<BTerrainFoliageQNChunk*> mFoliageChunks;
   LPDIRECT3DVERTEXBUFFER9         mChunkVertexBuffer;

   float                      mAnimTime;
   //FX File and FX Handles
   BFXLEffectFileLoader*           mpEffectLoader;
   BFXLEffect                      mTerrainFoliageShader;

   //ugg.. i hate duplicating this.. but it does speed things up...(i think...)
   BFXLEffectTechnique             mCurrTechnique;

   BFXLEffectParam                 mPosComprMin;
   BFXLEffectParam                 mPosComprRange;
   BFXLEffectParam                 mShaderTerrainDataValsHandle;
   BFXLEffectParam                 mShaderTerrainPositionTextureHandle;
   BFXLEffectParam                 mShaderTerrainBasisTextureHandle;

   BFXLEffectParam                 mShaderFoliagePositions;
   BFXLEffectParam                 mShaderFoliageNormals;
   
   BFXLEffectParam                 mShaderFoliageAlbedo;
   BFXLEffectParam                 mShaderFoliageNormal;
   BFXLEffectParam                 mShaderFoliageSpecular;
   BFXLEffectParam                 mShaderFoliageOpacity;
   BFXLEffectParam                 mShaderFoliageBacksideShadowScalar;

   BFXLEffectParam                 mShaderFoliageAnimTime;

   //BLACKMAP 
   BFXLEffectParam                 mBlackmapEnabled;
   BFXLEffectParam                 mBlackmapSampler;
   BFXLEffectParam                 mBlackmapUnexploredSampler;
   BFXLEffectParam                 mBlackmapParams0;
   BFXLEffectParam                 mBlackmapParams1;
   BFXLEffectParam                 mBlackmapParams2;

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

   void                       setVisControlRegs();

   enum 
   {
      cRenderPassANS =0,
      cRenderPassANSE =1,
      cRenderPassANSR =2,
      cRenderPassFull =3,

      cRenderPassVis =4,      //visualization pass
   };
   int                        setupTexturing(eTRenderPhase renderPhase,int foliageIndex);

private://BRenderCommandListenerInterface

   enum
   {
      cTRM_Destroy=0
   };
   bool                       destroyInternal();
   //BRenderCommandListenerInterface
   virtual void               processCommand(const BRenderCommandHeader& header, const unsigned char* pData);  
   virtual void               frameBegin(void);
   virtual void               frameEnd(void);
   virtual void               initDeviceData(void);
   virtual void               deinitDeviceData(void);


   void                       loadEffect(void);
   void                       initEffectConstants(void);
   void                       tickEffect(void);

   friend class BTerrain;
   friend class BTerrainIOLoader;
};

extern BTerrainFoliageManager gFoliageManager;
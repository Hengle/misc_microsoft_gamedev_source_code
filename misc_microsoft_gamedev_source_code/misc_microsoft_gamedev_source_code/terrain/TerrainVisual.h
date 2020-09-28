//============================================================================
//
//  TerrainVisual.h
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

//--------------------------------------------
struct XTDVisual;
class TerrainIOLoader;

#pragma warning(push)
#pragma warning(disable: 4324)
class BTerrain3DSimpleVisualPacket 
{

public:
   BTerrain3DSimpleVisualPacket():
      mTerrainPositions(0),
      mTerrainNormals(0),
      mTerrainAO(0),
      mTerrainAlpha(0),
      mpPhysicalMemoryPointer(0),
      mpPhysicalMemoryPointerAO(0),
      mpPhysicalMemoryPointerALPHA(0)
   {
   }
   
   ~BTerrain3DSimpleVisualPacket()
   {
      freeDeviceData();

      mTerrainPositions=NULL;
      mTerrainNormals=NULL;
      mTerrainAO=NULL;
      mTerrainAlpha=NULL;

      if (mpPhysicalMemoryPointer)
      {
         XPhysicalFree(mpPhysicalMemoryPointer);
         mpPhysicalMemoryPointer = NULL;
      }
      if (mpPhysicalMemoryPointerAO)
      {
         XPhysicalFree(mpPhysicalMemoryPointerAO);
         mpPhysicalMemoryPointerAO = NULL;
      }
      if (mpPhysicalMemoryPointerALPHA)
      {
         XPhysicalFree(mpPhysicalMemoryPointerALPHA);
         mpPhysicalMemoryPointerALPHA = NULL;
      }
   }
      
   XMVECTOR                   mPosCompMin;
   XMVECTOR                   mPosCompRange;
   
   LPDIRECT3DTEXTURE9         mTerrainPositions;
   LPDIRECT3DTEXTURE9         mTerrainNormals;
   LPDIRECT3DTEXTURE9         mTerrainAO;
   LPDIRECT3DTEXTURE9         mTerrainAlpha;

   int                        mPositionsSize;
   int                        mNormalsSize;
   int                        mTerrainAOSize;
   int                        mTerrainAlphaSize;
   
   void*                      mpPhysicalMemoryPointer;
   void*                      mpPhysicalMemoryPointerAO;
   void*                      mpPhysicalMemoryPointerALPHA;

   bool convertFromMemoryMain();
   void freeDeviceData();
};
#pragma warning(pop)

//--------------------------------------------
class BTerrainVisual : public BRenderCommandListener, BEventReceiverInterface
{
public:
   BTerrainVisual();
   ~BTerrainVisual();

   bool           init();
   bool           deinit();

   void           destroy();


   inline LPDIRECT3DTEXTURE9         getPosTex()         {if(!mVisualData) return NULL; return mVisualData->mTerrainPositions;};
   inline LPDIRECT3DTEXTURE9         getNormTex()        {if(!mVisualData) return NULL; return mVisualData->mTerrainNormals;};
   inline XMVECTOR                   getPosMin()         {if(!mVisualData) return XMVectorZero(); return mVisualData->mPosCompMin;};
   inline XMVECTOR                   getPosRange()       {if(!mVisualData) return XMVectorZero(); return mVisualData->mPosCompRange;};
   inline LPDIRECT3DTEXTURE9         getAOTex()          {if(!mVisualData) return NULL; return mVisualData->mTerrainAO;};
   inline LPDIRECT3DTEXTURE9         getAlphaTex()       {if(!mVisualData) return NULL; return mVisualData->mTerrainAlpha;};
   inline LPDIRECT3DTEXTURE9         getLightTex()       {if(!mVisualData) return NULL; return mTerrainAdditiveLights;};

   int                               getNumXVerts(){return mNumXVerts;};
   float                             getTileScale(){return mTileScale;}
   float                             getOOTileScale(){return mOOTileScale;}

  
   bool                             isLODEnabled();
   void                             setLODEnabled(bool onoff);
   bool                             isRefinementEnabled();
   void                             setRefinementEnabled(bool onoff);

 //  void                             computeAdaptiveEdgeTess(void);
   //needed for multithreaded edgeTesselation sampling
   void                             beginEdgeTesselationCalc();
   void                             endEdgeTesselationCalc();

   
   BTerrain3DSimpleVisualPacket     *getVisData() { return mVisualData;};
   
   void                             renderPatchBoxes();
   
   IDirect3DIndexBuffer9*           getPatchEdgeDynamicIB() const { return mpPatchEdgeDynamicIB; }

private:  //members

   BEventReceiverHandle            mEventHandle;


   //information for everyone else
   float                      mTileScale;
   float                      mOOTileScale;
   int                        mNumXVerts;

  // BTerrain3DVisualPacket     *mVisualData;   //nicely packaged..
   BTerrain3DSimpleVisualPacket  *mVisualData;


   //For hardware tesselation
   bool                       initTesselatorData(int numXVerts, int numZVerts);
   static void                calcWorkingPatchTess(uint minXPatch, uint minZPatch,uint maxXPatch, uint maxZPatch,uint mNumZPatches,
                                    byte* pMaxPatchTessallation,XMVECTOR* pPatchBBs,float* pWorkingPatchTess, 
                                    XMMATRIX worldToScreen,XMVECTOR camPos,XMMATRIX ident);
   void                       calcPatchEdgeTess();
   static void                tessWorkerThreadCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);

   int                        mNumXPatches;
   int                        mNumZPatches;
   byte                       *mpMaxPatchTessallation;
   float                      *mpWorkingPatchTess;
   XMVECTOR                   *mpPatchBBs;

   BDynamicRenderArray<uint>  mMulByXPatchesLookup;
  
   
   IDirect3DIndexBuffer9*     mpPatchEdgeDynamicIB;
     
   //For precomputed lighting on the terrain
   void                       clearLightTex();
   void                       *mpPhysicalLightingDataPointer;
   LPDIRECT3DTEXTURE9         mTerrainAdditiveLights;
   bool                       convertLightingFromMemory();

   //descrete index buffers for our LOD
   bool                       mLODEnabled;
   bool                       mRefinementEnabled;

private://BRenderCommandListenerInterface


   enum
   {
      cTVC_Destroy=0,
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
   //BEventReceiverInterface
   virtual bool               receiveEvent(const BEvent& event, BThreadIndex threadIndex);




   friend class BTerrainIOLoader;
   friend class BTerrainRender;
   friend class BTerrain;
   friend class BTerrainDeformer;
};

extern BTerrainVisual  gTerrainVisual;  

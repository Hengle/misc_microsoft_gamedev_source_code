//============================================================================
//
//  TerrainVisual.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
// terrain 
#include "TerrainPCH.h"
#include "Terrain.h"
#include "TerrainVisual.h"
#include "TerrainMetric.h"
#include "TerrainIO.h"
#include "TerrainQuadNode.h"
#include "TerrainDeformer.h"

// xcore
#include "consoleOutput.h"
#include "reloadManager.h"
#include "math\vmxIntersection.h"
#include "threading\workDistributor.h"

// xrender
#include "renderThread.h"
#include "bd3d.h"
#include "renderDraw.h"
#include "effect.h"
#include "renderEventClasses.h"
#include "asyncFileManager.h"

// xgameRender 
#include "..\xgameRender\tiledAA.h"


const float cTILE_SCALE = .25f;
const bool cMultithreadedEdgeCalc = true;


BTerrainVisual gTerrainVisual;

//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
BTerrainVisual::BTerrainVisual():
mTileScale(cTILE_SCALE),
mOOTileScale(1.0f / cTILE_SCALE),
mLODEnabled(true),
mRefinementEnabled(true),
mVisualData(0),
mpMaxPatchTessallation(0),
mpPhysicalLightingDataPointer(0),
mTerrainAdditiveLights(0),
mEventHandle(cInvalidEventReceiverHandle),
mpPatchEdgeDynamicIB(NULL)
{

}
//============================================================================
// BTerrainVisual::~BTerrainVisual
//============================================================================
BTerrainVisual::~BTerrainVisual()
{
   // Never do anything in here! This destructor will be called in an unknown order relative to other subsystems.
   //deinit();
}
//============================================================================
// BTerrainVisual::init
//============================================================================
bool BTerrainVisual::init()
{
   ASSERT_MAIN_THREAD

   commandListenerInit();

   return true;
}
//============================================================================
// BTerrainVisual::deinit
//============================================================================
bool BTerrainVisual::deinit()
{
   // Block for safety. 
   gRenderThread.blockUntilGPUIdle();

   commandListenerDeinit();

   gReloadManager.deregisterClient(mEventHandle);

   if (mEventHandle != cInvalidEventReceiverHandle)
   {
      gEventDispatcher.removeClientDeferred(mEventHandle, true);
      mEventHandle = cInvalidEventReceiverHandle;
   }

   return true;
}
//============================================================================
// BTerrainVisual::destroy
//============================================================================
void BTerrainVisual::destroy()
{
   gRenderThread.submitCommand(mCommandHandle,cTVC_Destroy);
}
//============================================================================
// BTerrainVisual::destroyInternal
//============================================================================
bool BTerrainVisual::destroyInternal()
{
   if(mVisualData)
   {
      mVisualData->freeDeviceData();
      delete mVisualData;
      mVisualData=NULL;
   }

   if(mpMaxPatchTessallation)
   {
      XPhysicalFree(mpMaxPatchTessallation);
      mpMaxPatchTessallation=NULL;
   }
   
   if(mpPatchBBs)
   {
      XPhysicalFree(mpPatchBBs);
      mpPatchBBs=NULL;
   }

   if(mpWorkingPatchTess)
   {
      delete [] mpWorkingPatchTess;
      mpWorkingPatchTess=NULL;
   }
   mMulByXPatchesLookup.clear();

   clearLightTex();
   
   return true;
}
//============================================================================
// BTerrainRender::BTerrainRender
//============================================================================
bool BTerrainVisual::isLODEnabled()
{
   return mLODEnabled;
}
//============================================================================
// BTerrainVisual::setLODEnabled
//============================================================================
void BTerrainVisual::setLODEnabled(bool onoff)
{
   mLODEnabled = onoff;
}
//============================================================================
// BTerrainVisual::isRefinementEnabled
//============================================================================
bool BTerrainVisual::isRefinementEnabled()
{
   return mRefinementEnabled;
}
//============================================================================
//  BTerrainVisual::setRefinementEnabled
//============================================================================
void BTerrainVisual::setRefinementEnabled(bool onoff)
{
   mRefinementEnabled = onoff;
}
//============================================================================
// BTerrainVisual::initDeviceData
//============================================================================
void BTerrainVisual::initDeviceData(void)
{
   ASSERT_RENDER_THREAD
      //TEXTURES CREATED BY TERRAINIO.CPP


   loadEffect();
}
//============================================================================
// BTerrainVisual::deinitDeviceData
//============================================================================
void BTerrainVisual::deinitDeviceData(void)
{
   ASSERT_RENDER_THREAD

}
//============================================================================
// BTerrainVisual::initTesselatorData
//============================================================================
bool BTerrainVisual::initTesselatorData(int numXVerts, int numZVerts)
{
   return true;
}
//============================================================================
// BTerrainVisual::renderPatchBoxes
//============================================================================
#include "vertexTypes.h"
#include "fixedFuncShaders.h"
void BTerrainVisual::renderPatchBoxes()
{
   const int NumQuads = mNumXPatches * mNumZPatches;

   BPVertex* pOutDat = (BPVertex*)gRenderDraw.lockDynamicVB(NumQuads * 2 , sizeof(BPVertex));
   

   //memcpy(pOutDat,mpPatchBBs,sizeof(BPVertex) * 2 * NumQuads);
   
   for(int i=0;i<NumQuads;i++)
   {
      pOutDat[i*2].pos.x = mpPatchBBs[i*2].x;
      pOutDat[i*2].pos.y = mpPatchBBs[i*2].y;
      pOutDat[i*2].pos.z = mpPatchBBs[i*2].z;
      pOutDat[i*2+1].pos.x = mpPatchBBs[i*2+1].x;
      pOutDat[i*2+1].pos.y = mpPatchBBs[i*2+1].y;
      pOutDat[i*2+1].pos.z = mpPatchBBs[i*2+1].z;
   }

   BD3D::mpDev->SetStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(BPVertex));

   gRenderDraw.unlockDynamicVB();


   BD3D::mpDev->SetIndices(NULL);
   
   BD3D::mpDev->SetVertexDeclaration(BPVertex::msVertexDecl);

   // Set vertex declaration to match, disable any vertex shaders.
   BD3D::mpDev->SetVertexShader(gFixedFuncShaders.workerGetVertexShader(cPosVS));
   BD3D::mpDev->SetPixelShader(gFixedFuncShaders.workerGetPixelShader(cWhitePS));

   XMMATRIX worldToProj = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToProj, true);
   BD3D::mpDev->SetVertexShaderConstantF(0, (float*) worldToProj.m, 4);       

   BD3D::mpDev->DrawPrimitive(D3DPT_LINELIST,0,NumQuads);

   
}
//============================================================================
// BTerrainVisual::computeAdaptiveEdgeTess
//============================================================================
#if 0
//void BTerrainVisual::computeAdaptiveEdgeTess(void)
//{
  // if (!mTessGPUShader.getEffect())
  //    return;


   //BDynamicRenderArray<float> patchAreas(mNumZPatches);
   //   
   //const XMMATRIX worldToScreen = gRenderDraw.getWorkerActiveMatrixTracker().getMatrixByValue(cMTWorldToProj,false) * gRenderDraw.getWorkerActiveMatrixTracker().getMatrixByValue(cMTProjToScreen,false) ;
   //const XMVECTOR camPos = gRenderDraw.getWorkerActiveMatrixTracker().getWorldCamPos();
   //const XMMATRIX ident = XMMatrixIdentity();

   //const int NumQuads = mNumXPatches * mNumZPatches;
   //const int numXChunks =gTerrain.getNumXChunks();

   //static float maxAreaCutoff= 10000000.f;

   //static float maxPatchTessallation[]={15.0f,7.0f,3.0f,1.0f}; //CLM - the values of mpMaxPatchTessallation, written to disk, reference this array
   //static float desiredPT[] = {1.0f/100.0f,1.0f/200.f,1.0f/300.0f,1.0f/400.0f };   //CLM - change these to whatever depending on your visual quality.
   //     
   //#ifndef BUILD_FINAL
   //if (mLODEnabled) 
   //{
   //#endif

   //   int qInd = 0;
   //   
   //   patchAreas.setAll(999999.0f);
   //   
   //   Utils::BPrefetchState prefetch = Utils::BeginPrefetch(mpPatchBBs, 3);
   //   
   //   for (int x = 0; x < mNumXPatches; x++)
   //   {
   //      Utils::TouchCacheLine(0, &mpMaxPatchTessallation[qInd]);
   //                              
   //      const int savedQInd = qInd;
   //                        
   //      for (int z = 0; z < mNumZPatches; z++)
   //      {
   //         prefetch = Utils::UpdatePrefetch(prefetch, &mpPatchBBs[2 * qInd], 3);
   //         
   //         XMVECTOR min = mpPatchBBs[2 * qInd];
   //         XMVECTOR max = mpPatchBBs[2 * qInd + 1];
   //                     
   //         BVMXIntersection::calculateBoxArea(patchAreas[z], camPos, min, max, ident, worldToScreen);
   //         
   //         qInd++;
   //      }
   //   
   //      qInd = savedQInd;
   //      
   //      // FastMemSet() will use dcbz128 to clear the cache lines of the dest. buffer, avoiding the reads from main memory.
   //      Utils::FastMemSet(&mpWorkingPatchTess[qInd], 0, mNumZPatches * sizeof(mpWorkingPatchTess[0]));
   //                     
   //      for (int z = 0; z < mNumZPatches; z++)
   //      {
   //         const int radDefInd = mpMaxPatchTessallation[qInd];
   //                  
   //         mpWorkingPatchTess[qInd] = (float)Math::fSelectClamp(patchAreas[z] * desiredPT[radDefInd], 1.0f, maxPatchTessallation[radDefInd]);   
   //         patchAreas[z] = 999999.0f;
   //         
   //         qInd++;
   //      }
   //   }

   //#ifndef BUILD_FINAL
   //}
   //else
   //{
   //   for(int i = 0; i < NumQuads; i++)
   //      mpWorkingPatchTess[i] = mRefinementEnabled ? maxPatchTessallation[mpMaxPatchTessallation[i]] : 15.0f;
   //}
   //#endif

   ////now that we've got the working volumes
   //const int offs[] = {0, -1, 1, 0, 0, 1, -1, 0};
   //const int patchChunkStride = numXChunks * 16;

   //int xChunk = 0;
   //int chunkXOff = 0;
   //int yChunk = 0;
   //int chunkYOff = 0;

   //float* pOutDat = (float*)gRenderDraw.lockDynamicIB(NumQuads * 4 * 4, D3DFMT_INDEX32);   

   //BDynamicRenderArray<uint> mulByXPatchesLookup(mNumZPatches);
   //for (int i = 0; i < mNumZPatches; i++)
   //   mulByXPatchesLookup[i] = i * mNumXPatches;
   //
   //for (int x = 0; x < mNumXPatches; x++)
   //{
   //   uint srcPatchIndex = x;
   //   
   //   for (int z = 0; z < mNumZPatches; z++)
   //   {
   //      //CLM This process has to match what our vertex shader is expecting to read so swizzle it by 4x4 chunks.

   //      int xContrib = ((xChunk * 16) + (chunkYOff * 4));        
   //      int yContrib = (patchChunkStride * yChunk) + chunkXOff; 
   //      int q = 4 * (xContrib + yContrib);

   //      float a = mpWorkingPatchTess[srcPatchIndex];
   //      srcPatchIndex += mNumXPatches;

   //      for(int j = 0; j < 4; j++)
   //      {
   //         int nx = x + offs[j * 2];
   //         int ny = z + offs[j * 2 + 1];

   //         //if (nx < 0 || nx >= mNumXPatches || ny < 0 || ny >= mNumZPatches)
   //         if ( (((uint)nx) >= ((uint)mNumXPatches)) || 
   //            (((uint)ny) >= ((uint)mNumZPatches))
   //            )
   //         {
   //            pOutDat[q + j] = a;
   //            continue;
   //         }

   //         float b = mpWorkingPatchTess[nx + mulByXPatchesLookup[ny]];      

   //         pOutDat[q + j] = Math::Max<float>(a, b); 
   //      }

   //      yChunk = (z + 1) >> 2;
   //      chunkYOff = (z + 1) & 3;
   //   }

   //   xChunk = (x + 1) >> 2;
   //   chunkXOff = (x + 1) & 3;

   //   yChunk = 0;
   //   chunkYOff = 0;
   //}

   //gRenderDraw.unlockDynamicIB();
//}
#endif
//============================================================================
// BTerrainVisual::convertLightingFromMemory
//============================================================================
void BTerrainVisual::clearLightTex()
{
   if(mpPhysicalLightingDataPointer)
   {
      XPhysicalFree(mpPhysicalLightingDataPointer);
      mpPhysicalLightingDataPointer=NULL;
   }
   if(mTerrainAdditiveLights)
   {
      delete mTerrainAdditiveLights;
      mTerrainAdditiveLights=NULL;
   }
   
}
//============================================================================
// BTerrainVisual::convertLightingFromMemory
//============================================================================
bool BTerrainVisual::convertLightingFromMemory()
{
   if(!mpPhysicalLightingDataPointer)
      return false;

   UINT imgSize =0;
   unsigned char *mDevicePtrCounter = (unsigned char *)mpPhysicalLightingDataPointer;


   int numxVerts = gTerrainVisual.getNumXVerts();

   //Additive lighting data
   mTerrainAdditiveLights = new D3DTexture();
   imgSize = XGSetTextureHeader(numxVerts,numxVerts,1,0,D3DFMT_DXT1,0,0,0,0,mTerrainAdditiveLights,NULL,NULL);
   XGOffsetResourceAddress( mTerrainAdditiveLights, mDevicePtrCounter ); 
   mDevicePtrCounter+=imgSize;

   //sRGB space
   GPUTEXTURE_FETCH_CONSTANT& fc = mTerrainAdditiveLights->Format;
   fc.SignX = GPUSIGN_GAMMA;
   fc.SignY = GPUSIGN_GAMMA;
   fc.SignZ = GPUSIGN_GAMMA;
   

   return true;
}
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//============================================================================
// BTerrainVisual::processCommand
//============================================================================
void BTerrainVisual::processCommand(const BRenderCommandHeader& header, const unsigned char* pData)
{
   ASSERT_RENDER_THREAD
      
   switch (header.mType)
   {
   case cTVC_Destroy:
      {
         destroyInternal();
         break;
      }
   }
};  
//============================================================================
// BTerrainVisual::frameBegin
//============================================================================
void BTerrainVisual::frameBegin(void)
{
   ASSERT_RENDER_THREAD
};
//============================================================================
//  BTerrainVisual::frameEnd
//============================================================================
void BTerrainVisual::frameEnd(void)
{
   ASSERT_RENDER_THREAD
   
   mpPatchEdgeDynamicIB = NULL;
};
//============================================================================
// BTerrainVisual::loadEffect
//============================================================================

void BTerrainVisual::loadEffect(void)
{
}   
//============================================================================
// BTerrainVisual::initEffectConstants
//============================================================================
void BTerrainVisual::initEffectConstants(void)
{  
   ASSERT_RENDER_THREAD

}
//============================================================================
// BTerrainVisual::receiveEvent
//============================================================================
bool BTerrainVisual::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_RENDER_THREAD

   return false;   
}



//============================================================================
// BTerrainVisual::calcWorkingPatchTess
//============================================================================
void BTerrainVisual::calcWorkingPatchTess(uint minXPatch, uint minZPatch,uint maxXPatch, uint maxZPatch,uint mNumZPatches,
                                                byte* pMaxPatchTessallation,XMVECTOR* pPatchBBs,float* pWorkingPatchTess, 
                                                XMMATRIX worldToScreen,XMVECTOR camPos,XMMATRIX ident)
{

  BDynamicRenderArray<float> mTempPatchAreas;
  mTempPatchAreas.resize((mNumZPatches));
   static const float maxPatchTessallation[]={15.0f,7.0f,3.0f,1.0f}; //CLM - the values of mpMaxPatchTessallation, written to disk, reference this array
   static const float desiredPT[] = {1.0f/100.0f,1.0f/500.f,1.0f/800.0f,1.0f/1000.0f };   //CLM - change these to whatever depending on your visual quality.

   //CLM early out here.. sometimes we get bogus data while the scenario is reloading??
   if(pPatchBBs==NULL || minXPatch > maxXPatch || minZPatch > maxZPatch || mNumZPatches ==0 )
      return;

#ifndef BUILD_FINAL
   //if (mLODEnabled) 
   //{
#endif

   int qInd = (minXPatch) + (minZPatch*mNumZPatches);

   /* char buf[256];
   sprintf_s(buf, sizeof(buf), "T %i \n", pWorkEntry->mMinZPatch);
   gConsoleOutput.output(cMsgError, "%s", buf);*/

   mTempPatchAreas.setAll(999999.0f);

   Utils::BPrefetchState prefetch = Utils::BeginPrefetch(pPatchBBs, 3);

   for (uint x = minXPatch; x < maxXPatch; x++)
   {
      Utils::TouchCacheLine(0, &pMaxPatchTessallation[qInd]);

      const int savedQInd = qInd;

      for (uint z = minZPatch; z < maxZPatch; z++)
      {

         prefetch = Utils::UpdatePrefetch(prefetch, &pPatchBBs[2 * qInd], 3);

         XMVECTOR min = pPatchBBs[2 * qInd];
         XMVECTOR max = pPatchBBs[2 * qInd + 1];

         BVMXIntersection::calculateBoxArea(mTempPatchAreas[z], camPos, min, max, ident, worldToScreen);

         qInd++;
      }

      qInd = savedQInd;

      // FastMemSet() will use dcbz128 to clear the cache lines of the dest. buffer, avoiding the reads from main memory.
      //   Utils::FastMemSet(&pWorkingPatchTess[qInd], 0, mNumZPatches * sizeof(pWorkingPatchTess[0]));

      
      for (uint z = minZPatch; z < maxZPatch; z++)
      {
         const int radDefInd = pMaxPatchTessallation[qInd];

         if(mTempPatchAreas[z]<300)
            pWorkingPatchTess[qInd]=1.0f;
         else
            pWorkingPatchTess[qInd] = (float)Math::fSelectClamp(mTempPatchAreas[z] * desiredPT[radDefInd], 1.0f, maxPatchTessallation[radDefInd]);   
         mTempPatchAreas[z] = 999999.0f;

         qInd++;
      }
   }

#ifndef BUILD_FINAL
   //}
   //else
   //{
   //   const int NumQuads = mNumXPatches * mNumZPatches;
   //   for(int i = 0; i < NumQuads; i++)
   //      mpWorkingPatchTess[i] = mRefinementEnabled ? maxPatchTessallation[mpMaxPatchTessallation[i]] : 15.0f;
   //}
#endif

   mTempPatchAreas.clear();
}

//============================================================================
// BTerrainVisual::calcPatchEdgeTess
//============================================================================
void BTerrainVisual::calcPatchEdgeTess()
{
   SCOPEDSAMPLE(BTerrainVisual_calcPatchEdgeTess)

   if(mpWorkingPatchTess==NULL)
      return;

   //Do our final gathering (joining) function here.
   //now that we've got the working volumes
   const int NumQuads = mNumXPatches * mNumZPatches;
   const int numXChunks =gTerrain.getNumXChunks();

   static const int offs[] = {0, -1, 1, 0, 0, 1, -1, 0};
   const int patchChunkStride = numXChunks * 16;
   //clm premultiply this by 4 to take into account the 4 edges per patch listing
 //const uint  pPatchTessOffsetInds[]={0,1,4,5,    2,3,6,7,    8,9,12,13,     10,11,14,15};
   static const uint  pPatchTessOffsetInds[]={0,4,16,20,  8,12,24,28, 32,36,48,52,   40,44,56,60};

   int xChunk = 0;
   int chunkXOff = 0;
   int yChunk = 0;
   int chunkYOff = 0;

   float* pOutDat = (float*)gRenderDraw.lockDynamicIB(NumQuads * 4 * 4, D3DFMT_INDEX32);   

   Utils::BPrefetchState prefetch = Utils::BeginPrefetch(pOutDat, 3);
   Utils::BPrefetchState prefetch2 = Utils::BeginPrefetch(&mMulByXPatchesLookup, 4);
   for (int x = 0; x < mNumXPatches; x++)
   {
      uint srcPatchIndex = x;

      for (int z = 0; z < mNumZPatches; z++)
      {
         //CLM This process has to match what our vertex shader is expecting to read so swizzle it by 4x4 chunks.

        /* int xContrib = ((xChunk * 16) + (chunkYOff * 4));        
         int yContrib = (patchChunkStride * yChunk) + chunkXOff; 
         int dstIndex = 4 * (xContrib + yContrib);*/


         //CLM This process has to match what our vertex shader is expecting to read so swizzle it by 2x2 x 2x2 chunks.
         int dstIndex = ((xChunk * 16) + (patchChunkStride * yChunk)) << 2;
         uint localIndex = chunkYOff * 4 + chunkXOff;
         dstIndex+= pPatchTessOffsetInds[localIndex];//<<2;

         float a = mpWorkingPatchTess[srcPatchIndex];
         srcPatchIndex += mNumXPatches;

         int jC =0;
         int jQ = dstIndex;
         int nK =0;
         for(int j = 0; j < 4; j++)
         {
            int nx = x + offs[jC];
            int ny = z + offs[jC + 1];

            prefetch = Utils::UpdatePrefetch(prefetch, &pOutDat[jQ], 3);

            
            if ( (((uint)nx) >= ((uint)mNumXPatches)) || (((uint)ny) >= ((uint)mNumZPatches)))
            {
               pOutDat[jQ] = a;
            }
            else
            {
               nK = nx + mMulByXPatchesLookup[ny];
               prefetch2 = Utils::UpdatePrefetch(prefetch2, &mpWorkingPatchTess[nK], 4);
               float b = mpWorkingPatchTess[nK];      

             

               //pOutDat[jQ] = Math::Max<float>(a, b); 
               pOutDat[jQ] = static_cast<float>(__fsel((a)-(b), a,b));

            }

            jC+=2;
            jQ++;
         }

         yChunk = (z + 1) >> 2;
         chunkYOff = (z + 1) & 3;
      }

      xChunk = (x + 1) >> 2;
      chunkXOff = (x + 1) & 3;

      yChunk = 0;
      chunkYOff = 0;
   }
   
   mpPatchEdgeDynamicIB = gRenderDraw.getDynamicIB();

   gRenderDraw.unlockDynamicIB();
}
//----------------------------------------
//----------------------------------------
//----------------------------------------
struct WorkThreadData
{
   uint mMinXPatch;
   uint mMaxXPatch;

   uint mMinZPatch;
   uint mMaxZPatch;
};

struct ThreadDataSnapshot
{
   uint mNumXPatches;
   uint mNumZPatches;

   byte     *mpMaxPatchTessallation;
   float    *mpWorkingPatchTess;
   XMVECTOR *mpPatchBBs;

   XMMATRIX mWorldToScreen;
   XMVECTOR mCamPos;
   XMMATRIX mIdent;

};
BDynamicArray<WorkThreadData, 4> mWorkEntries;

static BCountDownEvent mRemainingBuckets;
const int cTessBytesPerMaxTessPatch =1;
ThreadDataSnapshot mThreadDataSnapshot;

//============================================================================
// tessWorkerThreadCallback
//============================================================================
void BTerrainVisual::tessWorkerThreadCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   SCOPEDSAMPLE(tessWorkerThreadCallback);

//-- FIXING PREFIX BUG ID 6894
   const WorkThreadData* pWorkEntry = static_cast<WorkThreadData*>(privateData0);
//--
   ThreadDataSnapshot* pThreadedData = reinterpret_cast<ThreadDataSnapshot*>(privateData1);   

   //uint mNumXPatches = pThreadedData->mNumXPatches;
   uint mNumZPatches = pThreadedData->mNumZPatches;
   byte* pMaxPatchTessallation = pThreadedData->mpMaxPatchTessallation;
   XMVECTOR* pPatchBBs = pThreadedData->mpPatchBBs;
   float* pWorkingPatchTess = pThreadedData->mpWorkingPatchTess;

   BTerrainVisual::calcWorkingPatchTess(pWorkEntry->mMinXPatch, pWorkEntry->mMinZPatch, pWorkEntry->mMaxXPatch, pWorkEntry->mMaxZPatch, mNumZPatches, pMaxPatchTessallation, pPatchBBs, pWorkingPatchTess,
      pThreadedData->mWorldToScreen, pThreadedData->mCamPos,pThreadedData->mIdent);

   if (lastWorkEntryInBucket)
      mRemainingBuckets.decrement();
}

//============================================================================
// BTerrainVisual::tessFlushBegin
//============================================================================
void BTerrainVisual::beginEdgeTesselationCalc()
{
   if (!gTerrain.getLoadSuccessful())
      return;
   
   ASSERT_THREAD(cThreadIndexRender);

   SCOPEDSAMPLE(beginEdgeTesselationCalc);

   //if AA is disabled, then do immediate mode..
   if(!gTiledAAManager.getTilingEnabled())
   {
     
      calcWorkingPatchTess(0,0,mNumXPatches,mNumZPatches,mNumZPatches,mpMaxPatchTessallation,mpPatchBBs,mpWorkingPatchTess,
                           gRenderDraw.getWorkerActiveMatrixTracker().getMatrixByValue(cMTWorldToProj,false) * gRenderDraw.getWorkerActiveMatrixTracker().getMatrixByValue(cMTProjToScreen,false) ,
                           gRenderDraw.getWorkerActiveMatrixTracker().getWorldCamPos(),
                           XMMatrixIdentity());
      calcPatchEdgeTess();
      mWorkEntries.clear();
      return;
   }
   //Issue 1/4th the number of work entries
   //with each work entry doing 4 rows.

   uint totalNumWorkEntries = mNumZPatches>>2;


   uint cNumWorkEntriesPerBucketLog2 = 4;

   if (totalNumWorkEntries <= 16)           cNumWorkEntriesPerBucketLog2 = 1;
   else if (totalNumWorkEntries <= 32)      cNumWorkEntriesPerBucketLog2 = 2;
   else if (totalNumWorkEntries <= 64)      cNumWorkEntriesPerBucketLog2 = 3;

   uint cNumWorkEntriesPerBucket = 1U << cNumWorkEntriesPerBucketLog2;

   gWorkDistributor.flush();
   mWorkEntries.resize(totalNumWorkEntries);
   BDEBUG_ASSERT(cNumWorkEntriesPerBucket <= gWorkDistributor.getWorkEntryBucketSize());  
   uint totalBuckets = (totalNumWorkEntries + cNumWorkEntriesPerBucket - 1) >> cNumWorkEntriesPerBucketLog2;//totalNumWorkEntries / cNumWorkEntriesPerBucket;
   mRemainingBuckets.set(totalBuckets);

   mThreadDataSnapshot.mNumXPatches             = mNumXPatches;
   mThreadDataSnapshot.mNumZPatches             = mNumZPatches;
   mThreadDataSnapshot.mpMaxPatchTessallation   = mpMaxPatchTessallation;
   mThreadDataSnapshot.mpPatchBBs               = mpPatchBBs;
   mThreadDataSnapshot.mpWorkingPatchTess       = mpWorkingPatchTess;
   mThreadDataSnapshot.mWorldToScreen           = gRenderDraw.getWorkerActiveMatrixTracker().getMatrixByValue(cMTWorldToProj,false) * gRenderDraw.getWorkerActiveMatrixTracker().getMatrixByValue(cMTProjToScreen,false) ;
   mThreadDataSnapshot.mCamPos                  = gRenderDraw.getWorkerActiveMatrixTracker().getWorldCamPos();
   mThreadDataSnapshot.mIdent                   = XMMatrixIdentity();
   uchar* RESTRICT pThreadedDataSnapshot        = reinterpret_cast<uchar*>(&mThreadDataSnapshot);



   WorkThreadData* pNextWorkEntry = mWorkEntries.getPtr();  //so we'll be filling our work entries.


   //fill our frame storage, and issue off our worker commands
   uint numBucketWorkEntries = 0;
   {
      for(uint zWorkChunk =0; zWorkChunk < (uint)(mNumZPatches); zWorkChunk+=4)
      {
         //for now, each work is a full scanline of the grid
         WorkThreadData* currEntry = pNextWorkEntry;
         currEntry->mMinXPatch = 0 ;
         currEntry->mMaxXPatch = mNumXPatches;
         currEntry->mMinZPatch = zWorkChunk;
         currEntry->mMaxZPatch = zWorkChunk +4;

         //queue up the work
         gWorkDistributor.queue(tessWorkerThreadCallback, currEntry,(uint64)((uint32)pThreadedDataSnapshot));/* (((uint64)grannyBoneCount) << 32U) | (uint64)((uint32)pThreadedDataSnapshot));*/


         //handle bucket management
         numBucketWorkEntries++;
         if (numBucketWorkEntries == cNumWorkEntriesPerBucket)
         {
            numBucketWorkEntries = 0;
            gWorkDistributor.flush();
         }

         //increment our frame storage pointers
         pNextWorkEntry++;
      }
   }

   BDEBUG_ASSERT((pNextWorkEntry - mWorkEntries.getPtr()) <= static_cast<int>(mWorkEntries.getSize()));

   //start us rolling..
   gWorkDistributor.flush();
}


//============================================================================
// BTerrainVisual::tessFlushEnd
//============================================================================
void BTerrainVisual::endEdgeTesselationCalc()
{
   ASSERT_THREAD(cThreadIndexRender);

   SCOPEDSAMPLE(endEdgeTesselationCalc);

   if(!mWorkEntries.empty())
   {
      gWorkDistributor.waitSingle(mRemainingBuckets);

    //  gConsoleOutput.output(cMsgError, "TessDataFlushed");
      calcPatchEdgeTess();
   }
}




//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------

//============================================================================
// BTerrain3DSimpleVisualPacket::convertFromMemory
//============================================================================
bool BTerrain3DSimpleVisualPacket::convertFromMemoryMain()
{
   ASSERT_RENDER_THREAD

   SCOPEDSAMPLE(BTerrain3DSimpleVisualPacket_convertFromMemoryMain)
      //mType = c3DChunkPacket;

   UINT imgSize =0;
   unsigned char *mDevicePtrCounter = (unsigned char *)mpPhysicalMemoryPointer;

    

   int numxVerts = gTerrainVisual.getNumXVerts();
  
   //YAXIS   
   if(mpPhysicalMemoryPointer)
   {
      mTerrainPositions = new D3DTexture();
      imgSize = XGSetTextureHeader(numxVerts,numxVerts,1,0,D3DFMT_R11G11B10,0,0,0,0,mTerrainPositions,NULL,NULL);
      XGOffsetResourceAddress( mTerrainPositions, mDevicePtrCounter ); 
      mDevicePtrCounter+=imgSize;
      mPositionsSize=imgSize;

      mTerrainNormals = new D3DTexture();
      imgSize = XGSetTextureHeader(numxVerts,numxVerts,1,0,D3DFMT_R11G11B10,0,0,0,0,mTerrainNormals,NULL,NULL);
      XGOffsetResourceAddress( mTerrainNormals, mDevicePtrCounter); 
      mDevicePtrCounter+=imgSize;
      mNormalsSize=imgSize;
   }

   if(mpPhysicalMemoryPointerAO)
   {
      mTerrainAO = new D3DTexture();
      imgSize = XGSetTextureHeader(numxVerts,numxVerts,1,0,D3DFMT_LIN_DXT5A,0,0,0,0,mTerrainAO,NULL,NULL);
      XGOffsetResourceAddress( mTerrainAO, mpPhysicalMemoryPointerAO); 
      mTerrainAOSize=imgSize;
   }
   

  if(mpPhysicalMemoryPointerALPHA)
  {
      mTerrainAlpha = new D3DTexture();
      imgSize = XGSetTextureHeader(numxVerts,numxVerts,1,0,D3DFMT_LIN_DXT5A,0,0,0,0,mTerrainAlpha,NULL,NULL);
      XGOffsetResourceAddress( mTerrainAlpha, mpPhysicalMemoryPointerALPHA); 
      mTerrainAlphaSize=imgSize;
  }


 
/*
   BD3D::mpDev->CreateTexture(numxVerts,numxVerts,1,0,D3DFMT_LIN_R11G11B10,0,&mTerrainPositions,0);   
   mTerrainPositions->LockRect(0,&rect,0,0);
   DWORD *vals3 =(DWORD*)rect.pBits;
   for(int i=0;i<numxVerts*numxVerts;i++)
   {
      float val = -5+(10*(i / float(numxVerts*numxVerts)));
      vals3[i]=gTerrainDeformer.packPosToVisualFmt(0,val,0);
   }
   mTerrainPositions->UnlockRect(0);


   D3DLOCKED_RECT rect;
   BD3D::mpDev->CreateTexture(numxVerts,numxVerts,1,0,D3DFMT_LIN_A2R10G10B10,0,&mTerrainNormals,0);
   mTerrainNormals->LockRect(0,&rect,0,0);
   DWORD *vals4 =(DWORD*)rect.pBits;
   for(int i=0;i<numxVerts*numxVerts;i++)
   {
      vals4[i]=gTerrainDeformer.packNormalToVisualFMT(0,1,0,1);
   }
   mTerrainNormals->UnlockRect(0);
*/
   return true;

}

//============================================================================
// BTerrain3DSimpleVisualPacket::freeDeviceData
//============================================================================
void BTerrain3DSimpleVisualPacket::freeDeviceData()
{
   ASSERT_THREAD(cThreadIndexRender);

   //  ASSERT_RENDER_THREAD

   if(mTerrainPositions)
   {
      delete mTerrainPositions;
      mTerrainPositions=NULL;
   }

   if(mTerrainNormals) 
   {
      delete mTerrainNormals;
      mTerrainNormals=NULL;
   }
   if(mTerrainAO)
   {
      delete mTerrainAO;
      mTerrainAO=NULL;
   }
   if(mTerrainAlpha)
   {
      delete mTerrainAlpha;
      mTerrainAlpha=NULL;
   }
}


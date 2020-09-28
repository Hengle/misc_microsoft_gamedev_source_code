//============================================================================
//
//  Terrain.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "TerrainPCH.h"
 
// xcore
#include "threading\workDistributor.h"

// xrender
#include "renderThread.h"
#include "renderDraw.h"
#include "volumeCuller.h"
#include "vertexTypes.h"
#include "fixedFuncShaders.h"
#include "primDraw2D.h"

// terrain
#include "terrain.h"
#include "TerrainVisual.h"
#include "TerrainRender.h"
#include "TerrainQuadNode.h"
#include "TerrainMetric.h"
#include "TerrainSimRep.h"
#include "TerrainIO.h"
#include "TerrainTexturing.h"
#include "TerrainDeformer.h"
#include "TerrainHeightField.h"
#include "TerrainLitDecals.h"
#include "TerrainRibbon.h"

// terrain special effects
#include "RoadManager.h"
#include "TerrainFoliage.h"
#include "TerrainDynamicAlpha.h"

// xgameRender 
#include "..\xgameRender\tiledAA.h"
#include "..\xgameRender\occlusion.h"
#include "configsgamerender.h"

//============================================================================
// Globals
//============================================================================

BTerrain gTerrain;

eUGXGeomVisMode BTerrain::mVisMode=cVMDisabled;
eUGXGeomTextureMode BTerrain::mTextureMode=cTMNormal;

static BCountDownEvent mTerrainRemainingVisibilityBuckets;
static BCountDownEvent mTerrainRemainingShadowVisibilityBuckets;


bool sphereAABBIntersect(const D3DXVECTOR3 &aMin,const D3DXVECTOR3 &aMax, const BVector4 &packedSphere)
{
   // FROM GRAPHICS GEMS I - p.335
   float minSqDistance = 0.0f;

   if (packedSphere.x < aMin.x)
      minSqDistance += (packedSphere.x - aMin.x) * (packedSphere.x - aMin.x);
   else if (packedSphere.x > aMax.x)
      minSqDistance += (packedSphere.x - aMax.x) * (packedSphere.x - aMax.x);

   if (packedSphere.y < aMin.y)
      minSqDistance += (packedSphere.y - aMin.y) * (packedSphere.y - aMin.y);
   else if (packedSphere.y > aMax.y)
      minSqDistance += (packedSphere.y - aMax.y) * (packedSphere.y - aMax.y);

   if (packedSphere.z < aMin.z)
      minSqDistance += (packedSphere.z - aMin.z) * (packedSphere.z - aMin.z);
   else if (packedSphere.z > aMax.z)
      minSqDistance += (packedSphere.z - aMax.z) * (packedSphere.z - aMax.z);

   if (minSqDistance <= (packedSphere.w * packedSphere.w))
      return true;
   else
      return false;
}
//============================================================================
// BTerrain::BTerrain
//============================================================================
BTerrain::BTerrain():
   mpQuadGrid(0),
   mNumXChunks(0),
   mBoundingBoxGrowDist(0.0f),
   mLoadSuccessful(FALSE),
   mLoadPending(FALSE),
   mReloadPending(FALSE),
   mRenderQuadGridBBs(FALSE),
   mRenderVisGrid(FALSE),
   mRenderTextures(TRUE),
   mRenderRoads(TRUE),
   mRenderFoliage(TRUE),
   mRenderSkirt(TRUE),
   mNumBucketWorkEntries(0),
   cNumWorkEntriesPerBucket(1),
   mNumCurrWorkEntries(0),
   mEvalSceneNodesMTIssued(FALSE),
   mReflectionFadeoutDist(500.0f)
{
   mTerrainRemainingVisibilityBuckets.set(0);
   mTerrainRemainingShadowVisibilityBuckets.set(0);
#ifndef BUILD_FINAL
      mpMemoryReserve =0;
#endif

      mQuadSkirtChunkList.clear();
}

//============================================================================
// BTerrain::~BTerrain
//============================================================================
BTerrain::~BTerrain()
{
}

//============================================================================
// BTerrain::init
//============================================================================
bool BTerrain::init()
{
   ASSERT_THREAD(cThreadIndexSim);

   commandListenerInit();
   
   TRACEMEM

   gTerrainRender.init();
   
   TRACEMEM
   
   gTerrainVisual.init();
   
   TRACEMEM
   
   gTerrainTexturing.init();
   
   TRACEMEM

   gRoadManager.init();

   TRACEMEM

   gFoliageManager.init();

   TRACEMEM

   gTerrainDynamicAlpha.init();

   TRACEMEM

   gLitDecalManager.init();

   TRACEMEM

   gTerrainRibbonManager.init();

   TRACEMEM

   const uint cMaxExpectedNodes = 64 * 64;
   mAllUniqueNodes.reserve(cMaxExpectedNodes);
   mSceneNodes.reserve(cMaxExpectedNodes);
   for (uint i = 0; i < cMaxShadowNodeArrays; i++)
      mShadowNodes[i].reserve(cMaxExpectedNodes);

   mTempNodes.reserve(256);      
   
   mLoader.init();
   
   gTerrainHeightField.init();
   
   TRACEMEM

   return true;
}

//============================================================================
// BTerrain::deinit
//============================================================================
bool BTerrain::deinit()
{
   ASSERT_THREAD(cThreadIndexSim);

   // Block for safety. 
   gRenderThread.blockUntilGPUIdle();
   
   destroy();
   
   mLoader.deinit();

   commandListenerDeinit();

   // rg [2/1/06] - We must block until the worker thread (and GPU, for safety) are idle here, because the packets sent 
   // down to the terrain renderer have pointers to the terrain data!
   gRenderThread.blockUntilGPUIdle();

   gTerrainRender.deinit();         //should this be called from OUR deinit?
   gTerrainTexturing.deinit();
   gTerrainVisual.deinit(); 
   gRoadManager.deinit();
   gFoliageManager.deinit();
   gTerrainDynamicAlpha.deinit();
   gLitDecalManager.deinit();
   gTerrainRibbonManager.deinit();

   gTerrainHeightField.deinit();


   mAllUniqueNodes.clear();
   mSceneNodes.clear();
   for (uint i = 0; i < cMaxShadowNodeArrays; i++)
      mShadowNodes[i].clear();

   mTempNodes.clear();      

   return true;
}

//============================================================================
// BTerrain::destroy
//============================================================================
void BTerrain::destroy(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
  
   gRenderThread.submitCommand(mCommandHandle,cTECDestroy);
   gTerrainTexturing.destroy();
   gTerrainVisual.destroy();
   gRoadManager.destroy();
   gFoliageManager.destroy();
   gTerrainDynamicAlpha.destroy();
   gLitDecalManager.destroy();
   gTerrainRibbonManager.destroy();


   
   setLoadSuccessful(false);
   setLoadPending(false);
}

//============================================================================
// BTerrain::destroyInternal
//============================================================================
void BTerrain::destroyInternal()
{
   ASSERT_THREAD(cThreadIndexRender);


   if (mpQuadGrid)
   {
      for(int i=0;i<mNumXChunks*mNumXChunks;i++)
         mpQuadGrid[i].destroy();

      delete [] mpQuadGrid;
      mpQuadGrid=0;
   }

   mQuadSkirtChunkList.clear();

   for(uint k=0;k<4;k++)
   {
      mTileAAVisibleSkirtNodeInstances[k].clear();
      mTileAAVisibleNodeInstances[k].clear();
   }

   mShadowVisWorkEntries.clear();
   mExcludeObjects.clear();

   gTerrainHeightField.unload();

   clearExclusionObjectsInternal();
}

//============================================================================
// BTerrain::load
//============================================================================
bool BTerrain::load(long dirID, const char *filename, long terrainDirID, bool loadVisRep)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   TRACEMEM
   
   setLoadPending(true);
   setLoadSuccessful(false);
   
   mLoader.load(dirID, filename, terrainDirID, loadVisRep);
   
   gTerrainRender.clearBlackmapParams();
   
   TRACEMEM
   
   return true;
}


//============================================================================
// BTerrainQuadNodeDistanceSorter
//============================================================================
struct BTerrainQuadNodeDistanceSorter
{
   const XMVECTOR *mCamPos;
   BTerrainQuadNodeDistanceSorter(const XMVECTOR *camPos):mCamPos(camPos) {}

   bool operator() (const BTerrainQuadNode* rpStart,const  BTerrainQuadNode* rpEnd) const
   {
      const D3DXVECTOR3 centA = (rpStart->getDesc().m_max + rpStart->getDesc().m_min) * 0.5f;
      const D3DXVECTOR3 centB = (rpEnd->getDesc().m_max + rpEnd->getDesc().m_min) * 0.5f;

      float lDist2 = Math::Sqr(centA[0] - mCamPos->x) + Math::Sqr(centA[1] - mCamPos->y) + Math::Sqr(centA[2] - mCamPos->z);
      float rDist2 = Math::Sqr(centB[0] - mCamPos->x) + Math::Sqr(centB[1] - mCamPos->y) + Math::Sqr(centB[2] - mCamPos->z);

      return lDist2 < rDist2;

   }
}; 


//============================================================================
// BTerrain::getVisibleNodes
//============================================================================
void BTerrain::getVisibleNodes(BTerrainQuadNode::BGetVisibleNodesParams& state,const BDynamicArray<BTerrain::exclusionObject> *exclusionObjects)
{

   XMVECTOR xmEyePos = XMLoadVector4(&state.mEyePos);

   BTerrainQuadNodePtrArray nodes;
   nodes.reserve(1024);
   gTerrain.getAABBIntersection(nodes, state.mpVolumeCuller->getBaseMin(), state.mpVolumeCuller->getBaseMax(),xmEyePos, exclusionObjects, false, state.mIncludeSkirt);
      

   for(uint k = 0; k < nodes.getSize(); k++)
   {
      BTerrainQuadNode& node = *nodes[k];
      const BTerrainQuadNodeDesc& desc = node.getDesc();
      
      if(!state.mpVolumeCuller->isAABBVisible(desc.m_min,desc.m_max))
      {
        continue;
      }
      else
      {
         if (state.mFilterNonShadowCasters)
         {
            if(!desc.mCanCastShadows)
               continue;
         }

         if (state.mpAllUniqueNodes)
            state.mpAllUniqueNodes->pushBack(&node);

         if (state.mpNodes)
            state.mpNodes->push_back(&node);
      }
      
   }


   if(state.mpNodes!=NULL)
      if( state.mpNodes->size()>40)
         std::sort(state.mpNodes->begin(),state.mpNodes->end(), BTerrainQuadNodeDistanceSorter(&xmEyePos));

}

//============================================================================
// BTerrain::getAABBIntersection
//============================================================================

void BTerrain::getAABBIntersection(BTerrainQuadNodePtrArray &nodes, XMVECTOR min, XMVECTOR max, const XMVECTOR &camPos , const BDynamicArray<BTerrain::exclusionObject> *exclusionObjects, bool setNodeVisibility, bool includeSkirt)
{
   if (!mpQuadGrid)
      return;
      
   D3DXVECTOR3 bbMin(min.x, min.y, min.z);
   D3DXVECTOR3 bbMax(max.x, max.y, max.z);
   
   // Grow the bounding box to account for overhangs.
   bbMin.x -= mBoundingBoxGrowDist;
   bbMin.y -= mBoundingBoxGrowDist;
   bbMin.z -= mBoundingBoxGrowDist;
   bbMax.x += mBoundingBoxGrowDist;
   bbMax.y += mBoundingBoxGrowDist;
   bbMax.z += mBoundingBoxGrowDist;
 
   //get our area chunk bounds to intersect with
   int xMin=int((bbMin.x * gTerrainVisual.getOOTileScale()) * BTerrainQuadNode::getOOMaxNodeWidth());
   int xMax=int((bbMax.x * gTerrainVisual.getOOTileScale()) * BTerrainQuadNode::getOOMaxNodeWidth());
   int zMin=int((bbMin.z * gTerrainVisual.getOOTileScale()) * BTerrainQuadNode::getOOMaxNodeDepth());
   int zMax=int((bbMax.z * gTerrainVisual.getOOTileScale()) * BTerrainQuadNode::getOOMaxNodeDepth());
   
   if ((xMax < 0) || (xMin >= mNumXChunks) ||
       (zMax < 0) || (zMin >= mNumXChunks))
   {
      if (setNodeVisibility)
      {
         for (int x = 0; x < mNumXChunks; x++)
         {
            const int outerIndx = x * mNumXChunks;
            for (int z = 0; z < mNumXChunks; z++)
            {
               int indx = outerIndx + z;
               mpQuadGrid[indx].updateFromVisibility(false,camPos);
            }
         }
      }         
      
      if((mRenderSkirt) && (includeSkirt))
      {
         getAABBIntersectionSkirts(nodes, min,max,false);
      }

      return;
   }       
   
   xMin = Math::Clamp(xMin, 0, mNumXChunks - 1);
   xMax = Math::Clamp(xMax, 0, mNumXChunks - 1);
   zMin = Math::Clamp(zMin, 0, mNumXChunks - 1);
   zMax = Math::Clamp(zMax, 0, mNumXChunks - 1);
   
   if (xMax < xMin) std::swap(xMax, xMin);
   if (zMax < zMin) std::swap(zMax, zMin);

   // rg [6/13/06] - Use VMX here, XMVECTOR, etc. here 
//   BOOL visible=true;
   for (int x=xMin;x<=xMax;x++)
   {
      int outerIndx = x * mNumXChunks;
      for (int z=zMin;z<=zMax;z++)
      {
         int indx = outerIndx + z;
         
         const BTerrainQuadNodeDesc& desc = mpQuadGrid[indx].getDesc();
         if(aabbsIntersect(desc.m_min,desc.m_max,bbMin,bbMax))
         {
             bool excluded = false;
            if(exclusionObjects)
            {
               for(uint i=0;i<exclusionObjects->size();i++)
               {
                  if((*exclusionObjects)[i].mExclusionObjectType==0)
                  {
                     if(sphereAABBIntersect(desc.m_min,desc.m_max,(*exclusionObjects)[i].mData0))
                     {
                        excluded=true;
                        break;
                     }
                  }
                  else if((*exclusionObjects)[i].mExclusionObjectType==1)
                  {

                     if(aabbsIntersect(desc.m_min,desc.m_max,
                        D3DXVECTOR3((*exclusionObjects)[i].mData0.x, (*exclusionObjects)[i].mData0.y,(*exclusionObjects)[i].mData0.z),
                        D3DXVECTOR3((*exclusionObjects)[i].mData1.x, (*exclusionObjects)[i].mData1.y,(*exclusionObjects)[i].mData1.z)))
                     {

                        excluded=true;
                        break;
                     }
                  }
               }
            }
            if(!excluded)
            {
               nodes.push_back(&mpQuadGrid[indx]);
            }
         }
         else
         {
            if(setNodeVisibility)
               mpQuadGrid[indx].updateFromVisibility(false,camPos);
         }
      }
   }

   //CLM - nodes NOT in the frustum need to be marked.
   if(setNodeVisibility)
   {
      // We need to set everything else to visible false..
      
      // Leftmost column
      for (int x = 0; x < xMin; x++)
      {
         const int outerIndx = x * mNumXChunks;
         for (int z = 0; z < mNumXChunks; z++)
         {
            int indx = outerIndx + z;
            mpQuadGrid[indx].updateFromVisibility(false,camPos);
         }
      }
      
      // Top/bottom portions
      for (int x = xMin; x <= xMax; x++)
      {
         const int outerIndx = x * mNumXChunks;

         for (int z = 0; z < zMin; z++)
         {
            int indx = outerIndx + z;
            mpQuadGrid[indx].updateFromVisibility(false,camPos);
         }

         for (int z = zMax + 1; z < mNumXChunks; z++)
         {
            int indx = outerIndx + z;
            mpQuadGrid[indx].updateFromVisibility(false,camPos);
         }
      }
      
      // Rightmost column
      for (int x = xMax + 1; x < mNumXChunks; x++)
      {
         const int outerIndx = x * mNumXChunks;
         
         for (int z = 0; z < mNumXChunks; z++)
         {
            int indx = outerIndx + z;
            mpQuadGrid[indx].updateFromVisibility(false,camPos);
         }
      }
   }


  
   if((mRenderSkirt) && (includeSkirt))
   {
      getAABBIntersectionSkirts(nodes, min,max,false);
   }
}






//============================================================================
// BTerrain::getAABBIntersectionBig
//============================================================================
void BTerrain::getAABBIntersectionSkirts(BTerrainQuadNodePtrArray &nodes, XMVECTOR min, XMVECTOR max, bool invertTest)
{

   if(mQuadSkirtChunkList.size()!=0)
   {
      for(uint k=0;k<mQuadSkirtChunkList.size();k++)
      {
         nodes.push_back(&mQuadSkirtChunkList[k]);
      }
   }
 
}



//============================================================================
// BTerrain::evalSceneNodesLOD
//============================================================================
void BTerrain::evalSceneNodesLODImmediate(eRenderPass shadowPassIndex, const BVec4& sceneEyePos, const BVolumeCuller& volumeCuller, bool includeSkirt)
{
   evalSceneNodesLOD(shadowPassIndex, sceneEyePos, volumeCuller,  includeSkirt, false);
}
void BTerrain::evalSceneNodesLODDeferred(eRenderPass shadowPassIndex, const BVec4& sceneEyePos, const BVolumeCuller& volumeCuller, bool includeSkirt)
{
   evalSceneNodesLOD(shadowPassIndex, sceneEyePos, volumeCuller,  includeSkirt, true);
}

void BTerrain::evalSceneNodesLOD(eRenderPass shadowPassIndex, const BVec4& sceneEyePos, const BVolumeCuller& volumeCuller, bool includeSkirt, bool useWorkerPool)
{
   ASSERT_THREAD(cThreadIndexRender);

   // rg [9/18/06] - NEVER render anything inside here! D3D::BeginScene() has not been called yet, rendering stuff here may deadlock the GPU!

   if (!mLoadSuccessful)
      return;
   
   if(!gTiledAAManager.getTilingEnabled())
      useWorkerPool = false;     //CLM we can't trust that the gTileAAManager won't be accessed on other threads...

   SCOPEDSAMPLE(BTerrain_evalSceneNodesLOD);
   
   BTerrainQuadNode::BGetVisibleNodesParams getVisParams;
   
   getVisParams.mEyePos             = sceneEyePos;
   getVisParams.mpVolumeCuller      = &volumeCuller;
   getVisParams.mpAllUniqueNodes    = &mAllUniqueNodes;
   getVisParams.mFilterNonShadowCasters = false;
   getVisParams.mIncludeSkirt       = includeSkirt;
         
   switch (shadowPassIndex)
   {
      case cRPVisible:
      {
         mAllUniqueNodes.resize(0);
         mSceneNodes.resize(0);
         for (uint i = 0; i < cMaxShadowNodeArrays; i++)
            mShadowNodes[i].resize(0);
         
         getVisParams.mpNodes = &mSceneNodes;
         

         XMVECTOR xmEyePos = XMLoadVector4(&sceneEyePos);
         if(useWorkerPool)
            queueEvalSceneNodesVisibility(gRenderDraw.getWorkerActiveVolumeCuller(),xmEyePos);
         else
            evalSceneNodesVisibility(gRenderDraw.getWorkerActiveVolumeCuller(),xmEyePos, &mExcludeObjects);    //find nodes visible to the scene (and some other update stuff
         
         break;
      }
      case cRPDirShadowBuffer0:
      case cRPDirShadowBuffer1:
      case cRPDirShadowBuffer2:
      case cRPDirShadowBuffer3:
      case cRPDirShadowBuffer4:
      case cRPDirShadowBuffer5:
      case cRPDirShadowBuffer6:
      case cRPDirShadowBuffer7:
      {
         BDEBUG_ASSERT((shadowPassIndex >= 0) && (shadowPassIndex < cMaxShadowNodeArrays));
         getVisParams.mpNodes = &mShadowNodes[shadowPassIndex];
                        
         BDEBUG_ASSERT(getVisParams.mpNodes->empty());
       
         // Find this shadow buffer's visible nodes.
         if(useWorkerPool)
            queueGetVisibleShadowNodes(getVisParams);
         else
            getVisibleNodes(getVisParams,&mExcludeObjects);
         
         break;
      }
      case cRPLocalShadowBuffer:
      {
         getVisParams.mpNodes = NULL;
                  
         getVisParams.mFilterNonShadowCasters = true;
         
         // Find this shadow buffer's visible nodes.
         if(useWorkerPool)
            queueGetVisibleShadowNodes(getVisParams);
         else
            getVisibleNodes(getVisParams,&mExcludeObjects);
          
         
         break;
      }
      default:
      {
         BDEBUG_ASSERT(0);
      }
   }      
}
//============================================================================
// BTerrain::evalSceneNodesVisibility
//============================================================================
void BTerrain::evalSceneNodesVisibility(const BVolumeCuller& sceneCuller, const XMVECTOR &camPos,const BDynamicArray<BTerrain::exclusionObject> *exclusionObjects)
{
   if (!gTerrain.mLoadSuccessful || gTerrain.mpQuadGrid == NULL)
      return;

   SCOPEDSAMPLE(BTerrain_evalSceneNodesVisibility);

   const uint  maxTiles= gTiledAAManager.getNumTiles();
   for(uint k=0;k<maxTiles;k++)
   {
      gTerrain.mTileAAVisibleSkirtNodeInstances[k].clear();
      gTerrain.mTileAAVisibleNodeInstances[k].clear();
   }

   // INCREASE THE AGE OF ALL OUR QUADNODES
   for(int i=0;i<gTerrain.mNumXChunks*gTerrain.mNumXChunks;i++)
   {
      gTerrain.mpQuadGrid[i].increaseAge();
      gTerrain.mpQuadGrid[i].clearVisibility();
   }

   

   BTerrainQuadNodePtrArray nodeInstances;
   nodeInstances.reserve(1024);


   gTerrain.getAABBIntersection(nodeInstances, sceneCuller.getBaseMin(), sceneCuller.getBaseMax(),camPos, exclusionObjects, true);


#ifndef BUILD_FINAL
   int numVisTiles=0;
#endif
   for(uint k = 0; k < nodeInstances.getSize(); k++)
   {
      BTerrainQuadNode& nodeInstance = *nodeInstances[k];
      const BTerrainQuadNodeDesc& desc = nodeInstance.getDesc();   
      
      //node.mCacheInfo.mNumTilesVisible=0;
      int numTilesVisible = 0;

      for(uint i=0;i<maxTiles;i++)
      {
            nodeInstance.mVisibleInThisTile[i]=false;

         if(gTiledAAManager.getTileVolumeCuller(i).isAABBVisible(desc.m_min, desc.m_max))
         {
            //node.mCacheInfo.mNumTilesVisible++;
            numTilesVisible++;
            if(nodeInstance.mSkirtInfo.mIsSkirtChunk)
            {
               if(nodeInstance.mSkirtInfo.mQuadrant != -1)
               {
                  gTerrain.mTileAAVisibleSkirtNodeInstances[i].push_back(&nodeInstance);
               }
            }
            else
            {
               gTerrain.mTileAAVisibleNodeInstances[i].pushBack(&nodeInstance);
               nodeInstance.mVisibleInThisTile[i]=true;
            }
         }
      }

      // When dealing with a non-skirt quadnode cache some info
      if(!nodeInstance.mSkirtInfo.mIsSkirtChunk)
      {
         nodeInstance.mCacheInfo.mNumTilesVisible=numTilesVisible;

         if(nodeInstance.mCacheInfo.mNumTilesVisible)
         {
           

            nodeInstance.updateFromVisibility(true,camPos);
#ifndef BUILD_FINAL
            numVisTiles++;
#endif
         }
         else
         {
            nodeInstance.updateFromVisibility(false,camPos);
         }
      }


   } 

   //CLM [06.22.07] If we have a large number of nodes, attempt to sort them (chances are, we're getting some overdraw, or moving towards fps mode...
   //sorting them front-to-back helps, as we can save on depth buffering.
   for(uint i=0;i<maxTiles;i++)
   {
      if( gTerrain.mTileAAVisibleNodeInstances[i].size()>40)
         std::sort(gTerrain.mTileAAVisibleNodeInstances[i].begin(), gTerrain.mTileAAVisibleNodeInstances[i].end(), BTerrainQuadNodeDistanceSorter(&camPos));
   }


#ifndef BUILD_FINAL
   BTerrainMetrics::setNumVisibleNodes(numVisTiles);
   for(uint i=0;i<maxTiles;i++)
   {
      BTerrainMetrics::setQNVisInTile(gTerrain.mTileAAVisibleNodeInstances[i].size(),i);
   }
#endif

}

//============================================================================
// evalSceneNodesVisibilityWorkerCallback
//============================================================================
struct MainVisThreadPacket
{
   BVolumeCuller  mSceneCuller;
   XMVECTOR       mCamPos;    
   BDynamicArray<BTerrain::exclusionObject> mExclusionArray;
   uint           mIntrinsicStateSnapshotSize;
   const void*    mpIntrinsicStateSnapshot;
};

MainVisThreadPacket mTempVisThreadPacket;
void BTerrain::evalSceneNodesVisibilityWorkerCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{ 
//-- FIXING PREFIX BUG ID 6915
   const MainVisThreadPacket* bvc = static_cast<MainVisThreadPacket*>(privateData0);
//--
   BTerrain::evalSceneNodesVisibility(bvc->mSceneCuller, bvc->mCamPos, &bvc->mExclusionArray);

   if (lastWorkEntryInBucket)
      mTerrainRemainingVisibilityBuckets.decrement();
}
//============================================================================
// BTerrain::queueEvalSceneNodesVisibility
//============================================================================
void BTerrain::queueEvalSceneNodesVisibility(const BVolumeCuller& volumeCuller,const XMVECTOR &eyePos)
{
   ASSERT_THREAD(cThreadIndexRender);

   SCOPEDSAMPLE(BTerrain_queueEvalSceneNodesVisibility);


   //this is such a heavy function we do it in its' own bucket..
   gWorkDistributor.flush();

   mTerrainRemainingVisibilityBuckets.set(1);
   
   mTempVisThreadPacket.mCamPos = eyePos;
   memcpy(&mTempVisThreadPacket.mSceneCuller,&volumeCuller,sizeof(BVolumeCuller));

   mTempVisThreadPacket.mExclusionArray.clear();
   for(uint j=0;j<mExcludeObjects.size();j++)
      mTempVisThreadPacket.mExclusionArray.push_back(mExcludeObjects[j]);
      
   gWorkDistributor.queue(evalSceneNodesVisibilityWorkerCallback, &mTempVisThreadPacket,0);

   gWorkDistributor.flush();
   mEvalSceneNodesMTIssued=true;

}



//============================================================================
// BTerrain::beginBatchShadowVis
//============================================================================
void BTerrain::beginBatchShadowVis(int totalNumWorkEntries)
{
   ASSERT_THREAD(cThreadIndexRender);

   SCOPEDSAMPLE(BTerrain_beginBatchShadowVis);

   if(!gTiledAAManager.getTilingEnabled())
   {
      mShadowVisWorkEntries.clear();
      return;
   }

   uint cNumWorkEntriesPerBucketLog2 = 4;

   if (totalNumWorkEntries <= 16)           cNumWorkEntriesPerBucketLog2 = 1;
   else if (totalNumWorkEntries <= 32)      cNumWorkEntriesPerBucketLog2 = 2;
   else if (totalNumWorkEntries <= 64)      cNumWorkEntriesPerBucketLog2 = 3;

   cNumWorkEntriesPerBucket = 1U << cNumWorkEntriesPerBucketLog2;

   gWorkDistributor.flush();
   mShadowVisWorkEntries.resize(totalNumWorkEntries);

   BDEBUG_ASSERT(cNumWorkEntriesPerBucket <= gWorkDistributor.getWorkEntryBucketSize());  
   uint totalBuckets = (totalNumWorkEntries + cNumWorkEntriesPerBucket - 1) >> cNumWorkEntriesPerBucketLog2;
   mTerrainRemainingShadowVisibilityBuckets.set(totalBuckets);

   mNumBucketWorkEntries=0;
   mNumCurrWorkEntries=0;
}
//============================================================================
// getVisibleShadowNodesWorkerCallback
//============================================================================
void BTerrain::getVisibleShadowNodesWorkerCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   SCOPEDSAMPLE(getVisibleShadowNodesWorkerCallback);
   ShadowVisThreadPacket* pWorkEntry = static_cast<ShadowVisThreadPacket*>(privateData0);

   BTerrain::getVisibleNodes(pWorkEntry->mVisibleNodeParams,&pWorkEntry->mExclusionArray);

   if (lastWorkEntryInBucket)
      mTerrainRemainingShadowVisibilityBuckets.decrement();
}
//============================================================================
// BTerrain::queueGetVisibleShadowNodes
//============================================================================
void BTerrain::queueGetVisibleShadowNodes(BTerrainQuadNode::BGetVisibleNodesParams parms)
{
   SCOPEDSAMPLE(BTerrain_queueGetVisibleShadowNodes);

   memcpy(&mShadowVisWorkEntries[mNumCurrWorkEntries].mVisibleNodeParams, &parms, sizeof(BTerrainQuadNode::BGetVisibleNodesParams));
   mShadowVisWorkEntries[mNumCurrWorkEntries].mExclusionArray.clear();
   for(uint j=0;j<mExcludeObjects.size();j++)
      mShadowVisWorkEntries[mNumCurrWorkEntries].mExclusionArray.push_back(mExcludeObjects[j]);

   gWorkDistributor.queue(getVisibleShadowNodesWorkerCallback, &mShadowVisWorkEntries[mNumCurrWorkEntries],0);

   mNumCurrWorkEntries++;
   mNumBucketWorkEntries++;
   if (mNumBucketWorkEntries == cNumWorkEntriesPerBucket || mNumCurrWorkEntries >= mShadowVisWorkEntries.size())
   {
      mNumBucketWorkEntries = 0;
      gWorkDistributor.flush();
   }
}

//============================================================================
// BTerrain::joinBatchWork
//============================================================================
void BTerrain::joinBatchWork()
{
   if (!mLoadSuccessful)
      return;

   ASSERT_THREAD(cThreadIndexRender);

   SCOPEDSAMPLE(BTerrain_joinBatchWork);

   gTerrainVisual.endEdgeTesselationCalc();  //CLM not sure if this should be first to ensure we're not stalling here?

   if(mEvalSceneNodesMTIssued)
   {
      gWorkDistributor.waitSingle(mTerrainRemainingVisibilityBuckets);
      mEvalSceneNodesMTIssued=false;
   }

   if(!mShadowVisWorkEntries.empty())
      gWorkDistributor.waitSingle(mTerrainRemainingShadowVisibilityBuckets);

}
//============================================================================
// BTerrain::defragmentCaches
//============================================================================
void BTerrain::defragmentCaches(void)
{
   if (!mLoadSuccessful)
      return;
      
   gTerrainTexturing.defragmentCaches();
}


//============================================================================
// BTerrain::renderQuadGridBBs
//============================================================================
void BTerrain::renderQuadGridBBs()
{
   int numLines = 12;
   int numPointsPerChunk = numLines*2;
   int NumQuads = numLines;

   BD3D::mpDev->SetIndices(NULL);
   BD3D::mpDev->SetVertexDeclaration(BPVertex::msVertexDecl);

   // Set vertex declaration to match, disable any vertex shaders.
   BD3D::mpDev->SetVertexShader(gFixedFuncShaders.workerGetVertexShader(cPosVS));
   BD3D::mpDev->SetPixelShader(gFixedFuncShaders.workerGetPixelShader(cWhitePS));

   XMMATRIX worldToProj = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToProj, true);
   BD3D::mpDev->SetVertexShaderConstantF(0, (float*) worldToProj.m, 4);     


   int c=0;
   BTerrainQuadNodeDesc desc;
   for(int x=0;x<mNumXChunks;x++)
   {
      for(int z=0;z<mNumXChunks;z++)
      {
         desc = mpQuadGrid[c++].getDesc();
         int k = 0;
         BPVertex *outDat = (BPVertex*)gRenderDraw.lockDynamicVB(numPointsPerChunk , sizeof(BPVertex));
         outDat[k++].pos = D3DXVECTOR3(desc.m_min.x, desc.m_min.y,  desc.m_min.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_min.x, desc.m_min.y,  desc.m_max.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_min.x, desc.m_max.y,  desc.m_min.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_min.x, desc.m_max.y,  desc.m_max.z);

         outDat[k++].pos = D3DXVECTOR3(desc.m_min.x, desc.m_min.y,  desc.m_min.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_max.x, desc.m_min.y,  desc.m_min.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_min.x, desc.m_max.y,  desc.m_min.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_max.x, desc.m_max.y,  desc.m_min.z);

         outDat[k++].pos = D3DXVECTOR3(desc.m_max.x, desc.m_min.y,  desc.m_min.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_max.x, desc.m_min.y,  desc.m_max.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_max.x, desc.m_max.y,  desc.m_min.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_max.x, desc.m_max.y,  desc.m_max.z);

         outDat[k++].pos = D3DXVECTOR3(desc.m_min.x, desc.m_min.y,  desc.m_max.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_max.x, desc.m_min.y,  desc.m_max.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_min.x, desc.m_max.y,  desc.m_max.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_max.x, desc.m_max.y,  desc.m_max.z);


         outDat[k++].pos = D3DXVECTOR3(desc.m_min.x, desc.m_min.y,  desc.m_min.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_min.x, desc.m_max.y,  desc.m_min.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_max.x, desc.m_min.y,  desc.m_max.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_max.x, desc.m_max.y,  desc.m_max.z);
         
         outDat[k++].pos = D3DXVECTOR3(desc.m_max.x, desc.m_min.y,  desc.m_min.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_max.x, desc.m_max.y,  desc.m_min.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_min.x, desc.m_min.y,  desc.m_max.z);
         outDat[k++].pos = D3DXVECTOR3(desc.m_min.x, desc.m_max.y,  desc.m_max.z);


         //draw it...
         BD3D::mpDev->SetStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(BPVertex));

         gRenderDraw.unlockDynamicVB();

         BD3D::mpDev->DrawPrimitive(D3DPT_LINELIST,0,NumQuads);
        
      }
   }
}
//============================================================================
// BTerrain::compositeVisibleNodes
//============================================================================
void BTerrain::computeTileDCBs(int tileIndex)
{
}
//============================================================================
// BTerrain::compositeVisibleNodes
//============================================================================
void BTerrain::flushTileDCBs(int tileIndex)
{
}

//============================================================================
// BTerrain::compositeVisibleNodes
//============================================================================
void BTerrain::compositeVisibleNodes()
{
   SCOPEDSAMPLE(BTerrain_compositeVisibleNodes);

   if (!mLoadSuccessful)
      return;

   
   for (uint tileIndex = 0; tileIndex < gTiledAAManager.getNumTiles(); tileIndex++)
   {
      const BTerrainQuadNodePtrArray &mpNodeInstances = mTileAAVisibleNodeInstances[tileIndex];
      //gTerrainRender.compositeNodeInstanceTextures(&mpNodeInstances);// .renderCompositeNodeInstances(&mpNodeInstances);

      gTerrainTexturing.preCompositeSetup();

      for(unsigned int i=0;i<mpNodeInstances.size();i++)
      {   

         BTerrainQuadNode* node = mpNodeInstances[i];

         BASSERT(node);

         if( !node || 
            cUniqueTextureWidth >> node->mCacheInfo.mCurrLODLevel <= cTexSize360Cutoff || 
            !node->mRenderPacket ||
            !node->mRenderPacket->mTexturingData)
            continue;

         if(node->mRenderPacket->mTexturingData->mCachedUniqueTexture)
         {
            //if the texture is in the cache, and this node is visible, then mark it used.
            if(gTerrainTexturing.isTextureInCache(node->mRenderPacket->mTexturingData->mCachedUniqueTexture))
            {
               gTerrainTexturing.holdCachedTexture(node->mRenderPacket->mTexturingData->mCachedUniqueTexture);
               continue;
            }
         }

         node->mRenderPacket->mTexturingData->mCachedUniqueTexture = gTerrainTexturing.getAvailableCachedTexture(node->mCacheInfo.mCurrLODLevel);

         //we had NO room for this texture. ASSERT
         if(node->mRenderPacket->mTexturingData->mCachedUniqueTexture == NULL)
         {
            //CLM [12.06.07] Graceful fallout for FLS generation
            node->mCacheInfo.mCurrLODLevel=4;
            continue;
         }
         //BASSERT(node->mRenderPacket->mTexturingData->mCachedUniqueTexture != NULL);

         {
            node->mRenderPacket->mTexturingData->mCachedUniqueTexture->mpOwnerQuadNode = node;
            gTerrainTexturing.holdCachedTexture(node->mRenderPacket->mTexturingData->mCachedUniqueTexture);
            gTerrainTexturing.composeCompositeTexture(node->mRenderPacket->mTexturingData->mCachedUniqueTexture,node->mRenderPacket->mTexturingData->mLayerContainer);
         }


#ifndef BUILD_FINAL
         BTerrainMetrics::addNumThrashes(1);
#endif

      }
      gTerrainTexturing.postCompositeSetup();
   }
}

//============================================================================
// BTerrain::freeHeldCacheTextures
//============================================================================
void BTerrain::freeHeldCompositeTextures()
{
   SCOPEDSAMPLE(BTerrain_freeHeldCompositeTextures);

   if (!mLoadSuccessful)
      return;

   for (uint tileIndex = 0; tileIndex < gTiledAAManager.getNumTiles(); tileIndex++)
   {
      const BTerrainQuadNodePtrArray &mpNodeInstances = mTileAAVisibleNodeInstances[tileIndex];
      for(unsigned int i=0;i<mpNodeInstances.size();i++)
      {
         const BTerrainQuadNode* node = mpNodeInstances[i];
         BASSERT(node);
         if (!node)
            continue;
         gTerrainTexturing.unholdCachedTexture(node->mRenderPacket->mTexturingData->mCachedUniqueTexture);
      }
      //gTerrainRender.compositeNodeInstances(&mpNodeInstances);// .renderCompositeNodeInstances(&mpNodeInstances);
   }
}

//============================================================================
// BTerrain::occlusionCullNodes
//============================================================================
void BTerrain::occlusionCullNodes(const BTerrainQuadNodePtrArray *pNodeInstances,BTerrainQuadNodePtrArray &mpCulledNodeInstances)
{
   SCOPEDSAMPLE(BTerrain_occlusionCullNodes);

   if (!mLoadSuccessful)
      return;

   for(unsigned int i=0;i<(*pNodeInstances).size();i++)
   {
      const BTerrainQuadNode* node = (*pNodeInstances)[i];
      BASSERT(node);
      if (!node)
         continue;

      BVector mn;
      BVector mx;
      mn.set(node->getDesc().m_min.x,node->getDesc().m_min.y,node->getDesc().m_min.z);
      mx.set(node->getDesc().m_max.x,node->getDesc().m_max.y,node->getDesc().m_max.z);
      if(gOcclusionManager.testOcclusion(mn,mx, XMMatrixIdentity()) == BOcclusionManager::eOR_FullyHidden)
         continue;
      mpCulledNodeInstances.push_back((*pNodeInstances)[i]);
   }
   
}

//============================================================================
// BTerrain::renderTileBegin
//============================================================================
void BTerrain::renderTileBegin(eTRenderPhase phase, uint tileIndex)
{
   ASSERT_THREAD(cThreadIndexRender);

   if (!mLoadSuccessful)
      return;
      
   BDEBUG_ASSERT(tileIndex < cMaxTiles);

   SCOPEDSAMPLE(BTerrain_renderTileBegin);

   BTerrainQuadNodePtrArray &mpNodeInstances = mTileAAVisibleNodeInstances[tileIndex];
   BTerrainQuadNodePtrArray &mpSkirtNodeInstances = mTileAAVisibleSkirtNodeInstances[tileIndex];


   //CLM move occlusion testing here.

   BTerrainQuadNodePtrArray mpCulledNodeInstances;
   BTerrainQuadNodePtrArray mpCulledSkirtNodeInstances;
   if(gOcclusionManager.isOcclusionTestEnabled())
   {
      occlusionCullNodes(&mTileAAVisibleNodeInstances[tileIndex],mpCulledNodeInstances);
      mpNodeInstances = mpCulledNodeInstances;

      occlusionCullNodes(&mTileAAVisibleSkirtNodeInstances[tileIndex],mpCulledSkirtNodeInstances);
      mpSkirtNodeInstances = mpCulledSkirtNodeInstances;
   }

#ifndef BUILD_FINAL
   if(mRenderFoliage)
#endif
      gFoliageManager.render(mpQuadGrid,tileIndex);  //THIS SHOULD RECIEVE THE ACTIVE LIST OF QUADNODES FOR THIS TILE!

   gTerrainRender.beginDCBRender(&mpNodeInstances, &mpSkirtNodeInstances, true);
}

//============================================================================
// BTerrain::renderTileEnd
//============================================================================
void BTerrain::renderTileEnd(eTRenderPhase phase, uint tileIndex)
{
   SCOPEDSAMPLE(BTerrain_renderTileEnd);
   
   gTerrainRender.endDCBRender();


#ifndef BUILD_FINAL
   if (mRenderRoads)
#endif
      gRoadManager.render(mpQuadGrid, tileIndex);  //THIS SHOULD RECIEVE THE ACTIVE LIST OF QUADNODES FOR THIS TILE!


   gTerrainRibbonManager.render(tileIndex);

#ifndef BUILD_FINAL  
   if (mRenderQuadGridBBs)
      renderQuadGridBBs();
#endif
}

//============================================================================
// BTerrain::renderCustomNoTile
//============================================================================
void BTerrain::renderCustomNoTile(eTRenderPhase phase, eRenderPass shadowPassIndex, const BVec4* pSceneEyePos, const BVolumeCuller* pVolumeCuller, bool includeSkirt, bool filterNonShadowCasters, int overrideLODLevel)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   if (!mLoadSuccessful)
      return;

   BASSERT(phase!=cTRP_Full);

   SCOPEDSAMPLE(BTerrain_renderCustomNoTile);

   const BTerrainQuadNodePtrArray* pNodes = &mSceneNodes;
   
   if (pVolumeCuller)
   {
      mTempNodes.resize(0);
      pNodes = &mTempNodes;
      
      //CLM e3 2007 OPTOMIZATION!
      //if we're in reflect mode, use the grouped skirt boxes, and render them instead
       if (phase==cTRP_Reflect)
       {
            getAABBIntersectionSkirts(mTempNodes,pVolumeCuller->getBaseMin(),pVolumeCuller->getBaseMax(),true);
       }
       else
       {
          BTerrainQuadNode::BGetVisibleNodesParams getVisParams;

          getVisParams.mEyePos             = *pSceneEyePos;
          getVisParams.mpVolumeCuller      = pVolumeCuller;
          getVisParams.mpAllUniqueNodes    = NULL;
          getVisParams.mFilterNonShadowCasters  = filterNonShadowCasters;//(shadowPassIndex == cRPLocalShadowBuffer);
          getVisParams.mpNodes             = &mTempNodes;
          getVisParams.mIncludeSkirt       = includeSkirt;
         getVisibleNodes(getVisParams,&mExcludeObjects);
       }
   }
   else
   {
      switch (shadowPassIndex)
      {
         case cRPVisible:
         {
            BDEBUG_ASSERT(phase != cTRP_ShadowGen);
            break;
         }
         case cRPDirShadowBuffer0:
         case cRPDirShadowBuffer1:
         case cRPDirShadowBuffer2:
         case cRPDirShadowBuffer3:
         case cRPDirShadowBuffer4:
         case cRPDirShadowBuffer5:
         case cRPDirShadowBuffer6:
         case cRPDirShadowBuffer7:
         {
            BDEBUG_ASSERT(shadowPassIndex < cMaxShadowNodeArrays);
            pNodes = &mShadowNodes[shadowPassIndex];
            break;
         }
         case cRPLocalShadowBuffer:
         {
            BDEBUG_ASSERT(0);
         }
      }
   }      

   {
      // For the reflect phase, we actually render as full
      if (phase == cTRP_Reflect)
      {
         XMFLOAT4 camPosF4((*pSceneEyePos)[0],(*pSceneEyePos)[1],(*pSceneEyePos)[2],1);
         XMVECTOR camPos = XMLoadFloat4(&camPosF4);
         gTerrainRender.beginRender(cTRP_Full);
         for (uint i = 0; i < pNodes->size(); i++)
         {
            BTerrainQuadNodeDesc Desc =  (*pNodes)[i]->getDesc();
            XMFLOAT4 a(Desc.m_min.x,Desc.m_min.y,Desc.m_min.z,1);
            XMFLOAT4 b(Desc.m_max.x,Desc.m_max.y,Desc.m_max.z,1);
            XMVECTOR min=XMLoadFloat4(&a);
            XMVECTOR max=XMLoadFloat4(&b);

            //if we're too far away from the camera, don't render us..
            XMVECTOR bbCenter =(min + max) * 0.5f;
            XMVECTOR dist = XMVector4Length(bbCenter - camPos);

            XMFLOAT4 distf4;
            XMStoreFloat4(&distf4,dist);
            if(distf4.x > mReflectionFadeoutDist)
               continue;


            (*pNodes)[i]->render();
            
         }

         gTerrainRender.endRender();
      }
      else
      {
         //render GRASS
         #ifndef BUILD_FINAL
         if(mRenderFoliage)
         #endif
         gFoliageManager.renderCustom(phase,pNodes,mpQuadGrid);  //THIS SHOULD RECIEVE THE ACTIVE LIST OF QUADNODES FOR THIS TILE!


         gTerrainRender.beginRender(phase);
         for (uint i = 0; i < pNodes->size(); i++)
         {

            // Set override LOD level if one specified
            int currentLODLevel=0;
            if (overrideLODLevel >= 0 && !(*pNodes)[i]->mSkirtInfo.mIsSkirtChunk)
            {
               currentLODLevel = (*pNodes)[i]->mCacheInfo.mCurrLODLevel;
               (*pNodes)[i]->mCacheInfo.mCurrLODLevel = overrideLODLevel;
            }

            // Render
            (*pNodes)[i]->render();

            // Restore original LOD level
            if (overrideLODLevel >= 0 && !(*pNodes)[i]->mSkirtInfo.mIsSkirtChunk)
               (*pNodes)[i]->mCacheInfo.mCurrLODLevel = currentLODLevel;
         }

         gTerrainRender.endRender();
      }
         
       
   }
}

//============================================================================
// BTerrain::renderBegin
//============================================================================
void BTerrain::renderBegin(eTRenderPhase phase)
{
   if (phase == cTRP_Full || phase == cTRP_Reflect)
   {
      gTerrainTexturing.allocateTempResources();
      gTerrainRender.allocateTempResources();
#ifndef BUILD_FINAL
      if(mRenderRoads)
#endif
      gRoadManager.allocateTempResources();
#ifndef BUILD_FINAL
      if(mRenderFoliage)
#endif
         gFoliageManager.allocateTempResources();
   }
}

//============================================================================
// BTerrain::renderEnd
//============================================================================
void BTerrain::renderEnd(eTRenderPhase phase)
{
   if (phase == cTRP_Full || phase == cTRP_Reflect)
   {
      gTerrainTexturing.releaseTempResources();
      gTerrainRender.releaseTempResources();
#ifndef BUILD_FINAL
      if(mRenderRoads)
#endif
      gRoadManager.releaseTempResources();
#ifndef BUILD_FINAL
      if(mRenderFoliage)
#endif
         gFoliageManager.releaseTempResources();
   }
}

//============================================================================
// BTerrain::processCommand
//============================================================================
void BTerrain::processCommand(const BRenderCommandHeader& header, const unsigned char* pData)
{
   ASSERT_THREAD(cThreadIndexRender);

   switch (header.mType)
   {
      case cTECDestroy:
      {
         destroyInternal();
         break;
      }
      case cTECFlattenInstant:
      {
         flattenVisRepInstantInternal((deformPacket*)pData);
         break;
      }
      case cTECSetExclusionObject:
      {
         setExclusionObjectInternal((exclusionPacket *)(pData));
         break;
      }
      case cTECClearExclusionObjects:
      {
         clearExclusionObjectsInternal();
         break;
      }
   };
};  

//============================================================================
// BTerrain::frameBegin
//============================================================================
void BTerrain::frameBegin(void)
{
   ASSERT_THREAD(cThreadIndexRender);

   /*if(mReloadPending)
   {
      gTerrain.setLoadSuccessful(false);
      destroyInternal();
      gTerrainVisual.destroyInternal();
      gTerrainTexturing.destroyInternal();
      gRoadManager.destroyInternal();
      gFoliageManager.destroyInternal();
 
      mLoader.reloadInternal();

      setReloadPending(false);
   }*/

#ifndef BUILD_FINAL
   BTerrainMetrics::clearFrame();
#endif

};

//============================================================================
// BTerrain::frameEnd
//============================================================================
void BTerrain::frameEnd(void)
{
   ASSERT_THREAD(cThreadIndexRender);
};

//============================================================================
// BTerrain::deinitDeviceData
//============================================================================
void BTerrain::deinitDeviceData()
{
   ASSERT_THREAD(cThreadIndexRender);

#ifndef BUILD_FINAL
#ifndef BUILD_DEBUG
#ifndef USE_BUILD_INFO
   freeMemoryReserve();
#endif
#endif
#endif
}

//============================================================================
// BTerrain::initDeviceData
//============================================================================
void BTerrain::initDeviceData()
{
   ASSERT_THREAD(cThreadIndexRender);
#ifndef BUILD_FINAL
#ifndef BUILD_DEBUG
#ifndef USE_BUILD_INFO
   allocMemoryReserve();
#endif
#endif
#endif
}

//============================================================================
// BTerrain::getMin
//============================================================================
const D3DXVECTOR3& BTerrain::getMin(void) const 
{   
   if (mLoadPending)
   {
      static D3DXVECTOR3 v(-10, -10, -10);
      return v;
   }

   return mWorldMin; 
}

//============================================================================
// BTerrain::getMax
//============================================================================
const D3DXVECTOR3& BTerrain::getMax(void) const 
{
   if (mLoadPending)
   {
      static D3DXVECTOR3 v(10, 10, 10);
      return v;
   }
   return mWorldMax;  
}

//============================================================================
// BTerrain::flattenVisRep
//============================================================================
void BTerrain::flattenVisRepInstant(float mMinXPerc,float mMaxXPerc,float mMinZPerc,float mMaxZPerc,float mDesiredHeight,float mFalloffPerc)
{
   ASSERT_THREAD(cThreadIndexSim);
   deformPacket dp;
   dp.mDesiredHeight = mDesiredHeight;
   dp.mMinXPerc = Math::Clamp<float>(mMinXPerc,0,1);
   dp.mMaxXPerc = Math::Clamp<float>(mMaxXPerc,0,1);
   dp.mMinZPerc = Math::Clamp<float>(mMinZPerc,0,1);
   dp.mMaxZPerc = Math::Clamp<float>(mMaxZPerc,0,1);
   dp.mFalloffPerc = Math::Clamp<float>(mFalloffPerc,0,1);
   
   gRenderThread.submitCommand(mCommandHandle, cTECFlattenInstant, dp);
}
//============================================================================
// BTerrain::flattenVisRepInternal
//============================================================================
void BTerrain::flattenVisRepInstantInternal(const deformPacket* pkt)
{
   ASSERT_THREAD(cThreadIndexRender);
   gTerrainDeformer.queueFlattenCommand(pkt->mMinXPerc, pkt->mMaxXPerc, pkt->mMinZPerc, pkt->mMaxZPerc,pkt->mDesiredHeight,pkt->mFalloffPerc);
}
//============================================================================
// BTerrain::clearExclusionSpheres
//============================================================================
void BTerrain::clearExclusionObjects()
{
   ASSERT_THREAD(cThreadIndexSim);
   gRenderThread.submitCommand(mCommandHandle, cTECClearExclusionObjects);
}
//============================================================================
// BTerrain::flattenVisRepInternal
//============================================================================
void BTerrain::clearExclusionObjectsInternal()
{
   ASSERT_THREAD(cThreadIndexRender);


   mExcludeObjects.clear();
}

//============================================================================
// BTerrain::setExclusionSpheres
//============================================================================
void BTerrain::setExclusionSphere(const BVector &exclusionSphere, bool occluded)
{
   ASSERT_THREAD(cThreadIndexSim);

   exclusionPacket ept;
   ept.mExclusionObject.mData0 = exclusionSphere;
   ept.mExclusionObject.mExclusionObjectType = 0;
   ept.occlude = occluded;

   gRenderThread.submitCommand(mCommandHandle, cTECSetExclusionObject, ept);
}
//============================================================================
// BTerrain::setExclusionRectangle
//============================================================================
 void BTerrain::setExclusionRectangle(const BVector &min,const BVector &max, bool occluded)
 {
    ASSERT_THREAD(cThreadIndexSim);

    exclusionPacket ept;
    ept.mExclusionObject.mData0 = min;
    ept.mExclusionObject.mData1 = max;
    ept.mExclusionObject.mExclusionObjectType = 1;
    ept.occlude = occluded;

    gRenderThread.submitCommand(mCommandHandle, cTECSetExclusionObject, ept);
 }
//============================================================================
// BTerrain::flattenVisRepInternal
//============================================================================
void BTerrain::setExclusionObjectInternal(const exclusionPacket *pkt)
{
   ASSERT_THREAD(cThreadIndexRender);

   BDEBUG_ASSERT(pkt);

   
   if (pkt->occlude)
      mExcludeObjects.add(pkt->mExclusionObject);   
   else
      mExcludeObjects.remove(pkt->mExclusionObject);
}

//============================================================================
// BTerrain::calculateViewExtents
//============================================================================
uint BTerrain::calculateViewExtents(XMMATRIX worldToView, XMMATRIX viewToProj, const BVolumeCuller& volumeCuller, bool includeSkirt, AABB& viewBounds)
{
   BTerrainQuadNode::BGetVisibleNodesParams params;
        
   BTerrainQuadNodePtrArray nodes;
   params.mEyePos = BVec4(0.0f, 0.0f, 0.0f, 1.0f);
   params.mpNodes = &nodes;
   params.mpVolumeCuller = &volumeCuller;
   params.mFilterNonShadowCasters = false;
   params.mIncludeSkirt = includeSkirt;
   
   getVisibleNodes(params,&mExcludeObjects);

   for (uint nodeIndex = 0; nodeIndex < nodes.getSize(); nodeIndex++)
   {
      const BTerrainQuadNode& quadNode = *nodes[nodeIndex];
      
      const BTerrainQuadNodeDesc& quadDesc = quadNode.getDesc();
      
      AABB worldBounds;
      worldBounds[0].set(quadDesc.m_min[0], quadDesc.m_min[1], quadDesc.m_min[2]);
      worldBounds[1].set(quadDesc.m_max[0], quadDesc.m_max[1], quadDesc.m_max[2]);
      
      AABB nodeViewBounds(worldBounds.transform3(*reinterpret_cast<const BMatrix44*>(&worldToView)));
      
      viewBounds.expand(nodeViewBounds);
   }
   
   return nodes.getSize();
}

//============================================================================
// BTerrain::calculateProjZExtents
//============================================================================
uint BTerrain::calculateProjZExtents(XMMATRIX worldToView, XMMATRIX viewToProj, const BVolumeCuller& volumeCuller, bool includeSkirt, BInterval& zExtent)
{
   BTerrainQuadNode::BGetVisibleNodesParams params;

   BTerrainQuadNodePtrArray nodes;
   params.mEyePos = BVec4(0.0f, 0.0f, 0.0f, 1.0f);
   params.mpNodes = &nodes;
   params.mpVolumeCuller = &volumeCuller;
   params.mFilterNonShadowCasters = false;
   params.mIncludeSkirt = includeSkirt;

   getVisibleNodes(params,&mExcludeObjects);
   
   const XMMATRIX worldToProj = worldToView * viewToProj;

   for (uint nodeIndex = 0; nodeIndex < nodes.getSize(); nodeIndex++)
   {
      const BTerrainQuadNode& quadNode = *nodes[nodeIndex];

      const BTerrainQuadNodeDesc& quadDesc = quadNode.getDesc();

      AABB worldBounds;
      worldBounds[0].set(quadDesc.m_min[0], quadDesc.m_min[1], quadDesc.m_min[2]);
      worldBounds[1].set(quadDesc.m_max[0], quadDesc.m_max[1], quadDesc.m_max[2]);
      
      for (uint i = 0; i < 8; i++)
      {
         BVec4 c(worldBounds.corner(i));
         c[3] = 1.0f;
         BVec4 p(c * ((BMatrix44&)worldToProj));
         if (p[3] != 0.0f)
         {
            if (p[3] < 0.0f)
               zExtent.expand(0.0f);
            else
               zExtent.expand(p[2] / p[3]);
         }
      }
   }

   return nodes.getSize();
}

//============================================================================
// BTerrain::sceneIterate
//============================================================================
uint BTerrain::sceneIterate(const BSceneIterateParams& params, BSceneIteratorCallbackFunc pIteratorFunc, void* pData)
{
   BTerrainQuadNode::BGetVisibleNodesParams getVisibleNodesParams;

   BTerrainQuadNodePtrArray nodes;
   getVisibleNodesParams.mEyePos = BVec4(0.0f, 0.0f, 0.0f, 1.0f);
   getVisibleNodesParams.mpNodes = &nodes;
   getVisibleNodesParams.mpVolumeCuller = params.mpVolumeCuller;
   getVisibleNodesParams.mFilterNonShadowCasters = params.mShadowCastersOnly;
   getVisibleNodesParams.mIncludeSkirt = params.mIncludeSkirt;

   getVisibleNodes(getVisibleNodesParams,&mExcludeObjects);
   
   uint nodeIndex;
   for (nodeIndex = 0; nodeIndex < nodes.getSize(); nodeIndex++)
   {
      const BTerrainQuadNode& quadNode = *nodes[nodeIndex];

      const BTerrainQuadNodeDesc& quadDesc = quadNode.getDesc();
            
      XMVECTOR minBounds = XMLoadFloat3((const XMFLOAT3*)&quadDesc.m_min);
      XMVECTOR maxBounds = XMLoadFloat3((const XMFLOAT3*)&quadDesc.m_max);
      
      const uint chunkX = quadDesc.mMinXVert >> 6;
      const uint chunkY = quadDesc.mMinZVert >> 6;
            
      if (!pIteratorFunc(params, chunkX, chunkY, minBounds, maxBounds, pData))
         break;
   }
   
   return nodeIndex;
}

//============================================================================
// BTerrain::debugDraw
//============================================================================
#ifndef BUILD_FINAL
void BTerrain::debugDraw()
{  
   if (!mRenderVisGrid)
      return;

   int minGridX = 450;
   int minGridY = 200;
   int nodePtSize = 8;
   int GridWidth = mNumXChunks * (nodePtSize+1);

   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
   BD3D::mpDev->SetTexture(0, NULL);
   
   for(int i=0;i<mNumXChunks*mNumXChunks;i++)
   {
      if (!mpQuadGrid[i].mVisibleInThisTile[0] && 
          !mpQuadGrid[i].mVisibleInThisTile[1] &&
          !mpQuadGrid[i].mVisibleInThisTile[2] &&
          !mpQuadGrid[i].mVisibleInThisTile[3])
          continue;
      
      float xt = mpQuadGrid[i].getDesc().mMinXVert / 64.0f;
      float zt = mNumXChunks-(mpQuadGrid[i].getDesc().mMinZVert / 64.0f);

      int x = (int)(minGridX+xt*(nodePtSize+1));
      int z = (int)(minGridY+((zt*(nodePtSize+1))));

      BPrimDraw2D::drawSolidRect2D(x, z, x + nodePtSize, z + nodePtSize, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 0, cPosDiffuseVS, cDiffusePS);
   }

   //draw a border around us
   BPrimDraw2D::drawLine2D(minGridX,               minGridY + nodePtSize,               minGridX + GridWidth,      minGridY + nodePtSize,                     0xFF00FF00);
   BPrimDraw2D::drawLine2D(minGridX,               minGridY + nodePtSize + GridWidth,    minGridX + GridWidth,      minGridY + nodePtSize+ GridWidth,          0xFF00FF00);
   BPrimDraw2D::drawLine2D(minGridX,               minGridY + nodePtSize,               minGridX,                  minGridY  + nodePtSize+ GridWidth,         0xFF00FF00);
   BPrimDraw2D::drawLine2D(minGridX+ GridWidth,    minGridY + nodePtSize,               minGridX+ GridWidth,       minGridY  + nodePtSize+ GridWidth,         0xFF00FF00);


   
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);   
}


//============================================================================
// BTerrain::allocMemoryReserve
//============================================================================
void BTerrain::allocMemoryReserve()
{
   // rg [1/28/08] - Disabling this for now
#if 0
   ASSERT_THREAD(cThreadIndexRender);
   const uint memToReserve = 16 * (1024*1024);//16mb
   mpMemoryReserve= XPhysicalAlloc( memToReserve, MAXULONG_PTR, 0, PAGE_READWRITE);
   BVERIFY(mpMemoryReserve);
#endif   
}

//============================================================================
// BTerrain::freeMemoryReserve
//============================================================================
void BTerrain::freeMemoryReserve()
{
   ASSERT_THREAD(cThreadIndexRender);
   if(mpMemoryReserve)
   {
      XPhysicalFree(mpMemoryReserve);
      mpMemoryReserve=0;
   }
}
#endif
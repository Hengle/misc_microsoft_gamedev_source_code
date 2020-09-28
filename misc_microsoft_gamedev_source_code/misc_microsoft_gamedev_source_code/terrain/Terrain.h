//============================================================================
//
//  Terrain.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once
// terrain
#include "TerrainQuadNode.h"
#include "TerrainVisual.h"
#include "mathlib.h"
#include "TerrainIO.h"
#include "math\vectorInterval.h"


//xgamerender
#include "..\xgamerender\ugxGeomRenderTypes.h"

//============================================================================
// Forward declarations
//============================================================================
struct XTDVisual;
struct XTDSim;
class BTerrainIOLoader;
class deformPacket;
//============================================================================
// enum eTRenderPhase
//============================================================================
enum eTRenderPhase
{
   cTRP_Invalid = -1,

   cTRP_Full = 0,
   cTRP_ShadowGen,
   cTRP_Reflect,
  
   cTRP_Count,
};

//============================================================================
// class BTerrain
//============================================================================
class BTerrain : BRenderCommandListener
{
public:
    //----------------------------------------------------
   BTerrain();
   ~BTerrain();

    //----------------------------------------------------
   bool           init();           //called @ beginning of time
   bool           deinit();         //called @ end of time


   bool           isInitialized(void) const { return cInvalidCommandListenerHandle != mCommandHandle; }

   // Evaluates the visible or shadow buffer scene nodes to render. Must call with -1 first, 
   // which clears the internal node arrays and evaluates the scene's visible nodes.
   
   enum eRenderPass
   {
      cRPVisible           = -1,
      
      cRPDirShadowBuffer0  = 0,
      cRPDirShadowBuffer1,
      cRPDirShadowBuffer2,
      cRPDirShadowBuffer3,
      cRPDirShadowBuffer4,
      cRPDirShadowBuffer5,
      cRPDirShadowBuffer6,
      cRPDirShadowBuffer7,
      
      cRPLocalShadowBuffer
   };
   
    //----------------------------------------------------
   bool           load(long dirID, const char *filename, long terrainDirID, bool loadVisRep);
   void           destroy();    //called @ end of map
   //----------------------------------------------------

   // evalSceneNodes() must be called for every render pass in order to determine the LOD levels of all chunks that will be rendered in the current frame.
   void           evalSceneNodesLODDeferred(eRenderPass shadowPassIndex, const BVec4& sceneEyePos, const BVolumeCuller& volumeCuller, bool includeSkirt);
   void           evalSceneNodesLODImmediate(eRenderPass shadowPassIndex, const BVec4& sceneEyePos, const BVolumeCuller& volumeCuller, bool includeSkirt);


   //----------------------------------------------------
   //interface to call gTerrainTexturing.defragmentCaches();
   void           defragmentCaches(void);
   

   //----------------------------------------------------
   void           renderBegin(eTRenderPhase phase);

   // These methods are called once during rendering. Every visible node on the screen composits it's texture at the same time
   void           compositeVisibleNodes();
   void           freeHeldCompositeTextures();


   void           computeTileDCBs(int tileIndex);
   void           flushTileDCBs(int tileIndex);

   // This method renders the scene's or a shadow buffer's nodes using the specified phase.
   // The volume culler is used if not NULL, otherwise the previously evaluated nodes are used (only valid for visible and dir shadow buffer passes).
   // If pVolumeCuller is not NULL, pSceneEyePos must be valid.
   void           renderTileBegin(eTRenderPhase phase, uint tileIndex);
   void           renderTileEnd(eTRenderPhase phase, uint tileIndex);
   void           renderCustomNoTile(eTRenderPhase phase, eRenderPass shadowPassIndex, const BVec4* pSceneEyePos, const BVolumeCuller* pVolumeCuller, bool includeSkirt, bool filterNonShadowCasters, int overrideLODLevel = -1);
	
	void           renderEnd(eTRenderPhase phase);

   //----------------------------------------------------
   // getMin() and getMax() returns default values until the terrain is loaded.
   const D3DXVECTOR3& getMin(void) const;
   const D3DXVECTOR3& getMax(void) const;
   int getNumXChunks(){return mNumXChunks; };
   //----------------------------------------------------

   //----------------------------------------------------
   bool getLoadSuccessful(void) const { MemoryBarrier(); return mLoadSuccessful != 0; }
   bool getLoadPending(void) const { MemoryBarrier(); return mLoadPending != 0; }
   bool getReloadPending(void) const { MemoryBarrier(); return mReloadPending != 0; }

    //----------------------------------------------------
   //deformaions ( called from sim thread)
   void flattenVisRepInstant(float mMinXPerc,float mMaxXPerc,float mMinZPerc,float mMaxZPerc,float mDesiredHeight,float mFalloffPerc);

    //----------------------------------------------------
   void setExclusionSphere(const BVector &exclusionSpheres, bool occluded);
   void setExclusionRectangle(const BVector &min,const BVector &max, bool occluded);
   void clearExclusionObjects();

   //----------------------------------------------------
   uint calculateViewExtents(XMMATRIX worldToView, XMMATRIX viewToProj, const BVolumeCuller& volumeCuller, bool includeSkirt, AABB& viewBounds);
   uint calculateProjZExtents(XMMATRIX worldToView, XMMATRIX viewToProj, const BVolumeCuller& volumeCuller, bool includeSkirt, BInterval& zExtent);

   //----------------------------------------------------
   struct BSceneIterateParams
   {
      BSceneIterateParams() : 
         mpVolumeCuller(NULL), 
         mShadowCastersOnly(false),
         mIncludeSkirt(false)
      {
      }

      const BVolumeCuller* mpVolumeCuller;
      bool mShadowCastersOnly;
      bool mIncludeSkirt;
   };

   // Returns false to stop iteration.
   typedef bool (*BSceneIteratorCallbackFunc)(const BSceneIterateParams& params, uint chunkX, uint chunkY, XMVECTOR worldMin, XMVECTOR worldMax, void* pData);

   uint sceneIterate(const BSceneIterateParams& params, BSceneIteratorCallbackFunc pIteratorFunc, void* pData);
//----------------------------------------------------
   //batch LOD work
   
   void beginBatchShadowVis(int totalNumWorkEntries);
   void joinBatchWork();   //call this before we render!!
//----------------------------------------------------

#ifndef BUILD_FINAL
   void enableQuadGridBBRender(bool onOff) { mRenderQuadGridBBs=onOff; };
   void enableRenderTextures(bool onOff){mRenderTextures=onOff;   };
   void enableRenderRoads(bool onOff){mRenderRoads=onOff;   };
   void enableRenderFoliage(bool onOff){mRenderFoliage=onOff;   };
   bool isRenderFoliageEnable(){return mRenderFoliage;};
   void toggleRenderVisGrid(){mRenderVisGrid = !mRenderVisGrid;};
   bool isVisGridEnabled(){ return mRenderVisGrid;};
   static void setVisMode(eUGXGeomVisMode mode){mVisMode = mode;};
   static int getVisMode(){return mVisMode;   };
   static void setTextureMode(eUGXGeomTextureMode mode){mTextureMode = mode;};
   static int getTextureMode(){return mTextureMode;   };
   void debugDraw();
#endif

   void toggleRenderSkirt(){mRenderSkirt = !mRenderSkirt;};
   void setRenderSkirt(bool val) { mRenderSkirt=val; }
   bool getRenderSkirt() const { return mRenderSkirt; };

   void setReflectionFadeoutDist(float dist){ mReflectionFadeoutDist = dist;};
   float getReflectionFadeoutDist() {return mReflectionFadeoutDist;};

   //----------------------------------------------------

   D3DXMATRIX mQuadrantMatrices[8];
   //----------------------------------------------------
   class exclusionObject
   {
   public:
      exclusionObject():
         mData0(0.0f),
            mData1(0.0f),
            mExclusionObjectType(0)
      {

      }
      BVector mData0;
      BVector mData1;
      uchar   mExclusionObjectType;

      friend bool operator== (const exclusionObject& lhs, const exclusionObject& rhs) 
      {
         return lhs.mData0==rhs.mData0 && lhs.mData1==rhs.mData1 && lhs.mExclusionObjectType == rhs.mExclusionObjectType;
      }
   };

   //CLM TEMP
   const BTerrainQuadNode* const getQNGrid(){return mpQuadGrid;};
private:
   // FIXME: Too much interdepency.
   friend class BTerrainIOLoader;
   
   enum eTerrainCommands
   { 
      cTECDestroy = 0,
      cTECFlattenInstant,
      cTECSetExclusionObject,
      cTECClearExclusionObjects,
   };
   
   // Requires 16-byte alignment
   struct BEvalSceneNodesData
   {
      BVolumeCuller        mVolumeCuller;
      BVec4                mSceneEyePos;
      
      eRenderPass          mShadowPassIndex;
   };

   // Requires 16-byte alignment
   struct BTerrainRenderData
   {
      BVolumeCuller        mVolumeCuller;
      BVec4                mSceneEyePos;
      
      eTRenderPhase        mPhase;
      eRenderPass          mShadowPassIndex;
      
      bool                 mUseVolumeCuller : 1;
   };
   
   

   static eUGXGeomVisMode      mVisMode;
   static eUGXGeomTextureMode  mTextureMode;

   BTerrainQuadNode* mpQuadGrid;
   
   BDynamicArray<BTerrainQuadNode>  mQuadSkirtChunkList;

   int mNumXChunks;
   int mNumXSkirtChunks;
   float mBoundingBoxGrowDist;

   D3DXVECTOR3 mWorldMin;
   D3DXVECTOR3 mWorldMax;
   D3DXVECTOR3 mWorldMinTotal;   //includes skirt
   D3DXVECTOR3 mWorldMaxTotal;  //includes skirt

   
   BTerrainQuadNodePtrArray mAllUniqueNodes;

   BTerrainQuadNodePtrArray mSceneNodes;

   // Should match the # of cascaded shadow maps defined in dirShadowManager.h
   enum { cMaxShadowNodeArrays = 8 };
   BTerrainQuadNodePtrArray mShadowNodes[cMaxShadowNodeArrays];

   enum { cMaxTiles = 4 };
   BTerrainQuadNodePtrArray mTileAAVisibleNodeInstances[cMaxTiles];  //CLM CRAP! this should queary gTileAAManager.getNumTiles();
   BTerrainQuadNodePtrArray mTileAAVisibleSkirtNodeInstances[cMaxTiles];  //CLM CRAP! this should queary gTileAAManager.getNumTiles();

   BTerrainQuadNodePtrArray mTempNodes;
   
   BTerrainIOLoader mLoader;
   
   LONG mLoadPending;
   LONG mLoadSuccessful;
   LONG mReloadPending;
   
   bool           mRenderQuadGridBBs;
   bool           mRenderVisGrid;
   bool           mRenderTextures;
   bool           mRenderRoads;
   bool           mRenderFoliage;
   bool           mRenderSkirt;
   float          mReflectionFadeoutDist;

   void           setLoadSuccessful(bool success) { Sync::InterlockedExchangeExport(&mLoadSuccessful, success); }
   void           setLoadPending(bool pending) { Sync::InterlockedExchangeExport(&mLoadPending, pending); }
   void           setReloadPending(bool pending) { Sync::InterlockedExchangeExport(&mReloadPending, pending); }
         
   void	         initInternal();
   void	         destroyInternal();
         
   void           renderQuadGridBBs();
   
   //deformations(internal)
   void           flattenVisRepInstantInternal(const deformPacket *pkt);

   
   //----------------------------------------------------
   
   struct exclusionPacket
   {
      exclusionObject mExclusionObject;
      bool occlude;
   };

   void           setExclusionObjectInternal(const exclusionPacket *pkt);
   void           clearExclusionObjectsInternal();
   BDynamicArray<exclusionObject>  mExcludeObjects;

   //visible node list generation from a given frustum
   void           getAABBIntersection(BTerrainQuadNodePtrArray &nodes, XMVECTOR min, XMVECTOR max,const XMVECTOR &camPos, const BDynamicArray<BTerrain::exclusionObject> *exclusionObjects, bool setVisibility,  bool includeSkirt = true);
   void           getAABBIntersectionSkirts(BTerrainQuadNodePtrArray &nodes, XMVECTOR min, XMVECTOR max, bool invertTest);

   // Call to calculate what chunks are visible in general, and what tiles they lie in. The tile overlap count is important for composite texturing
   static void    evalSceneNodesVisibility(const BVolumeCuller& sceneCuller, const XMVECTOR &camPos,const BDynamicArray<BTerrain::exclusionObject> *exclusionObjects);
   //operations on our quadNode grid
   static void    getVisibleNodes(BTerrainQuadNode::BGetVisibleNodesParams& state,const BDynamicArray<BTerrain::exclusionObject> *exclusionObjects);

   
   void           evalSceneNodesLOD(eRenderPass shadowPassIndex, const BVec4& sceneEyePos, const BVolumeCuller& volumeCuller, bool includeSkirt, bool useWorkerPool);

   uint           mNumBucketWorkEntries;
   uint           cNumWorkEntriesPerBucket;
   uint           mNumCurrWorkEntries;

   struct ShadowVisThreadPacket
   {
      BTerrainQuadNode::BGetVisibleNodesParams mVisibleNodeParams;
      BDynamicArray<BTerrain::exclusionObject> mExclusionArray;
   };
   BDynamicArray<ShadowVisThreadPacket, 4> mShadowVisWorkEntries;
   bool           mEvalSceneNodesMTIssued;

  

   static void    evalSceneNodesVisibilityWorkerCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);
   void           queueEvalSceneNodesVisibility(const BVolumeCuller& volumeCuller, const XMVECTOR &xmEyePos);
   static void    getVisibleShadowNodesWorkerCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);
   void           queueGetVisibleShadowNodes(BTerrainQuadNode::BGetVisibleNodesParams parms);


   void occlusionCullNodes(const BTerrainQuadNodePtrArray *pNodeInstances,BTerrainQuadNodePtrArray &mpCulledNodeInstances);

   //BRenderCommandListenerInterface
   
   
   virtual void               processCommand(const BRenderCommandHeader& header, const unsigned char* pData);  
   virtual void               frameBegin(void);
   virtual void               frameEnd(void);
   virtual void               initDeviceData(void);
   virtual void               deinitDeviceData(void);

#ifndef BUILD_FINAL
   void allocMemoryReserve();
   void freeMemoryReserve();
   void* mpMemoryReserve;
#endif

   friend class BTerrainTexturing;

};
//-----------------------------------
extern BTerrain gTerrain;
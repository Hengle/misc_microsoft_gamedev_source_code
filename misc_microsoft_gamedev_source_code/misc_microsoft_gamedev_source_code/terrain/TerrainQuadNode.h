//============================================================================
//
//  TerrainQuadNode.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

// terrain
#include "TerrainVisual.h"
#include "volumeCuller.h"
   
//-----------------------------------------------------
struct XTDQuadNode;
class BFrustum;
class BTerrainRenderPacket;
class TerrainIOLoader;
 
enum eCacheValids
{
   cAge_UsedThisFrame   = 0x00000001,
   cAge_ToleranceWindow = 0x000000FF,
};
//-----------------------------------------------------
struct BTerrainQuadNodeDesc
{   
   //visual rep
   D3DXVECTOR3     m_min;
   D3DXVECTOR3     m_max;

   int		      mMinXVert,mMaxXVert;
   int		      mMinZVert,mMaxZVert;

   int            mTessPatchStartIndex;

   bool mCanCastShadows;

};

//---------------------------------------

class BTerrainQuadNode;
typedef BDynamicArray<BTerrainQuadNode*> BTerrainQuadNodePtrArray;

class BTerrainQuadNode
{
public:
	BTerrainQuadNode();
	~BTerrainQuadNode();
   void     destroy();
   void     deinitDeviceData();

   //void		render();
   void		render();

   //info about this node
   const BTerrainQuadNodeDesc& getDesc(void) const { return mDesc; }
   
   struct BGetVisibleNodesParams
   {
      BGetVisibleNodesParams() { clear(); }
      
      void clear(void) 
      {
         mEyePos.clear();
         mpNodes = NULL;
         mpAllUniqueNodes = NULL;
         mpVolumeCuller = NULL;
         mFilterNonShadowCasters = false;
         mIncludeSkirt = true;
      }
      
      BVec4                      mEyePos;
      
      // may be NULL
      BTerrainQuadNodePtrArray*  mpNodes;             
      
      // may be NULL
      BTerrainQuadNodePtrArray*  mpAllUniqueNodes;
      
      const BVolumeCuller*       mpVolumeCuller;
            
      bool                       mFilterNonShadowCasters;
      bool                       mIncludeSkirt;
   };
   

   static   int   getMaxNodeWidth();
   static   float getOOMaxNodeWidth();
   
   static   int   getMaxNodeDepth();
   static   float getOOMaxNodeDepth();

   BTerrainRenderPacket       *mRenderPacket;

   //what tiles are we visible in? This allows any visual params to be non-intrusive.
   enum { cMaxTiles = 4 };
   BOOL mVisibleInThisTile[cMaxTiles];  //CLM CRAP! this should queary gTileAAManager.getNumTiles();
   void clearVisibility()
   {  for(int i=0;i<cMaxTiles;i++)
      mVisibleInThisTile[i]=false;
   }


   //this returns if this quadnode has it's unique texture computed / streamed in.
   void updateFromVisibility(bool isVisible, const XMVECTOR camPos);


   struct cacheTextureInfo
   {
      int mNumTilesVisible;
      unsigned int mAgePredcition;
      int mCostThisFrame;
      int mCurrLODLevel;
      float mDistanceFromCamera;
   };

   cacheTextureInfo        mCacheInfo;
   void              calcCost();
   int               getCost() { return mCacheInfo.mCostThisFrame;   };
   void              increaseAge(){mCacheInfo.mAgePredcition = mCacheInfo.mAgePredcition<<1; };
   void              clearAge(){mCacheInfo.mAgePredcition = 0;};
   void              setUsedThisFrame(void)      {      mCacheInfo.mAgePredcition |= cAge_UsedThisFrame;      };
   bool              isUsedThisFrame(void)      {      return (mCacheInfo.mAgePredcition & cAge_UsedThisFrame) != 0;};
   bool              isUsedInTolerance(void)    {      return (mCacheInfo.mAgePredcition & cAge_ToleranceWindow) != 0;};

   struct skirtNodeInfo
   {
      BTerrainQuadNode *mpOwnerNode;
      int mQuadrant;
      int mSkirtBatchSize;
      bool mIsSkirtChunk;
   };
   skirtNodeInfo mSkirtInfo;

private:

   BTerrainQuadNodeDesc       mDesc;

	

   friend class BTerrainIOLoader;
};

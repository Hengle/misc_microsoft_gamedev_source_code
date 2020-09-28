//============================================================================
//
//  TerrainQuadNode.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
// xcore
#include "TerrainPCH.h"

// xrender
#include "renderThread.h"
#include "renderDraw.h"
#include "dirShadowManager.h"

// terrain
#include "TerrainIO.h"
#include "TerrainQuadNode.h"
#include "TerrainRender.h"
#include "terrainSimRep.h"
 
#include "TerrainMetric.h"
#include "TerrainTexturing.h"
//--------------------------------------------------
const unsigned int cMaxWidth	=	64;
const unsigned int cMaxHeight	=	64;
const unsigned int cNumFramesToHoldHandle = 1;


//============================================================================
// BTerrainQuadNode::getMaxNodeWidth
//============================================================================
int   BTerrainQuadNode::getMaxNodeWidth()
{
   return cMaxWidth;
};
//============================================================================
// BTerrainQuadNode::getOOMaxNodeWidth
//============================================================================
float   BTerrainQuadNode::getOOMaxNodeWidth()
{
   return 1.0f / cMaxWidth;
};
//============================================================================
// BTerrainQuadNode::getMaxNodeDepth
//============================================================================
int   BTerrainQuadNode::getMaxNodeDepth()
{
   return cMaxWidth;
};
//============================================================================
// BTerrainQuadNode::getOOMaxNodeDepth
//============================================================================
float   BTerrainQuadNode::getOOMaxNodeDepth()
{
   return 1.0f / cMaxWidth;
};
//============================================================================
// BTerrainQuadNode::BTerrainQuadNode
//============================================================================
BTerrainQuadNode::BTerrainQuadNode() : 
mRenderPacket(0)
{
   mCacheInfo.mCurrLODLevel=gTerrainTexturing.getMaxNumberMipLevels()-1;
   mCacheInfo.mNumTilesVisible=0;
   mCacheInfo.mCostThisFrame =0;
   mCacheInfo.mAgePredcition=0;

   for(int i=0;i<cMaxTiles;i++)
      mVisibleInThisTile[i]=false;
   /*
   for(int i = 0; i < cTotalNumberOfLevels; i++)
   {
      mCacheInfoPerLOD[i].mNumTilesVisible=0;
      mCacheInfoPerLOD[i].mCostThisFrame =0;
      mCacheInfoPerLOD[i].mAgePredcition=0;
   }
   */
}
//============================================================================
// BTerrainQuadNode::~BTerrainQuadNode
//============================================================================
BTerrainQuadNode::~BTerrainQuadNode()
{
   destroy();
}
//============================================================================
// BTerrainQuadNode::deinitDeviceData
//============================================================================
void BTerrainQuadNode::deinitDeviceData()
{
   if(mRenderPacket)
   {
      if(mRenderPacket->mTexturingData)
         mRenderPacket->mTexturingData->freeDeviceData();
      if(mRenderPacket->mVisualData)
         mRenderPacket->mVisualData->freeDeviceData();   
   }
}
//============================================================================
// BTerrainQuadNode::destroy
//============================================================================
void BTerrainQuadNode::destroy()
{
   {  
      if(mRenderPacket)
      {
         deinitDeviceData();

         
         delete mRenderPacket->mTexturingData;
         delete mRenderPacket->mVisualData;

         delete mRenderPacket;
      }
      
      mRenderPacket=0;  
   }
}


//============================================================================
// BTerrainQuadNode::render
//============================================================================
void		BTerrainQuadNode::render()
{
   if(mSkirtInfo.mIsSkirtChunk)
      gTerrainRender.renderSkirt(mSkirtInfo);
   else
      gTerrainRender.render(*this, mRenderPacket );
}

//============================================================================
// BTerrainQuadNode::updateFromVisibility
//============================================================================
void BTerrainQuadNode::updateFromVisibility(bool isVisible, const XMVECTOR camPos)
{
   if(isVisible)
   {
      XMFLOAT4 a(mDesc.m_min.x,mDesc.m_min.y,mDesc.m_min.z,1);
      XMFLOAT4 b(mDesc.m_max.x,mDesc.m_max.y,mDesc.m_max.z,1);
      XMVECTOR min=XMLoadFloat4(&a);
      XMVECTOR max=XMLoadFloat4(&b);

      int LODLevel = gTerrainTexturing.computeUniqueTextureLOD(min,max,camPos,mCacheInfo.mDistanceFromCamera);

      if(mCacheInfo.mCurrLODLevel!=LODLevel)
      {
         if ((mRenderPacket) && (mRenderPacket->mTexturingData))
            gTerrainTexturing.freeCachedTexture(mRenderPacket->mTexturingData->mCachedUniqueTexture);   
         mCacheInfo.mCurrLODLevel=LODLevel;
      }
      this->setUsedThisFrame();
   }
   else
   {
      if(!isUsedInTolerance())
      {
         if ((mRenderPacket) && (mRenderPacket->mTexturingData))
            gTerrainTexturing.freeCachedTexture(mRenderPacket->mTexturingData->mCachedUniqueTexture);
         clearAge();
      }
      

    //  gTerrainTexturing.freeCachedTexture(mRenderPacket->mTexturingData->mCachedUniqueTexture);
       
   }

   //caclulate our cost for this frame once, then use multiple times
   calcCost();
   
}

//============================================================================
// BTerrainQuadNode::calcCost
//============================================================================
void BTerrainQuadNode::calcCost()
{
   if(!mRenderPacket || !mRenderPacket->mTexturingData)
      return;

   //count the number of bits used
   //http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
   unsigned int v = mCacheInfo.mAgePredcition;
   unsigned int const w = v - ((v >> 1) & 0x55555555);                    // temp
   unsigned int const x = (w & 0x33333333) + ((w >> 2) & 0x33333333);     // temp
   unsigned int const c = ((x + (x >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count

   //CLM - PIX: LHS here!
   //CLM - PIX: MUL HERE!
   //float APC = c * 0.03125f;//(c / 32)
   //int RC = mRenderPacket->mTexturingData->mLayerContainer.mNumLayers * mCacheInfo.mNumTilesVisible;
   //return RC*APC;
   mCacheInfo.mCostThisFrame = static_cast<int>((c * 0.03125f) * (mRenderPacket->mTexturingData->mLayerContainer.getNumLayers() * mCacheInfo.mNumTilesVisible));
}

/*
//============================================================================
// BTerrainQuadNode::clearUnused
//============================================================================
void BTerrainQuadNode::clearUnused()
{     
   if(!isUsedInTolerance())
   {
      gTerrainTexturing.freeCachedTexture(mRenderPacket->mTexturingData->mCachedUniqueTexture);
      clearAge();
   }
}

//============================================================================
// BTerrainQuadNode::calcCostAtLOD
//============================================================================
void BTerrainQuadNode::calcCostAtLOD(int nLODLevel)
{
   if(!mRenderPacket || !mRenderPacket->mTexturingData)
      return;

   //count the number of bits used
   //http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
   unsigned int v = mCacheInfoPerLOD[nLODLevel].mAgePredcition;
   unsigned int const w = v - ((v >> 1) & 0x55555555);                    // temp
   unsigned int const x = (w & 0x33333333) + ((w >> 2) & 0x33333333);     // temp
   unsigned int const c = ((x + (x >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count

   //CLM - PIX: LHS here!
   //CLM - PIX: MUL HERE!
   //float APC = c * 0.03125f;//(c / 32)
   //int RC = mRenderPacket->mTexturingData->mLayerContainer.mNumLayers * mCacheInfo.mNumTilesVisible;
   //return RC*APC;
   mCacheInfoPerLOD[nLODLevel].mCostThisFrame = static_cast<int>((c * 0.03125f) * (mRenderPacket->mTexturingData->mLayerContainer.getNumLayers() * mCacheInfoPerLOD[nLODLevel].mNumTilesVisible));
}
*/
//---------------------------------------




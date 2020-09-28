//============================================================================
//
//  TerrainMetric.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "TerrainPCH.h"
#include "TerrainMetric.h"

long   BTerrainMetrics::mGPUMemCount=0;
long   BTerrainMetrics::mCacheMemGPUCount=0;
long   BTerrainMetrics::mTerrainGPUMemCount=0;
long   BTerrainMetrics::mAlphaTextureGPUCount =0;
long   BTerrainMetrics::mTesselationGPUCount = 0;
long   BTerrainMetrics::mCacheTextureGPUMemCount[cTextureTypeMax];
long   BTerrainMetrics::mArtistTerrainTextureGPUMem[cTextureTypeMax];

long   BTerrainMetrics::mCPUMemCount=0;
long   BTerrainMetrics::mCacheMemCPUCount=0;

long   BTerrainMetrics::mNumTrashes=0;
long   BTerrainMetrics::mNumVisibleQuadNodes=0;
long   BTerrainMetrics::mNumQuadNodesInTile[3];
long   BTerrainMetrics::mNumCompositeTextures[cNumMainCacheLevels];

long   BTerrainMetrics::mPrecomputedLightingGPUCount=0;
long   BTerrainMetrics::mUniqueAlbedoGPUCount=0;
long   BTerrainMetrics::mFoliageGPUCount=0;

long   BTerrainMetrics::mNumResolves=0;
long   BTerrainMetrics::mNumResolvedPixels=0;
long   BTerrainMetrics::mAristTexGPUCount =0;

//--------------------------------------------------
int BTerrainMetrics::getTotalGPUMem()
{
   return    mGPUMemCount;
}
//--------------------------------------------------
int BTerrainMetrics::getTotalCacheGPUMem()
{
   return mCacheMemGPUCount;
}
//--------------------------------------------------
int BTerrainMetrics::getTotalArtistsTexGPUMem()
{
   return mAristTexGPUCount;
}
//--------------------------------------------------
//general memory
void BTerrainMetrics::addCPUMem(int val)
{
#ifndef BUILD_FINAL 
   InterlockedExchangeAdd(&mCPUMemCount, val);   
#endif      
}

//--------------------------------------------------
void BTerrainMetrics::addTerrainGPUMem(int val)
{
#ifndef BUILD_FINAL   
   
   InterlockedExchangeAdd(&mTerrainGPUMemCount, val);  
   InterlockedExchangeAdd(&mGPUMemCount, val); 
#endif      
}
//--------------------------------------------------
void BTerrainMetrics::addCacheMemCPU(int val)
{
#ifndef BUILD_FINAL   
   InterlockedExchangeAdd(&mCacheMemCPUCount, val); 
   InterlockedExchangeAdd(&mCPUMemCount, val); 
#endif      
}
//--------------------------------------------------
void BTerrainMetrics::addResolve(int numPixels)
{
#ifndef BUILD_FINAL 
   InterlockedExchangeAdd(&mNumResolves, 1); 
   InterlockedExchangeAdd(&mNumResolvedPixels, numPixels); 
#endif 
}
//--------------------------------------------------
void BTerrainMetrics::addNumThrashes(int numThrashes)
{
#ifndef BUILD_FINAL 
    InterlockedExchangeAdd(&mNumTrashes, numThrashes);
#endif      
}
//--------------------------------------------------
void BTerrainMetrics::setNumVisibleNodes(int numNodes)
{
#ifndef BUILD_FINAL  
   InterlockedExchange(&mNumVisibleQuadNodes, numNodes);
#endif      
}
//--------------------------------------------------
void BTerrainMetrics::setQNVisInTile(int numNodes, int tileNum)
{
#ifndef BUILD_FINAL 
       InterlockedExchange(&mNumQuadNodesInTile[tileNum], numNodes);
#endif 
}
//--------------------------------------------------
void BTerrainMetrics::setNumCompositeTextures(int numTextures, int slotNum)
{
#ifndef BUILD_FINAL  
   InterlockedExchange(&mNumCompositeTextures[slotNum], numTextures);
#endif 
}
//--------------------------------------------------
void BTerrainMetrics::addTextureLayersGPUMem(int val)
{
#ifndef BUILD_FINAL   
   InterlockedExchangeAdd(&mAlphaTextureGPUCount, val);
   InterlockedExchangeAdd(&mGPUMemCount, val);   
#endif      
}
//--------------------------------------------------
void BTerrainMetrics::addTesselationGPUMem(int val)
{
#ifndef BUILD_FINAL   
   
   InterlockedExchangeAdd(&mTesselationGPUCount, val);
   InterlockedExchangeAdd(&mGPUMemCount, val);
#endif      
}
//--------------------------------------------------
void BTerrainMetrics::addCacheChannelGPUMem(int memCount, int channel)
{
#ifndef BUILD_FINAL 
   InterlockedExchangeAdd(&mCacheTextureGPUMemCount[channel], memCount);
   InterlockedExchangeAdd(&mGPUMemCount, memCount);
   InterlockedExchangeAdd(&mCacheMemGPUCount, memCount);
#endif 
}

//--------------------------------------------------
void BTerrainMetrics::addArtistTerrainTextureGPUMem(int memCount, int channel)
{
#ifndef BUILD_FINAL 
   InterlockedExchangeAdd(& mArtistTerrainTextureGPUMem[channel], memCount);
   InterlockedExchangeAdd(&mGPUMemCount, memCount);
   InterlockedExchangeAdd(&mAristTexGPUCount, memCount);
#endif 
}

//--------------------------------------------------
void BTerrainMetrics::addPrecomputedLightingGPU(int count)
{
#ifndef BUILD_FINAL 
   InterlockedExchangeAdd(&mPrecomputedLightingGPUCount, count);
   InterlockedExchangeAdd(&mGPUMemCount, count);
#endif      
}
//--------------------------------------------------
void BTerrainMetrics::addFoliageGPU(int count)
{
#ifndef BUILD_FINAL 
   InterlockedExchangeAdd(&mFoliageGPUCount , count);
   InterlockedExchangeAdd(&mGPUMemCount,count);
#endif      
}
//--------------------------------------------------
void BTerrainMetrics::addUniqueAlbedoGPU(int count)
{
#ifndef BUILD_FINAL 
   InterlockedExchangeAdd(&mUniqueAlbedoGPUCount , count);
   InterlockedExchangeAdd(&mGPUMemCount,count);
#endif      
}
//--------------------------------------------------
void BTerrainMetrics::clearFrame()
{
#ifndef BUILD_FINAL  
   InterlockedExchange(&mNumVisibleQuadNodes, 0);
   InterlockedExchange(&mNumTrashes, 0);
   InterlockedExchange(&mNumResolves, 0);
   InterlockedExchange(&mNumResolvedPixels, 0);

   for(int i=0;i<3;i++)
      InterlockedExchange(&mNumQuadNodesInTile[i], 0);

#endif      
}

//--------------------------------------------------
void BTerrainMetrics::clearAll()
{
   clearFrame();
#ifndef BUILD_FINAL  

   InterlockedExchange(&mCacheMemCPUCount,0);
   InterlockedExchange(&mCacheMemGPUCount,0);
   InterlockedExchange(&mTerrainGPUMemCount,0);
   InterlockedExchange(&mAlphaTextureGPUCount,0);
   InterlockedExchange(&mTesselationGPUCount,0);
   InterlockedExchange(&mGPUMemCount , 0);
   InterlockedExchange(&mCPUMemCount , 0);
   InterlockedExchange(&mAristTexGPUCount , 0);
   InterlockedExchange(&mFoliageGPUCount , 0);
   

   for(int i=0;i<cNumMainCacheLevels;i++)
      InterlockedExchange(&mNumCompositeTextures[i],0);

   for(int i=0;i<cTextureTypeMax;i++)
   {
      InterlockedExchange(&mCacheTextureGPUMemCount[i],0);
      InterlockedExchange(&mArtistTerrainTextureGPUMem[i],0);
   }

#endif  
}

//--------------------------------------------------

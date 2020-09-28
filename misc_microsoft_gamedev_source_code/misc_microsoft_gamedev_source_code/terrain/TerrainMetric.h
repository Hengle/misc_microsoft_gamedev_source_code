//============================================================================
//
//  TerrainMetric.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once
//terrain
#include "TerrainTexturing.h"

//render


// rg [2/1/06] - Looks like this class is or could be call from the main and worker threads. I can't tell, so for now I'm making it thread safe.
class BTerrainMetrics
{
public:
  
   //CPU MEM
   static void addCPUMem(int val);
   static void addCacheMemCPU(int val);

   //GPU MEM
   static int getTotalGPUMem();
   static void addTerrainGPUMem(int val);
   static void addTextureLayersGPUMem(int val);
   static void addTesselationGPUMem(int val);
   static void addArtistTerrainTextureGPUMem(int val, int channel);
   static void addUniqueAlbedoGPU(int count);
   static void addPrecomputedLightingGPU(int count);
   static void addFoliageGPU(int count);

   static int getTotalArtistsTexGPUMem();

   //CACHE STATS
   static int getTotalCacheGPUMem();
   static void addCacheChannelGPUMem(int memCount, int channel);
   static void addResolve(int numPixels);
   static void addNumThrashes(int numThrashes);
   static void setNumVisibleNodes(int numNodes);
   static void setQNVisInTile(int numNodes, int tileNum);
   static void setNumCompositeTextures(int numTextures, int slotNum);


   static void clearFrame();
   static void clearAll();


   //-------------------------------------------------
   
   static long    mTesselationGPUCount;
   static long    mTerrainGPUMemCount;
   static long    mAlphaTextureGPUCount;


   static long   mCPUMemCount;
   static long    mCacheMemCPUCount;

   static long    mPrecomputedLightingGPUCount;
   static long    mUniqueAlbedoGPUCount;
   static long    mFoliageGPUCount;

   //caches

   static long    mNumTrashes;
   static long    mNumVisibleQuadNodes;
   static long    mNumQuadNodesInTile[3];
   static long    mNumCompositeTextures[cNumMainCacheLevels];
   static long    mNumResolves;
   static long    mNumResolvedPixels;
   static long    mCacheTextureGPUMemCount[cTextureTypeMax];
   static long    mArtistTerrainTextureGPUMem[cTextureTypeMax];
private:
   static long   mGPUMemCount;
   static long    mCacheMemGPUCount;
   static long    mAristTexGPUCount;

};
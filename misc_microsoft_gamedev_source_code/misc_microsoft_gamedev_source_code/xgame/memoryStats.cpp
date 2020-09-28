// File: memoryStats.cpp
#include "common.h"

#ifndef BUILD_FINAL

#include <xbdm.h>
#include "globalObjects.h"
#include "memory\memoryHeap.h"
#include "memory\rockallHeapStats.h"
#include "particleHeap.h"
#include "xboxMemory.h"
#include "xboxTextureHeap.h"
#include "memory\dlmalloc.h"
#include "grannyManager.h"
#include "bfileStream.h"
#include "terrainMetric.h"
#include "database.h"
#include "gamesettings.h"
#include "xfs.h"
#include "archivemanager.h"
#include "stream\bufferStream.h"
#include "D3DTextureManager.h"
#include "particletexturemanager.h"
#include "flashmanager.h"
#include "debugTextDisplay.h"

#ifdef USE_SEPERATE_PARTICLE_HEAP   
   BMemoryHeap* gpRockallHeaps[] = { &gPrimaryHeap, &gSimHeap, &gNetworkHeap, &gStringHeap, &gRenderHeap, &gParticleHeap, &gParticleBlockHeap, &gFileBlockHeap, &gPhysCachedHeap, &gPhysWriteCombinedHeap, &gSyncHeap };
#else
   BMemoryHeap* gpRockallHeaps[] = { &gPrimaryHeap, &gSimHeap, &gNetworkHeap, &gStringHeap, &gRenderHeap, &gParticleBlockHeap, &gFileBlockHeap, &gPhysCachedHeap, &gPhysWriteCombinedHeap, &gSyncHeap };
#endif   

const uint cNumRockallHeaps = sizeof(gpRockallHeaps)/sizeof(gpRockallHeaps[0]);
// jce [11/19/2008] -- hack for external access.
uint cNumRockallHeapsEx = cNumRockallHeaps;

#endif // BUILD_FINAL

//==============================================================================
// printKernelMemoryStats
//==============================================================================
void printKernelMemoryStats(BConsoleOutput& consoleOutput)
{
#ifndef BUILD_FINAL
   consoleOutput.printf("---------------------");
   consoleOutput.printf("Kernel Memory Statistics");
   
   MEMORYSTATUS status;
   ZeroMemory(&status, sizeof(status));
   GlobalMemoryStatus(&status);

   const float MB = 1024U * 1024U;

   consoleOutput.printf(
      "GlobalMemoryStatus Total Allocated: %3.1fMB, Current Free: %3.1fMB, Initial Free: %3.1fMB",
      (gInitialKernelMemoryTracker.getInitialPhysicalFree() - status.dwAvailPhys) / MB,
      status.dwAvailPhys / MB,
      gInitialKernelMemoryTracker.getInitialPhysicalFree() / MB );

   DM_MEMORY_STATISTICS memStats;
   Utils::ClearObj(memStats);
   memStats.cbSize = sizeof(DM_MEMORY_STATISTICS);

   DmQueryTitleMemoryStatistics(&memStats);

   consoleOutput.printf("DmQueryTitleMemoryStatistics Total: %3.1f, Avail: %3.1f, Stack: %3.1f, VirtualPageTable: %3.1f, SystemPageTable: %3.1f",
      memStats.TotalPages*4096.0f/(1024.0f*1024.0f),
      memStats.AvailablePages*4096.0f/(1024.0f*1024.0f),
      memStats.StackPages*4096.0f/(1024.0f*1024.0f),
      memStats.VirtualPageTablePages*4096.0f/(1024.0f*1024.0f),
      memStats.SystemPageTablePages*4096.0f/(1024.0f*1024.0f));

   consoleOutput.printf("Pool: %3.1f, VirtualMapped: %3.1f, Image: %3.1f, FileCache: %3.1f, Contiguous: %3.1f, Debugger: %3.1f",
      memStats.PoolPages*4096.0f/(1024.0f*1024.0f),
      memStats.VirtualMappedPages*4096.0f/(1024.0f*1024.0f),
      memStats.ImagePages*4096.0f/(1024.0f*1024.0f),
      memStats.FileCachePages*4096.0f/(1024.0f*1024.0f),
      memStats.ContiguousPages*4096.0f/(1024.0f*1024.0f),
      memStats.DebuggerPages*4096.0f/(1024.0f*1024.0f));
#endif      
}

//==============================================================================
// printHeapStats
//==============================================================================
void printHeapStats(BConsoleOutput& consoleOutput, bool progressiveUpdate)
{
#ifndef BUILD_FINAL
   consoleOutput.printf("---------------------");
      
   BMemoryHeapStats totalStats("Totals");

   static BRockallHeapStats rockallHeapStats[cNumRockallHeaps];
   static uint prevRockallStatUpdateTime;
   static uint nextHeapToUpdate;

   const uint curTime = GetTickCount();
   const uint timeSinceUpdate = curTime - prevRockallStatUpdateTime;

   uint heapToUpdateStart = 0;
   uint heapToUpdateEnd = cNumRockallHeaps;
   
   if (progressiveUpdate)
   {
      if (timeSinceUpdate > 500)
      {
         prevRockallStatUpdateTime = curTime;

         heapToUpdateStart = nextHeapToUpdate;
         heapToUpdateEnd = nextHeapToUpdate + 1;
         
         if (++nextHeapToUpdate == cNumRockallHeaps)
            nextHeapToUpdate = 0;
      }
   }
   
   for (uint i = heapToUpdateStart; i < heapToUpdateEnd; i++)
   {
      if (gpRockallHeaps[i]->getHeapPtr())
      {
         gpRockallHeaps[i]->claimLock();

         bool computeUtilization = false;
#if TRACK_HEAP_UTILIZATION
         computeUtilization = true;
#endif         

         rockallHeapStats[i].update(gpRockallHeaps[i]->getHeapPtr(), false, computeUtilization);

         gpRockallHeaps[i]->releaseLock();
      }
   }

   consoleOutput.printf("Rockall heap statistics:");

   uint64 totalFreeBytes = 0;
   uint64 totalAllocBytes = 0;

   for (uint i = 0; i < cNumRockallHeaps; i++)
   {
      BRockallHeapStats& stats = rockallHeapStats[i];

      if (gpRockallHeaps[i]->getMSpace())
      {
#if 0      
         MALLINFO_FIELD_TYPE arena;    /* non-mmapped space allocated from system */
         MALLINFO_FIELD_TYPE ordblks;  /* number of free chunks */
         MALLINFO_FIELD_TYPE smblks;   /* footprint */
         MALLINFO_FIELD_TYPE hblks;    /* always 0 */
         MALLINFO_FIELD_TYPE hblkhd;   /* space in mmapped regions */
         MALLINFO_FIELD_TYPE usmblks;  /* maximum total allocated space */
         MALLINFO_FIELD_TYPE fsmblks;  /* always 0 */
         MALLINFO_FIELD_TYPE uordblks; /* total allocated space */
         MALLINFO_FIELD_TYPE fordblks; /* total free space */
         MALLINFO_FIELD_TYPE keepcost; /* releasable (via malloc_trim) space */
#endif   
                  
         struct mallinfo m;
         m = mspace_mallinfo((mspace)gpRockallHeaps[i]->getMSpace());
         
         consoleOutput.printf("%s [dl]: Footprint: %u, NonMMappedSpace: %u, FreeChunks: %u, SpaceInMMappedRegions: %u", 
            gpRockallHeaps[i]->getStats().mName, 
            m.smblks, 
            m.arena,
            m.ordblks,
            m.hblkhd);
            
         consoleOutput.printf("  MaxTotalAllocated: %u, TotalAllocated: %u, TotalFree: %u, Releasable: %u",
            m.usmblks,
            m.uordblks,
            m.fordblks,
            m.keepcost);
      }
      else if (stats.mFailed)
      {
#ifndef BUILD_FINAL
         consoleOutput.printf("%s: Heap walk FAILED! Heap may be corrupted!", gpRockallHeaps[i]->getStats().mName);
#else
         consoleOutput.printf("Heap walk FAILED! Heap may be corrupted!" );
#endif
      }
      else
      {
         totalFreeBytes += stats.mTotalFreeBytes;
         totalAllocBytes += stats.mTotalUsedBytes;            

#ifndef BUILD_FINAL
         consoleOutput.printf("%s: TotalPages: %u NumFreeBlocks: %u TotalFreeBytes: %u NumUsedBlocks: %u TotalUsedBytes: %u",
            gpRockallHeaps[i]->getStats().mName, 
            stats.mTotalPages,
            stats.mNumFreeBlocks,
            stats.mTotalFreeBytes,
            stats.mNumUsedBlocks,
            stats.mTotalUsedBytes);
#else
         consoleOutput.printf("TotalPages: %u NumFreeBlocks: %u TotalFreeBytes: %u NumUsedBlocks: %u TotalUsedBytes: %u",            
            stats.mTotalPages,
            stats.mNumFreeBlocks,
            stats.mTotalFreeBytes,
            stats.mNumUsedBlocks,
            stats.mTotalUsedBytes);
#endif
         
#if TRACK_HEAP_UTILIZATION
         consoleOutput.printf("  MaxFreeBlockBytes: %u MaxUsedBlockBytes: %u",
            stats.mMaxFreeBlockBytes,
            stats.mMaxUsedBlockBytes);

         consoleOutput.printf("  TotalUsedBlockZeroBytes: %u, AveBitsPerByteUtilized: %f", 
            stats.mTotalUsedBlockZeroBytes,
            stats.mAveBitsUtilizedPerByte);
#endif
      }                   
   }
   
   consoleOutput.printf("Rockall heap total free bytes: %I64u, Total alloc bytes: %I64u, Total free+alloc: %I64u", totalFreeBytes, totalAllocBytes, totalFreeBytes + totalAllocBytes);

#if DYNAMIC_ARRAY_TRACKING
   BDynamicArrayTracker::BStats arrayStats;
   BDynamicArrayTracker::getStatistics(arrayStats);

   consoleOutput.printf("Total BDynamicArray's: %u", arrayStats.mTotalArrays);
   consoleOutput.printf("  Total Used Elements: %u, Total Allocated Elements: %u", arrayStats.mTotalSize, arrayStats.mTotalCapacity);
   consoleOutput.printf("  Total Used Bytes: %u, Total Allocated Bytes: %u", arrayStats.mTotalSizeInBytes, arrayStats.mTotalCapacityInBytes);
#endif      

#if FREELIST_TRACKING
   BFreeListTracker::BStats freeListStats;
   BFreeListTracker::getStatistics(freeListStats, NULL);

   consoleOutput.printf("Total BFreeLists's: %u", freeListStats.mTotalFreeLists);
   consoleOutput.printf("  Elements Allocated: %u, Elements Acquired: %u", freeListStats.mTotalElementsAllocated, freeListStats.mTotalElementsAcquired);
   consoleOutput.printf("  Bytes Allocated: %u, Bytes Acquired: %u", freeListStats.mTotalBytesAllocated, freeListStats.mTotalBytesAcquired);
#endif      
   
#endif   
}

//==============================================================================
// printDetailedHeapStats
//==============================================================================
void printDetailedHeapStats(BConsoleOutput& consoleOutput, bool totals, bool deltas)
{
#ifndef BUILD_FINAL      
   consoleOutput.printf("---------------------");
   consoleOutput.printf(deltas ? "Delta Heap Usage Statistics" : "Heap Usage Statistics");
   
   BMemoryHeapStats totalStats("Totals");

   const BMemoryHeapStats* const pHeapStats[] = 
   { 
      &gPrimaryHeap.getStats(), 
      &gSimHeap.getStats(), 
      &gNetworkHeap.getStats(),
      &gStringHeap.getStats(),
      &gRenderHeap.getStats(), 
#ifdef USE_SEPERATE_PARTICLE_HEAP      
      &gParticleHeap.getStats(), 
#endif      
      &gParticleBlockHeap.getStats(),
      &gFileBlockHeap.getStats(),
      &gPhysCachedHeap.getStats(),
      &gPhysWriteCombinedHeap.getStats(),
      &gSyncHeap.getStats(),
      
    //  &mNewDeleteStats,
   //   &mXMemCachedHeapStats,
      &mXMemPhysicalHeapStats,
      &gXPhysicalAllocStats,
      &totalStats
   };

   const uint cNumHeaps = sizeof(pHeapStats)/sizeof(pHeapStats[0]);
   static BMemoryHeapStats mPrevStats[cNumHeaps];

   for (uint heapIndex = 0; heapIndex < cNumHeaps - 1; heapIndex++)
   {
      const BMemoryHeapStats& stats = *pHeapStats[heapIndex];

      // new/delete and XMemAlloc cached allocations are actually passed through to the render heap, so exclude them from the totals.
      if ((pHeapStats[heapIndex] != &mNewDeleteStats) && (pHeapStats[heapIndex] != &mXMemCachedHeapStats))
         totalStats.sum(stats);
   }
   
   if (totals)
   {
      for (uint heapIndex = 0; heapIndex < cNumHeaps; heapIndex++)
      {
         const BMemoryHeapStats& stats = *pHeapStats[heapIndex];

         consoleOutput.printf(
            "%s: CurNumAllocs: %i, CurAllocBytes: %i, MaxNumAllocs: %i, MaxAllocBytes: %i",
            stats.mName,
            stats.mCurrentAllocations,
            stats.mCurrentAllocationSize,
            stats.mMostAllocations,
            stats.mMostAllocationSize);

         consoleOutput.printf(" TotalNews: %i, TotalDeletes: %i TotalReallocs: %i, TotalAllocBytes: %I64i",
            stats.mTotalNews,
            stats.mTotalDeletes,
            stats.mTotalReallocations,
            stats.mTotalAllocationSize);
            
         consoleOutput.printf(" Locks:%u Time:%3.3fms Ave:%3.3fus Unknown:%u Sim:%u SimHelper:%u Rndr:%u RndHelper:%u IO:%u Misc:%u",
            stats.mTotalLocks, stats.mTotalLockTime * 1000.0f, stats.mTotalLocks ? (stats.mTotalLockTime * 1000000.0f / stats.mTotalLocks) : 0.0f,
            stats.mThreadHist[0],
            stats.mThreadHist[1+cThreadIndexSim],
            stats.mThreadHist[1+cThreadIndexSimHelper],
            stats.mThreadHist[1+cThreadIndexRender],
            stats.mThreadHist[1+cThreadIndexRenderHelper],
            stats.mThreadHist[1+cThreadIndexIO],
            stats.mThreadHist[1+cThreadIndexMisc]);
      }
   }
   
   if (deltas)
   {
      for (uint heapIndex = 0; heapIndex < cNumHeaps; heapIndex++)
      {
         const BMemoryHeapStats& stats = *pHeapStats[heapIndex];

         BMemoryHeapStats deltaStats(stats);
         deltaStats.createDelta(mPrevStats[heapIndex]);
         mPrevStats[heapIndex] = stats;

         consoleOutput.printf(
            "%s: CurNumAllocs: %i, CurAllocBytes: %i, MaxNumAllocs: %i, MaxAllocBytes: %i",
            stats.mName,
            deltaStats.mCurrentAllocations,
            deltaStats.mCurrentAllocationSize,
            deltaStats.mMostAllocations,
            deltaStats.mMostAllocationSize);

         consoleOutput.printf(" TotalNews: %i, TotalDeletes: %i TotalReallocs: %i, TotalAllocBytes: %I64i",
            deltaStats.mTotalNews,
            deltaStats.mTotalDeletes,
            deltaStats.mTotalReallocations,
            deltaStats.mTotalAllocationSize);
            
         consoleOutput.printf(" Locks:%u Time:%3.3fms Ave:%3.3fus Unknown:%u Sim:%u SimHelper:%u Rndr:%u RndHelper:%u IO:%u Misc:%u",
            deltaStats.mTotalLocks, deltaStats.mTotalLockTime * 1000.0f, deltaStats.mTotalLocks ? (deltaStats.mTotalLockTime * 1000000.0f / deltaStats.mTotalLocks) : 0.0f,
            deltaStats.mThreadHist[0],
            deltaStats.mThreadHist[1+cThreadIndexSim],
            deltaStats.mThreadHist[1+cThreadIndexSimHelper],
            deltaStats.mThreadHist[1+cThreadIndexRender],
            deltaStats.mThreadHist[1+cThreadIndexRenderHelper],
            deltaStats.mThreadHist[1+cThreadIndexIO],
            deltaStats.mThreadHist[1+cThreadIndexMisc]);
      }
   }
#endif   
}

//==============================================================================
// printDetailedHeapStats
//==============================================================================
void dumpQuickCompareStats(const char* file, long line)
{
#ifndef BUILD_FINAL      
 

   float cTotalVirtualAllocs = (float) (gPrimaryHeap.getStats().mCurrentAllocationSize +
                                    gSimHeap.getStats().mCurrentAllocationSize +
                                    gNetworkHeap.getStats().mCurrentAllocationSize +
                                    gStringHeap.getStats().mCurrentAllocationSize +
                                    gRenderHeap.getStats().mCurrentAllocationSize + 
                                    gParticleBlockHeap.getStats().mCurrentAllocationSize + 
                                    gFileBlockHeap.getStats().mCurrentAllocationSize + 
                                    gSyncHeap.getStats().mCurrentAllocationSize );


   float cTotalPhysicalAllocs =  (float) (gPhysCachedHeap.getStats().mCurrentAllocationSize +
                                       gPhysWriteCombinedHeap.getStats().mCurrentAllocationSize +
                                       mXMemPhysicalHeapStats.mCurrentAllocationSize +
                                       gXPhysicalAllocStats.mCurrentAllocationSize );




   BFixedString<256> buf;

   DM_MEMORY_STATISTICS memStats;
   Utils::ClearObj(memStats);
   memStats.cbSize = sizeof(DM_MEMORY_STATISTICS);

   DmQueryTitleMemoryStatistics(&memStats);


   buf.format("%s(%d): KVirtualAlloc:%fMB, KPhysicalAllc:%fMB | HVirtualAlloc:%fMB, HPhysicalAlloc:%fMB\n", 
      file, line, 
      memStats.VirtualMappedPages*4096.0f/(1024.0f*1024.0f),
      memStats.ContiguousPages*4096.0f/(1024.0f*1024.0f),
      cTotalVirtualAllocs/(1024.0f*1024.0f), 
      cTotalPhysicalAllocs/(1024.0f*1024.0f)
      );

   OutputDebugStringA(buf);    

#endif   
}

//==============================================================================
// printXboxTextureHeapStats
//==============================================================================
void printXboxTextureHeapStats(BConsoleOutput& consoleOutput)
{
#ifndef BUILD_FINAL
   if (!gpXboxTextureHeap)
      return;
   
   consoleOutput.printf("Xbox Texture Heap");
   consoleOutput.printf("");
   
   BTimer timer;
   timer.start();
   
   BXboxTextureHeap::BValleyStats valleyStats;
   gpXboxTextureHeap->getValleyStats(valleyStats);
   
   BXboxTextureHeap::BHeapStats heapStats;
   gpXboxTextureHeap->getHeapStats(heapStats);
   
   double totalTime = timer.getElapsedSeconds();
   consoleOutput.printf("Total statistics gathering time: %3.3fms", totalTime * 1000.0f);
   
   consoleOutput.printf("GPU Valley Stats:");
   consoleOutput.printf("         NumValleyClasses: %u", valleyStats.mNumValleyClasses);
   consoleOutput.printf("               NumValleys: %u", valleyStats.mNumValleys);

   consoleOutput.printf("             NumAvailable: %u", valleyStats.mNumAvailable);
   consoleOutput.printf("      TotalAvailableBytes: %u", valleyStats.mTotalAvailableBytes);

   consoleOutput.printf("              NumOccupied: %u", valleyStats.mNumOccupied);
   consoleOutput.printf("       TotalOccupiedBytes: %u", valleyStats.mTotalOccupiedBytes);

   consoleOutput.printf("      TotalAllocatedBytes: %u", valleyStats.mTotalAllocatedBytes);
   consoleOutput.printf("         TotalRegionBytes: %u", valleyStats.mTotalRegionBytes);
   consoleOutput.printf("        LargestRegionSize: %u", valleyStats.mLargestRegionSize);
   
   consoleOutput.printf("     TotalReservedValleys: %u", valleyStats.mTotalReserved);
   consoleOutput.printf("     TotalLongTermValleys: %u", valleyStats.mTotalLongTerm);
   consoleOutput.printf("  TotalUsingUnusedRegions: %u", valleyStats.mTotalUsingUnusedRegions);
   consoleOutput.printf("TotalOKToUseUnusedRegions: %u", valleyStats.mTotalOKToUseUnusedRegions);
   
   consoleOutput.printf("");
   
   consoleOutput.printf("CPU Heap Stats:\n");
   consoleOutput.printf("         TotalRegionBytes: %u", heapStats.mTotalRegionBytes);
   consoleOutput.printf("          TotalOperations: %u", heapStats.mTotalOperations);
   consoleOutput.printf("     DesignatedVictimSize: %u", heapStats.mDVSize);
   consoleOutput.printf("      TotalNumAllocations: %u", heapStats.mTotalNumAllocations);
   consoleOutput.printf("      TotalBytesAllocated: %u", heapStats.mTotalBytesAllocated);
   consoleOutput.printf("           TotalBytesFree: %u", heapStats.mTotalBytesFree);
   consoleOutput.printf("          TotalFreeChunks: %u", heapStats.mTotalFreeChunks);
   consoleOutput.printf("         LargestFreeChunk: %u", heapStats.mLargestFreeChunk);
   consoleOutput.printf("        TotalNonEmptyBins: %u", heapStats.mTotalBins);
   consoleOutput.printf("        TotalHeapOverhead: %u", heapStats.mTotalOverhead);
#endif
}

void dumpRockallStats()
{
#ifndef BUILD_FINAL   
   DmMapDevkitDrive();
   
   for (uint i = 0; i < cNumRockallHeaps; i++)
   {
      ROCKALL_FRONT_END* pRockallHeap = gpRockallHeaps[i]->getHeapPtr();
      
      if (pRockallHeap)
      {
         gpRockallHeaps[i]->claimLock();
         
         pRockallHeap->PrintMemStatistics();
         
         gpRockallHeaps[i]->releaseLock();
         
         gConsoleOutput.printf("Printed heap at 0x%08X name %s", pRockallHeap, gpRockallHeaps[i]->getStats().mName);
      }
   }
#endif
}

#ifndef BUILD_FINAL
//==============================================================================
// dumpTerrainStats
//==============================================================================
static void dumpTerrainStats(BBufferStream& outputStream)
{
   outputStream.printf("Terrain Memory\n");

   float mem = (BTerrainMetrics::getTotalGPUMem() + BTerrainMetrics::mCPUMemCount) / float(1024*1024);
   outputStream.printf("\"  Total CPU+GPU\", %4.3f\n", mem);

   mem = BTerrainMetrics::mCPUMemCount / float(1024*1024);
   outputStream.printf("\"   Total CPU\", %4.3f\n", mem);

   mem = BTerrainMetrics::getTotalGPUMem() / float(1024*1024);
   outputStream.printf("\"   Total GPU\", %4.3f\n", mem);

   outputStream.printf("\n");

   outputStream.printf("\"  GPU\", %4.3f\n", BTerrainMetrics::getTotalGPUMem());

   mem = BTerrainMetrics::mTerrainGPUMemCount / float(1024*1024);
   outputStream.printf("\"     Vertex/Normal/AO/Alpha GPU\", %4.3f\n", mem);

   mem = BTerrainMetrics::mAlphaTextureGPUCount / float(1024*1024);
   outputStream.printf("\"     Terrain Blend Textures GPU\", %4.3f\n", mem);

   mem = BTerrainMetrics::mUniqueAlbedoGPUCount / float(1024*1024);
   outputStream.printf("\"     Unique Albedo Texture GPU\", %4.3f\n", mem);

   mem = BTerrainMetrics::mFoliageGPUCount / float(1024*1024);
   outputStream.printf("\"     Foliage Data GPU\", %4.3f\n", mem);

   mem = BTerrainMetrics::mTesselationGPUCount / float(1024*1024);
   outputStream.printf("\"     Tessellation LOD GPU\", %4.3f\n", mem);

   mem = BTerrainMetrics::mPrecomputedLightingGPUCount / float(1024*1024);
   outputStream.printf("\"     Precomputed Lighting GPU\", %4.3f\n", mem);

   mem = BTerrainMetrics::getTotalCacheGPUMem() / float(1024*1024);      
   outputStream.printf("\"   Cache Textures\", %4.3f\n", mem);
   for(int i=0;i<cTextureTypeMax;i++)
   {
      mem = BTerrainMetrics::mCacheTextureGPUMemCount[i] / float(1024*1024);
      outputStream.printf("\"        TextureType %i GPU\", %4.3f\n",i, mem);
   }

   mem = BTerrainMetrics::getTotalArtistsTexGPUMem() / float(1024*1024);
   outputStream.printf("\"     Artist Textures\", %4.3f\n",mem);
   for(int i=0;i<cTextureTypeMax;i++)
   {
      if(BTerrainMetrics::mArtistTerrainTextureGPUMem[i]==0)
         mem=0;
      else
         mem = BTerrainMetrics::mArtistTerrainTextureGPUMem[i] / float(1024*1024);
      outputStream.printf("\"       TextureType %i GPU\", %4.3f\n",i, mem);
   }


   mem = BTerrainMetrics::mCacheMemCPUCount / float(1024*1024);
   outputStream.printf("\"  Cache CPU\", %4.3f\n", mem);
   outputStream.printf("\n");
}

//==============================================================================
// getModelTextureStats
//==============================================================================
static void getModelTextureStats(void* pData)
{
   BD3DTextureManager::BTextureAllocStatsArray* pTextureStats = (BD3DTextureManager::BTextureAllocStatsArray*)(pData);
   gD3DTextureManager.getTextureAllocStats(*pTextureStats, BD3DTextureManager::cUGXMaterial);
}

//==============================================================================
// getFlashTextureStats
//==============================================================================
static void getFlashTextureStats(void* pData)
{
   BD3DTextureManager::BTextureAllocStatsArray* pTextureStats = (BD3DTextureManager::BTextureAllocStatsArray*)(pData);
   gD3DTextureManager.getTextureAllocStats(*pTextureStats, BD3DTextureManager::cScaleformCommon | BD3DTextureManager::cScaleformPreGame | BD3DTextureManager::cScaleformInGame);
}

//==============================================================================
// getUITextureStats
//==============================================================================
static void getMiscTextureStats(void* pData)
{
   BD3DTextureManager::BTextureAllocStatsArray* pTextureStats = (BD3DTextureManager::BTextureAllocStatsArray*)(pData);
   
   gD3DTextureManager.getTextureAllocStats(*pTextureStats, 
      (DWORD)~(BD3DTextureManager::cUGXMaterial | BD3DTextureManager::cScaleformCommon | BD3DTextureManager::cScaleformPreGame | BD3DTextureManager::cScaleformInGame) );
}

//==============================================================================
// getParticleTextureStats
//==============================================================================
static void getParticleTextureStats(void* pData)
{
   BParticleTextureManager::BTextureAllocStatsArray* pParticleTextureStats = (BParticleTextureManager::BTextureAllocStatsArray*)(pData);

   gPSTextureManager.getTextureAllocStats(*pParticleTextureStats);
}

//==============================================================================
// getFlashFontCacheConfig
//==============================================================================
static void getFlashFontCacheConfig(void* pData)
{
   GFxFontCacheManager::TextureConfig* pStats = (GFxFontCacheManager::TextureConfig*)(pData);
   gFlashManager.getFontCacheConfig(*pStats);   
}

//==============================================================================
// getFlashProtoFileStats
//==============================================================================
static void getFlashProtoFileStats(void* pData)
{
   BFlashManager::BFlashAssetAllocStatsArray* pStats = (BFlashManager::BFlashAssetAllocStatsArray*)(pData);
   gFlashManager.getProtoAllocStats(*pStats);   
}

//==============================================================================
// getFlashInstanceFileStats
//==============================================================================
static void getFlashInstanceFileStats(void* pData)
{
   BFlashManager::BFlashAssetAllocStatsArray* pStats = (BFlashManager::BFlashAssetAllocStatsArray*)(pData);
   gFlashManager.getInstanceAllocStats(*pStats);   
}


//==============================================================================
// dumpHeapStats
//==============================================================================
void dumpHeapStats(BBufferStream& outputStream)
{
#ifndef BUILD_FINAL      

   outputStream.printf("\n\n\nHeap Stats:\n");
   outputStream.printf("Primary Heap, %f\n",gPrimaryHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f));
   outputStream.printf("Sim Heap, %f\n",gSimHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f));
   outputStream.printf("gNetworkHeap Heap, %f\n",gNetworkHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f));
   outputStream.printf("gStringHeap Heap, %f\n",gStringHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f));
   outputStream.printf("gRenderHeap Heap, %f\n",gRenderHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f)); 
   outputStream.printf("gParticleBlockHeap Heap, %f\n",gParticleBlockHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f));
   outputStream.printf("gFileBlockHeap Heap, %f\n",gFileBlockHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f));
   outputStream.printf("gSyncHeap Heap, %f\n",gSyncHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f));


   outputStream.printf("gPhysCachedHeap Heap, %f\n",gPhysCachedHeap.getStats().mCurrentAllocationSize /(1024.0f*1024.0f));
   outputStream.printf("gPhysWriteCombinedHeap Heap, %f\n",gPhysWriteCombinedHeap.getStats().mCurrentAllocationSize /(1024.0f*1024.0f));
   outputStream.printf("mXMemPhysicalHeapStats Heap, %f\n",mXMemPhysicalHeapStats.mCurrentAllocationSize /(1024.0f*1024.0f));
   outputStream.printf("gXPhysicalAllocStats Heap, %f\n",gXPhysicalAllocStats.mCurrentAllocationSize /(1024.0f*1024.0f));



   const float cTotalVirtualAllocs = (float) (gPrimaryHeap.getStats().mCurrentAllocationSize +
      gSimHeap.getStats().mCurrentAllocationSize +
      gNetworkHeap.getStats().mCurrentAllocationSize +
      gStringHeap.getStats().mCurrentAllocationSize +
      gRenderHeap.getStats().mCurrentAllocationSize + 
      gParticleBlockHeap.getStats().mCurrentAllocationSize + 
      gFileBlockHeap.getStats().mCurrentAllocationSize + 
      gSyncHeap.getStats().mCurrentAllocationSize );


   const float cTotalPhysicalAllocs =  (float) (gPhysCachedHeap.getStats().mCurrentAllocationSize +
      gPhysWriteCombinedHeap.getStats().mCurrentAllocationSize +
      mXMemPhysicalHeapStats.mCurrentAllocationSize +
      gXPhysicalAllocStats.mCurrentAllocationSize );



   outputStream.printf("\n\n\nDMQuery Page Info:\n");
   DM_MEMORY_STATISTICS memStats;
   Utils::ClearObj(memStats);
   memStats.cbSize = sizeof(DM_MEMORY_STATISTICS);

   DmQueryTitleMemoryStatistics(&memStats);
   outputStream.printf("Total, %3.1f\n Avail, %3.1f\n Stack, %3.1f\n VirtualPageTable, %3.1f\n SystemPageTable, %3.1f\n",
      memStats.TotalPages*4096.0f/(1024.0f*1024.0f),
      memStats.AvailablePages*4096.0f/(1024.0f*1024.0f),
      memStats.StackPages*4096.0f/(1024.0f*1024.0f),
      memStats.VirtualPageTablePages*4096.0f/(1024.0f*1024.0f),
      memStats.SystemPageTablePages*4096.0f/(1024.0f*1024.0f));

   outputStream.printf("Pool, %3.1f\n VirtualMapped, %3.1f\n Image, %3.1f\n FileCache, %3.1f\n Contiguous, %3.1f\n Debugger: %3.1f",
      memStats.PoolPages*4096.0f/(1024.0f*1024.0f),
      memStats.VirtualMappedPages*4096.0f/(1024.0f*1024.0f),
      memStats.ImagePages*4096.0f/(1024.0f*1024.0f),
      memStats.FileCachePages*4096.0f/(1024.0f*1024.0f),
      memStats.ContiguousPages*4096.0f/(1024.0f*1024.0f),
      memStats.DebuggerPages*4096.0f/(1024.0f*1024.0f));

   


#endif   
}


//==============================================================================
// dumpModelStats
//==============================================================================
static void dumpModelAndFlashStats(BBufferStream& outputStream)
{
   BGrannyManager::BAssetAllocStatsArray modelStats;    
   BGrannyManager::BAssetAllocStatsArray animStats;    
   BD3DTextureManager::BTextureAllocStatsArray modelTextureStats;
   BD3DTextureManager::BTextureAllocStatsArray flashTextureStats;
   BParticleTextureManager::BTextureAllocStatsArray particleTextureStats;

   gGrannyManager.getModelAllocStats(modelStats);   
   gGrannyManager.getAnimAllocStats(animStats);

   gRenderThread.submitCallback(getModelTextureStats, &modelTextureStats);
   gRenderThread.submitCallback(getFlashTextureStats, &flashTextureStats);
   gRenderThread.submitCallback(getParticleTextureStats, &particleTextureStats);

   gRenderThread.blockUntilWorkerIdle();

   uint totalModelTextureAllocSize = 0;
   for (uint i = 0; i < modelTextureStats.getSize(); i++)
      totalModelTextureAllocSize += modelTextureStats[i].mActualAllocationSize;

   uint totalFlashTextureAllocSize = 0;
   for (uint i = 0; i < flashTextureStats.getSize(); i++)
      totalFlashTextureAllocSize += flashTextureStats[i].mActualAllocationSize;

   uint totalModelAllocSize = 0;
   for (uint i = 0; i < modelStats.getSize(); i++)
      totalModelAllocSize += modelStats[i].mAllocationSize;

   uint totalAnimAllocSize = 0;
   for (uint i = 0; i < modelStats.getSize(); i++)
      totalAnimAllocSize += animStats[i].mAllocationSize;
   uint totalParticleAllocSize = 0;
   for (uint i = 0; i < particleTextureStats.getSize(); i++)
      totalParticleAllocSize += particleTextureStats[i].mAllocationSize;

   const float MB = 1024.0f * 1024.0f;   

   outputStream.printf("\"Total Flash Textures\", %3.3f\n", totalFlashTextureAllocSize / MB);
   outputStream.printf("\"Total Particle Texture Arrays\", %3.3f\n", totalParticleAllocSize / MB);
   outputStream.printf("\n");
   outputStream.printf("\"Total Model Assets\", %3.3f\n", (totalAnimAllocSize + totalModelAllocSize + totalModelTextureAllocSize) / MB);
   outputStream.printf("\"  Textures\", %3.3f\n", totalModelTextureAllocSize / MB);
   outputStream.printf("\"  Geom\", %3.3f\n", totalModelAllocSize / MB);
   outputStream.printf("\"  Anim\", %3.3f\n", totalAnimAllocSize / MB);

   modelStats.sort();
   animStats.sort();
   modelTextureStats.sort();
   flashTextureStats.sort();
   particleTextureStats.sort();

   outputStream.printf("\n");      
   outputStream.printf("Geom Texture Assets (%u), Size, Res, Levels, ArraySize, Format\n", modelTextureStats.getSize());
   for (uint i = 0; i < modelTextureStats.getSize(); i++)
   {
      outputStream.printf("\"   %s\", %3.3f, %ux%u, %u, %u, %s\n", 
         modelTextureStats[i].mFilename.getPtr(), 
         modelTextureStats[i].mActualAllocationSize / MB,
         modelTextureStats[i].mWidth, modelTextureStats[i].mHeight, modelTextureStats[i].mLevels, modelTextureStats[i].mArraySize,
         getDDXDataFormatString(modelTextureStats[i].mDDXFormat) );
   }

   outputStream.printf("\n");
   outputStream.printf("Geom Assets (%u)\n", modelStats.getSize());
   for (uint i = 0; i < modelStats.getSize(); i++)
      outputStream.printf("\"   %s\", %3.3f\n", modelStats[i].mFilename.getPtr(), modelStats[i].mAllocationSize / MB);

   outputStream.printf("\n");
   outputStream.printf("Anim Assets (%u)\n", animStats.getSize());
   for (uint i = 0; i < animStats.getSize(); i++)
      outputStream.printf("\"   %s\", %3.3f\n", animStats[i].mFilename.getPtr(), animStats[i].mAllocationSize / MB);

   outputStream.printf("\n");
   outputStream.printf("Flash Texture Assets (%u), Size, Res, Levels, ArraySize, Format\n", flashTextureStats.getSize());
   for (uint i = 0; i < flashTextureStats.getSize(); i++)
   {
      outputStream.printf("\"   %s\", %3.3f, %ux%u, %u, %u, %s\n", 
         flashTextureStats[i].mFilename.getPtr(), 
         flashTextureStats[i].mActualAllocationSize / MB,
         flashTextureStats[i].mWidth, flashTextureStats[i].mHeight, flashTextureStats[i].mLevels, flashTextureStats[i].mArraySize,
         getDDXDataFormatString(flashTextureStats[i].mDDXFormat) );
   }
   
   outputStream.printf("\n");
   outputStream.printf("Particle Texture Array Summary (%u), Size, Res, Levels, ArraySize, Used, Format\n", particleTextureStats.getSize());
   for (uint i = 0; i < particleTextureStats.getSize(); i++)
   {
      outputStream.printf("\"   %s\", %3.3f, %ux%u, %u, %u, %u, %s\n", 
         particleTextureStats[i].mFilenames.getSize() ? particleTextureStats[i].mFilenames[0].getPtr() : "?",
         particleTextureStats[i].mAllocationSize / MB,
         particleTextureStats[i].mWidth, particleTextureStats[i].mHeight, particleTextureStats[i].mLevels, 
         particleTextureStats[i].mArraySize, particleTextureStats[i].mFilenames.getSize(),
         getDDXDataFormatString(particleTextureStats[i].mDDXFormat) );
      for (uint j = 1; j < particleTextureStats[i].mFilenames.getSize(); j++)
      {
         outputStream.printf("\"   %s\"\n", particleTextureStats[i].mFilenames[j].getPtr());
      }
      outputStream.printf("\n");
   }
}

//==============================================================================
// dumpMiscTextureStats
//==============================================================================
static void dumpMiscTextureStats(BBufferStream& outputStream)
{
   BD3DTextureManager::BTextureAllocStatsArray miscTextureStats;
   
   gRenderThread.submitCallback(getMiscTextureStats, &miscTextureStats);
   
   gRenderThread.blockUntilWorkerIdle();
 
   uint totalMiscTextureAllocSize = 0;
   for (uint i = 0; i < miscTextureStats.getSize(); i++)
      totalMiscTextureAllocSize += miscTextureStats[i].mActualAllocationSize;  
   
   const float MB = 1024.0f * 1024.0f;   
   
   outputStream.printf("\"Total Misc Textures\", %3.3f\n", totalMiscTextureAllocSize / MB);

   miscTextureStats.sort();
   
   outputStream.printf("Misc Texture Assets (%u), Size, Res, Levels, ArraySize, Format, Manager\n", miscTextureStats.getSize());
   for (uint i = 0; i < miscTextureStats.getSize(); i++)
   {
      outputStream.printf("\"   %s\", %3.3f, %ux%u, %u, %u, %s, %s\n", 
         miscTextureStats[i].mFilename.getPtr(), 
         miscTextureStats[i].mActualAllocationSize / MB,
         miscTextureStats[i].mWidth, miscTextureStats[i].mHeight, miscTextureStats[i].mLevels, miscTextureStats[i].mArraySize,
         getDDXDataFormatString(miscTextureStats[i].mDDXFormat), miscTextureStats[i].mManager.getPtr() );
   }
}

//==============================================================================
// dumpFlashFileStats
//==============================================================================
static void dumpFlashFileStats(BBufferStream& outputStream)
{
   BFlashManager::BFlashAssetAllocStatsArray flashProtoStats;
   BFlashManager::BFlashAssetAllocStatsArray flashInstanceStats;
   BD3DTextureManager::BTextureAllocStatsArray flashTextureStats;
   gRenderThread.submitCallback(getFlashProtoFileStats, &flashProtoStats);
   gRenderThread.submitCallback(getFlashInstanceFileStats, &flashInstanceStats);
   gRenderThread.submitCallback(getFlashTextureStats, &flashTextureStats);
   
   gRenderThread.blockUntilWorkerIdle();

   uint totalFlashAllocSize = 0;
   uint totalFlashProtoAllocSize = 0;
   uint totalFlashInstanceAllocSize = 0;

   for (uint i = 0; i < flashProtoStats.getSize(); i++)
      totalFlashProtoAllocSize += flashProtoStats[i].mAllocationSize;

   for (uint i = 0; i < flashInstanceStats.getSize(); i++)
      totalFlashInstanceAllocSize += flashInstanceStats[i].mAllocationSize;

   totalFlashAllocSize = totalFlashInstanceAllocSize + totalFlashProtoAllocSize;

   const float MB = 1024.0f * 1024.0f;   
   
   outputStream.printf("\"Total Flash Files\", %3.3f\n", totalFlashAllocSize / MB);
   outputStream.printf("\"  ProtoData\", %3.3f\n", totalFlashProtoAllocSize / MB);
   outputStream.printf("\"  Instance\", %3.3f\n", totalFlashInstanceAllocSize / MB);
   
   // sort
   flashProtoStats.sort();
   flashInstanceStats.sort();
   flashTextureStats.sort();
      
   outputStream.printf("\n");
   outputStream.printf("Flash Files (%u)\n", flashProtoStats.getSize());
   for (uint i = 0; i < flashProtoStats.getSize(); i++)
      outputStream.printf("\"   %s\", %3.3f\n", flashProtoStats[i].mFilename.getPtr(), flashProtoStats[i].mAllocationSize / MB);   

   outputStream.printf("\n");
   outputStream.printf("Flash Instances (%u)\n", flashInstanceStats.getSize());
   for (uint i = 0; i < flashInstanceStats.getSize(); i++)
      outputStream.printf("\"   %s\", %3.3f\n", flashInstanceStats[i].mFilename.getPtr(), flashInstanceStats[i].mAllocationSize / MB);   
   outputStream.printf("\n");



   outputStream.printf("\n");
   outputStream.printf("Font Specific Section\n");
   outputStream.printf("The following stats are already included in the previous statistics and are only provided\n");
   outputStream.printf("to illustrate localization memory costs!\n");

   outputStream.printf("Font Flash Files\n");
   uint totalFlashFontProtoAllocSize = 0;
   uint totalFlashFontInstanceAllocSize = 0;
   
   outputStream.printf("Font Proto Files\n");
   for (uint f = 0; f < flashProtoStats.getSize(); f++)
   {
      if (flashProtoStats[f].mCategory == cFlashAssetCategoryCommon)
      {
         totalFlashFontProtoAllocSize += flashProtoStats[f].mAllocationSize;
         outputStream.printf("\"   %s\", %3.3f\n", flashProtoStats[f].mFilename.getPtr(), flashProtoStats[f].mAllocationSize / MB);
      }
   }
   
   outputStream.printf("Font Instance Files\n");
   for (uint f = 0; f < flashInstanceStats.getSize(); f++)
   {
      if (flashInstanceStats[f].mCategory == cFlashAssetCategoryCommon)
      {
         totalFlashFontInstanceAllocSize += flashInstanceStats[f].mAllocationSize;
         outputStream.printf("\"   %s\", %3.3f\n", flashInstanceStats[f].mFilename.getPtr(), flashInstanceStats[f].mAllocationSize / MB);   
      }
   }

   outputStream.printf("\n");   
   uint textureCount = 0;
   uint textureTotalMem = 0;
   outputStream.printf("Flash Font Texture Assets, Size, Res, Levels, ArraySize, Format\n");
   for (uint i = 0; i < flashTextureStats.getSize(); i++)
   {
      if (flashTextureStats[i].mMembershipMask & BD3DTextureManager::cScaleformCommon)
      {
         outputStream.printf("\"   %s\", %3.3f, %ux%u, %u, %u, %s\n", 
            flashTextureStats[i].mFilename.getPtr(), 
            flashTextureStats[i].mActualAllocationSize / MB,
            flashTextureStats[i].mWidth, flashTextureStats[i].mHeight, flashTextureStats[i].mLevels, flashTextureStats[i].mArraySize,
            getDDXDataFormatString(flashTextureStats[i].mDDXFormat) );

         textureTotalMem+= flashTextureStats[i].mActualAllocationSize;
         textureCount++;
      }
   }
   outputStream.printf("Font Texture Count, %u\n", textureCount);         

   outputStream.printf("\n");
   outputStream.printf("Font Memory Summary\n");
   outputStream.printf("Font Memory Proto Total   : ,%3.3f\n", totalFlashFontProtoAllocSize / MB);
   outputStream.printf("Font Memory Instance Total: ,%3.3f\n", totalFlashFontInstanceAllocSize / MB);
   outputStream.printf("Font Memory Texture Total : ,%3.3f\n", textureTotalMem / MB);

   uint finalTotal = totalFlashFontProtoAllocSize + totalFlashFontInstanceAllocSize + textureTotalMem;
   outputStream.printf("Font Memory Total         : ,%3.3f\n\n\n", finalTotal / MB);
}

//==============================================================================
// displayFontMemoryStats
//==============================================================================
void displayFontMemoryStats(BDebugTextDisplay& textDisplay)
{   
   BFlashManager::BFlashAssetAllocStatsArray flashProtoStats;
   BFlashManager::BFlashAssetAllocStatsArray flashInstanceStats;
   BD3DTextureManager::BTextureAllocStatsArray flashTextureStats;
   GFxFontCacheManager::TextureConfig flashFontCacheConfig;
   gRenderThread.submitCallback(getFlashProtoFileStats, &flashProtoStats);
   gRenderThread.submitCallback(getFlashInstanceFileStats, &flashInstanceStats);
   gRenderThread.submitCallback(getFlashTextureStats, &flashTextureStats);  
   gRenderThread.submitCallback(getFlashFontCacheConfig, &flashFontCacheConfig);
   gRenderThread.blockUntilWorkerIdle();

   const float MB = 1024.0f * 1024.0f;   
   
   textDisplay.skipLine();
   textDisplay.printf("Font Memory\n");

   float cacheMemoryTotal = (float)(flashFontCacheConfig.TextureHeight * flashFontCacheConfig.TextureWidth * flashFontCacheConfig.MaxNumTextures);
   textDisplay.skipLine();
   textDisplay.printf("Font Dynamic Cache Settings");
   textDisplay.printf("  Texture Width          : %u", flashFontCacheConfig.TextureWidth);
   textDisplay.printf("  Texture Height         : %u", flashFontCacheConfig.TextureHeight);
   textDisplay.printf("  Texture Count          : %u", flashFontCacheConfig.MaxNumTextures);
   textDisplay.printf("  Texture Slot Height    : %u", flashFontCacheConfig.MaxSlotHeight);
   textDisplay.printf("  Texture Slot Padding   : %u", flashFontCacheConfig.SlotPadding);
   textDisplay.printf("  Texture Update Width   : %u", flashFontCacheConfig.TexUpdWidth);
   textDisplay.printf("  Texture Update Height  : %u", flashFontCacheConfig.TexUpdHeight);   
   textDisplay.printf("  Cache Memory Total (mb): %3.3f", cacheMemoryTotal/MB);
   textDisplay.skipLine();
         
   uint totalFlashFontProtoAllocSize = 0;
   uint totalFlashFontInstanceAllocSize = 0;
   
   textDisplay.printf("Font Proto Files\n");
   for (uint f = 0; f < flashProtoStats.getSize(); f++)
   {
      if (flashProtoStats[f].mCategory == cFlashAssetCategoryCommon)
      {
         totalFlashFontProtoAllocSize += flashProtoStats[f].mAllocationSize;
         textDisplay.printf(" %s - %3.3f\n", flashProtoStats[f].mFilename.getPtr(), flashProtoStats[f].mAllocationSize / MB);
      }      
   }
   textDisplay.skipLine();   

   if (flashInstanceStats.getSize() > 0)
   {
      textDisplay.printf("Font Instance Files\n");
      for (uint f = 0; f < flashInstanceStats.getSize(); f++)
      {
         if (flashInstanceStats[f].mCategory == cFlashAssetCategoryCommon)
         {
            totalFlashFontInstanceAllocSize += flashInstanceStats[f].mAllocationSize;
            textDisplay.printf(" %s - %3.3f\n", flashInstanceStats[f].mFilename.getPtr(), flashInstanceStats[f].mAllocationSize / MB);   
         }
      }
      textDisplay.skipLine();
   }
   
   uint textureCount = 0;
   uint textureTotalMem = 0;
   if (flashTextureStats.getSize() > 0)
   {
      textDisplay.printf("Flash Font Texture Assets, Size, Res, Levels, ArraySize, Format\n");
      for (uint i = 0; i < flashTextureStats.getSize(); i++)
      {
         if (flashTextureStats[i].mMembershipMask & BD3DTextureManager::cScaleformCommon)
         {
            textDisplay.printf(" %s : %3.3f, %ux%u, %u, %u, %s\n", 
               flashTextureStats[i].mFilename.getPtr(), 
               flashTextureStats[i].mActualAllocationSize / MB,
               flashTextureStats[i].mWidth, flashTextureStats[i].mHeight, flashTextureStats[i].mLevels, flashTextureStats[i].mArraySize,
               getDDXDataFormatString(flashTextureStats[i].mDDXFormat) );

            textureTotalMem+= flashTextureStats[i].mActualAllocationSize;
            textureCount++;
         }
      }

      textDisplay.skipLine();
   }
   textDisplay.printf("Font Texture Count, %u\n", textureCount);         
   
   textDisplay.skipLine();
   textDisplay.printf("Font Memory Summary\n");
   textDisplay.printf("Font Memory Proto Total    (mb): %3.3f\n", totalFlashFontProtoAllocSize / MB);
   textDisplay.printf("Font Memory Instance Total (mb): %3.3f\n", totalFlashFontInstanceAllocSize / MB);   
   textDisplay.printf("Font Memory Texture Total  (mb): %3.3f\n", textureTotalMem / MB);
   textDisplay.printf("Font Cache Memory Total    (mb): %3.3f\n", cacheMemoryTotal / MB);

   textDisplay.skipLine();
   textDisplay.setColor(cColorWhite);
   uint finalTotal = totalFlashFontProtoAllocSize + totalFlashFontInstanceAllocSize + textureTotalMem + cacheMemoryTotal;

   float finalTotalF = finalTotal / MB;  
   textDisplay.printf("Font Memory Total          (mb): %3.3f\n\n\n", finalTotalF);
}

//==============================================================================
// createAssetStatsCSV
//==============================================================================
void createAssetStatsCSV(const char* pFilename)
{
   if (!pFilename)
      return;
      
   gRenderThread.blockUntilWorkerIdle();

   BFileSystemStream csvFileStream;
   if (!csvFileStream.open(cDirProduction, pFilename, cSFWritable | cSFSeekable))
   {
      gConsoleOutput.output(cMsgConsole, "Unable to open file: %s\n", pFilename);
      return;
   }

   BBufferStream outputStream(csvFileStream);

   // Get the current game settings
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if (pSettings)
   {
      BFixedStringMaxPath mapName;
      pSettings->getString(BGameSettings::cMapName, mapName, MAX_PATH);
      outputStream.printf("Map: %s\n", mapName.getPtr());
   }  

   MEMORYSTATUS status;
   GlobalMemoryStatus( &status );

   outputStream.printf("\"XFS: %s, Archives: %i, Memory Free: %4.1f, Memory Allocated: %4.1f\"\n\n", 
      gXFS.getServerIP(), 
      gArchiveManager.getArchivesEnabled(), 
      status.dwAvailPhys / (1024.0f * 1024.0f), 
      (gInitialKernelMemoryTracker.getInitialPhysicalFree() - status.dwAvailPhys)  / (1024.0f * 1024.0f) );    

   SYSTEMTIME systemTime;
   GetSystemTime(&systemTime);
   outputStream.printf("\"Generated at %02u/%02u/%04u %02u:%02u:%02u\"\n", 
      systemTime.wMonth, systemTime.wDay, systemTime.wYear,  
      systemTime.wHour, systemTime.wMinute, systemTime.wSecond);

   outputStream.printf("\n");
   
   dumpTerrainStats(outputStream);
   dumpModelAndFlashStats(outputStream);
   dumpFlashFileStats(outputStream);
   dumpMiscTextureStats(outputStream);
   dumpHeapStats(outputStream);

   outputStream.close();

   csvFileStream.close();

   gConsoleOutput.output(cMsgConsole, "Wrote file: %s\n", pFilename);
}

//==============================================================================
float getHeapBarChartValue()
{
   DM_MEMORY_STATISTICS memStats;
   Utils::ClearObj(memStats);
   memStats.cbSize = sizeof(DM_MEMORY_STATISTICS);

   DmQueryTitleMemoryStatistics(&memStats);

   float val;
   val = memStats.AvailablePages*4096.0f/(1024.0f*1024.0f);
   return val;
}

#else
//==============================================================================
// createAssetStatsCSV
//==============================================================================
void createAssetStatsCSV(void)
{
}
#endif // BUILD_FINAL

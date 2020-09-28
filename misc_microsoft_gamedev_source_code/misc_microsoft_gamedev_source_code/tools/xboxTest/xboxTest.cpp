// File: xboxTest.cpp
#include "xcore.h"
#include <d3d9.h>

#include "..\shared\consoleGameHelper.h"

#include "containers\nameValueMap.h"

#include "atgFont.h"
#include "atgSimpleShaders.h"
#include "atgPostProcess.h"

namespace ATG
{
   extern D3DDevice* g_pd3dDevice;
}   

#include "memory\bfHeapAllocator.h"
#include "math\random.h"
#include "timer.h"



bool test()
{
   BBlockStackAllocator allocator;
   allocator.init(&gPrimaryHeap, 65536, 65536, 4);
   
   allocator.alloc(16, 16);
   allocator.alloc(128*1024, 4096);
   allocator.freeAll();
   
   allocator.alloc(16, 16);
   allocator.freeAll();
   
   allocator.alloc(16, 16);
   allocator.freeAll();
   
   allocator.alloc(16, 16);
   allocator.freeAll();
   
   allocator.alloc(16, 16);
   allocator.freeAll();
   
   allocator.alloc(16, 16);
   allocator.freeAll();
   
   
   allocator.kill();


   
   


#if 0
   gBaseSize = 16*1024*1024;
   //gpBase = VirtualAlloc(NULL, gBaseSize, MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE | PAGE_GUARD);
   gpBase = VirtualAlloc(NULL, gBaseSize, MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE | PAGE_GUARD);
   BVERIFY(gpBase);
   
   for (uint i = 0; i < 2; i++)
   {
      BTimer timer;
      timer.start();
      
      __try
      {
         XMemSet(gpBase, 0xF3, gBaseSize);
      }
      // Catch any assertion exceptions.
      __except(handleException(GetExceptionInformation()))
      {
      }
         
      gConsoleOutput.printf("%u: Set time: %fms\n", i, timer.getElapsedSeconds()*1000.0f);
   }      
   
   VirtualFree(gpBase, 0, MEM_RELEASE);
#endif   

#if 0

   Random rand;
   rand.setSeed(42291);

#if 1   
   BDynamicArray<uchar, 16> buf(1024 * 1024 * 4);
   BDynamicArray<uchar, 16> buf2(1024 * 1024 * 8);

   BBFHeapAllocator allocator;

   allocator.addRegion(buf.getPtr(), buf.getSize()/2);
   allocator.addRegion(buf.getPtr()+buf.getSize()/2, buf.getSize()/2);

   allocator.addRegion(buf2.getPtr(), buf2.getSize());
   bool checkSuccess = allocator.check();
   BVERIFY(checkSuccess);
#endif   

   enum { cNumBlockPtrs = 8192 };
   BDynamicArray<void*> blockPtrs(cNumBlockPtrs);
   BDynamicArray<uint> blockSizes(cNumBlockPtrs);

   double totalAllocTime = 0.0f;
   uint totalAllocs = 0;
   double totalFreeTime = 0.0f;
   uint totalFrees = 0;

//   BTimer timer;

   double totalTime = 0.0f;
   for (uint i = 0; i < 1000; i++)
   {
      timer.start();
      totalTime += timer.getElapsedSeconds();
   }

   printf("Timer overhead: %3.3fus\n", totalTime/1000.0f*1000000.0f);
   double timerOverhead = totalTime/1000.0f;

   uint q = 0;
   for ( ; ; )
   {  
      q++;
      if (q == 100)
      {
         q = 0;
         totalAllocs = 0;
         totalAllocTime = 0;
         totalFrees = 0;
         totalFreeTime = 0;
      }

#if 1
      uint numToAlloc = rand.iRand(0, 256);
      uint numToFree = rand.iRand(0, 256);

      if (rand.iRand(0, 1000) < 20)
      {
         gConsoleOutput.printf("Freeing all blocks\n");
         
         for (uint i = 0; i < cNumBlockPtrs; i++)
         {
            if (blockPtrs[i])
            {
               for (uint e = 0; e < blockSizes[i]; e++)
                  BVERIFY(((uchar*)blockPtrs[i])[e] == (blockSizes[i] & 0xFF));                        
               memset(blockPtrs[i], 0, blockSizes[i]);

               timer.start();               

               bool success = allocator.alignedFree(blockPtrs[i]);
               BVERIFY(success);

               totalFreeTime += Math::Max<double>(0.0f, timer.getElapsedSeconds() - timerOverhead);
               totalFrees++;

               blockPtrs[i] = NULL;
               blockSizes[i] = 0;
            }
         }

         bool success = allocator.check();
         BVERIFY(success);

#if 1         
         allocator.clear();
         allocator.addRegion(buf.getPtr(), buf.getSize());
         allocator.addRegion(buf2.getPtr(), buf2.getSize());
#else
         textureHeap.freeUnusedValleys();      

         for (uint i = 0; i < cNumTex; i++)
         {
            uint width = 1<<rand.iRand(0, 11);
            uint height = 1<<rand.iRand(0, 11);
            uint mips = rand.iRand(0, 1 + Math::Min(Math::iLog2(width), Math::iLog2(height))) + 1;
            
            D3DFORMAT fmt;
            switch (rand.iRand(0, 4))
            {
               case 0: fmt = D3DFMT_DXT1; break;
               case 1: fmt = D3DFMT_LIN_DXT1; break;
               case 2: fmt = D3DFMT_DXT5; break;
               case 3: fmt = D3DFMT_A16B16G16R16F; break;
            }
            
            UINT baseSize, mipSize, allocSize;
            if (rand.iRand(0, 1))
               allocSize = XGSetTextureHeader(width, height, mips, D3DUSAGE_CPU_CACHED_MEMORY, fmt, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, &tex[i], &baseSize, &mipSize);
            else
               allocSize = XGSetArrayTextureHeader(width, height, rand.iRand(2, 16), mips, D3DUSAGE_CPU_CACHED_MEMORY, fmt, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, (D3DArrayTexture*)&tex[i], &baseSize, &mipSize);

            uint actualAllocSize;
            void* pData = textureHeap.getValley(&tex[i], &actualAllocSize);
            BVERIFY(pData);

            XGOffsetResourceAddress(&tex[i], pData);

            bool success = textureHeap.unlockValley(&tex[i]);
            BVERIFY(success);
         }      

         textureHeap.check();

         for (uint i = 0; i < cNumTex; i++)   
         {
            bool success = textureHeap.releaseValley(&tex[i]);
            BVERIFY(success);
         }

         success = textureHeap.check();
         BVERIFY(success);
#endif         
      }

      if (rand.iRand(0, 100) == 0)
         numToFree = 0;

      if (rand.iRand(0, 100) == 0)
         numToAlloc = 5000;

      for (uint c = 0; c < numToFree; c++)
      {
         uint i = rand.iRand(0, cNumBlockPtrs);
         if (blockPtrs[i])
         {
            for (uint e = 0; e < blockSizes[i]; e++)
               BVERIFY(((uchar*)blockPtrs[i])[e] == (blockSizes[i] & 0xFF));                        
            memset(blockPtrs[i], 0, blockSizes[i]);

            timer.start();            

            bool success = allocator.alignedFree(blockPtrs[i]);
            BVERIFY(success);

            totalFreeTime += Math::Max<double>(0.0f, timer.getElapsedSeconds() - timerOverhead);
            totalFrees++;

            blockPtrs[i] = NULL;
            blockSizes[i] = 0;
         }            
      }

      for (uint c = 0; c < numToAlloc; c++)
      {            
         uint i = rand.iRand(0, cNumBlockPtrs);

         if (blockPtrs[i])
         {
            for (uint e = 0; e < blockSizes[i]; e++)
               BVERIFY(((uchar*)blockPtrs[i])[e] == (blockSizes[i] & 0xFF));                        
            memset(blockPtrs[i], 0, blockSizes[i]);

            timer.start();            

            bool success = allocator.alignedFree(blockPtrs[i]);
            BVERIFY(success);

            totalFreeTime += Math::Max<double>(0.0f, timer.getElapsedSeconds() - timerOverhead);
            totalFrees++;

            blockPtrs[i] = NULL;
            blockSizes[i] = 0;
         }            

         const uint cMaxAllocSize = 65536;
         uint size = rand.iRand(1, cMaxAllocSize);
         if (rand.iRand(1, 1000) == 0)
            size = rand.iRand(1, cMaxAllocSize*10);

         timer.start();

         uint actualSize = 0;
         blockPtrs[i] = allocator.alignedAlloc(size, &actualSize);
         if (blockPtrs[i])
         {
            BVERIFY(actualSize >= size);
            BVERIFY(actualSize = allocator.alignedGetSize(blockPtrs[i]));
         }

         totalAllocTime += Math::Max<double>(0.0f, timer.getElapsedSeconds() - timerOverhead);
         totalAllocs++;

         blockSizes[i] = size;
         if (blockPtrs[i])
            memset(blockPtrs[i], size & 0xFF, size);
      }
#endif      

#if 1
      {
         const uint numEntries = rand.iRand(1, 1024);

         BDynamicArray<uchar*> pointers(numEntries);
         BDynamicArray<uint> sizes(numEntries);

         for (uint i = 0; i < numEntries; i++)
         {
            uint size = rand.iRand(1, 512);

            timer.start();

            pointers[i] = (uchar*)allocator.alignedAlloc(size);

            totalAllocTime += Math::Max<double>(0.0f, timer.getElapsedSeconds() - timerOverhead);
            totalAllocs++;

            if (!pointers[i])
               sizes[i] = 0;
            else
            {
               sizes[i] = size;
               memset(pointers[i], i & 0xFF, size);
            }
         }

         // near operation 0xbdb25e3d

         bool success = allocator.check();
         BVERIFY(success);

         for (uint i = 0; i < numEntries; i++)
         {
            if (!pointers[i])
               continue;

            for (uint s = 0; s < sizes[i]; s++)
            {
               BVERIFY(pointers[i][s] == (i & 0xFF));
            }
         }

         for (uint i = 0; i < numEntries; i++)          
         {
            uint r = rand.iRand(i, numEntries);
            std::swap(pointers[i], pointers[r]);
            std::swap(sizes[i], sizes[r]);
         }

         for (uint i = 0; i < numEntries; i++)          
         {
            if (pointers[i])
            {
               timer.start();

               bool success = allocator.alignedFree(pointers[i]);
               BVERIFY(success);

               totalFreeTime += Math::Max<double>(0.0f, timer.getElapsedSeconds() - timerOverhead);
               totalFrees++;

               pointers[i] = NULL;
            }
         }

         success = allocator.check();
         BVERIFY(success);
      }         
#endif      

#if 0
      BXboxTextureHeap::BHeapStats heapStats;
      textureHeap.getHeapStats(heapStats);
      BXboxTextureHeap::BValleyStats valleyStats;
      textureHeap.getValleyStats(valleyStats);
                  
      gConsoleOutput.printf("NumAllocations: %u, BytesAllocated: %u, BytesFree: %u, FreeChunks: %u, LargestFreeChunk: %u, Bins: %u, Overhead: %u\n",
         heapStats.mTotalNumAllocations,
         heapStats.mTotalBytesAllocated,
         heapStats.mTotalBytesFree,
         heapStats.mTotalFreeChunks,
         heapStats.mLargestFreeChunk,
         heapStats.mTotalBins,
         heapStats.mTotalOverhead);
            
      gConsoleOutput.printf("ValleyClasses: %u, NumValleys: %u, NumAvailable: %u, AvailableBytes: %u, NumLocked: %u, LockedBytes: %u, NumOccupied: %u, OccupiedBytes: %u, AllocatedBytes: %u, RegionBytes: %u, LargestRegionSize: %u\n",
         valleyStats.mNumValleyClasses,
         valleyStats.mNumValleys,
         valleyStats.mNumAvailable,
         valleyStats.mTotalAvailableBytes,
         valleyStats.mNumLocked,
         valleyStats.mTotalLockedBytes,
         valleyStats.mNumOccupied,
         valleyStats.mTotalOccupiedBytes,
         valleyStats.mTotalAllocatedBytes,
         valleyStats.mTotalRegionBytes,
         valleyStats.mLargestRegionSize);
#endif

      gConsoleOutput.printf("Ave alloc time: %6.3fus, free time: %6.3fus\n", totalAllocTime / totalAllocs * 1000000.0f, totalFreeTime / totalFrees * 1000000.0f);

   }      
#endif

   return 0;
}

void main(void)
{
   __try 
   {
   
      if (!BConsoleGameHelper::setup(false))
         return;
         
      if (!BConsoleGameHelper::consoleInit())
      {
         BConsoleGameHelper::deinit();
         return;      
      }
                                 
      for ( ; ; )
      {
         if (BConsoleGameHelper::getButtons() & XINPUT_GAMEPAD_START)
            break;
         
         BConsoleGameHelper::consoleRender();

         bool success = test();
         gConsoleOutput.printf("Test result: %u\n", success);

         MEMORYSTATUS status;
         ZeroMemory(&status, sizeof(status));
         GlobalMemoryStatus(&status);
         gConsoleOutput.printf("Bytes free: %u\n", status.dwAvailPhys);

         gConsoleOutput.printf("Press A button to continue or B to exit.\n");
         DWORD buttons = BConsoleGameHelper::waitForButtonPress();
         if (XINPUT_GAMEPAD_B & buttons)
            break;
      }

      BConsoleGameHelper::deinit();
   }
   __except(gAssertionSystem.xboxHandleAssertException(GetExceptionInformation()))
   {
   
   }
}

//============================================================================
//
//  fastdeformer.cpp
//  
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================
#include "xgeom.h"

#include "fastdeformer.h"

#ifndef BUILD_FINAL
//   #define DISPLAY_TIMINGS   
#endif

#define USE_C_IMPLEMENTATIONS

#ifdef USE_C_IMPLEMENTATIONS
   #define DeformerRigidToStackAligned          DeformerRigidToStackAlignedC
   #define DeformerRigidToStackAlignedNoTan     DeformerRigidToStackAlignedNoTanC
   #define DeformerRigidToStackAlignedPosOnly   DeformerRigidToStackAlignedPosOnlyC
   #define DeformerBlended                      DeformerBlendedC
   #define DeformerBlendedNoTan                 DeformerBlendedNoTanC
   #define DeformerBlendedPosOnly               DeformerBlendedPosOnlyC
   #define DeformerCopyMatrices                 DeformerCopyMatricesC
   #define DeformerGetFlatVertex                DeformerGetFlatVertexC
   #define DeformerGetBumpVertex                DeformerGetBumpVertexC
#else
   #define DeformerRigidToStackAligned          DeformerRigidToStackAlignedSSE
   #define DeformerRigidToStackAlignedNoTan     DeformerRigidToStackAlignedNoTanSSE
   #define DeformerRigidToStackAlignedPosOnly   DeformerRigidToStackAlignedPosOnlySSE
   #define DeformerBlended                      DeformerBlendedSSE
   #define DeformerBlendedNoTan                 DeformerBlendedNoTanSSE
   #define DeformerBlendedPosOnly               DeformerBlendedPosOnlySSE
   #define DeformerCopyMatrices                 DeformerCopyMatricesSSE
   #define DeformerGetFlatVertex                DeformerGetFlatVertexSSE
   #define DeformerGetBumpVertex                DeformerGetBumpVertexSSE
#endif

//==============================================================================
namespace BFastMeshDeformer {
//==============================================================================

//==============================================================================
// BFastMeshDeformer statics
//==============================================================================
HANDLE                     BDeformer::mHelperThread = INVALID_HANDLE_VALUE;
bool                       BDeformer::mHelperThreadExitFlag = false;
HANDLE                     BDeformer::mWakeupEvent = INVALID_HANDLE_VALUE;   
int volatile               BDeformer::mNumWorkEntriesProcessed = 0;

BWorkQueueEntry            BDeformer::mWorkQueue[cWorkQueueEntries];
int                        BDeformer::mWorkQueueSize = 0;
int                        BDeformer::mWorkQueueNextAvail = 0;
CRITICAL_SECTION           BDeformer::mWorkQueueCriticalSection;

VecType                    BDeformer::mMatrixBuf[cMatrixBufEntries];
int                        BDeformer::mMatrixBufSize = 0;

bool volatile              BDeformer::mWorkerThreadProcessingEnabled = true;

DWORD                      BDeformer::mNumDeformed[cNumDeformedTotalIndices];

//==============================================================================
// Globals 
//==============================================================================
static granny_int32x identityIndices[128] = 
{
   0,1,2,3,4,5,6,7,8,9,
      10,11,12,13,14,15,16,17,18,19,
      20,21,22,23,24,25,26,27,28,29,
      30,31,32,33,34,35,36,37,38,39,
      40,41,42,43,44,45,46,47,48,49,
      50,51,52,53,54,55,56,57,58,59,
      60,61,62,63,64,65,66,67,68,69,
      70,71,72,73,74,75,76,77,78,79,
      80,81,82,83,84,85,86,87,88,89,
      90,91,92,93,94,95,96,97,98,99,
      100,101,102,103,104,105,106,107,108,109,
      110,111,112,113,114,115,116,117,118,119,
      120,121,122,123,124,125,126,127
};

// For debugging only
bool gMultithreadingEnabled = true;
bool gAlwaysCopyMatrices = false;

//==============================================================================
// BDeformer::createPacketBoneIndices
//==============================================================================
void BDeformer::createPacketBoneIndices(void)
{
   if (mpPacketBoneIndices)
      delete [] mpPacketBoneIndices;
      
   mpPacketBoneIndices = new uchar[mpHeader->mNumPackets];
   
   uint totalPackets = 0;
      
   for (uint blockIndex = 0; blockIndex < mpHeader->mNumBlocks; blockIndex++)
   {
      for (uint i = 0; i < mpHeader->getBlocks()[blockIndex].mNumPackets; i++)
      {
         mpPacketBoneIndices[totalPackets] = mpHeader->getBlocks()[blockIndex].mBoneIndex;
         totalPackets++;
      }
   }
   
   BASSERT(totalPackets == mpHeader->mNumPackets);
}

//==============================================================================
// BDeformer::crcCheck
//==============================================================================
bool BDeformer::crcCheck(const uchar* pData, uint dataLen) const
{
   if (!pData)
      return false;

   if (!Utils::IsAligned(pData, 16))
      return false;  

   if (dataLen < sizeof(BDataHeader))
      return false;

   const BDataHeader* pHeader = reinterpret_cast<const BDataHeader*>(pData);

   return pHeader->crcCheck();
}

#ifdef XBOX
//==============================================================================
// BDeformer::endianSwap
//==============================================================================
bool BDeformer::endianSwap(uchar* pData, uint dataLen)
{
   if (!pData)
      return false;
      
   if (!Utils::IsAligned(pData, 16))
      return false;  
    
   if (dataLen < sizeof(BDataHeader))
      return false;
               
   BDataHeader* pHeader = reinterpret_cast<BDataHeader*>(pData);

   if (FAST_DEFORMER_DATA_VERSION_SWAPPED == pHeader->mDataVersion)
      pHeader->endianSwap();
      
   return true;
}
#endif

//==============================================================================
// BFastMeshDeformer::setData
// Data block must be 16-byte aligned. 
//==============================================================================
bool BDeformer::setData(const uchar* pData, uint dataLen)
{
   delete [] mpPacketBoneIndices;
   mpPacketBoneIndices = NULL;
   
   mpData = NULL;
   mpHeader = NULL;
   mDataLen = 0;   
   
   if (!pData)
      return false;
   
   if (!Utils::IsAligned(pData, 16))
      return false;
      
   if (dataLen < sizeof(BDataHeader))
      return false;

   mpHeader = reinterpret_cast<const BDataHeader*>(pData);
   
   if (FAST_DEFORMER_DATA_VERSION != mpHeader->mDataVersion)
   {
      mpHeader = NULL;
      return false;
   }
      
   if (dataLen < mpHeader->mDataSize + sizeof(DWORD) * BDataHeader::NumPrefixDWORDs)
   {
      mpHeader = NULL;
      return false;
   }
   
   //if (!mpHeader->crcCheck())
   //   return false;
         
   mpData = pData;
   mDataLen = dataLen;
   
   createPacketBoneIndices();
   
   return true;
}

//==============================================================================
// BDeformer::setMultithreadedStatics
//==============================================================================
void BDeformer::setMultithreadingStatics(void)
{
   mHelperThread                    = INVALID_HANDLE_VALUE;
   mHelperThreadExitFlag            = false;
   mWakeupEvent                     = INVALID_HANDLE_VALUE;   
   mNumWorkEntriesProcessed         = 0;
   mWorkQueueSize                   = 0;
   mWorkQueueNextAvail              = 0;
   mMatrixBufSize                   = 0;
   mWorkerThreadProcessingEnabled   = true;
}

//==============================================================================
// BDeformer::BDeformer
//==============================================================================
BDeformer::BDeformer() :
   mpData(NULL),
   mDataLen(0),
   mpHeader(NULL),
   mpPacketBoneIndices(NULL),
   mVersion(FAST_DEFORMER_CLASS_VERSION)
{
}

//==============================================================================
// BDeformer::~BDeformer
//==============================================================================
BDeformer::~BDeformer()
{
   delete [] mpPacketBoneIndices;
   sync();
   clear();
}

//==============================================================================
// BDeformer::clear
//==============================================================================
void BDeformer::clear(void)
{
   mpData = NULL;
   mDataLen = 0;
   mpHeader = 0;
   mpPacketBoneIndices = NULL;
}

//==============================================================================
// BDeformer::defaultPoolCallback
//==============================================================================
void BDeformer::defaultPoolCallback(bool create, void* pData1, void* pData2)
{
   create;
   pData1;
   pData2;
   sync();
}

//==============================================================================
// BDeformer::initDefaultPoolCallback
//==============================================================================
void BDeformer::initDefaultPoolCallback(void)
{
}

//==============================================================================
// BDeformer::deinitDefaultPoolCallback
//==============================================================================
void BDeformer::deinitDefaultPoolCallback(void)
{
}

//==============================================================================
// BDeformer::getStats
//==============================================================================
void BDeformer::getStats(BStats& stats)
{
   stats.mNumDeformedInMainThread   = mNumDeformed[cNumDeformedMainThreadIndex];
   stats.mNumDeformedInHelperThread = mNumDeformed[cNumDeformedHelperThreadIndex];
   stats.mNumDeformedByGranny       = mNumDeformed[cNumDeformedGrannyIndex];
}

//==============================================================================
// BDeformer::clearStats
//==============================================================================
void BDeformer::clearStats(void)
{
   Utils::ClearObj(mNumDeformed);
}

//==============================================================================
// BDeformer::getNextWorkQueueEntry
//==============================================================================
BWorkQueueEntry* BDeformer::getNextWorkQueueEntry(void)
{
   BWorkQueueEntry* pNextEntry = NULL;

   EnterCriticalSection(&mWorkQueueCriticalSection);

   if ((mWorkQueueSize) && (mWorkQueueNextAvail < mWorkQueueSize))
   {
      pNextEntry = &mWorkQueue[mWorkQueueNextAvail];
      mWorkQueueNextAvail++;
   }

   LeaveCriticalSection(&mWorkQueueCriticalSection);

   return pNextEntry;
}

//==============================================================================
// BDeformer::sync
//==============================================================================
void BDeformer::sync(void)
{
#ifdef DISPLAY_TIMINGS   
   static double totalSrcBytes;
   static uint totalVerts;
   static uint64 totalCycles; 
   static uint totalDeforms;

   uint64 startTime = ReadCycleCounter();
#endif

   if (mWorkQueueSize)
   {
      int numWorkEntriesProcessed = 0;
      const bool oldWorkerThreadProcessingEnabled = mWorkerThreadProcessingEnabled;

      for ( ; ; )
      {
         BWorkQueueEntry* pEntry = getNextWorkQueueEntry();
         if (!pEntry)
            break;

         // Disable the worker thread so we only have 1 thread banging on the destination buffer, which just slows all cores down.
         mWorkerThreadProcessingEnabled = false;

         pEntry->mpDeformer->processWorkEntry(*pEntry);
         mNumDeformed[cNumDeformedMainThreadIndex]++;
         numWorkEntriesProcessed++;

#ifdef DISPLAY_TIMINGS
         totalSrcBytes += pEntry->mpDeformer->mpHeader->mPacketDataSize + pEntry->mpDeformer->mpHeader->mBlendVertDataSize;
         totalVerts += pEntry->mpDeformer->mpHeader->mNumVerts;
         totalDeforms++;
#endif      
      }

      mWorkerThreadProcessingEnabled = oldWorkerThreadProcessingEnabled;

      if (INVALID_HANDLE_VALUE != mHelperThread)
      {
         // Spin to wait for the worker thread to finish up the final work entry.
         const DWORD startTime = GetTickCount();

         for ( ; ; )
         {
            if ((numWorkEntriesProcessed + mNumWorkEntriesProcessed) >= mWorkQueueSize)
               break;

            // Give up if we've waited too long. Shouldn't happen, but you never know.
            if ((GetTickCount() - startTime) > 5000)
               break;
         }
      }  

      BASSERT(numWorkEntriesProcessed + mNumWorkEntriesProcessed == mWorkQueueSize);       

      EnterCriticalSection(&mWorkQueueCriticalSection);

      mWorkQueueSize = 0;
      mWorkQueueNextAvail = 0;
      mNumWorkEntriesProcessed = 0;
      mMatrixBufSize = 0;

      LeaveCriticalSection(&mWorkQueueCriticalSection);
   }

#ifdef DISPLAY_TIMINGS
   uint64 endTime = ReadCycleCounter();

   totalCycles += endTime - startTime;

   if (totalVerts > 5000000)
   {
      char buf[256];

      sprintf(buf, "Total Deforms: %u, Total Verts: %u, Total Cycles: %I64u, Ave. cycles/vert: %f\n", 
         totalDeforms,
         totalVerts,
         totalCycles,
         double(totalCycles) / double(totalVerts));
      OutputDebugStringA(buf);

      sprintf(buf, "Ave. verts per deform: %f, Ave. src bytes per deform: %f\n",
         double(totalVerts) / double(totalDeforms),
         totalSrcBytes / double(totalDeforms));
      OutputDebugStringA(buf);

      totalDeforms = 0;
      totalCycles = 0;
      totalVerts = 0;         
      totalSrcBytes = 0;
   }
#endif
}

//==============================================================================
// BDeformer::wakeupWorkerThread
//==============================================================================
void BDeformer::wakeupWorkerThread(void)
{
   if (INVALID_HANDLE_VALUE != mWakeupEvent)
      SetEvent(mWakeupEvent);
}

//==============================================================================
// BDeformer::deform
//==============================================================================
bool BDeformer::deform(
                       const granny_int32x* MatrixIndices,
                       const granny_real32* MatrixBuffer4x4,
                       void* DestVertices,
                       BVertFormat vertFormat,
                       bool multithreaded,
                       bool copyMatrices,
                       bool finalBuild)
{
   BASSERT(NULL != MatrixIndices);
   BASSERT(NULL != MatrixBuffer4x4);
   BASSERT(NULL != DestVertices);

   if (!mpHeader)
      return false;

   BASSERT(FAST_DEFORMER_DATA_VERSION == mpHeader->mDataVersion);

   if (!finalBuild)
   {
      if (vertFormat == cAutoVertFormat)
      {
         if (mpHeader->mFlags & BDataHeader::cAllFlat)
            vertFormat = cFlatVertFormat;
         else
            vertFormat = cBumpVertFormat;
      }

      int vertSize = 1;
      switch (vertFormat)
      {
      case cBumpVertFormat: vertSize = sizeof(pngt3332_vertex); break;
      case cFlatVertFormat: vertSize = sizeof(pnt332_vertex); break;
      case cPosOnlyVertFormat: vertSize = sizeof(p3_vertex); break;
      }

      char* pFirst = reinterpret_cast<char*>(DestVertices);
      char* pLast = pFirst + mpHeader->mNumVerts * vertSize - 1;

      // Try to trigger an exception if the destination address is invalid.
      *pFirst = '\0';
      *pLast = '\0';
   }

   if (FAST_DEFORMER_CLASS_VERSION != mVersion)
   {
      char buf[256];

      StringCchPrintfA(buf, sizeof(buf),
         "Deformer library is out of sync with game build! fastdeformersse.cpp was compiled with version: %08X, BDeformer class is version: %08X",
         FAST_DEFORMER_CLASS_VERSION,
         mVersion);

      BFAIL(buf);

      return false;
   }

   if (finalBuild)
   {
#ifndef BUILD_FINAL
      BFAIL("Game exec is a final build, but deformer lib linked against exec was not compiled with BUILD_FINAL");
#endif
   }

   //   if ((!mSupported) || (!useFastDeformer) || (gConfig.isDefined(cConfigForceGrannyDeformer)))

   if ((gMultithreadingEnabled) && (multithreaded) && (mHelperThread != INVALID_HANDLE_VALUE))
   {
      if ((mWorkQueueSize == cWorkQueueEntries) || ((mMatrixBufSize + mpHeader->mNumBones * 4) > cMatrixBufEntries))
         sync();

      if ((copyMatrices) || (gAlwaysCopyMatrices))
      {
         VecType* pBufferedMatrices = &mMatrixBuf[mMatrixBufSize];

         mMatrixBufSize += mpHeader->mNumBones * 4;         

         DeformerCopyMatrices(mpHeader, pBufferedMatrices, reinterpret_cast<const VecType*>(MatrixBuffer4x4), MatrixIndices);

         EnterCriticalSection(&mWorkQueueCriticalSection);

         mWorkQueue[mWorkQueueSize] = BWorkQueueEntry(this, identityIndices, reinterpret_cast<const granny_real32*>(pBufferedMatrices), DestVertices, vertFormat);
         mWorkQueueSize++;
      }         
      else
      {
         EnterCriticalSection(&mWorkQueueCriticalSection);

         mWorkQueue[mWorkQueueSize] = BWorkQueueEntry(this, MatrixIndices, MatrixBuffer4x4, DestVertices, vertFormat);
         mWorkQueueSize++;
      }  

      LeaveCriticalSection(&mWorkQueueCriticalSection);       

      SetEvent(mWakeupEvent);

      return true;
   }
   else
   {
      processWorkEntry(BWorkQueueEntry(this, MatrixIndices, MatrixBuffer4x4, DestVertices, vertFormat));

      mNumDeformed[cNumDeformedMainThreadIndex]++;
   }

   return false;
}

//==============================================================================
// BDeformer::enableMultithreading
//==============================================================================
void BDeformer::enableMultithreading(void)
{
   initHelperThread();
}

//==============================================================================
// BDeformer::helperThreadProc
//==============================================================================
unsigned int __stdcall BDeformer::helperThreadProc(void* pArguments)
{
   pArguments;

   bool spinEnabled = false;

   for ( ; ; )
   {
      WaitForSingleObject(mWakeupEvent, 250);

      if (mHelperThreadExitFlag)
         break;

      if (mWorkerThreadProcessingEnabled)
      {
         BWorkQueueEntry* pEntry = getNextWorkQueueEntry();
         if (pEntry)
         {
            pEntry->mpDeformer->processWorkEntry(*pEntry);

            // Assumes 1 worker thread only
            mNumDeformed[cNumDeformedHelperThreadIndex]++;

            mNumWorkEntriesProcessed++;
         }

         if (pEntry)
         {
            DWORD spinCount = 0;

            for ( ; ; )
            {
               if (!mWorkerThreadProcessingEnabled)
                  break;

               pEntry = getNextWorkQueueEntry();
               if (pEntry)
               {
                  pEntry->mpDeformer->processWorkEntry(*pEntry);

                  // Assumes 1 worker thread only
                  mNumDeformed[cNumDeformedHelperThreadIndex]++;

                  mNumWorkEntriesProcessed++;

                  spinCount = 0;
               }
               else
               {
                  if (!spinEnabled)
                     break;

                  spinCount++;
                  if (spinCount >= 50000)
                     break;
               }
            }  
         }         
      }         
   }

   _endthreadex(0);
   return 0;
}

//==============================================================================
// BDeformer::initHelperThread
//==============================================================================
void BDeformer::initHelperThread(void)
{
   if (INVALID_HANDLE_VALUE != mHelperThread)
      return;

   setMultithreadingStatics();

   initDefaultPoolCallback();

   mWakeupEvent = CreateEvent(NULL, FALSE, FALSE, NULL); 
   if (NULL == mWakeupEvent)
   {
      BFAIL("BDeformer::initHelperThread: CreateEvent() failed!\n");

      return;
   }

   InitializeCriticalSection(&mWorkQueueCriticalSection);

   unsigned int threadID = 0;
   mHelperThread = (HANDLE)_beginthreadex(NULL, 0, &helperThreadProc, NULL, 0, &threadID);
   if (0 == mHelperThread)
   {
      mHelperThread = INVALID_HANDLE_VALUE;

      BFAIL("BDeformer::initHelperThread: _beginthreadex() failed!\n");

      CloseHandle(mWakeupEvent);
      mWakeupEvent = INVALID_HANDLE_VALUE;

      DeleteCriticalSection(&mWorkQueueCriticalSection);

      return;
   }

#ifdef XBOX
   XSetThreadProcessor(mHelperThread, 2);
#endif
}

//==============================================================================
// BDeformer::destroyHelperThread
//==============================================================================
void BDeformer::destroyHelperThread(void)
{
   if ((INVALID_HANDLE_VALUE != mHelperThread) && (0 != mHelperThread))
   {
      DeleteCriticalSection(&mWorkQueueCriticalSection);

      mHelperThreadExitFlag = true;
      //Should the return code be checked? WAIT_OBJECT_0
      WaitForSingleObject(mHelperThread, INFINITE);
      CloseHandle(mHelperThread);
      mHelperThread = INVALID_HANDLE_VALUE;
   }

   if ((INVALID_HANDLE_VALUE != mWakeupEvent) && (NULL != mWakeupEvent))
   {
      CloseHandle(mWakeupEvent);
      mWakeupEvent = INVALID_HANDLE_VALUE;
   }

   deinitDefaultPoolCallback();

   setMultithreadingStatics();
}

//==============================================================================
// BDeformer::processWorkEntry
//==============================================================================
void BDeformer::processWorkEntry(const BWorkQueueEntry& work)
{
   BVertFormat vertFormat = work.mVertFormat;

   if (vertFormat == cAutoVertFormat)
   {
      if (mpHeader->mFlags & BDataHeader::cAllFlat)
         vertFormat = cFlatVertFormat;
      else
         vertFormat = cBumpVertFormat;
   }

   switch (vertFormat)
   {
      case cPosOnlyVertFormat:
      {  
         DeformerRigidToStackAlignedPosOnly(mpHeader, work);

         p3_vertex* pDstVerts = reinterpret_cast<p3_vertex*>(work.mpDestVertices);

         DeformerBlendedPosOnly(
            work, 
            mpHeader->getBlendVerts(),
            &pDstVerts[mpHeader->mNumRigidVerts], 
            mpHeader->mNumBlendVerts);

         break;
      }
      case cFlatVertFormat:
      {
         DeformerRigidToStackAlignedNoTan(mpHeader, work);

         pnt332_vertex* pDstVerts = reinterpret_cast<pnt332_vertex*>(work.mpDestVertices);

         DeformerBlendedNoTan(
            work, 
            mpHeader->getBlendVerts(),
            &pDstVerts[mpHeader->mNumRigidVerts], 
            mpHeader->mNumBlendVerts);
         break;
      }
      case cBumpVertFormat:
      {
         DeformerRigidToStackAligned(mpHeader, work);

         pngt3332_vertex* pDstVerts = reinterpret_cast<pngt3332_vertex*>(work.mpDestVertices);

         DeformerBlended(
            work, 
            mpHeader->getBlendVerts(),
            &pDstVerts[mpHeader->mNumRigidVerts], 
            mpHeader->mNumBlendVerts);
         break;            
      }
   }      
}

//============================================================================
// BDeformer::getVertex
//============================================================================
void BDeformer::getVertex(BGrannyBumpVert& dst, uint vertIndex)
{
   DeformerGetBumpVertex(mpHeader, mpPacketBoneIndices, dst, vertIndex);
}

//============================================================================
// BDeformer::getVertex
//============================================================================
void BDeformer::getVertex(BGrannyFlatVert& dst, uint vertIndex)
{
   DeformerGetFlatVertex(mpHeader, mpPacketBoneIndices, dst, vertIndex);
}

//==============================================================================
} // namespace BFastMeshDeformer
//==============================================================================

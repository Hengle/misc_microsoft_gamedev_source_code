//============================================================================
//
//  DCBManager.cpp
//  
//  Copyright (c) 2007-2008, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "DCBManager.h"

BDCBManager gDCBManager;

//============================================================================
// BDCBManager::BDCBManager
//============================================================================
BDCBManager::BDCBManager() :
   mMaxActiveBuffers(0),
   mMaxActiveBytes(0),
   mTotalAllocatedBlocks(0),
   mTotalAllocatedBytes(0),
   mRunCounter(0),
   mMaxEverAllocatedBlocks(0),
   mMaxEverAllocatedBytes(0),
   mBuffers(0, 0, &gRenderHeap)
{

}

//============================================================================
// BDCBManager::~BDCBManager
//============================================================================
BDCBManager::~BDCBManager()
{

}

//============================================================================
// BDCBManager::init
//============================================================================
void BDCBManager::init(uint maxActiveBuffers, uint maxActiveBytes)
{
   deinit();
   
   mMaxActiveBuffers = maxActiveBuffers;
   mMaxActiveBytes = maxActiveBytes;
   
   mMaxEverAllocatedBlocks = 0;
   mMaxEverAllocatedBytes = 0;
   
   mBuffers.reserve(mMaxActiveBuffers);      
   
   commandListenerInit();
}

//============================================================================
// BDCBManager::deinit
//============================================================================
void BDCBManager::deinit()
{
   gRenderThread.blockUntilGPUIdle();
   
   commandListenerDeinit();
   
   for (uint i = 0; i < mBuffers.getSize(); i++)
   {
      if (mBuffers[i]->mpBuf)
         mBuffers[i]->mpBuf->Release();
         
      HEAP_DELETE(mBuffers[i], gRenderHeap);
   }
   
   mBuffers.clear();
   
   BDEBUG_ASSERT(0 == mTotalAllocatedBlocks);
   BDEBUG_ASSERT(0 == mTotalAllocatedBytes);
   
   mMaxActiveBuffers = NULL;
   mMaxActiveBytes = NULL;
}

//============================================================================
// BDCBManager::reclaim
//============================================================================
void BDCBManager::reclaim()
{
   gRenderThread.blockUntilGPUIdle();
               
   for (uint i = 0; i < mBuffers.getSize(); i++)
   {
      if (mBuffers[i]->mAcquired) 
         continue;
      
      if (mBuffers[i]->mpBuf)
      {
         mBuffers[i]->mpBuf->Release();
         mBuffers[i]->mpBuf = NULL;
      }
   }
}

//============================================================================
// BDCBManager::acquire
//============================================================================
IDirect3DCommandBuffer9* BDCBManager::acquire()
{
   int oldestBufIndex = -1;
   int freeBufIndex = -1;
   for (uint bufIndex = 0; bufIndex < mBuffers.getSize(); bufIndex++)
   {
      if ((freeBufIndex == -1) && (!mBuffers[bufIndex]->mpBuf))
         freeBufIndex = bufIndex;
         
      if ((mBuffers[bufIndex]->mpBuf) && (!mBuffers[bufIndex]->mAcquired))
      {
         if (!mBuffers[bufIndex]->mpBuf->IsBusy())
         {
            mBuffers[bufIndex]->mAcquired = true;
            
            mBuffers[bufIndex]->mpBuf->Fence = 0;
            
            return mBuffers[bufIndex]->mpBuf;
         }
            
         if ((oldestBufIndex == -1) || (mBuffers[bufIndex]->mRunCounter < mBuffers[oldestBufIndex]->mRunCounter))
            oldestBufIndex = bufIndex; 
      }            
   }      
            
   if ((oldestBufIndex != -1) && ((mBuffers.getSize() == mMaxActiveBuffers) || (mTotalAllocatedBytes >= mMaxActiveBytes)))
   {
      mBuffers[oldestBufIndex]->mAcquired = true;
      
      mBuffers[oldestBufIndex]->mpBuf->BlockUntilNotBusy();
      mBuffers[oldestBufIndex]->mpBuf->Fence = 0;
            
      return mBuffers[oldestBufIndex]->mpBuf;
   }
   
   if (freeBufIndex < 0)
   {
      freeBufIndex = mBuffers.getSize();
      mBuffers.enlarge(1);
   }
   
   BCommandBuffer* pNewBuf = HEAP_NEW(BCommandBuffer, gRenderHeap);
   mBuffers[freeBufIndex] = pNewBuf;
   
   pNewBuf->mpManager = this;
   pNewBuf->mBufIndex = freeBufIndex;
           
   HRESULT hres = BD3D::mpDev->CreateGrowableCommandBuffer(0, allocateCallback, freeCallback, queryCallback, (DWORD)pNewBuf, 128 * 1024, &pNewBuf->mpBuf);
   if (FAILED(hres))
   {
      return NULL;
   }

   pNewBuf->mpBuf->SetIdentifier(freeBufIndex);        
   pNewBuf->mAcquired = true;
      
   return pNewBuf->mpBuf;
}

//============================================================================
// BDCBManager::run
//============================================================================
void BDCBManager::run(IDirect3DCommandBuffer9* pBuf, DWORD PredicationSelect)
{
   const uint bufIndex = pBuf->GetIdentifier();
   BDEBUG_ASSERT((bufIndex < mBuffers.getSize()) && (mBuffers[bufIndex]->mpBuf == pBuf)); 

   BCommandBuffer& buffer = *mBuffers[bufIndex];
   BDEBUG_ASSERT(buffer.mAcquired);

   buffer.mRunCounter = mRunCounter;
   mRunCounter++;
   
   // MPB [12/5/2008] - Get viewport to restore after running the command buffer.
   // For some unknown reason the viewport height is incorrect in the second (upper)
   // tile after the mesh command buffer is run.  Restoring the viewport after
   // the command buffer run works around this problem.
   D3DVIEWPORT9 curViewport;
   BD3D::mpDev->GetViewport(&curViewport);

   BD3D::mpDev->RunCommandBuffer(pBuf, PredicationSelect);
   
   // Restore viewport - see comment above
   BD3D::mpDev->SetViewport(&curViewport);

   // rg [12/11/07] - This sucks, but otherwise we'll occasionally get a D3D error when D3D tries to block on the command buffer on a worker thread.
   //BD3D::mpDev->InsertFence();
}

//============================================================================
// BDCBManager::release
//============================================================================
void BDCBManager::release(IDirect3DCommandBuffer9* pBuf)
{
   const uint bufIndex = pBuf->GetIdentifier();
   BDEBUG_ASSERT((bufIndex < mBuffers.getSize()) && (mBuffers[bufIndex]->mpBuf == pBuf)); 
   
   BCommandBuffer& buffer = *mBuffers[bufIndex];
   
   BDEBUG_ASSERT(buffer.mAcquired);
   
   buffer.mAcquired = false;
}

//============================================================================
// BDCBManager::allocateCallback
//============================================================================
void* BDCBManager::allocateCallback(DWORD Context, DWORD Flags, DWORD* pSize, DWORD Alignment)
{
   BCommandBuffer& buffer = *(BCommandBuffer*)Context;
   
   BDCBManager& manager = *buffer.mpManager;
         
   int actualSize;
   void* pBlock = gPhysWriteCombinedHeap.AlignedNew(*pSize, Alignment, &actualSize);
   if (!pBlock)
      return NULL;
   
   *pSize = actualSize;
   
   buffer.mPtrs.pushBack(pBlock);
   buffer.mBytesAllocated += actualSize;
   
   manager.mTotalAllocatedBytes += actualSize;
   manager.mTotalAllocatedBlocks++;
   
   manager.mMaxEverAllocatedBlocks = Math::Max(manager.mMaxEverAllocatedBlocks, manager.mTotalAllocatedBlocks);
   manager.mMaxEverAllocatedBytes = Math::Max(manager.mMaxEverAllocatedBytes, manager.mTotalAllocatedBytes);
      
   return pBlock;
}

//============================================================================
// BDCBManager::allocateCallback
//============================================================================
void BDCBManager::freeCallback(DWORD Context)
{
   BCommandBuffer& buffer = *(BCommandBuffer*)Context;
   
   BDCBManager& manager = *buffer.mpManager;
            
   for (uint i = 0; i < buffer.mPtrs.getSize(); i++)
      gPhysWriteCombinedHeap.Delete(buffer.mPtrs[i]);

   BDEBUG_ASSERT(manager.mTotalAllocatedBlocks >= buffer.mPtrs.getSize());
   BDEBUG_ASSERT(manager.mTotalAllocatedBytes >= buffer.mBytesAllocated);
         
   manager.mTotalAllocatedBlocks -= buffer.mPtrs.getSize();
   manager.mTotalAllocatedBytes -= buffer.mBytesAllocated;
      
   buffer.mPtrs.resize(0);
   
   buffer.mBytesAllocated = 0;
}

//============================================================================
// BDCBManager::queryCallback
//============================================================================
void BDCBManager::queryCallback(DWORD Context, DWORD* pUsed, DWORD* pRemaining)
{
   BCommandBuffer& buffer = *(BCommandBuffer*)Context;
   
//-- FIXING PREFIX BUG ID 7121
   const BDCBManager& manager = *buffer.mpManager;
//--
            
   *pUsed = buffer.mBytesAllocated;
   if (manager.mTotalAllocatedBytes > manager.mMaxActiveBytes)
      *pRemaining = 0;
   else
      *pRemaining = manager.mMaxActiveBytes - manager.mTotalAllocatedBytes;
}

//============================================================================
// BDCBManager::beginLevelLoad
//============================================================================
void BDCBManager::beginLevelLoad(void)
{
   reclaim();   
}

//============================================================================
// BDCBManager::endLevelLoad
//============================================================================
void BDCBManager::endLevelLoad(void)
{

}

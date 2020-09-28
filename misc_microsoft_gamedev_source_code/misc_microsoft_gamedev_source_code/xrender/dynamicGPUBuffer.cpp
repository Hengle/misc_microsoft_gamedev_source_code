//============================================================================
//
// File: dynamicGPUBuffer.cpp
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "dynamicGPUBuffer.h"
#include "renderThread.h"
#include "renderDraw.h"
#include "BD3D.h"
#include "timer.h"

#define BDYNAMICGPUBUFFERASSERT BASSERT

const cMaxGPUFenceQueueSize = 65536;

//============================================================================
// BDynamicGPUBuffer::BDynamicGPUBuffer
//============================================================================
BDynamicGPUBuffer::BDynamicGPUBuffer()
{
   ASSERT_RENDER_THREAD
   
   clear();
}


//============================================================================
// BDynamicGPUBuffer::BDynamicGPUBuffer
//============================================================================
BDynamicGPUBuffer::BDynamicGPUBuffer(uint bufferSize, uint fenceQueueSize)
{
   ASSERT_RENDER_THREAD
   
   clear();
   
   init(bufferSize, fenceQueueSize);
}

//============================================================================
// BDynamicGPUBuffer::~BDynamicGPUBuffer
//============================================================================
BDynamicGPUBuffer::~BDynamicGPUBuffer()
{
   ASSERT_RENDER_THREAD
   
   if (mpBuf)
   {
      gRenderThread.blockUntilGPUIdle();
      
      XPhysicalFree(mpBuf);
   }
   
   commandListenerDeinit();
}

//============================================================================
// BDynamicGPUBuffer::clear
//============================================================================
void BDynamicGPUBuffer::clear(void)
{
   mpBuf = NULL;
   mBufferSize = 0;

   mCurOfsGPU = 0;
   mCurOfsCPU = 0;
   mOutstandingBytes = 0;

   mGPUFences.clear();

   mpCurWriteLock = NULL;
   mCurWriteLockSize = 0;

   mTotalLockBytes = 0;
   mStartLockOfs = 0;

#ifndef BUILD_FINAL            
   clearStats();
#endif   

   mLocked = false;
}

//============================================================================
// BDynamicGPUBuffer::init
//============================================================================
BOOL BDynamicGPUBuffer::init(uint bufferSize, uint fenceQueueSize)
{
   ASSERT_RENDER_THREAD
   
   if (bufferSize < cMinBufferSize)
      return FALSE;
   
   if (bufferSize == mBufferSize)
      return TRUE;
      
   deinit();
   
   commandListenerInit();
   
   bufferSize = Utils::AlignUpValue(bufferSize, 65536);
   
   mpBuf = XPhysicalAlloc(bufferSize, MAXULONG_PTR, 0, PAGE_READWRITE | PAGE_WRITECOMBINE | MEM_LARGE_PAGES);
   if (!mpBuf)
      return FALSE;
   
   mBufferSize = bufferSize;
   
   mGPUFences.resize(fenceQueueSize);
   
   return TRUE;
}

//============================================================================
// BDynamicGPUBuffer::deinit
//============================================================================
void BDynamicGPUBuffer::deinit(void)
{
   ASSERT_RENDER_THREAD 
         
   if (mpBuf)
   {
      gRenderThread.blockUntilGPUIdle();
      
      XPhysicalFree(mpBuf);
      mpBuf = NULL;
   }
   
   commandListenerDeinit();
   
   clear();
}

//============================================================================
// BDynamicGPUBuffer::flushPendingFences
//============================================================================
void BDynamicGPUBuffer::flushPendingFences(void)
{
   ASSERT_RENDER_THREAD;
   BDYNAMICGPUBUFFERASSERT(mpBuf);
   
   if (!mGPUFences.getEmpty())
   {
      const DWORD fenceHandle = mGPUFences.peekFront(0).mFenceHandle;

      if (!BD3D::mpDev->IsFencePending(fenceHandle))
      {
         mGPUFences.clear();

         mOutstandingBytes = mTotalLockBytes;

#ifndef BUILD_FINAL   
         mStats.mMaxOutstandingBytes = Math::Max(mStats.mMaxOutstandingBytes, mOutstandingBytes);
#endif            
         
         if (mOutstandingBytes)
         {
            mCurOfsGPU = mStartLockOfs;
         }
         else
         {
            mCurOfsCPU = 0;
            mCurOfsGPU = 0;
         }
         
         return;
      }         
   }
   
   while (!mGPUFences.getEmpty())
   {
      const BGPUFence& fence = mGPUFences.peekBack(0);
      
      if (!BD3D::mpDev->IsFencePending(fence.mFenceHandle))
      {
         mCurOfsGPU = fence.mEndOfs;
         
         BDYNAMICGPUBUFFERASSERT(mOutstandingBytes >= fence.mSize);
         mOutstandingBytes -= fence.mSize;
         
         mGPUFences.popBack();
      }
      else
      {
         break;
      }
   }
   
   if (!mOutstandingBytes)
   {
      mCurOfsCPU = 0;
      mCurOfsGPU = 0;
   }
}

//============================================================================
// BDynamicGPUBuffer::blockUntilNotBusy
//============================================================================
void BDynamicGPUBuffer::blockUntilNotBusy(void)
{
   ASSERT_RENDER_THREAD
   BDYNAMICGPUBUFFERASSERT(mpBuf);
      
   SCOPEDSAMPLE(BDynamicGPUBuffer_blockUntilNotBusy);
         
   if (!mOutstandingBytes)
      return;
      
#ifndef BUILD_FINAL                        
   BTimer timer;
   timer.start();
#endif      

   BScopedPIXNamedEvent pixEvent("BDynamicGPUBufferBlock");
   
   if (!mGPUFences.getSize())
   {
      //trace("BDynamicGPUBuffer::blockUntilNotBusy: ERROR: No fences to block on; blocking until GPU is idle!");
      
      gRenderThread.blockUntilGPUIdle();
   }
   else
   {
      const DWORD fenceHandle = mGPUFences.peekFront(0).mFenceHandle;
      
      BD3D::mpDev->BlockOnFence(fenceHandle);
      
      mGPUFences.clear();
   }  
      
#ifndef BUILD_FINAL      
   const double elapsedTime = timer.getElapsedSeconds();
   mStats.mTotalBlockTime += elapsedTime;
   mStats.mPeakBlockTime = Math::Max(mStats.mPeakBlockTime, elapsedTime);
   mStats.mTotalBlocks++;
#endif            
   
   mOutstandingBytes = mTotalLockBytes;

#ifndef BUILD_FINAL   
   mStats.mMaxOutstandingBytes = Math::Max(mStats.mMaxOutstandingBytes, mOutstandingBytes);
#endif   
   
   if (mOutstandingBytes)
   {
      mCurOfsGPU = mStartLockOfs;
   }
   else
   {
      mCurOfsCPU = 0;
      mCurOfsGPU = 0; 
   }      
}

//============================================================================
// BDynamicGPUBuffer::isBusy
//============================================================================
BOOL BDynamicGPUBuffer::isBusy(void)
{
   ASSERT_RENDER_THREAD
   BDYNAMICGPUBUFFERASSERT(mpBuf);
   
   flushPendingFences();
   
   return mOutstandingBytes > 0;
}

//============================================================================
// BDynamicGPUBuffer::getAvailSpace
//============================================================================
uint BDynamicGPUBuffer::getAvailSpace(uint& atStart, uint& atCur)
{
   BDYNAMICGPUBUFFERASSERT((mCurOfsGPU < mBufferSize) && (mCurOfsCPU < mBufferSize));
   
   if (!mOutstandingBytes)
   {
      BDYNAMICGPUBUFFERASSERT((mCurOfsGPU == 0) && (mCurOfsCPU == 0));
      
      atStart = mBufferSize;
      atCur = 0;
   }
   else if (mCurOfsGPU < mCurOfsCPU)
   {
      atStart = mCurOfsGPU;
      atCur = mBufferSize - mCurOfsCPU;   
   }
   else
   {
      atStart = 0;
      atCur = mCurOfsGPU - mCurOfsCPU;
   }

   return Math::Max(atStart, atCur);
}

//============================================================================
// BDynamicGPUBuffer::wouldBlock
//============================================================================
BOOL BDynamicGPUBuffer::wouldBlock(uint size, uint alignment)
{
   ASSERT_RENDER_THREAD
   BDYNAMICGPUBUFFERASSERT(mpBuf);
   BDYNAMICGPUBUFFERASSERT((alignment >= cMinAllocAlignment) && (alignment <= cMaxAllocAlignment) && Math::IsPow2(alignment));
   BDYNAMICGPUBUFFERASSERT(!mpCurWriteLock);
   
   flushPendingFences();
   
   size = Utils::AlignUpValue(size, cMinAllocAlignment);
   
   uint atStart, atCur;
   getAvailSpace(atStart, atCur);
   
   if (atStart >= size)
      return false;
   
   const uint curOfsSize = size + Utils::BytesToAlignUpValue(mCurOfsCPU, alignment);
   if (atCur >= curOfsSize)
      return false;
   
   return true;
}

//============================================================================
// BDynamicGPUBuffer::lockBeginWrite
//============================================================================
void* BDynamicGPUBuffer::lockBeginWrite(uint size, uint alignment, BOOL blockIfBusy)
{
   ASSERT_RENDER_THREAD
   BDYNAMICGPUBUFFERASSERT(mpBuf);
   BDYNAMICGPUBUFFERASSERT((alignment >= cMinAllocAlignment) && (alignment <= cMaxAllocAlignment) && Math::IsPow2(alignment));
   BDYNAMICGPUBUFFERASSERT(size > 0);
   BDYNAMICGPUBUFFERASSERT(size <= mBufferSize);
   BDYNAMICGPUBUFFERASSERT(!mpCurWriteLock);
      
   if ((!size) || (size > mBufferSize))
      return NULL;
      
   flushPendingFences();
      
   size = Utils::AlignUpValue(size, cMinAllocAlignment);

   uint atStart, atCur;
   getAvailSpace(atStart, atCur);

   uint curOfsSize = size + Utils::BytesToAlignUpValue(mCurOfsCPU, alignment);
   
   if ((atStart < size) && (atCur < curOfsSize))
   {
      if (!blockIfBusy)  
         return NULL;

      blockUntilNotBusy();
      
      getAvailSpace(atStart, atCur);
      
      curOfsSize = size + Utils::BytesToAlignUpValue(mCurOfsCPU, alignment);
   }
   
   void* p;
   
   if (atCur >= curOfsSize)
   {
      mCurOfsCPU += Utils::BytesToAlignUpValue(mCurOfsCPU, alignment);
      
      p = (uchar*)mpBuf + mCurOfsCPU;
      
      mCurOfsCPU += size;
      BDYNAMICGPUBUFFERASSERT(mCurOfsCPU <= mBufferSize);
      
      if (mCurOfsCPU == mBufferSize)
         mCurOfsCPU = 0;
   }
   else if (atStart >= size)
   {
      p = mpBuf;
      
      mCurOfsCPU = size;
      if (mCurOfsCPU == mBufferSize)
         mCurOfsCPU = 0;
   }
   else
   {
      // Can only happen if the user tries to stream too much using multiple locks.
      return NULL;  
   }
            
   if (!mLocked)
      mStartLockOfs = (uchar*)p - (uchar*)mpBuf;
   
   mOutstandingBytes += size;   
   mTotalLockBytes += size;
    
   mpCurWriteLock = p;
   mCurWriteLockSize = size;
   
   mLocked = true;  

#ifndef BUILD_FINAL   
   mStats.mMaxOutstandingBytes = Math::Max(mStats.mMaxOutstandingBytes, mOutstandingBytes);

   mStats.mTotalLocks++;
   mStats.mTotalLockBytes += size;
#endif      
   
   return p;
}

//============================================================================
// BDynamicGPUBuffer::lockEndWrite
//============================================================================
void BDynamicGPUBuffer::lockEndWrite(BOOL invalidateGpuCache)
{
   ASSERT_RENDER_THREAD
   BDYNAMICGPUBUFFERASSERT(mpBuf);
   BDYNAMICGPUBUFFERASSERT(mpCurWriteLock);
   
   if (invalidateGpuCache)
      BD3D::mpDev->InvalidateGpuCache(mpCurWriteLock, mCurWriteLockSize, 0);
   
   mpCurWriteLock = NULL;
   mCurWriteLockSize = 0;
}

//============================================================================
// BDynamicGPUBuffer::lockCopy
//============================================================================
void* BDynamicGPUBuffer::lockCopy(const void* pData, uint size, uint alignment, BOOL blockIfBusy)
{
   ASSERT_RENDER_THREAD
   BDYNAMICGPUBUFFERASSERT(mpBuf);
   BDYNAMICGPUBUFFERASSERT(pData);
            
   void* p = lockBeginWrite(size, alignment, blockIfBusy);
   if (!p)
      return p;
      
   memcpy(p, pData, size);
   
   lockEndWrite();
   
   return p;
}

//============================================================================
// BDynamicGPUBuffer::lockAllocate
//============================================================================
void* BDynamicGPUBuffer::lockAllocate(uint size, uint alignment, BOOL blockIfBusy)
{
   void* p = lockBeginWrite(size, alignment, blockIfBusy);
   if (!p)
      return p;
   
   lockEndWrite(false);
   
   return p;
}

//============================================================================
// BDynamicGPUBuffer::unlock
//============================================================================
void BDynamicGPUBuffer::unlock(void)
{
   ASSERT_RENDER_THREAD
   BDYNAMICGPUBUFFERASSERT(mpBuf);
         
   flushPendingFences();      
   
   if (!mTotalLockBytes)
      return;
         
   DWORD fenceHandle = BD3D::mpDev->InsertFence();
   
   BGPUFence gpuFence(mCurOfsCPU, mTotalLockBytes, fenceHandle);
   
   mLocked = false;
   mTotalLockBytes = 0;
   mStartLockOfs = 0;
   
   if (!mGPUFences.pushFront(gpuFence))
   {
      if (mGPUFences.getSize() >= cMaxGPUFenceQueueSize)
      {
         trace("BDynamicGPUBuffer::unlock: ERROR: Too many pending fences!");

#ifndef BUILD_FINAL         
         BTimer timer;
         timer.start();         
#endif         
         
         gRenderThread.blockUntilGPUIdle();

#ifndef BUILD_FINAL
         const double elapsedTime = timer.getElapsedSeconds();
         mStats.mTotalBlockTime += timer.getElapsedSeconds();
         mStats.mPeakBlockTime = Math::Max(mStats.mPeakBlockTime, elapsedTime);
         mStats.mTotalBlocks++;                  
#endif         
         
         mGPUFences.clear();
         
         mOutstandingBytes = 0;
         mCurOfsCPU = 0;
         mCurOfsGPU = 0;
      }
      else
      {
         trace("BDynamicGPUBuffer::unlock: WARNING: Resizing pending fence queue!");
         
         mGPUFences.resize(Math::Max(16U, mGPUFences.getMaxSize() * 2));
      
         BVERIFY(mGPUFences.pushFront(gpuFence));
         
#ifndef BUILD_FINAL
         mStats.mNumFenceQueueResizes++;
#endif         
      }
   }

#ifndef BUILD_FINAL   
   mStats.mPendingFences = mGPUFences.getSize();
#endif   
}

//============================================================================
// BDynamicGPUBuffer::createVB
//============================================================================
IDirect3DVertexBuffer9* BDynamicGPUBuffer::createVB(uint lengthInBytes)
{
   ASSERT_RENDER_THREAD
   BDYNAMICGPUBUFFERASSERT(mpBuf);
   
   IDirect3DVertexBuffer9* pVB = gRenderThread.workerAllocateFrameStorageObj<IDirect3DVertexBuffer9, false>();
   if (!pVB)
      return NULL;
      
   uchar* pBuffer = (uchar*)lockAllocate(lengthInBytes);
   if (!pBuffer)
      return NULL;

   XGSetVertexBufferHeader(lengthInBytes, 0, 0, 0, pVB);

   XGOffsetResourceAddress(pVB, pBuffer); 
         
   return pVB;
}   

//============================================================================
// BDynamicGPUBuffer::createVB
//============================================================================
IDirect3DVertexBuffer9* BDynamicGPUBuffer::createVB(IDirect3DVertexBuffer9* pVB, uint lengthInBytes)
{
   ASSERT_RENDER_THREAD
   BDYNAMICGPUBUFFERASSERT(mpBuf);
      
   if (!pVB)
      return NULL;
      
   uchar* pBuffer = (uchar*)lockAllocate(lengthInBytes);
   if (!pBuffer)
      return NULL;

   XGSetVertexBufferHeader(lengthInBytes, 0, 0, 0, pVB);

   XGOffsetResourceAddress(pVB, pBuffer); 
         
   return pVB;
}   

//============================================================================
// BDynamicGPUBuffer::createIB
//============================================================================
IDirect3DIndexBuffer9* BDynamicGPUBuffer::createIB(uint lengthInBytes, D3DFORMAT indexType)
{
   ASSERT_RENDER_THREAD
   BDYNAMICGPUBUFFERASSERT(mpBuf);
   
   IDirect3DIndexBuffer9* pIB;
   uchar* pBuffer;

   pIB = gRenderThread.workerAllocateFrameStorageObj<IDirect3DIndexBuffer9, false>();
   if (!pIB)
      return NULL;
   
   pBuffer = (uchar*)lockAllocate(lengthInBytes);
   if (!pBuffer)
      return NULL;
      
   XGSetIndexBufferHeader(lengthInBytes, 0, indexType, 0, 0, pIB );
   XGOffsetResourceAddress(pIB, pBuffer); 

   return pIB;
}

//============================================================================
// BDynamicGPUBuffer::createIB
//============================================================================
IDirect3DIndexBuffer9* BDynamicGPUBuffer::createIB(IDirect3DIndexBuffer9* pIB, uint lengthInBytes, D3DFORMAT indexType)
{
   ASSERT_RENDER_THREAD
   BDYNAMICGPUBUFFERASSERT(mpBuf);
   
   if (!pIB)
      return NULL;
   
   uchar* pBuffer = (uchar*)lockAllocate(lengthInBytes);
   if (!pBuffer)
      return NULL;
      
   XGSetIndexBufferHeader(lengthInBytes, 0, indexType, 0, 0, pIB );
   XGOffsetResourceAddress(pIB, pBuffer); 

   return pIB;
}

//============================================================================
// BDynamicGPUBuffer::createDynamicTexture
//============================================================================
IDirect3DTexture9* BDynamicGPUBuffer::createDynamicTexture(uint width, uint height, D3DFORMAT format, uint* pLen)
{
   IDirect3DTexture9* pTex = gRenderThread.workerAllocateFrameStorageObj<IDirect3DTexture9, false>();
   if (!pTex)
      return NULL;

   const uint size = XGSetTextureHeader(
      width,
      height,
      1, 
      0,
      format,
      0,
      0,
      XGHEADER_CONTIGUOUS_MIP_OFFSET,
      0,
      pTex,
      NULL,
      NULL ); 

   if (pLen)
      *pLen = size;      

   void* pData = (uchar*)lockAllocate(size, 4096);   

   XGOffsetResourceAddress(pTex, pData);       

   return pTex;      
}

//============================================================================
// BDynamicGPUBuffer::beginLevelLoad
//============================================================================
void BDynamicGPUBuffer::beginLevelLoad(void) 
{ 
   BASSERT(!mLocked);
   
   if ((mLocked) || (!mpBuf))
      return;
   
   gRenderThread.blockUntilGPUIdle();

   mGPUFences.clear();

   mOutstandingBytes = 0;
   mCurOfsCPU = 0;
   mCurOfsGPU = 0;

   XPhysicalFree(mpBuf);
   mpBuf = NULL;
}

//============================================================================
// BDynamicGPUBuffer::endLevelLoad
//============================================================================
void BDynamicGPUBuffer::endLevelLoad(void) 
{ 
   if (mpBuf)
      return;
            
   mpBuf = XPhysicalAlloc(mBufferSize, MAXULONG_PTR, 0, PAGE_READWRITE | PAGE_WRITECOMBINE | MEM_LARGE_PAGES);
   if (!mpBuf)
   {
      BFATAL_FAIL("Out of memory");  
   }
}

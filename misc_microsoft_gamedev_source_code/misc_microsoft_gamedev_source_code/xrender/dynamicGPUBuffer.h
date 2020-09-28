//============================================================================
//
// File: dynamicGPUBuffer.h
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once

#include "containers\queue.h"
#include "renderThread.h"

//============================================================================
// class BDynamicGPUBuffer
// This class can ONLY be constructed/destructed/called on the render thread!
//============================================================================
class BDynamicGPUBuffer : public BRenderCommandListener
{
   BDynamicGPUBuffer(const BDynamicGPUBuffer&);
   BDynamicGPUBuffer& operator= (const BDynamicGPUBuffer&);
   
public:
   enum { cMinAllocAlignment = 32, cMaxAllocAlignment = 4096 };
   enum { cMinBufferSize = 65536 };
   
   BDynamicGPUBuffer();
   BDynamicGPUBuffer(uint bufferSize, uint fenceQueueSize = cDefaultFenceQueueSize);
   ~BDynamicGPUBuffer();
   
   enum { cDefaultFenceQueueSize = 512 };
   BOOL init(uint bufferSize, uint fenceQueueSize = cDefaultFenceQueueSize);
   void deinit(void);

   BOOL getInitialized(void) const { return NULL != mpBuf; }
   uint getBufferSize(void) const { return mBufferSize; }

   // Blocks until the GPU is no longer reading from the buffer.
   void blockUntilNotBusy(void);
   
   // TRUE if the GPU is currently reading from the buffer.
   BOOL isBusy(void);
   
   // TRUE if the lock call with the same parameters would block.
   BOOL wouldBlock(uint size, uint alignment = cMinAllocAlignment);
   
   // lockBeginWrite() locks the dynamic buffer, then allocates the specified amount of buffer space.
   // unlock() MUST be called after drawing with the buffer.
   // lockBeginWrite/lockEndWrite may be called multiple times before unlock() is called.
   // Returns NULL if blockIfBusy is false and blocking is needed, OR if you try to lock too much using multiple locks.
   // IMPORTANT: 
   // Multiple locks between draws are discouraged - if you use them be prepared to handle lockBeginWrite() returning NULL!
   // You can always lock the entire buffer in a single lock, but if you are using multiple locks between calls to unlock()
   // don't try to lock more than half of the buffer total.
   void* lockBeginWrite(uint size, uint alignment = cMinAllocAlignment, BOOL blockIfBusy = true);
   
   // lockEndWrite() calls MUST be paired with lockBeginWrite() calls.
   // Call after finishing writing to the buffer.
   void lockEndWrite(BOOL invalidateGpuCache = true);
   
   // lockAllocate() calls lockBeginWrite()/lockEndWrite(false). You must invalidate the GPU cache's view of the range before drawing!
   // You must call unlock() after drawing.
   void* lockAllocate(uint size, uint alignment = cMinAllocAlignment, BOOL blockIfBusy = true);
      
   // lockCopy() calls lockBeginWrite()/memcpy/lockEndWrite(true). You must call unlock() after drawing.
   void* lockCopy(const void* pData, uint size, uint alignment = cMinAllocAlignment, BOOL blockIfBusy = true);

   // unlock() MUST be called after you finish drawing with any data allocated from this buffer.
   // This method should be called once AFTER all Draw calls that use this buffer.
   void unlock(void);

   // TRUE if the buffer is currently locked.
   BOOL getLocked(void) const { return mLocked; }
   
   // Flushes the pending GPU fence queue.
   void flushPendingFences(void);
   
   // Creates a dynamic VB/IB/texture. 
   // The resource header is allocated from CPU frame storage.
   // You must call unlock() some time after done drawing with the resource.
   // DO NOT free the resource. The resource can ONLY be used on the current frame.
   // NULL on failure.
   // IMPORTANT: Be sure to unset the resource from the D3D device before the frame is over!
   IDirect3DVertexBuffer9* createVB(uint lengthInBytes);
   IDirect3DIndexBuffer9* createIB(uint lengthInBytes, D3DFORMAT indexType = D3DFMT_INDEX16);

   // same as above functions but the caller is responsible for passing in a valid resource header struct
   // the lifetime of the header struct is the responsibility of the caller
   // no CPU frame storage is used
   IDirect3DVertexBuffer9* createVB(IDirect3DVertexBuffer9* pVB, uint lengthInBytes);
   IDirect3DIndexBuffer9* createIB(IDirect3DIndexBuffer9* pIB, uint lengthInBytes, D3DFORMAT indexType = D3DFMT_INDEX16);

   IDirect3DTexture9* createDynamicTexture(uint width, uint height, D3DFORMAT format, uint* pLen);
   
   struct BStats
   {
      uint mNumFenceQueueResizes;
      
      uint mTotalLocks;
      uint mTotalLockBytes;
      
      uint mPendingFences;
      uint mTotalBlocks;
      double mPeakBlockTime;
      double mTotalBlockTime;
      uint mMaxOutstandingBytes;
                                 
      uint mMaxEverTotalBlocks;
      uint mMaxEverPendingFences;
      uint mMaxEverOutstandingBytes;
      uint mMaxEverTotalLocks;
      uint mMaxEverTotalLockBytes;
      double mMaxEverPeakBlockTime;
      double mMaxEverTotalBlockTime;
      
      void clear() { Utils::ClearObj(*this); }
      
      void clearFrameStats() { mPendingFences = 0; mTotalLocks = 0; mTotalLockBytes = 0; mTotalBlocks = 0; mPeakBlockTime = 0.0f; mTotalBlockTime = 0.0f; mMaxOutstandingBytes = 0; }
      
      void clearMaxEverStats() { mMaxEverTotalBlocks = 0; mMaxEverPendingFences = 0; mMaxEverOutstandingBytes = 0; mMaxEverTotalLocks = 0; mMaxEverTotalLocks = 0; mMaxEverTotalLockBytes = 0; mMaxEverTotalBlockTime = 0.0f; mMaxEverPeakBlockTime = 0.0f; };
      
      void updateMaxEverStats()
      {
         mMaxEverOutstandingBytes = Math::Max(mMaxEverOutstandingBytes, mMaxOutstandingBytes);
         mMaxEverTotalLocks = Math::Max(mMaxEverTotalLocks, mTotalLocks);
         mMaxEverTotalLockBytes = Math::Max(mMaxEverTotalLockBytes, mTotalLockBytes);
         mMaxEverPendingFences = Math::Max(mMaxEverPendingFences, mPendingFences);
         mMaxEverPeakBlockTime = Math::Max(mMaxEverPeakBlockTime, mPeakBlockTime);
         mMaxEverTotalBlocks = Math::Max(mMaxEverTotalBlocks, mTotalBlocks);
         mMaxEverTotalBlockTime = Math::Max(mMaxEverTotalBlockTime, mTotalBlockTime);
         mMaxEverPeakBlockTime = Math::Max(mMaxEverPeakBlockTime, mPeakBlockTime);
      }
   };
   
#ifndef BUILD_FINAL         
   void clearStats(void) { mStats.clear(); }
   BStats& getStats(void) { return mStats; }
#endif   
      
private:
   void* mpBuf;
   uint mBufferSize;

   uint mCurOfsGPU;
   uint mCurOfsCPU;
   uint mOutstandingBytes;

   struct BGPUFence
   {
      BGPUFence() { }
      BGPUFence(uint endOfs, uint size, DWORD fenceHandle) : mEndOfs(endOfs), mSize(size), mFenceHandle(fenceHandle) { }
      
      uint mEndOfs;
      uint mSize;
      DWORD mFenceHandle;
   };

   // Front - New fences inserted here
   // Back - The fence we are waiting for
   BQueue<BGPUFence> mGPUFences;

   void* mpCurWriteLock;
   uint mCurWriteLockSize;
      
   uint mTotalLockBytes;
   uint mStartLockOfs;

#ifndef BUILD_FINAL               
   BStats mStats;
#endif

   bool mLocked : 1;
      
   void clear(void);
   
   uint getAvailSpace(uint& atStart, uint& atCur);
   
   virtual void beginLevelLoad(void);
   virtual void endLevelLoad(void);
};

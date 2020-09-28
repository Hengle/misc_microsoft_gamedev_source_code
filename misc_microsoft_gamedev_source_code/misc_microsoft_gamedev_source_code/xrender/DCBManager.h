// File: DCBManager.h
#pragma once
#include "renderThread.h"

//============================================================================
// class BDCBManager
//============================================================================
class BDCBManager : public BRenderCommandListener
{
public:
   BDCBManager();
   ~BDCBManager();

   // render thread only!   
   void init(uint maxActiveBuffers = 16, uint maxActiveBytes = 2*1024*1024);
   void deinit();
         
   // Returns NULL if buffer allocation fails (probably due to out of memory)
   IDirect3DCommandBuffer9* acquire();
   void run(IDirect3DCommandBuffer9* pBuf, DWORD PredicationSelect);
   void release(IDirect3DCommandBuffer9* pBuf);
   
   // reclaim() will release all unacquired buffers. It blocks so don't call this often!
   void reclaim();
      
   //this is a non-threadsafe read operation, so you're results per-frame may vary
   uint getTotalAllocatedBytes() { return mTotalAllocatedBytes; }

private:
   struct BCommandBuffer
   {
      BDCBManager*                     mpManager;  
      uint                             mBufIndex;
      
      IDirect3DCommandBuffer9*         mpBuf;
      uint                             mBytesAllocated;
      int                              mRunCounter;
      BSmallDynamicRenderArray<void*>  mPtrs;
      bool                             mAcquired;
      
      BCommandBuffer() { clear(); }      
            
      void clear() { mpBuf = NULL; mBytesAllocated = 0; mRunCounter = -1; mPtrs.clear(); mAcquired = false; }
   };
   
   BSegmentedArray<BCommandBuffer*, 5> mBuffers;
   
   uint                          mMaxActiveBuffers;
   uint                          mMaxActiveBytes;
      
   uint                          mTotalAllocatedBlocks;
   uint                          mTotalAllocatedBytes;
   
   uint                          mMaxEverAllocatedBlocks;
   uint                          mMaxEverAllocatedBytes;
   
   int                           mRunCounter;
               
   static void* allocateCallback(DWORD Context, DWORD Flags, DWORD* pSize, DWORD Alignment);
   static void freeCallback(DWORD Context);
   static void queryCallback(DWORD Context, DWORD* pUsed, DWORD* pRemaining);
   
   virtual void beginLevelLoad(void);
   virtual void endLevelLoad(void);
};

extern BDCBManager gDCBManager;

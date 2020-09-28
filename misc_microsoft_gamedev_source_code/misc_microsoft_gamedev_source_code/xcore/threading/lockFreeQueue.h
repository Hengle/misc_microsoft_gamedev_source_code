//============================================================================
//
// File: lockFreeQueue.h
// Copyright (c) 2005-2006, Ensemble Studios
//
// http://citeseer.ist.psu.edu/cache/papers/cs/268/ftp:zSzzSzftp.cs.rochester.eduzSzpubzSzpaperszSzsystemszSz96.PODC.Non-blocking_and_blocking_concurrent_queue_algorithms.pdf/michael96simple.pdf
// FIXME: Won't work on the PC because it doesn't support CAS2.
//
//============================================================================
#pragma once
#include "interlocked.h"

#ifdef XBOX
template<class T>
class BLockFreeQueue
{
   BLockFreeQueue(const BLockFreeQueue&);
   BLockFreeQueue& operator= (const BLockFreeQueue&);
   
public:
   BLockFreeQueue(BMemoryHeap* pHeap, uint maxNodes) :
      mpHeap(pHeap),
      mNumNodes(maxNodes + 1)
   {
      BDEBUG_ASSERT(maxNodes);
      
      if (!mpHeap) mpHeap = &gPrimaryHeap;
      
      mpListRaw = ALIGNED_NEW_ARRAY(BYTE, sizeof(BListPtrs) + 128 - 1, *mpHeap);
      mpList = reinterpret_cast<BListPtrs*>(Utils::AlignUp(mpListRaw, 128));
            
      mpNodes = ALIGNED_NEW_ARRAY(BNode, mNumNodes, *mpHeap);
      memset(mpNodes, 0, sizeof(BNode) * mNumNodes);
            
      InitializeSListHead(&mFreelist);
      
      for (int i = mNumNodes - 1; i >= 0; i--)
         InterlockedPushEntrySList(&mFreelist, mpNodes[i].getSListEntryPtr());
                  
      BNode* pNode = newNode();
      pNode->mNext.mPtr = NULL;
      
      mpList->mHead.mPtr = pNode;
      mpList->mHead.mCount = 0;
      mpList->mTail.mPtr = pNode;
      mpList->mTail.mCount = 0;
      
      MemoryBarrier();
   }
   
   ~BLockFreeQueue()
   {
      MemoryBarrier();
      
      ALIGNED_DELETE_ARRAY(mpNodes, *mpHeap);
      ALIGNED_DELETE_ARRAY(mpListRaw, *mpHeap);
   }
   
   uint getMaxNodes(void) const { return mNumNodes ? (mNumNodes - 1) : 0; }
      
   BOOL enqueue(const T& value)
   {
      BNode* pNode = newNode();
      if (!pNode)
         return FALSE;
                     
      pNode->mValue = value;
      pNode->mNext.mPtr = NULL;

      // Export barrier.         
      MemoryBarrier();
         
      BPointer tail, next;
      for ( ; ; )
      {
         tail = mpList->mTail; _ReadBarrier();
         next = tail.mPtr->mNext; _ReadBarrier();
                  
         if (next.mPtr == NULL)
         {
            BPointer newPointer(pNode, next.mCount + 1);
            if (Sync::CompareAndSwap64((LONG64*)&tail.mPtr->mNext, *(const LONG64*)&next, *(const LONG64*)&newPointer))
               break;
         }
         else
         {
            BPointer newPointer(next.mPtr, tail.mCount + 1);
            Sync::CompareAndSwap64((LONG64*)&mpList->mTail, *(const LONG64*)&tail, *(const LONG64*)&newPointer);
         }
      }
      
      BPointer newPointer(pNode, tail.mCount + 1);
      Sync::CompareAndSwap64((LONG64*)&mpList->mTail, *(const LONG64*)&tail, *(const LONG64*)&newPointer);
      
      return TRUE;
   }
   
   BOOL dequeue(T& value)
   {
      BPointer head, tail, next;
      for ( ; ; )
      {
         head = mpList->mHead; _ReadBarrier();
         tail = mpList->mTail; _ReadBarrier();
         next = head.mPtr->mNext; _ReadBarrier();

         // I'm relying on the entire cache line at mpList->mHead being synchronized here, so tail is consistent. This works on 360.
         if (!Sync::CompareAndSwap64((LONG64*)&mpList->mHead, *(const LONG64*)&head, *(const LONG64*)&head))
            continue;
            
         if (next.mPtr == NULL)
            return FALSE;
                           
         if (head.mPtr == tail.mPtr)
         {  
            BPointer newPointer(next.mPtr, tail.mCount + 1);
            Sync::CompareAndSwap64((LONG64*)&mpList->mTail, *(const LONG64*)&tail, *(const LONG64*)&newPointer);
         }
         else
         {
            // Import barrier.
            MemoryBarrier();
                        
            value = next.mPtr->mValue;
            
            // Not sure if this barrier is necessary. Make sure the read completes before the next CAS, otherwise another thread could trash next.mPtr->mValue.
            MemoryBarrier();
                                    
            BPointer newPointer(next.mPtr, head.mCount + 1);
            if (Sync::CompareAndSwap64((LONG64*)&mpList->mHead, *(const LONG64*)&head, *(const LONG64*)&newPointer))
               break;
         }
      }  
                  
      freeNode(head.mPtr);
      return TRUE;
   }
   
   // clear() dequeues entries until the queue is empty. 
   // There is no guarantee that the queue will still be empty when clear() returns!
   void clear(void)
   {
      for ( ; ; )
      {
         T val;
         BOOL result = dequeue(val);
         if (!result)
            break;
      }
   }
      
private:
   struct BNode;
   
   __declspec(align(8))
   struct BPointer
   {
      BPointer() { }
      BPointer(BNode* pPtr, uint count) : mPtr(pPtr), mCount(count) { }
      
      BNode* mPtr; 
      uint mCount;
      
      bool operator== (const BPointer &p) const { return (mPtr == p.mPtr) && (mCount == p.mCount); }
   };
   
   __declspec(align(8))
   struct BNode
   {
      BPointer mNext;
      T mValue;
       
      SLIST_ENTRY* getSListEntryPtr(void) { return reinterpret_cast<SLIST_ENTRY*>(this); }
   };
   
   struct BListPtrs
   {
      BPointer mHead;
      BPointer mTail;
   };
      
   BListPtrs*     mpList;
   SLIST_HEADER   mFreelist;      
   
   BNode*         mpNodes;
   uint           mNumNodes;
         
   BMemoryHeap*   mpHeap;
   
   BYTE*          mpListRaw;
   
   BNode* newNode(void)
   {
      BNode* pNode = (BNode*)InterlockedPopEntrySList(&mFreelist);
      return pNode;
   }  

   void freeNode(BNode* pNode)
   {
      if (pNode)
         InterlockedPushEntrySList(&mFreelist, (PSLIST_ENTRY)pNode);
   }
};

#else // !XBOX

template<class T>
class BLockFreeQueue
{
   BLockFreeQueue(const BLockFreeQueue&);
   BLockFreeQueue& operator= (const BLockFreeQueue&);

public:
   BLockFreeQueue(BMemoryHeap* pHeap, uint maxNodes) :
      mpHeap(pHeap),
      mNumNodes(maxNodes + 1)
   {
      BFATAL_FAIL("Not ported yet");
   }

   BOOL enqueue(const T& value) { return FALSE; }

   BOOL dequeue(T& value) { return FALSE; }

   void clear(void) { }
};

#endif // XBOX

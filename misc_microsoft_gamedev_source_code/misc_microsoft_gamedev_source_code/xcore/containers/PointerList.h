//============================================================================
//
//  PointerList.h
//  
//  Copyright (c) 1999-2002, Ensemble Studios
//
//============================================================================
#pragma once
#include "memory\alignedAlloc.h"

//----------------------------------------------------------------------------
//  Public Constants
//----------------------------------------------------------------------------
const unsigned long BPLIST_GROW_EXPONENTIAL = 0xFFFFFFFF;


//----------------------------------------------------------------------------
//  Class BPointerList
//----------------------------------------------------------------------------
template <class T> class BPointerList
{
public:
   //-- Callback Prototypes
   typedef long (CALLBACK COMPARE_FUNC)(const T& item1, const T& item2, void* pParam);

   //-- Construction/Destruction
   BPointerList(BMemoryHeap* pHeap);
   BPointerList(unsigned long initialSize = 0, unsigned long growSize = BPLIST_GROW_EXPONENTIAL, BMemoryHeap* pHeap = &gPrimaryHeap);
   BPointerList(const BPointerList<T>& list, BMemoryHeap* pHeap = &gPrimaryHeap);
   ~BPointerList();
   
   void setHeap(BMemoryHeap* pHeap) { reset(0, 0); mpHeap = pHeap; }
   BMemoryHeap* getHeap(void) const { return mpHeap; }

   //-- List Control
   void   reset      (unsigned long initialSize = 0, unsigned long growSize = BPLIST_GROW_EXPONENTIAL);
   void   empty      ();
   void   optimize   ();
   void   setGrowSize(unsigned long growSize);

   //-- List Info
   bool   isEmpty          () const;
   bool   isGrowExponential() const;
   unsigned long  getSize          () const;
   unsigned long  getGrowSize      () const;
   unsigned long  getAllocatedSize () const;

   //-- Adding Items
   BHandle addToHead(T* pItem);
   BHandle addToTail(T* pItem);
   BHandle addBefore(T* pItem, BHandle hItem);
   BHandle addAfter (T* pItem, BHandle hItem);
   BHandle addSorted(T* pItem, bool ascending = true);
   BHandle addSorted(T* pItem, COMPARE_FUNC* pFunc, void* pParam, bool ascending = true);

   //-- Removing Items
   T*      removeHead      ();
   T*      removeTail      ();
   T*      remove          (BHandle hItem);
   T*      removeAndGetPrev(BHandle& hItem);
   T*      removeAndGetNext(BHandle& hItem);

   //-- Relocation
   void   moveToHead(BHandle hItem);
   void   moveToTail(BHandle hItem);

   //-- Navigation   
   T*      getItem        (BHandle hItem)  const;
   T*      getItem        (unsigned long index)   const;
   T*      getHead        (BHandle& hItem) const;
   T*      getTail        (BHandle& hItem) const;
   T*      getPrev        (BHandle& hItem) const;
   T*      getNext        (BHandle& hItem) const;
   T*      getPrevWithWrap(BHandle& hItem) const;
   T*      getNextWithWrap(BHandle& hItem) const;

   //-- Searching and Sorting
   BHandle findPointerForward (const T* pItem, BHandle hStart = NULL) const;
   BHandle findPointerBackward(const T* pItem, BHandle hStart = NULL) const;
   BHandle findItemForward    (const T* pItem, BHandle hStart = NULL) const;
   BHandle findItemBackward   (const T* pItem, BHandle hStart = NULL) const;
   BHandle findItemForward    (COMPARE_FUNC* pFunc, void* pParam, const T* pItem, BHandle hStart = NULL) const;
   BHandle findItemBackward   (COMPARE_FUNC* pFunc, void* pParam, const T* pItem, BHandle hStart = NULL) const;
   void   quickSort          (bool ascending = true);
   void   insertionSort      (bool ascending = true);
   void   quickSort          (COMPARE_FUNC* pFunc, void* pParam, bool ascending = true);
   void   insertionSort      (COMPARE_FUNC* pFunc, void* pParam, bool ascending = true);

   //-- Operators
   BPointerList<T>& operator = (const BPointerList<T>& list);

private:
   //-- Private structs
   struct BPNode
   {
      BPNode* pPrev;
      BPNode* pNext;
      T*      pItem;
   };

   struct BPBlock
   {
      unsigned long   numNodes;
      BPBlock* pNextBlock;
      BPNode*  pNodes;
   };

   //-- Helper Functions
   bool    isValidHandle(BHandle hItem) const;
   void    addToFreeHead(BPNode* pNode);
   void    addBlock     (unsigned long numNodes);
   BPNode*  getFreeNode  ();
   void    linkToHead   (BPNode* pNode);
   void    linkToTail   (BPNode* pNode);
   void    unlink       (BPNode* pNode);

   //-- Sorting Functions
   BPNode*  getPivot     (bool ascending, unsigned long numNodes, BPNode* pFirst, BPNode* pLast);
   void    quickSort    (bool ascending, unsigned long numNodes, BPNode* pFirst, BPNode* pLast);
   void    insertionSort(bool ascending, unsigned long numNodes, BPNode* pFirst, BPNode* pLast);
   BPNode*  getPivot     (bool ascending, unsigned long numNodes, BPNode* pFirst, BPNode* pLast, COMPARE_FUNC* pFunc, void* pParam);
   void    quickSort    (bool ascending, unsigned long numNodes, BPNode* pFirst, BPNode* pLast, COMPARE_FUNC* pFunc, void* pParam);
   void    insertionSort(bool ascending, unsigned long numNodes, BPNode* pFirst, BPNode* pLast, COMPARE_FUNC* pFunc, void* pParam);

   //-- Private Data
   unsigned long   mSize;
   unsigned long   mGrowSize;
   unsigned long   mAllocatedSize;
   
   BPBlock* mpBlocks;
   BPNode*  mpFreeHead;
   BPNode*  mpUsedHead;
   BPNode*  mpUsedTail;
   
   BMemoryHeap* mpHeap;
   
   bool    mGrowExponential;
};

//----------------------------------------------------------------------------
//  Template Implementation
//----------------------------------------------------------------------------
#include "PointerList.inl"

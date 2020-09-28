//============================================================================
//
//  CopyList.h
//  
//  Copyright (c) 1999-2002, Ensemble Studios
//
//============================================================================


#ifndef __COPY_LIST_H__
#define __COPY_LIST_H__


//----------------------------------------------------------------------------
//  Public Constants
//----------------------------------------------------------------------------
const unsigned long BCLIST_GROW_EXPONENTIAL = 0xFFFFFFFF;


//----------------------------------------------------------------------------
//  Class BCopyList
//----------------------------------------------------------------------------
template <class T> class BCopyList
{
public:
   //-- Callback Prototypes
   typedef long (CALLBACK COMPARE_FUNC)(const T& item1, const T& item2, void* pParam);

   //-- Construction/Destruction
   BCopyList(unsigned long initialSize = 0, unsigned long growSize = BCLIST_GROW_EXPONENTIAL);
   BCopyList(const BCopyList<T>& list);
   ~BCopyList();

   //-- List Control
   void   reset      (unsigned long initialSize = 0, unsigned long growSize = BCLIST_GROW_EXPONENTIAL);
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
   BHandle addToHead(const T& item);
   BHandle addToTail(const T& item);
   BHandle addBefore(const T& item, BHandle hItem);
   BHandle addAfter (const T& item, BHandle hItem);
   BHandle addSorted(const T& item, bool ascending = true);
   BHandle addSorted(const T& item, COMPARE_FUNC* pFunc, void* pParam, bool ascending = true);

   //-- Removing Items
   void   removeHead      ();
   void   removeTail      ();
   void   remove          (BHandle hItem);
   T*      removeAndGetPrev(BHandle& hItem);
   T*      removeAndGetNext(BHandle& hItem);

   //-- Relocation
   void   moveToHead(BHandle hItem);
   void   moveToTail(BHandle hItem);

   //-- Navigation   
   static T* getItem        (BHandle hItem); // jce [2/12/2003] -- made this static since it's just a cast and offset of the pointer.
   T*        getItem        (unsigned long index)   const;
   T*        getHead        (BHandle& hItem) const;
   T*        getTail        (BHandle& hItem) const;
   T*        getPrev        (BHandle& hItem) const;
   T*        getNext        (BHandle& hItem) const;
   T*        getPrevWithWrap(BHandle& hItem) const;
   T*        getNextWithWrap(BHandle& hItem) const;

   //-- Searching and Sorting
   BHandle findItemForward (const T& item, BHandle hStart = NULL) const;
   BHandle findItemBackward(const T& item, BHandle hStart = NULL) const;
   BHandle findItemForward (COMPARE_FUNC* pFunc, void* pParam, const T& item, BHandle hStart = NULL) const;
   BHandle findItemBackward(COMPARE_FUNC* pFunc, void* pParam, const T& item, BHandle hStart = NULL) const;
   void   quickSort       (bool ascending = true);
   void   insertionSort   (bool ascending = true);
   void   quickSort       (COMPARE_FUNC* pFunc, void* pParam, bool ascending = true);
   void   insertionSort   (COMPARE_FUNC* pFunc, void* pParam, bool ascending = true);

   //-- Operators
   BCopyList<T>& operator = (const BCopyList<T>& list);

private:
   //-- Private structs
   struct BCNode
   {
      BCNode* pPrev;
      BCNode* pNext;
      T       item;
   };

   struct BCBlock
   {
      unsigned long   numNodes;
      BCBlock* pNextBlock;
      BCNode*  pNodes;
   };

   //-- Helper Functions
   bool    isValidHandle(BHandle hItem) const;
   void    addToFreeHead(BCNode* pNode);
   void    addBlock     (unsigned long numNodes);
   BCNode*  getFreeNode  ();
   void    swapNodes    (BCNode*& pNode1, BCNode*& pNode2);
   void    linkToHead   (BCNode* pNode);
   void    linkToTail   (BCNode* pNode);
   void    unlink       (BCNode* pNode);

   //-- Sorting Functions
   BCNode*  getPivot     (bool ascending, unsigned long numNodes, BCNode*& pFirst, BCNode*& pLast);
   void    quickSort    (bool ascending, unsigned long numNodes, BCNode*  pFirst, BCNode*  pLast);
   void    insertionSort(bool ascending, unsigned long numNodes, BCNode*  pFirst, BCNode*  pLast);
   BCNode*  getPivot     (bool ascending, unsigned long numNodes, BCNode*& pFirst, BCNode*& pLast, COMPARE_FUNC* pFunc, void* pParam);
   void    quickSort    (bool ascending, unsigned long numNodes, BCNode*  pFirst, BCNode*  pLast, COMPARE_FUNC* pFunc, void* pParam);
   void    insertionSort(bool ascending, unsigned long numNodes, BCNode*  pFirst, BCNode*  pLast, COMPARE_FUNC* pFunc, void* pParam);

   //-- Private Data
   unsigned long   mSize;
   unsigned long   mGrowSize;
   unsigned long   mAllocatedSize;
   bool    mGrowExponential;
   BCBlock* mpBlocks;
   BCNode*  mpFreeHead;
   BCNode*  mpUsedHead;
   BCNode*  mpUsedTail;
};


//----------------------------------------------------------------------------
//  LIST ALIAS
//----------------------------------------------------------------------------
#define BCList BCopyList


//----------------------------------------------------------------------------
//  Template Implementation
//----------------------------------------------------------------------------
#include "CopyList.inl"


#endif





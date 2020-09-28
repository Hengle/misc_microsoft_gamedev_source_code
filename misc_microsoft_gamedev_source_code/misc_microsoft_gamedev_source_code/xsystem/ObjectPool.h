//============================================================================
//
//  ObjectPool.h
//  
//  Copyright 2003, Ensemble Studios
//
//============================================================================



#ifndef __OBJECT_POOL_H__
#define __OBJECT_POOL_H__


//----------------------------------------------------------------------------
//  Public Constants
//----------------------------------------------------------------------------
static const unsigned long BOBJECT_POOL_GROW_EXPONENTIAL = 0xFFFFFFFF;


//----------------------------------------------------------------------------
//  Class BObjectPool
//----------------------------------------------------------------------------
template <class T> class BObjectPool
{
public:
   //-- Callback Prototypes
   typedef bool (CALLBACK ENUM_OBJECT_FUNC)(T* pObject, void* pParam);

   //-- Construction/Destruction
   BObjectPool(unsigned long initialSize = 0, unsigned long growSize = BOBJECT_POOL_GROW_EXPONENTIAL);
   ~BObjectPool();

   //-- Pool Operations
   T*       createObject ();
   void    destroyObject(T* pObject);
   void    enumObjects  (ENUM_OBJECT_FUNC* pFunc, void* pParam) const;

   //-- Pool Control
   void    reset            (unsigned long initialSize = 0, unsigned long growSize = BOBJECT_POOL_GROW_EXPONENTIAL);
   void    setGrowSize      (unsigned long growSize);
   unsigned long   getSize          () const;
   unsigned long   getGrowSize      () const;
   unsigned long   getAllocatedSize () const;
   bool    isGrowExponential() const;

private:
   //-- Private Structures
   struct Node
   {
      Node* mpNext;
      Node* mpPrev;
      BYTE  mItem[sizeof(T)];
   };

   struct Block
   {
      Block* mpNext;
      Node*  mpNodes;
   };

   //-- Disable copy constructor and assignment operator.
   BObjectPool(const BObjectPool<T>& pool);
   BObjectPool<T>& operator = (const BObjectPool<T>& pool);

   //-- Private Functions
   void  grow();
   void  releaseBlocks();

   //-- Private Data
   unsigned long mSize;
   unsigned long mGrowSize;
   unsigned long mAllocatedSize;
   Node*  mpFreeList;
   Node*  mpUsedList;
   Block* mpBlocks;
};


//----------------------------------------------------------------------------
//  Template Implementation
//----------------------------------------------------------------------------
#include "ObjectPool.inl"


#endif





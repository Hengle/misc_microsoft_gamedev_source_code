//==============================================================================
// blockpool.h
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

#ifndef _BLOCKPOOL_H_
#define _BLOCKPOOL_H_

//#include "containers/dynamicArray.h"
#include "poolable.h"

template <class Type, 
          long initialBlockSize=100, 
          int Alignment = 16, //(__alignof(Type) > 0) ? __alignof(Type) : 1, 
          // Note: we changed alignment to 16 for now because __alignof was returning different things between the .h/.cpp files causing the compiler to break.
          bool UseConstructorDestructor = true, 
          bool ClearNewObjects = false>
class BBlockPool
{
   public:   
      BBlockPool() :
         mBlockSize(initialBlockSize)
      {
         if(mBlockSize<1)
            mBlockSize=100;
      }

      BBlockPool(long blockSize) :
         mBlockSize(blockSize)
      {
         if(mBlockSize<1)
            mBlockSize=100;
      }

      ~BBlockPool()
      {
        clear();
      }

      void clear(void)
      {
          for(long blockIndex = 0; blockIndex < mBlocks.getNumber(); blockIndex++)   
          {
            Type* pAligned = mBlocks[blockIndex];
            if ((UseConstructorDestructor) && (!BIsBuiltInType<Type>::Flag))
            {   
               for (long idx = 0; idx < mBlockSize; idx++)
                  Utils::DestructInPlace(&pAligned[idx]);
            }
            BAlignedAlloc::Free(pAligned, gSimHeap);
          }
          mBlocks.clear();
          mUnused.clear();
      }
      
      void allocateBlocks(void)
      {
         BDEBUG_ASSERT(mBlockSize);
         
         // Allocate new block
         Type* pNewAligned = static_cast<Type*>(BAlignedAlloc::Malloc(mBlockSize * sizeof(Type), Alignment, gSimHeap));
                  
         //Add raw address to raw list
         mBlocks.add(pNewAligned);
                  
         //Construct the elements 
         constructElements(mBlockSize, pNewAligned);
         
         // Add each of the pointers to the unused list 
         for(long i=0; i<mBlockSize; i++)
            mUnused.add(pNewAligned+i);
      }

      void setBlockSize(long size) 
      {
         mBlockSize=size;
         
         if(mBlockSize<1)
            mBlockSize=100;
      }

      long getBlockSize(void) const
      {
         return(mBlockSize);
      }

      long getNumberBlocks( void )
      {
         return mBlocks.getNumber();
      }

      long getNumberFree( void )
      {
         return mUnused.getNumber();
      }

      long getTypeSize( void )
      {
         return sizeof(Type);
      }
      
      Type *get(void)
      {
         // If there's something in the unused list, hand that out.
         if(mUnused.getNumber()>0)
         {
            // Grab pointer.
            long index=mUnused.getNumber()-1;
            Type *temp=mUnused[index];

            // Shrink unused list.
            mUnused.setNumber(index);

            // Return pointer.
            return(temp);
         }

         // Ok, no free ones so we'll have to allocate another block.
         allocateBlocks();
         
         // Grab pointer.
         long index=mUnused.getNumber()-1;
         Type *temp=mUnused[index];

         // Shrink unused list.
         mUnused.setNumber(index);

         // Return pointer.
         return(temp);
      }

      void release(Type *item)
      {
         /*
         long count=mUnused.getNumber();
         for(long i=0; i<count; i++)
         {
            if(mUnused[i]==item)
            {
               BASSERT(0);
               return;
            }
         }
         */
         mUnused.add(item);
      }

   protected:
      long                        mBlockSize;
      BDynamicSimArray<Type*>     mBlocks;
      BDynamicSimArray<Type*>     mUnused;
      
      void constructElements(uint num, Type* pDst)
      {
         if (BIsBuiltInType<Type>::Flag)
         {          
            if ((ClearNewObjects) || (UseConstructorDestructor))
            {
               Utils::FastMemSet(pDst, 0, num * sizeof(Type));
            }
         }
         else
         {
            if (ClearNewObjects)
               Utils::FastMemSet(pDst, 0, num * sizeof(Type));

            if (UseConstructorDestructor)
            {
               const Type* pEndDst = pDst + num;
               while (pDst != pEndDst)
               {           
                  Utils::ConstructInPlace(pDst);               
                  pDst++;
               }
            }
         }         
      }
};

//-- helpers for class based block pools
#define DECLARE_BLOCKPOOL(T, size) \
static T  *getInstance( void ); \
static void releaseInstance( T *pObject ); \
static BBlockPool<T, size> mPool; \
static void clearBlockPool( void ) { mPool.clear(); } \
static long getNumberBlocks( void ) { return mPool.getNumberBlocks(); } \
static long getNumberAllocations( void ) { return (mPool.getNumberBlocks() * mPool.getBlockSize()); } \
static long getNumberFree( void ) { return mPool.getNumberFree(); } \
friend class BBlockPool<T, size>;

#define IMPLEMENT_BLOCKPOOL(T, size) \
BBlockPool<T, size> T::mPool; \
T* T::getInstance( void ) \
{ \
T* pObject = mPool.get(); \
if (pObject) pObject->acquire(); \
return (pObject); \
} \
\
void T::releaseInstance( T *pObject ) \
{ \
   if (pObject) pObject->release(); \
   mPool.release(pObject); \
} 

//BlockPool Test Code
#ifdef BUILD_DEBUG
class BlockPoolTest
{
public:
   BVector mVector;
   static void unitTest(long numItems)
   {
      BDynamicSimArray <BlockPoolTest *> mA;
      BDynamicSimArray <BlockPoolTest *> mB;

      BBlockPool<BlockPoolTest> *pPool = new BBlockPool<BlockPoolTest>;
      for(long i = 0; i < numItems; i++)
      {      
         BlockPoolTest *pA = pPool->get();
         mA.add(pA);
         BlockPoolTest *pB = pPool->get();
         mB.add(pB);

         float result = pA->mVector.dot(pB->mVector); result;
      }

      delete pPool;   
   }
};
#endif

#endif  //_BLOCKPOOL_H_

//==============================================================================
// eof: blockpool.h
//==============================================================================


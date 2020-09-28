//==============================================================================
// utsimplemanager.h
//
// right now this just wraps freelist
// in the future, this will have an allocator and id space manager to support
// all the types of managers we need in the game.  Use this to provide a quick
/// upgrade path to those features.
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

#include "containers\freelist.h"
#include "poolable.h"

#pragma once 

// If Growable is true, you cannot persist pointers returned by create, because
// they will go stale if the container backing this manager resizes. Use indices 
// instead.

template<class ValueType, bool Growable = false>
class UTSimpleManager
{
public:

   enum { growable = Growable };

   //-- init
   virtual bool init( long initialSize )
   {
      //-- note: this does not "check out" any objects
      //-- it only sets up an initial size
      //-- create will use these objects first (if growing is enabled)
      mUsedList.setNumber(initialSize);
      mInUseCount = 0;
      return mData.init(initialSize);
   }

   //-- reset
   virtual bool reset( void)
   {
      mData.clear();
      mInUseCount = 0;
      return (true);
   }

   //-- create
   virtual ValueType* create( long &id )
   {
      ValueType* pNew =  mData.acquire((uint&)id);
      if (!pNew)
         return NULL;

      pNew->acquire();
      addToUsedList(id);
      mInUseCount++;

      return pNew;
   }

   //-- release
   virtual bool release( long id )
   {
      ValueType *pReleasing = get(id);

      removeFromUsedList(id);      
      mData.release(id);

      if (pReleasing)
         pReleasing->release();

      BASSERT(mInUseCount > 0);
      mInUseCount--;

      return (true);
   }

   //-- getConst
   virtual const ValueType * getConst( long id ) const
   {
       if (!inUse(id))
          return NULL;

       return mData.getConst(id);
   }

   //-- get
   virtual ValueType*        get( long id )
   {
      return const_cast<ValueType*>(getConst(id));
   }

   //-- inUse
   bool inUse( long id ) const
   {
      if (id < 0 || id >= mUsedList.getNumber())
         return (false);

      return (mUsedList.isBitSet(id)!=0);
   }

   //-- getCapacity
   long  getCapacity( void )
   {
      return (long) mUsedList.getNumber();
   }

   long getInUseCount(void) const { return mInUseCount; }

   //-- protected data
protected:

    bool removeFromUsedList( uint id )
    {
       if (mUsedList.isBitSet(id) == 0)
          return (false);

       mUsedList.clearBit(id);
       return (true);
    }

    void addToUsedList( long id )
    {
       //-- make room if we have to
       if (id >= mUsedList.getNumber())
            mUsedList.setNumber(id+1);

       mUsedList.setBit(id);
    }

    long mInUseCount;
    BFreeList<ValueType                         mData;
    BBitArray                                   mUsedList;
};
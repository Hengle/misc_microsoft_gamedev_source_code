//==============================================================================
// entityref.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#ifndef _ENTITYREF_H_
#define _ENTITYREF_H_

// Includes
#include "poolable.h"

//==============================================================================
// BEntityRef
//==============================================================================
class BEntityRef
{
   public:
      enum
      {
         cTypeNone,
         cTypeTrainLimit,
      };

      BEntityRef() : mType(0), mID(-1) {}
      BEntityRef(long type, long id) : mType(type), mID(id) {}

      long  mType;
      long  mID;

};

//==============================================================================
// BEntityRefList
//==============================================================================
class BEntityRefList : public IPoolable
{
   public:

      //-- IPoolable implementation
      virtual void acquire();
      virtual void release();

      BStaticArray<class BEntityRef,4,false,false,4>  mData;

   protected:
      BEntityRefList() {}
      ~BEntityRefList() {}

};

#endif

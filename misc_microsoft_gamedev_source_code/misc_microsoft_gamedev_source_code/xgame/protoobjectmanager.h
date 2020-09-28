//==============================================================================
// protoobjectmanager.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#ifndef _PROTOOBJECTMANAGER_H_
#define _PROTOOBJECTMANAGER_H_

// Forward declarations
class BProtoObject;

//==============================================================================
// BProtoObjectManager
//==============================================================================
class BProtoObjectManager
{
   public:
      BProtoObjectManager();
      ~BProtoObjectManager();

      bool setup();
      void shutdown();

      BProtoObject* getProtoObject(long id) const;

   protected:
      BDynamicArray<BProtoObject*> mProtoObjects;

};

#endif

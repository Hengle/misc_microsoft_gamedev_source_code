//==============================================================================
// physicsinfomanager.h
//
// Copyright (c) 2004-2007 Ensemble Studios
//==============================================================================
#pragma once 


//==============================================================================
// Includes
#include "string\ustringtable.h"


//==============================================================================
// Forward declarations
class BPhysicsInfo;
class BPhysicsVehicleInfo;

//==============================================================================
// Const declarations


//==============================================================================
// BPhysicsInfoManager
//==============================================================================
class BPhysicsInfoManager
#ifdef ENABLE_RELOAD_MANAGER
   : public BEventReceiver
#endif
{
   public:
                              BPhysicsInfoManager();
                              ~BPhysicsInfoManager();

      bool                    init(void);
      void                    cleanup(void);

      // This is the basic function to call to get a physics info index for a given physics info, creating a new one
      // if necessary.
      long                    getOrCreate(const BCHAR_T *filename, bool forceLoad=false);

      BPhysicsInfo            *get(long index, bool load=true);

      long                    find(const BCHAR_T *filename);
      long                    create(const BCHAR_T *filename);

      void                    unloadAll(void);

      const BUStringTable<long>& getNameTable() const { return mNameTable; }
      int                        getNumberPhysicsInfos() const { return mPhysicsInfos.getNumber(); }

   protected:

#ifdef ENABLE_RELOAD_MANAGER
      bool                    receiveEvent(const BEvent& event, BThreadIndex threadIndex);
#endif
      void                    resetPhysicsInfo(BPhysicsInfo* pInfo, BPhysicsVehicleInfo* pOldData, BPhysicsVehicleInfo* pNewData);

      BDynamicSimArray<BPhysicsInfo*> mPhysicsInfos;
      BUStringTable<long>     mNameTable;
};


extern BPhysicsInfoManager gPhysicsInfoManager;

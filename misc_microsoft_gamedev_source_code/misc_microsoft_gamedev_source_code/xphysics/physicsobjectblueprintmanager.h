//==============================================================================
// physicsobjectblueprintmanager.h
//
// Copyright (c) 2004 Ensemble Studios
//==============================================================================



#ifndef _PHYSICSOBJECTBLUEPRINTMANAGER_H_
#define _PHYSICSOBJECTBLUEPRINTMANAGER_H_

//==============================================================================
// Includes
#include "string\ustringtable.h"


//==============================================================================
// Forward declarations
class BPhysicsObjectBlueprint;

//==============================================================================
// Const declarations


//==============================================================================
// BPhysicsObjectBlueprintManager
//==============================================================================
class BPhysicsObjectBlueprintManager
{
   public:
                              BPhysicsObjectBlueprintManager();
                              ~BPhysicsObjectBlueprintManager();

      // base directory ID
      void                    setBaseDirectoryID(long ID) { mBaseDirectoryID = ID; }
      long                    getBaseDirectoryID(void) const { return mBaseDirectoryID; }

      bool                    init(void);
      void                    cleanup(void);

      // This is the basic function to call to get a blueprint index for a given blueprint, creating a new one
      // if necessary.
      long                    getOrCreate(const BCHAR_T *filename, bool forceLoad=false);

      BPhysicsObjectBlueprint *get(long index, bool load=true);

      long                    find(const BCHAR_T *filename);
      long                    create(const BCHAR_T *filename);

      void                    unloadAll(void);

   protected:
      long                    mBaseDirectoryID;

      BDynamicSimArray<BPhysicsObjectBlueprint*>   mBlueprints;
      BUStringTable<long>     mNameTable;
};



#endif

//==============================================================================
// eof: physicsobjectblueprintmanager.h
//==============================================================================

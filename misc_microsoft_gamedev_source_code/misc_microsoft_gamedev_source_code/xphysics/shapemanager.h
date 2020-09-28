//==============================================================================
// shapemanager.h
//
// Copyright (c) 2004 Ensemble Studios
//==============================================================================



#ifndef _SHAPEMANAGER_H_
#define _SHAPEMANAGER_H_

//==============================================================================
// Includes
#include "string\ustringtable.h"


//==============================================================================
// Forward declarations
class BShape;

//==============================================================================
// Const declarations


//==============================================================================
// BShapeManager
//==============================================================================
class BShapeManager
{
   public:
                              BShapeManager();
                              ~BShapeManager();

      // base directory ID
      void                    setBaseDirectoryID(long ID) { mBaseDirectoryID = ID; }
      long                    getBaseDirectoryID(void) const { return mBaseDirectoryID; }

      bool                    init(void);
      void                    cleanup(void);

      // This is the basic function to call to get a shape index for a given shape, creating a new one
      // if necessary.
      long                    getOrCreate(const BCHAR_T *filename, bool forceLoad=false);

      BShape                  *get(long index, bool load=true);

      long                    find(const BCHAR_T *filename);
      long                    create(const BCHAR_T *filename);

      // model system callback
      static long             shapeIDFunc(const BCHAR_T *filename);

      void                    loadAll(void);
      void                    unloadAll(void);

      long                    addDynamicShape(BShape *pShape, const BCHAR_T *filename);

      bool                    resetShape(long id );

      const BUStringTable<long>& getNameTable() const { return mNameTable; }
      int                        getNumberShapes() const { return mShapes.getNumber(); }

protected:
      long                    mBaseDirectoryID;

      BDynamicSimArray<BShape*>   mShapes;
      BUStringTable<long>     mNameTable;
};





//==============================================================================
#endif // _SHAPEMANAGER_H_

//==============================================================================
// eof: shapemanager.h
//==============================================================================

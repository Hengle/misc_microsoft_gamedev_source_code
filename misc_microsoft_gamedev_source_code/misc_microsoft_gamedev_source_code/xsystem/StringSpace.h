//==============================================================================
// StringSpace.h
//
// Copyright (c) 1999, Ensemble Studios
//
// WARNINGS: DYNAMIC ALLOCATION
// + This data structure dynamically grows when adding or when asked to extend the
//   allocation.  The pMoved parameter on each of these calls can be used to learn
//   if the allocation has moved - which would invalidate any saved pointers. To 
//   learn if the allocation is moved pass a pointer to a boolean.  If you don't
//   care because you don't save pointers pass in NULL.
// + Beaware that load effectivly moves the data - so any pointer must be
//   recalculated after a load
//
//==============================================================================

#ifndef _STRINGSPACE_H_
#define _STRINGSPACE_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations

#include "DynamicSpace.h"

//==============================================================================
// Const declarations


//==============================================================================
class BStringSpace 
{
   public:
      BStringSpace() : mDynSpace(BStringSpace::tag, sizeof(char), BStringSpace::extent) {};

      // control functions
      bool     setAllocationCount(bool * pMoved, long count) { return mDynSpace.setAllocationCount(pMoved, count); };
      long     getUsedCount(void) const { return mDynSpace.getUsedCount(); };
      void     clearData(void) { mDynSpace.clearData(); };
      
      // access functions
      char *   getString(long index) const { return (char *) mDynSpace.getPointer(index); };
      long     add(bool * pMoved, const char * ptr) { return mDynSpace.add(pMoved, ptr, strlen(ptr) + 1); };

      // game functions
      long     checksum(void) const { return mDynSpace.checksum(); };
      bool     save(BChunkWriter *writer) { return mDynSpace.save(writer); };
      bool     load(BChunkReader *reader) { return mDynSpace.load(reader); };
      // FIXME: These functions below are left intact because I don't want to go rewrite all of XS's save/load
      // stuff right now (to use the chunker), but that should definitely be done at some point - pdb 7/31/00
      bool     save(BFile *pFile) { return mDynSpace.save(pFile); };
      bool     load(BFile *pFile) { return mDynSpace.load(pFile); };

      // misc functions
      long     copyData(unsigned char * buffer) { return mDynSpace.copyData(buffer); };

   protected:

      // Enums
      enum {
         tag = 0x12435687,
         extent = 1024
      };

      // Variables
      BDynamicSpace      mDynSpace;

}; // BStringSpace


//==============================================================================
#endif // _STRINGSPACE_H_

//==============================================================================
// eof: StringSpace.h
//==============================================================================


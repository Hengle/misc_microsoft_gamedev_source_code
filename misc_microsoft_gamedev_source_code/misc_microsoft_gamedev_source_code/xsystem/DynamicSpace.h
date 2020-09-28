//==============================================================================
// DynamicSpace.h
//
// Copyright (c) 1999, Ensemble Studios
//
// This is a base class for building dynamic spaces.  These can have variable
// sized data (like strings) or fixed sized (like structs).  
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

#ifndef _DYNAMICSPACE_H_
#define _DYNAMICSPACE_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations

class BChunkWriter;
class BChunkReader;

//==============================================================================
// Const declarations


//==============================================================================
class BDynamicSpace
{
   public:
      enum {
         AddFailed = -1,
      };

      // Constructors
      BDynamicSpace(long tag, long itemSize, long extentCount);

      // Destructors
      ~BDynamicSpace(void);

      // control functions
      void     clearData(void) { mUsedCount = 0; };
      void     clearDataTo(long newUsedCount) { BASSERT(newUsedCount <= mUsedCount); mUsedCount = newUsedCount; };
      bool     setAllocationCount(bool * pMoved, long count);
      long     getUsedCount(void) const { return mUsedCount; };
      
      // access functions
      void *   getPointer(long index) const 
      { 
         if ((index >= 0) && (index < mUsedCount))
            return mpSpace + (index * mItemSize);
         else
            return NULL;
      };
      long     add(bool * pMoved);     // adds a zero filled entry
      long     add(bool * pMoved, const void * ptr, long numberToAdd = 1);// returns offset or BDynamicSpace::AddFailed

      // game functions
      static long dataChecksum(const void * ptr, long length, long initial);
      long        checksum(void) const;             // returns checksum value
      bool        save(BChunkWriter * writer);
      bool        load(BChunkReader * reader);
      // FIXME: These functions below are left intact because I don't want to go rewrite all of XS's save/load
      // stuff right now (to use the chunker), but that should definitely be done at some point - pdb 7/31/00
      bool        save(BFile *pFile);
      bool        load(BFile *pFile);

      // misc functions
      long     copyData(unsigned char * buffer);

   protected:
      // Variables
      long              mTag;                     // identifies this data
      long              mItemSize;                // item size
      long              mExtentCount;             // count to grow by
      long              mAllocationCount;             
      long              mUsedCount;
      unsigned char *   mpSpace;                  // the data

}; // BDynamicSpace


//==============================================================================
#endif // _DYNAMICSPACE_H_

//==============================================================================
// eof: DynamicSpace.h
//==============================================================================


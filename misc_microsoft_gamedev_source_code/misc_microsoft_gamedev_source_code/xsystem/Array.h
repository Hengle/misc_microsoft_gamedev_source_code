//============================================================================
//
//  Array.h
//  
//  Copyright 2002, Ensemble Studios
//
//============================================================================



#ifndef __ARRAY_H__
#define __ARRAY_H__


//----------------------------------------------------------------------------
//  Public Constants
//----------------------------------------------------------------------------
static const unsigned long BARRAY_GROW_EXPONENTIAL = 0xFFFFFFFF;
static const unsigned long BARRAY_ITEM_NOT_FOUND   = 0xFFFFFFFF;
static const unsigned long BARRAY_REMOVE_ALL       = 0x00000000;
static const unsigned long BARRAY_REMOVE_FIRST     = 0x00000001;
static const unsigned long BARRAY_REMOVE_LAST      = 0x00000002;


//----------------------------------------------------------------------------
//  Class BArray
//----------------------------------------------------------------------------
template <class T> class BArray
{
public:
   //-- Construction/Destruction
   BArray(unsigned long initialSize = 0, unsigned long growSize = BARRAY_GROW_EXPONENTIAL);
   ~BArray();

   //-- Array Operations
   void  empty       ();
   unsigned long add         (const T& item);
   unsigned long addUnique   (const T& item);
   void  removeIndex (unsigned long index, bool preserveOrder = true);
   void  removeItem  (const T& item, unsigned long method = BARRAY_REMOVE_ALL, bool preserveOrder = true);
   void  insertAfter (unsigned long index, const T& item);
   void  insertBefore(unsigned long index, const T& item);
   void  set         (unsigned long index, const T& item);
   void  setAll      (const T& item);
   bool  isEmpty     () const;
   unsigned long getSize     () const;
   T*     getArray    ();
   T*     lockArray   (unsigned long requestedSize = -1);
   void  unlockArray (unsigned long newSize = -1);

   //-- Searching and Sorting
//   void  sort        (bool ascending = true);
//   unsigned long binarySearch(const T& item, unsigned long firstIndex = -1, unsigned long lastIndex = -1) const;
   unsigned long findLeft    (const T& item, unsigned long startIndex = -1) const;
   unsigned long findRight   (const T& item, unsigned long startIndex = -1) const;

   //-- Array Control
   void  reset            (unsigned long initialSize = 0, unsigned long growSize = BARRAY_GROW_EXPONENTIAL);
   void  setGrowSize      (unsigned long growSize);
   void  setAllocatedSize (unsigned long allocatedSize);
   unsigned long getGrowSize      () const;
   unsigned long getAllocatedSize () const;
   bool  isGrowExponential() const;
   void  enableGrowToIndex(bool enable);

   //-- Operators
   BArray<T>& operator =  (const BArray<T>& array);
   bool      operator == (const BArray<T>& array) const;
   bool      operator != (const BArray<T>& array) const;
   T&         operator [] (unsigned long index);
   const T&   operator [] (unsigned long index) const;

private:
   //-- Private Functions
   void  grow();

   //-- Private Data
   unsigned long mSize;
   unsigned long mGrowSize;
   unsigned long mAllocatedSize;
   bool  mGrowToIndex;
   T*     mpArray;
};


//----------------------------------------------------------------------------
//  Template Implementation
//----------------------------------------------------------------------------
#include "Array.inl"


#endif





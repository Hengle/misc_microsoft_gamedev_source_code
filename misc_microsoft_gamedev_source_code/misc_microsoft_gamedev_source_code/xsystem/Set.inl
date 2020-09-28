//============================================================================
//
//  Set.inl
//  
//  Copyright 2002, Ensemble Studios
//
//============================================================================


//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
template <class T> BSet<T>::BSet(unsigned long initialSize, unsigned long growSize)
{
   //-- Init all members
   mSize          = 0;
   mGrowSize      = 0;
   mAllocatedSize = 0;
   mpArray        = NULL;

   //-- Set up initial allocation.  This won't allocate anything if
   //-- initialSize is 0.
   reset(initialSize, growSize);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BSet<T>::~BSet()
{
   //-- Release memory.
   reset(0, 0);
}


//============================================================================
//  SET OPERATIONS
//============================================================================
template <class T> void BSet<T>::empty()
{
   //-- Just reset the size.
   mSize = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BSet<T>::add(const T& item)
{
   //-- See if we already have it.
   unsigned long index = find(item);
   if (index != BSET_ITEM_NOT_FOUND)
      return index;

   //-- See if we need to grow.
   if (mSize == mAllocatedSize)
      grow();

   //-- We should be able to add it now.
   BFATAL_ASSERT(mpArray);
   BFATAL_ASSERT(mSize < mAllocatedSize);

   //-- Add it.
   index = mSize++;
   mpArray[index] = item;

   //-- Return the index.
   return index;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BSet<T>::add(const BSet<T>& set)
{
   //-- If set is empty, we are done.
   if (set.mSize == 0)
      return mSize;

   //-- Add each item.
   BFATAL_ASSERT(set.mpArray);
   for (unsigned long index = 0; index < set.mSize; ++index)
      add(set.mpArray[index]);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BSet<T>::remove(unsigned long index, bool preserveOrder)
{
   //-- Non fatal error.
   BASSERT(index < mSize);
   if (index >= mSize)
      return;

   //-- Sanity
   BFATAL_ASSERT(mpArray);
   BFATAL_ASSERT(mSize);

   //-- Check for easy out.
   if (index == (mSize - 1))
   {
      mSize--;
      return;
   }

   //-- Do a "safe" remove.
   if (preserveOrder)
   {
      for (unsigned long index2 = index; index2 < mSize - 1; ++index2)
         mpArray[index2] = mpArray[index2 + 1];
      mSize--;      
      return;
   }

   //-- Do a "fast" remove.
   mpArray[index] = mpArray[mSize - 1];
   mSize--;      
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BSet<T>::remove(const T& item, bool preserveOrder)
{
   unsigned long index = find(item);
   if (index != BSET_ITEM_NOT_FOUND)
      remove(index, preserveOrder);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BSet<T>::remove(const BSet<T>& set, bool preserveOrder)
{
   //-- If set is empty, we are done.
   if (set.mSize == 0)
      return;

   //-- Remove each element that is in set.
   BFATAL_ASSERT(set.mpArray);
   for (unsigned long index = 0; index < set.mSize; index++)
      remove(set.mpArray[index], preserveOrder);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BSet<T>::find(const T& item) const
{
   //-- If we are empty, we are done.
   if (mSize == 0)
      return BSET_ITEM_NOT_FOUND;

   //-- Scan.
   BFATAL_ASSERT(mpArray);
   for (unsigned long index = 0; index < mSize; ++index)
   {
      if (mpArray[index] == item)
         return index;
   }

   //-- Not found.
   return BSET_ITEM_NOT_FOUND;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BSet<T>::contains(const T& item) const
{
   return (find(item) != BSET_ITEM_NOT_FOUND);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BSet<T>::contains(const BSet<T>& set) const
{
   //-- If set is empty, we say we contain it.
   if (set.mSize == 0)
      return true;

   //-- Check for easy out.
   if (set.mSize > mSize)
      return false;

   //-- See if this set contains all members of set.
   BFATAL_ASSERT(set.mpArray);
   for (unsigned long index = 0; index < set.mSize; ++index)
   {
      if (!contains(set.mpArray[index]))
         return false;
   }

   //-- Success.
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BSet<T>::isSubsetOf(const BSet<T>& set) const
{
   return set.contains(*this);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BSet<T>::isSupersetOf(const BSet<T>& set) const
{
   return contains(set);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BSet<T>::isEmpty() const
{
   return (mSize == 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BSet<T>::getNumItems() const
{
   return mSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> const T& BSet<T>::getItem(unsigned long index) const
{
   BFATAL_ASSERT(index < mSize);
   BFATAL_ASSERT(mpArray);
   return mpArray[index];
}


//============================================================================
//  DOUBLE SET OPERATIONS
//============================================================================
template <class T> unsigned long setToUnion(const BSet<T>& set1, const BSet<T>& set2)
{
   //-- Reset.
   empty();

   //-- Add all of set 1.
   if (set1.mSize)
   {
      BFATAL_ASSERT(set1.mpArray);
      for (unsigned long index = 0; index < set1.mSize; ++index)
         add(set1.mpArray[index]);
   }

   //-- Add all of set 2.
   if (set2.mSize)
   {
      BFATAL_ASSERT(set2.mpArray);
      for (unsigned long index = 0; index < set2.mSize; ++index)
         add(set2.mpArray[index]);
   }

   //-- Return the size.
   return mSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long setToIntersection(const BSet<T>& set1, const BSet<T>& set2)
{
   //-- Reset.
   empty();

   //-- Test for easy cases.
   if (set1.mSize == 0)
      *this = set2;
   if (set2.mSize == 0)
      *this = set1;

   //-- Add each element that is in both.
   BFATAL_ASSERT(set1.mpArray);
   for (unsigned long index = 0; index < set1.mSize; ++index)
   {
      if (set2.contains(set1.mpArray[index]))
         add(set1.mpArray[index]);
   }

   //-- Return the size.
   return mSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long setToDifference(const BSet<T>& set1, const BSet<T>& set2)
{
   //-- Reset.
   empty();

   //-- Test for easy cases.
   if (set2.mSize == 0)
      *this = set1;
   if (set1.mSize == 0)
      return;

   //-- Add each element that is in set1, but not set2.
   BFATAL_ASSERT(set1.mpArray);
   for (unsigned long index = 0; index < set1.mSize; ++index)
   {
      if (!set2.contains(set1.mpArray[index]))
         add(set1.mpArray[index]);
   }

   //-- Return the size.
   return mSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long setToSymmetricDifference(const BSet<T>& set1, const BSet<T>& set2)
{
   //-- Reset.
   empty();

   //-- Test for easy cases.
   if (set1.mSize == 0)
      *this = set2;
   if (set2.mSize == 0)
      *this = set1;

   //-- Add each element that is in set1 or set2, but not in both.
   BFATAL_ASSERT(set1.mpArray);
   for (unsigned long index = 0; index < set1.mSize; ++index)
   {
      if (!set2.contains(set1.mpArray[index]))
         add(set1.mpArray[index]);
   }
   BFATAL_ASSERT(set2.mpArray);
   for (index = 0; index < set2.mSize; ++index)
   {
      if (!set1.contains(set2.mpArray[index]))
         add(set2.mpArray[index]);
   }

   //-- Return the size.
   return mSize;
}


//============================================================================
//  STACK CONTROL
//============================================================================
template <class T> void BSet<T>::reset(unsigned long initialSize, unsigned long growSize)
{
   //-- Release the old data.
   if (mpArray)
      delete [] mpArray;

   //-- Reset members.
   mSize            = 0;
   mGrowSize        = growSize;
   mAllocatedSize   = initialSize;

   //-- Allocate storage if necessary.
   if (initialSize)
      mpArray = new T [initialSize];
   else
      mpArray = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BSet<T>::setGrowSize(unsigned long growSize)
{
   mGrowSize = growSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BSet<T>::setAllocatedSize(unsigned long allocatedSize)
{
   //-- You are NOT allowed chop off part of the set.  The caller is
   //-- responsible for validating this.
   BFATAL_ASSERT(allocatedSize >= mAllocatedSize);

   //-- See if we can just bail.
   if (allocatedSize == mAllocatedSize)
      return;

   //-- Compute the new size.
   unsigned long oldSize = mAllocatedSize;
   mAllocatedSize = allocatedSize;

   //-- Allocate the new array.
   T* pOldArray = mpArray;
   mpArray = new T [mAllocatedSize];

   //-- Copy the old data in.
   for (unsigned long index = 0; index < oldSize; ++index)
      mpArray[index] = pOldArray[index];

   //-- Free the old array.
   if (pOldArray)
      delete [] pOldArray;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BSet<T>::getGrowSize() const
{
   return mGrowSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BSet<T>::getAllocatedSize() const
{
   return mAllocatedSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BSet<T>::isGrowExponential() const
{
   return (mGrowSize == BSET_GROW_EXPONENTIAL);
}


//============================================================================
//  OPERATORS
//============================================================================
template <class T> BSet<T>& BSet<T>::operator = (const BSet<T>& set)
{
   //-- Clear ourself.
   reset(set.mAllocatedSize, set.mGrowSize);

   //-- See if we are empty.
   if (set.mAllocatedSize == 0)
      return *this;

   //-- Sanity.
   BFATAL_ASSERT(mpArray);
   BFATAL_ASSERT(mAllocatedSize >= set.mSize);

   //-- Copy the items in.
   mSize = set.mSize;
   for (long index = 0; index < mSize; ++index)
      mpArray[index] = set.mpArray[index];

   //-- Return ourself.
   return *this;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BSet<T>::operator == (const BSet<T>& set) const
{
   //-- Search for quick differences first.
   if (mSize != set.mSize)
      return false;

   //-- The size is the same, and if the size is 0 they must be equal.
   if (mSize == 0)
      return true;

   //-- Knowing that the size is the same, then if this set contains the
   //-- other, we know they must be equal.
   if (!contains(set))
      return false;

   //-- They are the same.
   return true;
}


//============================================================================
//  PRIVATE FUNCTIONS
//============================================================================
template <class T> void BSet<T>::grow()
{
   BFATAL_ASSERT(mSize == mAllocatedSize);
   BFATAL_ASSERT(mGrowSize > 0);
   BFATAL_ASSERT((mAllocatedSize == 0 && mpArray == NULL) || (mAllocatedSize > 0 && mpArray != NULL));

   //-- Compute the new size.
   unsigned long oldSize = mAllocatedSize;
   mAllocatedSize = (mGrowSize == BSET_GROW_EXPONENTIAL) ? mAllocatedSize * 2 : mAllocatedSize + mGrowSize;
   if (mAllocatedSize == 0)
      mAllocatedSize = 1;

   //-- Allocate the new array.
   T* pOldArray = mpArray;
   mpArray = new T [mAllocatedSize];

   //-- Copy the old data in.
   for (unsigned long index = 0; index < oldSize; ++index)
      mpArray[index] = pOldArray[index];

   //-- Free the old array.
   if (pOldArray)
      delete [] pOldArray;
}





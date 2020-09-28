//============================================================================
//
//  Array.inl
//  
//  Copyright 2002, Ensemble Studios
//
//============================================================================


//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
template <class T> BArray<T>::BArray(unsigned long initialSize, unsigned long growSize)
{
   //-- Init all members
   mSize          = 0;
   mGrowSize      = 0;
   mAllocatedSize = 0;
   mGrowToIndex   = false;
   mpArray        = NULL;

   //-- Set up initial allocation.  This won't allocate anything if
   //-- initialSize is 0.
   reset(initialSize, growSize);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BArray<T>::~BArray()
{
   //-- Release memory.
   reset(0, 0);
}


//============================================================================
//  ARRAY OPERATIONS
//============================================================================
template <class T> void BArray<T>::empty()
{
   mSize = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BArray<T>::add(const T& item)
{
   //-- See if we need to grow.
   BFATAL_ASSERT(mSize <= mAllocatedSize);
   if (mSize == mAllocatedSize)
   {
      grow();
      BFATAL_ASSERT(mSize < mAllocatedSize);
   }

   //-- Add the element.
   BFATAL_ASSERT(mpArray);
   unsigned long index = mSize++;
   mpArray[index] = item;

   //-- Return the index.
   return index;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BArray<T>::addUnique(const T& item)
{
   //-- See if we already have it.
   unsigned long index = findLeft(item);
   if (index != BARRAY_ITEM_NOT_FOUND)
      return index;

   //-- Just add it normally.
   return add(item);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BArray<T>::removeIndex(unsigned long index, bool preserveOrder)
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
template <class T> void BArray<T>::removeItem(const T& item, unsigned long method, bool /*preserveOrder*/)
{
   switch (method)
   {
      case BARRAY_REMOVE_ALL:
      {
         unsigned long index = findLeft(item);
         while (index != BARRAY_ITEM_NOT_FOUND)
         {
            removeIndex(index);
            index = findLeft(item, index);
         }
         return;
      }
      case BARRAY_REMOVE_FIRST:
      {
         unsigned long index = findLeft(item);
         if (index != BARRAY_ITEM_NOT_FOUND)
            removeIndex(index);
         return;
      }
      case BARRAY_REMOVE_LAST:
      {
         unsigned long index = findRight(item);
         if (index != BARRAY_ITEM_NOT_FOUND)
            removeIndex(index);
         return;
      }
   }

   BFATAL_FAIL("Invalid Argument:  method");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BArray<T>::insertAfter(unsigned long index, const T& item)
{
   //-- Nonfatal assert
   BASSERT(index < mSize);
   if (index >= mSize)
   {
      add(item);
      return;
   }

   //-- See if we need to grow.
   BFATAL_ASSERT(mSize <= mAllocatedSize);
   if (mSize == mAllocatedSize)
   {
      grow();
      BFATAL_ASSERT(mSize < mAllocatedSize);
   }

   //-- Move everything down.
   BFATAL_ASSERT(mpArray);
   for (unsigned long index2 = mSize; index2 > index + 1; --index2)
      mpArray[index2] = mpArray[index2 - 1];

   //-- Insert the item.
   mpArray[index + 1] = item;
   mSize++;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BArray<T>::insertBefore(unsigned long index, const T& item)
{
   //-- Nonfatal assert
   BASSERT(index < mSize);
   if (index >= mSize)
   {
      add(item);
      return;
   }

   //-- See if we need to grow.
   BFATAL_ASSERT(mSize <= mAllocatedSize);
   if (mSize == mAllocatedSize)
   {
      grow();
      BFATAL_ASSERT(mSize < mAllocatedSize);
   }

   //-- Move everything down.
   BFATAL_ASSERT(mpArray);
   for (unsigned long index2 = mSize; index2 > index; --index2)
      mpArray[index2] = mpArray[index2 - 1];

   //-- Insert the item.
   mpArray[index] = item;
   mSize++;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BArray<T>::set(unsigned long index, const T& item)
{
   //-- Nonfatal assert
   BASSERT(index < mSize);
   if (index >= mSize)
      return;

   //-- Set it.
   BFATAL_ASSERT(mpArray);
   mpArray[index] = item;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BArray<T>::setAll(const T& item)
{
   BFATAL_ASSERT((mSize == 0 && mpArray == NULL) || (mSize != 0 && mpArray != NULL));

   //-- Set all the items.
   for (unsigned long index = 0; index < mSize; ++index)
      mpArray[index] = item;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BArray<T>::isEmpty() const
{
   return (mSize == 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BArray<T>::getSize() const
{
   return mSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BArray<T>::getArray()
{
   return mpArray;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BArray<T>::lockArray(unsigned long requestedSize)
{
   if (requestedSize == -1)
      return mpArray;

   if (requestedSize > mAllocatedSize)
      setAllocatedSize(requestedSize);

   return mpArray;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BArray<T>::unlockArray(unsigned long newSize)
{
   if (newSize == -1)
      return;

   BFATAL_ASSERT(newSize <= mAllocatedSize);
   mSize = newSize;
   return;
}


//============================================================================
//  SEARCHING AND SORTING
//============================================================================
/*
template <class T> int __cdecl compareFunc(const void* pItem1, const void* pItem2)
{
   BFATAL_ASSERT(pItem1 && pItem2);
   T* p1 = (T*)pItem1;
   T* p2 = (T*)pItem2;

   //-- Do the comparison.
   if (*p1 < *p2)
      return -1;
   if (*p1 > *p2)
      return 1;
   return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BArray<T>::sort(bool ascending)
{
   //-- Check for easy out.
   if (mSize < 2)
      return;

   //-- Sanity.
   BFATAL_ASSERT(mpArray);

   //-- Sort it.
   qsort(mpArray, mSize, sizeof(T), BArray<T>::compareFunc);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BArray<T>::binarySearch(const T& item, unsigned long firstIndex, unsigned long lastIndex) const
{
   //-- See if array is empty.
   if (mSize == 0)
      return BARRAY_ITEM_NOT_FOUND;

   //-- Sanity.
   BFATAL_ASSERT(mpArray);

   //-- Fixup default 
   if (firstIndex == -1)
      firstIndex = 0;
   if (lastIndex == -1)
      lastIndex = mSize - 1;
   
   //-- Validate range.
   BFATAL_ASSERT(firstIndex <= lastIndex);
   BFATAL_ASSERT(lastIndex < mSize);

   //-- Search.
   T* pItem = (T*)bsearch(&item, &mpArray[firstIndex], lastIndex - firstIndex + 1, sizeof(T), compareFunc);
   if (pItem == NULL)
      return BARRAY_ITEM_NOT_FOUND;

   //-- Turn the pointer into an index.
   BFATAL_ASSERT(pItem >= &mpArray[firstIndex]);
   BFATAL_ASSERT(pItem <= &mpArray[lastIndex]);
   unsigned long index = (pItem - mpArray);
   return index;
}
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BArray<T>::findLeft(const T& item, unsigned long startIndex) const
{
   //-- See if array is empty.
   if (mSize == 0)
      return BARRAY_ITEM_NOT_FOUND;

   //-- Sanity.
   BFATAL_ASSERT(mpArray);

   //-- Fixup start index.
   if (startIndex == -1)
      startIndex = 0;
   if (startIndex >= mSize)
      return BARRAY_ITEM_NOT_FOUND;
   //BFATAL_ASSERT(startIndex < mSize);

   //-- Search.
   for (unsigned long index = startIndex; index < mSize; ++index)
   {
      if (mpArray[index] == item)
         return index;
   }

   //-- Not found.
   return BARRAY_ITEM_NOT_FOUND;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BArray<T>::findRight(const T& item, unsigned long startIndex) const
{
   //-- See if array is empty.
   if (mSize == 0)
      return BARRAY_ITEM_NOT_FOUND;

   //-- Sanity.
   BFATAL_ASSERT(mpArray);

   //-- Fixup start index.
   if (startIndex == -1)
      startIndex = mSize - 1;
   BFATAL_ASSERT(startIndex < mSize);

   //-- Search.
   for (unsigned long index = startIndex; index >= 0; --index)
   {
      if (mpArray[index] == item)
         return index;
   }

   //-- Not found.
   return BARRAY_ITEM_NOT_FOUND;
}


//============================================================================
//  ARRAY CONTROL
//============================================================================
template <class T> void BArray<T>::reset(unsigned long initialSize, unsigned long growSize)
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
template <class T> void BArray<T>::setGrowSize(unsigned long growSize)
{
   mGrowSize = growSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BArray<T>::setAllocatedSize(unsigned long allocatedSize)
{
   //-- You are NOT allowed chop off the end of the array.  The caller is
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
template <class T> unsigned long BArray<T>::getGrowSize() const
{
   return mGrowSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BArray<T>::getAllocatedSize() const
{
   return mAllocatedSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BArray<T>::isGrowExponential() const
{
   return (mGrowSize == BSTACK_GROW_EXPONENTIAL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BArray<T>::enableGrowToIndex(bool enable)
{
   mGrowToIndex = enable;
}


//============================================================================
//  OPERATORS
//============================================================================
template <class T> BArray<T>& BArray<T>::operator = (const BArray<T>& array)
{
   //-- Clear ourself.
   reset(array.mAllocatedSize, array.mGrowSize);
   mGrowToIndex = array.mGrowToIndex;

   //-- See if we are empty.
   if (array.mAllocatedSize == 0)
      return *this;

   //-- Sanity.
   BFATAL_ASSERT(mpArray);
   BFATAL_ASSERT(mAllocatedSize >= array.mSize);

   //-- Copy the items in.
   mSize = array.mSize;
   for (unsigned long index = 0; index < mSize; ++index)
      mpArray[index] = array.mpArray[index];

   //-- Return ourself.
   return *this;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BArray<T>::operator == (const BArray<T>& array) const
{
   //-- Search for quick differences first.
   if (mSize != array.mSize)
      return false;

   //-- The size is the same, and if the size is 0 they must be equal.
   if (mSize == 0)
      return true;

   //-- Sanity.
   BFATAL_ASSERT(mpArray && array.mpArray);

   //-- Check each element.
   for (unsigned long index = 0; index < mSize; ++index)
   {
      if (mpArray[index] != array.mpArray[index])
         return false;
   }

   //-- No differences found.
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BArray<T>::operator != (const BArray<T>& array) const
{
   return !(*this == array);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T& BArray<T>::operator [] (unsigned long index)
{
   if (mGrowToIndex)
   {
      if (index >= mAllocatedSize)
         setAllocatedSize(index + 1);
      
      if (index >= mSize)
         mSize = index + 1;

      BFATAL_ASSERT(mpArray);
      return mpArray[index];
   }

   BFATAL_ASSERT(index < mSize);
   BFATAL_ASSERT(mpArray);

   if (index >= mSize)
      mSize = index + 1;

   return mpArray[index];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> const T& BArray<T>::operator [] (unsigned long index) const
{
   BFATAL_ASSERT(index < mSize);
   BFATAL_ASSERT(mpArray);
   return mpArray[index];
}


//============================================================================
//  PRIVATE FUNCTIONS
//============================================================================
template <class T> void BArray<T>::grow()
{
   BFATAL_ASSERT(mSize == mAllocatedSize);
   BFATAL_ASSERT(mGrowSize > 0);
   BFATAL_ASSERT((mAllocatedSize == 0 && mpArray == NULL) || (mAllocatedSize > 0 && mpArray != NULL));

   //-- Compute the new size.
   unsigned long oldSize = mAllocatedSize;
   mAllocatedSize = (mGrowSize == BSTACK_GROW_EXPONENTIAL) ? mAllocatedSize * 2 : mAllocatedSize + mGrowSize;
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




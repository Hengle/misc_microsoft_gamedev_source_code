//============================================================================
//
//  Stack.inl
//  
//  Copyright 2002, Ensemble Studios
//
//============================================================================


//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
template <class T> BStack<T>::BStack(unsigned long initialSize, unsigned long growSize)
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
template <class T> BStack<T>::~BStack()
{
   //-- Release memory.
   reset(0, 0);
}


//============================================================================
//  STACK OPERATIONS
//============================================================================
template <class T> void BStack<T>::empty()
{
   mSize = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BStack<T>::push(const T& item)
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
   mpArray[mSize] = item;
   mSize++;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BStack<T>::pop()
{
   BFATAL_ASSERT(mpArray);
   BFATAL_ASSERT(mSize > 0);
   
   //-- Just adjust the size.
   mSize--;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BStack<T>::pop(T& item)
{
   BFATAL_ASSERT(mpArray);
   BFATAL_ASSERT(mSize > 0);
   
   //-- Return the popped item and adjust the size.
   mSize--;
   item = mpArray[mSize];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BStack<T>::isEmpty() const
{
   return (mSize == 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BStack<T>::getNumItems() const
{
   return mSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> const T& BStack<T>::getItem(unsigned long index) const
{
   BFATAL_ASSERT(mpArray);
   BFATAL_ASSERT(index < mSize);
   
   //-- Return the item.
   return mpArray[index];
}


//============================================================================
//  STACK CONTROL
//============================================================================
template <class T> void BStack<T>::reset(unsigned long initialSize, unsigned long growSize)
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
template <class T> void BStack<T>::setGrowSize(unsigned long growSize)
{
   mGrowSize = growSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BStack<T>::setAllocatedSize(unsigned long allocatedSize)
{
   //-- You are NOT allowed chop off the top off the stack.  The caller is
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
template <class T> unsigned long BStack<T>::getGrowSize() const
{
   return mGrowSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BStack<T>::getAllocatedSize() const
{
   return mAllocatedSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BStack<T>::isGrowExponential() const
{
   return (mGrowSize == BSTACK_GROW_EXPONENTIAL);
}


//============================================================================
//  OPERATORS
//============================================================================
template <class T> BStack<T>& BStack<T>::operator = (const BStack<T>& stack)
{
   //-- Clear ourself.
   reset(stack.mAllocatedSize, stack.mGrowSize);

   //-- See if we are empty.
   if (stack.mAllocatedSize == 0)
      return *this;

   //-- Sanity.
   BFATAL_ASSERT(mpArray);
   BFATAL_ASSERT(mAllocatedSize >= stack.mSize);

   //-- Copy the items in.
   mSize = stack.mSize;
   for (long index = 0; index < mSize; ++index)
      mpArray[index] = stack.mpArray[index];

   //-- Return ourself.
   return *this;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BStack<T>::operator == (const BStack<T>& stack) const
{
   //-- Search for quick differences first.
   if (mSize != stack.mSize)
      return false;

   //-- The size is the same, and if the size is 0 they must be equal.
   if (mSize == 0)
      return true;

   //-- Sanity.
   BFATAL_ASSERT(mpArray && stack.mpArray);

   //-- Check each element.
   for (long index = 0; index < mSize; ++index)
   {
      if (mpArray[index] != stack.mpArray[index])
         return false;
   }

   //-- No differences found.
   return true;
}


//============================================================================
//  PRIVATE FUNCTIONS
//============================================================================
template <class T> void BStack<T>::grow()
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




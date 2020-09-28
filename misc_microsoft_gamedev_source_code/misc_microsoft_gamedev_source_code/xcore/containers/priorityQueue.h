//============================================================================
//
// File: staticPriorityQueue.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

template<typename T>
class BLessFunctor
{
public:
   BLessFunctor() { }
   
   bool operator()(const T& lhs, const T& rhs) const
   {
      return lhs < rhs;
   }
};

template<typename T, uint N, typename Comp = BLessFunctor<T> >
class BStaticPriorityQueue
{
   uint mSize;
   T mEntries[N + 1];
   Comp mComparer;
   
   bool compare(const T& lhs, const T& rhs)
   {
      return mComparer(lhs, rhs);
   }

   void downHeapObject(const T& k)
   {
      uint i = 1;
      uint j;

      while ((j = 2U * i) <= mSize)  /* Find parent's two children */
      {
         /* Since left and right children aren't in any
         * particular order, find the lowest of the two.
         */

         if (j < mSize)
         {
            if (compare(mEntries[j + 1], mEntries[j]))
               j++;
         }

         /* Stop if the node doesn't need to move down the tree. */

         if (compare(k, mEntries[j]))
            break;

         mEntries[i] = mEntries[j]; 

         i = j;
      }

      mEntries[i] = k;
   }

   void downHeapEntry(uint i)
   {
      uint j;
      const T k(mEntries[i]);

      while ((j = 2U * i) <= mSize)  /* Find parent's two children */
      {
         /* Since left and right children aren't in any
         * particular order, find the lowest of the two.
         */

         if (j < mSize)
         {
            if (compare(mEntries[j + 1], mEntries[j]))
               j++;
         }

         /* Stop if the node doesn't need to move down the tree. */

         if (compare(k, mEntries[j]))
            break;

         mEntries[i] = mEntries[j]; 

         i = j;
      }

      mEntries[i] = k;
   }

   void upHeap(const T& t)
   {
      mSize++;

      uint j = mSize;

      for ( ; ; )
      {
         const uint i = j >> 1;
         if (!i)
            break;

         if (!(compare(t, mEntries[i])))
            break;

         mEntries[j] = mEntries[i];
         j = i;
      }

      mEntries[j] = t;
   }
      
public:
   BStaticPriorityQueue() : mSize(0)
   {
   }
   
   BStaticPriorityQueue(Comp& comparer) : mSize(0), mComparer(comparer)
   {
   }
   
   BStaticPriorityQueue(const T* pEntries, uint numEntries) : mSize(0)
   {
      init(pEntries, numEntries);
   }

   uint size(void) const { return mSize; }

   bool empty(void) const { return !size(); }

   void clear(void) { mSize = 0; }

   const T& operator[] (uint i) const  { BDEBUG_ASSERT(i < mSize);  return mEntries[i + 1]; } 
         T& operator[] (uint i)        { BDEBUG_ASSERT(i < mSize);  return mEntries[i + 1]; } 

   bool init(const T* pEntries, uint numEntries)
   {
      clear();
      
      mSize = min(N, numEntries);
      Utils::Copy(pEntries, pEntries + mSize, mEntries + 1);

      uint j = mSize >> 1;

      while (j != 0)
         downHeapEntry(j--); 
        
      for (uint i = mSize; i < numEntries; i++)
         push(pEntries[i]);               

      return true;
   }

   void push(const T& entry)
   {
      if (mSize == N)
      {
         if (compare(mEntries[1], entry))
            downHeapObject(entry);
      }
      else
         upHeap(entry);
   }

   const T& top(void) const
   {
      BDEBUG_ASSERT(mSize > 0);
      return mEntries[1];
   }

   bool pop(void) 
   {
      BDEBUG_ASSERT(mSize > 0);
                     
      return remove(0);
   }
   
   bool remove(uint i)
   {
      BDEBUG_ASSERT(i < mSize);
      
      if (i >= mSize)
         return false;

      mEntries[i + 1] = mEntries[mSize];
      
      mSize--;
      
      downHeapEntry(i + 1);
      
      return true;
   }
};

template<typename T, typename Comp = BLessFunctor<T> >
class BDynamicPriorityQueue
{
   uint mSize;
   uint mMaxSize;
   T* mpEntries;
   Comp mComparer;

   bool compare(const T& lhs, const T& rhs)
   {
      return mComparer(lhs, rhs);
   }

   void downHeapObject(const T& k)
   {
      uint i = 1;
      uint j;

      while ((j = 2U * i) <= mSize)  /* Find parent's two children */
      {
         /* Since left and right children aren't in any
         * particular order, find the lowest of the two.
         */

         if (j < mSize)
         {
            if (compare(mpEntries[j + 1], mpEntries[j]))
               j++;
         }

         /* Stop if the node doesn't need to move down the tree. */

         if (compare(k, mpEntries[j]))
            break;

         mpEntries[i] = mpEntries[j]; 

         i = j;
      }

      mpEntries[i] = k;
   }

   void downHeapEntry(uint i)
   {
      uint j;
      const T k(mpEntries[i]);

      while ((j = 2U * i) <= mSize)  /* Find parent's two children */
      {
         /* Since left and right children aren't in any
         * particular order, find the lowest of the two.
         */

         if (j < mSize)
         {
            if (compare(mpEntries[j + 1], mpEntries[j]))
               j++;
         }

         /* Stop if the node doesn't need to move down the tree. */

         if (compare(k, mpEntries[j]))
            break;

         mpEntries[i] = mpEntries[j]; 

         i = j;
      }

      mpEntries[i] = k;
   }

   void upHeap(const T& t)
   {
      mSize++;

      uint j = mSize;

      for ( ; ; )
      {
         const uint i = j >> 1;
         if (!i)
            break;

         if (!(compare(t, mpEntries[i])))
            break;

         mpEntries[j] = mpEntries[i];
         j = i;
      }

      mpEntries[j] = t;
   }

public:
   BDynamicPriorityQueue(uint n) : mSize(0), mMaxSize(n)
   {
      BDEBUG_ASSERT(n > 0);
      mpEntries = new T[mMaxSize + 1];
   }

   BDynamicPriorityQueue(uint n, Comp& comparer) : mSize(0), mMaxSize(n), mComparer(comparer)
   {
      BDEBUG_ASSERT(n > 0);
      mpEntries = new T[mMaxSize + 1];
   }

   BDynamicPriorityQueue(uint n, const T* pEntries, uint numEntries) : mSize(0), mMaxSize(n)
   {
      BDEBUG_ASSERT(n > 0);
      
      mpEntries = new T[mMaxSize + 1];
      
      init(pEntries, numEntries);
   }
   
   ~BDynamicPriorityQueue()
   {
      delete[] mpEntries;
   }

   uint size(void) const { return mSize; }
   
   uint capacity(void) const { return mMaxSize; }
      
   bool empty(void) const { return !size(); }

   void clear(void) { mSize = 0; }

   const T& operator[] (uint i) const  { BDEBUG_ASSERT(i < mSize);  return mpEntries[i + 1]; } 
         T& operator[] (uint i)        { BDEBUG_ASSERT(i < mSize);  return mpEntries[i + 1]; } 

   bool init(const T* pEntries, uint numEntries)
   {
      clear();

      mSize = min(mMaxSize, numEntries);
      Utils::Copy(pEntries, pEntries + mSize, mpEntries + 1);

      uint j = mSize >> 1;

      while (j != 0)
         downHeapEntry(j--); 

      for (uint i = mSize; i < numEntries; i++)
         push(pEntries[i]);               

      return true;
   }

   void push(const T& entry)
   {
      if (mSize == mMaxSize)
      {
         if (compare(mpEntries[1], entry))
            downHeapObject(entry);
      }
      else
         upHeap(entry);
   }

   const T& top(void) const
   {
      BDEBUG_ASSERT(mSize > 0);
      return mpEntries[1];
   }

   bool pop(void) 
   {
      BDEBUG_ASSERT(mSize > 0);

      return remove(0);
   }

   bool remove(uint i)
   {
      BDEBUG_ASSERT(i < mSize);

      if (i >= mSize)
         return false;

      mpEntries[i + 1] = mpEntries[mSize];

      mSize--;

      downHeapEntry(i + 1);

      return true;
   }
};

#if 0
bool testStaticPriorityQueue(void)
{
   for (uint l = 0; l < 1000000; l++)
   {
      uint size = rand() % 4096;

      BDynamicArray<uint> values(size);

      uint i;
      for (i = 0; i < size; i++)
         values[i] = rand();

      const uint N = 257;
      BStaticPriorityQueue<uint, N> queue(values.begin(), size);

      if (size)
         std::sort(values.begin(), values.end());

      i = size - queue.size();
      while (!queue.empty())
      {
         if (queue.top() != values[i])
            return false;

         i++;

         queue.pop();
      }
   }      
   
   return true;
}   

bool testDynamicPriorityQueue(void)
{
   for (uint l = 0; l < 1000000; l++)
   {
      uint size = rand() % 4096;

      BDynamicArray<uint> values(size);

      uint i;
      for (i = 0; i < size; i++)
         values[i] = rand();

      const uint N = 1 + (rand() % 4096);
      BDynamicPriorityQueue<uint> queue(N, values.begin(), size);

      if (size)
         std::sort(values.begin(), values.end());

      i = size - queue.size();
      while (!queue.empty())
      {
         if (queue.top() != values[i])
            return false;

         i++;

         queue.pop();
      }
   }      

   return true;
}   
#endif

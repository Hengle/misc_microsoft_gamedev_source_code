//==============================================================================
//
// File: segmentedArray.h
// 
// Copyright (c) 2007, Ensemble Studios
//
//==============================================================================
#pragma once

template<class ValueType, uint GroupSizeLog2>
class BSegmentedArray 
{
public:
   enum { cGroupSizeLog2    = GroupSizeLog2 };
   enum { cGroupSize        = 1U << cGroupSizeLog2 };
   enum { cGroupSizeMask    = cGroupSize - 1U };
   enum { cGroupSizeInBytes = sizeof(ValueType) << cGroupSizeLog2 };
   enum { cAlignment        = ALIGN_OF(ValueType) };

   typedef ValueType                         valueType;
   
   // For backwards compatibility
   typedef ValueType                         value_type;
   typedef ValueType&                        reference;
   typedef const ValueType&                  const_reference;
      
   inline BSegmentedArray(BMemoryHeap* pHeap = &gPrimaryHeap) : 
      mGroupPtrs(pHeap),
      mSize(0), 
      mCapacity(0)
   {
   }      
   
   inline BSegmentedArray(uint initialSize, uint initialCapacity = 0, BMemoryHeap* pHeap = &gPrimaryHeap) : 
      mGroupPtrs(pHeap),
      mSize(0), 
      mCapacity(0)
   {
      if (initialCapacity)
         changeCapacity(Math::Max(initialSize, initialCapacity));
      if (initialSize)
         resize(initialSize);
   } 
   
   inline BSegmentedArray(const BSegmentedArray& other) :
      mGroupPtrs(&gPrimaryHeap),
      mSize(0), 
      mCapacity(0)
   {
      *this = other;
   } 
   
   inline ~BSegmentedArray()
   {
      destructAllEntries();
   }
   
   BSegmentedArray& operator= (const BSegmentedArray& rhs) 
   {
      if (this == &rhs)
         return *this;
      
      clear();
      
      setHeap(rhs.getHeap());
      
      reserve(rhs.getSize());
      for (uint i = 0; i < rhs.getSize(); i++)
         pushBack(rhs[i]);
            
      return *this;
   }
   
   inline uint getSize() const { return mSize; }
   
   inline uint getCapacity() const { return mCapacity; }
   
   inline bool isEmpty() const { return !mSize; }
   
   inline BMemoryHeap* getHeap() const { return mGroupPtrs.getHeap(); }
   
   inline void setHeap(BMemoryHeap* pHeap) { mGroupPtrs.setHeap(pHeap); }
   
   inline void clear() { changeCapacity(0); }
   
   inline void resize(uint newSize)
   {
      if (newSize < mSize)
      {
         // Caller wants to decrease the number of live objects. Call the destructors on those objects going away.
         destructEntries(newSize, mSize - newSize);
      }
      else if (newSize > mSize)
      {
         // Caller wants to increase the number of live objects - see if capacity is high enough.
         if (newSize > mCapacity)
         {
            // Make room for new objects by growing the container's capacity.
            changeCapacity(newSize);

            BDEBUG_ASSERT(mCapacity >= newSize);
         }

         // Construct new objects starting at the end.
         constructEntries(mSize, newSize - mSize);
      }

      mSize = newSize;
   }
   
   inline void enlarge(uint num)
   {
      resize(getSize() + num);
   }
   
   inline void reserve(uint newCapacity)
   {
      changeCapacity(Math::Max(mSize, newCapacity));
   }
   
   inline void pushBack(const ValueType& val)
   {
      if (mSize == mCapacity)
         changeCapacity(mSize + 1);

      if (BIsBuiltInType<ValueType>::Flag)
         getObject(mSize) = val;
      else 
         Utils::ConstructInPlace(&getObject(mSize), val);
      
      mSize++;   
   }
   
   inline ValueType* pushBackNoConstruction(uint num)
   {
      BDEBUG_ASSERT(num);
      
      const uint curSize = getSize();
      
      enlarge(num);
      
      return &getObject(curSize);
   }
   
   inline void pushBack(const ValueType* pVal, uint num)
   {
      if (!num)
         return;
      BDEBUG_ASSERT(pVal);
         
      makeRoom(num);
      for (uint i = num; i > 0; i--, pVal++)
         pushBack(*pVal);
   }
   
   inline void pushSorted(const ValueType& value)
   {
      for (uint i = 0; i < mSize; i++)
         if (value < getObject(i))
         {
            insert(i, value);
            return;
         }

      pushBack(value);
   }
   
   inline void pushFront(const ValueType& value)
   {
      insert(0, value);
   }
   
   inline ValueType& grow() 
   { 
      const uint curSize = mSize;
      enlarge(1); 
      return getObject(curSize);
   }
   
   inline const ValueType& operator[] (uint index) const { BDEBUG_ASSERT(index < mSize); return getObject(index); }
   inline       ValueType& operator[] (uint index)       { BDEBUG_ASSERT(index < mSize); return getObject(index); }
   
   // Returns reference to first element in container.
   inline const ValueType& front(void) const  { BDEBUG_ASSERT(!isEmpty()); return (*this)[0]; }
   inline       ValueType& front(void)        { BDEBUG_ASSERT(!isEmpty()); return (*this)[0]; }

   // Returns reference to the last element in container.
   inline const ValueType& back(void) const   { BDEBUG_ASSERT(!isEmpty()); return (*this)[mSize - 1]; }
   inline       ValueType& back(void)         { BDEBUG_ASSERT(!isEmpty()); return (*this)[mSize - 1]; }
   
   // Given a pointer to an object within the container, this returns its index, or cInvalidIndex if the pointer is invalid.
   // This search must compare the pointer against all groups in the container.
   int findIndex(const ValueType* p) const
   {
      BDEBUG_ASSERT(p);
      
      for (uint i = 0; i < mGroupPtrs.getSize(); i++)
      {
         const ValueType* pGroupStart = static_cast<const ValueType*>(mGroupPtrs[i]);
         const ValueType* pGroupEnd = pGroupStart + cGroupSize;
         
         if ((p >= pGroupStart) && (p < pGroupEnd))
         {
            const uint index = (i << cGroupSizeLog2) + (p - pGroupStart);
            if (index >= mSize)
               return cInvalidIndex;
            
            return index;
         }
      }
      
      return cInvalidIndex;
   }
   
   inline void append(const BSegmentedArray& other)
   {
      if (other.getSize())
      {
         reserve(Math::Max(getCapacity(), getSize() + other.getSize()));
         for (uint i = 0; i < other.getSize(); i++)
            pushBack(other[i]);
      }
   }
   
   inline void popBack()
   {
      BDEBUG_ASSERT(mSize > 0);
      if (mSize)
      {  
         destructEntries(mSize - 1, 1);
         mSize--;
      }
   }
   
   inline void erase(uint begin, uint end)
   {
      if (begin == end)
         return;

      BDEBUG_ASSERT(end > begin);
      debugRangeCheck(begin, mSize);
      debugRangeCheckIncl(end, mSize);

      const uint numEntriesDeleted = end - begin;
      const uint numEntriesToCopy = mSize - end;

      for (uint i = 0; i < numEntriesToCopy; i++)
         getObject(begin + i) = getObject(end + i);

      resize(mSize - numEntriesDeleted);
   }

   inline void erase(uint index)
   {
      erase(index, index + 1);
   }

   inline void eraseUnordered(uint index)
   {
      BDEBUG_ASSERT(index < getSize());

      if ((index + 1) != getSize())
         getObject(index) = getObject(getSize() - 1);

      popBack();
   }
   
   inline void insert(uint pos, uint num, const ValueType* pObjects)
   {
      if (!num)
         return;

      if (pos > getSize())
      {
         BDEBUG_ASSERT(0);
         return;
      }
      else if (pos == getSize())
      {
         if (pObjects)
            pushBack(pObjects, num);
         else
            resize(getSize() + num);
         return;
      }

      const uint origSize = mSize;
      const uint newSize = mSize + num;
      makeRoom(num);

      mSize = newSize;

      uint srcIndex = pos;
      uint dstIndex = pos + num;
      uint srcNum = origSize - pos;

      BDEBUG_ASSERT((srcNum <= origSize) && (srcIndex < origSize) && (dstIndex < newSize) && ((dstIndex + srcNum) == newSize));
      
      for (int i = srcNum - 1; i >= 0; i--)
      {
         const ValueType& srcObject = getObject(srcIndex + i);
         Utils::ConstructInPlace(&getObject(dstIndex + i), srcObject);
         Utils::DestructInPlace(&srcObject);
      }

      if (pObjects)
      {
         for (uint i = 0; i < num; i++)
            Utils::ConstructInPlace(&getObject(pos + i), pObjects[i]);
      }
      else 
         constructEntries(pos, num);
   }

   inline void insert(uint pos, const ValueType& object)
   {
      insert(pos, 1, &object);
   }
   
   inline void swap(const BSegmentedArray& other)
   {
      mGroupPtrs.swap(other.mGroupPtrs);
      std::swap(mSize, other.mSize);
      std::swap(mCapacity, other.mCapacity);
   }
   
   friend bool operator== (const BSegmentedArray& lhs, const BSegmentedArray& rhs) 
   {
      if (lhs.getSize() != rhs.getSize())
         return false;
      for (uint objIter = 0; objIter < lhs.getSize(); objIter++)
         if (!(lhs[objIter] == rhs[objIter]))
            return false;
      return true;
   }

   friend bool operator< (const BSegmentedArray& lhs, const BSegmentedArray& rhs) 
   {
      if (lhs.getSize() < rhs.getSize())
         return true;
      else if (lhs.getSize() == rhs.getSize())
      {
         for (uint objIter = 0; objIter < lhs.getSize(); objIter++)
         {
            if (lhs[objIter] < rhs[objIter])
               return true;
            else if (!(lhs[objIter] == rhs[objIter]))
               return false;
         }
      }

      return false;
   }

   friend bool operator >  (const BSegmentedArray& a, const BSegmentedArray& b) { return b < a; }
   friend bool operator <= (const BSegmentedArray& a, const BSegmentedArray& b) { return !(b < a); }
   friend bool operator >= (const BSegmentedArray& a, const BSegmentedArray& b) { return !(a < b); }
   friend bool operator != (const BSegmentedArray& a, const BSegmentedArray& b) { return !(a == b); }   
   
   // Shell sort, obviously not the fastest but should be good enough until we implement STL-compatible iterator classes.
   inline void sort()
   {
      if (!getSize())
         return;
         
      const int n = getSize();
      
      static const uint cols[] = { 1391376, 463792, 198768, 86961, 33936, 13776, 4592, 1968, 861, 336, 112, 48, 21, 7, 3, 1 };
      
      for (int k = 0; k < 16; k++)
      {
         int h = cols[k];
         for (int i = h; i < n; i++)
         {
            int j = i;
            ValueType* pCur = &getObject(j);
            if (*pCur < getObject(j - h))
            {
               const ValueType v(*pCur);
                           
               do
               {
                  ValueType* pPrev = &getObject(j - h);
                                 
                  if (!(v < *pPrev))
                     break;
                  
                  *pCur = *pPrev;
                  pCur = pPrev;
                  
                  j -= h;
                  
               } while (j >= h);
                           
               getObject(j) = v;
            }                  
         }
      }
   }
   
   inline bool isSorted() const
   {
      for (uint i = 1; i < mSize; i++)
         if (getObject(i) < getObject(i - 1))
            return false;
      return true;
   }
         
   inline void removeDuplicates()
   {
      if (isEmpty())
         return;
         
      uint dstIndex = 1;
      const ValueType* pCur = &getObject(0);
      
      for (uint srcIndex = 1; srcIndex < mSize; srcIndex++)
      {
         const ValueType& src = getObject(srcIndex);
         
         if (src == *pCur)
            continue;
         
         if (srcIndex != dstIndex)
            getObject(dstIndex) = src;
         dstIndex++;
         
         pCur = &src;
      }
      
      resize(dstIndex);
   }

   // Binary search - returns cInvalidIndex if not found.
   inline int binarySearch(const ValueType& searchObj) const
   {
      int l = 0;
      int r = getSize() - 1;

      while (r >= l)
      {
         const int m = debugRangeCheck((l + r) >> 1, getSize());
         const ValueType& obj = (*this)[m];

         if (searchObj == obj)
            return m;
         else if (searchObj < obj)
            r = m - 1;
         else
            l = m + 1;
      }

      return cInvalidIndex;
   }

   inline bool binarySearch(const ValueType& searchObj, uint& index) const
   {
      int l = 0;
      int r = getSize() - 1;

      while (r >= l)
      {
         const int m = debugRangeCheck((l + r) >> 1, getSize());
         const ValueType& obj = (*this)[m];

         if (searchObj == obj)
         {
            index = m;
            return true;
         }
         else if (searchObj < obj)
            r = m - 1;
         else
            l = m + 1;
      }

      index = 0;
      return false;
   }

   // Sequential search, starting at beginning. Returns cInvalidIndex if not found.
   inline int find(const ValueType& searchObj) const
   {
      for (uint i = 0; i < mSize; i++)
         if (getObject(i) == searchObj)
            return i;
      return cInvalidIndex;
   }

   // Sequential search, starting at beginning. Returns false if not found.
   inline bool find(const ValueType& searchObj, uint& index) const
   {
      for (uint i = 0; i < mSize; i++)
      {
         if (getObject(i) == searchObj)
         {
            index = i;
            return true;
         }
      }
      index = 0;
      return false;
   }

   // Sequential search, starting at end. Returns cInvalidIndex if not found.
   inline int findLast(const ValueType& searchObj) const
   {
      for (int i = mSize - 1; i >= 0; i--)
         if (getObject(i) == searchObj)
            return i;
      return cInvalidIndex;
   }

   // Sequential search, starting at end. Returns false if not found.
   inline bool findLast(const ValueType& searchObj, uint& index) const
   {
      for (int i = mSize - 1; i >= 0; i--)
      {
         if (getObject(i) == searchObj)
         {
            index = i;
            return true;
         }
      }
      index = 0;
      return false;
   }

   inline void reverse()
   {
      const uint n = mSize >> 1;
      for (uint i = 0; i < n; i++)
         std::swap(getObject(i), getObject(mSize - 1 - i));
   }

   inline void setAll(const ValueType& value)
   {
      for (uint i = 0; i < mSize; i++)
         getObject(i) = value;
   }
   
   inline void flatten(ValueType* pDst, uint num) const
   {
      BDEBUG_ASSERT(pDst);
      num = Math::Min(mSize, num);
      if (!num)
         return;
            
      uint numFullGroups = mSize >> cGroupSizeLog2;
      const ValueType* const* p = (const ValueType* const*)mGroupPtrs.getPtr();
      
      for (uint i = numFullGroups; i > 0; i--)
      {
         const ValueType* pSrc = *p++;
         
         if (BIsBuiltInType<ValueType>::Flag) 
         {
            Utils::FastMemCpy(pDst, pSrc, cGroupSizeInBytes);
            pDst += cGroupSize;
         }
         else
         {
            for (uint j = cGroupSize; j > 0; j--)
               *pDst++ = *pSrc++;
         }
      }
      
      uint numRemaining = mSize & cGroupSizeMask;
      if (!numRemaining)
         return;
            
      const ValueType* pSrc = (const ValueType*)(mGroupPtrs[numFullGroups]);
      if (BIsBuiltInType<ValueType>::Flag) 
         Utils::FastMemCpy(pDst, pSrc, numRemaining * sizeof(ValueType));
      else
      {
         for (uint i = numRemaining; i > 0; i--)
            *pDst++ = *pSrc++;
      }            
   }
   
private:
   class BHeapBlockArray
   {
   public:
      inline BHeapBlockArray(BMemoryHeap* pHeap) :
         mGroupPtrs(BDynamicArrayHeapAllocator<void*, ALIGN_OF(void*)>(pHeap))
      {
      }

      inline ~BHeapBlockArray()
      {
         const uint size = mGroupPtrs.getSize();
         if (size)
         {
            void** p = mGroupPtrs.getPtr();
            BMemoryHeap* pHeap = getHeap();
            for (uint i = size; i > 0; i--)
               pHeap->Delete(*p++);
         }
      }

      inline void clear()
      {
         const uint size = mGroupPtrs.getSize();
         if (size)
         {
            void** p = mGroupPtrs.getPtr();
            BMemoryHeap* pHeap = getHeap();
            for (uint i = size; i > 0; i--)
               pHeap->Delete(*p++);
         }
         mGroupPtrs.clear();         
      }

      inline bool resize(uint newSize, uint blockSize)
      {
         const uint curSize = mGroupPtrs.getSize();
         BMemoryHeap* pHeap = getHeap();

         if (newSize < curSize)
         {
            for (uint i = newSize; i < curSize; i++)
               pHeap->Delete(mGroupPtrs[i]);

            mGroupPtrs.resize(newSize);
         }
         else if (newSize > curSize)
         {
            BDEBUG_ASSERT(blockSize);

            mGroupPtrs.resize(newSize);

            for (uint i = curSize; i < newSize; i++)
            {
               mGroupPtrs[i] = pHeap->New(blockSize, NULL, false);
               if (!mGroupPtrs[i])
               {
                  mGroupPtrs.resize(i);
                  return false;
               }
            }
         }

         return true;
      }

      inline uint getSize() const { return mGroupPtrs.getSize(); }

      inline void* operator[](uint index) const { return mGroupPtrs[index]; }

      inline void setHeap(BMemoryHeap* pHeap)
      {
         BDEBUG_ASSERT(!mGroupPtrs.getPtr());
         mGroupPtrs.getAllocator().setHeap(pHeap);
      }

      inline BMemoryHeap* getHeap() const { return mGroupPtrs.getAllocator().getHeap(); }

      inline void swap(BHeapBlockArray& other)
      {
         mGroupPtrs.swap(other.mGroupPtrs);
      }

      inline void* const* getPtr() const { return mGroupPtrs.getPtr(); }
      inline void** getPtr() { return mGroupPtrs.getPtr(); }

   private:
      typedef BDynamicArray<void*, sizeof(void*), BDynamicArrayHeapAllocator> BGroupPtrArray;
      BGroupPtrArray mGroupPtrs;
   };

   BHeapBlockArray      mGroupPtrs;
   uint                 mSize;
   uint                 mCapacity;
         
   const ValueType& getObject(uint index) const { return static_cast<const ValueType*>(mGroupPtrs[index >> cGroupSizeLog2])[index & cGroupSizeMask]; }
         ValueType& getObject(uint index)       { return static_cast<ValueType*>(mGroupPtrs[index >> cGroupSizeLog2])[index & cGroupSizeMask]; }
                        
   inline void constructEntriesDefault(uint first, uint num)
   {
      if (BIsBuiltInType<ValueType>::Flag) 
         return;

      uint cur = first;
      uint numToAlignUp = Math::Min(num, Utils::BytesToAlignUpValue(first, cGroupSize));
      for (uint i = numToAlignUp; i > 0; i--, cur++)
         Utils::ConstructInPlace(&getObject(cur));

      num -= numToAlignUp;
      if (!num)
         return;

      BDEBUG_ASSERT((cur & cGroupSizeMask) == 0);

      uint totalGroups = num >> cGroupSizeLog2;
      uint groupIndex = cur >> cGroupSizeLog2;
      for (uint i = totalGroups; i > 0; i--, groupIndex++)
         Utils::ConstructArrayInPlace((ValueType*)(mGroupPtrs[groupIndex]), cGroupSize);
      uint totalGroupEntries = totalGroups << cGroupSizeLog2;
      num -= totalGroupEntries;
      cur += totalGroupEntries;
      
      BDEBUG_ASSERT(num < cGroupSize);
      for (uint i = num; i > 0; i--, cur++)
         Utils::ConstructInPlace(&getObject(cur));
   }
         
   inline void constructEntriesZero(uint first, uint num)
   {
      uint cur = first;
      uint numToAlignUp = Math::Min(num, Utils::BytesToAlignUpValue(first, cGroupSize));
      Utils::FastMemSet(&getObject(cur), 0, numToAlignUp * sizeof(ValueType));
      
      num -= numToAlignUp;
      if (!num)
         return;
      
      cur += numToAlignUp;
      BDEBUG_ASSERT((cur & cGroupSizeMask) == 0);

      uint totalGroups = num >> cGroupSizeLog2;
      uint groupIndex = cur >> cGroupSizeLog2;
      for (uint i = totalGroups; i > 0; i--, groupIndex++)
         Utils::FastMemSet(mGroupPtrs[groupIndex], 0, cGroupSizeInBytes);
      
      uint totalGroupEntries = totalGroups << cGroupSizeLog2;
      
      num -= totalGroupEntries;
      if (!num)
         return;
      
      cur += totalGroupEntries;   
      BDEBUG_ASSERT((cur & cGroupSizeMask) == 0);
      
      BDEBUG_ASSERT(num < cGroupSize);
      Utils::FastMemSet(mGroupPtrs[cur >> cGroupSizeLog2], 0, num * sizeof(ValueType));
   }
   
   inline void constructEntries(uint first, uint num)
   {
      if (BIsBuiltInType<ValueType>::Flag) 
         constructEntriesZero(first, num);
      else
         constructEntriesDefault(first, num);
   }
   
   inline void destructGroup(ValueType* p)
   {
      if (!BIsBuiltInType<ValueType>::Flag) 
         Utils::DestructArrayInPlace(p, cGroupSize);
   }
   
   inline void destructEntries(uint first, uint num)
   {
      if (BIsBuiltInType<ValueType>::Flag) 
         return;
      
      uint cur = first;
      uint numToAlignUp = Math::Min(num, Utils::BytesToAlignUpValue(first, cGroupSize));
      for (uint i = numToAlignUp; i > 0; i--, cur++)
         Utils::DestructInPlace(&getObject(cur));
      
      num -= numToAlignUp;
      if (!num)
         return;
         
      BDEBUG_ASSERT((cur & cGroupSizeMask) == 0);
      
      uint totalGroups = num >> cGroupSizeLog2;
      uint groupIndex = cur >> cGroupSizeLog2;
      for (uint i = totalGroups; i > 0; i--, groupIndex++)
         destructGroup((ValueType*)(mGroupPtrs[groupIndex]));
      uint totalGroupEntries = totalGroups << cGroupSizeLog2;
      num -= totalGroupEntries;
      cur += totalGroupEntries;
      
      for (uint i = num; i > 0; i--, cur++)
         Utils::DestructInPlace(&getObject(cur));
   }
   
   inline void destructAllEntries()
   {
      if (BIsBuiltInType<ValueType>::Flag) 
         return;
         
      const uint numFullGroups = mSize >> cGroupSizeLog2;
      for (uint groupIndex = 0; groupIndex < numFullGroups; groupIndex++)
         destructGroup((ValueType*)(mGroupPtrs[groupIndex]));
      
      const uint numLeftOver = mSize & cGroupSizeMask;
      if (numLeftOver)      
      {
         ValueType* p = (ValueType*)(mGroupPtrs[numFullGroups]);
         
         for (uint i = numLeftOver; i > 0; i--, p++)
            Utils::DestructInPlace(p);
      }            
   }
               
   void changeCapacity(uint newCapacity)
   {
      if (newCapacity == mCapacity)
         return;
      else if (!newCapacity)      
      {
         destructAllEntries();
         mGroupPtrs.clear();
         mCapacity = 0;
         mSize = 0;  
         return;
      }
      
      const uint curNumGroups = mGroupPtrs.getSize();
      const uint newNumGroups = (newCapacity + cGroupSize - 1U) >> cGroupSizeLog2;
      newCapacity = newNumGroups << cGroupSizeLog2;
      
      if (curNumGroups != newNumGroups)
      {
         if (newCapacity < mSize)
         {
            destructEntries(newCapacity, mSize - newCapacity);
            mSize = newCapacity;
         }
         
         bool success = mGroupPtrs.resize(newNumGroups, cGroupSizeInBytes);
         BVERIFY(success);
      }         
      
      mCapacity = newCapacity;
   }
   
   inline uint getRoomLeft() const
   {  
      return mCapacity - mSize;
   }
      
   inline void makeRoom(uint numNewEntries)
   {
      BDEBUG_ASSERT(mSize <= mCapacity);
      if (numNewEntries > (mCapacity - mSize))
      {
         changeCapacity(mSize + numNewEntries);
         
         BDEBUG_ASSERT(numNewEntries <= (mCapacity - mSize));
      }
   }
};

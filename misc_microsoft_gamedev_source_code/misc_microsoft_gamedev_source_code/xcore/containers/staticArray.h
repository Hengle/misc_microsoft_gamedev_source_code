//============================================================================
//
// File: staticArray.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

// ------------------------------------------------------------------------------------------------------------------------
// class BStaticArray
// ValueType                  - Element type
// StaticSize                 - Number of objects to statically allocated within the container itself
// Growable                   - If false, container is fixed in size (must specify initial capacity in constructor!)
// UseConstructorDestructor   - If true, objects will be copy constructed when they are created.
// ------------------------------------------------------------------------------------------------------------------------
template<
   class ValueType, 
   uint StaticSize = 4, 
   bool Growable = true, 
   bool UseConstructorDestructor = true,
   template <class, uint>  class Allocator = BDynamicArrayPrimaryHeapAllocator
   >
class BStaticArray : public Allocator<ValueType, ALIGN_OF(ValueType) >
{
public:
   enum { cStaticSize = StaticSize };
   enum { cGrowable = Growable };
   enum { cAlignment = ALIGN_OF(ValueType) };
   
   typedef Allocator<ValueType, cAlignment>  allocator;
   
   typedef ValueType                         valueType;
   typedef ValueType*                        iterator;
   typedef const ValueType*                  constIterator;
   
   // For backwards compatibility
   typedef const ValueType*                  const_iterator;
   
   BStaticArray() : 
      mSize(0) 
   {
   }
   
   BStaticArray(const BStaticArray& other) :
      allocator(other.getAllocator()),
      mSize(0)
   {
      *this = other;
   }
        
   // initialSize - initial num. of entries
   explicit BStaticArray(uint initialSize, const allocator& alloc = allocator()) :
      allocator(alloc),
      mSize(0) 
   {
      resize(initialSize);
   }
   
   template<class T, uint A>
   BStaticArray(uint initialSize, const Allocator<T, A>& alloc) :
      allocator(alloc),
      mSize(0) 
   {
      resize(initialSize);
   }

   explicit BStaticArray(uint numVals, const ValueType* pVals, const allocator& alloc = allocator()) :
      allocator(alloc),
      mSize(0) 
   {
      pushBack(pVals, numVals);
   }

   explicit BStaticArray(const ValueType* pVals, const ValueType* pEndVals, const allocator& alloc = allocator()) :
      allocator(alloc),
      mSize(0)
   {
      BDEBUG_ASSERT(pEndVals >= pVals);

      pushBack(pVals, pEndVals - pVals);
   }
   
   explicit BStaticArray(const allocator& alloc) :
      allocator(alloc),
      mSize(0)
   {
   }
   
   template<class T, uint A>
   BStaticArray(const Allocator<T, A>& alloc) :
      allocator(alloc),
      mSize(0)
   {
   }
   
   ~BStaticArray()
   {
      destructEntries(0, getSize());

      if (Growable)
      {
         if (isDynamic())
            freeBuf(mData.mDynamic.mpAligned, mData.mDynamic.mCapacity);
      }            

#ifdef BUILD_DEBUG
      memset(this, 0xFD, sizeof(*this));
#endif      
   }
         
   BStaticArray& operator= (const BStaticArray& rhs)
   {
      if (this == &rhs)
         return *this;

      clear();
      
      allocator::operator= (static_cast<const allocator&>(rhs));

      if (!rhs.isEmpty())
         pushBack(rhs.getData(), rhs.getSize());

      return *this;
   }
   
   template<class OtherValueType, uint OtherStaticSize, bool OtherGrowable, bool OtherUseConstructorDestructor, template <class, uint> class OtherAllocator>
   BStaticArray& operator= (const BStaticArray<OtherValueType, OtherStaticSize, OtherGrowable, OtherUseConstructorDestructor, OtherAllocator>& rhs)
   {
      if ((const void*)this == (const void*)&rhs)
         return *this;
         
      clear();

      if (!rhs.empty())
      {
         reserve(rhs.getSize());
         for (uint i = 0; i < rhs.getSize(); i++)
            pushBack(rhs[i]);
      }            

      return *this;
   }
   
   inline const allocator& getAllocator(void) const 
   { 
      return *this; 
   }

   inline allocator& getAllocator(void)       
   { 
      return *this;
   }

   inline void setAllocator(allocator& alloc)
   {
      if (isDynamic())
      {
         BDEBUG_ASSERT(!mData.mDynamic.mpAligned);
      }
      
      allocator::operator= (alloc);
   }
   
   template<class T, uint A>
   inline void setAllocator(Allocator<T, A>& alloc)
   {
      if (isDynamic())
      {
         BDEBUG_ASSERT(!mData.mDynamic.mpAligned);
      }

      allocator::operator= (allocator(alloc));
   }

   // true if the container is empty
   inline bool isEmpty(void) const 
   { 
      return 0 == getSize(); 
   }
         
   // Num. of objects present in container
   inline uint getSize(void) const 
   { 
      return Growable ? (mSize & 0x7FFFFFFF) : mSize; 
   }
   
   // For backwards compatibility.
   inline uint size(void) const 
   { 
      return getSize();
   }
   
   // Returns the total size in bytes of the objects present in the container.
   inline uint getSizeInBytes(void) const
   {
      return getSize() * sizeof(ValueType);
   }

   // Current capacity of container
   inline uint getCapacity(void) const
   {
      return Growable ? (isDynamic() ? mData.mDynamic.mCapacity : StaticSize) : StaticSize;
   }
   
   inline bool isDynamic(void) const 
   { 
      return Growable ? (static_cast<int>(mSize) < 0) : false;
   }
         
   inline bool isFull(void) const 
   { 
      return getSize() == getCapacity(); 
   }
      
   // Deallocate, clears all entries
   void clear(void) 
   {
      changeCapacity(0);
      BDEBUG_ASSERT(!isDynamic() || (NULL == mData.mDynamic.mpAligned));
   }

   // Changes the number of objects in container               
   void resize(uint newSize) 
   {
      if (newSize < getSize())
         destructEntries(newSize, getSize() - newSize);
      else if (newSize > getSize())
      {
         // Caller wants to increase the number of live objects - see if capacity is high enough.
         if (newSize > getCapacity())
         {
            // Make room for new objects by growing the container's capacity.
            const uint numNewEntries = newSize - getSize();            
            makeRoom(numNewEntries);

            BDEBUG_ASSERT(getCapacity() >= newSize);
         }

         constructEntries(getSize(), newSize - getSize());
      }

      setSize(newSize);
   }
         
   ValueType* enlarge(uint num)
   {
      const uint curSize = getSize();
      resize(getSize() + num);
      if (!getPtr())
         return NULL;
      return getPtr() + curSize;
   }

   // Change's the container capacity -- current objects are always preserved.
   // If 0 is specified, the container's memory block is shrunk to the smallest possible size.
   void reserve(uint newCapacity)
   {
      changeCapacity(Math::Max(getSize(), newCapacity));
   }

   // Adds object to container
   void pushBack(const ValueType& val)
   {
      // Not safe to push an object from within the container.
      BDEBUG_ASSERT((&val < begin()) || (&val >= end()));
   
      if (roomLeft() < 1)
         makeRoom(1);

      BDEBUG_ASSERT(getSize() < getCapacity());
      
      if (BIsBuiltInType<ValueType>::Flag)
         *getPtr(getSize()) = val;
      else if (UseConstructorDestructor)
         Utils::ConstructInPlace(getPtr(getSize()), val);
      else if (sizeof(ValueType) <= 64)
         memcpy(getPtr(getSize()), &val, sizeof(ValueType));
      else
         Utils::FastMemCpy(getPtr(getSize()), &val, sizeof(ValueType));

      changeSize(1);
   }
         
   // Places num objects at end of container
   void pushBack(const ValueType* pVals, uint num)
   {
      BDEBUG_ASSERT(pVals);
      if (!num)
         return;

      if (roomLeft() < num)
         makeRoom(num);
               
      ValueType* pDst = getPtr(getSize());                
      if ((UseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
      {
         for (uint i = 0; i < num; i++)
            Utils::ConstructInPlace(&pDst[i], pVals[i]);
      }
      else
         Utils::FastMemCpy(pDst, pVals, num * sizeof(ValueType));

      changeSize(num);
      BDEBUG_ASSERT(getSize() <= getCapacity());
   }
   
   void append(const BStaticArray& other)
   {
      if (other.getSize())
         pushBack(other.getPtr(), other.getSize());
   }

   // Removes last object from container
   void popBack(void)
   {
      const uint curSize = getSize();
      BDEBUG_ASSERT(curSize > 0);
      if (curSize)
      {  
         destructEntries(curSize - 1, 1);
         changeSize(-1);
      }
   }

   // Returns pointer to first object - may return NULL if no memory allocated to container
   const ValueType* getData(void) const   { return getPtr(0); }
         ValueType* getData(void)         { return getPtr(0); }
   
   // Object accessor
   const ValueType& operator[] (uint i) const 
   { 
      BDEBUG_ASSERT(i < getSize());
      return *getPtr(i);
   }

   // Object accessor
   ValueType& operator[] (uint i) 
   { 
      BDEBUG_ASSERT(i < getSize());
      return *getPtr(i);
   }
   
   // Object accessor
   inline const ValueType& at(uint i) const 
   { 
      BDEBUG_ASSERT(i < getSize());
      return *getPtr(i);
   }

   // Object accessor
   inline ValueType& at(uint i) 
   { 
      BDEBUG_ASSERT(i < getSize());
      return *getPtr(i);
   }
   
   const ValueType& front(void) const  { BDEBUG_ASSERT(!isEmpty()); return (*this)[0]; }
         ValueType& front(void)        { BDEBUG_ASSERT(!isEmpty()); return (*this)[0]; }

   const ValueType& back(void) const   { BDEBUG_ASSERT(!isEmpty()); return (*this)[getSize() - 1]; }
         ValueType& back(void)         { BDEBUG_ASSERT(!isEmpty()); return (*this)[getSize() - 1]; }

   const ValueType* begin(void) const  { return getPtr(0); }            
         ValueType* begin(void)        { return getPtr(0); }                  

   const ValueType* end(void) const  { return getPtr(getSize()); }            
         ValueType* end(void)        { return getPtr(getSize()); }                  
         
   const ValueType* getPtr(uint i = 0) const
   {
      // This assert is fudged to allow access to the element after the last.
      BDEBUG_ASSERT(i <= getSize());

      if ((Growable) && (isDynamic()))
      {
         BDEBUG_ASSERT(mData.mDynamic.mpAligned);
         return &mData.mDynamic.mpAligned[i];
      }
      else
      {
         // This assert is fudged to allow access to the element after the last.
         BDEBUG_ASSERT(i <= StaticSize);
         return elementPtr(i);
      }
   }

   ValueType* getPtr(uint i = 0) 
   {
      // This assert is fudged to allow access to the element after the last.
      BDEBUG_ASSERT(i <= getSize());

      if ((Growable) && (isDynamic()))
      {
         BDEBUG_ASSERT(NULL != mData.mDynamic.mpAligned);
         return &mData.mDynamic.mpAligned[i];
      }
      else
      {
         // This assert is fudged to allow access to the element after the last.
         BDEBUG_ASSERT(i <= StaticSize);
         return elementPtr(i);
      }
   }

   void erase(uint begin, uint end)
   {
      if (end == begin)
         return;
         
      BDEBUG_ASSERT(end >= begin);
      BDEBUG_ASSERT(begin < getSize());
      BDEBUG_ASSERT(end <= getSize());

      const uint numEntriesDeleted = end - begin;
      const uint numEntriesToCopy = getSize() - end;

      if (numEntriesToCopy)
      {
         if ((UseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
         {
            for (uint i = 0; i < numEntriesToCopy; i++)
               (*this)[begin + i] = (*this)[end + i];
         }
         else
         {
            memmove(getPtr(begin), getPtr(end), numEntriesToCopy * sizeof(ValueType));
         }                  
      }

      resize(getSize() - numEntriesDeleted);
   }
   
   void erase(iterator beginIter, iterator endIter)
   {
      erase(beginIter - begin(), endIter - begin());
   }

   void erase(uint index)
   {
      erase(index, index + 1);
   }
   
   void eraseUnordered(uint index)
   {
      BDEBUG_ASSERT(index < getSize());
      
      if ((index + 1) != getSize())
         (*this)[index] = (*this)[getSize() - 1];
      
      popBack();
   }
   
   // Insert num entries starting at position pos. Current entries will be moved to make room.
   // If pos == getSize(), this is equivalent to calling pushBack(), or resizing the container.
   // If pObjects is NULL, entries are inserted and constructed using the value type's default constructor.
   void insert(uint pos, uint num, const ValueType* pObjects)
   {
      if (!num)
         return;

      const uint origSize = getSize();
      if (pos > origSize)
      {
         BDEBUG_ASSERT(0);
         return;
      }
      else if (pos == origSize)
      {
         // Special case if inserting at the end.
         if (pObjects)
            pushBack(pObjects, num);
         else
            resize(origSize + num);
         return;
      }

      // Make room for new elements.
      const uint newSize = origSize + num;
      if (roomLeft() < num)
         makeRoom(num);

      setSize(newSize);

      const uint srcIndex = pos;
      const uint dstIndex = pos + num;
      const uint srcNum = origSize - pos;

      BDEBUG_ASSERT((srcNum <= origSize) && (srcIndex < origSize) && (dstIndex < newSize) && ((dstIndex + srcNum) == newSize));

      ValueType* p = getPtr();
      
      // Relocate old elements being displaced by insertion.
      if ((UseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
      {
         ValueType* pSrc = p + srcIndex + srcNum - 1;
         ValueType* pDst = p + dstIndex + srcNum - 1;
         for (int i = srcNum - 1; i >= 0; i--, pDst--, pSrc--)
         {
            Utils::ConstructInPlace(pDst, *pSrc);
            Utils::DestructInPlace(pSrc);
         }
      }
      else
      {
         memmove(p + dstIndex, p + srcIndex, srcNum * sizeof(ValueType));
      }

      // Construct new elements.
      if (pObjects)
         copyObjects(num, p + pos, pObjects);
      else 
         constructEntries(pos, num);
   }

   void insert(uint pos, const ValueType& object)
   {
      insert(pos, 1, &object);
   }

   // Quickly swaps container's contents with another.
   BStaticArray& swap(BStaticArray& rhs)
   {
      if (isDynamic())
      {
         if (rhs.isDynamic())
         {
            // both dynamic
            std::swap(mSize,                    rhs.mSize);
            std::swap(mData.mDynamic.mpAligned, rhs.mData.mDynamic.mpAligned);
            std::swap(mData.mDynamic.mCapacity, rhs.mData.mDynamic.mCapacity);
         }
         else 
         {
            // left dynamic, right static
            ValueType* pOldAligned  = mData.mDynamic.mpAligned;
            uint oldCapacity        = mData.mDynamic.mCapacity;

            moveObjects(rhs.getSize(), elementPtr(), rhs.elementPtr());

            std::swap(mSize, rhs.mSize);               
                        
            rhs.mData.mDynamic.mCapacity  = oldCapacity;
            rhs.mData.mDynamic.mpAligned  = pOldAligned;
            
            BDEBUG_ASSERT(!isDynamic());
            BDEBUG_ASSERT(rhs.isDynamic());
         }
      }
      else
      {
         if (rhs.isDynamic())
         {
            // left static, right dynamic
            ValueType* pOldAligned  = rhs.mData.mDynamic.mpAligned;
            const uint oldCapacity  = rhs.mData.mDynamic.mCapacity;
            
            moveObjects(getSize(), rhs.elementPtr(), elementPtr());

            std::swap(mSize, rhs.mSize);               
            
            mData.mDynamic.mCapacity   = oldCapacity;
            mData.mDynamic.mpAligned   = pOldAligned;
         }
         else
         {
            // both static, swap objects
            const uint numToSwap = Math::Min(getSize(), rhs.getSize());
            for (uint i = 0; i < numToSwap; i++)
               std::swap(element(i), rhs.element(i));
            
            if (getSize() > numToSwap)
            {
               // lhs has extra
               moveObjects(getSize() - numToSwap, rhs.elementPtr(numToSwap), elementPtr(numToSwap));
            }
            else if (rhs.getSize() > numToSwap)
            {
               // rhs has extra
               moveObjects(rhs.getSize() - numToSwap, elementPtr(numToSwap), rhs.elementPtr(numToSwap));
            }
            
            std::swap(mSize, rhs.mSize);
         }
      }
      
      allocator::swap(rhs);
                  
      return *this;
   }

   friend bool operator== (const BStaticArray& lhs, const BStaticArray& rhs) 
   {
      if (lhs.getSize() != rhs.getSize())
         return false;
      for (uint objIter = 0; objIter < lhs.getSize(); objIter++)
         if (!(lhs[objIter] == rhs[objIter]))
            return false;
      return true;
   }

   friend bool operator< (const BStaticArray& lhs, const BStaticArray& rhs) 
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

   friend bool operator >  (const BStaticArray& a, const BStaticArray& b) { return   b <  a;  }
   friend bool operator <= (const BStaticArray& a, const BStaticArray& b) { return !(b <  a); }
   friend bool operator >= (const BStaticArray& a, const BStaticArray& b) { return !(a <  b); }
   friend bool operator != (const BStaticArray& a, const BStaticArray& b) { return !(a == b); }

   void sort(void)
   {
      if (getSize())
         std::sort(getPtr(0), getPtr(getSize()));
   }
   
   // Removes duplicate elements that are adjacent to each other using std::unique(). 
   // Container must be sorted!
   inline void removeDuplicates(void)
   {
      if (getSize())
      {
         ValueType* pBegin = begin();
         ValueType* pNewEnd = std::unique(pBegin, end());
         resize(pNewEnd - pBegin);
      }
   }

   // Returns cInvalidIndex if not found.
   int binarySearch(const ValueType& searchObj) const
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
   
   bool binarySearch(const ValueType& searchObj, uint& index) const
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
      const ValueType* p = getPtr();
      const uint size = getSize();
      for (uint i = 0; i < size; i++, p++)
         if (*p == searchObj)
            return i;
      return cInvalidIndex;
   }

   // Sequential search, starting at beginning. Returns false if not found.
   inline bool find(const ValueType& searchObj, uint& index) const
   {
      const ValueType* p = getPtr();
      const uint size = getSize();
      for (uint i = 0; i < size; i++, p++)
      {
         if (*p == searchObj)
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
      const ValueType* p = getPtr();
      const uint size = getSize();
      for (int i = size - 1; i >= 0; i--)
         if (p[i] == searchObj)
            return i;
      return cInvalidIndex;
   }

   // Sequential search, starting at end. Returns false if not found.
   inline bool findLast(const ValueType& searchObj, uint& index) const
   {
      const ValueType* p = getPtr();
      const uint size = getSize();
      for (int i = size - 1; i >= 0; i--)
      {
         if (p[i] == searchObj)
         {
            index = i;
            return true;
         }
      }
      index = 0;
      return false;
   }

   inline void reverse(void)
   {
      ValueType* p = getPtr();
      const uint size = getSize();
      const uint n = size >> 1;
      for (uint i = 0; i < n; i++)
         std::swap(p[i], p[size - 1 - i]);
   }
   
   inline void setAll(const ValueType& t)
   {
      ValueType* p = getPtr();
      const uint size = getSize();
      if ((BIsBuiltInType<ValueType>::Flag) && (sizeof(ValueType) == 1))
      {
         Utils::FastMemSet(p, *reinterpret_cast<const uchar*>(&t), size);
      }
      else
      {
         for (uint i = size; i > 0; i--, p++)
            *p = t;
      }            
   }

   inline BDataBuffer getDataBuffer(void) { return BDataBuffer(reinterpret_cast<BYTE*>(getPtr()), getSizeInBytes()); }
   inline BConstDataBuffer getConstDataBuffer(void) const { return BConstDataBuffer(reinterpret_cast<const BYTE*>(getPtr()), getSizeInBytes()); }
   
   inline BTypedConstDataBuffer<ValueType> getTypedConstDataBuffer(void) const { return BTypedConstDataBuffer<ValueType>(getPtr(), getSize()); }
   inline operator BTypedConstDataBuffer<ValueType> () const { return getTypedConstDataBuffer(); }

   inline BStaticArray& operator+= (const BTypedConstDataBuffer<ValueType>& buf) { if (buf.getLen()) pushBack(buf.getPtr(), buf.getLen()); return *this; }
   inline BStaticArray& assign (const BTypedConstDataBuffer<ValueType>& buf) { BDEBUG_ASSERT(buf.getPtr() != getPtr()); clear(); if (buf.getLen()) pushBack(buf.getPtr(), buf.getLen()); return *this; }

   // Unlike BDynamicArray's, getNumber() returns a uint, not a long. (BDynamicArray's must return a long for backwards compatibility with the old BSimpleArray class.)
   inline uint getNumber(void) const { return getSize(); }

   // setNumber() cannot fail. If it does, the app has ran out of memory, and setNumber will not return.
   inline bool setNumber(uint newSize, bool force = false) 
   { 
      force;
      resize(newSize); 
      return true; 
   }

   inline bool validIndex(uint val) const { return val < getSize(); }

   // add() cannot fail. If it does, the app has ran out of memory, and add will not return.
   // So the returned index will always be valid.
   inline uint add(const ValueType& t) { pushBack(t); return getSize() - 1; }
   inline bool add(const ValueType* pSrc, uint num) { pushBack(pSrc, num); return true; }
   
   inline const   ValueType& get(uint i) const  { return at(i); }
   inline         ValueType& get(uint i)        { return at(i); }

   inline ValueType& grow(void) { return *enlarge(1); }

   inline void swapIndices(uint index1, uint index2)
   {
      if (validIndex(index1) && validIndex(index2) && (index1 != index2))
         std::swap(at(index1), at(index2));
   }

   inline uint insertAtIndex(const ValueType& t, uint index)
   {
      insert(index, t);
      return index;
   }

   inline uint uniqueAdd(const ValueType& t)
   {
      int pos = find(t);
      if (cInvalidIndex == pos)
         pos = add(t);
      return pos;
   }

   inline bool removeIndex(uint index, bool preserveOrder = true)
   {
      if (preserveOrder)
         erase(index);
      else
         eraseUnordered(index);
      return true;
   }

   inline bool removeValueAllInstances(const ValueType& t, bool preserveOrder = true)
   {
      uint index = 0;
      bool valueRemoved = false;

      while (index < getSize())
      {
         if (getPtr()[index] == t)
         {
            removeIndex(index, preserveOrder);
            valueRemoved = true;
         }
         else
            index++;
      }

      return valueRemoved;
   }

   inline bool remove(const ValueType& t, bool preserveOrder = true) 
   { 
      return removeValue(t, preserveOrder); 
   }

   inline bool removeValue(const ValueType& t, bool preserveOrder = true)
   {
      int index = find(t);
      if (index == cInvalidIndex)
         return false;
      removeIndex(index, preserveOrder);
      return true;
   }

   inline bool removeValueReverse(const ValueType& t, bool preserveOrder = true)
   {
      int index = findLast(t);
      if (index == cInvalidIndex)
         return false;
      removeIndex(index, preserveOrder);
      return true;
   }

   inline void setAt(uint pos, const ValueType& t)
   {
      resize(Math::Max(pos + 1, getSize()));
      at(pos) = t;
   }

   inline void sort(int (__cdecl *pCompareFunc)(const void *, const void *))
   {
      if (getSize())
         qsort(getPtr(), getSize(), sizeof(ValueType), pCompareFunc);
   }

   inline bool contains(const ValueType& t) const { return find(t) != cInvalidIndex; }

private:
   union
   {
      uchar mBuf[StaticSize ? (StaticSize * sizeof(ValueType) + ((cAlignment > 4) ? (cAlignment - 1) : 0)) : 1];
      
      struct 
      {
         ValueType*  mpAligned;
         uint        mCapacity;
      } mDynamic;
   } mData;
   
   uint mSize;
   
   inline const ValueType* elementPtr(uint i = 0) const   
   { 
      if (cAlignment <= 4)
         return reinterpret_cast<const ValueType*>(&mData.mBuf) + i; 
      else
         return reinterpret_cast<const ValueType*>(Utils::AlignUp(&mData.mBuf, cAlignment)) + i; 
   }
   
   inline ValueType* elementPtr(uint i = 0)         
   { 
      if (cAlignment <= 4)
         return reinterpret_cast<ValueType*>(&mData.mBuf) + i; 
      else
         return reinterpret_cast<ValueType*>(Utils::AlignUp(&mData.mBuf, cAlignment)) + i; 
   }
         
   inline const ValueType& element(uint i) const   { return *elementPtr(i); }
   inline       ValueType& element(uint i)         { return *elementPtr(i); }
         
   inline void setDynamic(void) 
   {
      if (Growable)
         mSize |= 0x80000000;
   }

   inline void setStatic(void)
   {
      if (Growable)
         mSize &= 0x7FFFFFFF;
   }
               
   inline void setSize(uint newSize) 
   { 
      BDEBUG_ASSERT(newSize <= 0x7FFFFFFF);
      mSize = Growable ? ((mSize & 0x80000000) | newSize) : newSize; 
   }
   
   inline void changeSize(int delta)
   {
#ifdef BUILD_DEBUG
      const uint origSize = mSize;
      mSize += delta;  
      origSize;
      mSize;
      BDEBUG_ASSERT((mSize & 0x80000000) == (origSize & 0x80000000));
#else   
      mSize += delta;  
#endif      
   }
                  
   inline void destructEntries(uint first, uint num)
   {
      if (!num)
         return;

      BDEBUG_ASSERT(getSize() <= getCapacity());
      BDEBUG_ASSERT(first < getSize());
      BDEBUG_ASSERT(first + num <= getSize());

      if ((UseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
      {
         ValueType* p = getPtr() + first;
         
         for (uint i = num; i > 0; i--, p++)
            Utils::DestructInPlace(p);
      }
      
#ifdef BUILD_DEBUG
      memset(getPtr() + first, 0xFD, sizeof(ValueType) * num);
#endif
   }

   inline void destructAllEntries(void)
   {
      destructEntries(0, getSize());
   }

   inline void constructEntries(uint first, uint num)
   {
      if (!num)
         return;

      BDEBUG_ASSERT(getSize() <= getCapacity());            
      BDEBUG_ASSERT(first < getCapacity());
      BDEBUG_ASSERT(first + num <= getCapacity());
      
      if (UseConstructorDestructor)
      {
         constructObjects(num, getPtr(first), ValueType());
      }
   }

   inline void changeCapacity(uint newCapacity)
   {
      const uint origCapacity = getCapacity();
      BDEBUG_ASSERT(getSize() <= origCapacity);

      if (newCapacity == origCapacity)
         return;
         
      if (!newCapacity)
      {
         destructAllEntries();
         
         if (isDynamic())
         {
            freeBuf(mData.mDynamic.mpAligned, origCapacity);
            setStatic();
         }
         
         setSize(0);
         return;
      }
      
      const uint cAlignment = ALIGN_OF(ValueType);
      cAlignment;
      
      if (newCapacity < getSize())
      {
         destructEntries(newCapacity, getSize() - newCapacity);
         setSize(newCapacity);
      }
      
      if (newCapacity <= StaticSize)
      {
         if (isDynamic())
         {
            // Was dynamic, make static.
            ValueType* pOldAligned = mData.mDynamic.mpAligned;
                        
            moveObjects(getSize(), elementPtr(), pOldAligned);
                                          
            freeBuf(pOldAligned, origCapacity);
            
            setStatic();
         }
      }
      else if (!isDynamic())
      {
         // Static, promote to dynamic.
                  
         if (!Growable)
         {
            BFATAL_FAIL("BStaticArray::changeCapacity: Attempt to grow an ungrowable array!");
         }
                           
         // Allocate new block
         ValueType* pNewAligned = reinterpret_cast<ValueType*>(allocBuf(newCapacity));
         BVERIFY(pNewAligned);
         BDEBUG_ASSERT(Utils::IsAligned(pNewAligned, cAlignment));
         
         if (getSize())
         {
            BDEBUG_ASSERT(getSize() <= newCapacity);
            
            moveObjects(getSize(), pNewAligned, elementPtr());
         }
         
         mData.mDynamic.mpAligned = pNewAligned;
         mData.mDynamic.mCapacity = newCapacity;
         
         setDynamic();
      }
      else
      {
         // Already dynamic - allocate new block.
         ValueType* pNewAligned = reinterpret_cast<ValueType*>(allocBuf(newCapacity));
         BVERIFY(pNewAligned);
         BDEBUG_ASSERT(Utils::IsAligned(pNewAligned, cAlignment));
         
         if (getSize())
         {
            BDEBUG_ASSERT(getSize() <= newCapacity);

            if (!mData.mDynamic.mpAligned)
            {
               // Initialize new objects
               if (UseConstructorDestructor)
                  constructObjects(getSize(), pNewAligned, ValueType());
            }
            else
            {
               moveObjects(getSize(), pNewAligned, mData.mDynamic.mpAligned);
            }
         }

         freeBuf(mData.mDynamic.mpAligned, origCapacity);

         mData.mDynamic.mpAligned = pNewAligned;          
         mData.mDynamic.mCapacity = newCapacity;   
      }            
   }

   inline uint roomLeft(void) const
   {
      BDEBUG_ASSERT(getSize() <= getCapacity());

      const uint roomLeft = getCapacity() - getSize();

      return roomLeft;
   }

   inline void makeRoom(uint numNewEntries)
   {
      uint curRoomLeft = roomLeft();

      if (numNewEntries > curRoomLeft)
      {
         if (!Growable)
         {
            BFATAL_FAIL("BStaticArray::makeRoom: Attempt to grow an ungrowable array!");
         }
         else
         {
            const uint minCapacity = Math::Max<uint>(1, 4 / sizeof(ValueType));
            BVERIFY(getCapacity() < 0x3FFFFFFF);
            const uint newCapacity = Math::Max3(getCapacity() * 2, minCapacity, getSize() + numNewEntries);

            // First try taking the largest available block by resizing in-place
            changeCapacity(newCapacity);
            curRoomLeft = getCapacity() - getSize();
            
            BVERIFY(numNewEntries <= curRoomLeft);
         }               
      }
   }

   static inline void constructObjects(uint num, ValueType* pDst, const ValueType& v)
   {
      BDEBUG_ASSERT(UseConstructorDestructor);

      if (BIsBuiltInType<ValueType>::Flag)
      {
         Utils::FastMemSet(pDst, 0, num * sizeof(ValueType));
      }
      else
      {
         const ValueType* pEndDst = pDst + num;
         while (pDst != pEndDst)
         {
            Utils::ConstructInPlace(pDst, v);
            pDst++;
         }
      }         
   }
         
   // src/dst should not overlap!
   static inline void moveObjects(uint num, ValueType* pDst, const ValueType* pSrc)
   {
      if ((UseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
      {
         const ValueType* pEndDst = pDst + num;
         while (pDst != pEndDst)
         {
            Utils::ConstructInPlace(pDst, *pSrc);
            Utils::DestructInPlace(pSrc);
            pSrc++;
            pDst++;
         }
      }
      else
      {
         memmove(pDst, pSrc, num * sizeof(ValueType));
      }            
   }         
   
   // src/dst should not overlap!
   static inline void copyObjects(uint num, ValueType* pDst, const ValueType* pSrc, bool destructSrcElements = false)
   {
      BDEBUG_ASSERT( ((pSrc + num) <= pDst) || (pSrc >= (pDst + num)) );

      if ((UseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
      {
         const ValueType* pEndDst = pDst + num;
         while (pDst != pEndDst)
         {
            Utils::ConstructInPlace(pDst, *pSrc);
            if (destructSrcElements)
               Utils::DestructInPlace(pSrc);
            pSrc++;
            pDst++;
         }
      }
      else
      {
         Utils::FastMemCpy(pDst, pSrc, num * sizeof(ValueType));
      }
   }         

};

template<
   class ValueType, 
   uint StaticSize = 4, 
   bool Growable = true, 
   bool UseConstructorDestructor = true
>
class BStaticSimArray : public BStaticArray<ValueType, StaticSize, Growable, UseConstructorDestructor, BDynamicArraySimHeapAllocator>
{
public:
   BStaticSimArray() : BStaticArray()
   {
   }
   
   BStaticSimArray(uint initialSize) : BStaticArray(initialSize)
   {
   }
};

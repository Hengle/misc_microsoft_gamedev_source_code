#pragma once

template <class Key, class T, uint N, class LessThan, class Allocator> class BSkipList;
template <class Key, class T, uint N, class LessThan, class Allocator> class BSkipListNode;

//------------------------------------------------------------------------------
// struct BSkipListTypes
//------------------------------------------------------------------------------
template <class Key, class T, uint N, class LessThan, class Allocator>
struct BSkipListTypes
{
   enum { INVALID_INDEX = -1 };

   typedef Key                            KeyType;
   typedef T                              ReferentType;
   typedef LessThan                       LessThanType;
   typedef std::pair<const Key, T>        ValueType;
   typedef int                            IndexType;
   typedef BSkipList<Key, T, N, LessThan, Allocator> SkipListType;
   typedef BSkipListNode<Key, T, N, LessThan, Allocator> NodeType;

   enum
   {
      cFlagHead = 1
   };

   enum
   {
      cBothAreBuiltInTypes = (BIsBuiltInType<Key>::Flag != 0) && (BIsBuiltInType<T>::Flag != 0)
   };
};

//------------------------------------------------------------------------------
// struct BSkipListNode
//------------------------------------------------------------------------------
template <class Key, class T, uint N, class LessThan, class Allocator>
class BSkipListNode : public BSkipListTypes<Key, T, N, LessThan, Allocator>
{
public:
   static inline BSkipListNode* allocNode(uint numNextPointers, Allocator& allocator)
   {
      BDEBUG_ASSERT(numNextPointers > 0);

      DWORD* p = static_cast<DWORD*>(allocator.alloc(sizeof(BSkipListNode) + sizeof(BSkipListNode*) * (numNextPointers - 1)));

      BSkipListNode* pNode = reinterpret_cast<BSkipListNode*>(p + (numNextPointers - 1));
      if (!pNode)
         BFATAL_FAIL("Out of memory");

      pNode->mNumNextPointers = static_cast<uint8>(numNextPointers);
      pNode->mNumItems = 0;
      pNode->mFlags = 0;

      return pNode;
   }

   static inline void freeNode(BSkipListNode* pNode, Allocator& allocator)
   {
      if (!pNode)
         return;

      if (!cBothAreBuiltInTypes)
      {
         for (uint i = 0; i < pNode->mNumItems; i++)
            pNode->destroyItem(i);
      }

      DWORD* p = reinterpret_cast<DWORD*>(pNode);
      p -= (pNode->mNumNextPointers - 1);
      allocator.dealloc(p);
   }

   inline void setHeadFlag() { mFlags |= cFlagHead; }
   inline bool getHeadFlag() const { return (mFlags & cFlagHead) != 0; }

   inline void setNext(uint i, BSkipListNode* p) 
   {
      BDEBUG_ASSERT(i < mNumNextPointers);
      mpNext[-((int)i)] = p;
   }

   inline BSkipListNode* getNext(uint i) const
   {
      BDEBUG_ASSERT(i < mNumNextPointers);
      return mpNext[-((int)i)];
   }

   inline void setPrev(BSkipListNode* p) 
   {
      mpPrev = p;
   }

   inline BSkipListNode* getPrev() const
   {
      return mpPrev;
   }

   inline uint getNumItems() const { return mNumItems; }
   inline void setNumItems(uint numItems) { mNumItems = static_cast<uint16>(numItems); }
   inline bool isFull() const { return mNumItems == N; }
   inline bool isEmpty() const { return !mNumItems; }

   inline uint getNumNextPointers() const { return mNumNextPointers; }

   inline const ValueType& getItem(uint i) const { BDEBUG_ASSERT(!getHeadFlag() && (i < N)); return mItems[i]; }
   inline       ValueType& getItem(uint i)       { BDEBUG_ASSERT(!getHeadFlag() && (i < N)); return mItems[i]; }

   inline const Key& getItemKey(uint i) const { BDEBUG_ASSERT(!getHeadFlag() && (i < N)); return mItems[i].first; }

   inline const T& getItemData(uint i) const { BDEBUG_ASSERT(!getHeadFlag() && (i < N)); return mItems[i].second; }
   inline       T& getItemData(uint i)       { BDEBUG_ASSERT(!getHeadFlag() && (i < N)); return mItems[i].second; }

   inline const Key& getLowestKey() const { BDEBUG_ASSERT(!getHeadFlag()); return mItems[0].first; }
   inline const Key& getHighestKey() const { BDEBUG_ASSERT(!getHeadFlag()); BDEBUG_ASSERT(mNumItems); return mItems[mNumItems - 1].first; }

   inline bool keyLess(const Key& a, const Key& b, const LessThan& lessThan) const { return lessThan(a,  b); }
   inline bool keyLessEqual(const Key& a, const Key& b, const LessThan& lessThan) const { return !lessThan(b,  a); }
   inline bool keyGreater(const Key& a, const Key& b, const LessThan& lessThan) const { return lessThan(b,  a); }
   inline bool keyGreaterEqual(const Key& a, const Key& b, const LessThan& lessThan) const { return !lessThan(a,  b); }
   inline bool keyEqual(const Key& a, const Key& b, const LessThan& lessThan) const { return !lessThan(a, b) && !lessThan(b, a); }
   inline bool keyNotEqual(const Key& a, const Key& b, const LessThan& lessThan) const { return !keyEqual(a, b, lessThan); }

   inline bool findLowerKeyLinear(const Key& key, uint& index, const LessThan& lessThan)
   {
      BDEBUG_ASSERT(!getHeadFlag());

      for (uint i = 0; i < mNumItems; i++)
      {
         const Key& itemKey = mItems[i].first;

         if (keyEqual(key, itemKey, lessThan))
         {
            index = i;
            return true;
         }
         else if (keyLess(key, itemKey, lessThan))
         {
            index = i;
            return false;
         }
      }

      index = mNumItems;
      return false;
   }

   inline bool findAnyKeyBinary(const Key& key, uint& index, const LessThan& lessThan)
   {
      BDEBUG_ASSERT(!getHeadFlag());

      int l = 0;
      int r = mNumItems - 1;

      int m = 0;
      bool compResult = false;
      while (r >= l)
      {
         m = (l + r) >> 1;
         const Key& itemKey = mItems[m].first;

         if (keyEqual(itemKey, key, lessThan))
         {
            index = m;
            return true;
         }
         compResult = lessThan(itemKey, key);
         if (compResult)
            l = m + 1;
         else
            r = m - 1;
      }

      index = r + 1;

#ifdef BUILD_DEBUG      
      if (compResult)
         BDEBUG_ASSERT((int)index == m + 1);
      else
         BDEBUG_ASSERT((int)index == m);
#endif         

      BDEBUG_ASSERT(index <= N);
      BDEBUG_ASSERT((index >= mNumItems) || lessThan(key, mItems[index].first));

      return false;
   }

   // Finds first key less than or equal to key.
   inline bool findLowerKeyBinary(const Key& key, uint& index, const LessThan& lessThan)
   {
      BDEBUG_ASSERT(!getHeadFlag());

      if (!mNumItems)
      {
         index = 0;
         return false;
      }

      int l = 0;
      int r = mNumItems - 1;

      while (l < r)
      {
         int m = (l + r) >> 1;

         if (keyLessEqual(key, mItems[m].first, lessThan))
            r = m - 1;
         else
            l = m + 1;
      }

      if ((r < 0) || (keyLess(mItems[r].first, key, lessThan)))
         r++;

      index = r;

      if (keyEqual(mItems[index].first, key, lessThan))
         return true;

      BDEBUG_ASSERT(index <= N);
      BDEBUG_ASSERT((index >= mNumItems) || keyLess(key, mItems[index].first, lessThan));

      return false;
   }

   inline bool findLowerKey(const Key& key, uint& index, const LessThan& lessThan)
   {
      return (N < 4) ? findLowerKeyLinear(key, index, lessThan) : findLowerKeyBinary(key, index, lessThan);
   }

   // Finds first key greater than key.
   inline uint findUpperKeyBinary(const Key& key, const LessThan& lessThan)
   {
      BDEBUG_ASSERT(!getHeadFlag());

      if (!mNumItems)
         return 0;

      int l = 0;
      int r = mNumItems - 1;

      while (l < r)
      {
         int m = (l + r) >> 1;

         if (keyLess(key, mItems[m].first, lessThan))
            r = m - 1;
         else
            l = m + 1;
      }

      if ((r < 0) || (keyLessEqual(mItems[r].first, key, lessThan)))
         r++;

      return r;
   }

   inline int compareKey(const Key& key, LessThan& lessThan) const
   {
      BDEBUG_ASSERT(mNumItems);
      BDEBUG_ASSERT(!getHeadFlag());

      if (lessThan(key, mItems[0].first))
         return -1;

      if (lessThan(mItems[mNumItems - 1].first, key))
         return 1;

      return 0;
   }

   inline void constructItem(uint index, const Key& key, const T& data)
   {
      BDEBUG_ASSERT(!getHeadFlag());
      ValueType& item = mItems[index];

      if (cBothAreBuiltInTypes)
      {
         memcpy(const_cast<Key*>(&item.first), &key, sizeof(key));
         memcpy(&item.second, &data, sizeof(data));
      }
      else
      {
         Utils::ConstructInPlace(const_cast<Key*>(&item.first), key);
         Utils::ConstructInPlace(&item.second, data);
      }
   }

   inline void constructItem(uint index, const ValueType& v)
   {
      BDEBUG_ASSERT(!getHeadFlag());
      ValueType& item = mItems[index];

      if (cBothAreBuiltInTypes)
      {
         memcpy(&item, &v, sizeof(ValueType));
      }
      else
      {
         Utils::ConstructInPlace(const_cast<Key*>(&item.first), v.first);
         Utils::ConstructInPlace(&item.second, v.second);
      }
   }

   inline void destroyItem(uint index)
   {
      BDEBUG_ASSERT(!getHeadFlag());
      if (!cBothAreBuiltInTypes)
      {
         ValueType& v = mItems[index];
         Utils::DestructInPlace(&v);
      }
   }

   inline void moveEntries(uint dstIndex, uint srcIndex)
   {
      BDEBUG_ASSERT(!getHeadFlag());
      BDEBUG_ASSERT(srcIndex != dstIndex);
      if (cBothAreBuiltInTypes)
      {
         memcpy(&mItems[dstIndex], &mItems[srcIndex], sizeof(ValueType));
      }
      else
      {
         Utils::ConstructInPlace(const_cast<Key*>(&mItems[dstIndex].first), mItems[srcIndex].first);
         Utils::ConstructInPlace(&mItems[dstIndex].second, mItems[srcIndex].second);
         Utils::DestructInPlace(&mItems[srcIndex]);
      }
   }

   inline void removeItem(uint index)
   {
      BDEBUG_ASSERT(index < mNumItems);
      BDEBUG_ASSERT(!getHeadFlag());

      if (cBothAreBuiltInTypes)
      {
         const uint numItemsToMove = mNumItems - 1 - index;
         if (numItemsToMove)         
            memmove(&mItems[index], &mItems[index + 1], sizeof(ValueType) * numItemsToMove);
      }
      else
      {
         Utils::DestructInPlace(&mItems[index]);

         for (int i = index; i < (int)(mNumItems - 1); i++)
            moveEntries(i, i + 1);
      }            

      mNumItems--;
   }

   inline void insertItem(uint index, const Key& key, const T& data)
   {
      BDEBUG_ASSERT(mNumItems < N);
      BDEBUG_ASSERT(index <= mNumItems);
      BDEBUG_ASSERT(!getHeadFlag());

      for (int i = mNumItems - 1; i >= (int)index; i--)
         moveEntries(i + 1, i);

      constructItem(index, key, data);

      mNumItems++;
   }

   inline void insertItem(uint index, const ValueType& v)
   {
      insertItem(index, v.first, v.second);  
   }

   inline bool tryRotatePrev(BSkipListNode* pPrev)
   {
      BDEBUG_ASSERT(mNumItems);

      if (N < 2)
         return false;

      if (pPrev->getHeadFlag())
         return false;
      if (pPrev->getNumItems() == N)
         return false;

      pPrev->insertItem(pPrev->getNumItems(), mItems[0].first, mItems[0].second);

      removeItem(0);

      return true;
   }

   inline bool tryRotateNext()
   {
      BDEBUG_ASSERT(mNumItems);

      if (N < 2)
         return false;

      BSkipListNode* pNext = getNext(0);
      if (pNext->getHeadFlag())
         return false;
      if (pNext->getNumItems() == N)
         return false;

      pNext->insertItem(0, mItems[mNumItems - 1].first, mItems[mNumItems - 1].second);

      removeItem(mNumItems - 1);

      return true;
   }

private:
   // mpNext MUST be first. There may be additional node pointers BEFORE this structure if mNumNextPointers is > 1.
   BSkipListNode*    mpNext[1];     
   BSkipListNode*    mpPrev;

   uint16            mNumItems;
   uint8             mNumNextPointers;
   uint8             mFlags;

   ValueType         mItems[N];
};

//------------------------------------------------------------------------------
template <class Key, class T, uint N, class LessThan, class Allocator> class BSkipList;
template <class Key, class T, uint N, class LessThan, class Allocator> class BSkipListIterator;
template <class Key, class T, uint N, class LessThan, class Allocator> class BSkipListConstIterator;
//------------------------------------------------------------------------------
// class BSkipListIteratorBase
//------------------------------------------------------------------------------
template <class Key, class T, uint N, class LessThan, class Allocator>
class BSkipListIteratorBase : public BSkipListTypes<Key, T, N, LessThan, Allocator>
{
public:

protected:
   const SkipListType*  mpSkipList;
   const NodeType*      mpNode;
   uint                 mItemIndex;

   BSkipListIteratorBase() : mpSkipList(NULL), mpNode(NULL), mItemIndex(0) { } 

   BSkipListIteratorBase(const SkipListType* pSkipList, const NodeType* pNode, uint itemIndex) :
   mpSkipList(pSkipList), mpNode(pNode), mItemIndex(itemIndex) { }

   BSkipListIteratorBase(const BSkipListIteratorBase& other) :
   mpSkipList(other.mpSkipList), mpNode(other.mpNode), mItemIndex(other.mItemIndex) { }

   BSkipListIteratorBase& operator= (const BSkipListIteratorBase& rhs)
   {
      mpSkipList  = rhs.mpSkipList;
      mpNode      = rhs.mpNode;
      mItemIndex  = rhs.mItemIndex;
      return *this;
   }

   inline bool operator== (const BSkipListIteratorBase& rhs) const
   {
      return (mpSkipList == rhs.mpSkipList) && (mpNode == rhs.mpNode) && (mItemIndex == rhs.mItemIndex);
   }

   inline void moveForward() 
   {
      if ((!mpNode) || (mpNode->getHeadFlag()))
         return;

      if ((mItemIndex + 1) >= mpNode->getNumItems())
      {
         mpNode = mpNode->getNext(0);
         mItemIndex = 0;
      }
      else 
      {
         mItemIndex++;
      }
   }

   inline void moveBackward() 
   {
      if (!mpNode)
         return;

      if (0 == mItemIndex)
      {
         const NodeType* pPrevNode = mpNode->getPrev();
         if (pPrevNode == mpSkipList->mpHead)
            return;

         mpNode = pPrevNode;
         mItemIndex = pPrevNode->getNumItems() - 1;
      }
      else 
      {
         mItemIndex--;
      }
   }

   inline BSkipListIteratorBase move(int i) const
   {
      BSkipListIteratorBase result(*this);

      if (i < 0)
      {
         do
         {
            result.moveBackward();
            i++;
         } while (i < 0);
      }
      else
      {
         while (i > 0)
         {
            result.moveForward();
            i--;
         }
      }
      return result;
   }
};
//------------------------------------------------------------------------------
// class BSkipListIterator
//------------------------------------------------------------------------------
template <class Key, class T, uint N, class LessThan, class Allocator>
class BSkipListIterator : public BSkipListIteratorBase<Key, T, N, LessThan, Allocator>
{
private:
   friend class BSkipListConstIterator<Key, T, N, LessThan, Allocator>;
   friend class BSkipList<Key, T, N, LessThan, Allocator>;

   inline ValueType* getCurrent() const
   {
      BDEBUG_ASSERT(mpSkipList);
      BDEBUG_ASSERT(mpNode && !mpNode->getHeadFlag());
      return const_cast<ValueType*>(&mpNode->getItem(mItemIndex));
   }

public:
   inline BSkipListIterator() : BSkipListIteratorBase<Key, T, N, LessThan, Allocator>() { }
   inline BSkipListIterator(const SkipListType* pSkipList, const NodeType* pNode, uint index) : BSkipListIteratorBase<Key, T, N, LessThan, Allocator>(pSkipList, pNode, index) { }
   inline BSkipListIterator(const BSkipListIteratorBase& it) : BSkipListIteratorBase<Key, T, N, LessThan, Allocator>(it) { }
   inline BSkipListIterator& operator= (const BSkipListIteratorBase& it) { BSkipListIteratorBase<Key, T, N, LessThan, Allocator>::operator=(it); return *this; }

   inline BSkipListIterator operator++(int)  //post
   {
      BSkipListIterator result(*this);
      ++*this;
      return result;
   }

   inline BSkipListIterator& operator++()  //pre
   {
      BDEBUG_ASSERT(mpSkipList && mpNode);
      moveForward();
      return *this;
   }

   inline BSkipListIterator operator--(int)  //post
   {
      BSkipListIterator result(*this);
      --*this;
      return result;
   }

   inline BSkipListIterator& operator--()  //pre
   {
      BDEBUG_ASSERT(mpSkipList && mpNode);
      moveBackward();
      return *this;
   }

   inline ValueType& operator*() const {  return *getCurrent(); }
   inline ValueType* operator->() const { return getCurrent(); }

   inline BSkipListIterator operator+ (int i) const { return BSkipListIterator(move(i)); }
   inline BSkipListIterator operator- (int i) const { return move(-i); }
   inline BSkipListIterator& operator+= (int i) { *this = move(i); return *this; }
   inline BSkipListIterator& operator-= (int i) { *this = move(-i); return *this; }

   inline bool operator == (const BSkipListIterator& b) const { return BSkipListIteratorBase<Key, T, N, LessThan, Allocator>::operator==(b); }
   inline bool operator != (const BSkipListIterator& b) const { return !(*this == b); }
   inline bool operator == (const BSkipListConstIterator<Key, T, N, LessThan, Allocator>& b) const { return BSkipListIteratorBase<Key, T, N, LessThan, Allocator>::operator==(b); }   
   inline bool operator != (const BSkipListConstIterator<Key, T, N, LessThan, Allocator>& b) const { return !(*this == b); }
};
//------------------------------------------------------------------------------
// class BSkipListConstIterator
//------------------------------------------------------------------------------
template <class Key, class T, uint N, class LessThan, class Allocator>
class BSkipListConstIterator : public BSkipListIteratorBase<Key, T, N, LessThan, Allocator>
{
private:
   friend class BSkipListIterator<Key, T, N, LessThan, Allocator>;
   friend class BSkipList<Key, T, N, LessThan, Allocator>;

   const ValueType* getCurrent() const
   {
      BDEBUG_ASSERT(mpSkipList);
      BDEBUG_ASSERT(mpNode && !mpNode->getHeadFlag());
      return &mpNode->getItem(mItemIndex);
   }

public:
   inline BSkipListConstIterator() : BSkipListIteratorBase<Key, T, N, LessThan, Allocator>() { }
   inline BSkipListConstIterator(const SkipListType* pSkipList, const NodeType* pNode, uint index) : BSkipListIteratorBase<Key, T, N, LessThan, Allocator>(pSkipList, pNode, index) { }
   inline BSkipListConstIterator(const BSkipListConstIterator& it) : BSkipListIteratorBase<Key, T, N, LessThan, Allocator>(it) { }
   inline BSkipListConstIterator& operator= (const BSkipListConstIterator& it) { BSkipListIteratorBase<Key, T, N, LessThan, Allocator>::operator=(it); return *this; }
   inline BSkipListConstIterator(const BSkipListIteratorBase<Key, T, N, LessThan, Allocator>& it) : BSkipListIteratorBase<Key, T, N, LessThan, Allocator>(it) { }
   inline BSkipListConstIterator& operator= (const BSkipListIteratorBase<Key, T, N, LessThan, Allocator>& it) { BSkipListIteratorBase<Key, T, N, LessThan, Allocator>::operator=(it); return *this; }

   inline BSkipListConstIterator operator++(int)  //post
   {
      BSkipListConstIterator result(*this);
      ++*this;
      return result;
   }

   inline BSkipListConstIterator& operator++()  //pre
   {
      BDEBUG_ASSERT(mpSkipList);
      moveForward();
      return *this;
   }

   inline BSkipListConstIterator operator--(int)  //post
   {
      BSkipListConstIterator result(*this);
      --*this;
      return result;
   }

   inline BSkipListConstIterator& operator--()  //pre
   {
      BDEBUG_ASSERT(mpSkipList);
      moveBackward();
      return *this;
   }

   inline BSkipListConstIterator operator+ (int i) const { return move(i); }
   inline BSkipListConstIterator operator- (int i) const { return move(-i); }
   inline BSkipListConstIterator& operator+= (int i) { *this = move(i); return *this; }
   inline BSkipListConstIterator& operator-= (int i) { *this = move(-i); return *this; }

   inline const ValueType& operator*() const {  return *getCurrent(); }
   inline const ValueType* operator->() const { return getCurrent(); }

   inline bool operator == (const BSkipListConstIterator& b) const { return BSkipListIteratorBase<Key, T, N, LessThan, Allocator>::operator==(b); }
   inline bool operator != (const BSkipListConstIterator& b) const { return !(*this == b); }
   inline bool operator == (const BSkipListIterator<Key, T, N, LessThan, Allocator>& b) const { return BSkipListIteratorBase<Key, T, N, LessThan, Allocator>::operator==(b); }
   inline bool operator != (const BSkipListIterator<Key, T, N, LessThan, Allocator>& b) const { return !(*this == b); }
};
// File: hashMap.inl

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline int BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::tableSize(void) const 
{ 
   return mRootSize + mMaxEntries; 
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline bool BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::isFull(void) const
{
   return ((mFreeListHead == INVALID_INDEX) && (mNextFree == tableSize()));
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline int BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::freeEntry(void)
{
   if (mFreeListHead != INVALID_INDEX)
   {
      const int ret = mFreeListHead;
      debugRangeCheck(ret, mRootSize, tableSize());
      mFreeListHead = mpTable[ret].next;
      return ret;       
   }
   
   if (mNextFree == tableSize())
      return INVALID_INDEX;
   
   return mNextFree++;
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline void BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::freeEntry(int i)
{
   debugRangeCheck(i, mRootSize, tableSize());
   mpTable[i].next = mFreeListHead;
   mFreeListHead = i;
}
   
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline void BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::destructEntry(int i)
{
   debugRangeCheck(i, tableSize());
   if (UseConstructorDestructor)
   {
      Utils::DestructInPlace(const_cast<Key*>(&mpTable[i].first));
      Utils::DestructInPlace(const_cast<T*>(&mpTable[i].second));
   }
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline void BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::constructEntry(int i, const Key& key, const T& val)
{
   debugRangeCheck(i, tableSize());
   if (UseConstructorDestructor)
   {
      Utils::ConstructInPlace(const_cast<Key*>(&mpTable[i].first), key);
      Utils::ConstructInPlace(const_cast<T*>(&mpTable[i].second), val);
   }
   else
   {
      Utils::FastMemCpy(const_cast<Key*>(&mpTable[i].first), &key, sizeof(Key));
      Utils::FastMemCpy(const_cast<T*>(&mpTable[i].second), &val, sizeof(T));
   }
}
 
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>  
inline typename BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::InsertResult BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::createEntry(int root, int cur, const ValueType& value)
{
   constructEntry(cur, value.first, value.second);
   mNumEntries++;
   return std::make_pair(iterator(this, root, cur), true);
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline void BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::destroyEntry(int prev, int cur)
{
   if (prev != INVALID_INDEX)
   {
      // general entry
      debugRangeCheck(cur, mRootSize, tableSize());

      mpTable[prev].next = mpTable[cur].next;

      destructEntry(cur);
      freeEntry(cur);
   }
   else 
   {
      // root entry
      debugRangeCheck(cur, mRootSize);

      const int next = mpTable[cur].next;
      BDEBUG_ASSERT(next != cur);
      if (next == INVALID_INDEX)
      {
         destructEntry(cur);
         mpTable[cur].next = cur;
      }
      else
      {
         // Deleting a root entry, and there's a valid next node. 
         // We can't delete the root entry. Instead, move the next node into the root, and 
         // then delete the next entry.
         *const_cast<Key*>(&mpTable[cur].first) = mpTable[next].first;
         *const_cast<T*>(&mpTable[cur].second) = mpTable[next].second;
         mpTable[cur].next = mpTable[next].next;
         
         destructEntry(next);
         freeEntry(next);
      }
   }

   mNumEntries--;
   BDEBUG_ASSERT(mNumEntries >= 0);
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline int BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::findFirst(void) const
{
   while (mLowestRoot < mRootSize)
   {
      if (mpTable[mLowestRoot].next != mLowestRoot)
         break;
      ++mLowestRoot;
   }
   return mLowestRoot;
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline void BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::grow(void)
{
   const int new_maxEntries = Math::Max(1, 2 * mMaxEntries);
         
   HashMapType newMap(new_maxEntries, INT_MAX, mHasher, mEquals);
   
   for (const_iterator i = begin(); i != end(); ++i)
   {
      InsertResult results;
      newMap.insertNoGrow(*i, results);
   }
   
   swap(newMap);
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline void BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::init(int maxEntries, int rootSize)
{ 
   BDEBUG_ASSERT(maxEntries >= 0);
   
   if (mpTable)
   {
      mAlloc.dealloc(mpTable);
      mpTable = NULL;
   }
   
   mMaxEntries = maxEntries;
   // rg [1/19/08] - This was dividing by 8, but this seemed excessive.
   mRootSize = mMaxEntries ? ((rootSize == INT_MAX) ? Math::NextPowerOf2((maxEntries + 15) >> 4) : rootSize) : 0; 
   mRootMask = mRootSize - 1;
   mNumEntries = 0;
   mNextFree = mRootSize;
   mFreeListHead = INVALID_INDEX;
   mLowestRoot = mRootSize;
            
   if (mRootSize)
   {
      BDEBUG_ASSERT(mRootSize > 0);
      BDEBUG_ASSERT(Math::IsPow2(mRootSize));
      BDEBUG_ASSERT(mMaxEntries > 0);

      mpTable = reinterpret_cast<NodeType*>(mAlloc.alloc((mRootSize + mMaxEntries) * sizeof(NodeType)));
      if (!mpTable)
      {
         BFATAL_FAIL("BHashMap: alloc failed");
      }

      for (int i = 0; i < mRootSize; i++)
         mpTable[i].next = i;
   }            
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline void BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::destroy(void)
{
   if (mpTable)
   {
      if (mNumEntries) 
      {
         if ( (!BIsBuiltInType<Key>::Flag) || (!BIsBuiltInType<T>::Flag) )
         {
            for (int i = mLowestRoot; i < mRootSize; i++)
               if (mpTable[i].next != i)
               {
                  for (int cur = i; cur != INVALID_INDEX; cur = mpTable[cur].next)
                     destructEntry(cur);
               }                     
         }
      }

      mAlloc.dealloc(mpTable);
   }
}
       
// Returns false if one or more objects could not be inserted.
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline bool BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::clone(const BHashMap& other)
{
   init(other.mMaxEntries, other.mRootSize);
   
   bool succeeded = true;
   for (const_iterator it = other.begin(); it != other.end(); ++it)
   {
      InsertResult res = insert(*it);
      if (!res.second)
         succeeded = false;
   }
   return succeeded;
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline typename BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::const_iterator BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::findConst(const Key& key) const
{
   if (!mpTable)
      return end();
   const int root = mHasher(key) & mRootMask;
   if (mpTable[root].next == root)
      return end();
   int cur = root;
   do
   {
      if (mEquals(mpTable[cur].first, key))
         return const_iterator(this, root, cur);
      cur = mpTable[cur].next;
   } while (cur != INVALID_INDEX);
   return end();
}  
     
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>    
inline BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::BHashMap(int maxEntries, int rootSize, const HasherType& hashFunctor, const EqualsType& equalFunctor, const Allocator& alloc) :
   mHasher(hashFunctor),
   mEquals(equalFunctor),
   mAlloc(alloc),
   mpTable(NULL),
   mRootSize(0),
   mRootMask(0),
   mMaxEntries(0),
   mNumEntries(0),
   mNextFree(0),
   mFreeListHead(INVALID_INDEX),
   mLowestRoot(0)
{
   init(maxEntries, rootSize);
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::BHashMap(const BHashMap& other) :
   mHasher(other.mHasher),
   mEquals(other.mEquals),
   mAlloc(other.mAlloc),
   mpTable(NULL),
   mRootSize(0),
   mRootMask(0),
   mMaxEntries(0),
   mNumEntries(0),
   mNextFree(0),
   mFreeListHead(INVALID_INDEX),
   mLowestRoot(0)
{
   clone(other);
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>& BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::operator= (const BHashMap& rhs)
{
   if (this != &rhs)
   {
      destroy();
      
      mHasher = rhs.mHasher;
      mEquals = rhs.mEquals;
      mAlloc = rhs.mAlloc;
      mpTable = NULL;
      mRootSize = 0;
      mRootMask = 0;
      mMaxEntries = 0;
      mNumEntries = 0;
      mNextFree = 0;
      mFreeListHead = INVALID_INDEX;
      mLowestRoot = 0;
      
      clone(rhs);
   }
   
   return *this;
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::~BHashMap()
{
   destroy();
}

// This erases all objects from the container, but does not free the container's table allocation or change its maximum current size.
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline void BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::clear()
{
   if (mNumEntries)
   {
      for (int i = mLowestRoot; i < mRootSize; i++)
         if (mpTable[i].next != i)
         {
            if ( (!BIsBuiltInType<Key>::Flag) || (!BIsBuiltInType<T>::Flag) )
            {
               for (int cur = i; cur != INVALID_INDEX; cur = mpTable[cur].next)
                  destructEntry(cur);
            }
            mpTable[i].next = i;
         }
   }

   mNumEntries = 0;
   mNextFree = mRootSize;
   mFreeListHead = INVALID_INDEX;
   mLowestRoot = mRootSize;
}

// This erases all objects in the container, then changes the container's maximum size. 
// Specify 0 for maxEntries to force the container to delete the table.
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline void BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::setMaxEntries(int maxEntries, int rootSize)
{
   clear();
         
   init(maxEntries, rootSize);
}

// Returns the maximum number of objects that can be added to the container before it must grow.
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline int BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::getMaxEntries(void) const
{  
   return tableSize();
}

// Returns the current number of objects present in the container.
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline int BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::size(void) const
{
   return mNumEntries;
}

// Returns the current number of objects present in the container.
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline int BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::getSize(void) const
{
   return mNumEntries;
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline bool BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::empty(void) const
{
   return 0 == mNumEntries;
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline bool BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::getEmpty(void) const
{
   return 0 == mNumEntries;
}

// Returns the size of the allocated hash table in bytes.
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline uint BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::getAllocationSize(void) const
{
   return sizeof(NodeType) * getMaxEntries();
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline typename T& BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::operator[] (const Key& key)
{
   return (insert(std::make_pair(key, T()))).first->second;
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>   
inline bool BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::insertNoGrow(const ValueType& value, InsertResult& results)
{
   if (!mpTable)
      return false;
      
   const int root = mHasher(value.first) & mRootMask;
   if (mpTable[root].next == root)
   {
      mLowestRoot = Math::Min(mLowestRoot, root);
      mpTable[root].next = INVALID_INDEX;
      results = createEntry(root, root, value);
      return true;
   }

   int cur = root;

   do
   {
      if (mEquals(mpTable[cur].first, value.first))
      {
         results = std::make_pair(iterator(this, root, cur), false);
         return true;
      }
      
      cur = mpTable[cur].next;
   } while (cur != INVALID_INDEX);

   cur = freeEntry();
   if (INVALID_INDEX == cur)
      return false;

   mpTable[cur].next = mpTable[root].next;
   mpTable[root].next = cur;
   mLowestRoot = Math::Min(mLowestRoot, root);
   results = createEntry(root, cur, value);
   return true;
}

// Returned bool will be true on successful insertion. 
// False indicates an already existing key.
// The iterator points to the existing/new entry.
// std::make_pair() is typically used to create ValueType.
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline typename BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::InsertResult BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::insert(const ValueType& value)
{
   InsertResult results;
   
   bool succeeded = insertNoGrow(value, results);
   if (succeeded)
      return results;
      
   grow();
   
   succeeded = insertNoGrow(value, results);
   
   BDEBUG_ASSERT(succeeded);
   
   return results;
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline typename BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::InsertResult BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::insert(const Key& key, const T& value)
{
   return insert(std::make_pair(key, value));
}

// index returned from iterator's index() method
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline typename BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::ValueType& BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::get(IndexType index) 
{
   debugRangeCheck(index, tableSize());
   return mpTable[index];
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline typename const BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::ValueType& BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::get(IndexType index) const
{
   debugRangeCheck(index, tableSize());
   return mpTable[index];
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline typename BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::const_iterator BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::find(const Key& key) const
{
   return findConst(key);
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline typename BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::iterator BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::find(const Key& key) 
{
   const_iterator it(findConst(key));
   return iterator(this, it.mRootIndex, it.mIndex);
}

// invalidates all iterators/indices!
// true if found
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline bool BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::erase(const Key& key)
{
   if (!mpTable)
      return false;
   int cur = mHasher(key) & mRootMask;
   if (mpTable[cur].next == cur)
      return false;
   int prev = INVALID_INDEX;
   do
   {
      if (mEquals(mpTable[cur].first, key))
      {
         destroyEntry(prev, cur);
         return true;
      }
      prev = cur;
      cur = mpTable[cur].next;
   } while (cur != INVALID_INDEX);
   return false;
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline bool BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::erase(const iterator& it)
{
   return erase(it->first);
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline typename BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::iterator BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::begin(void) 
{
   if (empty())
      return end();
   const int first = findFirst();
   BDEBUG_ASSERT(first < mRootSize);
   return iterator(this, first, first);
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline typename BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::const_iterator BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::begin(void) const
{
   if (empty())
      return end();
   const int first = findFirst();
   BDEBUG_ASSERT(first < mRootSize);
   return const_iterator(this, first, first);
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline typename BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::iterator BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::end(void) 
{
   return iterator(this, mRootSize, mRootSize);
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline typename BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::const_iterator BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::end(void) const
{
   return const_iterator(this, mRootSize, mRootSize);
}

template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
inline void BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::swap(HashMapType& other)
{
   std::swap(mpTable, other.mpTable);
   std::swap(mRootSize, other.mRootSize);
   std::swap(mRootMask, other.mRootMask);
   std::swap(mMaxEntries, other.mMaxEntries);
   std::swap(mNumEntries, other.mNumEntries);
   std::swap(mNextFree, other.mNextFree);
   std::swap(mFreeListHead, other.mFreeListHead);
   std::swap(mLowestRoot, other.mLowestRoot);
   std::swap(mHasher, other.mHasher);
   std::swap(mEquals, other.mEquals);
   std::swap(mAlloc, other.mAlloc);
}
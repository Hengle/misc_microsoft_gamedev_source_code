// File: hashMapTypes.inl

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator> class BHashMap;
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator> class BHMIterator;
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator> class BHMConstIterator;

//------------------------------------------------------------------------------
// struct BHMNode
//------------------------------------------------------------------------------
template<class Key, class T> 
struct BHMNode : std::pair<const Key, T>
{
   int next;
};

//------------------------------------------------------------------------------
// class BHMTypes
//------------------------------------------------------------------------------
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
struct BHMTypes
{
   enum { INVALID_INDEX = -1 };

   typedef Key                           KeyType;
   typedef T                             ReferentType;
   typedef BHasher                       HasherType;
   typedef BEquals                       EqualsType;
   typedef std::pair<const Key, T>       ValueType;
   typedef BHMNode<const Key, T>         NodeType;
   typedef int                           IndexType;
   typedef BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator> HashMapType;
};

//------------------------------------------------------------------------------
// class BHMIteratorBase
//------------------------------------------------------------------------------
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
class BHMIteratorBase : public BHMTypes<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>
{
public:

protected:
   const HashMapType* mpHashMap;
   int mRootIndex, mIndex; 

   BHMIteratorBase() : mpHashMap(NULL), mRootIndex(INVALID_INDEX), mIndex(INVALID_INDEX) { }
   BHMIteratorBase(const HashMapType* Phash_map, int root_index, int index) :
   mpHashMap(Phash_map), mRootIndex(root_index), mIndex(index) { }
   BHMIteratorBase(const BHMIteratorBase& b) :
   mpHashMap(b.mpHashMap), mRootIndex(b.mRootIndex), mIndex(b.mIndex) { }

   BHMIteratorBase& operator= (const BHMIteratorBase& b)
   {
      mpHashMap = b.mpHashMap;
      mRootIndex = b.mRootIndex;
      mIndex = b.mIndex;
      return *this;
   }

   bool operator== (const BHMIteratorBase& b) const
   {
      return (mpHashMap == b.mpHashMap) && (mRootIndex == b.mRootIndex) && (mIndex == b.mIndex);
   }

   void probe(void) 
   {
      if (mRootIndex >= mpHashMap->mRootSize)
         return;

      int next = mpHashMap->mpTable[mIndex].next;
      if (next != INVALID_INDEX)
      {
         mIndex = next;
         return;
      }

      while (++mRootIndex < mpHashMap->mRootSize)
         if (mpHashMap->mpTable[mRootIndex].next != mRootIndex)
            break;

      mIndex = mRootIndex;
   }

public:
   IndexType index(void) const
   {
      BDEBUG_ASSERT(mpHashMap);
      debugRangeCheck(mRootIndex, mpHashMap->mRootSize);
      debugRangeCheck(mIndex, mpHashMap->tableSize());
      return mIndex;
   }
};

//------------------------------------------------------------------------------
// class BHMIterator
//------------------------------------------------------------------------------
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
class BHMIterator : public BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>
{
private:
   friend class BHMConstIterator<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>;
   friend class BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>;

   ValueType* getCurrent(void) const
   {
      BDEBUG_ASSERT(mpHashMap);
      debugRangeCheck(mRootIndex, mpHashMap->mRootSize);
      debugRangeCheck(mIndex, mpHashMap->tableSize());
      return &mpHashMap->mpTable[mIndex];
   }

public:
   BHMIterator() : BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>() { }
   BHMIterator(const HashMapType* Phash_map, int root_index, int index) : BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>(Phash_map, root_index, index) { }
   BHMIterator(const BHMIterator& it) : BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>(it) { }
   BHMIterator& operator= (const BHMIterator& it) { BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::operator=(it); return *this; }

   BHMIterator operator++(int)  //postincrement
   {
      BHMIterator result(*this);
      ++*this;
      return result;
   }

   BHMIterator& operator++()  //preincrement
   {
      BDEBUG_ASSERT(mpHashMap);
      debugRangeCheck(mRootIndex, mpHashMap->mRootSize);
      debugRangeCheck(mIndex, mpHashMap->tableSize());
      probe();
      return *this;
   }

   ValueType& operator*() const {  return *getCurrent(); }
   ValueType* operator->() const { return getCurrent(); }

   bool operator == (const BHMIterator& b) const {   return BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::operator==(b); }
   bool operator != (const BHMIterator& b) const { return !(*this == b); }
   bool operator == (const BHMConstIterator<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>& b) const {   return BHMIteratorBase<Key, T, BHasher, UseConstructorDestructor, Allocator, BEquals>::operator==(b); }   
   bool operator != (const BHMConstIterator<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>& b) const { return !(*this == b); }
};

//------------------------------------------------------------------------------
// class BHMConstIterator
//------------------------------------------------------------------------------
template <class Key, class T, class BHasher, class BEquals, bool UseConstructorDestructor, class Allocator>
class BHMConstIterator : public BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>
{
private:
   friend class BHMIterator<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>;
   friend class BHashMap<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>;

   const ValueType* getCurrent(void) const
   {
      BDEBUG_ASSERT(mpHashMap);
      debugRangeCheck(mRootIndex, mpHashMap->mRootSize);
      debugRangeCheck(mIndex, mpHashMap->tableSize());
      return &mpHashMap->mpTable[mIndex];
   }

public:
   BHMConstIterator() : BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>() { }
   BHMConstIterator(const HashMapType* Phash_map, int root_index, int index) : BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>(Phash_map, root_index, index) { }
   BHMConstIterator(const BHMConstIterator& it) : BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>(it) { }
   BHMConstIterator& operator= (const BHMConstIterator& it) { BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::operator=(it); return *this; }
   BHMConstIterator(const BHMIterator<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>& it) : BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>(it) { }
   BHMConstIterator& operator= (const BHMIterator<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>& it) { BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::operator=(it); return *this; }

   BHMConstIterator operator++(int)  //postincrement
   {
      BHMConstIterator result(*this);
      ++*this;
      return result;
   }

   BHMConstIterator& operator++()  //preincrement
   {
      BDEBUG_ASSERT(mpHashMap);
      debugRangeCheck(mRootIndex, mpHashMap->mRootSize);
      debugRangeCheck(mIndex, mpHashMap->tableSize());
      probe();
      return *this;
   }

   const ValueType& operator*() const {  return *getCurrent(); }
   const ValueType* operator->() const { return getCurrent(); }

   bool operator == (const BHMConstIterator& b) const {   return BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::operator==(b); }
   bool operator != (const BHMConstIterator& b) const { return !(*this == b); }
   bool operator == (const BHMIterator<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>& b) const {   return BHMIteratorBase<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>::operator==(b); }
   bool operator != (const BHMIterator<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>& b) const { return !(*this == b); }
};


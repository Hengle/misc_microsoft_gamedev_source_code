//==============================================================================
//
// File: skiplist.h
//
// Copyright (c) 2008 Ensemble Studios
//
// Paged skip list container class
//
//==============================================================================
#pragma once

#include "skipListTypes.inl"

//------------------------------------------------------------------------------
// class BSkipList
//------------------------------------------------------------------------------
template <class Key, class T = BEmptyStruct, uint N = 4, class LessThan = BLessThan<Key>, class Allocator = BPrimaryFixedHeapAllocator>
class BSkipList : BSkipListTypes<Key, T, N, LessThan, Allocator>
{
private:
   friend class BSkipListIteratorBase  <Key, T, N, LessThan, Allocator>;
   friend class BSkipListIterator      <Key, T, N, LessThan, Allocator>;
   friend class BSkipListConstIterator <Key, T, N, LessThan, Allocator>;
      
public:
   typedef BSkipListIterator     <Key, T, N, LessThan, Allocator> iterator;
   typedef BSkipListConstIterator<Key, T, N, LessThan, Allocator> const_iterator;
   
   typedef std::pair<iterator, bool> InsertResult;
      
private:
   NodeType*         mpHead;
   NodeType*         mpNil;
      
   uint              mMaxListLevel;
   uint              mListLevel;
   uint              mNumItems;
   uint              mNumNodes;
   
   LessThan          mLessThan;

   Allocator         mAlloc;
   
   DWORD             mRandSeed;
   
   enum { cMaxEverListLevel = 28 };
   
   inline void allocHeadNode();
   inline void freeHeadNode();
   inline void init(uint maxListLevel);
   inline void deinit();
   inline void clone(const BSkipList& other);
   inline uint getRandLevel();
   
   inline bool keyLess        (const Key& a, const Key& b) const { return mLessThan(a,  b); }
   inline bool keyLessEqual   (const Key& a, const Key& b) const { return !mLessThan(b,  a); }
   inline bool keyGreater     (const Key& a, const Key& b) const { return mLessThan(b,  a); }
   inline bool keyGreaterEqual(const Key& a, const Key& b) const { return !mLessThan(a,  b); }
   inline bool keyEqual       (const Key& a, const Key& b) const { return !mLessThan(a, b) && !mLessThan(b, a); }
   inline bool keyNotEqual    (const Key& a, const Key& b) const { return !keyEqual(a, b); }
      
public:  
   inline BSkipList(int maxListLevel = 10, const LessThan& lessThanFunctor = LessThan(), const Allocator& alloc = Allocator());
   inline BSkipList(const BSkipList& other);
   inline explicit BSkipList(const Allocator& alloc) : mAlloc(alloc) { init(); }
   
   inline ~BSkipList();
   
   inline BSkipList& operator= (const BSkipList& rhs);
      
   inline const Allocator& getAllocator() const  { return mAlloc; }
   inline       Allocator& getAllocator()        { return mAlloc; }
         
   inline const LessThan& getLessThan() const { return mLessThan; }
   inline       LessThan& getLessThan()       { return mLessThan; }
   
   inline uint getMaxListLevel() const { return mMaxListLevel; }
   inline uint getListLevel() const { return mListLevel; }
   
   inline void setMaxListLevel(uint maxListLevel);
   
   // Erases all items from the container.
   inline void clear();
   
   // Returns the number of items present in the container.
   inline uint getSize() const { return mNumItems; }
   inline bool getEmpty() const { return 0 == mNumItems; }
   
   // Returns the number of allocated nodes in the container.
   inline uint getNumNodes() const { return mNumNodes; }

   // Returns the data object associated with the specified key. Missing keys are automatically inserted.
   inline T& operator[] (const Key& key) { return insert(key).first->second; }
      
   // Returned bool will be true if the key is inserted, or false if the key already exists.
   // The returned iterator will point to the existing, or newly inserted entry.
   // If the key already exists, and allowDuplicates is false, the returned iterator will point to the first instance of the key.
   // If N == 1, existing iterators will not be invalidated.
   inline InsertResult insert(const Key& key, const T& data = T(), bool allowDuplicates = false);
   inline InsertResult insert(const ValueType& v, bool allowDuplicates = false) { return insert(v.first, v.second, allowDuplicates); }

   // If the key exists, the returned iterator will point to the first instance of the key. Otherwise end() is returned.
   inline const_iterator find(const Key& key) const;
   inline iterator find(const Key& key);
      
   // Returns an iterator to the first element with a key value that is equal to or greater than that of a specified key.
   inline const_iterator lowerBound(const Key& key) const;
   inline iterator lowerBound(const Key& key);
      
   // Returns an iterator to the first element with a key having a value that is greater than that of a specified key.
   inline const_iterator upperBound(const Key& key) const;
   inline iterator upperBound(const Key& key);
      
   // This method scans the nodes starting from the head if itemIndex < size/2, otherwise it scans backwards starting from the tail. This may be slow!
   inline const_iterator findByIndex(uint itemIndex) const;
   inline iterator findByIndex(uint itemIndex);
   
   // Deletes the first instance of key.
   // Returns true if found.
   // If N == 1, existing iterators will not be invalidated.
   inline bool erase(const Key& key);
   
   // Deletes the exact item pointed to by the iterator
   // Returns true if found.
   inline bool erase(const iterator& it);
   
   // Returns iterator to start of sorted sequence.
   inline iterator begin();
   inline const_iterator begin() const;
   
   // Returns iterator to the element immediately after the end of the sorted sequence.
   inline iterator end();
   inline const_iterator end() const;
   
   // Quickly swaps one container with another.
   void swap(SkipListType& other);
   
   // Compares two containers.
   bool operator== (const BSkipList& other) const;
   bool operator< (const BSkipList& other) const;
   
   // Returns true if container is valid.
   bool check() const;
};
//------------------------------------------------------------------------------
// Test functions
//------------------------------------------------------------------------------
extern void skipListTest();
#ifndef XBOX
extern void skipListPerfTest();
#endif
//------------------------------------------------------------------------------
#include "skiplist.inl"

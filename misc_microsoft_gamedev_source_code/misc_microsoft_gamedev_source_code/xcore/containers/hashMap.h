//------------------------------------------------------------------------------
// BHashMap.h - STL-like hash map container. 
// Rich Geldreich 
// Copyright (C) 2002 Blue Shift, Inc.
//
// STL-style hashmap class with built-in freelist management.
//
// Template definition:
//
// template <class Key, class T = BEmptyStruct, class BHasher = BHasher<Key>, class BEquals = BEqualTo<Key>, bool UseConstructorDestructor = true, class Allocator = BPrimaryFixedHeapAllocator>
// class BHashMap : BHMTypes<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>
//
// Key is the type that is hashed.
// T is the value associated with the key. If you don't want anything associated with the key, use an empty struct.
// BHasher is the object that hashes keys.
// BEquals is the object that compares keys.
// UseConstructorDestructor is true if you want the objects properly constructed/destructed, otherwise they are treated
// as C-style structs.
//
// Example code:
//
// typedef BHashMap<BString, long> BStringMap;
// BStringMap s;
// s.insert("Cool", 1);
// s.insert("Red", 2);
// s.insert("Blue", 3);
// BStringMap r(s);
// for (BStringMap::const_iterator it = r.begin(); it != r.end(); ++it)
//   printf("%s %u\n", it->first.getPtr(), it->second);
//
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "hash\hash.h"
//------------------------------------------------------------------------------
#include "hashMapTypes.inl"
//------------------------------------------------------------------------------
// class BHashmap
//------------------------------------------------------------------------------
template <class Key, class T = BEmptyStruct, class BHasher = BHasher<Key>, class BEquals = BEqualTo<Key>, bool UseConstructorDestructor = true, class Allocator = BPrimaryFixedHeapAllocator>
class BHashMap : BHMTypes<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>
{
private:
   friend class BHMIteratorBase  <Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>;
   friend class BHMIterator      <Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>;
   friend class BHMConstIterator <Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator>;

public:
   typedef BHMIterator     <Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator> iterator;
   typedef BHMConstIterator<Key, T, BHasher, BEquals, UseConstructorDestructor, Allocator> const_iterator;
   typedef std::pair<iterator, bool> InsertResult;      
            
private:
   NodeType* mpTable;
   
   int mRootSize;
   int mRootMask;
   int mMaxEntries;
   int mNumEntries;
   
   int mNextFree;        // next free contiguous entry in table
   int mFreeListHead;   // next free entry on free list
   
   mutable int mLowestRoot;    // lowest valid root entry

   HasherType mHasher;
   EqualsType mEquals;
   
   Allocator mAlloc;
      
   inline int tableSize(void) const;
   inline bool isFull(void) const;
   inline int freeEntry(void);
   inline void freeEntry(int i);
   inline void destructEntry(int i);
   inline void constructEntry(int i, const Key& key, const T& val);
   inline InsertResult createEntry(int root, int cur, const ValueType& value);
   inline void destroyEntry(int prev, int cur);
   inline int findFirst(void) const;
   inline void grow(void);
   inline void init(int maxEntries = 0, int rootSize = INT_MAX);
   inline void destroy(void);
   // Returns false if one or more objects could not be inserted.
   inline bool clone(const BHashMap& other);
   inline const_iterator findConst(const Key& key) const;
            
public:  
   inline BHashMap(int maxEntries = 0, int rootSize = INT_MAX, const HasherType& hashFunctor = HasherType(), const EqualsType& equalFunctor = EqualsType(), const Allocator& alloc = Allocator());
   inline BHashMap(const BHashMap& other);
   inline explicit BHashMap(const Allocator& alloc) : mAlloc(alloc), mpTable(NULL) { init(); }
         
   inline ~BHashMap();
   
   inline BHashMap& operator= (const BHashMap& rhs);
   
   inline Allocator& getAllocator(void) { return mAlloc; }

   // This erases all objects from the container, but does not free the container's table allocation or change its maximum current size.
   inline void clear();
   
   // This erases all objects in the container, then changes the container's maximum size. 
   // Specify 0 for maxEntries to force the container to delete the table.
   inline void setMaxEntries(int maxEntries, int rootSize = INT_MAX);
   
   // Returns the maximum number of objects that can be added to the container before it must grow.
   inline int getMaxEntries(void) const;

   // Returns the current number of objects present in the container.
   inline int size(void) const;
   inline int getSize(void) const;
   
   // Returns true if the container is empty.
   inline bool empty(void) const;
   inline bool getEmpty(void) const;
   
   // Returns the size of the allocated hash table in bytes.
   inline uint getAllocationSize(void) const;

   // Returns the data object associated with key. If key does not exist, a new entry will be inserted.
   inline T& operator[] (const Key& key);
      
   // Inserts a new entry into the container.      
   inline bool insertNoGrow(const ValueType& value, InsertResult& results);

   // Inserts a new entry into the container.
   // Returned bool will be true on successful insertion. 
   // False indicates an already existing key.
   // The iterator points to the existing/new entry.
   // std::make_pair() is typically used to create ValueType.
   inline InsertResult insert(const ValueType& value);
   inline InsertResult insert(const Key& key, const T& value = T());

   // The index value is returned from iterator's index() method.
   inline ValueType& get(IndexType index);
   inline const ValueType& get(IndexType index) const;

   // Returns iterator to entry with key, or end() if not present.
   inline const_iterator find(const Key& key) const;
   inline iterator find(const Key& key);
   
   // invalidates all iterators/indices!
   // true if found
   inline bool erase(const Key& key);
   inline bool erase(const iterator& it);

   // Returns iterator to start of sequence.
   inline iterator begin(void);
   inline const_iterator begin(void) const;
   
   // Returns iterator to the element immediately after the end of the sequence.
   inline iterator end(void);
   inline const_iterator end(void) const;
   
   // Quickly swaps two containers.
   inline void swap(HashMapType& other);
};
//------------------------------------------------------------------------------
#include "hashMap.inl"

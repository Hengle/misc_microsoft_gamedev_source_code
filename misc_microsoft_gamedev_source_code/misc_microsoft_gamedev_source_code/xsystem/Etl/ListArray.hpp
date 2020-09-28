
#pragma once

#include <Etl/Pair.hpp>
#include <Etl/Support.hpp>
#include <Etl/Array.hpp>

namespace Etl
{

//! Maintains an array that acts as a list that is
//! always sorted.
template <class T, class Cmp = CompareKeyDefault<T> >
class ListArray
{
public:
   ListArray(long lInitialCapacity);

   //! returns the array
   T* getAsArray(void);
   //! add data to the last
   inline bool add(const T data, bool sort = true);
   //! erase an index from the list
   inline void erase(long lIndex, bool bKeepOrder = true);
   //! get an item
   inline T&   get(long lIndex);
   //! returns the number of items in the list
   inline long size(void) const;
   //! returns the currenty capacity of the list
   inline long capacity(void) const;
   //! search for a value and return the index
   inline long search(const T data);
   //! sort the list
   inline void sort(void);
   //! sets the size back to zero
   inline void clear(void);
   //! returns true if the list is sorted
   inline bool isSorted(void) const;

protected:

   Etl::Array<T,Cmp> m_array;
   long m_lSize;
   bool m_bSorted;
};

template <class T, class Cmp>
ListArray<T,Cmp>::ListArray(long lInitialCapacity) :
   m_array(lInitialCapacity),
   m_lSize(0),
   m_bSorted(false)
{
}

template <class T, class Cmp>
inline long ListArray<T,Cmp>::size(void) const
{
   return m_lSize;
}

template <class T, class Cmp>
inline T* ListArray<T,Cmp>::getAsArray(void)
{
   return m_array.get();
}

template <class T, class Cmp>
inline bool ListArray<T,Cmp>::add(const T data, bool bSort)
{
   if (m_lSize >= m_array.capacity())
      return false;

   m_array[m_lSize++] = data;

   if (bSort && m_lSize > 0)
      m_array.sort(0, m_lSize - 1);

   m_bSorted = bSort;

   //return m_array.binarySearch(data, m_lSize - 1);
   return true;
}

template <class T, class Cmp>
inline T& ListArray<T,Cmp>::get(long lIndex)
{
   return m_array[lIndex];
}

template <class T, class Cmp>
inline void ListArray<T,Cmp>::erase(long lIndex, bool bKeepOrder)
{
   BASSERT(lIndex < m_lSize);

   if (lIndex < 0 || lIndex >= m_lSize)
      return;

   if (bKeepOrder)
      m_array.pack(lIndex, m_lSize - 1);
   else
   {
      m_bSorted = false;
      m_array[lIndex] = m_array[m_lSize - 1];
   }
   m_lSize--;
}

template <class T, class Cmp>
inline long ListArray<T,Cmp>::search(const T data)
{
   if (m_bSorted)
      return m_array.binarySearch(data, m_lSize - 1);
   else
   {
      for (long lIndex = 0; lIndex < m_lSize; lIndex++)
      {
         Cmp cmp;
         if (cmp(m_array[lIndex], data) == 0)
            return lIndex;
      }
   }
   return -1;
}

template <class T, class Cmp>
inline long ListArray<T,Cmp>::capacity(void) const
{
   return m_array.capacity();
}

template <class T, class Cmp>
inline void ListArray<T,Cmp>::sort(void)
{
   m_bSorted = true;
   m_array.sort(0, m_lSize - 1); 
}

template <class T, class Cmp>
inline void ListArray<T,Cmp>::clear(void)
{
   m_lSize = 0;
}

template <class T, class Cmp>
inline bool ListArray<T,Cmp>::isSorted(void) const
{
   return m_bSorted;
}

#define SKLA_BASE ListArray< Pair<Key, T>*, PtrPairKeyCompareDefault<Key, T, Cmp> >

//! Maintains an array that acts as a list that is
//! always sorted and is searchable by a key.
template <class Key, class T, class Cmp = CompareKeyDefault<Key> >
class KeyListArray : public SKLA_BASE
{
public:
   KeyListArray(long lInitialCapacity) : SKLA_BASE(lInitialCapacity) 
   {
      m_array.setAll(NULL);
   }

   ~KeyListArray()
   {
      for (long i = 0; i < m_lSize; ++i)
      {
         delete m_array[i];
         m_array[i] = 0;
      }
   }

   inline long add(const Key key, const T t, bool sort = true)
   {
      Pair<Key,T>* pPair = new Pair<Key,T>(key, t);
      if (SKLA_BASE::add(pPair, sort) == false)
      {
         delete pPair;
         return false;
      }
      return true;
   }

   inline long searchForKey(const Key key)
   {
      Pair<Key,T> pair(key);
      return SKLA_BASE::search(&pair);
   }

   inline long binarySearchForKey(const Key key)
   {
      Pair<Key,T> pair(key);
      return m_array.binarySearch(&pair, m_lSize - 1);
   }

   inline void erase(long lIndex, bool bKeepOrder = true) 
   {
      Pair<Key,T> *pair = m_array[lIndex];
      SKLA_BASE::erase(lIndex, bKeepOrder);
      delete pair;
   }

   inline bool eraseKey(const Key key, bool bKeepOrder = true)
   {
      long lIndex = searchForKey(key);
      if (lIndex < 0)
         return false;

      erase(lIndex, bKeepOrder);
      return true;
   }

   inline T& get(long lIndex)
   {
      return SKLA_BASE::get(lIndex)->second;
   }

   inline Key& getKey(long lIndex)
   {
      return SKLA_BASE::get(lIndex)->first;
   }
};

}




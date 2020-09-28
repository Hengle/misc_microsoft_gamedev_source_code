//==============================================================================
// Array.hpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#pragma once

#include <Etl/Pair.hpp>
#include <Etl/Support.hpp>

#pragma warning(disable : 4786)

namespace Etl
{

template <class T, class Cmp> class Array;
//
// Performs a binary search on a sorted array
//
template <class T, class Cmp>
inline long binarySearch(const Etl::Array<T, Cmp>& a, const T& t, long lBegin, long lEnd)
{
   Cmp cmp;

   long lLow = lBegin;
   long lHigh = lEnd;
   long lPivot = 0;
   while (lLow <= lHigh)
   {
      lPivot = (lLow + lHigh) / 2;
      if (cmp(t, a[lPivot]) < 0)
         lHigh = lPivot - 1;
      else if (cmp(t, a[lPivot]) > 0)
         lLow = lPivot + 1;
      else
         return lPivot;
   }

   return -1;
}

template <class T, class Cmp>
inline void quickSort(Etl::Array<T, Cmp>& a, long lLow, long lHigh)
{
#ifdef _DEBUG
   BASSERT(lLow >= 0);
   BASSERT(lHigh < a.capacity());
#endif

   Cmp cmp;

   if(lHigh > lLow)
   {
      long lLeft = lLow;
      long lRight = lHigh;
      T pivot = a[lLow];

      while(lRight >= lLeft)
      {
         while(cmp(a[lLeft], pivot) < 0)  lLeft++;
         while(cmp(a[lRight], pivot) > 0) lRight--;

         if(lLeft > lRight) break;

         Swap<T>(a[lLeft++], a[lRight--]);
      }
      quickSort(a, lLow, lRight);
      quickSort(a, lLeft, lHigh);
   }
}

template <class T, class Cmp = CompareKeyDefault<T> >
class Array
{
public:
   Array(long capacity);             // an array must always have a size
   Array(const Etl::Array<T, Cmp>&);
   ~Array();
   inline void grow(long lCapacity); // grow or shrink array to lCapacity number of elements
   inline long capacity(void) const; // returns the number of elements in the array
   inline long size(void) const;     // returns the number of bytes in the array

   inline void copy(const Etl::Array<T, Cmp>&);

   inline T* get(void) const;
   inline T& get(long lIndex) const;
   inline void set(long lIndex, const T& t);
   inline void setAll(const T val);

   inline void sort(void);
   inline void sort(long lFirstIndex, long lLastIndex);
   inline long search(const T& t) const;
   inline long binarySearch(const T& t) const;
   inline long binarySearch(const T& t, long lLastIndex) const;
   inline void pack(long lOpenIndex, long lLastIndex);

   inline bool operator == (const Etl::Array<T, Cmp>& a) const;
   inline bool operator != (const Etl::Array<T, Cmp>& a) const;
   inline void operator += (const Etl::Array<T, Cmp>&);

   inline T& operator [] (short);
   inline T& operator [] (int);
   inline T& operator [] (long);
   inline T& operator [] (unsigned short);
   inline T& operator [] (unsigned int);
   inline T& operator [] (unsigned long);

   inline operator T* () { return m_pArray; }
   inline operator const T* () const { return m_pArray; }
   inline Array<T, Cmp> operator = (const Etl::Array<T, Cmp>&); // same as copy

   T* getFirst(void) { return (m_pCurrent = m_pArray;) }
   T* getLast(void)  { return (m_pCurrent = m_pArray+(m_lCapacity-1)); }
   T* getNext(void)  { return (m_pCurrent < m_pArray+(m_lCapacity-1)) ? ++m_pCurrent : NULL; }
   T* getPrev(void)  { return (m_pCurrent > m_pArray) ? --m_pCurrent : NULL; }
   T* getCurrent(void) { return m_pCurrent; }

protected:

   long m_lCapacity;
   T* m_pArray;
   T* m_pCurrent;
};

template <class T, class Cmp>
Array<T, Cmp>::Array(long lCapacity) :
   m_lCapacity(lCapacity),
   m_pCurrent(0),
   m_pArray(0)
{
   m_pArray = new T[m_lCapacity];
}

template <class T, class Cmp>
Array<T, Cmp>::Array(const Array<T, Cmp>& a) :
  m_pArray(0),
  m_pCurrent(0),
  m_lCapacity(0)
{
   this->copy(a);
}

template <class T, class Cmp>
Array<T, Cmp>::~Array()
{
   delete [] m_pArray;
   m_pArray = 0;
}

template <class T, class Cmp>
inline T* Array<T, Cmp>::get(void) const
{
   return m_pArray;
}

template <class T, class Cmp>
inline void Array<T, Cmp>::copy(const Array<T, Cmp>& a)
{
   m_lCapacity = a.m_lCapacity;
   delete [] m_pArray;
   m_pArray = new T[m_lCapacity];
   // don't use memcpy, we might be copying classes
   for (long index = 0; index < m_lCapacity; ++index)
      m_pArray[index] = a.m_pArray[index];
}

template <class T, class Cmp>
inline Array<T, Cmp> Array<T, Cmp>::operator = (const Array<T, Cmp>& a)
{
   this->copy(a);
   return *this;
}

template <class T, class Cmp>
inline long Array<T, Cmp>::search(const T& t) const
{
   Cmp cmp;
   for (long lIndex = 0; lIndex < m_lCapacity; lIndex++)
   {
      if (cmp(m_pArray[lIndex], t) == 0)
         return lIndex;
   }
   return -1;
}

template <class T, class Cmp>
inline long Array<T, Cmp>::binarySearch(const T& t) const
{
   return Etl::binarySearch<T,Cmp>(*this, t, 0, m_lCapacity - 1);
}

template <class T, class Cmp>
inline long Array<T, Cmp>::binarySearch(const T& t, long lLastIndex) const
{
   return Etl::binarySearch<T,Cmp>(*this, t, 0, lLastIndex);
}

template <class T, class Cmp>
inline void Array<T, Cmp>::sort(void)
{
   quickSort<T,Cmp>(*this, 0, m_lCapacity - 1);
}

template <class T, class Cmp>
inline void Array<T, Cmp>::sort(long lFirstIndex, long lLastIndex)
{
#ifdef _DEBUG
   BASSERT(lFirstIndex < m_lCapacity);
   BASSERT(lFirstIndex >= 0);
   BASSERT(lLastIndex < m_lCapacity);
   BASSERT(lLastIndex >= 0);
#endif
   quickSort<T,Cmp>(*this, lFirstIndex, lLastIndex);
}

template <class T, class Cmp>
inline T& Array<T, Cmp>::operator [] (short index)
{
#ifdef _DEBUG
   BASSERT(index < m_lCapacity);
   BASSERT(index >= 0);
#endif
   return m_pArray[index];
}

template <class T, class Cmp>
inline T& Array<T, Cmp>::operator [] (int index)
{
#ifdef _DEBUG
   BASSERT(index < m_lCapacity);
   BASSERT(index >= 0);
#endif
   return m_pArray[index];
}

template <class T, class Cmp>
inline T& Array<T, Cmp>::operator [] (long index)
{
#ifdef _DEBUG
   BASSERT(index < m_lCapacity);
   BASSERT(index >= 0);
#endif
   return m_pArray[index];
}

template <class T, class Cmp>
inline T& Array<T, Cmp>::operator [] (unsigned short index)
{
#ifdef _DEBUG
   BASSERT(index < m_lCapacity);
#endif
   return m_pArray[index];
}

template <class T, class Cmp>
inline T& Array<T, Cmp>::operator [] (unsigned int index)
{
#ifdef _DEBUG
   BASSERT(index < m_lCapacity);
#endif
   return m_pArray[index];
}

template <class T, class Cmp>
inline T& Array<T, Cmp>::operator [] (unsigned long index)
{
#ifdef _DEBUG
   BASSERT(index < m_lCapacity);
#endif
   return m_pArray[index];
}

template <class T, class Cmp>
inline T& Array<T, Cmp>::get(long lIndex) const
{
#ifdef _DEBUG
   BASSERT(index < m_lCapacity);
   BASSERT(index >= 0);
#endif
   return m_pArray[lIndex];
}

template <class T, class Cmp>
inline void Array<T, Cmp>::set(long lIndex, const T& t)
{
#ifdef _DEBUG
   BASSERT(index < m_lCapacity);
   BASSERT(index >= 0);
#endif
   m_pArray[lIndex] = t;
}


template <class T, class Cmp>
inline void Array<T, Cmp>::grow(long lNewCapacity)
{
   T* pNewArray = new T[lNewCapacity];

   long lCopyLength = (lNewCapacity <= m_lCapacity) ? lNewCapacity : m_lCapacity;

   for (long lIndex = 0; lIndex < lCopyLength; ++lIndex)
      pNewArray[lIndex] = m_pArray[lIndex];

   delete [] m_pArray;
   m_pArray = pNewArray;
   m_lCapacity = lNewCapacity;
}

template <class T, class Cmp>
inline long Array<T, Cmp>::capacity(void) const
{
   return m_lCapacity;
}

template <class T, class Cmp>
inline long Array<T, Cmp>::size(void) const
{
   return m_lCapacity * sizeof(T);
}

template <class T, class Cmp>
inline void Array<T, Cmp>::setAll(T val)
{
   for (long lIndex = 0; lIndex < m_lCapacity; lIndex++)
      m_pArray[lIndex] = val;
}

template <class T, class Cmp>
inline bool Array<T, Cmp>::operator == (const Array& a) const
{
   if (m_lCapacity != a.m_lCapacity)
      return false;

   for (long index = 0; index < m_lCapacity; index++)
   {
      if (m_pArray[index] != a.m_pArray[index])
         return false;
   }
   return true;
}


template <class T, class Cmp>
inline bool Array<T, Cmp>::operator != (const Array& a) const
{
   if (m_lCapacity != a.m_lCapacity)
      return true;

   for (long index = 0; index < m_lCapacity; index++)
   {
      if (m_pArray[index] != a.m_pArray[index])
         return true;
   }
   return false;
}

template <class T, class Cmp>
inline void Array<T, Cmp>::operator += (const Array& a)
{
   T* pArray = new T[m_lCapacity + a.m_lCapacity];
   for (long p = 0; p < m_lCapacity; ++p)
      pArray[p] = m_pArray[p];

   for (;(p - a.m_lCapacity) < a.m_lCapacity; ++p)
      pArray[p] = a.m_pArray[p - a.m_lCapacity];

   m_lCapacity += a.m_lCapacity;

   delete [] m_pArray;
   m_pArray = 0;
   m_pArray = pArray;
}

template <class T, class Cmp>
inline void Array<T,Cmp>::pack(long lOpenIndex, long lLastIndex)
{
#ifdef _DEBUG
   BASSERT(lOpenIndex < m_lCapacity);
   BASSERT(lOpenIndex >= 0);
   BASSERT(lLastIndex < m_lCapacity);
   BASSERT(lLastIndex >= 0);
   BASSERT(lOpenIndex <= lLastIndex);
#endif
   for (long lIndex = lOpenIndex; lIndex < lLastIndex; lIndex++)
   {
      (*this)[lIndex] = (*this)[lIndex + 1];
   }
}

typedef Array<unsigned char> ByteArray;

}


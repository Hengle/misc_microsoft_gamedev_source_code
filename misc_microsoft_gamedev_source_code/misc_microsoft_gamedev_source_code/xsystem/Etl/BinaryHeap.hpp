#pragma once

#include <Etl/Support.hpp>
#include <Etl/Iterator.hpp>
#include <Etl/Array.hpp>

namespace Etl
{
template <class T, class Cmp = CompareKeyDefault<T> >
class BinaryHeap
{
public:

   enum { cDefaultLength = 128 };

   BinaryHeap(const T MinVal, long lInitialLength = cDefaultLength);

   void insert(const T);
   void insertOutOfOrder(const T);

   T    getMin(void);
   void deleteMin(void);
   void fixHeap(void);

   // n search time
   bool exists(const T&) const;

   long length(void)  const  { return m_lCurrentLength;   }
   long capacity(void) const { return m_array.capacity(); }

   const T get(long lIndex) const;

   bool isEmpty(void) const;

private:

   void resize(long lSize);
   void checkAndResize(void);
   void heapify(long lIndex);

   bool          m_fInOrder;
   long          m_lCurrentLength;
   Array<T, Cmp> m_array;
};

template <class T, class Cmp>
BinaryHeap<T,Cmp>::BinaryHeap(const T MinValue, long lInitialSize) :
   m_array(lInitialSize+1),
   m_lCurrentLength(0),
   m_fInOrder(true)
{
   m_array[0] = MinValue;
}

template <class T, class Cmp>
T BinaryHeap<T,Cmp>::getMin(void)
{
   BASSERT(!isEmpty());

   if (m_fInOrder == false)
      fixHeap();

   return m_array[1];
}

template <class T, class Cmp>
inline const T BinaryHeap<T,Cmp>::get(long lIndex) const
{
   return m_array[lIndex];
}

template <class T, class Cmp>
inline void BinaryHeap<T,Cmp>::resize(long lNewSize)
{
   m_array.grow(lNewSize);
}

template <class T, class Cmp>
inline void BinaryHeap<T,Cmp>::deleteMin(void)
{
    m_array[1] = m_array[m_lCurrentLength--];
    heapify(1);
}

template <class T, class Cmp>
inline void BinaryHeap<T,Cmp>::checkAndResize(void)
{
   if (m_lCurrentLength == m_array.capacity())
      resize((m_array.capacity() - 1) * 2);
}

template <class T, class Cmp>
inline void BinaryHeap<T,Cmp>::insert(const T x)
{
   Cmp cmp;
   if (m_fInOrder == false)
   {
      insertOutOfOrder(x);
   }
   else
   {
      long lIndex = ++m_lCurrentLength;
      checkAndResize();
      for (;cmp(x, m_array[lIndex / 2]) < 0; lIndex /= 2)
      {
         m_array[lIndex] = m_array[lIndex / 2];
      }
      m_array[lIndex] = x;
   }
}

template <class T, class Cmp>
inline void BinaryHeap<T,Cmp>::insertOutOfOrder(const T x)
{
   Cmp cmp;
   checkAndResize();
   m_array[++m_lCurrentLength] = x;
   if (cmp(x, m_array[m_lCurrentLength / 2]) < 0)
      m_fInOrder = false;
}

template <class T, class Cmp>
inline void BinaryHeap<T,Cmp>::heapify(long lIndex)
{
    T Tmp = m_array[lIndex];
    long lChild;
    Cmp cmp;

    for(;lIndex * 2 <= m_lCurrentLength; lIndex = lChild)
    {
        lChild = lIndex * 2;
        if (lChild != m_lCurrentLength && cmp(m_array[lChild+1], m_array[lChild]) < 0)
            lChild++;
        if (cmp(m_array[lChild], Tmp) < 0)
            m_array[lIndex] = m_array[lChild];
        else
            break;
    }
    m_array[lIndex] = Tmp;
}

template <class T, class Cmp>
inline void BinaryHeap<T,Cmp>::fixHeap(void)
{
    for( long i = m_lCurrentLength / 2; i > 0; --i)
    {
        heapify(i);
    }
    m_fInOrder = true;
}

template <class T, class Cmp>
inline bool BinaryHeap<T,Cmp>::isEmpty(void) const
{
   return (m_lCurrentLength == 0) ? true : false;
}

template <class T, class Cmp>
inline bool BinaryHeap<T, Cmp>::exists(const T& v) const
{
   if (m_array.search(v) < 0)
      return false;
   
   return true;
}

}
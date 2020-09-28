
#pragma once
#include <Etl/Pair.hpp>
#include <Etl/Support.hpp>
#include <Etl/BinaryHeap.hpp>

namespace Etl
{

template <class Key, class T, class Cmp>
struct MinPriorityCompareKeyDefault
{
   inline long operator() (const Pair<Key,T>& a, const Pair<Key,T>& b) const
   {
      Cmp cmp;
      // first is the key
      if (cmp(a.first, b.first) < 0) return -1;
      if (cmp(a.first ,b.first) > 0) return 1;
      return 0;
   }
};

template <class Key, class T, class Cmp = CompareKeyDefault<Key> >
class MinPriorityQueue
{
public:

   MinPriorityQueue(const Key minPriorityValue, long lSize);

   // push --
   //   push a value onto the priority queue with lPriority
   void push(const Key& priority, const T& t);

   // peek --
   //   peek at the min value
   T peek(void);

   // peekKey --
   //   peek at the min key
   Key peekKey(void);

   const T peek(long lIndex) const;

   const Key peekKey(long lIndex) const;

   // pop --
   //   pop the min priority value off the priority queue
   void pop(void);

   // count --
   //   number of items in the queue
   long count(void) const;

private:

   BinaryHeap< Pair<Key, T>, MinPriorityCompareKeyDefault<Key, T, Cmp> > m_heap;

};


template <class Key, class T, class Cmp>
MinPriorityQueue<Key, T, Cmp>::MinPriorityQueue(const Key minValue, long lSize) : 
   m_heap(Pair<Key,T>(minValue), lSize)
{
}

template <class Key, class T, class Cmp>
inline const Key MinPriorityQueue<Key, T, Cmp>::peekKey(long lIndex) const
{
   return m_heap.get(lIndex).first;
}

template <class Key, class T, class Cmp>
inline const T MinPriorityQueue<Key, T, Cmp>::peek(long lIndex) const
{
   return m_heap.get(lIndex).second;
}

template <class Key, class T, class Cmp>
inline void MinPriorityQueue<Key, T, Cmp>::push(const Key& priority, const T& data)
{
   m_heap.insert(Pair<Key, T>(priority, data));
}

template <class Key, class T, class Cmp>
inline Key MinPriorityQueue<Key, T, Cmp>::peekKey(void)
{
   return m_heap.getMin().first;
}

template <class Key, class T, class Cmp>
inline T MinPriorityQueue<Key, T, Cmp>::peek(void)
{
   return m_heap.getMin().second;
}

template <class Key, class T, class Cmp>
inline void MinPriorityQueue<Key, T, Cmp>::pop(void)
{
   m_heap.deleteMin();
}

template <class Key, class T, class Cmp>
inline long MinPriorityQueue<Key, T, Cmp>::count(void) const
{
   return m_heap.length();
}

}
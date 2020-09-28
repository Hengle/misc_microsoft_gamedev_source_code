//==============================================================================
// Iterator.hpp
//
// Copyright (c) 2000-2002, Ensemble Studios
//==============================================================================

#ifndef __Iterator__
#define __Iterator__

#pragma warning(disable : 4786)

namespace Etl
{

template <class T, class Bucket, class Container>
class ForwardIterator
{
public:
   ForwardIterator() {}
   ForwardIterator(const ForwardIterator<T, Bucket, Container>& it) : m_it(it.m_it) {}
   ForwardIterator(const Bucket& b) : m_it(b) {}

   inline T get(void) const { return Container::itget(m_it); }
   inline T operator * () const { return Container::itget(m_it); }
   inline T operator -> (void) { return Container::itget(m_it); }
   inline bool operator != (const ForwardIterator& it) const { return m_it != it.m_it; }
   inline bool operator == (const ForwardIterator& it) const { return m_it == it.m_it; }

   inline ForwardIterator<T, Bucket, Container> operator =
   (const ForwardIterator<T, Bucket, Container>& it) { m_it = it.m_it; return *this; }

   inline ForwardIterator<T, Bucket, Container> operator ++ (void)
      { Container::forward(m_it); return *this; }
   inline ForwardIterator<T, Bucket, Container> operator ++ (int)
      { Bucket it = m_it; Container::forward(m_it); return Container::itget(it); }


   //friend class Bucket;
   //friend Container;

   inline Bucket& GetBucket(void) { return m_it; }

   Bucket m_it;

};

template <class T, class Bucket, class Container>
class ReverseIterator
{
public:
   ReverseIterator();
   // todo: if needed
};

}
#endif

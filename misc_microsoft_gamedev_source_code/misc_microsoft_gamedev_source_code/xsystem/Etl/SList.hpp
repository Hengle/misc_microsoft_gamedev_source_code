//==============================================================================
// SList.hpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef __SList__
#define __SList__

#include <Etl/Support.hpp>
#include <Etl/Iterator.hpp>

namespace Etl
{

template <class T> struct SListNode;
template <class T> class SListIterator;

template <class T>
class SList
{
public:
   SList();
   virtual ~SList();

//   typedef SListIterator<T> Iterator;
   typedef ForwardIterator< T, SListNode<T>*, SList<T> > Iterator;

   Iterator add(const T&);
   Iterator addAtEnd( const T&);
   void insert(const T&, Iterator& it);
   Iterator erase(Iterator& it);
   unsigned long count(void) const;
   void clear(void);
   Iterator search(const T&);

   inline Iterator begin(void) { return Iterator(m_pHead); }
   inline Iterator end(void)   { return Iterator((SListNode<T>*)0); }


private:

   SListNode<T>* m_pHead;
   unsigned long m_ulCount;

   //friend class ForwardIterator< T, SListNode<T>*, SList<T> >;
   friend Iterator;
   static inline void forward(SListNode<T>*& it) { it = it->m_pNext; }
   static inline T& itget(SListNode<T>* it) { return it->data; }
};

template <class T>
SList<T>::SList() : m_pHead(0), m_ulCount(0)
{
}

template <class T>
SList<T>::~SList()
{
   clear();
}

template <class T>
inline unsigned long SList<T>::count(void) const
{
   return m_ulCount;
}

template <class T> typename
SList<T>::Iterator SList<T>::add(const T& t)
{
   // add the new node to the head of the list.
   //
   SListNode<T>* pNewNode = new SListNode<T>(t);
   pNewNode->m_pNext = m_pHead;
   m_pHead = pNewNode;
   ++m_ulCount;
   return Iterator(pNewNode);
}

template <class T> typename
SList<T>::Iterator SList<T>::addAtEnd(const T& t)
{
   if (m_pHead == 0)
      return add(t);

   // add the new node to the end of the list.
   //
   SListNode<T>* pNewNode = new SListNode<T>(t);
   
   SListNode<T>* pNode = m_pHead;
   while (pNode->m_pNext != 0)
      pNode = pNode->m_pNext;
      
   pNode->m_pNext = pNewNode;   
   ++m_ulCount;
   return Iterator(pNewNode);
}

template <class T>
void SList<T>::insert(const T& t, Iterator& it)
{
   SListNode<T>* pNewNode = new SListNode<T>(t);

   // are we inserting before the first node?
   // this also catches it == end and list being empty.
   //
   if (it.m_it == m_pHead)
   {
      pNewNode->m_pNext = m_pHead;
      m_pHead = pNewNode;
      ++m_ulCount;
   }
   // nope, find the node referenced by 'it'. check
   // to ensure we don't walk off the end of the list.
   //
   else
   {
      SListNode<T>* pNode = m_pHead;
      while (pNode && pNode->m_pNext != it.m_it)
         pNode = pNode->m_pNext;

      if (pNode)
      {
         pNewNode->m_pNext = pNode->m_pNext;
         pNode->m_pNext = pNewNode;
         ++m_ulCount;
      }
   }
}

template <class T> typename
SList<T>::Iterator SList<T>::erase(Iterator& it)
{
   // erasing nothing or empty list just
   // return the end iterator.
   //
   if ( it.m_it == 0 || m_pHead == 0)
      return end();

   SListNode<T>* pNode = m_pHead;

   // erasing the head?
   //
   if (it.m_it == pNode)
   {
      m_pHead = pNode->m_pNext;
      delete pNode;
      --m_ulCount;
      return Iterator(m_pHead);
   }
   else
   {
      while (pNode)
      {
         if (pNode->m_pNext == it.m_it)
         {
            SListNode<T>* pTmp = pNode->m_pNext;
            pNode->m_pNext = pTmp->m_pNext;
            delete pTmp;
            --m_ulCount;
            return Iterator(pNode->m_pNext);
         }

         pNode = pNode->m_pNext;
      }
   }

   // 'it' value doesn't exist in this list
   //   
   return end();
}

template <class T>
void SList<T>::clear(void)
{
	if (m_ulCount > 0)
	{
		while (m_pHead)
		{
			SListNode<T>* pNode = m_pHead;
			m_pHead = m_pHead->m_pNext;
			delete pNode;
		}
		m_ulCount = 0;
	}
}


template <class T> typename
SList<T>::Iterator SList<T>::search(const T& t)
{
   SListNode<T>* pNode = m_pHead;

   while (pNode)
   {
      if (pNode->data == t)
         return Iterator(pNode);

      pNode = pNode->m_pNext;
   }

   return end();
}

template <class T>
struct SListNode
{
   SListNode(const T& t) : data(t), m_pNext(0) {}
   inline operator T () { return data; }
   SListNode* m_pNext;
   T data;
};

}

#endif

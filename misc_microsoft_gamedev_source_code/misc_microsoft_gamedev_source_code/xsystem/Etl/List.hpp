//==============================================================================
// List.hpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================
#ifndef __List__
#define __List__


#define FOR_ALL_FORWARD(list, handle) \
   for (Etl::ListHandle handle = list.getFirst(); handle != 0; handle = list.getNext(handle))

#define FOR_ALL_REVERSE(list, handle) \
   for (Etl::ListHandle handle = list.getLast(); handle != 0; handle = list.getPrev(handle))

namespace Etl
{
template <class T> struct ListNode;

typedef long ListHandle;

template <class T>
class List
{
public:
   List();
   ~List();

   enum { cInvalidHandle = 0 };

   ListHandle insert(ListHandle, const T&);
   ListHandle erase(ListHandle);
   ListHandle pushFront(const T&);
   ListHandle pushBack(const T&);
   void       popFront(void);
   void       popBack(void);

   ListHandle moveToFirst(ListHandle);
   ListHandle moveToLast(ListHandle);

   long       count(void) const;
   void       clear(void);
   ListHandle search(const T&);
   T&         get(ListHandle);
   ListHandle getFirst(void);
   ListHandle getLast(void);
   ListHandle getNext(ListHandle);
   ListHandle getPrev(ListHandle);

private:
   struct ListNode
   {
   public:
      ListNode(const T& t) : data(t), pNext(0), pPrev(0) {}
      T data;  // don't not change order

      ListNode* pNext;
      ListNode* pPrev;
   };

   ListNode* m_pFirst;
   ListNode* m_pLast;

   long m_lCount;

};

template <class T>
List<T>::List() : m_pFirst(0), m_pLast(0), m_lCount(0)
{
}

template <class T>
List<T>::~List()
{
   clear();
}

template <class T>
inline long List<T>::count(void) const
{
   return m_lCount;
}

template <class T>
inline ListHandle List<T>::insert(ListHandle h, const T& t)
{
   ListNode* pNode = reinterpret_cast<ListNode*>(h);
   ListNode* pNewNode = new ListNode(t);
   if (!h || h == cInvalidHandle)
      return cInvalidHandle;
   else
   {
      if (pNode == m_pFirst)
         m_pFirst = pNewNode;
      if (pNode == m_pLast)
         m_pLast = pNewNode;
      pNewNode->pNext = pNode;
      pNewNode->pPrev = pNode->pPrev;
      pNode->pPrev    = pNewNode;
      if (pNewNode->pPrev)
         pNewNode->pPrev->pNext = pNewNode;
   }
   m_lCount++;
   return reinterpret_cast<ListHandle>(pNewNode);
}

template <class T>
inline ListHandle List<T>::pushFront(const T& t)
{
   ListNode* pNewNode = new ListNode(t);
   if (m_pFirst)
   {
      pNewNode->pNext = m_pFirst;
      m_pFirst->pPrev = pNewNode;
      m_pFirst = pNewNode;
   }
   else
      m_pFirst = m_pLast = pNewNode;

   m_lCount++;

   return reinterpret_cast<ListHandle>(pNewNode);
}

template <class T>
inline ListHandle List<T>::pushBack(const T& t)
{
   ListNode* pNewNode = new ListNode(t);
   if (m_pLast)
   {
      pNewNode->pPrev = m_pLast;
      m_pLast->pNext = pNewNode;
      m_pLast = pNewNode;
   }
   else
      m_pFirst = m_pLast = pNewNode;

   m_lCount++;

   return reinterpret_cast<ListHandle>(pNewNode);
}

template <class T>
inline void List<T>::popFront(void)
{
   ListNode* pNode = m_pFirst;
   if (pNode)
   {
      m_pFirst = m_pFirst->pNext;
      if (!m_pFirst)
         m_pLast = 0;
      delete pNode;
      m_lCount--;
   }
}

template <class T>
inline void List<T>::popBack(void)
{
   ListNode* pNode = m_pLast;
   if (pNode)
   {
      m_pLast = m_pLast->pPrev;
      if (!m_pLast)
         m_pFirst = 0;
      delete pNode;
      m_lCount--;
   }
}

template <class T>
inline Etl::ListHandle List<T>::erase(Etl::ListHandle h)
{
   List::ListNode* pNode = reinterpret_cast<List::ListNode*>(h);
   ListHandle hNext = reinterpret_cast<ListHandle>(pNode->pNext);
   if (pNode)
   {
      if (m_pFirst == pNode)
      {
         m_pFirst = m_pFirst->pNext;
         if (m_pFirst)
            m_pFirst->pPrev = 0;
      }
      if (m_pLast == pNode)
      {
         m_pLast = m_pLast->pPrev;
         if (m_pLast)
            m_pLast->pNext = 0;
      }

      if (pNode->pPrev)
         pNode->pPrev->pNext = pNode->pNext;
      if (pNode->pNext)
         pNode->pNext->pPrev = pNode->pPrev;

      m_lCount--;
      delete pNode;
   }
   else
      return cInvalidHandle;

   return hNext;
}

template <class T>
inline void List<T>::clear(void)
{
   ListNode* pCurrent = m_pFirst;
   while (m_pFirst)
   {
      m_pFirst = m_pFirst->pNext;
      delete pCurrent;
      pCurrent = m_pFirst;
      m_lCount--;
   }
   m_pFirst = 0;
   m_pLast  = 0;
}

template <class T>
inline ListHandle List<T>::search(const T& data)
{
   ListNode* pNode = m_pFirst;

   while (pNode && pNode->data != data)
      pNode = pNode->pNext;

   return reinterpret_cast<ListHandle>(pNode);
}

template <class T>
T& List<T>::get(ListHandle h)
{
   ListNode* pNode = reinterpret_cast<ListNode*>(h);
   return pNode->data;
}

template <class T>
ListHandle List<T>::getNext(ListHandle h)
{
   ListNode* pNode = reinterpret_cast<ListNode*>(h);
   return reinterpret_cast<ListHandle>(pNode->pNext);
}

template <class T>
ListHandle List<T>::getPrev(ListHandle h)
{
   ListNode* pNode = reinterpret_cast<ListNode*>(h);
   return reinterpret_cast<ListHandle>(pNode->pPrev);
}

template <class T>
ListHandle List<T>::getFirst(void)
{
   return reinterpret_cast<ListHandle>(m_pFirst);
}

template <class T>
ListHandle List<T>::getLast(void)
{
   return reinterpret_cast<ListHandle>(m_pLast);
}

template <class T>
ListHandle List<T>::moveToLast(ListHandle h)
{
   ListNode* pNode = reinterpret_cast<ListNode*>(h);
   if (pNode->pPrev)
      pNode->pPrev->pNext = pNode->pNext;
   if (pNode->pNext)
      pNode->pNext->pPrev = pNode->pPrev;

   pNode->pNext = 0;
   pNode->pPrev = m_pLast;
   m_pLast = pNode;
   return h;
}

template <class T>
ListHandle List<T>::moveToFirst(ListHandle h)
{
  ListNode* pNode = reinterpret_cast<ListNode*>(h);
   if (pNode->pPrev)
      pNode->pPrev->pNext = pNode->pNext;
   if (pNode->pNext)
      pNode->pNext->pPrev = pNode->pPrev;

   pNode->pPrev = 0;
   pNode->pNext = m_pFirst;
   m_pFirst = pNode;
   return h;
}

}
#endif

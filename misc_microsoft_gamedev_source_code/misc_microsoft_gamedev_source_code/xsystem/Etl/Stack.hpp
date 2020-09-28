//==============================================================================
// Stack.hpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef __Stack__
#define __Stack__

#pragma warning(disable : 4514)

namespace Etl
{

template <class T>
class Stack 
{
public:
   Stack();
   ~Stack();

   void push(T);
   T pop(void);

   void clear(void);

   unsigned long count(void) const;

   Stack<T> operator = (const Stack&);

private:
   struct Node
   {
      Node(const T& t) : data(t), pNext(0) {}
      T data;
      Node* pNext;
   };

   Node* m_pStack;
   unsigned long m_ulCount;
};

template <class T>
Stack<T>::Stack() : m_pStack(0), m_ulCount(0)
{
}

template <class T>
Stack<T>::~Stack()
{
   clear();
}

template <class T>
inline void Stack<T>::push(T t)
{
   Node* pNode = new Node(t);
   pNode->pNext = 0;

   pNode->pNext = m_pStack;
   m_pStack = pNode;

   m_ulCount++;
}

template <class T>
inline T Stack<T>::pop(void)
{
   Node* pNode = m_pStack;
   m_pStack = m_pStack->pNext;
   m_ulCount--;

   return pNode->data;
}

template <class T>
unsigned long Stack<T>::count(void) const
{
   return m_ulCount;
}

template <class T>
inline void Stack<T>::clear(void)
{
   Node* pNode = 0;

   while (m_pStack)
   {
      pNode = m_pStack;
      m_pStack = m_pStack->pNext;
      delete pNode;
   }
   m_pStack = 0;
   m_ulCount = 0;
}

template <class T>
inline Stack<T> Stack<T>::operator = (const Stack<T>& s)
{
   if (m_pStack || m_ulCount > 0)
      clear();

   if (s.m_ulCount > 0)
   {
      Node* pSCurrent = s.m_pStack;
      Node* pCurrent;
      Node* pNewStack = 0;

      m_ulCount = s.m_ulCount;

      pNewStack pCurrent = new Node(pSCurrent->data);
      while (pSCurrent)
      {
         pSCurrent = pSCurrent->pNext;
         pCurrent->pNext = new Node(pSCurrent->data);
         pCurrent = pCurrent->pNext;
      }
      m_pStack = pNewStack;
   }
   return *this;
}

}

#endif

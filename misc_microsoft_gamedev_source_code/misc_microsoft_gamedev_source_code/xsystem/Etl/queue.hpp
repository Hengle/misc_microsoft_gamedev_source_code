
#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <lock.h>

namespace Etl
{

template <class T>
class Queue
{
public:

   Queue();
   ~Queue();

   //! push item on queue
   void push(const T&);
   //! push empty item on queue
   void push(void);            
   //! pop an item from the queue
   bool pop(T&);
   //! peek at an the last
   bool peek(T&) const;
   //! returns the number if items in the queue
   long size(void) const;
   //! erase all items in the queue
   void clear();
   //! peek at the next item that is going to be popped from the queue
   T&   peekFirst(void) const;
   //! peek at the last item pushed onto the queue
   T&   peekLast(void) const;

private:

   struct Node
   {
      Node(const T& t) : data(t), pNext(0) {}
      T     data;
      Node* pNext;
   };

   Node* m_pHead;
   Node* m_pTail;
   long  m_lSize;

};

template <class T>
Queue<T>::Queue() :
   m_pHead(0),
   m_pTail(0),
   m_lSize(0)
{
}

template <class T>
Queue<T>::~Queue()
{
   clear();
}

template <class T>
void Queue<T>::push(const T& data)
{
   Node* pNewNode  = new Node(data);

   if (!m_pTail)
      m_pHead = m_pTail = pNewNode;
   else
   {
      m_pTail->pNext = pNewNode;
      m_pTail = pNewNode;
   }
   m_lSize++;
}

template <class T>
bool Queue<T>::pop(T& data)
{
   if (m_lSize > 0)
   {
      Node* pNode = m_pHead;
      m_pHead = m_pHead->pNext;

      data = pNode->data;
      delete pNode;
      m_lSize--;

      if (!m_pHead)
         m_pTail = 0;
   }
   else
      return false;

   return true;
}

template <class T>
bool Queue<T>::peek(T& data) const
{
   if (m_lSize > 0)
   {
      data = m_pHead->data;
      return true;
   }
   return false;
}

template <class T>
long Queue<T>::size(void) const
{
   return m_lSize;
}

template <class T>
void Queue<T>::clear(void)
{
   while (m_pHead)
   {
      Node* pNode = m_pHead;
      m_pHead = m_pHead->pNext;
      delete pNode;
   }
   m_pHead = 0;
   m_pTail = 0;
   m_lSize = 0;
}

template <class T>
T& Queue<T>::peekFirst(void) const
{
   BASSERT(m_pHead);
   return m_pHead->data;
}

template <class T>
T& Queue<T>::peekLast(void) const
{
   BASSERT(m_pHead);
   return m_pTail->data;
}

//
//! use q.lock() before accessing any members,
//! and q.unlock() when finished
//
template <class T>
class ThreadSafeQueue
{
public:

   inline void push(const T&);
   inline bool pop(T&);
   inline bool peek(T&) const;
   inline long size(void);
   inline void clear();

private:
   BLock         m_lock;
   Etl::Queue<T> m_queue;
};

template <class T>
inline void ThreadSafeQueue<T>::push(const T& data)
{
   m_lock.lock();
   m_queue.push(data);
   m_lock.unlock();
}

template <class T>
inline bool ThreadSafeQueue<T>::pop(T& data)
{
   m_lock.lock();
   bool retval =  m_queue.pop(data);
   m_lock.unlock();
   return retval;
}

template <class T>
inline bool ThreadSafeQueue<T>::peek(T& data) const
{
   m_lock.lock();
   bool retval = m_queue.peek(data);
   m_lock.unlock();
   return retval;
}

template <class T>
inline long ThreadSafeQueue<T>::size(void) 
{
   m_lock.lock();
   long size = m_queue.size();
   m_lock.unlock();
   return size;
}

template <class T>
inline void ThreadSafeQueue<T>::clear(void)
{
   m_lock.lock();
   m_queue.clear();
   m_lock.unlock();
}


}

#endif
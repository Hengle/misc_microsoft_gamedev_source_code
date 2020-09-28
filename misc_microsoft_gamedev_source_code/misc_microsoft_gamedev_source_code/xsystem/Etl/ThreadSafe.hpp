
#pragma once

//
// ThreadSafe.hpp usage
//
// 1. declare data members or class as threadsafe
//
// class Foo
// {
// public:
//   Foo();
//
//   void doIt(void)
//   {
//     ThreadSafePtr<Socket> pSocket(m_pSocket);
//    
//     pSocket->read();
//     pSocket->write();
//   }
// private:
//   ThreadSafe<Socket> m_pSocket;
// };
//
//
namespace Etl 
{

template <class T> class ThreadSafeCounted;
template <class T> class ThreadSafePtr;

template <class T>
class ThreadSafe 
{
public:
   ThreadSafe();
   ThreadSafe(T* ptr);   
   ThreadSafe(const ThreadSafe<T>& ts);
   ~ThreadSafe();

   bool           operator == (const ThreadSafe<T>&) const;
   ThreadSafe<T>& operator =  (const ThreadSafe<T>&);
   ThreadSafe<T>& operator =  (const T*);
   
private:
   friend class ThreadSafePtr<T>;   

   void lock(void)
   {     
      EnterCriticalSection(&m_pCounted->m_xCriticalSection);
   }

   void unlock(void)
   {
      LeaveCriticalSection(&m_pCounted->m_xCriticalSection);
   }

   inline operator T& ();
   inline operator T* ();
   inline T* operator -> () const;
   inline T& operator *  () const;
   
   ThreadSafeCounted<T>* m_pCounted;
};

template <class T>
ThreadSafe<T>::ThreadSafe() : m_pCounted(0)
{
}

template <class T>
ThreadSafe<T>::ThreadSafe(T* ptr) : m_pCounted(0)
{
   m_pCounted = new ThreadSafeCounted<T>(ptr);
   m_pCounted->ref();
}

template <class T>
ThreadSafe<T>::ThreadSafe(const ThreadSafe<T>& ts) : m_pCounted(0)
{
   ts.lock();
   m_pCounted = ts.m_pCounted;
   m_pCounted->ref();
   ts.unlock();
}

template <class T>
ThreadSafe<T>::~ThreadSafe()
{
   lock(); 
   if (m_pCounted && m_pCounted->unref() == 0)
   {
      unlock();
      delete m_pCounted;
   }
   else
      unlock();   
}

template <class T>
bool ThreadSafe<T>::operator == (const ThreadSafe<T>& ts) const
{
   bool retval = false;
   lock();
   ts.lock();
   if ((m_pCounted && ts.m_pCounted) && m_pCounted == ts.m_pCounted)
      retval = true;

   unlock();
   ts.unlock();

   return retval;
}

template <class T>
ThreadSafe<T>& ThreadSafe<T>::operator = (const ThreadSafe<T>& ts)
{
   lock();
   if (m_pCounted && m_pCounted->unref() == 0)
   {
      unlock();
      delete m_pCounted;
   }
   else 
      unlock();

   ts.lock();
   m_pCounted = ts.m_pCounted;
   m_pCounted->ref();
   ts.unlock();

   return *this;
}

template <class T>
inline ThreadSafe<T>& ThreadSafe<T>::operator = (const T* ptr)
{
   ThreadSafeCounted<T>* pOld = m_pCounted;

   if (pOld)
   {
      EnterCriticalSection(pOld->m_xCriticalSection);      

      m_pCounted = new ThreadSafeCount<T>(ptr);
      m_pCounted->ref();

      if (pOld && pOld->unref() > 0)
         pOld = 0;

      LeaveCriticalSection(pOld->m_xCriticalSection);
   }

   delete pOld;

   return *this;
}


template <class T>
ThreadSafe<T>::operator T& ()
{
   return *m_pCounted->m_ptr;
}

template <class T>
ThreadSafe<T>::operator T* ()
{
   return m_pCounted->m_ptr;
}

template <class T>
T* ThreadSafe<T>::operator -> () const
{
   return m_pCounted->m_ptr;
}

template <class T>
T& ThreadSafe<T>::operator * () const
{
   return *m_pCounted->m_ptr;
}

template <class T>
class ThreadSafePtr
{
public:
  ThreadSafePtr(ThreadSafe<T>& ts);
  ~ThreadSafePtr();

   inline T& operator *  ();
   inline T* operator -> ();
   inline operator T* ();
   inline operator T& ();
   inline const ThreadSafePtr<T>& operator =  (const ThreadSafePtr<T>&);
   inline const ThreadSafePtr<T>& operator =  (const ThreadSafe<T>&);
   inline bool                    operator == (const ThreadSafePtr<T>&) const;
   
   inline bool isNull(void) const;

private:

  ThreadSafe<T>& m_ts;
};

template <class T>
ThreadSafePtr<T>::ThreadSafePtr(ThreadSafe<T>& ts) : m_ts(ts)
{
   m_ts.lock();
}

template <class T>
ThreadSafePtr<T>::~ThreadSafePtr()
{
   m_ts.unlock();
}

template <class T>
inline ThreadSafePtr<T>::operator T* ()
{
   return m_ts;
}

template <class T>
inline ThreadSafePtr<T>::operator T& ()
{
   return m_ts;
}


template <class T>
inline T& ThreadSafePtr<T>::operator * () 
{
   return m_ts;
}

template <class T>
inline T* ThreadSafePtr<T>::operator -> ()
{
   return m_ts;
}

template <class T> 
class ThreadSafeCounted
{
private:
   friend class ThreadSafe<T>;

   ThreadSafeCounted() : m_ptr(0), m_lCount(0)
   {
      InitializeCriticalSection(&m_xCriticalSection);
   }

   ThreadSafeCounted(T* ptr) : m_ptr(ptr), m_lCount(0)
   {      
      BASSERT(ptr);
      InitializeCriticalSection(&m_xCriticalSection);
   }
   
   ~ThreadSafeCounted()
   {
      BASSERT(m_lCount == 0);
      DeleteCriticalSection(&m_xCriticalSection);      
      delete m_ptr;
   }

   long ref(void);
   long unref(void);

   CRITICAL_SECTION m_xCriticalSection;
   T*               m_ptr;
   long             m_lCount;
};

template <class T>
inline long ThreadSafeCounted<T>::ref(void)
{
   return ++m_lCount; 
}

template <class T>
inline long ThreadSafeCounted<T>::unref(void)
{
   BASSERT(m_lCount > 0); 
   return --m_lCount;
}


}

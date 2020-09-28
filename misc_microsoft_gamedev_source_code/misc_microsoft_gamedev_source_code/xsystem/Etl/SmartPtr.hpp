
#ifndef SMARTPTR_H
#define SMARTPTR_H

#include <Etl/RefObj.hpp>

namespace Etl
{

template <class T> class SmartCounted;

template <class T>
class SmartPtr
{
public:

   SmartPtr();
   SmartPtr(const SmartPtr<T>&);
   SmartPtr(T* ptr);
   ~SmartPtr();

   inline T& operator *  ()  const;
   inline T* operator -> ()  const;
   inline operator T* ();
   inline operator T& ();
   inline const SmartPtr<T>& operator =  (const SmartPtr<T>&);
   inline const SmartPtr<T>& operator =  (const T*);
   inline bool               operator == (const SmartPtr<T>&) const;
   
   inline bool isNull(void) const;
   inline void unbind(void);
   inline void bind(T* ptr);
private:

   SmartCounted<T>* m_pCounted;

};

template <class T>
SmartPtr<T>::SmartPtr() : m_pCounted(0)
{
}

template <class T>
SmartPtr<T>::SmartPtr(T* ptr) : m_pCounted(0)
{
   bind(ptr);
}

template <class T>
SmartPtr<T>::SmartPtr(const SmartPtr<T>& sptr) : m_pCounted(0)
{
   m_pCounted = sptr.m_pCounted;
   if (m_pCounted)
      m_pCounted->ref();
}

template <class T>
SmartPtr<T>::~SmartPtr()
{
   unbind();
}

template <class T>
inline T& SmartPtr<T>::operator * () const
{
   BASSERT(m_pCounted->m_ptr);
   return *m_pCounted->m_ptr;
}

template <class T>
inline T* SmartPtr<T>::operator -> () const
{
   BASSERT(m_pCounted->m_ptr);
   return m_pCounted->m_ptr;
}

template <class T>
inline SmartPtr<T>::operator T* ()
{
   return m_pCounted->m_ptr;
}

template <class T>
inline SmartPtr<T>::operator T& ()
{
   return *m_pCounted->m_ptr;
}

template <class T>
inline const SmartPtr<T>& SmartPtr<T>::operator = (const SmartPtr<T>& sptr)
{
   unbind();
   m_pCounted = sptr.m_pCounted;

   if (m_pCounted)
      m_pCounted->ref();   

   return *this;
}

template <class T>
inline const SmartPtr<T>& SmartPtr<T>::operator = (const T* ptr)
{
   bind(const_cast<T*>(ptr));
   return *this;
}

template <class T>
inline void SmartPtr<T>::bind(T* ptr)
{
   if (m_pCounted)
      unbind();

   if (ptr)
   {
      m_pCounted = new SmartCounted<T>(ptr);
      m_pCounted->ref();
   }
}

template <class T>
inline void SmartPtr<T>::unbind(void)
{
   if (m_pCounted && m_pCounted->unref() == 0)
      delete m_pCounted;

   m_pCounted = 0;    
}

template <class T>
inline bool SmartPtr<T>::isNull(void) const
{
   return (m_pCounted == 0);
}

template <class T>
inline bool SmartPtr<T>::operator == (const SmartPtr<T>& sp) const
{
   return (m_pCounted->m_ptr == sp.m_pCounted->m_ptr);
}

//////////////////////////////////////////////////////////

template <class T>
class SmartCounted
{
protected:
  friend class SmartPtr<T>;
  SmartCounted(T* ptr);
  virtual ~SmartCounted();

  inline long ref(void);
  inline long unref(void);

  T* const m_ptr;
  long m_lCount;
};

template <class T>
SmartCounted<T>::SmartCounted(T* ptr) : m_ptr(ptr), m_lCount(0)
{
   BASSERT(m_ptr);
}

template <class T>
SmartCounted<T>::~SmartCounted()
{
   BASSERT(m_lCount == 0);
   delete m_ptr;
}

template <class T>
inline long SmartCounted<T>::ref(void)
{
   return ++m_lCount; 
}

template <class T>
inline long SmartCounted<T>::unref(void)
{
   BASSERT(m_lCount > 0); 
   return --m_lCount;
}

}

#endif
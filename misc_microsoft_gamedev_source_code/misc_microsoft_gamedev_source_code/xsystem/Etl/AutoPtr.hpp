
#pragma once


template <class T>
class AutoPtr
{
public:

   AutoPtr(T* ptr);
   ~AutoPtr();

   T& operator *  () const;
   T* operator -> () const;

   void bind(T* ptr);
   void unbind(void);

private:

   T* m_ptr;

};

template <class T>
AutoPtr<T>::AutoPtr(T* ptr) : 
   m_ptr(ptr)
{
}

template <class T>
AutoPtr<T>::~AutoPtr()
{
   delete m_ptr;
}

template <class T>
T& AutoPtr<T>::operator * () const
{
   return *m_ptr;
}

template <class T>
T* AutoPtr<T>::operator -> () const
{
   return m_ptr;
}

template <class T>
void AutoPtr<T>::bind(T* ptr)
{
   delete m_ptr;
   m_ptr = ptr;
}

template <class T>
void AutoPtr<T>::unbind(void)
{
   delete m_ptr;
   m_ptr = 0;
}


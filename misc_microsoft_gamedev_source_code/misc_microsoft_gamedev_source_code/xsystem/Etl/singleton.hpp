//==============================================================================
// BSingleton.hpp
//==============================================================================

#ifndef	_Singleton_H_
#define	_Singleton_H_

#define	WIN32_LEAN_AND_MEAN
#ifndef XBOX
#include <windows.h>
#endif


//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
// This class implements a singleton as a static object
template <typename T>
class BSingleton
{
public:

   // call BSingleton::getInstance() to get a pointer to the object
	static T* getInstance()
   {
		return &mInstance;
	}

protected:
	BSingleton() {}
	virtual ~BSingleton() {}

	static T 	mInstance;
};

template <typename T>
T BSingleton<T>::mInstance;


//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
// This class implements a singleton as a reference counted object on the heap.
template <typename T>
class BSingletonPtr
{
public:

   // call BSingleton::getInstance() to get a pointer to the object
	static T* getInstance()
   {
		if ( 1 == InterlockedIncrement( &mCount ) ) 
			mInstance = new T;

		return mInstance;
	}

   // call pSingleton->releaseInstance() to release your pointer to the object
   // The instance is destroyed if the last pointer to the object is released.
   void releaseInstance()
   {
      if ( 0 == InterlockedDecrement( &mCount ) )
         destroyInstance();
   }

   // call BSingletonPtr::destroyInstance() to destroy the object and return memory
	static void destroyInstance()
   {
		delete mInstance;
		mInstance = NULL;
      mCount = 0;
	}

protected:
	BSingletonPtr() {}
	virtual ~BSingletonPtr() {}

	static T*	mInstance;
	static long	mCount;
};

template <typename T>
T* BSingletonPtr<T>::mInstance = NULL;

template <typename T>
long BSingletonPtr<T>::mCount = 0;


#endif	//	_Singleton_H_


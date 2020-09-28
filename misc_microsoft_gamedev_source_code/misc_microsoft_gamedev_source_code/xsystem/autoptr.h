//============================================================================
//
//  BAutoPtr.h
//  
//  Copyright (c) 2001, Ensemble Studios
//
//============================================================================


#pragma once

#ifndef __BAUTOPTR_H__
#define __BAUTOPTR_H__


//----------------------------------------------------------------------------
// Class BAutoPtr
//
// Pointer wrapper class that deletes its' contained pointer upon destruction.
//
// By having an overridden -> operator, you can use an instance of this
// class as if it were the original pointer.
//
// Useful for memory cleanup in methods with multiple exit points. Using this
// class keeps you from having to call delete before each exit point, but
// still cleans up on exit.
//
// WARNING: This class should be created on the stack for obvious reasons.
//
// Usage:
//
//   BAutoPtr< Foo > pFoo = new Foo;
//   // do something
//   pFoo->doit(); // calling a foo method through the autoptr
//   return true;  // the dtor for autoptr gets called and Foo gets
//                 // cleaned up.
//
//----------------------------------------------------------------------------
template< class T >
class BAutoPtr
{
   public:
      inline BAutoPtr( T* ptr=0 ) : mPtr( ptr )  {}

      inline BAutoPtr( const BAutoPtr< T >& other )
      {
         // I assume ownership of the pointer.
         mPtr = other.mPtr;
         other.mPtr = 0;
      }

      inline BAutoPtr< T >& operator=( BAutoPtr< T >& rhs )
      {
         // I assume ownership of the pointer.
         if ( mPtr ) delete mPtr;
         mPtr = rhs.mPtr;
         rhs.mPtr = 0;
         return *this;
      }

      inline ~BAutoPtr()  { if ( mPtr ) delete mPtr; }

      inline T* operator->()  { return get(); }
      inline T* get()  { return mPtr; }

      // Here mainly to allow if ( !ptr ) checks.
      inline operator void*()  { return mPtr; }

      // Release control of the pointer to my caller.
      inline T* detach()  { T* ptr = mPtr; mPtr = 0; return ptr; }

   private:
      T* mPtr;
};


#endif  // __BAUTOPTR_H__



//------------------------------------------------------------------------------------------------------------------------
//  File: packedPointer.h
//
//  Copyright (c) 2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#pragma once

#include "packedType.h"
#include "templateIf.h"
#include "typeTraits\TypeTraits.h"

template<typename T, bool BigEndian>
class BPackedPointer
{
   friend class BPackedPointer<T, !BigEndian>;

public: 
   typedef T                                          valueType;
   typedef typename Loki::TypeTraits<T>::PointeeType  pointeeType;
   enum { cBigEndian = BigEndian };

   BPackedPointer() 
   {
      BCOMPILETIMEASSERT(Loki::TypeTraits<T>::isPointer);
   }

   BPackedPointer(const BPackedPointer& other) : 
      mPtr(static_cast<T>(other.mPtr)) 
   { 
   }

   BPackedPointer(const BPackedPointer<T, !BigEndian>& other) : 
      mPtr(static_cast<T>(other.mPtr)) 
   { 
   }

   BPackedPointer(T pVal) : 
      mPtr(pVal) 
   { 
   }

   BPackedPointer& operator= (const BPackedPointer& rhs) 
   { 
      mPtr = static_cast<T>(rhs.mVal); 
      return *this; 
   }

   BPackedPointer& operator= (const BPackedPointer<T, !BigEndian>& rhs) 
   { 
      mPtr = static_cast<T>(rhs.mVal); 
      return *this; 
   }

   BPackedPointer& operator= (T pVal) 
   { 
      mPtr = pVal; 
      return *this; 
   }

   operator T() const { return mPtr; }

   pointeeType& operator*() const { return *mPtr; }
   pointeeType* operator->() const { return mPtr; }
   
   void offsetize(void* p)
   {
      mPtr = (T)((BYTE*)(T)mPtr - (BYTE*)p);
   }

private:
   BPackedType<T, BigEndian> mPtr;
};

#define PACKED_POINTER_TYPE(type) typename BIf< Packed, type, BPackedPointer< type , BigEndian> >::BResultType

template<typename T>
class BPackedObjectPointer
{
public: 
   typedef T                                          valueType;
   typedef typename Loki::TypeTraits<T>::PointeeType  pointeeType;

   operator T() const { return mPtr; }

   pointeeType& operator*() const { BDEBUG_ASSERT(mPtr); return *mPtr; }
   pointeeType* operator->() const { BDEBUG_ASSERT(mPtr); return mPtr; }
   
   bool unpack(const BDataBuffer& buf, bool unpackPointee = true) 
   { 
      if (mPtr == (T)0xFFFFFFFF) 
      {
         mPtr = NULL;
         return true;
      }

      if (((uint)mPtr >= buf.getLen()) || ((uint)mPtr & 1))
         return false;

      Utils::Pointerize(mPtr, buf.getPtr());

      if ((unpackPointee) && (!BIsBuiltInType<pointeeType>::Flag))
      {
         if (!BUnpackHelper<pointeeType>::unpack(*mPtr, buf))
            return false;
      }         

      return true;         
   }

private:
   T mPtr;
};

template<typename T, bool BigEndian>
class BPackedObjectPointerBuilder
{
   friend class BPackedObjectPointerBuilder<T, !BigEndian>;
   
public: 
   typedef T                                          valueType;
   typedef typename Loki::TypeTraits<T>::PointeeType  pointeeType;
   enum { cBigEndian = BigEndian };

   BPackedObjectPointerBuilder() :
      mPtr(NULL)
   {
   }

   BPackedObjectPointerBuilder(const BPackedObjectPointerBuilder& other)  
   { 
      if (other == NULL)
         mPtr = NULL;
      else
         alloc(*other);
   }
      
   BPackedObjectPointerBuilder(const BPackedObjectPointerBuilder<T, !BigEndian>& other)  
   { 
      if (other == NULL)
         mPtr = NULL;
      else
         alloc(*other);
   }
   
   ~BPackedObjectPointerBuilder()
   {
      release();
   }
   
   void create(const pointeeType& other = pointeeType())
   {  
      BDEBUG_ASSERT(!getPacked());
      
      if (isNull())
         alloc(other);
      else
         *getPtr() = other;
   }
   
   void destroy(void)
   {
      BDEBUG_ASSERT(!getPacked());
      
      release();
   }
   
   BPackedObjectPointerBuilder& operator= (const BPackedObjectPointerBuilder& rhs) 
   { 
      BDEBUG_ASSERT(!getPacked() && !rhs.getPacked());

      if (rhs == NULL)
      {
         release();
         mPtr = NULL;
      }
      else
      {
         if (isNull())
            alloc(*rhs);
         else
            *getPtr() = *rhs;
      }

      return *this; 
   }
      
   BPackedObjectPointerBuilder& operator= (const BPackedObjectPointerBuilder<T, !BigEndian>& rhs) 
   { 
      BDEBUG_ASSERT(!getPacked() && !rhs.getPacked());
      
      if (rhs == NULL)
      {
         release();
         mPtr = NULL;
      }
      else
      {
         if (isNull())
            alloc(*rhs);
         else
            *getPtr() = *rhs;
      }
      
      return *this; 
   }
   
   operator T() const { BDEBUG_ASSERT(!getPacked()); return getPtr(); }

   pointeeType& operator*() const { BDEBUG_ASSERT(!getPacked() && !isNull()); return *getPtr(); }
   pointeeType* operator->() const { BDEBUG_ASSERT(!getPacked() && !isNull()); return getPtr(); }
   
   bool pack(BPackState& state)
   {
      BDEBUG_ASSERT(!getPacked());
      
      T pCurPointee = getPtr();
      if (!pCurPointee)
      {
         mPtr = (T)0xFFFFFFFF;
         return true;
      }
      
      typedef Loki::TypeTraits<pointeeType>::NonConstType nonConstPointee;
                  
      nonConstPointee* pNewPointee = static_cast<nonConstPointee*>(state.getMemStack().alignedAlloc(sizeof(pointeeType), ALIGN_OF(pointeeType)));
      if (!pNewPointee)
         return false;
         
      BDEBUG_ASSERT(((uint)pNewPointee & 1) == 0);
         
      Utils::ConstructInPlace(pNewPointee, *pCurPointee);
                  
      mPtr = (T)(pNewPointee);
      
      T pNewPointeeT = (T)pNewPointee;
      if (!BPackHelper< nonConstPointee >::pack(*pNewPointeeT, state))
         return false;

      Utils::Offsetize(pNewPointee, state.getMemStack().getBasePtr());

      mPtr = (T)(pNewPointee);
            
      return true;
   } 
   
   bool getPacked(void) const
   {
      if (isNull())
         return false;
         
      if ((T)mPtr == (T)0xFFFFFFFF)
         return true;
               
      return (((uint)(T)mPtr) & 1) == 0;
   }
   
   bool isNull(void) const
   {
      return NULL == (T)mPtr;
   }

private:
   BPackedType<T, BigEndian> mPtr;
   
   void setPtr(void* p)
   {
      BDEBUG_ASSERT( ((uint)p & 1) == 0);
      mPtr = (T)((uint)p | 1);
   }
         
   T getPtr(void) const
   {
      return (T)(((uint)(T)mPtr) & ~1);
   }
      
   void alloc(const pointeeType& other = pointeeType())
   {
      setPtr(new pointeeType(other));
   }
   
   void release(void)
   {
      if (!getPacked())
      {
         delete getPtr();
         mPtr = NULL;
      }
   }
};

// To use this macro, ensure the Packed and BigEndian symbols are defined in the scope this macro appears in.
// type must be a pointer.
#define PACKED_OBJECT_POINTER_TYPE(type) typename BIf< Packed, BPackedObjectPointer< type >, BPackedObjectPointerBuilder< type , BigEndian> >::BResultType

template<typename T, bool BigEndian> struct BPackHelper< BPackedObjectPointerBuilder<T, BigEndian> > 
{ 
   static bool pack(BPackedObjectPointerBuilder<T, BigEndian>& r, BPackState& s) 
   { 
      return r.pack(s); 
   }
}; 

template<typename T> struct BUnpackHelper< BPackedObjectPointer<T> > 
{ 
   static bool unpack( BPackedObjectPointer<T>& r, const BDataBuffer& buf) 
   { 
      return r.unpack(buf); 
   } 
};

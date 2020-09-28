//------------------------------------------------------------------------------------------------------------------------
//  File: packedArray.h
//
//  Copyright (c) 2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#pragma once

#include "packedType.h"
#include "templateIf.h"

template<typename T>
class BPackedArray : public Utils::RelativeOperators< BPackedArray<T> >
{
public:
   typedef T valueType;
   
   BPackedArray() { }
      
   bool unpack(const BDataBuffer& buf, bool unpackElements = true) 
   { 
      if (mPtr == (T*)0xFFFFFFFF) 
      {
         mPtr = NULL;
         return 0 == getSize();
      }
      
      const uint alignment = ALIGN_OF(T);
      if (((uint)mPtr >= buf.getLen()) || ((uint)mPtr & (alignment - 1)))
         return false;
                        
      Utils::Pointerize(mPtr, buf.getPtr());
      
      if ((unpackElements) && (!BIsBuiltInType<T>::Flag))
      {
         for (uint i = 0; i < getSize(); i++)
         {
            if (!BUnpackHelper<T>::unpack(((T*)getPtr())[i], buf))
               return false;
         }
      }         
      
      return true;         
   }
   
   uint getSize(void) const { return mSize; }
   uint getSizeInBytes(void) const { return mSize * sizeof(T); }
   uint size(void) const { return getSize(); }
   bool empty(void) const { return 0 == getSize(); }

   const T& operator[] (uint i) const  { BDEBUG_ASSERT(i < getSize()); return getPtr()[i]; }
         T& operator[] (uint i)        { BDEBUG_ASSERT(i < getSize()); return getPtr()[i]; }
                     
   const T* getPtr(void) const   { return mPtr; }
         T* getPtr(void)         { return mPtr; }
                  
   bool operator== (const BPackedArray& rhs) const
   {
      if (mSize != rhs.getSize())
         return false;
      for (uint i = 0; i < mSize; i++)
         if (getPtr()[i] != rhs[i])
            return false;
      return true;            
   }
   
   bool operator< (const BPackedArray& rhs) const
   {
      if (getSize() < rhs.getSize())
         return true;
      else if (getSize() == rhs.getSize())
      {
         for (uint i = 0; i < getSize(); i++)
         {
            if ((*this)[i] < rhs[i])
               return true;
            else if (!((*this)[i] == rhs[i]))
               return false;
         }
      }           
      return false;            
   }
            
protected:
   uint     mSize;
   T*       mPtr;
};

template<typename T, bool BigEndian>
class BPackedArrayBuilder : public Utils::RelativeOperators< BPackedArrayBuilder<T, BigEndian> >
{
   friend class BPackedArrayBuilder<T, !BigEndian>;
   
public:
   enum { cBigEndian = BigEndian };
   typedef T valueType;
   typedef BDynamicArray<T> arrayType;
   
   BPackedArrayBuilder()  
   { 
      BCOMPILETIMEASSERT(sizeof(BPackedArrayBuilder) == sizeof(BPackedArray<T>));
      
      mSize = UINT_MAX;
      mPtr = new arrayType;
   }

   BPackedArrayBuilder(uint size) 
   {
      mSize = UINT_MAX;
      mPtr = new arrayType(size);
   }
      
   BPackedArrayBuilder(const BPackedArrayBuilder& rhs)
   {
      if (rhs.getPacked())
      {
         mSize = rhs.getPackedSize();
         mPtr = rhs.getPackedPtr();
      }
      else
      {
         mSize = UINT_MAX;
         mPtr = new arrayType;
         
         if (!rhs.empty())
         {
            pushBack(rhs.getPtr(), rhs.getSize());
         }            
      }         
   }
   
   template<typename OtherT, bool OtherBigEndian>
   BPackedArrayBuilder(const BPackedArrayBuilder<OtherT, OtherBigEndian>& other)
   {
      if (other.getPacked())
      {
         // This may be invalid - OtherT could differ from T!
         BDEBUG_ASSERT(sizeof(OtherT) == sizeof(T));
         mSize = other.getPackedSize();
         mPtr = other.getPackedPtr();
      }
      else
      {
         mSize = UINT_MAX;
         mPtr = new arrayType;

         if (!other.empty())
         {
            reserve(other.getSize());
            
            for (uint i = 0; i < other.getSize(); i++)
               pushBack(other[i]);
         }            
      }         
   }
         
   BPackedArrayBuilder& operator= (const BPackedArrayBuilder& rhs)
   {
      if (rhs.getPacked())
      {
         if (!getPacked())
         {
            delete &getArray();
         }
         mSize = rhs.mSize;
         mPtr = rhs.mPtr;
      }
      else
      {
         if (getPacked())
         {
            mSize = UINT_MAX;
            mPtr = new arrayType;
         }
         
         clear();
         if (!rhs.empty())
            pushBack(rhs.getPtr(), rhs.getSize());
      }
      
      return *this;
   }
   
   template<typename OtherT, bool OtherBigEndian>
   BPackedArrayBuilder& operator= (const BPackedArrayBuilder<OtherT, OtherBigEndian>& rhs)
   {
      if (rhs.getPacked())
      {
         if (!getPacked())
         {
            delete &getArray();
         }
         // This may be invalid - OtherT could differ from T!
         BDEBUG_ASSERT(sizeof(OtherT) == sizeof(T));
         mSize = rhs.getPackedSize();
         mPtr = rhs.getPackedPtr();
      }
      else
      {
         if (getPacked())
         {
            mSize = UINT_MAX;
            mPtr = new arrayType;
         }
         else
         {
            clear();
         }
         
         if (!rhs.empty())
         {
            reserve(rhs.getSize());
                  
            for (uint i = 0; i < rhs.getSize(); i++)
               pushBack(rhs[i]);
         }
      }         
         
      return *this;         
   }
           
   ~BPackedArrayBuilder()
   {
      if (!getPacked())
         delete &getArray();
   }
         
   bool getPacked(void) const 
   { 
      return mSize != UINT_MAX;
   }
   
   void reserve(uint capacity)
   {
      BDEBUG_ASSERT(!getPacked());
      getArray().reserve(capacity);
   }
   
   void pushBack(const T& value) 
   {  
      BDEBUG_ASSERT(!getPacked());
      getArray().pushBack(value); 
   }
   
   void push_back(const T& value) { pushBack(value); }
   
   void pushBack(const T* pVals, uint num)
   {  
      BDEBUG_ASSERT(!getPacked());
      getArray().pushBack(pVals, num);
   }
   
   T* enlarge(uint n)
   {
      BDEBUG_ASSERT(!getPacked());
      
      const uint curSize = getSize();
      
      resize(curSize + n);
      
      if (!getPtr())
         return NULL;   
         
      return getPtr() + curSize;
   }
   
   void popBack(void) 
   {  
      BDEBUG_ASSERT(!getPacked());
      
      getArray().popBack(); 
   }
   
   void clear(void) 
   { 
      BDEBUG_ASSERT(!getPacked());
      
      getArray().clear(); 
   }
   
   void resize(uint newSize) 
   {  
      BDEBUG_ASSERT(!getPacked());
      
      getArray().resize(newSize); 
   }
            
   uint getSize(void) const 
   { 
      if (getPacked())
         return mSize;
      else
         return getArray().getSize(); 
   }
   uint getSizeInBytes(void) const { return mSize * sizeof(T); }
   
   const T* begin(void) const { return getArray().begin(); }
         T* begin(void)       { return getArray().begin(); }
   const T* end(void) const   { return getArray().end(); }
         T* end(void)         { return getArray().end(); }
   
   uint size(void) const { return getSize();  }
   bool empty(void) const { return 0 == getSize(); }
   
   const T& operator[] (uint i) const  
   { 
      BDEBUG_ASSERT(i < getSize()); 
      return getPtr()[i];
   }
   
   T& operator[] (uint i)        
   { 
      BDEBUG_ASSERT(i < getSize()); 
      return getPtr()[i];
   }
   
   const T& at (uint i) const { return (*this)[i]; }
         T& at (uint i)       { return (*this)[i]; }
   
   const T* getPtr(void) const   
   { 
      BDEBUG_ASSERT(!getPacked());
      return getArray().getPtr();
   }
   
   T* getPtr(void) 
   { 
      BDEBUG_ASSERT(!getPacked());
      return getArray().getPtr();
   }
            
   bool operator== (const BPackedArrayBuilder& rhs) const
   {
      const BPackedArrayBuilder& lhs = *this;
      if (lhs.size() != rhs.size())
         return false;
      for (uint objIter = 0; objIter < lhs.size(); objIter++)
         if (!(lhs[objIter] == rhs[objIter]))
            return false;
      return true;
   }

   bool operator< (const BPackedArrayBuilder& rhs) const
   {
      const BPackedArrayBuilder& lhs = *this;
      if (lhs.size() < rhs.size())
         return true;
      else if (lhs.size() == rhs.size())
      {
         for (uint objIter = 0; objIter < lhs.size(); objIter++)
         {
            if (lhs[objIter] < rhs[objIter])
               return true;
            else if (!(lhs[objIter] == rhs[objIter]))
               return false;
         }
      }
      return false;
   }
   
   template<typename OtherT, bool OtherBigEndian>
   bool operator== (const BPackedArrayBuilder<OtherT, OtherBigEndian>& rhs) const
   {
      if (getSize() != rhs.getSize())
         return false;
      for (uint i = 0; i < getSize(); i++)
         if ((*this)[i] != rhs[i])
            return false;
      return true;            
   }
   
   template<typename OtherT, bool OtherBigEndian>
   bool operator< (const BPackedArrayBuilder<OtherT, OtherBigEndian>& rhs) const
   {
      if (getSize() < rhs.getSize())
         return true;
      else if (getSize() == rhs.getSize())
      {
         for (uint i = 0; i < getSize(); i++)
         {
            if ((*this)[i] < rhs[i])
               return true;
            else if (!((*this)[i] == rhs[i]))
               return false;
         }
      }           
      return false;            
   }
   
   template<typename OtherT, bool OtherBigEndian> bool operator >  (const BPackedArrayBuilder<OtherT, OtherBigEndian>& b) { return b < *this; }
   template<typename OtherT, bool OtherBigEndian> bool operator <= (const BPackedArrayBuilder<OtherT, OtherBigEndian>& b) { return !(b < *this); }
   template<typename OtherT, bool OtherBigEndian> bool operator >= (const BPackedArrayBuilder<OtherT, OtherBigEndian>& b) { return !(*this < b); }
   template<typename OtherT, bool OtherBigEndian> bool operator != (const BPackedArrayBuilder<OtherT, OtherBigEndian>& b) { return !(*this == b); }
   
   bool pack(BPackState& state, uint minAlignment = sizeof(DWORD))
   {
      BDEBUG_ASSERT(!getPacked());
      if (getPacked())
         return true;
         
      if (!packBase(state, minAlignment))
         return false;
      
      if (!BIsBuiltInType<T>::Flag)
      {
         T* p = static_cast<T*>(static_cast<void*>(mPtr));
         for (uint i = 0; i < getSize(); i++)
         {
            if (!BPackHelper<T>::pack(p[i], state))
               return false;
         }
      }         
         
      offsetize(state.getMemStack().getBasePtr());
      
      return true;
   }
   
   // If the container is unpacked, size will be 0xFFFFFFFF and Ptr will be a BDynamicArray<T> pointer!
   uint getPackedSize(void) const { return mSize; }
   void* getPackedPtr(void) const { return mPtr; }
         
private:
   const arrayType& getArray(void) const { BDEBUG_ASSERT(!getPacked()); return *static_cast<const arrayType*>(static_cast<const void*>(mPtr)); }
         arrayType& getArray(void)       { BDEBUG_ASSERT(!getPacked()); return *static_cast<      arrayType*>(static_cast<void*      >(mPtr)); }
         
   BPackedType<uint, BigEndian>  mSize;
   BPackedType<void*, BigEndian> mPtr;
   
   void offsetize(void* pBase)
   {
      if (mPtr == NULL)
         mPtr = reinterpret_cast<void*>(0xFFFFFFFF);
      else
         mPtr = reinterpret_cast<void*>(static_cast<uchar*>(static_cast<void*>(mPtr)) - static_cast<uchar*>(pBase));
   }
      
   bool packBase(BPackState& state, uint minAlignment)
   {
      BDEBUG_ASSERT(!getPacked());

      if (getPacked())
         return true;

      if (empty())               
      {
         delete &getArray();

         mSize = 0;
         mPtr = NULL;
         
         return true;
      }

      const uint alignOf = Math::Max<uint>(minAlignment, ALIGN_OF(T));
      const uint size = getSize();
      const uint sizeInBytes = Utils::AlignUpValue(size * sizeof(T), alignOf);

      T* p = static_cast<T*>(state.getMemStack().alloc(sizeInBytes));
      if (!p)
         return false;

      if (!BIsBuiltInType<T>::Flag)
      {
         for (uint i = 0; i < size; i++)
            Utils::ConstructInPlace(p + i, getArray()[i]);  
      }            
      else
      {
         Utils::FastMemCpy(p, getPtr(), sizeInBytes);  
      }

      delete &getArray();

      mSize = size;
      mPtr = p;
      
      return true;
   }
};

template<class T, bool BigEndian> struct BPackHelper< BPackedArrayBuilder<T, BigEndian> > 
{ 
   static bool pack(BPackedArrayBuilder<T, BigEndian>& r, BPackState& s) 
   { 
      return r.pack(s); 
   }
 }; 
 
template<class T> struct BUnpackHelper< BPackedArray<T> > 
{ 
   static bool unpack( BPackedArray<T>& r, const BDataBuffer& buf) 
   { 
      return r.unpack(buf); 
   } 
};

// To use this macro, ensure the Packed and BigEndian symbols are defined in the scope this macro appears in.
#define PACKED_ARRAY_TYPE(type) typename BIf< Packed, BPackedArray< type >, BPackedArrayBuilder< type , BigEndian> >::BResultType
//------------------------------------------------------------------------------------------------------------------------
//  File: packedType.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#pragma once
#include "endianSwitch.h"
#include "templateIf.h"
#include "typeTraits\TypeTraits.h"
#include "containers\hashMap.h"
#include "memory\memStack.h"

// Disable PREfast warnings
#pragma warning( push )
#pragma warning( disable : 25042)

// T should be an integral type: short, int, int64, float, double, or a pointer.
// If AlwaysReadAndWriteBytes=false and BigEndian==cBigEndianNative (i.e. BigEndian matches the system's endianness), 
// the gets/sets should be a no-op, otherwise endian switching will occur on the fly.
template<typename T, bool BigEndian = cBigEndianNative, bool AlwaysReadAndWriteBytes = false>
class BPackedType
{
   friend class BPackedType<T, BigEndian, !AlwaysReadAndWriteBytes>;
   friend class BPackedType<T, !BigEndian, false>;
   friend class BPackedType<T, !BigEndian, true>;
   
public: 
   typedef T valueType;
   
   BPackedType() { }
   
   BPackedType(T val) { set(val); }
   
   template<bool OtherBigEndian, bool OtherAlwaysReadAndWriteBytes>
   BPackedType(const BPackedType<T, OtherBigEndian, OtherAlwaysReadAndWriteBytes>& val) { set(val.get()); }
   
   template<bool OtherBigEndian, bool OtherAlwaysReadAndWriteBytes>
   BPackedType& operator= (const BPackedType<T, OtherBigEndian, OtherAlwaysReadAndWriteBytes>& rhs) { set(rhs.get()); return *this; }
   
   operator T() const { return get(); }
   
   BPackedType& operator= (T val) { set(val); return *this; }
         
   // This operator only makes sense when T is a pointer type.
   T operator->() const { return get(); }
      
   void offsetize(void* p) { set((T)((uchar*)get() - (uchar*)p )); }
   
   void endianSwitch(void) { Utils::EndianSwitch(this, sizeof(*this)); }
   
private:
   enum { cSizeInBytes = sizeof(T) };
   
   union
   {
      uchar mBytes[cSizeInBytes];
      T mValue;
   };
   
   typedef T valueType;
        
   void set(T val)
   {
      if (AlwaysReadAndWriteBytes)
      {
         Utils::WriteValue<T>(mBytes, val, BigEndian);
      }
      else if (BigEndian == cBigEndianNative)
      {
         *(T*)mBytes = val;
      }
      else
      {
         Utils::WriteValueByteSwapped<T>(mBytes, val);
      }
   }

   T get(void) const
   {
      if (AlwaysReadAndWriteBytes)
      {
         return Utils::GetValue<T>(mBytes, BigEndian);
      }
      else if (BigEndian == cBigEndianNative)
      {
         return *(const T*)mBytes;
      }
      else
      {
         return Utils::GetValueByteSwapped<T>(mBytes);      
      }
   }
};

class BPackState
{
   BPackState(const BPackState&);
   BPackState& operator= (const BPackState&);
   
public:
   BPackState(BMemStack& memStack) : 
      mMemStack(memStack)
   {
   }
   
   void* findPointerRemap(void* pKey) const
   {
      BPointerHashMap::const_iterator it(mPointerHash.find(pKey));
      if (it == mPointerHash.end())
         return NULL;
      return it->second;
   }
   
   bool addPointerRemap(void* pKey, void* pValue)
   {
      BPointerHashMap::InsertResult result(mPointerHash.insert(pKey, pValue));
      return result.second;
   }
   
         BMemStack& getMemStack(void)        { return mMemStack; }
   const BMemStack& getMemStack(void) const  { return mMemStack; }

private:
   typedef BHashMap<void*, void*> BPointerHashMap;
   BPointerHashMap mPointerHash;
   
   BMemStack& mMemStack;
};

template<class T> struct BPackHelper   { static bool pack(T& r, BPackState& s) { r; s; return true; } };
template<class T> struct BUnpackHelper { static bool unpack(T& r, const BDataBuffer& buf) { r; buf; return true; } };

#define DEFINE_BASIC_PACKABLE_TYPE(type) \
   template<> struct BPackHelper<type> { static bool pack(type& r, BPackState& s) { return r.pack(s); } }; \
   template<> struct BUnpackHelper<type> { static bool unpack(type& r, const BDataBuffer& buf) { return r.unpack(buf); } };
   
#define DEFINE_PACKABLE_TYPE(type) \
   template<bool BigEndian, bool Packed> struct BPackHelper< type<BigEndian, Packed> > { static bool pack( type<BigEndian, Packed>& r, BPackState& s) { return r.pack(s); } }; \
   template<bool BigEndian, bool Packed> struct BUnpackHelper< type<BigEndian, Packed> > { static bool unpack( type<BigEndian, Packed>& r, const BDataBuffer& buf) { return r.unpack(buf); } };

// To use this macro, ensure the Packed and BigEndian symbols are defined in the scope this macro appears in.
#define PACKED_TYPE(type) typename BIf< Packed, type, BPackedType<type, BigEndian > >::BResultType

#pragma warning( pop )

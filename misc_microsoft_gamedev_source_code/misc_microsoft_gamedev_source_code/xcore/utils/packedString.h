//------------------------------------------------------------------------------------------------------------------------
//  File: packableString.h
//
//  Copyright (c) 2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#pragma once
#include "packedType.h"
#include "dataBuffer.h"

#define BPACKED_STRING_ASSERT BDEBUG_ASSERT

class BPackedString 
{
public:
   BPackedString() { }
   BPackedString(const char* p) : mPtr(p) { }
   
   BPackedString& operator= (const char* p) { mPtr = p; return *this; }
   
   bool unpack(const BDataBuffer& buf) 
   { 
      if (mPtr == (const char*)0xFFFFFFFF) 
      {
         mPtr = NULL;
         return true;
      }
      
      if (((uint)mPtr >= buf.getLen()) || ((uint)mPtr & 1))
         return false;
      
      Utils::Pointerize(mPtr, buf.getPtr()); 
      
      return true;
   }
   
   operator const char* () const { return getPtr(); }
   
   const char* getPtr(void) const { return mPtr ? mPtr : ""; }
   const char* c_str(void) const { return getPtr(); }
   
   long length(void) const { return strlen(getPtr()); }
   bool isEmpty(void) const { return !length(); }
   bool getEmpty(void) const { return !length(); }
            
   bool operator== (const char* p) const { return _stricmp(mPtr, p) == 0; }
   bool operator!= (const char* p) const { return _stricmp(mPtr, p) != 0; }
   bool operator<  (const char* p) const { return _stricmp(mPtr, p) < 0; }
   bool operator>  (const char* p) const { return _stricmp(mPtr, p) > 0; }
   bool operator<= (const char* p) const { return _stricmp(mPtr, p) <= 0; }
   bool operator>= (const char* p) const { return _stricmp(mPtr, p) >= 0; }
   
   long compare(const char* pSrcString, bool caseSensitive = false, long srcCount = -1) const 
   { 
      long srcLength = strLength(pSrcString);

      if (isEmpty())
         return (srcLength == 0) ? 0 : -1;

      return strCompare(mPtr, length() + 1, pSrcString, srcLength, caseSensitive, srcCount);
   }

private:
   const char* mPtr;
};

template<bool BigEndian>
class BPackedStringBuilder
{
   friend class BPackedStringBuilder<!BigEndian>;
   
public:
   BPackedStringBuilder()
   {
      BCOMPILETIMEASSERT(sizeof(BPackedStringBuilder) == sizeof(BPackedString));
      setStrPtr(new BString);
   }
   
   BPackedStringBuilder(const char* pStr)
   {
      BCOMPILETIMEASSERT(sizeof(BPackedStringBuilder) == sizeof(BPackedString));
      setStrPtr(new BString(pStr));
   }
      
   BPackedStringBuilder(const BPackedStringBuilder& rhs)
   {
      if (rhs.getPacked())
         mPtr = rhs.mPtr;
      else
         setStrPtr(new BString(*rhs.getStrPtr()));
   }
   
   template<bool OtherBigEndian>
   BPackedStringBuilder(const BPackedStringBuilder<OtherBigEndian>& rhs)
   {
      BPACKED_STRING_ASSERT(!rhs.getPacked());
      
      setStrPtr(new BString(*rhs.getStrPtr()));
   }
   
   BPackedStringBuilder& operator=(const BPackedStringBuilder& rhs)
   {
      if (rhs.getPacked())
      {
         if (!getPacked())
            delete getStrPtr();
         
         mPtr = rhs.mPtr;
      }
      else
      {
         getStrPtr()->set(rhs.getPtr());
      }
      
      return *this;
   }
   
   template<bool OtherBigEndian>
   BPackedStringBuilder& operator=(const BPackedStringBuilder<OtherBigEndian>& rhs)
   {
      BPACKED_STRING_ASSERT(!getPacked() && !rhs.getPacked());
      
      getStrPtr()->set(rhs.getPtr());

      return *this;  
   }
   
   ~BPackedStringBuilder()
   {
      if (!getPacked())
         delete getStrPtr();
   }
   
   operator const char* () const { return getPtr(); }
   
   bool getPacked(void) const
   {
      return (((uint)(void*)mPtr & 1) == 0) || ((void*)mPtr == (void*)0xFFFFFFFF);
   }
   
   void set(const char* pStr)
   {
      getStrPtr()->set(pStr);
   }
   
   BPackedStringBuilder& operator= (const char* p) 
   {
      set(p);
      return *this;
   }
   
   void empty(void)
   {
      getStrPtr()->empty();
   }
   
   void toLower(void)
   {
      getStrPtr()->toLower();
   }
   
   void toUpper(void)
   {
      getStrPtr()->toUpper();
   }
   
   const char* getPtr(void) const
   {
      return getStrPtr()->getPtr();
   }
   
   const char* c_str(void) const
   {
      return getStrPtr()->getPtr();
   }
   
   long length(void) const { return getStrPtr()->length(); }
   bool isEmpty(void) const { return !length(); }
   bool getEmpty(void) const { return !length(); }
   
   bool operator== (const char* p) const { return _stricmp(getPtr(), p) == 0; }
   bool operator!= (const char* p) const { return _stricmp(getPtr(), p) != 0; }
   bool operator<  (const char* p) const { return _stricmp(getPtr(), p) < 0; }
   bool operator>  (const char* p) const { return _stricmp(getPtr(), p) > 0; }
   bool operator<= (const char* p) const { return _stricmp(getPtr(), p) <= 0; }
   bool operator>= (const char* p) const { return _stricmp(getPtr(), p) >= 0; }
   
   BPackedStringBuilder& operator+= (const BPackedStringBuilder& other)
   {
      getStrPtr()->append(other);
      return *this;
   }
   
   BPackedStringBuilder& operator+= (const char* pStr)
   {
      getStrPtr()->append(pStr);
      return *this;
   }
   
   bool pack(BPackState& state, bool offsetizePtr = true)
   {
      BPACKED_STRING_ASSERT(!getPacked());

      if (getPacked())
         return true;
         
      if (!length())
      {
         delete getStrPtr();    
         
         mPtr = NULL;
         
         if (offsetizePtr)
            offsetize(state.getMemStack().getBasePtr());         
         
         BPACKED_STRING_ASSERT(getPacked());

         return true;
      }
         
      uint sizeInBytes = length() + 1;
      
#ifndef BUILD_CHECKED      
      BPACKED_STRING_ASSERT(!IsBadReadPtr(getPtr(), sizeInBytes));
#endif      
      
      void* p = state.getMemStack().alignedAlloc(sizeInBytes, 2);
      if (!p)
         return false;
            
      memcpy(p, getPtr(), sizeInBytes);
      
      delete getStrPtr();
      
      mPtr = p;
      
      if (offsetizePtr)
         offsetize(state.getMemStack().getBasePtr());         
      
      BPACKED_STRING_ASSERT(getPacked());
      
      return true;
   }
   
   void clear(void)
   {
      getStrPtr()->empty();
   }

private:
   BPackedType<void*, BigEndian> mPtr;
   
   void setStrPtr(BString* p) 
   {
      BCOMPILETIMEASSERT(sizeof(BString*) == sizeof(uint));
      
      mPtr = (void*)((uint)p | 1);
   }
   
   BString* getStrPtr(void) const
   {
      BPACKED_STRING_ASSERT(!getPacked());
      
      return (BString*)((uint)((void*)mPtr) & ~1);
   }
   
   void offsetize(void* pBase)
   {
      if (mPtr == NULL)
         mPtr = reinterpret_cast<void*>(0xFFFFFFFF);
      else
         mPtr = reinterpret_cast<void*>(static_cast<uchar*>(static_cast<void*>(mPtr)) - static_cast<uchar*>(pBase));
   }

   void pointerize(void* pBase)
   {
      if (mPtr == reinterpret_cast<void*>(0xFFFFFFFF))
         mPtr = NULL;
      else
         mPtr = (reinterpret_cast<uint>(static_cast<void*>(mPtr)) + static_cast<uchar*>(pBase));
   }
};

#define PACKED_STRING typename BIf< Packed, BPackedString, BPackedStringBuilder< BigEndian > >::BResultType

template<bool BigEndian> struct BPackHelper< BPackedStringBuilder<BigEndian> > 
{ 
   static bool pack(BPackedStringBuilder<BigEndian>& r, BPackState& s) 
   { 
      return r.pack(s); 
   }
}; 

template<> struct BUnpackHelper< BPackedString > 
{ 
   static bool unpack( BPackedString& r, const BDataBuffer& buf) 
   { 
      return r.unpack(buf); 
   } 
};
//============================================================================
//
// File: nameValueMap.h
// Copyright (c) 2007, Ensemble Studios
//
// Basic name/value table with serialization support. 
//
//============================================================================
#pragma once

class BNameValueMap : public Utils::RelativeOperators<BNameValueMap>
{
public:
   BNameValueMap() :
      mSorted(false),
      mKeepSorted(false)
   {
   }
   
   BNameValueMap(const BNameValueMap& other)
   {
      mNameValues = other.mNameValues;
      mKeepSorted = other.mKeepSorted;
      mSorted = other.mSorted;
   }
   
   BNameValueMap& operator= (const BNameValueMap& rhs)
   {
      if (this != &rhs)
      {
         mNameValues = rhs.mNameValues;
         mKeepSorted = rhs.mKeepSorted;
         mSorted = rhs.mSorted;
      }
               
      return *this;
   }
   
   void clear()
   {
      mNameValues.clear();
      mSorted = false;
      mKeepSorted = false;
   }
   
   void setKeepSorted(bool keepSorted)
   { 
      if (mKeepSorted != keepSorted)
      {
         mKeepSorted = keepSorted;
         if (mKeepSorted)
            sort();
      }
   }
   
   bool getKeepSorted() const { return mKeepSorted; }
           
   void add(const char* pName, const char* pStr)
   {
      BNameValue* pV = add(pName);
      
      pV->mType = cTypeString;
      pV->mName.set(pName);
      pV->mValueString.set(pStr);
   }
   
   void add(const char* pName, int8 value)
   {
      BNameValue* pV = add(pName);

      pV->mType = cTypeInt8;
      pV->mName.set(pName);
      pV->mValueI8 = value;
   }

   void add(const char* pName, uint8 value)
   {
      BNameValue* pV = add(pName);

      pV->mType = cTypeUInt8;
      pV->mName.set(pName);
      pV->mValueU8 = value;
   }
   
   void add(const char* pName, int16 value)
   {
      BNameValue* pV = add(pName);

      pV->mType = cTypeInt16;
      pV->mName.set(pName);
      pV->mValueI16 = value;
   }

   void add(const char* pName, uint16 value)
   {
      BNameValue* pV = add(pName);

      pV->mType = cTypeUInt16;
      pV->mName.set(pName);
      pV->mValueU16 = value;
   }
   
   void add(const char* pName, int32 value)
   {
      BNameValue* pV = add(pName);
      
      pV->mType = cTypeInt32;
      pV->mName.set(pName);
      pV->mValueI32 = value;
   }
   
   void add(const char* pName, uint32 value)
   {
      BNameValue* pV = add(pName);
   
      pV->mType = cTypeUInt32;
      pV->mName.set(pName);
      pV->mValueU32 = value;
   }
   
   void add(const char* pName, int64 value)
   {
      BNameValue* pV = add(pName);

      pV->mType = cTypeInt64;
      pV->mName.set(pName);
      pV->mValueI64 = value;
   }
   
   void add(const char* pName, uint64 value)
   {
      BNameValue* pV = add(pName);

      pV->mType = cTypeUInt64;
      pV->mName.set(pName);
      pV->mValueI64 = value;
   }
   
   void add(const char* pName, bool value) 
   {
      BNameValue* pV = add(pName);

      pV->mType = cTypeBool;
      pV->mName.set(pName);
      pV->mValueBool = value;
   }
   
   void add(const char* pName, float value) 
   {
      BNameValue* pV = add(pName);

      pV->mType = cTypeFloat;
      pV->mName.set(pName);
      pV->mValueF32 = value;
   }
   
   void add(const char* pName, double value) 
   {
      BNameValue* pV = add(pName);

      pV->mType = cTypeDouble;
      pV->mName.set(pName);
      pV->mValueF64 = value;
   }
   
   void set(const char* pName, int8 v)
   {
      uint index;
      if (!find(pName, index))
         add(pName, v);
      else
      {
         mNameValues[index].mType = cTypeInt8;
         mNameValues[index].mValueString.empty();
         mNameValues[index].mValueI8 = v;
      }
   }

   void set(const char* pName, int16 v)
   {
      uint index;
      if (!find(pName, index))
         add(pName, v);
      else
      {
         mNameValues[index].mType = cTypeInt16;
         mNameValues[index].mValueString.empty();
         mNameValues[index].mValueI16 = v;
      }
   }

   void set(const char* pName, int32 v)
   {
      uint index;
      if (!find(pName, index))
         add(pName, v);
      else
      {
         mNameValues[index].mType = cTypeInt32;
         mNameValues[index].mValueString.empty();
         mNameValues[index].mValueI32 = v;
      }
   }

   void set(const char* pName, int64 v)
   {
      uint index;
      if (!find(pName, index))
         add(pName, v);
      else
      {
         mNameValues[index].mType = cTypeInt64;
         mNameValues[index].mValueString.empty();
         mNameValues[index].mValueI64 = v;
      }
   }

   void set(const char* pName, uint8 v)
   {
      uint index;
      if (!find(pName, index))
         add(pName, v);
      else
      {
         mNameValues[index].mType = cTypeUInt8;
         mNameValues[index].mValueString.empty();
         mNameValues[index].mValueU8 = v;
      }
   }

   void set(const char* pName, uint16 v)
   {
      uint index;
      if (!find(pName, index))
         add(pName, v);
      else
      {
         mNameValues[index].mType = cTypeUInt16;
         mNameValues[index].mValueString.empty();
         mNameValues[index].mValueU16 = v;
      }
   }

   void set(const char* pName, uint32 v)
   {
      uint index;
      if (!find(pName, index))
         add(pName, v);
      else
      {
         mNameValues[index].mType = cTypeUInt32;
         mNameValues[index].mValueString.empty();
         mNameValues[index].mValueU32 = v;
      }
   }

   void set(const char* pName, uint64 v)
   {
      uint index;
      if (!find(pName, index))
         add(pName, v);
      else
      {
         mNameValues[index].mType = cTypeUInt64;
         mNameValues[index].mValueString.empty();
         mNameValues[index].mValueU64 = v;
      }
   }

   void set(const char* pName, bool v)
   {
      uint index;
      if (!find(pName, index))
         add(pName, v);
      else
      {
         mNameValues[index].mType = cTypeBool;
         mNameValues[index].mValueString.empty();
         mNameValues[index].mValueBool = v;
      }
   }

   void set(const char* pName, float v)
   {
      uint index;
      if (!find(pName, index))
         add(pName, v);
      else
      {
         mNameValues[index].mType = cTypeFloat;
         mNameValues[index].mValueString.empty();
         mNameValues[index].mValueF32 = v;
      }
   }

   void set(const char* pName, double v)
   {
      uint index;
      if (!find(pName, index))
         add(pName, v);
      else
      {
         mNameValues[index].mType = cTypeDouble;
         mNameValues[index].mValueString.empty();
         mNameValues[index].mValueF64 = v;
      }
   }

   void set(const char* pName, const char* p)
   {
      uint index;
      if (!find(pName, index))
         add(pName, p);
      else
      {
         mNameValues[index].mType = cTypeString;
         mNameValues[index].mValueString.set(p);
      }
   }
   
   void sort()
   {
      mNameValues.sort();
      mSorted = true;
   }
   
   bool getSorted(void) const { return mSorted; }
         
   uint getSize() const { return mNameValues.getSize(); }
   uint getNumNameValues() const { return getSize(); }
   const BString& getName(uint index) const { return mNameValues[index].mName; }
         
   // Case sensitive!         
   bool find(const char* pName, uint& index, bool permitBinarySearching = true) const
   {
      index = 0;
      
      if ((permitBinarySearching) && (mSorted))
      {
         int low = 0;
         int high = mNameValues.getSize() - 1;
         while (high >= low)
         {
            const int mid = (low + high) >> 1;
            const int comp = mNameValues[mid].mName.compare(pName, true);
            
            if (comp == 0)
            {
               index = mid;
               return true;
            }
            else if (comp < 0)
               low = mid + 1;
            else
               high = mid - 1;
         }
      }
      else
      {
         for (uint i = 0; i < mNameValues.getSize(); i++)
         {
            if (mNameValues[i].mName.compare(pName, true) == 0)
            {
               index = i;
               return true;
            }
         }
      }
               
      return false;
   }
   
   bool doesExist(const char* pName, bool binarySearch = false) const
   {
      uint index;
      if (!find(pName, index, binarySearch))
         return false;
      index;
      return true;
   }
   
   uint getSerializeSize() const
   {
      if (getNumNameValues() > UINT16_MAX)
         return 0;
      
      uint totalSize = sizeof(WORD) * 2 + sizeof(BYTE);
      
      for (uint index = 0; index < getNumNameValues(); index++)
         totalSize += sizeof(BYTE) + (getName(index).length() + sizeof(BYTE)) + getDataSizeInBytes(index);
      
      return totalSize;
   }
   
   void* serialize(void* p, bool bigEndian = false) const
   {
      uchar* pDst = static_cast<uchar*>(p);
      
      writeObj(pDst, (WORD)getNumNameValues(), bigEndian);
      writeObj(pDst, (BYTE)mSorted, bigEndian);
      
      for (uint i = 0; i < getNumNameValues(); i++)
      {
         writeObj(pDst, (BYTE)getType(i), bigEndian);
         
         const BString& name = getName(i);
         uint nameSize = name.length() + 1;
         memcpy(pDst, name.getPtr(), nameSize);
         pDst += nameSize;
         
         const uint valueSize = getDataSizeInBytes(i);
         if ((getType(i) == cTypeString) || (bigEndian == cBigEndianNative))
            memcpy(pDst, getDataPtr(i), valueSize);
         else 
         {
            const uchar* pSrc = static_cast<const uchar*>(getDataPtr(i));
            for (uint i = 0; i < valueSize; i++)
               pDst[i] = pSrc[valueSize - 1 - i];
         }
         
         pDst += valueSize;
      }
      
      writeObj(pDst, (WORD)~(getNumNameValues()), bigEndian);
      
      return pDst;
   }
   
   bool deserialize(const void* p, uint bufLen)
   {
      const uchar* pSrcStart = static_cast<const uchar*>(p);
      const uchar* pSrc = pSrcStart;

      uint bytesRemaining = bufLen;
      if (bytesRemaining < (sizeof(WORD) * 2 + sizeof(BYTE)))
         return false;
      
      WORD size;
      if (!readObj(pSrc, size, bytesRemaining))
         return false;
      mNameValues.resize(size);
      
      BYTE sorted;
      if (!readObj(pSrc, sorted, bytesRemaining))
         return false;
      mSorted = (sorted != 0);
            
      for (uint i = 0; i < getNumNameValues(); i++)
      {
         BNameValue& nv = mNameValues[i];
         
         BYTE type;
         if (!readObj(pSrc, type, bytesRemaining))
            return false;
         if (type >= cTypeNum)
            return false;
         nv.mType = type;
         
         const uint nameLen = strlen((const char*)pSrc) + 1; 
         if (bytesRemaining < nameLen)
            return false;
         nv.mName.set((const char*)pSrc);
         pSrc += nameLen;
         bytesRemaining -= nameLen;
         
         if (type == cTypeString)
         {
            const uint valueLen = strlen((const char*)pSrc) + 1; 
            if (bytesRemaining < valueLen)
               return false;
            nv.mValueString.set((const char*)pSrc);
            pSrc += valueLen;
            bytesRemaining -= valueLen;
         }    
         else
         {
            nv.mValueString.empty();
            
            const uint typeSize = getTypeSize((eDataType)type);
            if (bytesRemaining < typeSize)
               return false;
            
            nv.mValueU64 = 0;
            memcpy(&nv.mValueU64, pSrc, typeSize);
            pSrc += typeSize;
            bytesRemaining -= typeSize;
         }            
      }
      
      WORD invSize;
      if (!readObj(pSrc, invSize, bytesRemaining))
         return false;
         
      if ((mKeepSorted) && (!mSorted))
         sort();
      
      return (bytesRemaining >= 0) && (invSize == (WORD)(~size));
   }
   
   enum eDataType
   {
      cTypeString,
      cTypeFloat,
      cTypeDouble,
      cTypeInt8,
      cTypeUInt8,
      cTypeInt16,
      cTypeUInt16,
      cTypeInt32,
      cTypeUInt32,
      cTypeInt64,
      cTypeUInt64,
      cTypeBool,
      
      cTypeNum
   };
   
   static bool isTypeString(eDataType type) { return type == cTypeString; }
   static bool isTypeBool(eDataType type) { return type == cTypeBool; }
   static bool isTypeFloatingPoint(eDataType type) { return (type == cTypeFloat) || (type == cTypeDouble); }
   
   static bool isTypeNumeric(eDataType type) 
   {
      switch (type)
      {
         case cTypeFloat:
         case cTypeDouble:
         case cTypeInt8:
         case cTypeUInt8:
         case cTypeInt16:
         case cTypeUInt16:
         case cTypeInt32:
         case cTypeUInt32:
         case cTypeInt64:
         case cTypeUInt64:
         case cTypeBool:
            return true;
      }
      return false;
   }
   
   static bool isTypeInteger(eDataType type) 
   {
      switch (type)
      {
         case cTypeInt8:
         case cTypeUInt8:
         case cTypeInt16:
         case cTypeUInt16:
         case cTypeInt32:
         case cTypeUInt32:
         case cTypeInt64:
         case cTypeUInt64:
         case cTypeBool:
            return true;
      }
      return false;
   }
   
   static bool isTypeSigned(eDataType type) 
   {
      switch (type)
      {
         case cTypeFloat:
         case cTypeDouble:
         case cTypeInt8:
         case cTypeInt16:
         case cTypeInt32:
         case cTypeInt64:
            return true;
      }
      return false;
   }
   
   static bool isTypeUnsigned(eDataType type) 
   {
      switch (type)
      {
         case cTypeUInt8:
         case cTypeUInt16:
         case cTypeUInt32:
         case cTypeUInt64:
         case cTypeBool:
            return true;
      }
      return false;
   }
   
   static uint getTypeSize(eDataType type) 
   {
      switch (type)
      {
         case cTypeFloat: 
         case cTypeInt32:
         case cTypeUInt32:
            return sizeof(DWORD);
         case cTypeInt16:
         case cTypeUInt16:
            return sizeof(WORD);
         case cTypeDouble:
         case cTypeInt64:
         case cTypeUInt64:
            return sizeof(uint64);
         case cTypeInt8:
         case cTypeUInt8:
         case cTypeBool:
            return 1;
      }
      
      return 0;            
   }
   
   eDataType getType(uint index) const { return (eDataType)mNameValues[index].mType; }
   
   uint getDataSizeInBytes(uint index) const
   {
      eDataType type = getType(index);
      
      if (cTypeString == type)
         return getString(index).length() + 1;
      else
         return getTypeSize(type);
   }
   
   const void* getDataPtr(uint index) const
   {
      if (getType(index) == cTypeString)
         return mNameValues[index].mValueString.getPtr();
      else
         return &mNameValues[index].mValueU64;
   }
         
   const BString& getString(uint index) const 
   {
      BDEBUG_ASSERT(mNameValues[index].mType == cTypeString);
      return mNameValues[index].mValueString;
   }
   
   bool get(const char* pName, BString& value) const 
   {
      uint index;
      if (!find(pName, index))
      {
         value.empty();
         return false;
      }
      if (!isTypeString(getType(index)))
         return false;
      value = getString(index);
      return true;
   }
   
   bool get(const char* pName, float& value) const 
   {
      value = 0.0f;
      uint index;
      if (!find(pName, index))
         return false;
      if (!isTypeNumeric(getType(index)))
         return false;
      value = getFloat(index);
      return true;
   }
   
   bool get(const char* pName, double& value) const 
   {
      value = 0.0f;
      uint index;
      if (!find(pName, index))
         return false;
      if (!isTypeNumeric(getType(index)))
         return false;
      value = getDouble(index);
      return true;
   }
   
   bool get(const char* pName, int8& value) const 
   {
      value = 0;
      uint index;
      if (!find(pName, index))
         return false;
      if (!isTypeNumeric(getType(index)))
         return false;
      int64 v = getSigned(index);
      if ((v < INT8_MIN) || (v > INT8_MAX))
         return false;
      value = (int8)v;
      return true;
   }
         
   bool get(const char* pName, int16& value) const 
   {
      value = 0;
      uint index;
      if (!find(pName, index))
         return false;
      if (!isTypeNumeric(getType(index)))
         return false;
      int64 v = getSigned(index);
      if ((v < INT16_MIN) || (v > INT16_MAX))
         return false;
      value = (int16)v;
      return true;
   }
   
   bool get(const char* pName, int32& value) const 
   {
      value = 0;
      uint index;
      if (!find(pName, index))
         return false;
      if (!isTypeNumeric(getType(index)))
         return false;
      int64 v = getSigned(index);
      if ((v < INT32_MIN) || (v > INT32_MAX))
         return false;
      value = (int32)v;
      return true;
   }
   
   bool get(const char* pName, int64& value) const 
   {
      value = 0;
      uint index;
      if (!find(pName, index))
         return false;
      if (!isTypeNumeric(getType(index)))
         return false;
      value = getSigned(index);
      return true;
   }
   
   bool get(const char* pName, uint8& value) const 
   {
      value = 0;
      uint index;
      if (!find(pName, index))
         return false;
      if (!isTypeNumeric(getType(index)))
         return false;
      uint64 v = getUnsigned(index);
      if (v > UINT8_MAX)
         return false;
      value = (uint8)v;
      return true;
   }
   
   bool get(const char* pName, uint16& value) const 
   {
      value = 0;
      uint index;
      if (!find(pName, index))
         return false;
      if (!isTypeNumeric(getType(index)))
         return false;
      uint64 v = getUnsigned(index);
      if (v > UINT16_MAX)
         return false;
      value = (uint16)v;
      return true;
   }
   
   bool get(const char* pName, uint32& value) const 
   {
      value = 0;
      uint index;
      if (!find(pName, index))
         return false;
      if (!isTypeNumeric(getType(index)))
         return false;
      uint64 v = getUnsigned(index);
      if (v > UINT32_MAX)
         return false;
      value = (uint32)v;
      return true;
   }
   
   bool get(const char* pName, uint64& value) const 
   {
      value = 0;
      uint index;
      if (!find(pName, index))
         return false;
      if (!isTypeNumeric(getType(index)))
         return false;
      value = getUnsigned(index);
      return true;
   }
   
   bool get(const char* pName, bool& value) const
   {
      value = false;
      uint index;
      if (!find(pName, index))
         return false;
      if (!isTypeNumeric(getType(index)))
         return false;
      value = (getSigned(index) != 0);
      return true;
   }
   
   uint64 getUInt64(const char* pName, uint64 def = 0) const { uint64 v; if (!get(pName, v)) return def; return v; }
   int64 getInt64(const char* pName, int64 def = 0) const { int64 v; if (!get(pName, v)) return def; return v; }

   uint32 getUInt32(const char* pName, uint32 def = 0) const { uint32 v; if (!get(pName, v)) return def; return v; }
   int32 getInt32(const char* pName, int32 def = 0) const { int32 v; if (!get(pName, v)) return def; return v; }

   uint16 getUInt16(const char* pName, const uint16 def = 0) const { uint16 v; if (!get(pName, v)) return def; return v; }
   int16 getInt16(const char* pName, const int16 def = 0) const { int16 v; if (!get(pName, v)) return def; return v; }

   uint8 getUInt8(const char* pName, const uint8 def = 0) const { uint8 v; if (!get(pName, v)) return def; return v; }
   int8 getInt8(const char* pName, const int8 def = 0) const { int8 v; if (!get(pName, v)) return def; return v; }   

   bool getBool(const char* pName, const bool def = false) const { bool v; if (!get(pName, v)) return def; return v; }
   BString getString(const char* pName, const BString& def = BString()) const { BString v; if (!get(pName, v)) return def; return v; }
   
   float getFloat(const char* pName, float def = 0.0f) const { float f; if (!get(pName, f)) return def; return f; }
   double getDouble(const char* pName, double def = 0.0f) const { double f; if (!get(pName, f)) return def; return f; }
                  
   float getFloat(uint index) const 
   {
      switch (mNameValues[index].mType)
      {
         case cTypeFloat:
            return mNameValues[index].mValueF32;  
         case cTypeDouble:
            return (float)mNameValues[index].mValueF64;  
         case cTypeInt32:
            return (float)mNameValues[index].mValueI32;  
         case cTypeUInt32:
            return (float)mNameValues[index].mValueU32;  
         case cTypeInt8:
            return (float)mNameValues[index].mValueI8;  
         case cTypeInt16:
            return (float)mNameValues[index].mValueI16;  
         case cTypeUInt8:
            return (float)mNameValues[index].mValueU8;  
         case cTypeUInt16:
            return (float)mNameValues[index].mValueU16;  
         case cTypeInt64:
            return (float)mNameValues[index].mValueI64;  
         case cTypeUInt64:
            return (float)mNameValues[index].mValueU64;  
         case cTypeBool:
            return (float)mNameValues[index].mValueBool;
      }
      
      BDEBUG_ASSERT(0);
      return 0.0f;
   }
   
   double getDouble(uint index) const 
   {
      switch (mNameValues[index].mType)
      {
         case cTypeFloat:
            return mNameValues[index].mValueF32;  
         case cTypeDouble:
            return mNameValues[index].mValueF64;  
         case cTypeInt32:
            return (double)mNameValues[index].mValueI32;  
         case cTypeUInt32:
            return (double)mNameValues[index].mValueU32;  
         case cTypeInt8:
            return (double)mNameValues[index].mValueI8;  
         case cTypeInt16:
            return (double)mNameValues[index].mValueI16;  
         case cTypeUInt8:
            return (double)mNameValues[index].mValueU8;  
         case cTypeUInt16:
            return (double)mNameValues[index].mValueU16;  
         case cTypeInt64:
            return (double)mNameValues[index].mValueI64;  
         case cTypeUInt64:
            return (double)mNameValues[index].mValueU64;  
         case cTypeBool:
            return (double)mNameValues[index].mValueBool;
      }
      
      BDEBUG_ASSERT(0);
      return 0.0f;
   }
   
   int64 getSigned(uint index) const 
   {
      switch (mNameValues[index].mType)
      {
         case cTypeFloat:
            return (int64)mNameValues[index].mValueF32;  
         case cTypeDouble:
            return (int64)mNameValues[index].mValueF64;  
         case cTypeInt32:
            return (int64)mNameValues[index].mValueI32;  
         case cTypeUInt32:
            return (int64)mNameValues[index].mValueU32;  
         case cTypeInt8:
            return (int64)mNameValues[index].mValueI8; 
         case cTypeInt16:
            return (int64)mNameValues[index].mValueI16;  
         case cTypeUInt8:
            return (int64)mNameValues[index].mValueU8;  
         case cTypeUInt16:
            return (int64)mNameValues[index].mValueU16;  
         case cTypeInt64:
            return (int64)mNameValues[index].mValueI64;  
         case cTypeUInt64:
            return (int64)mNameValues[index].mValueU64;  
         case cTypeBool:
            return (int64)mNameValues[index].mValueBool;
      }
      
      BDEBUG_ASSERT(0);
      return 0;
   }
   
   uint64 getUnsigned(uint index) const 
   {
      switch (mNameValues[index].mType)
      {
         case cTypeFloat:
            return (uint64)mNameValues[index].mValueF32;  
         case cTypeDouble:
            return (uint64)mNameValues[index].mValueF64;  
         case cTypeInt32:
            return (uint64)mNameValues[index].mValueI32;  
         case cTypeUInt32:
            return (uint64)mNameValues[index].mValueU32;  
         case cTypeInt8:
            return (uint64)mNameValues[index].mValueI8;  
         case cTypeInt16:
            return (uint64)mNameValues[index].mValueI16;  
         case cTypeUInt8:
            return (uint64)mNameValues[index].mValueU8;  
         case cTypeUInt16:
            return (uint64)mNameValues[index].mValueU16;  
         case cTypeInt64:
            return (uint64)mNameValues[index].mValueI64;  
         case cTypeUInt64:
            return (uint64)mNameValues[index].mValueU64;  
         case cTypeBool:
            return (uint64)mNameValues[index].mValueBool;
      }
      
      BDEBUG_ASSERT(0);
      return 0;
   }
   
   BString& convertToString(uint index, BString& str) const
   {
      const eDataType dataType = getType(index);
      if (isTypeString(dataType))
         str.set(mNameValues[index].mValueString);
      else if (isTypeFloatingPoint(dataType))
         str.format("%f", getDouble(index));
      else if (isTypeUnsigned(dataType))
         str.format("%u", getUnsigned(index));
      else if (isTypeSigned(dataType))
         str.format("%i", getSigned(index));
      else
         str = "?";
      return str;         
   }
   
   bool operator== (const BNameValueMap& rhs) const
   {
      const BNameValueMap& lhs = *this;
      if (lhs.getNumNameValues() != rhs.getNumNameValues())
         return false;
      
      for (uint lhsIndex = 0; lhsIndex < lhs.getNumNameValues(); lhsIndex++)
      {
         uint rhsIndex;
         if (!rhs.find(lhs.getName(lhsIndex), rhsIndex))
            return false;
         
         if (lhs.getType(lhsIndex) != rhs.getType(rhsIndex))
            return false;
         
         const uint dataSizeInBytes = lhs.getDataSizeInBytes(lhsIndex);
         if (dataSizeInBytes != rhs.getDataSizeInBytes(rhsIndex))
            return false;
         
         if (memcmp(lhs.getDataPtr(lhsIndex), rhs.getDataPtr(rhsIndex), dataSizeInBytes) != 0)
            return false;
      }
      
      return true;
   }
   
   // This is a value AND type wise operator less.
   bool operator< (const BNameValueMap& rhs) const
   {
      const BNameValueMap& lhs = *this;
                  
      BDynamicArray<BString> uniqueNames;
            
      for (uint lhsIndex = 0; lhsIndex < lhs.getNumNameValues(); lhsIndex++)
         uniqueNames.pushBack(lhs.getName(lhsIndex));
         
      for (uint rhsIndex = 0; rhsIndex < rhs.getNumNameValues(); rhsIndex++)
         uniqueNames.pushBack(rhs.getName(rhsIndex));
         
      uniqueNames.sort();
      uniqueNames.removeDuplicates();
      
      for (uint i = 0; i < uniqueNames.getSize(); i++)
      {
         const BString& name = uniqueNames[i];
           
         uint lhsIndex, rhsIndex;
         const bool foundLhs = lhs.find(name, lhsIndex);
         const bool foundRhs = rhs.find(name, rhsIndex);
         
         if ((!foundLhs) || (!foundRhs))
         {
            if (foundRhs)
               return true;
            else
               return false;
         }
         
         const eDataType dataType = getType(lhsIndex);
         if (dataType != rhs.getType(rhsIndex))
            return dataType < rhs.getType(rhsIndex);

         const uint dataSizeInBytes = lhs.getDataSizeInBytes(lhsIndex);
         if (dataSizeInBytes != rhs.getDataSizeInBytes(rhsIndex))
            return dataSizeInBytes < rhs.getDataSizeInBytes(rhsIndex);
         
         int compResult = 0;
         switch (dataType)
         {
            case cTypeString:
            {
               compResult = lhs.getString(lhsIndex).compare(rhs.getString(rhsIndex), true);
               break;
            }
            case cTypeFloat:
            case cTypeDouble:
            {
               const double l = lhs.getDouble(lhsIndex);
               const double r = rhs.getDouble(rhsIndex);
               if (l < r)
                  compResult = -1;
               else if (l > r)
                  compResult = 1;
               break;
            }
            case cTypeInt8:
            case cTypeInt16:
            case cTypeInt32:
            case cTypeInt64:
            {
               const int64 l = lhs.getSigned(lhsIndex);
               const int64 r = rhs.getSigned(rhsIndex);
               if (l < r)
                  compResult = -1;
               else if (l > r)
                  compResult = 1;
               break;
            }
            case cTypeUInt8:
            case cTypeUInt16:
            case cTypeUInt32:
            case cTypeUInt64:
            case cTypeBool:
            {
               const uint64 l = lhs.getUnsigned(lhsIndex);
               const uint64 r = rhs.getUnsigned(rhsIndex);
               if (l < r)
                  compResult = -1;
               else if (l > r)
                  compResult = 1;
               break;
            }
            default:
            {
               BDEBUG_ASSERT(0);
               return false;
            }
         }
         
         if (compResult != 0)
            return compResult < 0;
      }

      return false;
   }
   
   // Hash operator
   operator size_t() const
   {
      const uint numValues = mNameValues.getSize();
      
      size_t hash = hashFast(&numValues, sizeof(numValues));
      
      for (uint i = 0; i < numValues; i++)
      {
         const BNameValue& nameValue = mNameValues[i];
         
         hash = hashFast(nameValue.mName.getPtr(), nameValue.mName.length(), hash);
         hash = hashFast(&nameValue.mType, sizeof(nameValue.mType), hash);
         hash = hashFast(getDataPtr(i), getDataSizeInBytes(i), hash);
      }

      return hash;
   }

private:
   struct BNameValue
   {
      BNameValue() : mType(0), mValueU64(0) { }
      
      BNameValue(const BNameValue& other) :
         mType(other.mType),
         mName(other.mName),
         mValueString(other.mValueString),
         mValueU64(other.mValueU64)
      {
      }
      
      BNameValue& operator= (const BNameValue& rhs) 
      {
         if (this != &rhs)
         {
            mType = rhs.mType;
            mName = rhs.mName;
            mValueString = rhs.mValueString;
            mValueU64 = rhs.mValueU64;
         }
         return *this;
      }
      
      uint     mType;
      BString  mName;
      BString  mValueString;

      union
      {
         uint64   mValueU64;
         int64    mValueI64;
         double   mValueF64;
         
         uint32   mValueU32;
         int32    mValueI32;
         
         BOOL     mValueBool;
         float    mValueF32;
         
         uint16   mValueU16;
         int16    mValueI16;
         
         uint8    mValueU8;
         int8     mValueI8;
      };
      
      bool operator< (const BNameValue& rhs) const { return mName.compare(rhs.mName, true) < 0; }
   };

   typedef BDynamicArray<BNameValue> BNameValueArray;
   
   BNameValueArray   mNameValues;
   bool              mSorted : 1;
   bool              mKeepSorted : 1;
   
   template<class T>
   static void writeObj(uchar* & pDst, const T& obj, bool bigEndian)
   {
      const uint size = sizeof(obj);
      
      if (cBigEndianNative != bigEndian)
      {
         const uchar* pSrc = reinterpret_cast<const uchar*>(&obj);
         for (uint i = 0; i < sizeof(obj); i++)
            pDst[i] = pSrc[size - 1 - i];
      }
      else
      {
         memcpy(pDst, &obj, size);
      }
      pDst += sizeof(obj);
   }

   template<class T>
   static bool readObj(const uchar* & pSrc, T& obj, uint& bytesRemaining)
   {
      if (bytesRemaining < sizeof(obj))
         return false;
      
      bytesRemaining -= sizeof(obj);
      memcpy(&obj, pSrc, sizeof(obj));
      pSrc += sizeof(obj);
      
      return true;
   }
   
   uint findInsertionPoint(const char* pName) const
   {
      BDEBUG_ASSERT(pName);
      
      const uint numChildren = mNameValues.getSize();
      if (!numChildren)
         return 0;

      int l = 0;
      int r = numChildren - 1;
      int m = 0;
      int compResult = 0;
      while (r >= l)
      {
         m = (l + r) >> 1;
         
         compResult = mNameValues[m].mName.compare(pName, true);

         if (!compResult)
            return m;
         else if (compResult > 0)
            r = m - 1;
         else
            l = m + 1;
      }

      int result;
      if (compResult > 0)
         result = m;
      else
         result = m + 1;

      BDEBUG_ASSERT((result >= 0) && (result <= (int)mNameValues.getSize()));

      return result;
   }
   
   BNameValue* add(const char* pName)
   {
      if (mKeepSorted)
      {
         BDEBUG_ASSERT(mSorted);
         const uint index = findInsertionPoint(pName);
         mNameValues.insert(index, 1, NULL);
         return &mNameValues[index];         
      }
      else
      {
         mSorted = false;
         return &mNameValues.grow();
      }
   }
};
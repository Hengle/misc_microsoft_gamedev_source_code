//============================================================================
//
// File: binaryDataTreeReader.cpp
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "binaryDataTree.h"
#include "binaryDataTreeReader.h"
#include "hash\crc.h"
#include "nameValueMap.h"

namespace BBinaryDataTree
{
   BPackedDocumentReader::BPackedDocumentReader()
   {
      clear();
   }
   
   BPackedDocumentReader::BPackedDocumentReader(const BConstDataBuffer& dataBuffer) 
   {
      set(dataBuffer);
   }
   
   bool BPackedDocumentReader::set(const BConstDataBuffer& dataBuffer)
   {
      if (!setInternal(dataBuffer))
      {
         clear();
         return false;
      }
      return true;
   }
   
   bool BPackedDocumentReader::setInternal(const BConstDataBuffer& dataBuffer)
   {
      mDataBuffer = dataBuffer;
            
      if ( (!mDataBuffer.getPtr()) || 
           (mDataBuffer.getLen() < sizeof(BPackedHeader)) )
         return false;

      const BPackedHeader& header = *reinterpret_cast<const BPackedHeader*>(mDataBuffer.getPtr());

      const uint expectedSig = cBigEndianNative ? BPackedHeader::cBigEndianSig : BPackedHeader::cLittleEndianSig;
      if (header.mSig != expectedSig)
         return false;  
      
      BPackedHeader tempHeader(header);
      tempHeader.mHeaderCRC8 = 0;
      if (((uint8)calcCRC16(&tempHeader, sizeof(tempHeader)) != header.mHeaderCRC8))
         return false;
      
      if (mDataBuffer.getLen() < (sizeof(BPackedHeader) + header.mNumUserSections * sizeof(BPackedSection)))
         return false;
      
      mpSections = reinterpret_cast<const BPackedSection*>(mDataBuffer.getPtr() + sizeof(BPackedHeader));
      
      const uint totalSize = header.mDataSize + sizeof(BPackedHeader);
      if (mDataBuffer.getLen() < totalSize)
         return false;
         
      if (calcCRC32(mDataBuffer.getPtr() + sizeof(BPackedHeader), header.mDataSize) != header.mDataCRC32)
         return false;
      
      for (uint i = 0; i < header.mNumUserSections; i++)
         if ((mpSections[i].mOfs + mpSections[i].mSize) > mDataBuffer.getLen())
            return false;         
      
      uint curOfs = sizeof(header) + sizeof(BPackedSection) * header.mNumUserSections;

      const uint nodeOfs = header.mBaseSectionSizes[cNodeSectionIndex] ? curOfs : 0;
      curOfs += header.mBaseSectionSizes[cNodeSectionIndex];

      const uint nameValueOfs = header.mBaseSectionSizes[cNameValueSectionIndex] ? curOfs : 0;
      curOfs += header.mBaseSectionSizes[cNameValueSectionIndex];

      const uint nameDataOfs = header.mBaseSectionSizes[cNameDataSectionIndex] ? curOfs : 0;
      curOfs += header.mBaseSectionSizes[cNameDataSectionIndex];

      if (header.mBaseSectionSizes[cValueDataSectionIndex])
         curOfs += Utils::BytesToAlignUpValue(curOfs, 16);

      const uint valueDataOfs = header.mBaseSectionSizes[cValueDataSectionIndex] ? curOfs : 0;
      curOfs += header.mBaseSectionSizes[cValueDataSectionIndex];
      if (curOfs > mDataBuffer.getLen())
         return false;

      //const BYTE* pNameData   = nameDataOfs ? (pSrc + nameDataOfs) : NULL;
      //const BYTE* pValueData  = valueDataOfs ? (pSrc + valueDataOfs) : NULL;

      mNumNodes = header.mBaseSectionSizes[cNodeSectionIndex] / sizeof(BPackedNode);
      mpNodes = header.mBaseSectionSizes[cNodeSectionIndex] ? reinterpret_cast<const BPackedNode*>(mDataBuffer.getPtr() + nodeOfs) : NULL;

      mNumNameValues = header.mBaseSectionSizes[cNameValueSectionIndex] / sizeof(BPackedNameValue);
      mpNameValues = header.mBaseSectionSizes[cNameValueSectionIndex] ? reinterpret_cast<const BPackedNameValue*>(mDataBuffer.getPtr() + nameValueOfs) : NULL;
         
      mNameDataSize = header.mBaseSectionSizes[cNameDataSectionIndex];
      mpNameData   = mNameDataSize ? (mDataBuffer.getPtr() + nameDataOfs) : NULL;
      
      mValueDataSize = header.mBaseSectionSizes[cValueDataSectionIndex];
      mpValueData  = mValueDataSize ? (mDataBuffer.getPtr() + valueDataOfs) : NULL;
      
      // All documents must have at least a root node.
      if (!mNumNodes)
         return false;
               
      return true;
   }
   
   void BPackedDocumentReader::clear()
   {
      mDataBuffer.clear();
      
      mpSections = NULL;
      
      mNumNodes = 0;
      mpNodes = NULL;

      mNumNameValues = 0;
      mpNameValues = NULL;
      
      mpNameData = NULL;
      mpValueData = NULL;
   }
   
   uint BPackedDocumentReader::getNumUserSections() const
   {
      if (!isValid())
         return 0;
      
      return getHeader()->mNumUserSections;
   }
   
   uint32 BPackedDocumentReader::getUserSectionID(uint index) const
   {
      if ((!isValid()) || (index >= getNumUserSections()))
         return UINT_MAX;
      
      return mpSections[index].mID;
   }
   
   uint32 BPackedDocumentReader::getUserSectionSize(uint index) const
   {
      if ((!isValid()) || (index >= getNumUserSections()))
         return 0;
      
      return mpSections[index].mSize;
   }
   
   const void* BPackedDocumentReader::getUserSectionPtr(uint index) const
   {
      if ((!isValid()) || (index >= getNumUserSections()))
         return NULL;
      
      return mDataBuffer.getPtr() + mpSections[index].mOfs;
   }
   
   BPackedDocumentReader::BNode BPackedDocumentReader::getRoot() const
   {
      if (!isValid())
         return BNode(NULL, UINT_MAX);
      
      return mNumNodes ? BNode(this, 0) : BNode(NULL, UINT_MAX);
   }
   
   BPackedDocumentReader::BNode BPackedDocumentReader::getNode(uint nodeIndex) const
   {
      if ((!isValid()) || (nodeIndex >= mNumNodes))
         return BNode(NULL, UINT_MAX);

      return BNode(this, nodeIndex);
   }
     
   const char* BPackedDocumentReader::BValue::getName() const
   {
      if (!isValid())
         return NULL;

      return reinterpret_cast<const char*>(mpReader->mpNameData + mpReader->mpNameValues[mNameValueIndex].mNameOfs);
   }
   
   uint BPackedDocumentReader::BValue::getDataSizeUnchecked() const
   {
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];

      uint dataSize = (nameValue.mFlags & BPackedNameValue::cSizeMask) >> BPackedNameValue::cSizeShift;
      if (dataSize == ((1U << BPackedNameValue::cSizeBits) - 1U))
      {
         const uint32* p = static_cast<const uint32*>(getDataPtrUnchecked());
         dataSize = p[-1];
      }

      return dataSize;
   }

   uint BPackedDocumentReader::BValue::getDataSize() const
   {
      if (!isValid())
         return 0;
      
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      
      uint dataSize = (nameValue.mFlags & BPackedNameValue::cSizeMask) >> BPackedNameValue::cSizeShift;
      if (dataSize == ((1U << BPackedNameValue::cSizeBits) - 1U))
      {
         const uint32* p = static_cast<const uint32*>(getDataPtr());
         BDEBUG_ASSERT(p);
         if (!p)
            return 0;
         
         dataSize = p[-1];
         if (dataSize < (1U << BPackedNameValue::cSizeBits))
         {
            BDEBUG_ASSERT(0);
            dataSize = 0;
         }
      }
      
      return dataSize;
   }
   
   const void* BPackedDocumentReader::BValue::getDataPtrUnchecked() const
   {
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];

      if (nameValue.mFlags & BPackedNameValue::cDirectEncodingMask)
      {
         const BYTE* p = reinterpret_cast<const BYTE*>(&nameValue.mValue);

         if (cBigEndianNative)
         {
            const uint typeSize = getTypeSize();         

            if (typeSize == sizeof(uint16))
               p += 2;
            else if (typeSize == sizeof(uint8))
               p += 3;
         }

         return p;
      }

      return mpReader->mpValueData + nameValue.mValue;
   }
   
   const void* BPackedDocumentReader::BValue::getDataPtr() const
   {
      if (!isValid())
         return NULL;
               
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      
      if ((nameValue.mFlags & BPackedNameValue::cTypeMask) == (cTypeNull << BPackedNameValue::cTypeShift))
         return NULL;
      
      if (nameValue.mFlags & BPackedNameValue::cDirectEncodingMask)
      {
         const BYTE* p = reinterpret_cast<const BYTE*>(&nameValue.mValue);
         
         if (cBigEndianNative)
         {
            const uint typeSize = getTypeSize();         
            
            if (typeSize == sizeof(uint32))
            {
            }
            else if (typeSize == sizeof(uint16))
               p += 2;
            else if (typeSize == sizeof(uint8))
               p += 3;
            else
            {
               BDEBUG_ASSERT(0);
               p = NULL;
            }
         }
         
         return p;
      }
      
      if (nameValue.mValue >= mpReader->mValueDataSize)
      {
         BDEBUG_ASSERT(0);
         return NULL;
      }
      
      return mpReader->mpValueData + nameValue.mValue;
   }
   
   eTypeClass BPackedDocumentReader::BValue::getTypeClassUnchecked() const
   {
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      return static_cast<eTypeClass>((nameValue.mFlags & BPackedNameValue::cTypeMask) >> BPackedNameValue::cTypeShift);
   }

   eTypeClass BPackedDocumentReader::BValue::getTypeClass() const
   {
      if (!isValid())
         return cTypeNull;
      
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      
      eTypeClass typeClass = static_cast<eTypeClass>((nameValue.mFlags & BPackedNameValue::cTypeMask) >> BPackedNameValue::cTypeShift);
      if (typeClass >= cNumTypes)
      {
         BDEBUG_ASSERT(0);
         return cTypeNull;
      }
      
      return typeClass;
   }

   bool BPackedDocumentReader::BValue::isNull() const
   {
      return (cTypeNull == getTypeClassUnchecked());
   }
      
   bool BPackedDocumentReader::BValue::isBool() const
   {
      return (cTypeBool == getTypeClassUnchecked());
   }
   
   bool BPackedDocumentReader::BValue::isNumeric() const
   {
      switch (getTypeClassUnchecked())
      {
         case cTypeInt:
         case cTypeFloat:
            return true;
      }
      return false;
   }
   
   bool BPackedDocumentReader::BValue::isFloat() const
   {
      return (cTypeFloat == getTypeClassUnchecked());
   }
   
   bool BPackedDocumentReader::BValue::isUnsigned() const
   {
      if (!isValid())
         return false;
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      return (nameValue.mFlags & BPackedNameValue::cTypeIsUnsignedMask) != 0;
   }
   
   bool BPackedDocumentReader::BValue::isString() const
   {
      return (cTypeString == getTypeClassUnchecked());
   }
   
   uint BPackedDocumentReader::BValue::getTypeSizeLog2Unchecked() const
   {
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      return (nameValue.mFlags & BPackedNameValue::cTypeSizeInBytesLog2Mask) >> BPackedNameValue::cTypeSizeInBytesLog2Shift;
   }

   uint BPackedDocumentReader::BValue::getTypeSizeLog2() const
   {
      if (!isValid())
         return 0;
      
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      return (nameValue.mFlags & BPackedNameValue::cTypeSizeInBytesLog2Mask) >> BPackedNameValue::cTypeSizeInBytesLog2Shift;
   }
   
   uint BPackedDocumentReader::BValue::getTypeSizeUnchecked() const
   {
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      eTypeClass typeClass = static_cast<eTypeClass>((nameValue.mFlags & BPackedNameValue::cTypeMask) >> BPackedNameValue::cTypeShift);
      if (cTypeNull == typeClass)
         return 0;

      return 1U << ((nameValue.mFlags & BPackedNameValue::cTypeSizeInBytesLog2Mask) >> BPackedNameValue::cTypeSizeInBytesLog2Shift);
   }
   
   uint BPackedDocumentReader::BValue::getTypeSize() const
   {
      if (!isValid())
         return 0;

      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      eTypeClass typeClass = static_cast<eTypeClass>((nameValue.mFlags & BPackedNameValue::cTypeMask) >> BPackedNameValue::cTypeShift);
      if (cTypeNull == typeClass)
         return 0;
         
      return 1U << ((nameValue.mFlags & BPackedNameValue::cTypeSizeInBytesLog2Mask) >> BPackedNameValue::cTypeSizeInBytesLog2Shift);
   }

   uint BPackedDocumentReader::BValue::getArraySize() const
   {
      if (!isValid())
         return 0;
            
      uint arraySize = 0;
      
      const eTypeClass typeClass = getTypeClassUnchecked();
      if (cTypeNull == typeClass)
      {
      }
      else if (cTypeString == typeClass)
      {
         if (getTypeSizeLog2Unchecked() == 1)
         {
            if (!BStringArrayHelpers::getSize(static_cast<const wchar_t*>(getDataPtr()), getDataSize(), arraySize))
            {
               BDEBUG_ASSERT(0);
            }
         }
         else
         {
            if (!BStringArrayHelpers::getSize(static_cast<const char*>(getDataPtr()), getDataSize(), arraySize))
            {
               BDEBUG_ASSERT(0);
            }
         }
      }
      else
         arraySize = getDataSizeUnchecked() >> getTypeSizeLog2Unchecked();
      
      return arraySize;
   }

   bool BPackedDocumentReader::BValue::get(uint8& value, uint arrayIndex, uint8 def) const
   {
      uint64 v;
      if (!get(v, arrayIndex))
      {
         value = def;
         return false;
      }
      
      if (v > UINT8_MAX)
      {
         value = def;
         return false;
      }
      
      value = static_cast<uint8>(v);
      return true;
   }
   
   bool BPackedDocumentReader::BValue::get(int8& value, uint arrayIndex, int8 def) const
   {
      int64 v;
      if (!get(v, arrayIndex))
      {
         value = def;
         return false;
      }

      if ((v < INT8_MIN) || (v > INT8_MAX))
      {
         value = def;
         return false;
      }

      value = static_cast<int8>(v);
      return true;
   }
   
   bool BPackedDocumentReader::BValue::get(uint16& value, uint arrayIndex, uint16 def) const
   {
      uint64 v;
      if (!get(v, arrayIndex))
      {
         value = def;
         return false;
      }

      if (v > UINT16_MAX)
      {
         value = def;
         return false;
      }

      value = static_cast<uint16>(v);
      return true;
   }
   
   bool BPackedDocumentReader::BValue::get(int16& value, uint arrayIndex, int16 def) const
   {
      int64 v;
      if (!get(v, arrayIndex))
      {
         value = def;
         return false;
      }

      if ((v < INT16_MIN) || (v > INT16_MAX))
      {
         value = def;
         return false;
      }

      value = static_cast<int16>(v);
      return true;
   }
   
   bool BPackedDocumentReader::BValue::get(uint32& value, uint arrayIndex, uint32 def) const
   {
      uint64 v;
      if (!get(v, arrayIndex))
      {
         value = def;
         return false;
      }

      if (v > UINT32_MAX)
      {
         value = def;
         return false;
      }

      value = static_cast<uint32>(v);
      return true;
   }
   
   bool BPackedDocumentReader::BValue::get(int32& value, uint arrayIndex, int32 def) const
   {
      int64 v;
      if (!get(v, arrayIndex))
      {
         value = def;
         return false;
      }

      if ((v < INT32_MIN) || (v > INT32_MAX))
      {
         value = def;
         return false;
      }

      value = static_cast<int8>(v);
      return true;
   }
   
   bool BPackedDocumentReader::BValue::get(uint64& value, uint arrayIndex, uint64 def) const
   {
      if (!isValid())
      {
         value = def;
         return false;
      }
      
      BDEBUG_ASSERT(arrayIndex < getArraySize());
      
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      eTypeClass typeClass = static_cast<eTypeClass>((nameValue.mFlags & BPackedNameValue::cTypeMask) >> BPackedNameValue::cTypeShift);
      const uint typeSizeLog2 = (nameValue.mFlags & BPackedNameValue::cTypeSizeInBytesLog2Mask) >> BPackedNameValue::cTypeSizeInBytesLog2Shift;
      
      const void* p = getDataPtrUnchecked();
            
      switch (typeClass)
      {
         case cTypeNull:
         {
            value = def;
            break;
         }
         case cTypeBool:
         {
            value = static_cast<const bool*>(p)[arrayIndex];
            break;
         }
         case cTypeInt:
         {
            if (typeSizeLog2 == 3)
               value = static_cast<const uint64*>(p)[arrayIndex];
            else if (typeSizeLog2 == 2)
               value = static_cast<const uint32*>(p)[arrayIndex];
            else if (typeSizeLog2 == 1)
               value = static_cast<const uint16*>(p)[arrayIndex];
            else 
               value = static_cast<const uint8*>(p)[arrayIndex];
               
            if ((nameValue.mFlags & BPackedNameValue::cTypeIsUnsignedMask) == 0)
            {
               const uint64 bitsToShift = ((1U << typeSizeLog2) - 1) * 8U;
               const uint64 signMask = ((uint64)0x80) << bitsToShift;
               
               if (value & signMask)
               {
                  value = def;
                  return false;
               }
            }
               
            break;
         }
         case cTypeFloat:
         {
            double f;
            if (typeSizeLog2 == 3)
               f = static_cast<const double*>(p)[arrayIndex];
            else 
               f = static_cast<const float*>(p)[arrayIndex];
            
            if ((f < 0.0f) || (f > UINT64_MAX))
            {
               value = def;
               return false;
            }
            
            value = (uint64)f;
            break;
         }
         case cTypeString:
         {
            int64 v;
            
            errno = 0;
            
            if (!typeSizeLog2)
            {
               const char* pStr = BStringArrayHelpers::getString(static_cast<const char*>(p), getDataSizeUnchecked(), arrayIndex);
               if (!pStr)
               {
                  value = def;
                  return false;
               }
               v = _atoi64(pStr);
            }
            else
            {
               const wchar_t* pStr = BStringArrayHelpers::getString(static_cast<const wchar_t*>(p), getDataSizeUnchecked(), arrayIndex);
               if (!pStr)
               {
                  value = def;
                  return false;
               }
               v = _wtoi64(pStr);
            }
                           
            if ((v < 0) || (errno))
            {
               value = def;
               return false;
            }
            value = v;
         
            break;
         }
         default:
         {
            value = def;
            return false;
         }
      }
            
      return true;
   }
   
   bool BPackedDocumentReader::BValue::get(int64& value, uint arrayIndex, int64 def) const
   {
      if (!isValid())
      {
         value = def;
         return false;
      }
      
      BDEBUG_ASSERT(arrayIndex < getArraySize());
      
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      eTypeClass typeClass = static_cast<eTypeClass>((nameValue.mFlags & BPackedNameValue::cTypeMask) >> BPackedNameValue::cTypeShift);
      const uint typeSizeLog2 = (nameValue.mFlags & BPackedNameValue::cTypeSizeInBytesLog2Mask) >> BPackedNameValue::cTypeSizeInBytesLog2Shift;
      
      const void* p = getDataPtrUnchecked();
                        
      switch (typeClass)
      {
         case cTypeNull:
         {
            value = def;
            break;
         }
         case cTypeBool:
         {
            value = static_cast<const bool*>(p)[arrayIndex];
            break;
         }
         case cTypeInt:
         {
            if (typeSizeLog2 == 3)
               value = static_cast<const int64*>(p)[arrayIndex];
            else if (typeSizeLog2 == 2)
               value = static_cast<const int32*>(p)[arrayIndex];
            else if (typeSizeLog2 == 1)
               value = static_cast<const int16*>(p)[arrayIndex];
            else 
               value = static_cast<const int8*>(p)[arrayIndex];
               
            if (nameValue.mFlags & BPackedNameValue::cTypeIsUnsignedMask)
            {
               const uint64 bitsToShift = ((1U << typeSizeLog2) - 1) * 8U;
               const uint64 signMask = ((uint64)0x80) << bitsToShift;
               
               if (value & signMask)
               {
                  value = def;
                  return false;
               }
            }
               
            break;
         }
         case cTypeFloat:
         {
            double f;
            if (typeSizeLog2 == 3)
               f = static_cast<const double*>(p)[arrayIndex];
            else 
               f = static_cast<const float*>(p)[arrayIndex];
            
            if ((f < UINT64_MIN) || (f > UINT64_MAX))
            {
               value = def;
               return false;
            }
            
            value = (int64)f;
            break;
         }
         case cTypeString:
         {
            int64 v;
            
            errno = 0;
            
            if (!typeSizeLog2)
            {
               const char* pStr = BStringArrayHelpers::getString(static_cast<const char*>(p), getDataSizeUnchecked(), arrayIndex);
               if (!pStr)
               {
                  value = def;
                  return false;
               }
               v = _atoi64(pStr);
            }
            else
            {
               const wchar_t* pStr = BStringArrayHelpers::getString(static_cast<const wchar_t*>(p), getDataSizeUnchecked(), arrayIndex);
               if (!pStr)
               {
                  value = def;
                  return false;
               }
               v = _wtoi64(pStr);
            }
                           
            if (errno)
            {
               value = def;
               return false;
            }
            value = v;
         
            break;
         }
         default:
         {
            value = def;
            return false;
         }
      }
            
      return true;
   }
   
   bool BPackedDocumentReader::BValue::get(bool& value, uint arrayIndex, bool def) const
   {
      int64 v;
      if (!get(v, arrayIndex))
      {  
         value = def;
         return false;
      }
      value = (v != 0);
      return true;
   }
   
   bool BPackedDocumentReader::BValue::get(float& value, uint arrayIndex, float def) const
   {
      if (!isValid())
      {
         value = def;
         return false;
      }
      
      BDEBUG_ASSERT(arrayIndex < getArraySize());
      
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      eTypeClass typeClass = static_cast<eTypeClass>((nameValue.mFlags & BPackedNameValue::cTypeMask) >> BPackedNameValue::cTypeShift);
      const uint typeSizeLog2 = (nameValue.mFlags & BPackedNameValue::cTypeSizeInBytesLog2Mask) >> BPackedNameValue::cTypeSizeInBytesLog2Shift;
      
      const void* p = getDataPtrUnchecked();
                        
      switch (typeClass)
      {
         case cTypeNull:
         {
            value = def;
            break;
         }
         case cTypeBool:
         {
            value = static_cast<const bool*>(p)[arrayIndex];
            break;
         }
         case cTypeInt:
         {
            if (nameValue.mFlags & BPackedNameValue::cTypeIsUnsignedMask)
            {
               if (typeSizeLog2 == 3)
                  value = (float)static_cast<const uint64*>(p)[arrayIndex];
               else if (typeSizeLog2 == 2)
                  value = (float)static_cast<const uint32*>(p)[arrayIndex];
               else if (typeSizeLog2 == 1)
                  value = (float)static_cast<const uint16*>(p)[arrayIndex];
               else 
                  value = (float)static_cast<const uint8*>(p)[arrayIndex];
            }
            else
            {
               if (typeSizeLog2 == 3)
                  value = (float)static_cast<const int64*>(p)[arrayIndex];
               else if (typeSizeLog2 == 2)
                  value = (float)static_cast<const int32*>(p)[arrayIndex];
               else if (typeSizeLog2 == 1)
                  value = (float)static_cast<const int16*>(p)[arrayIndex];
               else 
                  value = (float)static_cast<const int8*>(p)[arrayIndex];
            }                  
            break;
         }
         case cTypeFloat:
         {
            if (typeSizeLog2 == 3)
               value = (float)static_cast<const double*>(p)[arrayIndex];
            else 
               value = static_cast<const float*>(p)[arrayIndex];
            break;
         }
         case cTypeString:
         {
            errno = 0;
            
            if (!typeSizeLog2)
            {
               const char* pStr = BStringArrayHelpers::getString(static_cast<const char*>(p), getDataSizeUnchecked(), arrayIndex);
               if (!pStr)
               {
                  value = def;
                  return false;
               }
               value = (float)atof(pStr);
            }
            else
            {
               const wchar_t* pStr = BStringArrayHelpers::getString(static_cast<const wchar_t*>(p), getDataSizeUnchecked(), arrayIndex);
               if (!pStr)
               {
                  value = def;
                  return false;
               }
               value = (float)_wtof(pStr);
            }
            
            if (errno)
            {
               value = def;
               return false;
            }
            break;
         }
         default:
         {
            value = def;
            return false;
         }
      }
            
      return true;
   }
   
   bool BPackedDocumentReader::BValue::get(double& value, uint arrayIndex, double def) const
   {
      if (!isValid())
      {
         value = def;
         return false;
      }
      
      BDEBUG_ASSERT(arrayIndex < getArraySize());
      
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      eTypeClass typeClass = static_cast<eTypeClass>((nameValue.mFlags & BPackedNameValue::cTypeMask) >> BPackedNameValue::cTypeShift);
      const uint typeSizeLog2 = (nameValue.mFlags & BPackedNameValue::cTypeSizeInBytesLog2Mask) >> BPackedNameValue::cTypeSizeInBytesLog2Shift;
      
      const void* p = getDataPtrUnchecked();
                        
      switch (typeClass)
      {
         case cTypeNull:
         {
            value = def;
            break;
         }
         case cTypeBool:
         {
            value = static_cast<const bool*>(p)[arrayIndex];
            break;
         }
         case cTypeInt:
         {
            if (nameValue.mFlags & BPackedNameValue::cTypeIsUnsignedMask)
            {
               if (typeSizeLog2 == 3)
                  value = (double)static_cast<const uint64*>(p)[arrayIndex];
               else if (typeSizeLog2 == 2)
                  value = (double)static_cast<const uint32*>(p)[arrayIndex];
               else if (typeSizeLog2 == 1)
                  value = (double)static_cast<const uint16*>(p)[arrayIndex];
               else 
                  value = (double)static_cast<const uint8*>(p)[arrayIndex];
            }
            else
            {
               if (typeSizeLog2 == 3)
                  value = (double)static_cast<const int64*>(p)[arrayIndex];
               else if (typeSizeLog2 == 2)
                  value = (double)static_cast<const int32*>(p)[arrayIndex];
               else if (typeSizeLog2 == 1)
                  value = (double)static_cast<const int16*>(p)[arrayIndex];
               else 
                  value = (double)static_cast<const int8*>(p)[arrayIndex];
            }                  
            break;
         }
         case cTypeFloat:
         {
            if (typeSizeLog2 == 3)
               value = static_cast<const double*>(p)[arrayIndex];
            else 
               value = (double)static_cast<const float*>(p)[arrayIndex];
            break;
         }
         case cTypeString:
         {
            errno = 0;
            
            if (!typeSizeLog2)
            {
               const char* pStr = BStringArrayHelpers::getString(static_cast<const char*>(p), getDataSizeUnchecked(), arrayIndex);
               if (!pStr)
               {
                  value = def;
                  return false;
               }
               value = atof(pStr);
            }
            else
            {
               const wchar_t* pStr = BStringArrayHelpers::getString(static_cast<const wchar_t*>(p), getDataSizeUnchecked(), arrayIndex);
               if (!pStr)
               {
                  value = def;
                  return false;
               }
               value = _wtof(pStr);
            }
            
            if (errno)
            {
               value = def;
               return false;
            }
            break;
         }
         default:
         {
            value = def;
            return false;
         }
      }
            
      return true;
   }
   
   bool BPackedDocumentReader::BValue::get(BString& value, uint arrayIndex, const BString& def) const
   {
      if (!isValid())
      {
         value = def;
         return false;
      }
      
      BDEBUG_ASSERT(arrayIndex < getArraySize());
      
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      eTypeClass typeClass = static_cast<eTypeClass>((nameValue.mFlags & BPackedNameValue::cTypeMask) >> BPackedNameValue::cTypeShift);
      const uint typeSizeLog2 = (nameValue.mFlags & BPackedNameValue::cTypeSizeInBytesLog2Mask) >> BPackedNameValue::cTypeSizeInBytesLog2Shift;
      
      const void* p = getDataPtrUnchecked();
                        
      switch (typeClass)
      {
         case cTypeNull:
         {
            value = def;
            break;
         }
         case cTypeBool:
         {
            value.format("%u", static_cast<const bool*>(p)[arrayIndex]);
            break;
         }
         case cTypeInt:
         {
            if (nameValue.mFlags & BPackedNameValue::cTypeIsUnsignedMask)
            {
               uint64 uv;
               if (typeSizeLog2 == 3)
                  uv = static_cast<const uint64*>(p)[arrayIndex];
               else if (typeSizeLog2 == 2)
                  uv = static_cast<const uint32*>(p)[arrayIndex];
               else if (typeSizeLog2 == 1)
                  uv = static_cast<const uint16*>(p)[arrayIndex];
               else 
                  uv = static_cast<const uint8*>(p)[arrayIndex];
               value.format("%I64u", uv);
            }
            else
            {
               uint64 iv;
               if (typeSizeLog2 == 3)
                  iv = static_cast<const int64*>(p)[arrayIndex];
               else if (typeSizeLog2 == 2)
                  iv = static_cast<const int32*>(p)[arrayIndex];
               else if (typeSizeLog2 == 1)
                  iv = static_cast<const int16*>(p)[arrayIndex];
               else 
                  iv = static_cast<const int8*>(p)[arrayIndex];
               value.format("%I64i", iv);                  
            }                  
            break;
         }
         case cTypeFloat:
         {
            double fv;
            if (typeSizeLog2 == 3)
               fv = static_cast<const double*>(p)[arrayIndex];
            else 
               fv = static_cast<const float*>(p)[arrayIndex];
            value.format("%f", fv);
            break;
         }
         case cTypeString:
         {
            if (!typeSizeLog2)
            {
               const char* pStr = BStringArrayHelpers::getString(static_cast<const char*>(p), getDataSizeUnchecked(), arrayIndex);
               value.set(pStr);
            }
            else
            {
               const wchar_t* pStr = BStringArrayHelpers::getString(static_cast<const wchar_t*>(p), getDataSizeUnchecked(), arrayIndex);
               if (!pStr)
               {
                  value = def;
                  return false;
               }
               value.set(pStr);
            }
            break;
         }
         default:
         {
            value = def;
            return false;
         }
      }
            
      return true;
   }
   
   bool BPackedDocumentReader::BValue::get(BUString& value, uint arrayIndex, const BUString& def) const
   {
      if (!isValid())
      {
         value = def;
         return false;
      }
      
      BDEBUG_ASSERT(arrayIndex < getArraySize());
      
      const BPackedNameValue& nameValue = mpReader->mpNameValues[mNameValueIndex];
      eTypeClass typeClass = static_cast<eTypeClass>((nameValue.mFlags & BPackedNameValue::cTypeMask) >> BPackedNameValue::cTypeShift);
      const uint typeSizeLog2 = (nameValue.mFlags & BPackedNameValue::cTypeSizeInBytesLog2Mask) >> BPackedNameValue::cTypeSizeInBytesLog2Shift;
      
      const void* p = getDataPtrUnchecked();
                        
      switch (typeClass)
      {
         case cTypeNull:
         {
            value = def;
            break;
         }
         case cTypeBool:
         {
            value.format(L"%u", static_cast<const bool*>(p)[arrayIndex]);
            break;
         }
         case cTypeInt:
         {
            if (nameValue.mFlags & BPackedNameValue::cTypeIsUnsignedMask)
            {
               uint64 uv;
               if (typeSizeLog2 == 3)
                  uv = static_cast<const uint64*>(p)[arrayIndex];
               else if (typeSizeLog2 == 2)
                  uv = static_cast<const uint32*>(p)[arrayIndex];
               else if (typeSizeLog2 == 1)
                  uv = static_cast<const uint16*>(p)[arrayIndex];
               else 
                  uv = static_cast<const uint8*>(p)[arrayIndex];
               value.format(L"%I64u", uv);
            }
            else
            {
               uint64 iv;
               if (typeSizeLog2 == 3)
                  iv = static_cast<const int64*>(p)[arrayIndex];
               else if (typeSizeLog2 == 2)
                  iv = static_cast<const int32*>(p)[arrayIndex];
               else if (typeSizeLog2 == 1)
                  iv = static_cast<const int16*>(p)[arrayIndex];
               else 
                  iv = static_cast<const int8*>(p)[arrayIndex];
               value.format(L"%I64i", iv);                  
            }                  
            break;
         }
         case cTypeFloat:
         {
            double fv;
            if (typeSizeLog2 == 3)
               fv = static_cast<const double*>(p)[arrayIndex];
            else 
               fv = static_cast<const float*>(p)[arrayIndex];
            value.format(L"%f", fv);
            break;
         }
         case cTypeString:
         {
            if (!typeSizeLog2)
            {
               const char* pStr = BStringArrayHelpers::getString(static_cast<const char*>(p), getDataSizeUnchecked(), arrayIndex);
               value.set(pStr);
            }
            else
            {
               const wchar_t* pStr = BStringArrayHelpers::getString(static_cast<const wchar_t*>(p), getDataSizeUnchecked(), arrayIndex);
               if (!pStr)
               {
                  value = def;
                  return false;
               }
               value.set(pStr);
            }
            break;
         }
         default:
         {
            value = def;
            return false;
         }
      }
            
      return true;
   }
   
   BPackedDocumentReader::BValue::operator bool() const
   {
      bool v;
      get(v);
      return v;
   }

   BPackedDocumentReader::BValue::operator uint8() const
   {
      uint8 v;
      get(v);
      return v;
   }
   
   BPackedDocumentReader::BValue::operator int8() const
   {
      int8 v;
      get(v);
      return v;
   }
   
   BPackedDocumentReader::BValue::operator uint16() const
   {
      uint16 v;
      get(v);
      return v;
   }
   
   BPackedDocumentReader::BValue::operator int16() const
   {
      int16 v;
      get(v);
      return v;
   }
   
   BPackedDocumentReader::BValue::operator uint32() const
   {
      uint32 v;
      get(v);
      return v;
   }
   
   BPackedDocumentReader::BValue::operator int32() const
   {
      int32 v;
      get(v);
      return v;
   }
   
   BPackedDocumentReader::BValue::operator uint64() const
   {
      uint64 v;
      get(v);
      return v;
   }
   
   BPackedDocumentReader::BValue::operator int64() const
   {
      int64 v;
      get(v);
      return v;
   }
   
   BPackedDocumentReader::BValue::operator float() const
   {
      float v;
      get(v);
      return v;
   }
   
   BPackedDocumentReader::BValue::operator double() const
   {
      double v;
      get(v);
      return v;
   }
   
   BPackedDocumentReader::BValue::operator BString() const
   {  
      BString str;
      get(str);
      return str;
   }
   
   BPackedDocumentReader::BValue::operator BUString() const
   {
      BUString str;
      get(str);
      return str;
   }
   
   const char* BPackedDocumentReader::BNode::getName() const
   {
      if (!isValid())
         return NULL;
      const uint nameValueIndex = mpReader->mpNodes[mNodeIndex].mNameValueOfs;
      return reinterpret_cast<const char*>(mpReader->mpNameData + mpReader->mpNameValues[nameValueIndex].mNameOfs);         
   }
      
   uint BPackedDocumentReader::BNode::getNumChildren() const
   {
      if (mNumChildren != UINT16_MAX)
         return mNumChildren;
         
      if (!isValid())
         return 0;

      const BPackedNode& packedNode = mpReader->mpNodes[mNodeIndex];
      
      uint numChildNodes = packedNode.mNumChildrenNodes;
      
      if (numChildNodes == 0xFF)
      {
         uint childNodeIndex = packedNode.mChildNodeIndex;
         
         const uint numNodes = mpReader->mNumNodes;
         for ( ; ; )
         {
            if ((childNodeIndex + numChildNodes) > numNodes)
               return false;
            else if ((childNodeIndex + numChildNodes) == numNodes)
               break;
            
            if (mpReader->mpNodes[childNodeIndex + numChildNodes].mParentIndex != mNodeIndex)
               break;
               
            numChildNodes++;
         }
      }
      
      mNumChildren = static_cast<uint16>(numChildNodes);
      
      return numChildNodes;
   }
   
   const char* BPackedDocumentReader::BNode::getChildName(uint index) const
   {
      if ((!isValid()) || (index >= getNumChildren()))
         return NULL;
          
      const BPackedNode& node = mpReader->mpNodes[mNodeIndex];  
      
      const BPackedNode& childNode = mpReader->mpNodes[node.mChildNodeIndex + index];
      
      const BPackedNameValue& childNameValue = mpReader->mpNameValues[childNode.mNameValueOfs];
      
      return reinterpret_cast<const char*>(mpReader->mpNameData + childNameValue.mNameOfs);
   }
   
   BPackedDocumentReader::BNode BPackedDocumentReader::BNode::getChild(uint index) const
   {
      if ((!isValid()) || (index >= getNumChildren()))
         return BNode(NULL, UINT_MAX);

      const BPackedNode& node = mpReader->mpNodes[mNodeIndex];  

      return BNode(mpReader, node.mChildNodeIndex + index);
   }
   
   bool BPackedDocumentReader::BNode::findChild(const char* pName, BNode& node) const
   {
      const uint numChildren = getNumChildren();
      for (uint i = 0; i < numChildren; i++)
      {
         if (strcmp(getChildName(i), pName) == 0)
         {
            const BPackedNode& childNode = mpReader->mpNodes[mNodeIndex];  
            node.init(mpReader, childNode.mChildNodeIndex + i);
            return true;
         }
      }
      node.clear();
      return false;
   }
   
   bool BPackedDocumentReader::BNode::doesChildExist(const char* pName) const
   {
      const uint numChildren = getNumChildren();
      for (uint i = 0; i < numChildren; i++)
         if (strcmp(getChildName(i), pName) == 0)
            return true;
      return false;
   }

   uint BPackedDocumentReader::BNode::getNumAttributes() const
   {
      if (mNumAttributes != UINT16_MAX)
         return mNumAttributes;

      if (!isValid())
         return 0;

      const BPackedNode& packedNode = mpReader->mpNodes[mNodeIndex];

      uint numNameValues = packedNode.mNumNameValues;
      if (numNameValues == 0xFF)
      {
         uint nameValueIndex = packedNode.mNameValueOfs;

         for ( ; ; )
         {
            if ((nameValueIndex + numNameValues) >= mpReader->mNumNameValues)
               return false;

            if (mpReader->mpNameValues[nameValueIndex + numNameValues].mFlags & BPackedNameValue::cLastNameValueMask)
               break;

            numNameValues++;
         }
      }
      BDEBUG_ASSERT(numNameValues);
      
      mNumAttributes = static_cast<uint16>(numNameValues - 1);

      return numNameValues - 1;
   }
   
   const char* BPackedDocumentReader::BNode::getAttributeName(uint index) const
   {
      if ((!isValid()) || (index >= getNumAttributes()))
         return NULL;
         
      const BPackedNode& node = mpReader->mpNodes[mNodeIndex];  
      
      const BPackedNameValue& nameValue = mpReader->mpNameValues[node.mNameValueOfs + index + 1];
      
      return reinterpret_cast<const char*>(mpReader->mpNameData + nameValue.mNameOfs);
   }
   
   BPackedDocumentReader::BValue BPackedDocumentReader::BNode::getAttribute(uint index) const
   {
      if ((!isValid()) || (index >= getNumAttributes()))
         return BValue(NULL, UINT_MAX);

      const BPackedNode& node = mpReader->mpNodes[mNodeIndex];  
   
      return BValue(mpReader, node.mNameValueOfs + index + 1);
   }
   
   bool BPackedDocumentReader::BNode::findAttribute(const char* pName, BValue& value) const
   {
      const uint numAttributes = getNumAttributes();
      for (uint i = 0; i < numAttributes; i++)
      {
         if (strcmp(getAttributeName(i), pName) == 0)
         {
            value.init(mpReader, mpReader->mpNodes[mNodeIndex].mNameValueOfs + i + 1);
            return true;
         }
      }
      value.clear();
      return false;
   }
   
   bool BPackedDocumentReader::BNode::doesAttributeExist(const char* pName) const
   {
      const uint numAttributes = getNumAttributes();
      for (uint i = 0; i < numAttributes; i++)
         if (strcmp(getAttributeName(i), pName) == 0)
            return true;
      return false;
   }
   
   bool BPackedDocumentReader::BNode::getNameValueMap(BNameValueMap& nameValueMap, bool asAttributes) const
   {
      BDEBUG_ASSERT(isValid());
      
      const uint num = asAttributes ? getNumAttributes() : getNumChildren();
      for (uint i = 0; i < num; i++)
      {
         BValue value(asAttributes ? getAttribute(i) : getChild(i));
         
         const char* pName = value.getName();
         const uint typeSize = value.getTypeSize();
         
         switch (value.getTypeClass())
         {
            case cTypeNull:
            {
               break;
            }
            case cTypeBool:
            {
               nameValueMap.set(pName, (bool)value);
               break;
            }
            case cTypeInt:
            {
               if (value.isUnsigned())
               {
                  if (typeSize <= sizeof(uint8))
                     nameValueMap.set(pName, (uint8)value);
                  else if (typeSize <= sizeof(uint16))
                     nameValueMap.set(pName, (uint16)value);
                  else if (typeSize <= sizeof(uint32))
                     nameValueMap.set(pName, (uint32)value);
                  else
                     nameValueMap.set(pName, (uint64)value);
               }
               else
               {
                  if (typeSize <= sizeof(int8))
                     nameValueMap.set(pName, (int8)value);
                  else if (typeSize <= sizeof(int16))
                     nameValueMap.set(pName, (int16)value);
                  else if (typeSize <= sizeof(int32))
                     nameValueMap.set(pName, (int32)value);
                  else
                     nameValueMap.set(pName, (int64)value);
               }
               break;
            }
            case cTypeFloat:
            {
               if (typeSize == sizeof(float))
                  nameValueMap.set(pName, (float)value);
               else
                  nameValueMap.set(pName, (double)value);
               break;
            }
            case cTypeString:
            {
               // Name value maps don't support UTF-16 yet!
               nameValueMap.set(pName, (BString)value);
               break;
            }
            default:
            {
               BDEBUG_ASSERT(0);
               return false;
            }
         }
      }
      
      return true;
   }
   
}



































//============================================================================
//
// File: binaryDataTree.h
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once
#include "hashMap.h"
#include "containers\freelist.h"
#include "utils\dataBuffer.h"
#include "stream\stream.h"
#include "utils\textDispatcher.h"

class BNameValueMap;

namespace BBinaryDataTree
{
   struct BPackedNameValue
   {
      uint32         mValue;

      uint16         mNameOfs;

      enum 
      {
         cTypeIsUnsignedMask        = 1U,
         cDirectEncodingMask        = 2U,

         cTypeShift                 = 2U,
         cTypeBits                  = 3U,
         cTypeMask                  = ((1U << cTypeBits) - 1U) << cTypeShift,

         cTypeSizeInBytesLog2Shift  = 5U,
         cTypeSizeInBytesLog2Bits   = 3U,
         cTypeSizeInBytesLog2Mask   = ((1U << cTypeSizeInBytesLog2Bits) - 1U) << cTypeSizeInBytesLog2Shift,

         cLastNameValueMask         = 0x100,

         cSizeShift                 = 9,
         cSizeBits                  = 7,
         cSizeMask                  = ((1U << cSizeBits) - 1U) << cSizeShift
      };

      uint16         mFlags;      

      void endianSwitch()
      {
         EndianSwitchWorker(this, this + 1, "iss");
      }      
   };

   struct BPackedNode
   {
      uint16         mParentIndex;
      uint16         mChildNodeIndex;

      uint16         mNameValueOfs;

      uint8          mNumNameValues;
      uint8          mNumChildrenNodes;

      void endianSwitch()
      {
         EndianSwitchWorker(this, this + 1, "ssscc");
      }
   };

   struct BPackedSection
   {
      uint32 mID;
      uint32 mSize;
      uint32 mOfs;

      void endianSwitch()
      {
         EndianSwitchWorker(this, this + 1, "iii");
      }
   };

   struct BPackedHeader 
   {
      enum { cLittleEndianSig = 0x3E, cBigEndianSig = 0xE3 };

      uint8          mSig;
      uint8          mHeaderDWORDs;
      uint8          mHeaderCRC8;
      uint8          mNumUserSections;

      uint32         mDataCRC32;
      uint32         mDataSize;
      
      uint32         mBaseSectionSizes[4];

      void endianSwitch()
      {
         EndianSwitchWorker(this, this + 1, "cccciiiiii");
      }
   };

   enum eTypeClass
   {  
      cTypeNull,
      cTypeBool,
      cTypeInt,
      cTypeFloat,
      cTypeString,
      
      cNumTypes
   };
   
   struct BTypeDesc
   {
      BTypeDesc() { clear(); }
      BTypeDesc(eTypeClass c, uint sizeInBytes, uint alignment, bool isUnsigned) : mClass(c), mSizeInBytes((uint16)sizeInBytes), mAlignment((uint16)alignment), mIsUnsigned(isUnsigned) { }

      void clear()
      {
         mClass         = cTypeNull;
         mIsUnsigned    = false;
         mSizeInBytes   = 0;
         mAlignment     = sizeof(DWORD);
      }

      eTypeClass  mClass;
      uint16      mSizeInBytes;
      uint16      mAlignment;
      bool        mIsUnsigned;

      bool operator== (const BTypeDesc& rhs) const
      {
         return (mClass == rhs.mClass) && (mSizeInBytes == rhs.mSizeInBytes) && (mAlignment == rhs.mAlignment) && (mIsUnsigned == rhs.mIsUnsigned);
      }

      operator size_t() const
      {
         uint hash = hashFast(&mClass, sizeof(mClass));
         hash = hashFast(&mSizeInBytes, sizeof(mSizeInBytes), hash);
         hash = hashFast(&mAlignment, sizeof(mAlignment), hash);
         hash = hashFast(&mIsUnsigned, sizeof(mIsUnsigned), hash);
         return hash;
      }
   };
   
   extern BTypeDesc gNullTypeDesc;
   extern BTypeDesc gBoolTypeDesc;
   extern BTypeDesc gUInt8TypeDesc, gInt8TypeDesc;
   extern BTypeDesc gUInt16TypeDesc, gInt16TypeDesc;
   extern BTypeDesc gUInt32TypeDesc, gInt32TypeDesc;
   extern BTypeDesc gUInt64TypeDesc, gInt64TypeDesc;
   extern BTypeDesc gFloatTypeDesc, gDoubleTypeDesc;
   extern BTypeDesc gStringTypeDesc, gUStringTypeDesc;
   extern BTypeDesc gFloatVectorTypeDesc;
   
   // The default BGetTypeDesc struct purposely doesn't define a get() method! 
   // If you get a compiler error, the data type you are trying to add is unsupported (directly).
   template<typename T> struct BGetTypeDesc { };
   template<> struct BGetTypeDesc<uint8> { static const BTypeDesc& get() { return gUInt8TypeDesc; } };
   template<> struct BGetTypeDesc<int8> { static const BTypeDesc& get() { return gInt8TypeDesc; } };
   template<> struct BGetTypeDesc<uint16> { static const BTypeDesc& get() { return gUInt16TypeDesc; } };
   template<> struct BGetTypeDesc<int16> { static const BTypeDesc& get() { return gInt16TypeDesc; } };
   template<> struct BGetTypeDesc<uint32> { static const BTypeDesc& get() { return gUInt32TypeDesc; } };
   template<> struct BGetTypeDesc<int32> { static const BTypeDesc& get() { return gInt32TypeDesc; } };
   template<> struct BGetTypeDesc<uint64> { static const BTypeDesc& get() { return gUInt64TypeDesc; } };
   template<> struct BGetTypeDesc<int64> { static const BTypeDesc& get() { return gInt64TypeDesc; } };
   template<> struct BGetTypeDesc<float> { static const BTypeDesc& get() { return gFloatTypeDesc; } };
   template<> struct BGetTypeDesc<double> { static const BTypeDesc& get() { return gDoubleTypeDesc; } };
   template<> struct BGetTypeDesc<bool> { static const BTypeDesc& get() { return gBoolTypeDesc; } };
   template<> struct BGetTypeDesc<__wchar_t> { static const BTypeDesc& get() { return gUInt16TypeDesc; } };
   
   enum eSectionID
   {
      cNodeSectionIndex       = 0,
      cNameValueSectionIndex  = 1,
      cNameDataSectionIndex   = 2,
      cValueDataSectionIndex  = 3,
      
      cNumBasicSections       = 4,
      
      cMaxUserSections        = 255
   };
        
   class BDocumentBuilder
   {
   public:
      typedef uint BUserSectionIndex;
      enum { cInvalidUserSectionIndex = UINT_MAX };

      typedef uint BNodeIndex;
      typedef BDynamicArray<BNodeIndex> BNodeIndexArray;

      typedef uint BAttributeIndex;
      enum { cInvalidAttributeIndex = UINT_MAX };
      
   private:
      struct BBuildVariantData
      {
         BBuildVariantData() { clear(); }

         void clear()
         {
            mTypeDesc.clear();
            mArraySize = 0;
            mData.clear();
         }

         BTypeDesc   mTypeDesc;
         uint        mArraySize;
         BByteArray  mData;

         mutable BString   mString;
         mutable BUString  mUString;

         bool operator== (const BBuildVariantData& rhs) const
         {
            return (mTypeDesc == rhs.mTypeDesc) && (mArraySize == rhs.mArraySize) && (mData == rhs.mData);
         }

         operator size_t() const
         {
            size_t hash = static_cast<size_t>(mTypeDesc);

            hash = hashFast(&mArraySize, sizeof(mArraySize), hash);

            if (!mData.isEmpty())
               hash = hashFast(mData.getPtr(), mData.getSize(), hash);

            return hash;
         }

         uint64            asUnsigned(uint arrayIndex = 0) const;       
         int64             asSigned(uint arrayIndex = 0) const;
         float             asFloat(uint arrayIndex = 0) const;
         double            asDouble(uint arrayIndex = 0) const;
         bool              asBool(uint arrayIndex = 0) const;
         const char*       asString(uint arrayIndex = 0, bool convertToString = true) const;
         const wchar_t*    asUString(uint arrayIndex = 0, bool convertToString = true) const;

         bool              set(const void* pValueData, uint valueDataSizeInBytes, const BTypeDesc& typeDesc, uint arraySize);
      };

      typedef BHashMap<BBuildVariantData, uint> BBuildVariantDataHashMap;      
      typedef BHashMap<BString, uint, BHasher<BString>, BStringEqualsCaseSensitive> BStringHashMap;

      struct BBuildNameValue
      {
         BString                    mName;
         BBuildVariantData          mVariant;

         void clear()
         {
            mName.empty();
            mVariant.clear();
         }

         bool set(const char* pName, const void* pValueData, uint valueDataSizeInBytes, const BTypeDesc& typeDesc, uint arraySize);
      };
      typedef BDynamicArray<BBuildNameValue> BBuildNameValueArray;

      struct BBuildNode
      {
         BBuildNode*                mpParent;
                  
         typedef BDynamicArray<BBuildNode*> BBuildNodePtrArray;
         BBuildNodePtrArray         mChildren;
         
         BBuildNameValueArray       mNameValues;

         void clear()
         {
            mpParent = NULL;
            mNameValues.clear();
            mChildren.clear();
         }
      };

      typedef BFreeList<BBuildNode> BBuildNodeFreeList;
      BBuildNodeFreeList mBuildNodes;
      
      BBuildNode* mpRootNode;

      struct BUserSection
      {
         uint32      mID;
         BByteArray  mData;
      };

      typedef BDynamicArray<BUserSection> BUserSectionArray;
      BUserSectionArray mUserSections;
            
   public:
      class BValue 
      {
         friend class BDocumentBuilder;
         
      public:
         BValue() : mpNode(NULL), mNameValueIndex(0) { }
         BValue(BBuildNode* pNode, uint nameValueIndex) : mpNode(pNode), mNameValueIndex(nameValueIndex) { }
         
         bool                    isValid() const { return NULL != mpNode; }
         
         void                    init(BBuildNode* pNode, uint nameValueIndex) { mpNode = pNode; mNameValueIndex = nameValueIndex; }
         void                    clear() { mpNode = NULL, mNameValueIndex = 0; }
         
         const BBuildNode*       getNode() const { return mpNode; }
         uint                    getNameValueIndex() const { return mNameValueIndex; }
      
         const char*             getName() const;
         
         const BTypeDesc&        getTypeDesc() const;
         
         uint                    getDataSizeInBytes() const;
         const void*             getDataPtr() const;
         
         uint                    getArraySize() const;
         
         uint64                  asUnsigned(uint arrayIndex = 0) const;       
         int64                   asSigned(uint arrayIndex = 0) const;
         float                   asFloat(uint arrayIndex = 0) const;
         double                  asDouble(uint arrayIndex = 0) const;
         bool                    asBool(uint arrayIndex = 0) const;
         const char*             asString(uint arrayIndex = 0, bool convertToString = true) const;
         const wchar_t*          asUString(uint arrayIndex = 0, bool convertToString = true) const;
         
         operator                bool() const { return asBool(); }
         operator                uint8() const { uint64 v = asUnsigned(); if (v > UINT8_MAX) return 0; else return (uint8)v; }
         operator                int8() const { int64 v = asSigned(); if ((v < INT8_MIN) || (v > INT8_MAX)) return 0; else return (int8)v; }
         operator                uint16() const { uint64 v = asUnsigned(); if (v > UINT16_MAX) return 0; else return (uint16)v; }
         operator                int16() const { int64 v = asSigned(); if ((v < INT16_MIN) || (v > INT16_MAX)) return 0; else return (int16)v; };
         operator                uint32() const { uint64 v = asUnsigned(); if (v > UINT32_MAX) return 0; else return (uint32)v; }
         operator                int32() const { int64 v = asSigned(); if ((v < INT32_MIN) || (v > INT32_MAX)) return 0; else return (int32)v; };;
         operator                uint64() const { return asUnsigned(); }
         operator                int64() const { return asSigned(); }
         operator                float() const { return asFloat(); }
         operator                double() const { return asDouble(); }
         operator                const char*() const { return asString(); }
         operator                const wchar_t*() const { return asUString(); }
         operator                BString() const { asString(); return mpNode->mNameValues[mNameValueIndex].mVariant.mString; }
         operator                BUString() const { asUString(); return mpNode->mNameValues[mNameValueIndex].mVariant.mUString; }
         
         bool                    setName(const char* pName);
         
         bool                    set(const char* pName, const void* pValueData, uint valueDataSizeInBytes, const BTypeDesc& typeDesc, uint arraySize);
         
         bool                    set(const void* pValueData, uint valueDataSizeInBytes, const BTypeDesc& typeDesc, uint arraySize);
         
         template<typename T> 
         bool                    set(T value) { return set(&value, sizeof(T), BGetTypeDesc<T>::get(), 1); }

         bool                    set(const char* pValue);
         bool                    set(const wchar_t* pValue);
         bool                    set(const BString& value) { return set(value.getPtr()); }
         bool                    set(const BUString& value) { return set(value.getPtr()); }
         
         bool                    set(const void* pValueData, uint dataLen);
         bool                    set(const BDynamicArray<BString>& strings);
                                         
      protected:
         BBuildNode*             mpNode;
         uint                    mNameValueIndex;
      };
      
      class BNode : public BValue
      {
         friend class BDocumentBuilder;
         
      public:
         BNode() : BValue(NULL, 0), mpBuilder(NULL) { }
         BNode(BDocumentBuilder* pBuilder, BBuildNode* pNode) : BValue(pNode, 0), mpBuilder(pBuilder) { }
                  
         void init(BDocumentBuilder* pBuilder, BBuildNode* pNode) { BValue::init(pNode, 0); mpBuilder = pBuilder; }
         
         BDocumentBuilder* getBuilder() const { return mpBuilder; }

         // Nodes                  
         BNode getParent() const { BDEBUG_ASSERT(mpNode); return BNode(mpBuilder, mpNode->mpParent); }
         
         uint getNumChildren() const { BDEBUG_ASSERT(mpNode); return mpNode->mChildren.getSize(); }
         BNode getChild(uint childIndex) const { BDEBUG_ASSERT(childIndex < getNumChildren()); return BNode(mpBuilder, mpNode->mChildren[childIndex]); }
         
         bool findChild(const char* pName, BNode& node) const
         {
            BDEBUG_ASSERT(mpNode);
            for (uint i = 0; i < mpNode->mChildren.getSize(); i++)
               if (strcmp(pName, mpNode->mChildren[i]->mNameValues[0].mName.getPtr()) == 0)
               {
                  node.init(mpBuilder, mpNode->mChildren[i]);
                  return true;
               }
            node.clear();
            return false;
         }
         
         BNode addChild(const char* pName, const void* pValueData, uint valueDataSizeInBytes, const BTypeDesc& typeDesc, uint arraySize);

         BNode addChild(const char* pName);

         template<typename T> 
         BNode addChild(const char* pName, T value) { return addChild(pName, &value, sizeof(T), BGetTypeDesc<T>::get(), 1); }

         BNode addChild(const char* pName, const char* pValue);
         BNode addChild(const char* pName, const wchar_t* pValue);
         BNode addChild(const char* pName, const BString& value) { return addChild(pName, value.getPtr()); }
         BNode addChild(const char* pName, const BUString& value) { return addChild(pName, value.getPtr()); }
         
         BNode addChild(const char* pName, const void* pValueData, uint dataLen);
         BNode addChild(const char* pName, const BDynamicArray<BString>& strings);
         
         // Attributes
         uint getNumAttributes() const { BDEBUG_ASSERT(mpNode); return mpNode->mNameValues.getSize() - 1; }
         BValue getAttribute(BAttributeIndex attributeIndex) const { BDEBUG_ASSERT(attributeIndex < getNumAttributes()); return BValue(mpNode, attributeIndex + 1); }
         
         bool findAttribute(const char* pName, BValue& value) const
         {
            BDEBUG_ASSERT(mpNode);
            for (uint i = 1; i < mpNode->mNameValues.size(); i++)
               if (strcmp(pName, mpNode->mNameValues[i].mName.getPtr()) == 0)
               {
                  value.init(mpNode, i);
                  return true;
               }
            value.clear();
            return false;
         }
         
         BValue addAttribute(const char* pName, const void* pValueData, uint valueDataSizeInBytes, const BTypeDesc& typeDesc, uint arraySize);  

         BValue addAttribute(const char* pName);

         template<typename T>                     
         BValue addAttribute(const char* pName, T value) { return addAttribute(pName, &value, sizeof(T), BGetTypeDesc<T>::get(), 1); }

         BValue addAttribute(const char* pName, const char* pValue);
         BValue addAttribute(const char* pName, const wchar_t* pValue);
         BValue addAttribute(const char* pName, const BString& value) { return addAttribute(pName, value.getPtr()); }
         BValue addAttribute(const char* pName, const BUString& value) { return addAttribute(pName, value.getPtr()); }
         
         BValue addAttribute(const char* pName, const void* pValueData, uint dataLen);
         BValue addAttribute(const char* pName, const BDynamicArray<BString>& strings);
         
         bool deleteAttribute(BAttributeIndex attributeIndex);
         
         bool getNameValueMap(BNameValueMap& nameValueMap, bool asAttributes) const;
         bool addNameValueMap(const BNameValueMap& nameValueMap, bool asAttributes);
      
      private:
         BDocumentBuilder* mpBuilder;
      };
     
      BDocumentBuilder();

      // Deletes all nodes, recreates root node. Root node will be named "root".            
      void              clear();
      
      BUserSectionIndex addUserSection(uint32 id, const void* pData, uint dataLen);
      
      bool              serialize(BByteArray& data, bool bigEndian);
      bool              deserialize(const BConstDataBuffer& data);
      
      // The dumpToXML() method is (currently) intended for debugging.
      // It's possible for dumpToXML() to create malformed XML files, depending on the element/attribute names chosen by the user.
      bool              dumpToXML(BTextDispatcher& textDispatcher, bool writeTypeDescAttributes = true);
                  
      uint              getNumNodes() const;
      BNode             getRootNode() { return BNode(this, mpRootNode); }
                                               
      // Deletes node and all children. 
      // Cannot delete the root node!
      bool              deleteNode(BNode node);
                                                            
   private:
      bool              packName(const BString& name, BStringHashMap& nameHash, BByteArray& nameData, uint& nameOfs);
      bool              packVariant(const BBuildVariantData& variant, BBuildVariantDataHashMap& variantHash, bool bigEndian, BByteArray& variantData, uint& variantOfs);
      bool              createNewNodes(BDynamicArray<BBuildNode>& newNodes);
      bool              createPackedNodes(bool bigEndian, BDynamicArray<BPackedNameValue>& packedNameValues, BDynamicArray<BPackedNode>& packedNodes, BByteArray& nameData, BByteArray& variantData);
      bool              deserializeInternal(const BConstDataBuffer& data);
      bool              dumpNodeToXML(BBuildNode* pNode, BTextDispatcher& textDispatcher, uint indent, bool writeTextDescAttributes) const;
      BValue            getValue(BNode nodeIndex, int attributeIndex);
      BNodeIndex        getNodeIndex(const BBuildNode* pNode) const;
      bool              isValidNode(const BBuildNode* pNode) const;
      
      BDocumentBuilder(const BDocumentBuilder&);
      BDocumentBuilder& operator= (const BDocumentBuilder&);
   };
      
}

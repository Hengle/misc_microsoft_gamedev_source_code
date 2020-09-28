//============================================================================
//
// File: binaryDataTreeReader.h
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once

namespace BBinaryDataTree
{
   class BPackedDocumentReader
   {
   public:
      class BValue
      {
      public:
         BValue() : mpReader(NULL), mNameValueIndex(UINT_MAX) { }
         BValue(const BPackedDocumentReader* pReader, uint nameValueIndex) : mpReader(pReader), mNameValueIndex(nameValueIndex) { }

         bool isValid() const { return NULL != mpReader; }
         
         void clear() { mpReader = NULL; mNameValueIndex = UINT_MAX; }
         
         void init(const BPackedDocumentReader* pReader, uint nameValueIndex) { mpReader = pReader; mNameValueIndex = nameValueIndex; }

         const BPackedDocumentReader* getReader() const { return mpReader; }
         uint getNameValueIndex() const { return mNameValueIndex; }

         const char* getName() const;
         int compareName(const char* p) const { return _stricmp(getName(), p); }

         uint getDataSize() const;
         const void* getDataPtr() const;

         eTypeClass getTypeClass() const;
         bool isNull() const;
         bool isBool() const;
         bool isNumeric() const;
         bool isFloat() const;
         bool isUnsigned() const;
         bool isString() const;

         uint getTypeSizeLog2() const;
         uint getTypeSize() const;

         uint getArraySize() const;

         bool get(uint8& value, uint arrayIndex = 0, uint8 def = 0) const;
         bool get(int8& value, uint arrayIndex = 0, int8 def = 0) const;
         bool get(uint16& value, uint arrayIndex = 0, uint16 def = 0) const;
         bool get(int16& value, uint arrayIndex = 0, int16 def = 0) const;
         bool get(uint32& value, uint arrayIndex = 0, uint32 def = 0) const;
         bool get(int32& value, uint arrayIndex = 0, int32 def = 0) const;
         bool get(uint64& value, uint arrayIndex = 0, uint64 def = 0) const;
         bool get(int64& value, uint arrayIndex = 0, int64 def = 0) const;
         bool get(bool& value, uint arrayIndex = 0, bool def = false) const;
         bool get(float& value, uint arrayIndex = 0, float def = 0) const;
         bool get(double& value, uint arrayIndex = 0, double def = 0) const;
         bool get(BString& value, uint arrayIndex = 0, const BString& def = BString()) const;
         bool get(BUString& value, uint arrayIndex = 0, const BUString& def = BUString()) const;
         
         uint64 asUnsigned(uint arrayIndex = 0, uint64 def = 0) const { uint64 v; get(v, arrayIndex, def); return v; }
         int64 asSigned(uint arrayIndex = 0, int64 def = 0) const { int64 v; get(v, arrayIndex, def); return v; }
         float asFloat(uint arrayIndex = 0, float def = 0) const { float v; get(v, arrayIndex, def); return v; }
         double asDouble(uint arrayIndex = 0, double def = 0) const { double v; get(v, arrayIndex, def); return v; }
         bool asBool(uint arrayIndex = 0, bool def = false) const { bool v; get(v, arrayIndex, def); return v; }
         
         operator bool() const;
         operator uint8() const;
         operator int8() const;
         operator uint16() const;
         operator int16() const;
         operator uint32() const;
         operator int32() const;
         operator uint64() const;
         operator int64() const;
         operator float() const;
         operator double() const;
         operator BString() const;
         operator BUString() const;
                           
      protected:
         const BPackedDocumentReader*  mpReader;
         uint                          mNameValueIndex;
         
         const void* getDataPtrUnchecked() const;
         uint getDataSizeUnchecked() const;
         uint getDataSizeChecked() const;
         eTypeClass getTypeClassUnchecked() const;
         uint getTypeSizeLog2Unchecked() const;
         uint getTypeSizeUnchecked() const;
      };            

      class BNode : public BValue
      {
      public:
         BNode() : BValue(), mNodeIndex(UINT16_MAX), mNumChildren(UINT16_MAX), mNumAttributes(UINT16_MAX) { }
         BNode(const BPackedDocumentReader* pReader, uint nodeIndex) : BValue(pReader, pReader ? pReader->mpNodes[nodeIndex].mNameValueOfs : 0), mNodeIndex(static_cast<uint16>(nodeIndex)), mNumChildren(UINT16_MAX), mNumAttributes(UINT16_MAX) { }
                  
         void clear() { BValue::clear(); mNodeIndex = UINT16_MAX; mNumChildren = UINT16_MAX; mNumAttributes = UINT16_MAX; }
         
         void init(const BPackedDocumentReader* pReader, uint nodeIndex) { BValue::init(pReader, pReader ? pReader->mpNodes[nodeIndex].mNameValueOfs : 0); mNodeIndex = static_cast<uint16>(nodeIndex); mNumChildren = UINT16_MAX; mNumAttributes = UINT16_MAX; }

         uint getNodeIndex() const { return mNodeIndex; }

         const char* getName() const;
         int compareName(const char* p) const { return _stricmp(getName(), p); }
         
         uint getNumChildren() const;
         const char* getChildName(uint index) const;
         BNode getChild(uint index) const;
         bool findChild(const char* pName, BNode& node) const;
         bool doesChildExist(const char* pName) const;

         uint getNumAttributes() const;
         const char* getAttributeName(uint index) const;
         BValue getAttribute(uint index) const;
         bool findAttribute(const char* pName, BValue& value) const;
         bool doesAttributeExist(const char* pName) const;

         template<typename T>
         inline bool getChildValue(const char* pName, T& value, const T& defValue = T()) const
         {
            BNode node;
            if (!findChild(pName, node))
            {
               value = defValue;
               return false;
            }
            return node.get(value);
         }
         
         template<typename T>
         bool getAttributeValue(const char* pName, T& value, const T& defValue = T()) const
         {
            BValue attrib;
            if (!findAttribute(pName, attrib))
            {
               value = defValue;
               return false;
            }
            return attrib.get(value);
         }
         
         bool getNameValueMap(BNameValueMap& nameValueMap, bool asAttributes) const;
         
      private:
         uint16                        mNodeIndex;
         mutable uint16                mNumChildren;
         mutable uint16                mNumAttributes;
      };
   
      BPackedDocumentReader();
      BPackedDocumentReader(const BConstDataBuffer& dataBuffer);
      
      bool isValid() const { return NULL != mDataBuffer.getPtr(); }
      
      bool set(const BConstDataBuffer& dataBuffer);
      
      void clear();
      
      const BConstDataBuffer& getDataBuffer() const { return mDataBuffer; };
      
      uint getNumNodes() const { return mNumNodes; }
            
      uint getNumUserSections() const;
      uint32 getUserSectionID(uint index) const;
      uint32 getUserSectionSize(uint index) const;
      const void* getUserSectionPtr(uint index) const;
      
      BNode getRoot() const;
      BNode getNode(uint nodeIndex) const;
                     
   private:
      BConstDataBuffer        mDataBuffer;
                  
      const BPackedSection*   mpSections;
      
      uint                    mNumNodes;
      const BPackedNode*      mpNodes;
      
      uint                    mNumNameValues;
      const BPackedNameValue* mpNameValues;
      
      uint                    mNameDataSize;
      const BYTE*             mpNameData;
      
      uint                    mValueDataSize;
      const BYTE*             mpValueData;
      
      const BPackedHeader* getHeader() const { return reinterpret_cast<const BPackedHeader*>(mDataBuffer.getPtr()); }
      bool setInternal(const BConstDataBuffer& dataBuffer);
   };
   
}   

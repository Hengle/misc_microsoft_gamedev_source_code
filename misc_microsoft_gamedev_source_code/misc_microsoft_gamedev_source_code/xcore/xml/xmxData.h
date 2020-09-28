//------------------------------------------------------------------------------------------------------------------------
//
//  File: xmxData.h
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#pragma once

#include "utils\packedArray.h"
#include "math\vector.h"

const uint cXMXECFFileID         = 0xE43ABC00;
const uint cXMXPackedDataChunkID = 0xA9C96500;
const uint cXMXFileInfoChunkID   = 0xA9C96501;

enum eXMXVariantType
{
   cXMXVTNull = 0,  // Empty string, always direct

   cXMXVTFloat24,   // 24-bit float, always direct
   cXMXVTFloat,     // 32-bit float, always an offset

   cXMXVTInt24,     // 24-bit int, always direct
   cXMXVTInt32,     // 32-bit int, always an offset

   cXMXVTFract24,   // 24-bit fixed point (value multiplied by 10,000), always direct

   cXMXVTDouble,    // 64-bit double, always an offset

   cXMXVTBool,      // "true" or "false", always direct
   
   cXMXVTString,    // ANSI string, direct or offset
   cXMXVTUString,   // Unicode string, direct or offset

   cXMXVTFloatVec,  // float vector 2, 3, or 4, always an offset

   cXMXVTNumTypes,
   
   cXMXVTTypeMask     = 0x0F,
   
   cXMXVTVecSizeShift = 4,
   cXMXVTVecSizeMask  = 0x30,
   
   cXMXVTUnsignedFlag = 0x40,
   cXMXVTOffsetFlag   = 0x80,
};

class BXMXVariantHelpers
{
public:
   static bool packFract24(double d, uint& val);
   static double unpackFract24(uint val);
   static bool packFloat24(float f, uint& newBits);
   static float unpackFloat24(uint i);
   
   static uint getVariantBits(uint variantValue) { return variantValue & 0xFFFFFF; }
   static uint getVariantTypeBits(uint variantValue) { return variantValue >> 24; }
   static eXMXVariantType getVariantType(uint variantValue) { return (eXMXVariantType)((variantValue >> 24) & cXMXVTTypeMask); }
   static bool getVariantIsOffset(uint variantValue) { return (getVariantTypeBits(variantValue) & cXMXVTOffsetFlag) != 0; }
   static bool getVariantIsUnsigned(uint variantValue) { return (getVariantTypeBits(variantValue) & cXMXVTUnsignedFlag) != 0; }
   
   static uint getVariantVecSize(uint variantValue)
   {
      if (getVariantType(variantValue) != cXMXVTFloatVec)
         return 0;
      return 1U + ((getVariantTypeBits(variantValue) & cXMXVTVecSizeMask) >> cXMXVTVecSizeShift);
   }
   
   // If unicode is true, pBuf will be filled with the Unicode string, otherwise it'll be ANSI.
   static bool unpackVariantToString(char* pBuf, uint bufSize, bool& unicode, const BConstDataBuffer& variantData, uint variantValue, bool permitUncode);
   
   // Returns a pointer within the variant data array, or a pointer to pBuf if the variant had to be unpacked.
   // If pBuf is NULL, and the variant is not an ANSI string encoded as an offset, or a null type, NULL will be returned.
   static const char* getVariantAsANSIString(char* pBuf, uint bufSize, const BConstDataBuffer& variantData, uint variantValue);
   
   static uint getVariantMaxStringBufferSize(const BConstDataBuffer& variantData, uint variantValue);
   
   // cXMXVTFloatVec is considered numeric.
   static bool isNumericVariant(uint variantValue);
   
   static bool isStringVariant(uint variantValue);
   
   static bool convertVariantToFloat(uint variantValue, float& val, const BConstDataBuffer& variantDataArray);
   static bool convertVariantToInt(uint variantValue, int& val, const BConstDataBuffer& variantDataArray);   
   static bool convertVariantToUInt(uint variantValue, uint& val, const BConstDataBuffer& variantDataArray);   
   static bool convertVariantToBool(uint variantValue, bool& val, const BConstDataBuffer& variantDataArray);   
   static bool convertVariantToVector(uint variantValue, BVector& val, const BConstDataBuffer& variantDataArray);
};

template<bool Packed, bool BigEndian>
class BXMXData
{
   friend class BXMXData<Packed, !BigEndian>;
   friend class BXMXData<!Packed, false>;   
   friend class BXMXData<!Packed, true>;   
   
public:
   enum { cSig = 0x71439800 };
   
   typedef PACKED_ARRAY_TYPE(uchar)    UCharArrayType;
   typedef PACKED_TYPE(uint)           UIntType;
   typedef PACKED_ARRAY_TYPE(UIntType) UIntArrayType;
  
   struct BVariant
   {
      BVariant() { }
      BVariant(uint i) : mData(i) { }
      
      void set(uint i) { mData = i; }
      uint get(void) const { return mData; }
      
      operator uint() const { return mData; }
      
      UIntType mData;
   };
         
   struct BAttribute
   {
      BVariant mName;
      BVariant mText;   
   };
   
   typedef PACKED_ARRAY_TYPE(BAttribute) AttributeArrayType;

   struct BNode
   {
      UIntType             mParentNode;
      BVariant             mName;
      BVariant             mText;
      AttributeArrayType   mAttributes;
      UIntArrayType        mChildren;
      
      bool pack(BPackState& state)
      {
         if (!mAttributes.pack(state))
            return false;
         if (!mChildren.pack(state))
            return false;
         return true;
      }
      
      bool unpack(const BDataBuffer& buf)
      {
         if (!mAttributes.unpack(buf))
            return false;
         if (!mChildren.unpack(buf))
            return false;
         return true;
      }
   };
         
   typedef PACKED_ARRAY_TYPE(BNode) NodeArrayType;
               
   void clear(void)
   {
      mSig = 0;
      mNodes.clear();
      mVariantData.clear();
   }
         
   uint getNumNodes(void) const { return mNodes.getSize(); }
   const BNode& getNode(uint i) const  { return mNodes[i]; }
         BNode& getNode(uint i)        { return mNodes[i]; }

   const NodeArrayType& getNodeArray(void) const   { return mNodes; }         
         NodeArrayType& getNodeArray(void)         { return mNodes; }         
         
   const UCharArrayType& getVariantData(void) const   { return mVariantData; }
         UCharArrayType& getVariantData(void)         { return mVariantData; }   
   
   bool pack(BPackState& state)
   {
      if (!mNodes.pack(state))
         return false;
      if (!mVariantData.pack(state))
         return false;
      mSig = cSig;         
      return true;
   }
   
   bool unpack(const BDataBuffer& buf)
   {
      if (mSig != cSig)
         return false;
      if (!mNodes.unpack(buf))
         return false;
      if (!mVariantData.unpack(buf))
         return false;
      
      const uint cMaxPracticalNodes = 100000000;
      if ((!mNodes.getSize()) || (mNodes.getSize() >= cMaxPracticalNodes))
         return false;

      const uint cMaxVariantDataSize = 0xFFFFFF;
      if (mVariantData.getSize() > cMaxVariantDataSize)
         return false;
                  
      return true;
   }

private:
   UIntType                mSig;
   NodeArrayType           mNodes;
   UCharArrayType          mVariantData;   
};

template<> struct BPackHelper  < typename BXMXData<false, false>::BNode >  { static bool pack( BXMXData<false, false>::BNode& r, BPackState& s) { return r.pack(s); } };
template<> struct BPackHelper  < typename BXMXData<false, true >::BNode >  { static bool pack( BXMXData<false, true>::BNode& r, BPackState& s) { return r.pack(s); } }; 
template<> struct BUnpackHelper< typename BXMXData<true,  false>::BNode >  { static bool unpack( BXMXData<true, false>::BNode& r, const BDataBuffer& buf) { return r.unpack(buf); } };
template<> struct BUnpackHelper< typename BXMXData<true,  true >::BNode >  { static bool unpack( BXMXData<true, true>::BNode& r, const BDataBuffer& buf) { return r.unpack(buf); } };

DEFINE_PACKABLE_TYPE(BXMXData);

typedef BXMXData<true, cBigEndianNative> BPackedXMXData;
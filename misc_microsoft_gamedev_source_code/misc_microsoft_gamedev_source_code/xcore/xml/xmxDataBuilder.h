//------------------------------------------------------------------------------------------------------------------------
//  File: xmxDataBuilder.h
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#pragma once

#include "xml\xmxData.h"
#include "xml\xmlDocument.h"

struct BXMXStats 
{
   uint mNumNodes;
   uint mNumAttributes;

   uint mNumInputVariants;
   uint mNumRedundantVariants;

   uint mNumDirectVariants;
   uint mNumIndirectVariants;

   uint mNumBool;
   uint mNumInt24;
   uint mNumFract24;
   uint mNumInt32;
   uint mNumFloat24;
   uint mNumFloat;
   uint mNumDouble;
   uint mNumDirectAnsiStrings;
   uint mNumIndirectAnsiStrings;
   uint mNumIndirectUnicodeStrings;
   uint mNumIndirectStringBytes;
   uint mNumNull;
   uint mNumVariantBytes;
   uint mNumFloatVecs;
};

template<bool BigEndian>
class BXMXDataBuilder
{
   BXMXDataBuilder(const BXMXDataBuilder&);
   BXMXDataBuilder& operator= (const BXMXDataBuilder&);
      
public:
   typedef BXMXData<false, BigEndian>  BXMXDataType;
   
   BXMXDataBuilder(const BXMLDocument& xmlReader, bool forceStringVariants = false, bool permitUnicodeStrings = false, bool forceUnicodeStrings = false) :
      mXMLReader(xmlReader),
      mSuccess(false),
      mPermitUnicodeStrings(permitUnicodeStrings),
      mForceUnicodeStrings(forceUnicodeStrings)
   {
      if (mForceUnicodeStrings)
         mPermitUnicodeStrings = true;
         
      Utils::ClearObj(mStats);
      
      BDEBUG_ASSERT(mXMLReader.getValid());
      
      mSuccess = compress(forceStringVariants);
      
      if (!mSuccess)
         mPackedData.clear();
   }
   
   const BXMLDocument& getXMLReader(void) const { return mXMLReader; }
   const BByteArray& getPackedData(void) const { return mPackedData; }
   
   bool getSuccess(void) const { return mSuccess; }

   const BXMXStats& getStats(void) const { return mStats; }

private:
   const BXMLDocument& mXMLReader;
   
   BXMXDataType            mXMXData;
         
   typedef BHashMap<BString, uint, BHasher<BString>, BStringEqualsCaseSensitive> BVariantHashMap;
   BVariantHashMap         mUncompressedHashMap;
   BVariantHashMap         mCompressedHashMap;
   
   typedef BHashMap<BUString, uint, BHasher<BUString>, BUStringEqualsCaseSensitive> BVariantUnicodeHashMap;
   BVariantUnicodeHashMap  mUncompressedUnicodeHashMap;
   
   BXMXStats               mStats;
   
   BByteArray              mPackedData;
      
   bool                    mPermitUnicodeStrings : 1;
   bool                    mForceUnicodeStrings : 1;
   
   bool                    mSuccess : 1;
   
   bool compress(bool forceStringVariants)
   {
      mXMXData.clear();
      mUncompressedHashMap.clear();
      mUncompressedUnicodeHashMap.clear();
      mCompressedHashMap.clear();
      mPackedData.clear();
      Utils::ClearObj(mStats);
            
      const uint numNodes = mXMLReader.getNumNodes();
      mStats.mNumNodes = numNodes;
      const BXMLDocument::BNodeArray& srcNodeArray = mXMLReader.getNodePool();
      BXMXDataType::NodeArrayType& dstNodeArray = mXMXData.getNodeArray();
      
      dstNodeArray.resize(numNodes);
      
      for (uint nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
      {
         const BXMLDocument::BNode& srcNode = srcNodeArray[nodeIndex];
         BXMXDataType::BNode& dstNode = dstNodeArray[nodeIndex];

         dstNode.mParentNode = srcNode.getParentNodeIndex();
         
         uint v;
         if (!compressVariant(srcNode.getName(), srcNode.getUName(), v, true, false)) 
            return false;
         dstNode.mName.set(v);
         BDEBUG_ASSERT( (BXMXVariantHelpers::getVariantType(v) == cXMXVTNull) || ((BXMXVariantHelpers::getVariantType(v) == cXMXVTString) && (BXMXVariantHelpers::getVariantIsOffset(v))) );
         
         if (!compressVariant(srcNode.getText(), srcNode.getUText(), v, srcNode.getTextIsCData() || forceStringVariants, true)) 
            return false;
         dstNode.mText.set(v);
         
         dstNode.mAttributes.resize(srcNode.getNumAttributes());
         mStats.mNumAttributes += srcNode.getNumAttributes();
         
         for (uint attributeIndex = 0; attributeIndex < srcNode.getNumAttributes(); attributeIndex++)
         {
            if (!compressVariant(srcNode.getAttribute(attributeIndex).mName, srcNode.getAttribute(attributeIndex).mUName, v, true, false)) 
               return false;
            dstNode.mAttributes[attributeIndex].mName.set(v);
            BDEBUG_ASSERT( (BXMXVariantHelpers::getVariantType(v) == cXMXVTNull) || ((BXMXVariantHelpers::getVariantType(v) == cXMXVTString) && (BXMXVariantHelpers::getVariantIsOffset(v))) );
            
            if (!compressVariant(srcNode.getAttribute(attributeIndex).mText, srcNode.getAttribute(attributeIndex).mUText, v, forceStringVariants, true)) 
               return false;
            dstNode.mAttributes[attributeIndex].mText.set(v);
         }
         
         dstNode.mChildren.resize(srcNode.getNumChildren());
         for (uint childIndex = 0; childIndex < srcNode.getNumChildren(); childIndex++)
            dstNode.mChildren[childIndex] = srcNode.getChildrenArray()[childIndex];
      }
      
      mStats.mNumVariantBytes = mXMXData.getVariantData().getSize();
      
      BMemStack memStack(&gPrimaryHeap, 32*1024*1024);
      BPackState packState(memStack);
      
      BXMXDataType* p = new(memStack) BXMXDataType(mXMXData);
      
      if (!p->pack(packState))
         return false;
    
      mPackedData.reserve(memStack.getAllocatedBytes());   
      mPackedData.pushBack(memStack.getBasePtr(), memStack.getAllocatedBytes());
       
      return true;
   }
   
   bool isSignedInteger(const BString& str, int& val, bool twentyFourBit)
   {
      if (str == "0")
      {
         val = 0;
         return true;
      }
      
      char* p;
      val = strtol(str.getPtr(), &p, 10);
      if ((val == 0) || (val == LONG_MIN) || (val == LONG_MAX))
         return false;
      if (*p != '\0') 
         return false;
      
      if (twentyFourBit)
         val &= 0xFFFFFF;
      
      int unpackedVal = val;
      if (unpackedVal & 0x800000)
         unpackedVal |= 0xFF000000;          
               
      char buf[256];
      sprintf_s(buf, sizeof(buf), "%i", unpackedVal);
      if (str == buf)
         return true;
            
      return false;
   }
   
   bool isUnsignedInteger(const BString& str, uint& val, bool twentyFourBit)
   {
      if (str == "0")
      {
         val = 0;
         return true;
      }

      char* p;
      val = strtoul(str.getPtr(), &p, 10);
      if ((val == 0) || (val == ULONG_MAX))
         return false;
      if (*p != '\0')
         return false;
         
      if (twentyFourBit)
         val &= 0xFFFFFF;

      char buf[256];
      sprintf_s(buf, sizeof(buf), "%u", val);
      if (str == buf)
         return true;
      
      return false;
   }
                     
   bool isFloat(const BString& str, uint& val, double relTol, bool twentyFourBit)
   {
      char* p;
      double d = strtod(str.getPtr(), &p);
      if (d == 0.0f)
         return false;
      else if ((d == +HUGE_VAL) || (d == -HUGE_VAL))
         return false;
      
      if (*p != '\0')
         return false;
         
      float f = (float)d;
      if (twentyFourBit)
      {
         if (!BXMXVariantHelpers::packFloat24(f, val))
            return false;
         
         f = BXMXVariantHelpers::unpackFloat24(val);
      }
      else
         val = *reinterpret_cast<uint*>(&f);
      
      if (!Math::IsValidFloat(f))
         return false;
      
      return Math::EqualRelTol<double>(d, f, relTol);
   }
         
   bool isDouble(const BString& str, uint64& val)
   {
      if ((str == "0") || (str == "0.0") || (str == "+0.0"))
      {
         val = 0;
         return true;
      }

      char* p;
      double d = strtod(str.getPtr(), &p);
      if ((d == 0.0f) || (d == +HUGE_VAL) || (d == -HUGE_VAL))
         return false;
         
      if (*p != '\0')
         return false;         
                    
      val = *reinterpret_cast<uint64*>(&d);
      
      return true;
   }
         
   bool isFract24(const BString& str, uint& val, double relTol)
   {
      uint64 dval;
      if (!isDouble(str, dval))
         return false;
         
      double d = *reinterpret_cast<const double*>(&dval);
      
      if (!BXMXVariantHelpers::packFract24(d, val))
         return false;
      
      double unpackedD = BXMXVariantHelpers::unpackFract24(val);
            
      return Math::EqualRelTol<double>(d, unpackedD, relTol);
   }
   
   bool compressVariant(const BString& str, const BUString& ustr, uint& variant, bool storeUncompressed, bool allowUnicode)
   {
      mStats.mNumInputVariants++;
      
      const uint len = str.length();
      const uint ulen = ustr.length();

      bool hasUnicodeChars = mForceUnicodeStrings && allowUnicode;
      if ((!hasUnicodeChars) && (mPermitUnicodeStrings) && (allowUnicode))
      {
         for (uint i = 0; i < ulen; i++)
         {  
            // First 128 characters are ASCII.
            if (ustr.getChar(i) >= 128)
            {
               hasUnicodeChars = true;
               break;
            }
         }
      }
      
      if (hasUnicodeChars)
         storeUncompressed = true;
      
      if (hasUnicodeChars)
      {
         BVariantUnicodeHashMap::const_iterator it = mUncompressedUnicodeHashMap.find(ustr);
         if (it != mUncompressedUnicodeHashMap.end())
         {
            mStats.mNumRedundantVariants++;

            variant = it->second;
            return true;
         }
      }
      else
      {
         BVariantHashMap& hashMap = storeUncompressed ? mUncompressedHashMap : mCompressedHashMap;
         BVariantHashMap::const_iterator it = hashMap.find(str);
         if (it != hashMap.end())
         {
            mStats.mNumRedundantVariants++;
            
            variant = it->second;
            return true;
         }
      }         
            
      uint vectorSize = 0;
      const uint cMaxVecSize = 4;
      uint vectorValues[cMaxVecSize];
      Utils::ClearObj(vectorValues);
         
      uint variantType = cXMXVTNull;
      uint64 variantData = 0;
      uint numIntegralDigits = 0;
      uint numFractionalDigits = 0;
      uint numSignificantDigits = 0;
      bool isNegativeZero = false;
      if (len)
      {
         if (hasUnicodeChars)
         {
            variantType = cXMXVTUString;
         }
         else if (storeUncompressed)
         {
            variantType = cXMXVTString;
         }
         else if (strcmp(str.getPtr(), "true") == 0)
         { 
            variantType = cXMXVTBool;
            variantData = 1;
         }
         else if (strcmp(str.getPtr(), "false") == 0)
         {
            variantType = cXMXVTBool;
            variantData = 0;
         }
         else if (str == "0")
         {
            variantType = cXMXVTUnsignedFlag | cXMXVTInt24;
            variantData = 0;
         }
         // Purposely disallowing exponential forms because we can't tell if "2d2" is a number or a valid text string!
         else if (Math::isNumeric(str, false, 0, NULL, &numIntegralDigits, &numFractionalDigits, &numSignificantDigits))
         {
            uint uval = 0;
            int ival = 0;
            
            if ((!numFractionalDigits) && (isUnsignedInteger(str, uval, true)))
            {
               variantType = cXMXVTUnsignedFlag | cXMXVTInt24;
               variantData = uval;
            }
            else if ((!numFractionalDigits) && (isUnsignedInteger(str, uval, false)))
            {
               variantType = cXMXVTUnsignedFlag | cXMXVTInt32;
               variantData = uval;
            }
            else if ((!numFractionalDigits) && (isSignedInteger(str, ival, true)))
            {
               variantType = cXMXVTInt24;
               variantData = (uint64)ival;
            }
            else if ((!numFractionalDigits) && (isSignedInteger(str, ival, false)))
            {
               variantType = cXMXVTInt32;
               variantData = (uint64)ival;
            }
            else if (Math::isZero(str, false, &isNegativeZero))
            {
               variantType = cXMXVTFloat24;
               variantData = isNegativeZero ? 0x800000 : 0;
            }
            else if ((numIntegralDigits <= 3) && (numFractionalDigits <= 4) && (isFract24(str, uval, 0.0000001f)))
            {
               variantType = cXMXVTFract24;
               variantData = (uint64)uval;
            }
            else if ((numSignificantDigits <= 6) && (isFloat(str, uval, 0.0000001f, true)))
            {
               variantType = cXMXVTFloat24;
               variantData = uval;
            }
            else if ((numSignificantDigits <= 8) && (isFloat(str, uval, 0.0000001f, false)))
            {
               variantType = cXMXVTFloat;
               variantData = uval;
            }
            else if ((numSignificantDigits <= 17) && (isDouble(str, variantData)))
            {
               variantType = cXMXVTDouble;
            }
            else
            {
               variantType = cXMXVTString;
            }
         }
         else
         {
            bool isFloatVector = true;
                        
            BString temp(str);
            for ( ; ; )
            {
               if (temp.isEmpty())
                  break;
                  
               BString subStr(temp);
               
               int i = temp.findLeft(',');
               if (i == 0)
               {
                  isFloatVector = false;
                  break;
               }
               else if (i == -1)
               {
                  temp.empty();
               }
               else 
               {
                  subStr.crop(0, i - 1);
                  temp.set(BString(temp.getPtr() + i + 1));
               }
               
               uint numSig;
               if (!Math::isNumeric(subStr, false, 0, NULL, NULL, NULL, &numSig))
               {
                  isFloatVector = false;
                  break;
               }
               
               if (numSig > 8)
               {
                  isFloatVector = false;
                  break;
               }
               
               uint val = 0;
               bool isNeg = false;
               if (Math::isZero(subStr, false, &isNeg))
               {
                  val = isNeg ? 0x80000000 : 0;
               }
               else if (!isFloat(subStr, val, 0.0000001f, false))
               {
                  isFloatVector = false;
                  break;
               }
               
               float f = *reinterpret_cast<float*>(&val);
                              
               // Any integer value outside this range will not be encoded as float32 without potential loss, so screw it.               
               if (fabs(f) > 16777215.0f)
               {
                  isFloatVector = false;
                  break;
               }
               
               if (vectorSize == cMaxVecSize)
               {
                  isFloatVector = false;
                  break;
               }
               
               vectorValues[vectorSize] = val;
               vectorSize++;   
            }
            
            if ((isFloatVector) && (vectorSize >= 2))
               variantType = cXMXVTFloatVec;
            else
               variantType = cXMXVTString;
         }
      }

      bool directEncoding = false;
      
      switch (variantType & cXMXVTTypeMask)         
      {
         case cXMXVTNull:
         case cXMXVTFloat24:
         case cXMXVTInt24:
         case cXMXVTFract24:
         case cXMXVTBool:
         {
            directEncoding = true;
            break;
         }
         case cXMXVTString:
         {
            if ((!storeUncompressed) && (len <= 3))
            {
               directEncoding = true;
               
               variantData = 0;
               if (len >= 1)
                  variantData = str.getChar(0);
               if (len >= 2)
                  variantData |= ((uint)str.getChar(1) << 8U);
               if (len == 3)
                  variantData |= ((uint)str.getChar(2) << 16U);
            }
            break;
         }
      }
                     
      if (directEncoding)
      {
         BDEBUG_ASSERT(variantData <= 0xFFFFFF);
      }
      else
      {
         BXMXDataType::UCharArrayType& variantDataArray = mXMXData.getVariantData();
         
         const uint type = (variantType & cXMXVTTypeMask);
         uint align = 4;
         if (type == cXMXVTDouble)   
            align = 8;
         else if (type == cXMXVTString)   
            align = 1;
         else if (type == cXMXVTUString)   
            align = 2;

         const uint bytesToAlignUp = Utils::BytesToAlignUpValue(variantDataArray.getSize(), align);
         for (uint i = 0; i < bytesToAlignUp; i++)
            variantDataArray.pushBack(0);
                  
         const uint variantDataArrayOfs = variantDataArray.getSize();
      
         uint32 variantData32 = static_cast<uint32>(variantData);              
         if (cBigEndianNative != BigEndian)
         {
            Utils::EndianSwitch(&variantData, sizeof(variantData));
            Utils::EndianSwitch(&variantData32, sizeof(variantData32));
         }            
                     
         switch (variantType & cXMXVTTypeMask)
         {
            case cXMXVTDouble:
            {
               variantDataArray.pushBack((uchar*)&variantData, 8);
               break;
            }
            case cXMXVTFloat:
            case cXMXVTInt32:
            {
               variantDataArray.pushBack((uchar*)&variantData32, 4);
               break;
            }
            case cXMXVTString:
            {
               for (uint i = 0; i < len; i++)
                  variantDataArray.pushBack(str.getChar(i));
               variantDataArray.pushBack(0);
               break;
            }
            case cXMXVTUString:
            {
               for (uint i = 0; i < ulen; i++)
               {
                  const WCHAR c = ustr.getChar(i);
                  
                  if (BigEndian)
                  {
                     variantDataArray.pushBack((uchar)(c >> 8));
                     variantDataArray.pushBack((uchar)(c & 0xFF));
                  }
                  else
                  {
                     variantDataArray.pushBack((uchar)(c & 0xFF));
                     variantDataArray.pushBack((uchar)(c >> 8));
                  }
               }
               variantDataArray.pushBack(0);
               variantDataArray.pushBack(0);
               break;
            }
            case cXMXVTFloatVec:
            {
               if (cBigEndianNative != BigEndian)
               {
                  for (uint i = 0; i < vectorSize; i++)
                     Utils::EndianSwitch(&vectorValues[i], sizeof(vectorValues[i]));
               }
                                 
               for (uint i = 0; i < vectorSize; i++)
                  variantDataArray.pushBack((uchar*)&vectorValues[i], 4);
               
               uint bits = (vectorSize - 1) << cXMXVTVecSizeShift;
               BDEBUG_ASSERT(bits <= cXMXVTVecSizeMask);
               variantType |= bits;
               break;
            }
            default:
            {
               BDEBUG_ASSERT(0);
               return false;
            }
         }
         
         if (variantDataArray.getSize() > 0xFFFFFF)
            return false;
         
         variantType |= cXMXVTOffsetFlag;
         variantData = variantDataArrayOfs;
      }
      
      if ((variantType & cXMXVTOffsetFlag) == 0)
         mStats.mNumDirectVariants++;
      else
         mStats.mNumIndirectVariants++;
      
      switch (variantType & cXMXVTTypeMask)
      {
         case cXMXVTInt24:
            mStats.mNumInt24++;
            break;
         case cXMXVTFract24:
            mStats.mNumFract24++;
            break;
         case cXMXVTInt32:
            mStats.mNumInt32++;
            break;
         case cXMXVTFloat24:
            mStats.mNumFloat24++;
            break;
         case cXMXVTFloat:
            mStats.mNumFloat++;
            break;
         case cXMXVTDouble:
            mStats.mNumDouble++;
            break;
         case cXMXVTNull:
            mStats.mNumNull++;
            break;
         case cXMXVTBool:
         {
            mStats.mNumBool++;
            break;
         }
         case cXMXVTUString:
         {
            mStats.mNumIndirectUnicodeStrings++;
            mStats.mNumIndirectStringBytes += sizeof(WCHAR) * (ulen + 1);
            
            break;
         }
         case cXMXVTString:
         {
            if (variantType & cXMXVTOffsetFlag)
            {
               mStats.mNumIndirectAnsiStrings++;
               mStats.mNumIndirectStringBytes += (len + 1);
            }
            else
               mStats.mNumDirectAnsiStrings++;
               
            break;
         }
         case cXMXVTFloatVec:
         {
            mStats.mNumFloatVecs++;
            break;
         }
         
      }
      
      BDEBUG_ASSERT(variantData <= 0xFFFFFF);
      variantData = (variantType << 24U) | variantData;
      
      variant = (uint)variantData;

      if (hasUnicodeChars)
      {
         BDEBUG_ASSERT(storeUncompressed);
         mUncompressedUnicodeHashMap.insert(ustr, variant);
      }
      else
      {
         BVariantHashMap& hashMap = storeUncompressed ? mUncompressedHashMap : mCompressedHashMap;
         hashMap.insert(str, variant);
      }
            
      return true;
   }
   
};

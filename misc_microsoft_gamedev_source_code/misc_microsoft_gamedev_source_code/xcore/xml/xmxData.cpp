//------------------------------------------------------------------------------------------------------------------------
//
//  File: xmxData.cpp
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#include "xcore.h"
#include "xmxData.h"
#include "string\convertToken.h"

bool BXMXVariantHelpers::packFract24(double d, uint& val)
{
   val = 0;
   if (d < 0.0f)
   {
      val |= 0x800000;
      d = -d;
   }

   d = floor(d * 10000.0f);
   if (d > 0x7FFFFF)
      return false;

   uint i = (uint)d;

   val = val | i;
   return true;
}

double BXMXVariantHelpers::unpackFract24(uint val)
{
   double d = (val & 0x7FFFFF) * (1.0 / 10000.0);
   if (val & 0x800000)
      d = -d;
   return d;
}

bool BXMXVariantHelpers::packFloat24(float f, uint& newBits)
{  
   if (!Math::IsValidFloat(f))
      return false;

   // 0,127
   // s1e6m17

   const uint bits = *reinterpret_cast<const uint*>(&f);
   const uint sign = bits & 0x80000000;
   const uint exponentBits = ((bits >> 23) & 0xFF);
   
   if (exponentBits == 0)
   {
      newBits = (sign != 0) ? 0x800000 : 0x000000;
      return true;
   }
   
   const int exponent = exponentBits - 127;
   const uint mantissa = bits & 0x7FFFFF;

   int newExponent = exponent + 31;
   if ((newExponent < 1) || (newExponent > 63))
      return false;

   uint r = mantissa & 63;
   uint newMantissa = mantissa & ~63;

   if (r >= 32)
   {
      bool inc = false;
      if (r == 32)
      {
         if (newMantissa & 64)
            inc = true;
      }
      else
         inc = true;

      if (inc)
      {
         newMantissa += 64;
         if (newMantissa >= 0x7FFFFF)
         {
            newMantissa = 0;
            newExponent++;
            if (newExponent > 63)
               return false;
         }
      }            
   }

   newMantissa >>= 6;

   BDEBUG_ASSERT((newExponent >= 1 && newExponent <= 63));
   BDEBUG_ASSERT(newMantissa <= 0x1FFFF);
   newBits = (sign ? 0x800000 : 0) | (newExponent << 17) | newMantissa;
   BDEBUG_ASSERT(newBits <= 0xFFFFFF);
   return true; 
}

float BXMXVariantHelpers::unpackFloat24(uint i)
{
   const uint sign = i & 0x800000;
   const int exponentBits = ((i >> 17) & 63);
   if (!exponentBits)
   {
      return sign ? -0.0f : 0.0f;
   }
   
   const int exponent = exponentBits - 31;
   const uint mantissa = i & 0x1FFFF;
      
   const uint newExponent = exponent + 127;

   const uint bits = (sign ? 0x80000000 : 0) | (newExponent << 23) | (mantissa << 6);

   return *reinterpret_cast<const float*>(&bits);
}

bool BXMXVariantHelpers::isNumericVariant(uint variantValue)
{
   switch (getVariantType(variantValue))
   {
      case cXMXVTFloat24:
      case cXMXVTFloat:
      case cXMXVTInt24:
      case cXMXVTInt32:
      case cXMXVTFract24:
      case cXMXVTDouble:
      case cXMXVTFloatVec:
         return true;
   }
   return false;
}

bool BXMXVariantHelpers::isStringVariant(uint variantValue)
{
   switch (getVariantType(variantValue))
   {
      case cXMXVTString:
      case cXMXVTUString:
         return true;
   }
   return false;
}

// rg [3/9/07] - I had to tweak a lot of these conversions methods to emulate how the old Wrench BXMLReader class converted things, particularly when the conversion failed.

bool BXMXVariantHelpers::convertVariantToFloat(uint variantValue, float& val, const BConstDataBuffer& variantDataArray)
{
   const eXMXVariantType variantType = getVariantType(variantValue);
   const uint variantTypeBits = getVariantTypeBits(variantValue);
   const uint variantBits = getVariantBits(variantValue);
   switch (variantType)
   {
      case cXMXVTUString:
      case cXMXVTString:
      {
         char buf[512];
         char* pStr = buf;
         if (((variantTypeBits & cXMXVTOffsetFlag) == 0) || (variantType == cXMXVTUString))
         {
            bool unicode;
            if (!unpackVariantToString(buf, sizeof(buf), unicode, variantDataArray, variantValue, false))
            {
               val = 0.0f;
               return true;
            }
         }
         else
         {
            pStr = (char*)(variantDataArray.getPtr() + variantBits);
         }
         
         if (!strlen(pStr))
            return false;
         
         _CRT_FLOAT cf;
         int result = _atoflt(&cf, pStr);
         if (result)
            val = 0.0f;
         else
            val = cf.f;
                     
         break;
      }
      case cXMXVTNull:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) == 0);
         return false;
      }
      case cXMXVTFloat24:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) == 0);
         val = unpackFloat24(variantBits);
         break;
      }
      case cXMXVTFloat:  
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
         val = *(const float*)(variantDataArray.getPtr() + variantBits);
         break;
      }
      case cXMXVTInt24:  
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) == 0);
         if (variantTypeBits & cXMXVTUnsignedFlag)
            val = (float)variantBits;
         else
         {
            int ival = (int)variantBits;
            if (ival & 0x800000)
               ival |= 0xFF000000;
            val = (float)ival;               
         }
         break;
      }
      case cXMXVTInt32:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
         if (variantTypeBits & cXMXVTUnsignedFlag)
            val = (float)*(const uint*)(variantDataArray.getPtr() + variantBits);
         else
            val = (float)*(const int*)(variantDataArray.getPtr() + variantBits);
         break;            
      }
      case cXMXVTFract24:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) == 0);
         val = (float)unpackFract24(variantBits);
         break;
      }
      case cXMXVTDouble:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
         val = (float)*(const double*)(variantDataArray.getPtr() + variantBits);
         break;
      }
      case cXMXVTBool: 
      {
         val = 0.0f;
         break;
      }
      case cXMXVTFloatVec:
      {  
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
         val = (float)*(const float*)(variantDataArray.getPtr() + variantBits);
         break;
      }
      default:
      {
         BDEBUG_ASSERT(0);
         val = 0.0f;
         break;
      }
   }
   
   return true;
}

bool BXMXVariantHelpers::convertVariantToInt(uint variantValue, int& val, const BConstDataBuffer& variantDataArray)   
{
   const eXMXVariantType variantType = getVariantType(variantValue);
   const uint variantTypeBits = getVariantTypeBits(variantValue);
   const uint variantBits = getVariantBits(variantValue);
   switch (variantType)
   {
      case cXMXVTUString:
      case cXMXVTString:
      {
         char buf[512];
         const char* pStr = buf;
         if (((variantTypeBits & cXMXVTOffsetFlag) == 0) || (variantType == cXMXVTUString))
         {
            bool unicode;
            if (!unpackVariantToString(buf, sizeof(buf), unicode, variantDataArray, variantValue, false))
            {
               val = 0;
               return true;
            }
         }
         else
         {
            pStr = (const char*)(variantDataArray.getPtr() + variantBits);
         }
         
         if (!strlen(pStr))
            return false;
            
         val = atoi(pStr);
         break;
      }
      case cXMXVTNull:
      {
         return false;
      }
      case cXMXVTFloat24:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) == 0);
         val = (int)unpackFloat24(variantBits);
         break;
      }
      case cXMXVTFloat:  
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
         val = (int)*(const float*)(variantDataArray.getPtr() + variantBits);
         break;
      }
      case cXMXVTInt24:  
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) == 0);
         if (variantTypeBits & cXMXVTUnsignedFlag)
            val = (int)variantBits;
         else
         {
            val = (int)variantBits;
            if (val & 0x800000)
               val |= 0xFF000000;
         }
         break;
      }
      case cXMXVTInt32:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
         if (variantTypeBits & cXMXVTUnsignedFlag)
            val = (int)*(const uint*)(variantDataArray.getPtr() + variantBits);
         else
            val = (int)*(const int*)(variantDataArray.getPtr() + variantBits);
         break;            
      }
      case cXMXVTFract24:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) == 0);
         val = (int)unpackFract24(variantBits);
         break;
      }
      case cXMXVTDouble:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
         val = (int)*(const double*)(variantDataArray.getPtr() + variantBits);
         break;
      }
      case cXMXVTBool: 
      {
         val = 0;
         break;
      }
      case cXMXVTFloatVec:
      {  
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
         val = (int)*(const float*)(variantDataArray.getPtr() + variantBits);
         break;
      }
      default:
      {
         BDEBUG_ASSERT(0);
         val = 0;
         break;
      }
   }
   
   return true;
}

bool BXMXVariantHelpers::convertVariantToUInt(uint variantValue, uint& val, const BConstDataBuffer& variantDataArray)
{
   const eXMXVariantType variantType = getVariantType(variantValue);
   const uint variantTypeBits = getVariantTypeBits(variantValue);
   const uint variantBits = getVariantBits(variantValue);
   switch (variantType)
   {
      case cXMXVTUString:
      case cXMXVTString:
      {
         char buf[512];
         const char* pStr = buf;
         if (((variantTypeBits & cXMXVTOffsetFlag) == 0) || (variantType == cXMXVTUString))
         {
            bool unicode;
            if (!unpackVariantToString(buf, sizeof(buf), unicode, variantDataArray, variantValue, false))
            {
               val = 0;
               return true;
            }
         }
         else
         {
            pStr = (const char*)(variantDataArray.getPtr() + variantBits);
         }
         
         if (!strlen(pStr))
            return false;
         
         int64 i = _atoi64(pStr);
         if (i < 0) 
            i = 0;
          else if (i > 0xFFFFFFFF)
            i = 0xFFFFFFFF;
             
         val = (uint)i;
         break;
      }
      case cXMXVTNull:
      {
         return false;
      }
      case cXMXVTFloat24:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) == 0);
         float f = unpackFloat24(variantBits);
         if (f < 0.0f) 
            val = 0;
         else if (f > 0xFFFFFFFF)
            val = 0xFFFFFFFF;
         else
            val = (uint)f;
         break;
      }
      case cXMXVTFloat:  
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
         float f = *(const float*)(variantDataArray.getPtr() + variantBits);
         if (f < 0.0f) 
            val = 0;
         else if (f > 0xFFFFFFFF)
            val = 0xFFFFFFFF;
         else
            val = (uint)f;
         break;
      }
      case cXMXVTInt24:  
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) == 0);
         if (variantTypeBits & cXMXVTUnsignedFlag)
            val = variantBits;
         else
         {
            int ival = (int)variantBits;
            if (ival & 0x800000)
               val = 0;
            val = (uint)ival;
         }
         break;
      }
      case cXMXVTInt32:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
         if (variantTypeBits & cXMXVTUnsignedFlag)
            val = *(const uint*)(variantDataArray.getPtr() + variantBits);
         else
         {
            int ival = *(const int*)(variantDataArray.getPtr() + variantBits);
            if (ival < 0)
               val = 0;
            val = (uint)ival;
         }
         break;            
      }
      case cXMXVTFract24:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) == 0);
         float f = (float)unpackFract24(variantBits);
         if (f < 0.0f) 
            val = 0;
         else if (f > 0xFFFFFFFF)
            val = 0xFFFFFFFF;
         else
            val = (uint)f;
         break;
      }
      case cXMXVTDouble:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
         double f = *(const double*)(variantDataArray.getPtr() + variantBits);
         if (f < 0.0f) 
            val = 0;
         else if (f > 0xFFFFFFFF)
            val = 0xFFFFFFFF;
         else
            val = (uint)f;
         break;
      }
      case cXMXVTBool: 
      {
         val = 0;
         break;
      }
      case cXMXVTFloatVec:
      {  
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
         float f = *(const float*)(variantDataArray.getPtr() + variantBits);
         if (f < 0.0f) 
            val = 0;
         else if (f > 0xFFFFFFFF)
            val = 0xFFFFFFFF;
         else
            val = (uint)f;
         break;
      }
      default:
      {
         BDEBUG_ASSERT(0);
         val = 0;
         break;
      }
   }
   
   return true;
}

bool BXMXVariantHelpers::convertVariantToBool(uint variantValue, bool& val, const BConstDataBuffer& variantDataArray)
{
   const eXMXVariantType variantType = getVariantType(variantValue);
   const uint variantTypeBits = getVariantTypeBits(variantValue);
   const uint variantBits = getVariantBits(variantValue);
   variantTypeBits;
   
   switch (variantType)
   {
      case cXMXVTBool:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) == 0);
         val = variantBits != 0;
         break;
      }
      case cXMXVTInt24:
      {
         BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) == 0);
         if (0 == variantBits)
            val = false;
         else if (1 == variantBits)
            val = true;
         else
            return false;
         break;
      }
      default:
      {
         char buf[512];
         bool unicode;
         if (!unpackVariantToString(buf, sizeof(buf), unicode, variantDataArray, variantValue, false))
            return false;
            
         if (!convertTokenToBool(buf, val))
            return false;
            
         break;
      }
   }
   
   return true;
}

bool BXMXVariantHelpers::convertVariantToVector(uint variantValue, BVector& val, const BConstDataBuffer& variantDataArray)
{
   const eXMXVariantType variantType = getVariantType(variantValue);
   const uint variantTypeBits = getVariantTypeBits(variantValue);
   const uint variantBits = getVariantBits(variantValue);
   variantTypeBits;
   
   if (variantType != cXMXVTFloatVec)
   {
      char buf[512];
      const char* pStr = getVariantAsANSIString(buf, sizeof(buf), variantDataArray, variantValue);
      if (!pStr)
      {
         val.set(0.0f, 0.0f, 0.0f);
         return false;
      }
      
      if (sscanf_s(pStr, "%f,%f,%f", &val.x, &val.y, &val.z) < 3)
      {
         val.set(0.0f, 0.0f, 0.0f);
         return false;
      }
            
      return true;
   }
   
   if (getVariantVecSize(variantValue) != 3)
   {
      val.set(0.0f, 0.0f, 0.0f);
      return false;
   }
   
   BDEBUG_ASSERT((variantTypeBits & cXMXVTOffsetFlag) != 0);
   
   const float* pFloats = (const float*)(variantDataArray.getPtr() + variantBits);
   
   val.set(pFloats[0], pFloats[1], pFloats[2]);
   
   return true;
}

const char* BXMXVariantHelpers::getVariantAsANSIString(char* pBuf, uint bufSize, const BConstDataBuffer& variantData, uint variantValue)
{
   const eXMXVariantType variantType = getVariantType(variantValue);
   const uint variantTypeBits = getVariantTypeBits(variantValue);
   const uint variantBits = getVariantBits(variantValue);
   
   if ((variantType == cXMXVTString) && (variantTypeBits & cXMXVTOffsetFlag))
      return (const char*)(variantData.getPtr() + variantBits);
   else if (variantType == cXMXVTNull)
      return "";
   
   if (!pBuf)
      return NULL;  
      
   bool unicode;
   if (!unpackVariantToString(pBuf, bufSize, unicode, variantData, variantValue, false))
      return NULL;
   
   return pBuf;
}

bool BXMXVariantHelpers::unpackVariantToString(char* pBuf, uint bufSize, bool& unicode, const BConstDataBuffer& variantData, uint variantValue, bool permitUncode)
{
   const BYTE* pVariantData = variantData.getPtr();
   const uint variantDataSize = variantData.getLen();

   unicode = false;
   
   if (!bufSize)
      return false;
   
   const uint variantTypeBits = getVariantTypeBits(variantValue);
   const uint variantBits = getVariantBits(variantValue);
   
   const BOOL offsetFlag = variantTypeBits & cXMXVTOffsetFlag;
   const uint variantType = variantTypeBits & cXMXVTTypeMask;
      
   const BOOL unsignedFlag = variantTypeBits & cXMXVTUnsignedFlag;
   const uint vecSize = 1 + ((variantTypeBits & cXMXVTVecSizeMask) >> cXMXVTVecSizeShift);
   
   uint strLen = 0;
   
   if (offsetFlag)
   {
      switch (variantType)
      {
         case cXMXVTFloat:    // always an offset
         case cXMXVTInt32:    // always an offset
         {
            if (variantBits & 3)
               return false;
               
            if ((variantBits + sizeof(float)) > variantDataSize)
               return false;
            break;
         }
         case cXMXVTDouble:   // always an offset
         {
            if (variantBits & 7)
               return false;
               
            if ((variantBits + sizeof(double)) > variantDataSize)
               return false;
            break;
         }
         case cXMXVTString:   // direct or offset
         {
            for ( ; ; )
            {
               if ((strLen + variantBits) >= variantDataSize)
                  return false;
               
               if (!pVariantData[strLen + variantBits])
                  break;
               
               strLen++;
            }
            break;
         }
         case cXMXVTUString:   // direct or offset
         {
            if (variantBits & 1)
               return false;
               
            for ( ; ; )
            {
               if ((variantBits + strLen * sizeof(WCHAR)) >= variantDataSize)
                  return false;
               
               const WCHAR c = *(WCHAR*)(pVariantData + variantBits + strLen * sizeof(WCHAR));
               if (!c)
                  break;
               
               strLen++;
            }
            break;
         }
         case cXMXVTFloatVec: // always an offset     
         {
            if (variantBits & 3)
               return false;
               
            if ((variantBits + vecSize * sizeof(float)) > variantDataSize)
               return false;
            break;
         }
         default:
            return false;
      }
   }   
   else
   {
      // These types should always be encoded as offsets.
      switch (variantType)
      {
         case cXMXVTFloat:    
         case cXMXVTInt32:    
         case cXMXVTDouble:   
         case cXMXVTFloatVec: 
         case cXMXVTUString:  
            return false;
      }
   }   
   
   switch (variantType)
   {
      case cXMXVTNull:
      {
         *pBuf = '\0';
         break;
      }
      case cXMXVTFloat24:  // always direct
      {
         float f = unpackFloat24(variantBits);
                  
         errno_t errorCode = _gcvt_s(pBuf, bufSize, f, 6);
         if (errorCode != 0)  
            return false;

         break;
      }
      case cXMXVTFloat:    // always an offset
      {
         // _CVTBUFSIZE 
                  
         errno_t errorCode = _gcvt_s(pBuf, bufSize, *(const float*)(pVariantData + variantBits), 8);
         if (errorCode != 0)  
            return false;

         break;
      }
      case cXMXVTInt24:    // always direct
      {
         if (unsignedFlag)
         {
            errno_t errorCode = _ultoa_s((uint)variantBits, pBuf, bufSize, 10);
            if (errorCode != 0)
               return false;
         }
         else
         {
            int val = (int)variantBits;
            if (val & 0x800000)
               val |= 0xFF000000;
               
            errno_t errorCode = _ltoa_s(val, pBuf, bufSize, 10);
            if (errorCode != 0)
               return false;
         }
         break;
      }
      case cXMXVTInt32:    // always an offset
      {
         if (unsignedFlag)
         {
            errno_t errorCode = _ultoa_s(*(const uint*)(pVariantData + variantBits), pBuf, bufSize, 10);
            if (errorCode != 0)
               return false;
         }
         else
         {
            errno_t errorCode = _ltoa_s(*(const int*)(pVariantData + variantBits), pBuf, bufSize, 10);
            if (errorCode != 0)
               return false;
         }
         break;
      }
      case cXMXVTFract24:  // always direct
      {
         //double f = unpackFract24(variantBits);
         //errno_t errorCode = _gcvt_s(pBuf, bufSize, f, 7);
         //if (errorCode != 0)  
         //   return false;
         
         const BOOL negFlag = (variantBits & 0x800000) != 0;
         uint i = (variantBits & 0x7FFFFF) / 10000;
         uint f = (variantBits & 0x7FFFFF) % 10000;
         if (!sprintf_s(pBuf, bufSize, negFlag ? "-%u.%04u" : "%u.%04u", i, f))
            return false;
                  
         break;
      }
      case cXMXVTDouble:   // always an offset
      {
         errno_t errorCode = _gcvt_s(pBuf, bufSize, *(const double*)(pVariantData + variantBits), 17);
         if (errorCode != 0)  
            return false;
            
         break;
      }
      case cXMXVTBool:     // always direct
      {
         if ((variantBits != 0) && (variantBits != 1))
            return false;
            
         if (0 != strcpy_s(pBuf, bufSize, variantBits ? "true" : "false"))
            return false;
         
         break;
      }
      case cXMXVTString:   // direct or offset
      {
         if (offsetFlag)
         {
            const char* pStr = (const char*)(pVariantData + variantBits);
                      
            if ((strLen + 1) > bufSize)
               return false;
               
            memcpy(pBuf, pStr, strLen + 1);
         }
         else
         {
            if (bufSize < 4)
               return false;
            
            pBuf[0] = (char)(variantBits & 0xFF);
            pBuf[1] = (char)((variantBits >> 8) & 0xFF);
            pBuf[2] = (char)((variantBits >> 16) & 0xFF);
            pBuf[3] = '\0';
         }
         break;
      }
      case cXMXVTUString:   // direct or offset
      {
         const WCHAR* pStr = (const WCHAR*)(pVariantData + variantBits);
         
         if (permitUncode)
         {
            if (((strLen + 1) * sizeof(WCHAR)) >= bufSize)
               return false;
            
            memcpy(pBuf, pStr, (strLen + 1) * sizeof(WCHAR));
            unicode = true;
         }
         else
         {
            const long numCharsNeeded = WideCharToMultiByte(CP_ACP, 0, pStr, strLen, NULL, 0, NULL, NULL);
            if (numCharsNeeded <= 0)
               return false;
            
            if ((numCharsNeeded + 1) >= (long)bufSize)
               return false;
            
            const long numCharsWritten = WideCharToMultiByte(CP_ACP, 0, pStr, strLen, pBuf, numCharsNeeded, NULL, NULL);     
            numCharsWritten;
            BDEBUG_ASSERT(numCharsWritten == numCharsNeeded);
            pBuf[numCharsNeeded] = '\0';
         }            
         
         break;
      }
      case cXMXVTFloatVec: // always an offset
      {
         char* pBufDst = pBuf;
         uint bufLeft = bufSize;
         
         for (uint i = 0; i < vecSize; i++)
         {
            float f = *(const float*)(pVariantData + variantBits + i * sizeof(float));

            errno_t errorCode = _gcvt_s(pBufDst, bufLeft, f, 8);
            if (errorCode != 0)  
               return false;

            uint len = strlen(pBufDst);
            pBufDst += len;
            BDEBUG_ASSERT((len + 1) <= bufLeft);
            bufLeft -= (len + 1);
            
            if (i != (vecSize - 1))
            {
               if (!bufLeft)
                  return false;
                  
               *pBufDst++ = ',';
               *pBufDst = '\0';
               bufLeft--;
            }                                    
         }
            
         break;
      }
      default:
      {
         return false;
      }
   }      
   
   return true;
}

uint BXMXVariantHelpers::getVariantMaxStringBufferSize(const BConstDataBuffer& variantData, uint variantValue)
{
   const eXMXVariantType variantType = getVariantType(variantValue);
   
   if (!isStringVariant(variantValue))   
   {
      // 128 chars should be enough to convert any numeric variant to a string, right?
      return 128;
   }
   
   const uint variantTypeBits = getVariantTypeBits(variantValue);
   const uint variantBits = getVariantBits(variantValue);
   
   if (variantType == cXMXVTUString)
   {
      BDEBUG_ASSERT(getVariantIsOffset(variantValue));
      const WCHAR* pStr = (const WCHAR*)(variantData.getPtr() + variantBits);
      return wcslen(pStr) + 1;
   }
   else if (variantTypeBits & cXMXVTOffsetFlag)
   {
      const char* pStr = (const char*)(variantData.getPtr() + variantBits);
      return strlen(pStr) + 1;
   }
   else
   {
      // Directly encoded strings can be 3 chars + 1 for null terminator.
      return 4;
   }
}



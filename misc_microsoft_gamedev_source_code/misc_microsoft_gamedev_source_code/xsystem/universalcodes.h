//==============================================================================
// universalcodes.h
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================
#pragma once

class BUniversalCoder
{
public:
   BUniversalCoder()
   {
      initReverseBits();
      initOmegaCodeTable();
   }

   template<typename BitPacker>
   bool encodeOmega(BitPacker& packer, uint val)
   {
      // Most of the time (94% in a few tests) val was 8 bits or less.
      if (val < OmegaCodeTableSize)
         return packer.encodeSmall(mOmegaCodeTable[val].mCode, mOmegaCodeTable[val].mLen);

      int len;
      uint64 code = getOmegaCode(len, val);
                        
      return packer.encode(code, len);
   }
   
   template<typename BitPacker>
   bool encodeOmegaSigned(BitPacker& packer, int val)
   {
      if (!packer.encodeSmall((val < 0) ? 1 : 0, 1))
         return false;
          
      if (val < 0)
         val = -val - 1;
      
      return encodeOmega(packer, val);
   }
   
   template<typename BitPacker>
   bool decodeOmega(BitPacker& packer, uint& value)
   {
      uint n;
      
      if (!packer.decodeLookahead(1))
      {  
         value = 0;
         return packer.decodeRemoveBits(1);
      }
      
      if (!packer.decode(n, 2))
         return false;
      
      n = mReverseByteTable[n << 6];
      
      while (packer.decodeLookahead(1))
      {
         const int len = n + 1;
         if (!packer.decode(n, len))
            return false;

         n = reverseDWORD(n << (32 - len));
      }
      
      BDEBUG_ASSERT(n != 0);
      value = n - 1;
      return packer.decodeRemoveBits(1);
   }
   
   template<typename BitPacker>
   bool decodeOmegaSigned(BitPacker& packer, int& value)
   {
      uint signFlag;
      if (!packer.decode(signFlag, 1))
         return false;
      
      uint uvalue;
      if (!decodeOmega(packer, uvalue))
         return false;
      
      value = uvalue;
      if (signFlag)
         value = -value - 1;
               
      return true;
   }
               
private:
   uchar mReverseByteTable[256];
   uchar mSigBits[256];
   
   struct BOmegaCode
   {
      ushort mCode;
      uchar mLen;
   }; 
   
   enum { OmegaCodeTableSize = 256 };
   BOmegaCode mOmegaCodeTable[OmegaCodeTableSize];
   
   void initOmegaCodeTable(void)
   {
      for (int i = 0; i < OmegaCodeTableSize; i++)
      {
         int len;
         uint64 code = getOmegaCode(len, i);
         BDEBUG_ASSERT(len <= 16);
         mOmegaCodeTable[i].mCode = static_cast<ushort>(code);
         mOmegaCodeTable[i].mLen = static_cast<uchar>(len);
      }
   }
      
   void initReverseBits(void) 
   {
      for (int i = 0; i < 256; i++)
      {
         int r = 0;
         int v = i;

         for (int j = 0; j < 8; j++)
         {
            r <<= 1;
            r |= (v & 1);
            v >>= 1;
         }

         mReverseByteTable[i] = static_cast<uchar>(r);
         
         int val = i;
         int l = 0;
         while (val)
         {
            val >>= 1;
            l++;
         }
         mSigBits[i] = static_cast<uchar>(l);
      }

      for (int i = 0; i < 256; i++)
      {
         BDEBUG_ASSERT(mReverseByteTable[mReverseByteTable[i]] == i);
      }
   }
   
   DWORD reverseDWORD(DWORD i) const
   {
      return (mReverseByteTable[ i        & 0xFF] << 24) | 
             (mReverseByteTable[(i >>  8) & 0xFF] << 16) |
             (mReverseByteTable[(i >> 16) & 0xFF] <<  8) |
             (mReverseByteTable[(i >> 24) & 0xFF]);
   }

   int numBits(uint val) const
   {
      int l = 0;
      if (val & 0xFF000000)
         l = 24;
      else if (val & 0x00FF0000)
         l = 16;
      else if (val & 0x0000FF00)
         l = 8;

      return l + mSigBits[val >> l];
   }
   
   uint64 getOmegaCode(int& len, uint val) const
   {
      BDEBUG_ASSERT(val != UINT_MAX);
      uint valueToCode = val + 1;

      uint64 code = 0;
      int bitPos = 63;

      while (valueToCode != 1)
      {
         BDEBUG_ASSERT(valueToCode > 0);
         const int valueToCodeBits = numBits(valueToCode);

         bitPos -= valueToCodeBits;
         BDEBUG_ASSERT(bitPos >= 0);

         valueToCode = reverseDWORD(valueToCode) >> (32 - valueToCodeBits);
         code |= (static_cast<uint64>(valueToCode) << bitPos);

         valueToCode = valueToCodeBits - 1;
      }

      BDEBUG_ASSERT(bitPos <= 64);

      code >>= bitPos;
      len = 64 - bitPos;
      
      return code;
   }
};


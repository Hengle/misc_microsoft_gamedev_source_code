
using System;
using System.Collections;



namespace EnsembleStudios.RemoteGameDebugger
{
   public class BUniversalCoder
   {

      public BUniversalCoder()
      {
         initReverseBits();
         initOmegaCodeTable();

         initLookaheadTable();
      }

      //template<typename BitPacker>
      public bool encodeOmega(ref BitPacker packer, uint val)
      {
         // Most of the time (94% in a few tests) val was 8 bits or less.
         if (val < OmegaCodeTableSize)
            return packer.encodeSmall(mOmegaCodeTable[val].mCode, mOmegaCodeTable[val].mLen);

         int len;
         UInt64 code = getOmegaCode(out len, val);
                           
         return packer.encode(code, len);
      }
   
      //template<typename BitPacker>
      public bool encodeOmegaSigned(ref BitPacker packer, int val)
      {
         if (!packer.encodeSmall((ushort)((val < 0) ? 1 : 0), 1))
            return false;
                
         if (val < 0)
            val = -val - 1;
            
         return encodeOmega(ref packer, (uint)val);
      }
   
      //template<typename BitPacker>
      public bool decodeOmega(ref BitPacker packer, out uint Value)
      {         
//         if (!(packer.decodeLookahead(1) != 0))
//         {  
//            Value = 0;
//            return packer.decodeRemoveBits(1);
//         }         
         
         uint lookaheadTableOfs = packer.decodeLookahead(LookaheadTableBits);    
         if (-1 != mLookaheadTable[lookaheadTableOfs].mCodeLen)
         {
            packer.decodeRemoveBits(mLookaheadTable[lookaheadTableOfs].mCodeLen);
            Value = mLookaheadTable[lookaheadTableOfs].mCodeVal;
            return true;
         }
         ////////////////////

         Value = 0;
         uint n;

            
         if (!packer.decode(out n, 2))
            return false;
            
         n = mReverseByteTable[n << 6];
            
         while (packer.decodeLookahead(1) != 0)
         {
            /*const*/ int len = (int)(n + 1);
            if (!packer.decode(out n, len))
               return false;

            n = reverseDWORD(n << (32 - len));
         }
            
         BDEBUG_ASSERT(n != 0);
         Value = n - 1;
         return packer.decodeRemoveBits(1);
      }
   
      //template<typename BitPacker>
      public bool decodeOmegaSigned(ref BitPacker packer, out int Value)
      {
         Value = 0;
         uint signFlag;
         if (!packer.decode(out signFlag, 1))
            return false;
               
         uint uvalue;
         if (!decodeOmega(ref packer, out uvalue))
            return false;
               
         Value = (int)uvalue;
         if (signFlag != 0)
            Value = -Value - 1;
                        
         return true;
      }

      //Private:
      byte[] mReverseByteTable = new byte[256];
      byte[] mSigBits = new byte[256];
   
      struct BOmegaCode
      {
         public ushort mCode;
         public byte mLen;
      }; 
   
      //enum eOmega { OmegaCodeTableSize = 256 };
      const short OmegaCodeTableSize = 256;
      BOmegaCode[] mOmegaCodeTable = new BOmegaCode[OmegaCodeTableSize];

      void initOmegaCodeTable()
      {
         for (int i = 0; i < OmegaCodeTableSize; i++)
         {
            int len;
            UInt64 code = getOmegaCode(out len, (uint) i);
            BDEBUG_ASSERT(len <= 16);
            mOmegaCodeTable[i].mCode = (ushort)(code);
            mOmegaCodeTable[i].mLen = (byte)(len);
         }
      }
      
      void initReverseBits() 
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

            mReverseByteTable[i] = (byte)(r);
         
            int val = i;
            int l = 0;
            while (val != 0)
            {
               val >>= 1;
               l++;
            }
            mSigBits[i] = (byte)(l);
         }

         for (int i = 0; i < 256; i++)
         {
            BDEBUG_ASSERT(mReverseByteTable[mReverseByteTable[i]] == i);
         }
      }
   
      void BDEBUG_ASSERT(bool val)
      {
         if(val == false)
            throw new System.Exception("BDEBUG_ASSERT");
      }
   
      //DWORD reverseDWORD(DWORD i) //const
      uint reverseDWORD(uint i) //const
      {
         return (uint)(mReverseByteTable[ i        & 0xFF] << 24) | 
            (uint)(mReverseByteTable[(i >>  8) & 0xFF] << 16) |
            (uint)(mReverseByteTable[(i >> 16) & 0xFF] <<  8) |
            (uint)(mReverseByteTable[(i >> 24) & 0xFF] );
      }

      int numBits(uint val) //const
      {
         int l = 0;
         if ((val & 0xFF000000) != 0)
            l = 24;
         else if ((val & 0x00FF0000) != 0)
            l = 16;
         else if ((val & 0x0000FF00) != 0)
            l = 8;

         return l + mSigBits[val >> l];
      }
   
      UInt64 getOmegaCode(out int len, uint val) //const
      {
         BDEBUG_ASSERT(val != UInt32.MaxValue);
         uint valueToCode = val + 1;

         UInt64 code = 0;
         int bitPos = 63;

         while (valueToCode != 1)
         {
            BDEBUG_ASSERT(valueToCode > 0);
            /*const*/ int valueToCodeBits = numBits(valueToCode);

            bitPos -= valueToCodeBits;
            BDEBUG_ASSERT(bitPos >= 0);

            valueToCode = reverseDWORD(valueToCode) >> (32 - valueToCodeBits);
            code |= ((UInt64)(valueToCode) << bitPos);

            valueToCode = (uint)(valueToCodeBits - 1);
         }

         BDEBUG_ASSERT(bitPos <= 64);

         code >>= bitPos;
         len = 64 - bitPos;
      
         return code;
      }


      struct LookaheadEntry
      {
         public ushort mCodeVal;
         public short mCodeLen;
      };

      const int LookaheadTableBits = 14;//14;//8;
      const int LookaheadTableSize = 1 << LookaheadTableBits;
      const int MaxLookaheadOmegaCodeSize = 16;

      LookaheadEntry[] mLookaheadTable = new LookaheadEntry[LookaheadTableSize];

//      ushort GetFromTable()
//      {
//         uint lookaheadTableOfs = packer.decodeLookahead(LookaheadTableBits);
//         if (-1 != mLookaheadTable[i].mCodeLen)
//         {
//            packer.decodeRemoveBits(mLookaheadTable[i].mCodeLen);
//            return mLookaheadTable[i].mCodeVal;
//         }
//      }

      void initLookaheadTable()
      {
         for (int i = 0; i < LookaheadTableSize; i++)
         {
            mLookaheadTable[i].mCodeVal = 0;
            mLookaheadTable[i].mCodeLen = -1;
         }
         uint val = 0;
         for ( ; ; )
         {
            int len;
            UInt64 code = getOmegaCode(out len, val);
            if (len > LookaheadTableBits)
               break;
            
            BDEBUG_ASSERT(len <= MaxLookaheadOmegaCodeSize);

            int followBits = LookaheadTableBits - len;
            for (int i = 0; i < (1 << followBits); i++)
            {
               //const 
               int tableOfs = (int)(code + (ulong)(i << len));
               BDEBUG_ASSERT((tableOfs >= 0) && (tableOfs < LookaheadTableSize));
               BDEBUG_ASSERT(mLookaheadTable[tableOfs].mCodeLen == -1);
               mLookaheadTable[tableOfs].mCodeVal = (ushort)val;
               mLookaheadTable[tableOfs].mCodeLen = (short)len;
            }
            val++;
         }
      }

   }

   












}
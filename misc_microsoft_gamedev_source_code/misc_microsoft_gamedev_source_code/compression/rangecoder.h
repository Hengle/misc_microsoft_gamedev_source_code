//==============================================================================
// rangecoder.h
// Based off Dmitry Subbotin's public domain carryless range coder.
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================

#pragma once

// The range coder traits class lets you control how bytes are read/written by the range coder.
class BRangeCoderTraits
{
public:
   // Returns false on failure.
   bool outputByte(uchar c)
   {
      c;
      return true;
   }
   
   // Returns -1 on failure.
   // At EOF, you should return 0x00's or 0xFF's (not an error) as the decoder may try to fetch more than it needs at the end
   // of the stream.
   int inputByte(void)
   {
      return 0;
   }
};

// Range coding is similar to arithmetic coding, except flushes are done per-byte instead of per-bit. 
// This makes range coding a lot faster than most arithmetic coders (probably even the Q-coder?), at a 
// very slight loss of compression efficiency.
template <typename Traits = BRangeCoderTraits>
class BRangeCoder
{
public:
   typedef Traits traits;
   
   BRangeCoder() :
      mLow(0),
      mCode(0),
      mRange(0),
      mTraits()
   {
   }
   
   BRangeCoder(const Traits& t) :
      mLow(0),
      mCode(0),
      mRange(0),
      mTraits(t)
   {
   }
   
   void encodeStart(void)
   {
      mLow = 0;  
      mRange = UINT_MAX;
   }
   
   // false on failure
   bool encode(uint cumFreq, uint freq, uint totFreq) 
   {
      BASSERT(mRange > 0);
      BASSERT(cumFreq + freq <= totFreq && freq && totFreq <= Bottom);
      
      mLow  += cumFreq * (mRange /= totFreq);
      mRange *= freq;
      while ((mLow ^ mLow + mRange) < Top || mRange < Bottom && ((mRange = static_cast<uint>(-static_cast<int>(mLow)) & (Bottom - 1)), 1))
      {
         if (!mTraits.outputByte(static_cast<uchar>(mLow >> 24)))
            return false;
         mRange <<= 8;
         mLow <<= 8;
      }
      
      return true;
   }
   
   // false on failure   
   bool encodeEnd(void)
   {
      for (int i = 0; i < 4; i++)
      {
         if (!mTraits.outputByte(static_cast<uchar>(mLow >> 24)))
            return false;
         mLow <<= 8;
      }
      
      return true;
   }
   
   // false on failure
   bool decodeStart(void)
   {
      mLow = 0;
      mCode = 0;  
      mRange = UINT_MAX;
      for (int i = 0; i < 4; i++) 
      {
         const int b = mTraits.inputByte();
         if (b < 0)
            return false;
         mCode = (mCode << 8) | b;
      }
      return true;
   }
   
   // If freq. is invalid this method always returns 0.
   uint getFreq (uint totFreq) 
   {
      uint tmp = (mCode - mLow) / (mRange /= totFreq);
                  
      if (tmp >= totFreq)
      {
         // The stream is invalid.
         return 0;
      }
      
      return tmp;          
   }

   // false on failure
   bool decode(uint cumFreq, uint freq, uint totFreq) 
   {
      totFreq;
      
      BASSERT(mRange > 0);
      BASSERT(cumFreq + freq <= totFreq && freq && totFreq <= Bottom);
      
      mLow  += cumFreq * mRange;
      mRange *= freq;
      while ((mLow ^ mLow + mRange) < Top || mRange < Bottom && ((mRange = ((uint)-((int)mLow)) & (Bottom - 1)), 1))
      {
         const int b = mTraits.inputByte();
         if (b < 0)
            return false;
            
         mCode = (mCode << 8) | b;
         mRange <<= 8;
         mLow <<= 8;
      }
      
      return true;
   }
      
   const Traits& getTraits(void) const { return mTraits; }
         Traits& getTraits(void)       { return mTraits; }   
      
private:
   uint mLow;
   uint mCode;
   uint mRange;
   Traits mTraits;
   
   enum 
   { 
      Top      = 1 << 24, 
      Bottom   = 1 << 16 
   };
};

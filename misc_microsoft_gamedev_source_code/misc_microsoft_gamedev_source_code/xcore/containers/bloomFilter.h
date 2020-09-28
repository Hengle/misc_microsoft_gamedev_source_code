//============================================================================
//
// File: bloomFilter.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "hash\hash.h"

// Also see: http://blogs.msdn.com/devdev/archive/2005/08/17/452827.aspx
// rg [7/24/07] - Needs to be retested!
class BBloomFilter
{
   // Should give a ~3% false positive rate with 3 hashes:
   // http://www.cs.wisc.edu/~cao/papers/summary-cache/node8.html
   
public:
   BBloomFilter(uint expectedMaxEntries = 0, uint numBitsPerEntry = 8) :
   {
      init(expectedMaxEntries, numBitsPerEntry);
   }

   void clear(void)
   {
      mBitVec.setAll(0);
   }
   
   void init(uint expectedMaxEntries, uint numBitsPerEntry = 8)
   {
      mExpectedMaxEntries = expectedMaxEntries;
      
      mTotalBits = expectedMaxEntries * numBitsPerEntry;
      if (!Math::IsPow2(mTotalBits))
         mTotalBits = Math::NextPowerOf2(mTotalBits);
      
      mBitVecSize = (mTotalBits + 7) >> 3;
      mHashMask = mBitVecSize - 1;      
      
      mBitVec.resize(mBitVecSize);
      
      mBitVec.setAll(0);
   }
   
   uint getExpectedMaxEntries(void) const
   {
      return mExpectedMaxEntries;
   }

   void insert(const BHash& hash)
   {
      BDEBUG_ASSERT(hash.size() >= 3);
      setBit(hash[0] & mHashMask);
      setBit(hash[1] & mHashMask);
      setBit(hash[2] & mHashMask);
   }

   // false = object definitely not present
   // true = object may be present, there is a chance of a false positive
   bool mayBePresent(const BHash& hash) const
   {
      BDEBUG_ASSERT(hash.size() >= 3);
      return (getBit(hash[0] & mHashMask) && getBit(hash[1] & mHashMask) && getBit(hash[2] & mHashMask));
   }
   
   // false = object definitely not present
   // true = object may be present, there is a chance of a false positive
   bool insertIfNotPresent(const BHash& hash)
   {
      BDEBUG_ASSERT(hash.size() >= 3);
      const bool result = mayBePresent(hash);
      insert(hash);
      return result;
   }
   
   void combine(const BBloomFilter& other)
   {
      BDEBUG_ASSERT(mBitVec.getSize() == other.mBitVec.getSize());
      for (uint i = 0; i < mBitVec.getSize(); i++)
         mBitVec[i] |= other.mBitVec[i];
   }
   
   const BByteArray& getBitVec(void) const   { return mBitVec; }
         BByteArray& getBitVec(void)         { return mBitVec; }
   
protected:
   uint mExpectedMaxEntries;
   uint mTotalBits;
   uint mBitVecSize;
   uint mHashMask;
   BByteArray mBitVec;

   bool getBit(int bit) const
   {
      debugRangeCheck(bit, mTotalBits);
      return 0 != (mBitVec[bit >> 3] & (1 << (bit & 7)));
   }

   void setBit(int bit)
   {
      debugRangeCheck(bit, mTotalBits);
      mBitVec[bit >> 3] |= (1 << (bit & 7));
   }
};

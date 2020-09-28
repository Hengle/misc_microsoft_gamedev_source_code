// File: bloom_filter.h
#pragma once
#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include "common/utils/utils.h"
#include "common/utils/hash.h"
#include "common/math/math.h"

namespace gr
{
	class BloomFilter
	{
		// Should give a ~3% false positive rate with 3 hashes:
		// http://www.cs.wisc.edu/~cao/papers/summary-cache/node8.html
		enum { NumBitsPerEntry = 8 };

	public:
		BloomFilter(int expectedMaxEntries) :
			mExpectedMaxEntries(expectedMaxEntries),
			mNumBits(Math::NextPowerOf2((DebugNull(expectedMaxEntries) * NumBitsPerEntry + 7) / 8))
		{
			mBitVec.resize(mNumBits);
		}

		void clear(void)
		{
			std::fill(mBitVec.begin(), mBitVec.end(), 0);
		}
		
		int size(void) const
		{
			return mExpectedMaxEntries;
		}

		void insert(const Hash& hash)
		{
			Assert(hash.size() >= 3);
			setBit(hash[0] & (mNumBits - 1));
			setBit(hash[1] & (mNumBits - 1));
			setBit(hash[2] & (mNumBits - 1));
		}

		// false = object definitely not present
		// true = object may be present, there is a chance of a false positive
		bool mayBePresent(const Hash& hash) const
		{
			Assert(hash.size() >= 3);
			return 0 != 
				(getBit(hash[0] & (mNumBits - 1)) | 
				 getBit(hash[1] & (mNumBits - 1)) | 
				 getBit(hash[2] & (mNumBits - 1)));
		}
		
		// false = object definitely not present
		// true = object may be present, there is a chance of a false positive
		bool insertIfNotPresent(const Hash& hash)
		{
			Assert(hash.size() >= 3);
      const bool result = mayBePresent(hash);
			insert(hash);
			return result;
		}
		
	protected:
		int mExpectedMaxEntries;
		int mNumBits;
		UCharVec mBitVec;
	
		int getBit(int bit) const
		{
			DebugRange(bit, mNumBits);
      return mBitVec[bit >> 3] & (1 << (bit & 7));
		}

		void setBit(int bit)
		{
			DebugRange(bit, mNumBits);
			mBitVec[bit >> 3] |= (1 << (bit & 7));
		}

		int getSetBit(int bit)
		{
			DebugRange(bit, mNumBits);
      const int ofs = bit >> 3;
			const int mask = 1 << (bit & 7);
			const int res = mBitVec[ofs] & mask;
			mBitVec[ofs] |= mask;
			return res;
		}
	};
} // namespace gr

#endif // BLOOM_FILTER_H


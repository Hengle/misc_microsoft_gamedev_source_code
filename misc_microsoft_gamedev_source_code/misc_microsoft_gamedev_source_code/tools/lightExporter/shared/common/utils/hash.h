// File: hash.h
#pragma once
#ifndef HASH_H
#define HASH_H

#include "common/core/core.h"

namespace gr
{
	class Hash
	{
	public:
		Hash()
		{
			clear();
		}

		explicit Hash(uint prevHash)
		{
			clear(prevHash);
		}

		Hash(const void* pData, int dataLen, uint prevHash = 0)
		{
			clear(prevHash);
			update(pData, dataLen);
		}

		void clear(uint prevHash = 0)
		{
			v[0] = v[1] = 0x9E3779B9;
			v[2] = prevHash;
		}

		int size(void) const
		{
			return NumElements;
		}

		uint operator[] (int i) const 
		{ 
			return v[DebugRange(i, size())]; 
		}

		Hash& update(const void* pData, uint dataLen);

		operator uint() const
		{
			return v[2];
		}
						
	protected:
		enum { NumElements = 3 };
		uint v[NumElements];
		
		void bitMix(void);
	};

	inline uint ComputeHash(const void* pData, uint dataLen, uint prevHash = 0)
	{
		return Hash(pData, dataLen, prevHash);
	}
			
	// BEWARE, this evil struct accesses the parent's raw memory!!
	template<class Parent>
	struct Bithashable 
	{
		operator size_t() const
		{
			return ComputeHash(this, sizeof(Parent));
		}

		operator Hash() const
		{
			return Hash(this, sizeof(Parent));
		}

		bool operator== (const Bithashable& b) const
		{
			const uint8* p = reinterpret_cast<const uint8*>(this);
			const uint8* q = reinterpret_cast<const uint8*>(&b);
			for (int i = 0; i < sizeof(Parent); i++)
			{
				if (p[i] != q[i])
					return false;
			}
			return true;
		}

		bool operator!= (const Bithashable& b) const
		{
			return !(*this == b);
		}

		bool operator< (const Bithashable& b) const
		{
			const uint8* p = reinterpret_cast<const uint8*>(this);
			const uint8* q = reinterpret_cast<const uint8*>(&b);
			for (int i = 0; i < sizeof(Parent); i++)
			{
				if (p[i] < q[i])
					return true;
				else if (p[i] != q[i])
					return false;
			}
			return false;
		}
	};
};

#endif // HASH_H
// File: hash.cpp
#include "hash.h"

namespace gr
{
	// See Jenkins, "The Hash", Dr. Dobb's Sept. 1997
	void Hash::bitMix(void)
	{ 
		v[0] -= v[1]; 
		v[0] -= v[2]; 
		v[0] ^= (v[2] >> 13); 
		v[1] -= v[2]; 
		v[1] -= v[0]; 
		v[1] ^= (v[0] << 8); 
		v[2] -= v[0]; 
		v[2] -= v[1]; 
		v[2] ^= (v[1] >> 13); 
		v[0] -= v[1]; 
		v[0] -= v[2]; 
		v[0] ^= (v[2] >> 12);  
		v[1] -= v[2]; 
		v[1] -= v[0]; 
		v[1] ^= (v[0] << 16); 
		v[2] -= v[0]; 
		v[2] -= v[1]; 
		v[2] ^= (v[1] >> 5); 
		v[0] -= v[1]; 
		v[0] -= v[2]; 
		v[0] ^= (v[2] >> 3);  
		v[1] -= v[2]; 
		v[1] -= v[0]; 
		v[1] ^= (v[0] << 10); 
		v[2] -= v[0]; 
		v[2] -= v[1]; 
		v[2] ^= (v[1] >> 15);
	}

	Hash& Hash::update(const void* pData, uint dataLen)
	{
		const uint* pData32 = reinterpret_cast<const uint*>(pData);
		uint len = dataLen;

		while (len >= 12)
		{
			v[0] += pData32[0];
			v[1] += pData32[1];
			v[2] += pData32[2];
			bitMix();

      pData32 += 3;
			len -= 12;
		}

		v[2] += dataLen;

		const uchar* pSrc = reinterpret_cast<const uchar*>(pData32);
		uchar* pDst = reinterpret_cast<uchar*>(v);

		while (len)
		{
			*pDst += *pSrc;
			
			pSrc++;
			pDst++;
							
			if (pDst == reinterpret_cast<uchar*>(&v[2]))
				pDst++;

			len--;
		}

		bitMix();

		return *this;
	}
} // namespace gr

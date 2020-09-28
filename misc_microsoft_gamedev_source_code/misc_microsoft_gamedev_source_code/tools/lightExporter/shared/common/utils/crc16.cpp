// File: crc16.cpp
#include "crc16.h"

namespace gr
{
	uint16 CalcCRC16Fast(const void* pData, int dataLen, uint16 curCRC)
	{
		static uint16 Table[256];

		if (!Table[0])
		{
			const int CRCPoly = 0x1021;
			for (int i = 0; i < 256; i++)
			{
				int q = (i << 8);

				for (int j = 0; j < 8; j++)
				{
					if (q & 0x8000)
						q = (q << 1) ^ CRCPoly;
					else
						q <<= 1;
				}
				Table[i] = q;
			}
		}

		curCRC ^= 0xFFFF;

		const uchar* pBytes = reinterpret_cast<const uchar*>(pData);
		while (dataLen > 4)
		{
			curCRC = (curCRC << 8) ^ Table[(pBytes[0] ^ (curCRC >> 8)) & 0xFF];
			curCRC = (curCRC << 8) ^ Table[(pBytes[1] ^ (curCRC >> 8)) & 0xFF];
			curCRC = (curCRC << 8) ^ Table[(pBytes[2] ^ (curCRC >> 8)) & 0xFF];
			curCRC = (curCRC << 8) ^ Table[(pBytes[3] ^ (curCRC >> 8)) & 0xFF];
			pBytes += 4;
			dataLen -= 4;
		}

		while (dataLen)
		{
			curCRC = (curCRC << 8) ^ Table[(pBytes[0] ^ (curCRC >> 8)) & 0xFF];
			pBytes++;
			dataLen--;
		}

		return curCRC ^ 0xFFFF;
	}

	uint16 CalcCRC16(const void* pData, int dataLen, uint16 curCRC)
	{
		curCRC ^= 0xFFFF;

		const uchar* pBytes = reinterpret_cast<const uchar*>(pData);
		while (dataLen)
		{
			const ushort temp = *pBytes++ ^ (curCRC >> 8);
			
			curCRC <<= 8;

			ushort temp2 = (temp >> 4) ^ temp;
			curCRC ^= temp2;
			
			temp2 <<= 5;
			curCRC ^= temp2;

			temp2 <<= 7;
			curCRC ^= temp2; 
			
			dataLen--;
		}
		
		return curCRC ^ 0xFFFF;
	}

} // namespace gr


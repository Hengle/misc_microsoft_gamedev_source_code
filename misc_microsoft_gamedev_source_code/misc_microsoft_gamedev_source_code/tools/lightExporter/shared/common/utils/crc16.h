// File: crc16.h
#pragma once
#ifndef CRC16_H
#define CRC16_H

#include "common/core/core.h"

namespace gr
{
	const int InitCRC16 = 0;
	
	// Uses 512 byte table
	uint16 CalcCRC16Fast(const void* pData, int dataLen, uint16 curCRC);
	
	// Non-table based
	uint16 CalcCRC16(const void* pData, int dataLen, uint16 curCRC);
};

#endif // CRC16_H

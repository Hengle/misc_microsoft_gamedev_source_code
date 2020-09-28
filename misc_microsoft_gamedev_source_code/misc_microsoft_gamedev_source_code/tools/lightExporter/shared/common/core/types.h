//-----------------------------------------------------------------------------
// File: types.h
// Common types/enums.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef TYPES_H
#define TYPES_H

#include "common/core/core.h"

namespace gr
{
  typedef unsigned char     uchar;
  typedef unsigned short    ushort;
  typedef unsigned int      uint;

  typedef char              int8;
  typedef unsigned char     uint8;

  typedef short             int16;
  typedef unsigned short    uint16;

  typedef int               int32;
  typedef unsigned int      uint32;

  typedef __int64           int64;
  typedef unsigned __int64  uint64;
  
  const uint8 UINT8_MAX = 0xff;
  const uint8 UINT8_MIN = 0x00;
  const int8 INT8_MAX = 0x7f;
  const int8 INT8_MIN = -0x80;

  const uint16 UINT16_MAX = 0xffff;
  const uint16 UINT16_MIN = 0x0000;
  const int16 INT16_MAX = 0x7fff;
  const int16 INT16_MIN = -0x8000;

  const uint32 UINT32_MAX = 0xffffffff;
  const uint32 UINT32_MIN = 0x00000000;
  const int32 INT32_MAX = 0x7fffffff;
  const int32 INT32_MIN = -(int32)0x80000000;

  const uint64 UINT64_MAX = 0xffffffffffffffff;
  const uint64 UINT64_MIN = 0x0000000000000000;
  const int64 INT64_MAX = 0x7fffffffffffffff;
  const int64 INT64_MIN = -(int64)0x8000000000000000;

	typedef std::vector<bool>		BoolVec;
  
  typedef std::vector<int>    IntVec;
  typedef std::vector<uint>   UIntVec;

  typedef std::vector<char>   CharVec;
  typedef std::vector<uchar>  UCharVec;  
  
  typedef std::vector<short>  ShortVec;
  typedef std::vector<ushort> UShortVec;

  typedef std::vector<int8>   Int8Vec;
  typedef std::vector<uint8>  UInt8Vec;
  
  typedef std::vector<int16>  Int16Vec;
  typedef std::vector<uint16> UInt16Vec;
  
  typedef std::vector<int32>  Int32Vec;
  typedef std::vector<uint32> UInt32Vec;
  
  typedef std::vector<float>  FloatVec;
  typedef std::vector<double> DoubleVec;

	enum EComponent 
	{ 
		X = 0, 
		Y = 1, 
		Z = 2, 
		W = 3
	};
	
	enum EResult
	{
		FAILURE		= -1,
		SUCCESS		= 0,
		PARALLEL	= 1,
		INSIDE		= 2,
		OUTSIDE		= 3,
		PARTIAL		= 4
	};

	enum EIndex
	{
		InvalidIndex = -1
	};

	enum EVarArg 
	{ 
		eVarArg 
	};
	
	enum EClear
	{
		eClear
	};
      
} // namespace gr

#endif // TYPES_H


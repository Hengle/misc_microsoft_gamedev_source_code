//============================================================================
//
// File: crc.h
// Copyright (c) 2005-2006, Ensemble Studios
//
// 16, 32, and 64 bit CRC calculations functions
//============================================================================
#pragma once

#define SUPPORT_CRC64 0

const uint64 cInitCRC16 = 0;

// All the CRC-16 functions use the same generator polynomials (which one is it?), so they will always return the same results.

// Function CalcCRC16
// Uses 512 byte table
uint16 calcCRC16(const void* pData, uint dataLen, uint16 curCRC = cInitCRC16);

inline uint16 calcCRC16Fast(const void* pData, uint dataLen, uint16 curCRC = cInitCRC16) { return calcCRC16(pData, dataLen, curCRC); }
   
// Function CalcCRC16NonTable
// Non-table based - won't load down L1/L2 caches with table
uint16 calcCRC16NonTable(const void* pData, uint dataLen, uint16 curCRC = cInitCRC16);

const uint32 cInitCRC32 = 0;

// Function CalcCRC32
// Table based, ZIP standard compatible CRC-32 with pre/post conditioning.
// Note: Not quite compatible with xcore/ecore's crc32() func, it doesn't do automatic pre/post conditioning!
// Uses 1024 byte table
uint32 calcCRC32(const void* pData, uint dataLen, uint32 curCRC = cInitCRC32, bool conditioning = true);

uint32 calcCRC32SmallTable(const BYTE *ptr, uint32 cnt, uint32 crc, bool conditioning = true);

#if SUPPORT_CRC64
const uint64 cInitCRC64 = 0;

// Function CalcCRC64
// With pre/post conditioning.
// Uses 2048 byte table
uint64 calcCRC64(const void* pData, uint dataLen, uint64 curCRC = cInitCRC64);
#endif
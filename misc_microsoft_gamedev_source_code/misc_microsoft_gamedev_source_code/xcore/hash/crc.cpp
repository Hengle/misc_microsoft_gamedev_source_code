//============================================================================
//
// File: crc.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "crc.h"

//============================================================================
// Globals
//============================================================================
static uint16 gCRC16Table[256];
static uint32 gCRC32Table[256];

#if SUPPORT_CRC64      
static uint64 gCRC64Table[256];
#endif

//============================================================================
// class BCRCTableInitializers
//============================================================================
class BCRCTableInitializers
{
public:
   BCRCTableInitializers()
   {
      initCRC16();
      
      initCRC32();

#if SUPPORT_CRC64      
      initCRC64();
#endif      
   }
   
private:
   void initCRC16(void)
   {
      const int CRCPoly = 0x1021;
      for (int i = 0; i < 256; i++)
      {
         uint q = (i << 8);

         for (int j = 0; j < 8; j++)
         {
            if (q & 0x8000)
               q = (q << 1) ^ CRCPoly;
            else
               q <<= 1;
         }
         gCRC16Table[i] = static_cast<uint16>(q);
      }
   }

   void initCRC32(void)
   {
      const uint32 crcPoly = 0xEDB88320;

      for (int i = 0; i < 256; i++) 
      {
         uint32 q = i;
         for (int j = 0; j < 8; j++) 
         {
            if (q & 1)
               q = (q >> 1) ^ crcPoly;
            else
               q >>= 1;
         }
         gCRC32Table[i] = q;
      }      
   }

#if SUPPORT_CRC64
   void initCRC64(void)
   {
      // The usual polynomial used is 0xD800000000000000ULL, but this is too sparse for effective hashing.
      const uint64 crcPoly = 0x95AC9329AC4BC9B5ULL;

      for (int i = 0; i < 256; i++) 
      {
         uint64 q = i;
         for (int j = 0; j < 8; j++) 
         {
            if (q & 1)
               q = (q >> 1) ^ crcPoly;
            else
               q >>= 1;
         }

         gCRC64Table[i] = q;
      }
   }
#endif      
};

//-------------------------------------------------------------------------------------------------------------
// Function calcCRC16Fast
//-------------------------------------------------------------------------------------------------------------
uint16 calcCRC16(const void* pData, uint dataLen, uint16 curCRC)
{
   BDEBUG_ASSERT(pData);
   BDEBUG_ASSERT(gCRC16Table[1] && gCRC16Table[255]);

   curCRC ^= 0xFFFF;

   const uchar* pBytes = reinterpret_cast<const uchar*>(pData);
   while (dataLen > 4)
   {
      curCRC = (curCRC << 8) ^ gCRC16Table[(pBytes[0] ^ (curCRC >> 8)) & 0xFF];
      curCRC = (curCRC << 8) ^ gCRC16Table[(pBytes[1] ^ (curCRC >> 8)) & 0xFF];
      curCRC = (curCRC << 8) ^ gCRC16Table[(pBytes[2] ^ (curCRC >> 8)) & 0xFF];
      curCRC = (curCRC << 8) ^ gCRC16Table[(pBytes[3] ^ (curCRC >> 8)) & 0xFF];
      pBytes += 4;
      dataLen -= 4;
   }

   while (dataLen)
   {
      curCRC = (curCRC << 8) ^ gCRC16Table[(pBytes[0] ^ (curCRC >> 8)) & 0xFF];
      pBytes++;
      dataLen--;
   }

   return curCRC ^ 0xFFFF;
}

// Function calcCRC16NonTable
uint16 calcCRC16NonTable(const void* pData, uint dataLen, uint16 curCRC)
{
   BDEBUG_ASSERT(pData);
   
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

// Function calcCRC32
uint32 calcCRC32(const void* pData, uint dataLen, uint32 curCRC, bool conditioning)
{
   BDEBUG_ASSERT(pData);
   BDEBUG_ASSERT(gCRC32Table[1] && gCRC32Table[255]);
      
   if (conditioning)
   {
      // pre conditioning
      curCRC ^= UINT_MAX;
   }
   
   const uchar* pBytes = reinterpret_cast<const uchar*>(pData);
   while (dataLen) 
   {
      curCRC = (curCRC >> 8) ^ gCRC32Table[(curCRC ^ static_cast<uint32>(*pBytes)) & 0xFF];
      pBytes++;
      dataLen--;
   } 
   
   if (conditioning)
   {
      // post conditioning
      curCRC ^= UINT_MAX;
   }      
   
   return curCRC;
}

#if SUPPORT_CRC64
// Function CalcCRC64
// Reference: W. H. Press, S. A. Teukolsky, W. T. Vetterling, and B. P. Flannery, 
// "Numerical recipes in C", second edition, page 896.
// "Also see An Improved 64-bit Cyclic Redundancy Check for Protein Sequences", David T. Jones, Department of Computer Science
// http://www.cs.ucl.ac.uk/staff/D.Jones/crcnote.pdf
// Primitive polynomial: 
//    x64 + x63 + x61 + x59 + x58 + x56 + x55 + x52 + x49 + x48 + x47 + x46 + x44 + x41 + x37 + x36 + x34 + x32 + x31 +
//    x28 + x26 + x23 + x22 + x19 + x16 + x13 + x12 + x10 + x9 + x6 + x4 + x3 + 1
// With pre/post conditioning.
uint64 calcCRC64(const void* pData, uint dataLen, uint64 curCRC)
{
   BDEBUG_ASSERT(pData);
   BDEBUG_ASSERT(gCRC64Table[1] && gCRC64Table[255]);

   // pre conditioning
   curCRC ^= 0xFFFFFFFFFFFFFFFFULL;
         
   const uchar* pBytes = reinterpret_cast<const uchar*>(pData);
   const uchar* pEndBytes = pBytes + dataLen;

   while (pBytes < pEndBytes) 
   {
      curCRC = (curCRC >> 8) ^ gCRC64Table[static_cast<uchar>(curCRC) ^ *pBytes];
      pBytes++;
   } 

   // post conditioning
   curCRC ^= 0xFFFFFFFFFFFFFFFFULL;

   return curCRC;
}
#endif

static const uint gCRC32SmallTableData[] = 
{
   0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
   0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
   0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
   0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

// Returns the same results as CalculateCRC32(), but only uses 64 bytes of CPU cache vs. 1024.
// See "A compact CCITT crc16 and crc32 C implementation that balances processor cache usage against speed",
// http://www.geocities.com/malbrain/
uint32 calcCRC32SmallTable(const BYTE *ptr, uint32 cnt, uint32 crc, bool conditioning)
{
   if (conditioning)
      crc ^= UINT_MAX;
   
   while (cnt--)
   {
      crc = (crc >> 4) ^ gCRC32SmallTableData[(crc & 0xf) ^ (*ptr & 0xf)];
      crc = (crc >> 4) ^ gCRC32SmallTableData[(crc & 0xf) ^ (*ptr++ >> 4)];
   }

   if (conditioning)
      crc ^= UINT_MAX;
      
   return crc;
}

//============================================================================
// Global initializer
//============================================================================
#pragma warning(disable: 4073)
#pragma init_seg(lib)
#pragma warning(default: 4073)

BCRCTableInitializers gCRCTableInitializers;

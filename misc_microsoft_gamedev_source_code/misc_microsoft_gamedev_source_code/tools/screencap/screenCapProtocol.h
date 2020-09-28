//-------------------------------------------------------------------------------------------------
//
// File: screenCapProtocol.h
// Copyright (c) 2007, Ensemble Studios
//
//-------------------------------------------------------------------------------------------------
#pragma once
#include "hash\crc.h"

namespace ScreenCapProtocol
{
   enum
   {
      cScreenCapVersion    = 0x100,
      cProtocolVersion     = 0x100,
      cPort                = 3667,
   };
   
   enum ePacketTypes
   {
      cPTInvalid = -1,     
      
      cPTHello,                  
      cPTBye,                    
      
      cPTFrame,
   };

#pragma pack(push, 1)
   struct BPacketHeader
   {
      enum { cSig = 0xDAB70976 };
      DWORD mSig;
      DWORD mDataSize;
      DWORD mDataCRC32;
      BYTE  mType;
      BYTE  mFlags;
      WORD  mHeaderCRC16;
            
      enum { cFlagDataIsNameValueMap = 1 };
                  
      BPacketHeader() { }
      
      BPacketHeader(DWORD dataSize, DWORD dataCRC32, BYTE type, BYTE flags, bool bigEndian) :
         mSig((DWORD)cSig),
         mDataSize(dataSize),
         mDataCRC32(dataCRC32),
         mType(type),
         mFlags(flags)
      {
         if (bigEndian != cBigEndianNative)
         {
            endianSwitch();
            
            mHeaderCRC16 = computeHeaderCRC16();
            
            EndianSwitchWords(&mHeaderCRC16, 1);
         }
         else
         {
            mHeaderCRC16 = computeHeaderCRC16();
         }
      }
      
      uint16 computeHeaderCRC16(void) const
      {
         return calcCRC16Fast(this, sizeof(*this) - sizeof(WORD));
      }
      
      void endianSwitch(void)
      {
         EndianSwitchWorker(this, this + 1, "iiiccs");
      }
   };
#pragma pack(pop)
   
}  

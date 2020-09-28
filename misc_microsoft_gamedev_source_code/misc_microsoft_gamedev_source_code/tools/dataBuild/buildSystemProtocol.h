//-------------------------------------------------------------------------------------------------
//
// File: buildSystemProtocol.h
// Copyright (c) 2007, Ensemble Studios
//
//-------------------------------------------------------------------------------------------------
#pragma once
#include "hash\crc.h"

namespace BuildSystemProtocol
{
   enum
   {
      cBuildSystemVersion     = 0x101,
      cBuildProtocolVersion   = 0x101,
      cBuildSystemPort        = 2667,
   };
   
   enum ePacketTypes
   {
      cPTInvalid = -1,     
      
      cPTHello,                  // both ways
      cPTKeepAlive,              // both ways
      cPTBye,                    // both ways
      cPTAutoUpdate,             // primary to helper
      
      cPTExecuteProcess,         // primary to helper
      cPTKillAllProcesses,       // primary to helper

      cPTExecuteProcessReply,    // helper to primary
      cPTKillAllProcessesReply,  // helper to primary
      cPTProcessCompleted,       // helper to primary
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
      
      BPacketHeader(DWORD dataSize, DWORD dataCRC32, BYTE type, BYTE flags) :
         mSig((DWORD)cSig),
         mDataSize(dataSize),
         mDataCRC32(dataCRC32),
         mType(type),
         mFlags(flags)
      {
         mHeaderCRC16 = computeHeaderCRC16();
      }
      
      uint16 computeHeaderCRC16(void) const
      {
         return calcCRC16Fast(this, sizeof(*this) - sizeof(WORD));
      }
   };
#pragma pack(pop)
   
}  



















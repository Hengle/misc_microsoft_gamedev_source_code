//==============================================================================
// allocationLoggerPackets.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Defines
//==============================================================================
#define CRUNTIME_HEAP         ((void*)0xFFFF0000)
#define XMEMPHYSICAL_HEAP     ((void*)0xFFFF0001)
#define XPHYSICALALLOC_HEAP   ((void*)0xFFFF0002)

//==============================================================================
// Packet structs
//==============================================================================
const uint cALStreamVersion = 0xDEAD0001;
const uint cALPacketPrefix = 0x7F;

enum eALPacketType
{
   cALVersion,
   cALRegisterHeap,
   cALNew,
   cALResize,
   cALDelete,
   cALSnapshot,
   cALEOF,
   cALFrame,
   cALIgnoreLeaf,
   
   cALNumPacketTypes
};

inline uint allocLogPacketGetSize(uint type);

#pragma pack(push, 1)
struct BALPacketBase
{
   BYTE mPacketPrefix;
   BYTE mPacketType;
   WORD mPacketSize;
   
   BALPacketBase(eALPacketType type) :
      mPacketPrefix((BYTE)cALPacketPrefix),
      mPacketType((BYTE)type),
      mPacketSize((WORD)allocLogPacketGetSize(type))
   {
   }
      
   void endianSwitch(void)
   {
      EndianSwitchWorker(this, this + 1, "ccs");
   }
};

struct BALPacketVersion : BALPacketBase
{
   DWORD mVersion;
   DWORD mXEXChecksum;
   DWORD mXEXBaseAddress;
   uint64 mTimerFreq;
   
   BALPacketVersion() : BALPacketBase(cALVersion) { }
   
   void endianSwitch(void)
   {
      BALPacketBase::endianSwitch();
      EndianSwitchDWords((DWORD*)&mVersion, 3);
      EndianSwitchQWords(&mTimerFreq, 1);
   }
};

struct BALPacketRegisterHeap : BALPacketBase
{
   void* mPtr;

   enum
   {
      cFlagPhysical = 1,
   };

   DWORD mFlags;

   BFixedString<32> mName;
   
   BALPacketRegisterHeap() : BALPacketBase(cALRegisterHeap) { }
   
   void endianSwitch(void)
   {
      BALPacketBase::endianSwitch();
      EndianSwitchDWords((DWORD*)&mPtr, 2);
   }
};

struct BALContext
{
   BYTE mThreadIndex;
   BYTE mBackTraceSize;
   uint64 mTime;

   enum { cMaxBackTrace = 16 };
   uint mBackTrace[cMaxBackTrace];
   
   void endianSwitch(void)
   {
      EndianSwitchWorker(this, this + 1, "ccqiiiiiiiiiiiiiiii");
   }
};

struct BALPacketNew : BALPacketBase
{
   void* mpHeap;
   uint mSize;
   void* mpBlock;
   uint mBlockSize;

   BALContext mContext;
   
   BALPacketNew() : BALPacketBase(cALNew) { }
   
   void endianSwitch(void)
   {
      BALPacketBase::endianSwitch();
      
      EndianSwitchDWords((DWORD*)&mpHeap, 4);
      
      mContext.endianSwitch();
   }
};

struct BALPacketResize : BALPacketBase
{
   void* mpHeap;
   void* mpOrigBlock;
   uint mNewSize;
   void* mpNewBlock;

   BALContext mContext;
   
   BALPacketResize() : BALPacketBase(cALResize) { }
   
   void endianSwitch(void)
   {
      BALPacketBase::endianSwitch();

      EndianSwitchDWords((DWORD*)&mpHeap, 4);

      mContext.endianSwitch();
   }
};

struct BALPacketDelete : BALPacketBase
{
   void* mpHeap;
   void* mpBlock;

   BALContext mContext;
   
   BALPacketDelete() : BALPacketBase(cALDelete) { }
   
   void endianSwitch(void)
   {
      BALPacketBase::endianSwitch();

      EndianSwitchDWords((DWORD*)&mpHeap, 2);

      mContext.endianSwitch();
   }
};

struct BALPacketEOF : BALPacketBase
{
   BALPacketEOF() : BALPacketBase(cALEOF) { }

   void endianSwitch(void)
   {
      BALPacketBase::endianSwitch();
   }
};

struct BALPacketSnapshot : BALPacketBase
{
   DWORD mIndex;
   
   BALPacketSnapshot() : BALPacketBase(cALSnapshot) { }

   void endianSwitch(void)
   {
      BALPacketBase::endianSwitch();
      EndianSwitchDWords((DWORD*)&mIndex, 1);
   }
};

struct BALPacketFrame : BALPacketBase
{
   DWORD mIndex;
   uint64 mTime;
   
   BALPacketFrame() : BALPacketBase(cALFrame) { }

   void endianSwitch(void)
   {
      BALPacketBase::endianSwitch();
      
      EndianSwitchDWords((DWORD*)&mIndex, 1);
      EndianSwitchQWords(&mTime, 1);
   }
};

struct BALPacketIgnoreLeaf : BALPacketBase
{
   BFixedString64 mSymbol;
   
   BALPacketIgnoreLeaf() : BALPacketBase(cALIgnoreLeaf) { }
   
   void endianSwitch(void)
   {
      BALPacketBase::endianSwitch();
   }
};

inline uint allocLogPacketGetSize(uint type)
{
   const uint packetSizes[] = 
   { 
      sizeof(BALPacketVersion), 
      sizeof(BALPacketRegisterHeap), 
      sizeof(BALPacketNew), 
      sizeof(BALPacketResize), 
      sizeof(BALPacketDelete), 
      sizeof(BALPacketSnapshot), 
      sizeof(BALPacketBase),
      sizeof(BALPacketFrame),
      sizeof(BALPacketIgnoreLeaf)
   };         

   BCOMPILETIMEASSERT( sizeof(packetSizes)/sizeof(packetSizes[0]) == cALNumPacketTypes);

   BDEBUG_ASSERT(type < cALNumPacketTypes);

   return packetSizes[type];
}

inline void allocLogPacketEndianSwitch(BALPacketBase* pBase)
{
   // Change this BCOMPILETIMEASSERT if you change the number of packet types!
   BCOMPILETIMEASSERT(cALNumPacketTypes == 9);
   switch (pBase->mPacketType)            
   { 
      case cALVersion:
      {
         BALPacketVersion* pPacket = reinterpret_cast<BALPacketVersion*>(pBase);
         pPacket->endianSwitch();
         break;
      }
      case cALRegisterHeap:
      {
         BALPacketRegisterHeap* pPacket = reinterpret_cast<BALPacketRegisterHeap*>(pBase);
         pPacket->endianSwitch();
         break;
      }
      case cALNew:
      {
         BALPacketNew* pPacket = reinterpret_cast<BALPacketNew*>(pBase);
         pPacket->endianSwitch();
         break;
      }
      case cALResize:
      {
         BALPacketResize* pPacket = reinterpret_cast<BALPacketResize*>(pBase);
         pPacket->endianSwitch();
         break;
      }
      case cALDelete:
      {
         BALPacketDelete* pPacket = reinterpret_cast<BALPacketDelete*>(pBase);
         pPacket->endianSwitch();
         break;
      }
      case cALSnapshot:
      {
         BALPacketSnapshot* pPacket = reinterpret_cast<BALPacketSnapshot*>(pBase);
         pPacket->endianSwitch();
         break;
      }
      case cALEOF:
      {
         pBase->endianSwitch();
         break;
      }
      case cALFrame:
      {
         BALPacketFrame* pPacket = reinterpret_cast<BALPacketFrame*>(pBase);
         pPacket->endianSwitch();
         break;
      }
      case cALIgnoreLeaf:
      {
         BALPacketIgnoreLeaf* pPacket = reinterpret_cast<BALPacketIgnoreLeaf*>(pBase);
         pPacket->endianSwitch();
         break;
      }
   }
}
const uint cALMaxPacketSize = 96;
#pragma pack(pop)


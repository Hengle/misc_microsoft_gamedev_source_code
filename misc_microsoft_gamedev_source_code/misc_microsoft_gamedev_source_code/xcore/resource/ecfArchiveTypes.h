//============================================================================
//
//  File: ecfArchiveTypes.h
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once
#include "ecfUtils.h"
#include "utils\packedType.h"

enum 
{  
   cArchiveFilenamesECFChunkID   = 0xB9B82DCD,
};

enum eECFArchiveCompMethod
{
   cECFCompMethodMask          = 7,

   cECFCompMethodStored        = 0,
   cECFCompMethodDeflateRaw    = 1,
   cECFCompMethodDeflateStream = 2
};

#pragma pack(push)
#pragma pack(4)
struct BECFArchiveHeader : public BECFHeader
{
   typedef BPackedType<uint, true> BPackedUInt;

   enum { cMagic = 0x05ABDBD8 };
   
   BECFArchiveHeader() : BECFHeader()
   {
      clear();
   }
   
   void clear(void)
   {
      Utils::ClearObj(*this);
   }

   BPackedUInt mArchiveHeaderMagic;
   BPackedUInt mSignatureSize;
   BPackedUInt mReserved[2];
};

// 32 bytes
struct BECFArchiveChunkHeader : public BECFChunkHeader
{
   typedef BPackedType<uint,   true> BPackedUInt;
   typedef BPackedType<uint64, true> BPackedUInt64; 
   
   BECFArchiveChunkHeader() { clear(); }

   void clear(void)
   {  
      Utils::ClearObj(*this); 
   }

   BPackedUInt64  mDate;
   BPackedUInt    mDecompSize;
   uchar          mCompTiger128[16];
   
   // BECFChunkHeader.mID contains the decompressed file data's Tiger64.
      
   uchar          mNameOfs[3];
   
   uchar          mUnused;
};
#pragma pack(pop)
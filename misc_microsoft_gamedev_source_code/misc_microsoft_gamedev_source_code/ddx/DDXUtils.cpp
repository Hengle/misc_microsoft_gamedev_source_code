//==============================================================================
//
// File: DDXUtils.cpp
//
// Copyright (c) 2006 Ensemble Studios
//
//==============================================================================
#include "xcore.h"
#include "resampler.h"

#include "containers\dynamicarray.h"
#include "math\generalvector.h"
#include "resource\ecfUtils.h"

#include "compression.h"
#include "colorutils.h"
#include "RGBAImage.h"
#include "imageutils.h"
#include "DXTUtils.h"
#include "JPEGcodec\JPGMain.h"
#include "DXTPacker.h"
#include "DXTUnpacker.h"
#include "bytepacker.h"
#include "containers\priorityQueue.h"
#include "DXTMunge.h"

#include "ImageResample.h"
#include "hash\adler32.h"

#include "DDXDef.h"
#include "DDXPackParams.h"
#include "DDXUtils.h"

#include "DDXSerializedTexture.h"

//------------------------------------------------------------------------------------------------------
// BDDXUtils::calcMaxMips
//------------------------------------------------------------------------------------------------------
uint BDDXUtils::calcMaxMips(uint width, uint height)
{
   uint numLevels = 1;

   while ((width > 1) || (height > 1))
   {
      width >>= 1;
      height >>= 1;
      numLevels++;
   }

   return numLevels;
}

//------------------------------------------------------------------------------------------------------
// BDDXUtils::calcMaxMipChainLevels
//------------------------------------------------------------------------------------------------------
uint BDDXUtils::calcMaxMipChainLevels(uint width, uint height)
{  
   return calcMaxMips(width, height) - 1;
}

//------------------------------------------------------------------------------------------------------
// BDDXUtils::calcMaxMipChainLevels
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::calcMipDimension(uint& outWidth, uint& outHeight, uint width, uint height, uint level)
{
   if (level >= calcMaxMips(width, height))
      return false;

   outWidth = Math::Max(1U, width >> level);
   outHeight = Math::Max(1U, height >> level);

   return true;
}

//------------------------------------------------------------------------------------------------------
void BDDXUtils::endianSwapHeader(BDDXHeader& header, uint actualHeaderSize)
{
   actualHeaderSize;
   EndianSwitchWorker(&header, &header + 1, "iiisscccciiii");
}

//------------------------------------------------------------------------------------------------------
// BDDXUtils::getHeader
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::getHeader(const uchar* pDDXData, uint DDXDataLen, BDDXHeader& outHeader)
{
   memset(&outHeader, 0, sizeof(outHeader));

   if ((!pDDXData) || (DDXDataLen < sizeof(BDDXHeader)))
      return false;

   const BDDXHeader& origHeader = *reinterpret_cast<const BDDXHeader*>(pDDXData);
   
   BDDXHeader header;
   Utils::ClearObj(header);
   
   DWORD headerSize = origHeader.mHeaderSize;
         
   const bool endianSwap = (origHeader.mHeaderMagic == static_cast<DWORD>(BDDXHeader::cDDXInvertedHeaderMagic));
   if (endianSwap)
      EndianSwitchDWords(&headerSize, 1);
      
   if (headerSize != sizeof(BDDXHeader))
      return false;      
         
   memcpy(&header, &origHeader, headerSize);
   if (endianSwap)
      endianSwapHeader(header, headerSize);
      
   if (header.mHeaderMagic != static_cast<DWORD>(BDDXHeader::cDDXHeaderMagic))
      return false;
   
   if (header.mHeaderAdler32 != calcAdler32(
      (uchar*)&origHeader + sizeof(DWORD) * BDDXHeader::cDDXHeaderDWORDsToSkip, 
      headerSize - sizeof(DWORD) * BDDXHeader::cDDXHeaderDWORDsToSkip))
   {      
      return false;
   }

   if (header.mMinRequiredVersion > BDDXHeader::cDDXVersion)
      return false;
   
   if (DDXDataLen < header.mHeaderSize)
      return false;   
      
   outHeader = header;         

   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUtils::setHeader
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::setHeader(uchar* pDDXData, uint DDXDataSize, const BDDXHeader& header)
{
   const bool endianSwap = cLittleEndianNative;
   
   if ((!pDDXData) || (DDXDataSize < sizeof(BDDXHeader)))
      return false;

   BDDXHeader newHeader;
   newHeader = header;
   
   newHeader.mHeaderMagic = static_cast<DWORD>(BDDXHeader::cDDXHeaderMagic);
   newHeader.mHeaderSize = sizeof(BDDXHeader);
   
   if (endianSwap)
      endianSwapHeader(newHeader, sizeof(BDDXHeader));

   newHeader.mHeaderAdler32 = calcAdler32((uchar*)&newHeader + sizeof(DWORD) * BDDXHeader::cDDXHeaderDWORDsToSkip, sizeof(BDDXHeader) - sizeof(DWORD) * BDDXHeader::cDDXHeaderDWORDsToSkip);
   
   if (endianSwap)
      EndianSwitchDWords(&newHeader.mHeaderAdler32, 1);

   *reinterpret_cast<BDDXHeader*>(pDDXData) = newHeader;

   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUtils::check
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::check(const uchar* pDDXData, uint DDXDataSize)
{
   BECFFileReader ecfFileReader(BConstDataBuffer(pDDXData, DDXDataSize));

   if (!ecfFileReader.check())      
      return false;

   if (ecfFileReader.getHeader()->getID() != cDDX_ECF_FILE_ID)
      return false;
   
   const int headerChunkIndex = ecfFileReader.findChunkByID(cDDX_ECF_HEADER_CHUNK_ID);
   if (headerChunkIndex < 0)
      return false;

   BDDXHeader header;
   if (!getHeader(ecfFileReader.getChunkDataByIndex(headerChunkIndex), ecfFileReader.getChunkDataLenByIndex(headerChunkIndex), header))
      return false;

   return true;      
}

//------------------------------------------------------------------------------------------------------
// BDDXUtils::getDesc
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::getDesc(const uchar* pDDXData, uint DDXDataSize, BDDXDesc& outDesc)
{
   BECFFileReader ecfFileReader(BConstDataBuffer(pDDXData, DDXDataSize));

   if (!ecfFileReader.check())      
      return false;

   if (ecfFileReader.getHeader()->getID() != cDDX_ECF_FILE_ID)
      return false;

   const int headerChunkIndex = ecfFileReader.findChunkByID(cDDX_ECF_HEADER_CHUNK_ID);
   if (headerChunkIndex < 0)
      return false;

   BDDXHeader header;
   if (!getHeader(ecfFileReader.getChunkDataByIndex(headerChunkIndex), ecfFileReader.getChunkDataLenByIndex(headerChunkIndex), header))
      return false;

   outDesc.mSizeOfStruct         = sizeof(BDDXDesc);
   outDesc.mWidth                = 1 << header.mDimensionPow2[0];
   outDesc.mHeight               = 1 << header.mDimensionPow2[1];
   outDesc.mTotalDataSize        = ecfFileReader.getHeader()->getFileSize();
   outDesc.mMipChainSize         = header.mMipChainSize;
   outDesc.mDataFormat           = header.mDataFormat;
   outDesc.mMip0DataSize         = ecfFileReader.getChunkDataLenByID(cDDX_ECF_MIP0_CHUNK_ID);      
   outDesc.mMipChainDataSize     = ecfFileReader.getChunkDataLenByID(cDDX_ECF_MIPCHAIN_CHUNK_ID);
   outDesc.mResourceType         = header.mResourceType;
   outDesc.mHeaderFlags          = header.mFlags;
   outDesc.mCreatorVersion       = header.mCreatorVersion;
   outDesc.mMinRequiredVersion   = header.mMinRequiredVersion;  
   outDesc.mPlatform             = static_cast<eDDXPlatform>(header.mPlatform);
         
   return true;
}


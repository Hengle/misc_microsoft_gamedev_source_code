//============================================================================
//
// File: DDXUnpacker.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "resampler.h"

#include "containers\dynamicarray.h"
#include "math\generalvector.h"
#include "resource\ecfUtils.h"

#include "compression.h"
#include "colorutils.h"
#include "RGBAImage.h"
#include "DXTUtils.h"
#include "JPEGcodec\JPGMain.h"
#include "DXTPacker.h"
#include "DXTUnpacker.h"
#include "bytepacker.h"
#include "containers\priorityQueue.h"
#include "DXTMunge.h"
#include "DeflateCodec.h"
#include "ImageResample.h"
#include "hash\adler32.h"

#include "DDXDef.h"
#include "DDXPackParams.h"
#include "DDXUtils.h"
#include "DDXUnpacker.h"

#include "DXTQPack.h"
#include "DXTQUnpack.h"

#include <xgraphics.h>

//------------------------------------------------------------------------------------------------------
// BDDXUnpacker::BDDXUnpacker
//------------------------------------------------------------------------------------------------------
BDDXUnpacker::BDDXUnpacker()
{
}

//------------------------------------------------------------------------------------------------------
// BDDXUnpacker::decompressNativeImage
//------------------------------------------------------------------------------------------------------
bool BDDXUnpacker::decompressNativeImage(
   const BYTE*& pData,
   uint& dataSize,
   const BDDXHeader& header, bool mipFlag,
   BByteArray& outStream)
{      
   mipFlag;
   
   // 5 because the image must be at least 1 byte
   if (dataSize < 5)
      return false;

   // If it's DXTM/DXMA, the first 4 bytes are the comp size (written by the codec).
   // Otherwise it's deflated raw data, which starts with the compressed size.
   uint compSize = (pData[0]<<24) | (pData[1]<<16) | (pData[2]<<8) | pData[3];
   if (compSize < 1)
      return false;

   pData += 4;
   dataSize -= 4;

   if (compSize > dataSize)
      return false;

   const eDDXDataFormat format = header.mDataFormat;
   
   if (getDDXDataFormatPackType(format) == cDDXTDPTMipsRaw)
   {
      BDeflateCodec deflateCodec;
      deflateCodec;
      if (!deflateCodec.inflateData(pData, compSize, outStream))
         return false;
   }
   else
   {
      BDEBUG_ASSERT(getDDXDataFormatPackType(format) == cDDXTDPTMipsCompressed);
      outStream.pushBack(pData - 4, compSize + 4);
   }
   
   pData += compSize;
   dataSize -= compSize;

   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUnpacker::decompressNativeTextureNormal
//------------------------------------------------------------------------------------------------------
bool BDDXUnpacker::decompressNativeTextureNormal(
   uint width, uint height,
   const BDDXHeader& header,
   const BYTE* pMip0Data, uint mip0DataSize,
   const BYTE* pMipChainData, uint mipChainDataSize,
   uint numFaces,
   bool unpackMipChain, 
   BByteArray& outStream,
   BDDXTextureInfo& outTextureInfo)
{
   if ((outTextureInfo.mPackType != cDDXTDPTMipsCompressed) && (outTextureInfo.mPackType != cDDXTDPTMipsRaw))
      return false;
   
   outTextureInfo;
         
   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
   {
      if (!decompressNativeImage(pMip0Data, mip0DataSize, header, false, outStream))
         return false;

      if ((unpackMipChain) && (header.mMipChainSize))
      {
         uint mipWidth = width; 
         uint mipHeight = height;      

         for (uint mipIndex = 0; mipIndex < header.mMipChainSize; mipIndex++)
         {
            mipWidth = Math::Max(1U, mipWidth >> 1);
            mipHeight = Math::Max(1U, mipHeight >> 1);

            if (!decompressNativeImage(pMipChainData, mipChainDataSize, header, true, outStream))
               return false;
         }
      }
   }      

   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUnpacker::inflateData
//------------------------------------------------------------------------------------------------------
bool BDDXUnpacker::inflateData(
   const BYTE* pData,
   uint dataSize,
   BByteArray& outStream)
{      
   // 5 because the image must be at least 1 byte
   if (dataSize < 5)
      return false;

   uint compSize = (pData[0]<<24) | (pData[1]<<16) | (pData[2]<<8) | pData[3];
   if (compSize < 1)
      return false;

   pData += 4;
   dataSize -= 4;

   if (compSize > dataSize)
      return false;

   BDeflateCodec deflateCodec;
   deflateCodec;
   if (!deflateCodec.inflateData(pData, compSize, outStream))
      return false;
   
   pData += compSize;
   dataSize -= compSize;

   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUnpacker::decompressNativeTextureXbox
//------------------------------------------------------------------------------------------------------
bool BDDXUnpacker::decompressNativeTextureXbox(
   uint width, uint height,
   const BDDXHeader& header,
   const BYTE* pMip0Data, uint mip0DataSize,
   const BYTE* pMipChainData, uint mipChainDataSize,
   uint numFaces,
   bool unpackMipChain, 
   BByteArray& outStream,
   BDDXTextureInfo& outTextureInfo,
   bool platformSpecificData)
{
   if (outTextureInfo.mPackType != cDDXTDPTMipsRaw)
      return false;
      
   if ((!mip0DataSize) || (!mipChainDataSize))
      return false;
   
   const bool cubemap = (header.mResourceType == cDDXResourceTypeCubeMap);
   if (cubemap)
   {
      if (mipChainDataSize != sizeof(IDirect3DCubeTexture9))
         return false;
   }         
   else
   {
      if (mipChainDataSize != sizeof(IDirect3DTexture9))
         return false;
   }
         
   union
   {
      IDirect3DBaseTexture9 d3dBaseTex;
      IDirect3DCubeTexture9 d3dCubeTex;
      IDirect3DTexture9 d3dTex;
   };
   
   const DWORD headerSize = cubemap ? sizeof(IDirect3DCubeTexture9) : sizeof(IDirect3DTexture9);
   
   memcpy(&d3dBaseTex, pMipChainData, headerSize);
   if (cLittleEndianNative)
      XGEndianSwapMemory(&d3dBaseTex, &d3dBaseTex, XGENDIAN_8IN32, sizeof(DWORD), headerSize / sizeof(DWORD));      
      
   XGTEXTURE_DESC d3dTexDesc;
   XGGetTextureDesc(&d3dBaseTex, 0, &d3dTexDesc);
   const DWORD dwNumLevels = d3dBaseTex.Format.MaxMipLevel + 1;
      
   if ((d3dTexDesc.Width != width) || (d3dTexDesc.Height != height))
      return false;
         
   if (dwNumLevels != (header.mMipChainSize + 1U))
      return false;
      
   UINT d3dTexBaseSize;
   UINT d3dTexMipSize;
   
   D3DTexture texHeader;   
   D3DCubeTexture texCubeHeader;   
   {
      DWORD Usage   = 0;
      UINT  Pitch   = d3dTexDesc.RowPitch;
      DWORD Flags   = 0;
      if( !d3dBaseTex.Format.PackedMips ) Flags |= XGHEADEREX_NONPACKED;
      if( d3dBaseTex.Format.BorderSize )  Flags |= XGHEADEREX_BORDER;
      DWORD dwBaseOffset = d3dBaseTex.Format.BaseAddress << 12;
      DWORD dwMipOffset  = d3dBaseTex.Format.MipAddress  << 12;
      
      if (cubemap)
      {
         XGSetCubeTextureHeaderEx( width, dwNumLevels, Usage, 
            d3dTexDesc.Format, d3dTexDesc.ExpBias, Flags,
            dwBaseOffset, dwMipOffset, 
            &texCubeHeader, &d3dTexBaseSize, &d3dTexMipSize);
         
         if (d3dBaseTex.Format.Pitch != texCubeHeader.Format.Pitch)
            return false;
      }
      else
      {
         XGSetTextureHeaderEx( width, height, dwNumLevels, Usage, 
            d3dTexDesc.Format, d3dTexDesc.ExpBias, Flags,
            dwBaseOffset, dwMipOffset, Pitch,
            &texHeader, &d3dTexBaseSize, &d3dTexMipSize);
            
         if (d3dBaseTex.Format.Pitch != texHeader.Format.Pitch)
            return false;
      }            
   }
   
   if (platformSpecificData)
   {
      outTextureInfo.mPlatform = cDDXPlatformXbox;
      
      outStream.reserve(outStream.size() + Utils::AlignUpValue(headerSize, 16) + d3dTexBaseSize + d3dTexMipSize);
      
      outStream.pushBack(reinterpret_cast<const BYTE*>(&d3dBaseTex), headerSize);
      
      uint numExtraBytes = Utils::BytesToAlignUpValue(headerSize, 16);
      while (numExtraBytes)
      {
         outStream.pushBack(0);
         numExtraBytes--;
      }
      
      if (!inflateData(pMip0Data, mip0DataSize, outStream))
         return false;
         
      return true;         
   }

   BByteArray d3dTexData;

   if (!inflateData(pMip0Data, mip0DataSize, d3dTexData))
      return false;
   
   if (d3dTexData.size() != (d3dTexBaseSize + d3dTexMipSize))
      return false;
            
   const DWORD mipTailBaseLevel = XGGetMipTailBaseLevel( width, height, d3dBaseTex.Format.BorderSize );
   mipTailBaseLevel;

   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
   {
      for (uint mipLevel = 0; mipLevel < (unpackMipChain ? (header.mMipChainSize + 1U) : 1U); mipLevel++)
      {
         uint mipWidth, mipHeight;
         BDDXUtils::calcMipDimension(mipWidth, mipHeight, width, height, mipLevel);

         int widthInBlocks  = mipWidth;
         int heightInBlocks = mipHeight;
         if (getDDXDataFormatIsDXT(header.mDataFormat))
         {
            widthInBlocks = Math::Max<int>(1, widthInBlocks >> 2);
            heightInBlocks = Math::Max<int>(1, heightInBlocks >> 2);
         }

         uint dstPitch;
         if (getDDXDataFormatIsDXT(header.mDataFormat))
            dstPitch = widthInBlocks * getDDXDataFormatDXTBlockSize(header.mDataFormat); 
         else
            dstPitch = (mipWidth * getDDXDataFormatBitsPerPixel(header.mDataFormat)) / 8;
            
         const uint dstSize = dstPitch * heightInBlocks;
                     
         const uint outStreamSize = outStream.size();
         outStream.resize(outStreamSize + dstSize);
         
         BYTE* pDst = outStream.getPtr() + outStreamSize;
         
//-- FIXING PREFIX BUG ID 6884
         const BYTE* pSrc = d3dTexData.getPtr();
//--
         
         if (mipLevel == 0)
            pSrc += (d3dBaseTex.Format.BaseAddress << 12);
         else
            pSrc += (d3dBaseTex.Format.MipAddress << 12);
                        
         const uint mipLevelOffset = XGGetMipLevelOffset( &d3dBaseTex, faceIndex, mipLevel );
         pSrc += mipLevelOffset;

         const DWORD flags = d3dBaseTex.Format.PackedMips ? 0 : XGTILE_NONPACKED;

         XGUntileTextureLevel(
            width,
            height,
            mipLevel,
            XGGetGpuFormat(d3dTexDesc.Format),
            flags,
            pDst,
            dstPitch,
            NULL,
            pSrc,
            NULL);
         
         // Xbox data is always big endian, but DDX always returns little endian data.
         if ((getDDXDataFormatIsDXT(header.mDataFormat)) || (getDDXDataFormatBitsPerPixel(header.mDataFormat) == 16))
         {                        
            const int numWORDs = dstSize / (sizeof(WORD));
            EndianSwitchWords(reinterpret_cast<WORD*>(pDst), numWORDs);
         }
         else if (getDDXDataFormatBitsPerPixel(header.mDataFormat) == 64)
         {
            if (header.mDataFormat == cDDXDataFormatA16B16G16R16F)
            {
               const int numWORDs = dstSize / (sizeof(WORD));
               EndianSwitchWords(reinterpret_cast<WORD*>(pDst), numWORDs);
            }
            else
            {
               return false;
            }
         }
         else if (getDDXDataFormatBitsPerPixel(header.mDataFormat) == 32)
         {
            const int numDWORDs = dstSize / (sizeof(DWORD));
            EndianSwitchDWords(reinterpret_cast<DWORD*>(pDst), numDWORDs);
         }
         else if (getDDXDataFormatBitsPerPixel(header.mDataFormat) != 8)
         {
            return false;  
         }
      }
   }   

   return true;
}   

bool BDDXUnpacker::getTextureInfo(const BECFFileReader& ecfFileReader, bool unpackMipChain, BDDXHeader& outHeader, BDDXTextureInfo& outTextureInfo)
{
   if (outTextureInfo.mSizeOfStruct != sizeof(BDDXTextureInfo))
      return false;

   if (!ecfFileReader.check())
      return false;

   if (ecfFileReader.getHeader()->getID() != cDDX_ECF_FILE_ID)
      return false;

   const int headerChunkIndex = ecfFileReader.findChunkByID(cDDX_ECF_HEADER_CHUNK_ID);
   if (headerChunkIndex < 0)
      return false;

   if (!BDDXUtils::getHeader(ecfFileReader.getChunkDataByIndex(headerChunkIndex), ecfFileReader.getChunkDataLenByIndex(headerChunkIndex), outHeader))
      return false;

   const uint width = 1 << outHeader.mDimensionPow2[0];
   const uint height = 1 << outHeader.mDimensionPow2[1];
   
   if ((width > BDDXHeader::cMaxWidth) || (height > BDDXHeader::cMaxHeight))
      return false;

   if (outHeader.mDataFormat >= cDDXDataFormatMax)
      return false;

   outTextureInfo.mSizeOfStruct = sizeof(BDDXTextureInfo);
   outTextureInfo.mWidth = width;
   outTextureInfo.mHeight = height;
   outTextureInfo.mNumMipChainLevels = unpackMipChain ? outHeader.mMipChainSize : 0;
   outTextureInfo.mDataFormat = outHeader.mDataFormat;
   outTextureInfo.mOrigDataFormat = outTextureInfo.mDataFormat;
   outTextureInfo.mResourceType = outHeader.mResourceType;
   outTextureInfo.mHasAlpha = (0 != (outHeader.mFlags & BDDXHeader::cDDXHeaderFlagsHasAlpha));
   outTextureInfo.mPlatform = cDDXPlatformNone;
   outTextureInfo.mHDRScale = outHeader.mHDRScale;
   outTextureInfo.mPackType = getDDXDataFormatPackType(outHeader.mDataFormat);   

   return true;
}                    

//------------------------------------------------------------------------------------------------------
// BDDXUnpacker::unpackToNative
//------------------------------------------------------------------------------------------------------
bool BDDXUnpacker::unpackToNative(
   const BYTE* pDDXData, uint DDXDataSize,
   bool unpackMipChain, bool platformSpecificData, 
   BDDXTextureInfo& outTextureInfo,
   BByteArray& outStream)
{
   if (outTextureInfo.mSizeOfStruct != sizeof(BDDXTextureInfo))
      return false;
      
   BECFFileReader ecfFileReader(BConstDataBuffer(pDDXData, DDXDataSize));
   
   BDDXHeader header;
   
   if (!getTextureInfo(ecfFileReader, unpackMipChain, header, outTextureInfo))
      return false;
      
   const uint width = 1 << header.mDimensionPow2[0];
   const uint height = 1 << header.mDimensionPow2[1];
   uint numFaces = 1;
   if (header.mResourceType == cDDXResourceTypeCubeMap)
      numFaces = 6;
   
   const BYTE* pMip0Data = NULL;
   uint mip0DataSize = 0;
   const int mip0ChunkIndex = ecfFileReader.findChunkByID(cDDX_ECF_MIP0_CHUNK_ID);
   if (mip0ChunkIndex >= 0)
   {
      pMip0Data = ecfFileReader.getChunkDataByIndex(mip0ChunkIndex);
      mip0DataSize = ecfFileReader.getChunkDataLenByIndex(mip0ChunkIndex);
   }
   
   const int mipChainChunkIndex = ecfFileReader.findChunkByID(cDDX_ECF_MIPCHAIN_CHUNK_ID);
   const BYTE* pMipChainData = NULL;
   uint mipChainDataSize = 0;
   if (mipChainChunkIndex >= 0)
   {
      pMipChainData = ecfFileReader.getChunkDataByIndex(mipChainChunkIndex);
      mipChainDataSize = ecfFileReader.getChunkDataLenByIndex(mipChainChunkIndex);
   }

   if (outTextureInfo.mPackType == cDDXTDPTNoMipsCompressed)
   {
      if (!pMip0Data)
         return false;
         
      if ((platformSpecificData) && (getDDXDataFormatIsDXTQ(header.mDataFormat)))
      {
         BDDXTextureInfo unpackedTextureInfo;
         
         IDirect3DTexture9 d3dHeader;
                  
         BDXTQUnpack dxtqUnpack;
         dxtqUnpack;
         
         uint numBaseBlocks, numMipChainBlocks, bytesPerBlock;
         if (!dxtqUnpack.getDXTQInfo(outTextureInfo, pMip0Data, mip0DataSize, unpackedTextureInfo, &d3dHeader, numBaseBlocks, numMipChainBlocks, bytesPerBlock))
            return false;
         
         outStream.resize(Utils::AlignUpValue(sizeof(d3dHeader), 16));
         memcpy(outStream.getPtr(), &d3dHeader, sizeof(d3dHeader));
         
         const uint dstTexSize = (numBaseBlocks + numMipChainBlocks) * bytesPerBlock;
         BYTE* pDstTex = outStream.enlarge(dstTexSize);
                  
         if (!dxtqUnpack.unpackDXTQToTiledDXT(outTextureInfo.mDataFormat, pMip0Data, mip0DataSize, pDstTex, dstTexSize, true, true))
            return false;
         
         outTextureInfo = unpackedTextureInfo;
      }
      else
      {
         outStream.pushBack(pMip0Data, mip0DataSize);
      }
   }
   else if ((header.mPlatform == cDDXPlatformNone) || (header.mCreatorVersion == 1))
   {
      return decompressNativeTextureNormal(width, height, header, pMip0Data, mip0DataSize, pMipChainData, mipChainDataSize, numFaces, unpackMipChain, outStream, outTextureInfo);
   }
   else if (header.mPlatform == cDDXPlatformXbox)
   {
      return decompressNativeTextureXbox(width, height, header, pMip0Data, mip0DataSize, pMipChainData, mipChainDataSize, numFaces, unpackMipChain, outStream, outTextureInfo, platformSpecificData);
   }      
   else
   {
      return false;
   }

   return true;
}


//============================================================================
//
// File: DDXPacker.cpp
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
#include "DXTMunge.h"
#include "DeflateCodec.h"
#include "ImageResample.h"
#include "hash\adler32.h"

#include "DDXDef.h"
#include "DDXPackParams.h"
#include "DDXUtils.h"
#include "DDXSerializedTexture.h"
#include "DDXPacker.h"

#include <xgraphics.h>

//------------------------------------------------------------------------------------------------------
// BDDXPacker::BDDXPacker
//------------------------------------------------------------------------------------------------------
BDDXPacker::BDDXPacker()
{
}

//------------------------------------------------------------------------------------------------------
// BDDXPacker::createDDXStream
//------------------------------------------------------------------------------------------------------
bool BDDXPacker::createDDXStream(
   const BMipDataArray* pFaceData,
   const uint width,
   const uint height,
   const bool hasAlpha,
   const float hdrScale,
   const eDDXDataFormat dataFormat,
   const eDDXResourceType resourceType,
   const uint numMipChainLevels,
   BByteArray& stream,
   eDDXPlatform platform)
{
   BDDXHeader header;      
   memset(&header, 0, sizeof(header));   

   header.mHeaderSize         = sizeof(BDDXHeader);
   header.mCreatorVersion     = static_cast<WORD>(BDDXHeader::cDDXVersion);
   header.mMinRequiredVersion = cDDX_MIN_REQUIRED_VERSION;
   header.mDimensionPow2[0]   = static_cast<BYTE>(Math::iLog2(width));
   header.mDimensionPow2[1]   = static_cast<BYTE>(Math::iLog2(height));
   header.mMipChainSize       = static_cast<BYTE>(numMipChainLevels);
   header.mDataFormat         = dataFormat;
   header.mResourceType       = resourceType;
   header.mPlatform           = static_cast<BYTE>(platform);
   header.mFlags              = 0;
   header.mHDRScale           = hdrScale;

   if (hasAlpha)
      header.mFlags |= BDDXHeader::cDDXHeaderFlagsHasAlpha;

   BECFFileBuilder ecfBuilder;
   ecfBuilder.setID(cDDX_ECF_FILE_ID);
      
   ecfBuilder.addChunk(cDDX_ECF_HEADER_CHUNK_ID, reinterpret_cast<const uchar*>(&header), sizeof(BDDXHeader));

   const uint numFaces = getDDXResourceTypeNumFaces(resourceType);

   BByteArray mipData;
   
   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)      
   {               
      for (uint mipChainIndex = 1; mipChainIndex < pFaceData[faceIndex].size(); mipChainIndex++)
      {
         if (pFaceData[faceIndex][mipChainIndex].size())
         {
            mipData.pushBack(&pFaceData[faceIndex][mipChainIndex][0], pFaceData[faceIndex][mipChainIndex].size());
         }
      }
   }
   
   if (mipData.getSize())
      ecfBuilder.addChunk(cDDX_ECF_MIPCHAIN_CHUNK_ID, mipData).setAlignmentLog2(4);

   mipData.resize(0);
      
   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)      
   {
      if ((pFaceData[faceIndex].size() >= 1) && (pFaceData[faceIndex][0].size()))
      {
         mipData.pushBack(&pFaceData[faceIndex][0][0], pFaceData[faceIndex][0].size());
      }
   }      

   if (mipData.getSize())
      ecfBuilder.addChunk(cDDX_ECF_MIP0_CHUNK_ID, mipData).setAlignmentLog2(4);
      
   if (!BDDXUtils::setHeader(ecfBuilder.getChunkByIndex(0).getDataPtr(), ecfBuilder.getChunkByIndex(0).getDataLen(), header))
      return false;
   
   if (!ecfBuilder.writeToFileInMemory(stream))
      return false;

   if (!BDDXUtils::check(stream.getPtr(), stream.getSize()))
      return false;
   
   return true;    
}

//------------------------------------------------------------------------------------------------------
// BDDXPacker::deflateData
//------------------------------------------------------------------------------------------------------
bool BDDXPacker::deflateData(const BYTE* pSrcData, uint srcDataSize, BByteArray& stream)
{   
   const uint streamOfs = stream.size();

   stream.resize(streamOfs + 4);

   if (!BDeflateCodec::deflateData(pSrcData, srcDataSize, stream))
      return false;

   if (stream.size() <= streamOfs)
      return false;

   uint compressedSize = (stream.size() - streamOfs);

   // Store size of compressed data (not including this DWORD) at beginning of stream.
   compressedSize -= 4;
   stream[streamOfs+0] = (uchar)(compressedSize >> 24);
   stream[streamOfs+1] = (uchar)(compressedSize >> 16);
   stream[streamOfs+2] = (uchar)(compressedSize >> 8);
   stream[streamOfs+3] = (uchar)(compressedSize);

   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXPacker::translateDDXFormatToD3D
//------------------------------------------------------------------------------------------------------
bool BDDXPacker::translateDDXFormatToD3D(D3DFORMAT& format, eDDXDataFormat ddxFormat, bool bigEndian, bool tiled)
{
   const DWORD bigEndianMask = bigEndian ? UINT_MAX : ~D3DDECLTYPE_ENDIAN_MASK;

   switch (ddxFormat)
   {
      case cDDXDataFormatA16B16G16R16F:
      {
         format = (D3DFORMAT)(D3DFMT_A16B16G16R16F & bigEndianMask); 
         break;
      }
      case cDDXDataFormatA8R8G8B8:     
      {
         format = (D3DFORMAT)(D3DFMT_A8R8G8B8 & bigEndianMask); 
         break;
      }
      case cDDXDataFormatA8B8G8R8:
      {
         format = (D3DFORMAT)(D3DFMT_A8B8G8R8 & bigEndianMask);
         break;
      }
      case cDDXDataFormatA8:        
      {
         format = D3DFMT_A8;
         break;
      }                 
      case cDDXDataFormatDXT1:                              //DXT1 or DXT1A
      {
         format = (D3DFORMAT)(D3DFMT_DXT1 & bigEndianMask);
         break;
      }
      case cDDXDataFormatDXT3:                              //explicit 4-bit alpha
      {
         format = (D3DFORMAT)(D3DFMT_DXT3 & bigEndianMask);
         break;
      }
      case cDDXDataFormatDXT5:                              //block alpha
      case cDDXDataFormatDXT5N:                             //swizzled normal map
      case cDDXDataFormatDXT5Y:                             //luma/chroma DXT5, alpha is in red
      case cDDXDataFormatDXT5H:
      {
         format = (D3DFORMAT)(D3DFMT_DXT5 & bigEndianMask);
         break;
      }
      case cDDXDataFormatDXN:
      {
         format = (D3DFORMAT)(D3DFMT_DXN & bigEndianMask);
         break;
      }
      default:
      {
         return false;
      }
   }

   //#define MAKELINFMT(D3dFmt) ((D3dFmt) & ~D3DFORMAT_TILED_MASK)
   if (!tiled)
      format = (D3DFORMAT)MAKELINFMT(format);

   return true;
}         

//------------------------------------------------------------------------------------------------------
// BDDXPacker::packTextureXbox
//------------------------------------------------------------------------------------------------------
bool BDDXPacker::packTextureXbox(
   BMipDataArray faceData[6], 
   const BDDXSerializedTexture& srcTexture,
   const BDDXTextureInfo& textureInfo,
   uint numFaces, bool cubemap, uint numMipChainLevels, uint totalMipLevels)
{
   if (textureInfo.mPackType != cDDXTDPTMipsRaw)
      return false;
   
   cubemap;
   numMipChainLevels;
   totalMipLevels;
      
   D3DFORMAT d3dFormat;
   if (!translateDDXFormatToD3D(d3dFormat, textureInfo.mDataFormat, true, true))
      return false;

   union
   {
      IDirect3DBaseTexture9 d3dBaseTex;
      IDirect3DTexture9 d3dTex;
      IDirect3DCubeTexture9 d3dCubeTex;
   };
   
   bool packedMips;

   UINT d3dTexBaseSize = 0;
   UINT d3dTexMipSize = 0;

   const DWORD texPitch = 0;
   DWORD d3dTexTotalSize;

   if (cubemap)
   {
      d3dTexTotalSize = XGSetCubeTextureHeader(
         textureInfo.mWidth,
         textureInfo.mNumMipChainLevels + 1,
         0,
         d3dFormat,
         D3DPOOL_DEFAULT,
         0,
         XGHEADER_CONTIGUOUS_MIP_OFFSET,
         &d3dCubeTex,
         &d3dTexBaseSize,
         &d3dTexMipSize);
   }
   else
   {
      d3dTexTotalSize = XGSetTextureHeader( 
         textureInfo.mWidth, 
         textureInfo.mHeight, 
         textureInfo.mNumMipChainLevels + 1, 0, 
         d3dFormat, D3DPOOL_DEFAULT, 
         0, 
         XGHEADER_CONTIGUOUS_MIP_OFFSET,
         texPitch, 
         &d3dTex, 
         &d3dTexBaseSize, 
         &d3dTexMipSize);
   }  
   
   packedMips = d3dBaseTex.Format.PackedMips;          

   BByteArray tempBuf;
   BByteArray tileBuf(d3dTexTotalSize);

   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
   {
      for (uint mipLevel = 0; mipLevel < textureInfo.mNumMipChainLevels + 1; mipLevel++)
      {     
         uint mipWidth, mipHeight;
         BDDXUtils::calcMipDimension(mipWidth, mipHeight, textureInfo.mWidth, textureInfo.mHeight, mipLevel);

         const uchar* pSrcData = srcTexture.getSurfaceData(faceIndex, mipLevel);
         const uint srcDataSize = srcTexture.getSurfaceDataSize(faceIndex, mipLevel);

         int widthInBlocks  = mipWidth;
         int heightInBlocks = mipHeight;
         if (getDDXDataFormatIsDXT(textureInfo.mDataFormat))
         {
            widthInBlocks = Math::Max<int>(1, widthInBlocks >> 2);
            heightInBlocks = Math::Max<int>(1, heightInBlocks >> 2);
         }

         int sourcePitch;
         if (getDDXDataFormatIsDXT(textureInfo.mDataFormat))
            sourcePitch = widthInBlocks * getDDXDataFormatDXTBlockSize(textureInfo.mDataFormat); 
         else
            sourcePitch = srcTexture.getSurfaceDataSize(faceIndex, mipLevel) / mipHeight;      

         tempBuf.resize(srcDataSize);
         memcpy(tempBuf.getPtr(), pSrcData, srcDataSize);

         const BYTE* pSrc = tempBuf.getPtr();

         if (cLittleEndianNative)
         {
            if ((getDDXDataFormatIsDXT(textureInfo.mDataFormat)) || (getDDXDataFormatBitsPerPixel(textureInfo.mDataFormat) == 16))
            {                        
               const int numWORDs = srcDataSize / (sizeof(WORD));
               EndianSwitchWords(reinterpret_cast<WORD*>(tempBuf.getPtr()), numWORDs);
            }
            else if (getDDXDataFormatBitsPerPixel(textureInfo.mDataFormat) == 64)
            {
               if (textureInfo.mDataFormat == cDDXDataFormatA16B16G16R16F)
               {
                  const int numWORDs = srcDataSize / (sizeof(WORD));
                  EndianSwitchWords(reinterpret_cast<WORD*>(tempBuf.getPtr()), numWORDs);
               }
               else
               {
                  return false;
               }
            }
            else if (getDDXDataFormatBitsPerPixel(textureInfo.mDataFormat) == 32)
            {
               const int numDWORDs = srcDataSize / (sizeof(DWORD));
               EndianSwitchDWords(reinterpret_cast<DWORD*>(tempBuf.getPtr()), numDWORDs);
            }
            else if (getDDXDataFormatBitsPerPixel(textureInfo.mDataFormat) != 8)
            {
               return false;  
            }
         }

         uint dstOfs = XGGetMipLevelOffset( &d3dBaseTex, faceIndex, mipLevel );

         if ((mipLevel > 0) && (d3dTexMipSize > 0))
            dstOfs += d3dTexBaseSize;

         BDEBUG_ASSERT(dstOfs < tileBuf.getSize());

         BYTE* pDst = tileBuf.getPtr() + dstOfs;

         const DWORD flags = packedMips ? 0 : XGTILE_NONPACKED;

         XGTileTextureLevel(
            textureInfo.mWidth, textureInfo.mHeight, mipLevel, 
            XGGetGpuFormat(d3dFormat), flags, pDst, NULL, pSrc, sourcePitch, NULL);            
      }         
   }
   
   faceData[0].resize(2);

   if (!deflateData(tileBuf.getPtr(), tileBuf.getSize(), faceData[0][0]))
      return false;

   BYTE* pHeader = reinterpret_cast<BYTE*>(&d3dBaseTex);
   const uint headerSize = cubemap ? sizeof(IDirect3DCubeTexture9) : sizeof(IDirect3DTexture9);
   
   if (cLittleEndianNative)
      XGEndianSwapMemory(pHeader, pHeader, XGENDIAN_8IN32, sizeof(DWORD), headerSize / sizeof(DWORD));
   
   faceData[0][1].pushBack(pHeader, headerSize);

   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXPacker::compressSurface
//------------------------------------------------------------------------------------------------------
bool BDDXPacker::compressSurface(
   const BYTE* pSrcData, uint srcDataSize,
   const BDDXTextureInfo& textureInfo,
   BByteArray& stream)
{
   if (textureInfo.mPackType == cDDXTDPTNoMipsCompressed)
      stream.pushBack(pSrcData, srcDataSize);
   else
      return deflateData(pSrcData, srcDataSize, stream);

   return true;
}   

//------------------------------------------------------------------------------------------------------
// BDDXPacker::packTextureNormal
//------------------------------------------------------------------------------------------------------ 
bool BDDXPacker::packTextureNormal(
   const BYTE* pData, const uint dataSize,
   BMipDataArray faceData[6], 
   const BDDXSerializedTexture& srcTexture,
   const BDDXTextureInfo& textureInfo,
   uint numFaces, bool cubemap, uint numMipChainLevels, uint totalMipLevels)
{
   pData;
   cubemap;
   dataSize;
   
   if ((textureInfo.mPackType != cDDXTDPTMipsRaw) && (textureInfo.mPackType != cDDXTDPTMipsCompressed))
      return false;
      
   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
   {
      faceData[faceIndex].resize(totalMipLevels);

      const uchar* pSrcData = srcTexture.getSurfaceData(faceIndex, 0);
      const uint srcDataSize = srcTexture.getSurfaceDataSize(faceIndex, 0);
      if (!compressSurface(pSrcData, srcDataSize, textureInfo, faceData[faceIndex][0]))
         return false;

      for (uint mipChainIndex = 0; mipChainIndex < numMipChainLevels; mipChainIndex++)
      {     
         uint mipWidth, mipHeight;
         BDDXUtils::calcMipDimension(mipWidth, mipHeight, textureInfo.mWidth, textureInfo.mHeight, mipChainIndex + 1);

         const uchar* pSrcData = srcTexture.getSurfaceData(faceIndex, mipChainIndex + 1);
         const uint srcDataSize = srcTexture.getSurfaceDataSize(faceIndex, mipChainIndex + 1);

         if (!compressSurface(pSrcData, srcDataSize, textureInfo, faceData[faceIndex][1 + mipChainIndex]))
            return false;       
      }         
   }
         
   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXPacker::packNative
//------------------------------------------------------------------------------------------------------ 
bool BDDXPacker::packNative(
   const BYTE* pData, const uint dataSize,
   const BDDXTextureInfo& textureInfo,
   BByteArray& stream,
   eDDXPlatform platform)
{
   if ((platform < 0) || (platform >= cDDXPlatformMax))
      return false;
         
   if (textureInfo.mSizeOfStruct != sizeof(BDDXTextureInfo))
      return false;
         
   if (textureInfo.mDataFormat == cDDXDataFormatInvalid)
      return false;
      
   if (textureInfo.mPlatform != cDDXPlatformNone)
      return false;
      
   const uint width = textureInfo.mWidth;
   const uint height = textureInfo.mHeight;

   if ((!Math::IsPow2(width)) || (!Math::IsPow2(height)))
      return false;
      
   const uint widthLog2 = Math::iLog2(textureInfo.mWidth);
   const uint heightLog2 = Math::iLog2(textureInfo.mHeight);

   if ((widthLog2 > BDDXHeader::cMaxWidthLog2) || (heightLog2 > BDDXHeader::cMaxHeightLog2))
      return false;

   const bool cubemap = (textureInfo.mResourceType == cDDXResourceTypeCubeMap);
   const uint numFaces = cubemap ? 6 : 1;
   if (cubemap)
   {
      // Cubemaps must be square.
      if (widthLog2 != heightLog2)
         return false;
   }
   
   if ((!pData) || (!dataSize))
      return false;

   const uint maxMipChainLevels = BDDXUtils::calcMaxMipChainLevels(width, height);
   if (textureInfo.mNumMipChainLevels > maxMipChainLevels)
      return false;

   const uint numMipChainLevels = textureInfo.mNumMipChainLevels;
   
   const uint totalMipLevels = 1 + numMipChainLevels;
   
   const BDDXSerializedTexture srcTexture(pData, dataSize, textureInfo);
   const int srcTextureDataSize = srcTexture.getTotalDataSize();
   if (srcTextureDataSize < 0)
      return false;
   if (static_cast<uint>(srcTextureDataSize) > dataSize)
      return false;

   BMipDataArray faceData[6];
            
   if (textureInfo.mPackType == cDDXTDPTNoMipsCompressed)
   {
      faceData[0].resize(1);
      faceData[0][0].pushBack(pData, dataSize);
      
      // Compressed data can't be converted to a platform specific format.
      platform = cDDXPlatformNone;
   }
   else if (platform == cDDXPlatformXbox)
   {
      if (!packTextureXbox(faceData, srcTexture, textureInfo, numFaces, cubemap, numMipChainLevels, totalMipLevels))
         return false;
   }
   else
   {
      if (!packTextureNormal(pData, dataSize, faceData, srcTexture, textureInfo, numFaces, cubemap, numMipChainLevels, totalMipLevels))
         return false;
   }      

   return createDDXStream(
      faceData, 
      textureInfo.mWidth, textureInfo.mHeight, textureInfo.mHasAlpha != FALSE, textureInfo.mHDRScale,
      textureInfo.mDataFormat, textureInfo.mResourceType, numMipChainLevels, 
      stream,
      platform);
}





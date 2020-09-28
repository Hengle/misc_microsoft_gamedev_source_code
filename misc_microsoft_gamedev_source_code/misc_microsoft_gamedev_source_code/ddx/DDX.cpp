// File: DDX.cpp
#include "ecore.h"
#include "resampler.h"
#include "mathutil.h"
#include "alignedarray.h"
#include "generalvector.h"

#include "compression.h"
#include "colorutils.h"
#include "RGBAImage.h"
#include "DXTUtils.h"
#include "JPEGcodec\JPGMain.h"
#include "DXTPacker.h"
#include "DXTUnpacker.h"
#include "bytepacker.h"
#include "priorityQueue.h"
#include "DXTMunge.h"
#include "DeflateCodec.h"
#include "ImageResample.h"
#include "adler32.h"

#include "DDXDef.h"
#include "DDXPackParams.h"
#include "DDX.h"

#include "DDXSerializedTexture.h"

const WORD DDX_MIN_REQUIRED_VERSION = 1;

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
// BDDXUtils::getHeader
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::getHeader(const uchar* pDDXData, uint DDXDataLen, BDDXHeader& outHeader)
{
   memset(&outHeader, 0, sizeof(outHeader));
   
   if ((!pDDXData) || (DDXDataLen < sizeof(BDDXHeader)))
      return false;
      
   const BDDXHeader& header = *reinterpret_cast<const BDDXHeader*>(pDDXData);

   if (header.mHeaderMagic != static_cast<DWORD>(BDDXHeader::cDDXHeaderMagic))
      return false;

   if (header.mHeaderSize < sizeof(BDDXHeader))
      return false;

   if (header.mHeaderAdler32 != adler32((uchar*)&header + sizeof(DWORD) * BDDXHeader::cDDXHeaderDWORDsToSkip, sizeof(BDDXHeader) - sizeof(DWORD) * BDDXHeader::cDDXHeaderDWORDsToSkip))
      return false;

   if (header.mMinRequiredVersion > BDDXHeader::cDDXVersion)
      return false;
      
   uint totalDataSize = sizeof(BDDXHeader);

   if (header.mMip0DataSize)
      totalDataSize = Math::Max<uint>(totalDataSize, header.mMip0DataOffset + header.mMip0DataSize);

   if (header.mMipChainDataSize)
      totalDataSize = Math::Max<uint>(totalDataSize, header.mMipChainDataOffset + header.mMipChainDataSize);
      
   if (DDXDataLen < totalDataSize)
      return false;   
      
   outHeader = header;         

   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUtils::setHeader
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::setHeader(uchar* pDDXData, uint DDXDataSize, const BDDXHeader& header)
{
   if ((!pDDXData) || (DDXDataSize < sizeof(BDDXHeader)))
      return false;

   BDDXHeader newHeader;
   newHeader = header;
         
   newHeader.mHeaderAdler32 = adler32((uchar*)&newHeader + sizeof(DWORD) * BDDXHeader::cDDXHeaderDWORDsToSkip, sizeof(BDDXHeader) - sizeof(DWORD) * BDDXHeader::cDDXHeaderDWORDsToSkip);

   *reinterpret_cast<BDDXHeader*>(pDDXData) = newHeader;
      
   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUtils::check
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::check(const uchar* pDDXData, uint DDXDataSize)
{
   BDDXHeader header;
   if (!getHeader(pDDXData, DDXDataSize, header))
      return false;
      
   if (header.mMip0DataSize)      
   {
      if (header.mMip0DataAdler32 != adler32(pDDXData + header.mMip0DataOffset, header.mMip0DataSize))
         return false;
   }
   
   if (header.mMipChainDataSize)
   {
      if (header.mMipChainDataAdler32 != adler32(pDDXData + header.mMipChainDataOffset, header.mMipChainDataSize))
         return false;
   }
      
   return true;      
}

//------------------------------------------------------------------------------------------------------
// BDDXUtils::getDesc
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::getDesc(const uchar* pDDXData, uint DDXDataSize, BDDXDesc& outDesc)
{
   BDDXHeader header;
   if (!getHeader(pDDXData, DDXDataSize, header))
      return false;
         
   outDesc.mSizeOfStruct = sizeof(BDDXDesc);
   outDesc.mWidth = 1 << header.mDimensionPow2[0];
   outDesc.mHeight = 1 << header.mDimensionPow2[0];
   outDesc.mTotalDataSize = sizeof(BDDXHeader);
   
   if (header.mMip0DataSize)
      outDesc.mTotalDataSize = Math::Max<uint>(outDesc.mTotalDataSize, header.mMip0DataOffset + header.mMip0DataSize);
      
   if (header.mMipChainDataSize)
      outDesc.mTotalDataSize = Math::Max<uint>(outDesc.mTotalDataSize, header.mMipChainDataOffset + header.mMipChainDataSize);
      
   outDesc.mMipChainSize = header.mMipChainSize;
   outDesc.mMip0DataFormat = header.mMip0Format;
   outDesc.mMip0DataSize = header.mMip0DataSize;      
   outDesc.mMipChainDataFormat = header.mMipChainFormat;
   outDesc.mMipChainDataSize = header.mMipChainDataSize;
   outDesc.mResourceType = header.mResourceType;
   outDesc.mHeaderFlags = header.mFlags;
   outDesc.mCreatorVersion = header.mCreatorVersion;
   outDesc.mMinRequiredVersion = header.mMinRequiredVersion;  
      
   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUtils::getChainSize
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::getChainSize(const uchar* pDDXData, uint DDXDataSize, uint& outNumTextures)
{
   if ((!pDDXData) || (DDXDataSize < sizeof(BDDXHeader)))
      return false;
      
   outNumTextures = 0;
   
   for ( ; ; )
   {
      BDDXHeader header;
      if (!getHeader(pDDXData, DDXDataSize, header))
         return false;
      
      outNumTextures++;
      
      if (0 == header.mLinkOffset)
         break;
                  
      pDDXData += header.mLinkOffset;
   }

   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUtils::getLinkedTexture
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::getLinkedTexture(const uchar* pDDXData, uint DDXDataSize, uint textureIndex, uint& outOfs)
{
   if ((!pDDXData) || (DDXDataSize < sizeof(BDDXHeader)))
      return false;
      
   outOfs = 0;
   uint curTextureIndex = 0;
   
   for ( ; ; )
   {
      BDDXHeader header;
      if (!getHeader(pDDXData, DDXDataSize, header))
         return false;
         
      if (textureIndex == curTextureIndex)
         break;         

      curTextureIndex++;
      
      if (0 == header.mLinkOffset)
         return false;

      pDDXData += header.mLinkOffset;
      outOfs += header.mLinkOffset;
   }
      
   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUtils::linkTextures
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::linkTextures(
   const uchar* pDDXFirstData, uint DDXFirstDataSize, 
   const uchar* pDDXSecondData, uint DDXSecondDataSize, 
   BAlignedArray<uchar>& outStream)
{
   if ((DDXFirstDataSize < sizeof(BDDXHeader)) || (DDXSecondDataSize < sizeof(BDDXHeader)))
      return false;
      
   const uint firstStreamOfs = outStream.size();
   outStream.pushBack(pDDXFirstData, DDXFirstDataSize);
   
   //const uint secondStreamOfs = outStream.size();
   outStream.pushBack(pDDXSecondData, DDXSecondDataSize);
         
   BDDXHeader header;
   if (!getHeader(&outStream[firstStreamOfs], DDXFirstDataSize, header))
      return false;
      
   header.mLinkOffset = DDXFirstDataSize;
   
   const uint combinedDataSize = DDXFirstDataSize + DDXSecondDataSize;
   if (!setHeader(&outStream[firstStreamOfs], combinedDataSize, header))
      return false;
   
   return true;
}                       

//------------------------------------------------------------------------------------------------------
// BDDXUtils::truncateLinkedTextures
//------------------------------------------------------------------------------------------------------
bool BDDXUtils::truncateLinkedTextures(uchar* pDDXData, uint DDXDataSize)
{
   BDDXHeader header;
   
   if (!getHeader(pDDXData, DDXDataSize, header))
      return false;
      
   header.mLinkOffset = 0;
   
   if (!setHeader(pDDXData, DDXDataSize, header))
      return false;
   
   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXPacker::BDDXPacker
//------------------------------------------------------------------------------------------------------
BDDXPacker::BDDXPacker()
{
}

//------------------------------------------------------------------------------------------------------
// BDDXPacker::swizzleAR
//------------------------------------------------------------------------------------------------------
void BDDXUtils::swizzleAR(BRGBAImage& dst, const BRGBAImage& src)
{
   if (&dst != &src)
      dst.setSize(src.getWidth(), src.getHeight());
      
   for (uint y = 0; y < src.getHeight(); y++)
   {
      for (uint x = 0; x < src.getWidth(); x++)
      {
         const BRGBAColor& color(src(x, y));
         dst(x, y) = BRGBAColor(color.a, color.g, color.b, color.r);
      }
   }               
}

//------------------------------------------------------------------------------------------------------
// BDDXPacker::renormalizeImage
//------------------------------------------------------------------------------------------------------
void BDDXPacker::renormalizeImage(BRGBAImage& dst, const BRGBAImage& src)
{
   dst.setSize(src.getWidth(), src.getHeight());
   
   for (uint y = 0; y < src.getHeight(); y++)
   {
      for (uint x = 0; x < src.getWidth(); x++)
      {
         BRGBAColor color(src(x, y));
                           
         float r = (color.r - 128) / 127.0f;
         float g = (color.g - 128) / 127.0f;
         float b = (color.b - 128) / 127.0f;
         
         float l = sqrt(r*r+g*g+b*b);
         if (l != 0.0f)
         {
            l = 1.0f / l;
            r *= l;
            g *= l;
            b *= l;
         }      
         
         color.r = (uchar)Math::Clamp(128.0f + Math::Sign(r) * floor(.5f + fabs(r * 127.0f)), 0.0f, 255.0f);
         color.g = (uchar)Math::Clamp(128.0f + Math::Sign(g) * floor(.5f + fabs(g * 127.0f)), 0.0f, 255.0f);
         color.b = (uchar)Math::Clamp(128.0f + Math::Sign(b) * floor(.5f + fabs(b * 127.0f)), 0.0f, 255.0f);
         
         if ((color.r == 128) && (color.g == 128))
         {
            if (color.b < 128)
               color.b = 1;
            else
               color.b = 255;
         }
         
         dst(x, y) = color;
      }
   }
}

//------------------------------------------------------------------------------------------------------
// BDDXPacker::prepareImage
//------------------------------------------------------------------------------------------------------
void BDDXPacker::prepareImage(BRGBAImage& dst, const BRGBAImage& src, const BDDXTextureInfo& textureInfo, const BDDXPackParams& options, bool mipFlag, eDDXDataFormat format)
{
   const BRGBAImage* pImage = &src;  

   BRGBAImage temp;

   if (!textureInfo.mHasAlpha)
   {
      temp.setSize(pImage->getWidth(), pImage->getHeight());

      for (uint y = 0; y < pImage->getHeight(); y++)
      {
         for (uint x = 0; x < pImage->getWidth(); x++)
         {
            temp(x, y) = (*pImage)(x, y);
            temp(x, y).a = 255;
         }
      }

      pImage = &temp;      
   }

   if ( 
         (options.mPackerFlags & BDDXPackParams::cRenormalize) || 
         (format == cDDXDataFormatDXT5N) || 
         ((textureInfo.mResourceType == cDDXResourceTypeNormalMap) && (mipFlag)) 
      )
   {
      renormalizeImage(temp, *pImage);         
      pImage = &temp;
   }
   
   bool padImage = false;
   switch (format)
   {
      case cDDXDataFormatDXT1:
      case cDDXDataFormatDXT3:
      case cDDXDataFormatDXT5:
      case cDDXDataFormatDXT5Y:
      case cDDXDataFormatDXT5N:
      case cDDXDataFormatDXTM:
      {
         padImage = true;
         break;
      }
   }

   BRGBAImage paddedImage;
   if ((padImage) && ((pImage->getWidth() < 4) || (pImage->getHeight() < 4)))
   {
      paddedImage.setSize((pImage->getWidth() + 3) & ~3, (pImage->getHeight() + 3) & ~3);

      for (uint y = 0; y < paddedImage.getHeight(); y++)
      {
         uint sy = y;
         if (sy >= pImage->getHeight()) sy = pImage->getHeight() - 1;               
         for (uint x = 0; x < paddedImage.getWidth(); x++)
         {
            uint sx = x;
            if (sx >= pImage->getWidth()) sx = pImage->getWidth() - 1;               
            paddedImage(x, y) = (*pImage)(sx, sy);
         }
      }

      pImage = &paddedImage;
   }
   
   dst = *pImage;
}

//------------------------------------------------------------------------------------------------------
// BDDXPacker::compressImage
//------------------------------------------------------------------------------------------------------
bool BDDXPacker::compressImage(
   const BRGBAImage& image,
   const BDDXTextureInfo& textureInfo,
   const BDDXPackParams& options,
   bool mipFlag,
   BAlignedArray<uchar>& stream)
{
   const eDDXDataFormat format = mipFlag ? options.mMipDataFormat : options.mDataFormat;

   BRGBAImage imageToCompress;
   BRGBAImage* pImage = &imageToCompress;
   
   prepareImage(imageToCompress, image, textureInfo, options, mipFlag, format);
      
   const uint streamOfs = stream.size();
   
   bool storeCompressedSize = false;
   BAlignedArray<uchar> dataToDeflate;
      
   switch (format)
   {
      case cDDXDataFormatA8B8G8R8:
      {
         dataToDeflate.reserve(pImage->getWidth() * pImage->getHeight() * sizeof(DWORD));

         // write a8 b8 g8 r8, 3,2,1,0
         for (uint y = 0; y < pImage->getHeight(); y++)
         {
            for (uint x = 0; x < pImage->getWidth(); x++)
            {
               dataToDeflate.pushBack((*pImage)(x, y).r);
               dataToDeflate.pushBack((*pImage)(x, y).g);
               dataToDeflate.pushBack((*pImage)(x, y).b);
               dataToDeflate.pushBack((*pImage)(x, y).a);
            }
         }
                  
         break;
      }
      case cDDXDataFormatA8R8G8B8:
      {
         dataToDeflate.reserve(pImage->getWidth() * pImage->getHeight() * sizeof(DWORD));
         
         // write a8 r8 g8 b8, 3,2,1,0
         for (uint y = 0; y < pImage->getHeight(); y++)
         {
            for (uint x = 0; x < pImage->getWidth(); x++)
            {
               dataToDeflate.pushBack((*pImage)(x, y).b);
               dataToDeflate.pushBack((*pImage)(x, y).g);
               dataToDeflate.pushBack((*pImage)(x, y).r);
               dataToDeflate.pushBack((*pImage)(x, y).a);
            }
         }
         
         break;
      }
      case cDDXDataFormatA8:
      {
         dataToDeflate.reserve(pImage->getWidth() * pImage->getHeight());

         for (uint y = 0; y < pImage->getHeight(); y++)
            for (uint x = 0; x < pImage->getWidth(); x++)
               dataToDeflate.pushBack((*pImage)(x, y).a);

         break;
      }
      case cDDXDataFormatDXT1:
      case cDDXDataFormatDXT3:
      case cDDXDataFormatDXT5:
      case cDDXDataFormatDXT5N:
      case cDDXDataFormatDXT5Y:
      {
         BDXTFormat dxtFormat = cDXT1;
         
         if (format == cDDXDataFormatDXT1)
         {
            if (textureInfo.mHasAlpha)
               dxtFormat = cDXT1A;
         }
         else if (format == cDDXDataFormatDXT3)
            dxtFormat = cDXT3;
         else
            dxtFormat = cDXT5;

         eDXTQuality dxtQuality = cDXTQualityNormal;
         if (options.mPackerFlags & BDDXPackParams::cDXTFast) 
            dxtQuality = cDXTQualityLowest;
         else if (options.mPackerFlags & BDDXPackParams::cDXTBest) 
            dxtQuality = cDXTQualityBest;
            
         bool perceptual = false;
         bool dithering = false;
         if ((format != cDDXDataFormatDXT5N) && (format != cDDXDataFormatDXT5Y) && (options.mResourceType != cDDXResourceTypeNormalMap)) 
         {
            perceptual = ((options.mPackerFlags & BDDXPackParams::cPerceptual) != 0);
            dithering = ((options.mPackerFlags & BDDXPackParams::cDXTDithering) != 0);
         }
                             
         BRGBAImage temp;
         if (format == cDDXDataFormatDXT5N)
         {   
            BDDXUtils::swizzleAR(temp, *pImage);
            pImage = &temp;
         }
         else if (format == cDDXDataFormatDXT5Y)
         {
            temp.setSize(pImage->getWidth(), pImage->getHeight());            
            
            for (uint y = 0; y < pImage->getHeight(); y++)
            {
               for (uint x = 0; x < pImage->getWidth(); x++)
               {
                  const BRGBAColor& srcColor = (*pImage)(x, y);

                  BRGBAColor16 yCoCg;
                  BColorUtils::RGBToYCoCgR(srcColor, yCoCg);

                  BRGBAColor& dstColor = temp(x, y);
                  dstColor.a = (uchar)yCoCg.r;
                  dstColor.r = (uchar)((srcColor.a * 31 + 127) / 255);
                  dstColor.g = (uchar)Math::Clamp<int>((yCoCg.g / 2) + 125, 0, 255);
                  dstColor.b = (uchar)Math::Clamp<int>((yCoCg.b / 2) + 123, 0, 255);
               }
            }
            
            pImage = &temp;
         }
                           
         BDXTPacker packer;
         if (!packer.pack(*pImage, dxtFormat, dxtQuality, perceptual, dithering, dataToDeflate))
            return false;
            
         break;
      }
      case cDDXDataFormatDXTM:
      {
         stream.resize(streamOfs + 4);
         
         storeCompressedSize = true;
         
         BDXTMunger munger;
         munger;
         
         const DWORD compMethod = mipFlag ? options.mDXTMCompressionMethod : options.mDXTMMipCompressionMethod;
         
         if (compMethod == 0)
         {
            BDXTMunger::BMethod2CompParams compParams;
            
            compParams.mHasAlpha = textureInfo.mHasAlpha;
            compParams.mGreyscale = false;
            compParams.mQuality = mipFlag ? options.mDXTMMipQuality : options.mDXTMQuality;
            compParams.mCodebookSize = mipFlag ? options.mDXTMMipCodebookSize : options.mDXTMCodebookSize;
            compParams.mVirtualCodebook = true;
            compParams.mUseAveDeltaImages = true;            
            compParams.mVisualQuant = false;
                                    
            if (!munger.compressMethod2(*pImage, compParams, stream))
               return false;
         }
         else if (compMethod == 1)
         {
            if (!munger.compressMethod1(
               *pImage, 
               textureInfo.mHasAlpha, 
               mipFlag ? options.mDXTMMipQuality : options.mDXTMQuality, 
               stream))
            {            
               return false;
            }     
         }
         else
         {
            return false;
         }
                  
         break;
      }
      default:
      {
         return false;
      }
   }   

   if (dataToDeflate.size())
   {
      stream.resize(streamOfs + 4);
      
      if (!BDeflateCodec::deflateData(&dataToDeflate[0], dataToDeflate.size(), stream))
         return false;
      
      storeCompressedSize = true;
   }
  
   if (storeCompressedSize)
   {
      if (stream.size() <= streamOfs)
         return false;
         
      uint compressedSize = stream.size() - streamOfs;
                  
      // Store size of compressed data (not including this DWORD) at beginning of stream.
      compressedSize -= 4;
      stream[streamOfs+0] = (uchar)(compressedSize >> 24);
      stream[streamOfs+1] = (uchar)(compressedSize >> 16);
      stream[streamOfs+2] = (uchar)(compressedSize >> 8);
      stream[streamOfs+3] = (uchar)(compressedSize);
   }      
      
   return true;
}   

//------------------------------------------------------------------------------------------------------
// BDDXPacker::packRGBA
//------------------------------------------------------------------------------------------------------ 
bool BDDXPacker::packRGBA(
   const BYTE* pData, const uint dataSize,
   const BDDXTextureInfo& textureInfo,
   const BDDXPackParams& options,
   BAlignedArray<BYTE>& stream)
{
   if (textureInfo.mSizeOfStruct != sizeof(BDDXTextureInfo))
      return false;
      
   if (options.mSizeOfStruct != sizeof(BDDXPackParams))
      return false;
         
   if (textureInfo.mMip0DataFormat != cDDXDataFormatA8B8G8R8)
      return false;
      
   if (textureInfo.mNumMipChainLevels)
   {
      if (textureInfo.mMipChainDataFormat != cDDXDataFormatA8B8G8R8)
         return false;
   }
      
   const uint width = textureInfo.mWidth;
   const uint height = textureInfo.mHeight;
   
   if ((!Math::IsPow2(width)) || (!Math::IsPow2(height)))
      return false;
      
   const uint widthLog2 = Math::iLog2(textureInfo.mWidth);
   const uint heightLog2 = Math::iLog2(textureInfo.mHeight);

   if ((widthLog2 > BDDXHeader::cMaxWidthLog2) || (heightLog2 > BDDXHeader::cMaxHeightLog2))
      return false;
      
   if (options.mResourceType == cDDXResourceTypeCubeMap)      
   {
      // Cubemaps must be square.
      if (widthLog2 != heightLog2)
         return false;
      
      // If they want to save a cubemap, the source texture must be a cubemap.
      if (textureInfo.mResourceType != cDDXResourceTypeCubeMap)
         return false;
   }
   else
   {
      // They don't want to write a cubemap, but that's what they gave us. Bail.
      if (textureInfo.mResourceType == cDDXResourceTypeCubeMap)
         return false;
   }
            
   if ((options.mDataFormat >= cDDXDataFormatMax) || (options.mMipDataFormat >= cDDXDataFormatMax) || (options.mResourceType >= cDDXResourceTypeMax))
      return false;
   
   if ((!pData) || (!dataSize))
      return false;

   const uint maxMipChainLevels = BDDXUtils::calcMaxMipChainLevels(width, height);
   if (textureInfo.mNumMipChainLevels > maxMipChainLevels)
      return false;
                       
   uint numMipChainLevels = textureInfo.mNumMipChainLevels;
   const bool generateMips = (options.mPackerFlags & BDDXPackParams::cGenerateMips) != 0;
   if (generateMips)
      numMipChainLevels = maxMipChainLevels;
   
   numMipChainLevels = Math::Min<uint>(options.mMipChainLevels, numMipChainLevels);
   
   const uint totalMipLevels = 1 + numMipChainLevels;
         
   const uint numFaces = (options.mResourceType == cDDXResourceTypeCubeMap) ? 6 : 1;
   
   const BDDXSerializedTexture srcTexture(pData, dataSize, textureInfo);
   if (srcTexture.getTotalDataSize() > dataSize)
      return false;
         
   BAlignedArray< BAlignedArray<uchar> > faceData[6];
            
   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
   {
      const BRGBAImage mip0Image((BRGBAColor*)srcTexture.getImageData(faceIndex, 0), width, height);
            
      faceData[faceIndex].resize(totalMipLevels);
      
      if (!compressImage(mip0Image, textureInfo, options, false, faceData[faceIndex][0]))
         return false;

      for (uint mipChainIndex = 0; mipChainIndex < numMipChainLevels; mipChainIndex++)
      {     
         uint mipWidth, mipHeight;
         BDDXUtils::calcMipDimension(mipWidth, mipHeight, width, height, mipChainIndex + 1);
            
         BRGBAImage mipImage;          
         
         if (generateMips)                  
         {
            const uint numChannels = textureInfo.mHasAlpha ? 4U : 3U;
            const bool useWrapAddressing = (0 != (options.mPackerFlags & BDDXPackParams::cUseWrapFiltering));
            const bool useSRGBFiltering = (options.mResourceType == cDDXResourceTypeNormalMap) ? false : true;
                                    
            BImageResampler resampler;
            resampler.resample(
               mip0Image, 
               mipImage, 
               mipWidth, mipHeight, 
               numChannels, 
               useWrapAddressing, 
               useSRGBFiltering);
         }
         else
         {
            mipImage.aliasToImage((BRGBAColor*)srcTexture.getImageData(faceIndex, mipChainIndex + 1), mipWidth, mipHeight);
         }  
         
         if (!compressImage(mipImage, textureInfo, options, true, faceData[faceIndex][1 + mipChainIndex]))
            return false;       
      }         
   }      
   
   BDDXHeader header;      
   memset(&header, 0, sizeof(header));   
      
   header.mHeaderSize = sizeof(BDDXHeader);
   header.mCreatorVersion = static_cast<WORD>(BDDXHeader::cDDXVersion);
   header.mMinRequiredVersion = DDX_MIN_REQUIRED_VERSION;
   header.mDimensionPow2[0] = static_cast<BYTE>(widthLog2);
   header.mDimensionPow2[1] = static_cast<BYTE>(heightLog2);
   header.mMipChainSize = static_cast<BYTE>(numMipChainLevels);
   header.mMip0Format = options.mDataFormat;
   header.mMipChainFormat = options.mMipDataFormat;
   header.mResourceType = options.mResourceType;
   header.mFlags = 0;
   
   if (textureInfo.mHasAlpha)
      header.mFlags |= BDDXHeader::cDDXHeaderFlagsHasAlpha;

   const DWORD headerOffset = stream.size();      
   stream.pushBack(reinterpret_cast<const uchar*>(&header), sizeof(BDDXHeader));
         
   //if (header.mMip0DataAdler32 != adler32(&stream[header.mMip0DataOffset], header.mMip0DataSize))
   //   return false;
   
   if (numMipChainLevels)
   {
      header.mMipChainDataOffset = stream.size();
      header.mMipChainDataAdler32 = INIT_ADLER32;
      
      for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)      
      {               
         for (uint mipChainIndex = 0; mipChainIndex < numMipChainLevels; mipChainIndex++)
         {
            if (faceData[faceIndex][mipChainIndex + 1].size())
               stream.pushBack(&faceData[faceIndex][mipChainIndex + 1][0], faceData[faceIndex][mipChainIndex + 1].size());
            
            header.mMipChainDataAdler32 = adler32(&faceData[faceIndex][mipChainIndex + 1][0], faceData[faceIndex][mipChainIndex + 1].size(), header.mMipChainDataAdler32);
            header.mMipChainDataSize += faceData[faceIndex][mipChainIndex + 1].size();
         }
      }
   }
   
   header.mMip0DataOffset = stream.size();
   header.mMip0DataAdler32 = INIT_ADLER32;

   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)      
   {
      if (faceData[faceIndex][0].size())
         stream.pushBack(&faceData[faceIndex][0][0], faceData[faceIndex][0].size());

      header.mMip0DataSize += faceData[faceIndex][0].size();
      header.mMip0DataAdler32 = adler32(&faceData[faceIndex][0][0], faceData[faceIndex][0].size(), header.mMip0DataAdler32);
   }      
   
   header.mHeaderMagic = static_cast<DWORD>(BDDXHeader::cDDXHeaderMagic);
   
   if (!BDDXUtils::setHeader(&stream[headerOffset], stream.size() - headerOffset, header))
      return false;
   
   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXPacker::packNative
//------------------------------------------------------------------------------------------------------ 
bool BDDXPacker::packNative(
   const BYTE* pData, const uint dataSize,
   const BDDXTextureInfo& textureInfo,
   const BDDXPackParams& options,
   BAlignedArray<BYTE>& stream)
{
   if (textureInfo.mSizeOfStruct != sizeof(BDDXTextureInfo))
      return false;

   if (options.mSizeOfStruct != sizeof(BDDXPackParams))
      return false;

   if (textureInfo.mMip0DataFormat != cDDXDataFormatA8B8G8R8)
      return false;

   if (textureInfo.mNumMipChainLevels)
   {
      if (textureInfo.mMipChainDataFormat == cDDXDataFormatInvalid)
         return false;
   }

   const uint width = textureInfo.mWidth;
   const uint height = textureInfo.mHeight;

   if ((!Math::IsPow2(width)) || (!Math::IsPow2(height)))
      return false;

   const uint widthLog2 = Math::iLog2(textureInfo.mWidth);
   const uint heightLog2 = Math::iLog2(textureInfo.mHeight);

   if ((widthLog2 > BDDXHeader::cMaxWidthLog2) || (heightLog2 > BDDXHeader::cMaxHeightLog2))
      return false;

   if (options.mResourceType == cDDXResourceTypeCubeMap)      
   {
      // Cubemaps must be square.
      if (widthLog2 != heightLog2)
         return false;

      // If they want to save a cubemap, the source texture must be a cubemap.
      if (textureInfo.mResourceType != cDDXResourceTypeCubeMap)
         return false;
   }
   else
   {
      // They don't want to write a cubemap, but that's what they gave us. Bail.
      if (textureInfo.mResourceType == cDDXResourceTypeCubeMap)
         return false;
   }

   if ((options.mDataFormat >= cDDXDataFormatMax) || (options.mMipDataFormat >= cDDXDataFormatMax) || (options.mResourceType >= cDDXResourceTypeMax))
      return false;

   if ((!pData) || (!dataSize))
      return false;

   const uint maxMipChainLevels = BDDXUtils::calcMaxMipChainLevels(width, height);
   if (textureInfo.mNumMipChainLevels > maxMipChainLevels)
      return false;

   uint numMipChainLevels = textureInfo.mNumMipChainLevels;
   const bool generateMips = (options.mPackerFlags & BDDXPackParams::cGenerateMips) != 0;
   if (generateMips)
      numMipChainLevels = maxMipChainLevels;

   numMipChainLevels = Math::Min<uint>(options.mMipChainLevels, numMipChainLevels);

   const uint totalMipLevels = 1 + numMipChainLevels;

   const uint numFaces = (options.mResourceType == cDDXResourceTypeCubeMap) ? 6 : 1;

   const BDDXSerializedTexture srcTexture(pData, dataSize, textureInfo);
   if (srcTexture.getTotalDataSize() > dataSize)
      return false;

   BAlignedArray< BAlignedArray<uchar> > faceData[6];

   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
   {
      const BRGBAImage mip0Image((BRGBAColor*)srcTexture.getImageData(faceIndex, 0), width, height);

      faceData[faceIndex].resize(totalMipLevels);

      if (!compressImage(mip0Image, textureInfo, options, false, faceData[faceIndex][0]))
         return false;

      for (uint mipChainIndex = 0; mipChainIndex < numMipChainLevels; mipChainIndex++)
      {     
         uint mipWidth, mipHeight;
         BDDXUtils::calcMipDimension(mipWidth, mipHeight, width, height, mipChainIndex + 1);

         BRGBAImage mipImage;          

         if (generateMips)                  
         {
            const uint numChannels = textureInfo.mHasAlpha ? 4U : 3U;
            const bool useWrapAddressing = (0 != (options.mPackerFlags & BDDXPackParams::cUseWrapFiltering));
            const bool useSRGBFiltering = (options.mResourceType == cDDXResourceTypeNormalMap) ? false : true;

            BImageResampler resampler;
            resampler.resample(
               mip0Image, 
               mipImage, 
               mipWidth, mipHeight, 
               numChannels, 
               useWrapAddressing, 
               useSRGBFiltering);
         }
         else
         {
            mipImage.aliasToImage((BRGBAColor*)srcTexture.getImageData(faceIndex, mipChainIndex + 1), mipWidth, mipHeight);
         }  

         if (!compressImage(mipImage, textureInfo, options, true, faceData[faceIndex][1 + mipChainIndex]))
            return false;       
      }         
   }      

   BDDXHeader header;      
   memset(&header, 0, sizeof(header));   

   header.mHeaderSize = sizeof(BDDXHeader);
   header.mCreatorVersion = static_cast<WORD>(BDDXHeader::cDDXVersion);
   header.mMinRequiredVersion = DDX_MIN_REQUIRED_VERSION;
   header.mDimensionPow2[0] = static_cast<BYTE>(widthLog2);
   header.mDimensionPow2[1] = static_cast<BYTE>(heightLog2);
   header.mMipChainSize = static_cast<BYTE>(numMipChainLevels);
   header.mMip0Format = options.mDataFormat;
   header.mMipChainFormat = options.mMipDataFormat;
   header.mResourceType = options.mResourceType;
   header.mFlags = 0;

   if (textureInfo.mHasAlpha)
      header.mFlags |= BDDXHeader::cDDXHeaderFlagsHasAlpha;

   const DWORD headerOffset = stream.size();      
   stream.pushBack(reinterpret_cast<const uchar*>(&header), sizeof(BDDXHeader));

   //if (header.mMip0DataAdler32 != adler32(&stream[header.mMip0DataOffset], header.mMip0DataSize))
   //   return false;

   if (numMipChainLevels)
   {
      header.mMipChainDataOffset = stream.size();
      header.mMipChainDataAdler32 = INIT_ADLER32;

      for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)      
      {               
         for (uint mipChainIndex = 0; mipChainIndex < numMipChainLevels; mipChainIndex++)
         {
            if (faceData[faceIndex][mipChainIndex + 1].size())
               stream.pushBack(&faceData[faceIndex][mipChainIndex + 1][0], faceData[faceIndex][mipChainIndex + 1].size());

            header.mMipChainDataAdler32 = adler32(&faceData[faceIndex][mipChainIndex + 1][0], faceData[faceIndex][mipChainIndex + 1].size(), header.mMipChainDataAdler32);
            header.mMipChainDataSize += faceData[faceIndex][mipChainIndex + 1].size();
         }
      }
   }

   header.mMip0DataOffset = stream.size();
   header.mMip0DataAdler32 = INIT_ADLER32;

   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)      
   {
      if (faceData[faceIndex][0].size())
         stream.pushBack(&faceData[faceIndex][0][0], faceData[faceIndex][0].size());

      header.mMip0DataSize += faceData[faceIndex][0].size();
      header.mMip0DataAdler32 = adler32(&faceData[faceIndex][0][0], faceData[faceIndex][0].size(), header.mMip0DataAdler32);
   }      

   header.mHeaderMagic = static_cast<DWORD>(BDDXHeader::cDDXHeaderMagic);

   if (!BDDXUtils::setHeader(&stream[headerOffset], stream.size() - headerOffset, header))
      return false;

   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUnpacker::BDDXUnpacker
//------------------------------------------------------------------------------------------------------
BDDXUnpacker::BDDXUnpacker()
{
}

//------------------------------------------------------------------------------------------------------
// BDDXUnpacker::decompressImage
//------------------------------------------------------------------------------------------------------
bool BDDXUnpacker::decompressImage(
   const BYTE*& pData,
   uint& dataSize,
   uint width, uint height, bool mipFlag, uint faceIndex, uint mipIndex,
   const BDDXHeader& header,
   BAlignedArray<uchar>& outStream, 
   BDDXTextureInfo& outTextureInfo)
{      
   // 5 because the image must be at least 1 byte
   if (dataSize < 5)
      return false;

   const uint compSize = (pData[0]<<24) | (pData[1]<<16) | (pData[2]<<8) | pData[3];
   if (compSize < 1)
      return false;
      
   pData += 4;
   dataSize -= 4;
   
   if (compSize > dataSize)
      return false;
      
   const eDDXDataFormat format = mipFlag ? header.mMipChainFormat : header.mMip0Format;
      
   if (format == cDDXDataFormatDXTM)
   {
      BDXTMunger munger;
      munger;
      
      BDXTFormat dxtFormat = cDXT1;
      if (header.mFlags & BDDXHeader::cDDXHeaderFlagsHasAlpha) // munger.imageHasAlphaData(pData, dataSize))
         dxtFormat = cDXT3;
      
      DWORD dstSize;
      if (!BDXTUtils::getSizeOfDXTData(dstSize, dxtFormat, Math::Max(4U, width), Math::Max(4U, height)))
         return false;
         
      const uint outStreamOfs = outStream.size();
      outStream.resize(outStreamOfs + dstSize);
            
      if (!munger.decompress(&outStream[outStreamOfs], dxtFormat, Math::Max(4U, width), Math::Max(4U, height), pData, dataSize))
         return false;

      eDDXDataFormat dataFormat = cDDXDataFormatInvalid;
      // Not every format will really occur here, just planning ahead.
      switch (dxtFormat)
      {
         case cDXT1:
         case cDXT1A:
         {
            dataFormat = cDDXDataFormatDXT1;
            break;
         }
         case cDXT3:
         {
            dataFormat = cDDXDataFormatDXT3;
            break;
         }
         case cDXT5:
         {
            dataFormat = cDDXDataFormatDXT5;
            break;
         }
      }
      
      if ((faceIndex == 0) && (mipIndex == 0))
      {
         if (mipFlag)
            outTextureInfo.mMipChainDataFormat = dataFormat;
         else
            outTextureInfo.mMip0DataFormat = dataFormat;
      }            
      else
      {
         if (mipFlag)
         {
            if (outTextureInfo.mMipChainDataFormat != dataFormat)
               return false;
         }
         else
         {
            if (outTextureInfo.mMip0DataFormat != dataFormat)
               return false;
         }
      }
   }
   else
   {
      BDeflateCodec deflateCodec;
      deflateCodec;
      if (!deflateCodec.inflateData(pData, compSize, outStream))
         return false;
   }

   pData += compSize;
   dataSize -= compSize;
   
   return true;
}
   
//------------------------------------------------------------------------------------------------------
// BDDXUnpacker::unpackToNative
//------------------------------------------------------------------------------------------------------
bool BDDXUnpacker::unpackToNative(
   const BYTE* pDDXData, uint DDXDataSize,
   bool unpackMipChain, uint textureIndex,
   BDDXTextureInfo& outTextureInfo,
   BAlignedArray<BYTE>& outStream)
{
   if (outTextureInfo.mSizeOfStruct != sizeof(BDDXTextureInfo))
      return false;
      
//   const uint origOutStreamSize = outStream.size();
   
   uint imageOfs;
   if (!BDDXUtils::getLinkedTexture(pDDXData, DDXDataSize, textureIndex, imageOfs))
      return false;
      
   pDDXData += imageOfs;
   DDXDataSize -= imageOfs;
                         
   BDDXHeader header;
   if (!BDDXUtils::getHeader(pDDXData, DDXDataSize, header))
      return false;
   
   const uint width = 1 << header.mDimensionPow2[0];
   const uint height = 1 << header.mDimensionPow2[1];
   
   uint numFaces = 1;
   if (header.mResourceType == cDDXResourceTypeCubeMap)
      numFaces = 6;
   
   if ((width > BDDXHeader::cMaxWidth) || (height > BDDXHeader::cMaxHeight))
      return false;
      
   if ((header.mMip0Format >= cDDXDataFormatMax) || (header.mMipChainFormat >= cDDXDataFormatMax))
      return false;

   outTextureInfo.mSizeOfStruct = sizeof(BDDXTextureInfo);
   outTextureInfo.mWidth = width;
   outTextureInfo.mHeight = height;
   outTextureInfo.mNumMipChainLevels = unpackMipChain ? header.mMipChainSize : 0;
   outTextureInfo.mMip0DataFormat = header.mMip0Format;
   outTextureInfo.mMipChainDataFormat = unpackMipChain ? header.mMipChainFormat : cDDXDataFormatInvalid;
   outTextureInfo.mResourceType = header.mResourceType;
   outTextureInfo.mHasAlpha = (0 != (header.mFlags & BDDXHeader::cDDXHeaderFlagsHasAlpha));
         
   if (header.mMip0DataAdler32 != adler32(pDDXData + header.mMip0DataOffset, header.mMip0DataSize))
      return false;
      
   if ((unpackMipChain) && (header.mMipChainSize))
   {
      if (header.mMipChainDataAdler32 != adler32(pDDXData + header.mMipChainDataOffset, header.mMipChainDataSize))
         return false;
   }         
      
   const BYTE* pMip0Data = pDDXData + header.mMip0DataOffset;
   uint mip0DataSize = header.mMip0DataSize;
   
   const BYTE* pMipChainData = pDDXData + header.mMipChainDataOffset;
   uint mipChainDataSize = header.mMipChainDataSize;
   
   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
   {
      if (!decompressImage(pMip0Data, mip0DataSize, width, height, false, faceIndex, 0, header, outStream, outTextureInfo))
         return false;
      
      if ((unpackMipChain) && (header.mMipChainSize))
      {
         uint mipWidth = width; 
         uint mipHeight = height;      
         
         for (uint mipIndex = 0; mipIndex < header.mMipChainSize; mipIndex++)
         {
            mipWidth = Math::Max(1U, mipWidth >> 1);
            mipHeight = Math::Max(1U, mipHeight >> 1);
            
            if (!decompressImage(pMipChainData, mipChainDataSize, mipWidth, mipHeight, true, faceIndex, mipIndex, header, outStream, outTextureInfo))
               return false;
         }
      }
   }      
            
   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUnpacker::convertToARGB
//------------------------------------------------------------------------------------------------------
bool BDDXUnpacker::convertToRGBA(const BYTE*& pData, uint& dataSize, uint width, uint height, bool mipFlag, uint faceIndex, uint mipIndex, const BDDXTextureInfo& nativeTextureInfo, BAlignedArray<BYTE>& outStream)
{
   mipIndex;
   faceIndex;
      
   const eDDXDataFormat format = mipFlag ? nativeTextureInfo.mMipChainDataFormat : nativeTextureInfo.mMip0DataFormat;
   
   switch (format)
   {
      case cDDXDataFormatInvalid:
      {
         return false;
      }
      case cDDXDataFormatA8R8G8B8:
      {  
         const uint size = width * height * sizeof(BRGBAColor);
         if (dataSize < size)
            return false;

         outStream.reserve(outStream.size() + size);
         
         for (uint i = 0; i < size; i += 4)
         {
            outStream.pushBack(pData[i+2]);
            outStream.pushBack(pData[i+1]);
            outStream.pushBack(pData[i+0]);
            outStream.pushBack(pData[i+3]);
         }

         pData += size;            
         dataSize -= size;                  
      
         break;
      }
      case cDDXDataFormatA8B8G8R8:
      {
         const uint size = width * height * sizeof(BRGBAColor);
         if (dataSize < size)
            return false;
         
         outStream.pushBack(pData, size);
                  
         pData += size;
         dataSize -= size;
         
         break;
      }
      case cDDXDataFormatA8:                         
      {
         const uint size = width * height;
         if (dataSize < size)
            return false;

         outStream.reserve(outStream.size() + size);
         
         for (uint i = 0; i < size; i++)
         {
            outStream.pushBack(pData[i]);
            outStream.pushBack(pData[i]);
            outStream.pushBack(pData[i]);
            outStream.pushBack(pData[i]);
         }

         pData += size;            
         dataSize -= size;                  
         
         break;
      }
      case cDDXDataFormatDXT1:                              
      case cDDXDataFormatDXT3:                              
      case cDDXDataFormatDXT5:
      case cDDXDataFormatDXT5Y:
      case cDDXDataFormatDXT5N:
      {
         BDXTUnpacker unpacker;
         unpacker;
         
         DWORD srcSize;
         
         BDXTFormat dxtFormat = cDXT1;
         if (format == cDDXDataFormatDXT1)
            dxtFormat = cDXT1;
         else if (format == cDDXDataFormatDXT3)
            dxtFormat = cDXT3;
         else if ((format == cDDXDataFormatDXT5) || (format == cDDXDataFormatDXT5Y) || (format == cDDXDataFormatDXT5N))
            dxtFormat = cDXT5;
         else
            return false;
                  
         if (!BDXTUtils::getSizeOfDXTData(srcSize, dxtFormat, width, height))
            return false;
         
         if (dataSize < srcSize)
            return false;
                        
         const uint dstSize = width * height * sizeof(BRGBAColor);
         
         uint outStreamOfs = outStream.size();
         
         outStream.resize(outStreamOfs + dstSize);
         
         BRGBAImage alias(reinterpret_cast<BRGBAColor*>(&outStream[outStreamOfs]), width, height);
                           
         if ((width < 4) || (height < 4))
         {
            BRGBAImage temp(Math::Max(4U, width), Math::Max(4U, height));
            
            if (!unpacker.unpack(temp, pData, dxtFormat, temp.getWidth(), temp.getHeight()))
               return false;
               
            for (uint y = 0; y < height; y++)
               for (uint x = 0; x < width; x++)
                  alias(x, y) = temp(x, y);
         }
         else
         {
            if (!unpacker.unpack(alias, pData, dxtFormat, width, height))
               return false;
         }               
         
         if (format == cDDXDataFormatDXT5Y)
         {
            for (uint y = 0; y < alias.getHeight(); y++)
            {
               for (uint x = 0; x < alias.getWidth(); x++)
               {
                  BRGBAColor& c = alias(x, y);
                                    
                  BRGBAColor16 yc(c.a, (c.g - 125) * 2, (c.b - 123) * 2, (c.r * 255 + 15) / 31);

                  BColorUtils::YCoCgRToRGB(yc, c);
               }
            }
         }            
         else if (format == cDDXDataFormatDXT5N)
         {
            BDDXUtils::swizzleAR(alias, alias);
         }
      
         pData += srcSize;
         dataSize -= srcSize;
         
         break;
      }
      default:
      {
         return false;
      }
   }
   
   return true;
}

//------------------------------------------------------------------------------------------------------
// BDDXUnpacker::unpackToRGBA
//------------------------------------------------------------------------------------------------------
bool BDDXUnpacker::unpackToRGBA(
   const BYTE* pDDXData, const uint DDXDataSize,
   bool unpackMipChain, uint textureIndex,
   BDDXTextureInfo& outTextureInfo,
   BAlignedArray<BYTE>& outStream)
{
   if (outTextureInfo.mSizeOfStruct != sizeof(BDDXTextureInfo))
      return false;
      
   BDDXTextureInfo nativeTextureInfo;
   BAlignedArray<BYTE> nativeStream;
   
   if (!unpackToNative(
      pDDXData, DDXDataSize,
      unpackMipChain, textureIndex,
      nativeTextureInfo,
      nativeStream))
   {      
      return false;      
   }
   
   const uint numFaces = (nativeTextureInfo.mResourceType == cDDXResourceTypeCubeMap) ? 6 : 1;
   
   BYTE* pSrc = &nativeStream[0];
   uint srcSize = nativeStream.size();
   
   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
   {
      if (!convertToRGBA(pSrc, srcSize, nativeTextureInfo.mWidth, nativeTextureInfo.mHeight, false, faceIndex, 0, nativeTextureInfo, outStream))
         return false;
  
      if (unpackMipChain)
      {
         uint mipWidth = nativeTextureInfo.mWidth; 
         uint mipHeight = nativeTextureInfo.mHeight;      

         for (uint mipIndex = 0; mipIndex < nativeTextureInfo.mNumMipChainLevels; mipIndex++)
         {
            mipWidth = Math::Max(1U, mipWidth >> 1);
            mipHeight = Math::Max(1U, mipHeight >> 1);

            if (!convertToRGBA(pSrc, srcSize, mipWidth, mipHeight, true, faceIndex, mipIndex, nativeTextureInfo, outStream))
               return false;
         }
      }
   }
   
   outTextureInfo = nativeTextureInfo;
   outTextureInfo.mMip0DataFormat = cDDXDataFormatA8B8G8R8;
   outTextureInfo.mMipChainDataFormat = unpackMipChain ? cDDXDataFormatA8B8G8R8 : cDDXDataFormatInvalid;
   outTextureInfo.mOrigMip0DataFormat = nativeTextureInfo.mMip0DataFormat;
   outTextureInfo.mOrigMipChainDataFormat = nativeTextureInfo.mMipChainDataFormat;
         
   return true;
}   

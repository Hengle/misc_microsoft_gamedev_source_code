//============================================================================
//
// File: DDXCodec.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "DDXCodec.h"
#include "resampler.h"
#include "containers\dynamicarray.h"
#include "math\generalvector.h"
#include "math\halfFloat.h"
#include "hash\adler32.h"
#include "compression.h"
#include "colorutils.h"
#include "RGBAImage.h"
#include "imageutils.h"
#include "DXTUtils.h"
#include "JPEGcodec\JPGMain.h"
#include "DXTPacker.h"
#include "DXTUnpacker.h"
#include "bytepacker.h"
#include "DXTMunge.h"
#include "ImageResample.h"

#include "DDXUtils.h"
#include "DDXSerializedTexture.h"
#include "HDRCodec.h"
#include "HDRUtils.h"

#include "DXTQPack.h"
#include "DXTQUnpack.h"

#ifndef XBOX
//   #define USE_SQUISH
#endif

#ifdef USE_SQUISH
   #include "squish\squish.h"
#endif

//------------------------------------------------------------------------------------------------------
// BDDXCodec::unpackSurface
//------------------------------------------------------------------------------------------------------
bool BDDXCodec::unpackSurface(
   const BYTE*& pData, DWORD& dataSize, 
   const DWORD width, const DWORD height, 
   const eDDXDataFormat format,
   const bool unpackToDXT,
   const float hdrScale,
   BByteArray& outStream,
   eDDXDataFormat& outFormat)
{
   unpackToDXT;
   
   BCOMPILETIMEASSERT((sizeof(BDDXDXTQPackParams) & 3) == 0);
   BCOMPILETIMEASSERT((sizeof(BDDXPackParams) & 3) == 0);
   BCOMPILETIMEASSERT((sizeof(BDDXTextureInfo) & 3) == 0);
   BCOMPILETIMEASSERT((sizeof(BDDXDesc) & 3) == 0);
               
   switch (format)
   {
      case cDDXDataFormatInvalid:
      {
         return false;
      }
      case cDDXDataFormatA16B16G16R16F:
      {
         const uint size = width * height * sizeof(uint64);
         if (dataSize < size)
            return false;

         outStream.pushBack(pData, size);

         pData += size;
         dataSize -= size;
         
         outFormat = cDDXDataFormatA16B16G16R16F;
         
         break;
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
         
         outFormat = cDDXDataFormatA8B8G8R8;

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
         
         outFormat = cDDXDataFormatA8B8G8R8;

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
         
         outFormat = cDDXDataFormatA8B8G8R8;         

         break;
      }
      case cDDXDataFormatDXT1:                              
      case cDDXDataFormatDXT3:                              
      case cDDXDataFormatDXT5:
      case cDDXDataFormatDXT5Y:
      case cDDXDataFormatDXT5N:
      case cDDXDataFormatDXT5H:
      case cDDXDataFormatDXN:
      {
         BDXTUnpacker unpacker;
         unpacker;

         DWORD srcSize;

         BDXTFormat dxtFormat = cDXT1;
         if (format == cDDXDataFormatDXT1)
            dxtFormat = cDXT1;
         else if (format == cDDXDataFormatDXT3)
            dxtFormat = cDXT3;
         else if ((format == cDDXDataFormatDXT5) || (format == cDDXDataFormatDXT5Y) || (format == cDDXDataFormatDXT5N) || (format == cDDXDataFormatDXT5H))
            dxtFormat = cDXT5;
         else if (format == cDDXDataFormatDXN)
            dxtFormat = cDXN;
         else
            return false;

         if (!BDXTUtils::getSizeOfDXTData(srcSize, dxtFormat, width, height))
            return false;

         if (dataSize < srcSize)
            return false;

         if (format == cDDXDataFormatDXT5H)
         {
            BRGBAFImage floatImage;
            BHDRCodec::unpackDXT5HToFloatImage(BConstDataBuffer(pData, dataSize), width, height, hdrScale, floatImage);

            const uint dstSize = width * height * sizeof(uint64);

            uint outStreamOfs = outStream.size();

            outStream.resize(outStreamOfs + dstSize);
            
            BRGBA16Image halfImage((BRGBAColor16*)&outStream[outStreamOfs], width, height);
            
            BHDRUtils::packFloatToHalfImage(floatImage, halfImage, 3, true);
                        
            outFormat = cDDXDataFormatA16B16G16R16F;
         }
         else
         {
            const uint dstSize = width * height * sizeof(BRGBAColor);

            uint outStreamOfs = outStream.size();

            outStream.resize(outStreamOfs + dstSize);

            BRGBAImage alias(reinterpret_cast<BRGBAColor*>(&outStream[outStreamOfs]), width, height);
            
            if (!unpacker.unpack(alias, pData, dxtFormat, width, height))
               return false;
            
            if (format == cDDXDataFormatDXT5Y)
            {
               for (uint y = 0; y < alias.getHeight(); y++)
               {
                  for (uint x = 0; x < alias.getWidth(); x++)
                  {
                     BRGBAColor& c = alias(x, y);

                     BRGBAColor16 yc(c.a, (c.g - 125) * 2, (c.b - 123) * 2, Math::iClampToByte((c.r * 255 + 15) / 31));
                     
                     BColorUtils::YCoCgRToRGB(yc, c);
                  }
               }
            }            
            else if (format == cDDXDataFormatDXT5N)
            {
               BImageUtils::swizzleAR(alias, alias);
            }
            else if (format == cDDXDataFormatDXN)
            {
               for (uint y = 0; y < alias.getHeight(); y++)
               {
                  for (uint x = 0; x < alias.getWidth(); x++)
                  {
                     BRGBAColor& c = alias(x, y);

                     float vx = Math::Clamp((c.r - 128.0f) / 127.0f, -1.0f, 1.0f);
                     float vy = Math::Clamp((c.g - 128.0f) / 127.0f, -1.0f, 1.0f);
                     float vz = sqrt(Math::Clamp(1.0f - vx * vx - vy * vy, 0.0f, 1.0f));
                     
                     vz = vz * 127.0f + 128.0f;
                     
                     int iz = Math::FloatToIntRound(vz);
                     iz = Math::Clamp(iz, 0, 255);
                     
                     c.b = static_cast<uchar>(iz);
                  }
               }
            }
            
            outFormat = cDDXDataFormatA8B8G8R8;
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
// BDDXCodec::unpackTexture
//------------------------------------------------------------------------------------------------------
bool BDDXCodec::unpackTexture(
   const BYTE* pNativeData, const DWORD nativeDataSize,
   const BDDXTextureInfo& nativeTextureInfo,
   BDDXTextureInfo& outTextureInfo,
   const bool unpackMipChain,
   const bool unpackToDXT,
   BByteArray& outStream)
{
   if (outTextureInfo.mSizeOfStruct != sizeof(BDDXTextureInfo))
      return false;
      
   if (nativeTextureInfo.mPlatform != cDDXPlatformNone)
      return false;
      
   eDDXDataFormat outFormat = cDDXDataFormatInvalid;
      
   const BYTE* pSrc = pNativeData;
   DWORD srcSize = nativeDataSize;
   BDDXTextureInfo srcTextureInfo = nativeTextureInfo;
      
   BByteArray uncompressedData;
   if (nativeTextureInfo.mPackType == cDDXTDPTNoMipsCompressed)
   {
      if (!getDDXDataFormatIsDXTQ(nativeTextureInfo.mDataFormat))
         return false;
         
      BDDXTextureInfo uncompressedTextureInfo;
      
      BDXTQUnpack dxtQUnpacker;
      dxtQUnpacker;
      if (!dxtQUnpacker.unpackDXTQToRawDXT(nativeTextureInfo, pNativeData, nativeDataSize, uncompressedTextureInfo, uncompressedData))
         return false;
            
      if (unpackToDXT)            
      {
         outStream.swap(uncompressedData);
         
         outTextureInfo = uncompressedTextureInfo;
                           
         if (!unpackMipChain)
            outTextureInfo.mNumMipChainLevels = 0;
         
         outTextureInfo.mOrigDataFormat = nativeTextureInfo.mDataFormat;
                  
         return true;
      }
      
      pSrc = uncompressedData.getPtr();
      srcSize = uncompressedData.getSize();
      srcTextureInfo = uncompressedTextureInfo;
   }
   
   BDEBUG_ASSERT(srcTextureInfo.mPackType != cDDXTDPTNoMipsCompressed);

   const uint numFaces = getDDXResourceTypeNumFaces(srcTextureInfo.mResourceType);
      
   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
   {
      if (!BDDXCodec::unpackSurface(pSrc, srcSize, srcTextureInfo.mWidth, srcTextureInfo.mHeight, srcTextureInfo.mDataFormat, unpackToDXT, srcTextureInfo.mHDRScale, outStream, outFormat))
         return false;
         
      if (unpackMipChain)
      {
         uint mipWidth = srcTextureInfo.mWidth; 
         uint mipHeight = srcTextureInfo.mHeight;      

         for (uint mipIndex = 0; mipIndex < srcTextureInfo.mNumMipChainLevels; mipIndex++)
         {
            mipWidth = Math::Max(1U, mipWidth >> 1);
            mipHeight = Math::Max(1U, mipHeight >> 1);

            eDDXDataFormat mipOutFormat;
            if (!BDDXCodec::unpackSurface(pSrc, srcSize, mipWidth, mipHeight, srcTextureInfo.mDataFormat, unpackToDXT, srcTextureInfo.mHDRScale, outStream, mipOutFormat))
               return false;
            
            BDEBUG_ASSERT(mipOutFormat == outFormat);                  
         }
      }
   }
   
   outTextureInfo = srcTextureInfo;
   outTextureInfo.mDataFormat = outFormat;
   if (!unpackMipChain)
      outTextureInfo.mNumMipChainLevels = 0;
   
   outTextureInfo.mOrigDataFormat = nativeTextureInfo.mDataFormat;
   outTextureInfo.mPackType = cDDXTDPTMipsRaw;

   return true;
}   

//------------------------------------------------------------------------------------------------------
// BDDXCodec::prepareImage
//------------------------------------------------------------------------------------------------------
void BDDXCodec::prepareImage(
   BRGBAImage& dst, 
   const BRGBAImage& src, 
   const bool hasAlpha,    
   const eDDXResourceType resourceType,
   const eDDXDataFormat format,
   const BDDXPackParams::eFlags packFlags)
{
   resourceType;
   
   const BRGBAImage* pImage = &src;  

   BRGBAImage temp;

   if (!hasAlpha)
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

   // rg [12/31/06] - Renormalizing the mips doesn't seem to be the best idea - it introduces high freq. artifacts into the mipmaps.
   // ddxconv no longer enables this option.
   if (packFlags & BDDXPackParams::cRenormalize)
   {
      BImageUtils::renormalizeImage(temp, *pImage, true);         
      pImage = &temp;
   }

   const bool padImage = getDDXDataFormatNeedsDXTPadding(format);

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
// BDDXCodec::packSurface32
//------------------------------------------------------------------------------------------------------
bool BDDXCodec::packSurface32(
   const BYTE* pData, const DWORD dataSize, 
   const DWORD width, const DWORD height, 
   const bool hasAlpha,    
   const eDDXResourceType resourceType,
   const eDDXDataFormat ddxFormat,
   const BDDXPackParams::eFlags packFlags,
   BByteArray& outStream)
{   
   dataSize;
      
   BRGBAImage image((BRGBAColor*)pData, width, height);

   BRGBAImage imageToCompress;
   BRGBAImage* pImage = &imageToCompress;

   prepareImage(imageToCompress, image, hasAlpha, resourceType, ddxFormat, packFlags);

   const uint streamOfs = outStream.size();

   bool storeCompressedSize = false;

   switch (ddxFormat)
   { 
      case cDDXDataFormatDXT5H:
      {
         float gammaTab[256];         
         for (uint i = 0; i < 256; i++)
            gammaTab[i] = pow(i / 255.0f, 2.2f);
         
         BRGBAFImage floatImage(pImage->getWidth(), pImage->getHeight());
         
         for (uint y = 0; y < pImage->getHeight(); y++)
         {
            for (uint x = 0; x < pImage->getWidth(); x++)
            {
               BRGBAColorF& dst = floatImage(x, y);
               
               for (uint c = 0; c < 3; c++)
                  dst[c] = gammaTab[(*pImage)(x, y)[c]];

               dst[3] = hasAlpha ? ((*pImage)(x, y)[3] / 255.0f) : 1.0f;
            }
         }
         
         BHDRCodec::packFloatToDXT5HImage(floatImage, outStream, 1.0f, cDDX_MAX_HDR_SCALE);
         
         break;
      }
      case cDDXDataFormatA16B16G16R16F:
      {
         ushort convTab[256];
         ushort gammaTab[256];         
         for (uint i = 0; i < 256; i++)
         {
            convTab[i] = HalfFloat::FloatToHalf(i / 255.0f);
            gammaTab[i] = HalfFloat::FloatToHalf(pow(i / 255.0f, 2.2f));
         }
         
         outStream.reserve(pImage->getWidth() * pImage->getHeight() * sizeof(uint64));
         
         for (uint y = 0; y < pImage->getHeight(); y++)
         {
            for (uint x = 0; x < pImage->getWidth(); x++)
            {
               for (uint c = 0; c < 3; c++)
               {
                  ushort v = gammaTab[(*pImage)(x, y)[c]];
                  // Write little endian word
                  outStream.pushBack(static_cast<uchar>(v & 0xFF));
                  outStream.pushBack(static_cast<uchar>(v >> 8));
               }
               
               ushort v = hasAlpha ? convTab[(*pImage)(x, y)[3]] : convTab[255];
               // Write little endian word
               outStream.pushBack(static_cast<uchar>(v & 0xFF));
               outStream.pushBack(static_cast<uchar>(v >> 8));
            }
         }
         
         break;
      }
      case cDDXDataFormatA8B8G8R8:
      {
         outStream.reserve(pImage->getWidth() * pImage->getHeight() * sizeof(DWORD));

         // write a8 b8 g8 r8, 3,2,1,0
         for (uint y = 0; y < pImage->getHeight(); y++)
         {
            for (uint x = 0; x < pImage->getWidth(); x++)
            {
               outStream.pushBack((*pImage)(x, y).r);
               outStream.pushBack((*pImage)(x, y).g);
               outStream.pushBack((*pImage)(x, y).b);
               outStream.pushBack((*pImage)(x, y).a);
            }
         }

         break;
      }
      case cDDXDataFormatA8R8G8B8:
      {
         outStream.reserve(pImage->getWidth() * pImage->getHeight() * sizeof(DWORD));

         // write a8 r8 g8 b8, 3,2,1,0
         for (uint y = 0; y < pImage->getHeight(); y++)
         {
            for (uint x = 0; x < pImage->getWidth(); x++)
            {
               outStream.pushBack((*pImage)(x, y).b);
               outStream.pushBack((*pImage)(x, y).g);
               outStream.pushBack((*pImage)(x, y).r);
               outStream.pushBack((*pImage)(x, y).a);
            }
         }

         break;
      }
      case cDDXDataFormatA8:
      {
         outStream.reserve(pImage->getWidth() * pImage->getHeight());

         for (uint y = 0; y < pImage->getHeight(); y++)
            for (uint x = 0; x < pImage->getWidth(); x++)
               outStream.pushBack((*pImage)(x, y).a);

         break;
      }
      case cDDXDataFormatDXT1:
      case cDDXDataFormatDXT3:
      case cDDXDataFormatDXT5:
      case cDDXDataFormatDXT5N:
      case cDDXDataFormatDXT5Y:
      case cDDXDataFormatDXN:
      {
         BDXTFormat dxtFormat = cDXT1;

         if (ddxFormat == cDDXDataFormatDXT1)
         {
            if (hasAlpha)
               dxtFormat = cDXT1A;
         }
         else if (ddxFormat == cDDXDataFormatDXT3)
            dxtFormat = cDXT3;
         else if (ddxFormat == cDDXDataFormatDXN)
            dxtFormat = cDXN;            
         else
            dxtFormat = cDXT5;

         eDXTQuality dxtQuality = cDXTQualityNormal;
         if (packFlags & BDDXPackParams::cDXTFast) 
            dxtQuality = cDXTQualityLowest;
         else if (packFlags & BDDXPackParams::cDXTBest) 
            dxtQuality = cDXTQualityBest;

         bool perceptual = false;
         bool dithering = false;
         if ((ddxFormat != cDDXDataFormatDXT5N) && (ddxFormat != cDDXDataFormatDXT5Y) && (resourceType != cDDXResourceTypeNormalMap)) 
         {
            perceptual = ((packFlags & BDDXPackParams::cPerceptual) != 0);
            dithering = ((packFlags & BDDXPackParams::cDXTDithering) != 0);
         }

         BRGBAImage temp;
         if (ddxFormat == cDDXDataFormatDXT5N)
         {   
            BImageUtils::swizzleAR(temp, *pImage);
            pImage = &temp;
         }
         else if (ddxFormat == cDDXDataFormatDXT5Y)
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
                  // rg [4/22/06] - FIXME: We're truncating alpha to 32 discrete levels, but the DXT5 packer further packs this to only 5 bits. 
                  // The whole process introduces a level shift in alpha.
                  dstColor.r = (uchar)((srcColor.a * 31 + 127) / 255);
                  dstColor.g = (uchar)Math::Clamp<int>((yCoCg.g / 2) + 125, 0, 255);
                  dstColor.b = (uchar)Math::Clamp<int>((yCoCg.b / 2) + 123, 0, 255);
               }
            }

            pImage = &temp;
         }

#ifdef USE_SQUISH 
         { 
            int flags = 0;
            bool unsupported = false;
            switch (dxtFormat)
            {
               case cDXT1: 
                  flags |= squish::kDxt1; 
                  break;
               case cDXT5: 
                  flags |= squish::kDxt5;
                  break;
               case cDXT3:
                  flags |= squish::kDxt3; 
                  break;
               default:
                  unsupported = true;
                  break;
            }
            
            if (unsupported)
            {
               BDXTPacker packer;
               if (!packer.pack(*pImage, dxtFormat, dxtQuality, perceptual, dithering, outStream))
                  return false;
            }
            else
            {
               if (!perceptual)
                  flags |= squish::kColourMetricUniform;
               else
                  flags |= squish::kColourMetricPerceptual;
                     
               const uint bytesPerBlock = ((dxtFormat == cDXT1) || (dxtFormat == cDXT1A)) ? 8 : 16;
               const uint numBlocks = ((pImage->getWidth() + 3) / 4) * ((pImage->getHeight() + 3) / 4);
               uchar* pDst = outStream.enlarge(bytesPerBlock * numBlocks);
               squish::CompressImage( (squish::u8 const*) pImage->getPtr(), pImage->getWidth(), pImage->getHeight(), pDst, flags);
            }               
         }
#else
         BDXTPacker packer;
         if (!packer.pack(*pImage, dxtFormat, dxtQuality, perceptual, dithering, outStream))
            return false;
#endif            

         break;
      }
      default:
      {
         return false;
      }
   }   

   if (storeCompressedSize)
   {
      if (outStream.size() <= streamOfs)
         return false;

      uint compressedSize = outStream.size() - streamOfs;

      // Store size of compressed data (not including this DWORD) at beginning of outStream.
      compressedSize -= 4;
      outStream[streamOfs+0] = (uchar)(compressedSize >> 24);
      outStream[streamOfs+1] = (uchar)(compressedSize >> 16);
      outStream[streamOfs+2] = (uchar)(compressedSize >> 8);
      outStream[streamOfs+3] = (uchar)(compressedSize);
   }      

   return true;
}   

//------------------------------------------------------------------------------------------------------
// BDDXCodec::packSurface64
//------------------------------------------------------------------------------------------------------
bool BDDXCodec::packSurface64(
   const BYTE* pData, const DWORD dataSize, 
   const DWORD width, const DWORD height, 
   const bool hasAlpha,    
   const eDDXResourceType resourceType,
   const eDDXDataFormat ddxFormat,
   const BDDXPackParams::eFlags packFlags,
   const float hdrScale,
   BByteArray& outStream)
{
   hasAlpha;
   resourceType;
   packFlags;
   
   BDEBUG_ASSERT(dataSize >= width * height * sizeof(uint64));
   
   BRGBA16Image halfImage((BRGBAColor16*)pData, width, height);
      
   switch (ddxFormat)
   {
      case cDDXDataFormatA16B16G16R16F:
      {
         if (!hasAlpha)
         {
            BRGBA16Image dstHalfImage((BRGBAColor16*)outStream.enlarge(width * height * sizeof(uint64)), width, height);
            
            ushort one = HalfFloat::FloatToHalfLittleEndian(1.0f);
            
            for (uint y = 0; y < height; y++)
            {
               for (uint x = 0; x < width; x++)
               {
                  dstHalfImage(x, y) = halfImage(x, y);
                  dstHalfImage(x, y)[3] = one;
               }  
            }
         }
         else
         {
            outStream.pushBack(pData, dataSize);
         }
         break;
      }
      case cDDXDataFormatDXT5H:
      {
         BRGBAFImage floatImage;
         
         BHDRUtils::unpackHalfFloatImage(halfImage, floatImage, 3, true);
         
         BHDRCodec::packFloatToDXT5HImage(floatImage, outStream, hdrScale, cDDX_MAX_HDR_SCALE);
               
         break;
      }
      default:
      {
         BRGBAFImage floatImage;

         BHDRUtils::unpackHalfFloatImage(halfImage, floatImage, hasAlpha ? 4 : 3, true);
         
         BRGBAImage image;
         
         BHDRUtils::convertFloatToRGB8Image(floatImage, image);
         
         return packSurface32((const BYTE*)&image(0, 0), width * height * sizeof(BRGBAColor), width, height, hasAlpha, resourceType, ddxFormat, packFlags, outStream);
      }
   }      
   
   return true;
}   

//------------------------------------------------------------------------------------------------------
// BDDXCodec::packSurface
//------------------------------------------------------------------------------------------------------
bool BDDXCodec::packSurface(
   const BYTE* pData, const DWORD dataSize, 
   const DWORD width, const DWORD height, 
   eDDXDataFormat srcFormat,
   const bool hasAlpha,    
   const eDDXResourceType resourceType,
   const eDDXDataFormat ddxFormat,
   const BDDXPackParams::eFlags packFlags,
   const float hdrScale,
   BByteArray& outStream)
{
   switch (srcFormat)
   {
      case cDDXDataFormatA8B8G8R8:
      {
         return packSurface32(pData, dataSize, width, height, hasAlpha, resourceType, ddxFormat, packFlags, outStream);
      }
      case cDDXDataFormatA16B16G16R16F:
      {
         return packSurface64(pData, dataSize, width, height, hasAlpha, resourceType, ddxFormat, packFlags, hdrScale, outStream);
      }
   }
   
   return false;        
}   

//------------------------------------------------------------------------------------------------------
// BDDXCodec::packTexture
//------------------------------------------------------------------------------------------------------ 
bool BDDXCodec::packTexture(
   const BYTE* pData, const DWORD dataSize,
   const BDDXTextureInfo& textureInfo,
   const BDDXPackParams& options,
   BDDXTextureInfo& outTextureInfo,
   BByteArray& stream)
{
   if (textureInfo.mSizeOfStruct != sizeof(BDDXTextureInfo))
      return false;

   if (options.mSizeOfStruct != sizeof(BDDXPackParams))
      return false;

   if ((textureInfo.mDataFormat != cDDXDataFormatA8B8G8R8) && (textureInfo.mDataFormat != cDDXDataFormatA16B16G16R16F))
      return false;
   
   if (textureInfo.mPackType != cDDXTDPTMipsRaw)
      return false;
                  
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
      
   if ((options.mDataFormat == cDDXDataFormatInvalid) || (options.mDataFormat >= cDDXDataFormatMax) || (options.mResourceType >= cDDXResourceTypeMax))
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
      
   const uint numFaces = (options.mResourceType == cDDXResourceTypeCubeMap) ? 6 : 1;
      
   BDDXSerializedTexture srcTexture(pData, dataSize, textureInfo);
   const int srcTextureDataSize = srcTexture.getTotalDataSize();
   if (srcTextureDataSize < 0)
      return false;
   if (static_cast<uint>(srcTextureDataSize) > dataSize)
      return false;

   BRGBAImage tempImage;   
   BByteArray tempImageData;
   
   BRGBAFImage mip0FloatImage[6];
   if (textureInfo.mDataFormat == cDDXDataFormatA16B16G16R16F)
   {
      outTextureInfo.mHDRScale = 0.0f;
      
      for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
      {
         BRGBA16Image halfImage;
         halfImage.aliasToImage((BRGBAColor16*)srcTexture.getSurfaceData(faceIndex), textureInfo.mWidth, textureInfo.mHeight);
                           
         for (uint y = 0; y < textureInfo.mHeight; y++)
            for (uint x = 0; x < textureInfo.mWidth; x++)
               for (uint c = 0; c < 3; c++)
                   outTextureInfo.mHDRScale = Math::Max(outTextureInfo.mHDRScale, HalfFloat::LittleEndianHalfToFloat((ushort)halfImage(x, y)[c]));
                                    
         if (generateMips) 
            BHDRUtils::unpackHalfFloatImage(halfImage, mip0FloatImage[faceIndex]);
      }  
      
      outTextureInfo.mHDRScale = Math::Min(outTextureInfo.mHDRScale, cDDX_MAX_HDR_SCALE);          
   }
   else if (options.mDataFormat == cDDXDataFormatDXT1)
   {
      // If packing to DXT1 (actually DXT1A), set RGB of all transparent pixels to black to prevent mipmap problems.
      
      tempImageData.resize(dataSize);
      memcpy(tempImageData.getPtr(), pData, dataSize);
      
      srcTexture.set(tempImageData.getPtr(), dataSize, textureInfo);
                  
      for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
      {
         for (uint mipIndex = 0; mipIndex <= textureInfo.mNumMipChainLevels; mipIndex++)
         {
            uint mipWidth, mipHeight;
            BDDXUtils::calcMipDimension(mipWidth, mipHeight, textureInfo.mWidth, textureInfo.mHeight, mipIndex);
            
            BRGBAImage image;
            image.aliasToImage((BRGBAColor*)srcTexture.getSurfaceData(faceIndex, mipIndex), mipWidth, mipHeight);
            
            for (uint y = 0; y < mipHeight; y++)
            {
               for (uint x = 0; x < mipWidth; x++)
               {
                  if ((textureInfo.mHasAlpha) && ((options.mPackerFlags & BDDXPackParams::cDXT1ABlocks) != 0))
                  {
                     if (image(x, y).a < 128)
                     {
                        image(x, y).r = 0;
                        image(x, y).g = 0;
                        image(x, y).b = 0;
                     }
                  }
                  else
                  {
                     image(x, y).a = 255;
                  }                  
               }
            }
         }            
      }  
   }
   else if ((options.mResourceType == cDDXResourceTypeNormalMap) && (generateMips) && (textureInfo.mNumMipChainLevels == 0))
   {
      const BRGBAImage mip0Image((BRGBAColor*)srcTexture.getSurfaceData(0, 0), width, height);

      tempImage = mip0Image;
      
      BImageUtils::renormalizeImage(tempImage, tempImage, true);         
      
      srcTexture.set((uchar*)tempImage.getPtr(), tempImage.getSizeInBytes(), textureInfo);
   }
      
   eDDXDataFormat packSurfaceFormat = options.mDataFormat;
   
   bool packDXTQ = false;
   
   switch (options.mDataFormat)
   {
      case cDDXDataFormatDXT1Q:
      case cDDXDataFormatDXT5Q:
      case cDDXDataFormatDXT5YQ:
      case cDDXDataFormatDXNQ:
      {
         packSurfaceFormat = cDDXDataFormatA8B8G8R8;
         packDXTQ = true;
         break;
      }
      case cDDXDataFormatDXT5HQ:
      {
         packSurfaceFormat = cDDXDataFormatA16B16G16R16F;
         packDXTQ = true;
         break;
      }
   }
   
   if ((packDXTQ) && (options.mResourceType == cDDXResourceTypeCubeMap))
      return false;
         
   const uint numChannels = textureInfo.mHasAlpha ? 4U : 3U;
   const bool useWrapAddressing = (0 != (options.mPackerFlags & BDDXPackParams::cUseWrapFiltering));
   
   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
   {
      if (!BDDXCodec::packSurface(
         srcTexture.getSurfaceData(faceIndex, 0), (width * height * getDDXDataFormatBitsPerPixel(textureInfo.mDataFormat)) / 8, width, height,
         textureInfo.mDataFormat, 
         textureInfo.mHasAlpha!=FALSE, 
         textureInfo.mResourceType, 
         packSurfaceFormat, 
         static_cast<BDDXPackParams::eFlags>(options.mPackerFlags), 
         outTextureInfo.mHDRScale,
         stream))
      {      
         return false;
      }

      for (uint mipChainIndex = 0; mipChainIndex < numMipChainLevels; mipChainIndex++)
      {     
         uint mipWidth, mipHeight;
         BDDXUtils::calcMipDimension(mipWidth, mipHeight, width, height, mipChainIndex + 1);
                  
         const BYTE* pMipData = NULL;
         uint mipDataSize = 0;
         
         BDDXPackParams::eFlags packerFlags = static_cast<BDDXPackParams::eFlags>(options.mPackerFlags);
         
         BRGBAImage mipImage;          
         BRGBA16Image mipImageHDR;
                                             
         if (textureInfo.mDataFormat == cDDXDataFormatA16B16G16R16F)
         {
            if (generateMips)                  
            {
               BRGBAFImage mipFloatImage;
               
               BImageResampler resampler;
               
               resampler.resample(
                  mip0FloatImage[faceIndex], 
                  mipFloatImage, 
                  mipWidth, mipHeight, 
                  numChannels, 
                  useWrapAddressing, 
                  "gaussian",
                  .75f,
                  0.0f,
                  outTextureInfo.mHDRScale);
               
               mipImageHDR.setSize(mipWidth, mipHeight);
               
               for (uint y = 0; y < mipHeight; y++)
                  for (uint x = 0; x < mipWidth; x++)
                     for (uint c = 0; c < 4; c++)
                        mipImageHDR(x, y)[c] = HalfFloat::FloatToHalfLittleEndian(mipFloatImage(x, y)[c]);
               
               pMipData = reinterpret_cast<const BYTE*>(&mipImageHDR(0, 0));
            }
            else
            {
               pMipData = reinterpret_cast<const BYTE*>(srcTexture.getSurfaceData(faceIndex, mipChainIndex + 1));
            }  
            
            mipDataSize = mipWidth * mipHeight * sizeof(uint64);
         }
         else
         {
            if (generateMips)                  
            {
               const BRGBAImage mip0Image((BRGBAColor*)srcTexture.getSurfaceData(faceIndex, 0), width, height);
               
               BImageResampler resampler;
               
               if (options.mResourceType == cDDXResourceTypeNormalMap)
               {
                  resampler.resample(
                     mip0Image, 
                     mipImage, 
                     mipWidth, mipHeight, 
                     numChannels, 
                     useWrapAddressing, 
                     false, 
                     "gaussian", 
                     .85f);
               }
               else
               {
                  resampler.resample(
                     mip0Image, 
                     mipImage, 
                     mipWidth, mipHeight, 
                     numChannels, 
                     useWrapAddressing, 
                     true);
               }
            }
            else
            {
               mipImage.aliasToImage((BRGBAColor*)srcTexture.getSurfaceData(faceIndex, mipChainIndex + 1), mipWidth, mipHeight);
            }  
            
            pMipData = reinterpret_cast<const BYTE*>(&mipImage(0, 0));
            mipDataSize = mipWidth * mipHeight * sizeof(DWORD);
         }         
         
         if (!BDDXCodec::packSurface(
            pMipData, mipDataSize,
            mipWidth, mipHeight,
            textureInfo.mDataFormat, 
            textureInfo.mHasAlpha != FALSE, 
            textureInfo.mResourceType, 
            packSurfaceFormat, 
            packerFlags, 
            outTextureInfo.mHDRScale,
            stream))
         {      
            return false;
         }
      }         
   }      
         
   outTextureInfo.mWidth               = textureInfo.mWidth;
   outTextureInfo.mHeight              = textureInfo.mHeight;
   outTextureInfo.mHasAlpha            = textureInfo.mHasAlpha;
   outTextureInfo.mNumMipChainLevels   = numMipChainLevels;
   outTextureInfo.mDataFormat          = options.mDataFormat;
   outTextureInfo.mResourceType        = options.mResourceType;
   outTextureInfo.mOrigDataFormat      = textureInfo.mDataFormat;
   outTextureInfo.mPackType            = getDDXDataFormatPackType(outTextureInfo.mDataFormat);
   outTextureInfo.mPlatform            = cDDXPlatformNone;
   
   if (packDXTQ)
   {
      BByteArray srcStream(stream);
      BDXTQPack packer;
      BDDXTextureInfo srcTextureInfo;
      srcTextureInfo.mWidth = textureInfo.mWidth;
      srcTextureInfo.mHeight = textureInfo.mHeight;
      srcTextureInfo.mDataFormat = packSurfaceFormat;
      srcTextureInfo.mNumMipChainLevels = numMipChainLevels;
      srcTextureInfo.mHasAlpha = textureInfo.mHasAlpha;
      if (!packer.packRGBAToDXTQ(srcTextureInfo, srcStream.getPtr(), srcStream.getSizeInBytes(), options, outTextureInfo.mHDRScale, stream))
         return false;
   }
   
   return true;
}


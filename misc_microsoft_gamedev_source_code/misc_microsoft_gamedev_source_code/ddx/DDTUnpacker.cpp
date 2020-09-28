//============================================================================
//
// File: DDTUnpacker.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"

#include "containers\dynamicarray.h"
#include "math\generalvector.h"

#include "compression.h"
#include "colorutils.h"
#include "RGBAImage.h"
#include "DXTUtils.h"

#include "DXTUnpacker.h"

#include "DDXDef.h"
#include "DDXPackParams.h"
#include "DDTUnpacker.h"

namespace
{ 
   #pragma pack(push, dxt, 1)
   
   class BT8ImageInfo
   {
   public:
      DWORD    mNumColors;
      DWORD    mRGB8PaletteOffset;
      DWORD    m565PaletteOffset;
      DWORD    m555PaletteOffset;
      DWORD    m1555PaletteOffset;
      DWORD    m444PaletteOffset;

      void endianSwap(void)
      {
         EndianSwitchWorker(this, this + 1, "iiiiii");
      }
   };

   class BDDTImageEntry
   {
   public:
      DWORD                   mOffsetToImageData;
      DWORD                   mImageDataSize;

      void endianSwap(void)
      {
         EndianSwitchWorker(this, this + 1, "ii");
      }
   };


   class BDDTHeader
   {
   public:
      DWORD    mHeader;
      BYTE     mTextureProperties;     // Cubemap, displacement map, No Alpha Test
      BYTE     mNumAlphaBits;          // 0, 1, or 4 bits of Alpha Data are suported
      BYTE     mStorageFormat;         // Raw32, Raw24, BT8, DX1, DX1+1bitAlpha, DXT3Swizzled
      BYTE     mNumLevels;             // Total number of images, >1 == Mipmap Chain of images
      long     mTextureWidth;          // Width of Image #0 (in chain)
      long     mTextureHeight;         // Height of Image #0 (in chain)


      void endianSwap(void)
      {
         EndianSwitchWorker(this, this + 1, "iccccii");
      }

   };

   #pragma pack(pop, dxt)

   #define TEX_NORMAL            0x00
   #define TEX_DITHER            0x01
   #define TEX_DXT5_NORMALMAP    0x02
   #define TEX_DISPLACEMENT_MAP  0x04
   #define TEX_CUBEMAP           0x08

   #define TII_PALETTE_MASK   0x000000FF

   enum BTextureFormat 
   {
      cTxUnknown = 0, 
      cTxRaw32, 
      cTxRaw24, 
      cTxBT8, 
      cTxDXT1, 
      cTxDXT1plusAlpha, 
      cDXT3Swizzled, 
      cAlphaData, 
      cRealDXT3,
      cRealDXT5,
      cTxV8U8,
      cNumTXConsts
   };
 
} // anonymous namespace

BDDTUnpacker::BDDTUnpacker()
{
}

bool BDDTUnpacker::getDesc(const uchar* pDDTData, uint DDTDataSize, BDDXTextureInfo& textureInfo)
{
   if (DDTDataSize < (sizeof(BDDTHeader) + sizeof(BDDTImageEntry)))
      return false;

   BDDTHeader header = *reinterpret_cast<const BDDTHeader*>(pDDTData);
   if (!cLittleEndianNative)
      header.endianSwap();

#define MAKE_DWORD(a, b, c, d) (DWORD(a) | (DWORD(b) << 8) | (DWORD(c) << 16) | (DWORD(d) << 24))
   if(header.mHeader != MAKE_DWORD('R', 'T', 'S', '3') )
      return false;

   if ((!Math::IsPow2(header.mTextureWidth)) || (!Math::IsPow2(header.mTextureHeight)))
      return false;

   if ((header.mTextureWidth < 1) || (header.mTextureHeight < 1))
      return false;

   if (header.mNumLevels < 1)
      return false;

   const BTextureFormat imageFormat = static_cast<BTextureFormat>(header.mStorageFormat);
   
   const bool cubemapFlag = (0 != (header.mTextureProperties & TEX_CUBEMAP));   

   textureInfo.mSizeOfStruct = sizeof(BDDXTextureInfo);   
   textureInfo.mWidth = header.mTextureWidth;
   textureInfo.mHeight = header.mTextureHeight;
   textureInfo.mResourceType = cubemapFlag ? cDDXResourceTypeCubeMap : cDDXResourceTypeRegularMap;
   textureInfo.mNumMipChainLevels = header.mNumLevels - 1;

   switch (imageFormat)
   {
      case cAlphaData: 
      {
         textureInfo.mOrigDataFormat = cDDXDataFormatA8; 
         textureInfo.mHasAlpha = true; 
         break;
      }
      case cTxBT8:     
      {
         textureInfo.mOrigDataFormat = cDDXDataFormatA8R8G8B8; 
         textureInfo.mHasAlpha = true; 
         break;
      }
      case cTxRaw32:   
      {
         textureInfo.mOrigDataFormat = cDDXDataFormatA8R8G8B8; 
         textureInfo.mHasAlpha = true; 
         break;
      }
      case cTxDXT1:    
      {
         textureInfo.mOrigDataFormat = cDDXDataFormatDXT1; 
         break;
      }
      case cRealDXT3:  
      {
         textureInfo.mOrigDataFormat = cDDXDataFormatDXT3; 
         textureInfo.mHasAlpha = true; 
         break;
      }
      case cRealDXT5:  
      {
         textureInfo.mHasAlpha = true; 

         if (header.mTextureProperties & TEX_DXT5_NORMALMAP)
         {
            textureInfo.mResourceType = cDDXResourceTypeNormalMap;      
            textureInfo.mOrigDataFormat = cDDXDataFormatDXT5N; 
         }
         else
            textureInfo.mOrigDataFormat = cDDXDataFormatDXT5; 

         break;
      }
   }   
   
   textureInfo.mDataFormat = textureInfo.mOrigDataFormat;
      
   return true;
}

bool BDDTUnpacker::unpack(const uchar* pDDTData, uint DDTDataSize, bool convertToABGR, BByteArray& textureData, BDDXTextureInfo& textureInfo)
{
   if (!getDesc(pDDTData, DDTDataSize, textureInfo))
      return false;
      
   BDDTHeader header = *reinterpret_cast<const BDDTHeader*>(pDDTData);
   if (!cLittleEndianNative)
      header.endianSwap();

   const BTextureFormat imageFormat = static_cast<BTextureFormat>(header.mStorageFormat);
   
   if ((imageFormat == cTxBT8) || (imageFormat == cTxRaw32))
      convertToABGR = true;
   
   if (convertToABGR)
   {
      textureInfo.mDataFormat = cDDXDataFormatA8B8G8R8;
   }
                     
   BDDTImageEntry imageEntries[6][14];
   const bool cubemapFlag = (0 != (header.mTextureProperties & TEX_CUBEMAP));   
   const uint numFaces = cubemapFlag ? 6 : 1;
   
   uint curDDTOfs = sizeof(BDDTHeader);
   
   BRGBAColor palette[256];
   Utils::ClearObj(palette);
   if (cTxBT8 == imageFormat)
   {
      if ((curDDTOfs + sizeof(BT8ImageInfo)) > DDTDataSize)
         return false;
               
      BT8ImageInfo paletteHeader = *reinterpret_cast<const BT8ImageInfo*>(&pDDTData[curDDTOfs]);
      if (!cLittleEndianNative)
         paletteHeader.endianSwap();
      
      curDDTOfs += sizeof(BT8ImageInfo);

      if (paletteHeader.m565PaletteOffset == 0)            
         return false;

      if ((paletteHeader.mNumColors < 1) || (paletteHeader.mNumColors > 256))
         return false;

      // Argghhhh 
      const BYTE* pPal565Data = &pDDTData[paletteHeader.m565PaletteOffset];
      
      for (uint palIndex = 0; palIndex < paletteHeader.mNumColors; palIndex++)
      {
         const uint packedColor = pPal565Data[palIndex * 2] | (pPal565Data[palIndex * 2 + 1] << 8);

         palette[palIndex] = BRGBAColor( 
            (((packedColor >> 11) & 31) * 255 + 15) / 31, 
            (((packedColor >>  5) & 63) * 255 + 31) / 63, 
            (((packedColor      ) & 31) * 255 + 15) / 31, 
            255);
      }         
   }
      
   for (uint i = 0; i < header.mNumLevels; i++)        
   {
      for (uint face = 0; face < numFaces; face++)   
      {
         if ((curDDTOfs + sizeof(BDDTImageEntry)) > DDTDataSize)
            return false;
            
         imageEntries[face][i] = *reinterpret_cast<const BDDTImageEntry*>(&pDDTData[curDDTOfs]);
         if (!cLittleEndianNative)  
            imageEntries[face][i].endianSwap();

         curDDTOfs += sizeof(BDDTImageEntry);

         if (imageEntries[face][i].mOffsetToImageData > DDTDataSize)
            return false;
      }
   }    
      
   textureData.resize(0);
   
   if (convertToABGR)
   { 
      for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
      {
         for (uint mipIndex = 0; mipIndex < header.mNumLevels; mipIndex++)
         {
            const uint mipWidth = Math::Max<uint>(1U, textureInfo.mWidth >> mipIndex);
            const uint mipHeight = Math::Max<uint>(1U, textureInfo.mHeight >> mipIndex);
            
            const uint sizeOfImage = sizeof(BRGBAColor) * mipWidth * mipHeight;
            const uint textureDataSize = textureData.size();
            textureData.resize(textureDataSize + sizeOfImage);         
            BRGBAImage aliasedImage(reinterpret_cast<BRGBAColor*>(&textureData[textureDataSize]), mipWidth, mipHeight);
                           
            const uchar* pImageData = &pDDTData[imageEntries[faceIndex][mipIndex].mOffsetToImageData];
            const uint imageDataSize = imageEntries[faceIndex][mipIndex].mImageDataSize;
      
            switch (imageFormat)
            {
               case cTxBT8:
               {
                  if ((imageDataSize != mipWidth * mipHeight))
                     return false;

                  for (uint y = 0; y < mipHeight; y++)
                     for (uint x = 0; x < mipWidth; x++)
                        aliasedImage(x, y) = palette[pImageData[x + y * mipWidth]];
               
                  break;
               }
               case cAlphaData:
               {
                  if ((imageDataSize != mipWidth * mipHeight))
                     return false;
                     
                  for (uint y = 0; y < mipHeight; y++)
                     for (uint x = 0; x < mipWidth; x++)
                        aliasedImage(x, y) = BRGBAColor(255, 255, 255, pImageData[x + y * mipWidth]);
                  
                  break;
               }
               case cTxRaw32:
               {
                  if (imageDataSize != mipWidth * mipHeight * sizeof(BRGBAColor))
                     return false;
                  
                  for (uint y = 0; y < mipHeight; y++)
                     for (uint x = 0; x < mipWidth; x++)
                     {
                        const uchar* pSrc = &pImageData[(x + y * mipWidth) * 4];
                        aliasedImage(x, y) = BRGBAColor(pSrc[2], pSrc[1], pSrc[0], pSrc[3]);
                     }
                                 
                  break;
               }
               case cTxDXT1:
               case cRealDXT3:
               case cRealDXT5:
               {
                  BDXTFormat dxtFormat = cDXT1;
                  switch (imageFormat)
                  {
                     case cRealDXT3: dxtFormat = cDXT3; break;
                     case cRealDXT5: dxtFormat = cDXT5; break;
                  }
                                                  
                  DWORD expectedSize;
                  if (!BDXTUtils::getSizeOfDXTData(expectedSize, dxtFormat, mipWidth, mipHeight))
                     return false;
                  
                  if (expectedSize != imageDataSize)
                     return false;
                     
                  BDXTUnpacker unpacker;
                  unpacker;
                           
                  if (!unpacker.unpack(aliasedImage, pImageData, dxtFormat, mipWidth, mipHeight))
                     return false;
                    
                  if (header.mTextureProperties & TEX_DXT5_NORMALMAP)
                  {
                     for (uint y = 0; y < aliasedImage.getHeight(); y++)
                     {
                        for (uint x = 0; x < aliasedImage.getWidth(); x++)
                        {
                           BRGBAColor& color = aliasedImage(x, y);
                           std::swap(color.a, color.r);
                        }
                     }                     
                  }
                  else if (dxtFormat == cDXT1)
                  {
                     for (uint y = 0; y < aliasedImage.getHeight(); y++)
                     {
                        for (uint x = 0; x < aliasedImage.getWidth(); x++)
                        {
                           const BRGBAColor& color = aliasedImage(x, y);
                           if (color.a < 255)
                           {
                              textureInfo.mHasAlpha = true;
                              break;
                           }
                        }
                        
                        if (textureInfo.mHasAlpha)
                           break;
                     }                     
                  }
                        
                  break;
               }
               default:
               {
                  return false;
               }
            } // format
            
         } // mipIndex
      } // faceIndex         
   }
   else
   {
      // Native data
      for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
      {
         for (uint mipIndex = 0; mipIndex < header.mNumLevels; mipIndex++)
         {
            const uchar* pImageData = &pDDTData[imageEntries[faceIndex][mipIndex].mOffsetToImageData];
            const uint imageDataSize = imageEntries[faceIndex][mipIndex].mImageDataSize;
            
            const uint textureDataOfs = textureData.size();
            textureData.resize(textureDataOfs + imageDataSize);         
            
            memcpy(&textureData[textureDataOfs], pImageData, imageDataSize);
         }
      }               
   }      

   return true;
}

//==============================================================================
//
// File: DDXSerializedTexture.h
//
// Copyright (c) 2006 Ensemble Studios
//
//==============================================================================
#pragma once

// All methods are inlined so clients can use this helper class without linking with the DDX library.
class BDDXSerializedTexture
{
public:
   // pTextureData can be NULL for fixed size texture formats.
   BDDXSerializedTexture(const unsigned char* pTextureData, unsigned int textureDataSize, const BDDXTextureInfo& textureInfo) :
      mpTextureData(pTextureData),
      mTextureDataSize(textureDataSize),
      mpTextureInfo(&textureInfo)
   {
   }
   
   void set(const unsigned char* pTextureData, unsigned int textureDataSize, const BDDXTextureInfo& textureInfo) 
   {
      mpTextureData = pTextureData;
      mTextureDataSize = textureDataSize;
      mpTextureInfo = &textureInfo;
   }
   
   const unsigned char* getTextureData(void) const { return mpTextureData; }
   const unsigned int getTextureDataSize(void) const { return mTextureDataSize; }
   const BDDXTextureInfo& getTextureInfo(void) const { return *mpTextureInfo; }

   unsigned int getNumMipLevels(void) const
   {
      return 1 + mpTextureInfo->mNumMipChainLevels;
   }

   unsigned int getWidth(unsigned int level = 0) const
   {
      return max(1, mpTextureInfo->mWidth >> level);
   }

   unsigned int getHeight(unsigned int level = 0) const
   {
      return max(1, mpTextureInfo->mHeight >> level);
   }
         
   unsigned int getPaddedWidth(unsigned int level = 0) const
   {
      return max(getPadSize(mpTextureInfo->mDataFormat), mpTextureInfo->mWidth >> level);
   }

   unsigned int getPaddedHeight(unsigned int level = 0) const
   {
      return max(getPadSize(mpTextureInfo->mDataFormat), mpTextureInfo->mHeight >> level);
   }
   
   unsigned int getNumFaces(void) const
   {
      return (mpTextureInfo->mResourceType == cDDXResourceTypeCubeMap) ? 6 : 1;
   }
         
   bool isFixedSize(void) const
   {
      return getDDXDataFormatIsFixedSize(mpTextureInfo->mDataFormat);
   }
   
   // Returns data size of a single face, single mip level.
   unsigned int getSurfaceFixedDataSize(unsigned int level = 0) const
   {
      const unsigned int paddedWidth = getPaddedWidth(level);
      const unsigned int paddedHeight = getPaddedHeight(level);
      
      return (paddedWidth * paddedHeight * getDDXDataFormatBitsPerPixel(mpTextureInfo->mDataFormat)) >> 3;
   }
   
   // Returns data size of a single face, single mip level, or -1 on error.
   int getSurfaceDataSize(unsigned int face = 0, unsigned int level = 0) const
   {
      switch (mpTextureInfo->mPackType)
      {
         case cDDXTDPTNoMipsCompressed:
         {
            if ((face == 0) && (level == 0))
               return mTextureDataSize;
            return -1;
         }
         case cDDXTDPTMipsRaw:
         {
            return getSurfaceFixedDataSize(level);
         }
         case cDDXTDPTMipsCompressed:
         {
            unsigned int curOfs = 0;

            for (unsigned int faceIndex = 0; faceIndex < getNumFaces(); faceIndex++)
            {
               for (unsigned int mipIndex = 0; mipIndex < getNumMipLevels(); mipIndex++)
               {
                  unsigned int size;

                  if (!mpTextureData)
                     return -1;

                  if ((curOfs + 4) > mTextureDataSize)
                     return -1;

                  const unsigned char* pData = &mpTextureData[curOfs];
                  size = (pData[0]<<24) | (pData[1]<<16) | (pData[2]<<8) | pData[3];

                  if ((!size) || (size > mTextureDataSize))
                     return -1;

                  size += 4;

                  curOfs += size;

                  if ((face == faceIndex) && (level == mipIndex))
                     return size;
               }
            }         
            break;
         }
         default:
         {
            return -1;
         }
      }
                  
      return -1;
   }
   
   // Calculates the actual total data size of texture, or -1 on error.
   int getTotalDataSize(void) const
   {
      if (mpTextureInfo->mPackType == cDDXTDPTNoMipsCompressed)
         return mTextureDataSize;
      
      unsigned int curOfs = 0;
      unsigned int totalSize = 0;
      
      for (unsigned int faceIndex = 0; faceIndex < getNumFaces(); faceIndex++)
      {
         for (unsigned int mipIndex = 0; mipIndex < getNumMipLevels(); mipIndex++)
         {
            unsigned int size;
            if (isFixedSize())
               size = getSurfaceFixedDataSize(mipIndex);
            else
            {
               if (!mpTextureData)
                  return -1;
               
               if ((curOfs + 4) > mTextureDataSize)
                  return -1;
                     
               const unsigned char* pData = &mpTextureData[curOfs];
               size = (pData[0]<<24) | (pData[1]<<16) | (pData[2]<<8) | pData[3];
               
               if ((!size) || (size > mTextureDataSize))
                  return -1;
                  
               size += 4;
            }
            
            curOfs += size;
            totalSize += size;
         }
      }         
         
      return totalSize;
   }
      
   int getSurfaceOffset(unsigned int faceIndex, unsigned int mipIndex) const
   {
      if (mpTextureInfo->mPackType == cDDXTDPTNoMipsCompressed)
      {
         if ((faceIndex != 0) || (mipIndex != 0))
            return -1;
         return 0;
      }
      
      if (faceIndex >= getNumFaces())
         return -1;

      if (mipIndex >= getNumMipLevels())
         return -1;

      unsigned int curOfs = 0;

      for (unsigned int faceIter = 0; faceIter < getNumFaces(); faceIter++)
      {
         for (unsigned int mipIter = 0; mipIter < getNumMipLevels(); mipIter++)
         {
            if ((faceIndex == faceIter) && (mipIndex == mipIter))
               return curOfs;
            
            unsigned int size;
            if (isFixedSize())
               size = getSurfaceFixedDataSize(mipIter);
            else
            {
               if (!mpTextureData)
                  return -1;

               if ((curOfs + 4) > mTextureDataSize)
                  return -1;

               const unsigned char* pData = &mpTextureData[curOfs];
               size = (pData[0]<<24) | (pData[1]<<16) | (pData[2]<<8) | pData[3];

               if ((!size) || (size > mTextureDataSize))
                  return -1;

               size += 4;
            }

            curOfs += size;
         }
      }         

      return -1;
   }

   const unsigned char* getSurfaceData(unsigned int faceIndex = 0, unsigned int mipIndex = 0) const
   {
      int offset = getSurfaceOffset(faceIndex, mipIndex);
      if ((offset < 0) || (!mpTextureData))
         return NULL;
         
      return mpTextureData + offset;
   }

private:
   const unsigned char* mpTextureData;
   unsigned int mTextureDataSize;
   const BDDXTextureInfo* mpTextureInfo;
      
   static unsigned int getPadSize(eDDXDataFormat format)
   {
      return getDDXDataFormatNeedsDXTPadding(format) ? 4 : 1;
   }
};

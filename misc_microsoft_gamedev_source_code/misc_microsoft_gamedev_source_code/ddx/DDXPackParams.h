// File: DDXPackParams.h
#pragma once

#pragma pack(push)
#pragma pack(4)

// Structs defined here should only consist of DWORD's.

struct BDDXDXTQPackParams
{
   BDDXDXTQPackParams() :
      mQualityFactor(cDefaultQualityFactor)
   {
   }
         
   enum { cDefaultQualityFactor = 3 };
   
   // Q factor, 0=worst, 6=best
   DWORD mQualityFactor;
};   

struct BDDXPackParams
{
   BDDXPackParams() :
      mSizeOfStruct(sizeof(BDDXPackParams)),
      mMipChainLevels(0),
      mDataFormat(cDDXDataFormatA8R8G8B8),
      mPackerFlags(cGenerateMips|cPerceptual),
      mResourceType(cDDXResourceTypeRegularMap)
   {
   }  
   
   DWORD                      mSizeOfStruct;
   
   enum { cDDXMaxMipChainLevels = 13 };
   // The maximum number of mipchain levels to store (independent of where they come from).
   DWORD                      mMipChainLevels;

   eDDXDataFormat             mDataFormat;
   
   enum eFlags
   {
      cUseWrapFiltering = (1<<0),
      cRenormalize      = (1<<1),
      cDXTDithering     = (1<<2),
      cGenerateMips     = (1<<4),
      cPerceptual       = (1<<8),
      cDXTFast          = (1<<9),
      cDXTBest          = (1<<10),
      cDXT1ABlocks      = (1<<11),
      
      cPFForceDWORD = 0xFFFFFFFF
   };
   DWORD                       mPackerFlags;
   
   eDDXResourceType            mResourceType;

   BDDXDXTQPackParams          mDXTQParams;
};

struct BDDXTextureInfo
{
   BDDXTextureInfo() :
      mSizeOfStruct(sizeof(BDDXTextureInfo)),
      mWidth(0),
      mHeight(0),
      mNumMipChainLevels(0),
      mDataFormat(cDDXDataFormatA8R8G8B8),
      mResourceType(cDDXResourceTypeRegularMap),
      mHasAlpha(false),
      mOrigDataFormat(cDDXDataFormatInvalid),
      mPlatform(cDDXPlatformNone),
      mHDRScale(1.0f),
      mPackType(cDDXTDPTMipsRaw)
   {
   }
   
   DWORD mSizeOfStruct;
   DWORD mWidth;
   DWORD mHeight;
   DWORD mNumMipChainLevels;
   eDDXDataFormat mDataFormat;
   eDDXResourceType mResourceType;
   BOOL mHasAlpha;
   eDDXPlatform mPlatform;
   eDDXDataFormat mOrigDataFormat;
   float mHDRScale;
   eDDXTextureDataPackType mPackType;
};

struct BDDXDesc
{
   BDDXDesc() :
      mSizeOfStruct(sizeof(BDDXDesc)),
      mWidth(0),
      mHeight(0),
      mTotalDataSize(0),
      mMipChainSize(0),
      mDataFormat(cDDXDataFormatA8R8G8B8),
      mMip0DataSize(0),
      mMipChainDataSize(0),
      mResourceType(cDDXResourceTypeRegularMap),
      mHeaderFlags(0),
      mCreatorVersion(0),
      mMinRequiredVersion(0),
      mPlatform(cDDXPlatformNone)
   {
   }

   DWORD mSizeOfStruct;
   DWORD mWidth;
   DWORD mHeight;
   DWORD mTotalDataSize;

   DWORD mMipChainSize;

   eDDXDataFormat mDataFormat;
   
   DWORD           mMip0DataSize;
   DWORD           mMipChainDataSize;

   eDDXResourceType mResourceType;
   DWORD mHeaderFlags;

   WORD mCreatorVersion;
   WORD mMinRequiredVersion;
   
   eDDXPlatform mPlatform;
};

#pragma pack(pop)

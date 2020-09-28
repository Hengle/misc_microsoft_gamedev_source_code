//============================================================================
//
// File: DDXDef.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

//-----------------------

enum eDDXDataFormat
{    
   cDDXDataFormatInvalid                  = 0,
   
   cDDXDataFormatA8R8G8B8,     
   cDDXDataFormatA8B8G8R8,
   cDDXDataFormatA8,      
                           
   cDDXDataFormatDXT1,                              //DXT1 or DXT1A
   cDDXDataFormatDXT3,                              //explicit 4-bit alpha
   cDDXDataFormatDXT5,                              //block alpha
   cDDXDataFormatDXT5N,                             //swizzled normal map
   cDDXDataFormatDXT5Y,                             //luma/chroma DXT5, alpha is in red
   cDDXDataFormatDXN,                               //DXN normal map
      
   cDDXDataFormatUnused0,              
   cDDXDataFormatUnused1,
         
   cDDXDataFormatDXT5H,                             //HDR, alpha is intensity
   cDDXDataFormatA16B16G16R16F, 
   
   cDDXDataFormatDXT1Q,
   cDDXDataFormatDXT5Q,
   cDDXDataFormatDXT5HQ,
   cDDXDataFormatDXNQ,
   cDDXDataFormatDXT5YQ,
   
   cDDXDataFormatMax,
   
   cDFForceDWORD              = 0xFFFFFFFF
};

inline const char* getDDXDataFormatString(eDDXDataFormat dataFormat) 
{
   switch (dataFormat)
   {
      case cDDXDataFormatA8B8G8R8:        return "A8B8G8R8";
      case cDDXDataFormatA8R8G8B8:        return "A8R8G8B8";
      case cDDXDataFormatA8:              return "A8";
      case cDDXDataFormatA16B16G16R16F:   return "A16B16G16R16F";
      case cDDXDataFormatDXT1:            return "DXT1";
      case cDDXDataFormatDXT3:            return "DXT3";
      case cDDXDataFormatDXT5:            return "DXT5";
      case cDDXDataFormatDXT5Y:           return "DXT5Y";
      case cDDXDataFormatDXT5N:           return "DXT5N";
      case cDDXDataFormatDXT5H:           return "DXT5H";
      case cDDXDataFormatDXN:             return "DXN";
      case cDDXDataFormatDXT1Q:           return "DXT1Q";
      case cDDXDataFormatDXT5Q:           return "DXT5Q";
      case cDDXDataFormatDXT5HQ:          return "DXT5HQ";
      case cDDXDataFormatDXT5YQ:          return "DXT5YQ";
      case cDDXDataFormatDXNQ:            return "DXNQ";
   }   
   return "Invalid";       
}

// Note: DXT1 formats aren't counted as having an alpha channel!
inline bool getDDXDataFormatHasAlpha(eDDXDataFormat dataFormat) 
{
   switch (dataFormat)
   {
      case cDDXDataFormatA8B8G8R8:        
      case cDDXDataFormatA8R8G8B8:        
      case cDDXDataFormatA8:              
      case cDDXDataFormatA16B16G16R16F:   
      case cDDXDataFormatDXT3:            
      case cDDXDataFormatDXT5:            
      case cDDXDataFormatDXT5Q:           
      case cDDXDataFormatDXT5YQ:
         return true;
   }   
   return false;
}

inline bool getDDXDataFormatNeedsDXTPadding(eDDXDataFormat dataFormat) 
{
   switch (dataFormat)
   {
      case cDDXDataFormatDXT1: 
      case cDDXDataFormatDXT3:
      case cDDXDataFormatDXT5:
      case cDDXDataFormatDXT5Y:
      case cDDXDataFormatDXT5N:
      case cDDXDataFormatDXT5H:
      case cDDXDataFormatDXN:
      case cDDXDataFormatDXT1Q:
      case cDDXDataFormatDXT5Q:
      case cDDXDataFormatDXT5HQ:
      case cDDXDataFormatDXT5YQ:
      case cDDXDataFormatDXNQ:
         return true;
   }   
   return false;
}

inline bool getDDXDataFormatIsDXT(eDDXDataFormat dataFormat) 
{
   return getDDXDataFormatNeedsDXTPadding(dataFormat);
}

// 0 on error (not a DXT format)
inline unsigned int getDDXDataFormatDXTBlockSize(eDDXDataFormat dataFormat) 
{
   if (!getDDXDataFormatIsDXT(dataFormat))
      return 0;

   switch (dataFormat)
   {
      case cDDXDataFormatDXT1: 
      case cDDXDataFormatDXT1Q:   
         return 8;
      case cDDXDataFormatDXT3:
      case cDDXDataFormatDXT5:
      case cDDXDataFormatDXT5Y:
      case cDDXDataFormatDXT5N:
      case cDDXDataFormatDXT5H:
      case cDDXDataFormatDXN:
      case cDDXDataFormatDXT5Q:
      case cDDXDataFormatDXT5HQ:
      case cDDXDataFormatDXT5YQ:
      case cDDXDataFormatDXNQ:
         return 16;
   }   
   
   return 0;
}

inline bool getDDXDataFormatIsHDR(eDDXDataFormat dataFormat)
{
   switch (dataFormat)
   {
      case cDDXDataFormatA16B16G16R16F:
      case cDDXDataFormatDXT5H:
      case cDDXDataFormatDXT5HQ:
         return true;
   }
   return false;
}

inline unsigned int getDDXDataFormatBitsPerPixel(eDDXDataFormat dataFormat) 
{
   switch (dataFormat)
   {
      case cDDXDataFormatA16B16G16R16F:
         return 64U;
      case cDDXDataFormatA8B8G8R8:
      case cDDXDataFormatA8R8G8B8:
         return 32U;
      case cDDXDataFormatA8:
         return 8U;
      case cDDXDataFormatDXT1: 
         return 4U;
      case cDDXDataFormatDXT3:
      case cDDXDataFormatDXT5:
      case cDDXDataFormatDXT5Y:
      case cDDXDataFormatDXT5N:
      case cDDXDataFormatDXT5H:
      case cDDXDataFormatDXN:
         return 8U;
   }   
   return 0U;       
}

enum eDDXTextureDataPackType
{
   cDDXTDPTMipsRaw,
   cDDXTDPTMipsCompressed,
   cDDXTDPTNoMipsCompressed,
};

inline eDDXTextureDataPackType getDDXDataFormatPackType(eDDXDataFormat dataFormat)
{
   switch (dataFormat)
   {
      case cDDXDataFormatDXT1Q:
      case cDDXDataFormatDXT5Q:
      case cDDXDataFormatDXT5HQ:
      case cDDXDataFormatDXT5YQ:
      case cDDXDataFormatDXNQ:
      {
         return cDDXTDPTNoMipsCompressed;
      }
   }

   return cDDXTDPTMipsRaw;
}

inline bool getDDXDataFormatIsDXTQ(eDDXDataFormat dataFormat) 
{
   switch (dataFormat)
   {
      case cDDXDataFormatDXT1Q:
      case cDDXDataFormatDXT5Q:
      case cDDXDataFormatDXT5HQ:
      case cDDXDataFormatDXT5YQ:
      case cDDXDataFormatDXNQ:
         return true;
   }

   return false;
}

inline eDDXDataFormat getDDXDXTQBaseFormat(eDDXDataFormat dataFormat) 
{
   switch (dataFormat)
   {
      case cDDXDataFormatDXT1Q:  return cDDXDataFormatDXT1;
      case cDDXDataFormatDXT5Q:  return cDDXDataFormatDXT5;
      case cDDXDataFormatDXT5HQ: return cDDXDataFormatDXT5H;
      case cDDXDataFormatDXT5YQ: return cDDXDataFormatDXT5Y;
      case cDDXDataFormatDXNQ:   return cDDXDataFormatDXN;
   }

   return cDDXDataFormatInvalid;
}

#ifdef XBOX      
inline eDDXDataFormat getDDXFormatFromD3DFormat(D3DFORMAT dataFormat) 
{
   switch (dataFormat)
   {
      case D3DFMT_DXT1:          return cDDXDataFormatDXT1;
      case D3DFMT_DXT5:          return cDDXDataFormatDXT5;
      case D3DFMT_DXN:           return cDDXDataFormatDXN;
      case D3DFMT_A8R8G8B8:      return cDDXDataFormatA8R8G8B8;
      case D3DFMT_A8B8G8R8:      return cDDXDataFormatA8B8G8R8;
      case D3DFMT_A8:            return cDDXDataFormatA8;
      case D3DFMT_A16B16G16R16F: return cDDXDataFormatA16B16G16R16F;
   }

   return cDDXDataFormatInvalid;
}
#endif      

inline bool getDDXDataFormatIsFixedSize(eDDXDataFormat dataFormat)
{
   switch (dataFormat)
   {
      case cDDXDataFormatDXT1Q:
      case cDDXDataFormatDXT5Q:
      case cDDXDataFormatDXT5HQ:
      case cDDXDataFormatDXT5YQ:
      case cDDXDataFormatDXNQ:
         return false;
   }

   return true;
}

//-----------------------

enum eDDXResourceType
{
   cDDXResourceTypeRegularMap = 0,
   cDDXResourceTypeNormalMap,
   cDDXResourceTypeCubeMap,

   cDDXResourceTypeMax,

   cITForceDWORD = 0xFFFFFFFF
};

inline const char* getDDXResourceTypeString(eDDXResourceType resourceType) 
{
   switch (resourceType)
   {
      case cDDXResourceTypeRegularMap: return "regular map";
      case cDDXResourceTypeNormalMap:  return "normal map";
      case cDDXResourceTypeCubeMap:    return "cubemap";
   }   
   return "Invalid";       
}

inline unsigned int getDDXResourceTypeNumFaces(eDDXResourceType resourceType)
{
   return (resourceType == cDDXResourceTypeCubeMap) ? 6U : 1U;
}

//-----------------------

enum eDDXPlatform
{
   cDDXPlatformNone,
   cDDXPlatformXbox,
   
   cDDXPlatformMax,
   
   cDDXPlatformForceDWORD = 0xFFFFFFFF
};

inline const char* getDDXPlatformString(eDDXPlatform platform)
{
   switch (platform)
   {
      case cDDXPlatformNone: return "None";
      case cDDXPlatformXbox: return "Xbox";
   }
   return "Invalid";
}

//-----------------------

const WORD             cDDX_MIN_REQUIRED_VERSION = 6;
const unsigned int     cDDX_ECF_FILE_ID           = 0x13CF5D01;
const unsigned __int64 cDDX_ECF_HEADER_CHUNK_ID   = 0X1D8828C6ECAF45F2;
const unsigned __int64 cDDX_ECF_MIP0_CHUNK_ID     = 0x3F74B8E87D2B44BF;
const unsigned __int64 cDDX_ECF_MIPCHAIN_CHUNK_ID = 0x46F1FD3F394348B8;
const float            cDDX_MAX_HDR_SCALE         = 64.0f;

#pragma pack(push)
#pragma pack(4)
class BDDXHeader
{
public:
   enum { cDDXVersion = 7 };
                     
   enum { cDDXHeaderMagic = 0xDDBB7738, cDDXInvertedHeaderMagic = 0x3877BBDD };
   enum { cDDXHeaderDWORDsToSkip = 3 };
   
   enum 
   { 
      cMaxWidthLog2 = 13, 
      cMaxHeightLog2 = 13, 
      cMaxMipChainSize = (cMaxWidthLog2 > cMaxHeightLog2) ? cMaxWidthLog2 : cMaxHeightLog2,
      cMaxWidth = 1 << cMaxWidthLog2, 
      cMaxHeight = 1 << cMaxHeightLog2
   };
   
   // If you modify this struct, also modify BDDXUtils::endianSwapHeader!
         
   DWORD                mHeaderMagic;
   DWORD                mHeaderSize;
   DWORD                mHeaderAdler32;
         
   WORD                 mCreatorVersion;
   WORD                 mMinRequiredVersion;
                     
   BYTE                 mDimensionPow2[2];
   BYTE                 mMipChainSize;
   BYTE                 mPlatform;
   
   eDDXDataFormat       mDataFormat;
      
   eDDXResourceType     mResourceType;
   
   enum eFlags
   {
      cDDXHeaderFlagsHasAlpha = (1<<0)
   };
   
   DWORD                mFlags;
   
   float                mHDRScale;
};

#pragma pack(pop)




// File: ImageFileConverter.cpp

#include <string>
#include <d3d9.h>
#include <d3dx9.h>
#include <windows.h>

#include "ecore.h"
#include "ddt.h"

#include "ddxdll.h"
#include "DDXDLLHelper.h"
#include "alignedarray.h"
#include "TextureFileConverter.h"
#include "DDXSerializedTexture.h"

#include "ColorUtils.h"
#include "RGBAImage.h"
#include "ImageUtils.h"

//---------------------------------------------------------------------------------------------------------------
// BTextureFileConverter::BTextureFileConverter
//---------------------------------------------------------------------------------------------------------------
BTextureFileConverter::BTextureFileConverter(LPDIRECT3DDEVICE9 pD3DDev, BDDXDLLHelper& DDXDLLHelper) :
   mpD3DDev(pD3DDev),
   mDDXDLLHelper(DDXDLLHelper)
{
}



//---------------------------------------------------------------------------------------------------------------
// BTextureFileConverter::loadFileData
//---------------------------------------------------------------------------------------------------------------
bool BTextureFileConverter::loadFileData(const char* pFilename, BAlignedArray<uchar>& data)
{
   FILE* pFile = fopen(pFilename, "rb");
   if (!pFile)      
      return false;

   fseek (pFile, 0, SEEK_END);
   const uint dataSize = ftell(pFile);

   data.resize(dataSize);

   if (dataSize)
   {
      fseek(pFile, 0, SEEK_SET);

      if (1 != fread(&data[0], dataSize, 1, pFile))
      {
         fclose(pFile);
         return false;
      }
   }      

   fclose(pFile);
   return true;
}

//---------------------------------------------------------------------------------------------------------------
// BTextureFileConverter::saveD3DXFile
//---------------------------------------------------------------------------------------------------------------
bool BTextureFileConverter::saveD3DXFile(
   const char* pDstFilename,
   const eFileType dstType,
   const uchar* pData,
   const uint dataSize,
   const BDDXTextureInfo& textureInfo,
   const uint maxMipLevelsToSave)
{
   dataSize;
   
   if ((!pData) || (!pDstFilename))
      return false;

   LPDIRECT3DTEXTURE9 pTex = NULL;
   LPDIRECT3DCUBETEXTURE9 pCubeTex = NULL;
      
   D3DXIMAGE_FILEFORMAT fmt;
   switch (dstType)
   {
      case cFileTypeDDS: fmt = D3DXIFF_DDS;  break;
      case cFileTypeBMP: fmt = D3DXIFF_BMP;  break;
      case cFileTypeTGA: fmt = D3DXIFF_TGA;  break;
      case cFileTypePNG: fmt = D3DXIFF_PNG;  break;
      case cFileTypeJPG: fmt = D3DXIFF_JPG;  break;
      default:
         return false;
   };

   BDDXSerializedTexture texture(pData, dataSize, textureInfo);
   
   IDirect3DBaseTexture9* pBaseTex = NULL; 
   
   const bool cubemap = (textureInfo.mResourceType == cDDXResourceTypeCubeMap);
         
   const uint mipLevelsToSave = Math::Min<uint>(maxMipLevelsToSave, textureInfo.mNumMipChainLevels + 1);
   
   D3DFORMAT texFormat;
   if (textureInfo.mNumMipChainLevels)
   {
      if (textureInfo.mMip0DataFormat != textureInfo.mMipChainDataFormat)
         return false;
   }
   
   switch (textureInfo.mMip0DataFormat)
   {
      case cDDXDataFormatA8:
      {
         texFormat = D3DFMT_A8;
         break;
      }
      case cDDXDataFormatA8R8G8B8: 
      {
         if (textureInfo.mHasAlpha)
            texFormat = D3DFMT_A8R8G8B8;
         else
            texFormat = D3DFMT_X8R8G8B8;
         break;
      }
      case cDDXDataFormatA8B8G8R8: 
      {
         if (textureInfo.mHasAlpha)
            texFormat = D3DFMT_A8B8G8R8;
         else
            texFormat = D3DFMT_X8B8G8R8;
         break;
      }
      case cDDXDataFormatDXT1:
      {
         texFormat = D3DFMT_DXT1;
         break;
      }
      case cDDXDataFormatDXT3:  
      {
         texFormat = D3DFMT_DXT3;
         break;
      }
      case cDDXDataFormatDXT5:  
      case cDDXDataFormatDXT5N:  
      case cDDXDataFormatDXT5Y:  
      {
         texFormat = D3DFMT_DXT5;
         break;
      }
      default:
      {
         return false;
      }
   }
   
   HRESULT hres;
   if (cubemap)
   {
      hres = mpD3DDev->CreateCubeTexture(textureInfo.mWidth, mipLevelsToSave, 0, texFormat, D3DPOOL_SYSTEMMEM, &pCubeTex, 0);
      pBaseTex = pCubeTex;
   }
   else   
   {
      hres = mpD3DDev->CreateTexture(textureInfo.mWidth, textureInfo.mHeight, mipLevelsToSave, 0, texFormat, D3DPOOL_SYSTEMMEM, &pTex, 0);
      pBaseTex = pTex;
   }
   
   if (FAILED(hres))
      return false;
                        
   const uint numFaces = cubemap ? 6 : 1;
   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
   {
      for (uint levelIndex = 0; levelIndex < mipLevelsToSave; levelIndex++)
      {
         const uint levelWidth = texture.getWidth(levelIndex);
         const uint levelHeight = texture.getHeight(levelIndex);
         const uchar* pLevelData = texture.getSurfaceData(faceIndex, levelIndex);
         const uint levelDataSize = texture.getSurfaceDataSize(faceIndex, levelIndex);
         
         D3DSURFACE_DESC desc;
         if (cubemap)
            pCubeTex->GetLevelDesc(levelIndex, &desc);
         else
            pTex->GetLevelDesc(levelIndex, &desc);
         
         if ((desc.Width != levelWidth) || (desc.Height != levelHeight))
         {
            pBaseTex->Release();
            return false;
         }
         
         D3DLOCKED_RECT lockedRect;
         if (cubemap)
            hres = pCubeTex->LockRect((D3DCUBEMAP_FACES)faceIndex, levelIndex, &lockedRect,0,0);
         else
            hres = pTex->LockRect(levelIndex, &lockedRect,0,0);
         
         if (FAILED(hres))
         {
            pBaseTex->Release();
            return false;
         }
         
         if ((texFormat == D3DFMT_DXT1) || (texFormat == D3DFMT_DXT3) || (texFormat == D3DFMT_DXT5))
         {
            memcpy(reinterpret_cast<uchar*>(lockedRect.pBits), pLevelData, levelDataSize);   
         }
         else
         {
            for (uint y = 0; y < levelHeight; y++)
               memcpy(reinterpret_cast<uchar*>(lockedRect.pBits) + y * lockedRect.Pitch, pLevelData + y * levelWidth * sizeof(DWORD), levelWidth * sizeof(DWORD));   
         }               
         
         if (cubemap)
            pCubeTex->UnlockRect((D3DCUBEMAP_FACES)faceIndex, levelIndex);
         else
            pTex->UnlockRect(levelIndex);
      }      
   }
   
   hres = D3DXSaveTextureToFile(pDstFilename, fmt, pBaseTex, NULL);
   
   pBaseTex->Release();
         
   return SUCCEEDED(hres);
};

//---------------------------------------------------------------------------------------------------------------
// BTextureFileConverter::loadD3DXFile
//---------------------------------------------------------------------------------------------------------------
bool BTextureFileConverter::loadD3DXFile( 
   const char *pSrcFilename, 
   BAlignedArray<uchar>& textureData,
   BDDXTextureInfo& textureInfo,
   bool D3DXGenerateMips,
   bool convertToABGR)
{
   if (!pSrcFilename)
      return false;
      
   HRESULT hres;
   
   D3DXIMAGE_INFO srcInfo;
   hres = D3DXGetImageInfoFromFile(pSrcFilename, &srcInfo);

   if (FAILED(hres))
      return false;
    
   bool cubemap = false;
      
   if (srcInfo.ResourceType == D3DRTYPE_CUBETEXTURE)
      cubemap = true;
   else if (srcInfo.ResourceType != D3DRTYPE_TEXTURE)
      return false;
   
   bool hasAlpha = false;
   
   switch (srcInfo.Format)
   {
      case D3DFMT_A8R8G8B8: 
      case D3DFMT_A1R5G5B5: 
      case D3DFMT_A4R4G4B4: 
      case D3DFMT_A8: 
      case D3DFMT_A8R3G3B2:
      case D3DFMT_A2B10G10R10: 
      case D3DFMT_A8B8G8R8: 
      case D3DFMT_A2R10G10B10:
      case D3DFMT_A16B16G16R16: 
      case D3DFMT_A8P8:
      case D3DFMT_A8L8:
      case D3DFMT_A4L4:
      case D3DFMT_A32B32G32R32F:
      case D3DFMT_A16B16G16R16F:
      case D3DFMT_A2W10V10U10:
      case D3DFMT_DXT2:
      case D3DFMT_DXT3:
      case D3DFMT_DXT4: 
      case D3DFMT_DXT5: 
         hasAlpha = true;
         break;
   }

   // Try to find the closest DDX format.
   textureInfo.mOrigMip0DataFormat = textureInfo.mOrigMipChainDataFormat = cDDXDataFormatA8R8G8B8;
        
   switch (srcInfo.Format)
   {
      case D3DFMT_A8:   
      {
         textureInfo.mOrigMip0DataFormat = textureInfo.mOrigMipChainDataFormat = cDDXDataFormatA8; 
         break;
      }
      case D3DFMT_DXT1: 
      {
         textureInfo.mOrigMip0DataFormat = textureInfo.mOrigMipChainDataFormat = cDDXDataFormatDXT1; 
         break;
      }
      case D3DFMT_DXT2: 
      case D3DFMT_DXT3: 
      {
         textureInfo.mOrigMip0DataFormat = textureInfo.mOrigMipChainDataFormat = cDDXDataFormatDXT3; 
         break;
      }
      case D3DFMT_DXT4: 
      case D3DFMT_DXT5: 
      {
         textureInfo.mOrigMip0DataFormat = textureInfo.mOrigMipChainDataFormat = cDDXDataFormatDXT5; 
         break;
      }
   }
   
   D3DFORMAT fmt = D3DFMT_A8B8G8R8;
   eDDXDataFormat ddxFmt = cDDXDataFormatA8B8G8R8;
   if (!convertToABGR)
   {
      switch (srcInfo.Format)
      {
         case D3DFMT_A8:   
         {
            fmt = D3DFMT_A8;
            ddxFmt = cDDXDataFormatA8;
            break;
         }
         case D3DFMT_DXT1: 
         {
            fmt = D3DFMT_DXT1;
            ddxFmt = cDDXDataFormatDXT1;
            break;
         }
         case D3DFMT_DXT2: 
         case D3DFMT_DXT3: 
         {
            fmt = D3DFMT_DXT3;
            ddxFmt = cDDXDataFormatDXT3;
            break;
         }
         case D3DFMT_DXT4: 
         case D3DFMT_DXT5: 
         {
            fmt = D3DFMT_DXT5;
            ddxFmt = cDDXDataFormatDXT5;
            break;
         }
      }
   }
   
   LPDIRECT3DTEXTURE9 pTex = NULL;
   LPDIRECT3DCUBETEXTURE9 pCubeTex = NULL;
   IDirect3DBaseTexture9* pBaseTex = NULL;

   printf("D3DX generate mips flag: %i\n", D3DXGenerateMips);
   
   if (cubemap)
   {
      hres = D3DXCreateCubeTextureFromFileEx(
         mpD3DDev, 
         pSrcFilename, 
         D3DX_DEFAULT, // Size
         D3DXGenerateMips ? D3DX_DEFAULT : srcInfo.MipLevels, //D3DX_DEFAULT, // MipLevels
         0, // Usage
         fmt, // Format
         D3DPOOL_SYSTEMMEM, // Pool
         D3DX_DEFAULT,  // Filter
         D3DX_FILTER_SRGB|D3DX_FILTER_TRIANGLE,  // MipFilter
         0,  // Colorkey
         NULL,          // srcInfo
         NULL,          // Pal
         &pCubeTex);
         
      pBaseTex = pCubeTex;         
   }
   else
   {    
      hres = D3DXCreateTextureFromFileEx(
         mpD3DDev, 
         pSrcFilename, 
         D3DX_DEFAULT, // Width
         D3DX_DEFAULT, // Height
         D3DXGenerateMips ? D3DX_DEFAULT : srcInfo.MipLevels, // MipLevels
         0, // Usage
         D3DFMT_A8B8G8R8,  // Format
         D3DPOOL_SYSTEMMEM, // Pool
         D3DX_DEFAULT,     // Filter
         D3DX_FILTER_SRGB|D3DX_FILTER_TRIANGLE, // MipFilter
         0, // Colorkey
         NULL, // pSrcInfo
         NULL, // pPalette
         &pTex); // ppTexture
      pBaseTex = pTex;         
   }         

   if (FAILED(hres))
      return false;

   D3DSURFACE_DESC desc;

   if (cubemap)
      hres = pCubeTex->GetLevelDesc(0, &desc);
   else
      hres = pTex->GetLevelDesc(0, &desc);

   if (FAILED(hres))
   {
      pBaseTex->Release();
      return false;
   }

   textureInfo.mWidth = desc.Width;
   textureInfo.mHeight = desc.Height;
   textureInfo.mMip0DataFormat = ddxFmt;
   textureInfo.mResourceType = cubemap ? cDDXResourceTypeCubeMap : cDDXResourceTypeRegularMap;
   
   if (pBaseTex->GetLevelCount() > 1)
   {
      textureInfo.mMipChainDataFormat = ddxFmt;
      textureInfo.mNumMipChainLevels = pBaseTex->GetLevelCount() - 1;
   }
      
   bool found1BitAlpha = false;
   
   const uint numFaces = cubemap ? 6 : 1;
   
   for (uint faceIndex = 0; faceIndex < numFaces; faceIndex++)
   {
      for (uint levelIndex = 0; levelIndex < pBaseTex->GetLevelCount(); levelIndex++)
      {      
         D3DLOCKED_RECT lockedRect;
         
         if (cubemap)
            hres = pCubeTex->LockRect((D3DCUBEMAP_FACES)faceIndex, levelIndex, &lockedRect, NULL, 0);
         else
            hres = pTex->LockRect(levelIndex, &lockedRect, NULL, 0);
         
         if (FAILED(hres))
         {
            pBaseTex->Release();
            return false;
         }
                  
         const uint minDim = getDDXDataFormatNeedsDXTPadding(ddxFmt) ? 4U : 1U;
         const uint mipWidth = Math::Max<uint>(minDim, textureInfo.mWidth >> levelIndex);
         const uint mipHeight = Math::Max<uint>(minDim, textureInfo.mHeight >> levelIndex);
            
         const uint bytesPerLine = (mipWidth * getDDXDataFormatBitsPerPixel(ddxFmt)) / 8;
         const uint dataSize = (mipWidth * mipHeight * getDDXDataFormatBitsPerPixel(ddxFmt)) / 8;
         
         const uint textureDataOfs = textureData.size();
         textureData.resize(textureData.size() + dataSize);
         
         const uchar* pTextureSrc = reinterpret_cast<const uchar*>(lockedRect.pBits);
         uchar* pTextureDst = &textureData[textureDataOfs];
         
         switch (ddxFmt)
         {
            case cDDXDataFormatDXT1:
            case cDDXDataFormatDXT3:
            case cDDXDataFormatDXT5:
            {
               const uint numCols = mipWidth / 4;
               const uint numRows = mipHeight / 4;
               const uint bytesPerBlock = (ddxFmt == cDDXDataFormatDXT1) ? 8 : 16;
                              
               for (uint y = 0; y < numRows; y++)
               {
                  const uchar* pSrc = pTextureSrc + y * lockedRect.Pitch;
                  const uchar* pDst = pTextureDst + y * numCols * bytesPerBlock;
                  for (uint x = 0; x < numCols; x++)
                  {
                     memcpy(pDst, pSrc, bytesPerBlock);
                                                                                    
                     if ((!found1BitAlpha) && (ddxFmt == cDDXDataFormatDXT1))
                     {
                        const WORD highColor = pDst[0]|(pDst[1]<<8);
                        const WORD lowColor = pDst[2]|(pDst[3]<<8);
                        
                        if (highColor <= lowColor)
                        {
                           // endianness doesn't matter here
                           const uint selectors = *reinterpret_cast<const uint*>(&pDst[4]);
                           for (uint i = 0; i < 16; i++)
                           {
                              if ((selectors & 3) == 3)
                              {
                                 found1BitAlpha = true;
                                 break;
                              }
                              
                              selectors >>= 2;
                           }
                        }
                     }
                     
                     pSrc += bytesPerBlock;
                     pDst += bytesPerBlock;
                  }
               }
                              
               break;
            }
            default:
            {
               for (uint y = 0; y < mipHeight; y++)
               {
                  uchar* pDst = pTextureDst + y * bytesPerLine;
                  memcpy(pDst, pTextureSrc + y * lockedRect.Pitch, bytesPerLine);
                  
                  if ((srcInfo.Format == D3DFMT_DXT1) && (!found1BitAlpha))
                  {
                     for (uint x = 0; x < mipWidth; x++)
                     {
                        if (pDst[x*4+3] < 255)
                        {
                           found1BitAlpha = true;
                           break;
                        }
                     }
                  }
               }
            
               break;
            }
         }
               
         if (cubemap)
            pCubeTex->UnlockRect((D3DCUBEMAP_FACES)faceIndex, levelIndex);
         else
            pTex->UnlockRect(levelIndex);
      }      
   }      

   pBaseTex->Release();
   
   if ((hasAlpha) || ((srcInfo.Format == D3DFMT_DXT1) && (found1BitAlpha)))
      textureInfo.mHasAlpha = true;
   
   return true;
}

//---------------------------------------------------------------------------------------------------------------
// BTextureFileConverter::loadDDTFile
//---------------------------------------------------------------------------------------------------------------
bool BTextureFileConverter::loadDDTFile( 
   const char *pSrcFilename, 
   BAlignedArray<uchar>& textureData,
   BDDXTextureInfo& textureInfo,
   const bool convertToABGR)
{
   BAlignedArray<uchar> DDTData;
   if (!loadFileData(pSrcFilename, DDTData))
      return false;

   //convert the data in mem to RGBA8
   IDDXBuffer* pDataHandle = NULL;

   if (!mDDXDLLHelper.getInterface()->DDTtoRGBA8(&pDataHandle, textureInfo, convertToABGR, &DDTData[0], DDTData.size()))
      return false;

   const uint dataSize = pDataHandle->getSize();

   textureData.resize(dataSize);
   memcpy(&textureData[0], pDataHandle->getPtr(), dataSize);

   pDataHandle->release();

   return true;
}   

//---------------------------------------------------------------------------------------------------------------
// BTextureFileConverter::saveDDXFile
//---------------------------------------------------------------------------------------------------------------
bool BTextureFileConverter::saveDDXFile( 
   const char* pDstFilename, 
   const uchar* pData, 
   const uint dataSize, 
   const BDDXTextureInfo& textureInfo,
   const BDDXPackParams& packParams)
{
#if 0
   printf("DDX Packer Parameters:\n");
   printf("               Filename: %s\n", pDstFilename);
   printf("    Max MipChain Levels: %i\n", packParams.mMipChainLevels);
   printf("            Data Format: %s\n", getDDXDataFormatString(packParams.mDataFormat));
   printf("        Mip Data Format: %s\n", getDDXDataFormatString(packParams.mMipDataFormat));
   printf("          Resource type: %s\n", getDDXResourceTypeString(packParams.mResourceType));

   printf("                  Flags: ");
   if (packParams.mPackerFlags & BDDXPackParams::cUseWrapFiltering) printf("usewrapfiltering ");
   if (packParams.mPackerFlags & BDDXPackParams::cRenormalize) printf("renormalize ");
   if (packParams.mPackerFlags & BDDXPackParams::cDXTDithering) printf("dxtdithering ");
   if (packParams.mPackerFlags & BDDXPackParams::cGenerateMips) printf("generatemips ");
   if (packParams.mPackerFlags & BDDXPackParams::cDXTFast) printf("fastDXT ");
   if (packParams.mPackerFlags & BDDXPackParams::cDXTBest) printf("bestDXT ");
   if (packParams.mPackerFlags & BDDXPackParams::cPerceptual) printf("perceptual ");
   printf("\n");

   printf("           DXTM Quality: %i\n", packParams.mDXTMQuality);
   printf("       DXTM MIP Quality: %i\n", packParams.mDXTMMipQuality);
   printf("     DXTM Codebook Size: %i\n", packParams.mDXTMCodebookSize);
   printf(" DXTM MIP Codebook Size: %i\n", packParams.mDXTMMipCodebookSize);
   printf("DXTM Compression Method: %i\n", packParams.mDXTMCompressionMethod);
#endif
   
   if ((!pDstFilename) || (!pData))
      return false;
      
   if (!Math::IsPow2(textureInfo.mWidth) || !Math::IsPow2(textureInfo.mHeight) || (textureInfo.mWidth < 1) || (textureInfo.mHeight < 1))
      return false;
      
   if (textureInfo.mResourceType == cDDXResourceTypeCubeMap)      
   {
      if (textureInfo.mWidth != textureInfo.mHeight)
         return false;
   }

   DWORD startTime = GetTickCount();
           
   IDDXBuffer* pDataHandle = NULL;
   
   if ( (textureInfo.mOrigMip0DataFormat == cDDXDataFormatA8B8G8R8) &&
        ((textureInfo.mOrigMip0DataFormat == cDDXDataFormatInvalid) || (textureInfo.mOrigMip0DataFormat == cDDXDataFormatA8B8G8R8)) )
   {        
      if (!mDDXDLLHelper.getInterface()->RGBA8toDDX(&pDataHandle, packParams, pData, dataSize, textureInfo))
         return false;
   }
   else
   {
      if (!mDDXDLLHelper.getInterface()->NativeToDDX(&pDataHandle, pData, dataSize, textureInfo))
         return false;
   }           
      
   DWORD endTime = GetTickCount();
   
   printf("Total DDX creation time: %u\n", endTime - startTime);

   FILE* pFile = NULL;
   pFile = fopen(pDstFilename, "wb");
   if (!pFile)  
      return (false);

   const bool ok = (fwrite(pDataHandle->getPtr(), pDataHandle->getSize(), 1, pFile) == 1);
        
   pDataHandle->release();

   if (0 != fclose(pFile))
      return false;

   return ok;
}

//---------------------------------------------------------------------------------------------------------------
// BTextureFileConverter::loadDDXFile
//---------------------------------------------------------------------------------------------------------------
bool BTextureFileConverter::loadDDXFile(
   const char* pSrcFilename, 
   BAlignedArray<uchar>& textureData, BDDXTextureInfo& textureInfo, bool convertToABGR)
{
   BAlignedArray<uchar> DDXData;
   if (!loadFileData(pSrcFilename, DDXData))
      return false;
      
   //convert the data in mem to RGBA8
   IDDXBuffer* pDataHandle = NULL;
   const bool unpackAllMips = true;
   const uint imageIndex = 0;

   if (convertToABGR)   
   {
      if (!mDDXDLLHelper.getInterface()->DDXtoRGBA8(&pDataHandle, textureInfo, &DDXData[0], DDXData.size(), unpackAllMips, imageIndex))
         return false;
   }         
   else
   {
      if (!mDDXDLLHelper.getInterface()->DDXtoNative(&pDataHandle, textureInfo, &DDXData[0], DDXData.size(), unpackAllMips, imageIndex))
         return false;
   }

   const uint dataSize = pDataHandle->getSize();

   textureData.resize(dataSize);
   memcpy(&textureData[0], pDataHandle->getPtr(), dataSize);

   pDataHandle->release();

   return true;
}

#if 0
//---------------------------------------------------------------------------------------------------------------
// BTextureFileConverter::loadFile
//---------------------------------------------------------------------------------------------------------------
bool BTextureFileConverter::loadFile(
   const char* pSrcFilename, const eFileType srcFileType, 
   BAlignedArray<uchar>& textureData, BDDXTextureInfo& textureInfo, bool D3DXGenerateMips, bool convertToABGR)
{
   bool success = false;
   
   switch (srcFileType)
   {
      case cFileTypeDDX:
      {
         success = loadDDXFile(pSrcFilename, textureData, textureInfo, convertToABGR);
         break;
      }
      case cFileTypeDDS:
      case cFileTypeBMP:
      case cFileTypeTGA:
      case cFileTypePNG:
      case cFileTypeJPG:
      {
         success = loadD3DXFile(pSrcFilename, textureData, textureInfo, D3DXGenerateMips);
         break;
      }
      case cFileTypeDDT:
      {
         success = loadDDTFile(pSrcFilename, textureData, textureInfo, convertToABGR);
         break;
      }
      default:
         break;
   };

   return success;
}

//---------------------------------------------------------------------------------------------------------------
// BTextureFileConverter::saveFile
//---------------------------------------------------------------------------------------------------------------
bool BTextureFileConverter::saveFile(
   const char* pDstFilename, 
   const eFileType dstFileType,
   const uchar* pData, const uint dataSize,
   const BDDXTextureInfo& textureInfo,
   const BDDXPackParams& packParams)
{
   switch(dstFileType)
   {
      case cFileTypeDDX:
      {
         
               
         return saveDDXFile(pDstFilename, pData, dataSize, textureInfo, packParams);
      }
      case cFileTypeDDS:
      case cFileTypeBMP:
      case cFileTypeTGA:
      case cFileTypePNG:
      case cFileTypeJPG:
      {
         return saveRGBA8toD3DXFile(pDstFilename, dstFileType, pData, dataSize, textureInfo, packParams.mMipChainLevels + 1);
      }
   };
   
   return false;
};
#endif

//---------------------------------------------------------------------------------------------------------------
// BTextureFileConverter::determineAlpha
//---------------------------------------------------------------------------------------------------------------
bool BTextureFileConverter::determineAlpha(const BAlignedArray<uchar>& textureData, const BDDXTextureInfo& textureInfo)
{
   BDDXSerializedTexture texture(&textureData[0], textureData.size(), textureInfo);
   
   for (uint faceIndex = 0; faceIndex < texture.getNumFaces(); faceIndex++)
   {
      for (uint levelIndex = 0; levelIndex < texture.getNumMipLevels(); levelIndex++)
      {
         const uchar* pImageData = texture.getSurfaceData(faceIndex, levelIndex);
         if (!pImageData)
            break;
            
         const uint levelWidth = texture.getWidth(levelIndex);
         const uint levelHeight = texture.getHeight(levelIndex);
                  
         for (uint y = 0; y < levelHeight; y++)
         {
            uchar* pLevelData = const_cast<uchar*>(pImageData) + y * levelWidth * sizeof(DWORD);
            for (uint x = 0; x < levelWidth; x++)
            {
               if (pLevelData[3] < 255)
               {
                  return true;
               }
               pLevelData += 4;
            }
         }
      }
   }
   
   return false;
}

//---------------------------------------------------------------------------------------------------------------
// BTextureFileConverter::setAlpha
//---------------------------------------------------------------------------------------------------------------
bool BTextureFileConverter::setAlpha(const BAlignedArray<uchar>& textureData, const BDDXTextureInfo& textureInfo, uchar alpha)
{
   BDDXSerializedTexture texture(&textureData[0], textureData.size(), textureInfo);

   for (uint faceIndex = 0; faceIndex < texture.getNumFaces(); faceIndex++)
   {
      for (uint levelIndex = 0; levelIndex < texture.getNumMipLevels(); levelIndex++)
      {
         const uchar* pImageData = texture.getSurfaceData(faceIndex, levelIndex);
         if (!pImageData)
            break;
         
         const uint levelWidth = texture.getWidth(levelIndex);
         const uint levelHeight = texture.getHeight(levelIndex);

         for (uint y = 0; y < levelHeight; y++)
         {
            uchar* pLevelData = const_cast<uchar*>(pImageData) + y * levelWidth * sizeof(DWORD);
            for (uint x = 0; x < levelWidth; x++)
            {
               pLevelData[3] = alpha;
               pLevelData += 4;
            }
         }
      }
   }

   return true;
}

//---------------------------------------------------------------------------------------------------------------
// BTextureFileConverter::convert
//---------------------------------------------------------------------------------------------------------------



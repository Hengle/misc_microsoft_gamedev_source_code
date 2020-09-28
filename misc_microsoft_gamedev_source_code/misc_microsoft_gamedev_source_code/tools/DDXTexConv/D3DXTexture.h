// File: D3DXTexture.h
#pragma once

class BD3DXTexture
{
public:
   BD3DXTexture(LPDIRECT3DDEVICE9 pD3DDev);
   
   static bool isSupportedFormat(eFileType fileType, eDDXDataFormat textureFormat);
   
   bool saveD3DXFile(
      const char* pDstFilename,
      const eFileType dstType,
      const uchar* pData,
      const uint dataSize,
      const BDDXTextureInfo& textureInfo,
      const uint maxMipLevelsToSave);
      
   bool loadD3DXFile( 
      const char *pSrcFilename, 
      BByteArray& textureData,
      BDDXTextureInfo& textureInfo,
      bool D3DXGenerateMips,
      bool convertToABGR);

private:
   LPDIRECT3DDEVICE9 mpD3DDev;
};

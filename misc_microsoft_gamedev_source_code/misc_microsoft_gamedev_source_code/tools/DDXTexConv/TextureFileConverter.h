// File: ImageFileConverter.h
#pragma once

class BTextureFileConverter
{
public:
   
   BTextureFileConverter(LPDIRECT3DDEVICE9 pD3DDev, BDDXDLLHelper& DDXDLLHelper);
   
   // D3DX
      
   
   // DDX 
   bool saveDDXFile( 
      const char* pDstFilename, 
      const uchar* pData, const uint dataSize, 
      const BDDXTextureInfo& textureInfo,
      const BDDXPackParams& packParams);

   bool loadDDXFile(
      const char* pSrcFilename, 
      BAlignedArray<uchar>& textureData, BDDXTextureInfo& textureInfo, bool convertToABGR);

   // DDT
   bool loadDDTFile( 
      const char *pSrcFilename, 
      BAlignedArray<uchar>& textureData,
      BDDXTextureInfo& textureInfo,
      const bool convertToABGR);

   // Helpers
#if 0   
   bool loadFile(
      const char* pSrcFilename, const eFileType srcFileType, 
      BAlignedArray<uchar>& textureData, BDDXTextureInfo& textureInfo, bool D3DXGenerateMips, bool convertToABGR);

   bool saveFile(
      const char* pDstFilename, 
      const eFileType dstFileType,
      const uchar* pData, const uint dataSize,
      const BDDXTextureInfo& textureInfo,
      const BDDXPackParams& packParams);
#endif

   static bool determineAlpha(const BAlignedArray<uchar>& textureData, const BDDXTextureInfo& textureInfo);
   static bool setAlpha(const BAlignedArray<uchar>& textureData, const BDDXTextureInfo& textureInfo, uchar alpha);
      
   

   bool loadFileData(const char* pFilename, BAlignedArray<uchar>& data);

private:
   LPDIRECT3DDEVICE9 mpD3DDev;
   BDDXDLLHelper& mDDXDLLHelper;
};   

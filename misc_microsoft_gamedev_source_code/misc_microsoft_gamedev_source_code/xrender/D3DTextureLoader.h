//============================================================================
//
// File: D3DTextureLoader.h
//
// Copyright (c) 2006-2008, Ensemble Studios
//
//============================================================================
#pragma once

#include "DDXDef.h"
#include "DDXPackParams.h"
#include "D3DTexture.h"

//============================================================================
// class BD3DTextureLoader
//============================================================================
class BD3DTextureLoader
{
   friend class BD3DTexture;
   
private:
   BD3DTextureLoader(const BD3DTextureLoader&);
   BD3DTextureLoader& operator= (const BD3DTextureLoader&);
   
public:
   BD3DTextureLoader();
   ~BD3DTextureLoader();
   
   void clear(bool deleteTexture = true);
   
   void releaseOwnership(void) { clear(false); }
   
   struct BCreateParams
   {
      BCreateParams() :
         mBigEndian(false),
         mTiled(true),
         mArraySize(0),
         mArrayIndex(0),
         mForceSRGBGamma(false),
         mAddUnusedRegionsToHeap(true),
         mLongTermAllocation(false),
         mUsePackedTextureManager(false)
      {
      }
      
      // The name of the manager creating this texture, for statistics only.
      BRenderString           mManager;
      // The name of the texture, for statistics only
      BRenderString           mName;
            
      uchar                   mArraySize;
      uchar                   mArrayIndex;
      
      // mBigEndian and mTiled have no affect on DDX textures -- except for texture arrays, because the texture data must be unpacked/repacked.
      bool                    mBigEndian       : 1;
      bool                    mTiled           : 1;
      
      // If true, the D3D texture fetch constant SignX/Y/Z values will be set to GAMMA. 
      // This will cause the GPU to convert fetched samples to an approximation of linear light. 
      bool                    mForceSRGBGamma  : 1;
      
      // If true, it's OK for unused texture regions will be added to the BXboxTextureManager's heap.
      // If false, the valley allocator will never try to use the unused regions within the returned texture.
      bool                    mAddUnusedRegionsToHeap : 1;
      
      // If true, this tells BXboxTextureHeap that you intend on keeping the texture around for a LONG time, even after scenarios unload.
      // (The particle texture manager does this, for example.)
      bool                    mLongTermAllocation : 1;
      
      // If true the base allocation of mipmapped DDX textures will be created using the packed texture manager.
      // If the packed texture manager can't handle this texture it will be created in the usual manner.
      bool                    mUsePackedTextureManager : 1;
   };

   // To load a texture array, call one of the create() methods with mArraySize equal to the array size, and mArrayIndex equal to the desired index
   // to load the texture info. mArraySize must equal the array's size for each call. The number of mips in the texture must be >= the number of mips
   // in the array. The size of each texture must be equal. 
   bool createFromDDXFileInMemory(const uchar* pData, uint dataLen, const BCreateParams& createParams = BCreateParams());
   bool createFromDDTFileInMemory(const uchar* pData, uint dataLen, const BCreateParams& createParams = BCreateParams());
   bool createFromXPRFileInMemory(const uchar* pData, uint dataLen, const BCreateParams& createParams = BCreateParams());
   
   // Supports Xbox specific data, or any of the DDX fixed formats (except for DXTM/DXTMA).
   bool createFromTextureData(const uchar* pData, uint dataLen, const BDDXTextureInfo& textureInfo, const BCreateParams& createParams = BCreateParams());
      
   bool isValid(void) const { return mD3DTexture.getBaseTexture() != NULL; }
   
   // IMPORTANT: DO NOT retrieve the IDirect3DBaseTexture* from the BD3DTexture, then try to Release() it as you would a normal D3D texture!
   // THE TEXTURES ALLOCATED BY THIS CLASS ARE NOT NECESSARILY CREATED BY D3D. So keep around the BD3DTexture object, or call BD3DTexture::releaseWildTexture() to release
   // textures created by this class.
         BD3DTexture& getD3DTexture(void)       { return mD3DTexture; }
   const BD3DTexture& getD3DTexture(void) const { return mD3DTexture; }
      
   BD3DTextureType getTextureType(void) const { return mD3DTexture.getType(); }
   bool getCubemapFlag(void) const { return cTTCubemap == getTextureType(); }

   IDirect3DBaseTexture9*  getBaseTexture(void) const    { return mD3DTexture.getBaseTexture(); }
   IDirect3DTexture9*      getTexture(void) const        { return mD3DTexture.getTexture(); }
   IDirect3DCubeTexture9*  getCubeTexture(void) const    { return mD3DTexture.getCubeTexture(); }
   IDirect3DArrayTexture9* getArrayTexture(void) const   { return mD3DTexture.getArrayTexture(); }
      
   // Asserts if the texture is not valid
   void getDesc(D3DSURFACE_DESC& desc) const;
   
   D3DFORMAT getFormat(void) const;
   
   float getHDRScale(void) const { return mHDRScale; } 
   
   eDDXDataFormat getDDXDataFormat(void) const { return mDDXDataFormat; }
   
   // Returns 0 if the texture is not valid.
   uint getWidth(void) const;
   uint getHeight(void) const;
   uint getLevels(void) const;
         
   // Returns 0 if the texture is not valid, or if it's not a texture array.
   uint getArraySize(void) const;   
   
   // Call this instead of getHDRScale for arrays / 3D textures.
   const float* getArrayHDRScale(void) const { return mArrayHDRScale.isEmpty() ? NULL : mArrayHDRScale.getPtr(); }
   const  BSmallDynamicRenderArray<float>& getArrayHDRScaleContainer(void) const { return mArrayHDRScale; }
   
   uint getAllocationSize(void) const { return mAllocationSize; }
            
protected:   
   BD3DTexture                      mD3DTexture;
   float                            mHDRScale;
   
   uint                             mNumSlices;
   BSmallDynamicRenderArray<float>  mArrayHDRScale;
   
   eDDXDataFormat                   mDDXDataFormat;
   uint                             mAllocationSize;
         
   static bool translateDDXFormatToD3D(D3DFORMAT& format, const BDDXTextureInfo& textureInfo, const BCreateParams& createParams);
   
   bool createFromRawTextureData(
      BD3DTextureType D3DTextureType,
      const uchar* pData, uint dataLen, 
      const BDDXTextureInfo& textureInfo, const BCreateParams& createParams);
   
   bool createFromXboxTextureData(
      BD3DTextureType D3DTextureType,
      const uchar* pData, uint dataLen, 
      const BDDXTextureInfo& textureInfo, const BCreateParams& createParams);

#if 0      
   void determineD3DTextureAllocationSize(void);
#endif   
};   

//============================================================================
//
//  TerrainTexturing.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "D3DTexture.h"
#include "threading\eventDispatcher.h"
#include "terrainIO.h"
#include "D3DTextureLoader.h"
#include "renderThread.h"

#include "StaticBitStateArray.h"

//xgamerender
#include "..\xgameRender\gpuDXTPack.h"
#include "atgFont.h"

class TerrainIOLoader;
class BTerrainCachedCompositeTexture;
class BTextureCache;
class BTerrainQuadNode;
class BFXLEffectFileLoader;

enum cTerrainTextureChannels
{
   cTextureTypeAlbedo   = 0,
   cTextureTypeNormal   ,
   cTextureTypeSpecular ,
   cTextureTypeEnvMask,
   cTextureTypeSelf,      
   
   cTextureTypeMax      
};


struct BTerrainTextureChannelParams
{
   cTerrainTextureChannels    mChannel;
   D3DFORMAT                  mMainCacheFormat;
   D3DFORMAT                  mSmallCacheFormat;
   // controls whether or not the GPU will convert samples read from the cache to linear light.
   // See BTextureCacheNode::create().
   // srgbRead can only be true for 1 or 3-component non-normal map textures, like albedo, specular, env. mask, etc.
   // It should not be true for packed DXT5H (HDR) textures.
   bool                       mCacheSRGBRead;

   // This controls whether or not each channels texture array uses sRGBRead.
   // See BTerrainTexturing::loadTextureCallback().
   bool                       mArraySRGBRead;

   // This controls whether or not each channels texture is HDR.
   // HDR textures get composed and created differently, and use an HDR surface.
   bool                       mIsHDR;
};

//IF YOU ADD/REMOVE CHANNELS YOU MUST CHANGE cNumCachedChannels (listed below) AS WELL!!
//not doing so will cause channels to be missing from the composite system and/or memory overrun problems
extern CONST _declspec(selectany) BTerrainTextureChannelParams gTerrainChannelParams[cTextureTypeMax]=
{
   { cTextureTypeAlbedo,   D3DFMT_DXT1,   D3DFMT_R5G6B5,                true,    false,   false},
   { cTextureTypeNormal,   D3DFMT_DXN,    (D3DFORMAT)0/*D3DFMT_G8R8*/,                  false,   false,   false},
   { cTextureTypeSpecular, D3DFMT_DXT1,   (D3DFORMAT)0/*D3DFMT_R5G6B5*/,                true,    false,   false},
   { cTextureTypeEnvMask,  D3DFMT_DXT1,   (D3DFORMAT)0/*D3DFMT_R5G6B5*/,                true,    false,   false},
   { cTextureTypeSelf,     D3DFMT_DXT5,  (D3DFORMAT)0/*D3DFMT_A16B16G16R16F_EXPAND*/,   false,   true,    true}
};

//CLM - change these to whatever depending on your visual quality.
extern CONST _declspec(selectany)  float gTextureLODDistance[] = 
{
   //0.0f,        // cUniqueTextureWidth starts here
   200.0f,        // cUniqueTextureWidth >> 1 starts here
   300.f,         // cUniqueTextureWidth >> 2 starts here
   300.0f,        // cUniqueTextureWidth >> 3 starts here
   550.0f         // cUniqueTextureWidth >> 4 starts here
}; 


//#define TINY_TEXTURE_CACHE   

enum cTerrainTextureNumbers
{
   cAlphaTextureWidth         = 64,
   cAlphaTextureHeight        = 64,

   cUniqueTextureWidth        = 512,
   cUniqueTextureHeight       = 512,

#ifdef TINY_TEXTURE_CACHE
   cMaxNumCachePages          = 4,       //number of mip0 textures in the cache
#else
   //cMaxNumCachePages          = 32,       //number of mip0 textures in the cache
   cMaxNumCachePages          = 20,       //number of mip0 textures in the cache
#endif   
   cNumMainCacheLevels        = 2,        //512, 256
   cNumMainTextureNumMips     = 2,       //ensures that 512>>(cNumMainCacheLevels-1) still great enough for our min 4k alighnment for resolve.

   cTexSize360Cutoff          = 128,
#ifdef TINY_TEXTURE_CACHE   
   cMaxNumSmallCachePages     = 32,       //number of pages in our small cache (texSize = 128x128 min)
#else
   cMaxNumSmallCachePages     = 256,       //number of pages in our small cache (texSize = 128x128 min)
#endif   
   cNumSmallCacheLevels       = 2,        //128, 64 NOTE, making this larger causes lockups!!!
   cNumSmallTextureNumMips    = 1,       //ensures that 512>>(cNumMainCacheLevels-1) still great enough for our min 4k alighnment for resolve.

   cNumCachedChannels         = cTextureTypeMax,        // < = cTextureTypeMax

   cTotalNumberOfLevels       = cNumSmallCacheLevels + cNumMainCacheLevels,

   cAlphaLayersFormat         = D3DFMT_A4R4G4B4,

   cNumMaxBlendsDoneAtOnce    = 32

};

enum eCacheState
{
   cCS_Free=0,              //this texture can be used
   cCS_Used,                  //this texture HAS been used
   cCS_StatesInFastList,      //CLM ALL STATES IN FAST LIST MUST BE BELOW THIS!

   cCS_UsedThisFrame,         //this texture is going to be used, and is being held
   cCS_Subdivided,            //this texture is subdivided to lower nodes
   cCS_Blocked,               //this texture is blocked by a parent texture being used

   cCS_FORCEBYTE = 0xFF
};

//--------------------------
class BTerrainCompositeSurface
{
public:
	BTerrainCompositeSurface();
	~BTerrainCompositeSurface();

	void  freeDeviceData();

	void create(int width, int height, bool isHDR);
	
	void allocateTempResources();
	void releaseTempResources();

	LPDIRECT3DSURFACE9 mRenderSurface;
   LPDIRECT3DTEXTURE9 mTempResolveTarget;

	int mWidth;
	int mHeight;
	bool mIsHDR;
};
//------------------------------------------------------------
class BTerrainCachedCompositeTexture
{
public:
	BTerrainCachedCompositeTexture();
	~BTerrainCachedCompositeTexture();

	void  freeDeviceData();

   void free();
   void copyTo(BTerrainCachedCompositeTexture *output);


	LPDIRECT3DTEXTURE9 mTextures[cNumCachedChannels];
   int mWidth;
   int mHeight;

   void *mpOwnerCacheNode;
   BTerrainQuadNode *mpOwnerQuadNode;
};
//------------------------------------------------------------
class BTerrainLayerContainerSplatData
{
public:
   BTerrainLayerContainerSplatData();
   ~BTerrainLayerContainerSplatData();

   void freeDeviceData();
   int getNumLayers();


   D3DXVECTOR4          *mLayerData;      //LayerIndex, uScale, vScale,1/LayerIndex
   int                  mNumLayers;
   int                  mNumAlignedLayers;

   LPDIRECT3DARRAYTEXTURE9 mAlphaLayerTexture;
   void                    *mpPhysicalMemoryPtr;
};
//------------------------------------------------------------
class BTerrainActiveTextureSetDecalInstanceInfo
{
public:
   int   mActiveTextureIndex;
   float mRotation;
   float mTileCenterX;
   float mTileCenterY;
   float mUScale;
   float mVScale;
   BManagedTextureHandle mpExternalAlphaTextureToUse;
};
//------------------------------------------------------------
//An active texture set decal uses the active texture set
//but uses an opacity defined as a decal externally
class BTerrainLayerContainerTextureSetDecalData
{
public:
   BTerrainLayerContainerTextureSetDecalData();
   ~BTerrainLayerContainerTextureSetDecalData();

   BDynamicArray<BTerrainActiveTextureSetDecalInstanceInfo> mTextureSetDecalInstances;

   int getNumAddedLayers(){ return mTextureSetDecalInstances.size();}
   void freeDeviceData(){}
   
};
//------------------------------------------------------------
class BTerrainLayerContainerDecalData
{
public:
   BTerrainLayerContainerDecalData();
   ~BTerrainLayerContainerDecalData();

   void freeDeviceData();
   int getNumLayers();

   int                  *mActiveDecalIndexes;
   int                  mNumLayers;
   int                  mNumAlignedLayers;

   LPDIRECT3DARRAYTEXTURE9 mAlphaLayerTexture;

   void                    *mpPhysicalMemoryPtr;
};
//------------------------------------------------------------
class BTerrainTextureLayerContainer
{
public:
   BTerrainTextureLayerContainer();
   ~BTerrainTextureLayerContainer();

   void freeDeviceData();
   int getNumLayers();
   int getNumSplatLayers();
   int getNumDecalLayers();
   int getNumTextureSetDecalLayers();

  BTerrainLayerContainerSplatData         mSplatData;
  BTerrainLayerContainerDecalData         mDecalData;
  BTerrainLayerContainerTextureSetDecalData    mTextureSetDecalData;


   bool              mSpecPassNeeded;
   bool              mSelfPassNeeded;
   bool              mEnvMaskPassNeeded;
   bool              mAlphaPassNeeded;
   bool              mIsFullyOpaque;
};

//------------------------------------------------------------
class BTerrainDecalLayerContainer
{
public:
   BTerrainDecalLayerContainer();
   ~BTerrainDecalLayerContainer();

   void freeDeviceData();
   int getNumLayers();
};
//------------------------------------------------------------
class BTerrainTexturingRenderData   //each quadnode chunk gets one of these
{
public: 

   BTerrainTexturingRenderData();
   ~BTerrainTexturingRenderData();
   void  freeDeviceData();

   BTerrainTextureLayerContainer mLayerContainer;
   BTerrainCachedCompositeTexture *mCachedUniqueTexture;

}; 

//------------------------------------------------------------
class BTerrainActiveDecalInfo
{
public:
   BFixedString256 mFilename;
};
//------------------------------------------------------------
class BTerrainActiveDecalInstanceInfo
{
public:
   int   mActiveDecalIndex;
   float mRotation;
   float mTileCenterX;
   float mTileCenterY;
   float mUScale;
   float mVScale;
};

//------------------------------------------------------------
class BTerrainActiveTextureInfo
{
public:
   BFixedString256 mFilename;
   int            mUScale;
   int            mVScale;
   int            mBlendOp;
};
//------------------------------------------------------------
class BTerrainActiveTextureArrayHolder
{
public:
   BD3DTexture                mD3DTextureArrayHandle;
   D3DXVECTOR4                *mHDRScales;
};
//------------------------------------------------------------
class BTerrainActiveDecalHolder
{
public:
   BD3DTexture             mTextures[cTextureTypeMax];
   BD3DTexture             mOpactityTexture;
   float                   mSelfHDRScale;
   char                    mTextureTypesUsed;         //bit array that you mask
};
//------------------------------------------------------------
class BTerrainTexturing : public BEventReceiver, public BRenderCommandListener
{
public:

   BTerrainTexturing();
   ~BTerrainTexturing();

   void           init(void);     //called @ beginning of time
   void           deinit(void);   //called @ end of time
      
   bool           loadTextures(long dirID, const XTTHeader& texList,BTerrainActiveTextureInfo *texInfo );
   bool           setLoadDecalTextures(long dirID, const XTTHeader& texList,BTerrainActiveDecalInfo *decalInfo,BTerrainActiveDecalInstanceInfo *instanceInfo   );
   void           destroy(void);
   
  
   
   static int                          getAlphaTextureWidth(void);       //returns the width of the alpha blending textures
   static int                          getAlphaTextureHeight(void);      //returns the height of the alpha blending textures

   static int                          getMaxNumberMipLevels(void);

   //CACHED TEXTURES
   int                                 computeUniqueTextureLOD(XMVECTOR woldMinVert, XMVECTOR worldMaxVert, const XMVECTOR camPos, float &distFromCam);
   BTerrainCachedCompositeTexture*     getAvailableCachedTexture(int LODLevel);
   void                                composeCompositeTexture(BTerrainCachedCompositeTexture *input,BTerrainTextureLayerContainer &layerInput);

   void                                initCaches(bool skipSpecular, bool skipEmissive, bool skipEnvMap);
   void                                freeAllCache(void);   
   void                                destroyCaches(void);
   void                                freeCachedTexture(BTerrainCachedCompositeTexture *tex);
   void                                useCachedTexture(BTerrainCachedCompositeTexture *tex);
   void                                holdCachedTexture(BTerrainCachedCompositeTexture *tex);
   void                                unholdCachedTexture(BTerrainCachedCompositeTexture *tex);
   void                                decoupleCachedTexture(BTerrainCachedCompositeTexture *tex);
   bool                                isTextureInCache(BTerrainCachedCompositeTexture *tex);
   void                                defragmentCaches(void);
   void                                dumpCachesToLog(int appendNum);


   void                                copyTexturesGPU(LPDIRECT3DTEXTURE9 src, LPDIRECT3DTEXTURE9 dst,int width, int height);

   //MAKE SURE YOU CALL THESE BEFORE/AFTER DOING BATCH COMPOSITING!
   void                                postCompositeSetup();
   void                                preCompositeSetup();
   
   void                                allocateTempResources();
   void                                releaseTempResources();


   BTextureCache       *getMainCache(){return mpMainCache;};

   void                 setEnvMapTexture(int dirID, const char *filename);
   BD3DTexture          *getEnvMapTexture(){return &mTerrainEnvMapTexture;};
   float                getEnvMapHDRScale(){return mTerrainEnvMapHDRScale;};
   int                  getNumActiveTextures() { return mXTDTextureInfo.mNumActiveTextures;};
   bool                 getOKToTexture() { return mActiveTextureSetsLoaded==cTextureTypeMax;};
   
   void                 setSpecExponentPower(float power){ mSpecExponentPower = power;};
   void                 setBumpPower(float power){mBumpPower=power;};

   void                 setSpecOnlyDirColor(float r, float g, float b){ mSpecOnlyDir_Col_Shad.x = r;mSpecOnlyDir_Col_Shad.y = g;mSpecOnlyDir_Col_Shad.z = b;};
   void                 setSpecOnlyDirShadowAttn(float attn){ mSpecOnlyDir_Col_Shad.w = attn;};
   void                 setSpecOnlyDirPower(float power){ mSpecOnlyDir_Dir_Int.w = power;};
   void                 setSpecOnlyDirDirection(float x, float y, float z){ mSpecOnlyDir_Dir_Int.x = x;mSpecOnlyDir_Dir_Int.y = y;mSpecOnlyDir_Dir_Int.z = z;};

   int                  getBuildingSplatIndexUNSC(){return mBuildingSplatIndexUNSC;}
   void                 setBuildingSplatIndexUNSC(int idx){mBuildingSplatIndexUNSC=idx;}

   int                  getBuildingSplatIndexCOVN(){return mBuildingSplatIndexCOVN;}
   void                 setBuildingSplatIndexCOVN(int idx){mBuildingSplatIndexCOVN=idx;}

   LPDIRECT3DTEXTURE9          getLargeUniqueAlbedoTexture(){return mLargeUniqueAlbedoTexture;};

   void                                setShowMips(bool onOff){mShowMips=onOff;};
   void                                setVisualizeCache(bool onOff){mShowCache=onOff;};
   void                                toggleVisualizeCache(){mShowCache=!mShowCache;};
   void                                debugDraw(ATG::Font& font);

   uint                                getGPUPackCount(void) const { return mGPUPackCount; }
   uint                                getGPUPackPixels(void) const { return mGPUPackPixels; }
   void                                resetGPUPackStats(void) { mGPUPackCount = 0; mGPUPackPixels = 0; }
   
   bool                                getLODEnabled(void) const { return mLODEnabled; }
   void                                setLODEnabled(bool enabled) { mLODEnabled = enabled; }

   const D3DBaseTexture*               getActiveTextureArray(uint channelIndex){BDEBUG_ASSERT(channelIndex < cTextureTypeMax); return mActiveTextures[channelIndex].mD3DTextureArrayHandle.getBaseTexture();};
   
   void                                addSplatDecal(float worldCenterX, float worldCenterY,int terrainSplatTextureIndex, float rotation, float xScale, float zScale, BManagedTextureHandle ExternalAlphaTex);

 
   BTerrainActiveDecalHolder           *mActiveDecals;

   void                                setMaxTextureLOD(int i){BASSERT(mDynamicCacheMaxLOD>=0 && mDynamicCacheMaxLOD<4); mDynamicCacheMaxLOD = i;}

   void              setCacheChannelSkips(bool skipSpec, bool skipEmissive, bool skipEnvMap){mCacheSkipSpecular = skipSpec; mCacheSkipEmissive = skipEmissive; mCacheSkipEnvMap = skipEnvMap;}
private:
   BTerrainActiveTextureArrayHolder    mActiveTextures[cTextureTypeMax];
   
   BTerrainActiveTextureInfo           *mActiveTexInfo;
   int                                 mActiveTextureSetsLoaded;

   
   BTerrainActiveDecalInfo             *mActiveDecalInfo;
   BTerrainActiveDecalInstanceInfo     *mActiveDecalInstanceInfo;
  
   
   bool                 mShowMips;
   bool                 mShowCache; 
   bool                 mLODEnabled;

	//cacheing
   BTerrainCompositeSurface*        getRenderSurface(bool isHDR=false);
   //compositing
   BTerrainCachedCompositeTexture*  compositeTexture(BTerrainTextureLayerContainer &layerInput, int width, int height);
   void                             compositeTexturesGPU(BTerrainTextureLayerContainer &layerInput, BTerrainCachedCompositeTexture *output);
   inline void                      composeSplatLayers(int channelIndex,int mipIndex,int resolveLevel,int levelWidth,BTerrainCompositeSurface *surf,bool doCompression,BTerrainTextureLayerContainer &layerInput, BTerrainCachedCompositeTexture *output);
   inline void                      composeDecalLayers(int channelIndex,int mipIndex,int resolveLevel,int levelWidth,BTerrainCompositeSurface *surf,bool doCompression,BTerrainTextureLayerContainer &layerInput, BTerrainCachedCompositeTexture *output);
   inline void                      composeTextureSetDecalLayers(int channelIndex,int mipIndex,int resolveLevel,int levelWidth,BTerrainCompositeSurface *surf,bool doCompression,BTerrainTextureLayerContainer &layerInput, BTerrainCachedCompositeTexture *output);
   inline void                      doComposeTextures(int channelIndex, int mipIndex,int resolveLevel,int levelWidth,BTerrainCompositeSurface *surf,bool doCompression,BTerrainTextureLayerContainer &layerInput, BTerrainCachedCompositeTexture *output);
   //actives
   bool                             addActiveTexture(const char *filename);

   void                             addSplatDecalInternal(float worldCenterX, float worldCenterY,int terrainSplatTextureIndex, float rotation, float xScale, float zScale, BManagedTextureHandle ExternalAlphaTex);


   LPDIRECT3DVERTEXDECLARATION9  m_pQuadVertexDecl;
   LPDIRECT3DVERTEXBUFFER9       m_pQuadVertexBuffer;
   int                           mNumVertsInQuad;

   void                          loadEnvMapTextureCallback(void *pData);
   BD3DTexture                   mTerrainEnvMapTexture;
   float                         mTerrainEnvMapHDRScale;

   float                         mSpecExponentPower;
   D3DXVECTOR4                   mSpecOnlyDir_Dir_Int;
   D3DXVECTOR4                   mSpecOnlyDir_Col_Shad;

   float                         mBumpPower;
   int                           mBuildingSplatIndexUNSC;
   int                           mBuildingSplatIndexCOVN;

   BFXLEffectFileLoader*            mpEffectLoader;
   BFXLEffect						      mCompShader;
   BFXLEffectTechnique              mCurrTechnique;
      
   BFXLEffectParam                 mShaderTargetSamplerHandle;
   BFXLEffectParam                 mShaderAlphaSamplerHandle;

   BFXLEffectParam                 mShaderTargetSamplerIndexArray;
   BFXLEffectParam                 mShaderNumLayers;
   BFXLEffectParam                 mShaderRcpNumAlignedLayers;
   BFXLEffectParam                 mShaderExplicitTargetMipLevel;
   BFXLEffectParam                 mShaderHighResHDRUVOffset;

   BFXLEffectParam                 mshaderLayerIndex;

   BFXLEffectParam                 mShaderTargetDecalSamplerHandle;
   BFXLEffectParam                 mShaderTargetDecalOpactiySamplerHandle;
   BFXLEffectParam                 mShaderTargetDecalAlphaSamplerHandle;


  


private:

   BTextureCache                *mpMainCache;
  
   int                                           mCacheDefragCounter;
   int                           mDynamicCacheMaxLOD;

   BTerrainCompositeSurface                     *mSharedSurface;
   BTerrainCompositeSurface                     *mSharedHDRSurface;
   
   IDirect3DTexture9*                           mpEDRAMSaveTexture;

   //temp vars for saving data
   D3DVIEWPORT9                                 mSavedViewPort;
   D3DSurface*                                  mpRenderTarget0;
   D3DSurface*                                  mpDepthStencil0;
#if 0   
   BGPUDXTPack                                  *mpDXTGPUPacker;
#endif   
   
   DWORD                                        mSavedColorWriteEnable;

   bool                                         mDoFreeCache;
   bool                                         mCacheSkipSpecular;
   bool                                         mCacheSkipEmissive;
   bool                                         mCacheSkipEnvMap;

   //Large Unique Textures
   void                       *mpLargeUniqueAlbedoDataPointer;
   int                        mLargeUniqueTextureWidth;
   int                        mLargeUniqueTextureHeight;
   int                        mLargeUniqueTextureMipCount;
   LPDIRECT3DTEXTURE9         mLargeUniqueAlbedoTexture;
   bool                       convertUniqueAlbedoFromMemory();

   //loading
   long mDirID;
   XTTHeader mXTDTextureInfo;
   
   uint mAlbedoTexturesDirty;
   uint mNormalTexturesDirty;
   uint mSpecularTexturesDirty;
   uint mSelfTexturesDirty;
   uint mEnvMaskTexturesDirty;
   uint mDecalTexturesDirty;
   
   uint mNextAsyncFileSetRequestID;
   
   uint mGPUPackCount;
   uint mGPUPackPixels;
   
   struct BAsyncTextureLoadState
   {
      uint mRequestID;
      BD3DTextureLoader* mpTextureLoader;
      uint mNumLoadedTextures;
      BDynamicArray<bool>     mValidLoadedTexture;
   };
   
   
   BAsyncTextureLoadState mAsyncTextureLoadState[cTextureTypeMax];
   
   void loadTextureCallback(void* pData);
   bool loadTerrainTextureArray(long dirID, const char* pBasePath, const char* pFileSuffix, const XTTHeader& texNames, uint textureType);
   bool loadAlbedoTextureArray(bool reloading = false);
   bool loadNormalTextureArray(bool reloading = false);
   bool loadSpecularTextureArray(bool reloading = false);
   bool loadSelfTextureArray(bool reloading = false);
   bool loadEnvMaskTextureArray(bool reloading = false);
   bool loadDecalTextures(bool reloading = false);
   void loadDecalCallback(void* pData);


   // tickEffect() must always be called before rendering any techniques in the effect.
   // It will wait for the effect to load if needed, update the effect's intrinsics parameters, and handle reloading.
   void tickEffect(void);
   void loadEffect(void);
   void initEffectConstants(void);
   
   struct GPU_QUAD_VERTEX
   {
      D3DXFLOAT16 x, y, z, w;
      D3DXFLOAT16 tu0, tv0;
      D3DXFLOAT16 tu1, tv1;
   };


   void preCompositeSaveEDRAM();
   void preCompositeRestoreEDRAM();

   enum 
   {
      cTTC_Destroy = 0,
      cTerrainTexturingReloadFileEvent = cEventClassFirstUser,
      cTTC_SplatDecal
   };
   
   void destroyInternal();

   void reloadInit(long dirID, const char* pBasePath, const XTTHeader& texNames);

   //Messaging stuff
   BCommandListenerHandle mCommandListenerHandle;

   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void) { }
   virtual void deinitDeviceData(void);
   virtual void beginLevelLoad(void);
   virtual void endLevelLoad(void);
         
   friend class BTerrainIOLoader;
   friend class BTerrainRender;
   friend class BTerrain;
};


//------------------------------------------------------------
extern BTerrainTexturing gTerrainTexturing;



























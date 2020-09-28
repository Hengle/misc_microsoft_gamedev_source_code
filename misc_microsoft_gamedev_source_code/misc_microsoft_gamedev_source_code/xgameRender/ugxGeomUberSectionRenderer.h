//============================================================================
//
//  ugxGeomUberSectionRenderer.h
//  
//  Copyright (c) 2006-2007, Ensemble Studios
//
//============================================================================
#pragma once
#include "D3DTextureManager.h"
#include "effect.h"
#include "ugxGeomRenderTypes.h"
#include "ugxGeom.h"
#include "ugxGeomSectionRenderer.h"
#include "debugTextDisplay.h"

class BUGXGeomData;

//============================================================================
// class BUGXGeomUberSectionRenderer
//============================================================================
class BUGXGeomUberSectionRenderer : public BEventReceiverInterface, public IUGXGeomSectionRenderer
{
   BUGXGeomUberSectionRenderer(const BUGXGeomUberSectionRenderer&);
   BUGXGeomUberSectionRenderer& operator= (const BUGXGeomUberSectionRenderer&);

public:
   BUGXGeomUberSectionRenderer();
   virtual ~BUGXGeomUberSectionRenderer();

   virtual bool init(IDirect3DVertexDeclaration9* pDecl, const Unigeom::BMaterial& material, const UnivertPackerType& vertexPacker);
   virtual void deinit(void);

   virtual void loadTextures(void);
   virtual void unloadTextures(void);

   virtual bool beginPass(const BUGXGeomRenderCommonInstanceData* pCommonData, void* pState, uint stateMaxSize, eUGXGeomVisMode visMode);
   virtual void endPass(const BUGXGeomRenderCommonInstanceData* pCommonData, void* pState, uint stateMaxSize);
   
   virtual void getTextures(BD3DTextureManager::BManagedTexture* const*& pTextureList, uint& numTextures, uint multiframeTextureIndex) const;
   
private:
   BEventReceiverHandle                mEventHandle;
               
   BD3DTextureManager::BManagedTexture* mpTextures[Unigeom::BMaterial::cNumMapTypes][Unigeom::BMaterial::cMaxMultiframeTextures];
      
   BVec4                               mSpecColorPower;
   BVec4                               mEnvControl;
   BVec4                               mUVChannel[3];
   BVec4                               mUVWVelocity[Unigeom::BMaterial::cNumMapTypes];

   uint                                mValidMapFlags;
   
   float                               mOpacity;
   
   uint                                mEffectManagerLoadCount;
   
   IDirect3DVertexDeclaration9*        mpDecl;
   
   ushort                              mUVScrollFlags;

   ushort                              mUVWrapFlags;
                  
   uchar                               mStream0Size;
   uchar                               mStream1Size;
   
   uchar                               mNumTextureFrames[Unigeom::BMaterial::cNumMapTypes];
         
   enum 
   { 
      cNormalOptimizedPassIndex        = 0, 
      cLocalLightingOptimizedPassIndex = 1,
      cMaxOptimizedPassIndices   
   };
   short                               mOptimizedPassIndices[cMaxOptimizedPassIndices];
   
   uchar                               mBaseEffectIndex;
   
   //uchar                               mBlendType;
   
   uchar                               mNumUV;
                                 
   bool                                mLoadedTextures   : 1;
   bool                                mRigid            : 1;
   bool                                mDeclHasTangent   : 1;
   bool                                mPixelXForm       : 1;
   bool                                mHasOpacityMap    : 1;
   bool                                mSkyMaterial      : 1;
   bool                                mRequiresBlending : 1;
   bool                                mUVScroll         : 1;
   bool                                mOpacityValid     : 1;
   bool                                mTwoSided         : 1;
   bool                                mZWrites          : 1;
   bool                                mGlobalEnvMapping : 1;
   bool                                mNeedsUVChannels  : 1;
   bool                                mNeedsSpecular    : 1;
   bool                                mTerrainConform   : 1;
   bool                                mDisableShadowReception : 1;
   
   struct BPassState
   {
      BFXLEffectTechnique mTechnique;
   };

   bool isValidMap(uint mapType) const { return (mValidMapFlags & (1 << mapType)) != 0; }
   
   bool initTextures(const Unigeom::BMaterial& material);
   bool initEffect(const Unigeom::BMaterial& material, const UnivertPackerType& vertexPacker);
   float getUVSelector(const Unigeom::BMaterial& mat, Unigeom::BMaterial::eMapType mapType);
   void calcUVOffset(Unigeom::BMaterial::eMapType mapType, float& u, float& v);
   void calcUVWOffset(Unigeom::BMaterial::eMapType mapType, float& u, float& v, float& w);
   void setTexture(Unigeom::BMaterial::eMapType type, int samplerReg, uint multiframeTextureIndex = 0, const BD3DTexture* pOverrideTex = NULL);
   bool processEffectCompileResults(const BEvent& event);
   bool processTextureStatusChanged(const BEvent& event);
   void setUVConstants(void);
   void findPass(void);
   static bool getIsSRGBTexture(eDefaultTexture typeIndex);
   static eDefaultTexture getDefaultTexture(Unigeom::BMaterial::eMapType typeIndex);
   static bool getIsSRGBTexture(Unigeom::BMaterial::eMapType typeIndex);
   
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};

//============================================================================
// class BUGXGeomUberSectionRendererArray
//============================================================================
class BUGXGeomUberSectionRendererArray : public IUGXGeomSectionRendererArray
{
   BUGXGeomUberSectionRendererArray(const BUGXGeomUberSectionRendererArray&);
   BUGXGeomUberSectionRendererArray& operator= (const BUGXGeomUberSectionRendererArray&);

public:
   BUGXGeomUberSectionRendererArray();
   virtual ~BUGXGeomUberSectionRendererArray();

   virtual bool init(BUGXGeomData* pGeomData);
   virtual void deinit(void);

   virtual uint getSize(void) const { return mNumSections; }
   virtual const IUGXGeomSectionRenderer& getSection(uint sectionIndex) const { BDEBUG_ASSERT(sectionIndex < mNumSections); return mpSections[sectionIndex]; }
   virtual IUGXGeomSectionRenderer& getSection(uint sectionIndex) { BDEBUG_ASSERT(sectionIndex < mNumSections); return mpSections[sectionIndex]; }

private:
   uint                          mNumSections;
   BUGXGeomUberSectionRenderer*  mpSections;
};

//============================================================================
// class BUGXGeomUberSectionRendererManager
//============================================================================
class BUGXGeomUberSectionRendererManager : public IUGXGeomSectionRendererManager
{
   BUGXGeomUberSectionRendererManager(const BUGXGeomUberSectionRendererManager&);
   BUGXGeomUberSectionRendererManager& operator= (const BUGXGeomUberSectionRendererManager&);

public:
   BUGXGeomUberSectionRendererManager();
   virtual ~BUGXGeomUberSectionRendererManager();

   virtual IUGXGeomSectionRendererArray* initSectionArray(BUGXGeomData* pGeomData);
   virtual void deinitSectionArray(IUGXGeomSectionRendererArray* pSectionArray);

   virtual void globalRenderBegin(double gameTime, eUGXGeomPass pass, eUGXGeomVisMode visMode, BManagedTextureHandle globalEnvMap, eUGXGeomTextureMode textureMode, const BBlackmapParams& blackmapParams);         

   // The common instance data pointed to by pCommonData must remain valid until renderEnd().
   virtual void renderBegin(const BUGXGeomRenderCommonInstanceData* pCommonData);
   virtual void renderSetLocalLightState(const BUGXGeomRenderCommonInstanceData* pCommonData);
   virtual void renderEnd(const BUGXGeomRenderCommonInstanceData* pCommonData);

   virtual void globalRenderEnd(eUGXGeomPass pass);
   
   enum { cMaxDisplayPages = 2 };
   static void displayStats(BDebugTextDisplay& textDisplay, uint page);
   
   static void dumpShaderMacros(bool optimizedShaders);
};


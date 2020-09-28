//============================================================================
//
//  ugxGeomSectionRenderer.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once
#include "D3DTextureManager.h"
#include "worldVisibility.h"

class BUGXGeomData;

//============================================================================
// class IUGXGeomSectionRenderer
//============================================================================
class IUGXGeomSectionRenderer
{
   IUGXGeomSectionRenderer(const IUGXGeomSectionRenderer&);
   IUGXGeomSectionRenderer& operator= (const IUGXGeomSectionRenderer&);
   
public:
   enum eBlendType
   {
      cBlendAlphaToCoverage,
      cBlendOver,
      cBlendAdditive,
      cBlendAlphaTest,
   };
   
   IUGXGeomSectionRenderer() : mBlendType(cBlendAlphaToCoverage), mLayerFlags(0), mDisableShadows(false) { }
   virtual ~IUGXGeomSectionRenderer() { }

   typedef UnivertPacker<true, cBigEndianNative> UnivertPackerType;
   
   virtual bool init(IDirect3DVertexDeclaration9* pDecl, const Unigeom::BMaterial& material, const UnivertPackerType& vertexPacker) = 0;
   virtual void deinit(void) = 0;

   virtual void loadTextures(void) = 0;
   virtual void unloadTextures(void) = 0;
   
   virtual bool beginPass(const BUGXGeomRenderCommonInstanceData* pCommonData, void* pState, uint stateMaxSize, eUGXGeomVisMode visMode) = 0;
   virtual void endPass(const BUGXGeomRenderCommonInstanceData* pCommonData, void* pState, uint stateMaxSize) = 0; 
   
   virtual void getTextures(BD3DTextureManager::BManagedTexture* const*& pTextureList, uint& numTextures, uint multiframeTextureIndex) const = 0; 
   
   eBlendType getBlendType(void) const { return mBlendType; }
   uint getLayerFlags(void) const { return mLayerFlags; }
   bool getDisableShadows(void) const { return mDisableShadows; }
   bool getLocalReflectionEnabled(void) const { return mLocalReflection; }

protected:
   eBlendType  mBlendType;
   uchar       mLayerFlags;
   bool        mDisableShadows : 1;
   bool        mLocalReflection  : 1;
};

//============================================================================
// class IUGXGeomSectionRendererArray
//============================================================================
class IUGXGeomSectionRendererArray
{
   IUGXGeomSectionRendererArray(const IUGXGeomSectionRendererArray&);
   IUGXGeomSectionRendererArray& operator= (const IUGXGeomSectionRendererArray&);
   
public:
   IUGXGeomSectionRendererArray() { }
   virtual ~IUGXGeomSectionRendererArray() { }
   
   virtual bool init(BUGXGeomData* pGeomData) = 0;
   virtual void deinit(void) = 0;
   
   virtual uint getSize(void) const = 0;
   virtual const IUGXGeomSectionRenderer& getSection(uint sectionIndex) const = 0;
   virtual IUGXGeomSectionRenderer& getSection(uint sectionIndex) = 0;
};

//============================================================================
// class IUGXGeomSectionRendererManager
//============================================================================
class IUGXGeomSectionRendererManager
{
   IUGXGeomSectionRendererManager(const IUGXGeomSectionRendererManager&);
   IUGXGeomSectionRendererManager& operator= (const IUGXGeomSectionRendererManager&);
   
public:
   IUGXGeomSectionRendererManager() { }
   virtual ~IUGXGeomSectionRendererManager() { }
   
   // May return NULL if a section init fails.
   virtual IUGXGeomSectionRendererArray* initSectionArray(BUGXGeomData* pGeomData) = 0;
   
   virtual void deinitSectionArray(IUGXGeomSectionRendererArray* pSectionArray) = 0;
   
   virtual void globalRenderBegin(double gameTime, eUGXGeomPass pass, eUGXGeomVisMode visMode, BManagedTextureHandle globalEnvMap, eUGXGeomTextureMode textureMode, const BBlackmapParams& blackmapParams) = 0;         

   // The common instance data pointed to by pCommonData must remain valid until renderEnd().
   virtual void renderBegin(const BUGXGeomRenderCommonInstanceData* pCommonData) = 0;
   virtual void renderSetLocalLightState(const BUGXGeomRenderCommonInstanceData* pCommonData) = 0;
   virtual void renderEnd(const BUGXGeomRenderCommonInstanceData* pCommonData) = 0;

   virtual void globalRenderEnd(eUGXGeomPass pass) = 0;         
};


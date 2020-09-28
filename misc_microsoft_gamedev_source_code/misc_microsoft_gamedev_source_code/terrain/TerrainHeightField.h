//==============================================================================
// TerrainHeightField.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once
#include "renderThread.h"
#include "math\vectorInterval.h"
#include "effectFileLoader.h"
#include "TerrainRender.h"

//==============================================================================
// class BTerrainHeightField
//==============================================================================
class BTerrainHeightField : public BRenderCommandListener
{
public:
   // Sim thread
   BTerrainHeightField();
   ~BTerrainHeightField();
   
   void init(void);
   void deinit(void);
   
   
   // All remaining methods can only be called from the render thread.
         
   bool loadECFXTHTInternal(int dirID, const char* pFilenameNoExtension);
   void unload(void);

   void computeHeightField(uint width = 256U, uint height = 256U);
   void releaseHeightField(void);
   
   void render(XMMATRIX worldMatrix, uint tessLevel);
   
   struct BHeightFieldAttributes
   {
      // Height field (x,y,z) to world space, where (x,y) are the texel coords, and z is the normalized depth [0.0, 1.0].
      XMMATRIX mNormZToWorld;
                        
      // Transforms world coords to continuous height field map coordinates.
      XMMATRIX mWorldToNormZ;
      
      uint mWidth;
      uint mHeight;
      
      const DWORD* mpTexels;
      void* mpPhysicalMemoryPointer;
      void* mpAlphaPhysicalMemoryPointer;
      
      AABB mBounds;
                  
      double mWorldMinY;
      double mWorldMaxY;
      double mWorldRangeY;
   };
   
   // Height field texture's format is D3DFMT_G16R16. R is bottom depth, G is top depth.
   // Will return NULL if terrain load failed.
   IDirect3DTexture9* getHeightFieldTex(void) const { return mpHeightField;} 
   const BHeightFieldAttributes& getHeightFieldAttributes(void) const { return mHeightFieldAttributes; }
   
   uint getMapWidth(void) const { return mHeightFieldAttributes.mWidth; }
   uint getMapHeight(void) const { return mHeightFieldAttributes.mHeight; }
   
   float computeWorldY(float normZ) const;
   float computeHeightfieldNormZ(float worldY) const; //given a Y in worldspace, give me the scaled position in heightfieldspace

   // These methods are slow!
   bool sampleHeights(int x, int y, float& lowNormZ, float& highNormZ) const;
   bool sampleHeights(float x, float y, float& lowNormZ, float& highNormZ) const;
   
   // Returns continuous map texel coordinates.
   XMVECTOR computeMapCoords(XMVECTOR worldPos) const;
   void computeMapCoords(XMVECTOR worldPos, int& x, int& y, float& normZ, bool clamp) const;
   
   XMVECTOR computeWorldCoords(int x, int y, float normZ) const;
                  
   bool castRay(XMVECTOR worldPos, float& lowNormZ, float& highNormZ) const;
   
   void renderHeightfieldForOcclusion(void);

   void renderDebugGrid(void);
   bool renderDebugHeightField(void);

   void flattenAreaInstant(float mMinXPerc, float mMaxXPerc, float mMinZPerc, float mMaxZPerc, float desiredHeight);
   
   void setBlackmapShaderParms(const BBlackmapParams& params);
   void setLightBufferingParams(void);
   
   enum eRenderPassIndex
   {
      eUnlitDecalPatch = 0,
      eLitDecalPatch = 1,
      eVisDecalPatch = 2,

      eLitRibbon = 3,
      eVisRibbon = 4,

      
   };

   bool renderRibbon(const D3DVertexBuffer* pVB, int vertMemSize, int numVerts, eRenderPassIndex techniquePassIndex);

   struct BPatchInstance
   {
      BVec3    mWorldPos;
      
      // HALF4
      HALF     mForwardX;
      HALF     mForwardY;
      HALF     mForwardZ;
      HALF     mRightX;
      
      // HALF4
      HALF     mRightY;
      HALF     mRightZ;
      HALF     mYOffset;
      HALF     mIntensity;

      HALF     mU;
      HALF     mV;
      HALF     mWidth;
      HALF     mHeight;
               
      D3DCOLOR mColor;
   };
   
   enum { cMinTessLevel = 1, cMaxTessLevel = 15 };
   
   bool renderPatches(
      const BPatchInstance* pInstances, 
      uint numInstances, 
      float tessLevel, 
      float yLowRange, float yHighRange,
      bool conformToTerrain, 
      eRenderPassIndex techniquePassIndex = eUnlitDecalPatch);
            
private:
   BHeightFieldAttributes mHeightFieldAttributes;
   IDirect3DTexture9* mpHeightField;
   IDirect3DTexture9* mpHeightFieldAlpha;
  
   BFXLEffectFileLoader* mpEffectLoader;
   BFXLEffectTechnique mRenderPatchesTechnique;
   BFXLEffectParam mHeightfieldSamplerParam;
   BFXLEffectParam mWorldToHeightfieldParam;
   BFXLEffectParam mHeightfieldYScaleOfsParam;
   BFXLEffectParam mConformToTerrainFlagParam;
   BFXLEffectParam mTerrainAlphaSamplerParam;
   BFXLEffectParam mHeightfieldSizesParam;

   //BLACKMAP 
   BFXLEffectParam                 mBlackmapEnabled;
   BFXLEffectParam                 mBlackmapSampler;
   BFXLEffectParam                 mBlackmapUnexploredSampler;
   BFXLEffectParam                 mBlackmapParams0;
   BFXLEffectParam                 mBlackmapParams1;
   BFXLEffectParam                 mBlackmapParams2;
   
   // Light Buffering
   BFXLEffectParam                 mLightBufferColorSampler;
   BFXLEffectParam                 mLightBufferVectorSampler;
   BFXLEffectParam                 mLightBufferingEnabled;
   BFXLEffectParam                 mWorldToLightBufCols;
   
   IDirect3DVertexDeclaration9* mpPatchInstanceDecl;
           
   void renderInit(uint width, uint height, IDirect3DSurface9* pDepthStencilSurf, bool furthestDepth, XMMATRIX worldToView, XMMATRIX viewToProj);
   void renderDeinit(IDirect3DTexture9* pDepthStencilTex);   
   void renderTerrain(void);
   void computeTransforms(XMMATRIX& worldToView, XMMATRIX& viewToProj, AABB& worldBounds, double& worldMinY, double& worldMaxY);
   IDirect3DTexture9* generateDepthBuffer(uint width, uint height, bool furthestDepth, XMMATRIX worldToView, XMMATRIX viewToProj);
   void fillHeightFieldTexture(uint width, uint height, IDirect3DTexture9* pTex, IDirect3DTexture9* pLowDepths, IDirect3DTexture9* pHighDepths, uint& minDepth, uint& maxDepth, double& minDepthF, double& maxDepthF);
   void initEffectParams(void);
   bool tickEffect(void);
               
   // BRenderCommandListener interface
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar *pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);

  
};

extern BTerrainHeightField gTerrainHeightField;
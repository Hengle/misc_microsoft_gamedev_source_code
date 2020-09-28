//============================================================================
// Ensemble Studios (c) 2006
//============================================================================
#pragma once

#include "effect.h"
#include "renderThread.h"
#include "bitvector.h"

const uint cMaxPalletteColors = 8;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct BParticleEmitterRenderCommand
{
   XMVECTOR       mPalletteColor[cMaxPalletteColors];
   XMMATRIX       mEmitterMatrix;
   XMMATRIX       mLastUpdateEmitterMatrix;
   XMVECTOR       mTextureAnimationData;
   XMVECTOR       mTimeData;
   XMVECTOR       mProgressionTextureV;
   XMVECTOR       mAABBMins;
   XMVECTOR       mAABBMaxs;
   XMVECTOR       mMultiTextureBlendMultiplier;
   XMVECTOR       mTextureStageCounts;
   XMVECTOR       mUVVelocity0;
   XMVECTOR       mUVVelocity1;
   XMVECTOR       mUVRandomOffsetSelector0;
   XMVECTOR       mUVRandomOffsetSelector1;
   XMVECTOR       mBeamForward;
   XMVECTOR       mBeamProgressionAlphaSelector;
   XMVECTOR       mVertex1Color;
   XMVECTOR       mVertex2Color;
   XMVECTOR       mVertex3Color;
   XMVECTOR       mVertex4Color;
   XMVECTOR       mTintColor;
   BVec4          mSoftParticleParams;
   BVec4          mSoftParticleParams2;
   int            mTextureHandle;
   int            mTextureHandle2;
   int            mTextureHandle3;
   int            mIntensityTextureHandle;   
   void*          mpVB;
   int            mStartOffsetVB;
   void*          mpIB;
   int            mStartOffsetIB;
   int            mVertexCount;
   int            mIndexCount;
   float          mEmitterAttraction;
   float          mOpacity;   
   float          mLightBufferIntensityScale;
   float          mTerrainDecalYOffset;
   float          mTerrainDecalTesselation;      
   BYTE           mBlendMode;
   BYTE           mNumPalletteColors;
   bool           mTextureAnimationToggle;
   bool           mIntensityMaskToggle;
   bool           mPalletteToggle;
   bool           mMultiTextureToggle;
   bool           mMultiTexture3LayersToggle;
   bool           mLightBuffering;
   bool           mSoftParticleToggle;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BParticleRenderer: public BRenderCommandListener, BEventReceiver
{
   public:    

      enum eFlags
      {
         eFlagInitialized,
         eFlagRenderBBoxes,
         eFlagTotal
      };

      enum eRenderCommand
      {
         eRenderBillboard = 0,
         eRenderUpfacing,         
         eRenderTrail,
         eRenderAxial,
         eRenderTrailStretch,
         eRenderTrailCrossPass1,
         eRenderTrailCrossPass2,
         eRenderTrailCrossStretchPass1,
         eRenderTrailCrossStretchPass2,
         eRenderBeam,
         eRenderVelocityAligned,
         eRenderTerrainPatch,
         eRenderBeamVertical,
         eRenderBeamHorizontal,
         eRenderCommandTotal
      };

      enum eRenderBlendMode
      {
         eRenderBlendMode_Alphablend = 0,
         eRenderBlendMode_Additive,
         eRenderBlendMode_Subtractive,
         eRenderBlendMode_Distortion,
         eRenderBlendMode_PremultiplyAlpha,
         eRenderBlendMode_Total,
      };

      enum eRenderCommandTechniquePass_Single
      {
         eRenderPassSingle_Alphablend=0,
         eRenderPassSingle_Alphablend_LightBuffer,
         eRenderPassSingle_Alphablend_IntensityMask,
         eRenderPassSingle_Alphablend_LightBuffer_IntensityMask,

         eRenderPassSingle_Additive,
         eRenderPassSingle_Additive_LightBuffer,
         eRenderPassSingle_Additive_IntensityMask,
         eRenderPassSingle_Additive_LightBuffer_IntensityMask,

         /*
         eRenderPassSingle_Subtractive,
         eRenderPassSingle_Subtractive_LightBuffer,
         eRenderPassSingle_Subtractive_IntensityMask,
         eRenderPassSingle_Subtractive_LightBuffer_IntensityMask,
         */

         eRenderPassSingle_Distortion,

         /*
         eRenderPassSingle_PremultipliedAlpha,
         eRenderPassSingle_PremultipliedAlpha_LightBuffer,
         eRenderPassSingle_PremultipliedAlpha_IntensityMask,
         eRenderPassSingle_PremultipliedAlpha_LightBuffer_IntensityMask,
         */

         eRenderPassSingleTotal
      };

      enum eRenderCommandTechniquePass_Multi
      {
         eRenderPassMulti_Alphablend=0,         
         eRenderPassMulti_Additive,         
         eRenderPassMulti_Subtractive,         
         eRenderPassMulti_Distortion,
         eRenderPassMulti_PremultipliedAlpha,

         eRenderPassMulti2_Alphablend,
         eRenderPassMulti2_Alphablend_LightBuffer,
         eRenderPassMulti2_Alphablend_IntensityMask,
         eRenderPassMulti2_Alphablend_LightBuffer_IntensityMask,

         eRenderPassMulti2_Additive,
         eRenderPassMulti2_Additive_LightBuffer,
         eRenderPassMulti2_Additive_IntensityMask,
         eRenderPassMulti2_Additive_LightBuffer_IntensityMask,

         /*
         eRenderPassMulti2_Subtractive,
         eRenderPassMulti2_Subtractive_LightBuffer,
         eRenderPassMulti2_Subtractive_IntensityMask,
         eRenderPassMulti2_Subtractive_LightBuffer_IntensityMask,
         */

         eRenderPassMulti2_Distortion,

         /*
         eRenderPassMulti2_PremultipliedAlpha,
         eRenderPassMulti2_PremultipliedAlpha_LightBuffer,
         eRenderPassMulti2_PremultipliedAlpha_IntensityMask,
         eRenderPassMulti2_PremultipliedAlpha_LightBuffer_IntensityMask,
         */

         eRenderPassMultiTotal
      };

      enum eRenderCommandTechniquePass_SoftSingle
      {
         eRenderPassSoftSingle_Alphablend=0,
         eRenderPassSoftSingle_Alphablend_LightBuffer,
         eRenderPassSoftSingle_Alphablend_IntensityMask,
         eRenderPassSoftSingle_Alphablend_LightBuffer_IntensityMask,

         eRenderPassSoftSingle_Additive,
         eRenderPassSoftSingle_Additive_LightBuffer,
         eRenderPassSoftSingle_Additive_IntensityMask,
         eRenderPassSoftSingle_Additive_LightBuffer_IntensityMask,

         /*
         eRenderPassSoftSingle_Subtractive,
         eRenderPassSoftSingle_Subtractive_LightBuffer,
         eRenderPassSoftSingle_Subtractive_IntensityMask,
         eRenderPassSoftSingle_Subtractive_LightBuffer_IntensityMask,
         */

         eRenderPassSoftSingle_Distortion,

         /*
         eRenderPassSoftSingle_PremultipliedAlpha,
         eRenderPassSoftSingle_PremultipliedAlpha_LightBuffer,
         eRenderPassSoftSingle_PremultipliedAlpha_IntensityMask,
         eRenderPassSoftSingle_PremultipliedAlpha_LightBuffer_IntensityMask,
         */
         
         eRenderPassSoftSingleTotal
      };

      enum eRenderCommandTechniquePass_SoftMulti
      {
         eRenderPassSoftMulti_Alphablend=0,         
         eRenderPassSoftMulti_Additive,         
         eRenderPassSoftMulti_Subtractive,         
         eRenderPassSoftMulti_Distortion,
         eRenderPassSoftMulti_PremultipliedAlpha,
         eRenderPassSoftMultiTotal
      };
      
      enum 
      {
         eEventClassReloadEffects = cEventClassFirstUser
      };
      
      BParticleRenderer();
     ~BParticleRenderer();

      //-- MAIN THREAD
      void init();
      void deInit();

      bool getFlag(long n) const { return(mFlags.isSet(n)!=0); }
      void setFlag(long n, bool v) { if(v) mFlags.set(n); else mFlags.unset(n); }
                 
      //-- BEvent Receiver interface
      bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);

      //-- WORKER THREAD
      
      void render(int effectIndex, const uchar* pData);
      
   private:
      BFXLEffect              mEffects[eRenderCommandTotal];
      BFXLEffectTechnique     mTechnique[eRenderCommandTotal];
      BFXLEffectTechnique     mMultiTextureTechnique[eRenderCommandTotal];
      BFXLEffectTechnique     mSoftParticleTechnique[eRenderCommandTotal];
      BFXLEffectTechnique     mSoftParticleMultiTextureTechnique[eRenderCommandTotal];

      BFXLEffectParam         mDiffuseTextureParam[eRenderCommandTotal];
      BFXLEffectParam         mDiffuse2TextureParam[eRenderCommandTotal];
      BFXLEffectParam         mDiffuse3TextureParam[eRenderCommandTotal];
      BFXLEffectParam         mIntensityTextureParam[eRenderCommandTotal];
      BFXLEffectParam         mTextureAnimationData[eRenderCommandTotal];
      BFXLEffectParam         mTextureAnimationToggle[eRenderCommandTotal];
      BFXLEffectParam         mIntensityMaskToggle[eRenderCommandTotal];
      BFXLEffectParam         mTimeData[eRenderCommandTotal];
      BFXLEffectParam         mEmitterMatrix[eRenderCommandTotal];
      BFXLEffectParam         mLastUpdateEmitterMatrix[eRenderCommandTotal];
      BFXLEffectParam         mEmitterAttraction[eRenderCommandTotal];
      BFXLEffectParam         mEmitterOpacity[eRenderCommandTotal];
      BFXLEffectParam         mProgressionTextureV[eRenderCommandTotal];
      BFXLEffectParam         mColorProgressionTextureParam[eRenderCommandTotal];
      BFXLEffectParam         mScaleProgressionTextureParam[eRenderCommandTotal];
      BFXLEffectParam         mIntensityProgressionTextureParam[eRenderCommandTotal];
      BFXLEffectParam         mRandomTextureParam[eRenderCommandTotal];
      BFXLEffectParam         mMultiTextureEnable3Layers[eRenderCommandTotal];
      BFXLEffectParam         mMultiTextureBlendMultiplier[eRenderCommandTotal];      
      BFXLEffectParam         mTextureStageCounts[eRenderCommandTotal];      
      BFXLEffectParam         mUVVelocity0[eRenderCommandTotal];
      BFXLEffectParam         mUVVelocity1[eRenderCommandTotal];
      BFXLEffectParam         mUVRandomOffsetSelector0[eRenderCommandTotal];
      BFXLEffectParam         mUVRandomOffsetSelector1[eRenderCommandTotal];

      BFXLEffectParam         mBeamForward[eRenderCommandTotal];
      BFXLEffectParam         mBeamProgressionAlphaSelector[eRenderCommandTotal];
      BFXLEffectParam         mVertex1Color[eRenderCommandTotal];
      BFXLEffectParam         mVertex2Color[eRenderCommandTotal];
      BFXLEffectParam         mVertex3Color[eRenderCommandTotal];
      BFXLEffectParam         mVertex4Color[eRenderCommandTotal];
      BFXLEffectParam         mTintColor[eRenderCommandTotal];

      BFXLEffectParam         mPalletteToggle[eRenderCommandTotal];
      BFXLEffectParam         mPalletteColorCount[eRenderCommandTotal];     
      
      BFXLEffectParam         mLightBufferSampler[eRenderCommandTotal];     
      BFXLEffectParam         mLightBufferingEnabled[eRenderCommandTotal];     
      BFXLEffectParam         mLightBufferIntensityScale[eRenderCommandTotal];     
      BFXLEffectParam         mWorldToLightBufCols[eRenderCommandTotal];     

      BFXLEffectParam         mHeightfieldSampler[eRenderCommandTotal];
      BFXLEffectParam         mWorldToHeightfieldParam[eRenderCommandTotal];
      BFXLEffectParam         mHeightfieldYScaleOfsParam[eRenderCommandTotal];
      BFXLEffectParam         mTerrainDecalYOffsetParam[eRenderCommandTotal];

      BFXLEffectParam         mSoftParticleParams[eRenderCommandTotal];
      BFXLEffectParam         mSoftParticleParams2[eRenderCommandTotal];

      BFXLEffectParam         mDepthTextureSampler[eRenderCommandTotal];
            
      UTBitVector<eFlagTotal> mFlags;

      // -- MAIN/WORKER
      void loadEffect();
      
      //-- MAIN
      void reloadInit();
      void reloadDeinit();

      //-- WORKER
      void initEffect(const BEvent& event);
      void render(const BParticleEmitterRenderCommand* RESTRICT pData, int effectIndex, int techniquePassIndex, int vertCount, int indexCount, D3DVertexDeclaration* pVertexDecl, int vertSize);
      void renderAABB(XMVECTOR min, XMVECTOR max);


      void setRenderStatesForPass(eRenderBlendMode blendMode);
      int  computeTechniquePass(const BParticleEmitterRenderCommand* pCommand);
      
      //Listener interface
      // init will be called from the worker thread after the D3D device is initialized.
      virtual void initDeviceData(void);

      // Called from worker thread.
      virtual void frameBegin(void);

      // Called from the worker thread to process commands.
      virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);

      // Called from worker thread.
      virtual void frameEnd(void);

      // deinit will be called from the worker thread before the RCL is freed, but always before the D3D device is release.
      virtual void deinitDeviceData(void);      
};
extern BParticleRenderer gParticleRenderer;

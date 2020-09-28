//============================================================================
// ParticleEffectData.h
// Copyright (c) 2006 Ensemble Studios
//============================================================================
#pragma once

#include "progression.h"
#include "containers\staticArray.h"

#pragma warning(disable:4324)  // C4324: structure was padded due to __declspec(align())

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BEmitterBaseData
{
public:
   BEmitterBaseData()
   {
      clear();
   }

   ~BEmitterBaseData() 
   {
   }

   int getMemoryCost();

   void clear(void)
   {     
      // Be sure to clear all members to something valid!
      mUpdateRadius = 10.0f;
      mMaxParticles = 1000;
      mMaxParticlesVar = 0.0f;
      mParticleLife = 1.0f;
      mParticleLifeVar = 0.0f;
      mGlobalFadeIn = 0.0f;
      mGlobalFadeInVar = 0.0f;
      mGlobalFadeOut = 0.0f;
      mGlobalFadeOutVar = 0.0f;

      mEmissionRate =100.0f;
      mEmissionRateVar = 0.0f;
      mStartDelay = 0.0f;
      mStartDelayVar = 0.0f;
      mInitialUpdate = 0.0f;
      mInitialUpdateVar = 0.0f;
      mEmissionTime = 1.0f;
      mEmissionTimeVar = 0.0f;
      mLoopDelay = 0.0f;
      mLoopDelayVar = 0.0f;
      mInitialDistance = 0.0f;
      mInitialDistanceVar = 0.0f;
      mVelocity = 1.0f;
      mVelocityVar = 0.0f;
      mAcceleration = 0.0f;
      mAccelerationVar = 0.0f;
      mTrailSegmentLength = 0.0f;
      mEmitterAttraction = 1.0f;
      mEmitterAttractionVar = 0.0f;
      mCollisionEnergyLoss = 0.0f;
      mCollisionEnergyLossVar = 0.0f;
      mCollisionOffset = 0.0f;


      mTiedToEmitter = false;
      mIgnoreRotation = false;
      mLoop = false;
      mAlwaysActive = false;
      mAlwaysRender = false;
      mKillImmediatelyOnRelease = false;
      
      mType = eBillBoard;
      mBlendMode = eAlphaBlend;
      mTrailEmissionType = eEmitByLength;
      mTrailUVType = eStretch;
      mBeamAlignmentType = eBeamAlignToCamera;

      mBeamTangent1 = XMVectorZero();
      mBeamTangent2 = XMVectorZero();
      mBeamTesselation = 1;

      mBeamColorByLength = false;
      mBeamOpacityByLength = false;      
      mBeamIntensityByLength = false;
      mCollisionDetectionTerrain = false;
      mSortParticles = false;
      mFillOptimized = false;
      mSoftParticles = false;
      
      mLightBuffer = false;
      mLightBufferValueLoaded = false;
      mLightBufferIntensityScale = 1.0f;

      mTerrainDecalTesselation = 1.0f;
      mTerrainDecalYOffset = 0.125f;
      mSoftParticleFadeRange = 1.0f;
   }        

   bool load(BXMLNode node, BXMLReader* pReader);
   
   void deInit() 
   { 
   }

   enum EmitterParticleTypeEnum
   {
      eBillBoard = 0,
      eOrientedAxialBillboard,
      eUpfacing,
      eTrail,
      eTrailCross,
      eBeam,
      eVelocityAligned,
      ePFX,
      eTerrainPatch,      
      eTypeTotal
   };

   enum EmitterBlendModeEnum 
   {
      eAlphaBlend = 0,
      eAdditive,
      eSubtractive,
      eDistortion,
      ePremultipliedAlpha,
      eBlendModeTotal
   };

   enum EmitterTrailEmissionType
   {
      eEmitByLength = 0,
      eEmitByTime,
      eTrailEmissionTypeTotal
   };

   enum EmitterTrailUVType
   {
      eStretch = 0,
      eFaceMap,
      eTrailUVTypeTotal
   };

   enum EmitterBeamAlignmentType
   {
      eBeamAlignToCamera,
      eBeamAlignVertical,
      eBeamAlignHorizontal,
      eBeamAlignTypeTotal
   };

   BFixedString128 mPFXFilepath;
   XMVECTOR mBeamTangent1;
   XMVECTOR mBeamTangent2;
   int      mBeamTesselation;
   

   float mUpdateRadius;
   int   mMaxParticles;
   float mMaxParticlesVar;

   float mParticleLife;
   float mParticleLifeVar;
   float mGlobalFadeIn;
   float mGlobalFadeInVar;
   float mGlobalFadeOut;
   float mGlobalFadeOutVar;

   float mEmissionRate;
   float mEmissionRateVar;
   float mStartDelay;
   float mStartDelayVar;
   float mInitialUpdate;
   float mInitialUpdateVar;
   float mEmissionTime;
   float mEmissionTimeVar;
   float mLoopDelay;
   float mLoopDelayVar;
   float mInitialDistance;
   float mInitialDistanceVar;
   float mVelocity;
   float mVelocityVar;
   float mAcceleration;
   float mAccelerationVar;

   float mTrailSegmentLength;

   float mEmitterAttraction;
   float mEmitterAttractionVar;

   float mCollisionEnergyLoss;
   float mCollisionEnergyLossVar;
   float mCollisionOffset;
   float mLightBufferIntensityScale;

   float mTerrainDecalTesselation;
   float mTerrainDecalYOffset;
   float mSoftParticleFadeRange;

   EmitterParticleTypeEnum  mType;
   EmitterBlendModeEnum     mBlendMode;
   EmitterTrailEmissionType mTrailEmissionType;
   EmitterTrailUVType       mTrailUVType;
   EmitterBeamAlignmentType mBeamAlignmentType;

   bool  mKillImmediatelyOnRelease;
   bool  mTiedToEmitter;
   bool  mIgnoreRotation;
   bool  mLoop;
   bool  mAlwaysActive;
   bool  mAlwaysRender;

   bool  mBeamColorByLength;
   bool  mBeamOpacityByLength;   
   bool  mBeamIntensityByLength;   
   bool  mCollisionDetectionTerrain;
   bool  mFillOptimized;
   bool  mSortParticles;       
   bool  mLightBuffer;
   bool  mLightBufferValueLoaded;
   bool  mSoftParticles;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BEmitterShapeData
{
   public:
      enum ShapeTypeEnum
      {
         ePoint = 0,
         eBox,
         eCylinder,
         eSphere,
         eHalfSphere,
         eRectangle,
         eCircle,
         eTypeTotal
      };
      
      BEmitterShapeData() { clear(); } 
     
      bool load(BXMLNode node, BXMLReader* pReader);
      void init();
      void deInit() { }

      int getMemoryCost();
      
      void clear(void)
      {
         mXSize = 0.0f;
         mYSize = 0.0f;
         mZSize = 0.0f;
         mXPosOffset = 0.0f;
         mYPosOffset = 0.0f;
         mZPosOffset = 0.0f;
         mTrajectoryInnerAngle = 0.0f;
         mTrajectoryOuterAngle = 0.0f;
         mTrajectoryPitch = 0.0f;
         mTrajectoryYaw = 0.0f;
         mTrajectoryBank = 0.0f;
         mEmitFromSurfaceRadius = 0.0f;
         mType = ePoint;
         mEmitFromSurface = false;
         mTopFactor = 0.0f;
         mSideFactor = 0.0f;
         mFrontFactor = 0.0f;
         mTotalFactor = 0.0f;
      }         

      float mXSize;
      float mYSize;
      float mZSize;
      float mXPosOffset;
      float mYPosOffset;
      float mZPosOffset;
      float mTrajectoryInnerAngle;
      float mTrajectoryOuterAngle;
      float mTrajectoryPitch;
      float mTrajectoryYaw;
      float mTrajectoryBank;
      float mEmitFromSurfaceRadius;
      ShapeTypeEnum mType;
      bool  mEmitFromSurface;

      //-- RunTime Data
      float mTopFactor;
      float mSideFactor;
      float mFrontFactor;
      float mTotalFactor;

};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BEmitterTextureStage
{
   public:
      BEmitterTextureStage() : mWeight(0.0f), mWidth(0), mHeight(0) {};
     ~BEmitterTextureStage(){};
      bool load(BXMLNode nNode, BXMLReader* pReader);

      int getMemoryCost();

      BString mFilename;
      float   mWeight;
      int     mWidth;
      int     mHeight;      
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BEmitterUVAnimation
{
   public:
      BEmitterUVAnimation() : mNumFrames(0), mFrameWidth(0), mFrameHeight(0), mFramesPerSecond(0), mUseUVAnimation(false), mScrollU(0.0f), mScrollV(0.0f), mUseRandomScrollOffsetU(false), mUseRandomScrollOffsetV(false) {};
     ~BEmitterUVAnimation(){};
      bool load(BXMLNode node, BXMLReader* pReader);

      int getMemoryCost();

      int    mNumFrames;
      int    mFrameWidth;
      int    mFrameHeight;
      float  mFramesPerSecond;
      bool   mUseUVAnimation;
      float  mScrollU;
      float  mScrollV;
      bool   mUseRandomScrollOffsetU;
      bool   mUseRandomScrollOffsetV;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BEmitterTextureSet
{
   public:
      BEmitterTextureSet(): mTotalWeight(1.0f), mInvTextureArraySize(0.0f), mTextureArrayIndex(-1), mTextureArrayNumUsed(0), mWidth(0), mHeight(0) { }
      
      bool load(BXMLNode node, BXMLReader* pReader);
      void deInit(void) { }

      int getMemoryCost();
                        
      BDynamicParticleArray<BEmitterTextureStage> mStages;
      BEmitterUVAnimation mUVAnimation;
      float mTotalWeight;
      
      float mInvTextureArraySize;      
      
      BStaticArray<uchar, 8> mTextureArraySlotIndices;
      
      short  mWidth;
      short  mHeight;
      
      short  mTextureArrayIndex;
      short  mTextureArrayNumUsed;
                                          
   private:
      bool init();
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BEmitterTextureData
{
   public:
      enum MultiTextureBlendTypeEnum
      {
         eBlendMultiply = 0,
         eBlendAlpha,         
         eBlendTotal
      };

      BEmitterTextureData(): mDiffuseLayer1To2BlendMode(eBlendMultiply), mDiffuseLayer2To3BlendMode(eBlendMultiply) {};
     ~BEmitterTextureData(){};
      bool load(BXMLNode node, BXMLReader* pReader);
      void deInit();
      
      int getMemoryCost();

      MultiTextureBlendTypeEnum mDiffuseLayer1To2BlendMode;
      MultiTextureBlendTypeEnum mDiffuseLayer2To3BlendMode;
      
      BEmitterTextureSet mDiffuse;
      BEmitterTextureSet mDiffuse2;
      BEmitterTextureSet mDiffuse3;
      BEmitterTextureSet mMasks;
      BEmitterTextureSet mIntensity;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BEmitterColorData
{
   public:
      enum ColorTypeEnum
      {
         eSingleColor = 0,
         ePalletteColor,
         eProgression,
         eTotal
      };
      BEmitterColorData() { clear(); }
     ~BEmitterColorData() { }
     
      bool load(BXMLNode node, BXMLReader* pReader);
      void deInit();

      int getMemoryCost();
      
      void clear(void)
      {
         mpProgression = NULL;
         Utils::ClearObj(mColor);
         mType = eSingleColor;
         mVertex1Color = XMVectorSplatOne();
         mVertex2Color = XMVectorSplatOne();
         mVertex3Color = XMVectorSplatOne();
         mVertex4Color = XMVectorSplatOne();
         mPlayerColorIntensity = XMVectorSplatOne();
         mPlayerColor = false;
         mSunColorIntensity = XMVectorSplatOne();
         mSunColor = false;
      }

      BColorProgression*            mpProgression;
      BDynamicParticleArray<BPalletteColor> mPallette;
      XMVECTOR                      mColor;
      XMVECTOR                      mVertex1Color;
      XMVECTOR                      mVertex2Color;
      XMVECTOR                      mVertex3Color;
      XMVECTOR                      mVertex4Color;
      XMVECTOR                      mPlayerColorIntensity;
      XMVECTOR                      mSunColorIntensity;
      ColorTypeEnum                 mType;
      bool                          mPlayerColor;
      bool                          mSunColor;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BEmitterFloatProgressionData
{
   public:
      BEmitterFloatProgressionData() { clear(); }
     
      bool load(BXMLNode node, BXMLReader* pReader);
      void deInit();

      int getMemoryCost();
      
      void clear(void)
      {
         mpProgression = NULL;
         mValue = 0;
         mValueVar = 0;
         mUseProgression = false;
      }
      
      BFloatProgression* mpProgression;
      float  mValue;
      float  mValueVar;
      bool   mUseProgression;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable:4324)
class BEmitterVectorProgressionData
{
   public:
      BEmitterVectorProgressionData() { clear(); }
     
      bool load(BXMLNode node, BXMLReader* pReader);
      void deInit();

      int getMemoryCost();
      
      void clear(void)
      {
         Utils::ClearObj(mValue);   
         Utils::ClearObj(mValueVar);
         mpProgression = NULL;
         mUseXProgression = false;
         mUseYProgression = false;
         mUseZProgression = false;
         mUseVarianceAndProgression = false;
      }

      void            initLookupTable(int entryCount);
      int             getLookupTableSize() const {return mLookupTable.getSize();};
      inline XMDHENN3 getLookupValue(int index) const { return mLookupTable[index]; } 
      
      XMVECTOR mValue;
      XMVECTOR mValueVar;
      BVectorProgression* mpProgression;      
      BDynamicParticleArray<XMDHENN3> mLookupTable;
      bool     mUseXProgression;
      bool     mUseYProgression;
      bool     mUseZProgression;
      bool     mUseVarianceAndProgression;
};
#pragma warning(pop)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BEmitterForceData
{
   public:
      BEmitterForceData() { clear(); }
     
      bool load(BXMLNode node, BXMLReader* pReader);
      void deInit() {}
      
      int getMemoryCost();

      void clear(void)
      {
         mMinAngularTumbleVelocity = 0;
         mMaxAngularTumbleVelocity = 0;
         mInternalGravity = 0;
         mInternalGravityVar = 0; 
         mInternalWindDirection = 0;
         mInternalWindDirectionVar = 0;
         mInternalWindSpeed = 0;
         mInternalWindSpeedVar = 0;
         mInternalWindDelay = 0;
         mInternalWindDelayVar = 0;

         mRandomOrientation = false;
         mTumbleBothDirections = false;
         mTumble = false;
         mUseInternalGravity = false;
         mUseInternalWind = false;
      }
      
      float mMinAngularTumbleVelocity;
      float mMaxAngularTumbleVelocity;
      float mInternalGravity;
      float mInternalGravityVar;
      float mInternalWindDirection;
      float mInternalWindDirectionVar;
      float mInternalWindSpeed;
      float mInternalWindSpeedVar;
      float mInternalWindDelay;
      float mInternalWindDelayVar;

      bool  mRandomOrientation;
      bool  mTumbleBothDirections;
      bool  mTumble;
      bool  mUseInternalGravity;
      bool  mUseInternalWind;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BMagnetData
{
   public:
      BMagnetData() { clear(); }
     
      bool load(BXMLNode node, BXMLReader* pReader);
      void deInit() {}

      int getMemoryCost();

      enum MagnetTypeEnum
      {
         eSphere = 0,
         eCylinder,
         eMagnetTypeTotal
      };
      
      void clear(void)
      {
         mForce = 0.0f;
         mRadius = 0.0f;
         mRotationalForce = 0.0f;
         mHeight = 0.0f;
         mTurbulence = 0.0f;
         mDampening = 0.0f;
         mPosOffset = XMVectorZero();
         mRotation  = XMMatrixIdentity();
      }
      
      XMMATRIX mRotation;
      XMVECTOR mPosOffset;
      float    mForce;
      float    mRotationalForce;
      float    mRadius;      
      float    mHeight;
      float    mTurbulence;
      float    mDampening;
      MagnetTypeEnum  mType;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BParticleEmitterDefinition
{
   public:
      BParticleEmitterDefinition() { clear(); }
     
      bool load(BXMLNode node, BXMLReader* pReader);
      void deInit();
      void computeBBoxOffset();
      void computeMaxEmitterLife();

      int getMemoryCost();
      
      void clear(void)
      {
         Utils::ClearObj(mProgressionV);
         Utils::ClearObj(mBBoxOffset);
         mMaxEmitterLife = 0;
      }
   
      BDynamicParticleArray<BMagnetData*> mMagnets;
      BEmitterBaseData                    mProperties;
      BEmitterShapeData                   mShape;
      BEmitterTextureData                 mTextures;
      BEmitterFloatProgressionData        mOpacity;
      BEmitterVectorProgressionData       mScale;
      BEmitterVectorProgressionData       mSpeed;
      BEmitterFloatProgressionData        mIntensity;
      BEmitterColorData                   mColor;
      BEmitterForceData                   mForce;

#ifndef BUILD_FINAL
      BString                             mParentPfxName;
#endif

      XMVECTOR                            mProgressionV; //-- V coordinates for the texture progressions
      XMVECTOR                            mBBoxOffset;   //-- offset to compute bboxes with efficiently
      float                               mMaxEmitterLife;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BParticleEffectDefinition
{
   public: 
      BParticleEffectDefinition() : mRefCount(0) {};
     ~BParticleEffectDefinition() {};

      int getMemoryCost();
      
      void deInit();
      bool load(const BString& fileName);

      void addRef(void) { mRefCount++; }
      void decRef(void) { mRefCount--; mRefCount = Math::Max((uint) 0, mRefCount); }
      uint getRefCount(void) { return mRefCount; };

   BDynamicParticleArray<BParticleEmitterDefinition*> mEmitters;
   BString mName;
   uint    mRefCount;
};

#pragma warning(default:4324)  

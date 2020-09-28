//============================================================================
//
//  ParticleSystemData.h
//
//  Copyright (c) 1999-2001 Ensemble Studios
//
//============================================================================


#ifndef __PARTICLE_SYSTEM_DATA_H__
#define __PARTICLE_SYSTEM_DATA_H__

//----------------------------------------------------------------------------
//  Particle System Data Version
//----------------------------------------------------------------------------
const long gPSDataVersion = 13;


//----------------------------------------------------------------------------
//  Particle System Enums
//----------------------------------------------------------------------------
enum PSParticleAppearance
{
   PSPA_INVALID = -1,
   PSPA_BILLBOARD,
   PSPA_AXIAL_BILLBOARD,
   PSPA_UP_FACING,
   PSPA_3D_SOMETHING,
   PSPA_TRAIL,
   PSPA_TRAIL_STRETCH,
   PSPA_TOTAL
};

enum PSEmitterShape
{
   PSES_INVALID = -1,
   PSES_POINT,
   PSES_BOX_SOLID,
   PSES_BOX_SURFACE,
   PSES_BOX_HOLLOW,
   PSES_ELLIPSOID_SOLID,
   PSES_ELLIPSOID_SURFACE,
   PSES_ELLIPSOID_HOLLOW,
   PSES_HALF_ELLIPSOID_SOLID,
   PSES_HALF_ELLIPSOID_SURFACE,
   PSES_HALF_ELLIPSOID_HOLLOW,
   PSES_RECTANGLE,
   PSES_CIRCLE,
   PSES_CIRCLE_XY,
   PSES_TOTAL
};

enum PSMaterialSettings
{
   PSMS_INVALID = -1,
   PSMS_NORMAL,
   PSMS_ADDITIVE,
   PSMS_SUBTRACTIVE,
   PSMS_TOTAL
};

enum PSTerrainInteraction
{
   PSTI_INVALID = -1,
   PSTI_FIXED,
   PSTI_MIN,
   PSTI_MAX,
   PSTI_TOTAL
};

enum PSCollisionResult
{
   PSCR_INVALID = -1,
   PSCR_DIE,
   PSCR_LINGER,
   PSCR_BOUNCE,
   PSCR_TOTAL
};


//----------------------------------------------------------------------------
//  Particle System Structs
//----------------------------------------------------------------------------
struct PSOpacityStage
{
   float opacity;
   float opacityVar;
   float hold;
   float fade;
};

struct PSScaleStage
{
   float scale;
   float scaleVar;
   float hold;
   float fade;
};

struct PSSpeedStage
{
   float speed;
   float speedVar;
   float hold;
   float fade;
};

struct PSColorStage
{
   bool  usePalette;
   DWORD color;
   float hold;
   float fade;
};

struct PSIntensityStage
{
   float intensity;
   float intensityVar;
   float hold;
   float fade;
};

struct PSCollisionType
{
   bool      spawnSystem;
   bool      collideTerrain;
   bool      collideWater;
   bool      collideUnits;
   long      result;
   long      numFileNames;
   float     lingerTime;
   float     lingerTimeVar;
   float     fadeTime;
   float     fadeTimeVar;
   float     energyLoss;
   float     energyLossVar;
   BSimString  name;
   BSimString* pFileNames;

   PSCollisionType& operator = (const PSCollisionType& type)
   {
      if (this == &type)
      {
         BASSERT(0);
         return *this;
      }
      spawnSystem    = type.spawnSystem;
      collideTerrain = type.collideTerrain;
      collideWater   = type.collideWater;
      collideUnits   = type.collideUnits;
      result         = type.result;
      numFileNames   = type.numFileNames;
      lingerTime     = type.lingerTime;
      lingerTimeVar  = type.lingerTimeVar;
      fadeTime       = type.fadeTime;
      fadeTimeVar    = type.fadeTimeVar;
      energyLoss     = type.energyLoss;
      energyLossVar  = type.energyLossVar;
      name           = type.name;
      pFileNames     = new BSimString [numFileNames];
      for (long file = 0; file < numFileNames; ++file)
         pFileNames[file] = type.pFileNames[file];
      return *this;
   }
};

struct PSParticleEvent
{
   float     effectStart;
   float     effectStartVar;
   bool      systemLinger;
   BSimString   filename;

   PSParticleEvent& operator = (const PSParticleEvent& type)
   {
      if (this == &type)
      {
         BASSERT(0);
         return *this;
      }
      effectStart    = type.effectStart;
      effectStartVar = type.effectStartVar;
      systemLinger   = type.systemLinger;
      filename       = type.filename;
      return *this;
   }
};

//-- Emitter Settings
class BEmitterData
{
   public:
      BEmitterData(){};
     ~BEmitterData(){};

   BEmitterData& operator=(const BEmitterData& d)
   {
      memcpy(this, &d, sizeof(BEmitterData));
      return *this;
   }

   bool  mTiedToEmitter;
   bool  mIgnoreRotation;
   bool  mEmitByMotion;
   bool  mLoop;
   bool  mInheritVelocity;
   bool  mUseMinVelocity;
   bool  mUseMaxVelocity;
   bool  mAlwaysActive;
   bool  mSyncWithAttackAnim;
   bool  mCastShadows;
   bool  mOrthoProj;
   long  mMaxParticles;
   long  mAppearanceType;
   float mUpdateRadius;
   float mMaxParticlesVar;
   float mParticleLife;
   float mParticleLifeVar;
   float mGlobalFadeIn;
   float mGlobalFadeInVar;
   float mGlobalFadeOut;
   float mGlobalFadeOutVar;
   float mEmitDistance;
   float mEmitDistanceVar;
   float mEmissionRate;
   float mEmissionRateVar;
   float mInitialDormancy;
   float mInitialDormancyVar;
   float mInitialUpdate;
   float mInitialUpdateVar;
   float mEmissionTime;
   float mEmissionTimeVar;
   float mDormantTime;
   float mDormantTimeVar;
   float mInitialDistance;
   float mInitialDistanceVar;
   float mInitialVelocity;
   float mInitialVelocityVar;
   float mAcceleration;
   float mAccelerationVar;
   float mInheritInfluence;
   float mInheritInfluenceVar;
   float mMinVelocity;
   float mMinVelocityVar;
   float mMaxVelocity;
   float mMaxVelocityVar;
};

//-- Emitter Shape
class BEmitterShape
{
   public: 
      bool  mStartFull;
      bool  mEmitAwayFromBias;
      bool  mUseSpreader;
      bool  mForceRiverFlowEmission;
      long  mShapeType;
      float mOuterXRadius;
      float mInnerXRadius;
      float mOuterYRadius;
      float mInnerYRadius;
      float mOuterZRadius;
      float mInnerZRadius;
      float mCenterHeight;
      float mOffAxis;
      float mOffAxisSpread;
      float mOffPlane;
      float mOffPlaneSpread;
      float mBiasPointHeight;
};

//-- Appearance
class BEmitterAppearance
{
   public :
      BEmitterAppearance() {};
     ~BEmitterAppearance() {};
   
   long  mNumFiles;
   long  mNumFrames;
   long  mFrameWidth;
   long  mFrameHeight;
   long  mMaterialType;
   DWORD mEmissive;
   DWORD mSpecular;
   float mSpecularExponent;
   float mFramesPerSecond;
   float mAnimationRate;
   float mAnimationRateVar;
   bool  mOrientByMotion;
};

//-- Depth Biasing
class BEmitterDepthBias
{
   public : 
      BEmitterDepthBias(){};
     ~BEmitterDepthBias(){};

   long  mNumFiles;
   long  mNumFrames;
   long  mFrameWidth;
   long  mFrameHeight;
   float mFramesPerSecond;
   float mZScale;
};

//-- Opacity
class BEmitterOpacity
{
   public: 
      BEmitterOpacity(){};
     ~BEmitterOpacity(){};

   bool  mLoopingCycle;
   long  mNumStages;
   float mOpacity;
   float mOpacityVar;
   float mCycleTime;
   float mCycleTimeVar;
};

//-- Scale
class BEmitterScale
{
   public:
      BEmitterScale() {};
     ~BEmitterScale() {};

   long  mNumStages;
   float mScale;
   float mScaleVar;
   float mXScale;
   float mXScaleVar;
   float mYScale;
   float mYScaleVar;
   float mZScale;
   float mZScaleVar;
   float mCycleTime;
   float mCycleTimeVar;
   bool  mLoopingCycle;
};

//-- Speed
class BEmitterSpeed
{
   public:
      BEmitterSpeed():
         mSpeed(1.0f),
         mXSpeed(1.0f),
         mYSpeed(1.0f),
         mZSpeed(1.0f)
      {
      };

     ~BEmitterSpeed() {};

   bool  mLoopingCycle;
   long  mNumStages;
   float mSpeed;
   float mSpeedVar;
   float mXSpeed;
   float mXSpeedVar;
   float mYSpeed;
   float mYSpeedVar;
   float mZSpeed;
   float mZSpeedVar;
   float mCycleTime;
   float mCycleTimeVar;
};

//-- Color
class BEmitterColor
{
   public:
      BEmitterColor() {};
     ~BEmitterColor() {};

   bool  mUsePalette;
   bool  mLoopingCycle;
   long  mNumPaletteColors;
   long  mNumStages;
   float mCycleTime;
   float mCycleTimeVar;
   float mfWorldLightingInfluence;
   DWORD mColor;
};

//-- Intensity
class BEmitterIntensity
{
   public: 
      BEmitterIntensity() {};
     ~BEmitterIntensity() {};

   bool  mLoopingCycle;
   long  mNumStages;
   float mIntensity;
   float mIntensityVar;
   float mCycleTime;
   float mCycleTimeVar;
};

//-- Forces
class BEmitterForces
{      
   public:
      BEmitterForces(){};
     ~BEmitterForces(){};

   float mInternalGravity;
   float mInternalGravityVar;
   float mInternalWindDirection;
   float mInternalWindDirectionVar;
   float mInternalWindSpeed;
   float mInternalWindSpeedVar;
   float mInternalWindDelay;
   float mInternalWindDelayVar;
   float mExternalWindInfluence;
   float mExternalWindInfluenceVar;
   float mExternalWindDelay;
   float mExternalWindDelayVar;
   float mMinAngularVelocity;
   float mMaxAngularVelocity;
   float mXAxis;
   float mXAxisVar;
   float mYAxis;
   float mYAxisVar;
   float mZAxis;
   float mZAxisVar;

   bool  mRandomOrientation;
   bool  mTumble;
   bool  mTumbleBothDirections;
   bool  mRandomAxis;
   bool  mUseWind;
   bool  mUseGravity;
};

//-- Collision
class BEmitterCollision
{
   public:
      BEmitterCollision() {};
     ~BEmitterCollision() {};

   long  mNumTypes;
   long  mTerrainInteractionType;
   float mTerrainHeight;
   float mTerrainHeightVar;
   float mTerrainDampenFactor;
};

class BEmitterEvents
{
   public:
      BEmitterEvents(){};
     ~BEmitterEvents(){};
   long mNumEvents;
};


//----------------------------------------------------------------------------
//  Class BParticleSystemData
//----------------------------------------------------------------------------
class BParticleSystemData
{
public:
   //-- Construction/Destruction
    BParticleSystemData();
   ~BParticleSystemData();
    BParticleSystemData(const BParticleSystemData& source);

   //-- Interface
   void  init                (void);
   void  clear               (void);
   bool  save                (const BSimString& fileName);
   bool  saveXML             (const BSimString& fileName);
   bool  reload              ();
   bool  load                (const BSimString& fileName);
   bool  loadXML             (const BSimString& fileName);
   void  setupEmitterShape   (void);
   float getMaxParticleRadius(void);


   BEmitterData       mEmitter;
   BEmitterShape      mShape;
   BEmitterAppearance mAppearance;
   BEmitterDepthBias  mDepthBiasing;
   BEmitterOpacity    mOpacity;
   BEmitterScale      mScale;
   BEmitterSpeed      mSpeed;
   BEmitterColor      mColor;
   BEmitterIntensity  mIntensity;
   BEmitterForces     mForces;
   BEmitterCollision  mCollision;
   BEmitterEvents     mEvents;

   //-- Run Time Data
   BSimString  mName;
   long     mVersion;
   long     mRefCount;
   DWORD    mTextureWidth;
   DWORD    mTextureHeight;
   float    mAppearanceFileWeight;      
   float    mTopFactor;
   float    mSideFactor;
   float    mFrontFactor;
   float    mTotalFactor;
   int*     mpAppFileIndices;   
      
   //-- Complex Storage
   DWORD*               mpPaletteColors;
   float*               mpAppearanceWeights;
   BSimString*             mpAppearanceFiles;
   PSOpacityStage*      mpOpacityStages;
   PSScaleStage*        mpScaleStages;
   PSSpeedStage*        mpSpeedStages;
   PSColorStage*        mpColorStages;
   PSCollisionType*     mpCollisionTypes;
   PSParticleEvent*     mpParticleEvents;
   PSIntensityStage*    mpIntensityStages;

   //-- Operators
   BParticleSystemData& operator = (const BParticleSystemData& data);

private:
   //-- Private Functions
   bool validate      ();
   bool loadMaterials ();
   bool load3DModels  ();
};


#endif


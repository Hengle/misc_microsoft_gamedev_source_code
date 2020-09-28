//============================================================================
//
//  ParticleSystemData.cpp
//
//  Copyright (c) 1999-2006 Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xparticlescommon.h"
#include "particleworkdirsetup.h"
#include "particletexturemanager.h"
#include "xmlreader.h"

//============================================================================
//  MACROS
//============================================================================
#define WRITE(var)             if (!file.write(&var, sizeof(var))) goto Error;
#define WRITE_ARRAY(var, size) if (!file.write( var, size       )) goto Error;
#define READ(var)              if (!file.read (&var, sizeof(var))) goto Error;
#define READ_ARRAY(var, size)  if (!file.read ( var, size)) goto Error;


//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
BParticleSystemData::BParticleSystemData():
   mpPaletteColors(NULL),
   mpAppearanceWeights(NULL),
   mpAppearanceFiles(NULL),
   mpOpacityStages(NULL),
   mpScaleStages(NULL),
   mpColorStages(NULL),
   mpCollisionTypes(NULL),
   mpSpeedStages(NULL),
   mpParticleEvents(NULL),
   mpIntensityStages(NULL),
   mpAppFileIndices(NULL)
{
   //-- Simple Storage
   Utils::ClearObj(mEmitter);
   Utils::ClearObj(mShape);
   Utils::ClearObj(mAppearance);
   Utils::ClearObj(mOpacity);
   Utils::ClearObj(mScale);
   Utils::ClearObj(mColor);
   Utils::ClearObj(mForces);
   Utils::ClearObj(mCollision);
   Utils::ClearObj(mSpeed);
   Utils::ClearObj(mEvents);
   Utils::ClearObj(mIntensity);

   mSpeed.mSpeed  = 1.0f;
   mSpeed.mXSpeed = 1.0f;
   mSpeed.mYSpeed = 1.0f;
   mSpeed.mZSpeed = 1.0f;

   mIntensity.mIntensity = 1.0f;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleSystemData::~BParticleSystemData()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleSystemData::BParticleSystemData(const BParticleSystemData& source)
{
   *this = source;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemData::init()
{
   mpPaletteColors = NULL;
   mpAppearanceWeights= NULL;
   mpAppearanceFiles= NULL;
   mpOpacityStages= NULL;
   mpScaleStages= NULL;
   mpColorStages= NULL;
   mpCollisionTypes= NULL;
   mpSpeedStages= NULL;
   mpParticleEvents= NULL;
   mpIntensityStages= NULL;
   mpAppFileIndices= NULL;

   mName.empty();
   mVersion              = gPSDataVersion;
   mRefCount             = 0;
   mTextureWidth         = 0;
   mTextureHeight        = 0;
   mAppearanceFileWeight = 0.0f;
   mTopFactor            = 0.0f;
   mSideFactor           = 0.0f;
   mFrontFactor          = 0.0f;
   mTotalFactor          = 0.0f;
   
   //-- Simple Storage
   Utils::ClearObj(mEmitter);
   Utils::ClearObj(mShape);
   Utils::ClearObj(mAppearance);
   Utils::ClearObj(mOpacity);
   Utils::ClearObj(mScale);
   Utils::ClearObj(mColor);
   Utils::ClearObj(mForces);
   Utils::ClearObj(mCollision);
   Utils::ClearObj(mSpeed);
   Utils::ClearObj(mEvents);
   Utils::ClearObj(mIntensity);

   mSpeed.mSpeed  = 1.0f;
   mSpeed.mXSpeed = 1.0f;
   mSpeed.mYSpeed = 1.0f;
   mSpeed.mZSpeed = 1.0f;

   mIntensity.mIntensity = 1.0f;
}

//============================================================================
//  INTERFACE
//============================================================================
void BParticleSystemData::clear()
{
   //-- Release memory.
   if (mpCollisionTypes)
   {
      for (long type = 0; type < mCollision.mNumTypes; ++type)
         delete [] mpCollisionTypes[type].pFileNames;
   }
   if (mpAppFileIndices)    delete [] mpAppFileIndices;
   if (mpPaletteColors)     delete [] mpPaletteColors;
   if (mpAppearanceWeights) delete [] mpAppearanceWeights;
   if (mpAppearanceFiles)   delete [] mpAppearanceFiles;
   if (mpOpacityStages)     delete [] mpOpacityStages;
   if (mpScaleStages)       delete [] mpScaleStages;
   if (mpColorStages)       delete [] mpColorStages;
   if (mpCollisionTypes)    delete [] mpCollisionTypes;
   if (mpSpeedStages)       delete [] mpSpeedStages;
   if (mpParticleEvents)    delete [] mpParticleEvents;
   if (mpIntensityStages)   delete [] mpIntensityStages;   

   init();

   /*
   //-- Run Time Data
   mName.empty();
   mVersion              = gPSDataVersion;
   mRefCount             = 0;
   mTextureWidth         = 0;
   mTextureHeight        = 0;
   mAppearanceFileWeight = 0.0f;
   mTopFactor            = 0.0f;
   mSideFactor           = 0.0f;
   mFrontFactor          = 0.0f;
   mTotalFactor          = 0.0f;
   mpAppFileIndices      = NULL;

   //-- Simple Storage
   Utils::ClearObj(mEmitter);
   Utils::ClearObj(mShape);
   Utils::ClearObj(mAppearance);
   Utils::ClearObj(mOpacity);
   Utils::ClearObj(mScale);
   Utils::ClearObj(mColor);
   Utils::ClearObj(mForces);
   Utils::ClearObj(mCollision);
   Utils::ClearObj(mSpeed);
   Utils::ClearObj(mEvents);
   Utils::ClearObj(mIntensity);

   mSpeed.mSpeed  = 1.0f;
   mSpeed.mXSpeed = 1.0f;
   mSpeed.mYSpeed = 1.0f;
   mSpeed.mZSpeed = 1.0f;

   mIntensity.mIntensity = 1.0f;   

   //-- Complex Storage
   mpPaletteColors     = NULL;
   mpAppearanceWeights = NULL;
   mpAppearanceFiles   = NULL;
   mpOpacityStages     = NULL;
   mpScaleStages       = NULL;
   mpColorStages       = NULL;
   mpCollisionTypes    = NULL;
   mpSpeedStages       = NULL;
   mpParticleEvents    = NULL;
   mpIntensityStages   = NULL;
   */
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemData::save(const BSimString& fileName)
{
   // save out the XML first
   return saveXML( fileName );
}


void XMLHelper( BXMLDocument &doc, BXMLNode *pNode, const BSimString &name, const long value)
{
   BSimString output;
   output.setToLong( value );
   BXMLNode *pValue = doc.createNode( name, pNode); 
   BASSERT(pValue);
   pValue->setText( output );
}

void XMLHelper( BXMLDocument &doc, BXMLNode *pNode, const BSimString &name, const float value)
{
   BSimString output;
   output.setToFloat( value );
   BXMLNode *pValue = doc.createNode( name, pNode); 
   BASSERT(pValue);
   pValue->setText( output );
}

void XMLHelper( BXMLDocument &doc, BXMLNode *pNode, const BSimString &name, const BSimString& value)
{
   BXMLNode *pValue = doc.createNode( name, pNode); 
   BASSERT(pValue);
   pValue->setText( value );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemData::saveXML(const BSimString& filename)
{
#if 0
   //-- create the XML tree
   BXMLDocument doc;
   BXMLNode *pNode = doc.createRootNode(B("ParticleFile"));
   if (!pNode)
      return (false);

   BXMLNode *pKid = doc.createNode(B("Emitter"), pNode);
   if (!pKid)
      return false;
   
   XMLHelper( doc, pKid, B("TiedToEmitter"),  (long)mEmitter.mTiedToEmitter);
   XMLHelper( doc, pKid, B("IgnoreRotation"), (long)mEmitter.mIgnoreRotation );
   XMLHelper( doc, pKid, B("EmitByMotion"), (long)mEmitter.mEmitByMotion );
   XMLHelper( doc, pKid, B("Loop"), (long)mEmitter.mLoop );
   XMLHelper( doc, pKid, B("InheritVelocity"), (long)mEmitter.mInheritVelocity );
   XMLHelper( doc, pKid, B("UseMinVelocity"), (long)mEmitter.mUseMinVelocity );
   XMLHelper( doc, pKid, B("UseMaxVelocity"), (long)mEmitter.mUseMaxVelocity );
   XMLHelper( doc, pKid, B("AlwaysActive"), (long)mEmitter.mAlwaysActive );
   XMLHelper( doc, pKid, B("CastShadows"), (long)mEmitter.mCastShadows );
   XMLHelper( doc, pKid, B("SyncWithAttackAnim"), (long)mEmitter.mSyncWithAttackAnim );
   XMLHelper( doc, pKid, B("MaxParticles"), mEmitter.mMaxParticles );
   XMLHelper( doc, pKid, B("AppearanceType"), mEmitter.mAppearanceType );
   XMLHelper( doc, pKid, B("UpdateRadius"), mEmitter.mUpdateRadius );
   XMLHelper( doc, pKid, B("MaxParticlesVar"), mEmitter.mMaxParticlesVar );
   XMLHelper( doc, pKid, B("ParticleLife"), mEmitter.mParticleLife );
   XMLHelper( doc, pKid, B("ParticleLifeVar"), mEmitter.mParticleLifeVar );
   XMLHelper( doc, pKid, B("GlobalFadeIn"), mEmitter.mGlobalFadeIn );
   XMLHelper( doc, pKid, B("GlobalFadeInVar"), mEmitter.mGlobalFadeInVar );
   XMLHelper( doc, pKid, B("GlobalFadeOut"), mEmitter.mGlobalFadeOut );
   XMLHelper( doc, pKid, B("GlobalFadeOutVar"), mEmitter.mGlobalFadeOutVar );
   XMLHelper( doc, pKid, B("EmitDistance"), mEmitter.mEmitDistance );
   XMLHelper( doc, pKid, B("EmitDistanceVar"), mEmitter.mEmitDistanceVar );
   XMLHelper( doc, pKid, B("EmissionRate"), mEmitter.mEmissionRate );
   XMLHelper( doc, pKid, B("EmissionRateVar"), mEmitter.mEmissionRateVar );
   XMLHelper( doc, pKid, B("InitialDormancy"), mEmitter.mInitialDormancy );
   XMLHelper( doc, pKid, B("InitialDormancyVar"), mEmitter.mInitialDormancyVar );
   XMLHelper( doc, pKid, B("InitialUpdate"), mEmitter.mInitialUpdate );
   XMLHelper( doc, pKid, B("InitialUpdateVar"), mEmitter.mInitialUpdateVar );
   XMLHelper( doc, pKid, B("EmissionTime"), mEmitter.mEmissionTime );
   XMLHelper( doc, pKid, B("EmissionTimeVar"), mEmitter.mEmissionTimeVar );
   XMLHelper( doc, pKid, B("DormantTime"), mEmitter.mDormantTime );
   XMLHelper( doc, pKid, B("DormantTimeVar"), mEmitter.mDormantTimeVar );
   XMLHelper( doc, pKid, B("InitialDistance"), mEmitter.mInitialDistance );
   XMLHelper( doc, pKid, B("InitialDistanceVar"), mEmitter.mInitialDistanceVar );
   XMLHelper( doc, pKid, B("InitialVelocity"), mEmitter.mInitialVelocity );
   XMLHelper( doc, pKid, B("InitialVelocityVar"), mEmitter.mInitialVelocityVar );
   XMLHelper( doc, pKid, B("Acceleration"), mEmitter.mAcceleration );
   XMLHelper( doc, pKid, B("AccelerationVar"), mEmitter.mAccelerationVar );
   XMLHelper( doc, pKid, B("InheritInfluence"), mEmitter.mInheritInfluence );
   XMLHelper( doc, pKid, B("InheritInfluenceVar"), mEmitter.mInheritInfluenceVar );
   XMLHelper( doc, pKid, B("MinVelocity"), mEmitter.mMinVelocity );
   XMLHelper( doc, pKid, B("MinVelocityVar"), mEmitter.mMinVelocityVar );
   XMLHelper( doc, pKid, B("MaxVelocity"), mEmitter.mMaxVelocity );
   XMLHelper( doc, pKid, B("MaxVelocityVar"), mEmitter.mMaxVelocityVar );

   pKid = doc.createNode(B("Shape"), pNode);
   if (!pKid)
      return false;
   
   XMLHelper( doc, pKid, B("StartFull"), (long)mShape.mStartFull );
   XMLHelper( doc, pKid, B("EmitAwayFromBias"), (long)mShape.mEmitAwayFromBias );
   XMLHelper( doc, pKid, B("UseSpreader"), (long)mShape.mUseSpreader );
   XMLHelper( doc, pKid, B("ShapeType"), mShape.mShapeType );
   XMLHelper( doc, pKid, B("OuterXRadius"), mShape.mOuterXRadius );
   XMLHelper( doc, pKid, B("InnerXRadius"), mShape.mInnerXRadius );
   XMLHelper( doc, pKid, B("OuterYRadius"), mShape.mOuterYRadius );
   XMLHelper( doc, pKid, B("InnerYRadius"), mShape.mInnerYRadius );
   XMLHelper( doc, pKid, B("OuterZRadius"), mShape.mOuterZRadius );
   XMLHelper( doc, pKid, B("InnerZRadius"), mShape.mInnerZRadius );
   XMLHelper( doc, pKid, B("CenterHeight"), mShape.mCenterHeight );
   XMLHelper( doc, pKid, B("OffAxis"), mShape.mOffAxis );
   XMLHelper( doc, pKid, B("OffAxisSpread"), mShape.mOffAxisSpread );
   XMLHelper( doc, pKid, B("OffPlane"), mShape.mOffPlane );
   XMLHelper( doc, pKid, B("OffPlaneSpread"), mShape.mOffPlaneSpread );
   XMLHelper( doc, pKid, B("BiasPointHeight"), mShape.mBiasPointHeight );
   XMLHelper( doc, pKid, B("ForceRiverFlowEmission"), (long)mShape.mForceRiverFlowEmission );

   pKid = doc.createNode(B("Appearance"), pNode);
   if (!pKid)
      return false;
   
   XMLHelper( doc, pKid, B("OrientByMotion"), (long)mAppearance.mOrientByMotion );
   XMLHelper( doc, pKid, B("NumFiles"), mAppearance.mNumFiles );
   XMLHelper( doc, pKid, B("NumFrames"), mAppearance.mNumFrames );
   XMLHelper( doc, pKid, B("FrameWidth"), mAppearance.mFrameWidth );
   XMLHelper( doc, pKid, B("FrameHeight"), mAppearance.mFrameHeight );
   XMLHelper( doc, pKid, B("MaterialType"), mAppearance.mMaterialType );
   XMLHelper( doc, pKid, B("Emissive"), (long)mAppearance.mEmissive );
   XMLHelper( doc, pKid, B("Specular"), (long)mAppearance.mSpecular );
   XMLHelper( doc, pKid, B("SpecularExponent"), mAppearance.mSpecularExponent );
   XMLHelper( doc, pKid, B("FramesPerSecond"), mAppearance.mFramesPerSecond );
   XMLHelper( doc, pKid, B("AnimationRate"), mAppearance.mAnimationRate );
   XMLHelper( doc, pKid, B("AnimationRateVar"), mAppearance.mAnimationRateVar );

   pKid = doc.createNode(B("AppearanceWeights"), pNode);
   if (!pKid)
      return false;
   
   for (long j = 0; j < mAppearance.mNumFiles; j++)
   {
      BXMLNode *pGrandKid = doc.createNode(B("Stage"), pKid );
      if (mpAppearanceFiles)
         XMLHelper( doc, pGrandKid, B("file"), mpAppearanceFiles[j]);
      if (mpAppFileIndices)
         XMLHelper( doc, pGrandKid, B("material"), mpAppFileIndices[j]);
      if (mpAppearanceWeights)
         XMLHelper( doc, pGrandKid, B("weight"), mpAppearanceWeights[j]);
   }

   // create a default material if none exist
   if (mAppearance.mNumFiles == 0)
   {
      BXMLNode *pGrandKid = doc.createNode(B("Stage"), pKid );
      XMLHelper( doc, pGrandKid, B("file"), B("default.ddt") );
      XMLHelper( doc, pGrandKid, B("weight"), 1.0f);
   }

   //-- Depth Biasing
   pKid = doc.createNode(B("DepthBias"), pNode);
   if (!pKid)
      return false;

   XMLHelper( doc, pKid, B("NumFiles"), mDepthBiasing.mNumFiles );
   XMLHelper( doc, pKid, B("NumFrames"), mDepthBiasing.mNumFrames );
   XMLHelper( doc, pKid, B("FrameWidth"), mDepthBiasing.mFrameWidth );
   XMLHelper( doc, pKid, B("FrameHeight"), mDepthBiasing.mFrameHeight );
   XMLHelper( doc, pKid, B("FramesPerSecond"), mDepthBiasing.mFramesPerSecond );
   XMLHelper( doc, pKid, B("ZScale"), mDepthBiasing.mZScale);


   pKid = doc.createNode(B("DepthBiasWeights"), pNode);
   if (!pKid)
      return false;

   for (long j = 0; j < mDepthBiasing.mNumFiles; j++)
   {
      BXMLNode *pGrandKid = doc.createNode(B("Stage"), pKid );
      if (mpDepthBiasFiles)
         XMLHelper( doc, pGrandKid, B("file"), mpDepthBiasFiles[j]);
      if (mpDepthBiasFileIndices)
         XMLHelper( doc, pGrandKid, B("material"), mpDepthBiasFileIndices[j]);
      if (mpDepthBiasWeights)
         XMLHelper( doc, pGrandKid, B("weight"), mpDepthBiasWeights[j]);
   }
   
   pKid = doc.createNode(B("Opacity"), pNode);
   if (!pKid)
      return false;
   
   XMLHelper( doc, pKid, B("LoopingCycle"), (long)mOpacity.mLoopingCycle );
   XMLHelper( doc, pKid, B("NumStages"), mOpacity.mNumStages );
   XMLHelper( doc, pKid, B("Opacity"), mOpacity.mOpacity );
   XMLHelper( doc, pKid, B("OpacityVar"), mOpacity.mOpacityVar );
   XMLHelper( doc, pKid, B("CycleTime"), mOpacity.mCycleTime );
   XMLHelper( doc, pKid, B("CycleTimeVar"), mOpacity.mCycleTimeVar );

   pKid = doc.createNode(B("OpacityStages"), pNode);
   if (!pKid)
      return false;
   
   //If it's not looping, the last fade (transition) must be zero, because there is nothing to transition to.
   if( !mOpacity.mLoopingCycle && mOpacity.mNumStages > 0 )
      mpOpacityStages[mOpacity.mNumStages-1].fade = 0.0f;

   for (long j = 0; j < mOpacity.mNumStages; j++)
   {
      BXMLNode *pGrandKid = doc.createNode(B("Stage"), pKid );
      XMLHelper( doc, pGrandKid, B("opacity"), mpOpacityStages[j].opacity );
      XMLHelper( doc, pGrandKid, B("opacityVar"), mpOpacityStages[j].opacityVar );
      XMLHelper( doc, pGrandKid, B("hold"), mpOpacityStages[j].hold );
      XMLHelper( doc, pGrandKid, B("fade"), mpOpacityStages[j].fade );
   }
   
   pKid = doc.createNode(B("Scale"), pNode);
   if (!pKid)
      return false;
   
   XMLHelper( doc, pKid, B("LoopingCycle"), (long)mScale.mLoopingCycle );
   XMLHelper( doc, pKid, B("NumStages"), mScale.mNumStages );
   XMLHelper( doc, pKid, B("Scale"), mScale.mScale );
   XMLHelper( doc, pKid, B("ScaleVar"), mScale.mScaleVar );
   XMLHelper( doc, pKid, B("XScale"), mScale.mXScale );
   XMLHelper( doc, pKid, B("XScaleVar"), mScale.mXScaleVar );
   XMLHelper( doc, pKid, B("YScale"), mScale.mYScale );
   XMLHelper( doc, pKid, B("YScaleVar"), mScale.mYScaleVar );
   XMLHelper( doc, pKid, B("ZScale"), mScale.mZScale );
   XMLHelper( doc, pKid, B("ZScaleVar"), mScale.mZScaleVar );
   XMLHelper( doc, pKid, B("CycleTime"), mScale.mCycleTime );
   XMLHelper( doc, pKid, B("CycleTimeVar"), mScale.mCycleTimeVar );

   pKid = doc.createNode(B("ScaleStages"), pNode);
   if (!pKid)
      return false;

   //If it's not looping, the last fade (transition) must be zero, because there is nothing to transition to.
   if( !mScale.mLoopingCycle && mScale.mNumStages > 0 )
      mpScaleStages[mScale.mNumStages-1].fade = 0.0f;

   for (long j = 0; j < mScale.mNumStages; j++)
   {
      BXMLNode *pGrandKid = doc.createNode(B("Stage"), pKid );
      XMLHelper( doc, pGrandKid, B("scale"), mpScaleStages[j].scale );
      XMLHelper( doc, pGrandKid, B("scaleVar"), mpScaleStages[j].scaleVar );
      XMLHelper( doc, pGrandKid, B("hold"), mpScaleStages[j].hold );
      XMLHelper( doc, pGrandKid, B("fade"), mpScaleStages[j].fade );
   }
   
   pKid = doc.createNode(B("Speed"), pNode);
   if (!pKid)
      return false;
   
   XMLHelper( doc, pKid, B("LoopingCycle"), (long)mSpeed.mLoopingCycle );
   XMLHelper( doc, pKid, B("NumStages"), mSpeed.mNumStages );
   XMLHelper( doc, pKid, B("Speed"), mSpeed.mSpeed );
   XMLHelper( doc, pKid, B("SpeedVar"), mSpeed.mSpeedVar );
   XMLHelper( doc, pKid, B("XSpeed"), mSpeed.mXSpeed );
   XMLHelper( doc, pKid, B("XSpeedVar"), mSpeed.mXSpeedVar );
   XMLHelper( doc, pKid, B("YSpeed"), mSpeed.mYSpeed );
   XMLHelper( doc, pKid, B("YSpeedVar"), mSpeed.mYSpeedVar );
   XMLHelper( doc, pKid, B("ZSpeed"), mSpeed.mZSpeed );
   XMLHelper( doc, pKid, B("ZSpeedVar"), mSpeed.mZSpeedVar );
   XMLHelper( doc, pKid, B("CycleTime"), mSpeed.mCycleTime );
   XMLHelper( doc, pKid, B("CycleTimeVar"), mSpeed.mCycleTimeVar );
   
   pKid = doc.createNode(B("SpeedStages"), pNode);
   if (!pKid)
      return false;

   //If it's not looping, the last fade (transition) must be zero, because there is nothing to transition to.
   if( !mSpeed.mLoopingCycle && mSpeed.mNumStages > 0 )
      mpSpeedStages[mSpeed.mNumStages-1].fade = 0.0f;

   for (long j = 0; j < mSpeed.mNumStages; j++)
   {
      BXMLNode *pGrandKid = doc.createNode(B("Stage"), pKid );
      XMLHelper( doc, pGrandKid, B("scale"), mpSpeedStages[j].speed );
      XMLHelper( doc, pGrandKid, B("scaleVar"), mpSpeedStages[j].speedVar );
      XMLHelper( doc, pGrandKid, B("hold"), mpSpeedStages[j].hold );
      XMLHelper( doc, pGrandKid, B("fade"), mpSpeedStages[j].fade );
   }

   pKid = doc.createNode(B("Intensity"), pNode);
   if (!pKid)
      return false;
   XMLHelper( doc, pKid, B("LoopingCycle"), (long)mIntensity.mLoopingCycle );
   XMLHelper( doc, pKid, B("NumStages"),    mIntensity.mNumStages );
   XMLHelper( doc, pKid, B("Intensity"),    mIntensity.mIntensity );
   XMLHelper( doc, pKid, B("IntensityVar"), mIntensity.mIntensityVar );
   XMLHelper( doc, pKid, B("CycleTime"),    mIntensity.mCycleTime );
   XMLHelper( doc, pKid, B("CycleTimeVar"), mIntensity.mCycleTimeVar );

   pKid = doc.createNode(B("IntensityStages"), pNode);
   if (!pKid)
      return false;
   for (long j = 0; j < mIntensity.mNumStages; j++)
   {
      BXMLNode *pGrandKid = doc.createNode(B("Stage"), pKid );
      XMLHelper( doc, pGrandKid, B("scale"),    mpIntensityStages[j].intensity );
      XMLHelper( doc, pGrandKid, B("scaleVar"), mpIntensityStages[j].intensityVar );
      XMLHelper( doc, pGrandKid, B("hold"),     mpIntensityStages[j].hold );
      XMLHelper( doc, pGrandKid, B("fade"),     mpIntensityStages[j].fade );
   }
   
   pKid = doc.createNode(B("Color"), pNode);
   if (!pKid)
      return false;
   
   XMLHelper( doc, pKid, B("UsePalette"), (long)mColor.mUsePalette );
   XMLHelper( doc, pKid, B("LoopingCycle"), (long)mColor.mLoopingCycle );
   XMLHelper( doc, pKid, B("NumPaletteColors"), mColor.mNumPaletteColors );
   XMLHelper( doc, pKid, B("NumStages"), mColor.mNumStages );
   XMLHelper( doc, pKid, B("CycleTime"), mColor.mCycleTime );
   XMLHelper( doc, pKid, B("CycleTimeVar"), mColor.mCycleTimeVar );
   XMLHelper( doc, pKid, B("fWorldLightingInfluence"), mColor.mfWorldLightingInfluence );
   XMLHelper( doc, pKid, B("Color"), (long)mColor.mColor );

   pKid = doc.createNode(B("PaletteColors"), pNode);
   if (!pKid)
      return false;
   
   for (long j = 0; j < mColor.mNumPaletteColors; j++)
   {
      XMLHelper( doc, pKid, B("Color"), (long)mpPaletteColors[j] );
   }

   pKid = doc.createNode(B("ColorStages"), pNode);
   if (!pKid)
      return false;

   //If it's not looping, the last fade (transition) must be zero, because there is nothing to transition to.
   if( !mColor.mLoopingCycle && mColor.mNumStages > 0 )
      mpColorStages[mColor.mNumStages-1].fade = 0.0f;

   for (long j = 0; j < mColor.mNumStages; j++)
   {
      BXMLNode *pGrandKid = doc.createNode(B("Stage"), pKid );
      XMLHelper( doc, pGrandKid, B("usePalette"), (long)mpColorStages[j].usePalette );
      XMLHelper( doc, pGrandKid, B("color"), (long)mpColorStages[j].color );
      XMLHelper( doc, pGrandKid, B("hold"), mpColorStages[j].hold );
      XMLHelper( doc, pGrandKid, B("fade"), mpColorStages[j].fade );
   }
   
   pKid = doc.createNode(B("Forces"), pNode);
   if (!pKid)
      return false;
   
   XMLHelper( doc, pKid, B("RandomOrientation"), (long)mForces.mRandomOrientation );
   XMLHelper( doc, pKid, B("Tumble"), (long)mForces.mTumble );
   XMLHelper( doc, pKid, B("TumbleBothDirections"), (long)mForces.mTumbleBothDirections );
   XMLHelper( doc, pKid, B("RandomAxis"), (long)mForces.mRandomAxis );
   XMLHelper( doc, pKid, B("InternalGravity"), mForces.mInternalGravity );
   XMLHelper( doc, pKid, B("InternalGravityVar"), mForces.mInternalGravityVar );
   XMLHelper( doc, pKid, B("InternalWindDirection"), mForces.mInternalWindDirection );
   XMLHelper( doc, pKid, B("InternalWindDirectionVar"), mForces.mInternalWindDirectionVar );
   XMLHelper( doc, pKid, B("InternalWindSpeed"), mForces.mInternalWindSpeed );
   XMLHelper( doc, pKid, B("InternalWindSpeedVar"), mForces.mInternalWindSpeedVar );
   XMLHelper( doc, pKid, B("InternalWindDelay"), mForces.mInternalWindDelay );
   XMLHelper( doc, pKid, B("InternalWindDelayVar"), mForces.mInternalWindDelayVar );
   XMLHelper( doc, pKid, B("ExternalWindInfluence"), mForces.mExternalWindInfluence );
   XMLHelper( doc, pKid, B("ExternalWindInfluenceVar"), mForces.mExternalWindInfluenceVar );
   XMLHelper( doc, pKid, B("ExternalWindDelay"), mForces.mExternalWindDelay );
   XMLHelper( doc, pKid, B("ExternalWindDelayVar"), mForces.mExternalWindDelayVar );
   XMLHelper( doc, pKid, B("RiverFlowInfluence"), mForces.mRiverFlowInfluence );
   XMLHelper( doc, pKid, B("RiverFlowInfluenceVar"), mForces.mRiverFlowInfluenceVar );
   XMLHelper( doc, pKid, B("MinAngularVelocity"), mForces.mMinAngularVelocity );
   XMLHelper( doc, pKid, B("MaxAngularVelocity"), mForces.mMaxAngularVelocity );
   XMLHelper( doc, pKid, B("XAxis"), mForces.mXAxis );
   XMLHelper( doc, pKid, B("XAxisVar"), mForces.mXAxisVar );
   XMLHelper( doc, pKid, B("YAxis"), mForces.mYAxis );
   XMLHelper( doc, pKid, B("YAxisVar"), mForces.mYAxisVar );
   XMLHelper( doc, pKid, B("ZAxis"), mForces.mZAxis );
   XMLHelper( doc, pKid, B("ZAxisVar"), mForces.mZAxisVar );

   pKid = doc.createNode(B("Collision"), pNode);
   if (!pKid)
      return false;
   
   XMLHelper( doc, pKid, B("NumTypes"), mCollision.mNumTypes );
   XMLHelper( doc, pKid, B("TerrainInteractionType"), mCollision.mTerrainInteractionType );
   XMLHelper( doc, pKid, B("TerrainHeight"), mCollision.mTerrainHeight );
   XMLHelper( doc, pKid, B("TerrainHeightVar"), mCollision.mTerrainHeightVar );
   XMLHelper( doc, pKid, B("TerrainDampenFactor"), mCollision.mTerrainDampenFactor );

   pKid = doc.createNode(B("CollisionStages"), pNode);
   if (!pKid)
      return false;
   
   for (long j = 0; j < mCollision.mNumTypes; j++)
   {
      BXMLNode *pGrandKid = doc.createNode(B("Stage"), pKid );
      XMLHelper( doc, pGrandKid, B("spawnSystem"), (long)mpCollisionTypes[j].spawnSystem );
      XMLHelper( doc, pGrandKid, B("collideTerrain"), (long)mpCollisionTypes[j].collideTerrain );
      XMLHelper( doc, pGrandKid, B("collideWater"), (long)mpCollisionTypes[j].collideWater );
      XMLHelper( doc, pGrandKid, B("collideUnits"), (long)mpCollisionTypes[j].collideUnits );
      XMLHelper( doc, pGrandKid, B("result"), mpCollisionTypes[j].result );
      XMLHelper( doc, pGrandKid, B("numFileNames"), mpCollisionTypes[j].numFileNames );
      XMLHelper( doc, pGrandKid, B("lingerTime"), mpCollisionTypes[j].lingerTime );
      XMLHelper( doc, pGrandKid, B("lingerTimeVar"), mpCollisionTypes[j].lingerTimeVar );
      XMLHelper( doc, pGrandKid, B("fadeTime"), mpCollisionTypes[j].fadeTime );
      XMLHelper( doc, pGrandKid, B("fadeTimeVar"), mpCollisionTypes[j].fadeTimeVar );
      XMLHelper( doc, pGrandKid, B("energyLoss"), mpCollisionTypes[j].energyLoss );
      XMLHelper( doc, pGrandKid, B("energyLossVar"), mpCollisionTypes[j].energyLossVar );
      XMLHelper( doc, pGrandKid, B("name"), mpCollisionTypes[j].name );

      pGrandKid = doc.createNode(B("Filenames"), pKid );
      for (long k = 0; k < mpCollisionTypes[j].numFileNames; k++)
      {
         XMLHelper( doc, pGrandKid, B("name"), mpCollisionTypes[j].pFileNames[k] );
      }
   }

   pKid = doc.createNode(B("ParticleEvents"), pNode);
   if (!pKid)
      return false;

   for (long j = 0; j < mEvents.mNumEvents; j++)
   {
      BXMLNode *pGrandKid = doc.createNode(B("Event"), pKid );
      XMLHelper( doc, pGrandKid, B("effectStart"), (float)mpParticleEvents[j].effectStart );
      XMLHelper( doc, pGrandKid, B("effectStartVar"), (float)mpParticleEvents[j].effectStartVar );
      XMLHelper( doc, pGrandKid, B("systemLinger"), (long)mpParticleEvents[j].systemLinger );
      XMLHelper( doc, pGrandKid, B("fileName"), mpParticleEvents[j].filename );
   }
   
   //
   // all done
   //
   BSimString realname = filename;

   realname.removeExtension();
   realname.append((".particle"));

   //-- write the new file
   bool absolutePath = isExternal(realname.asUnicode());
   if(absolutePath)
      doc.writeToFile(cDirAbsolutePath, realname, true);
   else
      doc.writeToFile(gModelManager->getParticleSystemManager()->getBaseDirectoryID(), realname, true);
#endif
   
   return (true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemData::loadXML(const BSimString& name)
{
   clear();
   
   //-- Open the file.
   bool ok;
   BXMLReader reader;

   BSimString realname = name;
   
   realname.removeExtension();
   realname.append((".particle"));
   
   bool absolutePath = isExternalFilename(realname);
   if(absolutePath)
      ok = reader.loadFileSAX(cDirAbsolutePath, realname, NULL, false);
   else
      ok = reader.loadFileSAX(cDirParticlesEffects, realname);

   if(!ok)
      return false;
   
   const BXMLNode *pRoot = reader.getRootNode();
   BXMLNode *pNode = NULL;
   
   if (pRoot == NULL)
   {
      {setBlogError(4137); blogerror("BParticleSystemData::loadXML Error parsing %s", name.getPtr());}
      return (false);
   }
   
   const BSimString &szRootName = pRoot->getName();
   
   // -- make sure we are dealing with the right kind of file
   if (szRootName.compare(("ParticleFile")) != 0)
   {
      {setBlogError(4138); blogerror("BParticleSystemData::loadXML: this is not a particle definition file.");}
      return (false);
   }
   
   // -- walk the tree and respond to the various tags appropriately
   long dwChildCount = pRoot->getNumberChildren();

   // -- loop through each "SoundSet" tag
   for (long i = 0; i < dwChildCount; i++)
   {
      pNode = pRoot->getChild(i);
      const BSimString &szTag = pNode->getName();
      
      // -- process each SoundSet or Category
      if (szTag.compare(("Emitter")) == 0)
      {  
         pNode->getChildValue("TiedToEmitter",  mEmitter.mTiedToEmitter);
         pNode->getChildValue("IgnoreRotation", mEmitter.mIgnoreRotation );
         pNode->getChildValue("EmitByMotion", mEmitter.mEmitByMotion );
         pNode->getChildValue("Loop", mEmitter.mLoop );
         pNode->getChildValue("InheritVelocity", mEmitter.mInheritVelocity );
         pNode->getChildValue("UseMinVelocity", mEmitter.mUseMinVelocity );
         pNode->getChildValue("UseMaxVelocity", mEmitter.mUseMaxVelocity );
         pNode->getChildValue("AlwaysActive", mEmitter.mAlwaysActive );
         pNode->getChildValue("CastShadows", mEmitter.mCastShadows );
         pNode->getChildValue("SyncWithAttackAnim", mEmitter.mSyncWithAttackAnim );
         pNode->getChildValue("MaxParticles", mEmitter.mMaxParticles );
         pNode->getChildValue("AppearanceType", mEmitter.mAppearanceType );
         pNode->getChildValue("UpdateRadius", mEmitter.mUpdateRadius );
         if (mEmitter.mUpdateRadius<3.0f)
            mEmitter.mUpdateRadius = 3.0f;
         else if (mEmitter.mUpdateRadius>25.0f)
            mEmitter.mUpdateRadius = 25.0f;
         pNode->getChildValue("MaxParticlesVar", mEmitter.mMaxParticlesVar );
         pNode->getChildValue("ParticleLife", mEmitter.mParticleLife );
         pNode->getChildValue("ParticleLifeVar", mEmitter.mParticleLifeVar );
         pNode->getChildValue("GlobalFadeIn", mEmitter.mGlobalFadeIn );
         pNode->getChildValue("GlobalFadeInVar", mEmitter.mGlobalFadeInVar );
         pNode->getChildValue("GlobalFadeOut", mEmitter.mGlobalFadeOut );
         pNode->getChildValue("GlobalFadeOutVar", mEmitter.mGlobalFadeOutVar );
         pNode->getChildValue("EmitDistance", mEmitter.mEmitDistance );
         pNode->getChildValue("EmitDistanceVar", mEmitter.mEmitDistanceVar );
         pNode->getChildValue("EmissionRate", mEmitter.mEmissionRate );
         pNode->getChildValue("EmissionRateVar", mEmitter.mEmissionRateVar );
         pNode->getChildValue("InitialDormancy", mEmitter.mInitialDormancy );
         pNode->getChildValue("InitialDormancyVar", mEmitter.mInitialDormancyVar );
         pNode->getChildValue("InitialUpdate", mEmitter.mInitialUpdate );
         pNode->getChildValue("InitialUpdateVar", mEmitter.mInitialUpdateVar );
         pNode->getChildValue("EmissionTime", mEmitter.mEmissionTime );
         pNode->getChildValue("EmissionTimeVar", mEmitter.mEmissionTimeVar );
         pNode->getChildValue("DormantTime", mEmitter.mDormantTime );
         pNode->getChildValue("DormantTimeVar", mEmitter.mDormantTimeVar );
         pNode->getChildValue("InitialDistance", mEmitter.mInitialDistance );
         pNode->getChildValue("InitialDistanceVar", mEmitter.mInitialDistanceVar );
         pNode->getChildValue("InitialVelocity", mEmitter.mInitialVelocity );
         pNode->getChildValue("InitialVelocityVar", mEmitter.mInitialVelocityVar );
         pNode->getChildValue("Acceleration", mEmitter.mAcceleration );
         pNode->getChildValue("AccelerationVar", mEmitter.mAccelerationVar );
         pNode->getChildValue("InheritInfluence", mEmitter.mInheritInfluence );
         pNode->getChildValue("InheritInfluenceVar", mEmitter.mInheritInfluenceVar );
         pNode->getChildValue("MinVelocity", mEmitter.mMinVelocity );
         pNode->getChildValue("MinVelocityVar", mEmitter.mMinVelocityVar );
         pNode->getChildValue("MaxVelocity", mEmitter.mMaxVelocity );
         pNode->getChildValue("MaxVelocityVar", mEmitter.mMaxVelocityVar );
      }
      // -- process each SoundSet or Category
      else if (szTag.compare(("Shape")) == 0)
      {  
         pNode->getChildValue("StartFull", mShape.mStartFull );
         pNode->getChildValue("EmitAwayFromBias", mShape.mEmitAwayFromBias );
         pNode->getChildValue("UseSpreader", mShape.mUseSpreader );
         pNode->getChildValue("ShapeType", mShape.mShapeType );
         pNode->getChildValue("OuterXRadius", mShape.mOuterXRadius );
         pNode->getChildValue("InnerXRadius", mShape.mInnerXRadius );
         pNode->getChildValue("OuterYRadius", mShape.mOuterYRadius );
         pNode->getChildValue("InnerYRadius", mShape.mInnerYRadius );
         pNode->getChildValue("OuterZRadius", mShape.mOuterZRadius );
         pNode->getChildValue("InnerZRadius", mShape.mInnerZRadius );
         pNode->getChildValue("CenterHeight", mShape.mCenterHeight );
         pNode->getChildValue("OffAxis", mShape.mOffAxis );
         pNode->getChildValue("OffAxisSpread", mShape.mOffAxisSpread );
         pNode->getChildValue("OffPlane", mShape.mOffPlane );
         pNode->getChildValue("OffPlaneSpread", mShape.mOffPlaneSpread );
         pNode->getChildValue("BiasPointHeight", mShape.mBiasPointHeight );
         pNode->getChildValue("ForceRiverFlowEmission", mShape.mForceRiverFlowEmission );
      }
      else if (szTag.compare(("Appearance")) == 0)
      {  
         pNode->getChildValue("OrientByMotion", mAppearance.mOrientByMotion );
         pNode->getChildValue("NumFiles", mAppearance.mNumFiles );
         pNode->getChildValue("NumFrames", mAppearance.mNumFrames );
         pNode->getChildValue("FrameWidth", mAppearance.mFrameWidth );
         pNode->getChildValue("FrameHeight", mAppearance.mFrameHeight );
         pNode->getChildValue("MaterialType", mAppearance.mMaterialType );
         pNode->getChildValue("Emissive", mAppearance.mEmissive );
         pNode->getChildValue("Specular", mAppearance.mSpecular );
         pNode->getChildValue("SpecularExponent", mAppearance.mSpecularExponent );
         pNode->getChildValue("FramesPerSecond", mAppearance.mFramesPerSecond );
         pNode->getChildValue("AnimationRate", mAppearance.mAnimationRate );
         pNode->getChildValue("AnimationRateVar", mAppearance.mAnimationRateVar );
      }
      else if (szTag.compare("AppearanceWeights") == 0)
      {
         // -- walk the tree and respond to the various tags appropriately
         mAppearance.mNumFiles = pNode->getNumberChildren();

         mpAppearanceFiles   = new BSimString         [mAppearance.mNumFiles];
         mpAppFileIndices    = new int             [mAppearance.mNumFiles];
         mpAppearanceWeights = new float           [mAppearance.mNumFiles];

         for (long j = 0; j < mAppearance.mNumFiles; j++)
         {
            BXMLNode *pKid = pNode->getChild(j);
            const BSimString &szKidTag = pKid->getName();
            if (szKidTag.compare(("Stage")) == 0)
            {
               const BSimString *ptr;
               pKid->getChildValue( "file",     &ptr);
               mpAppearanceFiles[j] = *ptr;
               //pKid->getChildValue( "material", mpAppFileIndices[j]);
               pKid->getChildValue( "weight",   mpAppearanceWeights[j]);
            }
         }
      }
      else if (szTag.compare(("DepthBias")) == 0)
      {  
         pNode->getChildValue("NumFiles", mDepthBiasing.mNumFiles );
         pNode->getChildValue("NumFrames", mDepthBiasing.mNumFrames );
         pNode->getChildValue("FrameWidth", mDepthBiasing.mFrameWidth );
         pNode->getChildValue("FrameHeight", mDepthBiasing.mFrameHeight );
         pNode->getChildValue("FramesPerSecond", mDepthBiasing.mFramesPerSecond );
         pNode->getChildValue("ZScale", mDepthBiasing.mZScale);
      }
      else if (szTag.compare(("DepthBiasWeights")) == 0)
      {
         //-- BTK DISABLED
#if 0
         // -- walk the tree and respond to the various tags appropriately
         mDepthBiasing.mNumFiles = pNode->getNumberChildren();

         mpDepthBiasFiles       = new BSimString[mDepthBiasing.mNumFiles];
         mpDepthBiasWeights     = new float  [mDepthBiasing.mNumFiles];

         for (long j = 0; j < mDepthBiasing.mNumFiles; j++)
         {
            const BXMLNode *pKid = pNode->getChild(j);
            const BSimString &szKidTag = pKid->getName();
            if (szKidTag.compare(("Stage")) == 0)
            {
               const BSimString *ptr;
               pKid->getChildValue( "file",     &ptr);
               mpDepthBiasFiles[j] = *ptr;               
               //pKid->getChildValue( "material", mpDepthBiasFileIndices[j]);
               pKid->getChildValue( "weight",   mpDepthBiasWeights[j]);
            }
         }
#endif
      }
      else if (szTag.compare(("Opacity")) == 0)
      {  
         pNode->getChildValue("LoopingCycle", mOpacity.mLoopingCycle );
         pNode->getChildValue("NumStages", mOpacity.mNumStages );
         pNode->getChildValue("Opacity", mOpacity.mOpacity );
         pNode->getChildValue("OpacityVar", mOpacity.mOpacityVar );
         pNode->getChildValue("CycleTime", mOpacity.mCycleTime );
         pNode->getChildValue("CycleTimeVar", mOpacity.mCycleTimeVar );
      }
      else if (szTag.compare(("OpacityStages")) == 0)
      {
         // -- walk the tree and respond to the various tags appropriately
         mOpacity.mNumStages = pNode->getNumberChildren();
         mpOpacityStages     = new PSOpacityStage  [mOpacity.mNumStages];
         for (long j = 0; j < mOpacity.mNumStages; j++)
         {
            BXMLNode *pKid = pNode->getChild(j);
            const BSimString &szKidTag = pKid->getName();
            if (szKidTag.compare(("Stage")) == 0)
            {
               pKid->getChildValue("opacity", mpOpacityStages[j].opacity );
               pKid->getChildValue("opacityVar", mpOpacityStages[j].opacityVar );
               pKid->getChildValue("hold", mpOpacityStages[j].hold );
               pKid->getChildValue("fade", mpOpacityStages[j].fade );
            }
         }
         //If it's not looping, the last fade (transition) must be zero, because there is nothing to transition to.
         if( !mOpacity.mLoopingCycle && mOpacity.mNumStages > 0 )
            mpOpacityStages[mOpacity.mNumStages-1].fade = 0.0f;
      }
      else if (szTag.compare(("Scale")) == 0)
      {  
         pNode->getChildValue("LoopingCycle", mScale.mLoopingCycle );
         pNode->getChildValue("NumStages", mScale.mNumStages );
         pNode->getChildValue("Scale", mScale.mScale );
         pNode->getChildValue("ScaleVar", mScale.mScaleVar );
         pNode->getChildValue("XScale", mScale.mXScale );
         pNode->getChildValue("XScaleVar", mScale.mXScaleVar );
         pNode->getChildValue("YScale", mScale.mYScale );
         pNode->getChildValue("YScaleVar", mScale.mYScaleVar );
         pNode->getChildValue("ZScale", mScale.mZScale );
         pNode->getChildValue("ZScaleVar", mScale.mZScaleVar );
         pNode->getChildValue("CycleTime", mScale.mCycleTime );
         pNode->getChildValue("CycleTimeVar", mScale.mCycleTimeVar );
      }
      else if (szTag.compare(("ScaleStages")) == 0)
      {
         // -- walk the tree and respond to the various tags appropriately
         mScale.mNumStages = pNode->getNumberChildren();
         mpScaleStages       = new PSScaleStage    [mScale.mNumStages];
         for (long j = 0; j < mScale.mNumStages; j++)
         {
            BXMLNode *pKid = pNode->getChild(j);
            const BSimString &szKidTag = pKid->getName();
            if (szKidTag.compare(("Stage")) == 0)
            {
               pKid->getChildValue("scale", mpScaleStages[j].scale );
               pKid->getChildValue("scaleVar", mpScaleStages[j].scaleVar );
               pKid->getChildValue("hold", mpScaleStages[j].hold );
               pKid->getChildValue("fade", mpScaleStages[j].fade );
            }
         }
         //If it's not looping, the last fade (transition) must be zero, because there is nothing to transition to.
         if( !mScale.mLoopingCycle && mScale.mNumStages > 0 )
            mpScaleStages[mScale.mNumStages-1].fade = 0.0f;
      }
      else if (szTag.compare(("Speed")) == 0)
      {  
         pNode->getChildValue("LoopingCycle", mSpeed.mLoopingCycle );
         pNode->getChildValue("NumStages", mSpeed.mNumStages );
         pNode->getChildValue("Speed", mSpeed.mSpeed );
         pNode->getChildValue("SpeedVar", mSpeed.mSpeedVar );
         pNode->getChildValue("XSpeed", mSpeed.mXSpeed );
         pNode->getChildValue("XSpeedVar", mSpeed.mXSpeedVar );
         pNode->getChildValue("YSpeed", mSpeed.mYSpeed );
         pNode->getChildValue("YSpeedVar", mSpeed.mYSpeedVar );
         pNode->getChildValue("ZSpeed", mSpeed.mZSpeed );
         pNode->getChildValue("ZSpeedVar", mSpeed.mZSpeedVar );
         pNode->getChildValue("CycleTime", mSpeed.mCycleTime );
         pNode->getChildValue("CycleTimeVar", mSpeed.mCycleTimeVar );
      }
      else if (szTag.compare(("SpeedStages")) == 0)
      {
         // -- walk the tree and respond to the various tags appropriately
         mSpeed.mNumStages = pNode->getNumberChildren();
         mpSpeedStages       = new PSSpeedStage    [mSpeed.mNumStages];
         for (long j = 0; j < mSpeed.mNumStages; j++)
         {
            BXMLNode *pKid = pNode->getChild(j);
            const BSimString &szKidTag = pKid->getName();
            if (szKidTag.compare(("Stage")) == 0)
            {
               pKid->getChildValue("scale", mpSpeedStages[j].speed );
               pKid->getChildValue("scaleVar", mpSpeedStages[j].speedVar );
               pKid->getChildValue("hold", mpSpeedStages[j].hold );
               pKid->getChildValue("fade", mpSpeedStages[j].fade );
            }
         }
         //If it's not looping, the last fade (transition) must be zero, because there is nothing to transition to.
         if( !mSpeed.mLoopingCycle && mSpeed.mNumStages > 0 )
            mpSpeedStages[mSpeed.mNumStages-1].fade = 0.0f;
      }
      else if (szTag.compare(("Intensity")) == 0 )
      {
         pNode->getChildValue("LoopingCycle", mIntensity.mLoopingCycle );
         pNode->getChildValue("NumStages",    mIntensity.mNumStages );
         pNode->getChildValue("Intensity",    mIntensity.mIntensity );
         pNode->getChildValue("IntensityVar", mIntensity.mIntensityVar );
         pNode->getChildValue("CycleTime",    mIntensity.mCycleTime );
         pNode->getChildValue("CycleTimeVar", mIntensity.mCycleTimeVar );
      }
      else if (szTag.compare(("IntensityStages")) == 0)
      {
         // -- walk the tree and respond to the various tags appropriately
         mIntensity.mNumStages = pNode->getNumberChildren();
         mpIntensityStages     = new PSIntensityStage[mIntensity.mNumStages];
         for (long j = 0; j < mIntensity.mNumStages; j++)
         {
            const BXMLNode *pKid = pNode->getChild(j);
            const BSimString &szKidTag = pKid->getName();
            if (szKidTag.compare(("Stage")) == 0)
            {
               pKid->getChildValue("scale",    mpIntensityStages[j].intensity );
               pKid->getChildValue("scaleVar", mpIntensityStages[j].intensityVar );
               pKid->getChildValue("hold",     mpIntensityStages[j].hold );
               pKid->getChildValue("fade",     mpIntensityStages[j].fade );
            }
         }
      }
      else if (szTag.compare(("Color")) == 0)
      {  
         pNode->getChildValue("UsePalette", mColor.mUsePalette );
         pNode->getChildValue("LoopingCycle", mColor.mLoopingCycle );
         pNode->getChildValue("NumPaletteColors", mColor.mNumPaletteColors );
         pNode->getChildValue("NumStages", mColor.mNumStages );
         pNode->getChildValue("CycleTime", mColor.mCycleTime );
         pNode->getChildValue("CycleTimeVar", mColor.mCycleTimeVar );
         pNode->getChildValue("fWorldLightingInfluence", mColor.mfWorldLightingInfluence );
         pNode->getChildValue("Color", mColor.mColor );
      }
      else if (szTag.compare(("PaletteColors")) == 0)
      {
         // -- walk the tree and respond to the various tags appropriately
         mColor.mNumPaletteColors = pNode->getNumberChildren();
         mpPaletteColors     = new DWORD[mColor.mNumPaletteColors];
         for (long j = 0; j < mColor.mNumPaletteColors; j++)
         {
            BXMLNode *pKid = pNode->getChild(j);
            const BSimString &szKidTag = pKid->getName();
            if (szKidTag.compare(("Color")) == 0)
                pKid->getTextAsDWORD(mpPaletteColors[j]);
         }
      }
      else if (szTag.compare(("ColorStages")) == 0)
      {
         // -- walk the tree and respond to the various tags appropriately
         mColor.mNumStages = pNode->getNumberChildren();
         mpColorStages       = new PSColorStage    [mColor.mNumStages];
         for (long j = 0; j < mColor.mNumStages; j++)
         {
            BXMLNode *pKid = pNode->getChild(j);
            const BSimString &szKidTag = pKid->getName();
            if (szKidTag.compare(("Stage")) == 0)
            {
               pKid->getChildValue("usePalette", mpColorStages[j].usePalette );
               pKid->getChildValue("color", mpColorStages[j].color );
               pKid->getChildValue("hold", mpColorStages[j].hold );
               pKid->getChildValue("fade", mpColorStages[j].fade );
            }
         }
         //If it's not looping, the last fade (transition) must be zero, because there is nothing to transition to.
         if( !mColor.mLoopingCycle && mColor.mNumStages > 0 )
            mpColorStages[mColor.mNumStages-1].fade = 0.0f;
      }
      else if (szTag.compare(("Forces")) == 0)
      {  
         pNode->getChildValue("RandomOrientation", mForces.mRandomOrientation );
         pNode->getChildValue("Tumble", mForces.mTumble );
         pNode->getChildValue("TumbleBothDirections", mForces.mTumbleBothDirections );
         pNode->getChildValue("RandomAxis", mForces.mRandomAxis );
         pNode->getChildValue("InternalGravity", mForces.mInternalGravity );
         pNode->getChildValue("InternalGravityVar", mForces.mInternalGravityVar );
         pNode->getChildValue("InternalWindDirection", mForces.mInternalWindDirection );
         pNode->getChildValue("InternalWindDirectionVar", mForces.mInternalWindDirectionVar );
         pNode->getChildValue("InternalWindSpeed", mForces.mInternalWindSpeed );
         pNode->getChildValue("InternalWindSpeedVar", mForces.mInternalWindSpeedVar );
         pNode->getChildValue("InternalWindDelay", mForces.mInternalWindDelay );
         pNode->getChildValue("InternalWindDelayVar", mForces.mInternalWindDelayVar );
         pNode->getChildValue("ExternalWindInfluence", mForces.mExternalWindInfluence );
         pNode->getChildValue("ExternalWindInfluenceVar", mForces.mExternalWindInfluenceVar );
         pNode->getChildValue("ExternalWindDelay", mForces.mExternalWindDelay );
         pNode->getChildValue("ExternalWindDelayVar", mForces.mExternalWindDelayVar );         
         pNode->getChildValue("MinAngularVelocity", mForces.mMinAngularVelocity );
         pNode->getChildValue("MaxAngularVelocity", mForces.mMaxAngularVelocity );
         pNode->getChildValue("XAxis", mForces.mXAxis );
         pNode->getChildValue("XAxisVar", mForces.mXAxisVar );
         pNode->getChildValue("YAxis", mForces.mYAxis );
         pNode->getChildValue("YAxisVar", mForces.mYAxisVar );
         pNode->getChildValue("ZAxis", mForces.mZAxis );
         pNode->getChildValue("ZAxisVar", mForces.mZAxisVar );

         mForces.mUseGravity = true;
         if (abs(mForces.mInternalGravity) < cFloatCompareEpsilon)
            mForces.mUseGravity = false;

         mForces.mUseWind = true;
         if (abs(mForces.mInternalWindSpeed) < cFloatCompareEpsilon)
            mForces.mUseWind = false;
      }
      else if (szTag.compare(("Collision")) == 0)
      {  
         pNode->getChildValue("NumTypes", mCollision.mNumTypes );
         pNode->getChildValue("TerrainInteractionType", mCollision.mTerrainInteractionType );
         pNode->getChildValue("TerrainHeight", mCollision.mTerrainHeight );
         pNode->getChildValue("TerrainHeightVar", mCollision.mTerrainHeightVar );
         pNode->getChildValue("TerrainDampenFactor", mCollision.mTerrainDampenFactor );
      }
      else if (szTag.compare(("CollisionStages")) == 0)
      {
         // -- walk the tree and respond to the various tags appropriately
         mCollision.mNumTypes = pNode->getNumberChildren() / 2;
         mpCollisionTypes    = new PSCollisionType [mCollision.mNumTypes];
         for (long j = 0; j < mCollision.mNumTypes; j++)
         {
            BXMLNode *pKid = pNode->getChild(j*2+0);
            const BSimString &szKidTag = pKid->getName();
            if (szKidTag.compare(("Stage")) == 0)
            {
               pKid->getChildValue("spawnSystem", mpCollisionTypes[j].spawnSystem );
               pKid->getChildValue("collideTerrain", mpCollisionTypes[j].collideTerrain );
               pKid->getChildValue("collideWater", mpCollisionTypes[j].collideWater );
               pKid->getChildValue("collideUnits", mpCollisionTypes[j].collideUnits );
               pKid->getChildValue("result", mpCollisionTypes[j].result );
               pKid->getChildValue("numFileNames", mpCollisionTypes[j].numFileNames );
               pKid->getChildValue("lingerTime", mpCollisionTypes[j].lingerTime );
               pKid->getChildValue("lingerTimeVar", mpCollisionTypes[j].lingerTimeVar );
               pKid->getChildValue("fadeTime", mpCollisionTypes[j].fadeTime );
               pKid->getChildValue("fadeTimeVar", mpCollisionTypes[j].fadeTimeVar );
               pKid->getChildValue("energyLoss", mpCollisionTypes[j].energyLoss );
               pKid->getChildValue("energyLossVar", mpCollisionTypes[j].energyLossVar );
               const BSimString *ptr;
               pKid->getChildValue( "name",     &ptr);
               mpCollisionTypes[j].name = *ptr;
            }
            pKid = pNode->getChild(j*2+1);
            const BSimString &szKidTag1 = pKid->getName();
            if (szKidTag1.compare(("Filenames")) == 0)
            {
               // -- walk the tree and respond to the various tags appropriately
               mpCollisionTypes[j].numFileNames = pKid->getNumberChildren();
               mpCollisionTypes[j].pFileNames   = new BSimString         [mpCollisionTypes[j].numFileNames];
               
               for (long k = 0; k < mpCollisionTypes[j].numFileNames; k++)
               {
                  BXMLNode *pGrandKid = pKid->getChild(k);
                  const BSimString &szKidTag2 = pGrandKid->getName();
                  if (szKidTag2.compare(("name")) == 0)
                     mpCollisionTypes[j].pFileNames[k] = pGrandKid->getText();
               }
            }
         }
      }
      else if (szTag.compare(("ParticleEvents")) == 0)
      {
         // -- walk the tree and respond to the various tags appropriately
         mEvents.mNumEvents = pNode->getNumberChildren();
         mpParticleEvents       = new PSParticleEvent    [mEvents.mNumEvents];
         for (long j = 0; j < mEvents.mNumEvents; j++)
         {
            BXMLNode *pKid = pNode->getChild(j);
            const BSimString &szKidTag = pKid->getName();
            if (szKidTag.compare(("Event")) == 0)
            {
               pKid->getChildValue("effectStart", mpParticleEvents[j].effectStart );
               pKid->getChildValue("effectStartVar", mpParticleEvents[j].effectStartVar );
               pKid->getChildValue("systemLinger", mpParticleEvents[j].systemLinger );
               const BSimString *ptr;
               pKid->getChildValue("fileName", &ptr );
               mpParticleEvents[j].filename = *ptr;
            }
         }
      }

   }
   
   //-- Validate.
   if (!validate())
   {  
      clear();
      return false;
   }
   
   if( mEmitter.mAppearanceType != PSPA_3D_SOMETHING )
   {
      //-- Load them materials.
      if (!loadMaterials())
      {  
         clear();
         return false;
      }
   }
   else
   {
      //-- Load the models
      if (!load3DModels())
      {  
         clear();
         return false;
      }
   }
   
   //-- Do some setup work.
   setupEmitterShape();
   
   //-- Success.
   mName     = realname;
   mRefCount = 1;

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemData::load(const BSimString& fileName)
{
   return loadXML(fileName);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemData::reload()
{
   BSimString nameCopy = mName;
   return load(nameCopy);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float BParticleSystemData::getMaxParticleRadius()
{
   float maxScale  = mScale.mScale  * (1.0f + mScale.mScaleVar);
   float maxXScale = mScale.mXScale * (1.0f + mScale.mXScaleVar);
   float maxYScale = mScale.mYScale * (1.0f + mScale.mYScaleVar);
//   float maxZScale = mScale.mZScale * (1.0f + mScale.mZScaleVar);

   //-- Modify by max progression scale.
   if (mpScaleStages && (mScale.mNumStages > 0))
   {
      float maxStageScale = 0.0f;
      for (long stage = 0; stage < mScale.mNumStages; ++stage)
      {
         float stageScale = (float)fabs(mpScaleStages[stage].scale * (1.0f + mpScaleStages[stage].scaleVar));
         if (maxStageScale < stageScale)
            maxStageScale = stageScale;
      }

      maxScale *= maxStageScale;
   }

   //-- Determine max 2D dimensions.
   float maxRadius = (float)sqrt(maxXScale*maxXScale + maxYScale*maxYScale);
   return (maxScale*0.5f * maxRadius);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemData::setupEmitterShape()
{
   //-- Basic validation.
   if (mShape.mInnerXRadius > mShape.mOuterXRadius)
      mShape.mInnerXRadius = mShape.mOuterXRadius;
   if (mShape.mInnerYRadius > mShape.mOuterYRadius)
      mShape.mInnerYRadius = mShape.mOuterYRadius;
   if (mShape.mInnerZRadius > mShape.mOuterZRadius)
      mShape.mInnerZRadius = mShape.mOuterZRadius;

   //-- Set up a box-surface emitter.
   if (mShape.mShapeType == PSES_BOX_SURFACE)
   {
      float xSpan = mShape.mOuterXRadius * 2;
      float ySpan = mShape.mOuterYRadius * 2;
      float zSpan = mShape.mOuterZRadius * 2;

      mTopFactor   = xSpan * zSpan;
      mSideFactor  = ySpan * zSpan;
      mFrontFactor = xSpan * ySpan;
      mTotalFactor = mTopFactor + mSideFactor + mFrontFactor;

      if (mTotalFactor < cFloatCompareEpsilon)
         mShape.mShapeType = PSES_POINT;
   }

   //-- Set up a box-hollow emitter.
   if (mShape.mShapeType == PSES_BOX_HOLLOW)
   {
      float xEdge = mShape.mOuterXRadius - mShape.mInnerXRadius;
      float yEdge = mShape.mOuterYRadius - mShape.mInnerYRadius;
      float zEdge = mShape.mOuterZRadius - mShape.mInnerZRadius;
      float xSpan = mShape.mOuterXRadius * 2;
      float ySpan = mShape.mOuterYRadius * 2;
      float zSpan = mShape.mOuterZRadius * 2;

      mTopFactor   = xSpan * zSpan * yEdge;
      mSideFactor  = (ySpan - 2*yEdge) * zSpan * xEdge;
      mFrontFactor = (xSpan - 2*xEdge) * (ySpan - 2*yEdge) * zEdge;
      mTotalFactor = mTopFactor + mSideFactor + mFrontFactor;

      if (mTotalFactor < cFloatCompareEpsilon)
         mShape.mShapeType = PSES_POINT;
   }

   //-- Set up a rectangle emitter.
   if (mShape.mShapeType == PSES_RECTANGLE)
   {
      float xSpan = mShape.mOuterXRadius * 2;
      float zSpan = mShape.mOuterZRadius * 2;

      mTopFactor   = xSpan;
      mSideFactor  = zSpan;
      mFrontFactor = 0;
      mTotalFactor = mTopFactor + mSideFactor + mFrontFactor;

      if (mTotalFactor < cFloatCompareEpsilon)
         mShape.mShapeType = PSES_POINT;
   }
}


//============================================================================
//  OPERATORS
//============================================================================
BParticleSystemData& BParticleSystemData::operator = (const BParticleSystemData& data)
{
   //-- Run Time Data
   mName                 = data.mName;
   mVersion              = data.mVersion;
   mRefCount             = data.mRefCount;
   mTextureWidth         = data.mTextureWidth;
   mTextureHeight        = data.mTextureHeight;
   mAppearanceFileWeight = data.mAppearanceFileWeight;
   mTopFactor            = data.mTopFactor;
   mSideFactor           = data.mSideFactor;
   mFrontFactor          = data.mFrontFactor;
   mTotalFactor          = data.mTotalFactor;
   
   //-- Structs   
   Utils::FastMemCpy(&mEmitter, &data.mEmitter, sizeof(data.mEmitter));
   Utils::FastMemCpy(&mShape, &data.mShape, sizeof(data.mShape));
   Utils::FastMemCpy(&mAppearance, &data.mAppearance, sizeof(data.mAppearance));
   Utils::FastMemCpy(&mDepthBiasing, &data.mDepthBiasing, sizeof(data.mDepthBiasing));
   Utils::FastMemCpy(&mOpacity, &data.mOpacity, sizeof(data.mOpacity));
   Utils::FastMemCpy(&mScale, &data.mScale, sizeof(data.mScale));
   Utils::FastMemCpy(&mSpeed, &data.mSpeed, sizeof(data.mSpeed));      
   Utils::FastMemCpy(&mColor, &data.mColor, sizeof(data.mColor));
   Utils::FastMemCpy(&mIntensity, &data.mIntensity, sizeof(data.mIntensity));
   Utils::FastMemCpy(&mForces, &data.mForces, sizeof(data.mForces));
   Utils::FastMemCpy(&mCollision, &data.mCollision, sizeof(data.mCollision));         
   Utils::FastMemCpy(&mEvents, &data.mEvents, sizeof(data.mEvents));

   mpAppFileIndices    = new int             [mAppearance.mNumFiles];
   mpPaletteColors     = new DWORD           [mColor.mNumPaletteColors];
   mpAppearanceWeights = new float           [mAppearance.mNumFiles];
   mpOpacityStages     = new PSOpacityStage  [mOpacity.mNumStages];
   mpScaleStages       = new PSScaleStage    [mScale.mNumStages];
   mpColorStages       = new PSColorStage    [mColor.mNumStages];
   mpSpeedStages       = new PSSpeedStage    [mSpeed.mNumStages];
   mpIntensityStages   = new PSIntensityStage[mIntensity.mNumStages];
   Utils::FastMemCpy(mpAppFileIndices,    data.mpAppFileIndices,        sizeof(int)*mAppearance.mNumFiles);
   Utils::FastMemCpy(mpPaletteColors,     data.mpPaletteColors,         sizeof(DWORD)*mColor.mNumPaletteColors);
   Utils::FastMemCpy(mpAppearanceWeights, data.mpAppearanceWeights,     sizeof(float)*mAppearance.mNumFiles);   
   Utils::FastMemCpy(mpOpacityStages,     data.mpOpacityStages,         sizeof(PSOpacityStage) * mOpacity.mNumStages);
   Utils::FastMemCpy(mpScaleStages,       data.mpScaleStages,           sizeof(PSScaleStage)* mScale.mNumStages);
   Utils::FastMemCpy(mpSpeedStages,       data.mpSpeedStages,           sizeof(PSSpeedStage)* mSpeed.mNumStages);
   Utils::FastMemCpy(mpColorStages,       data.mpColorStages,           sizeof(PSColorStage)* mColor.mNumStages);
   Utils::FastMemCpy(mpIntensityStages,   data.mpIntensityStages,       sizeof(PSIntensityStage)* mIntensity.mNumStages);

   //-- Complex Storage
   //-- data we need to copy by hand and can't just fast copy
   mpAppearanceFiles   = new BSimString         [mAppearance.mNumFiles];
   mpCollisionTypes    = new PSCollisionType [mCollision.mNumTypes];
   mpParticleEvents    = new PSParticleEvent [mEvents.mNumEvents];
   for (long index = 0; index < mAppearance.mNumFiles; ++index)
      mpAppearanceFiles[index] = data.mpAppearanceFiles[index];

   for (long index = 0; index < mCollision.mNumTypes; ++index)
      mpCollisionTypes[index] = data.mpCollisionTypes[index];

   for (long index = 0; index < mEvents.mNumEvents; ++index)
      mpParticleEvents[index] = data.mpParticleEvents[index];
  
   /*
   for (long index = 0; index < data.mAppearance.mNumFiles; ++index)
   mpAppFileIndices[index] = data.mpAppFileIndices[index];

   for (long index = 0; index < mColor.mNumPaletteColors; ++index)
      mpPaletteColors[index] = data.mpPaletteColors[index];

   for (long index = 0; index < mAppearance.mNumFiles; ++index)
      mpAppearanceWeights[index] = data.mpAppearanceWeights[index];

   for (long index = 0; index < mAppearance.mNumFiles; ++index)
      mpAppearanceFiles[index] = data.mpAppearanceFiles[index];

   for (long index = 0; index < mOpacity.mNumStages; ++index)
      mpOpacityStages[index] = data.mpOpacityStages[index];

   for (long index = 0; index < mScale.mNumStages; ++index)
      mpScaleStages[index] = data.mpScaleStages[index];

   for (long index = 0; index < mSpeed.mNumStages; ++index)
      mpSpeedStages[index] = data.mpSpeedStages[index];

   for (long index = 0; index < mColor.mNumStages; ++index)
      mpColorStages[index] = data.mpColorStages[index];

   for (long index = 0; index < mCollision.mNumTypes; ++index)
      mpCollisionTypes[index] = data.mpCollisionTypes[index];

   for (long index = 0; index < mEvents.mNumEvents; ++index)
      mpParticleEvents[index] = data.mpParticleEvents[index];

   for (long index = 0; index < mIntensity.mNumStages; ++index)
      mpIntensityStages[index] = data.mpIntensityStages[index];
   */

   return *this;
}


//============================================================================
//  PRIVATE FUNCTIONS
//============================================================================
bool BParticleSystemData::validate()
{
   if (mAppearance.mNumFiles == 0)
      return false;

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemData::loadMaterials()
{  
   /*
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(gRenderThread.getInitialized());

   int textureArrayID = 0;
   bool bSuccess = gPSTextureManager.loadTextureSet(mpAppearanceFiles, mAppearance.mNumFiles, &textureArrayID);
   BDEBUG_ASSERT(bSuccess == true);

   mAppearanceFileWeight = 0.0f;
   for (long mat = 0; mat < mAppearance.mNumFiles; ++mat)
   {
      mpAppFileIndices[mat] = textureArrayID;

      //-- FIX THIS
      mTextureHeight = 256;
      mTextureWidth  = 256;

      mAppearanceFileWeight += mpAppearanceWeights[mat];
   }
   */
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemData::load3DModels()
{
#if 0
   if((gModelManager==NULL) || (gModelManager->getRender() == NULL))
      return(false);

   mAppearanceFileWeight = 0.0f;
   for (long mat = 0; mat < mAppearance.mNumFiles; ++mat)
   {
      BSimString toLoad(mpAppearanceFiles[mat]);
      toLoad.removeExtension();
      mpAppFileIndices[mat] = gModelManager->getGrannyManager().getOrCreateModel(toLoad, true);  // MWC [7/30/2004] - do we want to force load???

      mAppearanceFileWeight += mpAppearanceWeights[mat];
   }
#endif
   return true;

}
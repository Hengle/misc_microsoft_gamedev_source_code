//============================================================================
// ParticleEffectData.cpp
// Copyright (c) 2006 Ensemble Studios
//============================================================================
#include "xparticlescommon.h"
#include "particleeffectdata.h"
#include "particletexturemanager.h"
#include "particleworkdirsetup.h"
#include "particlesystemmanager.h"
#include "xmlreader.h"
#include "math\VMXUtils.h"
#include "consoleOutput.h"

class BEnumLookup
{
   public:
      int      mEnum;
      char     mName[64];
};
#define BASEENUM(type) {BEmitterBaseData::type, B(#type)},
BEnumLookup gEmitterParticleTypeLookup[BEmitterBaseData::eTypeTotal]=
{
   BASEENUM(eBillBoard)
   BASEENUM(eOrientedAxialBillboard)
   BASEENUM(eUpfacing)
   BASEENUM(eTrail)
   BASEENUM(eTrailCross)
   BASEENUM(eBeam)
   BASEENUM(eVelocityAligned)
   BASEENUM(ePFX)
   BASEENUM(eTerrainPatch)
};

BEnumLookup gEmitterParticleBlendModeLookup[BEmitterBaseData::eBlendModeTotal]=
{
   BASEENUM(eAlphaBlend)
   BASEENUM(eAdditive)
   BASEENUM(eSubtractive)
   BASEENUM(eDistortion)   
   BASEENUM(ePremultipliedAlpha)   
};

BEnumLookup gEmitterTrailEmissionType[BEmitterBaseData::eTrailEmissionTypeTotal]=
{
   BASEENUM(eEmitByLength)
   BASEENUM(eEmitByTime)
};

BEnumLookup gEmitterTrailUVType[BEmitterBaseData::eTrailUVTypeTotal]=
{
   BASEENUM(eStretch)
   BASEENUM(eFaceMap)
};

BEnumLookup gBeamAlignType[BEmitterBaseData::eBeamAlignTypeTotal]=
{
   BASEENUM(eBeamAlignToCamera)
   BASEENUM(eBeamAlignVertical)
   BASEENUM(eBeamAlignHorizontal)
};
#undef BASEENUM

#define SHAPEENUM(type) {BEmitterShapeData::type, B(#type)},
BEnumLookup gEmitterShapeTypeLookup[BEmitterShapeData::eTypeTotal]=
{
   SHAPEENUM(ePoint)
   SHAPEENUM(eBox)
   SHAPEENUM(eCylinder)
   SHAPEENUM(eSphere)
   SHAPEENUM(eHalfSphere)
   SHAPEENUM(eRectangle)
   SHAPEENUM(eCircle)
};
#undef SHAPEENUM

#define COLORENUM(type) {BEmitterColorData::type, B(#type)},
BEnumLookup gEmitterColorTypeLookup[BEmitterColorData::eTotal]=
{
   COLORENUM(eSingleColor)
   COLORENUM(ePalletteColor)
   COLORENUM(eProgression)
};
#undef COLORENUM

#define MULTITEXTUREENUM(type) {BEmitterTextureData::type, B(#type)},
BEnumLookup gEmitterTextureDataBlendTypeLookup[BEmitterTextureData::eBlendTotal]=
{
   MULTITEXTUREENUM(eBlendMultiply)
   MULTITEXTUREENUM(eBlendAlpha)   
};
#undef MULTITEXTUREENUM


#define MAGNETENUM(type) {BMagnetData::type, B(#type)},
BEnumLookup gMagnetTypeLookup[BMagnetData::eMagnetTypeTotal]=
{
   MAGNETENUM(eSphere)
   MAGNETENUM(eCylinder)   
};
#undef MAGNETENUM

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEffectDefinition::deInit()
{
   int count = mEmitters.getSize();
   for (int i = 0; i < count; ++i)
   {
      if (mEmitters[i])
      {         
         mEmitters[i]->deInit();
         ALIGNED_DELETE(mEmitters[i], gParticleHeap);
      }
      mEmitters[i]=NULL;
   }
   mEmitters.clear();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BParticleEffectDefinition::getMemoryCost()
{
   int bytes = 0;

   for (int i = 0; i < mEmitters.getNumber(); ++i)
   {
      if (mEmitters[i])
         bytes += mEmitters[i]->getMemoryCost();
   }

   bytes += sizeof(mRefCount);

   return bytes;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleEffectDefinition::load(const BString& fileName)
{
   BXMLReader reader;
   BString realname = fileName;

   realname.removeExtension();
   realname.append(".pfx");

   mName = realname;

   bool success = reader.load(cDirParticlesRoot, realname);

   if (!success)
   {
      gConsoleOutput.error("BParticleEffectDefinition::load: Failed reading ParticleEmitter from file %s\n", realname.getPtr());
      
      return false;
   }
   
   BXMLNode root(reader.getRootNode());
   
   // -- make sure we are dealing with the right kind of file
   const BPackedString szRootName(root.getName());
   if (szRootName.compare(("ParticleEffect")) != 0)
   {
      gConsoleOutput.error("BParticleEffectDefinition::load: Failed reading ParticleEmitter from file %s\n", fileName.getPtr());
      
      {setBlogError(4138); blogerror("BParticleSystemData::loadXML: this is not a particle definition file.");}
      return (false);
   }

   BXMLNode node;
   int numChildren = root.getNumberChildren();
   for (int i = 0; i < numChildren; ++i)
   {
      node = root.getChild(i);
      const BPackedString szTag(node.getName());
      if (szTag.compare(("ParticleEmitter")) == 0)
      {
         BParticleEmitterDefinition* pNewEmitter = ALIGNED_NEW(BParticleEmitterDefinition, gParticleHeap);

         if (!pNewEmitter->load(node, &reader))
         {
            gConsoleOutput.error("BParticleEffectDefinition::load: Failed reading ParticleEmitter from file %s\n", fileName.getPtr());
            
            ALIGNED_DELETE(pNewEmitter, gParticleHeap);
            return false;
         }

         //-- create the progression textures
         gPSTextureManager.initEmitterProgression(pNewEmitter, 0, &pNewEmitter->mProgressionV);

         #ifndef BUILD_FINAL
         pNewEmitter->mParentPfxName = realname;
         #endif

         mEmitters.pushBack(pNewEmitter);
      }
      else
      {
         gConsoleOutput.warning("BParticleEffectDefinition::load: Unrecognized element name: %s\n", szTag.getPtr());
      }
   }
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitterDefinition::deInit()
{
   mProperties.deInit();
   mShape.deInit();
   mTextures.deInit();
   mOpacity.deInit();
   mScale.deInit();
   mSpeed.deInit();
   mIntensity.deInit();
   mColor.deInit();
   mForce.deInit();   

   int count = mMagnets.getSize();
   for (int i = 0; i < count; ++i)
   {
      BDEBUG_ASSERT(mMagnets[i]!=NULL);
      mMagnets[i]->deInit();
      ALIGNED_DELETE(mMagnets[i], gParticleHeap);
      mMagnets[i] = NULL;
   }
   mMagnets.clear();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BParticleEmitterDefinition::getMemoryCost()
{
   int bytes = 0;

   for (int i = 0; i < mMagnets.getNumber(); ++i)
   {
      if (mMagnets[i])
         bytes += mMagnets[i]->getMemoryCost();
   }

   bytes += mProperties.getMemoryCost();
   bytes += mShape.getMemoryCost();
   bytes += mTextures.getMemoryCost();
   bytes += mOpacity.getMemoryCost();
   bytes += mScale.getMemoryCost();
   bytes += mSpeed.getMemoryCost();
   bytes += mIntensity.getMemoryCost();
   bytes += mColor.getMemoryCost();
   bytes += mForce.getMemoryCost();
      
   bytes += sizeof(mProgressionV);
   bytes += sizeof(mBBoxOffset);
   bytes += sizeof(mMaxEmitterLife);

   return bytes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleEmitterDefinition::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   int numChildren = node.getNumberChildren();
   BXMLNode child;
   for (int i = 0; i < numChildren; ++i)
   {
      child = node.getChild(i);
      const BPackedString szTag(child.getName());
      if (szTag.compare(("EmitterData")) == 0)
      {         
         if (!mProperties.load(child, pReader))
            return false;

         computeMaxEmitterLife();
      }
      else if (szTag.compare(("ShapeData")) == 0)
      {
         if (!mShape.load(child, pReader))
            return false;
      }
      else if (szTag.compare(("TextureData")) == 0)
      {
         if (!mTextures.load(child, pReader))
            return false;
      }
      else if (szTag.compare(("ColorData")) == 0)
      {
         if (!mColor.load(child, pReader))
            return false;
      }
      else if (szTag.compare(("OpacityData")) == 0)
      {
         if (!mOpacity.load(child, pReader))
            return false;
      }
      else if (szTag.compare(("ScaleData")) == 0)
      {
         if (!mScale.load(child, pReader))
            return false;

         computeBBoxOffset();
      }
      else if (szTag.compare(("SpeedData")) == 0)
      {
         //-- load speed and create a lookup table for it
         if (!mSpeed.load(child, pReader))
            return false;         
      }
      else if (szTag.compare(("IntensityData")) == 0)
      {
         BEmitterFloatProgressionData fp;
         if (!mIntensity.load(child, pReader))
            return false;
      }
      else if (szTag.compare(("ForceData")) == 0)
      {
         if (!mForce.load(child, pReader))
            return false;
      }
      else if (szTag.compare(("ParticleMagnet")) == 0)
      {
         BMagnetData* pNewMagnet = ALIGNED_NEW(BMagnetData, gParticleHeap);

         if (!pNewMagnet->load(child, pReader))
         {
            gConsoleOutput.error("BMagnetData::load: Failed reading BMagnetData");            
            ALIGNED_DELETE(pNewMagnet, gParticleHeap);
            return false;
         }         

         mMagnets.pushBack(pNewMagnet);
      }
      else
      {
         gConsoleOutput.warning("BParticleEmitterDefinition::load::load: Unrecognized element name: %s\n", szTag.getPtr());
      }
   }   
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitterDefinition::computeBBoxOffset()
{
   XMVECTOR vScaleValueX    = XMLoadScalar(&mScale.mValue.x);
   XMVECTOR vScaleValueY    = XMLoadScalar(&mScale.mValue.y);
   XMVECTOR vScaleValueVarX = XMLoadScalar(&mScale.mValueVar.x);
   XMVECTOR vScaleValueVarY = XMLoadScalar(&mScale.mValueVar.y);

   //-- uniform scale
   XMVECTOR vScaleValueW    = XMLoadScalar(&mScale.mValue.w);
   XMVECTOR vScaleValueVarW = XMLoadScalar(&mScale.mValueVar.w);

   XMVECTOR vUniformScaleV  = PS_COMPUTE_VARIANCE_X(gVectorOne, vScaleValueW, vScaleValueVarW);
   //-- find the max of x and y
   XMVECTOR vScaleXY = XMVectorMax(PS_COMPUTE_VARIANCE_X(gVectorOne, vScaleValueX, vScaleValueVarX), PS_COMPUTE_VARIANCE_X(gVectorOne, vScaleValueY, vScaleValueVarY));

   //-- scale the uniformely
   vScaleXY = XMVectorMultiply(vScaleXY, vUniformScaleV);

   //-- make the bbox offset
   mBBoxOffset = __vrlimi(XMVectorZero(), XMVectorSplatX(vScaleXY), VRLIMI_CONST(1,1,1,0), 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitterDefinition::computeMaxEmitterLife()
{
   
   XMVECTOR vParticleLife    = XMLoadScalar(&mProperties.mParticleLife);
   XMVECTOR vParticleLifeVar = XMLoadScalar(&mProperties.mParticleLifeVar);
   XMVECTOR vMaxParticleLife = PS_COMPUTE_VARIANCE_X(gVectorOne, vParticleLife, vParticleLifeVar);

   XMVECTOR vEmissionTime    = XMLoadScalar(&mProperties.mEmissionTime);
   XMVECTOR vEmissionTimeVar = XMLoadScalar(&mProperties.mEmissionTimeVar);
   XMVECTOR vMaxEmissionTime = PS_COMPUTE_VARIANCE_X(gVectorOne, vEmissionTime, vEmissionTimeVar);

   XMVECTOR vEmissionRate    = XMLoadScalar(&mProperties.mEmissionRate);
   XMVECTOR vEmissionRateVar = XMLoadScalar(&mProperties.mEmissionRateVar);
   XMVECTOR vMaxEmissionRate = PS_COMPUTE_VARIANCE_X(gVectorOne, vEmissionRate, vEmissionRateVar);

   float maxParticlesFloat = static_cast<float>(mProperties.mMaxParticles);
   XMVECTOR vArtistDefinedMaxParticles    = XMLoadScalar(&maxParticlesFloat);
   XMVECTOR vArtistDefinedMaxParticlesVar = XMLoadScalar(&mProperties.mMaxParticlesVar);
   XMVECTOR vMaxArtistDefinedMaxParticles = PS_COMPUTE_VARIANCE_X(gVectorOne, vArtistDefinedMaxParticles, vArtistDefinedMaxParticlesVar);

   XMVECTOR vMaxParticles;
   XMVECTOR vMaxEmitterLife;
   if ((mProperties.mType == BEmitterBaseData::eTrail) || 
       (mProperties.mType == BEmitterBaseData::eTrailCross))
   {
      if (mProperties.mTrailEmissionType == BEmitterBaseData::eEmitByLength)
      {
         vMaxParticles = vMaxArtistDefinedMaxParticles;
      }
      else
      {
         vMaxParticles= XMVectorMultiply(vMaxEmissionRate, vMaxEmissionTime);
         vMaxParticles = XMVectorMin(vMaxParticles, vMaxArtistDefinedMaxParticles);
      }
   }
   else
   {
      vMaxParticles= XMVectorMultiply(vMaxEmissionRate, vMaxEmissionTime);
      vMaxParticles = XMVectorMin(vMaxParticles, vMaxArtistDefinedMaxParticles);
   }

   vMaxEmitterLife = XMVectorMultiply(vMaxParticles, vMaxParticleLife);
   mMaxEmitterLife = vMaxEmitterLife.x * 1000.0f;

   //trace ("Emitter Max Life = %4.6f", mMaxEmitterLife);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BEmitterBaseData::getMemoryCost()
{
   return sizeof(this);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BEmitterBaseData::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;
         
   node.getChildValue("TiedToEmitter", mTiedToEmitter);
   node.getChildValue("IgnoreRotation", mIgnoreRotation);   
   node.getChildValue("Loop", mLoop);
   node.getChildValue("AlwaysActive", mAlwaysActive);
   node.getChildValue("AlwaysRender", mAlwaysRender);
   node.getChildValue("KillImmediatelyOnRelease", mKillImmediatelyOnRelease);
   
   mLightBufferValueLoaded = node.getChildValue("LightBuffer", mLightBuffer);   
   node.getChildValue("LightBufferIntensityScale", mLightBufferIntensityScale);

   node.getChildValue("MaxParticles", mMaxParticles);
   node.getChildValue("MaxParticlesVar", mMaxParticlesVar);
      
   BString enumString;
   node.getChildValue("ParticleType", &enumString);
   int i;
   for (i = 0; i < eTypeTotal; ++i)
   {
      if (enumString.compare(gEmitterParticleTypeLookup[i].mName) == 0)
      {
         mType = (EmitterParticleTypeEnum) gEmitterParticleTypeLookup[i].mEnum;
         break;
      }
   }
   BDEBUG_ASSERT(i < eTypeTotal);
      
   node.getChildValue("BlendMode", &enumString);
   
   int j;
   for (j = 0; j < eBlendModeTotal; ++j)
   {
      if (enumString.compare(gEmitterParticleBlendModeLookup[j].mName) == 0)
      {
         mBlendMode = (EmitterBlendModeEnum) gEmitterParticleBlendModeLookup[j].mEnum;
         break;
      }
   }
   BDEBUG_ASSERT(j < eBlendModeTotal);

   node.getChildValue("UpdateRadius", mUpdateRadius);
   node.getChildValue("ParticleLife", mParticleLife);
   node.getChildValue("ParticleLifeVar", mParticleLifeVar);

   //-- we have to enforce a minimum lifespan for trail types because they require 2 verts to show up.  
   //-- The lowest lifetime for a trail particle must be at least as long as the max update time the
   //-- game says an update is allowed to take when the frame rate drops.  Currently this is
   //-- set to 0.1 seconds.  So each trail particle has to be at least alive for 0.1 seconds and no less to
   //-- ensure that they are visible even on really low frame rates since we lock step at 0.1
   if (mType == eTrail || mType == eTrailCross)
   {
      const float minTrailParticleLife = 0.11f;
      mParticleLife = Math::ClampLow(mParticleLife, minTrailParticleLife);
   }
   
   node.getChildValue("GlobalFadeIn", mGlobalFadeIn);
   node.getChildValue("GlobalFadeInVar", mGlobalFadeInVar);
   node.getChildValue("GlobalFadeOut", mGlobalFadeOut);
   node.getChildValue("GlobalFadeOutVar", mGlobalFadeOutVar);
   node.getChildValue("EmissionRate", mEmissionRate);
   node.getChildValue("EmissionRateVar", mEmissionRateVar);
   node.getChildValue("StartDelay", mStartDelay);
   node.getChildValue("StartDelayVar", mStartDelayVar);
   node.getChildValue("InitialUpdate", mInitialUpdate);
   node.getChildValue("InitialUpdateVar", mInitialUpdateVar);
   node.getChildValue("EmissionTime", mEmissionTime);
   node.getChildValue("EmissionTimeVar", mEmissionTimeVar);
   node.getChildValue("LoopDelay", mLoopDelay);
   node.getChildValue("LoopDelayVar", mLoopDelayVar);
   node.getChildValue("InitialDistance", mInitialDistance);
   node.getChildValue("InitialDistanceVar", mInitialDistanceVar);
   node.getChildValue("Velocity", mVelocity);
   node.getChildValue("VelocityVar", mVelocityVar);
   node.getChildValue("Acceleration", mAcceleration);
   node.getChildValue("AccelerationVar", mAccelerationVar);
   

   node.getChildValue("TrailEmissionType", &enumString);   
   for (i = 0; i < eTrailEmissionTypeTotal; ++i)
   {
      if (enumString.compare(gEmitterTrailEmissionType[i].mName) == 0)
      {
         mTrailEmissionType = (EmitterTrailEmissionType) gEmitterTrailEmissionType[i].mEnum;
         break;
      }
   }
   BDEBUG_ASSERT(i < eTrailEmissionTypeTotal);
   
   node.getChildValue("TrailUVType", &enumString);
   for (j = 0; j < eTrailUVTypeTotal; ++j)
   {
      if (enumString.compare(gEmitterTrailUVType[j].mName) == 0)
      {
         mTrailUVType = (EmitterTrailUVType) gEmitterTrailUVType[j].mEnum;
         break;
      }
   }
   BDEBUG_ASSERT(j < eTrailUVTypeTotal);

   node.getChildValue("TrailSegmentLength", mTrailSegmentLength);

   node.getChildValue("EmitterAttraction", mEmitterAttraction);
   node.getChildValue("EmitterAttractionVar", mEmitterAttractionVar);
   
   mBeamAlignmentType = eBeamAlignToCamera;
   node.getChildValue("BeamAlignmentType", &enumString);
   for (i = 0; i < eBeamAlignTypeTotal; ++i)
   {
      if (enumString.compare(gBeamAlignType[i].mName) == 0)
      {
         mBeamAlignmentType = (EmitterBeamAlignmentType) gBeamAlignType[i].mEnum;
         break;
      }
   }
   BDEBUG_ASSERT(mBeamAlignmentType < eBeamAlignTypeTotal);

   node.getChildValue("BeamTesselation", mBeamTesselation);
      
   float x = 0.0f;
   float y = 0.0f;
   float z = 0.0f;
   node.getChildValue("BeamTangent1X", x);
   node.getChildValue("BeamTangent1Y", y);
   node.getChildValue("BeamTangent1Z", x);
   mBeamTangent1 = XMVectorSet(x,y,z,0.0f);

   node.getChildValue("BeamTangent2X", x);
   node.getChildValue("BeamTangent2Y", y);
   node.getChildValue("BeamTangent2Z", x);
   mBeamTangent2 = XMVectorSet(x,y,z,0.0f);

   node.getChildValue("BeamColorByLength", mBeamColorByLength);
   node.getChildValue("BeamOpacityByLength", mBeamOpacityByLength);   
   node.getChildValue("BeamIntensityByLength", mBeamIntensityByLength);

   node.getChildValue("PFXFilePath", &mPFXFilepath);

   node.getChildValue("CollisionDetectionTerrain", mCollisionDetectionTerrain);
   node.getChildValue("CollisionEnergyLoss", mCollisionEnergyLoss);
   node.getChildValue("CollisionEnergyLossVar", mCollisionEnergyLossVar);
   node.getChildValue("CollisionOffset", mCollisionOffset);

   node.getChildValue("SortParticles", mSortParticles);
   node.getChildValue("FillOptimized", mFillOptimized);

   float tesselationLevel = 1.0f;
   node.getChildValue("TerrainDecalTesselation", tesselationLevel);
   mTerrainDecalTesselation = Math::Clamp(tesselationLevel, 1.0f, 15.0f);
   node.getChildValue("TerrainDecalYOffset", mTerrainDecalYOffset);

   // sof particles
   node.getChildValue("SoftParticles", mSoftParticles);
   float fadeRange = 1.0f;
   node.getChildValue("SoftParticleFadeRange", fadeRange);
   mSoftParticleFadeRange = Math::Clamp(fadeRange, 0.1f, 2.0f);          
   
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BEmitterShapeData::getMemoryCost()
{
   return sizeof(this);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BEmitterShapeData::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   BString enumString;
   node.getChildValue("ShapeType", &enumString);   
   
   int i;
   for (i = 0; i < eTypeTotal; ++i)
   {
      if (enumString.compare(gEmitterShapeTypeLookup[i].mName) == 0)
      {
         mType = (ShapeTypeEnum) gEmitterShapeTypeLookup[i].mEnum;
         break;
      }
   }
   BDEBUG_ASSERT(i < eTypeTotal);

   node.getChildValue("XSize", mXSize);
   node.getChildValue("YSize", mYSize);
   node.getChildValue("ZSize", mZSize);
   node.getChildValue("XPosOffset", mXPosOffset);
   node.getChildValue("YPosOffset", mYPosOffset);
   node.getChildValue("ZPosOffset", mZPosOffset);
   node.getChildValue("TrajectoryInnerAngle", mTrajectoryInnerAngle);
   node.getChildValue("TrajectoryOuterAngle", mTrajectoryOuterAngle);
   node.getChildValue("TrajectoryPitch", mTrajectoryPitch);
   node.getChildValue("TrajectoryYaw", mTrajectoryYaw);
   node.getChildValue("TrajectoryBank", mTrajectoryBank);
   node.getChildValue("EmitFromSurfaceRadius", mEmitFromSurfaceRadius);
   node.getChildValue("EmitFromSurface", mEmitFromSurface);

   // bullet proof trajectory angles
   mTrajectoryInnerAngle = Math::ClampHigh(mTrajectoryInnerAngle, mTrajectoryOuterAngle);
   mTrajectoryOuterAngle = Math::ClampLow(mTrajectoryOuterAngle, mTrajectoryInnerAngle);

   init();

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BEmitterShapeData::init()
{
   if (mType == eBox && mEmitFromSurface)
   {
      float xSpan = mXSize * 2;
      float ySpan = mYSize * 2;
      float zSpan = mZSize * 2;

      mTopFactor   = xSpan * zSpan;
      mSideFactor  = ySpan * zSpan;
      mFrontFactor = xSpan * ySpan;
      mTotalFactor = mTopFactor + mSideFactor + mFrontFactor;

      if (mTotalFactor < cFloatCompareEpsilon)
         mType = ePoint;
   }
   else if (mType == eRectangle)
   {
      float xSpan = mXSize * 2;
      float ySpan = mYSize * 2;

      mTopFactor   = xSpan;
      mSideFactor  = ySpan;
      mFrontFactor = 0;
      mTotalFactor = mTopFactor + mSideFactor + mFrontFactor;

      if (mTotalFactor < cFloatCompareEpsilon)
         mType = ePoint;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BEmitterTextureData::getMemoryCost()
{
   int bytes = 0;

   bytes += sizeof(mDiffuseLayer1To2BlendMode);
   bytes += sizeof(mDiffuseLayer2To3BlendMode);

   bytes += mDiffuse.getMemoryCost();
   bytes += mDiffuse2.getMemoryCost();
   bytes += mDiffuse3.getMemoryCost();
   bytes += mMasks.getMemoryCost();
   bytes += mIntensity.getMemoryCost();

   return bytes;      
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BEmitterTextureData::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   int numChildren = node.getNumberChildren();
   BXMLNode child;
   for (int i = 0; i < numChildren; ++i)
   {
      child = node.getChild(i);
      const BPackedString szTag(child.getName());
      if (szTag.compare("DiffuseLayer1To2BlendMode") == 0)
      {
         BString enumString;
         child.getTextAsString(enumString);
         //for (int i = 0; i < MultiTextureBlendTypeEnum::eBlendTotal; ++i)
         int j;
         for (j = 0; j < eBlendTotal; ++j)
         {
            if (enumString.compare(gEmitterTextureDataBlendTypeLookup[j].mName) == 0)
            {
               mDiffuseLayer1To2BlendMode = (MultiTextureBlendTypeEnum) gEmitterTextureDataBlendTypeLookup[j].mEnum;
               break;
            }
         }
         BDEBUG_ASSERT(j < eBlendTotal);
      }
      else if (szTag.compare("DiffuseLayer2To3BlendMode") == 0)
      {
         BString enumString;
         child.getTextAsString(enumString);
         //for (int i = 0; i < MultiTextureBlendTypeEnum::eBlendTotal; ++i)
         int j;
         for (j = 0; j < eBlendTotal; ++j)
         {
            if (enumString.compare(gEmitterTextureDataBlendTypeLookup[j].mName) == 0)
            {
               mDiffuseLayer2To3BlendMode = (MultiTextureBlendTypeEnum) gEmitterTextureDataBlendTypeLookup[j].mEnum;
               break;
            }
         }
         BDEBUG_ASSERT(j < eBlendTotal);
      }
      else if (szTag.compare(("Diffuse")) == 0)
      {
         if (!mDiffuse.load(child, pReader))
            return false;
      }
      else if (szTag.compare(("Diffuse2")) == 0)
      {
         if (!mDiffuse2.load(child, pReader))
            return false;
      }
      else if (szTag.compare(("Diffuse3")) == 0)
      {
         if (!mDiffuse3.load(child, pReader))
            return false;
      }
      else if (szTag.compare(("Masks")) == 0)
      {
         if (!mMasks.load(child, pReader))
            return false;
      }
      else if (szTag.compare(("Intensity")) == 0)
      {
         if (!mIntensity.load(child, pReader))
            return false;
      }
      else
      {
         gConsoleOutput.warning("BEmitterTextureData::load: Unrecognized element name: %s\n", szTag.getPtr());
      }

   }
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BEmitterTextureData::deInit()
{
   mDiffuse.deInit();
   mDiffuse2.deInit();
   mDiffuse3.deInit();
   mMasks.deInit();
   mIntensity.deInit();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BEmitterTextureSet::getMemoryCost()
{
   int bytes = 0;
   for (int i = 0; i < mStages.getNumber(); ++i)
      bytes += mStages[i].getMemoryCost();

   bytes += mUVAnimation.getMemoryCost();
   bytes += sizeof(mTotalWeight);
   bytes += sizeof(mInvTextureArraySize);
   bytes += sizeof(mWidth);
   bytes += sizeof(mHeight);
   bytes += sizeof(mTextureArrayIndex);
   bytes += sizeof(mTextureArrayNumUsed);
   bytes += mTextureArraySlotIndices.getSizeInBytes();

   return bytes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BEmitterTextureSet::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   int numChildren = node.getNumberChildren();     
   BXMLNode child;
   for (int i = 0; i < numChildren; ++i)
   {
      child = node.getChild(i);
      const BPackedString& szTag(child.getName());
      if (szTag.compare(("Textures")) == 0)
      {
         int count = child.getNumberChildren();
         mStages.resize(count);
         BXMLNode grandChild;
         for (int j = 0; j < count; ++j)
         {
            grandChild = child.getChild(j);
            if (!mStages[j].load(grandChild, pReader))
               return false;
         }
      }
      else if (szTag.compare(("UVAnimation")) == 0)
      {
         if (!mUVAnimation.load(child, pReader))
            return false;
      }      
   }

   init();

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BEmitterTextureSet::init()
{
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(gRenderThread.getInitialized());

   mTextureArrayIndex = -1;
   mTextureArraySlotIndices.resize(0);
   mInvTextureArraySize = 0.0f;
   mTextureArrayNumUsed = 0;
   mWidth = 0;
   mHeight = 0;
   
   //-- if there are no stages then just default to nothing
   if (mStages.getSize() == 0)
      return true;
      
   uint width, height;
   BParticleTextureManager::BTextureArrayIndex textureArrayIndex;
   BParticleTextureArrayManager::BTextureArraySlotIndices slotIndices;
   bool bSuccess = gPSTextureManager.loadTextureSet(this, textureArrayIndex, slotIndices, width, height);
   if (!bSuccess)
   {      
      gConsoleOutput.error("BEmitterTextureSet::init: loadTextureSet() failed");
      return false;
   }
   
   mTextureArrayIndex = (short)textureArrayIndex;
   mWidth  = (short)width;
   mHeight = (short)height;
   mTextureArraySlotIndices.resize(slotIndices.getSize());
   for (uint i = 0; i < slotIndices.getSize(); i++)
   {
      BDEBUG_ASSERT((slotIndices[i] >= 0) && (slotIndices[i] <= UCHAR_MAX));
      mTextureArraySlotIndices[i] = (uchar)slotIndices[i];
   }
   
   mTextureArrayNumUsed = static_cast<short>(gPSTextureManager.getTextureArrayNumUsed(textureArrayIndex));
   mInvTextureArraySize = 1.0f / gPSTextureManager.getTextureArraySize(textureArrayIndex);
   
   mTotalWeight = 0.0f;
   for (uint i = 0; i < mStages.getSize(); ++i)
   {
      mTotalWeight += mStages[i].mWeight;
   }
   return true;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BEmitterTextureStage::getMemoryCost()
{
   return sizeof(this);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BEmitterTextureStage::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   BString value;
   if (!node.getChildValue("File", &value))
      return false;
   
   mFilename = value;

   node.getChildValue("Weight", mWeight);
   
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BEmitterUVAnimation::getMemoryCost()
{
   return sizeof(this);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BEmitterUVAnimation::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   node.getChildValue("UVAnimationEnabled", mUseUVAnimation);
   node.getChildValue("NumFrames", mNumFrames);
   node.getChildValue("FrameWidth", mFrameWidth);
   node.getChildValue("FrameHeight", mFrameHeight);
   node.getChildValue("ScrollU", mScrollU);
   node.getChildValue("ScrollV", mScrollV);
   node.getChildValue("UseRandomScrollOffsetU", mUseRandomScrollOffsetU);
   node.getChildValue("UseRandomScrollOffsetV", mUseRandomScrollOffsetV);

   int framesPerSecond = 0;
   node.getChildValue("FramesPerSecond", framesPerSecond);
   mFramesPerSecond = 1.0f;
   if (framesPerSecond > 0)
   {      
      mFramesPerSecond = (float)framesPerSecond;
   }

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BEmitterColorData::getMemoryCost()
{
   int bytes = 0;
   if (mpProgression)
      bytes += mpProgression->getMemoryCost();

   for (int i = 0; i < mPallette.getNumber(); ++i)
      bytes += mPallette[i].getMemoryCost();

   bytes += sizeof(mColor);
   bytes += sizeof(mVertex1Color);
   bytes += sizeof(mVertex2Color);
   bytes += sizeof(mVertex3Color);
   bytes += sizeof(mVertex4Color);
   bytes += sizeof(mPlayerColorIntensity);
   bytes += sizeof(mSunColorIntensity);
   bytes += sizeof(mType);
   bytes += sizeof(mPlayerColor);
   bytes += sizeof(mSunColor);

   return bytes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BEmitterColorData::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   int numChildren = node.getNumberChildren();     
   BXMLNode child;
   for (int i = 0; i < numChildren; ++i)
   {
      child = node.getChild(i);
      const BPackedString& szTag(child.getName());
      if (szTag.compare(("Type")) == 0)
      {
         BString enumString;
         child.getTextAsString(enumString);

         int j;         
         for (j = 0; j < eTotal; ++j)
         {
            if (enumString.compare(gEmitterColorTypeLookup[j].mName) == 0)
            {
               mType = (ColorTypeEnum) gEmitterColorTypeLookup[j].mEnum;
               break;
            }
         }
         BDEBUG_ASSERT(j < eTotal);
      }
      else if (szTag.compare(("Color")) == 0)
      {
         DWORD color = 0;
         child.getTextAsDWORD(color);
         XMCOLOR xmColor;
         xmColor.c = color;
         mColor = XMLoadColor(&xmColor);
      }
      else if (szTag.compare(("ColorVertex1")) == 0)
      {
         DWORD color = 0;
         child.getTextAsDWORD(color);
         XMCOLOR xmColor;
         xmColor.c = color;
         mVertex1Color = XMLoadColor(&xmColor);
      }
      else if (szTag.compare(("ColorVertex2")) == 0)
      {
         DWORD color = 0;
         child.getTextAsDWORD(color);
         XMCOLOR xmColor;
         xmColor.c = color;
         mVertex2Color = XMLoadColor(&xmColor);
      }
      else if (szTag.compare(("ColorVertex3")) == 0)
      {
         DWORD color = 0;
         child.getTextAsDWORD(color);
         XMCOLOR xmColor;
         xmColor.c = color;
         mVertex3Color = XMLoadColor(&xmColor);
      }
      else if (szTag.compare(("ColorVertex4")) == 0)
      {
         DWORD color = 0;
         child.getTextAsDWORD(color);
         XMCOLOR xmColor;
         xmColor.c = color;
         mVertex4Color = XMLoadColor(&xmColor);
      }
      else if (szTag.compare(("ColorProgression")) == 0)
      {
         mpProgression = ALIGNED_NEW(BColorProgression, gParticleHeap);
         if (!mpProgression->load(child, pReader))
            return false;
      }
      else if (szTag.compare(("ColorPallette")) == 0)
      {
         int numColors = child.getNumberChildren();
         mPallette.resize(numColors);
         BXMLNode grandChild;
         
         for (int j = 0; j < numColors; ++j)
         {
            grandChild = child.getChild(j);
            if (!mPallette[j].load(grandChild, pReader))
               return false;
         }
      }
      else if (szTag.compare(("PlayerColor")) == 0)
      {
         child.getTextAsBool(mPlayerColor);
      }
      else if (szTag.compare(("PlayerColorIntensity")) == 0)
      {
         float value = 1.0f;
         if (child.getTextAsFloat(value))
         {
             value = Math::Clamp(value, 0.0f, 1.0f);
             mPlayerColorIntensity = XMVectorSet(value, value, value, 1.0f);
         }
      }      
      else if (szTag.compare(("SunColor")) == 0)
      {
         child.getTextAsBool(mSunColor);
      }
      else if (szTag.compare(("SunColorIntensity")) == 0)
      {
         float value = 1.0f;
         if (child.getTextAsFloat(value))
         {
            value = Math::Clamp(value, 0.0f, 1.0f);
            mSunColorIntensity = XMVectorSet(value, value, value, 1.0f);
         }
      }      
      else
      {
         gConsoleOutput.warning("BEmitterColorData::load: Unrecognized element name: %s\n", szTag.getPtr());
      }
   }
   return true;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BEmitterColorData::deInit()
{
   if (mpProgression)
      ALIGNED_DELETE(mpProgression, gParticleHeap);
   mpProgression = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BEmitterFloatProgressionData::getMemoryCost()
{
   int bytes = 0;
   if (mpProgression)
      bytes += mpProgression->getMemoryCost();

   bytes += sizeof(mValue);
   bytes += sizeof(mValueVar);
   bytes += sizeof(mUseProgression);

   return bytes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BEmitterFloatProgressionData::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   int numChildren = node.getNumberChildren();     
   BXMLNode child;
   for (int i = 0; i < numChildren; ++i)
   {
      child = node.getChild(i);
      const BPackedString& szTag(child.getName());
      if (szTag.compare(("UseProgression")) == 0)
      {
         child.getTextAsBool(mUseProgression);
      }
      else if (szTag.compare(("Value")) == 0)
      {
         child.getTextAsFloat(mValue);
      }
      else if (szTag.compare(("ValueVariance")) == 0)
      {
         child.getTextAsFloat(mValueVar);
      }
      else if (szTag.compare(("Progression")) == 0)
      {
         mpProgression = ALIGNED_NEW(BFloatProgression, gParticleHeap);
         if (!mpProgression->load(child, pReader))
            return false;
      }
      else
      {
         gConsoleOutput.warning("BEmitterFloatProgressionData::load: Unrecognized element name: %s\n", szTag.getPtr());
      }
   }

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BEmitterFloatProgressionData::deInit()
{
   if (mpProgression)
      ALIGNED_DELETE(mpProgression, gParticleHeap);
   mpProgression = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BEmitterVectorProgressionData::getMemoryCost()
{
   int bytes = 0;

   if (mpProgression)
      bytes += mpProgression->getMemoryCost();

   bytes += sizeof(mValue);
   bytes += sizeof(mValueVar);
   bytes += mLookupTable.getSizeInBytes();
   bytes += sizeof(mUseXProgression);
   bytes += sizeof(mUseYProgression);
   bytes += sizeof(mUseZProgression);
   bytes += sizeof(mUseVarianceAndProgression);

   return bytes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BEmitterVectorProgressionData::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   int numChildren = node.getNumberChildren();     
   BXMLNode child;
   float x = 1.0f;
   float y = 1.0f;
   float z = 1.0f;
   float w = 1.0f;
   float xVar = 0.0f;
   float yVar = 0.0f;
   float zVar = 0.0f;
   float wVar = 0.0f;
   for (int i = 0; i < numChildren; ++i)
   {
      child = node.getChild(i);
      const BPackedString szTag(child.getName());
      if (szTag.compare(("UseXProgression")) == 0)
      {
         child.getTextAsBool(mUseXProgression);
      }
      else if (szTag.compare(("UseYProgression")) == 0)
      {
         child.getTextAsBool(mUseYProgression);
      }
      else if (szTag.compare(("UseZProgression")) == 0)
      {
         child.getTextAsBool(mUseZProgression);
      }
      else if (szTag.compare(("ValueX")) == 0)
      {
         child.getTextAsFloat(x);
      }
      else if (szTag.compare(("ValueXVariance")) == 0)
      {
         child.getTextAsFloat(xVar);
      }
      else if (szTag.compare(("ValueY")) == 0)
      {
         child.getTextAsFloat(y);
      }
      else if (szTag.compare(("ValueYVariance")) == 0)
      {
         child.getTextAsFloat(yVar);
      }
      else if (szTag.compare(("ValueZ")) == 0)
      {
         child.getTextAsFloat(z);
      }
      else if (szTag.compare(("ValueZVariance")) == 0)
      {
         child.getTextAsFloat(zVar);
      }
      else if (szTag.compare(("UniformValue")) == 0)
      {
         child.getTextAsFloat(w);
      }
      else if (szTag.compare(("UniformValueVariance")) == 0)
      {
         child.getTextAsFloat(wVar);
      }
      else if (szTag.compare(("Progression")) == 0)
      {
         mpProgression = ALIGNED_NEW(BVectorProgression, gParticleHeap);
         if (!mpProgression->load(child, pReader))
            return false;
      }
      else
      {
         gConsoleOutput.warning("BEmitterVectorProgressionData::load: Unrecognized element name: %s\n", szTag.getPtr());
      }
   }

   mValue = XMVectorSet(x,y,z,w);
   mValueVar = XMVectorSet(xVar, yVar, zVar, wVar);

   mUseVarianceAndProgression = true;
   if (mpProgression)
   {
      if (!mUseXProgression && 
          !mUseYProgression && 
          !mUseZProgression &&          
          mValueVar.x <= cFloatCompareEpsilon && 
          mValueVar.y <= cFloatCompareEpsilon &&
          mValueVar.z <= cFloatCompareEpsilon)
      {
         mUseVarianceAndProgression = false;
      }      

      initLookupTable(128);
   }   
   else
   {
      initLookupTable(1);
   }

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BEmitterVectorProgressionData::deInit()
{
   if (mpProgression)
      ALIGNED_DELETE(mpProgression, gParticleHeap);
   mpProgression = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BEmitterVectorProgressionData::initLookupTable(int entryCount)
{
   mLookupTable.clear();
   mLookupTable.reserve(entryCount);

   if (!mpProgression)
   {
      XMDHENN3 packedValue;
      XMStoreDHenN3(&packedValue, gVectorOne);
      for (int i = 0; i < entryCount; i++)
         mLookupTable[i] = packedValue;

      return;
   }

   float oneOverDivisor = 1.0f / ((float) (entryCount - 1));   
   XMVECTOR x,y,z;
   x;
   XMVECTOR value;
   XMDHENN3 packedValue;
   XMVECTOR unpackedValue;
   for (int i = 0; i < entryCount; i++)
   {
      float alpha = (float) i * oneOverDivisor;

      if (mUseXProgression)
         mpProgression->mXProgression.getValue(alpha, 0, &value);
      else
         value = gVectorOne;

      if (mUseYProgression)
         mpProgression->mYProgression.getValue(alpha, 0, &y);
      else
         y = gVectorOne;

      if (mUseZProgression)
         mpProgression->mZProgression.getValue(alpha, 0, &z);
      else
         z = gVectorOne;

      value   =__vrlimi(value, y, VRLIMI_CONST(0,1,0,0), 3); // get Y
      value   =__vrlimi(value, z, VRLIMI_CONST(0,0,1,0), 2); // get Z

      XMStoreDHenN3(&packedValue, value);

      unpackedValue = XMLoadDHenN3(&packedValue);

      mLookupTable.add(packedValue);

      
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BEmitterForceData::getMemoryCost()
{
   return sizeof(BEmitterForceData);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BEmitterForceData::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   node.getChildValue("RandomOrientation", mRandomOrientation); 
   node.getChildValue("UseTumble", mTumble);
   node.getChildValue("MinAngularTumbleVelocity", mMinAngularTumbleVelocity);
   node.getChildValue("MaxAngularTumbleVelocity", mMaxAngularTumbleVelocity);
   node.getChildValue("TumbleBothDirections", mTumbleBothDirections);
   node.getChildValue("UseInternalGravity", mUseInternalGravity);
   node.getChildValue("InternalGravity", mInternalGravity);
   node.getChildValue("InternalGravityVar", mInternalGravityVar);
   node.getChildValue("UseInternalWind", mUseInternalWind);
   node.getChildValue("InternalWindDirection", mInternalWindDirection);
   node.getChildValue("InternalWindDirectionVar", mInternalWindDirectionVar);
   node.getChildValue("InternalWindSpeed", mInternalWindSpeed);
   node.getChildValue("InternalWindSpeedVar", mInternalWindSpeedVar);
   node.getChildValue("InternalWindDelay", mInternalWindDelay);
   node.getChildValue("InternalWindDelayVar", mInternalWindDelayVar);   
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BMagnetData::getMemoryCost()
{
   return sizeof(BMagnetData);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BMagnetData::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   BString enumString;
   node.getChildValue("MagnetType", &enumString);
   int i;
   for (i = 0; i < eMagnetTypeTotal; ++i)
   {
      if (enumString.compare(gMagnetTypeLookup[i].mName) == 0)
      {
         mType = (MagnetTypeEnum) gMagnetTypeLookup[i].mEnum;
         break;
      }
   }
   BDEBUG_ASSERT(i < eMagnetTypeTotal);

   node.getChildValue("Force", mForce); 
   node.getChildValue("RotationalForce", mRotationalForce);
   node.getChildValue("Radius", mRadius); 
   node.getChildValue("Height", mHeight); 
   node.getChildValue("Turbulence", mTurbulence);
   node.getChildValue("Dampening", mDampening);

   //-- Offset
   float x = 0.0f;
   float y = 0.0f;
   float z = 0.0f;
   node.getChildValue("XPosOffset", x); 
   node.getChildValue("YPosOffset", y); 
   node.getChildValue("ZPosOffset", z); 
   mPosOffset = XMVectorSet(x,y,z,0.0f);

   //-- Roatation
   float xRot = 0.0f;
   float yRot = 0.0f;
   float zRot = 0.0f;
   node.getChildValue("XRotation", xRot); 
   node.getChildValue("YRotation", yRot); 
   node.getChildValue("ZRotation", zRot);    
   
   mRotation = XMMatrixRotationRollPitchYaw(xRot * cRadiansPerDegree, yRot * cRadiansPerDegree, zRot * cRadiansPerDegree);   
   return true;
}
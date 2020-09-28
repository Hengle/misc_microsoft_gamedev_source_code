//============================================================================
//
//  ParticleSystemParticle.cpp
//
//  Copyright (c) 2000 Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xparticlescommon.h"
#include "MathUtil.h"
#include "ParticleSystemData.h"
#include "ParticleSystemParticle.h"
#include "particleeffectdata.h"
#include "particleemitter.h"
#include "ParticleSystemManager.h"
#include "timer.h"
#include "math\VMXUtils.h"

//============================================================================
//  CONSTANTS
//============================================================================
static const long MAX_COLLIDABLE_TIME_SLICE = 250;

//============================================================================
//  MACROS
//============================================================================
#define RANDOM(range)          BParticleSystemManager::getRandom(range)
#define RANDOM_RANGE(min, max) BParticleSystemManager::getRandomRange(min, max)
#define RTVARIANCE(index, val, var)     BParticleSystemManager::getIndexedVarience(index, val, var)
#define RTTIMEVARIANCE(index, val, var) ((int)(BParticleSystemManager::getIndexedVarience(index, val, var)*1000.0f))
#define GET_DATA               BASSERT(mpParent);                                \
                               BASSERT(mpParent->getData());                     \
                               if (!mpParent || !mpParent->getData()) return;    \
                               BParticleEmitterDefinition* pData = mpParent->getData();
#define GET_DATA2              BASSERT(mpParent);                                \
                               BASSERT(mpParent->getData());                     \
                               if (!mpParent || !mpParent->getData()) return 0;  \
                               BParticleEmitterDefinition* pData = mpParent->getData();

//============================================================================
//  INTERFACE
//============================================================================
void BParticleSystemParticle::reset()
{
   BCOMPILETIMEASSERT(sizeof(BParticleSystemParticle) == 64);
   mTumbleParams.x    = 0;
   mTumbleParams.y    = 0;
   mState             = eStateDead;
   mVarience          = 0;

   mTextureArrayZ.c = 0;
   mpEffect=NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemParticle::init(BParticleEmitter* RESTRICT pParent, int index, int currentTime, BMatrix emitterMatrix)
{
   reset();

   //-- Validate.
   BDEBUG_ASSERT(pParent);

//-- FIXING PREFIX BUG ID 7232
   const BParticleEmitterDefinition* pEmitterData = pParent->getData();
//--
   BDEBUG_ASSERT(pEmitterData!=NULL);
  
   //-- Init system data.
   mVarience      = (WORD) index;
   mState         = eStateActive;
   mBirthTime     = currentTime;
   mDeathTime     = computeDeathTime(pEmitterData);
   
   mFBirthTime    = currentTime * .001f; 
   mFOOLifeTime   = 1.0f / ((mDeathTime - mBirthTime) * .001f);
      
   if (pEmitterData->mProperties.mType == BEmitterBaseData::eTrail || 
       pEmitterData->mProperties.mType == BEmitterBaseData::eTrailCross ||
       pEmitterData->mProperties.mType == BEmitterBaseData::eBeam)

   {
      initTrail(pParent, pEmitterData, emitterMatrix);
      initAppearance(pEmitterData);
   }
   else if (pEmitterData->mProperties.mType == BEmitterBaseData::ePFX)
   {
      initPosition(pParent, pEmitterData);
      initVelocity(pEmitterData);
      initFinish(pParent, pEmitterData, emitterMatrix);
      initEffect(pParent, pEmitterData, emitterMatrix);
   }
   else
   {
      //-- Init all the other stuff.
      initPosition      (pParent, pEmitterData);
      initVelocity      (pEmitterData);
      initAppearance    (pEmitterData);
      initFinish        (pParent, pEmitterData, emitterMatrix);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BParticleSystemParticle::computeDeathTime(const BParticleEmitterDefinition* RESTRICT pEmitterData) const
{
   BDEBUG_ASSERT(pEmitterData != NULL);

   float indexedVariance = BParticleSystemManager::getIndexedVarience(mVarience, pEmitterData->mProperties.mParticleLife, pEmitterData->mProperties.mParticleLifeVar);
   float lifeSpan = indexedVariance * 1000.0f;
   if (lifeSpan < cFloatCompareEpsilon)
   {
      return 0;
   }
   
   int deathTime = (int)(mBirthTime + lifeSpan);
   return deathTime;
}

//============================================================================
//  PRIVATE INIT FUNCTIONS
//============================================================================
void BParticleSystemParticle::initPosition(const BParticleEmitter* RESTRICT pParent, const BParticleEmitterDefinition* RESTRICT pData)
{
   BDEBUG_ASSERT(pData!=NULL);
   BDEBUG_ASSERT(pParent!=NULL);

   //-- Choose a point based on the emitter shape.
   chooseEmissionPoint(pParent, pData, &mPosition);   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemParticle::initVelocity(const BParticleEmitterDefinition* RESTRICT pData)
{
   BDEBUG_ASSERT(pData!=NULL);

   if (pData->mProperties.mVelocity != 0.0f)
   {
      float minYRotation = 0.0f;  
      float maxYRotation = 360.0f;
      float minZRotation = pData->mShape.mTrajectoryInnerAngle * 0.5f;
      float maxZRotation = pData->mShape.mTrajectoryOuterAngle * 0.5f;
      float yVectorRotation = RANDOM_RANGE(minYRotation, maxYRotation) * cRadiansPerDegree;
      float zVectorRotation = RANDOM_RANGE(minZRotation, maxZRotation) * cRadiansPerDegree;

      if (zVectorRotation >= XM_PI)
         zVectorRotation = XM_PI - cFloatCompareEpsilon;

      float sinOfRotZ, cosOfRotZ;
      XMScalarSinCosEst(&sinOfRotZ, &cosOfRotZ, zVectorRotation);

      BVector temp;
      temp.set(sinOfRotZ, cosOfRotZ, 0.0f);
      BMatrix mtx;
      mtx.makeRotateY(yVectorRotation);
      mVelocity = XMVector3Transform(temp, (XMMATRIX)mtx);

      //-- trajectory calculation of a vector within the limitation of a cone
      float xRotation = pData->mShape.mTrajectoryPitch * cRadiansPerDegree;
      float yRotation = pData->mShape.mTrajectoryYaw * cRadiansPerDegree;
      float zRotation = pData->mShape.mTrajectoryBank * cRadiansPerDegree;
      mtx.makeRotateX(xRotation);
      mtx.multRotateY(yRotation);
      mtx.multRotateZ(zRotation);
      mVelocity = XMVector3Transform(mVelocity, (XMMATRIX)mtx);
         
      //-- Scale by desired velocity.
      mVelocity *= RTVARIANCE(mVarience, pData->mProperties.mVelocity, pData->mProperties.mVelocityVar);
      mVelocity.w = 0.0f;
   }
   else 
      mVelocity = XMVectorZero();

   //-- Init the initial orientation.

   //-- compute angle
   mTumbleParams.x = 0;
   if (pData->mForce.mRandomOrientation)
   {
      float tumbleAngle = RANDOM_RANGE(0.0f, cTwoPi);
      mTumbleParams.x = XMConvertFloatToHalf(tumbleAngle);
   }
   
   //-- Init the tumble velocity.
   mTumbleParams.y = 0;
   if (pData->mForce.mTumble)
   {
      float tumbleVelocity  = RANDOM_RANGE(pData->mForce.mMinAngularTumbleVelocity, pData->mForce.mMaxAngularTumbleVelocity);
      tumbleVelocity  *= cRadiansPerDegree;
      
      if (pData->mForce.mTumbleBothDirections)
      {
         if (RANDOM(2) == 0)
            tumbleVelocity = -tumbleVelocity;
      }
      mTumbleParams.y = XMConvertFloatToHalf(tumbleVelocity);
   }        
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemParticle::initAppearance(const BParticleEmitterDefinition* pData)
{
   BDEBUG_ASSERT(pData!=NULL);

   mTextureArrayZ.r = initTextureArrayZ(&pData->mTextures.mDiffuse);
   BDEBUG_ASSERT( (mTextureArrayZ.r < (uint)pData->mTextures.mDiffuse.mTextureArrayNumUsed) || (!pData->mTextures.mDiffuse.mTextureArrayNumUsed && !mTextureArrayZ.r) );
   
   mTextureArrayZ.g = initTextureArrayZ(&pData->mTextures.mDiffuse2);
   BDEBUG_ASSERT( (mTextureArrayZ.g < (uint)pData->mTextures.mDiffuse2.mTextureArrayNumUsed) || (!pData->mTextures.mDiffuse2.mTextureArrayNumUsed && !mTextureArrayZ.g) );
   
   mTextureArrayZ.b = initTextureArrayZ(&pData->mTextures.mDiffuse3);
   BDEBUG_ASSERT( (mTextureArrayZ.b < (uint)pData->mTextures.mDiffuse3.mTextureArrayNumUsed) || (!pData->mTextures.mDiffuse3.mTextureArrayNumUsed && !mTextureArrayZ.b) );
   
   mTextureArrayZ.a = initTextureArrayZ(&pData->mTextures.mIntensity);  
   BDEBUG_ASSERT( (mTextureArrayZ.a < (uint)pData->mTextures.mIntensity.mTextureArrayNumUsed) || (!pData->mTextures.mIntensity.mTextureArrayNumUsed && !mTextureArrayZ.a) );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE BParticleSystemParticle::initTextureArrayZ(const BEmitterTextureSet* pTextureSet)
{
   BDEBUG_ASSERT(pTextureSet!= NULL);
   //-- Sanity check.
   uint stageCount  = pTextureSet->mStages.getSize();
   if (stageCount <= 1)
   {
      return pTextureSet->mTextureArraySlotIndices.getSize() ? pTextureSet->mTextureArraySlotIndices[0] : 0;
   }
   
   //-- Choose a diffuse appearance file.
   BYTE textureArrayIndex = 0;   
   if (pTextureSet->mTotalWeight < cFloatCompareEpsilon)
   {
      textureArrayIndex = (BYTE) RANDOM(pTextureSet->mTextureArraySlotIndices.getSize());
   }
   else
   {
      float sum = 0.0f;
      float val = RANDOM_RANGE(0.0f, pTextureSet->mTotalWeight);
      for (uint index = 0; index < stageCount; ++index)
      {
         sum += pTextureSet->mStages[index].mWeight;
         if (sum > val)
         {
            textureArrayIndex = (BYTE) index;
            break;
         }
      }
   }
         
   return pTextureSet->mTextureArraySlotIndices.getSize() ? pTextureSet->mTextureArraySlotIndices[textureArrayIndex] : 0;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemParticle::initTrail(BParticleEmitter* RESTRICT pParent, const BParticleEmitterDefinition* RESTRICT pData, BMatrix emitterMatrix)
{
   //-- this assumes that trails are point emitters always
   //-- set the position   
   mPosition = XMVectorSet(pData->mShape.mXPosOffset, pData->mShape.mYPosOffset, pData->mShape.mZPosOffset, 0.0f);

   //-- get up vector
   mVelocity = emitterMatrix.r[1];   
   mTumbleParams.x = 0;
   mTumbleParams.y = 0;

   if (!pData->mProperties.mTiedToEmitter)
   {
      mPosition = XMVector3Transform(mPosition, emitterMatrix);      
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemParticle::initFinish(BParticleEmitter* RESTRICT pParent,  const BParticleEmitterDefinition* RESTRICT pData, BMatrix emitterMatrix)
{   
   BDEBUG_ASSERT(pData!=NULL);
   BDEBUG_ASSERT(pParent);

   //-- if the particle is not attached to the emitter at all times then we need to add
   //-- the emitters world matrix to the particle only when the particle is first created.
   if (!pData->mProperties.mTiedToEmitter)
   {
      mPosition = XMVector3Transform(mPosition, (XMMATRIX)emitterMatrix);      
      mVelocity = XMVector3TransformNormal(mVelocity, (XMMATRIX)emitterMatrix);      
   }

   //-- Move it out its initial distance.
   if (pData->mProperties.mInitialDistance != 0.0f)
   {
      BVector temp = mVelocity;
      temp.safeNormalize();
      temp *= RTVARIANCE(mVarience, pData->mProperties.mInitialDistance, pData->mProperties.mInitialDistanceVar);
      mPosition += temp;
   }
      
   //-- Ok, this little tidbit isn't terribly well thought out...but:
   //-- We need to orient up-facing particles by motion AFTER they have
   //-- inherited the parent transform.
   else if (pData->mProperties.mType == BEmitterBaseData::eUpfacing || pData->mProperties.mType == BEmitterBaseData::eVelocityAligned)
   {
      //-- Up facing particles deafult to "oriented by motion".
      BVector test, forward(0, 0, 1);
      test   = mVelocity;
      test.y = 0.0f;
      float mag = test.length();
      if (mag > cFloatCompareEpsilon)
      {
         test *= (1.0f / mag);

         float cosAngle = test.dot(forward);
         float tumbleAngle = XMScalarACos(cosAngle);
         if (test.x >= 0.0f)
            tumbleAngle = -tumbleAngle;

         mTumbleParams.x = XMConvertFloatToHalf(tumbleAngle);
      }
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemParticle::chooseEmissionPoint(const BParticleEmitter* RESTRICT pParent,  const BParticleEmitterDefinition* RESTRICT pData, XMVECTOR* pPos)
{   
   BDEBUG_ASSERT(pData!= NULL);
   BDEBUG_ASSERT(pParent!=NULL);
   BDEBUG_ASSERT(pPos!=NULL);

   if (pData->mShape.mType == BEmitterShapeData::ePoint)
   {
      *pPos = XMVectorSet(pData->mShape.mXPosOffset, pData->mShape.mYPosOffset, pData->mShape.mZPosOffset, 0.0f);
      return;
   }

   //-- trajectory calculation of a vector within the limitation of a cone
   float xRotation = pData->mShape.mTrajectoryPitch * cRadiansPerDegree;
   float yRotation = pData->mShape.mTrajectoryYaw * cRadiansPerDegree;
   float zRotation = pData->mShape.mTrajectoryBank * cRadiansPerDegree;
   BMatrix mtx;
   mtx.makeRotateX(xRotation);
   mtx.multRotateY(yRotation);
   mtx.multRotateZ(zRotation);

   XMVECTOR offset = XMVectorSet(pData->mShape.mXPosOffset, pData->mShape.mYPosOffset, pData->mShape.mZPosOffset, 0);

   //-- Use a shape description 
   switch (pData->mShape.mType)
   {
      case BEmitterShapeData::ePoint:
      {
         BASSERT(0);         
      }
      break;

      case BEmitterShapeData::eBox:
      {
         if (!pData->mShape.mEmitFromSurface)
         {
            float x = RANDOM_RANGE(-pData->mShape.mXSize, pData->mShape.mXSize);
            float y = RANDOM_RANGE(-pData->mShape.mYSize, pData->mShape.mYSize);
            float z = RANDOM_RANGE(-pData->mShape.mZSize, pData->mShape.mZSize);
            
            *pPos = XMVectorSet(x,y,z, 0);
            *pPos = XMVector3Transform(*pPos, (XMMATRIX)mtx);
            *pPos = XMVectorAdd(*pPos, offset);
            return;
         }
         else // emit from the surface
         {
            //-- Pick a face.
            float x            = 0.0f;
            float y            = 0.0f;
            float z            = 0.0f;
            float val          = RANDOM_RANGE(0, pData->mShape.mTotalFactor*2);
            bool  oppositeFace = false;
            if (val > pData->mShape.mTotalFactor)
            {
               oppositeFace = true;
               val -= pData->mShape.mTotalFactor;
            }

            if (val > (pData->mShape.mTopFactor + pData->mShape.mSideFactor))
            {
               //-- Its a front face.
               x = RANDOM_RANGE(-pData->mShape.mXSize,  pData->mShape.mXSize);
               y = RANDOM_RANGE(-pData->mShape.mYSize,  pData->mShape.mYSize);
               z = oppositeFace ? -pData->mShape.mZSize : pData->mShape.mZSize;
            }
            else if (val > pData->mShape.mTopFactor)
            {
               //-- Its a side face.
               x = oppositeFace ? -pData->mShape.mXSize : pData->mShape.mXSize;
               y = RANDOM_RANGE(-pData->mShape.mYSize,  pData->mShape.mYSize);
               z = RANDOM_RANGE(-pData->mShape.mZSize,  pData->mShape.mZSize);
            }
            else
            {
               //-- Its a top face.
               x = RANDOM_RANGE(-pData->mShape.mXSize,  pData->mShape.mXSize);
               y = oppositeFace ? -pData->mShape.mYSize : pData->mShape.mYSize;
               z = RANDOM_RANGE(-pData->mShape.mZSize,  pData->mShape.mZSize);
            }

            
            *pPos = XMVectorSet(x,y,z, 0);
            *pPos = XMVector3Transform(*pPos, (XMMATRIX)mtx);
            *pPos = XMVectorAdd(*pPos, offset);
            return;
         }
      }
      break;
      
      case BEmitterShapeData::eSphere: // PSES_ELLIPSOID_SOLID:
      {
         if (!pData->mShape.mEmitFromSurface)
         {
#if 1
            float y = RANDOM_RANGE(-1.0f, 1.0f);
            float yy = 1.0f - y * y;
            float t = RANDOM_RANGE(-XM_PI, XM_PI-cFloatCompareEpsilon);
            float alpha = RANDOM_RANGE(0.0f, 1.0f);
            *pPos = XMVectorZero();
            XMVECTOR vectorT = XMVectorSplatX(XMLoadScalar(&t));
            XMVECTOR vectorR = XMVectorSqrt(XMVectorSplatX(XMLoadScalar(&yy)));

            XMVECTOR sinV;
            XMVECTOR cosV;
            XMVectorSinCos(&sinV, &cosV, vectorT);

            XMVECTOR alphaV  = XMVectorMultiply(XMVectorSqrt(XMVectorSplatX(XMLoadScalar(&alpha))), XMVectorSplatX(XMLoadScalar(&pData->mShape.mXSize)));            
            XMVECTOR vectorY = XMLoadScalar(&y);
                        
            sinV = XMVectorMultiply(sinV, vectorR);
            cosV = XMVectorMultiply(cosV, vectorR);

            *pPos = __vrlimi(*pPos, cosV, VRLIMI_CONST(1,0,0,0), 0);
            *pPos = __vrlimi(*pPos, vectorY, VRLIMI_CONST(0,1,0,0), 3);
            *pPos = __vrlimi(*pPos, sinV, VRLIMI_CONST(0,0,1,0), 0);

            *pPos = XMVectorMultiply(*pPos, alphaV);
            *pPos = XMVectorAdd(*pPos, offset);
#else
            //-- Pretend its an ellipsoid surface.
            float y = RANDOM_RANGE(-1.0f, 1.0f);
            float t = RANDOM_RANGE(-XM_PI, XM_PI-cFloatCompareEpsilon);
            float r = (float)sqrt(1 - y*y);
            //float x = r*(float)cos(t);
            //float z = r*(float)sin(t);
            float sinT, cosT;
            XMScalarSinCosEst(&sinT, &cosT, t);
            float x = r*cosT;
            float z = r*sinT;
            
            //-- Modulate the surface radius.  The sqrt() corrects the center density problem.
            float alpha = RANDOM_RANGE(0.0f, 1.0f);
            alpha = (float)sqrt(alpha);

            x *= (alpha * pData->mShape.mXSize);
            y *= (alpha * pData->mShape.mXSize);
            z *= (alpha * pData->mShape.mXSize);


            *pPos = XMVectorSet(x,y,z, 0);
            *pPos = XMVectorAdd(*pPos, offset);
#endif
            return;
         }
         else
         {
            //case PSES_ELLIPSOID_SURFACE:
            float y = RANDOM_RANGE(-1.0f, 1.0f);
            float t = RANDOM_RANGE(-XM_PI, XM_PI-cFloatCompareEpsilon);
            float r = (float)sqrt(1 - y*y);
            float sinT, cosT;
            XMScalarSinCosEst(&sinT, &cosT, t);
            float x = r*cosT;
            float z = r*sinT;

            x *= pData->mShape.mXSize;
            y *= pData->mShape.mXSize;
            z *= pData->mShape.mXSize;


            *pPos = XMVectorSet(x,y,z, 0);
            *pPos = XMVectorAdd(*pPos, offset);
            
            return;
         }
      }
      break;
      
 
      //case PSES_HALF_ELLIPSOID_SOLID:
      case BEmitterShapeData::eHalfSphere:
      {
         if (!pData->mShape.mEmitFromSurface)
         {
            //-- Pretend its a half ellipsoid surface.
            float y = RANDOM_RANGE(0.0f, 1.0f);
            float t = RANDOM_RANGE(-XM_PI, XM_PI-cFloatCompareEpsilon);
            float r = (float)sqrt(1 - y*y);
            float sinT, cosT;
            XMScalarSinCosEst(&sinT, &cosT, t);
            float x = r*cosT;
            float z = r*sinT;

            //-- Modulate the surface radius.  The sqrt() corrects the center density problem.
            float alpha = RANDOM_RANGE(0.0f, 1.0f);
            alpha = (float)sqrt(alpha);
            x *= (alpha * pData->mShape.mXSize);
            y *= (alpha * pData->mShape.mXSize);
            z *= (alpha * pData->mShape.mXSize);

            *pPos = XMVectorSet(x,y,z, 0);
            *pPos = XMVector3Transform(*pPos, (XMMATRIX)mtx);
            *pPos = XMVectorAdd(*pPos, offset);
            
            return;
         }
         else //case PSES_HALF_ELLIPSOID_SURFACE:
         {
            
            float y = RANDOM_RANGE(0.0f, 1.0f);
            float t = RANDOM_RANGE(-XM_PI, XM_PI-cFloatCompareEpsilon);
            float r = (float)sqrt(1 - y*y);

            float sinT, cosT;
            XMScalarSinCosEst(&sinT, &cosT, t);
            float x = r*cosT;
            float z = r*sinT;

            x *= pData->mShape.mXSize;
            y *= pData->mShape.mXSize;
            z *= pData->mShape.mXSize;

            *pPos = XMVectorSet(x,y,z, 0);
            *pPos = XMVector3Transform(*pPos, (XMMATRIX)mtx);
            *pPos = XMVectorAdd(*pPos, offset);
            
            return;
         }
      }
      break;
      
      //case PSES_RECTANGLE:
      case BEmitterShapeData::eRectangle:
      {
         //-- Pick an edge.
         float x            = 0.0f;
         float z            = 0.0f;
         float val          = RANDOM_RANGE(0, pData->mShape.mTotalFactor*2);
         bool  oppositeFace = false;
         if (val > pData->mShape.mTotalFactor)
         {
            oppositeFace = true;
            val -= pData->mShape.mTotalFactor;
         }

         if (val > (pData->mShape.mTopFactor))
         {
            //-- Its a side edge.
            x = oppositeFace ? -pData->mShape.mXSize : pData->mShape.mXSize;
            z = RANDOM_RANGE(-pData->mShape.mYSize,  pData->mShape.mYSize);
         }
         else
         {
            //-- Its a top edge.
            x = RANDOM_RANGE(-pData->mShape.mXSize,  pData->mShape.mXSize);
            z = oppositeFace ? -pData->mShape.mYSize : pData->mShape.mYSize;
         }

         *pPos = XMVectorSet(x,0.0f,z, 0);
         
         *pPos = XMVector3Transform(*pPos, (XMMATRIX)mtx);
         *pPos = XMVectorAdd(*pPos, offset);
         
         return;
      }
      break;

      //case PSES_CIRCLE:
      case BEmitterShapeData::eCircle:
      {
         float x = 0.0f;
         float z = 0.0f;
         float angle  = RANDOM_RANGE(-XM_PI, XM_PI-cFloatCompareEpsilon);
         float sinT, cosT;
         XMScalarSinCosEst(&sinT, &cosT, angle);

         if (pData->mShape.mEmitFromSurface)
         {
            //-- Pretend its an ellipsoid surface.
            float radius = RANDOM_RANGE(-1.0f, 1.0f);            
            radius = (float)sqrt(1 - radius*radius);
            
            x = radius*cosT;
            z = radius*sinT;

            x *= pData->mShape.mXSize;
            z *= pData->mShape.mXSize;
         }
         else
         {           
            x = pData->mShape.mXSize*cosT;
            z = pData->mShape.mXSize*sinT;
         }

         *pPos = XMVectorSet(x,0.0f,z,0.0f);
         *pPos = XMVector3Transform(*pPos, (XMMATRIX)mtx);
         *pPos = XMVectorAdd(*pPos, offset);
         return;        
      }
      break;   
      case BEmitterShapeData::eCylinder:
      {           
         //-- Pretend its an ellipsoid surface.
         float radius    = RANDOM_RANGE(-1.0f, 1.0f);
         float angle = RANDOM_RANGE(-XM_PI, XM_PI-cFloatCompareEpsilon);
         radius = (float)sqrt(1 - radius*radius);
         float sinT, cosT;
         XMScalarSinCosEst(&sinT, &cosT, angle);
         float x = radius*cosT;
         float z = radius*sinT;

         //-- Modulate the surface radius.  The sqrt() corrects the center density problem.
         float alpha = 1.0f;
         if (!pData->mShape.mEmitFromSurface)
         {
            alpha = RANDOM_RANGE(0.0f, 1.0f);
            alpha = (float)sqrt(alpha);
         }

         float heightAlpha = RANDOM_RANGE(0.0f, 1.0f);
         x *= (alpha       * pData->mShape.mXSize);
         float y = (heightAlpha * pData->mShape.mYSize);
         z *= (alpha       * pData->mShape.mXSize);


         *pPos = XMVectorSet(x,y,z, 0);
         
         *pPos = XMVector3Transform(*pPos, (XMMATRIX)mtx);
         *pPos = XMVectorAdd(*pPos, offset);
         return;
      }
      break;
   }

   *pPos = XMVectorZero();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemParticle::initEffect(BParticleEmitter* RESTRICT pParent, const BParticleEmitterDefinition* RESTRICT pData, BMatrix emitterMatrix)
{
   int dataIndex = -1;
   gPSManager.getData(BStrConv::toB(pData->mProperties.mPFXFilepath), &dataIndex);

   if (dataIndex != -1)
   {
      BParticleCreateParams params;
      params.mDataHandle = dataIndex;
      params.mMatrix = emitterMatrix;
      params.mNearLayerEffect = false;      
      mpEffect = gPSManager.createEffect(params);
   }      
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemParticle::releaseEffect(bool bKillImmediately)
{
   if (mpEffect)
      gPSManager.releaseEffect(mpEffect, bKillImmediately);

   mpEffect = NULL;
}
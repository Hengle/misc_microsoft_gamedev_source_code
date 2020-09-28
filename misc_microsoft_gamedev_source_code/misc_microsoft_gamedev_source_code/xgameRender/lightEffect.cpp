//==============================================================================
// File: lightEffect.cpp
//
// Copyright (c) 2006, Ensemble Studios
//
// FIXME: Sergio - COPY+PASTE?? WTF?
// 
//==============================================================================
#include "xgameRender.h"
#include "math\quat.h"
#include "lightEffect.h"

const float cMaxToPhoenixScale = 1.0f/64.0f;

//==============================================================================
// BLightEffectData::BLightEffectData
//==============================================================================
BLightEffectData::BLightEffectData() :
   mValid(false)
{
}

//==============================================================================
// BLightEffectData::~BLightEffectData
//==============================================================================
BLightEffectData::~BLightEffectData()
{
}

//==============================================================================
// BLightEffectData::load
//==============================================================================
bool BLightEffectData::load(const char* pName, BStream& stream)
{
   clear();

   if (!mSceneData.read(stream))
      return false;

   mValid = mSceneData.numFrames() >= 1;
   
   mName.set(pName);
   
   return mValid;
}

//==============================================================================
// BLightEffectData::clear
//==============================================================================
void BLightEffectData::clear(void)
{
   mValid = false;
   mSceneData.clear();
   mName.empty();
}

//==============================================================================
// BLightEffectInstance::BLightEffectInstance
//==============================================================================
BLightEffectInstance::BLightEffectInstance() :
   mpScene(NULL),
   mCurTime(0.0f),
   mInitialized(false),
   mbVisible(true)
{
}

//==============================================================================
// BLightEffectInstance::~BLightEffectInstance
//==============================================================================
BLightEffectInstance::~BLightEffectInstance()
{
   clear();
}

//==============================================================================
// BLightEffectInstance::load
//==============================================================================
bool BLightEffectInstance::init(const BLightEffectData& data)
{
   clear();
         
   if (data.getValid())
   {
      mpScene = &data.sceneData();
   
      mInitialized = true;
      mbVisible = true;
   }
         
   return mInitialized;
}

//==============================================================================
// BLightEffectInstance::clear
//==============================================================================
void BLightEffectInstance::clear(void)
{
   mpScene = NULL;
   mInitialized = false;
   mCurTime = 0.0f;
   
   for (uint i = 0; i < mLocalLightHandles.getSize(); i++)
   {
      if (cInvalidLocalLightHandle != mLocalLightHandles[i])
         gSimSceneLightManager.freeLocalLight(mLocalLightHandles[i]);
   }
   mLocalLightHandles.clear();
}

//==============================================================================
// BLightEffectInstance:advanceTime
//==============================================================================
void BLightEffectInstance::advanceTime(float deltaT)
{
   const float duration = mpScene->duration();
   
   if (duration > 0.0f)
   {
      mCurTime += deltaT;
      mCurTime = Math::fPosMod(mCurTime, duration);
   }
   else
      mCurTime = 0.0f;
}

//==============================================================================
// BLightEffectInstance::findSceneInterpFrames
//==============================================================================
void BLightEffectInstance::findSceneInterpFrames(BInterpFrames& interpFrames)
{
   if (mCurTime >= mpScene->duration())
   {
      interpFrames.frame[0] = interpFrames.frame[1] = mpScene->numFrames() - 1;
      interpFrames.t = 0.0f;
      return;
   }

   int i;
   for (i = 0; i < mpScene->numFrames(); i++)
      if (mCurTime < (mpScene->frameTime(i) - mpScene->frameTime(0)))
         break;

   interpFrames.frame[0] = Math::Max(0, i - 1);
   interpFrames.frame[1] = i;

   debugRangeCheck(interpFrames.frame[0], mpScene->numFrames());
   debugRangeCheck(interpFrames.frame[1], mpScene->numFrames());

   const float frame0Time = mpScene->frameTime(interpFrames.frame[0]) - mpScene->frameTime(0);
   const float frame1Time = mpScene->frameTime(interpFrames.frame[1]) - mpScene->frameTime(0);
   const float duration = frame1Time - frame0Time;
   interpFrames.t = duration ? ((mCurTime - frame0Time) / duration) : 0.0f;
   debugRangeCheckIncl(interpFrames.t, 0.0f, 1.0f);
}

//==============================================================================
// BLightEffectInstance::interpModelToWorld
//==============================================================================
XMMATRIX BLightEffectInstance::interpModelToWorld(
   const LightData::BBase& obj0, 
   const LightData::BBase& obj1, 
   float t,
   const XMMATRIX& postXForm)
{
   XMMATRIX modelToMaxWorld;
   BMatrix44* p = (BMatrix44*)&modelToMaxWorld;
   
   BQuat s(BQuat::slerp(obj0.mOrient, obj1.mOrient, t));
      
   s.toMatrix(*p);

   p->setTranslate(BVec4(BVec3::lerp(obj0.mPos, obj1.mPos, t), 1.0f));

   return modelToMaxWorld * postXForm;
}

//==============================================================================
// BLightEffectInstance::makeMaxToPhoenix
//==============================================================================
void BLightEffectInstance::makeMaxToPhoenix(XMMATRIX& ret)
{
   ret = XMMatrixIdentity();
   
   ret.m[0][0] = -cMaxToPhoenixScale;
   ret.m[2][1] = cMaxToPhoenixScale;
   ret.m[1][1] = 0.0f;
   ret.m[1][2] = -cMaxToPhoenixScale;
   ret.m[2][2] = 0.0f;
}

//==============================================================================
// BLightEffectInstance::tick
//==============================================================================
void BLightEffectInstance::tick(float deltaT, const XMMATRIX& modelToWorld, float intensityScale)
{
   SCOPEDSAMPLE(BLightEffectInstance_tick);  

   if (!mInitialized)   
      return;

   // if we are not visible clear out lights
   if (!mbVisible)
   {
      for (uint i = 0; i < mLocalLightHandles.getSize(); i++)
      {
         if (cInvalidLocalLightHandle != mLocalLightHandles[i])
            gSimSceneLightManager.freeLocalLight(mLocalLightHandles[i]);
      }
      mLocalLightHandles.resize(0);
      return;
   }
         
   advanceTime(deltaT);
   
   BInterpFrames interpFrames;
   findSceneInterpFrames(interpFrames);

   const LightData::BFrame& frame0 = mpScene->frame(interpFrames.frame[0]);
   const LightData::BFrame& frame1 = mpScene->frame(interpFrames.frame[1]);
   const float t = interpFrames.t;   

   XMMATRIX maxToWorld;
   makeMaxToPhoenix(maxToWorld);
   maxToWorld = maxToWorld * modelToWorld;

   for (uint i = 0; i < frame0.numLights(); i++)
   {
      if (mLocalLightHandles.getSize() <= i)
         mLocalLightHandles.pushBack(gSimSceneLightManager.createLocalLight());

      BLocalLightHandle lightHandle = mLocalLightHandles[i];
      if (cInvalidLocalLightHandle == lightHandle)
         continue;

      BLocalLightParams& lightParams = gSimSceneLightManager.getLocalLightParams(lightHandle);

      const LightData::BLight& obj0 = frame0.light(i);
      const LightData::BLight& obj1 = frame1.light(i);

      switch (obj0.mType)
      {
         case LightData::eSpotLightType: 
            lightParams.setType(cLTSpot);
            break;
         case LightData::eOmniLightType: 
            lightParams.setType(cLTOmni);
            break;
         case LightData::eDirLightType: 
         {
            gSimSceneLightManager.setLocalLightEnabled(lightHandle, false);
            continue;
         }
      }

      const float intensity = Math::Lerp(obj0.mIntensity, obj1.mIntensity, t) * intensityScale;

      const BVec3 color(BVec3::lerp(obj0.mColor, obj1.mColor, t));
      if (obj0.mDiffuse)
         lightParams.setColor(XMVectorSet(color[0] * intensity, color[1] * intensity, color[2] * intensity, 0.0f));

      XMMATRIX m(interpModelToWorld(obj0, obj1, t, maxToWorld));

      gSimSceneLightManager.setLocalLightPos(lightHandle, (XMFLOAT3*)&m._41);

      BVec3 dir(-m.m[2][0], -m.m[2][1], -m.m[2][2]);
      dir.tryNormalize();

      BVec3 up(-m.m[1][0], -m.m[1][1], -m.m[1][2]);
      up.tryNormalize();

      gSimSceneLightManager.setLocalLightAt(lightHandle, (XMFLOAT3*)&dir);
      gSimSceneLightManager.setLocalLightRight(lightHandle, (XMFLOAT3*)&up);

      const float radius = Math::Lerp(obj0.mRadius, obj1.mRadius, t) * cMaxToPhoenixScale;
      const bool lightEnabled = (radius > .0125f) && (intensity > .0125f);
      gSimSceneLightManager.setLocalLightRadius(lightHandle, radius);

      lightParams.setFarAttenStart(Math::Lerp(obj0.mFarAttenStart, obj1.mFarAttenStart, t));
      lightParams.setDecayDist(Math::Lerp(obj0.mDecayDist, obj1.mDecayDist, t) * cMaxToPhoenixScale);
      lightParams.setPriority(-1);
      lightParams.setSpecular(obj0.mSpecular);
      lightParams.setLightBuffered(obj0.mLightBuffered);
     
      gSimSceneLightManager.setLocalLightShadows(lightHandle, obj0.mShadows);
      gSimSceneLightManager.setLocalLightSpotInner(lightHandle, Math::Lerp(obj0.mSpotInner, obj1.mSpotInner, t));
      gSimSceneLightManager.setLocalLightSpotOuter(lightHandle, Math::Lerp(obj0.mSpotOuter, obj1.mSpotOuter, t));

      gSimSceneLightManager.enforceLimits(lightHandle);

      gSimSceneLightManager.setLocalLightEnabled(lightHandle, lightEnabled);
   }

   if (frame0.numLights() < mLocalLightHandles.getSize())
   {
      for (uint i = frame0.numLights(); i < mLocalLightHandles.getSize(); i++)
      {
         if (cInvalidLocalLightHandle != mLocalLightHandles[i])
            gSimSceneLightManager.freeLocalLight(mLocalLightHandles[i]);
      }
      mLocalLightHandles.resize(frame0.numLights());         
   }
}

//==============================================================================
// BLightEffectInstance::getDuration
//==============================================================================
float BLightEffectInstance::getDuration(void) const
{
   if (!mInitialized)
      return 0.0f;
   return mpScene->duration();
}

//==============================================================================
// BLightEffectInstance::getTime
//==============================================================================
float BLightEffectInstance::getTime(void) const
{
   if (!mInitialized)
      return 0.0f;
   return mCurTime;
}

//==============================================================================
// BLightEffectInstance::setTime
//==============================================================================
void BLightEffectInstance::setTime(float t)
{
   if (!mInitialized)
      return;
   
   const float d = mpScene->duration();
   if (t > 0.0f)
      mCurTime = Math::fPosMod(t, d);
   else
      mCurTime = 0.0f;
}

//==============================================================================
// BCameraEffectInstance::BCameraEffectInstance
//==============================================================================
BCameraEffectInstance::BCameraEffectInstance() :
   mpScene(NULL),
   mCurTime(0.0f),
   mInitialized(false),
   mbVisible(true)
{
}

//==============================================================================
// BCameraEffectInstance::~BCameraEffectInstance
//==============================================================================
BCameraEffectInstance::~BCameraEffectInstance()
{
   clear();
}

//==============================================================================
// BCameraEffectInstance::load
//==============================================================================
bool BCameraEffectInstance::init(const BLightEffectData& data)
{
   clear();

   if (data.getValid())
   {
      mpScene = &data.sceneData();

      mInitialized = true;
      mbVisible = true;
   }

   return mInitialized;
}

//==============================================================================
// BCameraEffectInstance::clear
//==============================================================================
void BCameraEffectInstance::clear(void)
{
   mpScene = NULL;
   mInitialized = false;
   mCurTime = 0.0f;

   clearLights();
}

//==============================================================================
// BCameraEffectInstance::clearLights
//==============================================================================
void BCameraEffectInstance::clearLights(void)
{
   for (uint i = 0; i < mLocalLightHandles.getSize(); i++)
   {
      if (cInvalidLocalLightHandle != mLocalLightHandles[i])
         gSimSceneLightManager.freeLocalLight(mLocalLightHandles[i]);
   }
   mLocalLightHandles.clear();
}

//==============================================================================
// BCameraEffectInstance:advanceTime
//==============================================================================
void BCameraEffectInstance::advanceTime(float deltaT)
{
   const float duration = mpScene->duration();

   if (duration > 0.0f)
   {
      mCurTime += deltaT;
      mCurTime = Math::fPosMod(mCurTime, duration);
   }
   else
      mCurTime = 0.0f;
}

//==============================================================================
// BCameraEffectInstance::findSceneInterpFrames
//==============================================================================
void BCameraEffectInstance::findSceneInterpFrames(BInterpFrames& interpFrames)
{
   if (mCurTime >= mpScene->duration())
   {
      interpFrames.frame[0] = interpFrames.frame[1] = mpScene->numFrames() - 1;
      interpFrames.t = 0.0f;
      return;
   }

   int i;
   for (i = 0; i < mpScene->numFrames(); i++)
      if (mCurTime < (mpScene->frameTime(i) - mpScene->frameTime(0)))
         break;

   interpFrames.frame[0] = Math::Max(0, i - 1);
   interpFrames.frame[1] = i;

   debugRangeCheck(interpFrames.frame[0], mpScene->numFrames());
   debugRangeCheck(interpFrames.frame[1], mpScene->numFrames());

   const float frame0Time = mpScene->frameTime(interpFrames.frame[0]) - mpScene->frameTime(0);
   const float frame1Time = mpScene->frameTime(interpFrames.frame[1]) - mpScene->frameTime(0);
   const float duration = frame1Time - frame0Time;
   interpFrames.t = duration ? ((mCurTime - frame0Time) / duration) : 0.0f;
   debugRangeCheckIncl(interpFrames.t, 0.0f, 1.0f);
}

//==============================================================================
// BCameraEffectInstance::interpModelToWorld
//==============================================================================
XMMATRIX BCameraEffectInstance::interpModelToWorld(
   const LightData::BBase& obj0, 
   const LightData::BBase& obj1, 
   float t,
   const XMMATRIX& postXForm)
{
   XMMATRIX modelToMaxWorld;
   BMatrix44* p = (BMatrix44*)&modelToMaxWorld;

   BQuat s(BQuat::slerp(obj0.mOrient, obj1.mOrient, t));

   s.toMatrix(*p);

   p->setTranslate(BVec4(BVec3::lerp(obj0.mPos, obj1.mPos, t), 1.0f));

   return modelToMaxWorld * postXForm;
}

//==============================================================================
// BCameraEffectInstance::makeMaxToPhoenix
//==============================================================================
void BCameraEffectInstance::makeMaxToPhoenix(XMMATRIX& ret)
{
   ret = XMMatrixIdentity();

   ret.m[0][0] = -cMaxToPhoenixScale;
   ret.m[2][1] = cMaxToPhoenixScale;
   ret.m[1][1] = 0.0f;
   ret.m[1][2] = -cMaxToPhoenixScale;
   ret.m[2][2] = 0.0f;
}


//==============================================================================
// BCameraEffectInstance::tick
//==============================================================================
void BCameraEffectInstance::tick(
   float deltaT, const XMMATRIX& modelToWorld, XMMATRIX& mat, 
   float& fov, float& dofDepth, float& dofNear, float& dofFar, float& nearClip, float& farClip)
{
   fov = 45.0f;
   dofDepth = 0.0f;
   dofNear = 0.0f;
   dofFar = 50000.0f;
   nearClip = 0.0f;
   farClip = 0.0f;
   
   if (!mInitialized)   
      return;

   // if we are not visible clear out lights
   if (!mbVisible)
   {
      for (uint i = 0; i < mLocalLightHandles.getSize(); i++)
      {
         if (cInvalidLocalLightHandle != mLocalLightHandles[i])
            gSimSceneLightManager.freeLocalLight(mLocalLightHandles[i]);
      }
      mLocalLightHandles.resize(0);
      return;
   }

   advanceTime(deltaT);

   BInterpFrames interpFrames;
   findSceneInterpFrames(interpFrames);

   const LightData::BFrame& frame0 = mpScene->frame(interpFrames.frame[0]);
   const LightData::BFrame& frame1 = mpScene->frame(interpFrames.frame[1]);
   const float t = interpFrames.t;   

   XMMATRIX maxToWorld;
   makeMaxToPhoenix(maxToWorld);
   maxToWorld = maxToWorld * modelToWorld;

   // Interpolate lights
   for (uint i = 0; i < frame0.numLights(); i++)
   {
      if (mLocalLightHandles.getSize() <= i)
         mLocalLightHandles.pushBack(gSimSceneLightManager.createLocalLight());

      BLocalLightHandle lightHandle = mLocalLightHandles[i];
      if (cInvalidLocalLightHandle == lightHandle)
         continue;

      BLocalLightParams& lightParams = gSimSceneLightManager.getLocalLightParams(lightHandle);

      const LightData::BLight& obj0 = frame0.light(i);
      const LightData::BLight& obj1 = frame1.light(i);

      switch (obj0.mType)
      {
         case LightData::eSpotLightType: 
            lightParams.setType(cLTSpot);
            break;
         case LightData::eOmniLightType: 
            lightParams.setType(cLTOmni);
            break;
         case LightData::eDirLightType: 
         {
            gSimSceneLightManager.setLocalLightEnabled(lightHandle, false);
            continue;
         }
      }

      const float intensity = Math::Lerp(obj0.mIntensity, obj1.mIntensity, t);

      const BVec3 color(BVec3::lerp(obj0.mColor, obj1.mColor, t));
      if (obj0.mDiffuse)
         lightParams.setColor(XMVectorSet(color[0] * intensity, color[1] * intensity, color[2] * intensity, 0.0f));

      XMMATRIX m(interpModelToWorld(obj0, obj1, t, maxToWorld));

      gSimSceneLightManager.setLocalLightPos(lightHandle, (XMFLOAT3*)&m._41);

      BVec3 dir(-m.m[2][0], -m.m[2][1], -m.m[2][2]);
      dir.tryNormalize();

      BVec3 up(-m.m[1][0], -m.m[1][1], -m.m[1][2]);
      up.tryNormalize();

      gSimSceneLightManager.setLocalLightAt(lightHandle, (XMFLOAT3*)&dir);
      gSimSceneLightManager.setLocalLightRight(lightHandle, (XMFLOAT3*)&up);

      const float radius = Math::Lerp(obj0.mRadius, obj1.mRadius, t) * cMaxToPhoenixScale;
      const bool lightEnabled = (radius > .0125f) && (intensity > .0125f);
      gSimSceneLightManager.setLocalLightRadius(lightHandle, radius);

      lightParams.setFarAttenStart(Math::Lerp(obj0.mFarAttenStart, obj1.mFarAttenStart, t));
      lightParams.setDecayDist(Math::Lerp(obj0.mDecayDist, obj1.mDecayDist, t) * cMaxToPhoenixScale);
      lightParams.setSpecular(obj0.mSpecular);
      lightParams.setLightBuffered(true);

      gSimSceneLightManager.setLocalLightShadows(lightHandle, obj0.mShadows);
      gSimSceneLightManager.setLocalLightSpotInner(lightHandle, Math::Lerp(obj0.mSpotInner, obj1.mSpotInner, t));
      gSimSceneLightManager.setLocalLightSpotOuter(lightHandle, Math::Lerp(obj0.mSpotOuter, obj1.mSpotOuter, t));

      gSimSceneLightManager.enforceLimits(lightHandle);
      
      gSimSceneLightManager.setLocalLightEnabled(lightHandle, lightEnabled);
   }

   if (frame0.numLights() < mLocalLightHandles.getSize())
   {
      for (uint i = frame0.numLights(); i < mLocalLightHandles.getSize(); i++)
      {
         if (cInvalidLocalLightHandle != mLocalLightHandles[i])
            gSimSceneLightManager.freeLocalLight(mLocalLightHandles[i]);
      }
      mLocalLightHandles.resize(frame0.numLights());         
   }

   // Interpolate camera
   if (frame0.numCameras() < 1)
      return;

   const LightData::BCamera& obj0 = frame0.camera(0);
   const LightData::BCamera& obj1 = frame1.camera(0);

   fov = Math::Lerp(obj0.mFov, obj1.mFov, t);
   dofDepth = Math::Lerp(obj0.mFocalDepth, obj1.mFocalDepth, t) * cMaxToPhoenixScale;
   dofNear = Math::Lerp(obj0.mNearRange, obj1.mNearRange, t) * cMaxToPhoenixScale;
   dofFar = Math::Lerp(obj0.mFarRange, obj1.mFarRange, t) * cMaxToPhoenixScale;
   nearClip = Math::Lerp(obj0.mNearClip, obj1.mNearClip, t) * cMaxToPhoenixScale;
   farClip = Math::Lerp(obj0.mFarClip, obj1.mFarClip, t) * cMaxToPhoenixScale;
   
   XMMATRIX m(interpModelToWorld(obj0, obj1, t, maxToWorld));

   mat = m;
}

//==============================================================================
// BCameraEffectInstance::getDuration
//==============================================================================
float BCameraEffectInstance::getDuration(void) const
{
   if (!mInitialized)
      return 0.0f;
   return mpScene->duration();
}

//==============================================================================
// BCameraEffectInstance::getTime
//==============================================================================
float BCameraEffectInstance::getTime(void) const
{
   if (!mInitialized)
      return 0.0f;
   return mCurTime;
}

//==============================================================================
// BCameraEffectInstance::setTime
//==============================================================================
void BCameraEffectInstance::setTime(float t)
{
   if (!mInitialized)
      return;

   const float d = mpScene->duration();
   if (t > 0.0f)
      mCurTime = Math::fPosMod(t, d);
   else
      mCurTime = 0.0f;
}

//==============================================================================
// BCameraEffectInstance::hasCamera
//==============================================================================
bool BCameraEffectInstance::hasCamera()
{
   if(!mpScene)
      return false;

   if(mpScene->numFrames() <= 0)
      return false;

   const LightData::BFrame& frame0 = mpScene->frame(0);

   if(frame0.numCameras() < 1)
      return false;

   return true;
}
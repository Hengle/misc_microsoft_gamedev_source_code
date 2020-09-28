//==============================================================================
// File: lightEffect.h
//
// Copyright (c) 2006, Ensemble Studios
//==============================================================================
#pragma once
#include "lightData.h"
#include "sceneLightManager.h"
#include "math\generalMatrix.h"

#define LIGHT_EFFECT_EXTENSION "lgt"

//==============================================================================
// class BLightEffectData
//==============================================================================
class BLightEffectData
{
public:
   BLightEffectData();
   ~BLightEffectData();
   
   bool load(const char* pName, BStream& stream);
   void clear(void);
   
   const BString& getName(void) const { return mName; }
   bool getValid(void) const { return mValid; }
   const LightData::BScene& sceneData(void) const { return mSceneData; }
   
private:
   BString mName;
   LightData::BScene mSceneData;
   bool mValid;
};

//==============================================================================
// class BLightEffectInstance
//==============================================================================
class BLightEffectInstance
{
public:
   BLightEffectInstance();
   ~BLightEffectInstance();
         
   bool init(const BLightEffectData& data);
         
   void clear(void);      
   void setVisibility(bool bVisible) {mbVisible = bVisible;};
   void tick(float deltaT, const XMMATRIX& modelToWorld, float intensityScale);
   
   float getDuration(void) const;
   float getTime(void) const;
   void setTime(float t);
            
private:
   const LightData::BScene* mpScene;
      
   typedef BDynamicArray<BLocalLightHandle> BLocalLightHandleArray;
   BLocalLightHandleArray mLocalLightHandles;
      
   float mCurTime;
   
   bool mInitialized : 1;
   bool mbVisible : 1;
   
   struct BInterpFrames
   {
      int frame[2];
      float t;

      BInterpFrames()
      {
         frame[0] = frame[1] = 0;
         t = 0.0f;
      }
   };
   
   void findSceneInterpFrames(BInterpFrames& interpFrames);
   
   XMMATRIX interpModelToWorld(const LightData::BBase& obj0, const LightData::BBase& obj1, float t, const XMMATRIX& postXForm);
   
   void makeMaxToPhoenix(XMMATRIX& ret);
   void advanceTime(float deltaT);
};


//==============================================================================
// class BCameraEffectInstance
//==============================================================================
class BCameraEffectInstance
{
public:
   BCameraEffectInstance();
   ~BCameraEffectInstance();

   bool init(const BLightEffectData& data);
   void clear(void);      
   void clearLights(void);      
   void setVisibility(bool bVisible) {mbVisible = bVisible;};
   void tick(float deltaT, const XMMATRIX& modelToWorld, XMMATRIX& matrix, float& fov, float& dofDepth, float& dofNear, float& dofFar, float& nearClip, float& farClip);

   float getDuration(void) const;
   float getTime(void) const;
   void setTime(float t);

   bool hasCamera();

private:
   const LightData::BScene* mpScene;

   typedef BDynamicArray<BLocalLightHandle> BLocalLightHandleArray;
   BLocalLightHandleArray mLocalLightHandles;

   float mCurTime;
   bool mInitialized : 1;
   bool mbVisible : 1;

   struct BInterpFrames
   {
      int frame[2];
      float t;

      BInterpFrames()
      {
         frame[0] = frame[1] = 0;
         t = 0.0f;
      }
   };

   void findSceneInterpFrames(BInterpFrames& interpFrames);

   XMMATRIX interpModelToWorld(const LightData::BBase& obj0, const LightData::BBase& obj1, float t, const XMMATRIX& postXForm);

   void makeMaxToPhoenix(XMMATRIX& ret);
   void advanceTime(float deltaT);
};


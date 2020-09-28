//-----------------------------------------------------------------------------
// File: lightData.h
//-----------------------------------------------------------------------------
#pragma once

#include "math\generalVector.h"
#include "string\fixedString.h"
#include "stream\stream.h"

namespace LightData
{
   __declspec(selectany) extern uint gDeserializeVersion;
   
   enum 
   { 
      cSceneDataVersion = 0xCCDD0110
   };

   enum ELightType
   {
      eInvalidLightType = 0,

      eSpotLightType,
      eOmniLightType,
      eDirLightType,

      eNumLightTypes
   };

   struct BBase
   {
      BFixedString64 mName;
      BVec3 mPos;
      BVec4 mOrient;

      BBase()
      {
         clear();
      }

      void clear(void)
      {
         mName.clear();
         mPos.setZero();
         mOrient.setZero();
      }

      friend BStream& operator>> (BStream& src, BBase& dst) 
      {
         return src >> dst.mName >> dst.mPos >> dst.mOrient;
      }

      friend BStream& operator<< (BStream& dst, const BBase& src) 
      {
         return dst << src.mName << src.mPos << src.mOrient;
      }
   };

   enum EDecayType
   {
      eInvalidDecayType = -1,

      eDecayNone = 0,
      eDecayInverse = 1,
      eDecayInverseSquare = 2,

      eNumDecays
   };

   struct BLight : BBase
   {
      BFixedString64 mMask;
      ELightType mType;

      float mRadius;
      float mFarAttenStart;

      float mDecayDist;

      EDecayType mDecayType;

      float mIntensity;
      BVec3 mColor;

      float mSpotInner;
      float mSpotOuter;

      bool mShadows;
      bool mDiffuse;
      bool mSpecular;

      bool mFog;
      bool mFogShadows;
      float mFogDensity;
      bool mLightBuffered;

      BLight()
      {
         clear();
      }

      void clear(void)
      {
         BBase::clear();

         mMask.clear();
         mType = eInvalidLightType;
         mRadius = 0;
         mFarAttenStart = 0;

         mDecayDist = 0;

         mDecayType = eInvalidDecayType;

         mIntensity = 0;
         mColor.setZero();

         mSpotInner = 0;
         mSpotOuter = 0;

         mShadows = false;
         mDiffuse = false;
         mSpecular = false;

         mFog = false;
         mFogShadows = false;
         mFogDensity = 1.0f;
         mLightBuffered = true;
      }

      friend BStream& operator>> (BStream& src, BLight& dst) 
      {
         uchar type;
         uchar decayType;
         
         src >> static_cast<BBase&>(dst)
            >> dst.mMask 
            >> type
            >> dst.mRadius 
            >> dst.mFarAttenStart
            >> dst.mDecayDist
            >> decayType
            >> dst.mIntensity
            >> dst.mColor
            >> dst.mSpotInner
            >> dst.mSpotOuter
            >> dst.mShadows
            >> dst.mDiffuse
            >> dst.mSpecular
            >> dst.mFog
            >> dst.mFogShadows 
            >> dst.mFogDensity;
            
         if (gDeserializeVersion >= 0xCCDD0109)
            src >> dst.mLightBuffered;

         dst.mType = static_cast<ELightType>(type);
         dst.mDecayType = static_cast<EDecayType>(decayType);
         
         return src;
      }

      friend BStream& operator<< (BStream& dst, const BLight& src) 
      {
         return dst << static_cast<const BBase&>(src)
            << src.mMask 
            << static_cast<uchar>(src.mType)
            << src.mRadius 
            << src.mFarAttenStart
            << src.mDecayDist
            << static_cast<uchar>(src.mDecayType)
            << src.mIntensity
            << src.mColor
            << src.mSpotInner
            << src.mSpotOuter
            << src.mShadows
            << src.mDiffuse
            << src.mSpecular 
            << src.mFog
            << src.mFogShadows 
            << src.mFogDensity;
      }
   };

   enum EObjectType
   {
      eInvalid = 0,

      eGeom,
      eEmitter,

      eNumObjectTypes
   };

   struct BObject : BBase
   {
      EObjectType mType;
      float mVisibility;
      float mSphereRadius;
      BDynamicArray<char> mUDP;

      BObject()
      {
         clear();
      }

      void clear(void)
      {
         BBase::clear();
         mVisibility = 1.0f;
         mSphereRadius = 0.0f;
         mUDP.clear();
      }

      friend BStream& operator>> (BStream& src, BObject& dst)
      {
         uchar type;
         
         src >> static_cast<BBase&>(dst)
            >> type
            >> dst.mVisibility 
            >> dst.mSphereRadius;

         dst.mType = (EObjectType)type;

         bool success = src.readVec(dst.mUDP);
         success;
         return src;
      }

      friend BStream& operator<< (BStream& dst, const BObject& src)
      {
         dst << static_cast<const BBase&>(src)
            << static_cast<uchar>(src.mType) 
            << src.mVisibility 
            << src.mSphereRadius;

         bool success = dst.writeVec(src.mUDP);
         success;
         return dst;
      }
   };

   struct BCamera : BBase
   {
      float mFov;
      float mFocalDepth;
      float mNearRange;
      float mFarRange;
      float mNearClip;
      float mFarClip;

      BCamera()
      {
         clear();
      }

      void clear(void)
      {
         BBase::clear();
         mFov = 45.0f/57.2957;
         mFocalDepth = 0.0f;
         mNearRange = 0.0f;
         mFarRange = 0.0f;
         mNearClip = 0.0f;
         mFarClip = 0.0f;
      }

      friend BStream& operator>> (BStream& src, BCamera& dst)
      {
			if ((gDeserializeVersion & 0xFF) >= 10)
			{
			   return src >> static_cast<BBase&>(dst)
				   >> dst.mFov
				   >> dst.mFocalDepth
				   >> dst.mNearRange
				   >> dst.mFarRange
				   >> dst.mNearClip
				   >> dst.mFarClip;
			}
         else if ((gDeserializeVersion & 0xFF) >= 8)
         {
            return src >> static_cast<BBase&>(dst)
               >> dst.mFov
               >> dst.mFocalDepth
               >> dst.mNearRange
               >> dst.mFarRange;
         }
         else
         {
            return src >> static_cast<BBase&>(dst)
               >> dst.mFov;
         }					   
      }

      friend BStream& operator<< (BStream& dst, const BCamera& src)
      {
         return dst << static_cast<const BBase&>(src)
            << src.mFov
            << src.mFocalDepth
            << src.mNearRange
            << src.mFarRange
				<< src.mNearClip
				<< src.mFarClip;
      }
   };

   class BFrame
   {
   public:
      BFrame(float time = 0.0f)
      {
         clear();
         mTime = time;
      }

      void clear(void)
      {
         mTime = 0.0f;
         mLights.clear();
      }

      float time(void) const 
      {
         return mTime;
      }

      void setTime(float time) 
      {
         mTime = time;
      }

      uint numLights(void) const 
      {
         return mLights.size();
      }

      const BLight& light(int i) const 
      { 
         return mLights[debugRangeCheck(i, numLights())];
      }

      void insertLight(const BLight& light) 
      {
         mLights.push_back(light);
      }

      int numObjects(void) const
      {
         return mObjects.size();
      }

      const BObject& object(int i) const
      {
         return mObjects[debugRangeCheck(i, numObjects())];
      }

      void insertObject(const BObject& object)
      {
         mObjects.push_back(object);
      }

      int numCameras(void) const
      {
         return mCameras.size();
      }

      const BCamera& camera(int i) const
      {
         return mCameras[debugRangeCheck(i, numCameras())];
      }

      void insertCamera(const BCamera& camera)
      {
         mCameras.push_back(camera);
      }

      friend BStream& operator>> (BStream& src, BFrame& dst)
      {
         src.readObj(dst.mTime);
         src.readVec(dst.mLights);
         src.readVec(dst.mObjects);
         src.readVec(dst.mCameras);
         return src;
      }

      friend BStream& operator<< (BStream& dst, const BFrame& src)
      {
         dst.writeObj(src.mTime);
         dst.writeVec(src.mLights);
         dst.writeVec(src.mObjects);
         dst.writeVec(src.mCameras);
         return dst;
      }

   private:
      float mTime;
      BDynamicArray<BLight> mLights;
      BDynamicArray<BObject> mObjects;
      BDynamicArray<BCamera> mCameras;
   };

   class BScene
   {
   public:
      BScene()
      {
      }

      void clear(void)
      {
         mFrames.clear();
      }

      int numFrames(void) const
      {
         return mFrames.size();
      }

      const BFrame& frame(int i) const
      {
         return mFrames[debugRangeCheck(i, numFrames())];
      }

      float frameTime(int i) const
      {
         return frame(i).time();
      }

      float duration(void) const
      {
         if (!numFrames())
            return 0.0f;

         float d = Math::Max(0.0f, frameTime(numFrames() - 1) - frameTime(0));
         return d;
      }

      void insertFrame(const BFrame& frame)
      {
         mFrames.push_back(frame);
      }

      // false on failure
      bool read(BStream& src)
      {
         clear();

         uint ver;
         src >> ver;
         gDeserializeVersion = ver;
         if ((ver >> 8) != (((uint)cSceneDataVersion) >> 8))
            return true;

         src.readVec(mFrames);

         src >> ver;
         if (gDeserializeVersion != ver)
            return true;

         return !src.errorStatus();
      }

      friend BStream& operator>> (BStream& src, BScene& dst)
      {
         dst.read(src);
         return src;
      }

      friend BStream& operator<< (BStream& dst, const BScene& src)
      {
         dst << uint(cSceneDataVersion);
         dst.writeVec(src.mFrames);
         dst << uint(cSceneDataVersion);
         return dst;
      }

   private:
      BDynamicArray<BFrame> mFrames;
   };

} // namespace LightData

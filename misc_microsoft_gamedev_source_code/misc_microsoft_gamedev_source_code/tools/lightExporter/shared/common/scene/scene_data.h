//-----------------------------------------------------------------------------
// File: scene_data.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef SCENEDATA_H
#define SCENEDATA_H

#include "common/math/vector.h"
#include "common/utils/string.h"
#include "common/utils/stream.h"

namespace gr
{
	namespace SceneData
	{
	   __declspec(selectany) extern uint gDeserializeVersion;
	   
		enum 
		{ 
			SceneDataVersion = 0xCCDD0110
		};

		enum ELightType
		{
			eInvalidLightType = 0,
			
			eSpotLightType,
			eOmniLightType,
			eDirLightType,
			
			eNumLightTypes
		};

		struct Base
		{
			SmallString mName;
			Vec<3> mPos;
			Vec<4> mOrient;
			
			Base()
			{
				clear();
			}
			
			void clear(void)
			{
				mName.clear();
				mPos.setZero();
				mOrient.setZero();
			}
			
			friend Stream& operator>> (Stream& src, Base& dst) 
			{
				return src >> dst.mName >> dst.mPos >> dst.mOrient;
			}

			friend Stream& operator<< (Stream& dst, const Base& src) 
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
		
		struct Light : Base
		{
			SmallString mMask;
			ELightType mType;
			
			float mRadius;
			float mFarAttenStart;
			
			float mDecayDist;
						
			EDecayType mDecayType;

			float mIntensity;
			Vec<3> mColor;

			float mSpotInner;
			float mSpotOuter;

			bool mShadows;
			bool mDiffuse;
			bool mSpecular;
			
			bool mFog;
			bool mFogShadows;
			bool mLightBuffered;
			float mFogDensity;

			Light()
			{
				clear();
			}

      void clear(void)
			{
				Base::clear();
				
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

			friend Stream& operator>> (Stream& src, Light& dst) 
			{
				src >> static_cast<Base&>(dst)
					>> dst.mMask 
					>> *reinterpret_cast<uchar*>(&dst.mType) 
					>> dst.mRadius 
					>> dst.mFarAttenStart
					>> dst.mDecayDist
					>> *reinterpret_cast<uchar*>(&dst.mDecayType)
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

            return src;					
			}

			friend Stream& operator<< (Stream& dst, const Light& src) 
			{
				return dst << static_cast<const Base&>(src)
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
					<< src.mFogDensity
					<< src.mLightBuffered;
			}
		};
		
		enum EObjectType
		{
			eInvalid = 0,
			
			eGeom,
			eEmitter,
			
			eNumObjectTypes
		};

		struct Object : Base
		{
			EObjectType mType;
			float mVisibility;
			float mSphereRadius;
			std::vector<char> mUDP;
			
			Object()
			{
				clear();
			}

			void clear(void)
			{
				Base::clear();
				mVisibility = 1.0f;
				mSphereRadius = 0.0f;
				mUDP.clear();
			}

			friend Stream& operator>> (Stream& src, Object& dst)
			{
				src >> static_cast<Base&>(dst)
					>> *reinterpret_cast<uchar*>(&dst.mType)
					>> dst.mVisibility 
					>> dst.mSphereRadius;
					
				return src.readVec(dst.mUDP);
			}

			friend Stream& operator<< (Stream& dst, const Object& src)
			{
				dst << static_cast<const Base&>(src)
					<< static_cast<uchar>(src.mType) 
					<< src.mVisibility 
					<< src.mSphereRadius;
					
				return dst.writeVec(src.mUDP);
			}
    };
    
    struct Camera : Base
    {
			float mFov;
			float mFocalDepth;
			float mNearRange;
			float mFarRange;
			float mNearClip;
			float mFarClip;

			Camera()
			{
				clear();
			}
			
			void clear(void)
			{
				Base::clear();
				mFov = 45.0f/57.2957f;
				mFocalDepth = 0.0f;
				mNearRange = 0.0f;
				mFarRange = 0.0f;
				mNearClip = 0.0f;
				mFarClip = 0.0f;
			}

			friend Stream& operator>> (Stream& src, Camera& dst)
			{
				if ((gDeserializeVersion & 0xFF) >= 10)
				{
				   return src >> static_cast<Base&>(dst)
					   >> dst.mFov
					   >> dst.mFocalDepth
					   >> dst.mNearRange
					   >> dst.mFarRange
					   >> dst.mNearClip
					   >> dst.mFarClip;
				}
				else if ((gDeserializeVersion & 0xFF) >= 8)
				{
				   return src >> static_cast<Base&>(dst)
					   >> dst.mFov
					   >> dst.mFocalDepth
					   >> dst.mNearRange
					   >> dst.mFarRange;
				}
				else
				{
					return src >> static_cast<Base&>(dst)
						>> dst.mFov;
				}					   
			}

			friend Stream& operator<< (Stream& dst, const Camera& src)
			{
				return dst << static_cast<const Base&>(src)
					<< src.mFov
					<< src.mFocalDepth
					<< src.mNearRange
					<< src.mFarRange
					<< src.mNearClip
					<< src.mFarClip;
			}
    };
        
		class Frame
		{
		public:
			Frame(float time = 0.0f)
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

			int numLights(void) const 
			{
				return mLights.size();
			}

			const Light& light(int i) const 
			{ 
				return mLights[DebugRange(i, numLights())];
			}

			void insertLight(const Light& light) 
			{
				mLights.push_back(light);
			}

			int numObjects(void) const
			{
				return mObjects.size();
			}

			const Object& object(int i) const
			{
				return mObjects[DebugRange(i, numObjects())];
			}

			void insertObject(const Object& object)
			{
				mObjects.push_back(object);
			}
			
			int numCameras(void) const
			{
				return mCameras.size();
			}
			
			const Camera& camera(int i) const
			{
				return mCameras[DebugRange(i, numCameras())];
			}
			
			void insertCamera(const Camera& camera)
			{
				mCameras.push_back(camera);
			}
			      
			friend Stream& operator>> (Stream& src, Frame& dst)
			{
				return src.readObj(dst.mTime).readVec(dst.mLights).readVec(dst.mObjects).readVec(dst.mCameras);
			}

			friend Stream& operator<< (Stream& dst, const Frame& src)
			{
				return dst.writeObj(src.mTime).writeVec(src.mLights).writeVec(src.mObjects).writeVec(src.mCameras);
			}

		private:
			float mTime;
			std::vector<Light> mLights;
			std::vector<Object> mObjects;
			std::vector<Camera> mCameras;
		};

    class Scene
		{
		public:
			Scene()
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

			const Frame& frame(int i) const
			{
				return mFrames[DebugRange(i, numFrames())];
			}

			float frameTime(int i) const
			{
				return frame(i).time();
			}

			float duration(void) const
			{
				if (!numFrames())
					return 0.0f;

				return frameTime(numFrames() - 1);
			}

			void insertFrame(const Frame& frame)
			{
				mFrames.push_back(frame);
			}

			// true on failure
			bool read(Stream& src)
			{
				clear();

            uint ver;
				src >> ver;
				gDeserializeVersion = ver;
				if ((ver >> 8) != (((uint)SceneDataVersion) >> 8))
					return true;

				src.readVec(mFrames);
								
				src >> ver;
				if (gDeserializeVersion != ver)
					return true;

				return src.errorStatus();
			}

			friend Stream& operator>> (Stream& src, Scene& dst)
			{
				dst.read(src);
				return src;
			}
			
			friend Stream& operator<< (Stream& dst, const Scene& src)
			{
				dst << uint(SceneDataVersion);
				dst.writeVec(src.mFrames);
				dst << uint(SceneDataVersion);
				return dst;
			}
		
		private:
			std::vector<Frame> mFrames;
		};

	} // namespace SceneData

} // namespace gr

#endif // SCENEDATA_H


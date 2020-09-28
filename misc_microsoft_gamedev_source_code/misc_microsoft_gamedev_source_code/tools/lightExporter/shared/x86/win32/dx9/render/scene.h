//-----------------------------------------------------------------------------
// File: scene.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef SCENE_H
#define SCENE_H

#include "ugx_render.h"
#include "anim.h"
#include "body_bounds.h"

#include "common/math/quat.h"
#include "common/math/intersection.h"
#include "common/scene/scene_data.h"
#include "common/utils/logfile.h"
#include "common/utils/tweaker.h"
#include "common/utils/object_cache.h"

#include "common/property/PropertyFile.h"

#include "x86/win32/dx9/render/lighting_engine.h"

namespace gr
{
	const int SceneMaxLights = 256;
	const int SceneMaxObjects = 512;

	class LightTweakers
	{
		enum { MaxLights = SceneMaxLights };

		TweakerPage* mpTweakerPage;
		
		int mCurLight;
		int mLightTypes[MaxLights];
		float mDirYaw[MaxLights];
		float mDirPitch[MaxLights];
		Vec4 mUp[MaxLights];
		Vec4 mRight[MaxLights];
		float mSpinYaw[MaxLights];
		float mSpinPitch[MaxLights];
		float mPosX[MaxLights];
		float mPosY[MaxLights];
		float mPosZ[MaxLights];
		float mRadius[MaxLights];
		float mRadiusPulse[MaxLights];
		float mSpotInner[MaxLights];
		float mSpotOuter[MaxLights];
		float mFarAttenStart[MaxLights];
		float mDecayDist[MaxLights];
		float mDiffInten[MaxLights];
		float mSpecInten[MaxLights];
		bool mSepSpecInten[MaxLights];
		float mIntenPulse[MaxLights];
		float mXOfs[MaxLights];
		float mColor0R[MaxLights];
		float mColor0G[MaxLights];
		float mColor0B[MaxLights];
		float mColor1R[MaxLights];
		float mColor1G[MaxLights];
		float mColor1B[MaxLights];
		bool mShadows[MaxLights];
		bool mFog[MaxLights];
		bool mFogShadows[MaxLights];
		float mFogDensity[MaxLights];
		float mX;
		bool mDisableAllLights;
		bool mLightAnim;
		bool mHeadLight;
		int mHeadLightSlot;
					
		void newTweaker(const char* pName, int* pArray, const char** ppDesc, int numDesc, bool wrap = false);
		void newTweaker(const char* pName, float* pArray, float low, float high, float inc, bool singleStep = false, bool wrap = false);
		void newTweaker(const char* pName, bool* pArray);
		
	public:
		LightTweakers();
		~LightTweakers();
		
		bool lightAnimationEnabled(void) const;
		void clear(void);
		void init(void);
		void tick(float deltaT);
		int numSlots(void) const;
		bool isActive(int slot);
		Light& get(int slot, Light& params) const;
		void set(int slot, const Light& params);
	};
	
	class Scene;

	class MeshCache : public ObjectCache<std::string, RenderMesh*>
	{
	public:
		MeshCache() : ObjectCache<std::string, RenderMesh*>(true)
		{
		}

		virtual Object createObject(const Key& key)
		{
			Verify(false);
			return new RenderMesh();
		}

		virtual void destroyObject(Object& obj)
		{
			delete obj;
		}
	};

	class AnimCache : public ObjectCache<std::string, Anim*>
	{
	public:
		AnimCache() : ObjectCache<std::string, Anim*>(true)
		{
		}

		virtual Object createObject(const Key& key)
		{
			Verify(false);
			return new Anim();
		}

		virtual void destroyObject(Object& obj)
		{
			delete obj;
		}
	};
		
	class SceneShadowBufferModelRenderer : public ShadowBufferModelRenderer
	{
		Scene& mScene;
	public:
		SceneShadowBufferModelRenderer(Scene& scene) : 
			ShadowBufferModelRenderer(),
			mScene(scene)
		{
		}

		virtual ~SceneShadowBufferModelRenderer()
		{
		}

		virtual bool render(const ShadowBufferParams& shadowBufferParams, int pass);
	};

	class SceneInstance
	{
	public:
		SceneInstance(Anim* pAnim, RenderMesh* pMesh, RenderMesh* pShadowMesh, const Sphere& boundingSphere, int sceneDataObjectIndex) :
			mpAnim(pAnim),
			mpMesh(pMesh),
			mpShadowMesh(pShadowMesh),
			mBoundingSphere(boundingSphere),
			mSceneDataObjectIndex(sceneDataObjectIndex)
		{
			setModelToWorld(Matrix44::I);
		}

		SceneInstance(const SceneInstance& rhs)
		{
			*this = rhs;
		}

		SceneInstance& operator= (const SceneInstance& rhs)
		{
			mpAnim = rhs.mpAnim;
			mpMesh = rhs.mpMesh;
			mpShadowMesh = rhs.mpShadowMesh;
			mBoundingSphere = rhs.mBoundingSphere;
			mSceneDataObjectIndex = rhs.mSceneDataObjectIndex;
			mModelToWorld[0] = rhs.mModelToWorld[0];
			mModelToWorld[1] = rhs.mModelToWorld[1];
			mModelToWorld[2] = rhs.mModelToWorld[2];
			mModelToWorld[3] = rhs.mModelToWorld[3];

			return *this;
		}

		virtual ~SceneInstance()
		{
		}

		Anim* anim(void) const 
		{ 
			return mpAnim; 
		}
		
		RenderMesh* mesh(void) const 
		{ 
			return mpMesh; 
		}
		
		RenderMesh* shadowMesh(void) const 
		{ 
			return mpShadowMesh; 
		}
		
		const Sphere& boundingSphere(void) const 
		{ 
			return mBoundingSphere; 
		}
		
		int sceneDataObjectIndex(void) const
		{
			return mSceneDataObjectIndex;
		}
		
		void setModelToWorld(const Matrix44& m) 
		{
			mModelToWorld[0] = m[0];
			mModelToWorld[1] = m[1];
			mModelToWorld[2] = m[2];
			mModelToWorld[3] = m[3];
		}
		
		Matrix44 getModelToWorld(void) const
		{
			return Matrix44(mModelToWorld[0], mModelToWorld[1], mModelToWorld[2], mModelToWorld[3]);
		}
       
	private:
		Anim* mpAnim;
		RenderMesh* mpMesh;
		RenderMesh* mpShadowMesh;
    Sphere mBoundingSphere;
    int mSceneDataObjectIndex;
    Vec<4> mModelToWorld[4];
		
		void clear(void)
		{
			mpAnim = NULL;
			mpMesh = NULL;
			mpShadowMesh = NULL;
			mBoundingSphere.clear();
			mSceneDataObjectIndex = 0;
		}
	};
	
	class SceneEmitterInstance
	{
	public:
		SceneEmitterInstance()
		{
			clear();
		}
		
		SceneEmitterInstance(const std::string& name, int sceneObjectIndex, ParticleSystem* pSystem) :
			mName(name),
			mSceneObjectIndex(sceneObjectIndex),
			mpSystem(pSystem)
		{
		}
						
		const std::string& name(void) const 
		{
			return mName;
		}
		
		int sceneObjectIndex(void) const
		{
			return mSceneObjectIndex;
		}
		
		ParticleSystem* particleSystem(void) const
		{
			return mpSystem;
		}
			
	private:
		std::string mName;
		int mSceneObjectIndex;
		ParticleSystem* mpSystem;
		
		void clear(void)
		{
			mName = "";
			mSceneObjectIndex = 0;
			mpSystem = NULL;
		}
	};

	class Scene
	{
		Scene(Scene& other);
		Scene& operator= (Scene& other);
		
	public:
		Scene();
			    
		virtual ~Scene();
		
		void tick(float deltaT);
		
		// true on repeat
		bool advanceTime(float deltaT);
		
		void resetTime(void);
		
		float duration(void) const;
		
		float curTime(void) const;
		
		bool loaded(void) const;
				
		void clear(void);
				
		// true on failure
		bool load(const char* pFilename);
		
		void renderInitStates(void);
		
		void renderDeinitStates(void);
		
		float getExposure(void);
		
		int numCameras(void) const;
		Camera getCamera(int index, float aspect, float n, float f);
		
		void beginScene(void);
						
		void renderFirst(void);
				
		void renderSecond(void);
				
		void light(void);
				
		void final(bool toneMap);
						
		void endScene(void);
				
		//void final(void);
				
		static void initStaticTweakers(void);
		
		void oneTimeSceneInit(void);
				
		void initDeviceObjects(void);
				
		void deleteDeviceObjects(void);
				
		void invalidateDeviceObjects(void);
				
		void restoreDeviceObjects(void);
		
		void startOfFrame(void);
		
		bool renderShadowModels(const ShadowBufferParams& shadowBufferParams, int pass);
		bool renderShadowModelsSpotOmni(const ShadowBufferParams& shadowBufferParams, int pass);
		
		void render(bool toneMap);
		
		bool setInstanceModelToWorld(const char* pName, const Matrix44& objToWorld);
				
		void* operator new  (size_t size);
		void* operator new[] (size_t size);
		void operator delete  (void* p);
		void operator delete[] (void* p);
	
	protected:
		RenderViewport mRenderViewport;

		LightTweakers mLightTweakers;

		SceneData::Scene mSceneData;
		std::vector<PropertyFile> mObjectProps;
		
		std::vector<SceneInstance> mInstances;
		std::vector<SceneEmitterInstance> mEmitters;
		
		Matrix44 mModelToWorld[SceneMaxObjects];
		bool mVis[SceneMaxObjects];

		float mCurTime;
		float mDuration;
		
		MeshCache mMeshCache;
		MeshCache mShadowMeshCache;
		AnimCache mAnimCache;
		
		ParticleSystemManager* mpParticleManager;
		
		int mExposureObjectIndex;
		
		TextureProxy* mLightMasks[SceneMaxLights];
		
		BodyBounds mBodyBounds;
		
		static bool mRenderParticles;
		static bool mTickParticles;
		static bool mRenderHier;
		
		bool loadObjects(void);
		RenderMesh* loadMesh(const char* pFilename);
		RenderMesh* loadRawMesh(const char* pFilename);
		Anim* loadAnim(const char* pFilename);
		void findDuration(void);

		struct InterpFrames
		{
			int frame[2];
			float t;
			
			InterpFrames()
			{
				frame[0] = frame[1] = 0;
				t = 0.0f;
			}
		};

		void findSceneInterpFrames(InterpFrames& interpFrames);
		Matrix44 interpModelToWorld(
			const SceneData::Base& obj0, 
			const SceneData::Base& obj1, 
			float t, 
			const Matrix44& postXForm = Matrix44::makeMaxToD3D()); // <--- EVIL
		
		void setupLights(void);
		void updateLights(void);
		void renderLight(const Light& params);
		void renderParticles(void);
		bool parseObjectProps(void);
		void loadMeshObject(int objectIndex, const SceneData::Object& obj, const PropertySet& props);
		void loadEmitterObject(int objectIndex, const SceneData::Object& obj, const PropertySet& props);
		void loadExposureObject(int objectIndex, const SceneData::Object& obj, const PropertySet& props);
		void tickParticles(float deltaT);
		void resetParticles(void);
	};

	inline bool SceneShadowBufferModelRenderer::render(const ShadowBufferParams& shadowBufferParams, int pass)
	{
		return mScene.renderShadowModels(shadowBufferParams, pass);
	}

} // namespace gr

#endif // SCENE_H



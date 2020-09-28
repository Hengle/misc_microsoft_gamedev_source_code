//-----------------------------------------------------------------------------
// File: scene.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "scene.h"

#include "common/geom/ugx_instancer.h"

#define MESH_EXTENSION "UGX"
#define RAW_MESH_EXTENSION "UGF"
#define ANIM_EXTENSION "ANM"
#define EMITTER_EXTENSION "TXT"
	
namespace gr
{
	void LightTweakers::newTweaker(const char* pName, int* pArray, const char** ppDesc, int numDesc, bool wrap)
	{
		IntegralTweakerEntry* pEntry = new IntegralTweakerEntry(pName, pArray, ppDesc, numDesc, MaxLights, &mCurLight);
		pEntry->setWrap(wrap);
		mpTweakerPage->add(pEntry);
	}

	void LightTweakers::newTweaker(const char* pName, float* pArray, float low, float high, float inc, bool singleStep, bool wrap)
	{
		IntegralTweakerEntry* pEntry = new IntegralTweakerEntry(pName, pArray, low, high, inc, singleStep, MaxLights, &mCurLight);
		pEntry->setWrap(wrap);
		mpTweakerPage->add(pEntry);
	}

	void LightTweakers::newTweaker(const char* pName, bool* pArray)
	{
		mpTweakerPage->add(new IntegralTweakerEntry(pName, pArray, MaxLights, &mCurLight));
	}

	LightTweakers::LightTweakers() :
		mpTweakerPage(NULL),
		mHeadLight(false),
		mHeadLightSlot(255)
	{
		clear();
	}

	LightTweakers::~LightTweakers()
	{
	}

	bool LightTweakers::lightAnimationEnabled(void) const
	{
		return mLightAnim;
	}

	void LightTweakers::clear(void)
	{
		mCurLight = 0;
		mX = 0.0f;

		for (int i = 0; i < MaxLights; i++)
		{
			mRadius[i] = 300.0f;
			mRadiusPulse[i] = 0.0f;
			mSpotInner[i] = 20.0f;
			mSpotOuter[i] = 60.0f;
			mFarAttenStart[i] = .4f;
			mDecayDist[i] = 120.0f;
			mDiffInten[i] = 10;
			mSpecInten[i] = 10;
			mSepSpecInten[i] = false;
			mColor0R[i] = mColor0G[i] = mColor0B[i] = 1.0f;
			mColor1R[i] = mColor1G[i] = mColor1B[i] = 1.0f;
			mDirPitch[i] = -80.0f;
			mPosY[i] = 100.0f;
			mSpinYaw[i] = 0.0f;
			mSpinPitch[i] = 0.0f;
			mIntenPulse[i] = 0.0f;
			mXOfs[i] = Math::fRand(0.0f, Math::fTwoPi);

			mLightTypes[i] = 0;
			mDirYaw[i] = 0;
			mDirPitch[i] = 0;
			mUp[i].setZero();
			mRight[i].setZero();
			mSpinYaw[i] = 0;
			mSpinPitch[i] = 0;
			mPosX[i] = 0;
			mPosZ[i] = 0;
												
			mShadows[i] = true;
			
			mFog[i] = false;
			mFogShadows[i] = false;
			mFogDensity[i] = 1.0f;
		}

		//mLightTypes[MaxLights - 1] = eSpot + 1;
		mRadius[MaxLights - 1] = 600.0f;

		if (mpTweakerPage)
		{
			gSharedTweaker.get().del(mpTweakerPage);
			mpTweakerPage = NULL;
		}

		mDisableAllLights = false;
		mLightAnim = true;
	}

	void LightTweakers::init(void)
	{
		clear();

		mpTweakerPage = new TweakerPage("Lights");
		
		gSharedTweaker.get().add(mpTweakerPage);
					
		mpTweakerPage->add(new IntegralTweakerEntry("Light animation", &mLightAnim));
		mpTweakerPage->add(new IntegralTweakerEntry("Disable all lights", &mDisableAllLights));
		mpTweakerPage->add(new IntegralTweakerEntry("Head light", &mHeadLight));
		mpTweakerPage->add(new IntegralTweakerEntry("Head light slot", &mHeadLightSlot, 0, MaxLights - 1, 1, true));
				
		mpTweakerPage->add(new IntegralTweakerEntry("Light #", &mCurLight, 0, MaxLights - 1, 1, true));

		static const char* pLightDesc[] = { "Inactive", "Omni", "Spot", "Directional" };
		const int NumLightTypes = 4;
		newTweaker("Type", mLightTypes, pLightDesc, sizeof(pLightDesc) / sizeof(pLightDesc[0]));
		
		newTweaker("PosX", mPosX, -10000, 10000, 5.0f);
		newTweaker("PosY", mPosY, -10000, 10000, 5.0f);
		newTweaker("PosZ", mPosZ, -10000, 10000, 5.0f);
		newTweaker("Radius", mRadius, 1, 5000, 5.0f);
		newTweaker("RadiusPulse", mRadiusPulse, 0.0f, 30.0f, .01f);
		newTweaker("FarAttenStart", mFarAttenStart, 0.0f, 1.0f, .02f);
		newTweaker("DecayDist", mDecayDist, 0.0f, 5000, 1.0f);
		
		newTweaker("SpotDirYaw", mDirYaw, 0.0f, 359.999f, 3.0f, false, true);
		newTweaker("SpotDirPitch", mDirPitch, 0.0f, 359.999f, 3.0f, false, true);
		newTweaker("SpotDirYawSpin", mSpinYaw, -720.0f, 720.0f, 1.0f, false, false);
		newTweaker("SpotDirPitchSpin", mSpinPitch, -720.0f, 720.0f, 1.0f, false, false);
		newTweaker("SpotInner", mSpotInner, 0.0f, 160.0f, 2.0f);
		newTweaker("SpotOuter", mSpotOuter, 0.0f, 160.0f, 2.0f);
				
		newTweaker("Intensity Pulse", mIntenPulse, 0.0f, 30.0f, .01f);
		newTweaker("Allow Seperate Spec/Diff Inten", mSepSpecInten);
		newTweaker("Linear Diff Intensity", mDiffInten, -200.0f, 1200.0f, 1.0f);
				
		newTweaker("  LinearColor0R", mColor0R, -200.0f, 200.0f, .0075f);
		newTweaker("  LinearColor0G", mColor0G, -200.0f, 200.0f, .0075f);
		newTweaker("  LinearColor0B", mColor0B, -200.0f, 200.0f, .0075f);
		
		newTweaker("Linear Spec Intensity", mSpecInten, -200.0f, 200.0f, 1.0f);
		newTweaker("  LinearColor1R", mColor1R, -200.0f, 200.0f, .0075f);
		newTweaker("  LinearColor1G", mColor1G, -200.0f, 200.0f, .0075f);
		newTweaker("  LinearColor1B", mColor1B, -200.0f, 200.0f, .0075f);
		newTweaker("Shadows", mShadows);
		
		newTweaker("Fog", mFog);
		newTweaker("FogShadows", mFogShadows);
		newTweaker("FogDensity", mFogDensity, 0.0f, 1.0f, 1.0f/128.0f);
	}

	void LightTweakers::tick(float deltaT)
	{
		mX += deltaT;
		if (mX > 4096)
			mX = 0.0f;

		for (int i = 0; i < MaxLights; i++)
		{
			mDirYaw[i] = Math::fPosMod(mDirYaw[i] + mSpinYaw[i] * deltaT, 360.0f);
			mDirPitch[i] = Math::fPosMod(mDirPitch[i] + mSpinPitch[i] * deltaT, 360.0f);
		}

		if (mDisableAllLights)
		{
			mDisableAllLights = false;
			for (int i = 0; i < MaxLights; i++)
				mLightTypes[i] = 0;
		}

		if (mHeadLight)
		{
			const int i = mHeadLightSlot;
			const Matrix44& worldToView = RenderEngine::sceneRenderViewport().camera().worldToView();
			const Matrix44& viewToWorld = RenderEngine::sceneRenderViewport().camera().viewToWorld();
			
			const float headLightYOfs = 20;
			mPosX[i] = viewToWorld.getTranslate()[0];
			mPosY[i] = viewToWorld.getTranslate()[1] + headLightYOfs;
			mPosZ[i] = viewToWorld.getTranslate()[2];
      
			const Vec4 sphericalDir(Vec4::makeSpherical(viewToWorld.getRow(2)));

			mDirYaw[i] = Math::fPosMod(Math::fRadToDeg(sphericalDir[0]), 360.0f);
			mDirPitch[i] = Math::fPosMod(Math::fRadToDeg(sphericalDir[1]), 360.0f);
		}
	}

	int LightTweakers::numSlots(void) const
	{
		return MaxLights;
	}

	bool LightTweakers::isActive(int slot)
	{
		return mLightTypes[DebugRange(slot, MaxLights)] != 0;
	}

	Light& LightTweakers::get(int slot, Light& params) const
	{
		DebugRange(slot, MaxLights);
      						
		params.mType = (ELightType)(mLightTypes[slot] - 1);
		
		params.mPos = Vec4(mPosX[slot], mPosY[slot], mPosZ[slot], 1.0f);
		
		params.mDir = Vec4::makeCartesian(Vec4(Math::fDegToRad(mDirYaw[slot]), Math::fDegToRad(mDirPitch[slot]), 1.0f, 0.0f));
		
		params.mRadius = mRadius[slot];
		if (mRadiusPulse[slot] != 0.0f)
		{
			params.mRadius *= (.5f + .5f * sin(mXOfs[slot] + mX * Math::fTwoPi * mRadiusPulse[slot]));
		}

		params.mSpotInner = Math::fDegToRad(Math::Min(mSpotInner[slot], mSpotOuter[slot]));
		params.mSpotOuter = Math::fDegToRad(Math::Max(mSpotInner[slot], mSpotOuter[slot]));
		
		if (Math::EqualTol(params.mSpotInner, params.mSpotOuter, Math::fDegToRad(.1f)))
		{
			if (params.mSpotOuter < Math::fDegToRad(179.0f))
				params.mSpotOuter += Math::fDegToRad(1.0f);
			else
				params.mSpotInner -= Math::fDegToRad(1.0f);
		}
		
		params.mFarAttenStart = mFarAttenStart[slot];
		params.mDecayDist = mDecayDist[slot];
		params.mShadows = mShadows[slot];
		
		float diffInten = mDiffInten[slot];
		
		float specInten = diffInten;
		if (mSepSpecInten[slot])
			specInten = mSpecInten[slot];

		if (mIntenPulse[slot] != 0.0f)
		{
			diffInten *= (.5f + .5f * sin(mXOfs[slot] + mX * Math::fTwoPi * mIntenPulse[slot]));
			specInten *= (.5f + .5f * sin(mXOfs[slot] + mX * Math::fTwoPi * mIntenPulse[slot]));
		}
							
		params.mColor0[0] = mColor0R[slot] * diffInten;
		params.mColor0[1] = mColor0G[slot] * diffInten;
		params.mColor0[2] = mColor0B[slot] * diffInten;
		params.mColor1[0] = mColor1R[slot] * specInten;
		params.mColor1[1] = mColor1G[slot] * specInten;
		params.mColor1[2] = mColor1B[slot] * specInten;
		
		params.mUp = mUp[slot];
		params.mRight = mRight[slot];
		
		params.mFog = mFog[slot];
		params.mFogShadows = mFogShadows[slot];
		params.mFogDensity = mFogDensity[slot];
		
		return params;
	}

	void LightTweakers::set(int slot, const Light& params)
	{
		DebugRange(slot, MaxLights);

		mLightTypes[slot] = params.mType + 1;
					
		const Vec4 sphericalDir(Vec4::makeSpherical(params.mDir));

		mDirYaw[slot] = Math::fPosMod(Math::fRadToDeg(sphericalDir[0]), 360.0f);
		mDirPitch[slot] = Math::fPosMod(Math::fRadToDeg(sphericalDir[1]), 360.0f);

		mPosX[slot] = params.mPos[0];
		mPosY[slot] = params.mPos[1];
		mPosZ[slot] = params.mPos[2];
		mRadius[slot] = params.mRadius;
		
		mSpotInner[slot] = Math::fRadToDeg(params.mSpotInner);
		mSpotOuter[slot] = Math::fRadToDeg(params.mSpotOuter);
		
		mFarAttenStart[slot] = params.mFarAttenStart;
		mDecayDist[slot] = params.mDecayDist;
				
		mDiffInten[slot] = 1.0f;
		mSpecInten[slot] = 1.0f;
		mSepSpecInten[slot] = true;
		
		mColor0R[slot] = params.mColor0[0];
		mColor0G[slot] = params.mColor0[1];
		mColor0B[slot] = params.mColor0[2];
		mColor1R[slot] = params.mColor1[0];
		mColor1G[slot] = params.mColor1[1];
		mColor1B[slot] = params.mColor1[2];
		mShadows[slot] = params.mShadows;
		
		mUp[slot] = params.mUp;
		mRight[slot] = params.mRight;
		
		mFog[slot] = params.mFog;
		mFogShadows[slot] = params.mFogShadows;
		mFogDensity[slot] = params.mFogDensity;
	}

	bool Scene::mRenderHier = false;
	bool Scene::mRenderParticles = true;
	bool Scene::mTickParticles = true;

	Scene::Scene() : 
		mCurTime(0.0f),
		mDuration(0.0f),
		mpParticleManager(NULL),
		mExposureObjectIndex(-1)
	{
		Utils::ClearObj(mLightMasks);
	}
  
	Scene::~Scene()
	{
		clear();
		
		delete mpParticleManager;
	}
	
	void Scene::tickParticles(float deltaT)
	{
		if (!mTickParticles)
			return;
		
		deltaT = Math::Max(0.0f, deltaT);
		
		const Matrix44 D3DToMax(Matrix44::makeMaxToD3D().inverse());
		
		DebugNull(mpParticleManager);
						
		InterpFrames interpFrames;
		findSceneInterpFrames(interpFrames);
		
		const SceneData::Frame& frame0 = mSceneData.frame(interpFrames.frame[0]);
		const SceneData::Frame& frame1 = mSceneData.frame(interpFrames.frame[1]);
									
		for (int i = 0; i < mEmitters.size(); i++)
		{
			ParticleSystem* pSystem = mEmitters[i].particleSystem();
			const SceneData::Object& obj0 = frame0.object(mEmitters[i].sceneObjectIndex());
			const SceneData::Object& obj1 = frame1.object(mEmitters[i].sceneObjectIndex());
						
			const Matrix44 modelToWorld(D3DToMax * interpModelToWorld(obj0, obj1, interpFrames.t));
			
			float vis = Math::Lerp(obj0.mVisibility, obj1.mVisibility, interpFrames.t);
			
			pSystem->Emitter().FollowObject() = *reinterpret_cast<const D3DXMATRIX*>(&modelToWorld);
			pSystem->Emitter().Enabled() = (vis >= .00125f);
			pSystem->Emitter().Intensity() = vis;
			pSystem->Update(deltaT);
		}
	}
	
	void Scene::resetParticles(void)
	{
		for (int i = 0; i < mEmitters.size(); i++)
		{
			ParticleSystem* pSystem = mEmitters[i].particleSystem();
			pSystem->Reset();
		}
	}

	void Scene::tick(float deltaT)
	{
		mLightTweakers.tick(deltaT);
	}
		
	// true on repeat
	bool Scene::advanceTime(float deltaT)
	{
		bool ret = false;
		
		if (mDuration)
		{
			mCurTime = mCurTime + deltaT;
			if (mCurTime >= mDuration)
			{
				mCurTime = 0.0f;
				ret = true;
			}
			else if (mCurTime < 0.0f)
				mCurTime = mDuration - 1.0f/30.0f;
			
			if (0.0f != deltaT)
				updateLights();
		}
		else
			mCurTime = 0.0f;

		if (loaded())
			tickParticles(deltaT);
		
		return ret;
	}
			
	void Scene::resetTime(void)
	{
		mCurTime = 0.0f;
		
		resetParticles();
		
		updateLights();
	}

	float Scene::duration(void) const
	{
		return mDuration;
	}

	float Scene::curTime(void) const
	{
		return mCurTime;
	}

	bool Scene::loaded(void) const
	{
		return 0 != mSceneData.numFrames();
	}
	
	void Scene::clear(void)
	{
		mLightTweakers.init();

		mSceneData.clear();
		
		mInstances.clear();
		mEmitters.clear();

		mMeshCache.clear();
		mShadowMeshCache.clear();
		mAnimCache.clear();
		
		mCurTime = 0.0f;
		mDuration = 0.0f;
		
		mObjectProps.clear();
		
		mExposureObjectIndex = -1;
		
		if (mpParticleManager)
			mpParticleManager->Clear();
		
		Utils::ClearObj(mLightMasks);
		
		RenderEngine::flushTextures();
	}
	
	// true on failure
	bool Scene::parseObjectProps(void)
	{
		Status("parseObjectProps:\n");
		
		const SceneData::Frame& frame = mSceneData.frame(0);

		Verify(frame.numObjects() < SceneMaxObjects);
					
		mObjectProps.resize(frame.numObjects());
		
		for (int i = 0; i < frame.numObjects(); i++)
		{
			const SceneData::Object& object = frame.object(i);
			
			if (!object.mUDP.empty())
			{
				ReadableDataStream stream(&object.mUDP[0], object.mUDP.size());
				PropertyStreamLoader loader(object.mName.c_str(), stream);
				
				PropertyFile props(loader);			
				
				Status("Object %i: \"%s\" UDP Parse Warnings: %i Errors: %i\n",
					i,
					object.mName.c_str(),
					props.GetWarningCount(),
					props.GetErrorCount());
					
				if (props.GetWarningCount() + props.GetErrorCount())
				{
					Status("Error/warning messages:\n");
					gSystem.message(System::ERR, props.GetErrorsAndWarnings().c_str());
					
					Status("Scene::parseObjectProps: UDP Parse failure\n");
					return true;
				}
				
				if (!props.HasPropertySet(""))
				{
					Error("Error: UDP parse succeeded, but the root property set is missing!\n");
					Status("Scene::parseObjectProps: UDP Parse failure\n");
					return true;
				}
				
				const PropertySet& propSet = props.GetPropertySet("");
				if (!propSet.HasPropertyValue("type"))
				{
					Error("Error: UDP parse succeeded, and the root set was found, but the \"type\" value is missing!\n");
					Status("Scene::parseObjectProps: UDP Parse failure\n");
					return true;
				}
					
				mObjectProps[i] = props;
			}
			else
				Status("Object %i: \"%s\" No UDP\n", i, object.mName.c_str());
		}
		
		Status("Scene::parseObjectProps: Success\n");
		
		return false;
	}
			
	// true on failure
	bool Scene::load(const char* pFilename)
	{
		clear();
		
		const DWORD beforeAvail = D3D::getDevice()->GetAvailableTextureMem();
		Status("Available texture memory: %i\n", beforeAvail);
		
		Status("Scene::load: Loading scene file \"%s\"\n", pFilename);
					
		std::auto_ptr<Stream> pStream = std::auto_ptr<Stream>(gFileSystem.createStream(pFilename));
		
		if ((!pStream.get()) || (pStream->errorStatus()))
		{
			Error("Scene::load: Unable to open file \"%s\"!", pFilename);
			return true;
		}

		if (mSceneData.read(*pStream))
		{
			Error("Scene::load: SceneData reader failed!\n");
			clear();
			return true;
		}

		if (0 == mSceneData.numFrames())
		{
			Error("Scene::load: Scene contains no frames!\n");
			clear();
			return true;
		}

		if (mSceneData.frame(0).numLights() > SceneMaxLights)
		{
			Error("Scene::load: Scene contains too many lights!\n");
			clear();
			return true;
		}
		
		Status("Scene::load: Frames: %i, Lights: %i, Objects: %i\n", 
			mSceneData.numFrames(), 
			mSceneData.frame(0).numLights(),
			mSceneData.frame(0).numObjects());
		
		if (parseObjectProps())
		{
			Error("Scene::load: Unable to parse object properties!\n");
			clear();
			return true;
		}

		loadObjects();
		
		findDuration();

		setupLights();
		
		updateLights();
		
		Status("Scene::load: Mesh instances: %i, Emitters: %i\n", mInstances.size(), mEmitters.size());
		
		const DWORD afterAvail = D3D::getDevice()->GetAvailableTextureMem();
		Status("Available texture memory: %i, Difference: %i\n", afterAvail, beforeAvail - afterAvail);

		Status("Scene::load: Success\n");
					
		return false;
	}

	void Scene::renderInitStates(void)
	{
		//RenderEngine::setCameraMatrices(mRenderViewport.camera());
				
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		D3D::setRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	}

	void Scene::renderDeinitStates(void)
	{
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		D3D::setRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	}
	
	int Scene::numCameras(void) const
	{
		if (!loaded())
			return 0;
		
		return mSceneData.frame(0).numCameras();
	}
	
	float Scene::getExposure(void) 
	{
		if ((!loaded()) || (-1 == mExposureObjectIndex))
			return 0;
			
		InterpFrames interpFrames;
		findSceneInterpFrames(interpFrames);
		
		const SceneData::Frame& frame0 = mSceneData.frame(interpFrames.frame[0]);
		const SceneData::Frame& frame1 = mSceneData.frame(interpFrames.frame[1]);	
		
		const SceneData::Object& obj0 = frame0.object(mExposureObjectIndex);
		const SceneData::Object& obj1 = frame1.object(mExposureObjectIndex);
		
		const float vis = Math::Lerp(obj0.mVisibility, obj1.mVisibility, interpFrames.t);
		
		const float ExposureBias = 8.0f;
		return Math::Clamp(vis - ExposureBias, -8.0f, 8.0f);
	}
	
	Camera Scene::getCamera(int index, float aspect, float n, float f) 
	{
		Assert(loaded());

		InterpFrames interpFrames;
		findSceneInterpFrames(interpFrames);
		
		const SceneData::Frame& frame0 = mSceneData.frame(interpFrames.frame[0]);
		const SceneData::Frame& frame1 = mSceneData.frame(interpFrames.frame[1]);	
		
		const SceneData::Camera& obj0 = frame0.camera(index);
		const SceneData::Camera& obj1 = frame1.camera(index);
					
		const float PopDistThreshold = 25.0f;
		// HACK HACK: Ensures camera snaps to new position when framerate isn't exactly 30
		if ((obj0.mPos - obj1.mPos).len() > PopDistThreshold)
		{
			if (interpFrames.t < .5f)
				interpFrames.t = 0.0f;
			else
				interpFrames.t = 1.0f;
		}
				
		const Matrix44 maxViewToWorld(interpModelToWorld(obj0, obj1, interpFrames.t, Matrix44::I));	

		Matrix44 maxCamFlip;

		maxCamFlip.setIdentity();

		maxCamFlip[0][0] = -1.0f;
		
		maxCamFlip[2][1] = 1.0f;
		maxCamFlip[1][1] = 0.0f;
		
		maxCamFlip[1][2] = 1.0f;
		maxCamFlip[2][2] = 0.0f;
				
		Matrix44 viewToWorld = maxViewToWorld;
				
		Matrix44 worldToView(viewToWorld.inverse() * maxCamFlip);
		
		worldToView.setTranslate(Vec4(0,0,0,1));
		
		const Matrix44 maxTran(Matrix44::makeTranslate(Vec4::multiply(Vec4(-1,-1,-1,1), maxViewToWorld.getTranslate())));
				
		const Matrix44 MaxToD3D(Matrix44::makeMaxToD3D());
		const Matrix44 D3DToMax(Matrix44::makeMaxToD3D().inverse());
		
		Camera camera;		
		
		Matrix44 finalWorldToView(
			D3DToMax * 
			maxTran *
			worldToView *
			MaxToD3D);
				
		camera.setWorldToView(finalWorldToView);				
		
		camera.setViewToProj(aspect, n, f, Math::Lerp(obj0.mFov, obj1.mFov, interpFrames.t));
						
		return camera;
	}

	void Scene::beginScene(void)
	{
		mRenderViewport = RenderEngine::sceneRenderViewport();
	}

	void Scene::endScene(void)
	{
	}
	
	void Scene::renderFirst()
	{
		RenderEngine::lightingEngine().beginFirst();
		
		renderInitStates();

		if (!loaded())
		{
			renderDeinitStates();
			
			RenderEngine::lightingEngine().endFirst();
			return;
		}
		
		InterpFrames interpFrames;
		findSceneInterpFrames(interpFrames);
		
		const SceneData::Frame& frame0 = mSceneData.frame(interpFrames.frame[0]);
		const SceneData::Frame& frame1 = mSceneData.frame(interpFrames.frame[1]);
						
		mBodyBounds.clear();
		
		for (int i = 0; i < mInstances.size(); i++)
		{
			SceneInstance& sceneInstance = mInstances[i];

			RenderMesh* pMesh = sceneInstance.mesh();
			if (!pMesh)
				continue;
				
			Anim* pAnim = sceneInstance.anim();
			
			const SceneData::Object& obj0 = frame0.object(sceneInstance.sceneDataObjectIndex());
			const SceneData::Object& obj1 = frame1.object(sceneInstance.sceneDataObjectIndex());
						
			const Matrix44 modelToWorld(interpModelToWorld(obj0, obj1, interpFrames.t) * sceneInstance.getModelToWorld());
						
			mModelToWorld[i] = modelToWorld;
			mVis[i] = false;
			
			const float vis = Math::Lerp(obj0.mVisibility, obj1.mVisibility, interpFrames.t);
			if (vis >= .999f)
			{
				Sphere boundingSphere(
					Vec4(sceneInstance.boundingSphere().origin(), 1.0f) * modelToWorld, 
					sceneInstance.boundingSphere().radius());
        			
				if (OUTSIDE == Intersection::sphereFrustum(boundingSphere, mRenderViewport.camera().worldFrustum()))
					continue;
				
				mVis[i] = true;
				pMesh->render(modelToWorld, pAnim, mCurTime, eNormalPass);

				if (mRenderHier)
					pMesh->renderHier(modelToWorld, pAnim, mCurTime);
					
				pMesh->getBodyBounds(mBodyBounds, modelToWorld, pAnim, mCurTime);
			}
		}
		
		mBodyBounds.findCenter();

		renderDeinitStates();
		
		RenderEngine::lightingEngine().endFirst();
	}

	void Scene::renderSecond()
	{
		RenderEngine::lightingEngine().beginSecond();
		
		renderInitStates();
		
		D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
		
		if (!loaded())
		{
			D3D::setRenderState(D3DRS_ZWRITEENABLE, TRUE);
						
			renderDeinitStates();
			RenderEngine::lightingEngine().endSecond();
			return;
		}
						
		for (int i = 0; i < mInstances.size(); i++)
		{
			SceneInstance& sceneInstance = mInstances[i];

			RenderMesh* pMesh = sceneInstance.mesh();
			if ((!pMesh) || (!mVis[i]))
				continue;
				
			Anim* pAnim = sceneInstance.anim();
											
			pMesh->render(mModelToWorld[i], pAnim, mCurTime, eColorPass);
		}
		
		D3D::setRenderState(D3DRS_ZWRITEENABLE, TRUE);

		renderDeinitStates();
		RenderEngine::lightingEngine().endSecond();
	}

	void Scene::light(void)
	{
		if (!loaded())
			return;
			
		RenderEngine::lightingEngine().beginLight(mBodyBounds);

		for (int i = 0; i < mLightTweakers.numSlots(); i++)
		{
			if (mLightTweakers.isActive(i))
			{
				Light params;
				
				mLightTweakers.get(i, params);
				
				params.mMask = mLightMasks[i];
				
				renderLight(params);
			}
		}
		
		RenderEngine::lightingEngine().endLight();
	}
			
	void Scene::renderParticles(void)
	{
		if (mRenderParticles)
			RenderEngine::particleRenderer().render(mpParticleManager, mRenderViewport);
	}
		
	void Scene::final(bool toneMap)
	{
		RenderEngine::lightingEngine().beginFinal();
		
		renderParticles();
		
		RenderEngine::lightingEngine().endFinal(getExposure(), toneMap);
	}
		
	void Scene::initStaticTweakers(void)
	{
		gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Render wireframe hierarchy", &mRenderHier));
		
		gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Render particles", &mRenderParticles));
		gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Tick particles", &mTickParticles));
	}

	void Scene::oneTimeSceneInit(void)					
	{
		mLightTweakers.init();

		for (MeshCache::Container::iterator it = mMeshCache.getObjects().begin(); it != mMeshCache.getObjects().end();	++it)
			it->second->oneTimeSceneInit();
		
		for (MeshCache::Container::iterator it = mShadowMeshCache.getObjects().begin(); it != mShadowMeshCache.getObjects().end();	++it)
			it->second->oneTimeSceneInit();
			
		if (!mpParticleManager)
			mpParticleManager = ParticleSystemManager::CreateParticleSystemManager();
	}
	
	void Scene::initDeviceObjects(void)				
	{ 
		for (MeshCache::Container::iterator it = mMeshCache.getObjects().begin(); it != mMeshCache.getObjects().end();	++it)
			it->second->initDeviceObjects();

		for (MeshCache::Container::iterator it = mShadowMeshCache.getObjects().begin(); it != mShadowMeshCache.getObjects().end();	++it)
			it->second->initDeviceObjects();
	}
	
	void Scene::deleteDeviceObjects(void)			
	{ 
		for (MeshCache::Container::iterator it = mMeshCache.getObjects().begin(); it != mMeshCache.getObjects().end();	++it)
			it->second->deleteDeviceObjects();

		for (MeshCache::Container::iterator it = mShadowMeshCache.getObjects().begin(); it != mShadowMeshCache.getObjects().end();	++it)
			it->second->deleteDeviceObjects();
	}
	
	void Scene::invalidateDeviceObjects(void)	
	{ 
		for (MeshCache::Container::iterator it = mMeshCache.getObjects().begin(); it != mMeshCache.getObjects().end();	++it)
			it->second->invalidateDeviceObjects();

		for (MeshCache::Container::iterator it = mShadowMeshCache.getObjects().begin(); it != mShadowMeshCache.getObjects().end();	++it)
			it->second->invalidateDeviceObjects();
	}
	
	void Scene::restoreDeviceObjects(void)			
	{	
		for (MeshCache::Container::iterator it = mMeshCache.getObjects().begin(); it != mMeshCache.getObjects().end();	++it)
			it->second->restoreDeviceObjects();

		for (MeshCache::Container::iterator it = mShadowMeshCache.getObjects().begin(); it != mShadowMeshCache.getObjects().end();	++it)
			it->second->restoreDeviceObjects();
	}

	void Scene::startOfFrame(void)							
	{ 
		for (MeshCache::Container::iterator it = mMeshCache.getObjects().begin(); it != mMeshCache.getObjects().end();	++it)
			it->second->startOfFrame();

		for (MeshCache::Container::iterator it = mShadowMeshCache.getObjects().begin(); it != mShadowMeshCache.getObjects().end();	++it)
			it->second->startOfFrame();
	}

	bool Scene::renderShadowModels(const ShadowBufferParams& shadowBufferParams, int pass)
	{
		if (!loaded())
			return false;
			
		switch (shadowBufferParams.light().mType)
		{
			case eSpot:
			case eOmnidirectional:
				return renderShadowModelsSpotOmni(shadowBufferParams, pass);
		}
					
		return false;
	}
		
	bool Scene::renderShadowModelsSpotOmni(const ShadowBufferParams& shadowBufferParams, int pass)
	{
		D3D::setTransform(D3DTS_VIEW, shadowBufferParams.worldToView());
		D3D::setTransform(D3DTS_PROJECTION, shadowBufferParams.viewToProj());
				      		
		Cone lightCone;
		
		if (eSpot == shadowBufferParams.light().mType)
		{
			lightCone.set(shadowBufferParams.light().mPos, 
				shadowBufferParams.light().mDir, 
				shadowBufferParams.light().mSpotOuter * .5f);
		}

		Sphere lightSphere(
			shadowBufferParams.light().mPos, 
			shadowBufferParams.light().mRadius);

		int numRendered = 0;
				
		for (int i = 0; i < mInstances.size(); i++)
		{
			SceneInstance& sceneInstance = mInstances[i];

			RenderMesh* pMesh = sceneInstance.shadowMesh();
			Anim* pAnim = sceneInstance.anim();

			if (!pMesh)
				continue;
			
			const Matrix44& modelToWorld = mModelToWorld[i];

			Sphere boundingSphere(
				Vec4(sceneInstance.boundingSphere().origin(), 1.0f) * modelToWorld, 
				sceneInstance.boundingSphere().radius());
        
			if (!Intersection::sphereSphere(boundingSphere, lightSphere))
				continue;

			if (eSpot == shadowBufferParams.light().mType)
			{
				if (!Intersection::coneSphere(lightCone, boundingSphere))
					continue;

				if (OUTSIDE == Intersection::sphereFrustum(boundingSphere, shadowBufferParams.lightCamera().worldFrustum()))
					continue;
					
				pMesh->render(modelToWorld, pAnim, mCurTime, eSpotShadowPass);
			}
			else
			{
				pMesh->render(modelToWorld, pAnim, mCurTime, eOmniShadowPass);
			}
												
			numRendered++;
		}

		return numRendered > 0;
	}
			
	void Scene::loadEmitterObject(int objectIndex, const SceneData::Object& obj, const PropertySet& props)
	{	
		BigString emitterName(ReadBigString(props, "name", ""));
		BigString emitterFilename(emitterName + "." EMITTER_EXTENSION);
		
		Status("Scene::loadEmitterObject: Loading file \"%s\"\n", emitterFilename.c_str());
		
		PropertyFileLoader loader(emitterFilename);
		if (!loader.CanRead())
		{
			Error("Scene::loadEmitterObject: Can't open file \"%s\"\n", emitterFilename.c_str());
			return;
		}

		PropertyFile emitterProps(loader);
		
		Status("Emitter \"%s\" UDP Parse Warnings: %i Errors: %i\n",
			emitterName.c_str(),
			emitterProps.GetWarningCount(),
			emitterProps.GetErrorCount());
			
		if (emitterProps.GetWarningCount() + emitterProps.GetErrorCount())
		{
			Status("Error/warning messages:\n");
			gSystem.message(System::ERR, emitterProps.GetErrorsAndWarnings().c_str());
			
			Status("Scene::loadEmitterObject: UDP Parse failure\n");
			return;
		}
		
		ParticleSystem particleSystem(emitterProps);
		mpParticleManager->DefineSystem(emitterName, particleSystem);
		
		ParticleSystem* pSystem = mpParticleManager->CreateInstance(emitterName);
		
		mEmitters.push_back(SceneEmitterInstance(emitterName.c_str(), objectIndex, pSystem));
	}
	
	void Scene::loadMeshObject(int objectIndex, const SceneData::Object& obj, const PropertySet& props)
	{
		BigString meshName = ReadBigString(props, "mesh", "");
		BigString shadowName = ReadBigString(props, "shadowmesh", "");
		BigString animName = ReadBigString(props, "anim", "");
		
		if (meshName.empty())
		{
			Error("loadMeshObject: Can't find mesh name, or name is empty, for object \"%s\"\n", obj.mName.c_str());
			return;
		}
		
		if (shadowName.empty())
			shadowName = meshName + "_shadow";
			
		if (animName.empty())
			animName = meshName;
																							
		BigString meshFilename((meshName + "." + BigString(MESH_EXTENSION)).tolower());
		BigString shadowMeshFilename((shadowName + "." + BigString(MESH_EXTENSION)).tolower());
		BigString rawMeshFilename((meshName + "." + BigString(RAW_MESH_EXTENSION)).tolower());
		BigString rawShadowMeshFilename((shadowName + "." + BigString(RAW_MESH_EXTENSION)).tolower());
		BigString animFilename((animName + "." + BigString(ANIM_EXTENSION)).tolower());
						
		RenderMesh* pMesh = NULL;			
		RenderMesh* pShadowMesh = NULL;
		RenderMesh** ppMesh = mMeshCache.find(meshFilename.c_str());
		RenderMesh** ppShadowMesh = mShadowMeshCache.find(shadowMeshFilename.c_str());
										
		if (!ppMesh)
		{
			Status("Scene::loadMeshObject: Mesh cache miss: \"%s\"\n", meshFilename.c_str());
			
			pMesh = loadMesh(meshFilename);
			pShadowMesh = loadMesh(shadowMeshFilename);
			
			if (!pMesh)
				pMesh = loadRawMesh(rawMeshFilename);

			if (!pShadowMesh)
				pShadowMesh = loadRawMesh(rawShadowMeshFilename);
			
			if (pShadowMesh)
			{
				if (!pShadowMesh->shadowGeom())
				{
					delete pShadowMesh;
					pShadowMesh = NULL;
					Status("Scene::loadMeshObject: File \"%s\" is not a shadow mesh!\n",
						shadowMeshFilename.c_str());
				}
			}
			
			if (pMesh)
			{
				mMeshCache.insert(meshFilename.c_str(), pMesh);
				if (pShadowMesh)
					mShadowMeshCache.insert(shadowMeshFilename.c_str(), pShadowMesh);
			}
		}
		else
		{
			Status("Scene::loadMeshObject: Mesh cache hit: \"%s\"\n", meshFilename.c_str());
			
			pMesh = *ppMesh;
			if (ppShadowMesh)
				pShadowMesh = *ppShadowMesh;
		}
		
		Anim* pAnim = NULL;
		if (pMesh)
		{
			Anim** ppAnim = mAnimCache.find(animFilename.c_str());

			if (!ppAnim)
			{
				Status("Scene::loadMeshObject: Anim cache miss: \"%s\"\n", animFilename.c_str());
				
				pAnim = loadAnim(animFilename);
				if (pAnim)
				{
					pAnim->setLoop(false);

					mAnimCache.insert(animFilename.c_str(), pAnim);
				}
			}
			else 
			{
				Status("Scene::loadMeshObject: Anim cache hit: \"%s\"\n", animFilename.c_str());
				
				pAnim = *ppAnim;
			}
		}

		Sphere boundingSphere(Vec3(0), 0);
		if (pMesh)
			boundingSphere = pMesh->calcBoundingSphere(pAnim);
		
		mInstances.push_back(SceneInstance(pAnim, pMesh, pShadowMesh, boundingSphere, objectIndex));
	}
	
	void Scene::loadExposureObject(int objectIndex, const SceneData::Object& obj, const PropertySet& props)
	{
		mExposureObjectIndex = objectIndex;
		
		Status("Scene::loadExposureObject: Found at obj index %i\n", objectIndex);
	}

	// true on failure
	bool Scene::loadObjects(void) 
	{
		const SceneData::Frame& frame = mSceneData.frame(0);

		Verify(frame.numObjects() < SceneMaxObjects);
					
		for (int i = 0; i < frame.numObjects(); i++)
		{
			const SceneData::Object& obj = frame.object(i);
									
			const PropertySet& props = mObjectProps[i].GetPropertySet("");
											
			BigString typeName(ReadBigString(props, "type", "").tolower());
									
			if (typeName == "mesh")
				loadMeshObject(i, obj, props);
			else if (typeName == "emitter")
				loadEmitterObject(i, obj, props);
			else if (typeName == "exposure")
				loadExposureObject(i, obj, props);
			else
			{
				Error("Scene::loadObjects: Unrecognized object type: Name: \"%s\" Type: \"%s\"\n", 
					obj.mName.c_str(), 
					typeName);
					
				return true;
			}
		}
		
		return false;
	}

	RenderMesh* Scene::loadMesh(const char* pFilename)
	{
		RenderMesh* pMesh = NULL;
		
		Status("Scene::loadMesh: Filename \"%s\"\n", pFilename);

		std::auto_ptr<Stream> pStream = std::auto_ptr<Stream>(gFileSystem.createStream(pFilename));

		if ((!pStream.get()) || (pStream->errorStatus()))
			Status("Scene::loadMesh: Load failed\n");
		else
		{
			pMesh = new RenderMesh();

			//if (pMesh->load(stream, shadow))
			if (pMesh->load(*pStream))
			{
				delete pMesh;
				pMesh = NULL;
			}
			
			Status("Scene::loadMesh: Load succeeded\n");
		}
								
		return pMesh;
	}
	
	RenderMesh* Scene::loadRawMesh(const char* pFilename)
	{
		RenderMesh* pMesh = NULL;
		
		Status("Scene::loadRawMesh: Filename \"%s\"\n", pFilename);

		std::auto_ptr<Stream> pStream = std::auto_ptr<Stream>(gFileSystem.createStream(pFilename));

		if ((!pStream.get()) || (pStream->errorStatus()))
			Status("Scene::loadRawMesh: Load failed\n");
		else
		{
			Unigeom::Geom geom;
	
			Status("Scene::loadRawMesh: Reading file: \"%s\".\n", pFilename);
	
			if (geom.read(*pStream))
			{
				Status("Scene::loadRawMesh: Load failed!\n");
				return NULL;
			}
			else
			{
				LogFile log;
				log.setTraceEcho(true);
				
				UGXInstancer instancer(geom, log);
				
				pMesh = new RenderMesh(instancer.geom());
								
				Status("Scene::loadMesh: Load succeeded\n");
			}
		}
								
		return pMesh;
	}

	Anim* Scene::loadAnim(const char* pFilename)
	{
		Anim* pAnim = NULL;

		Status("Scene::loadAnim: Filename \"%s\"\n", pFilename);
		
		std::auto_ptr<Stream> pStream = std::auto_ptr<Stream>(gFileSystem.createStream(pFilename));

		if ((!pStream.get()) || (pStream->errorStatus()))
			Status("Scene::loadAnim: Load failed\n");
		else
		{
			pAnim = new Anim();

			if (pAnim->load(*pStream))
			{
				delete pAnim;
				pAnim = NULL;
			}
			
			Status("Scene::loadAnim: Load succeeded\n");
		}
								
		return pAnim;
	}
	
	void Scene::findDuration(void) 
	{
		mDuration = mSceneData.duration();
		for (int i = 0; i < mInstances.size(); i++)
			if (mInstances[i].anim())
				mDuration = Math::Max(mDuration, mInstances[i].anim()->duration());
		
		Status("Scene::findDuration: %f secs\n", mDuration);
	}

	void Scene::findSceneInterpFrames(InterpFrames& interpFrames)
	{
		if (mCurTime >= mSceneData.duration())
		{
			interpFrames.frame[0] = interpFrames.frame[1] = mSceneData.numFrames() - 1;
			interpFrames.t = 0.0f;
			return;
		}
		
		for (int i = 0; i < mSceneData.numFrames(); i++)
			if (mCurTime < mSceneData.frameTime(i))
				break;

		interpFrames.frame[0] = Math::Max(0, i - 1);
		interpFrames.frame[1] = i;

    DebugRange(interpFrames.frame[0], mSceneData.numFrames());
		DebugRange(interpFrames.frame[1], mSceneData.numFrames());

		const float frame0Time = mSceneData.frameTime(interpFrames.frame[0]);
		const float frame1Time = mSceneData.frameTime(interpFrames.frame[1]);
		const float duration = frame1Time - frame0Time;
		interpFrames.t = duration ? ((mCurTime - frame0Time) / duration) : 0.0f;
		DebugRangeIncl(interpFrames.t, 0.0f, 1.0f);
	}
	
	//FIXME: YYUCK!! Exporter should apply similarity xform!
	Matrix44 Scene::interpModelToWorld(
		const SceneData::Base& obj0, 
		const SceneData::Base& obj1, 
		float t,
		const Matrix44& postXForm)
	{
		Matrix44 modelToMaxWorld(Quat::slerp(obj0.mOrient, obj1.mOrient, t));
		
		modelToMaxWorld.setTranslate(Vec<3>::lerp(obj0.mPos, obj1.mPos, t));

		return modelToMaxWorld * postXForm;
	}

	void Scene::setupLights(void)
	{
		const SceneData::Frame& frame0 = mSceneData.frame(0);
		
		for (int i = 0; i < frame0.numLights(); i++)
		{
			const SceneData::Light& light = frame0.light(i);
			
			if (!light.mMask.empty())
			{
				mLightMasks[i] = RenderEngine::textureManager().create(light.mMask);
				if ((mLightMasks[i]) && (!mLightMasks[i]->get()))
					mLightMasks[i] = NULL;
				else
					Status("Scene::setupLights: Loaded light mask \"%s\"\n", light.mMask.c_str());
			}
		}
	}
	
	void Scene::updateLights(void)
	{
		if (!mSceneData.numFrames())
			return;

		if (!mLightTweakers.lightAnimationEnabled())
			return;

		InterpFrames interpFrames;
		findSceneInterpFrames(interpFrames);

		const SceneData::Frame& frame0 = mSceneData.frame(interpFrames.frame[0]);
		const SceneData::Frame& frame1 = mSceneData.frame(interpFrames.frame[1]);
		const float t = interpFrames.t;
		
		for (int i = 0; i < frame0.numLights(); i++)
		{
			const SceneData::Light& obj0 = frame0.light(i);
			const SceneData::Light& obj1 = frame1.light(i);

      Light params;
			
			Assert(obj0.mType == obj1.mType);

			switch (obj0.mType)
			{
				case SceneData::eSpotLightType: params.mType = eSpot; break;
				case SceneData::eOmniLightType: params.mType = eOmnidirectional; break;
				case SceneData::eDirLightType: params.mType = eDirectional; break;
			}
			
			const float intensity = Math::Lerp(obj0.mIntensity, obj1.mIntensity, t);
			
			const Vec3 color(Vec3::lerp(obj0.mColor, obj1.mColor, t));
			if (obj0.mDiffuse)
			{
				params.mColor0[0] = color[0] * intensity;
				params.mColor0[1] = color[1] * intensity;
				params.mColor0[2] = color[2] * intensity;
			}

			if (obj0.mSpecular)
			{
				params.mColor1[0] = color[0] * intensity;
				params.mColor1[1] = color[1] * intensity;
				params.mColor1[2] = color[2] * intensity;
			}

			Matrix44 m(interpModelToWorld(obj0, obj1, t));

			params.mPos = m.getTranslate();
			params.mDir = -m.getRow(2).normalized();
			params.mUp = -m.getRow(1).normalized();
			params.mRight = -m.getRow(0).normalized();
			params.mRadius = Math::Lerp(obj0.mRadius, obj1.mRadius, t);
			params.mFarAttenStart = Math::Lerp(obj0.mFarAttenStart, obj1.mFarAttenStart, t);
			params.mDecayDist = Math::Lerp(obj0.mDecayDist, obj1.mDecayDist, t);
      			
			params.mShadows = obj0.mShadows;
			params.mSpotInner = Math::Lerp(obj0.mSpotInner, obj1.mSpotInner, t);
			params.mSpotOuter = Math::Lerp(obj0.mSpotOuter, obj1.mSpotOuter, t);
			params.mFog = obj0.mFog;
			params.mFogShadows = obj0.mFogShadows;
			params.mFogDensity = obj0.mFogDensity;

			mLightTweakers.set(i, params);
    }
	}

	void Scene::renderLight(const Light& params)
	{
		RenderEngine::lightingEngine().drawLight(
			params, 
			SceneShadowBufferModelRenderer(*this));
  }
  
  void Scene::render(bool toneMap)
  {
		beginScene();
		renderFirst();
		renderSecond();
		light();
		final(toneMap);
		endScene();
	}
	
	bool Scene::setInstanceModelToWorld(const char* pName, const Matrix44& objToWorld)
	{
		if (!loaded())
			return true;
			
		const SceneData::Frame& frame = mSceneData.frame(0);
		
		for (int i = 0; i < mInstances.size(); i++)
		{
			const SceneData::Object& obj = frame.object(mInstances[i].sceneDataObjectIndex());
			
			if (obj.mName == pName)
			{
				mInstances[i].setModelToWorld(objToWorld);
				return false;
			}
		}
		return true;
	}
  
  void* Scene::operator new  (size_t size)
  {
		return _aligned_malloc(size, _alignof(Scene));
  }
  
	void* Scene::operator new[] (size_t size)
	{
		return _aligned_malloc(size, _alignof(Scene));
	}
	
	void Scene::operator delete  (void* p)
	{
		if (p)
			_aligned_free(p);
	}
	
	void Scene::operator delete[] (void* p)
	{
		if (p)
			_aligned_free(p);
	}
};


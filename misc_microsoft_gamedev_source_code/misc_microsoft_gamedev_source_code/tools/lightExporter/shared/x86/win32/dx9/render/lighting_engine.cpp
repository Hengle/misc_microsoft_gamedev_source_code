//-----------------------------------------------------------------------------
// File: lighting_engine.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "lighting_engine.h"

#include "device.h"
#include "render_engine.h"
#include "render_helpers.h"

#include "common/geom/unigeom.h"

#include "x86/core/timer_x86.h"
#include "x86/win32/dx9/utils/tweaker_dx9.h"

#include "../../bin/shader_regs.inc"

namespace gr
{
	LightingEngine gLightingEngine;
		  
	LightingEngine::LightingEngine() :
		mViewTileComplexity(true),
		mViewLightBounds(false),
		mLightIndex(0),
		mViewTileLightIndex(0),
		mEffects(RenderEngine::effects()),
		mIndirectLighting(true),
		mpBodyBounds(NULL)
  {
		
  }
  
  LightingEngine::~LightingEngine()
  {
		deleteDeviceObjects();
  }

	void LightingEngine::clear(void)
	{

	}
		
	void LightingEngine::initTweakers(void)
	{
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("Light of interest index", &mViewTileLightIndex, 0, 255, 1, true));
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("Relative tile complexity", &mViewTileComplexity));
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("View light bounds", &mViewLightBounds));
		gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Irradiance volume pass", &mIndirectLighting));
	}

	void LightingEngine::oneTimeSceneInit(void)
	{
		initTweakers();

		mToneMapper.oneTimeSceneInit();
		mShadowBuffer.oneTimeSceneInit();
		mLightTilizer.oneTimeSceneInit();
		mVolumeFog.oneTimeSceneInit();
	}

	void LightingEngine::startOfFrame(float deltaT)
  {
		mShadowBuffer.startOfFrame(deltaT);
		mLightTilizer.startOfFrame();
		mVolumeFog.startOfFrame(deltaT);
  }

  void LightingEngine::initDeviceObjects(void)
  {
		mToneMapper.initDeviceObjects();
		mShadowBuffer.initDeviceObjects();
		mLightTilizer.initDeviceObjects();
		mVolumeFog.initDeviceObjects();
	}
  
  void LightingEngine::deleteDeviceObjects(void)
  {
		mToneMapper.deleteDeviceObjects();
		mShadowBuffer.deleteDeviceObjects();
		mLightTilizer.deleteDeviceObjects();
		mVolumeFog.deleteDeviceObjects();
  }

  void LightingEngine::invalidateDeviceObjects(void)
  {
		mToneMapper.invalidateDeviceObjects();
		mShadowBuffer.invalidateDeviceObjects();
		mLightTilizer.invalidateDeviceObjects();
		mVolumeFog.invalidateDeviceObjects();
  }
  
  void LightingEngine::restoreDeviceObjects(void)
  {
		mToneMapper.restoreDeviceObjects();
		mShadowBuffer.restoreDeviceObjects();
		mLightTilizer.restoreDeviceObjects();
		mVolumeFog.restoreDeviceObjects();
  }
  
  void LightingEngine::reloadEffects(void)
  {
  }
	
	void LightingEngine::beginScene(const RenderViewport& renderViewport)
	{
		mRenderViewport = renderViewport;
		
		mVolumeFog.beginScene();
		
		mLightTilizer.beginScene(mRenderViewport);
	}

	void LightingEngine::endScene(void)
	{
	}
	
  void LightingEngine::beginFirst(void)
  {
		RenderEngine::bufferManager().setRenderTarget(0, eDBuffer);
    RenderEngine::bufferManager().setRenderTarget(1, eRBuffer);
    RenderEngine::bufferManager().setRenderTarget(2, eNBuffer);
    RenderEngine::bufferManager().setRenderTarget(3, eEnvBuffer);
    		    
    // assumes viewport is entire texture!

//		if (RenderEngine::lightProbeRender())
			D3D::clear(D3DCLEAR_TARGET);
    
    RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
  }
			  
  void LightingEngine::endFirst(void)
  {
    mRenderViewport.setToDevice(SET_ALL);
		D3D::setRenderTarget(1, NULL);
    D3D::setRenderTarget(2, NULL);
		D3D::setRenderTarget(3, NULL);
		    
    // should set viewport here!
  }
  
  void LightingEngine::beginSecond(void)
  {
		RenderEngine::bufferManager().setRenderTarget(0, eABuffer);
    RenderEngine::bufferManager().setRenderTarget(1, eSBuffer);
    RenderEngine::bufferManager().setRenderTarget(2, eEmissiveBuffer);
    D3D::setRenderTarget(3, NULL);
        
    // assumes viewport is entire texture!

//		if (RenderEngine::lightProbeRender())
		  D3D::clear(D3DCLEAR_TARGET);
	    
    RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
  }
			  
  void LightingEngine::endSecond(void)
  {
    mRenderViewport.setToDevice(SET_ALL);
		D3D::setRenderTarget(1, NULL);
    D3D::setRenderTarget(2, NULL);
		D3D::setRenderTarget(3, NULL);
		    
    // should set viewport here!
  }

	void LightingEngine::beginSSQPasses(void)
	{
		D3D::disableCulling();
    D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
		D3D::clearTextures();
		
		D3D::setVertexShader(NULL);
		D3D::setPixelShader(NULL);
	}

	void LightingEngine::endSSQPasses(void)
	{
		mRenderViewport.setToDevice(SET_RENDERTARGET);
		D3D::setRenderTarget(1, NULL);
		D3D::setRenderTarget(2, NULL);
		D3D::setRenderTarget(3, NULL);

    D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    D3D::setRenderState(D3DRS_ZWRITEENABLE, TRUE);
    
    D3D::enableCulling();

		D3D::setVertexShader(NULL);
		D3D::setPixelShader(NULL);
		RenderEngine::vertexDeclManager().setToDevice(eInvalidDeclaration);
  
    D3D::clearTextures();

		for (int i = 0; i < D3D::NumSamplers; i++)
    {
      D3D::setWrapAddressing(i);
      D3D::setLinearFiltering(i);
			D3D::setSamplerState(i, D3DSAMP_SRGBTEXTURE, FALSE);
    }
	}

	void LightingEngine::setLightRenderStates(void)
	{
		D3D::disableCulling();

    D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
		D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
	}

  void LightingEngine::beginLight(const BodyBounds& bodyBounds)
  {
		mpBodyBounds = &bodyBounds;
		mLightTilizer.setBodyBounds(bodyBounds);
		
    RenderEngine::bufferManager().setRenderTarget(0, eDABuffer);
		RenderEngine::bufferManager().setRenderTarget(1, eSABuffer);
		RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
    
    D3D::clear(0L, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0L);

		setLightRenderStates();
				
		mLightTilizer.clearStats();
		
		mLightIndex = 0;
		
		//if ((!RenderEngine::lightProbeRender()) && (mIndirectLighting))
		if (mIndirectLighting)
		{
			RenderEngine::dynaCube().renderIndirectLight2();
		}
  }
	
	void LightingEngine::drawOmniSpotLight(const Light& params, ShadowBufferModelRenderer& shadowBufferModelRenderer)
	{
		mLightIndex++;

		Vec4 viewPos(params.mPos * mRenderViewport.camera().worldToView());
		Vec4 viewDir(Matrix44::transformNormal(params.mDir, mRenderViewport.camera().worldToView()));
		
		if (params.mRadius <= 1.0f)
			return;

		bool gatherStats = mViewTileComplexity;
		if ((!mViewTileComplexity) && (mLightIndex == mViewTileLightIndex + 1))
			gatherStats = true;
			
		if (mLightTilizer.setLight(params, *DebugNull(mpBodyBounds), gatherStats))
			return;
				
		if (mViewLightBounds)
			mLightPolyhedrons.push_back(mLightTilizer.getPolyhedron());

		bool hasShadows = false;
						
		ShadowBufferParams shadowBufferParams(params, mLightTilizer, *DebugNull(mpBodyBounds));
		
		if (((eSpot == params.mType) || (eOmnidirectional == params.mType)) && (params.mShadows))
		{
			hasShadows = mShadowBuffer.renderShadowModels(shadowBufferParams, shadowBufferModelRenderer);
		}
		else
		{
			// HACK HACK
			// So Read-Modify-Write works properly: flushes GPU backside/frontside caches (?)
			RenderEngine::bufferManager().setRenderTarget(0, eABuffer);
			RenderEngine::bufferManager().setRenderTarget(1, eNBuffer);
			RenderEngine::bufferManager().setRenderTarget(0, eDABuffer);
			RenderEngine::bufferManager().setRenderTarget(1, eSABuffer);
		}
		
		RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
						
		mVolumeFog.render(shadowBufferParams, hasShadows);

		D3D::clearTextures();								
				
		mEffects.setVector("gOmniViewPos", viewPos);
		mEffects.setVector("gOmniDiffuse", params.mColor0);
		mEffects.setVector("gOmniSpecular", params.mColor1);
		    		
		float farAttenStart = params.mFarAttenStart;
		if (farAttenStart >= 1.0f)
			farAttenStart = .999f;

		const float omniOOFalloffRange = 1.0f / (1.0f - farAttenStart);
				
		mEffects.setFloat("gOmniAttenAdd", 1.0f + farAttenStart * omniOOFalloffRange);
		mEffects.setFloat("gOmniAttenMul", -((1.0f / params.mRadius) * omniOOFalloffRange));

//x * -1/radius * (1/(e-s)) + 1 + s * (1/(e-s)
//1 + -x/radius * oor + s * orr

		float decayDist = params.mDecayDist;
		if (decayDist < .01f)
			decayDist = .01f;
		mEffects.setFloat("gOmniDecayDist", decayDist);
		
		if (params.mType == eSpot)
		{
			mEffects.setVector("gSpotViewDirection", viewDir); 
						
			float angleMul = 1.0f / ((cos(params.mSpotInner * .5f) - cos(params.mSpotOuter * .5f)));
			float angleAdd = 1 - cos(params.mSpotInner * .5f) * angleMul;
			
			mEffects.setVector("gSpotParams", Vec4(angleMul, angleAdd, 0.0f, 0)); 
		}
		
		if (params.mMask)
			mEffects.setTexture("gMaskTexture", params.mMask->get());
								
		if (params.mType == eSpot)
		{
			if (params.mMask)
			{
				Matrix44 worldToLight(Matrix44::I);
				worldToLight.setColumn(0, params.mRight);
				worldToLight.setColumn(1, params.mUp);
				worldToLight.setColumn(2, params.mDir);
				
				const float scale = Math::fCalcScaleFromFOV(params.mSpotOuter);
				
				const Matrix44 normLightToScaledLight(Matrix44::makeScale(Vec4(scale, scale, 1.0f, 1.0f)));
				
				mEffects.setMatrix("gViewToLight", D3D::getMatrix(eViewToWorld) * worldToLight * normLightToScaledLight);
				
				if (hasShadows)
					mEffects.beginTechnique("ShadowedSpotLightMasked");
				else
					mEffects.beginTechnique("SpotLightMasked");
			}
			else
			{
				if (hasShadows)
					mEffects.beginTechnique("ShadowedSpotLight");
				else
					mEffects.beginTechnique("SpotLight");
			}
		}
		else
		{
			if (params.mMask)
			{
				Matrix44 worldToLight(Matrix44::I);
				worldToLight.setColumn(0, params.mRight);
				worldToLight.setColumn(1, params.mUp);
				worldToLight.setColumn(2, params.mDir);
				
				mEffects.setMatrix("gViewToLight", D3D::getMatrix(eViewToWorld) * worldToLight);
				
				if (hasShadows)
					mEffects.beginTechnique("ShadowedOmniLightMasked");
				else
					mEffects.beginTechnique("OmniLightMasked");
			}
			else 
			{
				if (hasShadows)
					mEffects.beginTechnique("ShadowedOmniLight");
				else
					mEffects.beginTechnique("OmniLight");
			}
		}
		
		mLightTilizer.renderTiles();
										
		mEffects.endTechnique();
	}

	void LightingEngine::drawDirLight(const Light& params)
	{
		D3D::clearTextures();

		mEffects.beginTechnique("DirLight");

		Vec4 viewDir(Matrix44::transformNormal(params.mDir, mRenderViewport.camera().worldToView()));
		mEffects.setVector("gDirViewLightVec", -viewDir);
		mEffects.setVector("gDirDiffuse", params.mColor0);
		mEffects.setVector("gDirSpecular", params.mColor1);

		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_FALSE);

		RenderHelpers::renderSSQ(true);

		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

		mEffects.endTechnique();
	}
  
  void LightingEngine::drawLight(const Light& params, ShadowBufferModelRenderer& shadowBufferModelRenderer)
  {
    switch (params.mType)
		{
			case eDirectional:
				drawDirLight(params);
				break;
			case eOmnidirectional:
			case eSpot:
				drawOmniSpotLight(params, shadowBufferModelRenderer);
				break;
		}
	}
  
  void LightingEngine::endLight(void)
  {
		endSSQPasses();

		mpBodyBounds = NULL;		
  }

  void LightingEngine::finalAccumPass(const FinalLightPassParams& params)
  {
		mEffects.beginTechnique(mVolumeFog.frameHasVolumeFog() ? "FinalPassFog" : "FinalPass");
						
		RenderEngine::bufferManager().setRenderTarget(0, eFBuffer);
		RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
		//D3D::setRenderTarget(1, NULL);
		//D3D::setRenderTarget(2, NULL);
		//D3D::setRenderTarget(3, NULL);

		RenderHelpers::renderSSQ(true);
				
		mEffects.endTechnique();
  }
	
	void LightingEngine::beginFinal(void)
  {
		beginSSQPasses();
		
		finalAccumPass(FinalLightPassParams());
		
		endSSQPasses();
  }
  
  void LightingEngine::endFinal(float exposure, bool toneMap)
  {
		if (toneMap)
		{
			beginSSQPasses();
		
			mToneMapper.toneMap(exposure);
		
			endSSQPasses();
								
			//if (mViewTiler)
				mLightTilizer.displayTiles();

			if (mViewLightBounds)
				displayLightBounds();

			mShadowBuffer.displayShadowMap();
		}
	}

	void LightingEngine::displayLightBounds(void)
	{
		RenderEngine::sceneRenderViewport().setToDevice(SET_ALL);
		D3D::setTransform(D3DTS_WORLD, mRenderViewport.getMatrix(eViewToWorld));		

		D3D::setFVF(D3DFVF_XYZ);

		D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);		
		D3D::setRenderState(D3DRS_LIGHTING, FALSE);
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_FALSE);

		D3D::disableCulling();
		
		for (uint i = 0; i < mLightPolyhedrons.size(); i++)
		{
			const ConvexPolyhedron& p = mLightPolyhedrons[i];

			if (p.numTris())
			{
				D3D::drawIndexedPrimitiveUP(
					D3DPT_TRIANGLELIST,
					0,
					p.numVerts(),
					p.numTris(),
					&p.tri(0),
					D3DFMT_INDEX16,
					&p.vert(0),
					sizeof(Vec3));
			}
		}

		D3D::enableCulling();
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

		mLightPolyhedrons.erase(mLightPolyhedrons.begin(), mLightPolyhedrons.end());

		D3D::setTransform(D3DTS_WORLD, Matrix44::I);
	}
	  
} // namespace gr


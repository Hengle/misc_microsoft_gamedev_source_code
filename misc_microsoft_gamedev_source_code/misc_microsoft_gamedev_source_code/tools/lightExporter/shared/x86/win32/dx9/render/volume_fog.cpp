// File: volume_fog.cpp
#include "volume_fog.h"
#include "shadow_buffer.h"
#include "x86/win32/dx9/utils/tweaker_dx9.h"
#include "render_engine.h"
#include "render_helpers.h"
#include "noise/noise.h"

namespace gr
{
	VolumeFog::VolumeFog() :
		mNumShadowTracePasses(3),
		mpVolumeTex(NULL),
		mEffects(RenderEngine::effects()),
		mFrameDeltaT(0),
		mFogDensityPasses(0),
		mVolOrigin(0,126,0,1),
		mVolDim(850,250,850,1),
		mVolRot(0),
		mVolDimK(0),
		mClearedFogAccumBuffer(false),
		mEnabled(true),
		mFogDensity(2.0f)
	{
	
	}

	VolumeFog::~VolumeFog()
	{
		deleteDeviceObjects();
	}
		
	void VolumeFog::initTweakers(void)
	{
		gSharedTweaker.createEntry(eFog, new IntegralTweakerEntry("Enabled", &mEnabled));
		gSharedTweaker.createEntry(eFog, new IntegralTweakerEntry(" Base density", &mFogDensity, 0.001f, 10.0f, .01f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eFog, new IntegralTweakerEntry(" Shadow tracing passes", &mNumShadowTracePasses, 0, MAX_VOL_TRACING_PASSES, 1, true, 0, NULL, false));
		gSharedTweaker.createEntry(eFog, new IntegralTweakerEntry(" Density tracing passes", &mFogDensityPasses, 0, MAX_FOG_DENSITY_PASSES, 1, true, 0, NULL, false));
	}

	void VolumeFog::initDeviceObjects(void)
	{
		createVolumeTexture();
	}

	void VolumeFog::deleteDeviceObjects(void)
	{
		D3D::safeRelease(mpVolumeTex);
	}

	void VolumeFog::invalidateDeviceObjects(void)
	{
	}

	void VolumeFog::restoreDeviceObjects(void)
	{
	}

	void VolumeFog::oneTimeSceneInit(void)
	{
		initTweakers();
	}

	void VolumeFog::createVolumeTexture(void)
	{
		const int VolTextureWidth = 32;
		const int VolTextureHeight = 32;
		const int VolTextureDepth = 32;
						
		mpVolumeTex = D3D::createVolumeTexture(
			VolTextureWidth, VolTextureHeight, VolTextureDepth, 
			1, 0, D3DFMT_A8, D3DPOOL_MANAGED);
			
		D3DLOCKED_BOX box;
		mpVolumeTex->LockBox(0, &box, NULL, 0);
		
		float minf = Math::fNearlyInfinite;
		float maxf = -Math::fNearlyInfinite;
		
		for (int z = 0; z < VolTextureDepth; z++)
		{
			for (int y = 0; y < VolTextureHeight; y++)
			{
				uchar* pRowBits = reinterpret_cast<uchar*>(box.pBits) + y * box.RowPitch + z * box.SlicePitch;
				for (int x = 0; x < VolTextureWidth; x++)
				{
					uchar* pBits = pRowBits + x;
					
					bool border = ((z == 0) || (z == VolTextureDepth - 1)) || 
						((y == 0) || (y == VolTextureHeight - 1)) ||
						((x == 0) || (x == VolTextureWidth - 1));
					
					if (border)
						*pBits = 0;
					else
					{
						float s = 1.0f;
						float f = noise3_rep(
							s*x/float(VolTextureWidth-1), 
							s*y/float(VolTextureHeight-1), 
							s*z/float(VolTextureDepth-1), 
							s, s, s);
						f=fabs(f);
						minf = Math::Min(minf, f);
						maxf = Math::Max(maxf, f);
						*pBits = Math::Clamp((pow(f / .2185f, 3.0f) + Math::fRand(-.25f, .25f)), 0.0f, 1.0f) * 255;
					}
				}
			}
		}
		
		mpVolumeTex->UnlockBox(0);
	}
	
	void VolumeFog::beginScene(void)
	{
		mClearedFogAccumBuffer = false;
		
	}

	void VolumeFog::startOfFrame(float deltaT)
	{
		mEffects.setTexture("gFogVolume", mpVolumeTex);
		
		mFrameDeltaT = deltaT;

#if 0																
		mVolDimK = Math::Min<float>(1024, mVolDimK + deltaT);
		//mVolDim.x = Math::Lerp(200, 400, sin(mVolDimK * Math::fDegToRad(5.0f)) * .5 + .5);
		//mVolDim.y = Math::Lerp(200, 400, cos(mVolDimK * Math::fDegToRad(4.0f)) * .5 + .5);
		//mVolDim.z = Math::Lerp(200, 400, sin(mVolDimK * Math::fDegToRad(1.3f)) * .5 + .5);
		mVolRot.x += Math::fDegToRad(.5f) * mFrameDeltaT;
		mVolRot.y += Math::fDegToRad(1.3f) * mFrameDeltaT;
		mVolRot.z += Math::fDegToRad(1.0f) * mFrameDeltaT;
		mVolRot.x = Math::fPosMod(mVolRot.x, Math::fTwoPi);
		mVolRot.y = Math::fPosMod(mVolRot.y, Math::fTwoPi);
		mVolRot.z = Math::fPosMod(mVolRot.z, Math::fTwoPi);
		mVolOrigin.x += 19.0f * mFrameDeltaT;
		mVolOrigin.y -= 25.0f * mFrameDeltaT;
		mVolOrigin.z += 7.0f * mFrameDeltaT;
#endif
		
		if (mVolOrigin.y < -4096.0f)
		{
			mVolOrigin.x = 0.0f;
			mVolOrigin.y = 0.0f;
			mVolOrigin.z = 0.0f;
		}
	}

	void VolumeFog::renderInit(void)
	{
		D3D::clearTextures();

		D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
				
		D3D::setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);

		D3D::setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		D3D::setRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
		D3D::setRenderState(D3DRS_ALPHAREF, 0);
		
		D3D::setRenderTarget(1, NULL);
		
		if (!mClearedFogAccumBuffer)
		{
			mClearedFogAccumBuffer = true;

			RenderEngine::bufferManager().setRenderTarget(0, eFogAccumBuffer);			
			RenderEngine::bufferManager().setDepthStencilSurface(eSDBuffer);
			
			RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
					
			D3D::clear(D3DCLEAR_TARGET);	
		}
	}
		
	void VolumeFog::renderDeinit(void)
	{
		D3D::setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		
		D3D::disableCulling();
    D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
		D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
		D3D::clearTextures();
		
		RenderEngine::bufferManager().setRenderTarget(0, eDABuffer);
		RenderEngine::bufferManager().setDepthStencilSurface(eSDBuffer);
		RenderEngine::bufferManager().setRenderTarget(1, eSABuffer);
						
		RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
	}
			
	void VolumeFog::calcInshadowFractTrace(ShadowBufferParams& shadowBufferParams)
	{
		if (0 == mNumShadowTracePasses)
			return;
			
		D3D::setRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		D3D::setRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		D3D::setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		D3D::setRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		
		RenderEngine::bufferManager().setRenderTarget(0, ePLBuffer);	
		D3D::setRenderTarget(1, NULL);

		D3D::setDepthStencilSurface(RenderEngine::bufferManager().getSurface(eSDBuffer));
				
		RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
		
//		D3D::setFVF(D3DFVF_XYZ);
						
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
						
		mEffects.setVector("gTraceFract", Vec4(1.0f / (mNumShadowTracePasses * 16.0f)));
		
		for (int pass = 0; pass < mNumShadowTracePasses; pass++)
		{
			if (1 == pass)
				D3D::setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				
			mEffects.setFloat("gTraceStepSkip", pass * 16.0f);
			
			mEffects.beginTechnique("RenderTrace");
		
			shadowBufferParams.tilizer().renderTiles();
								
			mEffects.endTechnique();
		}
		
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		
		D3D::setRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		D3D::setRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		D3D::setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		D3D::setRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	}

	void VolumeFog::renderVolume(ShadowBufferParams& shadowBufferParams)
	{
		ConvexPolyhedron volume(shadowBufferParams.tilizer().getRawPolyhedron());
		
		Plane nearPlane(RenderEngine::sceneRenderViewport().camera().viewFrustum().plane(Frustum::NEAR_PLANE));
		nearPlane.d += 1.0f;
		ConvexPolyhedron::VertVector cap;
		volume.generateCap(cap, nearPlane);		
										
		static float weldEps = .000000125f;
		volume.weld(weldEps);
		
		RenderEngine::bufferManager().setRenderTarget(0, eFogFrontBuffer);	
		D3D::setDepthStencilSurface(RenderEngine::bufferManager().getSurface(eSDBuffer));
		
		RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
								
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
		D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
		
		if (cap.size())
			D3D::clear(D3DCLEAR_TARGET | D3DCLEAR_STENCIL);
		else
			D3D::clear(D3DCLEAR_STENCIL);
		
		D3D::setRenderState(D3DRS_STENCILENABLE, TRUE);
		D3D::setRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		D3D::setRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
		D3D::setRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCR);
		D3D::setRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		D3D::setRenderState(D3DRS_STENCILMASK, 0xFFFFFFFF);
		D3D::setRenderState(D3DRS_STENCILWRITEMASK, 0xFFFFFFFF);		
		D3D::setRenderState(D3DRS_STENCILREF, 0);
																
		D3D::enableCullingCCW();
		
		D3D::setFVF(D3DFVF_XYZ);
		
		mEffects.setMatrix("gRenderVolumeViewToProj", D3D::getMatrix(eViewToProj));
		
		mEffects.beginTechnique("RenderVolume");
						
		if (volume.numTris())
		{
			D3D::drawIndexedPrimitiveUP(
				D3DPT_TRIANGLELIST,
				0,
				volume.numVerts(),
				volume.numTris(),
				&volume.tri(0),
				D3DFMT_INDEX16,
				&volume.vert(0),
				sizeof(Vec3));
		}
						
		mEffects.endTechnique();
						
		D3D::enableCullingCW();
		
		RenderEngine::bufferManager().setRenderTarget(0, eFogBackBuffer);	
		
		RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
				
		mEffects.beginTechnique("RenderVolume");
		
		if (volume.numTris())
		{
			D3D::drawIndexedPrimitiveUP(
				D3DPT_TRIANGLELIST,
				0,
				volume.numVerts(),
				volume.numTris(),
				&volume.tri(0),
				D3DFMT_INDEX16,
				&volume.vert(0),
				sizeof(Vec3));
		}
									
		mEffects.endTechnique();
		
		D3D::disableCulling();
						
		D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		
		D3D::setRenderState(D3DRS_STENCILENABLE, FALSE);
		D3D::setRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		D3D::setRenderState(D3DRS_STENCILREF, 0);
		D3D::setRenderState(D3DRS_STENCILWRITEMASK, 0xFFFFFFFF);		
	}

	void VolumeFog::volumeTrace(ShadowBufferParams& shadowBufferParams)
	{
		if (0 == mFogDensityPasses)
			return;
			
		D3D::setRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		D3D::setRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		D3D::setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		D3D::setRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		
		RenderEngine::bufferManager().setRenderTarget(0, eDensityBuffer);	
		D3D::setRenderTarget(1, NULL);
		
		D3D::clear(D3DCLEAR_TARGET);

		D3D::setDepthStencilSurface(RenderEngine::bufferManager().getSurface(eSDBuffer));
				
		RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
								
		Matrix44 worldToVolume(Matrix44::makeTranslate(Vec4::multiply(mVolOrigin, Vec4(-1, -1, -1, 1))));
		worldToVolume *= Matrix44::makeScale(Vec4(2.0f/mVolDim.x, 2.0f/mVolDim.y, 2.0f/mVolDim.z, 1.0f));
		worldToVolume *= Matrix44::makeTranslate(Vec4(1, 1, 1, 1));
		worldToVolume *= Matrix44::makeScale(Vec4(.5, .5, .5, 1));
		worldToVolume *= Matrix44::makeRotate(0, mVolRot.x);
		worldToVolume *= Matrix44::makeRotate(1, mVolRot.y);		
		worldToVolume *= Matrix44::makeRotate(2, mVolRot.z);		
		
		const Matrix44 viewToLight(D3D::getMatrix(eViewToWorld) * worldToVolume);
					
		mEffects.setVector("gViewToLightCol0", viewToLight.getColumn(0));
		mEffects.setVector("gViewToLightCol1", viewToLight.getColumn(1));
		mEffects.setVector("gViewToLightCol2", viewToLight.getColumn(2));
		mEffects.setVector("gViewToLightTran", Vec4(viewToLight.getTranslate().x, viewToLight.getTranslate().y, viewToLight.getTranslate().z, 0.0f));
		
		const int samplesPerPass = 27;
		
		mEffects.setVector("gTraceFract", Vec4(1.0f / float(mFogDensityPasses * samplesPerPass)));
		
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		
		for (int pass = 0; pass < mFogDensityPasses; pass++)
		{
			if (1 == pass)
				D3D::setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				
			mEffects.setFloat("gTraceStepSkip", pass * samplesPerPass);
							
			mEffects.beginTechnique("RenderVolumeTrace");
			
			shadowBufferParams.tilizer().renderTiles();
									
			mEffects.endTechnique();
		}
				
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		
		D3D::setRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		D3D::setRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		D3D::setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		D3D::setRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);										
	}

	void VolumeFog::calcFog(ShadowBufferParams& shadowBufferParams, bool hasShadows)
	{
		D3D::setRenderState(D3DRS_STENCILENABLE, TRUE);
		D3D::setRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		D3D::setRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
		D3D::setRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		D3D::setRenderState(D3DRS_STENCILFUNC, D3DCMP_LESSEQUAL);
		D3D::setRenderState(D3DRS_STENCILREF, 1);
		D3D::setRenderState(D3DRS_STENCILMASK, 0xFFFFFFFF);
		D3D::setRenderState(D3DRS_STENCILWRITEMASK, 0xFFFFFFFF);		
		
		RenderEngine::bufferManager().setRenderTarget(0, eFogAccumBuffer);	
		
		RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
		
		mEffects.setVector("gRenderFogColor", shadowBufferParams.light().mColor0 * mFogDensity * shadowBufferParams.light().mFogDensity);
		mEffects.setVector("gRenderFogLightPos", shadowBufferParams.light().mPos * D3D::getMatrix(eWorldToView));
		
		D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
		D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
				
		if ((mNumShadowTracePasses) && (hasShadows))
			mEffects.beginTechnique((mFogDensityPasses > 0) ? "RenderDenseShadowedFog" : "RenderShadowedFog");
		else
			mEffects.beginTechnique((mFogDensityPasses > 0) ? "RenderDenseFog" : "RenderFog");
												
		shadowBufferParams.tilizer().renderTiles();
			
		mEffects.endTechnique();
		
		D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		
		D3D::setRenderState(D3DRS_STENCILENABLE, FALSE);
		D3D::setRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		D3D::setRenderState(D3DRS_STENCILREF, 0);
	}

	void VolumeFog::render(ShadowBufferParams& shadowBufferParams, bool hasShadows)
	{
		if ((!mEnabled) || (!shadowBufferParams.light().mFog))
			return;
			
		if ((shadowBufferParams.light().mType != eSpot) || (!shadowBufferParams.light().mFogShadows))
			hasShadows = false;

		renderInit();

		renderVolume(shadowBufferParams);

		if (hasShadows)
			calcInshadowFractTrace(shadowBufferParams);
		
		volumeTrace(shadowBufferParams);
		
		calcFog(shadowBufferParams, hasShadows);

		renderDeinit();
	}
} // namespace gr

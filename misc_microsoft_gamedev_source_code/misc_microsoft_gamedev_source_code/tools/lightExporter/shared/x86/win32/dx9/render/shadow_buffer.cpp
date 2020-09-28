// File: shadow_buffer.cpp
#include "shadow_buffer.h"
#include "x86/win32/dx9/utils/tweaker_dx9.h"
#include "render_engine.h"
#include "render_helpers.h"
#include "noise/noise.h"

namespace gr
{
	namespace
	{
		const char* pShadowBufferFilters[] = { 
			"Bilinear PCF (1 pass, 4 samples)",
			"Bilinear depth jittered PCF (1 pass, 4 samples)",
			"4x bilinear depth jittered PCF (1 pass, 9 samples)",
			"Stochastic depth/position jittered PCF (16 samples per pass)",
			"Separable 4x4 lowpass jittered PCF (1 pass, 16 samples)",
			"Separable 5x5 lowpass jittered PCF (2 passes, 25 samples)"
		};
		const int NumShadowBufferFilters = sizeof(pShadowBufferFilters)/sizeof(pShadowBufferFilters[0]);
	} // anonymous namespace
			
	void ShadowBufferParams::initCamera(int pass)
	{
		if (mLightParams.mType == eOmnidirectional)
		{
			Matrix44 worldToView(Matrix44::I);
			worldToView.setTranslate(Vec4(-mLightParams.mPos, 1.0f));
			
			if (pass)
				//worldToView = worldToView * Matrix44::makeScale(Vec4(1,1,-1,1));
				worldToView = worldToView * Matrix44::makeScale(Vec4(-1,1,-1,1));
				
			mLightCamera.setWorldToView(worldToView);
			
			// Dummy projection, not really used
			const float NearPlane = 10.0f;
			const float FarPlane = 10000.0f;
			mLightCamera.setViewToProj(1.0f, NearPlane, FarPlane, Math::fDegToRad(90.0f));		
		}
		else if (mLightParams.mType == eSpot)
		{
			const float NearPlane = 10.0f;
			const float FarPlane = 10000.0f;
			
			const Vec3 bodyCenter(mBodyBounds.center());
						
			mLightCamera.setWorldToView(
				mLightParams.mPos, 
				Vec4(bodyCenter, 1.0f),
				Vec4(0,1,0,0));
				
			mLightCamera.setViewToProj(1.0f, NearPlane, FarPlane, Math::fDegToRad(170.0f));		
			
			const Matrix44 worldToProj(mLightCamera.worldToView() * mLightCamera.viewToProj());
		
			const Vec3 lightPos(mLightParams.mPos);
			const Vec3 toBodyDir((bodyCenter - lightPos).normalize());
			
			Vec3 minProj(Math::fNearlyInfinite);
			Vec3 maxProj(-Math::fNearlyInfinite);
						
			float minDot = Math::fNearlyInfinite;
			for (int i = 0; i < mBodyBounds.numNodes(); i++)
			{
				for (int j = 0; j < 8; j++)
				{
					const Vec4 cornerPoint(mBodyBounds[i].corner(j));
					
					Vec4 projCornerPoint(cornerPoint * worldToProj);
					if (projCornerPoint.w < 1.0f)
					{
						minDot = 0.0f;
						break;
					}
					
					projCornerPoint.project();
					minProj = Vec3::elementMin(minProj, projCornerPoint);
					maxProj = Vec3::elementMax(maxProj, projCornerPoint);
					
					const Vec3 toCornerDir((Vec3(cornerPoint) - lightPos).normalize());
					
					minDot = Math::Min(minDot, Math::ClampLow(toCornerDir * toBodyDir, 0.0f));
				}
				
				if (0.0f == minDot)
					break;
			}
		
			float aspect = Math::Clamp(
				Math::Max(fabs(maxProj[0]), fabs(minProj[0])) / Math::Max(fabs(maxProj[1]), fabs(minProj[1])),
				.1f,
				8.0f);
							
			float fov = acos(minDot) * 2.0f;
			
			if (fov > Math::fDegToRad(100.0f))
			{
				mLightCamera.setWorldToView(
					mLightParams.mPos, 
					mLightParams.mPos + mLightParams.mDir,
					Vec4(0,1,0,0));
				
				aspect = 1.0f;
				fov = mLightParams.mSpotOuter;
			}
									
			mLightCamera.setViewToProj(aspect, NearPlane, FarPlane, fov);		
		}
	}

	Matrix44 ShadowBufferParams::viewToLight(void) const
	{
		if (mLightParams.mType == eSpot)
		{
			return D3D::getMatrix(eViewToWorld) * 
				worldToView() * 
				viewToProj() * 
				Matrix44::makeScale(Vec4(.5f, -.5f, 1.0f, 1.0f));		// includes (.5,-.5) scale/yflip!
		}
		else
		{
			// this looks weird, but the shader code uses the 4th column (W) as Z-- a D3D proj. matrix will output viewspace Z as W
			return D3D::getMatrix(eViewToWorld) * 
				worldToView() *
				viewToProj();
		}
	}

	ShadowBuffer::ShadowBuffer() :
		mShadowBufferNumPasses(1),
		mShadowBufferDepthJitterL(-2.9f),
		mShadowBufferDepthJitterH(-3.96f),
		mShadowBufferPosJitterR(2.0f),
		mEffects(RenderEngine::effects()),
		mViewShadowBuffer(false),
		mViewVBuffer(false),
		mFilterType(eKernel4x4),//eBilinearJittered4),
		mFrameDeltaT(0.0f),
		mEnabled(true),
		mSecondDepth(true)
	{
		Utils::ClearObj(mpJitterTextures);
	}

	ShadowBuffer::~ShadowBuffer()
	{
		deleteDeviceObjects();
	}
  
	void ShadowBuffer::initTweakers(void)
	{
		
		gSharedTweaker.createEntry(eShadows, new IntegralTweakerEntry("Shadow buffering", &mEnabled));
		gSharedTweaker.createEntry(eShadows, new IntegralTweakerEntry("  Second depth shadow buffer", &mSecondDepth));
		gSharedTweaker.createEntry(eShadows, new IntegralTweakerEntry("  View shadow depth buffer", &mViewShadowBuffer));
		gSharedTweaker.createEntry(eShadows, new IntegralTweakerEntry("  View visibility buffer", &mViewVBuffer));

		StaticAssert(NumShadowBufferFilters == eNumFilters);
		gSharedTweaker.createEntry(eShadows, new IntegralTweakerEntry("  Shadow buffer filter type", &mFilterType, pShadowBufferFilters, NumShadowBufferFilters));

		gSharedTweaker.createEntry(eShadows, new IntegralTweakerEntry("  Shadow buffer depth jitter low", &mShadowBufferDepthJitterL, -10.0f, 10.0f, .01f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eShadows, new IntegralTweakerEntry("  Shadow buffer depth jitter high", &mShadowBufferDepthJitterH, -10.0f, 10.0f, .01f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eShadows, new IntegralTweakerEntry("  Shadow buffer stochastic filter pos jitter radius", &mShadowBufferPosJitterR, 0.0f, 20, .01f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eShadows, new IntegralTweakerEntry("  Shadow buffer stochastic filter passes", &mShadowBufferNumPasses, 1, MAX_JITTER_PASSES, 1, true, 0, NULL, false));
		
	}
  
	void ShadowBuffer::initDeviceObjects(void)
	{
		createJitterTextures();
	}

	void ShadowBuffer::deleteDeviceObjects(void)
	{
		for (int i = 0; i < NUM_JITTER_TEXTURES; i++)
			D3D::safeRelease(mpJitterTextures[i]);
	}

	void ShadowBuffer::invalidateDeviceObjects(void)
	{
	}

	void ShadowBuffer::restoreDeviceObjects(void)
	{
	}

	void ShadowBuffer::oneTimeSceneInit(void)
	{
		initTweakers();
	}

	void ShadowBuffer::startOfFrame(float deltaT)
	{
		mFrameDeltaT = deltaT;
		
		initShadowBufferJitterTables();
	}

	void ShadowBuffer::setJitterTextureTexel(const D3DLOCKED_RECT& rect, int x, int y, DWORD p)
	{
		DebugRange<int, int>(x, JITTER_TEXTURE_WIDTH);
		DebugRange<int, int>(y, JITTER_TEXTURE_HEIGHT);
		DWORD* pPixel = reinterpret_cast<DWORD*>(
			(reinterpret_cast<uchar*>(rect.pBits) + x * sizeof(DWORD) + y * rect.Pitch)
		);
		*pPixel = p;
	}
	
	void ShadowBuffer::createJitterTextures(void)
	{
		D3DLOCKED_RECT lRects[NUM_JITTER_TEXTURES];

		for (int i = 0; i < NUM_JITTER_TEXTURES; i++)
		{
			D3D::safeRelease(mpJitterTextures[i]);
			mpJitterTextures[i] = D3D::createTexture(JITTER_TEXTURE_WIDTH, JITTER_TEXTURE_HEIGHT, 1, 0);
			mpJitterTextures[i]->LockRect(0, &lRects[i], NULL, 0);
		}

		for (int pass = 0; pass < 2; pass++)
		{
			const int filterWidth = pass ? 4 : 5;
			const float radius = (filterWidth - 1.0f) * .5f;
			const float origin = (filterWidth - 1.0f) * .5f;
			
			for (int io = 0; io < 256; io++)
			{
				const float xOfs = 0.0f;
				const float yOfs = io * 1.0f/256.0f;

				float m[5][5];
				Utils::ClearObj(m);

				srand(1234);

				const int NUM_SAMPLES = 256;
				for (int i = 0; i < NUM_SAMPLES; i++)
				{
					const float xd = Math::fRand(0.0f, radius);
					const float yd = Math::fRand(0.0f, radius);
					
					const float a = Math::fRand(0.0f, Math::fTwoPi);
					const float x = cos(a) * xd;
					const float y = sin(a) * yd;

					int ix = (int)floor(x + origin + xOfs);
					int iy = (int)floor(y + origin + yOfs);
					Assert(ix >= 0 && ix < filterWidth);
					Assert(iy >= 0 && iy < filterWidth);

					m[iy][ix] += 1.0f / NUM_SAMPLES;
				}
	      
				float sum[5];
				Utils::ClearObj(sum);
				for (int i = 0; i < 5; i++)
					for (int j = 0; j < 5; j++)
						sum[i] += m[i][j];
				
				union
				{
					DWORD v;  
					uchar e[4]; // [0,3]=B G R A  Z Y X W
				} p;

				for (int k = 0; k < 4; k++)
				{
					int r = k;
					if (r < 3)
						r = 2 - r;
					p.e[DebugRange(r, 4)] = Math::Clamp(Math::FloatToIntTrunc(floor(sum[k] * 256.0f + .5f)), 0, 255);
				}

				setJitterTextureTexel(lRects[pass], io, 0, p.v);
			}
		}

		for (int i = 0; i < NUM_JITTER_TEXTURES; i++)
			mpJitterTextures[i]->UnlockRect(0);
	}

	void ShadowBuffer::initShadowBufferJitterTables(void)
	{
		srand(55296);
		for (int i = 0; i < MAX_JITTER_PASSES * 4; i++)
		{
			//const float L = -2.4f;
			//const float H = -4.4f;
			const float L = mShadowBufferDepthJitterL;
			const float H = mShadowBufferDepthJitterH;
			mJitterDepth[i] = Vec4(Math::fRand(L, H),	Math::fRand(L, H), Math::fRand(L, H),	Math::fRand(L, H));
		}

		for (int i = 0; i < MAX_JITTER_PASSES * 8; i++)
		{
			Vec4 r;
			for (int j = 0; j < 2; j++)
			{
				//const float D = 2.65f;
				const float D = mShadowBufferPosJitterR;
				float d = Math::fRand(0.0f, D);
				float a = Math::fRand(0.0f, Math::fTwoPi);
				float x = cos(a) * d / float(ShadowBufferWidth);
				float y = sin(a) * d / float(ShadowBufferHeight);
		
				if (j)
				{
					r[2] = y;
					r[3] = x;
				}
				else
				{
					r[0] = x;
					r[1] = y;
				}
				
				mDPJitterPos[i * 2 + j] = Vec4(x * .5, y, 0, 0);
			}
			mJitterPos[i] = r;
		}
	}

	void ShadowBuffer::beginShadowRender(int pass)
	{
		D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		D3D::setRenderState(D3DRS_ZWRITEENABLE, TRUE);
		D3D::disableCulling();
		
		if (mSecondDepth)
			D3D::enableCullingCW();
		
		// no need to clear stencil: D16
		D3D::clear(0L, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(255,255,255,255), 1.0f, 0L);
	}

	// FIXME: should be moved into the lighting engine
	void ShadowBuffer::endShadowRender(const ShadowBufferParams& params)
	{
		D3D::disableCulling();
    D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
		D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
		D3D::clearTextures();

		RenderEngine::bufferManager().setRenderTarget(0, eDABuffer);
		D3D::setDepthStencilSurface(RenderEngine::bufferManager().getSurface(eSDBuffer));
		RenderEngine::bufferManager().setRenderTarget(1, eSABuffer);

		RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);
	}
	
	void ShadowBuffer::beginVBufferRender(const ShadowBufferParams& params)
	{
		RenderEngine::bufferManager().setRenderTarget(0, eVBuffer);
		D3D::setRenderTarget(1, NULL);

		if (mViewVBuffer)
			D3D::clear(D3DCLEAR_TARGET, D3DCOLOR_ARGB(128,128,128,128));
		//else
// RG HACK HACK
// Wasteful!		
			D3D::clear(D3DCLEAR_TARGET, D3DCOLOR_ARGB(255,255,255,255));

		D3D::setDepthStencilSurface(RenderEngine::bufferManager().getSurface(eSDBuffer));

		RenderEngine::sceneRenderViewport().setToDevice(SET_VIEWPORT|SET_MATRICES);

		D3D::disableCulling();
    D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
		D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
		D3D::setRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		D3D::clearTextures();								
		
		const Matrix44 viewToLight(params.viewToLight());
					
		mEffects.setVector("gViewToLightCol0", viewToLight.getColumn(0));
		mEffects.setVector("gViewToLightCol1", viewToLight.getColumn(1));
		// Important: Outputs 3rd row (W), not 2nd (Z)!!
		mEffects.setVector("gViewToLightCol2", viewToLight.getColumn(3));
		mEffects.setVector("gViewToLightTran", Vec4(viewToLight.getTranslate().x, viewToLight.getTranslate().y, viewToLight.getTranslate().w, 0.0f));
	}

	void ShadowBuffer::endVBufferRender(void)
	{
		D3D::setRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	}

	void ShadowBuffer::fillVBufferSpotStochastic4x4(ShadowBufferParams& shadowBufferParams)
	{
		beginVBufferRender(shadowBufferParams);

		mEffects.setVector("gShadowBufferJitterScale", Vec4(1.0f / (mShadowBufferNumPasses * 16.0f)));

		for (int pass = 0; pass < mShadowBufferNumPasses; pass++)
		{
			if (1 == pass)
			{
				D3D::setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				D3D::setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
				D3D::setRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			}

			mEffects.setVectorArray("gJSc", &mJitterPos[pass * 8], 8);
			mEffects.setVectorArray("gJSr", &mJitterDepth[pass * 4], 4);
			
			mEffects.beginTechnique("ShadowBufferJittered4x4");
												
			shadowBufferParams.tilizer().renderShadowTiles();
		    
			mEffects.endTechnique();
		}
		
		endVBufferRender();
	}

	void ShadowBuffer::fillVBufferSpot4x4Kernel(ShadowBufferParams& shadowBufferParams)
	{
		beginVBufferRender(shadowBufferParams);
					    		
		mEffects.setVectorArray("gJSr2", mJitterDepth, 7);
						
		mEffects.beginTechnique("ShadowBufferJittered4x4Kernel");

		D3D::setTextureClampPoint(0, mpJitterTextures[1]);
							
		shadowBufferParams.tilizer().renderShadowTiles();
	    
		mEffects.endTechnique();
	
		endVBufferRender();
	}

	void ShadowBuffer::fillVBufferSpot5x5Kernel(ShadowBufferParams& shadowBufferParams)
	{
		beginVBufferRender(shadowBufferParams);
					    		
		mEffects.setVectorArray("gJSr2", mJitterDepth, 7);
								
		for (int pass = 0; pass < 2; pass++)
		{
			D3D::clearTextures();
			D3D::setTextureClampPoint(0, mpJitterTextures[0]);

			if (0 == pass)
			{
				mEffects.beginTechnique("ShadowBufferJittered5x5KernelP1");
			}
			else if (1 == pass)
			{
				D3D::setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				D3D::setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
				D3D::setRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
				
				mEffects.beginTechnique("ShadowBufferJittered5x5KernelP2");
			}
							
			shadowBufferParams.tilizer().renderShadowTiles();
	    
			mEffects.endTechnique();
		}

		endVBufferRender();
	}

	void ShadowBuffer::fillVBufferSpotBilinear(ShadowBufferParams& shadowBufferParams)
	{
		beginVBufferRender(shadowBufferParams);
						
		mEffects.beginTechnique("ShadowBufferBilinear");
				
		shadowBufferParams.tilizer().renderShadowTiles();
	  
		mEffects.endTechnique();

		endVBufferRender();
	}

	void ShadowBuffer::fillVBufferSpotBilinearJittered(ShadowBufferParams& shadowBufferParams)
	{
		beginVBufferRender(shadowBufferParams);
				
		mEffects.setVectorArray("gJSr", mJitterDepth, 4);
				
		mEffects.beginTechnique("ShadowBufferBilinearJittered");
				
		shadowBufferParams.tilizer().renderShadowTiles();
	  
		mEffects.endTechnique();

		endVBufferRender();
	}

	void ShadowBuffer::fillVBufferSpotBilinearJittered4(ShadowBufferParams& shadowBufferParams)
	{
		beginVBufferRender(shadowBufferParams);
				
		mEffects.setVectorArray("gJSr", mJitterDepth, 4);
				
		mEffects.beginTechnique("ShadowBufferBilinearJittered4");
				
		shadowBufferParams.tilizer().renderShadowTiles();
	  
		mEffects.endTechnique();

		endVBufferRender();
	}

	void ShadowBuffer::fillVBufferSpot(ShadowBufferParams& shadowBufferParams)
	{
		switch (mFilterType)
		{
			case eBilinear:
				fillVBufferSpotBilinear(shadowBufferParams); 
				break;
			case eBilinearJittered: 
				fillVBufferSpotBilinearJittered(shadowBufferParams); 
				break;
			case eBilinearJittered4: 
				fillVBufferSpotBilinearJittered4(shadowBufferParams); 
				break;
			case eStochastic4x4: 
				fillVBufferSpotStochastic4x4(shadowBufferParams); 
				break;
			case eKernel4x4: 
				fillVBufferSpot4x4Kernel(shadowBufferParams); 
				break;
			case eKernel5x5: 
				fillVBufferSpot5x5Kernel(shadowBufferParams); 
				break;
			default:
				Assert(false);
		}
	}
	
	void ShadowBuffer::fillVBufferOmni(ShadowBufferParams& shadowBufferParams)
	{
		beginVBufferRender(shadowBufferParams);

		mEffects.setVector("gShadowBufferJitterScale", Vec4(1.0f / (mShadowBufferNumPasses * 16.0f)));
		
		for (int pass = 0; pass < mShadowBufferNumPasses; pass++)
		{
			if (1 == pass)
			{
				D3D::setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				D3D::setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
				D3D::setRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			}
				
			mEffects.setVectorArray("gDPJSc", &mDPJitterPos[pass * 16], 16);
				
			mEffects.beginTechnique("ShadowBufferOmniBilinearJittered");
				
			shadowBufferParams.tilizer().renderShadowTiles();
	  
			mEffects.endTechnique();
		}

		endVBufferRender();
	}

	bool ShadowBuffer::renderShadowModels(
		ShadowBufferParams& shadowBufferParams,
		ShadowBufferModelRenderer& shadowBufferModelRenderer)
	{
		Assert((eSpot == shadowBufferParams.light().mType) || (eOmnidirectional == shadowBufferParams.light().mType));
		
		if (!mEnabled)
			return false;
								
		bool hasShadows;
		
		if (eSpot == shadowBufferParams.light().mType)
		{
			RenderEngine::bufferManager().setRenderTarget(0, eHBuffer);
			D3D::setRenderTarget(1, NULL);
			D3D::setDepthStencilSurface(RenderEngine::bufferManager().getSurface(eHDBuffer));
		
			beginShadowRender(0);
									
			hasShadows = shadowBufferModelRenderer.render(shadowBufferParams, 0);
			
			if (hasShadows)
				fillVBufferSpot(shadowBufferParams);
		}
		else
		{
			RenderEngine::bufferManager().setRenderTarget(0, eDPBuffer);
			D3D::setRenderTarget(1, NULL);
			D3D::setDepthStencilSurface(RenderEngine::bufferManager().getSurface(eDPDBuffer));
			
			if (mViewShadowBuffer)
				D3D::clear(0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255,255,255,255), 1.0f, 0L);						
			
			beginShadowRender(0);
						
			D3DVIEWPORT9 viewport;
			viewport.X = 0;
			viewport.Y = 0;
			viewport.Width = ShadowBufferWidth;
			viewport.Height = ShadowBufferHeight;
			viewport.MinZ = 0.0f;
			viewport.MaxZ = 1.0f;
			D3D::setViewport(viewport);
																				
			hasShadows = shadowBufferModelRenderer.render(shadowBufferParams, 0);

			viewport.X = ShadowBufferWidth;
			D3D::setViewport(viewport);
												
			shadowBufferParams.initCamera(1);
			
			hasShadows = hasShadows | shadowBufferModelRenderer.render(shadowBufferParams, 1);
								
			if (hasShadows)
			{
				shadowBufferParams.initCamera(0);
				fillVBufferOmni(shadowBufferParams);						
			}
		}
		
		endShadowRender(shadowBufferParams);	
						
		return hasShadows;
	}

	void ShadowBuffer::displayShadowMap(void)
	{
		if ((!mViewShadowBuffer) && (!mViewVBuffer))
			return;

		D3D::disableCulling();

		D3D::clearTextures();
				
		if (mViewShadowBuffer)
		{
			mEffects.beginTechnique("RenderShadowBuffer");
		}
		else
		{
			mEffects.beginTechnique("RenderVisibilityBuffer");
		}
				
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		
		//RenderHelpers::renderSSQ(false, .3f, .3f, RenderEngine::bufferManager().getBackbufferWidth() * .7f, 0);
		RenderHelpers::renderSSQ(false, 1.0f, 1.0f, 0.0f, 0);

		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

		mEffects.endTechnique();

		D3D::enableCulling();

		D3D::setVertexShader(NULL);
		D3D::setPixelShader(NULL);

		RenderEngine::vertexDeclManager().setToDevice(eInvalidDeclaration);
  
    D3D::clearTextures();
	}

} // namespace gr


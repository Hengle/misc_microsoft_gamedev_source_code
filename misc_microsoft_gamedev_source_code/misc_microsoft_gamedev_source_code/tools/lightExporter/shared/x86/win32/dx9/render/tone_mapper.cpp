// tone_mapper.cpp
#include "tone_mapper.h"

#include "device.h"
#include "render_engine.h"
#include "render_helpers.h"

#include "common/geom/unigeom.h"
#include "x86/core/timer_x86.h"
#include "x86/win32/dx9/utils/tweaker_dx9.h"
#include "x86/win32/dx9/render/hdr_screenshot.h"

namespace gr
{
	//const float gGlareBufferScale = 32.0f;
	const float gGlareBufferScale = 8.0f;

	static const char* pToneMappers[] = { 
		"Photographic tone mapper", 
		"Gamma correction", 
		"No gamma correction", 
		"Diffuse Accumulation",
		"Specular Accumulation",
		"Normals",
		"Reflection",
		"Albedo",
		"Front specular",
		"Side specular",
		"Specular power",
		"Emissive RGB (Mantissa)",
		"Emissive A (Exponent)",
		"Env RGB (Mantissa)",
		"Env A (Exponent)",
	};

	ToneMapper::ToneMapper() :
		mEffects(RenderEngine::effects()),
		mType(ePhotoToneMapper),
		//mType(eViewNormals),
		mDisplayGamma(2.0f),//2.1f),
		mExposure(0.0f),
		mRelExposure(0.0f),
		mGreyFStops(3.5f),
		mKneeLow(0.0f),
		mKneeHigh(5.0f),
		mGlareInten(1.0f),
		mpGlareMaskTexture(NULL),
		mStreakOfs(60.0f),
		mStreakExposure(-7.0f),//-9.0f),
		mStreakElements(6),
		mTweakerStreakIndex(0),
		mStreakOverallInten(1.0f),
		mBloomExposure(-2.0f),//-3.5f),
		mNumBloomPasses(1),
		mBloomSigma(3.5f),//2.0f),
		mBloomR(1.0f),
		mBloomG(1.0f),
		mBloomB(1.0f),
		mBrightMaskOfs(4.0f)
	{
		for (int i = 0; i < MaxStreakElements; i++)
		{
			mNumStreakPasses[i] = 3;
			mStreakFalloff[i] = .905f;
			mStreakInten[i] = 1.0f;
			mStreakR[i] = 1.1f;
			mStreakG[i] = 1.0f;
			mStreakB[i] = 1.0f;
			mStreakElementOfs[i] = 0.0f;
		}
	}

	ToneMapper::~ToneMapper()
	{
	}

	void ToneMapper::deleteDeviceObjects(void)
	{
		D3D::safeRelease(mpGlareMaskTexture);
	}

	void ToneMapper::invalidateDeviceObjects(void)
	{
	}

	void ToneMapper::restoreDeviceObjects(void)
	{
	}

	void ToneMapper::oneTimeSceneInit(void)
	{
		initTweakers();
	}

	void ToneMapper::initDeviceObjects(void)
	{
		createGlareMaskTexture();
	}

	void ToneMapper::initTweakers(void)
	{
		gSharedTweaker.createEntry(eToneMapper, new IntegralTweakerEntry("Type", &mType, pToneMappers, eNumToneMappers));
		gSharedTweaker.createEntry(eToneMapper, new IntegralTweakerEntry("  Display Gamma", &mDisplayGamma, 0.8f, 4.0f, .035f, false, 0, NULL, false));		
		gSharedTweaker.createEntry(eToneMapper, new IntegralTweakerEntry("  Relative Exposure", &mRelExposure, -10.0f, 10.0f, .075f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eToneMapper, new IntegralTweakerEntry("  Grey FStops", &mGreyFStops, 0.5f, 7.0f, .075f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eToneMapper, new IntegralTweakerEntry("  KneeLow", &mKneeLow, -10.0f, 5.0f, .075f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eToneMapper, new IntegralTweakerEntry("  KneeHigh", &mKneeHigh, -10.0f, 15.0f, .075f, false, 0, NULL, false));

		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("Bright mask offset", &mBrightMaskOfs, 0.0f, 16.0f, .05f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("Final Bloom/Streak Intensity", &mGlareInten, -16.0f, 16.0f, .02f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("  Streak exposure", &mStreakExposure, -15.0f, 0.0f, .1f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("  Streak offset (degrees)", &mStreakOfs, 0.0f, 360.0f, 1.0f, false, 0, NULL, true));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("  Streak overall intensity", &mStreakOverallInten, 0.0f, 2.0f, .01f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("  Streak elements", &mStreakElements, 0, MaxStreakElements, 1, true));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("  Streak element index", &mTweakerStreakIndex, 0, MaxStreakElements - 1, 1, true));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("    Streak element offset (degrees)", mStreakElementOfs, 0.0f, 360.0f, 1.0f, false, MaxStreakElements, &mTweakerStreakIndex, true));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("    Streak filter passes", mNumStreakPasses, 1, 8, 1, true, MaxStreakElements, &mTweakerStreakIndex));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("    Streak filter falloff", mStreakFalloff, .5f, 1.0f, .001f, false, MaxStreakElements, &mTweakerStreakIndex, false));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("    Streak filter inten", mStreakInten, .5f, 1.5f, .005f, false, MaxStreakElements, &mTweakerStreakIndex, false));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("    Streak R", mStreakR, -8.0f, 8.0f, .01f, false, MaxStreakElements, &mTweakerStreakIndex, false));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("    Streak G", mStreakG, -8.0f, 8.0f, .01f, false, MaxStreakElements, &mTweakerStreakIndex, false));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("    Streak B", mStreakB, -8.0f, 8.0f, .01f, false, MaxStreakElements, &mTweakerStreakIndex, false));

		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("Bloom filter passes", &mNumBloomPasses, 0, 8, 1, true));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("  Bloom exposure", &mBloomExposure, -15.0f, 0.0f, .1f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("  Bloom sigma", &mBloomSigma, 0.1f, 10.0f, .1f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("  Bloom R", &mBloomR, -8.0f, 8.0f, .01f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("  Bloom G", &mBloomG, -8.0f, 8.0f, .01f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eGlare, new IntegralTweakerEntry("  Bloom B", &mBloomB, -8.0f, 8.0f, .01f, false, 0, NULL, false));
	}

	void ToneMapper::createGlareMaskTexture(void)
	{
		const int width = 128;
		const int height = 128;

		mpGlareMaskTexture = D3D::createTexture(width, height, 1, 0, D3DFMT_A8);
		
		D3DLOCKED_RECT r;
		
		D3D::errCheck(mpGlareMaskTexture->LockRect(0, &r, NULL, 0));
		uchar* pBits = reinterpret_cast<uchar*>(r.pBits);
		
		const float b = 1.0f;

		for (int y = 0; y < width; y++)
		{
			float yf = 1.0f;
				
			if (y < b) 
				yf = y / b;
			else if (y >= height - b)
				yf = (height - 1.0f - y) / b;

			for (int x = 0; x < height; x++)
			{
			  float xf = 1.0f;
				
				if (x < b) 
					xf = x / b;
				else if (x >= height - b)
					xf = (height - 1.0f - x) / b;
								
				const float f = xf * yf;
				pBits[x + y * r.Pitch] = Math::FloatToIntTrunc(f * 255.0f + .5f);
			}
		}
		
		mpGlareMaskTexture->UnlockRect(0);
	}

	float ToneMapper::knee(float x, float f)
  {
    return log(x * f + 1) / f;
  }

	float ToneMapper::findF(float x, float y)
  {
    float fLow = 0.0f;
    float fHigh = 16.0f;
    
		for (int i = 0; i < 48; i++)
    {
      const float f2 = .5f * (fLow + fHigh);
      const float y2 = knee(x, f2);
      if (y2 < y)
        fHigh = f2;
      else
        fLow = f2;
    }

    return (fLow + fHigh) * .5f;
  }

	void ToneMapper::setToneMapParams(void)
	{
		const float greyFStops = mGreyFStops;
		const float m = pow(2.0f, 2.47393f + mExposure);
		const float kl = pow(2.0f, mKneeLow);
		const float f = findF(pow(2.0f, mKneeHigh) - kl, pow(2.0f, greyFStops) - kl);
		const float g = 1.0f / mDisplayGamma;
		const float oof = 1.0f / f;

		float r = pow(pow(2.0f, -greyFStops), g);
				
		mEffects.setFloat("gToneMapM", m);
		mEffects.setFloat("gToneMapKl", kl);
		mEffects.setFloat("gToneMapF", f);
		mEffects.setFloat("gToneMapG", g);
		mEffects.setFloat("gToneMapR", r);
		mEffects.setFloat("gToneMapOOF", oof);
		mEffects.setFloat("gToneMapGlareInten", mGlareInten);
	}
		
	void ToneMapper::photoToneMapDownsample(ETexture srcBuffer, ETexture dstBuffer)
	{
		mEffects.setVector("gResample2BrightMaskOfs", Vec4(mBrightMaskOfs));
		
#if HALFRES_DOWNSAMPLE
		static float O = .5f;
		
		mEffects.beginTechnique("Resample2Mask");
				
    mEffects.setFloat("gResample2HalfPixelX", O / RenderEngine::bufferManager().getBackbufferWidth());
		mEffects.setFloat("gResample2HalfPixelY", O / RenderEngine::bufferManager().getBackbufferHeight());
		
		const float m = pow(2.0f, 2.47393f + mExposure + relStops);
		mEffects.setFloat("gResample2PixelScale", m * .25f * 1.0f / gGlareBufferScale);

		D3D::setTextureClampLinear(1, mpGlareMaskTexture);
		
		RenderEngine::bufferManager().setRenderTarget(0, dstBuffer);

		D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(srcBuffer));

		RenderHelpers::renderSSQ();

    mEffects.endTechnique();

		D3D::clearTextures();
#else
		static float O = .5f;

		mEffects.beginTechnique("Resample2");
				
    mEffects.setFloat("gResample2HalfPixelX", O / RenderEngine::bufferManager().getBackbufferWidth());
		mEffects.setFloat("gResample2HalfPixelY", O / RenderEngine::bufferManager().getBackbufferHeight());
		
		mEffects.setFloat("gResample2PixelScale", .25f * 1.0f / gGlareBufferScale);
		
		RenderEngine::bufferManager().setRenderTarget(0, eG2Buffer);

		D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(srcBuffer));

		RenderHelpers::renderSSQ();

    mEffects.endTechnique();

		D3D::clearTextures();

		// -----

		mEffects.beginTechnique("Resample2Mask");
				
    mEffects.setFloat("gResample2HalfPixelX", O / (RenderEngine::bufferManager().getBackbufferWidth() * .5f));
		mEffects.setFloat("gResample2HalfPixelY", O / (RenderEngine::bufferManager().getBackbufferHeight() * .5f));
		mEffects.setFloat("gResample2PixelScale", .25f);

		D3D::setTextureClampLinear(1, mpGlareMaskTexture);
		
		RenderEngine::bufferManager().setRenderTarget(0, dstBuffer);

		D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eG2Buffer));

		RenderHelpers::renderSSQ();

    mEffects.endTechnique();

		D3D::clearTextures();
#endif		
	}
	
	void ToneMapper::photoToneMapLowPass(float relStops, ETexture dstBuffer)
	{
		int numPasses = mNumBloomPasses;
		float bloomSigma = mBloomSigma;
		
		const int pass = 0; 
		
		{
			float mul, ofs;

			static float k = 1, ko = 0;
			static float r = .5f, ro = 0.0f;
			mul = pow(2.0f, pass * k + ko);
			ofs = 0.0f;
						
			const int BLOOM_DIMENSION = 12;
			
			D3DXVECTOR2 offsets[BLOOM_DIMENSION];
			float coefficients[BLOOM_DIMENSION];

			mEffects.beginTechnique("BloomFilter");

			RenderEngine::bufferManager().setRenderTarget(0, eG1Buffer);
			//RenderEngine::bufferManager().setRenderTarget(0, dstBuffer);
			
			float t = 0;
			for(int i=0; i < BLOOM_DIMENSION; i++)
			{
				coefficients[i]  = (float) exp(-(i*i)/(2.0f*bloomSigma*bloomSigma));
				if (coefficients[i] < .000125f)
					coefficients[i] = 0.0f;

				t += coefficients[i];
								
				offsets[i].x = (ofs + (mul * i)) / D3D::getRenderTargetWidth();
				offsets[i].y = 0;
			}
			      			
			float m = 1.0f;
			if (!pass)
				m = pow(2.0f, 2.47393f + mExposure + relStops);
				
			for(int i=0; i < BLOOM_DIMENSION; i++)
			{
				coefficients[i] /= t;
				coefficients[i] *= .5f;
				coefficients[i] *= m;
				if (coefficients[i] < .000125f)
					coefficients[i] = 0.0f;
			}

			mEffects.get()->SetValue("gBloomFilterTapScale", coefficients, sizeof(float)*BLOOM_DIMENSION);
			mEffects.get()->SetValue("gBloomFilterTapOfs", offsets, sizeof(D3DXVECTOR2)*BLOOM_DIMENSION);

			mEffects.setVector("gBloomColor", Vec4(1));

			D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eG0Buffer));
	    
			RenderHelpers::renderSSQ();

			mEffects.endTechnique();

			//-----
#if 1

			mEffects.beginTechnique("BloomFilter");

			if (pass == numPasses - 1)
				RenderEngine::bufferManager().setRenderTarget(0, dstBuffer);
			else
				RenderEngine::bufferManager().setRenderTarget(0, eG0Buffer);
			
			for(int i=0; i < BLOOM_DIMENSION; i++)
			{
				offsets[i].y = (ofs + (mul * i)) / D3D::getRenderTargetHeight();
				offsets[i].x = 0;
			}

			mEffects.get()->SetValue("gBloomFilterTapScale", coefficients, sizeof(float)*BLOOM_DIMENSION);
			mEffects.get()->SetValue("gBloomFilterTapOfs", offsets, sizeof(D3DXVECTOR2)*BLOOM_DIMENSION);
			
			D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eG1Buffer));

			if (pass == numPasses - 1)
				mEffects.setVector("gBloomColor", Vec4(mBloomR, mBloomG, mBloomB, 1.0f));
			else
				mEffects.setVector("gBloomColor", Vec4(1));
	    
			RenderHelpers::renderSSQ();

			mEffects.endTechnique();
#endif
			D3D::clearTextures();
		}
	}
	
	void ToneMapper::photoToneMapAccum(
		ETexture srcBuffer,
		ETexture dstBuffer,
		bool clearDst,
		float r, float g, float b)
	{
		mEffects.beginTechnique("Accumulate");

		RenderEngine::bufferManager().setRenderTarget(0, dstBuffer);
		if (clearDst)
			D3D::clear(D3DCLEAR_TARGET);

		D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(srcBuffer));
		D3D::setTextureClampPoint(1, RenderEngine::bufferManager().getTexture(dstBuffer));

		mEffects.setVector("gAccumulateRGBA", Vec4(r, g, b, 1.0f));

		RenderHelpers::renderSSQ();

		mEffects.endTechnique();
		
		D3D::clearTextures();
	}

	ETexture ToneMapper::photoToneMapStreak(
		int numPasses,
		float dir,
		float falloff,
		float inten, float exposure,
		ETexture firstBuffer,
		ETexture buffer0,
		ETexture buffer1)
	{
		ETexture dstBuffer;
		
		for (int pass = 1; pass <= numPasses; pass++)
		{
			mEffects.beginTechnique("StreakFilter");

			if (pass & 1)
			{
				dstBuffer = buffer1;
				if (pass == 1)
					D3D::setTextureClampLinear(0, RenderEngine::bufferManager().getTexture(firstBuffer));
				else
					D3D::setTextureClampLinear(0, RenderEngine::bufferManager().getTexture(buffer0));
			}
			else
			{
				dstBuffer = buffer0;
				D3D::setTextureClampLinear(0, RenderEngine::bufferManager().getTexture(buffer1));
			}
			
			RenderEngine::bufferManager().setRenderTarget(0, dstBuffer);
						
			const int NumStreakTaps = 4;
			
			const float b = pow(NumStreakTaps, pass - 1);
			const float a = falloff;
									
			D3DXVECTOR2 offsets[NumStreakTaps];
			Vec3 coefficients[NumStreakTaps];
			
			Vec3 avColor[4] = 
			{
				Vec3(0.5f, 0.5f, 0.5f),
				Vec3(1.0f, 0.2f, 0.2f),
				Vec3(0.2f, 0.6f, 0.2f),
				Vec3(0.2f, 0.2f, 1.0f),
			};
			
			float ratio = float(numPasses - (pass - 1)) / numPasses;
			//float ratio = float(pass - 1) / numPasses;
			
			for(int s = 0; s < NumStreakTaps; s++)
			{
				float sampleInten = pow(a, b * s) * inten;
				if (1 == pass)
					sampleInten *= exposure;
					
				if (sampleInten < .000125f)
					sampleInten = 0.0f;
					
				coefficients[s] = Vec3::lerp(avColor[s] * 2.0f, Vec3(1, 1, 1), ratio) * sampleInten;
																
				static float tapOfs = 0.0f;//.5f;
				
				const float dist = b * s;
				offsets[s].x = (tapOfs + (sin(dir) * dist)) / D3D::getRenderTargetWidth();
				offsets[s].y = (tapOfs + (cos(dir) * dist)) / D3D::getRenderTargetHeight();
			}
			
			mEffects.get()->SetValue("gStreakFilterTapColor", coefficients, sizeof(coefficients));
			mEffects.get()->SetValue("gStreakFilterTapOfs", offsets, sizeof(offsets));
	    
			RenderHelpers::renderSSQ();

			mEffects.endTechnique();
		}

		D3D::clearTextures();

		return dstBuffer;
	}

#if 0	
	void ToneMapper::test(void)
	{
		for (int i = 0; i < 256; i++)
		{
			D3DLOCKED_RECT r;
		
			IDirect3DTexture9* pTex = RenderEngine::bufferManager().getTexture(eDefaultDiffuse);
			
			D3D::errCheck(pTex->LockRect(0, &r, NULL, 0));
			*reinterpret_cast<DWORD*>(r.pBits) = D3DCOLOR_ARGB(255, i, i, i);
			pTex->UnlockRect(0);
		
			mEffects.beginTechnique("ParticleRender");
			
			RenderEngine::bufferManager().setRenderTarget(0, eXBuffer);
			
			D3D::clear(D3DCLEAR_TARGET);
			
			D3D::setRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			
			D3D::clearTextures();
			
			D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eDefaultDiffuse));
			
			mEffects.setVector("gParticleModulateColor", Vec4(1,1,1,1));
			
			RenderHelpers::renderSSQ();
											
			mEffects.endTechnique();

			D3D::clearTextures();
			
			std::vector<uchar> bits;
			HDRScreenshot::getSurfaceBits(bits, RenderEngine::bufferManager().getSurface(eXBuffer));
		}
	}
#endif

	void ToneMapper::photoToneMapFinal(ETexture glareBuffer)
	{
		D3DSURFACE_DESC desc;
		RenderEngine::bufferManager().getTexture(glareBuffer)->GetLevelDesc(0, &desc);
		
		mEffects.setVector("gToneMapFinalGlareDim", Vec4(desc.Width, desc.Height, 0, 0));
		mEffects.setVector("gToneMapFinalGlareOODim", Vec4(1.0f/desc.Width, 1.0f/desc.Height, 0, 0));
				
		mEffects.beginTechnique("ToneMapPhotoFinal");
			
		setToneMapParams();
		
		mEffects.setFloat("gToneMapFinalGlareBufferScale", gGlareBufferScale);

		//D3D::setRenderTarget(0, RenderEngine::sceneRenderViewport().surf());
		RenderEngine::sceneRenderViewport().setToDevice(SET_ALL);

		D3D::setTextureClampLinear(0, RenderEngine::bufferManager().getTexture(glareBuffer));
						
		RenderHelpers::renderSSQ();
						
		mEffects.endTechnique();

		D3D::clearTextures();
	}

	void ToneMapper::photoToneMap(void)
	{
		photoToneMapDownsample(eFBuffer, eG0Buffer);
		
		if (mNumBloomPasses)
		{
			photoToneMapLowPass(mBloomExposure, eG4Buffer);
		}
		else
		{
			RenderEngine::bufferManager().setRenderTarget(0, eG4Buffer);
			D3D::clear(D3DCLEAR_TARGET);
		}

		if (mStreakElements)
		{
			//photoToneMapDownsample(mStreakExposure, eG0Buffer);

			for (int i = 0; i < mStreakElements; i++)
			{
				float r = mStreakOfs + mStreakElementOfs[i] + (i + .5f) / float(mStreakElements) * 360.0f;
				float m = pow(2.0f, 2.47393f + mExposure + mStreakExposure);
				ETexture glareBuffer = 
					photoToneMapStreak(
					mNumStreakPasses[i],
					Math::fDegToRad(r), 
					mStreakFalloff[i],
					mStreakInten[i], m,
					eG0Buffer, 
					eG1Buffer, 
					eG3Buffer);
				
				photoToneMapAccum(glareBuffer, eG4Buffer, false, 
					mStreakOverallInten * mStreakR[i], 
					mStreakOverallInten * mStreakG[i], 
					mStreakOverallInten * mStreakB[i]);
			}
		}
				
		photoToneMapFinal(eG4Buffer);
	}

  void ToneMapper::toneMap(float exposure)
  {
		mExposure = exposure + mRelExposure;
		
		int type = mType;
		
		if (
				(RenderEngine::sceneRenderViewport().viewportWidth() != RenderEngine::bufferManager().getBackbufferWidth()) ||
				(RenderEngine::sceneRenderViewport().viewportHeight() != RenderEngine::bufferManager().getBackbufferHeight())
			 )
		{
			type = eLinearToneMapper;
		}
		else if (ePhotoToneMapper == type)
		{
			photoToneMap();
			return;
		}
				
		const float m = pow(2.0f, mExposure);
		const float g = 1.0f / mDisplayGamma;

		mEffects.setFloat("gToneMapM", m);
		mEffects.setFloat("gToneMapG", g);
		switch (type)
		{
			case eLinearToneMapper:
			{
				mEffects.beginTechnique("ToneMapLinear");
				
				break;
			}
			case eRawToneMapper:
			{
				mEffects.beginTechnique("ToneMapRaw");
				break;
			}
			case eDiffuseAccum:
			{
				mEffects.beginTechnique("ViewAccum");
				D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eDABuffer));
				break;
			}
			case eSpecAccum:
			{
				mEffects.beginTechnique("ViewAccum");
				D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eSABuffer));
				break;
			}
			case eViewNormals:
			{
				mEffects.beginTechnique("ViewBufferRGB");
				D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eNBuffer));
				break;
			}
			case eViewReflection:
			{
				mEffects.beginTechnique("ViewBufferRGB");
				D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eRBuffer));
				break;
			}
			case eViewAlbedo:
			{
				mEffects.beginTechnique("ViewBufferRGB");
				D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eABuffer));
				break;
			}
			case eViewFrontSpec:
			{
				mEffects.beginTechnique("ViewBufferRGB");
				D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eSBuffer));
				break;
			}
			case eViewSideSpec:
			{
				mEffects.beginTechnique("ViewBufferRGB");
				D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eSBuffer));
				break;
			}
			case eViewSpecPower:
			{
				mEffects.beginTechnique("ViewBufferComponent");
				D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eNBuffer));
				mEffects.setVector("gViewBufferComponentDot", Vec4(0,0,0,1));
				break;
			}
			case eViewEmissive:
			{
				mEffects.beginTechnique("ViewBufferRGB");
				D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eEmissiveBuffer));
				break;
			}
			case eViewEmissiveAlpha:
			{
				mEffects.beginTechnique("ViewBufferComponent");
				D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eEmissiveBuffer));
				mEffects.setVector("gViewBufferComponentDot", Vec4(0,0,0,1));
				break;
			}
			case eViewEnv:
			{
				mEffects.beginTechnique("ViewBufferRGB");
				D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eEnvBuffer));
				break;
			}
			case eViewEnvAlpha:
			{
				mEffects.beginTechnique("ViewBufferComponent");
				D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(eEnvBuffer));
				mEffects.setVector("gViewBufferComponentDot", Vec4(0,0,0,1));
				break;
			}
		}

		RenderEngine::sceneRenderViewport().setToDevice(SET_ALL);
				
		RenderHelpers::renderSSQ();
				
		mEffects.endTechnique();
		
		D3D::clearTextures();
  }

} // namespace gr


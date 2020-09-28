// File: shadow_buffer.h
#ifndef SHADOW_BUFFER_H
#define SHADOW_BUFFER_H
#pragma once

#include "common/render/camera.h"
#include "light_tilizer.h"
#include "render_viewport.h"
#include "effects.h"
#include "body_bounds.h"

namespace gr
{
	class ShadowBufferParams
	{
	public:
		ShadowBufferParams(const Light& light, const LightTilizer& tilizer, const BodyBounds& bodyBounds) : 
			mLightParams(light),
			mTilizer(tilizer),
			mBodyBounds(bodyBounds)
		{
			initCamera(0);
		}

		~ShadowBufferParams()
		{
		}

		void initCamera(int pass);
				
		// viewspace is defined by D3D::getMatrix(eViewToWorld)
		Matrix44 viewToLight(void) const;
		
		const Matrix44& worldToView(void) const { return mLightCamera.worldToView(); }
		const Matrix44& viewToProj(void) const { return mLightCamera.viewToProj(); }
		
		const Camera& lightCamera(void) const { return mLightCamera; }
		const Light& light(void) const { return mLightParams; }
		
		const LightTilizer& tilizer(void) const { return mTilizer; }
    
	private:
		Camera mLightCamera;
		const Light& mLightParams;
		const LightTilizer& mTilizer;
		const BodyBounds& mBodyBounds;
	};

	class ShadowBufferModelRenderer
	{
	public:
		ShadowBufferModelRenderer()
		{
		}

		virtual ~ShadowBufferModelRenderer()
		{
		}

		// true if anything was rendered
		virtual bool render(const ShadowBufferParams& shadowBufferParams, int pass) = 0;
	};

	class ShadowBuffer
	{
	public:
		ShadowBuffer();
		~ShadowBuffer();

		void initDeviceObjects(void);
		void deleteDeviceObjects(void);
		void invalidateDeviceObjects(void);
		void restoreDeviceObjects(void);
		void oneTimeSceneInit(void);
		void startOfFrame(float deltaT);
		
		bool renderShadowModels(
			ShadowBufferParams& shadowBufferParams,
			ShadowBufferModelRenderer& shadowBufferModelRenderer);

		void displayShadowMap(void);
		
	private:
		enum EFilter
		{
			eBilinear,
			eBilinearJittered,
			eBilinearJittered4,
			eStochastic4x4,
			eKernel4x4,
			eKernel5x5,
			eNumFilters
		};

		int mFilterType;

		int mShadowBufferNumPasses;
		float mShadowBufferDepthJitterL;
		float mShadowBufferDepthJitterH;
		float mShadowBufferPosJitterR;
		bool mViewShadowBuffer;
		bool mViewVBuffer;
												
		enum { MAX_JITTER_PASSES = 8 };
		Vec4 mJitterDepth[4*MAX_JITTER_PASSES];
		Vec4 mJitterPos[8*MAX_JITTER_PASSES];
		Vec4 mDPJitterPos[16*MAX_JITTER_PASSES];
				
		enum { NUM_JITTER_TEXTURES = 2 };
		enum { JITTER_TEXTURE_WIDTH = 256 };
		enum { JITTER_TEXTURE_HEIGHT = 1 };
		IDirect3DTexture9* mpJitterTextures[NUM_JITTER_TEXTURES];
				
		Effects& mEffects;
		
		float mFrameDeltaT;
		
		bool mEnabled;
		
		bool mSecondDepth;
				
		void beginVBufferRender(const ShadowBufferParams& params);
		void fillVBufferSpotBilinear(ShadowBufferParams& shadowBufferParams);
		void fillVBufferSpotBilinearJittered(ShadowBufferParams& shadowBufferParams);
		void fillVBufferSpotBilinearJittered4(ShadowBufferParams& shadowBufferParams);
		void fillVBufferSpotStochastic4x4(ShadowBufferParams& shadowBufferParams);
		void fillVBufferSpot5x5Kernel(ShadowBufferParams& shadowBufferParams);
		void fillVBufferSpot4x4Kernel(ShadowBufferParams& shadowBufferParams);
		void fillVBufferSpot(ShadowBufferParams& shadowBufferParams);
		void fillVBufferOmni(ShadowBufferParams& shadowBufferParams);
		void endVBufferRender(void);

		void initShadowBufferJitterTables(void);
		void setJitterTextureTexel(const D3DLOCKED_RECT& rect, int x, int y, DWORD p);
		void createJitterTextures(void);
		void beginShadowRender(int pass);
		void endShadowRender(const ShadowBufferParams& params);
		void initTweakers(void);
		void initVBufferRender(void);
	};

} // namespace gr

#endif // SHADOW_BUFFER_H










// File: volume_fog.h
#pragma once
#ifndef VOLUME_FOG_H
#define VOLUME_FOG_H

#include "common/render/camera.h"
#include "light_tilizer.h"
#include "render_viewport.h"
#include "effects.h"

namespace gr
{
	class ShadowBufferParams;
	
	class VolumeFog
	{
	public:	
		VolumeFog();
		~VolumeFog();

		void initDeviceObjects(void);
		void deleteDeviceObjects(void);
		void invalidateDeviceObjects(void);
		void restoreDeviceObjects(void);
		void oneTimeSceneInit(void);
		void startOfFrame(float deltaT);
		void beginScene(void);
		
		void render(ShadowBufferParams& shadowBufferParams, bool hasShadows);
		bool frameHasVolumeFog(void) const { return mClearedFogAccumBuffer; }
		
	private:
		enum { MAX_VOL_TRACING_PASSES = 16 };
		int mNumShadowTracePasses;
						
		IDirect3DVolumeTexture9* mpVolumeTex;
		
		Effects& mEffects;
		
		float mFrameDeltaT;
		
		enum { MAX_FOG_DENSITY_PASSES = 9 };
		int mFogDensityPasses;
		
		Vec4 mVolOrigin;
		Vec4 mVolDim;
		Vec4 mVolRot;
		float mVolDimK;
		bool mClearedFogAccumBuffer;
		bool mEnabled;
		float mFogDensity;
		
		void initTweakers(void);
		void createVolumeTexture(void);
		void renderInit(void);
		void renderDeinit(void);
		void calcInshadowFractTrace(ShadowBufferParams& shadowBufferParams);
		void renderVolume(ShadowBufferParams& shadowBufferParams);
		void volumeTrace(ShadowBufferParams& shadowBufferParams);
		void calcFog(ShadowBufferParams& shadowBufferParams, bool hasShadows);
	};

} // namespace gr

#endif // VOLUME_FOG_H
//-----------------------------------------------------------------------------
// File: lighting_engine.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef LIGHTING_ENGINE_H

#include "common/math/vector.h"
#include "common/math/plane.h"
#include "common/render/frustum.h"

#include "tone_mapper.h"
#include "light.h"
#include "shadow_buffer.h"
#include "light_tilizer.h"
#include "render_viewport.h"
#include "effects.h"
#include "volume_fog.h"
#include "body_bounds.h"

#include <d3d9.h>

namespace gr
{
	struct FinalLightPassParams
	{
    Vec4 mHemiTop;
		Vec4 mHemiBottom;
		Vec4 mHemiAxis;
		Vec4 mSkinColor;
		
		FinalLightPassParams() : mHemiTop(0), mHemiBottom(0), mHemiAxis(0,0,1,0), mSkinColor(0)
		{
		}
	};
		
	class LightingEngine
	{
	public:
		LightingEngine();
		~LightingEngine();

		void clear(void);

		void initDeviceObjects(void);
		void deleteDeviceObjects(void);
		void invalidateDeviceObjects(void);
		void restoreDeviceObjects(void);
		void oneTimeSceneInit(void);
		void reloadEffects(void);

		void startOfFrame(float deltaT);
		
		void beginScene(const RenderViewport& renderViewport);
		void endScene(void);

		void beginFirst(void);
		void endFirst(void);
		
		void beginSecond(void);
		void endSecond(void);
		
		void beginLight(const BodyBounds& bodyBounds);
		void drawLight(const Light& params, ShadowBufferModelRenderer& shadowBufferModelRenderer);
		void endLight(void);

		void beginFinal(void);
		void endFinal(float exposure, bool toneMap);
						
		int getTileWidth(void) const
		{
			return mLightTilizer.getTileWidth();
		}

		int getTileHeight(void) const
		{
			return mLightTilizer.getTileHeight();
		}

	private:
		Effects& mEffects;
		RenderViewport mRenderViewport;
		ToneMapper mToneMapper;
		ShadowBuffer mShadowBuffer;
		LightTilizer mLightTilizer;
		VolumeFog mVolumeFog;
				
		bool mViewTileComplexity;
		bool mViewLightBounds;

		int mLightIndex;
		int mViewTileLightIndex;
		bool mIndirectLighting;
		
		const BodyBounds* mpBodyBounds;

		std::vector<ConvexPolyhedron> mLightPolyhedrons;
		
		LightingEngine(const LightingEngine& b);
		LightingEngine& operator= (const LightingEngine& b);

		void initTweakers(void);
		
		void beginSSQPasses(void);
		void endSSQPasses(void);

		void setLightRenderStates(void);
		void drawOmniSpotLight(const Light& params, ShadowBufferModelRenderer& shadowBufferModelRenderer);
		void drawDirLight(const Light& params);
				
		void finalAccumPass(const FinalLightPassParams& params);
		
		void displayShadowMap(void);
		void displayLightBounds(void);
	};
	
} // namespace gr

#endif // LIGHTING_ENGINE_H


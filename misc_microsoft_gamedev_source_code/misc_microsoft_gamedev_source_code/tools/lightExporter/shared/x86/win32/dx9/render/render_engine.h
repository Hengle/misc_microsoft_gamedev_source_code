//-----------------------------------------------------------------------------
// render_engine.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef RENDER_ENGINE_H
#define RENDER_ENGINE_H

#include "effects.h"
#include "texture_manager.h"
#include "buffer_manager.h"
#include "render_viewport.h"
#include "vertex_decl_manager.h"
#include "lighting_engine.h"
#include "dynacube.h"

#include "x86/win32/dx9/particle/ParticleRenderer.h"

#include "common/render/camera.h"

namespace gr
{
	class RenderEngine
	{
	public:
		RenderEngine();
		~RenderEngine();

		static void setDevice(IDirect3DDevice9* pDev);

		static HWND mainWindow(void);
		static void setMainWindow(HWND mainWindow);
		
		static void initDeviceObjects(void);
		static void deleteDeviceObjects(void);
		static void invalidateDeviceObjects(void);
		static void restoreDeviceObjects(void);
		static void oneTimeSceneInit(void);
		static void startOfFrame(float deltaT);
		static void tick(float deltaT);

		static void clear(void);

		static void initStaticTweakers(void);
		
		static Effects&				effects(void)								{ return mEffects; }
		static TextureManager& textureManager(void)				{ return mTextureManager; }
		static LightingEngine& lightingEngine(void)				{ return mLightingEngine; }
		
		static BufferManager&  bufferManager(void)					{ return mBufferManager; }
		static VertexDeclManager& vertexDeclManager(void)		{ return mVertexDeclManager; }

		static void beginScene(const RenderViewport& renderViewport, bool lightProbe = false);
		static void endScene(void);

		static const RenderViewport& sceneRenderViewport(void) { return mSceneRenderViewport; }
		
		static const BigString& effectPath(void) { return mEffectPath; }
		
		static void reloadEffects(void);
		
		static ParticleRenderer& particleRenderer(void) { return mParticleRenderer; }
		
		static DynaCube& dynaCube(void) { return mDynaCube; }
		
		static void flushTextures(void);
		
		// true if a light probe is currently being rendered
		static bool lightProbeRender(void) { return mLightProbe; }
		
		static void RenderEngine::drawLine(const Vec3& start, const Vec3& end, const Vec3& color);
		static void flushLines(void);
						
	private:
		static LightingEngine	mLightingEngine;
		static TextureManager	mTextureManager;	
		static BufferManager mBufferManager;
		static VertexDeclManager mVertexDeclManager;
		static ParticleRenderer mParticleRenderer;
		static DynaCube	mDynaCube;
				
		static BigString mEffectPath;
		static Effects mEffects;
		
		static HWND	mMainWindow;

		static RenderViewport	mSceneRenderViewport;
		static bool mLightProbe;
		
		static void setDefaultDeviceStates(void);
		static void setEffectBasename(void);
		static void updateEffectTextures(void);
	};
		
} // namespace gr

#endif // RENDER_ENGINE_H






//-----------------------------------------------------------------------------
// File: render_engine.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "render_engine.h"
#include "ugx_render.h"

namespace gr
{
	LightingEngine	RenderEngine::mLightingEngine;
	TextureManager	RenderEngine::mTextureManager;	
	BufferManager		RenderEngine::mBufferManager;
	VertexDeclManager RenderEngine::mVertexDeclManager;
	Effects					RenderEngine::mEffects;
	BigString				RenderEngine::mEffectPath;
	DynaCube				RenderEngine::mDynaCube;

	HWND						RenderEngine::mMainWindow;
	ParticleRenderer RenderEngine::mParticleRenderer;

	RenderViewport	RenderEngine::mSceneRenderViewport;
	bool						RenderEngine::mLightProbe;
		
	void RenderEngine::setDevice(IDirect3DDevice9* pDev)
	{
		D3D::setDevice(pDev);
	}

	void RenderEngine::setMainWindow(HWND mainWindow)
	{
		mMainWindow = mainWindow;
	}

	HWND RenderEngine::mainWindow(void)
	{
		return mMainWindow;
	}

	void RenderEngine::setEffectBasename(void)
	{
		BigString modulePath;
		GetModuleFileName(NULL, modulePath, modulePath.bufSize());
		modulePath.removeFilename();

		mEffectPath = modulePath;
				
		mEffects.setBasename(mEffectPath + "shaders");
	}
	
	void RenderEngine::reloadEffects(void)
	{
		mEffects.reload();
		mParticleRenderer.reloadEffects();
		mLightingEngine.reloadEffects();
	}

	void RenderEngine::initDeviceObjects(void)
	{
		mEffects.initDeviceObjects();
		mTextureManager.initDeviceObjects();
		mBufferManager.initDeviceObjects();
		mVertexDeclManager.initDeviceObjects();
		mLightingEngine.initDeviceObjects();
		mParticleRenderer.initDeviceObjects();
		mDynaCube.initDeviceObjects();
	}

	void RenderEngine::deleteDeviceObjects(void)
	{
		mEffects.deleteDeviceObjects();
		mTextureManager.deleteDeviceObjects();
		mBufferManager.deleteDeviceObjects();
		mVertexDeclManager.deleteDeviceObjects();
		mLightingEngine.deleteDeviceObjects();
		mParticleRenderer.deleteDeviceObjects();
		mDynaCube.deleteDeviceObjects();

		D3D::clearTextures();
		D3D::invalidateCache();
	}
	
	void RenderEngine::invalidateDeviceObjects(void)
	{
		mEffects.invalidateDeviceObjects();
		mTextureManager.invalidateDeviceObjects();
		mBufferManager.invalidateDeviceObjects();
		mLightingEngine.invalidateDeviceObjects();
		mParticleRenderer.invalidateDeviceObjects();
		mDynaCube.invalidateDeviceObjects();
		
		D3D::clearTextures();
    D3D::invalidateCache();
	}

	void RenderEngine::setDefaultDeviceStates(void)
	{
		D3D::setRenderState(D3DRS_DITHERENABLE,    TRUE );
		D3D::setRenderState(D3DRS_SPECULARENABLE,  FALSE );
		D3D::setRenderState(D3DRS_ZENABLE,         D3DZB_TRUE );
		D3D::setRenderState(D3DRS_AMBIENT,         D3DCOLOR_ARGB(0,20,20,20));
		D3D::setRenderState(D3DRS_LIGHTING,        TRUE );
		D3D::setRenderState(D3DRS_LASTPIXEL,			 TRUE);
	}

	void RenderEngine::restoreDeviceObjects(void)
	{
		mEffects.restoreDeviceObjects();
		mTextureManager.restoreDeviceObjects();
		mBufferManager.restoreDeviceObjects();
		mLightingEngine.restoreDeviceObjects();
		mParticleRenderer.restoreDeviceObjects();
		mDynaCube.restoreDeviceObjects();
		
		setDefaultDeviceStates();
	}

	void RenderEngine::oneTimeSceneInit(void)
	{
		setEffectBasename();
		mTextureManager.oneTimeSceneInit();
		mBufferManager.oneTimeSceneInit();
		mLightingEngine.oneTimeSceneInit();
		mParticleRenderer.oneTimeSceneInit();
		mDynaCube.oneTimeSceneInit();
	}

	void RenderEngine::tick(float deltaT)
	{
		mTextureManager.tick(deltaT);
	}
	
	void RenderEngine::updateEffectTextures(void)
	{
		for (int i = 0; i < eNumTextures; i++)
		{
			ETexture texture = static_cast<ETexture>(i);
			if (mBufferManager.getTexture(texture))
				mEffects.setTexture(texture);
		}
	}

	void RenderEngine::startOfFrame(float deltaT)
	{
		D3D::invalidateCache();

		//mEffects.startOfFrame();
		mTextureManager.startOfFrame();
		mBufferManager.startOfFrame();
		mLightingEngine.startOfFrame(deltaT);
				
		updateEffectTextures();
	}

	void RenderEngine::beginScene(const RenderViewport& renderViewport, bool lightProbe)
	{
		mSceneRenderViewport = renderViewport;
		mLightProbe = lightProbe;
		
		mSceneRenderViewport.setToDevice();
		
		D3D::setRenderState(D3DRS_ZENABLE, TRUE);
		D3D::enableCulling();
		D3D::clearTextures();
		D3D::setPixelShader(NULL);
		D3D::setVertexShader(NULL);
		
		mLightingEngine.beginScene(mSceneRenderViewport);
	}

	void RenderEngine::endScene(void)
	{
		mLightingEngine.endScene();

		D3D::flushCache();
	}

	void RenderEngine::clear(void)
	{
		mEffects.clear();
		mTextureManager.clear();
		mLightingEngine.clear();
	}

	void RenderEngine::initStaticTweakers(void)
	{
		RenderMesh::initStaticTweakers();
	}
	
	void RenderEngine::flushTextures(void)
	{
		mTextureManager.flush();
		mParticleRenderer.flushTextures();
	}
	
	struct LineDef
	{
		Vec3 start;
		Vec3 end;
		Vec3 color;
		
		LineDef(
			const Vec3& s,
			const Vec3& e,
			const Vec3& c) : start(s), end(e), color(c)
		{
		}
	};
	
	typedef std::vector<LineDef> LineDefVec;
	static LineDefVec gQueuedLines;
		
	void RenderEngine::drawLine(const Vec3& start, const Vec3& end, const Vec3& color)
	{
		gQueuedLines.push_back(LineDef(start, end, color));
	}
	
	struct WireVertex
	{
		Vec3 position;       // vertex position
		D3DCOLOR diffuse;
		
		WireVertex() { }
		WireVertex(const Vec3& p, D3DCOLOR d) : position(p), diffuse(d) { }
	};
	#define D3DFVF_WIREVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)
	
	void RenderEngine::flushLines(void)
	{
		if (gQueuedLines.empty())
			return;
			
		std::vector<WireVertex> verts;
		verts.reserve(gQueuedLines.size() * 2);
		
		for (int i = 0; i < gQueuedLines.size(); i++)
		{
			D3DCOLOR c = D3DCOLOR_ARGB(255, 
				Math::iClampToByte(Math::FloatToIntRound(gQueuedLines[i].color[0] * 255.0f)),
				Math::iClampToByte(Math::FloatToIntRound(gQueuedLines[i].color[1] * 255.0f)),
				Math::iClampToByte(Math::FloatToIntRound(gQueuedLines[i].color[2] * 255.0f))
				);
				
			verts.push_back(WireVertex(gQueuedLines[i].start, c));
			verts.push_back(WireVertex(gQueuedLines[i].end, c));
		}
									
		D3D::setVertexShader(NULL);
		D3D::setPixelShader(NULL);
		
		D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		D3D::setRenderState(D3DRS_LIGHTING, FALSE);
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		
		D3D::setFVF(D3DFVF_WIREVERTEX);
		D3D::drawPrimitiveUP(D3DPT_LINELIST, gQueuedLines.size(), &verts[0], sizeof(verts[0]));
		
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		D3D::setRenderState(D3DRS_LIGHTING, TRUE);
		
		gQueuedLines.erase(gQueuedLines.begin(), gQueuedLines.end());
	}
						  
} // namespace gr









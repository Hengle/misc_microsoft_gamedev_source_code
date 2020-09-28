//-----------------------------------------------------------------------------
// File: buffer_manager.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "buffer_manager.h"

namespace gr
{
	namespace
	{	
		// Effect Texture Names
		struct
		{
			const char* pName;
			ETexture eTexture;
		} gEffectTextureNames[] = 
		{
			{ "gDefaultDiffuse", eDefaultDiffuse },
			{ "gDefaultNormal", eDefaultNormal },
			{ "gBlackTexture", eBlackTexture },
			{ "gDBuffer",  eDBuffer },
			{ "gRBuffer",  eRBuffer },
			{ "gNBuffer",  eNBuffer },
			{ "gDABuffer", eDABuffer },
			{ "gSABuffer", eSABuffer },
			{ "gKBuffer",  eKBuffer },
			{ "gSDBuffer", eSDBuffer },
			{ "gG0Buffer", eG0Buffer },
			{ "gG1Buffer", eG1Buffer },
			{ "gG2Buffer", eG2Buffer },
			{ "gG3Buffer", eG3Buffer },
			{ "gG4Buffer", eG4Buffer },
			{ "gHBuffer",	 eHBuffer },
			{ "gHDBuffer", eHDBuffer },
			{ "gVBuffer",	 eVBuffer },
			{ "gFBuffer",	 eFBuffer },
			{ "gABuffer",	 eABuffer },
			{ "gSBuffer",	 eSBuffer },
			{ "gPLBuffer",	 ePLBuffer },
			{ "gPHBuffer",	 ePHBuffer },			
			{ "gDPBuffer",	 eDPBuffer },			
			{ "gDPDBuffer",	 eDPDBuffer },			
			{ "gFogAccumBuffer",  eFogAccumBuffer },
			{ "gFogFrontBuffer",  eFogFrontBuffer },
			{ "gFogBackBuffer",  eFogBackBuffer },
			{ "gEmissiveBuffer", eEmissiveBuffer },
			{ "gEnvBuffer", eEnvBuffer },
			{ "gDensityBuffer", eDensityBuffer }
		};
		const int NumEffectTextureNames = sizeof(gEffectTextureNames) / sizeof(gEffectTextureNames[0]);
	} // anonymous namespace

	BufferManager::BufferManager() :
		mCubemaps(eNumCubemaps), 
		mTextures(eNumTextures), 
		mSurfaces(eNumTextures)
	{
	}

	BufferManager::~BufferManager()
	{
	}
	
	void BufferManager::initDeviceObjects(void)
	{
		clear();
		
		initTextures();

		updateBackbufferSurface();
	}
	
	void BufferManager::deleteDeviceObjects(void)
	{
		clear();
	}

	void BufferManager::invalidateDeviceObjects(void)
	{
		D3D::safeReleaseVec(mSurfaces);
		
		releaseRenderTargets();
	}

	void BufferManager::restoreDeviceObjects(void)
	{
		updateBackbufferSurface();
		createRenderTargets();
		updateSurfaces();
	}

	void BufferManager::oneTimeSceneInit(void)
	{
	}
		
	void BufferManager::startOfFrame(void)
	{
		updateBackbufferSurface();
	}

	void BufferManager::clear(void)
	{
		D3D::safeReleaseVec(mCubemaps);
		D3D::safeReleaseVec(mSurfaces);
		D3D::safeReleaseVec(mTextures);
	}

	void WINAPI BufferManager::FillNormCubemap(D3DXVECTOR4 *pOut, CONST D3DXVECTOR3 *pTexCoord, CONST D3DXVECTOR3 *pTexelSize, LPVOID pData)
	{
		D3DXVECTOR3 n;
		D3DXVec3Normalize(&n, pTexCoord);
		pOut->x = n.x;
		pOut->y = n.y;
		pOut->z = n.z;
		pOut->w = 1.0f;
	}

	void BufferManager::createCubemaps(void)
	{
		const int CUBE_TEXTURE_EDGE_LEN = 128;

		mCubemaps[eNormCubemap] = D3D::createCubeTexture(CUBE_TEXTURE_EDGE_LEN);

		D3DXFillCubeTexture(mCubemaps[eNormCubemap], FillNormCubemap, NULL);
	}

	void BufferManager::createDefaultTextures(void)
	{
		mTextures[eDefaultDiffuse] = D3D::createTexture(1, 1);
		mTextures[eDefaultNormal] = D3D::createTexture(1, 1);
		mTextures[eBlackTexture] = D3D::createTexture(1, 1);

		D3DLOCKED_RECT r;
		
		D3D::errCheck(mTextures[eDefaultDiffuse]->LockRect(0, &r, NULL, 0));
		*reinterpret_cast<DWORD*>(r.pBits) = D3DCOLOR_ARGB(255, 255, 255, 255);
		mTextures[eDefaultDiffuse]->UnlockRect(0);

		D3D::errCheck(mTextures[eDefaultNormal]->LockRect(0, &r, NULL, 0));
		*reinterpret_cast<DWORD*>(r.pBits) = D3DCOLOR_ARGB(255, 128, 128, 255);
		mTextures[eDefaultNormal]->UnlockRect(0);
		
		D3D::errCheck(mTextures[eBlackTexture]->LockRect(0, &r, NULL, 0));
		*reinterpret_cast<DWORD*>(r.pBits) = D3DCOLOR_ARGB(0, 0, 0, 0);
		mTextures[eBlackTexture]->UnlockRect(0);
	}

	IDirect3DTexture9* BufferManager::createFSRenderTarget(D3DFORMAT format, int xDiv, int yDiv)
	{
		return D3D::createTexture(
			getBackbufferWidth() / xDiv, 
			getBackbufferHeight() / yDiv, 
			1, 
			D3DUSAGE_RENDERTARGET, 
			format, 
			D3DPOOL_DEFAULT);
	}

	void BufferManager::createRenderTargets(void)
	{
		mTextures[eDBuffer] = createFSRenderTarget(D3DFMT_R32F);
		mTextures[eRBuffer] = createFSRenderTarget(D3DFMT_A2R10G10B10);
		mTextures[eNBuffer] = createFSRenderTarget(D3DFMT_A8R8G8B8);
		mTextures[eABuffer] = createFSRenderTarget(D3DFMT_A8R8G8B8);
		mTextures[eSBuffer] = createFSRenderTarget(D3DFMT_A8R8G8B8);
				
		mTextures[eDABuffer] = createFSRenderTarget(D3DFMT_A16B16G16R16F);
		mTextures[eSABuffer] = createFSRenderTarget(D3DFMT_A16B16G16R16F);

#if HALFRES_DOWNSAMPLE
		mTextures[eG0Buffer] = createFSRenderTarget(D3DFMT_A16B16G16R16, 2, 2);
		mTextures[eG1Buffer] = createFSRenderTarget(D3DFMT_A16B16G16R16, 2, 2);
		mTextures[eG2Buffer] = createFSRenderTarget(D3DFMT_A16B16G16R16, 2, 2);
		mTextures[eG3Buffer] = createFSRenderTarget(D3DFMT_A16B16G16R16, 2, 2);
		mTextures[eG4Buffer] = createFSRenderTarget(D3DFMT_A16B16G16R16, 2, 2);
#else		
#if 0
		mTextures[eG0Buffer] = createFSRenderTarget(D3DFMT_A16B16G16R16, 4, 4);
		mTextures[eG1Buffer] = createFSRenderTarget(D3DFMT_A16B16G16R16, 4, 4);
		mTextures[eG2Buffer] = createFSRenderTarget(D3DFMT_A16B16G16R16, 2, 2);
		mTextures[eG3Buffer] = createFSRenderTarget(D3DFMT_A16B16G16R16, 4, 4);
		mTextures[eG4Buffer] = createFSRenderTarget(D3DFMT_A16B16G16R16, 4, 4);
#endif		
		
		mTextures[eG0Buffer] = createFSRenderTarget(D3DFMT_A2B10G10R10, 4, 4);
		mTextures[eG1Buffer] = createFSRenderTarget(D3DFMT_A2B10G10R10, 4, 4);
		mTextures[eG2Buffer] = createFSRenderTarget(D3DFMT_A2B10G10R10, 2, 2);
		mTextures[eG3Buffer] = createFSRenderTarget(D3DFMT_A2B10G10R10, 4, 4);
		mTextures[eG4Buffer] = createFSRenderTarget(D3DFMT_A2B10G10R10, 4, 4);
#endif		
		
		mTextures[eHBuffer] =  D3D::createTexture(ShadowBufferWidth, ShadowBufferHeight, 
			//1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT);
			1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT);
								
		mSurfaces[eHDBuffer] = D3D::createDepthStencilSurface(ShadowBufferWidth, ShadowBufferHeight, 
			D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE);

		mTextures[eVBuffer] = createFSRenderTarget(D3DFMT_A8R8G8B8, 1, 1);
		
		mTextures[eFBuffer] = createFSRenderTarget(D3DFMT_A16B16G16R16F);
		
		mTextures[ePLBuffer] = createFSRenderTarget(D3DFMT_A8R8G8B8);
		mTextures[ePHBuffer] = createFSRenderTarget(D3DFMT_A8R8G8B8);
		
		mTextures[eDPBuffer] = D3D::createTexture(ShadowBufferWidth * 2, ShadowBufferHeight, 
			1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT);
			
		mSurfaces[eDPDBuffer] = D3D::createDepthStencilSurface(ShadowBufferWidth * 2, ShadowBufferHeight, 
			D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE);
			
		mTextures[eFogAccumBuffer] = createFSRenderTarget(D3DFMT_A16B16G16R16F);
		mTextures[eFogFrontBuffer] = createFSRenderTarget(D3DFMT_R32F);
		mTextures[eFogBackBuffer] = createFSRenderTarget(D3DFMT_R32F);
		
		mTextures[eEmissiveBuffer] = createFSRenderTarget(D3DFMT_A8R8G8B8);
		mTextures[eEnvBuffer] = createFSRenderTarget(D3DFMT_A8R8G8B8);
		mTextures[eDensityBuffer] = createFSRenderTarget(D3DFMT_R16F);
	}

	void BufferManager::releaseRenderTargets(void)
	{
		D3D::clearTextures();

		for (int i = eFirstNonManagedTexture; i < eNumTextures; i++)
		{
			D3D::safeRelease(mTextures[i]); 
			D3D::safeRelease(mSurfaces[i]); 
		}
	}

	void BufferManager::updateBackbufferSurface(void)
	{
		D3D::safeRelease(mSurfaces[eKBuffer]);
		mSurfaces[eKBuffer] = D3D::getBackBuffer(0, 0);
		getSurface(eKBuffer)->GetDesc(&mBackBufDesc);

		D3D::safeRelease(mSurfaces[eSDBuffer]);
		mSurfaces[eSDBuffer] = D3D::getDepthStencilSurface();
	}

	void BufferManager::createTextures(void)
	{
		updateBackbufferSurface();

		createDefaultTextures();
	}

	void BufferManager::updateSurfaces(void)
	{
		for (int i = 0; i < eNumTextures; i++)
			if ((mTextures[i]) && (!mSurfaces[i]))
				D3D::errCheck(mTextures[i]->GetSurfaceLevel(0, &mSurfaces[i]));
	}

	void BufferManager::initTextures(void)
	{
		createCubemaps();
		createTextures();
		updateSurfaces();
	}

	IDirect3DCubeTexture9* BufferManager::getCubemap(ECubemap cubemapIndex) const
	{
		return mCubemaps[DebugRange<int>(cubemapIndex, eNumCubemaps)];
	}
	
	IDirect3DTexture9* BufferManager::getTexture(ETexture textureIndex) const
	{
		return mTextures[DebugRange<int>(textureIndex, eNumTextures)];
	}
	
	IDirect3DSurface9* BufferManager::getSurface(ETexture textureIndex) const
	{
		return mSurfaces[DebugRange<int>(textureIndex, eNumTextures)];
	}

	const char* BufferManager::getTextureEffectName(ETexture textureIndex) const
	{
		return gEffectTextureNames[DebugRange<int>(textureIndex, eNumTextures)].pName;
	}

	const D3DSURFACE_DESC& BufferManager::getBackbufferDesc(void)
	{
		return mBackBufDesc;
	}

	int BufferManager::getBackbufferWidth(void) const
	{
		return mBackBufDesc.Width;
	}

	int BufferManager::getBackbufferHeight(void) const
	{
		return mBackBufDesc.Height;
	}
} // namespace gr


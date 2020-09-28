//-----------------------------------------------------------------------------
// File: buffer_manager.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "common/utils/utils.h"
#include "device.h"
#include <d3dx9.h>

namespace gr
{
//	#define HALFRES_DOWNSAMPLE 1

#if 0
	const int ShadowBufferWidth = 1024;
	const int ShadowBufferHeight = 1024;
#else	
	const int ShadowBufferWidth = 512;
	const int ShadowBufferHeight = 512;
#endif	
		
	enum ECubemap
	{
		eInvalidCubemap = -1,

		eNormCubemap,

		eNumCubemaps
	};

	enum ETexture
	{
		eInvalidTexture = -1,

		eDefaultDiffuse,
		eDefaultNormal,
		eBlackTexture,

		eFirstNonManagedTexture,
	
		eDBuffer = eFirstNonManagedTexture,			// Depth 
				
		eRBuffer,			// Reflection
		eNBuffer,			// Normal 
		eDABuffer,		// Diffuse Accumulation 
		eSABuffer,		// Specular Accumulation
		eKBuffer,			// Backbuffer
		eSDBuffer,    // Scene's depth buffer

		eG0Buffer,		// Gloss buffer 0 (quarter res)
		eG1Buffer,		// Gloss buffer 1 (quarter res)
		eG2Buffer,		// Gloss buffer 2 (half res)
		eG3Buffer,		// Gloss buffer 3 (quarter res)
		eG4Buffer,		// Gloss buffer 4 (quarter res)

		eHBuffer,			// Shadow buffer
		eHDBuffer,		// Shadow buffer depth/stencil
		eVBuffer,			// Visibility buffer
		
		eFBuffer,			// Final buffer
		eABuffer,			// Albedo buffer
		eSBuffer,			// Front specular buffer
		
		ePLBuffer,		// Particle accum low bits
		ePHBuffer,		// Particle accum high bits
		
		eDPBuffer,		// dual paraboloid buffer
		eDPDBuffer,		// dual paraboloid depth buffer
						
		eFogAccumBuffer,		// Volume fog accum buffer (16f16f16f16f)
		eFogFrontBuffer,			// Volume fog frontsides (32f)
		eFogBackBuffer,			// Volume fog frontsides (32f)
						
		eEmissiveBuffer,	// Emissive
		eEnvBuffer,				// Environment map
		
		eDensityBuffer,
      
		eNumTextures
	};
	
	class BufferManager
	{
	public:
		BufferManager();
		virtual ~BufferManager();

		void initDeviceObjects(void);
		void deleteDeviceObjects(void);
		void invalidateDeviceObjects(void);
		void restoreDeviceObjects(void);
		void oneTimeSceneInit(void);
		void startOfFrame(void);

		void clear(void);

		IDirect3DCubeTexture9*	getCubemap(ECubemap cubemapIndex) const;
		IDirect3DTexture9*			getTexture(ETexture textureIndex) const;
		IDirect3DSurface9*			getSurface(ETexture textureIndex) const;
		const char*							getTextureEffectName(ETexture textureIndex) const;

		const D3DSURFACE_DESC& getBackbufferDesc(void);

		int getBackbufferWidth(void) const;
		int getBackbufferHeight(void) const;
		
		void setRenderTarget(DWORD RenderTargetIndex,	ETexture texture) const
		{
			D3D::setRenderTarget(RenderTargetIndex, (texture == eInvalidTexture) ? NULL : getSurface(texture));
		}
		
		void setDepthStencilSurface(ETexture texture) const
		{
			D3D::setDepthStencilSurface((texture == eInvalidTexture) ? NULL : getSurface(texture));
		}
		
		void setTexture(DWORD stage, ETexture texture) const
		{
			D3D::setTexture(stage, getTexture(texture));
		}

	private:
		std::vector<IDirect3DCubeTexture9*> mCubemaps;
		std::vector<IDirect3DTexture9*> mTextures;
		std::vector<IDirect3DSurface9*> mSurfaces;
		D3DSURFACE_DESC mBackBufDesc;
				
		void createCubemaps(void);
		void createDefaultTextures(void);
		IDirect3DTexture9* createFSRenderTarget(D3DFORMAT format, int xDiv = 1, int yDiv = 1);
		void createRenderTargets(void);
		void releaseRenderTargets(void);
		void updateBackbufferSurface(void);
		void createTextures(void);

		void updateSurfaces(void);
		void initTextures(void);
		
		static void WINAPI FillNormCubemap(D3DXVECTOR4 *pOut, CONST D3DXVECTOR3 *pTexCoord, CONST D3DXVECTOR3 *pTexelSize, LPVOID pData);
	};

} // namespace gr

#endif // BUFFER_MANAGER_H
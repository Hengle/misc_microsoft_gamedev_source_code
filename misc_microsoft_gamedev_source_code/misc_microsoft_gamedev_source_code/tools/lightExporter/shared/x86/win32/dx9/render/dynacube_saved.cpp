// File: dynacube.cpp
#include "dynacube.h"
#include "render_helpers.h"

namespace gr
{
	DynaCube::DynaCube()
	{
		Utils::ClearObj(mpBasisTextures);
	}
	
	DynaCube::~DynaCube()
	{
	}
	
	void DynaCube::setTexelComp(const D3DLOCKED_RECT& rect, int x, int y, int c, float f)
	{
		const int bytesPerComp = sizeof(uint16);
		const int bytesPerPixel = 4 * bytesPerComp;
		int texOfs = x * bytesPerPixel + y * rect.Pitch + c * bytesPerComp;
			
		*reinterpret_cast<uint16*>(	
			reinterpret_cast<uchar*>(rect.pBits) + texOfs	
			) = Half::FloatToHalf(f, true);
	}
	
	void DynaCube::initSHBasisTextures(void)
	{
		for (int faceNum = 0; faceNum < 6; faceNum++)
		{
			for (int i = 0; i < NumBasisTextures; i++)
			{
				D3D::safeRelease(mpBasisTextures[faceNum][i]);
				
				mpBasisTextures[faceNum][i] = D3D::createTexture(
					BasisTextureDim,
					BasisTextureDim,
					1,
					D3DFMT_A16B16G16R16F);
			}
		}
		
		CubeMap dummyCubeMap(BasisTextureDim, BasisTextureDim);
		
		for (int faceNum = 0; faceNum < 6; faceNum++)
		{
			D3DLOCKED_RECT rects[NumBasisTextures];
			
			for (int i = 0; i < NumBasisTextures; i++)
				mpBasisTextures[faceNum][i]->LockRect(&rects[i]);
						
			for (int x = 0; x < BasisTextureDim; x++)
			{
				for (int y = 0; y < BasisTextureDim; y++)
				{
					const float solidAngle = dummyCubeMap.sphericalArea(faceNum, x, y);
					const Vec3 dir(dummyCubeMap.vector(faceNum, x, y));
					const Vec9 sh(SphericalHarmonic::evaluate(dir));
										
					//    mem  coeff-1
					//    3012 
					// 0: ARGB 3012
					// 1: ARGB 7456
					
					// skips DC (constant) coefficient, 0 is actually SH coefficient #1
					for (int c = 0; c < 8; c++)
						setTexelComp(rects[c >> 2], x, y, c & 3, sh[c + 1] * solidAngle);
				}
			}
				
			for (int i = 0; i < NumBasisTextures; i++)
				mpBasisTextures[faceNum][i]->UnlockRect();
		}
	}
	
	void DynaCube::initWorkSurfaces(void)
	{
		for (int i = 0; i < NumWorkSurfaces; i++)
		{
			D3D::safeRelease(mpWorkSurf[i]);
			mpWorkSurf[i] = D3D::createTexture(
				WorkSurfDim, WorkSurfDim, 
				1, 
				D3DUSAGE_RENDERTARGET, 
				D3DFMT_A16B16G16R16F, 
				D3DPOOL_DEFAULT);
		}
	}
	
	void DynaCube::initDeviceObjects(void)
	{
		initSHBasisTextures();
		initWorkSurfaces();
	}
	
	void DynaCube::releaseBasisTextures(void)
	{
		for (int i = 0; i < NumBasisTextures; i++)
			D3D::safeRelease(mpBasisTextures[i]);	
	}
	
	void DynaCube::releaseWorkSurfaces(void)
	{
		for (int i = 0; i < NumWorkSurfaces; i++)
			D3D::safeRelease(mpWorkSurf[i]);
	}
		
	void DynaCube::deleteDeviceObjects(void)
	{
		releaseBasisTextures();
		releaseWorkSurfaces();
	}
	
	void DynaCube::invalidateDeviceObjects(void)
	{
		releaseWorkSurfaces();
	}
	
	void DynaCube::restoreDeviceObjects(void)
	{
		initWorkSurfaces();
	}
	
	void DynaCube::oneTimeSceneInit(void)
	{
	}
	
	void DynaCube::multiply(
		IDirect3DSurface9* pSrc, 
		IDirect3DTexture* pA,
		IDirect3D* pB)
	{
	}
		
	void DynaCube::renderLightProbe(
		IDirect3DCubeTexture9* pCubeTex, 
		std::vector<IDirect3DTexture9*> faces,
		Scene* pScene, 
		const Vec4& camLoc)
	{
		DebugNull(pCubeTex);
		
		D3DSURFACE_DESC desc;
		pCubeTex->GetLevelDesc(0, &desc);
		
		const int CubemapDim = desc.Width;
	
		const int minDim = Math::Min(RenderEngine::bufferManager().getBackbufferWidth(), RenderEngine::bufferManager().getBackbufferHeight());
		if (minDim < CubemapDim)
			return;
			
		RenderViewport probeViewport;
		
		D3DVIEWPORT9 viewport;
		viewport.X = 0;
		viewport.Y = 0;
		viewport.Width = CubemapDim;
		viewport.Height = CubemapDim;
		viewport.MinZ = 0.0f;
		viewport.MaxZ = 1.0f;
	
		probeViewport.setViewport(viewport);
				
		probeViewport.setSurf(RenderEngine::bufferManager().getSurface(eKBuffer));
				
		const float fAspect = 1.0f;
		const float zNear = 2.5f;
		const float zFar = 10000.0f;
		probeViewport.camera().setViewToProj(fAspect, zNear, zFar, Math::fDegToRad(90.0f));
	  						
		Vec4 camDir;
		
		RenderEngine::bufferManager().setRenderTarget(0, eKBuffer);
		RenderEngine::bufferManager().setDepthStencilSurface(eSDBuffer);					
		
		for (int i = 0; i < 6; i++)
		{
			camDir = Vec4::makeAxisVector(i >> 1, (i & 1) ? -1.0f : 1.0f);
			
			float roll = 0.0f;
			if ((i == D3DCUBEMAP_FACE_NEGATIVE_Y) || (i == D3DCUBEMAP_FACE_POSITIVE_Y))
				roll = Math::fDegToRad(-90.0f);
				
				probeViewport.camera().setWorldToView(
					Matrix44::makeCamera(camLoc, camLoc + camDir, Vec4(0, 1, 0, 0), roll)
				);
						
			probeViewport.updateMatrices();
			
			D3D::clear( 0L, NULL, D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, D3DCOLOR_ARGB(255,0,0,0), 1.0f, 0L );
			
			pScene->startOfFrame();  
				
			RenderEngine::beginScene(probeViewport, true);

			pScene->render(false);
						
			RenderEngine::endScene();
												
			IDirect3DSurface9* pCubeSurf;
			Verify(SUCCEEDED(pCubeTex->GetCubeMapSurface(static_cast<D3DCUBEMAP_FACES>(i), 0, &pCubeSurf)));
			
			D3D::setRenderTarget(0, pCubeSurf);
			D3D::setDepthStencilSurface(NULL);
			
			D3D::setRenderState(D3DRS_ZENABLE, FALSE);
			D3D::disableCulling();
			D3D::clearTextures();
					
			RenderEngine::effects().beginTechnique("CopyRadiance");
									
			RenderHelpers::renderSSQ(
					false, 
					CubemapDim / float(RenderEngine::bufferManager().getBackbufferWidth()),
					CubemapDim / float(RenderEngine::bufferManager().getBackbufferHeight())
				);
			
			RenderEngine::effects().endTechnique();
			
			RenderEngine::bufferManager().setRenderTarget(0, eKBuffer);
			RenderEngine::bufferManager().setDepthStencilSurface(eSDBuffer);					
			
			D3D::setRenderState(D3DRS_ZENABLE, TRUE);
			D3D::enableCulling();
			D3D::clearTextures();
			D3D::setPixelShader(NULL);
			D3D::setVertexShader(NULL);
			
			pCubeSurf->Release();		
		}
	}
	
} // namespace gr


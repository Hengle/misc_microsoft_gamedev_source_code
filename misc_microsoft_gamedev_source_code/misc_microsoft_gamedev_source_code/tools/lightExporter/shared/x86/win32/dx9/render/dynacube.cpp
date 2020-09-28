// File: dynacube.cpp
#include "dynacube.h"
#include "render_engine.h"
#include "scene.h"
#include "render_helpers.h"
#include "d3d_cubemap.h"

#define LOWPREC 1

#if LOWPREC
	#define COMP4_FORMAT	D3DFMT_A16B16G16R16F 
	#define COMP1_FORMAT D3DFMT_R16F
	#define BYTES_PER_COMP 2
#else
	#define COMP4_FORMAT D3DFMT_A32B32G32R32F 
	#define COMP1_FORMAT D3DFMT_R32F
	#define BYTES_PER_COMP 4
#endif

namespace gr
{
	DynaCube::DynaCube() :
		mLowLimit(3),
		mHighLimit(3)
	{
		Utils::ClearObj(mpBasisTextures);
		Utils::ClearObj(mpWorkSurf);
		Utils::ClearObj(mpWorkTex);
		Utils::ClearObj(mpIrradVolumes);
		Utils::ClearObj(mpBasisCubes);

#if 1
		Vec4 low(-2350, 536, -1600, 1);
		Vec4 high(1518, 1438, 1045, 1);
		
		mVolOrigin = (low + high) * .5f;
		mVolDim = high - mVolOrigin;
#else
		mVolOrigin = Vec4(0,126,0,1);
		mVolDim = Vec4(400,62.5f,400,0);
#endif		
	}
	
	DynaCube::~DynaCube()
	{
	}
	
	void DynaCube::setTexelComp(const D3DLOCKED_RECT& rect, int x, int y, int c, float f, int bytesPerComp, int numComps)
	{
		const int bytesPerPixel = bytesPerComp * numComps;
		const int texOfs = x * bytesPerPixel + y * rect.Pitch + c * bytesPerComp;

#if LOWPREC			
		*reinterpret_cast<uint16*>(	
			reinterpret_cast<uchar*>(rect.pBits) + texOfs	
			) = Half::FloatToHalf(f, true);
#else
		*reinterpret_cast<float*>(	
			reinterpret_cast<uchar*>(rect.pBits) + texOfs	
			) = f;
#endif			
	}
	
	void DynaCube::initSHBasisTextures(void)
	{
		for (int faceNum = 0; faceNum < 6; faceNum++)
		{
			for (int i = 0; i < NumBasisCubes; i++)
			{
				D3D::safeRelease(mpBasisCubes[i]);
				mpBasisCubes[i] = D3D::createCubeTexture(BasisTextureDim, 1, 0, COMP4_FORMAT);
			}
			
			for (int i = 0; i < NumBasisTextures; i++)
			{
				D3D::safeRelease(mpBasisTextures[faceNum][i]);
				
				mpBasisTextures[faceNum][i] = D3D::createTexture(
					BasisTextureDim,
					BasisTextureDim,
					1,
					0,
					(i < 2) ? COMP4_FORMAT : COMP1_FORMAT);
			}
		}
		
		D3DCubeMap dummyCubeMap(BasisTextureDim, BasisTextureDim);
		
		for (int faceNum = 0; faceNum < 6; faceNum++)
		{
			D3DLOCKED_RECT rects[NumBasisTextures];
			
			for (int i = 0; i < NumBasisTextures; i++)
				mpBasisTextures[faceNum][i]->LockRect(0, &rects[i], NULL, 0);
				
			D3DLOCKED_RECT cubeRects[NumBasisCubes];
			for (int i = 0; i < NumBasisCubes; i++)
				mpBasisCubes[i]->LockRect((D3DCUBEMAP_FACES)faceNum, 0, &cubeRects[i], NULL, 0);
																
			for (int x = 0; x < BasisTextureDim; x++)
			{
				for (int y = 0; y < BasisTextureDim; y++)
				{
					const Vec3 dir(dummyCubeMap.vector(faceNum, x, y));
					const Vec<9> sh(SphericalHarmonic::evaluate(dir));
										
					//    mem  coeff-1
					//    3012 
					// 0: ARGB 3012
					// 1: ARGB 7456
					
					for (int c = 0; c < 9; c++)
					{
						if (c == 0)
						{
							const float solidAngle = dummyCubeMap.sphericalArea(faceNum, x, y);
							setTexelComp(rects[2], x, y, 0, solidAngle, BYTES_PER_COMP, 1);
						}
						else
						{
							setTexelComp(rects[(c-1) >> 2], x, y, (c-1) & 3, sh[c], BYTES_PER_COMP, 4);
							
							setTexelComp(cubeRects[(c-1) >> 2], x, y, (c-1) & 3, sh[c], BYTES_PER_COMP, 4);
						}
					}
				}
			}
			
			for (int i = 0; i < NumBasisCubes; i++)
				mpBasisCubes[i]->UnlockRect((D3DCUBEMAP_FACES)faceNum, 0);
				
			for (int i = 0; i < NumBasisTextures; i++)
				mpBasisTextures[faceNum][i]->UnlockRect(0);
		}
	}
	
	void DynaCube::initWorkBuffers(void)
	{
		for (int i = 0; i < NumWorkBuffers; i++)
		{
			D3D::safeRelease(mpWorkSurf[i]);
			D3D::safeRelease(mpWorkTex[i]);
		}
										
		mpWorkTex[RadianceBuffer] = D3D::createTexture(CubeTexWidth, CubeTexHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT);
		mpWorkTex[MulBuffer] = D3D::createTexture(CubeTexWidth, CubeTexHeight, 1, D3DUSAGE_RENDERTARGET, COMP4_FORMAT, D3DPOOL_DEFAULT);
		mpWorkTex[Work1Buffer] = D3D::createTexture(CubeTexWidth, CubeTexHeight, 1, D3DUSAGE_RENDERTARGET, COMP4_FORMAT, D3DPOOL_DEFAULT);
		mpWorkTex[Work2Buffer] = D3D::createTexture(CubeTexWidth, CubeTexHeight, 1, D3DUSAGE_RENDERTARGET, COMP4_FORMAT, D3DPOOL_DEFAULT);
		mpWorkTex[CoefficientBuffer] = D3D::createTexture(16, 1,  1, D3DUSAGE_RENDERTARGET, COMP4_FORMAT, D3DPOOL_DEFAULT);

		for (int i = 0; i < NumWorkBuffers; i++)						
			mpWorkTex[i]->GetSurfaceLevel(0, &mpWorkSurf[i]);
	}
	
	void DynaCube::initIrradVolumes(void)
	{
	}
	
	void DynaCube::releaseIrradVolumes(void)
	{
		for (int i = 0; i < 9; i++)
			D3D::safeRelease(mpIrradVolumes[i]);
	}
	
	void DynaCube::initDeviceObjects(void)
	{
		initSHBasisTextures();
		initWorkBuffers();
		initIrradVolumes();
	}
	
	void DynaCube::releaseBasisTextures(void)
	{
		for (int i = 0; i < NumBasisCubes; i++)
			D3D::safeRelease(mpBasisCubes[i]);
				
		for (int f = 0; f < 6; f++)
			for (int i = 0; i < NumBasisTextures; i++)
				D3D::safeRelease(mpBasisTextures[f][i]);	
	}
	
	void DynaCube::releaseWorkBuffers(void)
	{
		for (int i = 0; i < NumWorkBuffers; i++)
		{
			D3D::safeRelease(mpWorkSurf[i]);
			D3D::safeRelease(mpWorkTex[i]);
		}
	}
		
	void DynaCube::deleteDeviceObjects(void)
	{
		releaseBasisTextures();
		releaseWorkBuffers();
		releaseIrradVolumes();
	}
	
	void DynaCube::invalidateDeviceObjects(void)
	{
		releaseWorkBuffers();
	}
	
	void DynaCube::restoreDeviceObjects(void)
	{
		initWorkBuffers();
	}
	
	void DynaCube::oneTimeSceneInit(void)
	{
	}
	
	void DynaCube::cubeMul(
		IDirect3DSurface9* pDst,
		IDirect3DTexture9* pCubemap,
		int coeffIndex)
	{
		D3D::clearTextures();
		
		//for (int i = 0; i < 6; i++)
		//	HRESULT hres = D3DXSaveTextureToFile(BigString(eVarArg, "area%i.dds", i), D3DXIFF_DDS, mpBasisTextures[i][2], NULL);		
		
		D3D::setRenderTarget(0, pDst);
		D3D::setTextureClampPoint(0, pCubemap);
								
		RenderEngine::effects().setVector("CubeFaceMulComp", 
			coeffIndex ? Vec4::makeAxisVector((coeffIndex - 1) & 3) : Vec4(0) );
					
		for (int faceNum = 0; faceNum < 6; faceNum++)
		{
			D3D::setTextureClampPoint(1, mpBasisTextures[faceNum][Math::Max(0, (coeffIndex - 1)) >> 2]);
			D3D::setTextureClampPoint(2, mpBasisTextures[faceNum][2]);
			
			RenderEngine::effects().beginTechnique(coeffIndex ? "CubeFaceMul" : "CubeFaceMulConst");
			
			RECT rect;
			
			rect.left = CubeFaceDim * faceNum;
			rect.right = rect.left + CubeFaceDim;
			rect.top = 0;
			rect.bottom = CubeTexHeight;
			
			RenderHelpers::renderSSQ(rect, rect, CubeTexWidth, CubeTexHeight);
					
			RenderEngine::effects().endTechnique();
		}

		//RenderHelpers::saveRenderTarget(pDst, "mul.dds");
				
	}
	
	void DynaCube::cubeSum(
		IDirect3DSurface9* pDst, 
		int dstX, 
		IDirect3DTexture9* pCubemap, 
		float s)
	{
		D3D::clearTextures();
		
		RenderEngine::effects().setFloat("gCubeResampleUOfs", .5f / CubeTexWidth);
		RenderEngine::effects().setFloat("gCubeResampleVOfs", .5f / CubeTexHeight);
					
		// sum by decimation
		int i;
		for (i = 0; i < Math::iLog2(CubeFaceDim); i++)
		{
			D3D::setRenderTarget(0, mpWorkSurf[Work1Buffer + (i & 1)]);
			
#if DEBUG
			D3D::clear(D3DCLEAR_TARGET, D3DCOLOR_ARGB(255,20,20,20));
#endif			
			
			if (0 == i)
				D3D::setTextureClampPoint(0, pCubemap);
			else
				D3D::setTextureClampPoint(0, mpWorkTex[Work1Buffer + ((i - 1) & 1)]);
						
			RenderEngine::effects().beginTechnique("CubeResample");
			
			RECT srcRect, dstRect;
			
			srcRect.left = 0;
			srcRect.right = CubeWidth >> i;
			srcRect.top = 0;
			srcRect.bottom = CubeFaceDim >> i;
			
			dstRect.left = 0;
			dstRect.right = CubeWidth >> (i + 1);
			dstRect.top = 0;
			dstRect.bottom = CubeFaceDim >> (i + 1);
			
			RenderHelpers::renderSSQ(dstRect, srcRect, CubeTexWidth, CubeTexHeight);
					
			RenderEngine::effects().endTechnique();
			
			//RenderHelpers::saveRenderTarget(mpWorkSurf[Work1Buffer + (i & 1)], BigString(eVarArg, "test%i.dds", i));
		}
		
		// now add final 6 texels
		D3D::setRenderTarget(0, pDst);
		D3D::setTextureClampPoint(0, mpWorkTex[Work1Buffer + ((i - 1) & 1)]);
		
		RenderEngine::effects().setFloat("gCubeResampleUOfs", 1.0f / CubeTexWidth);
		RenderEngine::effects().setFloat("gCubeResampleVOfs", 0.0f);
		RenderEngine::effects().setFloat("gCubeResampleScale", s);
		
		RenderEngine::effects().beginTechnique("CubeSum");
		
		RECT srcRect, dstRect;
		
		srcRect.left = 0;
		srcRect.right = 1;
		srcRect.top = 0;
		srcRect.bottom = 1;
		
		dstRect.left = dstX;
		dstRect.right = dstX + 1;
		dstRect.top = 0;
		dstRect.bottom = 1;
		
		RenderHelpers::renderSSQ(dstRect, srcRect, CubeTexWidth, CubeTexHeight);
				
		RenderEngine::effects().endTechnique();
		
#if 0		
		{
			UCharVec bits;
			RenderHelpers::getRenderTargetBits(bits, mpWorkSurf[MulBuffer]);
			
			float sum[3];
			Utils::ClearObj(sum);
			for (int y = 0; y < CubeTexHeight; y++)
			{
				for (int x = 0; x < CubeWidth; x++)
				{
					for (int c = 0; c < 3; c++)
					{
						float f = *reinterpret_cast<float*>(&bits[x * 16 + y * CubeTexWidth * 16 + c * 4]);
						sum[c] += f;
					}
				}
			}
			printf("%f %f %f\n", sum[0]*s, sum[1]*s, sum[2]*s);
		}
#endif		
	}
	
	void DynaCube::cubeFilter(
		IDirect3DSurface9* pDst,
		IDirect3DTexture9* pCubemap)
	{
		D3D::setDepthStencilSurface(NULL);
		D3D::setRenderState(D3DRS_ZENABLE, FALSE);
		D3D::disableCulling();
		D3D::clearTextures();
							
		const Vec<9> irradConvCoefficients(1.0f);//SphericalHarmonic::irradConvolutionCoefficients());
		
		for (int i = 0; i < 9; i++)
		{
			cubeMul(mpWorkSurf[MulBuffer], pCubemap, i);
			
			const float y00 = .282095f;
			float s = irradConvCoefficients[i];
			if (0 == i)
				s *= y00;
				
			cubeSum(pDst, i, mpWorkTex[MulBuffer], s);
		}
		
		RenderEngine::bufferManager().setRenderTarget(0, eKBuffer);
		RenderEngine::bufferManager().setDepthStencilSurface(eSDBuffer);					
			
		D3D::setRenderState(D3DRS_ZENABLE, TRUE);
		D3D::enableCulling();
		D3D::clearTextures();
		D3D::setPixelShader(NULL);
		D3D::setVertexShader(NULL);
	}
		
	SphericalHarmonic::Vec9Vector DynaCube::getSHCoeffs(IDirect3DSurface9* pSurf)
	{
		UCharVec bits;
		RenderHelpers::getRenderTargetBits(bits, pSurf);
		
		SphericalHarmonic::Vec9Vector coeffs(3);
		
		for (int c = 0; c < 3; c++)
		{
			for (int i = 0; i < 9; i++)
			{
#if LOWPREC				
				float s = Half::HalfToFloat(bits[i*8+c*2+0] | (bits[i*8+c*2+1]<<8));
#else
				float s = *reinterpret_cast<float*>(&bits[i*16+c*4]);
#endif				
			
				coeffs[c][i] = s;
			}
		}
		
		return coeffs;
	}
		
	void DynaCube::renderLightProbe(
		IDirect3DSurface9* pDst,
		Scene* pScene, 
		const Vec4& camLoc)
	{
		DebugNull(pDst);
		
		D3DSURFACE_DESC desc;
		pDst->GetDesc(&desc);
		
		const int CubemapDim = desc.Height;
	
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
						
			D3D::setRenderTarget(0, pDst);
			D3D::setDepthStencilSurface(NULL);
			
#if DEBUG			
			if (!i)
				D3D::clear(D3DCLEAR_TARGET, D3DCOLOR_ARGB(255,20,20,20));
#endif			
			
			D3D::setRenderState(D3DRS_ZENABLE, FALSE);
			D3D::disableCulling();
			D3D::clearTextures();
					
			RenderEngine::effects().beginTechnique("CopyRadiance");
										
			RECT dstRect, texRect;
			
			dstRect.left = i * CubemapDim;
			dstRect.right = dstRect.left + CubemapDim;
			dstRect.top = 0;
			dstRect.bottom = CubemapDim;
			
			texRect.left = texRect.top = 0;
			texRect.right = texRect.bottom = CubemapDim;
						
			RenderHelpers::renderSSQ(
				dstRect, 
				texRect,
				RenderEngine::bufferManager().getBackbufferWidth(),
				RenderEngine::bufferManager().getBackbufferHeight());
							
			RenderEngine::effects().endTechnique();
			
			RenderEngine::bufferManager().setRenderTarget(0, eKBuffer);
			RenderEngine::bufferManager().setDepthStencilSurface(eSDBuffer);					
			
			D3D::setRenderState(D3DRS_ZENABLE, TRUE);
			D3D::enableCulling();
			D3D::clearTextures();
			D3D::setPixelShader(NULL);
			D3D::setVertexShader(NULL);
		}
	}
	
	void DynaCube::radianceProbe(Scene* pScene, const Vec4& camLoc)
	{
		renderLightProbe(mpWorkSurf[RadianceBuffer], pScene, camLoc);
		
		//RenderHelpers::saveRenderTarget(mpWorkSurf[RadianceBuffer], "test.dds");
		
		cubeFilter(mpWorkSurf[CoefficientBuffer], mpWorkTex[RadianceBuffer]);
	}
	
	SphericalHarmonic::Vec9Vector DynaCube::getRadianceProbeSHCoeffs(void)
	{
		return getSHCoeffs(mpWorkSurf[CoefficientBuffer]);
	}
	
	void DynaCube::loadIrradVolume(void)
	{
		for (int i = 0; i < 9; i++)
		{
			D3D::safeRelease(mpIrradVolumes[i]);
	
			HRESULT hres = D3DXCreateVolumeTextureFromFileEx(
				D3D::getDevice(),
				BigString(eVarArg, "irrad%i", i), 
				D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
				1,
				0,
				D3DFMT_UNKNOWN,
				D3DPOOL_MANAGED,
				D3DX_DEFAULT,
				D3DX_DEFAULT,
				0,
				NULL,
				NULL,
				&mpIrradVolumes[i]);		
		}
		
		FILE* pFile = fopen("irrad.txt", "r");
		if (pFile)
		{
			for (int c = 0; c < 3; c++)
			{
				for (int i = 0; i < 9; i++)
					fscanf(pFile, "%f %f ", &mLowLimit[c][i], &mHighLimit[c][i]);
				fscanf(pFile, "\r\n");
			}
			fclose(pFile);
		}
	}
	
	void DynaCube::createIrradVolume(Scene* pScene)
	{
#if DEBUG	
		const int VolWidth = 4;
		const int VolHeight = 4;
		const int VolDepth = 4;
#else
		const int VolWidth  = 32;//64;
		const int VolHeight = 8;//32;
		const int VolDepth  = 32;//64;
#endif
				
		for (int i = 0; i < 9; i++)
			D3D::safeRelease(mpIrradVolumes[i]);
		
		const int NumPasses = 1;
		for (int passNum = 0; passNum < NumPasses; passNum++)
		{
			printf("Pass %i\n", passNum);
			
			std::vector<SphericalHarmonic::Vec9Vector> samples(VolWidth*VolHeight*VolDepth);
				
			for (int c = 0; c < 3; c++)
			{
				mLowLimit[c] = Vec9(Math::fNearlyInfinite);
				mHighLimit[c] = Vec9(-Math::fNearlyInfinite);
			}
			
			Matrix44 worldToVolume(Matrix44::makeTranslate(Vec4::multiply(mVolOrigin, Vec4(-1, -1, -1, 1))));
			worldToVolume *= Matrix44::makeScale(Vec4(1.0f/mVolDim.x, 1.0f/mVolDim.y, 1.0f/mVolDim.z, 1.0f));
			worldToVolume *= Matrix44::makeTranslate(Vec4(1, 1, 1, 1));
			worldToVolume *= Matrix44::makeScale(Vec4(.5, .5, .5, 1));
			const Matrix44 volumeToWorld(worldToVolume.inverse());
						
			DWORD s = GetTickCount();
								
			for (int x = 0; x < VolWidth; x++)
			{	
				for (int y = 0; y < VolHeight; y++)
				{
					for (int z = 0; z < VolDepth; z++)
					{
						Vec4 pos(
							Vec4((x + .5f) / float(VolWidth), (y + .5f) / float(VolHeight), (z + .5f) / float(VolDepth), 1) * 
								volumeToWorld);
								
						//pos = RenderEngine::sceneRenderViewport().camera().getPos();
						
						pos.setW(1.0f);
			
						radianceProbe(pScene, pos);

						SphericalHarmonic::Vec9Vector e(getRadianceProbeSHCoeffs());
											
						samples.at(z*VolWidth*VolHeight+y*VolWidth+x) = e;
						
						for (int c = 0; c < 3; c++)
						{
							mLowLimit[c] = Vec9::elementMin(mLowLimit[c], e[c]);
							mHighLimit[c] = Vec9::elementMax(mHighLimit[c], e[c]);
						}
					}
				}
				printf("%i of %i\n", x, VolWidth);
			}

	#if 1		
			for (int c = 0; c < 3; c++)
			{
				for (int i = 0; i < 9; i++)
					printf("[%f %f] ", mLowLimit[c][i], mHighLimit[c][i]);
				printf("\n");
			}
				
			DWORD e = GetTickCount();
			printf("%f secs\n", float(e - s) * .001f);
	#endif		
			
			for (int i = 0; i < 9; i++)
			{
				D3D::safeRelease(mpIrradVolumes[i]);
				if (!mpIrradVolumes[i])
				{
					mpIrradVolumes[i] = D3D::createVolumeTexture(
						VolWidth, VolHeight, VolDepth, 1, 0, 
						//D3DFMT_A2B10G10R10, 
						D3DFMT_A16B16G16R16,
						D3DPOOL_MANAGED);
				}
					
				D3DLOCKED_BOX box;
				mpIrradVolumes[i]->LockBox(
					0,
					&box,
					NULL,
					0);
					
				const DWORD bpp = sizeof(uint16) * 4;
				for (int z = 0; z < VolDepth; z++)
				{
					for (int y = 0; y < VolHeight; y++)
					{
						for (int x = 0; x < VolWidth; x++)
						{
							uint16* pPixel = reinterpret_cast<uint16*>(
								reinterpret_cast<uchar*>(box.pBits) + x*bpp + y*box.RowPitch + z*box.SlicePitch
								);
							
							const SphericalHarmonic::Vec9Vector& c = samples[x + y * VolWidth + z * VolWidth * VolHeight];
																			
							float s[3];
							for (int j = 0; j < 3; j++)
							{
								float range = (mHighLimit[j][i] - mLowLimit[j][i]);
								if (fabs(range) < Math::fSmallEpsilon)
									s[j] = 0;
								else
									s[j] = (c[j][i] - mLowLimit[j][i]) / range;
							}
							
							pPixel[0] = Math::Clamp(Math::FloatToIntRound(65535.0f * s[0]),0,65535);
							pPixel[1] = Math::Clamp(Math::FloatToIntRound(65535.0f * s[1]),0,65535);
							pPixel[2] = Math::Clamp(Math::FloatToIntRound(65535.0f * s[2]),0,65535);
							pPixel[3] = 0.0f;

	#if 0																									
							*pPixel = (Math::Clamp(Math::FloatToIntRound(1023.0f * ((a==3)?s[0]:t[0])),0,1023)    ) |
												(Math::Clamp(Math::FloatToIntRound(1023.0f * ((a==3)?s[1]:t[1])),0,1023)<<10) |
												(Math::Clamp(Math::FloatToIntRound(1023.0f * ((a==3)?s[2]:t[2])),0,1023)<<20) |
												(Math::Clamp(a, 0, 3) << 30);
	#endif											
						}
					}
				}
					
				mpIrradVolumes[i]->UnlockBox(0);
			}
		}

		for (int i = 0; i < 9; i++)
		{
			IDirect3DVolume9* pVol;
			mpIrradVolumes[i]->GetVolumeLevel(0, &pVol);
			
			HRESULT hres = D3DXSaveVolumeToFile(
				BigString(eVarArg, "irrad%i", i), 
				D3DXIFF_DDS,
				pVol,
				NULL,
				NULL);
			
			pVol->Release();
		}
		
		FILE* pFile = fopen("irrad.txt", "w");
		for (int c = 0; c < 3; c++)
		{
			for (int i = 0; i < 9; i++)
				fprintf(pFile, "%f %f ", mLowLimit[c][i], mHighLimit[c][i]);
			fprintf(pFile, "\r\n");
		}
		fclose(pFile);
	}
	
	void DynaCube::renderIndirectLight(void)
	{
		if (!mpIrradVolumes[0])
			return;
			
		D3D::clearTextures();
														
		Matrix44 worldToVolume(Matrix44::makeTranslate(Vec4::multiply(mVolOrigin, Vec4(-1, -1, -1, 1))));
		worldToVolume *= Matrix44::makeScale(Vec4(1.0f/mVolDim.x, 1.0f/mVolDim.y, 1.0f/mVolDim.z, 1.0f));
		worldToVolume *= Matrix44::makeTranslate(Vec4(1, 1, 1, 1));
		worldToVolume *= Matrix44::makeScale(Vec4(.5, .5, .5, 1));								
		Matrix44 viewToVolume(RenderEngine::sceneRenderViewport().getMatrix(eViewToWorld) * worldToVolume);
		
		RenderEngine::effects().setMatrix("gCalcIndirectLightViewToVolume", viewToVolume);
		RenderEngine::effects().setMatrix("gCalcIndirectLightViewToWorld", D3D::getMatrix(eViewToWorld));
		
		D3D::setRenderTarget(1, NULL);
				
		int dstIndex = 0;
		
		Vec4 lMul[9], lAdd[9];
		
		for (int i = 0; i < 9; i++)
		{
			D3D::setTextureClampLinear(i, mpIrradVolumes[i]);
			D3D::setSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
		}
		
		for (int l = 0; l <= 2; l++)
		{
			for (int m = -l; m <= l; m++)
			{
				const int srcIndex = SphericalHarmonic::index(l, m);
												
				lMul[dstIndex] = Vec4(
					mHighLimit[0][srcIndex] - mLowLimit[0][srcIndex],
					mHighLimit[1][srcIndex] - mLowLimit[1][srcIndex],
					mHighLimit[2][srcIndex] - mLowLimit[2][srcIndex],
					0.0f);
					
				lAdd[dstIndex] = Vec4(
					mLowLimit[0][srcIndex],
					mLowLimit[1][srcIndex],
					mLowLimit[2][srcIndex],
					0.0f);
				
				dstIndex++;
			}
		}
		
		lMul[0] *= .282095f * 3.141593f;		lAdd[0] *= .282095f * 3.141593f;
		lMul[1] *= -.488603f * 2.094395f;		lAdd[1] *= -.488603f * 2.094395f;
		lMul[2] *= .488603f * 2.094395f;		lAdd[2] *= .488603f * 2.094395f;
		lMul[3] *= -.488603f * 2.094395f;		lAdd[3] *= -.488603f * 2.094395f;
		lMul[4] *= .785398f * 1.092548f;		lAdd[4] *= .785398f * 1.092548f;
		lMul[5] *= -.785398f * 1.092548f;		lAdd[5] *= -.785398f * 1.092548f;
		lMul[6] *= .785398f * .315392f;			lAdd[6] *= .785398f * .315392f;
		lMul[7] *= -.785398f * 1.092548f;		lAdd[7] *= -.785398f * 1.092548f;
		lMul[8] *= .785398f * .546274f;			lAdd[8] *= .785398f * .546274f;
								
		RenderEngine::effects().setVectorArray("gLMul", lMul, 9);
		RenderEngine::effects().setVectorArray("gLAdd", lAdd, 9);
		
		D3D::setRenderState(D3DRS_ZENABLE, FALSE);
										
		RenderEngine::effects().beginTechnique("IndirectLight");
						
		RenderHelpers::renderSSQ(true);
						
		RenderEngine::effects().endTechnique();	
		
		D3D::setRenderState(D3DRS_ZENABLE, TRUE);
		
		D3D::clearTextures();
		D3D::setPixelShader(NULL);
		D3D::setVertexShader(NULL);
		
		RenderEngine::bufferManager().setRenderTarget(1, eSABuffer);
	}
	
	void DynaCube::renderIndirectLight2(void)
	{
		if (!mpIrradVolumes[0])
			return;
			
		D3D::clearTextures();
										
		Matrix44 worldToVolume(Matrix44::makeTranslate(Vec4::multiply(mVolOrigin, Vec4(-1, -1, -1, 1))));
		worldToVolume *= Matrix44::makeScale(Vec4(1.0f/mVolDim.x, 1.0f/mVolDim.y, 1.0f/mVolDim.z, 1.0f));
		worldToVolume *= Matrix44::makeTranslate(Vec4(1, 1, 1, 1));
		worldToVolume *= Matrix44::makeScale(Vec4(.5, .5, .5, 1));								
		Matrix44 viewToVolume(RenderEngine::sceneRenderViewport().getMatrix(eViewToWorld) * worldToVolume);
		
		RenderEngine::effects().setMatrix("gCalcIndirectLightViewToVolume", viewToVolume);
		RenderEngine::effects().setMatrix("gCalcIndirectLightViewToWorld", D3D::getMatrix(eViewToWorld));
		
		D3D::setRenderTarget(1, NULL);
				
		int dstIndex = 0;
		
		Vec4 lMul[9], lAdd[9];
		
		for (int i = 0; i < 9; i++)
		{
			D3D::setTextureClampLinear(i, mpIrradVolumes[i]);
			//D3D::setTextureClampPoint(i, mpIrradVolumes[i]);
			D3D::setSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
		}
		
		D3D::setTextureClampPoint(9, mpBasisCubes[0]);
		D3D::setTextureClampPoint(10, mpBasisCubes[1]);
		
		for (int l = 0; l <= 2; l++)
		{
			for (int m = -l; m <= l; m++)
			{
				const int srcIndex = SphericalHarmonic::index(l, m);
												
				lMul[dstIndex] = Vec4(
					mHighLimit[0][srcIndex] - mLowLimit[0][srcIndex],
					mHighLimit[1][srcIndex] - mLowLimit[1][srcIndex],
					mHighLimit[2][srcIndex] - mLowLimit[2][srcIndex],
					0.0f);
					
				lAdd[dstIndex] = Vec4(
					mLowLimit[0][srcIndex],
					mLowLimit[1][srcIndex],
					mLowLimit[2][srcIndex],
					0.0f);
				
				dstIndex++;
			}
		}
		
		lMul[0] *= .282095f * 3.141593f;		lAdd[0] *= .282095f * 3.141593f;
		lMul[1] *= 2.094395f;		lAdd[1] *= 2.094395f;
		lMul[2] *= 2.094395f;		lAdd[2] *= 2.094395f;
		lMul[3] *= 2.094395f;		lAdd[3] *= 2.094395f;
		lMul[4] *= .785398f;		lAdd[4] *= .785398f;
		lMul[5] *= .785398f;		lAdd[5] *= .785398f;
		lMul[6] *= .785398f;		lAdd[6] *= .785398f;
		lMul[7] *= .785398f;		lAdd[7] *= .785398f;
		lMul[8] *= .785398f;		lAdd[8] *= .785398f;
												
		RenderEngine::effects().setVectorArray("gLMul", lMul, 9);
		RenderEngine::effects().setVectorArray("gLAdd", lAdd, 9);
		
		D3D::setRenderState(D3DRS_ZENABLE, FALSE);
										
		RenderEngine::effects().beginTechnique("IndirectLight2");
						
		RenderHelpers::renderSSQ(true);
						
		RenderEngine::effects().endTechnique();	
		
		D3D::setRenderState(D3DRS_ZENABLE, TRUE);
		
		D3D::clearTextures();
		D3D::setPixelShader(NULL);
		D3D::setVertexShader(NULL);
		
		RenderEngine::bufferManager().setRenderTarget(1, eSABuffer);
	}
			
} // namespace gr










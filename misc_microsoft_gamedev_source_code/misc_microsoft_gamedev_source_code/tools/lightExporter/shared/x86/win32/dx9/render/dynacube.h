// File: dynacube.h
#pragma once
#ifndef DYNACUBE_H
#define DYNACUBE_H

#include "d3d9.h"
#include "common/math/vector.h"
#include "common/render/spherical_harmonic.h"

namespace gr
{
	class Scene;
	
	class DynaCube
	{
	public:
		DynaCube();
		~DynaCube();
				
		void initDeviceObjects(void);
		void deleteDeviceObjects(void);
		void invalidateDeviceObjects(void);
		void restoreDeviceObjects(void);
		void oneTimeSceneInit(void);
		
		void renderLightProbe(
			IDirect3DSurface9* pDst,
			Scene* pScene, 
			const Vec4& camLoc);
			
		void radianceProbe(Scene* pScene, const Vec4& camLoc);
		SphericalHarmonic::Vec9Vector getRadianceProbeSHCoeffs(void);
		void createIrradVolume(Scene* pScene);
		
		void renderIndirectLight(void);
		void renderIndirectLight2(void);
		void loadIrradVolume(void);
	
	private:
		enum { BasisTextureDim = 64 };
		enum { NumBasisTextures = 3 };
		IDirect3DTexture9* mpBasisTextures[6][NumBasisTextures];
		
		enum { NumBasisCubes = 2 };
		IDirect3DCubeTexture9* mpBasisCubes[2];
		
		IDirect3DVolumeTexture9* mpIrradVolumes[9];
		SphericalHarmonic::Vec9Vector mLowLimit;
		SphericalHarmonic::Vec9Vector mHighLimit;		
		
		Vec4 mVolOrigin;
		Vec4 mVolDim;
				
		enum { 
			CubeFaceDim = 64, 
			CubeWidth = CubeFaceDim * 6,
			CubeTexWidth = CubeFaceDim * 8,
			CubeTexHeight = CubeFaceDim
			 };
		
		enum
		{
			RadianceBuffer,
			MulBuffer,
			Work1Buffer,
			Work2Buffer,
			CoefficientBuffer,												
			NumWorkBuffers 
		};
		
		IDirect3DTexture9* mpWorkTex[NumWorkBuffers];
		IDirect3DSurface9* mpWorkSurf[NumWorkBuffers];

		void initSHBasisTextures(void);
		void initWorkBuffers(void);
		void releaseBasisTextures(void);
		void releaseWorkBuffers(void);
		void cubeMul(
			IDirect3DSurface9* pDst,
			IDirect3DTexture9* pCubemap,
			int ACCoeffIndex);
		void cubeSum(
			IDirect3DSurface9* pDst, 
			int dstX, 
			IDirect3DTexture9* pCubemap, 
			float s);
		void cubeFilter(
			IDirect3DSurface9* pDst,
			IDirect3DTexture9* pCubemap);
			
		SphericalHarmonic::Vec9Vector getSHCoeffs(IDirect3DSurface9* pSurf);
		
		void initIrradVolumes(void);
		void releaseIrradVolumes(void);
				
		static void setTexelComp(const D3DLOCKED_RECT& rect, int x, int y, int c, float f, int bytesPerComp, int numComps);
	};
	
} // namespace gr

#endif // DYNACUBE_H

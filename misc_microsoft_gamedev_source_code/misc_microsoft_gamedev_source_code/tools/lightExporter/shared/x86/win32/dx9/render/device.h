//-----------------------------------------------------------------------------
// File: device.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef DEVICE_H
#define DEVICE_H

#include <d3d9.h>

#include "state_cache.h"
#include "matrix_tracker.h"

#include "common/core/core.h"
#include "common/math/vector.h"

namespace gr
{
	class D3D
	{
	public:
		// # of texture stages
		//enum { NumTextures						= 8 };
		//enum { NumTexturesLog2				= 3 };
		
		enum { NumTextures						= 16 };
		enum { NumTexturesLog2				= 4 };
		
		// # of samplers
		enum { NumSamplers						= 16 };
		enum { NumSamplersLog2				= 4  };
		
		enum { MaxRenderTargets				= 4 };

		static void errCheck(HRESULT hres)
		{
			hres;
			Assert(SUCCEEDED(hres));
		}
		
		static void errCheckFail(HRESULT hres)
		{
			hres;
			Verify(SUCCEEDED(hres));
		}

		template<class T>	static void safeRelease(T*& p)	
		{	
			if (p) 
			{ 
				p->Release(); 
				p = NULL; 
			}
		}

		template<class T>	static void safeReleaseVec(T& vec)
		{
			for (T::iterator it = vec.begin(); it != vec.end(); ++it)
				D3D::safeRelease(*it);
		}

		static IDirect3DDevice9* getDevice(void)
		{
			return mpDev;
    }

		static StateCache& getCache(void)
		{
			return mStateCache;
		}

		static const MatrixTracker& getMatrixTracker(void)
		{
			return mMatrixTracker;
		}

		static const Matrix44& getMatrix(EMatrix matrixIndex, bool transposed = false)
		{
			return mMatrixTracker.getMatrix(matrixIndex, transposed);
		}
				
		static void setDevice(IDirect3DDevice9* pDev)
		{
			mpDev = pDev;

			if (pDev)
			{
				syncMatrixTracker();
				updateRenderTarget();
				invalidateCache();
			}
		}
		
		static void setTextureUncached(DWORD stage, IDirect3DBaseTexture9* pTex)
		{
			errCheck(mpDev->SetTexture(stage, pTex));
		}

		static void setRenderStateUncached(D3DRENDERSTATETYPE State, DWORD Value)
		{
			errCheck(mpDev->SetRenderState(State, Value));
		}

		static void setTextureStageStateUncached(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
		{
			errCheck(mpDev->SetTextureStageState(Stage, Type, Value));
		}

		static void setSamplerStateUncached(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
		{
			errCheck(mpDev->SetSamplerState(Sampler, Type, Value));
		}

		static void setTexture(DWORD stage, IDirect3DBaseTexture9* pTex)
		{
			mStateCache.setTexture(stage, pTex);
		}

		// helper: sets texture, addressing to clamp and filtering to point, SRGB
		static void setTextureClampPoint(DWORD stage, IDirect3DBaseTexture9* pTex, bool SRGBTexture = false)
		{
			mStateCache.setTexture(stage, pTex);
			D3D::setClampAddressing(stage);
			D3D::setPointFiltering(stage);
			D3D::setSRGBTexture(stage, SRGBTexture);
    }

		// helper: sets texture, addressing to clamp and filtering to linear, SRGB
		static void setTextureClampLinear(DWORD stage, IDirect3DBaseTexture9* pTex, bool SRGBTexture = false)
		{
			mStateCache.setTexture(stage, pTex);
			D3D::setClampAddressing(stage);
			D3D::setLinearFiltering(stage);
			D3D::setSRGBTexture(stage, SRGBTexture);
		}
		
		static void setTextureWrapLinear(DWORD stage, IDirect3DBaseTexture9* pTex, bool SRGBTexture = false)
		{
			mStateCache.setTexture(stage, pTex);
			D3D::setWrapAddressing(stage);
			D3D::setLinearFiltering(stage);
			D3D::setSRGBTexture(stage, SRGBTexture);
		}

		static void setRenderState(D3DRENDERSTATETYPE State, DWORD Value)
		{
			mStateCache.setRenderState(State, Value);
		}

		static void setTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
		{
			mStateCache.setTextureStageState(Stage, Type, Value);
		}

		static void setSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
		{
			mStateCache.setSamplerState(Sampler, Type, Value);
		}

		static void setWrapAddressing(DWORD Sampler)
		{
			setSamplerState(Sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
			setSamplerState(Sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
			setSamplerState(Sampler, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
		}

		static void setClampAddressing(DWORD Sampler)
		{
			setSamplerState(Sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			setSamplerState(Sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
			setSamplerState(Sampler, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
		}

		static void setPointFiltering(DWORD Sampler)
		{
			setSamplerState(Sampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			setSamplerState(Sampler, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			setSamplerState(Sampler, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
		}

		static void setLinearFiltering(DWORD Sampler)
		{
			setSamplerState(Sampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			setSamplerState(Sampler, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			setSamplerState(Sampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		}

		static void setAnisotropicFiltering(DWORD Sampler)
		{
			setSamplerState(Sampler, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
			setSamplerState(Sampler, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
			setSamplerState(Sampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		}

		static void setMaxAnisotropy(DWORD Sampler, DWORD maxAniso)
		{
			setSamplerState(Sampler, D3DSAMP_MAXANISOTROPY, maxAniso);
		}

		static void setSRGBTexture(DWORD Sampler, bool SRGBTexture)
		{
			setSamplerState(Sampler, D3DSAMP_SRGBTEXTURE, SRGBTexture);
		}

		static void disableCulling(void)
		{
			setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		}

		// D3D default
		static void enableCullingCCW(void)	
		{
			setRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		}
		
		static void enableCullingCW(void)
		{
			setRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		}

		static void enableCulling(void)	
		{
			enableCullingCCW();
		}

		static void flushCache(void)
		{
			mStateCache.flush();
		}

		static void invalidateCache(void)
		{
			mStateCache.invalidate();
			resetCachedState();
		}

		static DWORD getRenderStateUncached(D3DRENDERSTATETYPE State)
		{
			DWORD ret;
			errCheck(mpDev->GetRenderState(State, &ret));
			return ret;
		}

		static DWORD getTextureStageStateUncached(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type)
		{
	    DWORD ret;
			errCheck(mpDev->GetTextureStageState(Stage, Type, &ret));
			return ret;
		}

		static IDirect3DBaseTexture9* getTextureUncached(DWORD Stage)
		{
			IDirect3DBaseTexture9* pRet;
			errCheck(mpDev->GetTexture(Stage, &pRet));
			return pRet;
		}

		static drawIndexedPrimitive(
			D3DPRIMITIVETYPE Type,
			INT BaseVertexIndex,
			UINT MinIndex,
			UINT NumVertices,
			UINT StartIndex,
			UINT PrimitiveCount)
		{
			flushCache();
			errCheck(mpDev->DrawIndexedPrimitive(Type, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount));
		}

		static drawIndexedPrimitiveUP(
			D3DPRIMITIVETYPE PrimitiveType,
			UINT MinVertexIndex,
			UINT NumVertexIndices,
			UINT PrimitiveCount,
			const void *pIndexData,
			D3DFORMAT IndexDataFormat,
			CONST void* pVertexStreamZeroData,
			UINT VertexStreamZeroStride)
		{
			flushCache();
			errCheck(mpDev->DrawIndexedPrimitiveUP(
				PrimitiveType,
				MinVertexIndex,
				NumVertexIndices,
				PrimitiveCount,
				pIndexData,
				IndexDataFormat,
				pVertexStreamZeroData,
				VertexStreamZeroStride));
		}

		static drawPrimitive(
			D3DPRIMITIVETYPE PrimitiveType,
			UINT StartVertex,
			UINT PrimitiveCount)
		{
			flushCache();
			errCheck(mpDev->DrawPrimitive(
				PrimitiveType,
				StartVertex,
				PrimitiveCount));
		}

		static drawPrimitiveUP(          
			D3DPRIMITIVETYPE PrimitiveType,
			UINT PrimitiveCount,
			const void *pVertexStreamZeroData,
			UINT VertexStreamZeroStride)
		{
			flushCache();
			errCheck(mpDev->DrawPrimitiveUP(          
				PrimitiveType,
				PrimitiveCount,
				pVertexStreamZeroData,
				VertexStreamZeroStride));
		}

		static void beginScene(void)
		{
			errCheck(mpDev->BeginScene());
		}

		static void endScene(void)
		{
			errCheck(mpDev->EndScene());
		}

		static void clear(
			DWORD Count,
			const D3DRECT *pRects,
			DWORD Flags,
			D3DCOLOR Color = 0,
			float Z = 0,
			DWORD Stencil = 0)
		{
			flushCache();
			errCheck(mpDev->Clear(Count, pRects, Flags, Color, Z, Stencil));
		}

		static void clear(
			DWORD Flags,
			D3DCOLOR Color = 0,
			float Z = 0.0f,
			DWORD Stencil = 0)
		{
			clear(0, NULL, Flags, Color, Z, Stencil);
		}

		static IDirect3DCubeTexture9* createCubeTexture(          
			UINT EdgeLength,
			UINT Levels = 1,
			DWORD Usage = 0,
			D3DFORMAT Format = D3DFMT_A8R8G8B8,
			D3DPOOL Pool = D3DPOOL_MANAGED)
		{
			invalidateCache();
			IDirect3DCubeTexture9* pRet;
			errCheckFail(mpDev->CreateCubeTexture(          
				EdgeLength,
				Levels,
				Usage,
				Format,
				Pool,
				&pRet,
				NULL));
			return pRet;
		}

		static IDirect3DSurface9* createDepthStencilSurface(          
			UINT Width,
			UINT Height,
			D3DFORMAT Format,
			D3DMULTISAMPLE_TYPE MultiSample,
			DWORD MultisampleQuality,
			BOOL Discard)
   	{
   		invalidateCache();
			IDirect3DSurface9* pRet;
			errCheckFail(mpDev->CreateDepthStencilSurface(          
				Width,
				Height,
				Format,
				MultiSample,
				MultisampleQuality,
				Discard,
				&pRet,
				NULL));
			return pRet;
		}

		static IDirect3DIndexBuffer9* createIndexBuffer(          
			UINT Length,
			DWORD Usage,
			D3DFORMAT Format,
			D3DPOOL Pool)
		{
			invalidateCache();
			IDirect3DIndexBuffer9* pRet;
			errCheckFail(mpDev->CreateIndexBuffer(
				Length,
				Usage,
				Format,
				Pool,
				&pRet,
				NULL));
			return pRet;
		}

		static IDirect3DPixelShader9* createPixelShader(
			CONST DWORD *pFunction)
		{
			invalidateCache();
			IDirect3DPixelShader9* pRet;
			errCheckFail(mpDev->CreatePixelShader(pFunction, &pRet));
			return pRet;
		}

		static IDirect3DQuery9* createQuery(
			D3DQUERYTYPE Type)
		{
			invalidateCache();
			IDirect3DQuery9* pRet;
			errCheckFail(mpDev->CreateQuery(
				Type,
				&pRet));
			return pRet;
		}

		static IDirect3DSurface9* createRenderTarget(          
			UINT Width,
			UINT Height,
			D3DFORMAT Format,
			D3DMULTISAMPLE_TYPE MultiSample,
			DWORD MultisampleQuality,
			BOOL Lockable)
		{
			invalidateCache();
			IDirect3DSurface9* pRet;
			errCheckFail(mpDev->CreateRenderTarget(
				Width,
				Height,
				Format,
				MultiSample,
				MultisampleQuality,
				Lockable,
				&pRet,
				NULL));
			return pRet;
		}

		static IDirect3DTexture9* createTexture(          
			UINT Width,
			UINT Height,
			UINT Levels = 1,
			DWORD Usage = 0,
			D3DFORMAT Format = D3DFMT_A8R8G8B8,
			D3DPOOL Pool = D3DPOOL_MANAGED)
		{
			invalidateCache();
			IDirect3DTexture9* pRet;
			errCheckFail(mpDev->CreateTexture(
				Width,
				Height,
				Levels,
				Usage,
				Format,
				Pool,
				&pRet,
				NULL));
			return pRet;
		}

		static IDirect3DVertexBuffer9* createVertexBuffer(          
			UINT Length,
			DWORD Usage,
			DWORD FVF,
			D3DPOOL Pool)
		{
			invalidateCache();
			IDirect3DVertexBuffer9* pRet;
			errCheckFail(mpDev->CreateVertexBuffer(          
				Length,
				Usage,
				FVF,
				Pool,
				&pRet,
				NULL));
			return pRet;
		}

		static IDirect3DVertexDeclaration9* createVertexDeclaration(          
			CONST D3DVERTEXELEMENT9* pVertexElements)
		{
			invalidateCache();
			IDirect3DVertexDeclaration9* pRet;
			errCheckFail(mpDev->CreateVertexDeclaration(          
				pVertexElements,
				&pRet));
			return pRet;
		}

		static IDirect3DVertexShader9* createVertexShader(          
			const DWORD *pFunction)
		{
			invalidateCache();
			IDirect3DVertexShader9* pRet;
			errCheckFail(mpDev->CreateVertexShader(
				pFunction,
				&pRet));
			return pRet;
		}

		static IDirect3DVolumeTexture9* createVolumeTexture(          
			UINT Width,
			UINT Height,
			UINT Depth,
			UINT Levels,
			DWORD Usage,
			D3DFORMAT Format,
			D3DPOOL Pool)
		{
			invalidateCache();
			IDirect3DVolumeTexture9* pRet;
			errCheckFail(mpDev->CreateVolumeTexture(
				Width,
				Height,
				Depth,
				Levels,
				Usage,
				Format,
				Pool,
				&pRet,
				NULL));
			return pRet;
		}

		static IDirect3DSurface9* getBackBuffer(          
			UINT iSwapChain,
			UINT BackBuffer)
		{
			IDirect3DSurface9* pRet;
			errCheck(mpDev->GetBackBuffer(
				iSwapChain,
				BackBuffer,
				D3DBACKBUFFER_TYPE_MONO,
				&pRet));
			return pRet;
		}

		static IDirect3DSurface9* getDepthStencilSurface(void)
		{
			IDirect3DSurface9* pRet;
			errCheck(mpDev->GetDepthStencilSurface(&pRet));
			return pRet;
		}

		static void getRasterStatus(          
			UINT iSwapChain,
			D3DRASTER_STATUS *pRasterStatus)
		{
			errCheck(mpDev->GetRasterStatus(iSwapChain, pRasterStatus));
		}

		static void present(          
			CONST RECT *pSourceRect,
			CONST RECT *pDestRect,
			HWND hDestWindowOverride,
			CONST RGNDATA *pDirtyRegion)
		{
			errCheck(mpDev->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion));
		}

		static void setDepthStencilSurface(IDirect3DSurface9* pSurf)
		{
			if (mpCurDepthStencilSurf == pSurf)
				return;
				
			mpCurDepthStencilSurf = pSurf;
				
			errCheck(mpDev->SetDepthStencilSurface(pSurf));
		}

		static void setFVF(DWORD FVF)
		{
			mpCurDecl = NULL;
			
			errCheck(mpDev->SetFVF(FVF));
		}

		static void setGammaRamp(          
			UINT iSwapChain,
			DWORD Flags,
			CONST D3DGAMMARAMP *pRamp)
		{
			mpDev->SetGammaRamp(iSwapChain, Flags, pRamp);
		}

		static void setIndices(IDirect3DIndexBuffer9 *pIndexData)
		{
			errCheck(mpDev->SetIndices(pIndexData));
		}

		static void setPixelShader(IDirect3DPixelShader9* pShader)
		{
			if (pShader == mpCurPShader)
				return;
			
			mpCurPShader = pShader;
				
			errCheck(mpDev->SetPixelShader(pShader));
		}
		
		static void setPixelShaderConstantB(
			UINT StartRegister,
			CONST BOOL *pConstantData,
			UINT BoolCount)
		{
			errCheck(mpDev->SetPixelShaderConstantB(StartRegister, pConstantData, BoolCount));
		}

		static void setPixelShaderConstantF(          
			UINT StartRegister,
			CONST float *pConstantData,
			UINT Vector4fCount)
		{
			errCheck(mpDev->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount));
		}

		static void setPixelShaderConstantF(          
			UINT StartRegister,
			CONST Vec4& v)
		{
			setPixelShaderConstantF(StartRegister, reinterpret_cast<const float*>(&v), 1);
		}

		static void setPixelShaderConstantF(          
			UINT StartRegister,
			CONST Matrix44& m)
		{
			setPixelShaderConstantF(StartRegister, reinterpret_cast<const float*>(&m), 4);
		}
		
		static void setPixelShaderConstantF(          
			UINT StartRegister,
			EMatrix matrixIndex, 
			bool transposed = false)
		{
			setPixelShaderConstantF(StartRegister, getMatrixTracker().getMatrix(matrixIndex, transposed));
		}

		static void setPixelShaderConstantI(          
			UINT StartRegister,
			CONST int *pConstantData,
			UINT Vector4iCount)
		{
			errCheck(mpDev->SetPixelShaderConstantI(StartRegister, pConstantData,	Vector4iCount));
		}

		static void setScissorRect(CONST RECT *pRect)
		{
			errCheck(mpDev->SetScissorRect(pRect));
		}

		static void setStreamSource(          
			UINT StreamNumber,
			IDirect3DVertexBuffer9 *pStreamData,
			UINT OffsetInBytes,
			UINT Stride)
		{
			errCheck(mpDev->SetStreamSource(
				StreamNumber,
				pStreamData,
				OffsetInBytes,
				Stride));
		}
				
		static void setTransform(          
			D3DTRANSFORMSTATETYPE State,
			CONST D3DMATRIX *pMatrix)
		{
			errCheck(mpDev->SetTransform(State, pMatrix));

			Matrix44 t;
			memcpy(&t, pMatrix, sizeof(Matrix44));
			
			switch (State)
			{
				case D3DTS_WORLD:
					mMatrixTracker.setMatrix(eModelToWorld, t);
					break;
				case D3DTS_VIEW:
					mMatrixTracker.setMatrix(eWorldToView, t);
					break;
				case D3DTS_PROJECTION:
					mMatrixTracker.setMatrix(eViewToProj, t);
					break;
				default:
					Assert(false);
			}
		}

		static void setTransform(          
			D3DTRANSFORMSTATETYPE State,
			const Matrix44& matrix)
		{
			errCheck(mpDev->SetTransform(State, reinterpret_cast<const D3DMATRIX*>(&matrix)));

			switch (State)
			{
				case D3DTS_WORLD:
					mMatrixTracker.setMatrix(eModelToWorld, matrix);
					break;
				case D3DTS_VIEW:
					mMatrixTracker.setMatrix(eWorldToView, matrix);
					break;
				case D3DTS_PROJECTION:
					mMatrixTracker.setMatrix(eViewToProj, matrix);
					break;
				default:
					Assert(false);
			}
		}

		static void setTransformUntracked(
			D3DTRANSFORMSTATETYPE State,
			const Matrix44& matrix)
		{
			errCheck(mpDev->SetTransform(State, reinterpret_cast<const D3DMATRIX*>(&matrix)));
		}

		static void setVertexDeclaration(
			IDirect3DVertexDeclaration9 *pDecl)
		{
			if (pDecl == mpCurDecl)
				return;
		
			mpCurDecl = pDecl;
				
			errCheck(mpDev->SetVertexDeclaration(pDecl));
		}

		static void setVertexShader(          
			IDirect3DVertexShader9* pShader)
		{
			if (pShader == mpCurVShader)
				return;
				
			mpCurVShader = pShader;
			
			errCheck(mpDev->SetVertexShader(pShader));
		}

		static void setVertexShaderConstantB(          
			UINT StartRegister,
			CONST BOOL *pConstantData,
			UINT BoolCount)
		{
			errCheck(mpDev->SetVertexShaderConstantB(StartRegister, pConstantData, BoolCount));
		}

		static void setVertexShaderConstantF(          
			UINT StartRegister,
			CONST float *pConstantData,
			UINT Vector4fCount)
		{
			errCheck(mpDev->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount));
		}

		static void setVertexShaderConstantF(          
			UINT StartRegister,
			CONST Vec4& v)
		{
			setVertexShaderConstantF(StartRegister, reinterpret_cast<const float*>(&v), 1);
		}

		static void setVertexShaderConstantF(          
			UINT StartRegister,
			CONST Matrix44& m)
		{
			setVertexShaderConstantF(StartRegister, reinterpret_cast<const float*>(&m), 4);
		}
		
		static void setVertexShaderConstantF(          
			UINT StartRegister,
			EMatrix matrixIndex, 
			bool transposed = false)
		{
			setVertexShaderConstantF(StartRegister, getMatrixTracker().getMatrix(matrixIndex, transposed));
		}
				
		static void setVertexShaderConstantI(          
			UINT StartRegister,
			CONST int *pConstantData,
			UINT Vector4iCount)
		{
			errCheck(mpDev->SetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount));
		}
				
		static void stretchRect(          
			IDirect3DSurface9 *pSourceSurface,
			CONST RECT *pSourceRect,
			IDirect3DSurface9 *pDestSurface,
			CONST RECT *pDestRect,
			D3DTEXTUREFILTERTYPE Filter)
		{
			errCheck(mpDev->StretchRect(
				pSourceSurface,
				pSourceRect,
				pDestSurface,
				pDestRect,
				Filter));
		}

		static DWORD getSamplerStateUncached(
			DWORD Sampler,
			D3DSAMPLERSTATETYPE Type)
		{
			DWORD ret;
			errCheck(mpDev->GetSamplerState(Sampler, Type, &ret));
			return ret;
		}

		static void getTransform(
			D3DTRANSFORMSTATETYPE State,
			D3DMATRIX* pMatrix)
		{
			errCheck(mpDev->GetTransform(State, pMatrix));
		}

		static void getTransform(
			D3DTRANSFORMSTATETYPE State,
			Matrix44& matrix)
		{
			errCheck(mpDev->GetTransform(State, reinterpret_cast<D3DMATRIX*>(&matrix)));
		}

		static Matrix44 getTransform(D3DTRANSFORMSTATETYPE State)
		{
			Matrix44 ret;
			getTransform(State, ret);
			return ret;
		}

		static void setViewport(const D3DVIEWPORT9& viewport)
		{
			errCheck(mpDev->SetViewport(&viewport));

			mMatrixTracker.setProjToScreenMatrix(viewport);
		}

		static D3DVIEWPORT9 getViewport(void)
		{
			D3DVIEWPORT9 viewport;
			errCheck(mpDev->GetViewport(&viewport));
			return viewport;
		}

		static void clearTextures(void)
		{
			for (int i = 0; i < StateCache::NumTextures; i++)
			{
				setTextureWrapLinear(i, NULL, false);
				//setTexture(i, NULL);
			}
		}

		static void setRenderTarget(
			DWORD RenderTargetIndex,
			IDirect3DSurface9 *pRenderTarget)
		{
			DebugRange(RenderTargetIndex, MaxRenderTargets);
			if (mpCurRenderTarget[RenderTargetIndex] != pRenderTarget)
			{
				mpCurRenderTarget[RenderTargetIndex] = pRenderTarget;
				
				flushCache();
				errCheck(mpDev->SetRenderTarget(RenderTargetIndex, pRenderTarget));
			
				if (0 == RenderTargetIndex)
				{
					mMatrixTracker.setProjToScreenMatrix(getViewport());
				
					updateRenderTarget(pRenderTarget);
				}
			}
		}

		static void setRenderTarget(
			DWORD RenderTargetIndex,
			IDirect3DSurface9 *pRenderTarget,
			const D3DVIEWPORT9& viewport)
		{
			flushCache();
      errCheck(mpDev->SetRenderTarget(RenderTargetIndex, pRenderTarget));
			
			setViewport(viewport);
			
			if (0 == RenderTargetIndex)
				updateRenderTarget(pRenderTarget);
		}

		static IDirect3DSurface9* createOffscreenPlainSurface(          
			UINT Width,
			UINT Height,
			D3DFORMAT Format,
			D3DPOOL Pool)
		{
			invalidateCache();
			IDirect3DSurface9* pRet = NULL;
			errCheckFail(mpDev->CreateOffscreenPlainSurface(Width, Height, Format, Pool, &pRet, NULL));
			return pRet;
		}

		static IDirect3DSurface9* getRenderTarget(DWORD RenderTargetIndex)
		{
			IDirect3DSurface9* pRet;
			errCheck(mpDev->GetRenderTarget(RenderTargetIndex, &pRet));
			return pRet;
		}

		static void getRenderTargetData(          
			IDirect3DSurface9* pRenderTarget,
			IDirect3DSurface9* pDestSurface)
		{
			errCheck(mpDev->GetRenderTargetData(pRenderTarget, pDestSurface));
		}

		static int getRenderTargetWidth(void)
		{
			return mRenderTargetDesc.Width;
		}

		static int getRenderTargetHeight(void)
		{
			return mRenderTargetDesc.Height;
		}

		static const D3DSURFACE_DESC& getRenderTargetDesc(void)
		{
			return mRenderTargetDesc;
		}
		
		// vertex declaration helper method
		static int getVertexDeclarationTypeSize(BYTE type)
		{
			int size = 0;
			switch (type)
			{
				case D3DDECLTYPE_FLOAT1    :  size = 4 * 1; break; // 1D float expanded to (value, 0., 0., 1.)
				case D3DDECLTYPE_FLOAT2    :  size = 4 * 2; break; // 2D float expanded to (value, value, 0., 1.)
				case D3DDECLTYPE_FLOAT3    :  size = 4 * 3; break; // 3D float expanded to (value, value, value, 1.)
				case D3DDECLTYPE_FLOAT4    :  size = 4 * 4; break; // 4D float
				case D3DDECLTYPE_D3DCOLOR  :  size = 4; break; // 4D packed unsigned bytes mapped to 0. to 1. range Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
				case D3DDECLTYPE_UBYTE4    :  size = 4; break; // 4D unsigned byte
				case D3DDECLTYPE_SHORT2    :  size = 4; break; // 2D signed short expanded to (value, value, 0., 1.)
				case D3DDECLTYPE_SHORT4    :  size = 8; break; // 4D signed short
				case D3DDECLTYPE_UBYTE4N   :  size = 4; break; // Each of 4 bytes is normalized by dividing to 255.0
				case D3DDECLTYPE_SHORT2N   :  size = 4; break; // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
				case D3DDECLTYPE_SHORT4N   :  size = 8; break; // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
				case D3DDECLTYPE_USHORT2N  :  size = 4; break; // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
				case D3DDECLTYPE_USHORT4N  :  size = 8; break; // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
				case D3DDECLTYPE_UDEC3     :  size = 4; break; // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
				case D3DDECLTYPE_DEC3N     :  size = 4; break; // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
				case D3DDECLTYPE_FLOAT16_2 :  size = 4; break; // Two 16-bit floating point values, expanded to (value, value, 0, 1)
				case D3DDECLTYPE_FLOAT16_4 :  size = 8; break; // Four 16-bit floating point values
				default:
					Verify(false);
			}
			return size;
		}
		
		static const char* getVertexDeclarationTypeName(BYTE type)
		{
			int size = 0;
			switch (type)
			{
				case D3DDECLTYPE_FLOAT1    :  return "FLOAT1"; break; // 1D float expanded to (value, 0., 0., 1.)
				case D3DDECLTYPE_FLOAT2    :  return "FLOAT2"; break; // 2D float expanded to (value, value, 0., 1.)
				case D3DDECLTYPE_FLOAT3    :  return "FLOAT3"; break; // 3D float expanded to (value, value, value, 1.)
				case D3DDECLTYPE_FLOAT4    :  return "FLOAT4"; break; // 4D float
				case D3DDECLTYPE_D3DCOLOR  :  return "D3DCOLOR"; break; // 4D packed unsigned bytes mapped to 0. to 1. range Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
				case D3DDECLTYPE_UBYTE4    :  return "UBYTE4"; break; // 4D unsigned byte
				case D3DDECLTYPE_SHORT2    :  return "SHORT2"; break; // 2D signed short expanded to (value, value, 0., 1.)
				case D3DDECLTYPE_SHORT4    :  return "SHORT4"; break; // 4D signed short
				case D3DDECLTYPE_UBYTE4N   :  return "UBYTE4N"; break; // Each of 4 bytes is normalized by dividing to 255.0
				case D3DDECLTYPE_SHORT2N   :  return "SHORT2N"; break; // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
				case D3DDECLTYPE_SHORT4N   :  return "SHORT4N"; break; // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
				case D3DDECLTYPE_USHORT2N  :  return "USHORT2N"; break; // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
				case D3DDECLTYPE_USHORT4N  :  return "USHORT4N"; break; // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
				case D3DDECLTYPE_UDEC3     :  return "UDEC3"; break; // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
				case D3DDECLTYPE_DEC3N     :  return "DEC3N"; break; // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
				case D3DDECLTYPE_FLOAT16_2 :  return "FLOAT16_2"; break; // Two 16-bit floating point values, expanded to (value, value, 0, 1)
				case D3DDECLTYPE_FLOAT16_4 :  return "FLOAT16_4"; break; // Four 16-bit floating point values
				default:
					Verify(false);
			}
			return "";
		}
		
		static const char* getVertexDeclarationUsageName(BYTE usage)
		{
			switch (usage)
			{
				case D3DDECLUSAGE_POSITION			: return "POSITION";
				case D3DDECLUSAGE_BLENDWEIGHT		: return "BLENDWEIGHT";
				case D3DDECLUSAGE_BLENDINDICES	: return "BLENDINDICES";
				case D3DDECLUSAGE_NORMAL				: return "NORMAL";
				case D3DDECLUSAGE_PSIZE					: return "PSIZE";
				case D3DDECLUSAGE_TEXCOORD			: return "TEXCOORD";
				case D3DDECLUSAGE_TANGENT				: return "TANGENT";
				case D3DDECLUSAGE_BINORMAL			: return "BINORMAL";
				case D3DDECLUSAGE_TESSFACTOR		: return "TESSFACTOR";
				case D3DDECLUSAGE_POSITIONT			: return "POSITIONT";
				case D3DDECLUSAGE_COLOR					: return "COLOR";
				case D3DDECLUSAGE_FOG						: return "FOG";
				case D3DDECLUSAGE_DEPTH					: return "DEPTH";
				case D3DDECLUSAGE_SAMPLE				: return "SAMPLE";
				default:
					Verify(false);
			}
			return "";
		}

		// vertex declaration helper method
		static int setVertexDeclarationOffsets(D3DVERTEXELEMENT9* pElements)
		{
			int curOfs = 0;
			int prevStream = -1;

			while (0xFF != pElements->Stream)
			{
				if (pElements->Stream != prevStream)
				{
					prevStream = pElements->Stream;
					curOfs = 0;
				}
				
				const int size = getVertexDeclarationTypeSize(pElements->Type);
				
				pElements->Offset = curOfs;
				
				curOfs += size;

				pElements++;
			}
			
			return curOfs;
		}
		
		static void dumpVertexDeclaration(D3DVERTEXELEMENT9* pElements)
		{
			Status("D3D::dumpVertexDeclaration:\n");
			
			int totalSize = 0;
			while (0xFF != pElements->Stream)
			{
				const char* pTypeName = getVertexDeclarationTypeName(pElements->Type);
				const char* pUsageName = getVertexDeclarationUsageName(pElements->Usage);
				const int size = getVertexDeclarationTypeSize(pElements->Type);
							
				Status("  Stream: %i, Ofs: %02i, Typ: %s, Len: %i, Mthd: %i, Usage: %s, UsgIdx: %i\n",
					pElements->Stream,
					pElements->Offset,
					pTypeName,
					size,
					pElements->Method,
					pUsageName, 
					pElements->UsageIndex);
				
				totalSize += size;
				
				pElements++;
			}
			
			Status("  Total size: %i\n", totalSize);
		}
		
	private:
		static IDirect3DDevice9* mpDev;
		static StateCache mStateCache;
		static MatrixTracker mMatrixTracker;
		static D3DSURFACE_DESC mRenderTargetDesc;
		
		static IDirect3DVertexShader9* mpCurVShader;
		static IDirect3DPixelShader9* mpCurPShader;
		static IDirect3DVertexDeclaration9* mpCurDecl;
		static IDirect3DSurface9* mpCurDepthStencilSurf;
		static IDirect3DSurface9* mpCurRenderTarget[MaxRenderTargets];
		
		static void syncMatrixTracker(void)
		{
			mMatrixTracker.setMatrix(eModelToWorld, D3D::getTransform(D3DTS_WORLD));
			mMatrixTracker.setMatrix(eWorldToView, D3D::getTransform(D3DTS_VIEW));
			mMatrixTracker.setMatrix(eViewToProj, D3D::getTransform(D3DTS_PROJECTION));
			mMatrixTracker.setProjToScreenMatrix(getViewport());
		}
		
		static void updateRenderTarget(void)
		{
			IDirect3DSurface9* pSurf = getRenderTarget(0);
			pSurf->GetDesc(&mRenderTargetDesc);
			pSurf->Release();
		}

		static void updateRenderTarget(IDirect3DSurface9* pSurf)
		{
			pSurf->GetDesc(&mRenderTargetDesc);
		}
		
		static void resetCachedState(void)
		{
			mpCurVShader = NULL;
			mpCurPShader = NULL;
			mpCurDecl = NULL;
			mpCurDepthStencilSurf = NULL;
			Utils::ClearObj(mpCurRenderTarget);
		}

	}; // struct D3D

} // namespace gr

#endif // DEVICE_H

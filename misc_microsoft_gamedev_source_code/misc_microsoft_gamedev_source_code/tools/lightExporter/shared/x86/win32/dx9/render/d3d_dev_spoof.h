//-----------------------------------------------------------------------------
// d3d_dev_spoof.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef D3D_DEV_SPOOF_H
#define D3D_DEV_SPOOF_H

#include "device.h"

namespace gr
{
	struct __declspec(uuid("D0223B96-BF7A-43fd-92BD-A43B0D82B9EB")) IMyBaseDirect3DDevice9;

	struct __declspec(novtable) IMyBaseDirect3DDevice9 : public IUnknown
	{
		/*** IUnknown methods ***/
		virtual HRESULT __stdcall QueryInterface( const IID & riid, void** ppvObj) = 0;
		virtual ULONG __stdcall AddRef(void) = 0;
		virtual ULONG __stdcall Release(void) = 0;

		/*** IDirect3DDevice9 methods ***/
		virtual HRESULT __stdcall TestCooperativeLevel(void) = 0;
		virtual UINT __stdcall GetAvailableTextureMem(void) = 0;
		virtual HRESULT __stdcall EvictManagedResources(void) = 0;
		virtual HRESULT __stdcall GetDirect3D( IDirect3D9** ppD3D9) = 0;
		virtual HRESULT __stdcall GetDeviceCaps( D3DCAPS9* pCaps) = 0;
		virtual HRESULT __stdcall GetDisplayMode( UINT iSwapChain,D3DDISPLAYMODE* pMode) = 0;
		virtual HRESULT __stdcall GetCreationParameters( D3DDEVICE_CREATION_PARAMETERS *pParameters) = 0;
		virtual HRESULT __stdcall SetCursorProperties( UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap) = 0;
		virtual void __stdcall SetCursorPosition( int X,int Y,DWORD Flags) = 0;
		virtual BOOL __stdcall ShowCursor( BOOL bShow) = 0;
		virtual HRESULT __stdcall CreateAdditionalSwapChain( D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain) = 0;
		virtual HRESULT __stdcall GetSwapChain( UINT iSwapChain,IDirect3DSwapChain9** pSwapChain) = 0;
		virtual UINT __stdcall GetNumberOfSwapChains(void) = 0;
		virtual HRESULT __stdcall Reset( D3DPRESENT_PARAMETERS* pPresentationParameters) = 0;
		virtual HRESULT __stdcall Present( const RECT* pSourceRect,const RECT* pDestRect,HWND hDestWindowOverride,const RGNDATA* pDirtyRegion) = 0;
		virtual HRESULT __stdcall GetBackBuffer( UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer) = 0;
		virtual HRESULT __stdcall GetRasterStatus( UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus) = 0;
		virtual HRESULT __stdcall SetDialogBoxMode( BOOL bEnableDialogs) = 0;
		virtual void __stdcall SetGammaRamp( UINT iSwapChain,DWORD Flags,const D3DGAMMARAMP* pRamp) = 0;
		virtual void __stdcall GetGammaRamp( UINT iSwapChain,D3DGAMMARAMP* pRamp) = 0;
		virtual HRESULT __stdcall CreateTexture( UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle) = 0;
		virtual HRESULT __stdcall CreateVolumeTexture( UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle) = 0;
		virtual HRESULT __stdcall CreateCubeTexture( UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle) = 0;
		virtual HRESULT __stdcall CreateVertexBuffer( UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle) = 0;
		virtual HRESULT __stdcall CreateIndexBuffer( UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle) = 0;
		virtual HRESULT __stdcall CreateRenderTarget( UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) = 0;
		virtual HRESULT __stdcall CreateDepthStencilSurface( UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) = 0;
		virtual HRESULT __stdcall UpdateSurface( IDirect3DSurface9* pSourceSurface,const RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,const POINT* pDestPoint) = 0;
		virtual HRESULT __stdcall UpdateTexture( IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture) = 0;
		virtual HRESULT __stdcall GetRenderTargetData( IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface) = 0;
		virtual HRESULT __stdcall GetFrontBufferData( UINT iSwapChain,IDirect3DSurface9* pDestSurface) = 0;
		virtual HRESULT __stdcall StretchRect( IDirect3DSurface9* pSourceSurface,const RECT* pSourceRect,IDirect3DSurface9* pDestSurface,const RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter) = 0;
		virtual HRESULT __stdcall ColorFill( IDirect3DSurface9* pSurface,const RECT* pRect,D3DCOLOR color) = 0;
		virtual HRESULT __stdcall CreateOffscreenPlainSurface( UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) = 0;
		virtual HRESULT __stdcall SetRenderTarget( DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget) = 0;
		virtual HRESULT __stdcall GetRenderTarget( DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget) = 0;
		virtual HRESULT __stdcall SetDepthStencilSurface( IDirect3DSurface9* pNewZStencil) = 0;
		virtual HRESULT __stdcall GetDepthStencilSurface( IDirect3DSurface9** ppZStencilSurface) = 0;
		virtual HRESULT __stdcall BeginScene(void) = 0;
		virtual HRESULT __stdcall EndScene(void) = 0;
		virtual HRESULT __stdcall Clear( DWORD Count,const D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil) = 0;
		virtual HRESULT __stdcall SetTransform( D3DTRANSFORMSTATETYPE State,const D3DMATRIX* pMatrix) = 0;
		virtual HRESULT __stdcall GetTransform( D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) = 0;
		virtual HRESULT __stdcall MultiplyTransform( D3DTRANSFORMSTATETYPE,const D3DMATRIX*) = 0;
		virtual HRESULT __stdcall SetViewport( const D3DVIEWPORT9* pViewport) = 0;
		virtual HRESULT __stdcall GetViewport( D3DVIEWPORT9* pViewport) = 0;
		virtual HRESULT __stdcall SetMaterial( const D3DMATERIAL9* pMaterial) = 0;
		virtual HRESULT __stdcall GetMaterial( D3DMATERIAL9* pMaterial) = 0;
		virtual HRESULT __stdcall SetLight( DWORD Index,const D3DLIGHT9*) = 0;
		virtual HRESULT __stdcall GetLight( DWORD Index,D3DLIGHT9*) = 0;
		virtual HRESULT __stdcall LightEnable( DWORD Index,BOOL Enable) = 0;
		virtual HRESULT __stdcall GetLightEnable( DWORD Index,BOOL* pEnable) = 0;
		virtual HRESULT __stdcall SetClipPlane( DWORD Index,const float* pPlane) = 0;
		virtual HRESULT __stdcall GetClipPlane( DWORD Index,float* pPlane) = 0;
		virtual HRESULT __stdcall SetRenderState( D3DRENDERSTATETYPE State,DWORD Value) = 0;
		virtual HRESULT __stdcall GetRenderState( D3DRENDERSTATETYPE State,DWORD* pValue) = 0;
		virtual HRESULT __stdcall CreateStateBlock( D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB) = 0;
		virtual HRESULT __stdcall BeginStateBlock(void) = 0;
		virtual HRESULT __stdcall EndStateBlock( IDirect3DStateBlock9** ppSB) = 0;
		virtual HRESULT __stdcall SetClipStatus( const D3DCLIPSTATUS9* pClipStatus) = 0;
		virtual HRESULT __stdcall GetClipStatus( D3DCLIPSTATUS9* pClipStatus) = 0;
		virtual HRESULT __stdcall GetTexture( DWORD Stage,IDirect3DBaseTexture9** ppTexture) = 0;
		virtual HRESULT __stdcall SetTexture( DWORD Stage,IDirect3DBaseTexture9* pTexture) = 0;
		virtual HRESULT __stdcall GetTextureStageState( DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) = 0;
		virtual HRESULT __stdcall SetTextureStageState( DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value) = 0;
		virtual HRESULT __stdcall GetSamplerState( DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue) = 0;
		virtual HRESULT __stdcall SetSamplerState( DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value) = 0;
		virtual HRESULT __stdcall ValidateDevice( DWORD* pNumPasses) = 0;
		virtual HRESULT __stdcall SetPaletteEntries( UINT PaletteNumber,const PALETTEENTRY* pEntries) = 0;
		virtual HRESULT __stdcall GetPaletteEntries( UINT PaletteNumber,PALETTEENTRY* pEntries) = 0;
		virtual HRESULT __stdcall SetCurrentTexturePalette( UINT PaletteNumber) = 0;
		virtual HRESULT __stdcall GetCurrentTexturePalette( UINT *PaletteNumber) = 0;
		virtual HRESULT __stdcall SetScissorRect( const RECT* pRect) = 0;
		virtual HRESULT __stdcall GetScissorRect( RECT* pRect) = 0;
		virtual HRESULT __stdcall SetSoftwareVertexProcessing( BOOL bSoftware) = 0;
		virtual BOOL __stdcall GetSoftwareVertexProcessing(void) = 0;
		virtual HRESULT __stdcall SetNPatchMode( float nSegments) = 0;
		virtual float __stdcall GetNPatchMode(void) = 0;
		virtual HRESULT __stdcall DrawPrimitive( D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) = 0;
		virtual HRESULT __stdcall DrawIndexedPrimitive( D3DPRIMITIVETYPE,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount) = 0;
		virtual HRESULT __stdcall DrawPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,const void* pVertexStreamZeroData,UINT VertexStreamZeroStride) = 0;
		virtual HRESULT __stdcall DrawIndexedPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,const void* pIndexData,D3DFORMAT IndexDataFormat,const void* pVertexStreamZeroData,UINT VertexStreamZeroStride) = 0;
		virtual HRESULT __stdcall ProcessVertices( UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags) = 0;
		virtual HRESULT __stdcall CreateVertexDeclaration( const D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl) = 0;
		virtual HRESULT __stdcall SetVertexDeclaration( IDirect3DVertexDeclaration9* pDecl) = 0;
		virtual HRESULT __stdcall GetVertexDeclaration( IDirect3DVertexDeclaration9** ppDecl) = 0;
		virtual HRESULT __stdcall SetFVF( DWORD FVF) = 0;
		virtual HRESULT __stdcall GetFVF( DWORD* pFVF) = 0;
		virtual HRESULT __stdcall CreateVertexShader( const DWORD* pFunction,IDirect3DVertexShader9** ppShader) = 0;
		virtual HRESULT __stdcall SetVertexShader( IDirect3DVertexShader9* pShader) = 0;
		virtual HRESULT __stdcall GetVertexShader( IDirect3DVertexShader9** ppShader) = 0;
		virtual HRESULT __stdcall SetVertexShaderConstantF( UINT StartRegister,const float* pConstantData,UINT Vector4fCount) = 0;
		virtual HRESULT __stdcall GetVertexShaderConstantF( UINT StartRegister,float* pConstantData,UINT Vector4fCount) = 0;
		virtual HRESULT __stdcall SetVertexShaderConstantI( UINT StartRegister,const int* pConstantData,UINT Vector4iCount) = 0;
		virtual HRESULT __stdcall GetVertexShaderConstantI( UINT StartRegister,int* pConstantData,UINT Vector4iCount) = 0;
		virtual HRESULT __stdcall SetVertexShaderConstantB( UINT StartRegister,const BOOL* pConstantData,UINT  BoolCount) = 0;
		virtual HRESULT __stdcall GetVertexShaderConstantB( UINT StartRegister,BOOL* pConstantData,UINT BoolCount) = 0;
		virtual HRESULT __stdcall SetStreamSource( UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride) = 0;
		virtual HRESULT __stdcall GetStreamSource( UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* OffsetInBytes,UINT* pStride) = 0;
		virtual HRESULT __stdcall SetStreamSourceFreq( UINT StreamNumber,UINT Divider) = 0;
		virtual HRESULT __stdcall GetStreamSourceFreq( UINT StreamNumber,UINT* Divider) = 0;
		virtual HRESULT __stdcall SetIndices( IDirect3DIndexBuffer9* pIndexData) = 0;
		virtual HRESULT __stdcall GetIndices( IDirect3DIndexBuffer9** ppIndexData) = 0;
		virtual HRESULT __stdcall CreatePixelShader( const DWORD* pFunction,IDirect3DPixelShader9** ppShader) = 0;
		virtual HRESULT __stdcall SetPixelShader( IDirect3DPixelShader9* pShader) = 0;
		virtual HRESULT __stdcall GetPixelShader( IDirect3DPixelShader9** ppShader) = 0;
		virtual HRESULT __stdcall SetPixelShaderConstantF( UINT StartRegister,const float* pConstantData,UINT Vector4fCount) = 0;
		virtual HRESULT __stdcall GetPixelShaderConstantF( UINT StartRegister,float* pConstantData,UINT Vector4fCount) = 0;
		virtual HRESULT __stdcall SetPixelShaderConstantI( UINT StartRegister,const int* pConstantData,UINT Vector4iCount) = 0;
		virtual HRESULT __stdcall GetPixelShaderConstantI( UINT StartRegister,int* pConstantData,UINT Vector4iCount) = 0;
		virtual HRESULT __stdcall SetPixelShaderConstantB( UINT StartRegister,const BOOL* pConstantData,UINT  BoolCount) = 0;
		virtual HRESULT __stdcall GetPixelShaderConstantB( UINT StartRegister,BOOL* pConstantData,UINT BoolCount) = 0;
		virtual HRESULT __stdcall DrawRectPatch( UINT Handle,const float* pNumSegs,const D3DRECTPATCH_INFO* pRectPatchInfo) = 0;
		virtual HRESULT __stdcall DrawTriPatch( UINT Handle,const float* pNumSegs,const D3DTRIPATCH_INFO* pTriPatchInfo) = 0;
		virtual HRESULT __stdcall DeletePatch( UINT Handle) = 0;
		virtual HRESULT __stdcall CreateQuery( D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery) = 0;
	};
	
	struct IMyDirect3DDevice9 : IMyBaseDirect3DDevice9
	{
		int mInStateBlockCount;

		IMyDirect3DDevice9() :
			mInStateBlockCount(0)
		{
		}

		/*** IUnknown methods ***/
		virtual HRESULT __stdcall QueryInterface( const IID & riid, void** ppvObj)
		{
			return D3D::getDevice()->QueryInterface(riid, ppvObj);
		}

		virtual ULONG __stdcall AddRef(void)
		{
			return D3D::getDevice()->AddRef();
		}

		virtual ULONG __stdcall Release(void)
		{
			return D3D::getDevice()->Release();
		}

		/*** IDirect3DDevice9 methods ***/
		virtual HRESULT __stdcall TestCooperativeLevel(void)
		{
			return D3D::getDevice()->TestCooperativeLevel();
		}

		virtual UINT __stdcall GetAvailableTextureMem(void)
		{
			return D3D::getDevice()->GetAvailableTextureMem();
		}

		virtual HRESULT __stdcall EvictManagedResources(void)
		{
			return D3D::getDevice()->EvictManagedResources();
		}

		virtual HRESULT __stdcall GetDirect3D( IDirect3D9** ppD3D9)
		{
			return D3D::getDevice()->GetDirect3D(ppD3D9);
		}

		virtual HRESULT __stdcall GetDeviceCaps( D3DCAPS9* pCaps)
		{
			return D3D::getDevice()->GetDeviceCaps(pCaps);
		}

		virtual HRESULT __stdcall GetDisplayMode( UINT iSwapChain,D3DDISPLAYMODE* pMode)
		{
			return D3D::getDevice()->GetDisplayMode(iSwapChain, pMode);
		}

		virtual HRESULT __stdcall GetCreationParameters( D3DDEVICE_CREATION_PARAMETERS *pParameters)
		{
			return D3D::getDevice()->GetCreationParameters(pParameters);
		}

		virtual HRESULT __stdcall SetCursorProperties( UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap)
		{
			return D3D::getDevice()->SetCursorProperties(XHotSpot,YHotSpot,pCursorBitmap);
		}

		virtual void __stdcall SetCursorPosition( int X,int Y,DWORD Flags)
		{
			D3D::getDevice()->SetCursorPosition(X, Y, Flags);
		}

		virtual BOOL __stdcall ShowCursor( BOOL bShow)
		{
			return D3D::getDevice()->ShowCursor(bShow);
		}

		virtual HRESULT __stdcall CreateAdditionalSwapChain( D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain)
		{
			return D3D::getDevice()->CreateAdditionalSwapChain( pPresentationParameters,pSwapChain);
		}

		virtual HRESULT __stdcall GetSwapChain( UINT iSwapChain,IDirect3DSwapChain9** pSwapChain)
		{
			return D3D::getDevice()->GetSwapChain( iSwapChain,pSwapChain);
		}

		virtual UINT __stdcall GetNumberOfSwapChains(void)
		{
			return D3D::getDevice()->GetNumberOfSwapChains();
		}

		virtual HRESULT __stdcall Reset( D3DPRESENT_PARAMETERS* pPresentationParameters)
		{
			return D3D::getDevice()->Reset(pPresentationParameters);
		}

		virtual HRESULT __stdcall Present( const RECT* pSourceRect,const RECT* pDestRect,HWND hDestWindowOverride,const RGNDATA* pDirtyRegion)
		{
			return D3D::getDevice()->Present(pSourceRect,pDestRect,hDestWindowOverride,pDirtyRegion);
		}

		virtual HRESULT __stdcall GetBackBuffer( UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer)
		{
			return D3D::getDevice()->GetBackBuffer(iSwapChain,iBackBuffer,Type,ppBackBuffer);
		}

		virtual HRESULT __stdcall GetRasterStatus( UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus)
		{
			return D3D::getDevice()->GetRasterStatus(iSwapChain,pRasterStatus);
		}

		virtual HRESULT __stdcall SetDialogBoxMode( BOOL bEnableDialogs)
		{
			return D3D::getDevice()->SetDialogBoxMode(bEnableDialogs);
		}

		virtual void __stdcall SetGammaRamp( UINT iSwapChain,DWORD Flags,const D3DGAMMARAMP* pRamp)
		{
			return D3D::getDevice()->SetGammaRamp( iSwapChain,Flags,pRamp);
		}

		virtual void __stdcall GetGammaRamp( UINT iSwapChain,D3DGAMMARAMP* pRamp)
		{
			return D3D::getDevice()->GetGammaRamp( iSwapChain,pRamp);
		}

		virtual HRESULT __stdcall CreateTexture( UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle)
		{
			return D3D::getDevice()->CreateTexture( Width,Height,Levels,Usage,Format,Pool,ppTexture,pSharedHandle);
		}

		virtual HRESULT __stdcall CreateVolumeTexture( UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle)
		{
			return D3D::getDevice()->CreateVolumeTexture( Width,Height,Depth,Levels,Usage,Format,Pool,ppVolumeTexture,pSharedHandle);
		}

		virtual HRESULT __stdcall CreateCubeTexture( UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle)
		{
			return D3D::getDevice()->CreateCubeTexture( EdgeLength,Levels,Usage,Format,Pool,ppCubeTexture,pSharedHandle);
		}

		virtual HRESULT __stdcall CreateVertexBuffer( UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle)
		{
			return D3D::getDevice()->CreateVertexBuffer( Length,Usage,FVF,Pool,ppVertexBuffer,pSharedHandle);
		}

		virtual HRESULT __stdcall CreateIndexBuffer( UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle)
		{
			return D3D::getDevice()->CreateIndexBuffer( Length,Usage,Format,Pool,ppIndexBuffer,pSharedHandle);
		}

		virtual HRESULT __stdcall CreateRenderTarget( UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle)
		{
			return D3D::getDevice()->CreateRenderTarget( Width,Height,Format,MultiSample,MultisampleQuality,Lockable,ppSurface,pSharedHandle);
		}

		virtual HRESULT __stdcall CreateDepthStencilSurface( UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle)
		{
			return D3D::getDevice()->CreateDepthStencilSurface( Width,Height,Format,MultiSample,MultisampleQuality,Discard,ppSurface,pSharedHandle);
		}

		virtual HRESULT __stdcall UpdateSurface( IDirect3DSurface9* pSourceSurface,const RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,const POINT* pDestPoint)
		{
			return D3D::getDevice()->UpdateSurface( pSourceSurface,pSourceRect,pDestinationSurface,pDestPoint);
		}

		virtual HRESULT __stdcall UpdateTexture( IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture)
		{
			return D3D::getDevice()->UpdateTexture( pSourceTexture,pDestinationTexture);
		}

		virtual HRESULT __stdcall GetRenderTargetData( IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface)
		{
			return D3D::getDevice()->GetRenderTargetData( pRenderTarget,pDestSurface);
		}

		virtual HRESULT __stdcall GetFrontBufferData( UINT iSwapChain,IDirect3DSurface9* pDestSurface)
		{
			return D3D::getDevice()->GetFrontBufferData( iSwapChain,pDestSurface);
		}

		virtual HRESULT __stdcall StretchRect( IDirect3DSurface9* pSourceSurface,const RECT* pSourceRect,IDirect3DSurface9* pDestSurface,const RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter)
		{
			return D3D::getDevice()->StretchRect( pSourceSurface,pSourceRect,pDestSurface,pDestRect,Filter);
		}

		virtual HRESULT __stdcall ColorFill( IDirect3DSurface9* pSurface,const RECT* pRect,D3DCOLOR color)
		{
			return D3D::getDevice()->ColorFill( pSurface,pRect,color);
		}

		virtual HRESULT __stdcall CreateOffscreenPlainSurface( UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle)
		{
			return D3D::getDevice()->CreateOffscreenPlainSurface( Width,Height,Format,Pool,ppSurface,pSharedHandle);
		}

		virtual HRESULT __stdcall SetRenderTarget( DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget)
		{
			return D3D::getDevice()->SetRenderTarget( RenderTargetIndex,pRenderTarget);
		}

		virtual HRESULT __stdcall GetRenderTarget( DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget)
		{
			return D3D::getDevice()->GetRenderTarget( RenderTargetIndex,ppRenderTarget);
		}

		virtual HRESULT __stdcall SetDepthStencilSurface( IDirect3DSurface9* pNewZStencil)
		{
			return D3D::getDevice()->SetDepthStencilSurface( pNewZStencil);
		}

		virtual HRESULT __stdcall GetDepthStencilSurface( IDirect3DSurface9** ppZStencilSurface)
		{
			return D3D::getDevice()->GetDepthStencilSurface( ppZStencilSurface);
		}

		virtual HRESULT __stdcall BeginScene(void)
		{
			return D3D::getDevice()->BeginScene();
		}

		virtual HRESULT __stdcall EndScene(void)
		{
			return D3D::getDevice()->EndScene();
		}

		virtual HRESULT __stdcall Clear( DWORD Count,const D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil)
		{
			return D3D::getDevice()->Clear( Count,pRects,Flags,Color,Z,Stencil);
		}

		virtual HRESULT __stdcall SetTransform( D3DTRANSFORMSTATETYPE State,const D3DMATRIX* pMatrix)
		{
			return D3D::getDevice()->SetTransform( State,pMatrix);
		}

		virtual HRESULT __stdcall GetTransform( D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix)
		{
			return D3D::getDevice()->GetTransform( State,pMatrix);
		}

		virtual HRESULT __stdcall MultiplyTransform( D3DTRANSFORMSTATETYPE State,const D3DMATRIX* pMatrix)
		{
			return D3D::getDevice()->MultiplyTransform(State, pMatrix);
		}

		virtual HRESULT __stdcall SetViewport( const D3DVIEWPORT9* pViewport)
		{
			return D3D::getDevice()->SetViewport( pViewport);
		}

		virtual HRESULT __stdcall GetViewport( D3DVIEWPORT9* pViewport)
		{
			return D3D::getDevice()->GetViewport( pViewport);
		}

		virtual HRESULT __stdcall SetMaterial( const D3DMATERIAL9* pMaterial)
		{
			return D3D::getDevice()->SetMaterial(pMaterial);
		}

		virtual HRESULT __stdcall GetMaterial( D3DMATERIAL9* pMaterial)
		{
			return D3D::getDevice()->GetMaterial(pMaterial);
		}

		virtual HRESULT __stdcall SetLight( DWORD Index,const D3DLIGHT9* pLight)
		{
			return D3D::getDevice()->SetLight(Index, pLight);
		}

		virtual HRESULT __stdcall GetLight( DWORD Index,D3DLIGHT9* pLight)
		{
			return D3D::getDevice()->GetLight(Index, pLight);
		}

		virtual HRESULT __stdcall LightEnable( DWORD Index,BOOL Enable)
		{
			return D3D::getDevice()->LightEnable(Index, Enable);
		}

		virtual HRESULT __stdcall GetLightEnable( DWORD Index,BOOL* pEnable)
		{
			return D3D::getDevice()->GetLightEnable(Index, pEnable);
		}

		virtual HRESULT __stdcall SetClipPlane( DWORD Index,const float* pPlane)
		{
			return D3D::getDevice()->SetClipPlane(Index, pPlane);
		}

		virtual HRESULT __stdcall GetClipPlane( DWORD Index,float* pPlane)
		{
			return D3D::getDevice()->GetClipPlane(Index, pPlane);
		}

		virtual HRESULT __stdcall SetRenderState( D3DRENDERSTATETYPE State,DWORD Value)
		{
			return D3D::getDevice()->SetRenderState(State, Value);
		}

		virtual HRESULT __stdcall GetRenderState( D3DRENDERSTATETYPE State,DWORD* pValue)
		{
			return D3D::getDevice()->GetRenderState(State, pValue);
		}

		virtual HRESULT __stdcall CreateStateBlock( D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB)
		{
			return D3D::getDevice()->CreateStateBlock(Type, ppSB);
		}

		virtual HRESULT __stdcall BeginStateBlock(void)
		{
			mInStateBlockCount++;
			return D3D::getDevice()->BeginStateBlock();
		}

		virtual HRESULT __stdcall EndStateBlock( IDirect3DStateBlock9** ppSB)
		{
			mInStateBlockCount--;
			return D3D::getDevice()->EndStateBlock(ppSB);
		}
	
		virtual HRESULT __stdcall SetClipStatus( const D3DCLIPSTATUS9* pClipStatus)
		{
			return D3D::getDevice()->SetClipStatus(pClipStatus);
		}

		virtual HRESULT __stdcall GetClipStatus( D3DCLIPSTATUS9* pClipStatus)
		{
			return D3D::getDevice()->GetClipStatus(pClipStatus);
		}

		virtual HRESULT __stdcall GetTexture( DWORD Stage,IDirect3DBaseTexture9** ppTexture)
		{
			return D3D::getDevice()->GetTexture(Stage, ppTexture);
		}

		virtual HRESULT __stdcall SetTexture( DWORD Stage,IDirect3DBaseTexture9* pTexture)
		{
			return D3D::getDevice()->SetTexture(Stage, pTexture);
		}

		virtual HRESULT __stdcall GetTextureStageState( DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue)
		{
			return D3D::getDevice()->GetTextureStageState( Stage,Type,pValue);
		}

		virtual HRESULT __stdcall SetTextureStageState( DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value)
		{
			return D3D::getDevice()->SetTextureStageState( Stage,Type,Value);
		}

		virtual HRESULT __stdcall GetSamplerState( DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue)
		{
			return D3D::getDevice()->GetSamplerState(Sampler,Type,pValue);
		}

		virtual HRESULT __stdcall SetSamplerState( DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value)
		{
			return D3D::getDevice()->SetSamplerState( Sampler,Type,Value);
		}

		virtual HRESULT __stdcall ValidateDevice( DWORD* pNumPasses)
		{
			return D3D::getDevice()->ValidateDevice( pNumPasses);
		}

		virtual HRESULT __stdcall SetPaletteEntries( UINT PaletteNumber,const PALETTEENTRY* pEntries)
		{
			return D3D::getDevice()->SetPaletteEntries( PaletteNumber,pEntries);
		}

		virtual HRESULT __stdcall GetPaletteEntries( UINT PaletteNumber,PALETTEENTRY* pEntries)
		{
			return D3D::getDevice()->GetPaletteEntries( PaletteNumber,pEntries);
		}

		virtual HRESULT __stdcall SetCurrentTexturePalette( UINT PaletteNumber)
		{
			return D3D::getDevice()->SetCurrentTexturePalette( PaletteNumber);
		}

		virtual HRESULT __stdcall GetCurrentTexturePalette( UINT *PaletteNumber)
		{
			return D3D::getDevice()->GetCurrentTexturePalette( PaletteNumber);
		}

		virtual HRESULT __stdcall SetScissorRect( const RECT* pRect)
		{
			return D3D::getDevice()->SetScissorRect( pRect);
		}

		virtual HRESULT __stdcall GetScissorRect( RECT* pRect)
		{
			return D3D::getDevice()->GetScissorRect( pRect);
		}

		virtual HRESULT __stdcall SetSoftwareVertexProcessing( BOOL bSoftware)
		{
			return D3D::getDevice()->SetSoftwareVertexProcessing( bSoftware);
		}

		virtual BOOL __stdcall GetSoftwareVertexProcessing(void)
		{
			return D3D::getDevice()->GetSoftwareVertexProcessing();
		}

		virtual HRESULT __stdcall SetNPatchMode( float nSegments)
		{
			return D3D::getDevice()->SetNPatchMode( nSegments);
		}

		virtual float __stdcall GetNPatchMode(void)
		{
			return D3D::getDevice()->GetNPatchMode();
		}

		virtual HRESULT __stdcall DrawPrimitive( D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount)
		{
			return D3D::getDevice()->DrawPrimitive( PrimitiveType,StartVertex,PrimitiveCount);
		}

		virtual HRESULT __stdcall DrawIndexedPrimitive( D3DPRIMITIVETYPE PrimitiveType,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount)
		{
			return D3D::getDevice()->DrawIndexedPrimitive( PrimitiveType,BaseVertexIndex,MinVertexIndex,NumVertices,startIndex,primCount);
		}

		virtual HRESULT __stdcall DrawPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,const void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
		{
			return D3D::getDevice()->DrawPrimitiveUP( PrimitiveType,PrimitiveCount,pVertexStreamZeroData,VertexStreamZeroStride);
		}

		virtual HRESULT __stdcall DrawIndexedPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,const void* pIndexData,D3DFORMAT IndexDataFormat,const void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
		{
			return D3D::getDevice()->DrawIndexedPrimitiveUP( PrimitiveType,MinVertexIndex,NumVertices,PrimitiveCount,pIndexData,IndexDataFormat,pVertexStreamZeroData,VertexStreamZeroStride);
		}

		virtual HRESULT __stdcall ProcessVertices( UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags)
		{
			return D3D::getDevice()->ProcessVertices( SrcStartIndex,DestIndex,VertexCount,pDestBuffer,pVertexDecl,Flags);
		}

		virtual HRESULT __stdcall CreateVertexDeclaration( const D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl)
		{
			return D3D::getDevice()->CreateVertexDeclaration( pVertexElements,ppDecl);
		}

		virtual HRESULT __stdcall SetVertexDeclaration( IDirect3DVertexDeclaration9* pDecl)
		{
			return D3D::getDevice()->SetVertexDeclaration( pDecl);
		}

		virtual HRESULT __stdcall GetVertexDeclaration( IDirect3DVertexDeclaration9** ppDecl)
		{
			return D3D::getDevice()->GetVertexDeclaration( ppDecl);
		}

		virtual HRESULT __stdcall SetFVF( DWORD FVF)
		{
			return D3D::getDevice()->SetFVF(FVF);
		}

		virtual HRESULT __stdcall GetFVF( DWORD* pFVF)
		{
			return D3D::getDevice()->GetFVF(pFVF);
		}

		virtual HRESULT __stdcall CreateVertexShader( const DWORD* pFunction,IDirect3DVertexShader9** ppShader)
		{
			return D3D::getDevice()->CreateVertexShader(pFunction,ppShader);
		}

		virtual HRESULT __stdcall SetVertexShader( IDirect3DVertexShader9* pShader)
		{
			return D3D::getDevice()->SetVertexShader( pShader);
		}

		virtual HRESULT __stdcall GetVertexShader( IDirect3DVertexShader9** ppShader)
		{
			return D3D::getDevice()->GetVertexShader( ppShader);
		}

		virtual HRESULT __stdcall SetVertexShaderConstantF( UINT StartRegister,const float* pConstantData,UINT Vector4fCount)
		{
			return D3D::getDevice()->SetVertexShaderConstantF( StartRegister,pConstantData,Vector4fCount);
		}

		virtual HRESULT __stdcall GetVertexShaderConstantF( UINT StartRegister,float* pConstantData,UINT Vector4fCount)
		{
			return D3D::getDevice()->GetVertexShaderConstantF( StartRegister,pConstantData,Vector4fCount);
		}

		virtual HRESULT __stdcall SetVertexShaderConstantI( UINT StartRegister,const int* pConstantData,UINT Vector4iCount)
		{
			return D3D::getDevice()->SetVertexShaderConstantI( StartRegister,pConstantData,Vector4iCount);
		}

		virtual HRESULT __stdcall GetVertexShaderConstantI( UINT StartRegister,int* pConstantData,UINT Vector4iCount)
		{
			return D3D::getDevice()->GetVertexShaderConstantI( StartRegister,pConstantData,Vector4iCount);
		}

		virtual HRESULT __stdcall SetVertexShaderConstantB( UINT StartRegister,const BOOL* pConstantData,UINT  BoolCount)
		{
			return D3D::getDevice()->SetVertexShaderConstantB( StartRegister,pConstantData,BoolCount);
		}

		virtual HRESULT __stdcall GetVertexShaderConstantB( UINT StartRegister,BOOL* pConstantData,UINT BoolCount)
		{
			return D3D::getDevice()->GetVertexShaderConstantB( StartRegister,pConstantData,BoolCount);
		}

		virtual HRESULT __stdcall SetStreamSource( UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride)
		{
			return D3D::getDevice()->SetStreamSource( StreamNumber,pStreamData,OffsetInBytes,Stride);
		}

		virtual HRESULT __stdcall GetStreamSource( UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* OffsetInBytes,UINT* pStride)
		{
			return D3D::getDevice()->GetStreamSource( StreamNumber,ppStreamData,OffsetInBytes,pStride);
		}

		virtual HRESULT __stdcall SetStreamSourceFreq( UINT StreamNumber,UINT Divider)
		{
			return D3D::getDevice()->SetStreamSourceFreq( StreamNumber,Divider);
		}

		virtual HRESULT __stdcall GetStreamSourceFreq( UINT StreamNumber,UINT* Divider)
		{
			return D3D::getDevice()->GetStreamSourceFreq( StreamNumber,Divider);
		}

		virtual HRESULT __stdcall SetIndices( IDirect3DIndexBuffer9* pIndexData)
		{
			return D3D::getDevice()->SetIndices( pIndexData);
		}

		virtual HRESULT __stdcall GetIndices( IDirect3DIndexBuffer9** ppIndexData)
		{
			return D3D::getDevice()->GetIndices( ppIndexData);
		}

		virtual HRESULT __stdcall CreatePixelShader( const DWORD* pFunction,IDirect3DPixelShader9** ppShader)
		{
			return D3D::getDevice()->CreatePixelShader( pFunction,ppShader);
		}

		virtual HRESULT __stdcall SetPixelShader( IDirect3DPixelShader9* pShader)
		{
			return D3D::getDevice()->SetPixelShader(pShader);
		}

		virtual HRESULT __stdcall GetPixelShader( IDirect3DPixelShader9** ppShader)
		{
			return D3D::getDevice()->GetPixelShader(ppShader);
		}

		virtual HRESULT __stdcall SetPixelShaderConstantF( UINT StartRegister,const float* pConstantData,UINT Vector4fCount)
		{
			return D3D::getDevice()->SetPixelShaderConstantF( StartRegister,pConstantData,Vector4fCount);
		}

		virtual HRESULT __stdcall GetPixelShaderConstantF( UINT StartRegister,float* pConstantData,UINT Vector4fCount)
		{
			return D3D::getDevice()->GetPixelShaderConstantF( StartRegister,pConstantData,Vector4fCount);
		}

		virtual HRESULT __stdcall SetPixelShaderConstantI( UINT StartRegister,const int* pConstantData,UINT Vector4iCount)
		{
			return D3D::getDevice()->SetPixelShaderConstantI( StartRegister,pConstantData,Vector4iCount);
		}

		virtual HRESULT __stdcall GetPixelShaderConstantI( UINT StartRegister,int* pConstantData,UINT Vector4iCount)
		{
			return D3D::getDevice()->GetPixelShaderConstantI( StartRegister,pConstantData,Vector4iCount);
		}

		virtual HRESULT __stdcall SetPixelShaderConstantB( UINT StartRegister,const BOOL* pConstantData,UINT  BoolCount)
		{
			return D3D::getDevice()->SetPixelShaderConstantB( StartRegister,pConstantData,BoolCount);
		}

		virtual HRESULT __stdcall GetPixelShaderConstantB( UINT StartRegister,BOOL* pConstantData,UINT BoolCount)
		{
			return D3D::getDevice()->GetPixelShaderConstantB( StartRegister,pConstantData,BoolCount);
		}

		virtual HRESULT __stdcall DrawRectPatch( UINT Handle,const float* pNumSegs,const D3DRECTPATCH_INFO* pRectPatchInfo)
		{
			return D3D::getDevice()->DrawRectPatch( Handle,pNumSegs,pRectPatchInfo);
		}

		virtual HRESULT __stdcall DrawTriPatch( UINT Handle,const float* pNumSegs,const D3DTRIPATCH_INFO* pTriPatchInfo)
		{
			return D3D::getDevice()->DrawTriPatch( Handle,pNumSegs,pTriPatchInfo);
		}

		virtual HRESULT __stdcall DeletePatch( UINT Handle)
		{
			return D3D::getDevice()->DeletePatch(Handle);
		}

		virtual HRESULT __stdcall CreateQuery( D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery)
		{
			return D3D::getDevice()->CreateQuery(Type, ppQuery);
		}
	};

	extern IMyDirect3DDevice9 gMyD3DDevice;

	inline IDirect3DDevice9* GetMyD3DDevice(void)
	{
		return reinterpret_cast<IDirect3DDevice9*>(&gMyD3DDevice);
	}

} // namespace gr

#endif // D3D_DEV_SPOOF_H

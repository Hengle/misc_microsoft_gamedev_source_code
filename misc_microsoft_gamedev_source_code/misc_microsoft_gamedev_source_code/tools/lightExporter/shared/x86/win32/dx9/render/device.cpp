//-----------------------------------------------------------------------------
// File: device.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "device.h"

namespace gr
{
	IDirect3DDevice9* D3D::mpDev;
	StateCache D3D::mStateCache;
	MatrixTracker	D3D::mMatrixTracker;
	D3DSURFACE_DESC D3D::mRenderTargetDesc;
	IDirect3DVertexShader9* D3D::mpCurVShader;
	IDirect3DPixelShader9* D3D::mpCurPShader;
	IDirect3DVertexDeclaration9* D3D::mpCurDecl;
	IDirect3DSurface9* D3D::mpCurDepthStencilSurf;
	IDirect3DSurface9* D3D::mpCurRenderTarget[MaxRenderTargets];
		
} // namespace gr
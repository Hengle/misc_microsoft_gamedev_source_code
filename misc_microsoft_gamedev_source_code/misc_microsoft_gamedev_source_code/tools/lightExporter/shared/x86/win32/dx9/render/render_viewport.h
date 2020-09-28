//-----------------------------------------------------------------------------
// File: render_viewport.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef RENDER_VIEWPORT_H
#define RENDER_VIEWPORT_H

#include "matrix_tracker.h"
#include "device.h"
#include "common/render/camera.h"

#include <d3d9.h>

namespace gr
{
		// rendertarget/renderviewport set flags
	enum 
	{ 
		SET_RENDERTARGET = 1, 
		SET_VIEWPORT = 2, 
		SET_MATRICES = 4, 
		SET_ALL = 7 
	};

	class RenderViewport
	{
	public:
		RenderViewport() : 
			mpSurf(NULL)
		{
			mViewport.X = mViewport.Y = 0;
			mViewport.Width = mViewport.Height = 1;
		}

		RenderViewport(IDirect3DSurface9* pSurf, const Camera& camera, const D3DVIEWPORT9& viewport) :
			mpSurf(pSurf),
			mCamera(camera)
		{
			mViewport = viewport;
			updateMatrices();
		}
			
		RenderViewport(const RenderViewport& rhs)
		{
			*this = rhs;
		}

		RenderViewport& operator= (const RenderViewport& rhs)
		{
			mpSurf = rhs.mpSurf;
			mCamera = rhs.mCamera;
			mViewport = rhs.mViewport;
			mMatrixTracker = rhs.mMatrixTracker;
			return *this;
		}

		virtual ~RenderViewport()
		{
		}

		void updateMatrices(void)
		{
			mMatrixTracker.setMatrix(eWorldToView, mCamera.worldToView());
			mMatrixTracker.setMatrix(eViewToProj, mCamera.viewToProj());
			updateProjToScreenMatrix();
		}

		IDirect3DSurface9* surf(void) const { return mpSurf; }
		void setSurf(IDirect3DSurface9* pSurf) { mpSurf = pSurf; }

		Camera& camera(void) { return mCamera; }
		const Camera& camera(void) const { return mCamera; }

		void setCamera(const Camera& camera) 
		{
			mCamera = camera;
			updateMatrices();
		}
		
		const MatrixTracker& matrixTracker(void) const { return mMatrixTracker; }

		const Matrix44& getMatrix(EMatrix matrixIndex, bool transposed = false) const
		{
			return mMatrixTracker.getMatrix(matrixIndex, transposed);
		}

		Matrix44 viewToScreenMatrix(void) const
		{
			return mCamera.viewToProj() * mMatrixTracker.getMatrix(eProjToScreen);
		}

		const D3DVIEWPORT9& viewport(void) const
		{
			return mViewport;
		}

		void setViewport(const D3DVIEWPORT9& viewport)
		{
			mViewport = viewport;
			updateProjToScreenMatrix();
		}

		int surfWidth(void) const
		{
			if (!mpSurf)
				return 0;

			D3DSURFACE_DESC desc;
			mpSurf->GetDesc(&desc);
			return desc.Width;
		}

		int surfHeight(void) const
		{
			if (!mpSurf)
				return 0;

			D3DSURFACE_DESC desc;
			mpSurf->GetDesc(&desc);
			return desc.Height;
		}
		    
		int viewportWidth(void) const { return mViewport.Width; }
		int viewportHeight(void) const { return mViewport.Height; }
		int viewportX(void) const { return mViewport.X; }
		int viewportY(void) const { return mViewport.Y; }

		void setToDevice(int setFlags = SET_ALL) const
		{
			if (setFlags & SET_RENDERTARGET)
			{
				if (setFlags & SET_VIEWPORT)
					D3D::setRenderTarget(0, mpSurf, mViewport);
				else
					D3D::setRenderTarget(0, mpSurf);
			}
			else if (setFlags & SET_VIEWPORT)
				D3D::setViewport(mViewport);
			
			if (setFlags & SET_MATRICES)
			{
				D3D::setTransform(D3DTS_VIEW, mCamera.worldToView());
				D3D::setTransform(D3DTS_PROJECTION, mCamera.viewToProj());
			}
		}
    		   		
	protected:
		IDirect3DSurface9* mpSurf;
		Camera mCamera;
		MatrixTracker mMatrixTracker;
		D3DVIEWPORT9 mViewport;

		void updateProjToScreenMatrix(void) 
		{
			mMatrixTracker.setProjToScreenMatrix(mViewport);
		}
	};
} // namespace gr

#endif // RENDER_VIEWPORT_H

//-----------------------------------------------------------------------------
// File: camera.h
// Copyright (C)2003 Blank Cartridge, Inc.
//-----------------------------------------------------------------------------
#pragma once
#ifndef CAMERA_H
#define CAMERA_H

//FIXME: Eliminate reliance on D3DX!

#include "common/math/vector.h"
#include "common/render/frustum.h"

#include <d3dx9math.h>

namespace gr
{
	class Camera
	{
	public:
		Camera() 
		{
			clear();
		}

		virtual ~Camera()
		{
		}
    
		void clear(void)
		{
			mWorldToView.setIdentity();
			mViewToWorld.setIdentity();
			mViewToProj.setIdentity();
			
			mWorldFrustum.set(mWorldToView * mViewToProj);
			mViewFrustum.set(mViewToProj);
		}

    const Matrix44& worldToView(void) const { return mWorldToView; }
		const Matrix44& viewToWorld(void) const { return mViewToWorld; }
		const Matrix44& viewToProj(void) const { return mViewToProj; }

		void setViewToProj(const Matrix44& viewToProj)
		{
			mViewToProj = viewToProj;
			
			mWorldFrustum.set(mWorldToView * mViewToProj);
			mViewFrustum.set(mViewToProj);
		}

    void setViewToProj(float aspect, float zNear, float zFar, float fullFov)
		{
			D3DXMatrixPerspectiveFovLH( 
				reinterpret_cast<D3DXMATRIX*>(&mViewToProj), 
				fullFov, 
				aspect, 
				zNear, 
				zFar);

			mWorldFrustum.set(mWorldToView * mViewToProj);
			mViewFrustum.set(mViewToProj);
		}

		void setWorldToView(const Matrix44& worldToView)
		{
			mWorldToView = worldToView;
			mViewToWorld = mWorldToView.inverse();
			mWorldFrustum.set(mWorldToView * mViewToProj);
		}

		void setWorldToView(const Vec4& pos, const Vec4& at, const Vec4& up, float roll = 0.0f)
		{
			mWorldToView = Matrix44::makeCamera(pos, at, up, roll);
			mViewToWorld = mWorldToView.inverse();
			mWorldFrustum.set(mWorldToView * mViewToProj);
		}

		const Frustum& worldFrustum(void) const
		{
			return mWorldFrustum;
		}

		const Frustum& viewFrustum(void) const
		{
			return mViewFrustum;
		}

		// May be very slightly inaccurate!
		const Vec4& getPos(void) const { return Vec4(mViewToWorld.getTranslate()).setW(1.0f); }

		Vec4 getAt(void) const { return mWorldToView.getColumn(2); }
		Vec4 getUp(void) const { return mWorldToView.getColumn(1); }
		Vec4 getRight(void) const { return mWorldToView.getColumn(0);	}
		
	protected:
		Matrix44 mWorldToView;
		Matrix44 mViewToWorld;
		Matrix44 mViewToProj;
		Frustum mWorldFrustum;
		Frustum mViewFrustum;
	};
	
} // namespace gr

#endif // CAMERA_H

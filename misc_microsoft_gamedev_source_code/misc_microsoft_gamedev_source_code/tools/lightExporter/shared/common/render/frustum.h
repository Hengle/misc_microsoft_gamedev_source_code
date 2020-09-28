//-----------------------------------------------------------------------------
// File: frustum.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef FRUSTUM_H
#define FRUSTUM_H

#include "common/math/plane.h"

namespace gr
{
	class Frustum
	{
	public:
		enum PlaneIndex
		{
			LEFT_PLANE,		// pos X
			RIGHT_PLANE,	// neg X
			BOTTOM_PLANE,	// pos Y
			TOP_PLANE,		// neg Y
			NEAR_PLANE,		// pos Z
			FAR_PLANE,		// neg Z
			NUM_PLANES,
			ALL_PLANES = (1 << NUM_PLANES) - 1
		};

		Frustum()
		{
		}

		Frustum(const Matrix44& proj)
		{
			set(proj);
		}
		
		Frustum(const Frustum& rhs)
		{
			*this = rhs;
		}

		Frustum& operator= (const Frustum& rhs)
		{
			for (int i = 0; i < NUM_PLANES; i++)
				mPlanes[i] = rhs.mPlanes[i];
			return *this;
		}

		Frustum& set(const Matrix44& proj)
		{
			float sign = 1.0f;

			for (int i = 0; i < NUM_PLANES; i++, sign *= -1.0f)
			{
				mPlanes[i].setFromEquation(
					(proj.getColumn(i >> 1) * sign + (Vec4((NEAR_PLANE != i) ? proj.getColumn(3) : Vec4(0.0f)))).normalize3()
					);
			}
			return *this;
		}

		Frustum& transform(const Matrix44& m)
		{
			for (int i = 0; i < NUM_PLANES; i++)
				mPlanes[i] = Plane::transformOrthonormal(mPlanes[i], m);
			return *this;
		}

		const Plane& plane(int i) const { return mPlanes[DebugRange(i, NUM_PLANES)]; }
	  
	private:
		Plane mPlanes[NUM_PLANES];
	};

} // namespace gr

#endif // FRUSTUM_H


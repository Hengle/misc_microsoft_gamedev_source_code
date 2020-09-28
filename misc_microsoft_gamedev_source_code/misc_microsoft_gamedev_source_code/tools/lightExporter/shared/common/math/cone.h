//-----------------------------------------------------------------------------
// File: cone.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef CONE_H
#define CONE_H

#include "common/math/vector.h"

namespace gr
{
	class Cone
	{
	public:
		Cone()
		{
		}
								
		// Accepts cone half angle in radians: [0, PI/2)
		Cone(const Vec3& apex, const Vec3& axis, float ang)
		{
			set(apex, axis, ang);
		}
				
		Cone(const Vec3& apex, const Vec3& axis, const Vec3& corner)
		{
			set(apex, axis, corner);
		}
		
		void set(const Vec3& apex, const Vec3& axis, float ang)
		{
			mApex = apex;
			mAxis = axis;
			mCosAng = cos(ang);
			mSinAng = sin(ang);
		}

		void set(const Vec3& apex, const Vec3& axis, const Vec3& corner)
		{
			mApex = apex;
			mAxis = axis;

			mCosAng = (axis * (corner - apex)) / (corner - apex).len();
			mSinAng = sin(acos(mCosAng));
		}

					float& cosAng(void)			{ return mCosAng; }
		const float cosAng(void) const { return mCosAng; }

					float& sinAng(void)				{ return mSinAng; }
		const float sinAng(void) const		{ return mSinAng; }

					Vec3& apex(void)				{ return mApex; }
		const Vec3& apex(void) const	{ return mApex; }

					Vec3& axis(void)				{ return mAxis; }
		const Vec3& axis(void) const	{ return mAxis; }
		
	private:
		float mCosAng;
		float mSinAng;
		Vec3 mApex;
		Vec3 mAxis;
	};
	
} // namespace gr

#endif // CONE_H

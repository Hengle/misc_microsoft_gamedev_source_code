//-----------------------------------------------------------------------------
// File: arcball.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef ARCBALL_H
#define ARCBALL_H

#include "common/math/quat.h"
#include "common/math/plane.h"
#include "common/math/intersection.h"

namespace gr
{
	Quat ArcBall(
		const Vec3& projPos,
		const Vec3& objPos, 
		const Vec3& prevDir,
		const Vec3& newDir,
		const float fudgeFactor = 4.0f)
	{
		Assert(prevDir.isUnit());
		Assert(newDir.isUnit());

		Vec3 normal(objPos - projPos);
		
		const float ooDist = normal.oneOverLen();
		Assert(ooDist > 0.0f);
		
		normal *= ooDist;

		const Plane plane(normal, normal * objPos);

		Vec3 u, v;
		EResult ures = Intersection::ray3Plane(u, plane, Ray3(projPos, prevDir));
		EResult vres = Intersection::ray3Plane(v, plane, Ray3(projPos, newDir));

		if ((ures != SUCCESS) || (vres != SUCCESS))
			return Quat::I;

		u = (u - objPos) * ooDist * fudgeFactor;
		v = (v - objPos) * ooDist * fudgeFactor;
		
		const float uLen = u.len();
		if (uLen > 1.0f)
			u /= uLen;
		else
			u -= plane.n * sqrt(1.0f - Math::Sqr(uLen));

		const float vLen = v.len();
		if (vLen > 1.0f)
			v /= vLen;
		else
			v -= plane.n * sqrt(1.0f - Math::Sqr(vLen));

		return Quat::makeRotationArc(u, v);
	}
	
} // namespace gr

#endif // ARCBALL_H

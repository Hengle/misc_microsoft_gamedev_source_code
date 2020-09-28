//-----------------------------------------------------------------------------
// File: intersection.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "common/math/plane.h"
#include "common/render/frustum.h"
#include "common/math/cone.h"
#include "common/math/hypersphere.h"
#include "common/math/hyper_ray_line.h"

namespace gr
{
	struct Intersection
	{
	  static EResult ray3Plane(
			Vec3& dest,
			const Plane& plane,
			const Ray3& ray,
			float maxDist = Math::fNearlyInfinite,
			float parallelEps = Math::fTinyEpsilon)
		{
			const float dirAlong = plane.n * ray.n;

			if (fabs(dirAlong) < parallelEps)
				return PARALLEL;

			const float posAlong = plane.n * ray.o;

			const float t = (plane.d - posAlong) / dirAlong;
			if ((t > maxDist) || (t < 0.0f))   
				return FAILURE;

			dest = ray.evaluate(t);

			return SUCCESS;
		}

		// true if touching
		static bool sphereSphere(const Sphere& a,	const Sphere& b)
		{
			return a.touches(b);
		}
		
		// Other useful funcs:
		// ray3vsSphere
		// ray3vsAABB
		// SpherevsAABB
		// AABBvsAABB
		// SpherevsPlane
		// ConevsPoint
		// ConevsPlane
		
		static EResult sphereFrustum(
			const Sphere& sphere,
			const Frustum& frustum)
		{
			EResult result = INSIDE;

			for (int i = 0; i < Frustum::NUM_PLANES; i++)
			{
				const float d = frustum.plane(i).distanceFromPoint(sphere.origin());
				
				if (d < -sphere.radius())
					return OUTSIDE;

				if (d <= sphere.radius())
					result = PARTIAL;
			}

			return result;
		}

		// Returns number of intersection points.
		static int raySphere(Vec3 pPoint[2], const Ray3& ray, const Sphere& sphere)
		{
			const Vec3 diff(ray.origin() - sphere.origin());
			const float a = ray.norm().squaredLen();
			const float b = diff * ray.norm();
			const float c = diff.squaredLen() - Math::Sqr(sphere.radius());
						
			float m[2];
			const float k = Math::Sqr(b) - a * c;

			if (k >= 0.0f)
			{
				if (k > 0.0f)
				{
					const float r = sqrt(k);
					
					const float ooA = 1.0f / a;
					m[0] = (-b - r) * ooA;
					m[1] = (-b + r) * ooA;

					if (m[0] >= 0.0f)
					{
						pPoint[0] = ray.origin() + m[0] * ray.norm();
						pPoint[1] = ray.origin() + m[1] * ray.norm();
						return 2;
					}
					else if (m[1] >= 0.0f)
					{
						pPoint[0] = ray.origin() + m[1] * ray.norm();
						return 1;
					}
				}
				else
				{
					if ((m[0] = -b / a) >= 0.0f)
					{
						pPoint[0] = ray.origin() + m[0] * ray.norm();
						return 1;
					}
				}
			}
		
			return 0;			
		}

		static bool conePoint(const Cone& c, const Vec3& p) 
		{
			const float a = c.axis() * (p - c.apex());

			if (a < 0.0f) 
				return false;
			
			return ((p - c.apex()).len2() * c.cosAng() * c.cosAng()) <= (a * a);
		}

		static bool coneSphere(const Cone& c, const Sphere& s)
		{
			const Vec3 diff(s.origin() - c.apex());
			const float frsqr = s.radius() * s.radius();
			const float flsqr = diff.len2();
			if (flsqr <= frsqr)
				return true;

			const float fdot = diff * c.axis();
			const float fdotsqr = fdot * fdot;
			const float fcossqr = c.cosAng() * c.cosAng();
			if ((fdotsqr >= flsqr * fcossqr) && (fdot > 0.0f))
				return true;

			const float fulen = sqrt(fabs(flsqr - fdotsqr));
			const float ftest = c.cosAng() * fdot + c.sinAng() * fulen;
			const float fdiscr = ftest * ftest - flsqr + frsqr;
			return fdiscr >= 0.0f && ftest >= 0.0f;
		}
	};

} // namespace gr

#endif // INTERSECTION_H

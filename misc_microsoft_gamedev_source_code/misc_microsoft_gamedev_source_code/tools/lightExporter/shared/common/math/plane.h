//-----------------------------------------------------------------------------
// File: plane.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef PLANE_H
#define PLANE_H

#include "common/math/vector.h"

namespace gr
{
	struct ParametricPlane;

	struct Plane
	{
		Vec3 n;
		// Positive distance along normal to plane's origin
		float d;			
	
		Plane()
		{
		}

		Plane(float x, float y, float z, float dist) : n(x, y, z), d(dist)
		{
		}

		Plane(const Vec3& a, const Vec3& b, const Vec3& c)
		{
			setFromTriangle(a, b, c);
		}

		Plane(const Vec3& norm, float dist) : n(norm), d(dist)
		{
		}

		Plane(const Vec3& norm, const Vec3& org) : n(norm), d(norm * org)
		{
		}
		
		Plane(const ParametricPlane& pp);

		Plane(const Vec4& equation) : n(equation), d(-equation[3])
		{
		}

		Plane(const Vec4& norm, float dist) : n(norm), d(dist)
		{
		}

		bool isValid(void) const
		{
			return 0.0f != n.norm();
		}

		Plane& setInvalid(void) 
		{
			n.setZero();
			d = 0;
			return *this;
		}

		// true on failure
		bool setFromTriangle(const Vec3& a, const Vec3& b, const Vec3& c, float eps = Math::fTinyEpsilon)
		{
			n = (c - b) % (a - b);
			
			if (n.squaredLen() <= eps)
			{
				n.setZero();
				d = 0.0;
				return true;
			}

			d = n.normalize() * b;

			return false;
		}

		Plane& setFromNormalOrigin(const Vec3& norm, const Vec3& org)
		{
			n = norm;
			d = norm * org;
			return *this;
		}

		Plane& setFromParametricPlane(const ParametricPlane& pp);

		Plane& normalize(void)
		{
			float len = n.tryNormalize();
			if (0.0f == len)
			{
				setInvalid();
				return *this;
			}
			len = 1.0f / len;
			n *= len;
			d *= len;
			return *this;
		}

		Vec3 origin(void) const
		{
			return n * d;
		}

		const Vec3& normal(void) const	{ return n; }
					Vec3& normal(void)				{ return n; }

		const float& distance(void) const { return d; }
					float& distance(void)				{ return d; }

		bool operator== (const Plane& b) const
		{
			return (n == b.n) && (d == b.d);
		}

		bool operator!= (const Plane& b) const
		{
			return !(*this == b);
		}

		float distanceFromPoint(const Vec3& p) const
		{
			return p * n - d;
		}

		Plane flipped(void) const
		{
			return Plane(-n, -d);
		}

		Vec4 equation(void) const
		{
			return Vec4(n[0], n[1], n[2], -d);
		}
		
		const Plane& set(float x, float y, float z, float d)
		{
			n[0] = x;
			n[1] = y;
			n[2] = z;
			d = d;
			return *this;
		}

		const Plane& setFromEquation(const Vec4& v) 
		{
			n = v;
			d = -v[3];
			return *this;
		}

		// x0 axis change with respect to axis x1
		float gradient(int x0, int x1, float eps = Math::fTinyEpsilon) const
		{
			const float dA = -n[x1];
			const float dB = n[x0];
			return (fabs(dB) <= eps) ? 0.0f : dA / dB;
		}
		
		// Slow! This could be greatly optimized if assumptions could be made about the matrix.
		friend Plane operator* (const Plane& p, const Matrix44& m)
		{
			Plane ret;
			ret.n = Matrix44::transformNormalTransposed(m.inverse(), p.n).normalize();
			ret.d = Matrix44::transformPoint(p.origin(), m).dot3(ret.n);
			return ret;
		}

		static Plane transformOrthonormal(const Plane& p, const Matrix44& m)
		{
			Plane ret;
			ret.n = Matrix44::transformNormal(p.n, m);
	    ret.d = p.d + Vec3(m.getTranslate()) * ret.n;
			return ret;
		}

		Vec3 project(const Vec3& p) const
		{
			return p - n * distanceFromPoint(p);
		}

		// returns value of desiredAxis given the other two
		float project(const Vec3& p, int desiredAxis) const
		{
			const int axis2[] = { 1, 0, 0 };
			const int axis3[] = { 2, 2, 1 };

			const int a2 = axis2[DebugRange(desiredAxis, 3)];
			const int a3 = axis3[desiredAxis];

			if (0.0f == n[desiredAxis])
				return 0.0f;
			return (d - p[a2] * n[a2] - p[a3] * n[a3]) / n[desiredAxis];
		}
		
		void clipPoly(std::vector<Vec3>& result, const std::vector<Vec3>& verts) const
		{
			result.erase(result.begin(), result.end());

			if (verts.empty())
				return;

			float prevDist = distanceFromPoint(verts[0]);
			const int numVerts = static_cast<int>(verts.size());
				
			for (int prev = 0; prev < numVerts; prev++)
			{
				int cur = Math::NextWrap(prev, numVerts);
				float curDist = distanceFromPoint(verts[cur]);

				if (prevDist >= 0.0f)
					result.push_back(verts[prev]);

				if (((prevDist < 0.0f) && (curDist > 0.0f)) ||
						((prevDist > 0.0f) && (curDist < 0.0f)))
				{
					result.push_back(Vec3::lerp(verts[prev], verts[cur], prevDist / (prevDist - curDist)));
				}
				
				prevDist = curDist;
			}
		}
	};

	struct ParametricPlane
	{
		Vec3 o;
		Vec3 u;
		Vec3 v;

		ParametricPlane()
		{
		}

		ParametricPlane(const Plane& p)
		{
			setFromPlane(p);
		}

		ParametricPlane(const Vec3& orig, const Vec3& uAxis, const Vec3& vAxis) :
			o(orig), u(uAxis), v(vAxis)
		{
		}

		ParametricPlane(const Vec3& orig, const Vec3& norm)
		{
			setFromOriginNormal(orig, norm);
		}

		ParametricPlane& setFromOriginAxes(const Vec3& orig, const Vec3& uAxis, const Vec3& vAxis)
		{
			o = orig;
			u = uAxis;
			v = vAxis;
			return *this;
		}

		Vec2 project(const Vec3& p) const
		{
			return Vec2((p - o) * u, (p - o) * v);
		}

		Vec3 point(const Vec2& p) const
		{
			return o + u * p[0] + v * p[1];
		}

		Vec3 normal(void) const
		{
			return u % v;
		}

		const Vec3& origin(void) const	{ return o; }
					Vec3& origin(void)				{ return o; }

		const Vec3& uAxis(void) const	{ return u; }
	   			Vec3& uAxis(void)				{ return u; }

		const Vec3& vAxis(void) const	{ return v; }
	  			Vec3& vAxis(void)				{ return v; }

		ParametricPlane& normalize(void)
		{
			u.normalize();
			v.normalize();
			return *this;
		}

		ParametricPlane& setFromPlane(const Plane& p)
		{
			o = p.origin();
			u = Vec3::removeCompUnit(Vec3::makeAxisVector(p.n.minorAxis()), p.n).normalize();
			v = p.n % u;
			return *this;
		}

		ParametricPlane& setFromOriginNormal(const Vec3& orig, const Vec3& norm)
		{
			o = orig;
			u = Vec3::removeCompUnit(Vec3::makeAxisVector(norm.minorAxis()), norm).normalize();
			v = norm % u;
			return *this;
		}

		static ParametricPlane makePlanarProjection(const Plane& p)
		{
			int x0, x1;

			switch (p.n.majorAxis())
			{
				case 0: x0 = 1; x1 = 2;	if (p.n[0] > 0.0f) { x0 = 2; x1 = 1; } break;
				case 1: x0 = 2; x1 = 0;	if (p.n[1] > 0.0f) { x0 = 0; x1 = 2; } break;
				case 2: x0 = 0; x1 = 1;	if (p.n[2] > 0.0f) { x0 = 1; x1 = 0; } break;
			}

			return ParametricPlane(Vec3(0), Vec3::makeAxisVector(x0), Vec3::makeAxisVector(x1));
		}
	};

	inline Plane::Plane(const ParametricPlane& pp) :
		n(pp.normal()),
		d(pp.origin() * pp.normal())
	{
	}

	inline Plane& Plane::setFromParametricPlane(const ParametricPlane& pp)
	{
		n = pp.normal();
		d = pp.origin() * pp.normal();
		return *this;
	}

} // namespace gr

#endif // PLANE_H
















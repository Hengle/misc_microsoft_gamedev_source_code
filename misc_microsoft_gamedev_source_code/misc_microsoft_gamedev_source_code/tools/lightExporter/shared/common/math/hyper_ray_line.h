//-----------------------------------------------------------------------------
// File: hyper_ray_line.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef HYPER_RAY_LINE_H
#define HYPER_RAY_LINE_H

#include "common/math/vector.h"

namespace gr
{
	template<class T>
	struct HyperRay
	{
		T o, n;

		HyperRay(void) 
		{ 
		}
		
		HyperRay(const T& origin, const T& norm) : o(origin), n(norm) 
		{ 
		}

		T evaluate(float t) const
		{
			return n * t + o;
		}

		const T& origin(void) const { return o; }
					T& origin(void)			 { return o; }

		const T& norm(void) const { return n; }
					T& norm(void)			 { return n; }
	};

	typedef HyperRay<Vec2> Ray2;
	typedef HyperRay<Vec3> Ray3;
	
	template<class T>
	struct HyperLine
	{
		T s, e;
		
		HyperLine() 
		{ 
		}

		HyperLine(const T& start, const T& end) : s(start), e(end) 
		{ 
		}

		const T& start(void) const { return s; }
					T& start(void)				{ return s; }
		
		const T& end(void) const		{ return e; }
					T& end(void)					{ return e; }

		T vector(void) const
		{
			return end - start;
		}

		T unitDir(void) const
		{
			return vector.normalize();
		}

		float len(void) const
		{
			return vector.len();
		}

		T evaluate(float t) const
		{	
			return T::Lerp(s, e, t);
		}

		HyperRay<T> ray(void) const
		{
			return HyperRay<T>(s, unitDir());
		}

		const HyperLine& invert(void) 
		{
			std::swap(s, e);
			return *this;
		}
			
		HyperLine inverted(void) const
		{
			return HyperLine(e, s);
		}

		static bool equalTol(const HyperLine& a, const HyperLine& b, float tol = FSmallEpsilon)
		{
			return T::equalTol(a.s, b.s, tol) && T::equalTol(a.e, b.e, tol);
		}
	};

	typedef HyperLine<Vec2> HyperLine2;
	typedef HyperLine<Vec3> HyperLine3;

} // namespace gr

#endif // HYPER_RAY_LINE_H

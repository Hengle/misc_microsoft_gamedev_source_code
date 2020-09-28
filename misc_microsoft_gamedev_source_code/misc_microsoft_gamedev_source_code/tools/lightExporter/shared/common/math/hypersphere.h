//-----------------------------------------------------------------------------
// File: hypersphere.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef HYPERSPHERE_H
#define HYPERSPHERE_H

#include "common/math/vector.h"
#include "common/utils/stream.h"
#include "common/utils/logfile.h"

namespace gr
{
	template<class T>
	struct Hypersphere
	{
		T o;
		float r;

		Hypersphere()
		{
		}

		Hypersphere(const T& origin, float radius) : o(origin), r(radius)
		{
		}

		void clear(void)
		{
			o.setZero();
			r = 0;
		}
		
		Hypersphere(EClear e)
		{
			clear();
		}

		const T& origin(void) const	{ return o; }
					T& origin(void)				{ return o; }

		const float  radius(void) const	{ return r; }
				  float& radius(void)				{ return r; }

		float dist2(const T& p) const
		{
			return (p - o).norm();
		}

		bool contains(const T& p) const
		{
			return dist2(p) < r * r;
		}

		bool containsOrTouches(const T& p) const
		{
			return dist2(p) <= r * r;
		}
		
		bool touches(const Hypersphere& h) const
		{
			return dist2(h.o) <= Math::Sqr(r + h.r);
		}

		Hypersphere& expand(const T& point)
		{
			const float dist2 = (point - o).squaredLen();
			if (dist2 > r * r)
				r = sqrt(dist2);
		}
		
		void log(LogFile& l) const
		{
			l.printf("Origin: ");
			o.log(l);
			l.printf("Radius: %f\n", r);
		}
		
		template<class T>
		friend Stream& operator<< (Stream& dst, const Hypersphere<T>& src)
		{
			return dst << src.o << src.r;
		}
		
		template<class T>
		friend Stream& operator>> (Stream& src, Hypersphere<T>& dst)
		{
			return src >> dst.o >> dst.r;
		}
	};

	typedef Hypersphere<Vec2> Circle;
	typedef Hypersphere<Vec3> Sphere;
	
} // namespace gr

#endif // HYPERSPHERE_H

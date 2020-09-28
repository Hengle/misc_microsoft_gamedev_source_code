// File: cubemap.h
#pragma once
#ifndef CUBEMAP_H
#define CUBEMAP_H

#include "common/math/vector.h"

namespace gr
{
	class CubeMap
	{
	public:
		typedef std::vector<Vec3> Vec3Vec;
		
		CubeMap(int width, int height);
						
		int width(void) const { return mWidth; }
		int height(void) const { return mHeight; }
		
		const Vec3Vec& face(int i) const { return mFaces[DebugRange(i, 6)]; }
					Vec3Vec& face(int i)				{ return mFaces[DebugRange(i, 6)]; }
				
		Vec3& pixel(int i, int x, int y)
		{
			return mFaces[DebugRange(i, 6)][DebugRange(x, mWidth) + DebugRange(y, mHeight) * mWidth];
		}
		
		const Vec3& pixel(int i, int x, int y) const
		{
			return mFaces[DebugRange(i, 6)][DebugRange(x, mWidth) + DebugRange(y, mHeight) * mWidth];
		}
		
		Vec3 vector(int f, int x, int y, bool normalize = true, float ofsX = .5f, float ofsY = .5f) const;
		
		// returns solid angle of cubemap face (f) texel (x,y)
		// sum of all texels is 4*Pi
		double sphericalArea(int f, int x, int y) const;
		
		// Point sampling
		Vec3 sample(const Vec3& dir) const;
						
	protected:
		std::vector<Vec3Vec> mFaces;
		int mWidth, mHeight;
	};
	
} // namespace gr

#endif // CUBEMAP_H

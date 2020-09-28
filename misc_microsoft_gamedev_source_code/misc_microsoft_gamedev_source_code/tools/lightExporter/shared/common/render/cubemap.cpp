// File: cubemap.cpp
#include "cubemap.h"
#include "common/math/half_float.h"

namespace gr
{
	CubeMap::CubeMap(int width, int height) :
		mWidth(width),
		mHeight(height),
		mFaces(6)
	{
		for (int i = 0; i < 6; i++)
			mFaces[i].resize(mWidth * mHeight);
	}
			
	Vec3 CubeMap::vector(int f, int x, int y, bool normalize, float ofsX, float ofsY) const
	{
		DebugRange(f, 0, 6);
		
		DebugRange(x, mWidth);
		DebugRange(y, mHeight);
				
		float fx = (x + ofsX) / mWidth;
		float fy = (y + ofsY) / mHeight;

		fx = fx * 2.0f - 1.0f;
		fy = fy * 2.0f - 1.0f;
		
		Vec3 ret;
		switch (f)
		{
			case 0: ret[0] = 1.0f;  ret[1] = -fy;		ret[2] = -fx;		break;
			case 1: ret[0] = -1.0f; ret[1] = -fy;		ret[2] = fx;		break;
			case 2: ret[0] = fx;		ret[1] = 1.0f;	ret[2] = fy;		break;
			case 3: ret[0] = fx;		ret[1] = -1.0f; ret[2] = -fy;		break;
			case 4: ret[0] = fx;		ret[1] = -fy;		ret[2] = 1.0f;	break;
			case 5: ret[0] = -fx;		ret[1] = -fy;		ret[2] = -1.0f;	break;
		}

		if (normalize)
			ret.normalize();
		
		return ret;
	}
	
	double CubeMap::sphericalArea(int f, int x, int y) const
	{
#if 0	
		double lat[5];
		double lng[5];

		int t = 0;
		for (int yc = 0; yc < 2; yc++)
		{
			for (int xc = 0; xc < 2; xc++)
			{
				Vec3 r(vector(0, x, y, true, (xc ^ yc), yc));
								
				lng[t] = atan2(r[1], r[0]);
				lat[t] = asin(r[2]);
				t++;
			}
		}
		
		lng[t] = lng[t - 1];
		lat[t] = lat[t - 1];

		return (Math::fPi * SphericalPolyArea(lat, lng, 4)) / 180.0f;
#endif		
		
		// Check radiosity hemicube references for alternate methods?
		
		// Computes differential solid angle
		// Doesn't take projected shape into account, not as accurate ???
		// But SphericalPolyArea() suffers from accuracy problems!
		Vec3 r(vector(0, x, y, false, .5f, .5f));
		Vec3 rNorm(r.normalized());
		
		Vec3 n;
		if (fabs(r[0]) == 1.0f)
			n = Vec3(-r[0], 0.0f, 0.0f);
		else if (fabs(r[1]) == 1.0f)
			n = Vec3(0.0f, -r[1], 0.0f);
		else
			n = Vec3(0.0f, 0.0f, -r[2]);

		float dA = (2.0f / mWidth) * (2.0f / mHeight);

		float cosB = -n * rNorm;

		float dw = (cosB * dA) / r.len2();
		
		return dw;
	}

	// Point sampling
	Vec3 CubeMap::sample(const Vec3& dir) const
	{
		const float x = fabs(dir[0]);
		const float y = fabs(dir[1]);
		const float z = fabs(dir[2]);
		
		int f;
		float xt, yt, m;
		
		if ((x >= y) && (x >= z))
		{
			m = x;
			yt = -dir[1];
			if (dir[0] >= 0.0f)
			{
				f = 0;
				xt = -dir[2];
			}
			else
			{
				f = 1;
				xt = dir[2];
			}
		}
		else if ((y >= x) && (y >= z))
		{
			m = y;
			xt = dir[0];
			if (dir[1] >= 0.0f)
			{
				f = 2;
				yt = dir[2];
			}
			else
			{
				f = 3;
				yt = -dir[2];
			}
		}
		else
		{
			m = z;
			yt = -dir[1];
			if (dir[2] >= 0.0f)
			{
				f = 4;
				xt = dir[0];
			}
			else
			{
				f = 5;
				xt = -dir[0];
			}
		}

		float oom = .5f / m;
		
		xt = ((xt * oom) + .5f) * mWidth + .5f;
		yt = ((yt * oom) + .5f) * mHeight + .5f;
		
		const int px = Math::Clamp(Math::FloatToIntTrunc(xt), 0, mWidth - 1);
		const int py = Math::Clamp(Math::FloatToIntTrunc(yt), 0, mHeight - 1);
		
		return pixel(f, px, py);
	}
} // namespace gr


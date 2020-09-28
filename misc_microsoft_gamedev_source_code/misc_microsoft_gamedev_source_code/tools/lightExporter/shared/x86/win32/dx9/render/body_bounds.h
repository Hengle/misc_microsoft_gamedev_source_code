// body_bounds.h
#pragma once
#ifndef BODY_BOUNDS_H
#define BODY_BOUNDS_H

#include "common/math/general_vector.h"

namespace gr
{
	class NodeBounds
	{
		AABB mBounds;
		Vec<4> mBoundsToWorld[4];

	public:
		NodeBounds() { }
		
		NodeBounds(const AABB& bounds, const Matrix44& boundsToWorld) :
			mBounds(bounds)
		{
			mBoundsToWorld[0] = boundsToWorld.getRow(0);
			mBoundsToWorld[1] = boundsToWorld.getRow(1);
			mBoundsToWorld[2] = boundsToWorld.getRow(2);
			mBoundsToWorld[3] = boundsToWorld.getRow(3);					
		}
		
		const AABB& getBounds(void) const 
		{ 
			return mBounds; 
		}
		
		Matrix44 getBoundsToWorld(void) const
		{
			Matrix44 ret;
			ret.setRow(0, mBoundsToWorld[0]);
			ret.setRow(1, mBoundsToWorld[1]);
			ret.setRow(2, mBoundsToWorld[2]);
			ret.setRow(3, mBoundsToWorld[3]);
			return ret;
		}
		
		Vec4 center(void) const
		{
			return Vec4(mBounds.center(), 1.0f) * getBoundsToWorld();
		}
		
		Vec4 corner(int i) const
		{
			return Vec4(mBounds.corner(i), 1.0f) * getBoundsToWorld();
		}
	};
	
	typedef std::vector<NodeBounds> NodeBoundsVec;

	class BodyBounds
	{
	public:
		BodyBounds() : mCenter(0)
		{
		}
		
		void clear(void)
		{
			mNodeBounds.erase(mNodeBounds.begin(), mNodeBounds.end());
			mCenter.setZero();
		}
		
		void insert(const NodeBounds& bounds)
		{
			mNodeBounds.push_back(bounds);
		}
		
		int numNodes(void) const { return mNodeBounds.size();	}
		
		const NodeBounds& operator[] (int i) const	{ return mNodeBounds[DebugRange(i, numNodes())]; }
					NodeBounds& operator[] (int i)				{ return mNodeBounds[DebugRange(i, numNodes())]; }
					
		void findCenter(void)
		{
			mCenter.setZero();
			
			float totalVolume = 0.0f;
			for (int i = 0; i < numNodes(); i++)
				totalVolume += mNodeBounds[i].getBounds().volume();
									
			for (int i = 0; i < numNodes(); i++)
				mCenter += mNodeBounds[i].center() * mNodeBounds[i].getBounds().volume();
				
			mCenter /= totalVolume;
		}
						
		const Vec3& center(void) const
		{
			return mCenter;
		}
					
	private:
		NodeBoundsVec mNodeBounds;
		Vec3 mCenter;
	};
};

#endif // BODY_BOUNDS_H

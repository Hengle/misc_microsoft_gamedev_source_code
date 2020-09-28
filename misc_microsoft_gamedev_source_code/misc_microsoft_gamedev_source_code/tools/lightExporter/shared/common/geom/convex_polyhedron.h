//-----------------------------------------------------------------------------
// File: convex_polyhedron.h
// x86 optimized vector/matrix classes.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef CONVEX_POLYHEDRON_H
#define CONVEX_POLYHEDRON_H

#include "common/math/vector.h"
#include "common/geom/indexed_tri.h"
#include "common/math/intersection.h"

#include <algorithm>

namespace gr
{
	class ConvexPolyhedron
	{
	public:
		typedef Vec3 VertType;
		typedef IndexedTri16 IndexedTriType;

		typedef std::vector<VertType> VertVector;
		typedef std::vector<IndexedTriType> IndexedTriVector;

		ConvexPolyhedron()
		{
		}

		ConvexPolyhedron(const VertVector& verts, const IndexedTriVector& tris) :
			mVerts(verts),
			mTris(tris)
		{
		}

		ConvexPolyhedron& set(const VertVector& verts, const IndexedTriVector& tris)
		{
			mVerts = verts;
			mTris = tris;
			return *this;
		}

		void clear(void)
		{
			mVerts.clear();
			mTris.clear();
		}

		void erase(void)
		{
			mVerts.erase(mVerts.begin(), mVerts.end());
			mTris.erase(mTris.begin(), mTris.end());
		}

		int numVerts(void) const { return static_cast<int>(mVerts.size()); }
		int numTris(void) const { return static_cast<int>(mTris.size()); }

		bool empty(void) const { return !numTris(); }

    ConvexPolyhedron& createCappedCone(const Vec3& apex, const Vec3& dir, float fullAngle, float dist, int NumCirclePoints = 10)
		{
			const float t = fullAngle * .5f;
			float x = cos(t) * dist;
			float y = sin(t) * dist;
			x = (x + dist) / 2.0f;
			y = y / 2.0f;
			const float normFactor = dist / sqrt(x * x + y * y);
			const float normRadius = dist * normFactor;

			mVerts.resize(NumCirclePoints + 2);
			mVerts[0] = apex;
			mVerts[1] = apex + dir * normRadius;

      float backRad = tan(fullAngle * .5f) * normRadius;
			const ParametricPlane backPlane(mVerts[1], dir);
			
      for (int i = 0; i < NumCirclePoints; i++)
			{
				const float rad = i * Math::fTwoPi / float(NumCirclePoints);
				
				Vec3 p(backPlane.point(Vec2(cos(rad), sin(rad)) * backRad));

				Ray3 ray(mVerts[0], (p - mVerts[0]).normalize());

				Vec3 points[2];
				int numPoints = Intersection::raySphere( 
					points,
					ray,
					Sphere(apex, normRadius));
			
				if (numPoints)
					p = points[0];

				mVerts[i + 2] = p;
			}

      mTris.resize(NumCirclePoints * 2);

			for (int i = 0; i < NumCirclePoints; i++)
			{
				mTris[i][2] = 0;
				mTris[i][1] = 2 + i;
				mTris[i][0] = 2 + ((i + 1) % NumCirclePoints);
			}

			for (int i = 0; i < NumCirclePoints; i++)
			{
				mTris[NumCirclePoints + i][2] = 1;
				mTris[NumCirclePoints + i][1] = 2 + ((i + 1) % NumCirclePoints);
				mTris[NumCirclePoints + i][0] = 2 + i;
			}

			return *this;
		}

		ConvexPolyhedron& createSphere(const Vec3& orig, float radius, int nPitch, int nYaw)		
		{
			Assert(nPitch >= 3);
			Assert(nYaw >= 4);

			mVerts.resize(2 + (nPitch - 2) * nYaw);
			
			int v = 0;
			for (int i = 0; i < nPitch; i++)
			{
				const float pitch = Math::Lerp(-Math::fHalfPi, Math::fHalfPi, i * 1.0f / float(nPitch - 1));

				const int yawLimit = ((i > 0) && (i < nPitch - 1)) ? nYaw : 1;
					
				for (int j = 0; j < yawLimit; j++)
				{
					const float yaw = Math::Lerp(-Math::fPi, Math::fPi, j * 1.0f / float(yawLimit));

					mVerts[DebugRange<int>(v, mVerts.size())] = orig + Vec3(Vec4::makeCartesian(Vec4(yaw, pitch, radius, 0)));
					v++;
				}
			}

			const int numVerts = static_cast<int>(mVerts.size());
			Assert(v == numVerts);

			mTris.resize(nYaw * 2 + (nPitch - 3) * nYaw * 2);

			const int bottom = 0;
			const int top = v - 1;
			const int firstRow = 1;
			const int vertsPerRow = nYaw;
			const int lastRow = firstRow + vertsPerRow * (nPitch - 3);
						
			const int a = 0;
			const int b = 1;
			const int c = 2;

			int t = 0;
			for (int i = 0; i < nPitch - 3; i++)
			{
				const int startVert = firstRow + vertsPerRow * i;
				for (int j = 0; j < nYaw; j++)
				{
					DebugRange<int>(t, mTris.size());
					mTris[t][a] = DebugRange(startVert + j, numVerts);
					mTris[t][b] = DebugRange(startVert + ((j + 1) % vertsPerRow), numVerts);
					mTris[t][c] = DebugRange(startVert + j + vertsPerRow, numVerts);
					t++;

					DebugRange<int>(t, mTris.size());
					mTris[t][a] = DebugRange(startVert + ((j + 1) % vertsPerRow), numVerts);
					mTris[t][b] = DebugRange(startVert + ((j + 1) % vertsPerRow) + vertsPerRow, numVerts);
					mTris[t][c] = DebugRange(startVert + j + vertsPerRow, numVerts);
					t++;

				}
			}

      for (int i = 0; i < nYaw; i++)
			{
				DebugRange<int>(t, mTris.size());
				mTris[t][a] = DebugRange(bottom, numVerts);
				mTris[t][b] = DebugRange(firstRow + ((i + 1) % vertsPerRow), numVerts);
				mTris[t][c] = DebugRange(firstRow + i, numVerts);
				t++;
			}

			for (int i = 0; i < nYaw; i++)
			{
				DebugRange<int>(t, mTris.size());
				mTris[t][a] = DebugRange(top, numVerts);
				mTris[t][b] = DebugRange(lastRow + i, numVerts);
				mTris[t][c] = DebugRange(lastRow + ((i + 1) % vertsPerRow), numVerts);
				t++;
			}

			Assert(t == static_cast<int>(mTris.size()));

			//mTris.resize(t);

			return *this;
		}

		const VertVector& getVerts(void) const	{ return mVerts; }
					VertVector& getVerts(void)				{ return mVerts; }
		
		const IndexedTriVector& getTris(void) const { return mTris; }
					IndexedTriVector& getTris(void)				{ return mTris; }

		const Vec3& vert(int i) const { return mVerts[DebugRange<int>(i, mVerts.size())]; }
		const IndexedTriType& tri(int i) const { return mTris[DebugRange<int>(i, mTris.size())]; }

		void clipAgainstFrustum(ConvexPolyhedron& dst, const Frustum& frustum) const
		{
			clipAgainstPlane(dst, frustum.plane(0));
			if (dst.empty())
				return;

			ConvexPolyhedron temp;
			
			ConvexPolyhedron* pSrc = &dst;
			ConvexPolyhedron* pDst = &temp;
			for (int i = 1; i < Frustum::NUM_PLANES; i++)
			{
        pSrc->clipAgainstPlane(*pDst, frustum.plane(i), false);//i == Frustum::NEAR_PLANE);
				std::swap(pSrc, pDst);

				//if (pDst->empty())
				if (pSrc->empty()) // fixed Jan 20 2004
					break;
			}
			
			if (pSrc != &dst)
				dst = *pSrc;
		}
				
		void clipAgainstPlane(
			ConvexPolyhedron& dst, 
			const Plane& plane,
			bool cap = true) const
		{
			VertVector clippedPoly;
			clippedPoly.reserve(8);

			dst.erase();

			ParametricPlane pp(plane);
			
			VertVector capPoly(4);
			VertVector capPolyTemp;
			if (cap)
			{
				capPoly.reserve(32);
				capPolyTemp.reserve(32);
	      		
				capPoly[3] = pp.point(Vec2(-1e+6, -1e+6));
				capPoly[2] = pp.point(Vec2(+1e+6, -1e+6));
				capPoly[1] = pp.point(Vec2(+1e+6, +1e+6));
				capPoly[0] = pp.point(Vec2(-1e+6, +1e+6));
			}

			VertVector srcTri(3);            
			for (int i = 0; i < numTris(); i++)
			{
				srcTri[0] = vert(tri(i)[0]);
				srcTri[1] = vert(tri(i)[1]);
				srcTri[2] = vert(tri(i)[2]);

				const Vec3& a = srcTri[2];
				const Vec3& b = srcTri[1];
				const Vec3& c = srcTri[0];
				
				clipPoly(clippedPoly, srcTri, plane);
        
				if (clippedPoly.size() < 3)
					continue;

				dst.addPoly(clippedPoly);
								
				if ((cap) && (capPoly.size() >= 3))
				{
					Vec3 n((c - b) % (a - b));
					if (n.squaredLen() >= Math::fTinyEpsilon)
					{
						n.normalize();
										
						const Plane triPlane(n, n * b);
						
						clipPoly(capPolyTemp, capPoly, triPlane);
						capPoly.swap(capPolyTemp);
					}
				}
			}

			if ((cap) && (dst.numTris()))
			{
				if (capPoly.size() >= 3)
					dst.addPoly(capPoly);
			}
		}
		
		AABB getBounds(void) const
		{
			AABB bounds(AABB::eInitExpand);
			
			for (int i = 0; i < mVerts.size(); i++)
				bounds.expand(mVerts[i]);
			
			return bounds;
		}
		
		void generateCap(VertVector& dst,	const Plane& plane) const
		{
			dst.erase(dst.begin(), dst.end());

			ParametricPlane pp(plane);
			
			VertVector capPoly(4);
			VertVector capPolyTemp;
			capPoly.reserve(32);
			capPolyTemp.reserve(32);
	      		
			capPoly[3] = pp.point(Vec2(-1e+6, -1e+6));
			capPoly[2] = pp.point(Vec2(+1e+6, -1e+6));
			capPoly[1] = pp.point(Vec2(+1e+6, +1e+6));
			capPoly[0] = pp.point(Vec2(-1e+6, +1e+6));

			VertVector srcTri(3);            
			for (int i = 0; i < numTris(); i++)
			{
				srcTri[0] = vert(tri(i)[0]);
				srcTri[1] = vert(tri(i)[1]);
				srcTri[2] = vert(tri(i)[2]);

				const Vec3& a = srcTri[2];
				const Vec3& b = srcTri[1];
				const Vec3& c = srcTri[0];
				
				Vec3 n((c - b) % (a - b));
				if (n.squaredLen() >= Math::fTinyEpsilon)
				{
					n.normalize();
										
					const Plane triPlane(n, n * b);
						
					clipPoly(capPolyTemp, capPoly, triPlane);
					capPoly.swap(capPolyTemp);
					
					if (capPoly.size() < 3)
						break;
				}
			}

			if (capPoly.size() >= 3)
				dst = capPoly;
		}
		
		class VertexComparer
		{
			const VertVector& mVerts;
			
		public:
			VertexComparer(const VertVector& verts) : mVerts(verts)
			{
			}
			
			bool operator() (int i, int j) const
			{
				for (int a = 0; a < 3; a++)
				{
					if (mVerts[i][a] < mVerts[j][a])
						return true;
					else if (mVerts[i][a] != mVerts[j][a])
						return false;
				}
				
				return false;
			}
		};
		
		void weld(float eps = .0125f)
		{
			if (mVerts.size() < 2)
				return;
			
			IntVec indices(mVerts.size());
			
			for (int i = 0; i < mVerts.size(); i++)
				indices[i] = i;
				
			VertexComparer comp(mVerts);
				
			std::sort(indices.begin(), indices.end(), comp);
			
			IntVec oldToNew(mVerts.size());
			for (int i = 0; i < mVerts.size(); i++)
				oldToNew[i] = -1;
						
			VertVector newVerts;
			newVerts.reserve(mVerts.size() / 2);
									
			for (int i = 0; i < mVerts.size(); i++)
			{
				if (-1 != oldToNew[indices[i]])
					continue;
									
				const VertType& vi = mVerts[DebugRange(indices[i], mVerts.size())];
				
				const int newVertIndex = newVerts.size();
				oldToNew[indices[i]] = newVertIndex;
				newVerts.push_back(vi);

				for (int j = i + 1; j < mVerts.size(); j++)
				{
					const VertType& vj = mVerts[DebugRange(indices[j], mVerts.size())];
					
					VertType diff(vj - vi);
					
					if (diff[0] > eps)
						break;
						
					if (
							(fabs(vj[1] - vi[1]) < eps) && 
							(fabs(vj[2] - vi[2]) < eps)
						)
					{
						oldToNew[indices[j]] = newVertIndex;
					}
				}
			}
						
			std::swap(newVerts, mVerts);
			
			for (int i = 0; i < mTris.size(); i++)
			{
				mTris[i][0] = oldToNew[DebugRange(mTris[i][0], oldToNew.size())];
				mTris[i][1] = oldToNew[DebugRange(mTris[i][1], oldToNew.size())];
				mTris[i][2] = oldToNew[DebugRange(mTris[i][2], oldToNew.size())];
			}
		}
		    	
	private:

		VertVector mVerts;
		IndexedTriVector mTris;
		
		static void clipPoly(VertVector& result, const VertVector& verts, const Plane& plane) 
		{
			result.erase(result.begin(), result.end());

			if (verts.empty())
				return;

			float prevDist = plane.distanceFromPoint(verts[0]);
			const int numVerts = static_cast<int>(verts.size());
				
			for (int prev = 0; prev < numVerts; prev++)
			{
				int cur = Math::NextWrap(prev, numVerts);
				float curDist = plane.distanceFromPoint(verts[cur]);

				if (prevDist >= 0.0f)
					result.push_back(verts[prev]);

				if (((prevDist < 0.0f) && (curDist > 0.0f)) ||
						((prevDist > 0.0f) && (curDist < 0.0f)))
				{
					result.push_back(VertType::lerp(verts[prev], verts[cur], prevDist / (prevDist - curDist)));
				}
				
				prevDist = curDist;
			}
		}

		void addPoly(const VertVector& poly)
		{
			Assert(poly.size() >= 3);

			int firstVert = static_cast<int>(mVerts.size());
			mVerts.push_back(poly[0]);
							
			for (int j = 0; j < poly.size() - 2; j++)
			{
				mVerts.push_back(poly[1 + j]);
				mVerts.push_back(poly[2 + j]);
			
				mTris.push_back(IndexedTriType(firstVert, firstVert + 1 + j * 2, firstVert + 2 + j * 2));
			}
		}
	}; 

} // namespace gr

#endif // CONVEX_POLYHEDRON_H


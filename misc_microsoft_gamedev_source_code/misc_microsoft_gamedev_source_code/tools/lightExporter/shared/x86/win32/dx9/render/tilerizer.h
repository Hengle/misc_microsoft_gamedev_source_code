//-----------------------------------------------------------------------------
// File: tilerizer.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef TILERIZER_H
#define TILERIZER_H

#include "common/math/vector.h"

namespace gr
{
	class Tilerizer
	{
	public:
		class CellLimits
		{
		public:
			CellLimits()
			{
				clear();
			}
			
			void clear(void)
			{
				mLowestZ = +Math::fNearlyInfinite;
			}

			void sample(float z)
			{
				mLowestZ = Math::Min(mLowestZ, z);
			}
			
			void set(float z)
			{
				mLowestZ = z;
			}

			float lowestZ(void) const { return mLowestZ; }
		
		private:
			float mLowestZ;
		};

		Tilerizer(int planeWidth, int planeHeight, int cellWidth, int cellHeight) :
			mPlaneWidth(planeWidth),
			mPlaneHeight(planeHeight),
			mCellWidth(cellWidth),
			mCellHeight(cellHeight)
		{
			init();
		}

		void resize(int planeWidth, int planeHeight, int cellWidth, int cellHeight)
		{
			mPlaneWidth = planeWidth;
			mPlaneHeight = planeHeight;
			mCellWidth = cellWidth;
			mCellHeight = cellHeight;
			
			init();

			clearCells();
		}
		
		void rasterizeTri(const Vec3 pPoints[3])
		{
			Assert(pPoints);

			Vec2Interval bbox(Vec2Interval::eInitExpand);
			
			Vec2 cells[3];
			float minPossibleZ = Math::fNearlyInfinite;
			float maxPossibleZ = -Math::fNearlyInfinite;
			for (int i = 0; i < 3; i++)
			{
				bbox.expand(cells[i] = pointToCell(pPoints[i]));
				minPossibleZ = Math::Min(minPossibleZ, pPoints[i][2]);
				maxPossibleZ = Math::Max(maxPossibleZ, pPoints[i][2]);
			}

			const int left		= Math::FloatToIntTrunc(bbox.low()[0]);
			const int right		= Math::FloatToIntTrunc(bbox.high()[0]);
			const int top			= Math::FloatToIntTrunc(bbox.low()[1]);
			const int bottom	= Math::FloatToIntTrunc(bbox.high()[1]);
			const int width		= right - left + 1;
			const int height	= bottom - top + 1;

			mLeft		= Math::Min(mLeft, left);
			mRight	= Math::Max(mRight, right);
			mTop		= Math::Min(mTop, top);
			mBottom = Math::Max(mBottom, bottom);

			Assert(right >= left);
			Assert(bottom >= top);

			DebugRange(left,		mCellsX);
			DebugRange(right,		mCellsX);
			DebugRange(top,			mCellsY);
			DebugRange(bottom,	mCellsY);
						
			rasterizeEdge(cells[0], cells[1]);
			rasterizeEdge(cells[1], cells[2]);
			rasterizeEdge(cells[2], cells[0]);
      
			Plane plane;
			const bool badPlane = plane.setFromTriangle(
				Vec3(cells[0][0], cells[0][1], pPoints[0][2]),
				Vec3(cells[1][0], cells[1][1], pPoints[1][2]),
				Vec3(cells[2][0], cells[2][1], pPoints[2][2]));
			const float dZoverdX = badPlane ? 0.0f : plane.gradient(2, 0, 0);
			const float dZoverdY = badPlane ? 0.0f : plane.gradient(2, 1, 0);
			
			// left, top
			float curZ = pPoints[0][2] + 
				(left - cells[0][0]) * dZoverdX + 
				(top - cells[0][1]) * dZoverdY;
						
			int yIndex = cellToIndex(0, top);
			for (int y = top; y <= bottom; y++, yIndex += mCellsX, curZ += dZoverdY)
			{
				Assert(mSpans[y].mLow <= mSpans[y].mHigh);
        			
				float z = curZ + (mSpans[y].mLow - left) * dZoverdX;

				float minZ = Math::Min(z, z + dZoverdX);
				minZ = Math::Min(minZ, z + dZoverdX + dZoverdY);
				minZ = Math::Min(minZ, z + dZoverdY);
				
				for (int x = mSpans[y].mLow; x <= mSpans[y].mHigh; x++, minZ += dZoverdX)
				{
					const int index = yIndex + x;
					
					int prevOccupied = mCellOccupied[index];
					mCellOccupied[index] = true;

					mSetTiles += 1 - prevOccupied;
					
					mCellLimits[index].sample(Math::Clamp(minZ, minPossibleZ, maxPossibleZ));
				}

				mSpans[y].initExpand();
			}
		}

		bool isCellOccupied(int x, int y) const
		{
			return mCellOccupied[cellToIndex(x, y)];
		}

		const CellLimits& getCellLimits(int x, int y) const
		{
			return mCellLimits[cellToIndex(x, y)];
		}

		int planeWidth(void)	const { return mPlaneWidth; }
		int planeHeight(void) const { return mPlaneHeight; }
		int cellWidth(void)		const { return mCellWidth; }
		int cellHeight(void)	const { return mCellHeight; }
		int cellsX(void)			const { return mCellsX; }
		int cellsY(void)			const { return mCellsY; }
		int cellsTotal(void)	const { return mCellsTotal; }

		int getLeft(void)		const { return mLeft; }
		int getRight(void)	const { return mRight; }
		int getTop(void)		const { return mTop; }
		int getBottom(void) const { return mBottom; }

		int getNumSetTiles(void) const { return mSetTiles; }

		void clearBounds(void)
		{
			mLeft = INT_MAX;
			mRight = INT_MIN;
			mTop = INT_MAX;
			mBottom = INT_MIN;
		}

		void clearCells(void)
		{
			for (int i = 0; i < mCellsTotal; i++)
			{
				mCellLimits[i].clear();
				mCellOccupied[i] = false;
			}
			mSetTiles = 0;
		}
		
		// true if backface culled
		// should be called frontfaceCull?
		bool rasterizeScreenspaceTri(const Vec4* pVerts, const RenderViewport& renderViewport, bool backfaceCull = true)
		{
			Assert(pVerts);
			
			Vec3 v[3];
			
			const int width = renderViewport.viewport().Width - 1; //mRenderViewport.surfWidth() - 1;
			const int height = renderViewport.viewport().Height - 1;// mRenderViewport.surfHeight() - 1;

			for (int j = 0; j < 3; j++)
			{
				const Vec4& t = pVerts[j];
				
				const float oow = (t.w > 0.0f) ? (1.0f / t.w) : 0.0f;

				v[j][0] = t.x * oow;
				v[j][1] = t.y * oow;
				v[j][2] = oow;

				v[j][0] = Math::Clamp<float>(v[j][0], 0, width);
				v[j][1] = Math::Clamp<float>(v[j][1], 0, height);
			}

			Plane p;
			bool badTri = p.setFromTriangle(Vec3(v[0][0], v[0][1], 0), Vec3(v[1][0], v[1][1], 0),	Vec3(v[2][0], v[2][1], Math::fTinyEpsilon));
			
			bool backFaceCulled = false;
							
			if (!badTri)
			{
				if ((!backfaceCull) || (p.normal()[2] < 0.0f))
       		rasterizeTri(v);
				else
					backFaceCulled = true;
			}
			
			return backFaceCulled;
		}
		
		// verts is destroyed!
		void rasterizeViewspacePoly(Vec3Vec& verts, const RenderViewport& renderViewport, bool backfaceFull)
		{
			std::vector<Vec3> temp;
			temp.reserve(8);
			
			Vec3Vec* pSrc = &verts;
			Vec3Vec* pDst = &temp;
			for (int i = 0; i < Frustum::NUM_PLANES; i++)
			{
				renderViewport.camera().viewFrustum().plane(i).clipPoly(*pDst, *pSrc);
				std::swap(pSrc, pDst);

				if (pSrc->size() < 3)
					return;
			}
			
			const Vec3Vec& clippedVerts = *pSrc;
							
			Verify(clippedVerts.size() <= 64);
			Vec4 screenVerts[64];
			
			const Matrix44& viewToScreen = renderViewport.viewToScreenMatrix();
			
			for (int i = 0; i < clippedVerts.size(); i++)
				screenVerts[i] = Vec4(clippedVerts[i], 1.0f) * viewToScreen;
				
			Vec4 triVerts[3];
			triVerts[0] = screenVerts[0];
				
			for (int i = 0; i < clippedVerts.size() - 2; i++)
			{
				triVerts[1] = screenVerts[i + 1];
				triVerts[2] = screenVerts[i + 2];
				
				// skip the other tris if backface culled (all tris should be on the same plane)
				if (rasterizeScreenspaceTri(triVerts, renderViewport, backfaceFull))
					break;
			}
		}
		
		void unionOp(const Tilerizer& a, const Tilerizer& b)
		{
			clearBounds();
			clearCells();
			
			mLeft		= Math::Min(a.getLeft(), b.getLeft());
			mRight	= Math::Max(a.getRight(), b.getRight());
			mTop		= Math::Min(a.getTop(), b.getTop());
			mBottom = Math::Max(a.getBottom(), b.getBottom());
			
			for (int ty = mTop; ty <= mBottom; ty++)
			{
				for (int tx = mLeft; tx <= mRight; tx++)
				{
					if ((a.isCellOccupied(tx, ty)) || (b.isCellOccupied(tx, ty)))
					{
						const int index = cellToIndex(tx, ty);
						
						mCellOccupied[index] = true;

						mSetTiles++;
					
						mCellLimits[index].set(
							Math::Min(a.getCellLimits(tx, ty).lowestZ(), b.getCellLimits(tx, ty).lowestZ())
						);
					}
				}
			}
		}
		
			
		void andOp(const Tilerizer& a, const Tilerizer& b)
		{
			clearBounds();
			clearCells();
			
			mLeft		= Math::Max(a.getLeft(), b.getLeft());
			mRight	= Math::Min(a.getRight(), b.getRight());
			mTop		= Math::Max(a.getTop(), b.getTop());
			mBottom = Math::Min(a.getBottom(), b.getBottom());
			
			for (int ty = mTop; ty <= mBottom; ty++)
			{
				for (int tx = mLeft; tx <= mRight; tx++)
				{
					if ((a.isCellOccupied(tx, ty)) && (b.isCellOccupied(tx, ty)))
					{
						const int index = cellToIndex(tx, ty);
						
						mCellOccupied[index] = true;

						mSetTiles++;
					
						mCellLimits[index].set(
							Math::Max(a.getCellLimits(tx, ty).lowestZ(), b.getCellLimits(tx, ty).lowestZ())
						);
					}
				}
			}
		}
		
		void assignOp(const Tilerizer& b)
		{
			Assert(
				(mPlaneWidth  == b.mPlaneWidth) &&
				(mPlaneHeight == b.mPlaneHeight) &&
				(mCellWidth   == b.mCellWidth) &&
				(mCellHeight  == b.mCellHeight) &&
				(mCellsTotal == b.mCellsTotal)
			);
						
			mLeft		= b.getLeft();
			mRight	= b.getRight();
			mTop		= b.getTop();
			mBottom = b.getBottom();
			
			mSetTiles = b.mSetTiles;
			
			std::copy(b.mCellLimits.begin(), b.mCellLimits.end(), mCellLimits.begin());
			std::copy(b.mCellOccupied.begin(), b.mCellOccupied.end(), mCellOccupied.begin());
		}
		
	private:
	
		// undefined ops
		Tilerizer& operator= (const Tilerizer&);
		Tilerizer(const Tilerizer&);

		int mPlaneWidth;
		int mPlaneHeight;
		int mCellWidth;
		int mCellHeight;
		int mCellsX;
		int mCellsY;
		int mCellsTotal;

		int mLeft, mRight, mTop, mBottom;
		
		int mSetTiles;

		std::vector<CellLimits> mCellLimits;
		std::vector<bool> mCellOccupied;
		
		struct Span
		{
			int16 mLow;
			int16 mHigh;
			
			Span()
			{
				initExpand();
			}

			void initExpand(void)
			{
				mLow = INT16_MAX;
				mHigh = INT16_MIN;
			}

			void expand(int x)
			{
				if (x < mLow) mLow = x;
				if (x > mHigh) mHigh = x;
			}
		};

		std::vector<Span> mSpans;

		void init(void)
		{
			Verify(0 == (mPlaneWidth % mCellWidth));
			Verify(0 == (mPlaneHeight % mCellHeight));

			mCellsX = mPlaneWidth / mCellWidth;
			mCellsY = mPlaneHeight / mCellHeight;
			mCellsTotal = mCellsX * mCellsY;

			mCellLimits.resize(mCellsTotal);
			mCellOccupied.resize(mCellsTotal);
			
			mSpans.resize(mCellsY);

			clearBounds();

			mSetTiles = 0;
		}
				
		Vec2 pointToCell(const Vec3& a) const
		{
			const Vec2 ret(a[0] / float(mCellWidth), a[1] / float(mCellHeight));
      
			DebugRange<float>(ret[0], 0, mCellsX);
			DebugRange<float>(ret[1], 0, mCellsY);

			return ret;
		}

		int cellToIndex(int x, int y) const
		{
			return DebugRange(x, mCellsX) + DebugRange(y, mCellsY) * mCellsX;
		}

		static float rayAxisPlane(
			const Vec2& pos, const Vec2& dir, int axis, float d)
		{
			const float Eps = .00000125f;
			if (fabs(dir[axis]) < Eps)
				return Math::fNearlyInfinite;
			
			return -(pos[axis] - d) / dir[axis];
		}

		void rasterizeEdge(Vec2 s, Vec2 e)
		{
			const Vec2 dir((e - s).normalize());
			
			const int startX = Math::FloatToIntTrunc(s[0]);
			const int startY = Math::FloatToIntTrunc(s[1]);
			
			const int endX = Math::FloatToIntTrunc(e[0]);
			const int endY = Math::FloatToIntTrunc(e[1]);
			
			const int stepX = Math::Sign(dir[0]);
			const int stepY = Math::Sign(dir[1]);

      const int limitX = endX + stepX;
			const int limitY = endY + stepY;

			Vec2 p(s - Vec2(startX + .5f, startY + .5f));
      
			float maxX = rayAxisPlane(p, dir, 0, stepX * .5f);
			float maxY = rayAxisPlane(p, dir, 1, stepY * .5f);

			float deltaX = maxX - rayAxisPlane(p, dir, 0, -stepX * .5f);
			float deltaY = maxY - rayAxisPlane(p, dir, 1, -stepY * .5f);

			int x = startX;
			int y = startY;

			for ( ; ; )
			{
				mSpans[DebugRange(y, mCellsY)].expand(DebugRange(x, mCellsX));
        
				bool xStep, yStep;

				if (maxX == maxY)
				{
					xStep = true;
					yStep = true;
				}
				else if (maxX < maxY)
				{
					xStep = true;
					yStep = false;
				}
				else
				{
					xStep = false;
					yStep = true;
				}

				if (xStep)
				{
					x += stepX;
					if (x == limitX)
						break;
					maxX += deltaX;
        }

				if (yStep)
				{
					y += stepY;
					if (y == limitY)
						break;
					maxY += deltaY;
        }
			}
		}

  }; 

} // namespace gr

#endif // TILERIZER_H



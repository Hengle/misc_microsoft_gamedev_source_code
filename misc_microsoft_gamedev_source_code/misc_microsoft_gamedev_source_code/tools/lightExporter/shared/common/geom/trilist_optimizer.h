//-----------------------------------------------------------------------------
// trilist_optimizer.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef TRILIST_OPTIMIZER_H
#define TRILIST_OPTIMIZER_H

#include "vertex_cache.h"
#include "indexed_tri.h"

namespace gr
{
	// Quickly maps verts to tris.
	class VertexMap
	{
	public:
		typedef std::vector<TriIndex> TriIndexVec;
				
		VertexMap(int numVerts) : mTriIndexLists(numVerts)
		{
		}

		VertexMap(const IndexedTriVec& tris)
		{
			VertIndex maxVertIndex = 0;
			
			for (uint i = 0; i < tris.size(); i++)
				maxVertIndex = Math::Max(maxVertIndex, tris[i].maxVertIndex());
			
			Assert(maxVertIndex >= 0);

			mTriIndexLists.resize(maxVertIndex + 1);

			insertTris(tris);
		}

		void insertVert(TriIndex triIndex, VertIndex vertIndex)
		{
			TriIndexVec& trilist = mTriIndexLists[DebugRange(vertIndex, mTriIndexLists.size())];

			if (std::find(trilist.begin(), trilist.end(), vertIndex) == trilist.end())
				trilist.push_back(triIndex);
		}

		void insertTris(const IndexedTriVec& tris)
		{
      for (uint i = 0; i < tris.size(); i++)
			{
				insertVert(i, tris[i][0]);
				insertVert(i, tris[i][1]);
				insertVert(i, tris[i][2]);
			}
		}

		const TriIndexVec& trisUsingVert(VertIndex vertIndex) const
		{
			return mTriIndexLists[DebugRange(vertIndex, mTriIndexLists.size())];
		}

		int numVerts(void) const
		{
			return static_cast<int>(mTriIndexLists.size());
		}
		
	private:
		std::vector<TriIndexVec> mTriIndexLists;
	};

	class TriListOptimizer
	{
	public:
		TriListOptimizer(const IndexedTriVec& tris) : mTris(tris)
		{
			optimize();
		}

		const IntVec& triOrder(void) const
		{
			return mTriOrder;
		}

		int numTris(void) const 
		{ 
			return static_cast<int>(mTris.size()); 
		}

	private:
		const IndexedTriVec& mTris;
		IntVec mTriOrder;
   		
		void optimize(void)
		{
			VertexMap vertexMap(mTris);

			const int numVerts = vertexMap.numVerts();
						
			const int VertexCacheSize = 24;
			VertexCache vcache(VertexCacheSize);

			int numTrisLeft = numTris();
			int lowestFreeTriIndex = 0;

			std::vector<bool> triProcessed(numTris());

			while (numTrisLeft)
			{
				std::list<TriIndex> candidateTris;

				for (int vcacheIndex = 0; vcacheIndex < vcache.numEntries(); vcacheIndex++)
				{
					if (-1 != vcache.at(vcacheIndex))
					{
						const VertexMap::TriIndexVec& tris = vertexMap.trisUsingVert(vcache.at(vcacheIndex));
						candidateTris.insert(candidateTris.begin(), tris.begin(), tris.end());
					}
				}

				candidateTris.sort();
				candidateTris.unique();

				TriIndex bestTriIndex = -1;
				float lowestCost = Math::fNearlyInfinite;

				for (std::list<TriIndex>::const_iterator it = candidateTris.begin(); 
					it != candidateTris.end(); ++it)
				{
					const TriIndex candidateTriIndex = *it;
					
					if (triProcessed[DebugRange(candidateTriIndex, numTris())])
						continue;

					const float cost = vcache.cost(mTris[candidateTriIndex]);
					if (cost < lowestCost)
					{
						lowestCost = cost;
						bestTriIndex = candidateTriIndex;
						if (0.0f == lowestCost)
							break;
					}
				}

				if (-1 == bestTriIndex)
				{
					for ( ; lowestFreeTriIndex < numTris(); lowestFreeTriIndex++)
						if (!triProcessed[lowestFreeTriIndex])
							break;

					Assert(lowestFreeTriIndex != numTris());

					bestTriIndex = lowestFreeTriIndex;
					
					lowestFreeTriIndex++;
				}

				DebugRange(bestTriIndex, numTris());
				Assert(!triProcessed[bestTriIndex]);
				
				vcache.load(mTris[bestTriIndex]);
				mTriOrder.push_back(bestTriIndex);
				triProcessed[bestTriIndex] = true;
				numTrisLeft--;
			}
    }
  };

} // namespace gr

#endif // TRILIST_OPTIMIZER_H


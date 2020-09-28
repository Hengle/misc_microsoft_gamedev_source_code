//-----------------------------------------------------------------------------
// File: indexed_tri_mesh.h
// x86 optimized vector/matrix classes.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef INDEXED_TRI_MESH_H
#define INDEXED_TRI_MESH_H

#include "common/utils/unifier.h"
#include "univert.h"
#include "indexed_tri.h"
#include <algorithm>

namespace gr
{
	class IndexedTriMesh
	{
	public:
		typedef Unifier<Univert> UnivertUnifier;
		typedef std::vector<IntVec> IntVecVec;

		IndexedTriMesh(const UnivertAttributes& vertAttributes) : 
			mVertAttributes(vertAttributes)
		{
		}
		
		void clear(void)
		{
			mTriIndices.clear();
			mVertices.clear();
			mVertAttributes.clear();
			mVertAdj.clear();
		}

		void insert(const Unitri& tri)
		{
			mTriIndices.push_back(IndexedTri32());

			IndexedTri32& dstTri = mTriIndices.back();

			for (int vertIndex = 0; vertIndex < 3; vertIndex++)
				dstTri[vertIndex] = mVertices.insert(tri[vertIndex].select(mVertAttributes)).first;
		}

		// Creates list of triangles using each vertex.
		void createVertexAdjacency(void)
		{
			mVertAdj.resize(numVerts());		

			for (int triIndex = 0; triIndex < numTris(); triIndex++)
			{
				for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
				{
					const int vertIndex = tri(triIndex)[triVertIndex];

					IntVec& trilist = mVertAdj[DebugRange(vertIndex, numVerts())];
					
					if (std::find(trilist.begin(), trilist.end(), triIndex) == trilist.end())
						trilist.push_back(triIndex);
				}
			}
		}

		bool hasVertexAdjacency(void) const
		{
			return mVertAdj.size() == numVerts();
		}

		int numTris(void) const { return static_cast<int>(mTriIndices.size()); }
		const IndexedTri32& tri(int i) const { return mTriIndices[DebugRange(i, numTris())]; }

		int numVerts(void) const { return mVertices.size(); }
		const Univert& vertex(int i) const { return mVertices[i]; }
		const IntVec& vertexAdjacency(int i) const { return mVertAdj[DebugRange<int>(i, mVertAdj.size())]; }

		const IndexedTri32Vec&			tris(void) const { return mTriIndices; }
		const UnivertUnifier&				vertices(void) const { return mVertices; }
		const UnivertAttributes&		vertAttributes(void) const { return mVertAttributes; }
			    
	protected:
		IndexedTri32Vec mTriIndices;
		UnivertUnifier mVertices;
		UnivertAttributes mVertAttributes;
		IntVecVec mVertAdj;
	};
	
} // namespace gr

#endif // INDEXED_TRI_MESH_H

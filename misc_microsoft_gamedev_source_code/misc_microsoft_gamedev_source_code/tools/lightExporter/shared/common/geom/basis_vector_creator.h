//-----------------------------------------------------------------------------
// File: basis_vector_creator.h
// x86 optimized vector/matrix classes.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef BASIS_VECTOR_CREATOR_H
#define BASIS_VECTOR_CREATOR_H

#include "common/math/vector.h"
#include "indexed_tri.h"
#include "univert.h"

namespace gr
{
	struct BasisVectorCreator
	{
		// Directly adds basis vectors to an unindexed mesh.,
		static void add(UnitriVec& tris, const int uvChannel, const int basisIndex);
		
		// Container of triangles that influence each output vertices basis vectors.
		class TriVertBasisContributors
		{
		public:
			TriVertBasisContributors()
			{
			}
			
			const IntVec& axisTris(int basisAxis, int triVert) const	{ return mAxisTris[DebugRange(basisAxis, 2)][DebugRange(triVert, 3)]; }
						IntVec& axisTris(int basisAxis, int triVert)				{ return mAxisTris[DebugRange(basisAxis, 2)][DebugRange(triVert, 3)]; }
						
		protected:
			IntVec mAxisTris[2][3];
		};
	
		typedef std::vector<TriVertBasisContributors> TriVertBasisContributorsVec;
		
		// Create the tris that contribute to each tri's vertex.
		static void createTriVertBasisContributors(
			TriVertBasisContributorsVec& triVertBasisContributorsVec,
			const IndexedTriVec& tris, 
			const UnivertVec& verts, 
			const int uvChannel);

		// Per-triangle tangent basis vectors
		struct TriBasis
		{
			Vec3 v[3];
			float s[2];
			bool valid;
			
			void set(const Vec3* pVerts,	const Vec2* pTexcoords);
		};
		typedef std::vector<TriBasis> TriBasisVec;
	
		static void createTriBasisVec(TriBasisVec& triBasisVec, const UnitriVec& tris, const int uvChannel);
		static void createTriBasisVec(TriBasisVec& triBasisVec, const IndexedTriVec& tris, const UnivertVec& verts, const int uvChannel);
		
		static void generateBasis(
			Vec<4>& tangent,
			Vec<4>& binormal,
			const Vec3& vertNorm,
			const IntVec& tangentTris,
			const IntVec& binormalTris,
			const TriBasisVec& triBasisVec);
		
		static void orthonormalize(Vec3* pV, const Vec3& vertNorm);
	};

} // namespace gr

#endif // BASIS_VECTOR_CREATOR_H

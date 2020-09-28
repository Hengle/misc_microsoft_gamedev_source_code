//-----------------------------------------------------------------------------
// File: basis_vector_creator.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "basis_vector_creator.h"

#include "indexed_tri_mesh.h"
#include "common/math/texture_space_basis.h"
#include "common/math/plane.h"

namespace gr
{
	const float gSTAngleThreshold = .25f;
	
	void BasisVectorCreator::TriBasis::set(const Vec3* pVerts,	const Vec2* pTexcoords)
	{
		Vec3 basisVecs[2];
		Utils::ClearObj(basisVecs);

		const Vec3 norm((pVerts[2] - pVerts[0]) % (pVerts[0] - pVerts[1]));
		
		const float Eps = .0000001f;
		if (norm.norm() > Eps)
			MakeTextureSpaceBasis(basisVecs, pTexcoords, pVerts);

		valid = true;
		if ((basisVecs[0].norm() < Eps) || (basisVecs[1].norm() < Eps))
		{
			//basisVecs[0] = Vec3(1,0,0);
			//basisVecs[1] = Vec3(0,1,0);
			
			const ParametricPlane pp(ParametricPlane::makePlanarProjection(Plane(norm, pVerts[0])));
			basisVecs[0] = pp.u;
			basisVecs[1] = pp.v;
				
			valid = false;
		}
		
    s[0] = basisVecs[0].len();
		s[1] = basisVecs[1].len();
		v[0] = basisVecs[0].normalized();
		v[1] = basisVecs[1].normalized();
		v[2] = norm.normalized();
	}

	void BasisVectorCreator::createTriBasisVec(
		TriBasisVec& triBasisVec, 
		const UnitriVec& tris, 
		const int uvChannel)
	{
		triBasisVec.resize(tris.size());

		for (int triIndex = 0; triIndex < tris.size(); triIndex++)
		{
			Vec3 v[3];
			Vec2 t[3];
							
			for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
			{
				v[triVertIndex] = tris[triIndex][triVertIndex].p;
				t[triVertIndex] = tris[triIndex][triVertIndex].texcoord(uvChannel);
			}
			
			triBasisVec[triIndex].set(v, t);
		}
	}
	
	void BasisVectorCreator::createTriBasisVec(
		TriBasisVec& triBasisVec, 
		const IndexedTriVec& tris, 
		const UnivertVec& verts,
		const int uvChannel)
	{
		triBasisVec.resize(tris.size());

		for (int triIndex = 0; triIndex < tris.size(); triIndex++)
		{
			Vec3 v[3];
			Vec2 t[3];
							
			for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
			{
				v[triVertIndex] = verts[tris[triIndex][triVertIndex]].p;
				t[triVertIndex] = verts[tris[triIndex][triVertIndex]].texcoord(uvChannel);
			}
			
			triBasisVec[triIndex].set(v, t);
		}
	}
	
	void BasisVectorCreator::orthonormalize(Vec3* pV, const Vec3& vertNorm)
	{
		// Experimental algorithm!
		// An attempt to minimize/equalize tangent/binormal distortion due to orthonormalization.
		const int NumIterations = 8;
		for (int j = 0; j < NumIterations; j++)
		{
			for (int k = 0; k < 2; k++)
			{
				Vec3& x = pV[k];
				Vec3& y = pV[k ^ 1];

				Vec3 tx(Vec3::removeCompUnit(x, vertNorm).normalized());
				Vec3 ty = tx % vertNorm;
				if ((ty * y) < 0.0f)
					ty *= -1.0f;

				y = (y + ty) * .5f;
			}
		}

		Vec3 x(Vec3::removeCompUnit(pV[0], vertNorm).normalized());
		Vec3 y(x % vertNorm);
		if ((y * pV[1]) < 0.0f)
			y *= -1.0f;
			
		pV[0] = x;
		pV[1] = y;
	}

	void BasisVectorCreator::add(UnitriVec& tris, const int uvChannel, const int basisIndex)
	{
		const int numTris = static_cast<int>(tris.size());

		UnivertAttributes pnAttributes;
		pnAttributes.pos = true;
		pnAttributes.norm = true;

		IndexedTriMesh indexedTriMesh(pnAttributes);
			      	
		for (int triIndex = 0; triIndex < numTris; triIndex++)
			indexedTriMesh.insert(tris[triIndex]);

		indexedTriMesh.createVertexAdjacency();

    TriBasisVec triBasisVec;
		createTriBasisVec(triBasisVec, tris, uvChannel);

		for (int triIndex = 0; triIndex < numTris; triIndex++)
		{
			for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
			{
				const int pnVertIndex = indexedTriMesh.tri(triIndex)[triVertIndex];
				const IntVec& triAdj = indexedTriMesh.vertexAdjacency(pnVertIndex);
          				
				Vec3 v[2];
				float s[2];

				const Vec3 vertPos(tris[triIndex][triVertIndex].p);
				const Vec3 vertNorm(tris[triIndex][triVertIndex].n.normalized());
				const ParametricPlane pp(ParametricPlane::makePlanarProjection(Plane(vertNorm, vertPos)));
				
				v[0] = pp.u;
				v[1] = pp.v;
				s[0] = 1.0f;
				s[1] = 1.0f;
                  
				for (int axisIndex = 0; axisIndex < 2; axisIndex++)
				{
					Vec3 vecSum(0);
					float scaleSum(0);
					int n = 0;

					bool haveCenterVec = false;
					Vec3 centerVec;
					if (triBasisVec[triIndex].valid)
					{
						centerVec = triBasisVec[triIndex].v[axisIndex];
						haveCenterVec = true;
					}
                      
					for (int i = 0; i < triAdj.size(); i++)
					{
						const int otherTriIndex = triAdj[i];
						const TriBasis& basis = triBasisVec[otherTriIndex];
						if (!basis.valid)
							continue;

						if (haveCenterVec)
						{
							if ((basis.v[axisIndex] * centerVec) < gSTAngleThreshold)
								continue;
						}

						vecSum += basis.v[axisIndex];
						scaleSum += basis.s[axisIndex];
            n++;              							
						
						if ((n == 1) && (!haveCenterVec))
						{
							centerVec = vecSum;
							haveCenterVec = true;
						}
					}

          const float SumEps = .00001f;
					if ((n) && (vecSum.norm() >= SumEps))
					{
						vecSum.normalize();
						scaleSum /= n;
            
						v[axisIndex] = vecSum;
						s[axisIndex] = scaleSum;
					}
				}
				
				orthonormalize(v, vertNorm);
				
				tris[triIndex][triVertIndex].s[basisIndex] = Vec<4>(v[0][0], v[0][1], v[0][2], s[0]);
				tris[triIndex][triVertIndex].t[basisIndex] = Vec<4>(v[1][0], v[1][1], v[1][2], s[1]);
			}
		}
	}
				
	void BasisVectorCreator::createTriVertBasisContributors(
		TriVertBasisContributorsVec& triVertBasisContributorsVec,
		const IndexedTriVec& tris, 
		const UnivertVec& verts, 
		const int uvChannel)
	{
		const int numTris = static_cast<int>(tris.size());
		
		triVertBasisContributorsVec.resize(numTris);

		UnivertAttributes pnAttributes;
		pnAttributes.pos = true;
		pnAttributes.norm = true;

		IndexedTriMesh indexedTriMesh(pnAttributes);
			      	
		for (int triIndex = 0; triIndex < numTris; triIndex++)
		{
			Unitri tri;
			
			for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
				tri[triVertIndex] = verts[tris[triIndex][triVertIndex]];
				
			indexedTriMesh.insert(tri);
		}

		indexedTriMesh.createVertexAdjacency();

    TriBasisVec triBasisVec;
		createTriBasisVec(triBasisVec, tris, verts, uvChannel);
		
		for (int triIndex = 0; triIndex < numTris; triIndex++)
		{
			for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
			{
				const int pnVertIndex = indexedTriMesh.tri(triIndex)[triVertIndex];
				const IntVec& triAdj = indexedTriMesh.vertexAdjacency(pnVertIndex);
							
				const Vec3 vertPos(verts[tris[triIndex][triVertIndex]].p);
				const Vec3 vertNorm(verts[tris[triIndex][triVertIndex]].n.normalized());
				const ParametricPlane pp(ParametricPlane::makePlanarProjection(Plane(vertNorm, vertPos)));
								                  
				IntVec contributingTris;
				contributingTris.reserve(4);
				
				for (int axisIndex = 0; axisIndex < 2; axisIndex++)
				{
					bool haveCenterVec = false;
					Vec3 centerVec;
					if (triBasisVec[triIndex].valid)
					{
						centerVec = triBasisVec[triIndex].v[axisIndex];
						haveCenterVec = true;
					}
                      
					for (int i = 0; i < triAdj.size(); i++)
					{
						const int otherTriIndex = triAdj[i];
						const TriBasis& basis = triBasisVec[otherTriIndex];
						if (!basis.valid)
							continue;

						if (haveCenterVec)
						{
							if ((basis.v[axisIndex] * centerVec) < gSTAngleThreshold)
								continue;
						}

						contributingTris.push_back(otherTriIndex);
						
						if ((contributingTris.size() == 1) && (!haveCenterVec))
						{
							centerVec = basis.v[axisIndex];
							haveCenterVec = true;
						}
					}
					
					if (contributingTris.empty())
						contributingTris.push_back(triIndex);
					else
						std::sort(contributingTris.begin(), contributingTris.end());
					
					triVertBasisContributorsVec[triIndex].axisTris(axisIndex, triVertIndex) = contributingTris;
				}
			}
		}		
	}
	
	void BasisVectorCreator::generateBasis(
		Vec<4>& tangent,
		Vec<4>& binormal,
		const Vec3& vertNorm,
		const IntVec& tangentTris,
		const IntVec& binormalTris,
		const TriBasisVec& triBasisVec)
	{
		Vec3 v[2];
		float s[2];
		
		for (int axisIndex = 0; axisIndex < 2; axisIndex++)
		{
			const IntVec& contributingTris = axisIndex ? binormalTris : tangentTris;
			Assert(!contributingTris.empty());
			
			if ((1 == contributingTris.size()) && (!triBasisVec[contributingTris[0]].valid))
			{
				const TriBasis& basis = triBasisVec[contributingTris[0]];
				v[axisIndex] = basis.v[axisIndex];
				s[axisIndex] = 1.0f;
				continue;
			}

			Vec3 vecSum(0);
			float scaleSum(0);

			int numValid = 0;			
			for (int i = 0; i < contributingTris.size(); i++)
			{
				const int otherTriIndex = contributingTris[i];
				const TriBasis& basis = triBasisVec[DebugRange(otherTriIndex, triBasisVec.size())];
								
				//Assert(basis.valid);
				if (basis.valid)					
				{
					vecSum		+= basis.v[axisIndex];
					scaleSum	+= basis.s[axisIndex];
					numValid++;
				}
			}

			const float SumEps = .00001f;
			if (vecSum.norm() >= SumEps)
			{
				vecSum.normalize();
				scaleSum /= numValid;
	      
				v[axisIndex] = vecSum;
				s[axisIndex] = scaleSum;
			}
			else
			{
				// shouldn't happen unless model deforms all to hell
				v[axisIndex] = axisIndex ? Vec3(0,1,0) : Vec3(1,0,0);
				s[axisIndex] = 1.0f;
			}
		}
		
		orthonormalize(v, vertNorm);
							
		// Stores recip for basis scales, not actual scale!
		tangent = Vec<4>(v[0], (s[0] != 0.0f) ? (1.0f / s[0]) : 0.0f);
		binormal = Vec<4>(v[1], (s[1] != 0.0f) ? (1.0f / s[1]) : 0.0f);
	}

} // namespace gr




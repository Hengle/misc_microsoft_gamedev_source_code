//-----------------------------------------------------------------------------
// File: basis_vector_creator.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "xgeom.h"

#include "math\plane.h"
// local
#include "basisVectorCreator.h"
#include "textureSpaceBasis.h"
#include "indexedTriMesh.h"

const float gSTAngleThreshold = .25f;

void BasisVectorCreator::TriBasis::set(const BVec3* pVerts,  const BVec2* pTexcoords)
{
   BVec3 basisVecs[2];
   Utils::ClearObj(basisVecs);

   const BVec3 norm((pVerts[2] - pVerts[0]) % (pVerts[0] - pVerts[1]));
   
   const float Eps = .0000001f;
   if (norm.norm() > Eps)
      MakeTextureSpaceBasis(basisVecs, pTexcoords, pVerts);

   valid = true;
   if ((basisVecs[0].norm() < Eps) || (basisVecs[1].norm() < Eps))
   {
      //basisVecs[0] = BVec3(1,0,0);
      //basisVecs[1] = BVec3(0,1,0);
      
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

   for (uint triIndex = 0; triIndex < tris.size(); triIndex++)
   {
      BVec3 v[3];
      BVec2 t[3];
                  
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

   for (uint triIndex = 0; triIndex < tris.size(); triIndex++)
   {
      BVec3 v[3];
      BVec2 t[3];
                  
      for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
      {
         v[triVertIndex] = verts[tris[triIndex][triVertIndex]].p;
         t[triVertIndex] = verts[tris[triIndex][triVertIndex]].texcoord(uvChannel);
      }
      
      triBasisVec[triIndex].set(v, t);
   }
}

void BasisVectorCreator::orthonormalize(BVec3* pV, const BVec3& vertNorm)
{
   // Experimental algorithm!
   // An attempt to minimize/equalize tangent/binormal distortion due to orthonormalization.
   const int NumIterations = 8;
   for (int j = 0; j < NumIterations; j++)
   {
      for (int k = 0; k < 2; k++)
      {
//-- FIXING PREFIX BUG ID 7657
         const BVec3& x = pV[k];
//--
         BVec3& y = pV[k ^ 1];

         BVec3 tx(BVec3::removeCompUnit(x, vertNorm).normalized());
         BVec3 ty = tx % vertNorm;
         if ((ty * y) < 0.0f)
            ty *= -1.0f;

         y = (y + ty) * .5f;
      }
   }

   BVec3 x(BVec3::removeCompUnit(pV[0], vertNorm).normalized());
   BVec3 y(x % vertNorm);
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

   IndexedTriMeshBuilder indexedTriMeshBuilder(pnAttributes);
               
   for (int triIndex = 0; triIndex < numTris; triIndex++)
      indexedTriMeshBuilder.insert(tris[triIndex]);

   indexedTriMeshBuilder.createVertexAdjacency();

   TriBasisVec triBasisVec;
   createTriBasisVec(triBasisVec, tris, uvChannel);

   for (int triIndex = 0; triIndex < numTris; triIndex++)
   {
      for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
      {
         const int pnVertIndex = indexedTriMeshBuilder.tri(triIndex)[triVertIndex];
         const IntVec& triAdj = indexedTriMeshBuilder.vertAdjacency(pnVertIndex);
                  
         BVec3 v[2];
         float s[2];

         const BVec3 vertPos(tris[triIndex][triVertIndex].p);
         const BVec3 vertNorm(tris[triIndex][triVertIndex].n.normalized());
         const ParametricPlane pp(ParametricPlane::makePlanarProjection(Plane(vertNorm, vertPos)));
         
         v[0] = pp.u;
         v[1] = pp.v;
         s[0] = 1.0f;
         s[1] = 1.0f;
               
         for (int axisIndex = 0; axisIndex < 2; axisIndex++)
         {
            BVec3 vecSum(0.0f);
            float scaleSum(0);
            int n = 0;

            bool haveCenterVec = false;
            BVec3 centerVec;
            if (triBasisVec[triIndex].valid)
            {
               centerVec = triBasisVec[triIndex].v[axisIndex];
               haveCenterVec = true;
            }
                   
            for (uint i = 0; i < triAdj.size(); i++)
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
         
         tris[triIndex][triVertIndex].s[basisIndex] = BVecN<4>(v[0][0], v[0][1], v[0][2], s[0]);
         tris[triIndex][triVertIndex].t[basisIndex] = BVecN<4>(v[1][0], v[1][1], v[1][2], s[1]);
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

   IndexedTriMeshBuilder indexedTriMeshBuilder(pnAttributes);
               
   for (int triIndex = 0; triIndex < numTris; triIndex++)
   {
      Unitri tri;
      
      for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
         tri[triVertIndex] = verts[tris[triIndex][triVertIndex]];
         
      indexedTriMeshBuilder.insert(tri);
   }

   indexedTriMeshBuilder.createVertexAdjacency();

   TriBasisVec triBasisVec;
   createTriBasisVec(triBasisVec, tris, verts, uvChannel);
   
   IntVec contributingTris;
   contributingTris.reserve(512);

   for (int triIndex = 0; triIndex < numTris; triIndex++)
   {
      for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
      {
         const int pnVertIndex = indexedTriMeshBuilder.tri(triIndex)[triVertIndex];
         const IntVec& triAdj = indexedTriMeshBuilder.vertAdjacency(pnVertIndex);
                  
         const BVec3 vertPos(verts[tris[triIndex][triVertIndex]].p);
         const BVec3 vertNorm(verts[tris[triIndex][triVertIndex]].n.normalized());
         const ParametricPlane pp(ParametricPlane::makePlanarProjection(Plane(vertNorm, vertPos)));
                                       
         contributingTris.resize(0); //erase(contributingTris.begin(), contributingTris.end());
         
         for (int axisIndex = 0; axisIndex < 2; axisIndex++)
         {
            bool haveCenterVec = false;
            BVec3 centerVec;
            if (triBasisVec[triIndex].valid)
            {
               centerVec = triBasisVec[triIndex].v[axisIndex];
               haveCenterVec = true;
            }
                   
            for (uint i = 0; i < triAdj.size(); i++)
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
   BVecN<4>& tangent,
   BVecN<4>& binormal,
   const BVec3& vertNorm,
   const IntVec& tangentTris,
   const IntVec& binormalTris,
   const TriBasisVec& triBasisVec)
{
   BVec3 v[2];
   float s[2];
   
   for (int axisIndex = 0; axisIndex < 2; axisIndex++)
   {
      const IntVec& contributingTris = axisIndex ? binormalTris : tangentTris;
      BASSERT(!contributingTris.empty());
      
      if ((1 == contributingTris.size()) && (!triBasisVec[contributingTris[0]].valid))
      {
         const TriBasis& basis = triBasisVec[contributingTris[0]];
         v[axisIndex] = basis.v[axisIndex];
         s[axisIndex] = 1.0f;
         continue;
      }

      BVec3 vecSum(0.0f);
      float scaleSum(0);

      int numValid = 0;       
      for (uint i = 0; i < contributingTris.size(); i++)
      {
         const int otherTriIndex = contributingTris[i];
         const TriBasis& basis = triBasisVec[debugRangeCheck(otherTriIndex, triBasisVec.size())];
                     
         //BASSERT(basis.valid);
         if (basis.valid)              
         {
            vecSum      += basis.v[axisIndex];
            scaleSum += basis.s[axisIndex];
            numValid++;
         }
      }

      const float SumEps = .00001f;
      if (vecSum.norm() >= SumEps)
      {
         vecSum.normalize();
         scaleSum /= max(numValid, 1);
      
         v[axisIndex] = vecSum;
         s[axisIndex] = scaleSum;
      }
      else
      {
         // shouldn't happen unless model deforms all to hell
         v[axisIndex] = axisIndex ? BVec3(0,1,0) : BVec3(1,0,0);
         s[axisIndex] = 1.0f;
      }
   }
   
   orthonormalize(v, vertNorm);
                  
   // Stores recip for basis scales, not actual scale!
   tangent = BVecN<4>(v[0], (s[0] != 0.0f) ? (1.0f / s[0]) : 0.0f);
   binormal = BVecN<4>(v[1], (s[1] != 0.0f) ? (1.0f / s[1]) : 0.0f);
}


//-----------------------------------------------------------------------------
// File: basisVectorCreator.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

// xcore
#include "math/vector.h"

// local
#include "indexedTri.h"
#include "univert.h"

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
      
      const IntVec& axisTris(int basisAxis, int triVert) const { return mAxisTris[debugRangeCheck(basisAxis, 2)][debugRangeCheck(triVert, 3)]; }
            IntVec& axisTris(int basisAxis, int triVert)          { return mAxisTris[debugRangeCheck(basisAxis, 2)][debugRangeCheck(triVert, 3)]; }
               
   protected:
      IntVec mAxisTris[2][3];
   };

   typedef BDynamicArray<TriVertBasisContributors> TriVertBasisContributorsVec;
   
   // Create the tris that contribute to each tri's vertex.
   static void createTriVertBasisContributors(
      TriVertBasisContributorsVec& triVertBasisContributorsVec,
      const IndexedTriVec& tris, 
      const UnivertVec& verts, 
      const int uvChannel);

   // Per-triangle tangent basis vectors
   struct TriBasis
   {
      BVec3 v[3];
      float s[2];
      bool valid;
      
      void set(const BVec3* pVerts,  const BVec2* pTexcoords);
   };
   typedef BDynamicArray<TriBasis> TriBasisVec;

   static void createTriBasisVec(TriBasisVec& triBasisVec, const UnitriVec& tris, const int uvChannel);
   static void createTriBasisVec(TriBasisVec& triBasisVec, const IndexedTriVec& tris, const UnivertVec& verts, const int uvChannel);
   
   static void generateBasis(
      BVecN<4>& tangent,
      BVecN<4>& binormal,
      const BVec3& vertNorm,
      const IntVec& tangentTris,
      const IntVec& binormalTris,
      const TriBasisVec& triBasisVec);
   
   static void orthonormalize(BVec3* pV, const BVec3& vertNorm);
}; // struct BasisVectorCreator


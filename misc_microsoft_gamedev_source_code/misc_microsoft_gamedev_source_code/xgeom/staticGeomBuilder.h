// File: staticGeomBuilder.h
#pragma once

#include "containers\unifier.h"

#include "aabbTree.h"
#include "aabbTreeBuilder.h"
#include "tri.h"
#include "unigeom.h"

class BStaticGeomBuilder
{
public:
   BStaticGeomBuilder(bool fast, const Unigeom::Geom& mSrcGeom);
   
   bool getSucceeded(void) const { return mSucceeded; }
   const BString& getError(void) const { return mError; }
   
   const Unigeom::Geom& getChunkedGeom(void) const { return mChunkedGeom; }
   const BAABBTree& getAABBTree(void) const { return mpAABBTreeBuilder->tree(); }
   
   int getRigidBoneIndex(void) const { return mRigidBoneIndex; }
     
private:
   bool mFast;
   const Unigeom::Geom& mSrcGeom;
               
   typedef BIndexTri<Univert> BUnivertTri;
   
   typedef BDynamicArray<BUnivertTri> BUnivertTriVec;
   BUnivertTriVec mMeshTris;
   IntVec mMeshTrisOrigTriIndex;
        
   typedef BDynamicArray<BUnivertTri> BUnivertTriVec;
   typedef BAABBTreeBuilder<BUnivertTriVec> BAABBTreeBuilderBUnivertTriVec;
   std::auto_ptr<BAABBTreeBuilderBUnivertTriVec> mpAABBTreeBuilder;
   
   Unigeom::Geom mChunkedGeom;

   AABB mBounds;   
   float mMaxTriArea, mMaxTriDim;
   float mAveTriArea, mAveTriDim;
   
   int mRigidBoneIndex;
      
   BString mError;
   bool mSucceeded;
   
   void setError(const char* pMsg, ...);      
   void createTempMeshTri(int origTriIndex, const BUnivertTri& tri);
   bool createTempMesh(void);
   void createTree(void);
   void fixUVCoords(BUnivertTriVec& tris);
   void createLeafNodeGeom(const BAABBTree::BNode& node, int leafNodeIndex, Unigeom::BUnpackedAccessoryType& accessory);
   void createChunkedMesh(void);
   void createFinalTree(void);
   bool build(void);
};

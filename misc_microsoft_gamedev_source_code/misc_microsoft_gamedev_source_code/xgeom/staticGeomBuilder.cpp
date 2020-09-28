// File: staticGeomBuilder.cpp
#include "xgeom.h"
#include "staticGeomBuilder.h"

BStaticGeomBuilder::BStaticGeomBuilder(bool fast, const Unigeom::Geom& mSrcGeom) :
   mFast(fast),
   mSrcGeom(mSrcGeom),
   mMaxTriArea(0), mAveTriArea(0),
   mMaxTriDim(0), mAveTriDim(0),
   mSucceeded(false),
   mRigidBoneIndex(-1)
{
   mBounds.clear();
   
   if (mSrcGeom.numTris() >= 1)
      build();
   else
      setError("Source geometry is empty");
}

void BStaticGeomBuilder::setError(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   mError.formatArgs(pMsg, args);
   va_end(args);
   
   mSucceeded = false;
}

void BStaticGeomBuilder::createTempMeshTri(int origTriIndex, const BUnivertTri& tri)
{
   const AABB triBounds(tri.bounds());
   const float maxTriDimension = triBounds.dimension(triBounds.majorDimension());
   const float triArea = tri.area();

   mMaxTriArea = Math::Max(mMaxTriArea, triArea);
   mAveTriArea += triArea;
   mMaxTriDim = Math::Max(mMaxTriDim, maxTriDimension);
   mAveTriDim += maxTriDimension;

   mMeshTris.push_back(tri);
   mMeshTrisOrigTriIndex.push_back(origTriIndex);
}

bool BStaticGeomBuilder::createTempMesh(void)
{
   mRigidBoneIndex = -1;
   
   const int numTris = mSrcGeom.numTris();

   mBounds.initExpand();
   
   int rigidBoneIndex = -1;
   
   for (int triIndex = 0; triIndex < numTris; triIndex++)
   {
      BUnivertTri tri(mSrcGeom.tri(triIndex).materialIndex());

      for (int triVertIndex = 0; triVertIndex < NumTriVerts; triVertIndex++)
      {
         Univert& vert = tri[triVertIndex];
        
         vert = mSrcGeom.vert(mSrcGeom.tri(triIndex)[triVertIndex]);
         
         mBounds.expand(vert);
         
         if (vert.boneIndex(0) != Univert::DefaultBoneIndex)
         {
            if (rigidBoneIndex != -1)
            {
               if (vert.boneIndex(0) != rigidBoneIndex)
               {
                  setError("Large models can only be bound to a single bone!\n");
                  return false;
               }
            }
            else
               rigidBoneIndex = vert.boneIndex(0);
         }
         
         for (uint i = 1; i < Univert::MaxInfluences; i++)
         {
            if (vert.boneIndex(i) != Univert::DefaultBoneIndex)
            {
               setError("Vertex blending is not supported on large models!");
               return false;
            }
         }
         
         //tri[triVertIndex].clearBoneIndicesAndWeights();
      }

      createTempMeshTri(triIndex, tri);
   }
   
   mRigidBoneIndex = rigidBoneIndex;

   gConsoleOutput.printf("Rigid Bone Index: %i, Name: %s\n", rigidBoneIndex, (rigidBoneIndex != -1) ? mSrcGeom.bone(rigidBoneIndex).name().getPtr() : "(none)");
   gConsoleOutput.printf("AABB: (%f, %f, %f) - (%f, %f, %f)\n", mBounds[0][0], mBounds[0][1], mBounds[0][2], mBounds[1][0], mBounds[1][1], mBounds[1][2]);
   gConsoleOutput.printf("Dimensions: (%f, %f, %f)\nMajor dimension: %u, Minor Dimension: %u\n", mBounds.dimension(0), mBounds.dimension(1), mBounds.dimension(2), mBounds.majorDimension(), mBounds.minorDimension());
   gConsoleOutput.printf("Max tri area: %f\n", mMaxTriArea);
   gConsoleOutput.printf("Ave tri area: %f\n", mAveTriArea / float(numTris));
   gConsoleOutput.printf("Max tri dimension: %f\n", mMaxTriDim);
   gConsoleOutput.printf("Ave tri dimension: %f\n", mAveTriDim / float(numTris));
   
   return true;
}

void BStaticGeomBuilder::createTree(void)
{
   BAABBTreeBuilderBUnivertTriVec::BParams params;

#if 0   
   params.maxNodeDim = 20.0f;        // nodes with dimensions larger than this will be split     
   params.maxNodeObjects = 2000;      // if node exceeds this # of objects, it will always be split even if it's small
   params.minNodeObjects = 50;      // min # of objects for a node to be splittable
   params.smallObjIndexGroupThresh = 50; // tris in index groups smaller than this will be forced to a separate left tree
   params.optimizeForRendering = true;
   params.maxNodeExpansionRatio = 1.3f;
   params.maxMergeNodeObjects = 5000;
#endif   

   params.maxNodeDim = 35.0f;        // nodes with dimensions larger than this will be split     
   params.maxNodeObjects = 5000;      // if node exceeds this # of objects, it will always be split even if it's small
   params.minNodeObjects = 80;      // min # of objects for a node to be splittable
   params.smallObjIndexGroupThresh = 50; // tris in index groups smaller than this will be forced to a separate left tree
   params.optimizeForRendering = true;
   params.maxNodeExpansionRatio = 10.0f; // 1.3f
   params.maxMergeNodeObjects = 10000;
   params.maxExpandedNodeDim = 50.0f;
      
   mpAABBTreeBuilder = std::auto_ptr<BAABBTreeBuilderBUnivertTriVec>(
      new BAABBTreeBuilderBUnivertTriVec(mMeshTris, mSrcGeom.numMaterials(), mFast, params)
      );
}

void BStaticGeomBuilder::fixUVCoords(BUnivertTriVec& tris)
{
   const float MaxUVRange = 8.0f;

   for (int uvChannelIndex = 0; uvChannelIndex < Univert::MaxUVCoords; uvChannelIndex++)
   {
      Rect uvBounds(Rect::eInitExpand);

      float maxTriU = 0;
      float maxTriV = 0;

      for (uint triIter = 0; triIter < tris.size(); triIter++)
      {
         BUnivertTri& tri = tris[triIter];

         Rect triUVBounds(Rect::eInitExpand);
         for (int triVertIter = 0; triVertIter < NumTriVerts; triVertIter++)
            triUVBounds.expand(tri[triVertIter].texcoord(uvChannelIndex));   

         uvBounds.expand(triUVBounds);

         maxTriU = Math::Max(maxTriU, triUVBounds.dimension(0));
         maxTriV = Math::Max(maxTriV, triUVBounds.dimension(1));

         float uOfs = triUVBounds[0][0] / MaxUVRange;
         float vOfs = triUVBounds[0][1] / MaxUVRange;
         if (uOfs < 0.0f) uOfs = ceil(uOfs); else uOfs = floor(uOfs);
         if (vOfs < 0.0f) vOfs = ceil(vOfs); else vOfs = floor(vOfs);

         uOfs *= MaxUVRange;
         vOfs *= MaxUVRange;

         if ((uOfs) || (vOfs))
         {
            for (int triVertIter = 0; triVertIter < NumTriVerts; triVertIter++)
            {
               tri[triVertIter].texcoord(uvChannelIndex)[0] -= uOfs;
               tri[triVertIter].texcoord(uvChannelIndex)[1] -= vOfs;
            }
         }
      }

      float maxU = Math::Max(fabs(uvBounds[0][0]), fabs(uvBounds[1][0]));
      float maxV = Math::Max(fabs(uvBounds[0][1]), fabs(uvBounds[1][1]));

      trace("MaxTriUDim: %f, MaxTriVDim: %f, MaxUMag: %f MaxVMag: %f", maxTriU, maxTriV, maxU, maxV);
   }
}

void BStaticGeomBuilder::createLeafNodeGeom(const BAABBTree::BNode& node, int leafNodeIndex, Unigeom::BUnpackedAccessoryType& accessory)
{
   BUnivertTriVec leafTris;
   leafTris.reserve(node.numObjects());

   for (int leafTriIter = 0; leafTriIter < node.numObjects(); leafTriIter++)
   {
      const int meshTriIndex = debugRangeCheck(node.objIndex(leafTriIter), mMeshTris.size());

      BUnivertTri leafTri(mMeshTris[meshTriIndex]);

      leafTris.push_back(leafTri);
   }

   // This may cause problems with UV wrapping on Phoenix.
   fixUVCoords(leafTris);

   typedef Unifier<Univert> UnivertUnifier;
   UnivertUnifier mLeafVertUnifier;

   const int firstChunkedVert = mChunkedGeom.numVerts();

   for (int leafTriIter = 0; leafTriIter < node.numObjects(); leafTriIter++)
   {
      const int meshTriIndex = debugRangeCheck(node.objIndex(leafTriIter), mMeshTris.size());

      Unigeom::Tri unigeomTri;

      const int origTriIndex = mMeshTrisOrigTriIndex[meshTriIndex];
      const int materialIndex = mSrcGeom.tri(origTriIndex).materialIndex();
      Unigeom::BMaterial triMaterial(mSrcGeom.material(materialIndex));
      triMaterial.setAccessoryIndex(leafNodeIndex);
      
      // FIXME: Merging materials is very slow!
      unigeomTri.setMaterialIndex(mChunkedGeom.insertMaterial(triMaterial));

      for (int triVertIter = 0; triVertIter < NumTriVerts; triVertIter++)
         unigeomTri[triVertIter] = 
         firstChunkedVert + mLeafVertUnifier.insert(leafTris[leafTriIter][triVertIter]).first;

      Unigeom::TriIndex triIndex = mChunkedGeom.insertTri(unigeomTri);
      accessory.objectIndices().pushBack(triIndex);
   }

   trace("BStaticGeomBuilder::createLeafNodeGeom: Tris: %i, Remerged Verts: %i", node.numObjects(), mLeafVertUnifier.size());

   Unigeom::MorphTarget& chunkedMorphTarget = mChunkedGeom.morphTarget(0);
   for (int leafVertIter = 0; leafVertIter < mLeafVertUnifier.size(); leafVertIter++)
   {
      const Univert& vert = mLeafVertUnifier[leafVertIter];
      mChunkedGeom.insertVert(vert);

      Unigeom::MorphVert morphVert;
      morphVert.p = vert.p;
      morphVert.n = vert.n;
      chunkedMorphTarget.insertVert(morphVert);
   }
}

void BStaticGeomBuilder::createChunkedMesh(void)
{
   Unigeom::MorphTarget chunkedMorphTarget;
   mChunkedGeom.insertMorphTarget(chunkedMorphTarget);

   const BAABBTree& tree = mpAABBTreeBuilder->tree();

   for (int nodeIter = 0; nodeIter < tree.numNodes(); nodeIter++)
   {
      Unigeom::BUnpackedAccessoryType accessory;
            
      const BAABBTree::BNode& node = tree.node(nodeIter);
      if (node.leaf())
         createLeafNodeGeom(node, nodeIter, accessory);
         
      if (mRigidBoneIndex != -1)
      {
         accessory.setFirstBone(mRigidBoneIndex);
         accessory.setNumBones(1);
      }
      
      mChunkedGeom.insertAccessory(accessory);
   }
   
   mChunkedGeom.bones() = mSrcGeom.bones();
}

void BStaticGeomBuilder::createFinalTree(void)
{
   BAABBTree& tree = mpAABBTreeBuilder->tree();

   for (int nodeIter = 0; nodeIter < tree.numNodes(); nodeIter++)
   {
      BAABBTree::BNode& node = tree.node(nodeIter);
      node.objIndices().clear();
      
      AABB& bounds = node.bounds();
      
      for (uint i = 0; i < 3; i++)
      {
         if (bounds.dimension(i) == 0.0f)
         {
            bounds[0][i] -= .0000125f;
            bounds[1][i] += .0000125f;
         }
      }         
   }
}

bool BStaticGeomBuilder::build(void)
{
   mSucceeded = false;
   
   gConsoleOutput.printf("BStaticGeomBuilder::build: Creating temp mesh\n");

   if (!createTempMesh())
      return false;

   gConsoleOutput.printf("BStaticGeomBuilder::build: Creating ABT\n");

   createTree();

   gConsoleOutput.printf("BStaticGeomBuilder::build: Creating chunked mesh\n");

   createChunkedMesh();

   createFinalTree();

   mChunkedGeom.setModelType(Unigeom::Geom::cMTLarge);
   
   mSucceeded = true;
   return true;
}

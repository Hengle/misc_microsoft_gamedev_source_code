//-----------------------------------------------------------------------------
// File: indexedTriMesh.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

// xcore
#include "containers\unifier.h"
#include "math\vectorInterval.h"

// local
#include "indexedTri.h"
#include "tri.h"

//-----------------------------------------------------------------------------
// -- Converts unindexed mesh to indexed mesh, with vertex adjacency
//-----------------------------------------------------------------------------
class IndexedTriMeshBuilder
{
public:
   typedef Unifier<Univert> UnivertUnifier;
   typedef BDynamicArray<IntVec> IntVecVec;

   IndexedTriMeshBuilder() { }
   
   IndexedTriMeshBuilder(const UnivertAttributes& vertAttributes) : 
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
   
   void insert(const UnitriVec& tris)
   {
      for (uint triIter = 0; triIter < tris.size(); triIter++)
         insert(tris[triIter]);
   }

#if 0   
   void insert(const Unigeom::Geom& geom)
   {
      for (int triIter = 0; triIter < geom.numTris(); triIter++)
         insert(geom.unitri(triIter));
   }
#endif   

   // Creates list of triangles using each vertex.
   void createVertexAdjacency(void)
   {
      mVertAdj.resize(numVerts());     

      for (int triIndex = 0; triIndex < numTris(); triIndex++)
      {
         for (int triVertIndex = 0; triVertIndex < NumTriVerts; triVertIndex++)
         {
            const int vertIndex = tri(triIndex)[triVertIndex];

            IntVec& trilist = mVertAdj[debugRangeCheck(vertIndex, numVerts())];

            if (std::find(trilist.begin(), trilist.end(), triIndex) == trilist.end())
               trilist.push_back(triIndex);
         }
      }
   }

   bool hasVertexAdjacency(void) const
   {
      return mVertAdj.size() == (uint)numVerts();
   }

   int numTris(void) const { return static_cast<int>(mTriIndices.size()); }
   const IndexedTri32& tri(int triIndex) const { return mTriIndices[debugRangeCheck(triIndex, numTris())]; }

   int numVerts(void) const { return mVertices.size(); }
   const Univert& vert(int vertIndex) const { return mVertices[vertIndex]; }
   const BVec3& vertPos(int vertIndex) const { return mVertices[vertIndex].pos(); }
   const BVec3& vertNorm(int vertIndex) const { return mVertices[vertIndex].norm(); }
   const IntVec& vertAdjacency(int vertIndex) const { return mVertAdj[debugRangeCheck<int>(vertIndex, mVertAdj.size())]; }
   
   const Univert& triVert(int triIndex, int triVertIndex) const { return vert(tri(triIndex)[triVertIndex]); }
   
   Unitri unitri(int triIndex) const { return Unitri(triVert(triIndex, 0), triVert(triIndex, 1), triVert(triIndex, 2)); }
   
   BTri3 tri3(int triIndex) const { return BTri3(triVert(triIndex, 0).pos(), triVert(triIndex, 1).pos(), triVert(triIndex, 2).pos()); }
   
   const IndexedTri32Vec&        tris(void) const { return mTriIndices; }
   const UnivertUnifier&         vertices(void) const { return mVertices; }
   const UnivertAttributes&      vertAttributes(void) const { return mVertAttributes; }
   
   void setVertAttributes(const UnivertAttributes& vertAttributes) { mVertAttributes = vertAttributes; }
   
   AABB bounds(void) const
   {
      AABB aabb(AABB::eInitExpand);
      for (int vertIter = 0; vertIter < numVerts(); vertIter++)
         aabb.expand(vertPos(vertIter));
      return aabb;
   }

protected:
   IndexedTri32Vec mTriIndices;
   UnivertUnifier mVertices;
   UnivertAttributes mVertAttributes;
   IntVecVec mVertAdj;
};

//-----------------------------------------------------------------------------
// -- Connected edge
//-----------------------------------------------------------------------------
class ConnectedIndexedEdge : public IndexedEdge
{
public:
   ConnectedIndexedEdge() { }
   
   ConnectedIndexedEdge(const IndexedEdge& indexedEdge) : IndexedEdge(indexedEdge) { }
   
   void clear(void)
   {
      mSides[0].resize(0); //erase(mSides[0].begin(), mSides[0].end());
   }
   
   int numTris(int side) const                        { return static_cast<int>(mSides[debugRangeCheck(side, NumEdgeSides)].size()); }
   TriIndex triIndex(int side, int index = 0) const   { return mSides[debugRangeCheck(side, NumEdgeSides)][debugRangeCheck(index, numTris(side))]; }
   TriIndex& triIndex(int side, int index = 0)        { return mSides[debugRangeCheck(side, NumEdgeSides)][debugRangeCheck(index, numTris(side))]; }
   
   const TriIndexVec& sideVec(int side) const               { return mSides[debugRangeCheck(side, NumEdgeSides)]; }
         TriIndexVec& sideVec(int side)                     { return mSides[debugRangeCheck(side, NumEdgeSides)]; }
   
   TriIndex leftTri(int index = 0) const
   {
      if (!numTris(0))
         return cInvalidIndex;
      return triIndex(0, index);
   }      
   
   TriIndex rightTri(int index = 0) const
   {
      if (!numTris(1))
         return cInvalidIndex;
      return triIndex(1, index);
   }
   
   // returns <sideIter, index>
   // or <cInvalidIndex, cInvalidIndex>
   std::pair<int, int> findTri(TriIndex triIndex) const
   {
      for (int sideIter = 0; sideIter < NumEdgeSides; sideIter++)
      {
         for (int i = 0; i < numTris(sideIter); i++)
            if (triIndex == this->triIndex(sideIter, i))
               return std::make_pair(sideIter, i);
      }
      return std::make_pair(cInvalidIndex, cInvalidIndex);
   }
   
   int findTriOnSide(int side, TriIndex triIndex) const
   {
      debugRangeCheck(side, NumEdgeSides);
      for (int i = 0; i < numTris(side); i++)
         if (triIndex == this->triIndex(side, i))
            return i;
      return cInvalidIndex;
   }
   
   TriIndex neighborTri(TriIndex triIndex) const
   {
      std::pair<int, int> findRes = findTri(triIndex);
      if (cInvalidIndex == findRes.first)
         return cInvalidIndex;
      
      const int side = 1 - findRes.first;
      
      if (!numTris(side))
         return cInvalidIndex;
         
      return this->triIndex(side);
   }
   
   bool operator== (const ConnectedIndexedEdge& rhs) const
   {
      if (static_cast<const IndexedEdge&>(*this) != static_cast<const IndexedEdge&>(rhs))
         return false;
         
      for (int sideIter = 0; sideIter < NumEdgeSides; sideIter++)
      {
         if (mSides[sideIter].size() != rhs.mSides[sideIter].size())
            return false;
            
         for (uint i = 0; i < mSides[sideIter].size(); i++)
         {
            if (mSides[sideIter][i] != rhs.mSides[sideIter][i])
               return false;
         }
      }
      
      return true;
   }
   
   bool operator< (const ConnectedIndexedEdge& rhs) const
   {
      if (static_cast<const IndexedEdge&>(*this) < static_cast<const IndexedEdge&>(rhs))
         return true;
      else if (static_cast<const IndexedEdge&>(*this) == static_cast<const IndexedEdge&>(rhs))
      {
         for (int sideIter = 0; sideIter < NumEdgeSides; sideIter++)
         {
            if (mSides[sideIter].size() < rhs.mSides[sideIter].size())
               return true;
            else if (mSides[sideIter].size() == rhs.mSides[sideIter].size())
            {
               for (uint i = 0; i < mSides[sideIter].size(); i++)
               {
                  if (mSides[sideIter][i] < rhs.mSides[sideIter][i])
                     return true;
                  else if (mSides[sideIter][i] != rhs.mSides[sideIter][i])
                     return false;
               }
            }
         }
      }

      return false;
   }
   
   operator size_t() const
   {
      uint hash = static_cast<uint>(static_cast<size_t>(static_cast<const IndexedEdge&>(*this)));
      for (int sideIter = 0; sideIter < NumEdgeSides; sideIter++)
         hash = hashFast(&mSides[sideIter][0], static_cast<int>(mSides[sideIter].size()) * sizeof(TriIndex), hash);
      return hash;
   }
   
private:
   TriIndexVec mSides[NumEdgeSides];
};

typedef BDynamicArray<ConnectedIndexedEdge> ConnectedIndexedEdgeVec;

//-----------------------------------------------------------------------------
// -- Connected triangle 
//-----------------------------------------------------------------------------
class ConnectedTriEdges
{
public:
   enum { OppositeFlag = 0x80000000 };
   
   ConnectedTriEdges() { }
   
   ConnectedTriEdges(EdgeIndex a, EdgeIndex b, EdgeIndex c)
   {
      mEdges[0] = a;
      mEdges[1] = b;
      mEdges[2] = c;
   }
   
   void clear(void)
   {  
      mEdges[0] = cInvalidIndex;
      mEdges[1] = cInvalidIndex;
      mEdges[2] = cInvalidIndex;
   }
               
   EdgeIndex edge(int i) const      { return mEdges[debugRangeCheck(i, NumTriEdges)]; }
   EdgeIndex& edge(int i)           { return mEdges[debugRangeCheck(i, NumTriEdges)]; }
   
   EdgeIndex operator[] (int i) const  { return edge(i); }
   EdgeIndex& operator[] (int i)       { return edge(i); }
   
   EdgeIndex edgeIndex(int i) const   { return mEdges[debugRangeCheck(i, NumTriEdges)] & ~OppositeFlag; }
   int edgeSide(int i) const { return (mEdges[debugRangeCheck(i, NumTriEdges)] & OppositeFlag) != 0; }

private:
   EdgeIndex mEdges[NumTriEdges];
};

typedef BDynamicArray<ConnectedTriEdges> ConnectedTriEdgesVec;
      
//-----------------------------------------------------------------------------         
// -- Connected tri mesh         
//-----------------------------------------------------------------------------
class ConnectedIndexedTriMesh
{
public:
   ConnectedIndexedTriMesh()
   {
      clear();
   }
   
   ConnectedIndexedTriMesh(const IndexedTriMeshBuilder& indexedTriMesh)
   {
      clear();
      
      insert(indexedTriMesh);
   }
   
   void clear(void)
   {
      mHasConnectivity = false;
      mLowestVertIndex = 0xFFFFFFFF;
      mHighestVertIndex = 0;
      mTris.clear();
      mTriEdges.clear();
      mEdges.clear();
      mVertAdj.clear();
      mTriIslands.clear();
      mNumTriIslands = 0;
   }
   
   void insert(const IndexedTri& tri)
   {
      BASSERT(!mHasConnectivity);
      
      mTris.push_back(tri);
      
      for (int triVertIter = 0; triVertIter < NumTriVerts; triVertIter++)
      {
         mLowestVertIndex = Math::Min<uint>(mLowestVertIndex, tri[triVertIter]);
         mHighestVertIndex = Math::Max<uint>(mHighestVertIndex, tri[triVertIter]);
      }
   }
   
   void insert(const IndexedTriMeshBuilder& indexedTriMesh)
   {
      for (int i = 0; i < indexedTriMesh.numTris(); i++)
         insert(indexedTriMesh.tri(i));
   }
   
   void createConnectivity(void)
   {
      BASSERT(!mHasConnectivity);
      mHasConnectivity = true;
      
      createVertexAdj();
      createEdges();
   }
   
   void createTriIslands(void)
   {
      BASSERT(!mNumTriIslands);
      mTriIslands.resize(numTris());
      
      mNumTriIslands = 0;
      
      std::fill(mTriIslands.begin(), mTriIslands.end(), cInvalidIndex);
      
      BDynamicArray<int> trisToSet;
      
      for (int triIter = 0; triIter < numTris(); triIter++)
      {
         if (cInvalidIndex != mTriIslands[triIter])
            continue;
         
         trisToSet.pushBack(triIter);
         
         do 
         {
            const TriIndex triIndex = trisToSet.back();
            trisToSet.popBack();
            
            mTriIslands[debugRangeCheck(triIndex, mTriIslands.size())] = mNumTriIslands;
            
            for (int triEdgeIter = 0; triEdgeIter < NumTriEdges; triEdgeIter++)
            {
               for (int i = 0; i < numOppositeTris(triIndex, triEdgeIter); i++)
               {
                  if (cInvalidIndex == mTriIslands[debugRangeCheck(oppositeTri(triIndex, triEdgeIter, i), mTriIslands.size())])
                     trisToSet.pushBack(oppositeTri(triIndex, triEdgeIter, i));
               }
            }
         } while (!trisToSet.empty());
         
         mNumTriIslands++;
      }
   }
                                 
   int numVerts(void) const { BASSERT(mHasConnectivity); return static_cast<int>(mVertAdj.size()); }
   const IntVec& vertAdj(int vertIndex) const { BASSERT(mHasConnectivity); return mVertAdj[debugRangeCheck(vertIndex, numVerts())]; }
   
   VertIndex lowestVertex(void) const { return mLowestVertIndex; }
   VertIndex highestVertex(void) const { return mHighestVertIndex; }
   
   int numTris(void) const { return static_cast<int>(mTris.size()); }
   
   const IndexedTriVec& triVec(void) const   { return mTris; }
         IndexedTriVec& triVec(void)         { return mTris; }
         
   const IndexedTri& tri(int triIndex) const  { return mTris[debugRangeCheck(triIndex, numTris())]; }
         IndexedTri& tri(int triIndex)        { return mTris[debugRangeCheck(triIndex, numTris())]; }
   
   const ConnectedTriEdgesVec& triEdgesVec(void) const   { BASSERT(mHasConnectivity); return mTriEdges; }
         ConnectedTriEdgesVec& triEdgesVec(void)         { BASSERT(mHasConnectivity); return mTriEdges; }
         
   const ConnectedTriEdges& triEdges(int triIndex) const { BASSERT(mHasConnectivity); return mTriEdges[debugRangeCheck(triIndex, mTriEdges.size())]; }
         ConnectedTriEdges& triEdges(int triIndex)       { BASSERT(mHasConnectivity); return mTriEdges[debugRangeCheck(triIndex, mTriEdges.size())]; }
   
   const ConnectedIndexedEdgeVec& edgesVec(void) const     { BASSERT(mHasConnectivity); return mEdges; }
         ConnectedIndexedEdgeVec& edgesVec(void)           { BASSERT(mHasConnectivity); return mEdges; }
         
   int numEdges(void) const { return static_cast<int>(mEdges.size()); }
   const ConnectedIndexedEdge& edge(int edgeIndex) const { BASSERT(mHasConnectivity); return mEdges[debugRangeCheck(edgeIndex, numEdges())]; }
         ConnectedIndexedEdge& edge(int edgeIndex)       { BASSERT(mHasConnectivity); return mEdges[debugRangeCheck(edgeIndex, numEdges())]; }
      
   int numOppositeTris(TriIndex triIndex, TriEdgeIndex triEdgeIndex) const
   {
      BASSERT(mHasConnectivity); 
      
      debugRangeCheck(triIndex, numTris());
      debugRangeCheck(triEdgeIndex, NumTriEdges);
      
      const EdgeIndex edgeIndex = mTriEdges[triIndex].edgeIndex(triEdgeIndex);
      const EdgeSideIndex sideIndex = mTriEdges[triIndex].edgeSide(triEdgeIndex);

      debugRangeCheck(edgeIndex, mEdges.size());
      debugRangeCheck(sideIndex, NumEdgeSides);
      
      BASSERT(mEdges[edgeIndex].numTris(sideIndex) != 0);
      BASSERT(mEdges[edgeIndex].findTri(triIndex).first != cInvalidIndex);
      
      return mEdges[edgeIndex].numTris(1 - sideIndex);
   }
   
   int oppositeTri(TriIndex triIndex, TriEdgeIndex triEdgeIndex, int oppositeTriIndex = 0) const
   {
      BASSERT(mHasConnectivity);
      
      debugRangeCheck(triIndex, numTris());
      debugRangeCheck(triEdgeIndex, NumTriEdges);

      const EdgeIndex edgeIndex = mTriEdges[triIndex].edgeIndex(triEdgeIndex);
      const EdgeSideIndex sideIndex = mTriEdges[triIndex].edgeSide(triEdgeIndex);

      debugRangeCheck(edgeIndex, mEdges.size());
      debugRangeCheck(sideIndex, NumEdgeSides);

      BASSERT(mEdges[edgeIndex].numTris(sideIndex) != 0);
      BASSERT(mEdges[edgeIndex].findTri(triIndex).first != cInvalidIndex);

      if (oppositeTriIndex >= mEdges[edgeIndex].numTris(1 - sideIndex))
         return cInvalidIndex;
         
      return mEdges[edgeIndex].triIndex(1 - sideIndex, oppositeTriIndex);
   }
   
   std::pair<EdgeIndex, TriEdgeIndex> sharedEdge(TriIndex triA, TriIndex triB) const
   {
      BASSERT(mHasConnectivity);
      
      debugRangeCheck(triA, numTris());
      debugRangeCheck(triB, numTris());
      
      for (int triEdgeIter = 0; triEdgeIter < NumTriEdges; triEdgeIter++)
      {
         const EdgeIndex edgeIndex = mTriEdges[triA].edgeIndex(triEdgeIter);
         const EdgeSideIndex sideIndex = mTriEdges[triA].edgeSide(triEdgeIter);
         
         debugRangeCheck(edgeIndex, mEdges.size());
         debugRangeCheck(sideIndex, NumEdgeSides);
         
         if (cInvalidIndex != mEdges[edgeIndex].findTriOnSide(1 - sideIndex, triB))
            return std::make_pair(edgeIndex, triEdgeIter);
      }
      
      return std::make_pair(cInvalidIndex, cInvalidIndex);
   }
   
   void log(BTextDispatcher& m) const
   {
      m.printf("Lowest vert index: %i, Highest vert index: %i, Actual verts: %i\n", mLowestVertIndex, mHighestVertIndex, mHighestVertIndex - mLowestVertIndex + 1);
      m.printf("Num Tris: %i, Num Edges: %i\n", numTris(), numEdges());
      
      m.printf("Tris:\n");
      m.indent(1);
      for (int i = 0; i < numTris(); i++)
         m.printf("%08i Verts: %i %i %i, Edges: %i[%i] %i[%i] %i[%i]\n", i, mTris[i][0], mTris[i][1], mTris[i][2], mTriEdges[i].edgeIndex(0), mTriEdges[i].edgeSide(0), mTriEdges[i].edgeIndex(1), mTriEdges[i].edgeSide(1), mTriEdges[i].edgeIndex(2), mTriEdges[i].edgeSide(2));
      m.indent(-1);

      m.printf("Vertex adj:\n");
      m.indent(1);
      for (int i = 0; i < numVerts(); i++)
      {
         m.printf("%08i: ", i);
         for (uint j = 0; j < vertAdj(i).size(); j++)
            m.printf("%i ", vertAdj(i)[j]);
         m.printf("\n");
      }
      m.indent(-1);
               
      m.printf("Edges:\n");
      m.indent(1);
      for (int i = 0; i < numEdges(); i++)
      {
         m.printf("%08i %i %i\n", i, mEdges[i][0], mEdges[i][1]);
         
         m.printf("Side 0 tris %i: ", mEdges[i].numTris(0));
         for (int j = 0; j < mEdges[i].numTris(0); j++)
            m.printf("%i ", mEdges[i].triIndex(0, j));
         m.printf("\n");
         
         m.printf("Side 1 tris %i: ", mEdges[i].numTris(1));
         for (int j = 0; j < mEdges[i].numTris(1); j++)
            m.printf("%i ", mEdges[i].triIndex(1, j));
         m.printf("\n");
      }
      m.indent(-1);
   }
   
   int numTriIslands(void) const { return mNumTriIslands; }
   int triIslandIndex(TriIndex triIndex) const { return mTriIslands[debugRangeCheck(triIndex, mTriIslands.size())]; }
      
private:
   bool mHasConnectivity;
   VertIndex mLowestVertIndex;
   VertIndex mHighestVertIndex;
   
   // triangle vertex indices
   IndexedTriVec mTris;
   
   // triangle edge indices
   ConnectedTriEdgesVec mTriEdges;
   
   // edges, each side has list of triangle indices
   ConnectedIndexedEdgeVec mEdges;
   
   // vertex adjacency: array of triangle indices which use each vertex
   BDynamicArray<IntVec> mVertAdj;
   
   IntVec mTriIslands;
   int mNumTriIslands;
               
   void createVertexAdj(void)
   {
      mVertAdj.resize(mHighestVertIndex + 1);

      for (uint triIter = 0; triIter < mTris.size(); triIter++)
      {
         for (int triVertIter = 0; triVertIter < NumTriVerts; triVertIter++)
         {
            const int vertIndex = mTris[triIter][triVertIter];

            IntVec& trilist = mVertAdj[debugRangeCheck(vertIndex, numVerts())];

            if (std::find(trilist.begin(), trilist.end(), triIter) == trilist.end())
               trilist.push_back(triIter);
         }
      }
   }

   void createEdges(void)
   {
      mTriEdges.resize(mTris.size());

      typedef Unifier<IndexedEdge> IndexedEdgeUnifier;
      IndexedEdgeUnifier edgeUnifier;

      // find all triangle edges
      for (uint triIter = 0; triIter < mTris.size(); triIter++)
      {
         for (int triVertIter = 0; triVertIter < NumTriVerts; triVertIter++)
         {
            const IndexedEdge edge(mTris[triIter].edge(triVertIter));
            const IndexedEdge permuteEdge(edge.side(1));

            IndexedEdgeUnifier::Index edgeIndex = edgeUnifier.find(permuteEdge);
            if (cInvalidIndex != edgeIndex)
               mTriEdges[triIter][triVertIter] = edgeIndex | ConnectedTriEdges::OppositeFlag;
            else
               mTriEdges[triIter][triVertIter] = edgeUnifier.insert(edge).first;
         }
      }
      
      // now create edges
      mEdges.resize(edgeUnifier.size());
      
      for (int i = 0; i < edgeUnifier.size(); i++)
         mEdges[i] = edgeUnifier[i];
               
      for (uint triIter = 0; triIter < mTris.size(); triIter++)
      {
         for (int triEdgeIter = 0; triEdgeIter < NumTriVerts; triEdgeIter++)
         {
            const EdgeIndex edgeIndex = mTriEdges[triIter].edgeIndex(triEdgeIter);
            const EdgeSideIndex sideIndex = mTriEdges[triIter].edgeSide(triEdgeIter);
            
            debugRangeCheck(edgeIndex, mEdges.size());
            debugRangeCheck(sideIndex, NumEdgeSides);
            
            TriIndexVec& sideVec = mEdges[edgeIndex].sideVec(sideIndex);
            
            if (std::find(sideVec.begin(), sideVec.end(), triIter) == sideVec.end())
               sideVec.push_back(triIter);
         }
      }
   }
}; // class IndexedTriMeshBuilder


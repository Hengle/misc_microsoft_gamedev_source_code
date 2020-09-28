//-----------------------------------------------------------------------------
// trilist_optimizer.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

// Quickly maps verts to tris.
class VertexMap
{
public:
   typedef BDynamicArray<TriIndex> TriIndexVec;
         
   VertexMap(int numVerts) : mTriIndexLists(numVerts)
   {
   }

   VertexMap(const IndexedTriVec& tris)
   {
      VertIndex maxVertIndex = 0;
      
      for (uint i = 0; i < tris.size(); i++)
         maxVertIndex = Math::Max<int>(maxVertIndex, tris[i].maxVertIndex());
      
      BASSERT(maxVertIndex >= 0);

      mTriIndexLists.resize(maxVertIndex + 1);

      insertTris(tris);
   }

   void insertVert(TriIndex triIndex, VertIndex vertIndex)
   {
      TriIndexVec& trilist = mTriIndexLists[debugRangeCheck(vertIndex, mTriIndexLists.size())];

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
      return mTriIndexLists[debugRangeCheck(vertIndex, mTriIndexLists.size())];
   }

   int numVerts(void) const
   {
      return static_cast<int>(mTriIndexLists.size());
   }
   
private:
   BDynamicArray<TriIndexVec> mTriIndexLists;
};

class TriListOptimizer
{
public:
   TriListOptimizer(const IndexedTriVec& tris) : mTris(tris), mTotalCost(0)
   {
      optimize();
   }

   const IntVec& triOrder(void) const { return mTriOrder; }

   int numTris(void) const { return static_cast<int>(mTris.size()); }
   
   int totalCost(void) const { return mTotalCost; }

private:
   const IndexedTriVec& mTris;
   IntVec mTriOrder;
   int mTotalCost;
      
   void optimize(void)
   {
      VertexMap vertexMap(mTris);

      //const int numVerts = vertexMap.numVerts();
               
      const int VertexCacheSize = 24;
      VertexCache vcache(VertexCacheSize);

      int numTrisLeft = numTris();
      int lowestFreeTriIndex = 0;

      BDynamicArray<bool> triProcessed(numTris());

      mTotalCost = 0;
      
      BDynamicArray<TriIndex> candidateTris;
      candidateTris.reserve(512);
      
      while (numTrisLeft)
      {
         candidateTris.resize(0); //erase(candidateTris.begin(), candidateTris.end());

         for (int vcacheIndex = 0; vcacheIndex < vcache.numEntries(); vcacheIndex++)
         {
            if (-1 != vcache.at(vcacheIndex))
            {
               const VertexMap::TriIndexVec& tris = vertexMap.trisUsingVert(vcache.at(vcacheIndex));
               //candidateTris.insert(candidateTris.end(), tris.begin(), tris.end());
               
               for (uint i = 0; i < tris.size(); i++)
               {
                  const TriIndex candidateTriIndex = tris[i];
                  
                  if (!triProcessed[debugRangeCheck(candidateTriIndex, numTris())])
                     candidateTris.push_back(candidateTriIndex);
               }
            }
         }

         std::sort(candidateTris.begin(), candidateTris.end());
         BDynamicArray<TriIndex>::const_iterator lastCandidate = std::unique(candidateTris.begin(), candidateTris.end());
         
         TriIndex bestTriIndex = -1;
         float lowestCost = Math::fNearlyInfinite;

         for (BDynamicArray<TriIndex>::const_iterator it = candidateTris.begin(); 
            it != lastCandidate; ++it)
         {
            const TriIndex candidateTriIndex = *it;
            
            BASSERT(!triProcessed[debugRangeCheck(candidateTriIndex, numTris())]);
               
            const float cost = vcache.cost(mTris[candidateTriIndex], lowestCost);
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

            BASSERT(lowestFreeTriIndex != numTris());

            bestTriIndex = lowestFreeTriIndex;
            
            lowestFreeTriIndex++;
         }

         debugRangeCheck(bestTriIndex, numTris());
         BASSERT(!triProcessed[bestTriIndex]);
         
         mTotalCost += vcache.load(mTris[bestTriIndex]);
         mTriOrder.push_back(bestTriIndex);
         triProcessed[bestTriIndex] = true;
         numTrisLeft--;
      }
   }
}; // class VertexMap


//-----------------------------------------------------------------------------
// File: vertex_cache.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include "indexedTri.h"

class VertexCache
{
public:
   enum 
   { 
      MaxEntries = 64 
   };

   VertexCache(const VertexCache& a) 
   { 
      *this = a; 
   }

   VertexCache(int numEntries)
   {
      BASSERT(numEntries <= MaxEntries);
      mNumEntries = numEntries;
      clear();
   }

   void clear()
   {
      std::fill(mEntries, mEntries + mNumEntries, -1);
   }

   VertexCache& operator= (const VertexCache& rhs)
   {
      if (&rhs == this)
         return *this;
      mNumEntries = rhs.mNumEntries;
      Utils::Copy(rhs.mEntries, rhs.mEntries + rhs.mNumEntries, mEntries);
      return *this;
   }

   int push_back(int index)
   {
      const int oldestIndex = mEntries[mNumEntries - 1];
      for (int i = mNumEntries - 2; i >= 0; i--)
         mEntries[i + 1] = mEntries[i];
      mEntries[0] = index;
      return oldestIndex;
   }

   int at(int index) const
   {
      return mEntries[debugRangeCheck(index, mNumEntries)];
   }

   void set(int index, int value)
   {
      mEntries[debugRangeCheck(index, mNumEntries)] = value;
   }

   int numEntries(void) const
   {
      return mNumEntries;
   }

   // -1 if not present
   int find(int index) const
   {
      const int* pRes = std::find(mEntries, mEntries + mNumEntries, index);
      if (pRes == mEntries + mNumEntries)
         return -1;
      return pRes - mEntries;
   }

   int numHits(const IndexedTri& tri) const
   {
      int num = 0;
      for (int i = 0; i < NumTriVerts; i++)
         if (-1 != find(tri[i]))
            num++;
      return num;
   }

   float cost(const IndexedTri& tri, float abortThresh = Math::fNearlyInfinite) const
   {
      float cost = 0.0f;

      for (int i = 0; i < NumTriVerts; i++)
      {
         const int pos = find(tri[i]);
         if (-1 == pos)
         {
            cost += 1.0f;
         }
         else
         {
            cost += .01f * (pos * (1.0f/float(mNumEntries)));
         }
         if (cost > abortThresh)
            break;
      }

      return cost;
   }

   // returns cost
   int load(const IndexedTri& tri) 
   {
      int cost = 0;
      for (int i = 0; i < NumTriVerts; i++)
         if (-1 == find(tri[i]))
         {
            push_back(tri[i]);
            cost++;
         }
      return cost;
   }

private:
   int mNumEntries;
   
   // First is youngest
   int mEntries[MaxEntries];
};


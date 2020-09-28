//==============================================================================
// pathhull.h
//
// Copyright (c) 1999-2007, Ensemble Studios
//==============================================================================
#pragma once

#include "bitvector.h"

class BConvexHull;
class BOPQuadHull;

//==============================================================================
class BPathHull
{
   public:

      void                    init(const BOPQuadHull *hull);
      const BOPQuadHull       *mHull;

      bool                    mbVisited;        // For the hull itself
      BBitVector              mPointVisited;    // For each point
      BDynamicSimArray<BVector>   mLastVisited;     // The vector we were examining when we first visited this node.

}; 

typedef BDynamicSimArray<BPathHull> BSimplePathHullArray;
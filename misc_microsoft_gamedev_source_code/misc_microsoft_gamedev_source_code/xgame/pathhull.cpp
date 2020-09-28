//==============================================================================
// pathhull.cpp
//
// Copyright (c) 1999, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "pathhull.h"
#include "convexhull.h"

//==============================================================================
// BPathHull::init
//==============================================================================
void BPathHull::init(const BOPQuadHull *hull)
{
   // Save hull.
   mHull=hull;

   // Size array.
   if(mHull)
   {
      // jce todo: make this static I guess
      mLastVisited.setNumber(4);
   }
   else
      mLastVisited.setNumber(0);

   // Not yet visited.
   mbVisited=false;
   mPointVisited.zero();
}


//==============================================================================
// eof: pathhull.cpp
//==============================================================================

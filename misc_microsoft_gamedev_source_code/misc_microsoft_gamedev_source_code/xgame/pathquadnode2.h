//==============================================================================
// pathquad.h
//
// Copyright (c) 2000-2007, Ensemble Studios
//==============================================================================
#pragma once

#define DEBUG_PATHQUADLAND

//==============================================================================
class BPathQuadNode2
{
   public:
      BPathQuadNode2() : mlObstructionMask(0x0000)
#ifdef DEBUG_PATHQUADLAND
      , mColor(cColorBlack)
#endif
      {}

//      long                       mlInUse;
      unsigned long              mlObstructionMask;      // What we're obstructed with
      float                      mfMinX;
      float                      mfMinZ;
      float                      mfMaxX;
      float                      mfMaxZ;
#ifdef DEBUG_PATHQUADLAND
      BColor                     mColor;                 // Debug color
#endif      

}; // BPathQuadNode2


typedef BQuadTree<BPathQuadNode2> BPathQuadTree2;
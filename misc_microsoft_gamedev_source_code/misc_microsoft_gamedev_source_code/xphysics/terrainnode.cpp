//==============================================================================
// terrainnode.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "terrainnode.h"

//==============================================================================

//==============================================================================
// BTerrainNode::BTerrainNode
//==============================================================================
BTerrainNode::BTerrainNode() 
{
   mSurfaces.setNumber(0);
}

//==============================================================================
// BTerrainNode::addSurfaceInfo
//==============================================================================
void BTerrainNode::addSurfaceInfo(long triangle, long surface)
{
   mSurfaces.setNumber(triangle+1);
   mSurfaces[triangle] = surface;
}

//==============================================================================
// BTerrainNode::getSurface
//==============================================================================
long BTerrainNode::getSurface(long triangle) const
{
   if ((triangle < 0) || (triangle >= mSurfaces.getNumber()))
      return(-1);
   return(mSurfaces[triangle]);
}

//==============================================================================
// BTerrainNode::operator=
//==============================================================================
BTerrainNode &BTerrainNode::operator=(const BTerrainNode &node)
{
   long size = node.mSurfaces.getNumber();
   mSurfaces.setNumber(size);
   long i;
   for (i=0; i < size; i++)
   {
      mSurfaces[i] = node.mSurfaces[i];
   }
   mID = node.mID;
   return(*this);
}

//==============================================================================

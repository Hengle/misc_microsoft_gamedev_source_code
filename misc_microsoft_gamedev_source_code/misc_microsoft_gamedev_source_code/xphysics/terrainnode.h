//==============================================================================
// terrainnode.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _TERRAINNODE_H_
#define _TERRAINNODE_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations


//==============================================================================
// BTerrainNode
//==============================================================================
class BTerrainNode
{
public:
   BTerrainNode();

   void              addSurfaceInfo(long triangle, long surface);
   long              getSurface(long triangle) const;

   long              getID(void) const { return mID; }
   void              setID(long v) { mID = v; }

   BTerrainNode &operator=(const BTerrainNode &node);

protected:
   // Xemu [10/21/2003] -- List of surfaces by triangle
   BDynamicSimArray<long>      mSurfaces;
   long              mID;
};

#endif
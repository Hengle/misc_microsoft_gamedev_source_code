//============================================================================
//
//  TerrainSimRep.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

//XCORE
#include "xcore.h"

struct XTDSim;
class BTerrainQuadNode;
class BTerrainPhysicsHeightfield;

typedef USHORT HALF;

//------------------------------------------
class BTerrainSimRep
{
public:

   BTerrainSimRep();
   ~BTerrainSimRep();

   //creation
   bool              loadECFXSDInternal(int dirID,const char *filenameNoExtention);
/*
   void              destroy();

   //intersection 
   bool  rayIntersects(const BVector origin, const BVector dir, BVector &intersectionPt, bool allowOriginUnderTerrain = false) const;
   bool  rayIntersectsGuaranteed(const BVector origin, const BVector dir, BVector &intersectionPt) const;   //if misses terrain, return XZ plane intersect
   bool  segmentIntersects(const BVector pt0, const BVector pt1, BVector &intersectionPt, bool allowOriginUnderTerrain = false) const;

   //Height Query
   bool	            getHeight(BVector &position, bool clamp) const;                                        //given an XZ, returns mHeights[x *width + z]
   bool	            getHeight(const int x, const int z, float &retHeight, bool clamp) const;               //given an XZ, returns mHeights[x *width + z]
   bool              getHeightRaycast(BVector origin, float &retHeight, bool clamp) const;                  //casts a downward ray (0,-1,0) directly to the indexed triangles.

   //Tile type query
   BYTE              getTileType(BVector position) const;
   BYTE              getTileType(int x, int z) const;

   int               getNumXVerts() const   {return mNumXVerts;}
*/
   int               getNumXTiles() const   {return mNumXDataTiles;} 
   float             getTileScale() const   {return mDataTileScale;}
/*
   float             getReciprocalTileScale() const     { return mRecTileScale; }
   float             getVisualToSimTileScalar() const   { return mVisualToSimScalar; }

   bool              createPhysics();
   void              releasePhysics();

   void              addDebugLineOverTerrain(BVector point, BVector point2, DWORD color1, DWORD color2, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugThickLineOverTerrain(BVector point, BVector point2, float thickness, DWORD color1, DWORD color2, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugBoxOverTerrain(BVector p1, BVector p2, BVector p3, BVector p4, DWORD color, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugThickBoxOverTerrain(BVector p1, BVector p2, BVector p3, BVector p4, float thickness, DWORD color, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugCircleOverTerrain(BVector center, float radius, DWORD color, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugThickCircleOverTerrain(BVector center, float radius, float thickness, DWORD color, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
*/
   BYTE              *getObstructionMap() const    { return mpObstructionMap; }
/*
   //Debug
   void              render();

   // Conversion accessors
   int               clampTile(int tile) const                 { return Math::Clamp(tile, 0, getNumXTiles() - 1); }
   float             tileToWorld(int tile) const               { return float(tile) * mTileScale; }
   int               worldToTile(float world) const            { return int(world * mRecTileScale); }
   BVector           tileToWorldNoHeight(int tx, int tz) const { return BVector(tileToWorld(tx), 0.0f, tileToWorld(tz)); }
   BVector           tileToWorld(int tx, int tz) const         { BVector world = tileToWorld(tx, tz); getHeight(tx, tz, world.y, false); return world; }
   void              worldToTile(BVector world, int &tx, int &tz) const { XMVECTOR tilePosition = __vctsxs(XMVectorMultiply(world, XMVectorReplicate(getReciprocalTileScale())), 0); tx = tilePosition.u[0]; tz = tilePosition.u[2]; }
   void              worldToTile(float wx, float wz, int &tx, int &tz) const { tx = worldToTile(wx); tz = worldToTile(wz); }
*/
private:
//   bool  rayIntersectsInternal(const long iX, const long iZ, const float x, const float z, const BVector origin, const BVector dir, BVector &intersectionPt) const;


   //sim height rep
   HALF        *mpHeights;                //for heights
   int         mNumXHeightBlocks;               //for heights
   int         mNumXHeightVerts;
   int         mDataTileToHeightVertScalar;         //always integer scalar (x2, x4, etc)
   float       mHeightTileScale;
   float       mRcpHeightTileScale;

   //sim data rep
   int         mNumXDataTiles;                //for tiles
   float       mDataTileScale;
   float       mRecDataTileScale;
   float       mVisualToSimScalar;
   BYTE        *mpTileType;
   BYTE        *mpObstructionMap;

   BTerrainPhysicsHeightfield       *mpPhysicsTerrain;
   
};

extern BTerrainSimRep gTerrainSimRep;

bool LoadXBOXSimRep(long dirID, const char *filename);

//============================================================================
//
//  TerrainSimRep.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

struct XTDSim;
class BTerrainQuadNode;
class BTerrainPhysicsHeightfield;
class BOPQuadHull;
#include "debugprimitives.h"


//------------------------------------------
class BTerrainSimRep
{
public:

   BTerrainSimRep();
   ~BTerrainSimRep();

   //creation
   bool              loadFromFile(long dirID, const char *filename);
   void              destroy();
   bool              getLoaded() const { return mLoaded; }

   //intersection 
   bool  rayIntersects(const BVector origin, const BVector dir, BVector &intersectionPt, bool allowOriginUnderTerrain = false) const;
   bool  rayIntersectsGuaranteed(const BVector origin, const BVector dir, BVector &intersectionPt) const;   //if misses terrain, return XZ plane intersect
   bool  segmentIntersects(const BVector pt0, const BVector pt1, BVector &intersectionPt, bool allowOriginUnderTerrain = false) const;

   //Camera Height Query
   bool	            getHeight(BVector &position, bool clamp) const;                                        //given an XZ, returns mHeights[x *width + z]
   bool	            getHeight(const int x, const int z, float &retHeight, bool clamp) const;               //given an XZ, returns mHeights[x *width + z]
   bool              getHeightRaycast(BVector origin, float &retHeight, bool clamp) const;                  //casts a downward ray (0,-1,0) directly to the indexed triangles.
   float             getHeightFast(XMVECTOR v) const;


   //Camera Height Query
   bool              cameraHeightsLoaded(){return mpCameraHeights!=NULL;};
   bool	            getCameraHeight(BVector &position, bool clamp) const;
   bool	            getCameraHeight(const int x, const int z, float &retHeight, bool clamp) const;               //given an XZ, returns mpCameraHeights[x *width + z]
   bool              getCameraHeightRaycast(BVector origin, float &retHeight, bool clamp, bool reflect=false) const; //casts a downward ray (0,-1,0) directly to the indexed triangles.
   bool              rayIntersectsCamera(const BVector origin, const BVector dir, BVector &intersectionPt, bool allowOriginUnderTerrain = false) const;

   //Flight Height Query
   bool              flightHeightsLoaded(){return mpFlightHeights!=NULL;};
   bool	            getFlightHeight(BVector &position, bool clamp) const;
   bool	            getFlightHeight(const int x, const int z, float &retHeight, bool clamp) const;               //given an XZ, returns mpCameraHeights[x *width + z]
   bool              getFlightHeightRaycast(BVector origin, float &retHeight, bool clamp, bool reflect=false) const; //casts a downward ray (0,-1,0) directly to the indexed triangles.
   bool              rayIntersectsFlight(const BVector origin, const BVector dir, BVector &intersectionPt, bool allowOriginUnderTerrain = false) const;


   //Tile type query
   BYTE              getTileType(BVector position) const;
   BYTE              getTileType(int x, int z) const;

   int               getNumXHeightVerts() const   {return mNumXHeightVerts;}
   float             getHeightTileScale() const   {return mHeightTileScale;}

   int               getNumXDataTiles() const   {return mNumXDataTiles;} 
   float             getDataTileScale() const   {return mDataTileScale;}

   float             getReciprocalDataTileScale() const     { return mRecDataTileScale; }

   float             getVisualToSimTileScalar() const   { return mVisualToSimScalar; }

   bool              createPhysics();
   void              releasePhysics();

   void              addDebugLineOverTerrain(BVector point, BVector point2, DWORD color1, DWORD color2, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugThickLineOverTerrain(BVector point, BVector point2, float thickness, DWORD color1, DWORD color2, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugSquareOverTerrain(BVector p1, BVector p2, BVector p3, BVector p4, DWORD color, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugSquareOverTerrain(BVector center, BVector forward, DWORD color, float terrainOffset, float scale = 1.0f, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugThickSquareOverTerrain(BVector p1, BVector p2, BVector p3, BVector p4, float thickness, DWORD color, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugThickSquareOverTerrain(BVector center, BVector forward, float thickness, DWORD color, float terrainOffset, float scale = 1.0f, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugTriangleOverTerrain(BVector center, BVector forward, DWORD color, float terrainOffset, float scale = 1.0f, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugThickTriangleOverTerrain(BVector center, BVector forward, float thickness, DWORD color, float terrainOffset, float scale = 1.0f, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugCrossOverTerrain(BVector center, BVector forward, DWORD color, float terrainOffset, float scale = 1.0f, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugThickCrossOverTerrain(BVector center, BVector forward, float thickness, DWORD color, float terrainOffset, float scale = 1.0f, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugArrowOverTerrain(BVector center, BVector forward, DWORD color, float terrainOffset, float scale = 1.0f, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugThickArrowOverTerrain(BVector center, BVector forward, float thickness, DWORD color, float terrainOffset, float scale = 1.0f, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugCircleOverTerrain(BVector center, float radius, DWORD color, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f, float radians = cTwoPi, float angularOffset = 0.0f);
   void              addDebugThickCircleOverTerrain(BVector center, float radius, float thickness, DWORD color, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f, float radians = cTwoPi, float angularOffset = 0.0f);
   void              addDebugPointOverTerrain(BVector center, float radius, DWORD color, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugThickPointOverTerrain(BVector center, float radius, DWORD color, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);
   void              addDebugRoundedBoxOverTerrain(BOPQuadHull* obsHull, float range, DWORD color, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);

   void              addDebugLineOverCameraRep(BVector point, BVector point2, DWORD color1, DWORD color2, float terrainOffset, int category = BDebugPrimitives::cCategoryNone, float timeout=-1.0f);

   BYTE              *getObstructionMap() const    { return mpObstructionMap; }
   BYTE              *getBuildableMap() const      { return mpBuildableMap; }
 
   //Debug
   void              debugRender();
   void              debugRenderCameraHeights();
   void              debugRenderFlightHeights();

   // Conversion accessors
   int               clampTile(int tile) const                 { return Math::Clamp(tile, 0, getNumXDataTiles() - 1); }
   float             tileToWorld(int tile) const               { return float(tile) * mDataTileScale; }
   int               worldToTile(float world) const            { return int(world * mRecDataTileScale); }
   BVector           tileToWorldNoHeight(int tx, int tz) const { return BVector(tileToWorld(tx), 0.0f, tileToWorld(tz)); }
   BVector           tileToWorld(int tx, int tz) const         { BVector world = tileToWorld(tx, tz); getHeight(tx, tz, world.y, false); return world; }
   void              worldToTile(BVector world, int &tx, int &tz) const { XMVECTOR tilePosition = __vctsxs(XMVectorMultiply(world, XMVectorReplicate(getReciprocalDataTileScale())), 0); tx = tilePosition.u[0]; tz = tilePosition.u[2]; }
   void              worldToTile(float wx, float wz, int &tx, int &tz) const { tx = worldToTile(wx); tz = worldToTile(wz); }
   void              clampWorld(BVector &position);
   void              clampWorldWithBuffer(BVector &position, float buffer);

   const BTerrainPhysicsHeightfield*   getPhysicsHeightfield() const { return mpPhysicsTerrain; }

   //deformation
   void              flattenInstant(float mMinXPerc,float mMaxXPerc,float mMinZPerc,float mMaxZPerc,float desiredHeight);

private:
   bool  loadECFXSDInternal(int dirID,const char *filenameNoExtention);
   bool  rayIntersectsInternal(const long iX, const long iZ, const float x, const float z, const BVector origin, const BVector dir, BVector &intersectionPt) const;
   bool  rayIntersectsCameraInternal(const long iX, const long iZ, const float x, const float z, const BVector origin, const BVector dir, BVector &intersectionPt) const;
   bool  rayIntersectsFlightInternal(const long iX, const long iZ, const float x, const float z, const BVector origin, const BVector dir, BVector &intersectionPt) const;

   void  clampRayOrgToGrid(const BVector org, const BVector dir, BVector& clampedOrg)  const;

   //sim height rep
   XMVECTOR    mRcpHeightTileScaleV;
   HALF        *mpHeights;                //for heights
   int         mNumXHeightBlocks;               //for heights
   int         mNumXHeightVerts;


   int         mBlockSize;
   int         mBlockPitch;
   int         mHeightBlocksPitch;
   

   int         mDataTileToHeightVertScalar;         //always integer scalar (x2, x4, etc)
   float       mHeightTileScale;
   float       mRcpHeightTileScale;
   void        setHeight(const int x, const int z, float Height);

   //sim data rep
   int         mNumXDataTiles;                //for tiles
   float       mDataTileScale;
   float       mRecDataTileScale;
   float       mVisualToSimScalar;
   BYTE        *mpTileType;
   BYTE        *mpObstructionMap;
   BYTE        *mpBuildableMap;

   //camera height rep
   HALF        *mpCameraHeights;                //for heights
   int         mNumXCameraHeightBlocks;               //for heights
   int         mNumXCameraHeightVerts;
   float       mCameraHeightTileScale;
   float       mRcpCameraHeightTileScale;

   //flight height rep
   HALF        *mpFlightHeights;                //for heights
   int         mNumXFlightHeightBlocks;               //for heights
   int         mNumXFlightHeightVerts;
   float       mFlightHeightTileScale;
   float       mRcpFlightHeightTileScale;

   bool        mLoaded;

   

   
   
   
   
   

   BTerrainPhysicsHeightfield       *mpPhysicsTerrain;
};

extern BTerrainSimRep gTerrainSimRep;
//============================================================================
//
//  TerrainSimRep.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
// xgame 
#include "common.h"

//terrain
#include "TerrainSimRep.h"
#include "terrain.h"
#include "mathutil.h"
#include "terrainphysicsheightfield.h"
#include "obstructionmanager.h"
#include "debugprimitives.h"
#include "render.h"
#include "renderThread.h"

//xcore
#include "consoleOutput.h"
#include "resource\ecfUtils.h"
#include "bfileStream.h"

#define cNumCirclePoints   32
#define cNumThickCirclePoints 48

//---------------------------------------------------------
BTerrainSimRep gTerrainSimRep;

//---------------------------------------------------------
BTerrainSimRep::BTerrainSimRep():
   mpHeights(NULL),
   mpCameraHeights(NULL),
   mpFlightHeights(NULL),
   mpBuildableMap(NULL),
   mpTileType(NULL),
   mpPhysicsTerrain(NULL),
   mLoaded(false),
   mBlockSize(64),
   mBlockPitch(8),
   mHeightBlocksPitch(0)
{

}
//---------------------------------------------------------
BTerrainSimRep::~BTerrainSimRep()
{
   destroy();  
}
//---------------------------------------------------------
bool BTerrainSimRep::loadFromFile(long dirID, const char *filename)
{
   return (loadECFXSDInternal(dirID,filename));
}
//---------------------------------------------------------
void BTerrainSimRep::destroy()
{
   gRenderThread.blockUntilGPUIdle();

   releasePhysics();

   if (mpTileType)
   {
      delete [] mpTileType;
      mpTileType = NULL;
   }

   if (mpHeights)
   {
      delete [] mpHeights;
      mpHeights = NULL;
   }

   if (mpObstructionMap)
   {
      delete [] mpObstructionMap;
      mpObstructionMap = NULL;
   }
   if (mpBuildableMap)
   {
      delete []mpBuildableMap;
      mpBuildableMap = NULL;
   }
   
   if (mpCameraHeights)
   {
      delete []mpCameraHeights;
      mpCameraHeights = NULL;
   }

   if (mpFlightHeights)
   {
      delete []mpFlightHeights;
      mpFlightHeights = NULL;
   }

   mLoaded=false;
}
//---------------------------------------
bool	BTerrainSimRep::getHeight(BVector &position, bool clamp) const
{
   XMVECTOR tilePosition = __vctsxs(XMVectorMultiply(position, mRcpHeightTileScaleV), 0);
   const int x = tilePosition.u[0];
   const int z = tilePosition.u[2];
   return getHeight(x, z, position.y, clamp);
}
//---------------------------------------
bool	BTerrainSimRep::getHeight(const int heightGridX, const int heightGridY,float &retHeight,bool clamp) const
{
   BDEBUG_ASSERT(mpHeights);
   
   //we assume that [x,y] are in height grid space
   int tx=heightGridX;
   int ty=heightGridY;
   if(heightGridX >= mNumXHeightVerts)
   {
      if(clamp)   tx=mNumXHeightVerts-1;
      else        return false;
   }
   if(heightGridX < 0)
   {
      if(clamp)   tx=0;
      else        return false;
   }

   if(heightGridY >= mNumXHeightVerts)
   {
      if(clamp)   ty=mNumXHeightVerts-1;
      else        return false;
   }  
   if(heightGridY < 0)   
   {
      if(clamp)   ty=0;
      else        return false;
   }

   //convert from raster order to blocking
   const uint blockSize = 64;     // 8x8
   const uint blockPitch = 8;     
   const uint pitch = mNumXHeightBlocks*64;

   const uint bx = (tx>>3) *blockSize;
   const uint by = (ty>>3) *pitch;

   const uint BlkIndx = (by+bx);

   const uint lx = (tx&0x07);
   const uint ly = (ty&0x07) * blockPitch;

   const uint lIndx = lx+ly;

   const uint indx = BlkIndx + lIndx;

   //CLM [05.02.08] Removed LHS
   //retHeight=XMConvertHalfToFloat(mpHeights[indx]);
   XMVECTOR vX = XMLoadHalf2((XMHALF2*)&mpHeights[indx]);
   XMStoreScalar(&retHeight,vX);

   return true;
}


//---------------------------------------
float BTerrainSimRep::getHeightFast(XMVECTOR v) const
{
   BDEBUG_ASSERT(mpHeights);

   XMVECTOR tilePosition = __vctsxs(XMVectorMultiply(v, mRcpHeightTileScaleV), 0);
   int tx = Math::Clamp((int) tilePosition.u[0], 0, mNumXHeightVerts-1);
   int ty = Math::Clamp((int) tilePosition.u[2], 0, mNumXHeightVerts-1);
               
   int bx = (tx>>3)*mBlockSize;
   int by = (ty>>3)*mHeightBlocksPitch;   
   int lx = (tx&0x07);
   int ly = (ty&0x07)*mBlockPitch;
      
   //CLM [05.02.08] Removed LHS
   //return XMConvertHalfToFloat(*(mpHeights+bx+by+lx+ly));
   float retHeight = 0;
   const uint indx = bx+by+lx+ly;
   XMVECTOR vX = XMLoadHalf2((XMHALF2*)&mpHeights[indx]);
   XMStoreScalar(&retHeight,vX);
   return retHeight;
}

//---------------------------------------
void BTerrainSimRep::setHeight(const int heightGridX, const int heightGridY, float Height)
{
   BDEBUG_ASSERT(mpHeights);

   //we assume that [x,y] are in height grid space
   int tx=heightGridX;
   int ty=heightGridY;
   if(heightGridX < 0 || heightGridY < 0 || heightGridX >= mNumXHeightVerts || heightGridY >= mNumXHeightVerts)
      return;
   
  


   //convert from raster order to blocking
   int blockSize = 64;     // 8x8
   int blockPitch = 8;     
   int pitch = mNumXHeightBlocks*64;

   int bx = (tx>>3) *blockSize;
   int by = (ty>>3) *pitch;

   int BlkIndx = (by+bx);

   int lx = (tx&0x07);
   int ly = (ty&0x07) * blockPitch;

   int lIndx = lx+ly;

   int indx = BlkIndx + lIndx;

   mpHeights[indx]=XMConvertFloatToHalf(Height);
}
//---------------------------------------
bool BTerrainSimRep::rayIntersects(const BVector origin, const BVector dir, BVector &intersectionPt, bool allowOriginUnderTerrain) const
{
   SCOPEDSAMPLE(BTerrainSimRep_rayIntersects)
   BDEBUG_ASSERT(mpHeights);

   // X or Z major slope
   const float    dirX = dir.x;
   const float    dirZ = dir.z;
   const float    aDirX = abs(dirX);
   const float    aDirZ = abs(dirZ);
   const float    recADirX = 1.0f / aDirX;
   const float    recADirZ = 1.0f / aDirZ;
   const bool     straightY = ((aDirX + aDirZ) < cFloatCompareEpsilon) ? true : false;
   const bool     xMajor = (aDirX >= aDirZ) ? true : false;
   const bool     xPositive = (dirX >= 0.0f) ? true : false;
   const bool     zPositive = (dirZ >= 0.0f) ? true : false;
   const float    fMaxTiles = float(mNumXHeightVerts-1);
   const float    maxWorld = fMaxTiles * mHeightTileScale;
   // Save position
   BVector  position(origin);

   // Place the ray's origin within the map's area, if possible
   const bool     leftClip = (position.x < 0.0f) ? true : false;
   const bool     rightClip = (position.x >= maxWorld) ? true : false; //CLM [09.04.07] increased bordering...
   const bool     topClip = (position.z >= maxWorld) ? true : false;//CLM [09.04.07] increased bordering...
   const bool     bottomClip = (position.z < 0.0f) ? true : false;
   if ((leftClip && xPositive) || (rightClip && !xPositive) || (topClip && !zPositive) || (bottomClip && zPositive))
   {
      const bool xValid = (aDirX == 0.0f) ? false : true;
      const bool zValid = (aDirZ == 0.0f) ? false : true;

      float xRatio, zRatio;

      if (xValid)
      {
         if (leftClip || rightClip)
            xRatio = abs(position.x - ((xPositive) ? 0.0f : maxWorld)) * recADirX;
         else
            xRatio = 0.0f;
      }
      else
         xRatio = 0.0f;

      if (zValid)
      {
         if (topClip || bottomClip)
            zRatio = abs(position.z - ((zPositive) ? 0.0f : maxWorld)) * recADirZ;
         else
            zRatio = 0.0f;
      }
      else
         zRatio = 0.0f;

      if (xValid && (xRatio > zRatio))
         position = XMVectorMultiplyAdd(dir, XMVectorReplicate(xRatio), position);
      else if (zValid && (zRatio > xRatio))
         position = XMVectorMultiplyAdd(dir, XMVectorReplicate(zRatio), position);
   }

   // Make sure we start over the terrain
   float height;
   if (!allowOriginUnderTerrain && (!getHeightRaycast(position, height, false) || (position.y < height)))
      return false;

   // Convert world to tile
   BVector  truncatedTile = XMVectorTruncate(XMVectorMultiply(position, mRcpHeightTileScaleV));
   // Save off tile coordinates (float and int versions)
   float x = Math::Min(truncatedTile.x, fMaxTiles-1);//maxWorld - 1.0f); //CLM [12.07.07] this should be in tile space.. No?
   float z = Math::Min(truncatedTile.z, fMaxTiles-1);//maxWorld - 1.0f); //CLM [12.07.07] this should be in tile space.. No?
   long  iX = long(x);
   long  iZ = long(z);
   // inc variables
   BVector  dirScale;

   if (xMajor)
      dirScale = XMVectorReplicate(recADirX);
   else
      dirScale = XMVectorReplicate(recADirZ);

   // Keep looping until we're off the map
   while ((iX >= 0) && (iX < fMaxTiles) && (iZ >= 0) && (iZ < fMaxTiles))
   {
      if (rayIntersectsInternal(iX, iZ, x, z, origin, dir, intersectionPt))
         return true;

      // Exit if dir is straight up or down
      if (straightY)
         return false;

      // find next tile
      const float oldX = x;
      const float oldZ = z;
      const long  oldIX = iX;
      const long  oldIZ = iZ;
      const BVector  oldPosition(position);

      //CLM [04.28.08] The float-walk method is fine, but causes us to test ray intersections
      // on the same tile multiple times (often 2x-3x!!)
      // This small loop ensures we don't doddle in the same tile.
      do
      {
         //CLM Theres some issues with LHS and jumping between the VMX register, Float register, and Int register
         // For now, this is small beans though..
         position = XMVectorMultiplyAdd(dirScale, dir, position);
         truncatedTile = XMVectorTruncate(XMVectorMultiply(position, mRcpHeightTileScaleV));
         x = truncatedTile.x;
         z = truncatedTile.z;
         iX = long(x);
         iZ = long(z);
      }while(oldIX == iX && oldIZ == iZ);
      

      // Take care of holes caused by diagonal movement
      if ((iX != oldIX) && (iZ != oldIZ))
      {
         if (rayIntersectsInternal(iX, oldIZ, x, oldZ, origin, dir, intersectionPt))
            return true;

         if (rayIntersectsInternal(oldIX, iZ, oldX, z, origin, dir, intersectionPt))
            return true;
      }
   };

   return false;
}
//---------------------------------------
bool  BTerrainSimRep::rayIntersectsGuaranteed(const BVector origin, const BVector dir, BVector &intersectionPt) const
{
   if (!rayIntersects(origin, dir, intersectionPt))
   {
      intersectionPt.x = origin.x;
      intersectionPt.y = 0.0f;
      intersectionPt.z = origin.z;
   }
   return true;
}
//---------------------------------------
bool  BTerrainSimRep::segmentIntersects(const BVector pt0, const BVector pt1, BVector &intersectionPt, bool allowOriginUnderTerrain) const
{
   BDEBUG_ASSERT(mpHeights);

   BVector dir, diff;
   diff.assignDifference(pt1, pt0);
   dir = diff;
   dir.normalize();

   bool hit = false;
   hit = rayIntersects(pt0, dir, intersectionPt, allowOriginUnderTerrain);

   if (hit)
   {
      float targetLen = diff.lengthSquared();
      diff.assignDifference(intersectionPt, pt0);
      float intersectionLen = diff.lengthSquared();

      return (intersectionLen <= targetLen) ? true : false;
   }

   return false;
}
//---------------------------------------
bool  BTerrainSimRep::getHeightRaycast(BVector origin, float &retHeight, bool clamp) const
{
   BDEBUG_ASSERT(mpHeights);

   retHeight = 0.0f;

   // Convert world to tile
   BVector  truncatedTile = XMVectorTruncate(XMVectorMultiply(origin, mRcpHeightTileScaleV));
   origin.y = 0.0f;
   // Clamp
   if (clamp)
      truncatedTile = XMVectorClamp(truncatedTile, XMVectorZero(), XMVectorReplicate(float(mNumXHeightVerts - 1)));
   // Save off tile coordinates (float and int versions)
   float x = truncatedTile.x;
   float z = truncatedTile.z;
   long  iX = long(x);
   long  iZ = long(z);

   if ((iX < 0) || (iX > mNumXHeightVerts-1) || (iZ < 0) || (iZ > mNumXHeightVerts-1))
      return false;

   // Calculate tile coordinates in world space
   const float x1 = x * mHeightTileScale;
   const float z1 = z * mHeightTileScale;

   float y0, y1, y2, y3;
   getHeight(iX, iZ, y0, true);
   getHeight(iX, iZ + 1, y1, true);
   getHeight(iX + 1, iZ + 1, y2, true);
   getHeight(iX + 1, iZ, y3, true);

   float topX = LERP(y1, y2, (origin.x - x1) * mRcpHeightTileScale);
   float botX = LERP(y0, y3, (origin.x - x1) * mRcpHeightTileScale);

   retHeight = LERP(botX, topX, (origin.z - z1) * mRcpHeightTileScale);


   return true;
}

//---------------------------------------
bool  BTerrainSimRep::getCameraHeightRaycast(BVector origin, float &retHeight, bool clamp, bool reflect) const
{
   // This needs to return the same results than when doing an intersection test, else bad things happen
   // Call rayIntersectsCamera for now instead of the fast approach. (SAT)
   //
   BVector pos = origin + BVector(0.0f, 1000.0f, 0.0f);
   BVector dir(0.0f, -1.0f, 0.0f);
   BVector intersection;

   bool hit = rayIntersectsCamera(pos, dir, intersection);

   if(hit)
      retHeight = intersection.y;

   return(hit);



   BVector position = origin;
   if (reflect)
   {
      // If the position passed in (origin) is off the map, reflect it to a corresponding position that is within map boundaries
      if (origin.x < 0.0f)
         position.x = -origin.x;
      if (origin.z < 0.0f)
         position.z = -origin.z;
      if (origin.x > gTerrain.getMax().x)
         position.x = 2.0f * gTerrain.getMax().x - origin.x;
      if (origin.z > gTerrain.getMax().z)
         position.z = 2.0f * gTerrain.getMax().z - origin.z;
   }

   if (!mpCameraHeights)
      return getHeightRaycast(origin, retHeight, clamp);

   retHeight = 0.0f;

   // Convert world to tile
   BVector  truncatedTile = XMVectorTruncate(XMVectorMultiply(origin, XMVectorReplicate(mRcpCameraHeightTileScale)));
   origin.y = 0.0f;

   // Clamp
   if (clamp)
      truncatedTile = XMVectorClamp(truncatedTile, XMVectorZero(), XMVectorReplicate(float(mNumXCameraHeightVerts - 1)));
   // Save off tile coordinates (float and int versions)
   float x = truncatedTile.x;
   float z = truncatedTile.z;
   long  iX = long(x);
   long  iZ = long(z);

   if ((iX < 0) || (iX > mNumXCameraHeightVerts-1) || (iZ < 0) || (iZ > mNumXCameraHeightVerts-1))
      return false;

   // Calculate tile coordinates in world space
   const float x1 = x * mCameraHeightTileScale;
   const float z1 = z * mCameraHeightTileScale;

   float y0, y1, y2, y3;
   getCameraHeight(iX, iZ, y0, true);
   getCameraHeight(iX, iZ + 1, y1, true);
   getCameraHeight(iX + 1, iZ + 1, y2, true);
   getCameraHeight(iX + 1, iZ, y3, true);

   float topX = LERP(y1, y2, Math::Clamp((position.x - x1) * mRcpCameraHeightTileScale, 0.0f, 1.0f));
   float botX = LERP(y0, y3, Math::Clamp((position.x - x1) * mRcpCameraHeightTileScale, 0.0f, 1.0f));
   retHeight = LERP(botX, topX, Math::Clamp((position.z - z1) * mRcpCameraHeightTileScale, 0.0f, 1.0f));

   return true;
}

//---------------------------------------
bool	BTerrainSimRep::getCameraHeight(BVector &position, bool clamp) const
{
   XMVECTOR tilePosition = __vctsxs(XMVectorMultiply(position, XMVectorReplicate(mRcpCameraHeightTileScale)), 0);
   const int x = tilePosition.u[0];
   const int z = tilePosition.u[2];
   return getCameraHeight(x, z, position.y, clamp);
}

//---------------------------------------
bool	BTerrainSimRep::getCameraHeight(const int heightGridX, const int heightGridY,float &retHeight,bool clamp) const
{
   if (!mpCameraHeights)
      return getHeight(heightGridX, heightGridY, retHeight, clamp);

   //we assume that [x,y] are in height grid space
   int tx=heightGridX;
   int ty=heightGridY;
   if(heightGridX > mNumXCameraHeightVerts)
   {
      if(clamp)   tx=mNumXCameraHeightVerts-1;
      else        return false;
   }
   if(heightGridX < 0)
   {
      if(clamp)   tx=0;
      else        return false;
   }

   if(heightGridY > mNumXCameraHeightVerts)
   {
      if(clamp)   ty=mNumXCameraHeightVerts-1;
      else        return false;
   }  
   if(heightGridY < 0)   
   {
      if(clamp)   ty=0;
      else        return false;
   }

   //convert from raster order to blocking
   int blockSize = 64;     // 8x8
   int blockPitch = 8;     
   int pitch = mNumXCameraHeightBlocks*64;

   int bx = (tx>>3) *blockSize;
   int by = (ty>>3) *pitch;

   int BlkIndx = (by+bx);

   int lx = (tx&0x07);
   int ly = (ty&0x07) * blockPitch;

   int lIndx = lx+ly;

   int indx = BlkIndx + lIndx;

   retHeight=XMConvertHalfToFloat(mpCameraHeights[indx]);

   return true;
}
//---------------------------------------
bool BTerrainSimRep::rayIntersectsCamera(const BVector origin, const BVector dir, BVector &intersectionPt, bool allowOriginUnderTerrain) const
{
   if (!mpCameraHeights)
      return rayIntersects(origin, dir, intersectionPt, allowOriginUnderTerrain);

   SCOPEDSAMPLE(BTerrainSimRep_rayIntersectsCamera)

   // X or Z major slope
   const float    dirX = dir.x;
   const float    dirZ = dir.z;
   const float    aDirX = abs(dirX);
   const float    aDirZ = abs(dirZ);
   const float    recADirX = 1.0f / aDirX;
   const float    recADirZ = 1.0f / aDirZ;
   const bool     straightY = ((aDirX + aDirZ) < cFloatCompareEpsilon) ? true : false;
   const bool     xMajor = (aDirX >= aDirZ) ? true : false;
   const bool     xPositive = (dirX >= 0.0f) ? true : false;
   const bool     zPositive = (dirZ >= 0.0f) ? true : false;
   const float    fMaxTiles = float(mNumXCameraHeightVerts-1);
   const float    maxWorld = fMaxTiles * mCameraHeightTileScale;
   // Save position
   BVector  position(origin);

   // Place the ray's origin within the map's area, if possible
   const bool     leftClip = (position.x < 0.0f) ? true : false;
   const bool     rightClip = (position.x >= maxWorld) ? true : false;
   const bool     topClip = (position.z >= maxWorld) ? true : false;
   const bool     bottomClip = (position.z < 0.0f) ? true : false;
   if ((leftClip && xPositive) || (rightClip && !xPositive) || (topClip && !zPositive) || (bottomClip && zPositive))
   {
      const bool xValid = (aDirX == 0.0f) ? false : true;
      const bool zValid = (aDirZ == 0.0f) ? false : true;

      float xRatio, zRatio;

      if (xValid)
      {
         if (leftClip || rightClip)
            xRatio = abs(position.x - ((xPositive) ? 0.0f : maxWorld)) * recADirX;
         else
            xRatio = 0.0f;
      }
      else
         xRatio = 0.0f;

      if (zValid)
      {
         if (topClip || bottomClip)
            zRatio = abs(position.z - ((zPositive) ? 0.0f : maxWorld)) * recADirZ;
         else
            zRatio = 0.0f;
      }
      else
         zRatio = 0.0f;

      if (xValid && (xRatio > zRatio))
         position = XMVectorMultiplyAdd(dir, XMVectorReplicate(xRatio), position);
      else if (zValid && (zRatio > xRatio))
         position = XMVectorMultiplyAdd(dir, XMVectorReplicate(zRatio), position);
   }
/*
   // Make sure we start over the terrain
   float height;
   if (!allowOriginUnderTerrain && (!getCameraHeightRaycast(position, height, false) || (position.y < height)))
      return false;
*/
   // Convert world to tile
   BVector  truncatedTile = XMVectorTruncate(XMVectorMultiply(position, XMVectorReplicate(mRcpCameraHeightTileScale)));
   // Save off tile coordinates (float and int versions)
   float x = Math::Min(truncatedTile.x, fMaxTiles-1);//maxWorld - 1.0f); //CLM [12.07.07] this should be in tile space.. No?
   float z = Math::Min(truncatedTile.z, fMaxTiles-1);//maxWorld - 1.0f); //CLM [12.07.07] this should be in tile space.. No?
   long  iX = long(x);
   long  iZ = long(z);
   // inc variables
   BVector  dirScale;

   if (xMajor)
      dirScale = XMVectorReplicate(recADirX);
   else
      dirScale = XMVectorReplicate(recADirZ);

   // Keep looping until we're off the map
   while ((iX >= 0) && (iX < fMaxTiles) && (iZ >= 0) && (iZ < fMaxTiles))
   {
      if (rayIntersectsCameraInternal(iX, iZ, x, z, origin, dir, intersectionPt))
         return true;

      // Exit if dir is straight up or down
      if (straightY)
         return false;

      // find next tile
      const float oldX = x;
      const float oldZ = z;
      const long  oldIX = iX;
      const long  oldIZ = iZ;
      const BVector  oldPosition(position);
      position = XMVectorMultiplyAdd(dirScale, dir, position);
      truncatedTile = XMVectorTruncate(XMVectorMultiply(position, XMVectorReplicate(mRcpCameraHeightTileScale)));
      x = truncatedTile.x;
      z = truncatedTile.z;
      iX = long(x);
      iZ = long(z);

      // Take care of holes caused by diagonal movement
      if ((iX != oldIX) && (iZ != oldIZ))
      {
         if (rayIntersectsCameraInternal(iX, oldIZ, x, oldZ, origin, dir, intersectionPt))
            return true;

         if (rayIntersectsCameraInternal(oldIX, iZ, oldX, z, origin, dir, intersectionPt))
            return true;
      }
   };

   return false;
}
//---------------------------------------
bool BTerrainSimRep::rayIntersectsCameraInternal(const long iX, const long iZ, const float x, const float z, const BVector origin, const BVector dir, BVector &intersectionPt) const
{
   BVector  bottomPoint;
   BVector  verts[3];
   float y0, y1, y2, y3;
   bool  hit = false;

   // Calculate tile coordinates in world space
   const float x1 = x * mCameraHeightTileScale;
   const float z1 = z * mCameraHeightTileScale;
   const float x2 = (x + 1.0f) * mCameraHeightTileScale;
   const float z2 = (z + 1.0f) * mCameraHeightTileScale;

   // test top triangle
   getCameraHeight(iX, iZ, y0, true);
   getCameraHeight(iX, iZ + 1, y1, true);
   getCameraHeight(iX + 1, iZ + 1, y2, true);
   verts[0].set(x1, y0, z1);
   verts[1].set(x1, y1, z2);
   verts[2].set(x2, y2, z2);
   if (raySegmentIntersectionTriangle(verts, origin, dir, false, intersectionPt, cFloatCompareEpsilon * 100.0f))
      hit = true;

   // test bottom triangle
   getCameraHeight(iX + 1, iZ, y3, true);
   verts[1] = verts[2];
   verts[2].set(x2, y3, z1);
   if (raySegmentIntersectionTriangle(verts, origin, dir, false, bottomPoint, cFloatCompareEpsilon * 100.0f))
   {
      // Ray could intersect both triangles. If it does, use the closest intersection.
      if (!hit || (origin.distanceSqr(bottomPoint) < origin.distanceSqr(intersectionPt)))
         intersectionPt = bottomPoint;
      hit = true;
   }

   return hit;
}
//---------------------------------------
bool  BTerrainSimRep::getFlightHeightRaycast(BVector origin, float &retHeight, bool clamp, bool reflect) const
{
   BVector position = origin;
   if (reflect)
   {
      // If the position passed in (origin) is off the map, reflect it to a corresponding position that is within map boundaries
      if (origin.x < 0.0f)
         position.x = -origin.x;
      if (origin.z < 0.0f)
         position.z = -origin.z;

      float maxXZ = gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale();
      if (origin.x > maxXZ)
         position.x = 2.0f * maxXZ - origin.x;
      if (origin.z > maxXZ)
         position.z = 2.0f * maxXZ - origin.z;
   }

   if (!mpFlightHeights)
      return getHeightRaycast(origin, retHeight, clamp);

   retHeight = 0.0f;

   // Convert world to tile
   BVector  truncatedTile = XMVectorTruncate(XMVectorMultiply(origin, XMVectorReplicate(mRcpFlightHeightTileScale)));
   origin.y = 0.0f;

   // Clamp
   if (clamp)
      truncatedTile = XMVectorClamp(truncatedTile, XMVectorZero(), XMVectorReplicate(float(mNumXFlightHeightVerts - 1)));
   // Save off tile coordinates (float and int versions)
   float x = truncatedTile.x;
   float z = truncatedTile.z;
   long  iX = long(x);
   long  iZ = long(z);

   if ((iX < 0) || (iX > mNumXFlightHeightVerts-1) || (iZ < 0) || (iZ > mNumXFlightHeightVerts-1))
      return false;

   // Calculate tile coordinates in world space
   const float x1 = x * mFlightHeightTileScale;
   const float z1 = z * mFlightHeightTileScale;

   float y0, y1, y2, y3;
   getFlightHeight(iX, iZ, y0, true);
   getFlightHeight(iX, iZ + 1, y1, true);
   getFlightHeight(iX + 1, iZ + 1, y2, true);
   getFlightHeight(iX + 1, iZ, y3, true);

   float topX = LERP(y1, y2, Math::Clamp((position.x - x1) * mRcpFlightHeightTileScale, 0.0f, 1.0f));
   float botX = LERP(y0, y3, Math::Clamp((position.x - x1) * mRcpFlightHeightTileScale, 0.0f, 1.0f));
   retHeight = LERP(botX, topX, Math::Clamp((position.z - z1) * mRcpFlightHeightTileScale, 0.0f, 1.0f));

   return true;
}

//---------------------------------------
bool	BTerrainSimRep::getFlightHeight(BVector &position, bool clamp) const
{
   XMVECTOR tilePosition = __vctsxs(XMVectorMultiply(position, XMVectorReplicate(mRcpFlightHeightTileScale)), 0);
   const int x = tilePosition.u[0];
   const int z = tilePosition.u[2];
   return getFlightHeight(x, z, position.y, clamp);
}

//---------------------------------------
bool	BTerrainSimRep::getFlightHeight(const int heightGridX, const int heightGridY,float &retHeight,bool clamp) const
{
   if (!mpFlightHeights)
      return getHeight(heightGridX, heightGridY, retHeight, clamp);

   //we assume that [x,y] are in height grid space
   int tx=heightGridX;
   int ty=heightGridY;
   if(heightGridX > mNumXFlightHeightVerts)
   {
      if(clamp)   tx=mNumXFlightHeightVerts-1;
      else        return false;
   }
   if(heightGridX < 0)
   {
      if(clamp)   tx=0;
      else        return false;
   }

   if(heightGridY > mNumXFlightHeightVerts)
   {
      if(clamp)   ty=mNumXFlightHeightVerts-1;
      else        return false;
   }  
   if(heightGridY < 0)   
   {
      if(clamp)   ty=0;
      else        return false;
   }

   //convert from raster order to blocking
   int blockSize = 64;     // 8x8
   int blockPitch = 8;     
   int pitch = mNumXFlightHeightBlocks*64;

   int bx = (tx>>3) *blockSize;
   int by = (ty>>3) *pitch;

   int BlkIndx = (by+bx);

   int lx = (tx&0x07);
   int ly = (ty&0x07) * blockPitch;

   int lIndx = lx+ly;

   int indx = BlkIndx + lIndx;

   retHeight=XMConvertHalfToFloat(mpFlightHeights[indx]);

   return true;
}
//---------------------------------------
bool BTerrainSimRep::rayIntersectsFlight(const BVector origin, const BVector dir, BVector &intersectionPt, bool allowOriginUnderTerrain) const
{
   if (!mpFlightHeights)
      return rayIntersects(origin, dir, intersectionPt, allowOriginUnderTerrain);

   SCOPEDSAMPLE(BTerrainSimRep_rayIntersectsFlight)

      // X or Z major slope
      const float    dirX = dir.x;
   const float    dirZ = dir.z;
   const float    aDirX = abs(dirX);
   const float    aDirZ = abs(dirZ);
   const float    recADirX = 1.0f / aDirX;
   const float    recADirZ = 1.0f / aDirZ;
   const bool     straightY = ((aDirX + aDirZ) < cFloatCompareEpsilon) ? true : false;
   const bool     xMajor = (aDirX >= aDirZ) ? true : false;
   const bool     xPositive = (dirX >= 0.0f) ? true : false;
   const bool     zPositive = (dirZ >= 0.0f) ? true : false;
   const float    fMaxTiles = float(mNumXFlightHeightVerts-1);
   const float    maxWorld = fMaxTiles * mFlightHeightTileScale;
   // Save position
   BVector  position(origin);

   // Place the ray's origin within the map's area, if possible
   const bool     leftClip = (position.x < 0.0f) ? true : false;
   const bool     rightClip = (position.x >= maxWorld) ? true : false;
   const bool     topClip = (position.z >= maxWorld) ? true : false;
   const bool     bottomClip = (position.z < 0.0f) ? true : false;
   if ((leftClip && xPositive) || (rightClip && !xPositive) || (topClip && !zPositive) || (bottomClip && zPositive))
   {
      const bool xValid = (aDirX == 0.0f) ? false : true;
      const bool zValid = (aDirZ == 0.0f) ? false : true;

      float xRatio, zRatio;

      if (xValid)
      {
         if (leftClip || rightClip)
            xRatio = abs(position.x - ((xPositive) ? 0.0f : maxWorld)) * recADirX;
         else
            xRatio = 0.0f;
      }
      else
         xRatio = 0.0f;

      if (zValid)
      {
         if (topClip || bottomClip)
            zRatio = abs(position.z - ((zPositive) ? 0.0f : maxWorld)) * recADirZ;
         else
            zRatio = 0.0f;
      }
      else
         zRatio = 0.0f;

      if (xValid && (xRatio > zRatio))
         position = XMVectorMultiplyAdd(dir, XMVectorReplicate(xRatio), position);
      else if (zValid && (zRatio > xRatio))
         position = XMVectorMultiplyAdd(dir, XMVectorReplicate(zRatio), position);
   }

   // Make sure we start over the terrain
   float height;
   if (!allowOriginUnderTerrain && (!getFlightHeightRaycast(position, height, false) || (position.y < height)))
      return false;

   // Convert world to tile
   BVector  truncatedTile = XMVectorTruncate(XMVectorMultiply(position, XMVectorReplicate(mRcpFlightHeightTileScale)));
   // Save off tile coordinates (float and int versions)
   float x = Math::Min(truncatedTile.x, fMaxTiles-1);//maxWorld - 1.0f); //CLM [12.07.07] this should be in tile space.. No?
   float z = Math::Min(truncatedTile.z, fMaxTiles-1);//maxWorld - 1.0f); //CLM [12.07.07] this should be in tile space.. No?
   long  iX = long(x);
   long  iZ = long(z);
   // inc variables
   BVector  dirScale;

   if (xMajor)
      dirScale = XMVectorReplicate(recADirX);
   else
      dirScale = XMVectorReplicate(recADirZ);

   // Keep looping until we're off the map
   while ((iX >= 0) && (iX < fMaxTiles) && (iZ >= 0) && (iZ < fMaxTiles))
   {
      if (rayIntersectsFlightInternal(iX, iZ, x, z, origin, dir, intersectionPt))
         return true;

      // Exit if dir is straight up or down
      if (straightY)
         return false;

      // find next tile
      const float oldX = x;
      const float oldZ = z;
      const long  oldIX = iX;
      const long  oldIZ = iZ;
      const BVector  oldPosition(position);
      position = XMVectorMultiplyAdd(dirScale, dir, position);
      truncatedTile = XMVectorTruncate(XMVectorMultiply(position, XMVectorReplicate(mRcpFlightHeightTileScale)));
      x = truncatedTile.x;
      z = truncatedTile.z;
      iX = long(x);
      iZ = long(z);

      // Take care of holes caused by diagonal movement
      if ((iX != oldIX) && (iZ != oldIZ))
      {
         if (rayIntersectsFlightInternal(iX, oldIZ, x, oldZ, origin, dir, intersectionPt))
            return true;

         if (rayIntersectsFlightInternal(oldIX, iZ, oldX, z, origin, dir, intersectionPt))
            return true;
      }
   };

   return false;
}
//---------------------------------------
bool BTerrainSimRep::rayIntersectsFlightInternal(const long iX, const long iZ, const float x, const float z, const BVector origin, const BVector dir, BVector &intersectionPt) const
{
   BVector  bottomPoint;
   BVector  verts[3];
   float y0, y1, y2, y3;
   bool  hit = false;

   // Calculate tile coordinates in world space
   const float x1 = x * mFlightHeightTileScale;
   const float z1 = z * mFlightHeightTileScale;
   const float x2 = (x + 1.0f) * mFlightHeightTileScale;
   const float z2 = (z + 1.0f) * mFlightHeightTileScale;

   // test top triangle
   getFlightHeight(iX, iZ, y0, true);
   getFlightHeight(iX, iZ + 1, y1, true);
   getFlightHeight(iX + 1, iZ + 1, y2, true);
   verts[0].set(x1, y0, z1);
   verts[1].set(x1, y1, z2);
   verts[2].set(x2, y2, z2);
   if (raySegmentIntersectionTriangle(verts, origin, dir, false, intersectionPt, cFloatCompareEpsilon * 100.0f))
      hit = true;

   // test bottom triangle
   getFlightHeight(iX + 1, iZ, y3, true);
   verts[1] = verts[2];
   verts[2].set(x2, y3, z1);
   if (raySegmentIntersectionTriangle(verts, origin, dir, false, bottomPoint, cFloatCompareEpsilon * 100.0f))
   {
      // Ray could intersect both triangles. If it does, use the closest intersection.
      if (!hit || (origin.distanceSqr(bottomPoint) < origin.distanceSqr(intersectionPt)))
         intersectionPt = bottomPoint;
      hit = true;
   }

   return hit;
}

//---------------------------------------
bool BTerrainSimRep::createPhysics( void )
{
   // Physics.
   releasePhysics();
   mpPhysicsTerrain = new BTerrainPhysicsHeightfield;
   if(!mpPhysicsTerrain)
      return(false);
   mpPhysicsTerrain->init();  

   return (true);

}

//---------------------------------------
void BTerrainSimRep::releasePhysics(void)
{
   if (mpPhysicsTerrain)
   {
      delete mpPhysicsTerrain;
      mpPhysicsTerrain = NULL;
   }
}

//---------------------------------------
bool BTerrainSimRep::rayIntersectsInternal(const long iX, const long iZ, const float x, const float z, const BVector origin, const BVector dir, BVector &intersectionPt) const
{
   BVector  bottomPoint;
   BVector  verts[3];
   float* pVerts0 = (float*)&verts[0];
   float* pVerts1 = (float*)&verts[1];
   float* pVerts2 = (float*)&verts[2];
   float y0, y1, y2, y3;
   bool  hit = false;

   // Calculate tile coordinates in world space
   const float x1 = x * mHeightTileScale;
   const float z1 = z * mHeightTileScale;
   const float x2 = (x + 1.0f) * mHeightTileScale;
   const float z2 = (z + 1.0f) * mHeightTileScale;

   // test top triangle
   getHeight(iX, iZ, y0, true);
   getHeight(iX, iZ + 1, y1, true);
   getHeight(iX + 1, iZ + 1, y2, true);
   pVerts0[0] = x1; pVerts0[1] = y0; pVerts0[2] = z1; // CLM [05.02.08] removed LHS -> verts[0].set(x1, y0, z1);
   pVerts1[0] = x1; pVerts1[1] = y1; pVerts1[2] = z2; // CLM [05.02.08] removed LHS -> verts[1].set(x1, y1, z2);
   pVerts2[0] = x2; pVerts2[1] = y2; pVerts2[2] = z2; // CLM [05.02.08] removed LHS -> verts[2].set(x2, y2, z2);
   if (raySegmentIntersectionTriangle(verts, origin, dir, false, intersectionPt, cFloatCompareEpsilon * 100.0f))
      hit = true;

   // test bottom triangle
   getHeight(iX + 1, iZ, y3, true);
   pVerts1[0] = x2; pVerts1[1] = y2; pVerts1[2] = z2; // CLM [05.02.08 removed LHS -> verts[1] = verts[2];
   pVerts2[0] = x2; pVerts2[1] = y3; pVerts2[2] = z1; // CLM [05.02.08] removed LHS ->verts[2].set(x2, y3, z1);
   if (raySegmentIntersectionTriangle(verts, origin, dir, false, bottomPoint, cFloatCompareEpsilon * 100.0f))
   {
      // Ray could intersect both triangles. If it does, use the closest intersection.
      if (!hit || (origin.distanceSqr(bottomPoint) < origin.distanceSqr(intersectionPt)))
         intersectionPt = bottomPoint;
      hit = true;
   }

   return hit;
}
//---------------------------------------
void  BTerrainSimRep::addDebugLineOverTerrain(BVector point, BVector point2, DWORD color1, DWORD color2, float terrainOffset, int category, float timeout)
{
   /*
   BVector p1T = point;
   BVector p2T = point2;
   getHeight(p1T, true);
   getHeight(p2T, true);

   p1T.y += terrainOffset;
   p2T.y += terrainOffset;

   gpDebugPrimitives->addDebugLine(p1T, p2T, color1, color2, category, timeout);
   */
   
   // jce [9/10/2008] -- replacing with a version that subdivides to keep the line actually over the terrain.  Old version
   // is above if this turns out to cause too many segments and someone needs to revert it.
   
   // Get vector from point 1 to 2.
   BVector v = point2-point;
   
   // Length of line.
   float len = v.length();
   
   // Bail if no length.
   if(len < cFloatCompareEpsilon)
      return;

   // Normalize.
   v /= len;
   
   // First point.
   BVector p1 = point;
   getHeight(p1, true);
   p1.y += terrainOffset;

   // Walk the line in subdivisions
   const float cSubDivLength = 4.0f;
   for(float t=cSubDivLength; t<len; t += cSubDivLength)
   {
      // Next point.
      BVector p2 = point + t*v;
      getHeight(p2, true);
      p2.y += terrainOffset;
      
      // Add the line.
      gpDebugPrimitives->addDebugLine(p1, p2, color1, color2, category, timeout);
      
      // New first point is old second point.
      p1 = p2;
   }
   
   // Final (probably partial length) line.
   BVector p2 = point2;
   getHeight(p2, true);
   p2.y += terrainOffset;
   gpDebugPrimitives->addDebugLine(p1, p2, color1, color2, category, timeout);
}


//---------------------------------------
void  BTerrainSimRep::addDebugThickLineOverTerrain(BVector point, BVector point2, float thickness, DWORD color1, DWORD color2, float terrainOffset, int category, float timeout)
{
   BVector p1T = point;
   BVector p2T = point2;
   getHeight(p1T, true);
   getHeight(p2T, true);

   p1T.y += terrainOffset;
   p2T.y += terrainOffset;

   gpDebugPrimitives->addDebugThickLine(p1T, p2T, thickness, color1, color2, category, timeout);
}

//---------------------------------------
void BTerrainSimRep::addDebugSquareOverTerrain(BVector p1, BVector p2, BVector p3, BVector p4, DWORD color, float terrainOffset, int category, float timeout)
{
   addDebugLineOverTerrain(p1, p2, color, color, terrainOffset, category, timeout);
   addDebugLineOverTerrain(p2, p3, color, color, terrainOffset, category, timeout);
   addDebugLineOverTerrain(p3, p4, color, color, terrainOffset, category, timeout);
   addDebugLineOverTerrain(p4, p1, color, color, terrainOffset, category, timeout);
}

//---------------------------------------
void BTerrainSimRep::addDebugSquareOverTerrain(BVector center, BVector forward, DWORD color, float terrainOffset, float scale, int category, float timeout)
{
   forward.normalize();
   BVector right;
   right.assignCrossProduct(cYAxisVector, forward);

   BVector sf = scale*forward;
   BVector sr = scale*right;
   BVector p1 = center + sf - sr;
   BVector p2 = center + sf + sr;
   BVector p3 = center - sf + sr;
   BVector p4 = center - sf - sr;

   addDebugLineOverTerrain(p1, p2, color, color, terrainOffset, category, timeout);
   addDebugLineOverTerrain(p2, p3, color, color, terrainOffset, category, timeout);
   addDebugLineOverTerrain(p3, p4, color, color, terrainOffset, category, timeout);
   addDebugLineOverTerrain(p4, p1, color, color, terrainOffset, category, timeout);
}

//---------------------------------------
void BTerrainSimRep::addDebugThickSquareOverTerrain(BVector p1, BVector p2, BVector p3, BVector p4, float thickness, DWORD color, float terrainOffset, int category, float timeout)
{
   BVector p1p2 = p2 - p1;
   BVector p3p4 = p4 - p3;
   p1p2.normalize();
   p3p4.normalize();

   addDebugThickLineOverTerrain(p1-(thickness*p1p2), p2+(thickness*p1p2), thickness, color, color, terrainOffset, category, timeout);
   addDebugThickLineOverTerrain(p2, p3, thickness, color, color, terrainOffset, category, timeout);
   addDebugThickLineOverTerrain(p3-(thickness*p3p4), p4+(thickness*p3p4), thickness, color, color, terrainOffset, category, timeout);
   addDebugThickLineOverTerrain(p4, p1, thickness, color, color, terrainOffset, category, timeout);
}


//---------------------------------------
void BTerrainSimRep::addDebugThickSquareOverTerrain(BVector center, BVector forward, float thickness, DWORD color, float terrainOffset, float scale, int category, float timeout)
{
   forward.normalize();
   BVector right;
   right.assignCrossProduct(cYAxisVector, forward);

   BVector sf = scale*forward;
   BVector sr = scale*right;
   BVector p1 = center + sf - sr;
   BVector p2 = center + sf + sr;
   BVector p3 = center - sf + sr;
   BVector p4 = center - sf - sr;
   BVector p1p2 = p2 - p1;
   BVector p3p4 = p4 - p3;
   p1p2.normalize();
   p3p4.normalize();

   addDebugThickLineOverTerrain(p1-(thickness*p1p2), p2+(thickness*p1p2), thickness, color, color, terrainOffset, category, timeout);
   addDebugThickLineOverTerrain(p2, p3, thickness, color, color, terrainOffset, category, timeout);
   addDebugThickLineOverTerrain(p3-(thickness*p3p4), p4+(thickness*p3p4), thickness, color, color, terrainOffset, category, timeout);
   addDebugThickLineOverTerrain(p4, p1, thickness, color, color, terrainOffset, category, timeout);
}


//---------------------------------------
void BTerrainSimRep::addDebugTriangleOverTerrain(BVector center, BVector forward, DWORD color, float terrainOffset, float scale, int category, float timeout)
{
   forward.normalize();
   BVector right;
   right.assignCrossProduct(cYAxisVector, forward);

   BVector sf = scale*forward;
   BVector sr = scale*right;
   BVector p1 = center + sf;
   BVector p2 = center - sf + sr;
   BVector p3 = center - sf + sr;

   addDebugLineOverTerrain(p1, p2, color, color, terrainOffset, category, timeout);
   addDebugLineOverTerrain(p2, p3, color, color, terrainOffset, category, timeout);
   addDebugLineOverTerrain(p3, p1, color, color, terrainOffset, category, timeout);
}



//---------------------------------------
void BTerrainSimRep::addDebugThickTriangleOverTerrain(BVector center, BVector forward, float thickness, DWORD color, float terrainOffset, float scale, int category, float timeout)
{
   forward.normalize();
   BVector right;
   right.assignCrossProduct(cYAxisVector, forward);

   BVector sf = scale*forward;
   BVector sr = scale*right;
   BVector p1 = center + sf;
   BVector p2 = center - sf + sr;
   BVector p3 = center - sf - sr;

   addDebugThickLineOverTerrain(p1, p2, thickness, color, color, terrainOffset, category, timeout);
   addDebugThickLineOverTerrain(p2, p3, thickness, color, color, terrainOffset, category, timeout);
   addDebugThickLineOverTerrain(p3, p1, thickness, color, color, terrainOffset, category, timeout);
}


//---------------------------------------
void BTerrainSimRep::addDebugCrossOverTerrain(BVector center, BVector forward, DWORD color, float terrainOffset, float scale, int category, float timeout)
{
   forward.normalize();
   BVector right;
   right.assignCrossProduct(cYAxisVector, forward);

   BVector sf = scale*forward;
   BVector sr = scale*right;
   BVector p1 = center + sf - sr;
   BVector p2 = center + sf + sr;
   BVector p3 = center - sf + sr;
   BVector p4 = center - sf - sr;

   addDebugLineOverTerrain(p1, p3, color, color, terrainOffset, category, timeout);
   addDebugLineOverTerrain(p2, p4, color, color, terrainOffset, category, timeout);
}


//---------------------------------------
void BTerrainSimRep::addDebugThickCrossOverTerrain(BVector center, BVector forward, float thickness, DWORD color, float terrainOffset, float scale, int category, float timeout)
{
   forward.normalize();
   BVector right;
   right.assignCrossProduct(cYAxisVector, forward);

   BVector sf = scale*forward;
   BVector sr = scale*right;
   BVector p1 = center + sf - sr;
   BVector p2 = center + sf + sr;
   BVector p3 = center - sf + sr;
   BVector p4 = center - sf - sr;

   addDebugThickLineOverTerrain(p1, p3, thickness, color, color, terrainOffset, category, timeout);
   addDebugThickLineOverTerrain(p2, p4, thickness, color, color, terrainOffset, category, timeout);
}


//---------------------------------------
void BTerrainSimRep::addDebugArrowOverTerrain(BVector center, BVector forward, DWORD color, float terrainOffset, float scale, int category, float timeout)
{
   forward.normalize();
   BVector right;
   right.assignCrossProduct(cYAxisVector, forward);

   BVector sf = scale*forward;
   BVector p1 = center + sf;
   BVector p2 = center - sf;
   addDebugLineOverTerrain(p1, p2, color, color, terrainOffset, category, timeout);

   BVector triCenter = center + 0.5f*sf;
   float triScale = 0.5f*scale;
   addDebugTriangleOverTerrain(triCenter, forward, color, terrainOffset, triScale, category, timeout);
}

//---------------------------------------
void BTerrainSimRep::addDebugThickArrowOverTerrain(BVector center, BVector forward, float thickness, DWORD color, float terrainOffset, float scale, int category, float timeout)
{
   forward.normalize();
   BVector right;
   right.assignCrossProduct(cYAxisVector, forward);

   BVector sf = scale*forward;
   BVector p1 = center + sf;
   BVector p2 = center - sf;
   addDebugThickLineOverTerrain(p1, p2, thickness, color, color, terrainOffset, category, timeout);

   BVector triCenter = center + 0.5f*sf;
   float triScale = 0.5f*scale;
   addDebugThickTriangleOverTerrain(triCenter, forward, thickness, color, terrainOffset, triScale, category, timeout);
}


//---------------------------------------
void BTerrainSimRep::addDebugCircleOverTerrain(BVector center, float radius, DWORD color, float terrainOffset, int category, float timeout, float radians, float angularOffset)
{   
   float radiansPerSegment = radians / (cNumCirclePoints - 1);
   BVector circleSelectPoints[cNumCirclePoints];
   for (long segment = 0; segment < cNumCirclePoints; segment++)
   {
      float sinAngle;
      float cosAngle;
      XMScalarSinCos(&sinAngle, &cosAngle, angularOffset);
      BVector v(sinAngle * radius + center.x, 0.0f, cosAngle * radius + center.z);
      getHeight(v, true);
      v.y += terrainOffset;
      circleSelectPoints[segment] = v;
      angularOffset += radiansPerSegment;
   }

   long p1 = 0;
   long p2 = 1;
   for (; p2 < cNumCirclePoints; p1++, p2++)
      gpDebugPrimitives->addDebugLine(circleSelectPoints[p1], circleSelectPoints[p2], color, color, category, timeout);
}

//---------------------------------------
void BTerrainSimRep::addDebugThickCircleOverTerrain(BVector center, float radius, float thickness, DWORD color, float terrainOffset, int category, float timeout, float radians, float angularOffset)
{   
   float radiansPerSegment = radians / (cNumThickCirclePoints - 1);
   BVector circleSelectPoints[cNumThickCirclePoints];
   for (long segment = 0; segment < cNumThickCirclePoints; segment++)
   {
      float sinAngle;
      float cosAngle;
      XMScalarSinCos(&sinAngle, &cosAngle, angularOffset);
      BVector v(sinAngle * radius + center.x, 0.0f, cosAngle * radius + center.z);
      getHeight(v, true);
      v.y += terrainOffset;
      circleSelectPoints[segment] = v;
      angularOffset += radiansPerSegment;
   }

   long p1 = 0;
   long p2 = 1;
   for (; p2 < cNumThickCirclePoints; p1++, p2++)
      gpDebugPrimitives->addDebugThickLine(circleSelectPoints[p1], circleSelectPoints[p2], thickness, color, color, category, timeout);
}

//---------------------------------------
void  BTerrainSimRep::addDebugPointOverTerrain(BVector center, float radius, DWORD color, float terrainOffset, int category, float timeout)
{
   BVector centerPoint=center;
   getHeight(centerPoint, true);
   centerPoint.y+=terrainOffset;

   //X
   BVector point1=centerPoint-(cXAxisVector*radius/2.0f);
   BVector point2=centerPoint+(cXAxisVector*radius/2.0f);
   gpDebugPrimitives->addDebugLine(point1, point2, color, color, category, timeout);
   
   //Z
   point1=centerPoint-(cZAxisVector*radius/2.0f);
   point2=centerPoint+(cZAxisVector*radius/2.0f);
   gpDebugPrimitives->addDebugLine(point1, point2, color, color, category, timeout);
   
   //Y
   point1=centerPoint-(cYAxisVector*radius/2.0f);
   point2=centerPoint+(cYAxisVector*radius/2.0f);
   gpDebugPrimitives->addDebugLine(point1, point2, color, color, category, timeout);
}
//---------------------------------------
void  BTerrainSimRep::addDebugThickPointOverTerrain(BVector center, float radius, DWORD color, float terrainOffset, int category, float timeout)
{
   BVector centerPoint=center;
   getHeight(centerPoint, true);
   centerPoint.y+=terrainOffset;

   //X
   BVector point1=centerPoint-(cXAxisVector*radius/2.0f);
   BVector point2=centerPoint+(cXAxisVector*radius/2.0f);
   gpDebugPrimitives->addDebugThickLine(point1, point2, color, color, category, timeout);
   
   //Z
   point1=centerPoint-(cZAxisVector*radius/2.0f);
   point2=centerPoint+(cZAxisVector*radius/2.0f);
   gpDebugPrimitives->addDebugThickLine(point1, point2, color, color, category, timeout);
   
   //Y
   point1=centerPoint-(cYAxisVector*radius/2.0f);
   point2=centerPoint+(cYAxisVector*radius/2.0f);
   gpDebugPrimitives->addDebugThickLine(point1, point2, color, color, category, timeout);
}

//---------------------------------------
void BTerrainSimRep::addDebugRoundedBoxOverTerrain(BOPQuadHull* obsHull, float range, DWORD color, float terrainOffset, int category, float timeout)
{
   if (!obsHull)
      return;

   // Hull points
   BVector pt1(obsHull->mX1, 0.0f, obsHull->mZ1);
   BVector pt2(obsHull->mX2, 0.0f, obsHull->mZ2);
   BVector pt3(obsHull->mX3, 0.0f, obsHull->mZ3);
   BVector pt4(obsHull->mX4, 0.0f, obsHull->mZ4);

   // Line segments to offset from box
   BVector dir12 = pt2 - pt1;
   dir12.normalize();
   dir12 *= range;
   BVector dir23(dir12.z, 0.0f, -dir12.x);

   // Line segments
   gTerrainSimRep.addDebugLineOverTerrain(pt1 - dir23, pt2 - dir23, color, color, terrainOffset);
   gTerrainSimRep.addDebugLineOverTerrain(pt2 + dir12, pt3 + dir12, color, color, terrainOffset);
   gTerrainSimRep.addDebugLineOverTerrain(pt3 + dir23, pt4 + dir23, color, color, terrainOffset);
   gTerrainSimRep.addDebugLineOverTerrain(pt4 - dir12, pt1 - dir12, color, color, terrainOffset);

   // Corner arcs
   float angle = dir12.getAngleAroundY();
   gTerrainSimRep.addDebugCircleOverTerrain(pt1, range, color, terrainOffset, category, timeout, cPiOver2, angle + cPi);
   gTerrainSimRep.addDebugCircleOverTerrain(pt2, range, color, terrainOffset, category, timeout, cPiOver2, angle - cPiOver2);
   gTerrainSimRep.addDebugCircleOverTerrain(pt3, range, color, terrainOffset, category, timeout, cPiOver2, angle);
   gTerrainSimRep.addDebugCircleOverTerrain(pt4, range, color, terrainOffset, category, timeout, cPiOver2, angle + cPiOver2);
}

//---------------------------------------
void  BTerrainSimRep::addDebugLineOverCameraRep(BVector point, BVector point2, DWORD color1, DWORD color2, float terrainOffset, int category, float timeout)
{
   BVector p1T = point;
   BVector p2T = point2;
   getCameraHeightRaycast(p1T, p1T.y, true);
   getCameraHeightRaycast(p2T, p2T.y, true);

   p1T.y += terrainOffset;
   p2T.y += terrainOffset;

   gpDebugPrimitives->addDebugLine(p1T, p2T, color1, color2, category, timeout);
}

//---------------------------------------
BYTE BTerrainSimRep::getTileType(BVector position) const
{
   XMVECTOR tilePosition = __vctsxs(XMVectorMultiply(position, XMVectorReplicate(mRecDataTileScale)), 0);
   const int x = tilePosition.u[0];
   const int z = tilePosition.u[2];

   return getTileType(x, z);
}

//---------------------------------------
BYTE BTerrainSimRep::getTileType(int x, int z) const
{
   x = Math::Clamp(x, 0, mNumXDataTiles - 1);
   z = Math::Clamp(z, 0, mNumXDataTiles - 1);

   return mpTileType[z * mNumXDataTiles + x];
}

//---------------------------------------
void BTerrainSimRep::debugRender()
{
   const float maxDistance = 100.0f;
   const float maxDistanceSqr = maxDistance * maxDistance;
   BVector cameraPos = gRenderDraw.getMainSceneMatrixTracker().getWorldCamPos();

   for (int vx = 0; vx < mNumXHeightVerts; vx++)
      for (int vz = 0; vz < mNumXHeightVerts; vz++)
      {
         BVector tileScale = XMVectorReplicate(mHeightTileScale);
         int vx2 = vx + 1;
         int vz2 = vz + 1;
         BVector p1 = XMVectorMultiply(BVector(vx, 0.0f, vz), tileScale);
         BVector p2 = XMVectorMultiply(BVector(vx2, 0.0f, vz), tileScale);
         BVector p3 = XMVectorMultiply(BVector(vx2, 0.0f, vz2), tileScale);
         BVector p4 = XMVectorMultiply(BVector(vx, 0.0f, vz2), tileScale);
         getHeight(vx, vz, p1.y, false);
         getHeight(vx2, vz, p2.y, false);
         getHeight(vx2, vz2, p3.y, false);
         getHeight(vx, vz2, p4.y, false);
         p1.y += 0.1f;
         p2.y += 0.1f;
         p3.y += 0.1f;
         p4.y += 0.1f;
         bool rightOK = (vx2 < mNumXHeightVerts) ? true : false;
         bool topOK = (vz2 < mNumXHeightVerts) ? true : false;

         if (cameraPos.xzDistanceSqr(p1) > maxDistanceSqr)
            continue;

         if(!gRender.getViewParams().isPointOnScreen(p1) && !gRender.getViewParams().isPointOnScreen(p3))
            continue;

         if (rightOK && (vz == 0))
            gpDebugPrimitives->addDebugLine(p1, p2, 0.1f, cDWORDBlue, cDWORDBlue);
         if (topOK && (vx == 0))
            gpDebugPrimitives->addDebugLine(p1, p4, 0.1f, cDWORDBlue, cDWORDBlue);
         if (rightOK && topOK)
         {
            gpDebugPrimitives->addDebugLine(p2, p3, 0.1f, cDWORDBlue, cDWORDBlue);
            gpDebugPrimitives->addDebugLine(p4, p3, 0.1f, cDWORDBlue, cDWORDBlue);
         }
      }
}

//---------------------------------------
void BTerrainSimRep::debugRenderCameraHeights()
{
   //const float maxDistance = 100.0f;
  // const float maxDistanceSqr = maxDistance * maxDistance;
   BVector cameraPos = gRenderDraw.getMainSceneMatrixTracker().getWorldCamPos();

   for (int vx = 0; vx < mNumXCameraHeightVerts; vx++)
      for (int vz = 0; vz < mNumXCameraHeightVerts; vz++)
      {
         BVector tileScale = XMVectorReplicate(mCameraHeightTileScale);
         int vx2 = vx + 1;
         int vz2 = vz + 1;
         BVector p1 = XMVectorMultiply(BVector(vx, 0.0f, vz), tileScale);
         BVector p2 = XMVectorMultiply(BVector(vx2, 0.0f, vz), tileScale);
         BVector p3 = XMVectorMultiply(BVector(vx2, 0.0f, vz2), tileScale);
         BVector p4 = XMVectorMultiply(BVector(vx, 0.0f, vz2), tileScale);
         getCameraHeight(vx, vz, p1.y, false);
         getCameraHeight(vx2, vz, p2.y, false);
         getCameraHeight(vx2, vz2, p3.y, false);
         getCameraHeight(vx, vz2, p4.y, false);
         p1.y += 0.1f;
         p2.y += 0.1f;
         p3.y += 0.1f;
         p4.y += 0.1f;
         bool rightOK = (vx2 < mNumXCameraHeightVerts) ? true : false;
         bool topOK = (vz2 < mNumXCameraHeightVerts) ? true : false;

       //  if (cameraPos.xzDistanceSqr(p1) > maxDistanceSqr)
         //   continue;

       //  if(!gRender.getViewParams().isPointOnScreen(p1) && !gRender.getViewParams().isPointOnScreen(p3))
       //     continue;

         if (rightOK && (vz == 0))
            gpDebugPrimitives->addDebugLine(p1, p2, 0.1f, cDWORDBlue, cDWORDBlue);
         if (topOK && (vx == 0))
            gpDebugPrimitives->addDebugLine(p1, p4, 0.1f, cDWORDBlue, cDWORDBlue);
         if (rightOK && topOK)
         {
            gpDebugPrimitives->addDebugLine(p2, p3, 0.1f, cDWORDBlue, cDWORDBlue);
            gpDebugPrimitives->addDebugLine(p4, p3, 0.1f, cDWORDBlue, cDWORDBlue);
            gpDebugPrimitives->addDebugLine(p1, p3, 0.1f, cDWORDBlue, cDWORDBlue);
         }


         
         // Render raycast grid also
         //

         if((vx < mNumXCameraHeightVerts - 1) && (vz < mNumXCameraHeightVerts - 1))
         {
            // Tile
            p1.y = 0.0f;
            long numTessPerTile = 5;
            float step = (mCameraHeightTileScale / (float)numTessPerTile);

            BVector pos1(0.0f, 0.0f, 0.0f);
            BVector pos2(0.0f, 0.0f, 0.0f);

            for (int i = 0; i < numTessPerTile; i++)
            {
               for (int j = 0; j < numTessPerTile; j++)
               {
                  // Height
                  pos1.set(i * step, 0.0f, j * step);
                  pos1 += p1;

                  getCameraHeightRaycast(pos1, pos1.y, true);

                  pos2 = pos1;
                  pos2.y -= 1.0f;

                  gpDebugPrimitives->addDebugLine(pos1, pos2, 0.1f, cDWORDBlue, cDWORDBlue);
               }
            }
         }
      }
}

//---------------------------------------
void BTerrainSimRep::debugRenderFlightHeights()
{
   //const float maxDistance = 100.0f;
   // const float maxDistanceSqr = maxDistance * maxDistance;
   BVector cameraPos = gRenderDraw.getMainSceneMatrixTracker().getWorldCamPos();

   for (int vx = 0; vx < mNumXFlightHeightVerts; vx++)
      for (int vz = 0; vz < mNumXFlightHeightVerts; vz++)
      {
         BVector tileScale = XMVectorReplicate(mFlightHeightTileScale);
         int vx2 = vx + 1;
         int vz2 = vz + 1;
         BVector p1 = XMVectorMultiply(BVector(vx, 0.0f, vz), tileScale);
         BVector p2 = XMVectorMultiply(BVector(vx2, 0.0f, vz), tileScale);
         BVector p3 = XMVectorMultiply(BVector(vx2, 0.0f, vz2), tileScale);
         BVector p4 = XMVectorMultiply(BVector(vx, 0.0f, vz2), tileScale);
         getFlightHeight(vx, vz, p1.y, false);
         getFlightHeight(vx2, vz, p2.y, false);
         getFlightHeight(vx2, vz2, p3.y, false);
         getFlightHeight(vx, vz2, p4.y, false);
         p1.y += 0.1f;
         p2.y += 0.1f;
         p3.y += 0.1f;
         p4.y += 0.1f;
         bool rightOK = (vx2 < mNumXFlightHeightVerts) ? true : false;
         bool topOK = (vz2 < mNumXFlightHeightVerts) ? true : false;

         //  if (cameraPos.xzDistanceSqr(p1) > maxDistanceSqr)
         //   continue;

         //  if(!gRender.getViewParams().isPointOnScreen(p1) && !gRender.getViewParams().isPointOnScreen(p3))
         //     continue;

         if (rightOK && (vz == 0))
            gpDebugPrimitives->addDebugLine(p1, p2, 0.1f, cDWORDBlue, cDWORDBlue);
         if (topOK && (vx == 0))
            gpDebugPrimitives->addDebugLine(p1, p4, 0.1f, cDWORDBlue, cDWORDBlue);
         if (rightOK && topOK)
         {
            gpDebugPrimitives->addDebugLine(p2, p3, 0.1f, cDWORDBlue, cDWORDBlue);
            gpDebugPrimitives->addDebugLine(p4, p3, 0.1f, cDWORDBlue, cDWORDBlue);
            gpDebugPrimitives->addDebugLine(p1, p3, 0.1f, cDWORDBlue, cDWORDBlue);
         }
      }
}

//==============================================================================
// BTerrainSimRep::loadECFXTH Enums
//==============================================================================
enum eXSDChunkID
{
   cXSD_Version         = 0x0004,

   cXSD_XSDHeader       = 0x1111,
   cXSD_SimHeights      = 0x2222,
   cXSD_Obstructions    = 0x4444,
   cXSD_TileTypes       = 0x8888,
   cXSD_CamHeights      = 0xAAAA,
   cXSD_FlightHeights   = 0xABBB,
   cXSD_Buildable      = 0xCCCC,
   cXSD_FloodPassable      = 0xDDDD,
   cXSD_ScarabPassable      = 0xEEEE,
};

//==============================================================================
// BTerrainSimRep::loadECFXSD
//==============================================================================
bool BTerrainSimRep::loadECFXSDInternal(int dirID,const char *filenameNoExtention)
{
//   char buf[256];
   BString filenameXSD(filenameNoExtention);
   filenameXSD += ".xsd";

   //dirID = cDirProduction;
   gConsoleOutput.resource("BTerrainSimRep::loadECFXSDInternal: %s", filenameXSD.getPtr());

   BFileSystemStream tfile;
   if (!tfile.open(dirID, filenameXSD, cSFReadable | cSFSeekable | cSFEnableBuffering | cSFDiscardOnClose))
   {
      gConsoleOutput.error("BTerrainSimRep::loadECFXSDInternal : Unable to open file %s\n", filenameXSD.getPtr());
      return false;
   }



   BECFFileStream ecfReader;
   if(!ecfReader.open(&tfile)) //file checking and version ID done during constructor
   {
      gConsoleOutput.error("BTerrainSimRep::loadECFXSDInternal : ECFHeader or Checksum invalid  %s\n", filenameXSD.getPtr());
      tfile.close();
      return false;
   }

   BECFHeader ecfHeader = ecfReader.getHeader();
   int numECFChunks = ecfHeader.getNumChunks();

   //find our PTHChunk header
   int headerIndex = ecfReader.findChunkByID(cXSD_XSDHeader);
   if(headerIndex==-1)
   { 
      gConsoleOutput.error("BTerrainSimRep:loadECFXSDInternal : Could not find XSD Chunk Header\n");
      tfile.close();
      return false;
   }
   ecfReader.seekToChunk(headerIndex);


   //check our header data
   int version =0;
   ecfReader.getStream()->readObj(version);
   if(version !=cXSD_Version)
   {
      gConsoleOutput.error("BTerrainSimRep:loadECFXSDInternal : XSD Wrong Version\n");
      tfile.close();
      return false;
   }

   //read in some groovy stuff

   //Sim Data Rep
   ecfReader.getStream()->readObj(mNumXDataTiles);  //this is for obstruction etc
   ecfReader.getStream()->readObj(mDataTileScale);
   ecfReader.getStream()->readObj(mVisualToSimScalar);
   mRecDataTileScale = 1.0f / mDataTileScale;

   //Sim Heights Rep
   int numCacheFriendlyXVerts = 0;
   ecfReader.getStream()->readObj(mNumXHeightVerts);
   ecfReader.getStream()->readObj(numCacheFriendlyXVerts);  //CLM our tiles aren't cache friendly...
   ecfReader.getStream()->readObj(mHeightTileScale);
   ecfReader.getStream()->readObj(mDataTileToHeightVertScalar);
   mNumXHeightBlocks = numCacheFriendlyXVerts / 8;
   mRcpHeightTileScale = 1.0f/mHeightTileScale;

   //-- cache values
   mRcpHeightTileScaleV = XMVectorReplicate(mRcpHeightTileScale);
   mBlockPitch = 8;
   mBlockSize  = 64;
   mHeightBlocksPitch = mNumXHeightBlocks * 64;

   //we're good. start walking through chunks and doing stuff...
   for(int i=0;i<numECFChunks;i++)
   {
      const BECFChunkHeader& cHeader = ecfReader.getChunkHeader(i);
      const uint64 id = cHeader.getID();
      switch(id)
      {
      case cXSD_SimHeights:
         {
            ecfReader.seekToChunk(i);

            if(mpHeights)   delete [] mpHeights;
            mpHeights = new HALF[numCacheFriendlyXVerts*numCacheFriendlyXVerts];
            ecfReader.getStream()->readBytes(mpHeights,numCacheFriendlyXVerts*numCacheFriendlyXVerts*sizeof(HALF) );
            
            break;
         }
      case cXSD_Obstructions:
         {
            ecfReader.seekToChunk(i);

            if(mpObstructionMap)   delete [] mpObstructionMap;
            const int obstructionFlagsSize = mNumXDataTiles * mNumXDataTiles * sizeof(BYTE);
            mpObstructionMap = new BYTE[obstructionFlagsSize];
            ecfReader.getStream()->readBytes(mpObstructionMap,obstructionFlagsSize );

            break;
         }
      case cXSD_Buildable:
         {
            ecfReader.seekToChunk(i);

            if(mpBuildableMap)   delete [] mpBuildableMap;
            const int size = mNumXDataTiles * mNumXDataTiles * sizeof(BYTE);
            mpBuildableMap = new BYTE[size];
            ecfReader.getStream()->readBytes(mpBuildableMap,size );

            break;
         }
     
      case cXSD_TileTypes:
         {
            ecfReader.seekToChunk(i);

            //tile types
            if(mpTileType)   delete [] mpTileType;
            const int tileTypeArraySize = mNumXDataTiles * mNumXDataTiles * sizeof(BYTE);
            mpTileType = new BYTE[tileTypeArraySize];
            ecfReader.getStream()->readBytes(mpTileType, tileTypeArraySize);

            break;
         }
      case cXSD_CamHeights:
         {
            ecfReader.seekToChunk(i);
            int numCacheFriendlyXCameraVerts=0;
            ecfReader.getStream()->readObj(mNumXCameraHeightVerts);
            ecfReader.getStream()->readObj(numCacheFriendlyXCameraVerts);
            ecfReader.getStream()->readObj(mCameraHeightTileScale);
            mNumXCameraHeightBlocks = numCacheFriendlyXCameraVerts / 8;
            mRcpCameraHeightTileScale = 1.0f/mCameraHeightTileScale;

            //tile types
            if(mpCameraHeights)   delete [] mpCameraHeights;
            mpCameraHeights = new HALF[numCacheFriendlyXCameraVerts*numCacheFriendlyXCameraVerts];
            ecfReader.getStream()->readBytes(mpCameraHeights,numCacheFriendlyXCameraVerts*numCacheFriendlyXCameraVerts*sizeof(HALF) );

            break;
         }
      case cXSD_FlightHeights:
         {
            ecfReader.seekToChunk(i);
            int numCacheFriendlyXFlightVerts=0;
            ecfReader.getStream()->readObj(mNumXFlightHeightVerts);
            ecfReader.getStream()->readObj(numCacheFriendlyXFlightVerts);
            ecfReader.getStream()->readObj(mFlightHeightTileScale);
            mNumXFlightHeightBlocks = numCacheFriendlyXFlightVerts / 8;
            mRcpFlightHeightTileScale = 1.0f/mFlightHeightTileScale;

            //tile types
            if(mpFlightHeights)   delete [] mpFlightHeights;
            mpFlightHeights = new HALF[numCacheFriendlyXFlightVerts*numCacheFriendlyXFlightVerts];
            ecfReader.getStream()->readBytes(mpFlightHeights,numCacheFriendlyXFlightVerts*numCacheFriendlyXFlightVerts*sizeof(HALF) );

            break;
         }
         


      }
   }


   tfile.close();
   mLoaded=true;
   return true;
}


//==============================================================================
// BTerrainSimRep::clampWorld
// MS 4/29/2008: clamps a world position to the terrain's XZ boundaries
//==============================================================================
void BTerrainSimRep::clampWorld(BVector &position)
{
   float oldY = position.y;
   position = XMVectorClamp(position, XMVectorZero(), XMVectorReplicate(float((mNumXHeightVerts - 1) * mHeightTileScale)));
   position.y = oldY;
}

//==============================================================================
// BTerrainSimRep::clampWorld
// MS 11/19/2008: clamps a world position to the terrain's XZ boundaries shrunk by the passed in buffer
// I'm not rewiring the above clampWorld function to call this even though it could... don't want to
// change any upstream behavior.
//==============================================================================
void BTerrainSimRep::clampWorldWithBuffer(BVector &position, float buffer)
{
   float oldY = position.y;
   position = XMVectorClamp(position, XMVectorReplicate(buffer), XMVectorReplicate(float((mNumXHeightVerts - 1) * mHeightTileScale) - buffer));
   position.y = oldY;
}

//==============================================================================
// BTerrainSimRep::flattenInstant
//==============================================================================
void BTerrainSimRep::flattenInstant(float mMinXPerc,float mMaxXPerc,float mMinZPerc,float mMaxZPerc,float desiredHeight)
{
   float mFalloffPerc = 0.3f;

   int minX = (int)floor(mNumXHeightVerts * mMinXPerc);
   int maxX = (int)ceil(mNumXHeightVerts * mMaxXPerc);
   int minZ = (int)floor(mNumXHeightVerts * mMinZPerc);
   int maxZ = (int)ceil(mNumXHeightVerts * mMaxZPerc);

   int xLen = maxX - minX;
   int zLen = maxZ - minZ;

   int xCenter = minX + (xLen>>1);
   int zCenter = minZ + (zLen>>1);

   int xFalloffStart = (int)((xLen>>1) * (1.0f-mFalloffPerc));
   int xFalloffLen = (int)((xLen>>1) * mFalloffPerc);
   int zFalloffStart = (int)((zLen>>1) * (1.0f-mFalloffPerc));
   int zFalloffLen = (int)((zLen>>1) * mFalloffPerc);

   for(int y = minZ; y < maxZ; y++)
   {
      for(int x =  minX; x < maxX; x++)   
      {
         if(x < 0 || y  < 0 ||
            x >= mNumXHeightVerts || y  >= mNumXHeightVerts)
            continue;


         int xDiff = x-xCenter;
         int distX = (int)Math::fSqrt((float)(xDiff*xDiff));
         float inflPercX  = 1;
         if(distX>=xFalloffStart)
            inflPercX = Math::Clamp<float>(1.0f-((distX-xFalloffStart)/(float)xFalloffLen),0,1);


         int zDiff = y-zCenter;
         int distZ = (int)Math::fSqrt((float)(zDiff*zDiff));
         float inflPercZ  = 1;
         if(distZ >= zFalloffStart)
            inflPercZ = Math::Clamp<float>(1.0f-((distZ-zFalloffStart)/(float)zFalloffLen),0,1);

         float inflPerc = Math::Min<float>(inflPercX,inflPercZ);

         if(inflPerc < 1.0f)
         {
            float h = 0;
            getHeight(x,y,h,true);

            h = (inflPerc*desiredHeight) + ((1-inflPerc)*h);

            setHeight(x,y,h);

         }
         else
         {
            setHeight(x,y,desiredHeight);
         }
      }
   }

   //issue the command to flatten the VISREP from here...
   gTerrain.flattenVisRepInstant(mMinXPerc,mMaxXPerc,mMinZPerc,mMaxZPerc,desiredHeight,mFalloffPerc);
}
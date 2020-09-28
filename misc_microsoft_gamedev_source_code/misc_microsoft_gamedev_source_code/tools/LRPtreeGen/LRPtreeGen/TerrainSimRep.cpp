//============================================================================
//
//  TerrainSimRep.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
// xgame 
//#include "common.h"

//terrain
#include "TerrainSimRep.h"
//#include "terrain.h"
//#include "mathutil.h"
//#include "terrainphysicsheightfield.h"
//#include "obstructionmanager.h"
//#include "debugprimitives.h"
//#include "render.h"

//xcore
#include "xcore.h"
#include "xcorelib.h"
#include "bfileStream.h"

#include "file\win32FileStream.h"
#include "resource/resourceTag.h"
#include "resource/ecfFileData.h"
#include "resource\ecfUtils.h"

#include "stream\dynamicStream.h"
#include "xml\xmxDataBuilder.h"
#include "xml\xmxDataDumper.h"
#include "utils\consoleAppHelper.h"
#include "consoleOutput.h"

#include "utils\commandLineParser.h"
#include "utils\consoleAppHelper.h"
#include "stream\cfileStream.h"
#include "utils\endianSwitch.h"
#include "resource\ecfUtils.h"
#include "resource\resourceTag.h"

#define cNumCirclePoints   32
#define cNumThickCirclePoints 48

//---------------------------------------------------------
BTerrainSimRep gTerrainSimRep;

//---------------------------------------------------------
BTerrainSimRep::BTerrainSimRep()
//   mpHeights(NULL),
//   mpTileType(NULL),
//   mpPhysicsTerrain(NULL)
{

}
//---------------------------------------------------------
BTerrainSimRep::~BTerrainSimRep()
{
//   destroy();  
}

/*
//---------------------------------------------------------
bool BTerrainSimRep::loadFromFile(long dirID, const char *filename)
{
   return (loadECFXSDInternal(dirID,filename));
//
//
//   const uint cBufSize = 1024;
//   char buf[cBufSize];
//   sprintf_s(buf, sizeof(buf), "BTerrainSimRep::loadFromFile : Load XSD Failed\n");
//
//   //get our XTD name
//   char path[256];
//   strcpy_s(path,filename);
//   strcat_s(path,".xsd");
//
//   //open the file
//   BFile file;
//   if (!file.openReadOnly(dirID, path))
//   { 
//      sprintf_s(buf, sizeof(buf), "BTerrainSimRep::loadFromFile : Could not load XSD For Read Only. Ensure file permissions, and re-export\n");
//      goto loadFailed;
//   }
//
//   //SIM DATA////////////////////////////////////////////////////////////////////////
//   XSDHeader head;   
//   if (!file.read(&head,sizeof(XSDHeader)))
//      goto loadFailed;
//
//   //VERSION CHECK!
//   if(head.version!=XSD_VERSION)
//   { 
//      sprintf_s(buf, sizeof(buf), "BTerrainSimRep::loadFromFile : XSD file header version (%i) conflicts desired version (%i)\n",head.version,XSD_VERSION);
//      goto loadFailed;
//   }
//
//
//   int numCacheFriendlyXTiles = head.simNumXTiles;
//   int numCacheFriendlyXVerts = numCacheFriendlyXTiles + 1;
//
//   mNumXTiles = head.terrainNumXTiles;
//   mNumXVerts = mNumXTiles + 1;
//   mTileScale = head.simTileScale;
//   mRecTileScale = 1.0f / mTileScale;
//   mVisualToSimScalar = head.simVisualTomSimScalar;
//   mNumXBlocks = numCacheFriendlyXVerts / 8;
//
//   //height values
//   if(mpHeights)   delete [] mpHeights;
//   mpHeights = new HALF[numCacheFriendlyXVerts*numCacheFriendlyXVerts];
//   if (!file.read(mpHeights,numCacheFriendlyXVerts*numCacheFriendlyXVerts*sizeof(HALF)))
//      goto loadFailed;
//
//   //obstruction flags
//   if(mpObstructionMap)   delete [] mpObstructionMap;
//   const int obstructionFlagsSize = mNumXTiles * mNumXTiles * sizeof(BYTE);
//   mpObstructionMap = new BYTE[obstructionFlagsSize];
//   if (!file.read(mpObstructionMap, obstructionFlagsSize))
//      goto loadFailed;
//#ifdef ENABLE_OLD_PATHER
//   gObsManager.deinitialize();
//#endif
//
//   //tile types
//   if(mpTileType)   delete [] mpTileType;
//   const int tileTypeArraySize = obstructionFlagsSize;
//   mpTileType = new BYTE[tileTypeArraySize];
//   if (!file.read(mpTileType, tileTypeArraySize))
//      goto loadFailed;
//
//   file.close();
//   return true;
//
//loadFailed:////////////////////////////////////////////////////////////////////////////
//   // rg [2/1/06] - I would rather us clean up everything and return false, instead of failing.
//   BFATAL_FAIL(buf);
//   //return false;
}
//---------------------------------------------------------
void BTerrainSimRep::destroy()
{
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
}
//---------------------------------------
bool	BTerrainSimRep::getHeight(BVector &position, bool clamp) const
{
   XMVECTOR tilePosition = __vctsxs(XMVectorMultiply(position, XMVectorReplicate(getReciprocalTileScale())), 0);
   const int x = tilePosition.u[0];
   const int z = tilePosition.u[2];
   return getHeight(x, z, position.y, clamp);
}
//---------------------------------------
bool	BTerrainSimRep::getHeight(const int x, const int y,float &retHeight,bool clamp) const
{
   BDEBUG_ASSERT(mpHeights);
   
   int tx=x;
   int ty=y;
   if(x > mNumXTiles)
   {
      if(clamp)   tx=mNumXTiles;
      else        return false;
   }
   if(x < 0)
   {
      if(clamp)   tx=0;
      else        return false;
   }

   if(y > mNumXTiles)
   {
      if(clamp)   ty=mNumXTiles;
      else        return false;
   }  
   if(y < 0)   
   {
      if(clamp)   ty=0;
      else        return false;
   }

   //convert from raster order to blocking
   int blockSize = 64;     // 8x8
   int blockPitch = 8;     
   int pitch = mNumXBlocks*64;

   int bx = (tx>>3) *blockSize;
   int by = (ty>>3) *pitch;

   int BlkIndx = (by+bx);

   int lx = (tx&0x07);
   int ly = (ty&0x07) * blockPitch;

   int lIndx = lx+ly;

   int indx = BlkIndx + lIndx;

   retHeight=XMConvertHalfToFloat(mpHeights[indx]);

   return true;
}
//---------------------------------------
bool BTerrainSimRep::rayIntersects(const BVector origin, const BVector dir, BVector &intersectionPt, bool allowOriginUnderTerrain) const
{
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
   const float    maxWorld = float(mNumXTiles) * mTileScale;
   const float    fMaxTiles = float(mNumXTiles);
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
            xRatio = abs(position.x - ((xPositive) ? 0.0f : maxWorld-mTileScale)) * recADirX;
         else
            xRatio = 0.0f;
      }
      else
         xRatio = 0.0f;

      if (zValid)
      {
         if (topClip || bottomClip)
            zRatio = abs(position.z - ((zPositive) ? 0.0f : maxWorld-mTileScale)) * recADirZ;
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
   BVector  truncatedTile = XMVectorTruncate(XMVectorMultiply(position, XMVectorReplicate(mRecTileScale)));
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
   while ((iX >= 0) && (iX < mNumXTiles) && (iZ >= 0) && (iZ < mNumXTiles))
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
      position = XMVectorMultiplyAdd(dirScale, dir, position);
      truncatedTile = XMVectorTruncate(XMVectorMultiply(position, XMVectorReplicate(mRecTileScale)));
      x = truncatedTile.x;
      z = truncatedTile.z;
      iX = long(x);
      iZ = long(z);

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
   BVector  truncatedTile = XMVectorTruncate(XMVectorMultiply(origin, XMVectorReplicate(mRecTileScale)));
   origin.y = 0.0f;
   // Clamp
   if (clamp)
      truncatedTile = XMVectorClamp(truncatedTile, XMVectorZero(), XMVectorReplicate(float(mNumXTiles - 1)));
   // Save off tile coordinates (float and int versions)
   float x = truncatedTile.x;
   float z = truncatedTile.z;
   long  iX = long(x);
   long  iZ = long(z);

   if ((iX < 0) || (iX >= mNumXTiles) || (iZ < 0) || (iZ >= mNumXTiles))
      return false;

   // Calculate tile coordinates in world space
   const float x1 = x * mTileScale;
   const float z1 = z * mTileScale;
   const float x2 = (x + 1.0f) * mTileScale;
   const float z2 = (z + 1.0f) * mTileScale;

   // Determine whether we're intersecting with the top or bottom triangle
   if (origin.distanceSqr(BVector(x1, 0.0f, z2)) <= origin.distanceSqr(BVector(x2, 0.0f, z1)))
   {
      // Top
      float y0, y1, y2;
      getHeight(iX, iZ, y0, true);
      getHeight(iX, iZ + 1, y1, true);
      getHeight(iX + 1, iZ + 1, y2, true);

      retHeight = LERP(y0, y1, (origin.z - z1) * mRecTileScale);
      retHeight = LERP(retHeight, y2, (origin.x - x1) * mRecTileScale);
   }
   else
   {
      // Bottom
      float y0, y2, y3;
      getHeight(iX, iZ, y0, true);
      getHeight(iX + 1, iZ + 1, y2, true);
      getHeight(iX + 1, iZ, y3, true);

      retHeight = LERP(y0, y2, (origin.z - z1) * mRecTileScale);
      retHeight = LERP(retHeight, y3, (origin.x - x1) * mRecTileScale);
   }

   return true;
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
   delete mpPhysicsTerrain;
   mpPhysicsTerrain = NULL;
}

//---------------------------------------
bool BTerrainSimRep::rayIntersectsInternal(const long iX, const long iZ, const float x, const float z, const BVector origin, const BVector dir, BVector &intersectionPt) const
{
   BVector  bottomPoint;
   BVector  verts[3];
   float y0, y1, y2, y3;
   bool  hit = false;

   // Calculate tile coordinates in world space
   const float x1 = x * mTileScale;
   const float z1 = z * mTileScale;
   const float x2 = (x + 1.0f) * mTileScale;
   const float z2 = (z + 1.0f) * mTileScale;

   // test top triangle
   getHeight(iX, iZ, y0, true);
   getHeight(iX, iZ + 1, y1, true);
   getHeight(iX + 1, iZ + 1, y2, true);
   verts[0].set(x1, y0, z1);
   verts[1].set(x1, y1, z2);
   verts[2].set(x2, y2, z2);
   if (raySegmentIntersectionTriangle(verts, origin, dir, false, intersectionPt, cFloatCompareEpsilon * 100.0f))
      hit = true;

   // test bottom triangle
   getHeight(iX + 1, iZ, y3, true);
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
void  BTerrainSimRep::addDebugLineOverTerrain(BVector point, BVector point2, DWORD color1, DWORD color2, float terrainOffset, int category, float timeout)
{
   BVector p1T = point;
   BVector p2T = point2;
   getHeight(p1T, true);
   getHeight(p2T, true);

   p1T.y += terrainOffset;
   p2T.y += terrainOffset;

   gpDebugPrimitives->addDebugLine(p1T, p2T, color1, color2, category, timeout);
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
void BTerrainSimRep::addDebugBoxOverTerrain(BVector p1, BVector p2, BVector p3, BVector p4, DWORD color, float terrainOffset, int category, float timeout)
{
   addDebugLineOverTerrain(p1, p2, color, color, terrainOffset, category, timeout);
   addDebugLineOverTerrain(p2, p3, color, color, terrainOffset, category, timeout);
   addDebugLineOverTerrain(p3, p4, color, color, terrainOffset, category, timeout);
   addDebugLineOverTerrain(p4, p1, color, color, terrainOffset, category, timeout);
}

//---------------------------------------
void BTerrainSimRep::addDebugThickBoxOverTerrain(BVector p1, BVector p2, BVector p3, BVector p4, float thickness, DWORD color, float terrainOffset, int category, float timeout)
{
   BVector p5,p6,p7,p8;
   p5 = p1;
   p6 = p2;
   p7 = p3;
   p8 = p4;

   p5.x -= thickness;
   p5.z -= thickness;

   p6.x += thickness;
   p6.z -= thickness;

   p7.x += thickness;
   p7.z += thickness;

   p8.x -= thickness;
   p8.z += thickness;

   addDebugThickLineOverTerrain(p5, p6, thickness, color, color, terrainOffset, category, timeout);
   addDebugThickLineOverTerrain(p2, p3, thickness, color, color, terrainOffset, category, timeout);
   addDebugThickLineOverTerrain(p7, p8, thickness, color, color, terrainOffset, category, timeout);
   addDebugThickLineOverTerrain(p4, p1, thickness, color, color, terrainOffset, category, timeout);
}

//---------------------------------------
void BTerrainSimRep::addDebugCircleOverTerrain(BVector center, float radius, DWORD color, float terrainOffset, int category, float timeout)
{   
   float radiansPerSegment = cTwoPi / cNumCirclePoints;
   float angularOffset = 0.0f;
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
   for (long p2 = 1; p2 < cNumCirclePoints; p1++, p2++)
      gpDebugPrimitives->addDebugLine(circleSelectPoints[p1], circleSelectPoints[p2], color, color, category, timeout);
}

//---------------------------------------
void BTerrainSimRep::addDebugThickCircleOverTerrain(BVector center, float radius, float thickness, DWORD color, float terrainOffset, int category, float timeout)
{   
   float radiansPerSegment = cTwoPi / cNumThickCirclePoints;
   float angularOffset = 0.0f;
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

   for (long i = 0; i < cNumThickCirclePoints; i++)
      gpDebugPrimitives->addDebugThickLine(circleSelectPoints[i], (i==cNumThickCirclePoints-1? circleSelectPoints[0]:circleSelectPoints[i+1]), thickness, color, color, category, timeout);

}

//---------------------------------------
BYTE BTerrainSimRep::getTileType(BVector position) const
{
   XMVECTOR tilePosition = __vctsxs(XMVectorMultiply(position, XMVectorReplicate(getReciprocalTileScale())), 0);
   const int x = tilePosition.u[0];
   const int z = tilePosition.u[2];

   return getTileType(x, z);
}

//---------------------------------------
BYTE BTerrainSimRep::getTileType(int x, int z) const
{
   x = Math::Clamp(x, 0, mNumXTiles - 1);
   z = Math::Clamp(z, 0, mNumXTiles - 1);

   return mpTileType[z * mNumXTiles + x];
}

//---------------------------------------
void BTerrainSimRep::render()
{
   const float maxDistance = 100.0f;
   const float maxDistanceSqr = maxDistance * maxDistance;
   BVector cameraPos = gRenderDraw.getMainSceneMatrixTracker().getWorldCamPos();

   for (int vx = 0; vx < mNumXTiles; vx++)
      for (int vz = 0; vz < mNumXTiles; vz++)
      {
         BVector tileScale = XMVectorReplicate(mTileScale);
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
         bool rightOK = (vx2 < mNumXVerts) ? true : false;
         bool topOK = (vz2 < mNumXVerts) ? true : false;

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
*/
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
//   gConsoleOutput.resource("BTerrainSimRep::loadECFXSDInternal: %s", filenameXSD.getPtr());

//   BFileSystemStream tfile;
   BCFileStream tfile;
   if (!tfile.open(filenameXSD, cSFReadable | cSFSeekable | cSFEnableBuffering))
   {
      return false;
   }



   BECFFileStream ecfReader;
   if(!ecfReader.open(&tfile)) //file checking and version ID done during constructor
   {
//      gConsoleOutput.error("BTerrainSimRep::loadECFXSDInternal : ECFHeader or Checksum invalid  %s\n", filenameXSD.getPtr());
      tfile.close();
      return false;
   }

   BECFHeader ecfHeader = ecfReader.getHeader();
   int numECFChunks = ecfHeader.getNumChunks();

   //find our PTHChunk header
   int headerIndex = ecfReader.findChunkByID(cXSD_XSDHeader);
   if(headerIndex==-1)
   { 
//      gConsoleOutput.error("BTerrainSimRep:loadECFXSDInternal : Could not find XSD Chunk Header\n");
      tfile.close();
      return false;
   }
   ecfReader.seekToChunk(headerIndex);


   //check our header data
   int version =0;
   ecfReader.getStream()->readObj(version);
   if(version !=cXSD_Version)
   {
//      gConsoleOutput.error("BTerrainSimRep:loadECFXSDInternal : Could not find XSD Chunk Header\n");
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

      }
   }

   ecfReader.close();
   tfile.close();
   return true;
}
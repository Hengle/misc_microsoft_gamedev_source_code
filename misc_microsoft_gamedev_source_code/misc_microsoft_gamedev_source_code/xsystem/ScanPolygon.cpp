//============================================================================
//
//  ScanPolygon.cpp
//
//  Copyright (c) 1999-2001, Ensemble Studios
//
//============================================================================

//-- GRG +++ improvement:  The line stepper sucks because drawing a line
//-- between the same two points can yield different results, depending on
//-- the direction it is traversed.  It would be cool if there was a forward
//-- stepper that produced a symmetric line.  It would also be cool if the
//-- stepper was customized for y-stepping only.

//-- GRG +++ improvement:  Add a triangle function that doesn't make any
//-- allocations or use dynamic storage.

//-- GRG +++ clipping:  changes trace line for clipped edges!  But only
//-- sometimes and only by a pixel.  Still, thats not good.

//-- GRG +++ duplicate point reporting:  we could put a special case check in
//-- that fixes duplicate reporting for downward facing concave verts.  That
//-- would help cuz then the only double reporting case left would be for 
//-- overlapping vertices and lines.


//============================================================================
//  INCLUDES
//============================================================================
#include "xsystem.h"
#include "tilecoordinates.h"
#include "ScanConversion.h"


//============================================================================
//  PRIVATE CONSTANTS
//============================================================================
static const long sgMaxClipVerts = 1024;


//============================================================================
//  PRIVATE STRUCTS
//============================================================================
struct PolyEdge
{
   long endIndex;
   LineStepForward line;
};


//============================================================================
//  PRIVATE GLOBALS
//============================================================================
static BTileCoordinates sgClipSpace1[sgMaxClipVerts];
static BTileCoordinates sgClipSpace2[sgMaxClipVerts];


//============================================================================
//  CLIPPING FUNCTIONS
//============================================================================
void intersectX(const BTileCoordinates& p1, const BTileCoordinates& p2, BTileCoordinates& pi, long x)
{
   pi.x = x;
   pi.z = p1.z + ((x - p1.x)*(p2.z - p1.z))/(p2.x - p1.x);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void intersectZ(const BTileCoordinates& p1, const BTileCoordinates& p2, BTileCoordinates& pi, long z)
{
   pi.z = z;
   pi.x = p1.x + ((z - p1.z)*(p2.x - p1.x))/(p2.z - p1.z);
}

//----------------------------------------------------------------------------
//  clipPolygon()
//  Implements a standard Sutherland and Hodgman polygon clipper.  The
//  clipped points are returned via a pointer to a private buffer.  You must
//  use the clipped points right away, as the buffer can be overwritten by
//  the next call to clipPolygon().  The size of the clipped points is
//  returned in outSize;
//----------------------------------------------------------------------------
BTileCoordinates* clipPolygon(const BTileCoordinates* inPoints, long inSize, long& outSize, long xmin, long zmin, long xmax, long zmax)
{
   #define CHECK_SIZE_1 if (outSize >= sgMaxClipVerts) { outSize = 0; return NULL; }
   #define CHECK_SIZE_2 if (outSize >= sgMaxClipVerts-1) { outSize = 0; return NULL; }

   BTileCoordinates p1, p2, pi;

   //-- Make sure our min/maxes are happy.
   long temp;
   if (xmin > xmax) { temp = xmin; xmin = xmax; xmax = temp; }
   if (zmin > zmax) { temp = zmin; zmin = zmax; zmax = temp; }

   //-- Gottahave at least 3 points.
   if (inSize < 3)
   {
      outSize = 0;
      return NULL;
   }

   //-- Clip against left side.
   outSize = 0;
   p1      = inPoints[inSize-1];
   for (long point = 0; point < inSize; point++)
   {
      p2 = inPoints[point];
      if (p2.x > xmin)
      {
         if (p1.x > xmin)
         {
            CHECK_SIZE_1
            sgClipSpace1[outSize++] = p2;
         }
         else
         {
            CHECK_SIZE_2
            intersectX(p1, p2, pi, xmin);
            sgClipSpace1[outSize++] = pi;
            sgClipSpace1[outSize++] = p2;
         }
      }
      else
      {
         if (p1.x > xmin)
         {
            CHECK_SIZE_1
            intersectX(p1, p2, pi, xmin);
            sgClipSpace1[outSize++] = pi;
         }
      }
      p1 = p2;
   }

   //-- Make sure we still have some points.
   if (outSize == 0) 
      return NULL;

   //-- Clip against right side.
   inSize   = outSize;
   inPoints = sgClipSpace1;
   outSize  = 0;
   p1       = inPoints[inSize-1];
   for (long point = 0; point < inSize; point++)
   {
      p2 = inPoints[point];
      if (p2.x < xmax)
      {
         if (p1.x < xmax)
         {
            CHECK_SIZE_1
            sgClipSpace2[outSize++] = p2;
         }
         else
         {
            CHECK_SIZE_2
            intersectX(p1, p2, pi, xmax);
            sgClipSpace2[outSize++] = pi;
            sgClipSpace2[outSize++] = p2;
         }
      }
      else
      {
         if (p1.x < xmax)
         {
            CHECK_SIZE_1
            intersectX(p1, p2, pi, xmax);
            sgClipSpace2[outSize++] = pi;
         }
      }
      p1 = p2;
   }

   //-- Make sure we still have some points.
   if (outSize == 0) 
      return NULL;

   //-- Clip against top side.
   inSize   = outSize;
   inPoints = sgClipSpace2;
   outSize  = 0;
   p1       = inPoints[inSize-1];
   for (long point = 0; point < inSize; point++)
   {
      p2 = inPoints[point];
      if (p2.z > zmin)
      {
         if (p1.z > zmin)
         {
            CHECK_SIZE_1
            sgClipSpace1[outSize++] = p2;
         }
         else
         {
            CHECK_SIZE_2
            intersectZ(p1, p2, pi, zmin);
            sgClipSpace1[outSize++] = pi;
            sgClipSpace1[outSize++] = p2;
         }
      }
      else
      {
         if (p1.z > zmin)
         {
            CHECK_SIZE_1
            intersectZ(p1, p2, pi, zmin);
            sgClipSpace1[outSize++] = pi;
         }
      }
      p1 = p2;
   }

   //-- Make sure we still have some points.
   if (outSize == 0) 
      return NULL;

   //-- Clip against bottom side.
   inSize   = outSize;
   inPoints = sgClipSpace1;
   outSize  = 0;
   p1       = inPoints[inSize-1];
   for (long point = 0; point < inSize; point++)
   {
      p2 = inPoints[point];
      if (p2.z < zmax)
      {
         if (p1.z < zmax)
         {
            CHECK_SIZE_1
            sgClipSpace2[outSize++] = p2;
         }
         else
         {
            CHECK_SIZE_2
            intersectZ(p1, p2, pi, zmax);
            sgClipSpace2[outSize++] = pi;
            sgClipSpace2[outSize++] = p2;
         }
      }
      else
      {
         if (p1.z < zmax)
         {
            CHECK_SIZE_1
            intersectZ(p1, p2, pi, zmax);
            sgClipSpace2[outSize++] = pi;
         }
      }
      p1 = p2;
   }

   //-- Make sure we still have some points.
   if (outSize == 0) 
      return NULL;

   return sgClipSpace2;

   #undef CHECK_SIZE_1
   #undef CHECK_SIZE_2
}


//============================================================================
//  POLYGON FUNCTIONS
//============================================================================
int __cdecl sortCheckpoints(const void* elem1, const void* elem2)
{
   long z1 = *(long*)elem1;
   long z2 = *(long*)elem2;

   if (z1 < z2)
      return -1;
   if (z1 > z2)
      return 1;
   return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int __cdecl sortInactiveEdges(const void* elem1, const void* elem2)
{
   PolyEdge* pEdge1 = *(PolyEdge**)elem1;
   PolyEdge* pEdge2 = *(PolyEdge**)elem2;

   if (pEdge1->line.curZ() < pEdge2->line.curZ())
      return -1;
   if (pEdge1->line.curZ() > pEdge2->line.curZ())
      return 1;

   //-- For inactive edges, ties don't matter.  So we don't have to do
   //-- anything special when the are equal.
   return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int __cdecl sortActiveEdges(const void* elem1, const void* elem2)
{
   PolyEdge* pEdge1 = *(PolyEdge**)elem1;
   PolyEdge* pEdge2 = *(PolyEdge**)elem2;

   if (pEdge1->line.curX() < pEdge2->line.curX())
      return -1;
   if (pEdge1->line.curX() > pEdge2->line.curX())
      return 1;

   //-- If the x values are the same, then we want the line that is "right of"
   //-- the other line to be considered the lesser.  We use pEdge1 as the
   //-- control and test pEdge2 against it.  Note the trickery in computing 
   //-- x1, y1.  This effectively gives a vector 90 degrees to the right of
   //-- pEdge1's line.  Which is what we need for the dot product test.
   long x1  = -(pEdge1->line.z2() - pEdge1->line.curZ());
   long z1  =  (pEdge1->line.x2() - pEdge1->line.curX());
   long x2  =  (pEdge2->line.x2() - pEdge2->line.curX());
   long z2  =  (pEdge2->line.z2() - pEdge2->line.curZ());
   long dot = x1*x2 + z1*z2;

   //-- If the dot product is positive, pEdge2 is aimed to the right of pEdge1.  Which
   //-- makes pEdge1 greater than pEdge2.
   if (dot > 0)
      return 1;

   return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long getChildren(const BTileCoordinates* pPoints, long numPoints, long index, long& child1, long& child2)
{
   long prev = (index == 0) ? numPoints - 1 : index - 1;
   long next = (index == numPoints - 1) ? 0 : index + 1;
   
   if (pPoints[prev].z > pPoints[index].z)
   {
      child1 = prev;
      if (pPoints[next].z > pPoints[index].z)
      {
         child2 = next;
         return 2;
      }
      return 1;
   }
   else
   {
      if (pPoints[next].z > pPoints[index].z)
      {
         child1 = next;
         return 1;
      }
   }
   return 0;
}

//---------------------------------------------------------------------------- 
//  advanceEdge()
//  This moves an edge down to the next z value. It returns the span of x
//  values that was encountered.
//---------------------------------------------------------------------------- 
void advanceEdge(PolyEdge& edge, long& minX, long& maxX)
{
   long dummyX, dummyZ;
   long prevX = edge.line.curX();
   long stopZ = edge.line.curZ() + 1;
   minX = prevX;
   maxX = prevX;
   while (edge.line.curZ() != stopZ)
   {
      prevX = edge.line.curX();
      edge.line.step(dummyX, dummyZ);
   }

   //-- Update the x span.
   if (minX > prevX)
      minX = prevX;
   if (maxX < prevX)
      maxX = prevX;
}

//---------------------------------------------------------------------------- 
//  endEdge()
//  This moves an edge until it dies.  It returns the span of x values that
//  was encountered.
//---------------------------------------------------------------------------- 
void endEdge(PolyEdge& edge, long& minX, long& maxX)
{
   long dummyX, dummyZ;
   minX = edge.line.curX();
   maxX = edge.line.curX();
   while (edge.line.step(dummyX, dummyZ));

   //-- Update the x span.
   if (minX > edge.line.curX())
      minX = edge.line.curX();
   if (maxX < edge.line.curX())
      maxX = edge.line.curX();
}

//---------------------------------------------------------------------------- 
//  scanEdges()
//  This groups the edges in the active edge list by pairs and then scans
//  them down to the z terminator.
//---------------------------------------------------------------------------- 
void scanEdges(BDynamicSimArray<PolyEdge*>& activeEdges, long checkpoint, POLYGON_FUNC* pFunc, void* pParam)
{
   //-- There had damn well better be an EVEN number of edges.
   BASSERT(activeEdges.getNumber() % 2 == 0);

   //-- Run each pair down to its terminator, scanning at each z increment.
   PolyEdge** ppPolyEdges = activeEdges.getData();
   long numPairs = activeEdges.getNumber() / 2;
   for (long pair = 0; pair < numPairs; ++pair)
   {
      PolyEdge* pEdge1 = *(ppPolyEdges++);
      PolyEdge* pEdge2 = *(ppPolyEdges++);
      while (pEdge1->line.curZ() != checkpoint)
      {
         long minX1, minX2, maxX1, maxX2;
         long z = pEdge1->line.curZ();
         advanceEdge(*pEdge1, minX1, maxX1);
         advanceEdge(*pEdge2, minX2, maxX2);

         //-- Scan it.
         if (minX1 > minX2)
            minX1 = minX2;
         if (maxX1 < maxX2)
            maxX1 = maxX2;

         for (long x = minX1; x <= maxX1; ++x)
            pFunc(x, z, pParam);
      }
      BASSERT(pEdge1->line.curZ() == checkpoint);
      BASSERT(pEdge2->line.curZ() == checkpoint);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void removeEdges(BDynamicSimArray<PolyEdge*>& activeEdges, BTileCoordinates* pPoints, long numPoints, POLYGON_FUNC* pFunc, void* pParam)
{
   //-- There had damn well better be an EVEN number of edges.
   BASSERT(activeEdges.getNumber() % 2 == 0);

   BDynamicSimArray<long> deadEdges;

   //-- Search for dead edges.
   PolyEdge** ppPolyEdges = activeEdges.getData();
   long numPairs = activeEdges.getNumber() / 2;
   for (long pair = 0; pair < numPairs; ++pair)
   {
      PolyEdge* pEdge1 = *(ppPolyEdges++);
      PolyEdge* pEdge2 = *(ppPolyEdges++);

      //-- An edge is dead if its current z coordinate is equal to its end z
      //-- coordinate.  Keep in mind that the edge pair has not scanned this z
      //-- coordinate yet.
      bool edge1Dead = false;
      bool edge2Dead = false;
      if (pEdge1->line.curZ() == pEdge1->line.z2())
      {
         edge1Dead = true;
         deadEdges.add(pair*2);
      }
      if (pEdge2->line.curZ() == pEdge2->line.z2())
      {
         edge2Dead = true;
         deadEdges.add(pair*2+1);
      }
      
      //-- If both edges in the pair have died, and neither edge has a child,
      //-- then we have to draw the pair's last scan line.
      if (!edge1Dead || !edge2Dead)
         continue;

      long dummyChild1, dummyChild2;
      long edge1Children = getChildren(pPoints, numPoints, pEdge1->endIndex, dummyChild1, dummyChild2);
      long edge2Children = getChildren(pPoints, numPoints, pEdge2->endIndex, dummyChild1, dummyChild2);
      if (edge1Children || edge2Children)
         continue;

      //-- Do the last scan for the pair.
      long minX1, minX2, maxX1, maxX2;
      long z = pEdge1->line.curZ();
      endEdge(*pEdge1, minX1, maxX1);
      endEdge(*pEdge2, minX2, maxX2);

      //-- Scan it.
      if (minX1 > minX2)
         minX1 = minX2;
      if (maxX1 < maxX2)
         maxX1 = maxX2;

      for (long x = minX1; x <= maxX1; ++x)
         pFunc(x, z, pParam);
   }

   //-- Clean out the dead edges.  Clean them in reverse order so the array
   //-- wont screw us when it re-aligns the array.
   long  numDeadEdges = deadEdges.getNumber();
   const long* pDeadEdges   = deadEdges.getData();
   for (long edge = numDeadEdges - 1; edge >= 0; --edge)
      activeEdges.removeIndex(pDeadEdges[edge]);
}

//----------------------------------------------------------------------------
//  scanPolygon()
//  Works on any simple polygon that can be described by a list of points.
//  The points can be clockwise or counter clockwise, it is irrelavent.  You
//  do not need to specify a closed loop of points.  The last point in
//  pPoints is automatically closed to the first point.
//
//  NOTE:  For the most part, each point in the polygon interior is reported
//  exactly once.  However, for degenerate cases like overlapped lines and
//  downward facing concave vertices, duplicate point reporting will happen.
//----------------------------------------------------------------------------
void scanPolygon(BTileCoordinates* pPoints, long numPoints, const ClipRect* pClip, POLYGON_FUNC* pFunc, void* pParam)
{
   //-- This function doesn't accomplish anything without a callback.
   if (pFunc == NULL)
      return;

   //-- Gotta have at least 3 points.
   if (numPoints < 3)
      return;

   //-- Clip.
   if (pClip)
   {
      pPoints = clipPolygon(pPoints, numPoints, numPoints, pClip->xmin, pClip->zmin, pClip->xmax, pClip->zmax);
      if ((pPoints == NULL) || (numPoints < 3))
         return;
   }

   //-- Build our edges.
   long      numEdges   = 0;
   PolyEdge* pPolyEdges = new PolyEdge [numPoints];
   for (long point = 0; point < numPoints; ++point)
   {
      long child1, child2;
      long numChildren = getChildren(pPoints, numPoints, point, child1, child2);
      
      if (numChildren > 0)
      {
         long x1 = pPoints[point].x;
         long z1 = pPoints[point].z;
         long x2 = pPoints[child1].x;
         long z2 = pPoints[child1].z;
         pPolyEdges[numEdges].line.init(x1, z1, x2, z2, NULL);
         pPolyEdges[numEdges].endIndex = child1;
         numEdges++;
      }
      if (numChildren > 1)
      {
         long x1 = pPoints[point].x;
         long z1 = pPoints[point].z;
         long x2 = pPoints[child2].x;
         long z2 = pPoints[child2].z;
         pPolyEdges[numEdges].line.init(x1, z1, x2, z2, NULL);
         pPolyEdges[numEdges].endIndex = child2;
         numEdges++;
      }
   }
   BASSERT(numEdges <= numPoints);

   //-- It is possible to have 0 edges.  This would happen if the polygon was
   //-- a flat line.
   if (numEdges == 0)
   {
      //-- Just find the min and max x, and scan those.
      long z    = pPoints[0].z;
      long minX = pPoints[0].x;
      long maxX = pPoints[0].x;
      for (long point = 1; point < numPoints; ++point)
      {
         if (minX > pPoints[point].x)
            minX = pPoints[point].x;
         if (maxX < pPoints[point].x)
            maxX = pPoints[point].x;
      }

      //-- Scan.
      for (long x = minX; x <= maxX; ++x)
         pFunc(x, z, pParam);
   }

   //-- Now we want to have a list of the z values of points, sorted in 
   //-- ascending order.  We use these to determine when to stop each scan
   //-- and check for more edges to start.
   BDynamicSimArray<long> checkPoints;
   for (long point = 0; point < numPoints; ++point)
      checkPoints.add(pPoints[point].z);
   checkPoints.sort(sortCheckpoints);
   const long* pCheckpoint = checkPoints.getData();
   
   //-- Now we want to have a list of the inactive edges, sorted by z value.
   BDynamicSimArray<PolyEdge*> inactiveEdges;
   for (long edge = 0; edge < numEdges; ++edge)
      inactiveEdges.add(&pPolyEdges[edge]);
   inactiveEdges.sort(sortInactiveEdges);
   PolyEdge** ppInactiveEdges = inactiveEdges.getData();

   //-- Start the active edge list with all the top level edges.
   BDynamicSimArray<PolyEdge*> activeEdges;
   long numEdgesStarted = 0;
   while (numEdgesStarted < numEdges || activeEdges.getNumber())
   {
      //-- Add the new edges.
      while ((numEdgesStarted < numEdges) && ((*ppInactiveEdges)->line.z1() == *pCheckpoint))
      {
         activeEdges.add(*ppInactiveEdges);
         numEdgesStarted++;
         ppInactiveEdges++;
      }

      //-- Sort the active edge by their x values.
      activeEdges.sort(sortActiveEdges);

      //-- Advance to the next checkpoint.
      long oldCheckpoint = *pCheckpoint;
      while (*pCheckpoint == oldCheckpoint)
         pCheckpoint++;

      //-- Run the active edges down to the new checkpoint.
      scanEdges(activeEdges, *pCheckpoint, pFunc, pParam);

      //-- Remove the dead edges.
      removeEdges(activeEdges, pPoints, numPoints, pFunc, pParam);
   }

   delete [] pPolyEdges;
}



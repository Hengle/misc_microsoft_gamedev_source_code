//==============================================================================
// convexpolyhedron.cpp
//
// Very simple convex polyhedron code: nothing fancy
//
// Copyright (c) 2004, Ensemble Studios
// Copyright (c) Microsoft Corporation. All rights reserved. (derived from XGRlib)
//==============================================================================

//=============================================================================
// Includes.
#include "xsystem.h"
#include "convexpolyhedron.h"
#include "plane.h"
#include "mathutil.h"

BConvexPolyhedron& BConvexPolyhedron::set(const VertVector& verts, const IndexedTriVector& tris)
{
   mVerts = verts;
   mTris = tris;
   return *this;
}

void BConvexPolyhedron::clear(void)
{
   mVerts.clear();
   mTris.clear();
}
void BConvexPolyhedron::clipAgainstPlane(BConvexPolyhedron& dst, const BPlane& plane, bool cap /*=true*/) const
{
   static VertVector clippedPoly;
   clippedPoly.clear();
   clippedPoly.reserve(8);

   dst.clear();

   static   VertVector  capPoly;
   static   SegmentList capSegList;
   static   Segment     capSeg;

   if (cap)
   {
      capPoly.clear();
      capPoly.reserve(32);

      capSegList.clear();
      capSegList.reserve(32);
   }

#ifndef BUILD_FINAL
   BVector averagePt(cOriginVector);
   for (long i=0; i < numVerts(); ++i)
      averagePt += vert(i);
   averagePt = averagePt / float(numVerts());
#endif

   BVector clippedEdgePoint1, clippedEdgePoint2;

   static VertVector srcTri;
   srcTri.clear();
   srcTri.setNumber(3);
   for (long i=0; i < numTris(); ++i)
   {
      srcTri[0] = vert(tri(i).v1);
      srcTri[1] = vert(tri(i).v2);
      srcTri[2] = vert(tri(i).v3);

      bool bTriIsCut = clipPoly(clippedPoly, srcTri, plane,
                                clippedEdgePoint1, clippedEdgePoint2);

      if (clippedPoly.getNumber() < 3)
         continue;

      dst.addPoly(clippedPoly);


      if((cap) && (bTriIsCut))
      {
         capSeg.mStart = clippedEdgePoint1;
         capSeg.mEnd = clippedEdgePoint2;
         capSegList.add(capSeg);
      }
   }

   if((cap) && (capSegList.getNumber() > 0))
   {
      // Build the polygon out of all the edges.
      //

      // Add first vertex
      capPoly.add(capSegList[0].mStart);
      capPoly.add(capSegList[0].mEnd);
      BVector vertexToMatch = capSegList[0].mEnd;
      capSegList.removeIndex(0);

      int i;
      while(capSegList.getNumber() > 1)
      {
         long numSegs = capSegList.getNumber();

         float minDist = FLT_MAX;
         int minIndex = -1;
         bool startOrEnd = false;
         float distSqr;

         for(i = 0; i < numSegs; i++)
         {
            distSqr = vertexToMatch.distanceSqr(capSegList[i].mEnd);
            if( distSqr < minDist )
            {
               minDist = distSqr;
               minIndex = i;
               startOrEnd = true;
            }

            distSqr = vertexToMatch.distanceSqr(capSegList[i].mStart);
            if( distSqr < minDist )
            {
               minDist = distSqr;
               minIndex = i;
               startOrEnd = false;
            }
         }

         if( startOrEnd )
         {
            //if( !vertexToMatch.almostEqual(capSegList[minIndex].mEnd, 0.0005f))
            //{
            //   BASSERT(0);
            //}

            capPoly.add(capSegList[minIndex].mStart);
            vertexToMatch = capSegList[minIndex].mStart;
         }
         else
         {
            //if( !vertexToMatch.almostEqual(capSegList[minIndex].mStart, 0.0005f))
            //{
            //   BASSERT(0);
            //}

            capPoly.add(capSegList[minIndex].mEnd);
            vertexToMatch = capSegList[minIndex].mEnd;
         }
         capSegList.removeIndex(minIndex);
      }

      // Now that the cap poly is built, add it.
      if (capPoly.getNumber() >= 3)
         dst.addPoly(capPoly);
   }
}

void BConvexPolyhedron::addPoly(const VertVector& poly)
{
   BASSERT(poly.getNumber() >= 3);
   addPoly(poly.getPtr(), poly.getNumber());
}
void BConvexPolyhedron::addPoly(const VertType* pPoly, long numVerts)
{
   BASSERT(numVerts >= 3);
   BASSERT(pPoly);
   if(!pPoly) return;

   long firstVert = mVerts.getNumber();
   mVerts.add(pPoly[0]);

   for (int j = 0; j < numVerts - 2; j++)
   {
      mVerts.add(pPoly[1 + j]);
      mVerts.add(pPoly[2 + j]);

      IndexedTriType newTri;
      newTri.v1 = firstVert;
      newTri.v2 = firstVert + 1 + j * 2;
      newTri.v3 = firstVert + 2 + j * 2;
      mTris.add(newTri);
   }
}

/*static*/ void BConvexPolyhedron::clipPoly(VertVector& result, const VertVector& verts, const BPlane& plane)
{
   result.clear();

   if (verts.getNumber()<=0)
      return;

   float prevDist = plane.distanceFromPoint(verts[0]);
   const long numVerts = verts.getNumber();

   for (long prev = 0; prev < numVerts; prev++)
   {
      long cur = Math::NextWrap<long>(prev, numVerts);
      float curDist = plane.distanceFromPoint(verts[cur]);

      if (prevDist >= 0.0f)
         result.add(verts[prev]);

      if (((prevDist < 0.0f) && (curDist > 0.0f)) ||
         ((prevDist > 0.0f) && (curDist < 0.0f)))
      {
         result.add(Math::Lerp<VertType, float>(verts[prev], verts[cur], prevDist / (prevDist - curDist)));
      }
   }
}


/*static*/ bool BConvexPolyhedron::clipPoly(VertVector& result, const VertVector& verts, const BPlane& plane,
                                             BVector &clippedEdgePoint1, BVector &clippedEdgePoint2)
{
   result.clear();

   bool bPolyIsCut = false;

   if (verts.getNumber()<=0)
      return( bPolyIsCut );

   float prevDist = plane.distanceFromPoint(verts[0]);
   const long numVerts = verts.getNumber();

   for (long prev = 0; prev < numVerts; prev++)
   {
      long cur = Math::NextWrap<long>(prev, numVerts);
      float curDist = plane.distanceFromPoint(verts[cur]);

      if (prevDist >= 0.0f)
         result.add(verts[prev]);

      if (((prevDist < 0.0f) && (curDist > 0.0f)) ||
         ((prevDist > 0.0f) && (curDist < 0.0f)))
      {
         BVector intersectPoint = Math::Lerp<VertType, float>(verts[prev], verts[cur], prevDist / (prevDist - curDist));
         result.add(intersectPoint);

         if( !bPolyIsCut )
            clippedEdgePoint1 = intersectPoint;
         else
            clippedEdgePoint2 = intersectPoint;

         bPolyIsCut = true;
      }

      prevDist = curDist;
   }

   return( bPolyIsCut );
}

//==========================================================================================================
// eof convexpolyhedron.cpp
//==========================================================================================================

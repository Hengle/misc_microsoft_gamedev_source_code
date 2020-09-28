//==============================================================================
// convexpolyhedron.h
//
// Very simple convex polyhedron code: nothing fancy
//
// Copyright (c) 2004, Ensemble Studios
// Copyright (c) Microsoft Corporation. All rights reserved. (derived from XGRlib)
//==============================================================================

#ifndef _CONVEXPOLYHEDRON_H_
#define _CONVEXPOLYHEDRON_H_

//=============================================================================
// Includes.
//#include "xsystem.h"

//=============================================================================
// Forward declarations.
class BPlane;

//=============================================================================
class BConvexPolyhedron
{
   public:

      typedef BVector                           VertType;
      typedef struct{long v1;long v2;long v3;}  IndexedTriType;
      typedef BDynamicSimArray<VertType>            VertVector;
      typedef BDynamicSimArray<IndexedTriType>      IndexedTriVector;


      struct Segment
      {
      public:
	      BVector mStart;
         BVector mEnd;
      };

      typedef  BDynamicSimArray<struct Segment> SegmentList;


      BConvexPolyhedron(void) {}
      ~BConvexPolyhedron(void) {clear();}

      // Methods
      BConvexPolyhedron& set(const VertVector& verts, const IndexedTriVector& tris);
      void clear(void);
      void addPoly(const VertVector& poly);
      void addPoly(const VertType* pPoly, long numVerts);

      // Queries
      long numVerts(void) const {return mVerts.getNumber();}
      long numTris(void) const {return mTris.getNumber();}
      bool isEmpty(void) const { return !numTris(); }

      // Gets
      const VertVector&       getVerts(void) const    { return mVerts; }
      VertVector&             getVerts(void)          { return mVerts; }
      const IndexedTriVector& getTris(void) const { return mTris; }
      IndexedTriVector&       getTris(void)       { return mTris; }
      const VertType&         vert(long i) const   { return mVerts.get(i); }
      const IndexedTriType&   tri(long i) const    { return mTris.get(i); }

      // Clipping
      void clipAgainstPlane(BConvexPolyhedron& dst, const BPlane& plane, bool cap = true) const;
      static void clipPoly(VertVector& result, const VertVector& verts, const BPlane& plane);
      static bool clipPoly(VertVector& result, const VertVector& verts, const BPlane& plane, 
                           BVector &clippedEdgePoint1, BVector &clippedEdgePoint2);

      // operators
      BConvexPolyhedron& operator=(BConvexPolyhedron& src)
      {
         this->set(src.mVerts, src.mTris);
         return *this;
      }

protected:

      VertVector        mVerts;
      IndexedTriVector  mTris;
};

#endif

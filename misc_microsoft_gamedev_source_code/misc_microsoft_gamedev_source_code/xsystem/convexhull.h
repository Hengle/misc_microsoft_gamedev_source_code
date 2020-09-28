//==============================================================================
// convexhull.h
//
// Copyright (c) 1999-2000, Ensemble Studios
//==============================================================================

#ifndef _CONVEXHULL_H_
#define _CONVEXHULL_H_

//=============================================================================
// Includes.
#include "bitarray.h"
//#include "xsystem.h"
#include "gamefilemacros.h"

//=============================================================================
// Forward declarations.
class BChunkWriter;
class BChunkReader;

//=============================================================================
class BConvexHull
{
   public:
      enum
      {
         cSpecialIntersection,       // normal intersection
         cSpecialNoIntersection,     // no intersection
         cSpecialInvalidDirection    // intersection -- ray goes in "invalid" direction for pathing
      };

      enum
      {
         cMinMaxNone,
         cMinMaxEnter,
         cMinMaxExit,
         cMinMaxThrough,
         cMinMaxInside
      };

                              BConvexHull(void);
                              BConvexHull(const BConvexHull &h);
      virtual                 ~BConvexHull(void);

      BConvexHull             &operator=(const BConvexHull &h);
      bool                    operator ==(const BConvexHull &hull) const;

      long                    getPointCount(void) const {return(mPoints.getNumber());}
      const BVector           &getPoint(long index) const {BASSERT(index>=0 && index<mPoints.getNumber()); return(mPoints[index]);}
      BDynamicSimVectorArray      &getPoints(void) {return(mPoints);}
      const BDynamicSimVectorArray &getPoints(void) const {return(mPoints);}

      const BVector           &getBoundingMin(void) const {return(mBoundingMin);}
      const BVector           &getBoundingMax(void) const {return(mBoundingMax);}

      const BVector           &getCenter(void) const {return(mCenter);}

      bool                    initialize(const BVector *points, const long num, const bool alreadyConvex);
      bool                    addPoints(const BVector *points, const long num, const bool bValidPoints = false);
      bool                    removePoint(const long index);

      bool                    inside(const BVector &point) const;
      bool                    insideOrOnEdge(const BVector &point, bool *pbOnEdge = NULL) const;

      long                    hullVertex(const BVector &point, const float epsilon=cFloatCompareEpsilon) const;
      bool                    overlapsHull(const BConvexHull &hull, float errorEpsilon=0.0f) const;
      bool                    overlapsBox(const float minX, const float minZ, const float maxX, const float maxZ) const;

      bool                    expandInto(float expansion, BDynamicSimVectorArray &expandedPoints) const;
      bool                    expand(float expansion);
      void                    scale(float scale);
      void                    bevel(float edgeCutPercent);
      void                    grow(float amount);
      bool                    expandFrom(float expansion, const BConvexHull &hull);
      bool                    combine(const BConvexHull& hull);

      bool                    segmentIntersects(const BVector &point1, const BVector &point2, const long ignorePoint,
                                 BVector &iPoint, long &segmentIndex, float &distanceSqr, long &lNumIntersections,
                                 bool checkBBox, bool bCheckInside = true, float ignoreDistSqr=0.0f) const;
      bool                    segmentIntersects(const BVector &point1, const BVector &point2, bool checkBBox) const;

      long                    minMaxSegIntersect(const BVector &point1, const BVector &point2, 
                                 BVector &minPoint, BVector &maxPoint) const;

      inline bool             segmentMightIntersectBox(const BVector &point1, const BVector &point2) const;

      bool                    rayIntersects(const BVector &point, const BVector &vector, BVector &iPoint, long *segmentIndex, 
                                 float *distSqr) const;
      long                    rayIntersectsSpecial(const BVector &point, const BVector &vector, const BVector *point2, BVector &iPoint, 
                                 long *segmentIndex, float *distSqr) const;
      bool                    rayIntersects(const BVector &point, const BVector &vector) const;
      bool                    rayCoincidentToEdge(const BVector &point, const BVector &vector) const;

      // Special function for pather.
      long                    patherSegmentIntersect(const BVector &start, const BVector &goal, BVector &iPoint, float &distSqr, long &segIndex);

      BVector                 getSegmentDir(long segmentIndex) const;

      float                   distance(const BVector &point) const;
      float                   distanceSqr(const BVector &point) const;
      float                   distance(const BConvexHull &hull) const;
      float                   distanceSqr(const BConvexHull &hull) const;
      bool                    inRange(const BVector &point, const float range) const;
      bool                    inRange(const BConvexHull &hull, const float range) const;
 
      BVector                 findClosestPointOnHull(const BVector &vStart, long *plSegmentIndex = NULL, 
                                    float *pfClosestDistSqr = NULL);
      //Area methods.
      float                   calculateArea( void );

      virtual void            clear(void);
      void                    releaseMemory() {mPoints.clear();}

      //Save/load.
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      GFDECLAREVERSION();

   protected:
      virtual bool            addPoint(const BVector &point);
      void                    computeCenter(void);
      void                    computeBoundingBox(void);
      inline bool             insideBox(const BVector &point) const;
      inline bool             rayMightIntersectBox(const BVector &point, const BVector &vector) const;

      void                    sortPoints(void);
      void                    scanHullUpper(void);
      void                    scanHullLower(void);

      static BDynamicSimVectorArray  msScratchPoints;
      static BBitArray        mUsedIndices;
      static BDynamicSimLongArray mUpperIndices;
      static BDynamicSimLongArray mLowerIndices;

      BDynamicSimVectorArray       mPoints;
      BVector                 mBoundingMin, mBoundingMax, mCenter;
};

typedef BDynamicSimArray<BConvexHull> BSimpleConvexHullArray;

#endif

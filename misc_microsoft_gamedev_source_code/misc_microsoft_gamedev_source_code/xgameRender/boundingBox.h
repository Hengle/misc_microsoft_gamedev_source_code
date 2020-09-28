//==============================================================================
// boundingbox.h
//
// Copyright (c) 1997-2001 Ensemble Studios
//==============================================================================

#pragma once 

#include "BoundingSphere.h"
#include "debugPrimitives.h"

//==============================================================================
// 
//==============================================================================
class BPlane;
class BPoint;

//==============================================================================
// rg [1/18/06] - This class is not thread safe.
class BBoundingBox
{
   public:

      


                              BBoundingBox(void);
                              ~BBoundingBox(void);
      void                    initialize(const BVector &minCorner, const BVector &maxCorner);
      void                    initializeTransformed(const BVector &minCorner, const BVector &maxCorner, const BMatrix &matrix);
      void                    translate(const BVector &t);
      void                    transform(const BMatrix &matrix);
                              
      bool                    rayIntersectsSphere(const BVector &origin, const BVector &vector) const;
      bool                    rayIntersectsSphere(const BVector &origin, const BVector &vector, float scale) const;
      bool                    spheresIntersect(const BVector &p, const float radius) const;
                              
      // float *distanceSqr indicates the distance (squared) to check for a collision.  Past that distance it ignores.
      // float &intersectDistanceSqr is the output of the distance (squared) to the intersection.
      bool                    raySegmentIntersects(const BVector &origin, const BVector &vector, bool segment, float *distanceSqr, float &intersectDistanceSqr) const;
      bool                    raySegmentIntersects(const BVector &origin, const BVector &vector, bool segment, float *distanceSqr, float scale, float &intersectDistanceSqr) const;
      bool                    raySegmentIntersectsFirst(const BVector &origin, const BVector &vector, bool segment, float *distanceSqr, float &intersectDistanceSqr) const;

      bool                    fastRayIntersect(const BVector &origin, const BVector &vector) const;

      bool                    lineIntersectsScreenError(const BMatrix &viewMatrix, const BVector &origin, const BVector &vector, float pixelError) const;
                              
      bool                    insideAABB(const BVector &p) const;

      bool                    insideAABBXZ(const BVector &p) const;

      void                    computeCorners(BVector corners[8]) const;
      void                    computeMinCornerAABB(BVector &minCorner) const;
      void                    computeMaxCornerAABB(BVector &maxCorner) const;

      void                    computeWorldCorners(BVector &minCorner, BVector &maxCorner) const;
      void                    computeScreenCorners(BPoint &minCorner, BPoint &maxCorner) const;
                              
      const BVector           &getCenter(void) const { return(mCenter); }
      const BVector           *getAxes(void) const {return(mAxes);}
      const float             *getExtents(void) const {return(mExtents);}
      float                   getSphereRadius() const { return(mSphereRadius); }

      // Pass useCurrentWorldMatrix==true if your bbox points are in local space and you want to set a matrix yourself.
      void                    draw(DWORD color = 0xFFFFFFFF, bool useCurrentWorldMatrix = false, uint category = BDebugPrimitives::cCategoryNone, float timeout = -1.0f) const;   

      bool                    getSidePlanes(BPlane planes[4], bool normalPointingIn=true) const;

      bool                    isZeroVolume() const;

      void                    merge(const BBoundingBox* pRhs);


   protected:
      // TOTALS:                                // 80 bytes
      BVector                 mCenter;          // 16 bytes
      BVector                 mAxes[3];         // 48 bytes
      float                   mExtents[3];      // 12 bytes
      float                   mSphereRadius;    // 4 bytes
      
      // rg [2/23/06] - Not thread safe!
      static BVector          mTransformedCorners[8];
                  
      void                    fixCorners(const BVector &minCorner, BVector &maxCorner);
                                    
      void                    clipFace(BVector **vertices, const float nearZ, const float farZ,
                                 const BVector &rightNormal, const BVector &leftNormal, 
                                 const BVector &topNormal, const BVector &bottomNormal, long clipHint) const;
      void                    clipFaceFront(BVector **vertices, BVector *to, const float nearZ) const;
      void                    clipFaceBack(const float farZ) const;
      void                    clipFacePlaneOrigin(const BVector &n) const;
                              
      static bool             raySegmentIntersects(const BVector corners[8], const BVector &origin, const BVector &vector, 
                                 bool segment, float *distanceSqr);
}; // BBoundingBox            



//==============================================================================
// Copyright (c) 2000-2004 Ensemble Studios
// Copyright (c) Microsoft Corporation. All rights reserved. (BParametricPlane code derived from XGR)
//
// Plane
//==============================================================================

#ifndef _PLANE_H_
#define _PLANE_H_

//==============================================================================
// class BPlane
//==============================================================================
class BPlane
{
   // This is mostly a data class, so all of it's members are public.
   public:
      enum
      {
         cFrontOfPlane,
         cBehindPlane,
         cOnPlane,
      };

                          BPlane();
                          BPlane(const BVector &point1, const BVector &point2, const BVector &point3);
                          BPlane(const BVector &normal, const BVector &point);
                          BPlane(const BPlane& sourcePlane);
                          BPlane(const float a, const float b, const float c, const float d);
                         ~BPlane();

      BPlane&             operator=(const BPlane& sourcePlane);
      bool                operator ==(const BPlane& sourcePlane) const;

      void                assign(const BVector &point1, const BVector &point2, const BVector &point3);
      void                assign(const BVector *vertices, const WORD indices[3]);
      void                assign(const BVector &normal, const BVector &point);
      void                assign(const float a, const float b, const float c, const float d);
      void                normalize();

      long                checkPoint(const BVector &point) const;
      long                checkSphere(const BVector &position, float radius) const;

      float               intersect(const BVector &point, const BVector &vector) const;

      BVector             closest(const BVector &point) const;

      float               calcX(float y, float z) const;
      float               calcY(float x, float z) const;
      float               calcZ(float x, float y) const;
      BVector             getPoint(const BVector2& pt2D) const;

      BVector             origin(void) const { return mNormal * (-mDistance); }
      float               distanceFromPoint(const BVector& pt) const;

      BVector             mNormal;
      float               mDistance;
};

#endif


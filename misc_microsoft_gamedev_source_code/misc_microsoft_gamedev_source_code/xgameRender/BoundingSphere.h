//============================================================================
//
//  BoundingSphere.h
//
//  Copyright (c) 1997-2001, Ensemble Studios
//
//============================================================================
#pragma once 

//----------------------------------------------------------------------------
//  Class BBoundingSphere
//----------------------------------------------------------------------------
class BBoundingSphere
{
public:
   

   //-- Construction/Destruction
   BBoundingSphere ();
   BBoundingSphere (const BVector& center, float radius);
   ~BBoundingSphere();

   //-- Interface
   bool                   rayIntersectsSphere(const BVector& origin, const BVector& vector) const;
   bool                   rayIntersectsSphere(const BVector& origin, const BVector& vector, float scale) const;
   bool                   spheresIntersect   (const BVector& center, float radius) const;

   void                   translate(const BVector& translation);
   void                   debugList() const;
   void                   draw(DWORD color) const;

   //-- Inline Interface
   inline void            initialize(const BVector& center, float radius) { mCenter = center; mRadius = radius; }
   inline void            setCenter (const BVector& center)               { mCenter = center;                   }
   inline void            setRadius (float radius)                        { mRadius = radius;                   }
   inline float           getRadius () const                              { return mRadius;                     }
   inline const BVector&  getCenter () const                              { return mCenter;                     }


protected:

   //-- Protected Data
   BVector                mCenter;
   float                  mRadius;
};


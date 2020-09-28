//=============================================================================
//
//  SphereCoord.h
//
//=============================================================================


#ifndef __SPHERE_COORD_H__
#define __SPHERE_COORD_H__


//-----------------------------------------------------------------------------
//  Class BSphereCoord
//-----------------------------------------------------------------------------
class BSphereCoord
{
public:
   //-- Data
   float mR;     //-- Radius
   float mPhi;   //-- Heading around vertical
   float mTheta; //-- Tilt off vertical

   //-- Construction/Destruction
   BSphereCoord();
   BSphereCoord(float r, float phi, float theta);
   ~BSphereCoord();

   //-- Interface
   void  set       (float r, float phi, float theta);
   void  setDegrees(float r, float phi, float theta);
   void  setVector (const BVector& v);
   void  getVector (BVector& v);
   void  slerp     (BSphereCoord& s1, BSphereCoord& s2, float alpha);
};


#endif

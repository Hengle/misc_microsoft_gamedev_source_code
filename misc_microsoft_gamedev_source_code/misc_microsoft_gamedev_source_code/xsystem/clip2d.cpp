//==============================================================================
// clip2d.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#include "xsystem.h"

//============================================================================
//  PRIVATE CONSTANTS
//============================================================================
static const long OUTSIDE_LEFT   = 0x0001;
static const long OUTSIDE_RIGHT  = 0x0002;
static const long OUTSIDE_TOP    = 0x0004;
static const long OUTSIDE_BOTTOM = 0x0008;


//============================================================================
//  CLIP FUNCTIONS
//============================================================================
//----------------------------------------------------------------------------
//  encode()
//  This is the octant encoding helper function for the Cohen-Sutherland line
//  clipping algorithm.
//----------------------------------------------------------------------------
template <class Type> long encode(Type x, Type z, Type xmin, Type zmin, Type xmax, Type zmax)
{
   long code = 0;

   if (x < xmin)
      code |= OUTSIDE_LEFT;
   else if (x > xmax)
      code |= OUTSIDE_RIGHT;

   if (z < zmin)
      code |= OUTSIDE_TOP;
   else if (z > zmax)
      code |= OUTSIDE_BOTTOM;

   return code;
}


//----------------------------------------------------------------------------
//  clipLine()
//  Uses the Cohen-Sutherland line clipping algorithm.  Based on source from
//  Foley Van Dam, p. 116.
//----------------------------------------------------------------------------
template <class Type> bool templateClipLine(Type &x1, Type &z1, Type &x2, Type &z2, Type xmin, Type zmin, Type xmax, Type zmax)
{
   //-- Make sure our min/maxes are happy.
   Type temp;
   if (xmin > xmax) { temp = xmin; xmin = xmax; xmax = temp; }
   if (zmin > zmax) { temp = zmin; zmin = zmax; zmax = temp; }

   //-- Encode the start and end point. 
   Type x = 0;
   Type z = 0;
   long code;
   long code1 = encode(x1, z1, xmin, zmin, xmax, zmax);
   long code2 = encode(x2, z2, xmin, zmin, xmax, zmax);

   //-- Resolve the line's intersections with the clip regions until we get to where we can
   //-- trivially accept or reject the line.
   for (;;)
   {
      //-- Check for trivial inclusion/exclusion.  If both codes are 0, then
      //-- both points are inside the clip region.  If the logical AND of the
      //-- codes is not 0, that means both endpoints are outside the region in
      //-- such a way that the line segment will not cross the clipping region.
      if (!code1 && !code2)
         return true;
      else if (code1 & code2)
         return false;

      //-- Non trivial case.  Choose a code and resolve its intersection with
      //-- the clipping region.
      code = code1 ? code1 : code2;

      if (code & OUTSIDE_LEFT)
      {
         z = z1 + ((z2-z1) * (xmin-x1)) / (x2-x1);
         x = xmin;
      }
      else if (code & OUTSIDE_RIGHT)
      {
         z = z1 + ((z2-z1) * (xmax-x1)) / (x2-x1);
         x = xmax;
      }
      else if (code & OUTSIDE_TOP)
      {
         x = x1 + ((x2-x1) * (zmin-z1)) / (z2-z1);
         z = zmin;
      }
      else if (code & OUTSIDE_BOTTOM)
      {
         x = x1 + ((x2-x1) * (zmax-z1)) / (z2-z1);
         z = zmax;
      }

      if (code == code1)
      {
         x1 = x;
         z1 = z;
         code1 = encode(x1, z1, xmin, zmin, xmax, zmax);
      }
      else
      {
         x2 = x;
         z2 = z;
         code2 = encode(x2, z2, xmin, zmin, xmax, zmax);
      }
   }

   //-- Actually, the function can never get here, but we gotta have a
   //-- return code anyway.
   //return false;
}


// Make long version from template.
bool clipLine(long &x1, long &z1, long &x2, long &z2, long xmin, long zmin, long xmax, long zmax)
{
   return(templateClipLine<long>(x1, z1, x2, z2, xmin, zmin, xmax, zmax));
}

// Make float version from template.
bool clipLine(float &x1, float &z1, float &x2, float &z2, float xmin, float zmin, float xmax, float zmax)
{
   return(templateClipLine<float>(x1, z1, x2, z2, xmin, zmin, xmax, zmax));
}


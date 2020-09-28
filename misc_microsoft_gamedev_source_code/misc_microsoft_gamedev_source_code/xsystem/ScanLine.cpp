//============================================================================
//
//  ScanLine.cpp
//
//  Copyright (c) 1999-2001, Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xsystem.h"
#include "ScanConversion.h"
#include "clip2d.h"


//============================================================================
//  LINE FUNCTIONS
//============================================================================
//----------------------------------------------------------------------------
//  scanLineForward()
//  Calls the callback pFunc for every point on the line.  Uses Bresenham's
//  algorithm to traverse the line from (x1, z1) to (x2, z2).  If pClip is not
//  NULL the line is clipped against pClip.  Otherwise, no clipping is
//  performed.
//
//  This code is based on source found at:
//  http://graphics.lcs.mit.edu/courses/6.837/F98/Lecture5/Slide20.html
//----------------------------------------------------------------------------
void scanLineForward(long x1, long z1, long x2, long z2, const ClipRect* pClip, FORWARD_LINE_FUNC* pFunc, void* pParam)
{
   //-- This function doesn't accomplish anything without a callback.
   if (pFunc == NULL)
      return;

   //-- Clip it.
   if (pClip)
   {
      if (!clipLine(x1, z1, x2, z2, pClip->xmin, pClip->zmin, pClip->xmax, pClip->zmax))
         return;
   }

   //-- Compute the deltas and step sizes.
   long stepx, stepz;
   long dz = z2 - z1;
   long dx = x2 - x1;
   if (dz < 0) { dz = -dz; stepz = -1; } else { stepz = 1; }
   if (dx < 0) { dx = -dx; stepx = -1; } else { stepx = 1; }
   dz <<= 1;
   dx <<= 1;

   //-- Traverse the line.
   long px = x1;
   long pz = z1;
   pFunc(px, pz, pParam);
   if (dx > dz)
   {
      //-- X dominant line.
      long fraction = dz - (dx >> 1);
      while (px != x2)
      {
         //-- See if we need to increment along the non-dominant axis.
         if (fraction >= 0)
         {
            pz       += stepz;
            fraction -= dx;
         }

         //-- Always increment along the dominant axis.
         px       += stepx;
         fraction += dz;
         pFunc(px, pz, pParam);
      }
   }
   else
   {
      //-- Z dominant line.
      int fraction = dx - (dz >> 1);
      while (pz != z2)
      {
         //-- See if we need to increment along the non-dominant axis.
         if (fraction >= 0)
         {
            px       += stepx;
            fraction -= dz;
         }

         //-- Always increment along the dominant axis.
         pz       += stepz;
         fraction += dx;
         pFunc(px, pz, pParam);
      }
   }
}

//----------------------------------------------------------------------------
//  scanLineSymmetric()
//  Calls the callback pFunc for every point on the line.  Uses the symmetric
//  double step algorithm to traverse the line in both directions at once.
//  If pClip is not NULL the line is clipped against pClip.  Otherwise, no
//  clipping is performed.
// 
//  NOTE:  The original symmetric double step algorithm forces symmetry, and
//  thus sometimes causes a "hump" at the center of the line.  This algorithm
//  corrects the "hump" problem.
//
//  This code is based on source found at:
//  http://graphics.lcs.mit.edu/courses/6.837/F98/Lecture5/Slide22.html
//----------------------------------------------------------------------------
void scanLineSymmetric(long x1, long z1, long x2, long z2, const ClipRect* pClip, SYMMETRIC_LINE_FUNC* pFunc, void* pParam)
{
   //-- This function doesn't accomplish anything without a callback.
   if (pFunc == NULL)
      return;

   //-- Clip it.
   if (pClip)
   {
      if (!clipLine(x1, z1, x2, z2, pClip->xmin, pClip->zmin, pClip->xmax, pClip->zmax))
         return;
   }

   //-- Handle the special case of the line being a point.
   if ((x1 == x2) && (z1 == z2))
   {
      pFunc(x1, z1, false, pParam);
      return;
   }

   //-- Compute the deltas and step sizes.
   int stepx, stepz;
   int dz = z2 - z1;
   int dx = x2 - x1;
   if (dz < 0) { dz = -dz; stepz = -1; } else { stepz = 1; }
   if (dx < 0) { dx = -dx; stepx = -1; } else { stepx = 1; }

   //-- Okay, here's where we thank MIT for providing this code and then we
   //-- consider the matter closed.  In other words, I don't have any idea
   //-- whats going on below here, but it works (after a few bug fixes, their
   //-- code had some typos!).
   pFunc(x1, z1, false, pParam);
   pFunc(x2, z2, true,  pParam);
   if (dx > dz)
   {
      //-- X dominant line.
      int length = (dx - 1) >> 2;
      int extras = (dx - 1) & 3;
      int incr2  = (dz << 2) - (dx << 1);
      if (incr2 < 0)
      {
         int c     = dz << 1;
         int incr1 = c << 1;
         int d     = incr1 - dx;
         for (long i = 0; i < length; i++)
         {
            x1 += stepx;
            x2 -= stepx;
            if (d < 0)
            {                                          // Pattern:
               pFunc(x1,          z1, false, pParam);  //
               pFunc(x1 += stepx, z1, false, pParam);  //  x o o
               pFunc(x2,          z2, true,  pParam);  //
               pFunc(x2 -= stepx, z2, true,  pParam);
               d += incr1;
            }
            else
            {
               if (d < c)
               {                                                   // Pattern:
                  pFunc(x1,          z1,          false, pParam);  //      o
                  pFunc(x1 += stepx, z1 += stepz, false, pParam);  //  x o
                  pFunc(x2,          z2,          true,  pParam);  //
                  pFunc(x2 -= stepx, z2 -= stepz, true,  pParam);
               }
               else
               {
                  pFunc(x1,          z1 += stepz, false, pParam);  // Pattern:
                  pFunc(x1 += stepx, z1,          false, pParam);  //    o o 
                  pFunc(x2,          z2 -= stepz, true,  pParam);  //  x
                  pFunc(x2 -= stepx, z2,          true,  pParam);  //
               }
               d += incr2;
            }
         }
         if (extras > 0)
         {
            if (d < 0)
            {
               pFunc(x1 += stepx, z1, false, pParam);
               if (extras > 1) pFunc(x1 += stepx, z1, false, pParam);
               if (extras > 2) pFunc(x2 -= stepx, z2, true,  pParam);
            }
            else if (d < c)
            {
               pFunc(x1 += stepx, z1, false, pParam);
               if (extras > 1) pFunc(x1 += stepx, z1 += stepz, false, pParam);
               if (extras > 2) pFunc(x2 -= stepx, z2         , true,  pParam);
            }
            else
            {
               pFunc(x1 += stepx, z1 += stepz, false, pParam);
               if (extras > 1) pFunc(x1 += stepx, z1,          false, pParam);
               if (extras > 2) pFunc(x2 -= stepx, z2 -= stepz, true,  pParam);
            }
         }
      }
      else
      {
         int c     = (dz - dx) << 1;
         int incr1 = c << 1;
         int d     = incr1 + dx;
         for (int i = 0; i < length; i++)
         {
            x1 += stepx;
            x2 -= stepx;
            if (d > 0)
            {
               pFunc(x1,          z1 += stepz, false, pParam);  // Pattern:
               pFunc(x1 += stepx, z1 += stepz, false, pParam);  //      o
               pFunc(x2,          z2 -= stepz, true,  pParam);  //    o
               pFunc(x2 -= stepx, z2 -= stepz, true,  pParam);	 //  x
               d += incr1;
            }
            else
            {
               if (d < c)
               {
                  pFunc(x1,          z1,          false, pParam);  // Pattern:
                  pFunc(x1 += stepx, z1 += stepz, false, pParam);  //      o
                  pFunc(x2,          z2,          true,  pParam);  //  x o
                  pFunc(x2 -= stepx, z2 -= stepz, true,  pParam);  //
               }
               else
               {
                  pFunc(x1,          z1 += stepz, false, pParam);  // Pattern:
                  pFunc(x1 += stepx, z1,          false, pParam);  //    o o
                  pFunc(x2,          z2 -= stepz, true,  pParam);  //  x
                  pFunc(x2 -= stepx, z2,          true,  pParam);  //
               }
               d += incr2;
            }
         }
         if (extras > 0)
         {
            if (d > 0)
            {
               pFunc(x1 += stepx, z1 += stepz, false, pParam);
               if (extras > 1) pFunc(x1 += stepx, z1 += stepz, false, pParam);
               if (extras > 2) pFunc(x2 -= stepx, z2 -= stepz, true,  pParam);
            }
            else if (d < c)
            {
               pFunc(x1 += stepx, z1, false, pParam);
               if (extras > 1) pFunc(x1 += stepx, z1 += stepz, false, pParam);
               if (extras > 2) pFunc(x2 -= stepx, z2,          true,  pParam);
            }
            else
            {
               pFunc(x1 += stepx, z1 += stepz, false, pParam);
               if (extras > 1) pFunc(x1 += stepx, z1, false, pParam);
               if (extras > 2)
               {
                  if (d > c) pFunc(x2 -= stepx, z2 -= stepz, true, pParam);
                  else       pFunc(x2 -= stepx, z2,          true, pParam);
               }
            }
         }
      }
   }
   else
   {
      //-- z dominant line.
      int length = (dz - 1) >> 2;
      int extras = (dz - 1) & 3;
      int incr2  = (dx << 2) - (dz << 1);
      if (incr2 < 0)
      {
         int c     = dx << 1;
         int incr1 = c << 1;
         int d     = incr1 - dz;
         for (int i = 0; i < length; i++)
         {
            z1 += stepz;
            z2 -= stepz;
            if (d < 0)
            {
               pFunc(x1, z1,          false, pParam);
               pFunc(x1, z1 += stepz, false, pParam);
               pFunc(x2, z2,          true,  pParam);
               pFunc(x2, z2 -= stepz, true,  pParam);
               d += incr1;
            }
            else
            {
               if (d < c)
               {
                  pFunc(x1,          z1,          false, pParam);
                  pFunc(x1 += stepx, z1 += stepz, false, pParam);
                  pFunc(x2,          z2,          true,  pParam);
                  pFunc(x2 -= stepx, z2 -= stepz, true,  pParam);
               }
               else
               {
                  pFunc(x1 += stepx, z1,          false, pParam);
                  pFunc(x1,          z1 += stepz, false, pParam);
                  pFunc(x2 -= stepx, z2,          true,  pParam);
                  pFunc(x2,          z2 -= stepz, true,  pParam);
               }
               d += incr2;
            }
         }
         if (extras > 0)
         {
            if (d < 0)
            {
               pFunc(x1, z1 += stepz, false, pParam);
               if (extras > 1) pFunc(x1, z1 += stepz, false, pParam);
               if (extras > 2) pFunc(x2, z2 -= stepz, true,  pParam);
            }
            else if (d < c)
            {
               pFunc(x1, z1 += stepz, false, pParam);
               if (extras > 1) pFunc(x1 += stepx, z1 += stepz, false, pParam);
               if (extras > 2) pFunc(x2,          z2 -= stepz, true,  pParam);
            }
            else
            {
               pFunc(x1 += stepx, z1 += stepz, false, pParam);
               if (extras > 1) pFunc(x1,          z1 += stepz, false, pParam);
               if (extras > 2) pFunc(x2 -= stepx, z2 -= stepz, true,  pParam);
            }
         }
      }
      else
      {
         int c     = (dx - dz) << 1;
         int incr1 = c << 1;
         int d     = incr1 + dz;
         for (int i = 0; i < length; i++)
         {
            z1 += stepz;
            z2 -= stepz;
            if (d > 0)
            {
               pFunc(x1 += stepx, z1,          false, pParam);
               pFunc(x1 += stepx, z1 += stepz, false, pParam);
               pFunc(x2 -= stepx, z2,          true,  pParam);
               pFunc(x2 -= stepx, z2 -= stepz, true,  pParam);
               d += incr1;
            }
            else
            {
               if (d < c)
               {
                  pFunc(x1,          z1,          false, pParam);
                  pFunc(x1 += stepx, z1 += stepz, false, pParam);
                  pFunc(x2,          z2,          true,  pParam);
                  pFunc(x2 -= stepx, z2 -= stepz, true,  pParam);
               }
               else
               {
                  pFunc(x1 += stepx, z1,          false, pParam);
                  pFunc(x1,          z1 += stepz, false, pParam);
                  pFunc(x2 -= stepx, z2,          true,  pParam);
                  pFunc(x2,          z2 -= stepz, true,  pParam);
               }
               d += incr2;
            }
         }
         if (extras > 0)
         {
            if (d > 0)
            {
               pFunc(x1 += stepx, z1 += stepz, false, pParam);
               if (extras > 1) pFunc(x1 += stepx, z1 += stepz, false, pParam);
               if (extras > 2) pFunc(x2 -= stepx, z2 -= stepz, true,  pParam);
            }
            else if (d < c)
            {
               pFunc(x1, z1 += stepz, false, pParam);
               if (extras > 1) pFunc(x1 += stepx, z1 += stepz, false, pParam);
               if (extras > 2) pFunc(x2,          z2 -= stepz, true,  pParam);
            }
            else
            {
               pFunc(x1 += stepx, z1 += stepz, false, pParam);
               if (extras > 1) pFunc(x1, z1 += stepz, false, pParam);
               if (extras > 2)
               {
                  if (d > c) pFunc(x2 -= stepx, z2 -= stepz, true, pParam);
                  else       pFunc(x2,          z2 -= stepz, true, pParam);
               }
            }
         }
      }
   }
}







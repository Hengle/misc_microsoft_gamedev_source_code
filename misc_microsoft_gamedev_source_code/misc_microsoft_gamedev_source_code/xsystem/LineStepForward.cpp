//============================================================================
//
//  LineStepForward.cpp
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
//  STEP FUNCTION PROTOTYPE (ONLY FOR INTERNAL USE)
//============================================================================
typedef bool (CALLBACK STEP_FUNC)(void* pParam);


//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
LineStepForward::LineStepForward()
{
   clear();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LineStepForward::~LineStepForward()
{
}


//============================================================================
//  INTERFACE
//============================================================================
void LineStepForward::init(long x1, long z1, long x2, long z2, ClipRect* pClip)
{
   clear();

   //-- Clip it.
   if (pClip)
   {
      //-- If the line is fully clipped out, we MUST abort here.
      if (!clipLine(x1, z1, x2, z2, pClip->xmin, pClip->zmin, pClip->xmax, pClip->zmax))
         return;
   }

   //-- Compute the deltas and step sizes.
   mx1 = x1;
   mz1 = z1;
   mx2 = x2;
   mz2 = z2;
   mpx = x1;
   mpz = z1;
   mdz = z2 - z1;
   mdx = x2 - x1;
   if (mdz < 0) { mdz = -mdz; mstepz = -1; } else { mstepz = 1; }
   if (mdx < 0) { mdx = -mdx; mstepx = -1; } else { mstepx = 1; }
   mdz <<= 1;
   mdx <<= 1;
   mbFirstTime = true;
}

//----------------------------------------------------------------------------
//  step()
//  Call this after you have inited the line.  If this returns true, the
//  values returned in x, z specify a valid point on the line.  If this
//  returns false, there are no more points on the line.  To fully trace a
//  line, keep calling this function until it returns false.
//----------------------------------------------------------------------------
bool LineStepForward::step(long& x, long& z)
{
   //-- See if its our first step.
   if (mbFirstTime)
   {
      x = mpx;
      z = mpz;

      if (mdx > mdz)
         mfraction  = mdz - (mdx >> 1);
      else
         mfraction  = mdx - (mdz >> 1);

      mbFirstTime = false;
      return true;
   }

   if (mdx > mdz)
   {
      //-- See if we are done.
      if (mpx == mx2)
         return false;

      //-- See if we need to increment along the non-dominant axis.
      if (mfraction >= 0)
      {
         mpz       += mstepz;
         mfraction -= mdx;
      }

      //-- Always increment along the dominant axis.
      mpx       += mstepx;
      mfraction += mdz;

      x = mpx;
      z = mpz;

      return true;
   }
   else
   {
      //-- See if we are done.
      if (mpz == mz2)
         return false;


      //-- See if we need to increment along the non-dominant axis.
      if (mfraction >= 0)
      {
         mpx       += mstepx;
         mfraction -= mdz;
      }

      //-- Always increment along the dominant axis.
      mpz       += mstepz;
      mfraction += mdx;

      x = mpx;
      z = mpz;

      return true;
   }
}


//============================================================================
//  PRIVATE FUNCTIONS
//============================================================================
void LineStepForward::clear()
{
   mx1        = 0;
   mz1        = 0;
   mx2        = 0;
   mz2        = 0;
   mpx        = 0;
   mpz        = 0;
   mdx        = 0;
   mdz        = 0;
   mstepx     = 0;
   mstepz     = 0;
   mfraction  = 0;
//   mpStepFunc = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool LineStepForward::stepXDominant()
{
   //-- See if we are done.
   if (mpx == mx2)
      return false;

   //-- See if we need to increment along the non-dominant axis.
   if (mfraction >= 0)
   {
      mpz       += mstepz;
      mfraction -= mdx;
   }

   //-- Always increment along the dominant axis.
   mpx       += mstepx;
   mfraction += mdz;
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool LineStepForward::stepZDominant()
{
   //-- See if we are done.
   if (mpz == mz2)
      return false;


   //-- See if we need to increment along the non-dominant axis.
   if (mfraction >= 0)
   {
      mpx       += mstepx;
      mfraction -= mdz;
   }

   //-- Always increment along the dominant axis.
   mpz       += mstepz;
   mfraction += mdx;
   return true;
}


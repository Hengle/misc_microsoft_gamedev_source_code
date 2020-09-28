//============================================================================
//
//  LineStepForward.h
//
//  This class implements bresenham's line drawing algorithm in a steppable
//  version.  This means that you ask for each point along the way explicitly,
//  rather than specifying a callback and getting all the points at once.
//  This is useful for tracing several lines in parallel, such as scanning
//  two sides of a triangle at the same time.
//
//  Copyright (c) 1999-2001, Ensemble Studios
//
//============================================================================


#ifndef __LINE_STEP_FORWARD_H__
#define __LINE_STEP_FORWARD_H__


//----------------------------------------------------------------------------
//  Externs
//----------------------------------------------------------------------------
struct ClipRect;


//----------------------------------------------------------------------------
//  Class LineStepForward
//----------------------------------------------------------------------------
class LineStepForward
{
public:
   //-- Construction/Destruction
   LineStepForward();
   ~LineStepForward();

   //-- Interface
   void init(long x1, long z1, long x2, long z2, ClipRect* pClip);
   bool step(long& x, long& z);

   //-- Inlines
   inline long x1()   { return mx1; }
   inline long z1()   { return mz1; }
   inline long x2()   { return mx2; }
   inline long z2()   { return mz2; }
   inline long curX() { return mpx; }
   inline long curZ() { return mpz; }

private:
   //-- Private Data
   long  mx1, mz1, mx2, mz2;
   long  mpx, mpz;
   long  mdx, mdz;
   long  mstepx, mstepz;
   long  mfraction;
   bool  mbFirstTime;

   //-- Private Functions
   void clear();
   bool stepXDominant();
   bool stepZDominant();
};


#endif




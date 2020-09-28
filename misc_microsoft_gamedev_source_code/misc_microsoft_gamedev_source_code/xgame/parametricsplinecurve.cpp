//==============================================================================
// parametricsplinecurve.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "parametricsplinecurve.h"
#include "gamefilemacros.h"

//==============================================================================
//==============================================================================
void BParametricSplineCurve::reset()
{
   mA[0].set(0.0f, 0.0f, 0.0f);
   mA[1].set(0.0f, 0.0f, 0.0f);
   mA[2].set(0.0f, 0.0f, 0.0f);
   mValid = false;
}

//==============================================================================
//==============================================================================
void BParametricSplineCurve::init(const BVector& start, const BVector& middle, const BVector& end)
{
   BVector p[3];

   p[0] = start;
   p[1] = middle;
   p[2] = end;

   mA[0] = p[0];
   mA[2] = ((p[1] - p[0]) - 0.5f * (p[2] - p[0])) / -0.25f;
   mA[1] = p[2] - p[0] - mA[2];

   mValid = true;
}

//==============================================================================
//==============================================================================
BVector BParametricSplineCurve::evaluate(float t)
{
   return ((mA[2] * t * t) + (mA[1] * t) + mA[0]);
}

//==============================================================================
//==============================================================================
bool BParametricSplineCurve::save(BStream* pStream, int saveType) const
{
   GFWRITEPTR(pStream, sizeof(BVector)*3, mA);
   GFWRITEVAR(pStream, bool, mValid);
   return true;
}

//==============================================================================
//==============================================================================
bool BParametricSplineCurve::load(BStream* pStream, int saveType)
{
   GFREADPTR(pStream, sizeof(BVector)*3, mA);
   GFREADVAR(pStream, bool, mValid);
   return true;
}

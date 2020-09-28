//==============================================================================
// aimissionscore.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#include "common.h"
#include "aidebug.h"
#include "aimissionscore.h"

GFIMPLEMENTVERSION(BAIMissionScore, 1);

//==============================================================================
// applyBias
// Adjust the number to modify in a sigmoid-like fashion.  That is, small numbers
// are expanded by a fraction of their value up to doubling.  Numbers near max are
// expanded by a fraction of the remaining headroom.  For example,
// applyBias(0.7, 0, 1, 0.6) gives 0.88, i.e. 60% of the way to the top.
// applyBias(0.1, 0, 1, 0.6) gives 0.16, i.e. added 60% of the initial value, or
// increased distance from the min by 60%.  
// Essentially, this applies the smaller of two adjustments:  the Bias fraction of
// headroom, or the Bias fraction of footroom.  
// A bias of 1.0 will double the number (for values below the min->max midpoint),
// and will return max if the number is above the min->max midpoint.
//==============================================================================

float BAIMissionScore::applyBias(float numberToModify, float min, float max, float bias)
{
   float headRoomDelta = (max - numberToModify) * bias;
   float footRoomDelta = (numberToModify - min) * bias;

   if (Math::fAbs(headRoomDelta) > Math::fAbs(footRoomDelta))
      return(numberToModify + footRoomDelta);
   else
      return(numberToModify + headRoomDelta);
}

//==============================================================================
// BAIMissionScore::save
//==============================================================================
bool BAIMissionScore::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, float, mTotal);
   GFWRITEVAR(pStream, float, mInstance);
   GFWRITEVAR(pStream, float, mClass);
   GFWRITEVAR(pStream, float, mAfford);
   GFWRITEVAR(pStream, float, mPermission);
   return true;
}

//==============================================================================
// BAIMissionScore::load
//==============================================================================
bool BAIMissionScore::load(BStream* pStream, int saveType)
{  
   GFREADVAR(pStream, float, mTotal);
   GFREADVAR(pStream, float, mInstance);
   GFREADVAR(pStream, float, mClass);
   GFREADVAR(pStream, float, mAfford);
   GFREADVAR(pStream, float, mPermission);
   return true;
}

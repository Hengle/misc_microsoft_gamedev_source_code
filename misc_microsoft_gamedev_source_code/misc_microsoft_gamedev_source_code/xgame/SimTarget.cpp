//==============================================================================
// SimTarget.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "SimTarget.h"
#include "gamefile.h"
#include "savegame.h"

GFIMPLEMENTVERSION(BSimTarget,1);

//==============================================================================
//==============================================================================
bool BSimTarget::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mPosition);
   GFWRITEVAR(pStream, BEntityID, mID);
   GFWRITEVAR(pStream, float, mRange);
   GFWRITEVAR(pStream, long, mAbilityID);

   GFWRITEBITBOOL(pStream, mPositionValid);
   GFWRITEBITBOOL(pStream, mRangeValid);
   GFWRITEBITBOOL(pStream, mAbilityIDValid);
   return true;
}

//==============================================================================
//==============================================================================
bool BSimTarget::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mPosition);
   GFREADVAR(pStream, BEntityID, mID);
   GFREADVAR(pStream, float, mRange);
   GFREADVAR(pStream, long, mAbilityID);

   GFREADBITBOOL(pStream, mPositionValid);
   GFREADBITBOOL(pStream, mRangeValid);
   GFREADBITBOOL(pStream, mAbilityIDValid);

   gSaveGame.remapAbilityID(mAbilityID);
   return true;
}

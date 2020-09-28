//==============================================================================
// unitactionrevealtoteam.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "MaximumSupportedPlayers.h"
#include "unit.h"
#include "unitactionrevealtoteam.h"
#include "visiblemap.h"
#include "uimanager.h"
#include "minimap.h"
#include "configsgame.h"
#include "usermanager.h"
#include "user.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionRevealToTeam, 5, &gSimHeap);


//==============================================================================
//==============================================================================
void BUnitActionRevealToTeam::syncWithUnit()
{
//-- FIXING PREFIX BUG ID 4915
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   // Fixes Bug PHX-11698
   float revealRadius = pUnit->getObstructionRadius();
   const BProtoObject* pProtoObject = pUnit->getProtoObject();
   if (pProtoObject)
   {
      float protoRevealRadius = pProtoObject->getRevealRadius();
      if (protoRevealRadius > cFloatCompareEpsilon)
         revealRadius = protoRevealRadius;
   }

   mRevealRadius = Math::Max(revealRadius, gDatabase.getMinimumRevealerSize());
   
   mSimRevealRadius = (long)(Math::Max(mRevealRadius * gTerrainSimRep.getReciprocalDataTileScale(), 1.0f));
   XMVECTOR simPosition = __vctsxs(XMVectorMultiply(pUnit->getPosition(), XMVectorReplicate(gTerrainSimRep.getReciprocalDataTileScale())), 0);
   mRevealX = simPosition.u[0];
   mRevealZ = simPosition.u[2];
}


//==============================================================================
//==============================================================================
bool BUnitActionRevealToTeam::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   // Init variables
   syncWithUnit();
   mTotalRevealCount = 0;
   Utils::FastMemSet(mRevealCount, 0, sizeof(mRevealCount));

   // Go to working state
   setState(cStateWorking);

   return (true);
}


//==============================================================================
//==============================================================================
void BUnitActionRevealToTeam::disconnect(void)
{
   BASSERT(mpOwner);

   if (mTotalRevealCount > 0)
   {
      // Iterate through teams and unreveal
      for (uint i = 1; i < cMaximumSupportedTeams; i++)
      {
         // unreveal
         if (mRevealCount[i] > 0)
         {
            gVisibleMap.unexploreCircularRegion(mRevealX, mRevealZ, mSimRevealRadius, i);

            mTotalRevealCount -= mRevealCount[i];
            mRevealCount[i] = 0;
         }
      }
   }

   BASSERT(mTotalRevealCount == 0);

   return (BAction::disconnect());
}


//==============================================================================
//==============================================================================
bool BUnitActionRevealToTeam::update(float elapsed)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   if (mState == cStateWorking)
   {
      // If we're done, kill the action
      if (mTotalRevealCount == 0)
      {
         setState(cStateDone);
         return true;
      }

      // Get new unit position
      BVector position = pUnit->getPosition();
      long oldRevealX = mRevealX;
      long oldRevealZ = mRevealZ;
      long oldRevealRadius = mSimRevealRadius;
      syncWithUnit();

      // Don't bother updating visiblemap on a unit that hasn't moved
      bool updateVisibleMap = ((oldRevealX != mRevealX) || (oldRevealZ != mRevealZ) || (oldRevealRadius != mSimRevealRadius));

      // Iterate through teams and reveal
      for (uint i = 1; i < cMaximumSupportedTeams; i++)
      {
         // Reveal to those that need to see us
         if (mRevealCount[i] > 0)
         {
            const BUser * const user = gUserManager.getUser(BUserManager::cPrimaryUser);
            if (user->getTeamID() == (BTeamID)i)
            {
               //if (!gConfig.isDefined(cConfigFlashGameUI))
               //   gMiniMap.reveal(position, mRevealRadius);
               //else
               gUIManager->revealMinimap(position, mRevealRadius);
            }

            if (updateVisibleMap)
            {
               if (oldRevealRadius == mSimRevealRadius)
               {
                  // No radius change. Do quick update.
                  gVisibleMap.updateCircularRegion(oldRevealX, oldRevealZ, mRevealX, mRevealZ, mSimRevealRadius, i);
               }
               else
               {
                  // The radius changed. Unexplore then re-explore.
                  gVisibleMap.unexploreCircularRegion(oldRevealX, oldRevealZ, oldRevealRadius, i);
                  gVisibleMap.exploreCircularRegion(mRevealX, mRevealZ, mSimRevealRadius, i);
               }
            }
         }
      }
   }

   return true;
}


//==============================================================================
//==============================================================================
void BUnitActionRevealToTeam::addReveal(BTeamID teamID)
{
   BASSERT(teamID < cMaximumSupportedTeams);
   BASSERT(mTotalRevealCount >= 0);
   BASSERT(mRevealCount[teamID] >= 0);

   // SLB: We don't reveal for Gaia
   if (teamID <= 0)
      return;

   // Explore if we don't currently reveal for this team
   if (mRevealCount[teamID] == 0)
      gVisibleMap.exploreCircularRegion(mRevealX, mRevealZ, mSimRevealRadius, teamID);

   // Inc reveal count
   mTotalRevealCount++;
   mRevealCount[teamID]++;
}


//==============================================================================
//==============================================================================
void BUnitActionRevealToTeam::removeReveal(BTeamID teamID)
{
   BASSERT(teamID < cMaximumSupportedTeams);

   // SLB: We don't reveal for Gaia
   if (teamID <= 0)
      return;

   // Prevent a reveal count underflow
   if (mRevealCount[teamID] > 0)
   {
      // Dec reveal count
      mRevealCount[teamID]--;
      mTotalRevealCount--;

      // Unexplore if we no longer reveal for this team
      if (mRevealCount[teamID] == 0)
         gVisibleMap.unexploreCircularRegion(mRevealX, mRevealZ, mSimRevealRadius, teamID);

      BASSERT(mTotalRevealCount >= 0);
      BASSERT(mRevealCount[teamID] >= 0);
   }
}


//==============================================================================
//==============================================================================
bool BUnitActionRevealToTeam::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITEVAR(pStream, float, mRevealRadius);
   GFWRITEVAR(pStream, long, mSimRevealRadius);
   GFWRITEVAR(pStream, long, mRevealX);
   GFWRITEVAR(pStream, long, mRevealZ);
   GFWRITEVAR(pStream, long, mTotalRevealCount);

   GFWRITEVAL(pStream, long, cMaximumSupportedTeams);
   for (long i=0; i<cMaximumSupportedTeams; i++)
      GFWRITEVAR(pStream, long, mRevealCount[i]);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionRevealToTeam::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADVAR(pStream, float, mRevealRadius);
   GFREADVAR(pStream, long, mSimRevealRadius);
   GFREADVAR(pStream, long, mRevealX);
   GFREADVAR(pStream, long, mRevealZ);
   GFREADVAR(pStream, long, mTotalRevealCount);

   long maxTeams;
   GFREADVAR(pStream, long, maxTeams);
   GFVERIFYCOUNT(maxTeams, 16);
   for (long i=0; i<maxTeams; i++)
   {
      int count;
      GFREADVAR(pStream, long, count);
      if (i < cMaximumSupportedTeams)
      {
         mRevealCount[i] = count;
         if (count > 0)
            gVisibleMap.exploreCircularRegion(mRevealX, mRevealZ, mSimRevealRadius, i);
      }
   }

   return true;
}

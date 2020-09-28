//==============================================================================
// aimissiontarget.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aidebug.h"
#include "aimission.h"
#include "aimissiontarget.h"
#include "alert.h"
#include "configsgame.h"
#include "kb.h"
#include "player.h"
#include "world.h"

GFIMPLEMENTVERSION(BAIMissionTarget, 2);

//==============================================================================
//==============================================================================
BAIMissionTarget::BAIMissionTarget()
{
   mID = 0;
   resetNonIDData();
}


//==============================================================================
//==============================================================================
BAIMissionTarget::~BAIMissionTarget()
{
}

//==============================================================================
//==============================================================================
void BAIMissionTarget::resetNonIDData()
{
   mPosition.zero();
   mScore.zero();
   mRadius = 0.0f;
   mPlayerID = cInvalidPlayerID;
   mKBBaseID = cInvalidKBBaseID;
   mTargetType = MissionTargetType::cInvalid;
   mMissionType = MissionType::cInvalid;
   mWrapperRefs.clear();
   mbDestroyOnNoRefs = false;
   mFlagAllowScoring = true;
#ifndef BUILD_FINAL
   mName.empty();
   mDebugID = 0;
#endif
}


//==============================================================================
//==============================================================================
uint BAIMissionTarget::getNumFlareAlerts() const
{   
   uint numFlareAlerts = 0;

   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (pPlayer)
   {
      float radiusSqr = mRadius * mRadius;
      BAlertManager *pAM = pPlayer->getAlertManager();
      const BAlertIDArray alertIDs = pAM->getAlertIDs();
      uint numAlertIDs = alertIDs.getSize();
      BVector targetPos = this->getPosition();
      for (uint i=0; i<numAlertIDs; i++)
      {
//-- FIXING PREFIX BUG ID 2106
         const BAlert* pAlert = pAM->getAlert(alertIDs[i]);
//--
         if (!pAlert)
            continue;
         if (pAlert->getType() != AlertType::cFlare)
            continue;
         if (pAlert->getQueued())
            continue;
         if (pAlert->getLocation().xzDistanceSqr(targetPos) > radiusSqr)
            continue;

         numFlareAlerts++;
      }
   }

   return (numFlareAlerts);
}

//==============================================================================
//==============================================================================
uint BAIMissionTarget::getNumAttackAlerts() const
{
   uint numAttackAlerts = 0;

   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (pPlayer)
   {
      float radiusSqr = mRadius * mRadius;
      BAlertManager *pAM = pPlayer->getAlertManager();
      const BAlertIDArray alertIDs = pAM->getAlertIDs();
      uint numAlertIDs = alertIDs.getSize();
      BVector targetPos = this->getPosition();
      for (uint i=0; i<numAlertIDs; i++)
      {
//-- FIXING PREFIX BUG ID 2107
         const BAlert* pAlert = pAM->getAlert(alertIDs[i]);
//--
         if (!pAlert)
            continue;
         if (pAlert->getType() != AlertType::cAttack)
            continue;
         if (pAlert->getQueued())
            continue;
         if (pAlert->getLocation().xzDistanceSqr(targetPos) > radiusSqr)
            continue;

         numAttackAlerts++;
      }
   }

   return (numAttackAlerts);
}

#ifndef BUILD_FINAL
void BAIMissionTarget::debugRender() const
{

}
#endif

//==============================================================================
// BAIMissionTarget::save
//==============================================================================
bool BAIMissionTarget::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mPosition);
   GFWRITEVAR(pStream, float, mRadius);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITECLASS(pStream, saveType, mScore);
   GFWRITEVAR(pStream, BKBBaseID, mKBBaseID);
   GFWRITEVAR(pStream, BAIMissionTargetType, mTargetType);
   GFWRITEVAR(pStream, BAIMissionType, mMissionType);
   GFWRITEVAR(pStream, BAIMissionTargetID, mID);
   GFWRITEARRAY(pStream, BAIMissionTargetWrapperID, mWrapperRefs, uint16, 1000);

   GFWRITEBITBOOL(pStream, mbDestroyOnNoRefs);
   GFWRITEBITBOOL(pStream, mFlagAllowScoring);

#ifndef BUILD_FINAL  
   GFWRITESTRING(pStream, BSimString, mName, 100);
   GFWRITEVAR(pStream, uint, mDebugID);
#else
   BSimString tempStr;
   GFWRITESTRING(pStream, BSimString, tempStr, 100);
   GFWRITEVAL(pStream, uint, 0);
#endif

   return true;
}

//==============================================================================
// BAIMissionTarget::load
//==============================================================================
bool BAIMissionTarget::load(BStream* pStream, int saveType)
{  
   GFREADVECTOR(pStream, mPosition);
   GFREADVAR(pStream, float, mRadius);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADCLASS(pStream, saveType, mScore);
   GFREADVAR(pStream, BKBBaseID, mKBBaseID);
   GFREADVAR(pStream, BAIMissionTargetType, mTargetType);
   GFREADVAR(pStream, BAIMissionType, mMissionType);
   GFREADVAR(pStream, BAIMissionTargetID, mID);
   GFREADARRAY(pStream, BAIMissionTargetWrapperID, mWrapperRefs, uint16, 1000);

   GFREADBITBOOL(pStream, mbDestroyOnNoRefs);

   mFlagAllowScoring = true;
   if (BAIMissionTarget::mGameFileVersion >= 2)
      GFREADBITBOOL(pStream, mFlagAllowScoring);

#ifndef BUILD_FINAL  
   GFREADSTRING(pStream, BSimString, mName, 100);
   GFREADVAR(pStream, uint, mDebugID);
#else
   BSimString tempStr;
   GFREADSTRING(pStream, BSimString, tempStr, 100);
   GFREADTEMPVAL(pStream, uint);
#endif

   return true;
}

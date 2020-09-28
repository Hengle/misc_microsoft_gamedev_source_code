//==============================================================================
// aipowermission.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aidebug.h"
#include "aipowermission.h"
#include "kb.h"
#include "power.h"
#include "powercarpetbombing.h"
#include "powercleansing.h"
#include "powerorbital.h"
#include "powerrage.h"
#include "powerrepair.h"
#include "powermanager.h"
#include "powerwave.h"
#include "simhelper.h"
#include "squad.h"
#include "tactic.h"
#include "unitquery.h"
#include "world.h"

GFIMPLEMENTVERSION(BAIPowerMission, 3);

//==============================================================================
//==============================================================================
void BAIPowerMission::resetNonIDData()
{
   // Do the base reset.
   BAIMission::resetNonIDData();

   // Power mission specific reset.
   mType = MissionType::cPower;
   mPowerID = cInvalidPowerID;
   mOwnerID = cInvalidObjectID;
   mTimestampNextInput = 0;
   mTimeElapsedSinceLastInput = 0.0f;

   mTimeElapsedSinceLastRageJump = 0;
   mTimeElapsedSinceLastRageMove = 0;
}


//==============================================================================
//==============================================================================
void BAIPowerMission::update(DWORD currentGameTime)
{
   switch (mState)
   {
   case MissionState::cCreate:
      {
         updateStateCreate(currentGameTime);
         break;
      }
   //case MissionState::cRally:
   //case MissionState::cTransit:
   case MissionState::cWorking:
      {
         updateStateWorking(currentGameTime);
         break;
      }
   case MissionState::cSuccess:
   case MissionState::cFailure:
   case MissionState::cInvalid:
      {
         BAI* pAI = gWorld->getAI(mPlayerID);
         if (pAI)
            pAI->AIQTopicLotto(currentGameTime);
         break;
      }
   }
}


//==============================================================================
//==============================================================================
void BAIPowerMission::updateStateCreate(DWORD currentGameTime)
{
   if (processTerminalConditions(currentGameTime))
      return;

   // Initialize some stuff
   const BPower* pPower = gWorld->getPowerManager()->getPowerByID(mPowerID);
   if (pPower)
   {
      mPreviousPosition = pPower->getTargetLocation();
      mTimeElapsedSinceLastInput = 0.0f;
      mTimestampNextInput = currentGameTime + 3000;
   }

   setState(MissionState::cWorking);
   return;
}


//==============================================================================
//==============================================================================
void BAIPowerMission::updateStateWorking(DWORD currentGameTime)
{
   if (!mFlagActive)
      return;

   // Check for lots of stuff that means this mission is over.
   if (processTerminalConditions(currentGameTime))
      return;

   // This is checked in process terminal conditions so should never be null.
   BPower* pPower = gWorld->getPowerManager()->getPowerByID(mPowerID);
   BASSERT(pPower);
   if (!pPower)
      return;

   BPowerType powerType = pPower->getType();
   switch (powerType)
   {
      case PowerType::cCleansing:
      {
         updateCleansing(static_cast<BPowerCleansing*>(pPower), currentGameTime);
         break;
      }
      case PowerType::cRage:
      {
         updateRage(static_cast<BPowerRage*>(pPower), currentGameTime);
         break;
      }
      case PowerType::cWave:
      {
         updateWave(static_cast<BPowerWave*>(pPower), currentGameTime);
         break;
      }
      case PowerType::cOrbital:
      {
         updateOrbital(static_cast<BPowerOrbital*>(pPower), currentGameTime);
         break;
      }
      default:
      {
         BFAIL("Unsupported power type in AIPowerMission::updateStateWorking");
         break;
      }
   }
}


//==============================================================================
//==============================================================================
void BAIPowerMission::updateCleansing(BPowerCleansing* pPower, DWORD currentGameTime)
{
   if (!pPower)
      return;

   mTimeElapsedSinceLastInput += gWorld->getLastUpdateLengthFloat();

   // MARCHACK - Hard-code max time to 10 seconds.
   if (pPower->getElapsed() >= 10.0)
   {
      BPowerInput powerInput;
      powerInput.mType = PowerInputType::cUserCancel;
      pPower->submitInput(powerInput);
      return;
   }

   const BSquad* pBeamSquad = pPower->getBeamSquad();
   if (!pBeamSquad)
      return;

   // Look at the beam if we aren't already.
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (pPlayer)
      pPlayer->setLookAtPos(pBeamSquad->getPosition());

   // Submit an input this frequently.
   if (currentGameTime < mTimestampNextInput)
      return;

   const BSquad* pHeroSquad = gWorld->getSquad(mOwnerID);
   if (!pHeroSquad)
      return;  

   float maxBeamDist = pPower->getMaxBeamDistance();
   float minBeamDist = pPower->getMinBeamDistance();
   //BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(pPower->getProtoPowerID());
   //BTactic *pTactic = pPower->getBeamUnit()->getTactic();
   //BASSERT(pTactic);
   //BProtoAction *pBeamProtoAction = pTactic->getProtoAction(0, NULL, XMVectorZero(), NULL, XMVectorZero(), cInvalidObjectID, -1, false);
   //BASSERT(pBeamProtoAction);
   //float beamBlastSize = pBeamProtoAction->getAOERadius();
   float beamBlastSize = 8.0f;  // ???
   float beamBlastAvoidance = beamBlastSize * 2.0f;
   float maxTravelDist = pPower->getMaxBeamSpeed() * mTimeElapsedSinceLastInput;//pPower->getCommandInterval();
   float maxRetargetDist = 50.0f;
   float allyHPthreshold = 10000.0f;

   // Don't bother with KB - only steer towards visible units for now.
   BEntityIDArray targetUnits(0, 64);
   BUnitQuery query(pHeroSquad->getPosition(), maxBeamDist, true);
   query.setRelation(getPlayerID(), cRelationTypeEnemy);
   query.setUnitVisibility(getPlayerID());
   gWorld->getUnitsInArea(&query, &targetUnits);

   bool targetFound = false;

   const BUnit* pNewClosestTargetUnit = NULL;
   BVector currentBeamPos = pBeamSquad->getPosition();

   while (!targetFound)
   {
      // [6/30/2008 xemu] if we have no valid targets, abort the power
      if (targetUnits.getNumber() == 0)
      {
         BPowerInput powerInput;
         powerInput.mType = PowerInputType::cUserCancel;
         pPower->submitInput(powerInput);
         return;
      }

      // Find the closest 
      BEntityID newClosestTargetID = BSimHelper::computeClosestToPoint(currentBeamPos, targetUnits);
      pNewClosestTargetUnit = gWorld->getUnit(newClosestTargetID);

      // JAR - if no unit could be found and cInvalidObjectID was returned, or for whatever reason
      //       it could not be resolved to a unit. Abort the power.
      if(!pNewClosestTargetUnit)
      {
         BPowerInput powerInput;
         powerInput.mType = PowerInputType::cUserCancel;
         pPower->submitInput(powerInput);
         return;
      }

      targetFound = true;

      // [6/30/2008 xemu] filter out targets that are invalid, for a variety of reasons

      // [6/30/2008 xemu] if we're too close to the casting hero, then bail 
      BVector beamDelta = currentBeamPos - pHeroSquad->getPosition();
      BVector targetDelta = pNewClosestTargetUnit->getPosition() - pHeroSquad->getPosition();
      if (targetDelta.lengthSquared() < minBeamDist * minBeamDist)
      {
         targetUnits.remove(newClosestTargetID);
         targetFound = false;
         continue;
      }

      // [6/30/2008 xemu] check an area around the blast size to minimize going too close to friendlies 
      BEntityIDArray friendlyUnits(0, 64);
      BUnitQuery friendlyQuery(pNewClosestTargetUnit->getPosition(), beamBlastAvoidance, true);
      friendlyQuery.setRelation(getPlayerID(), cRelationTypeAlly);
      friendlyQuery.setUnitVisibility(getPlayerID());
      gWorld->getUnitsInArea(&friendlyQuery, &friendlyUnits);

      // [6/30/2008 xemu] friendlies, so try another target.  Really this wants to be sophisticated to not even go through enemies, but hopefully this approximation will do
      if (friendlyUnits.getNumber() > 0)
      {
         float totalAllyHP = 0.0f;
         uint numAllies = friendlyUnits.getSize();
         for (uint i = 0; i < numAllies; i++)
         {
            const BUnit* pUnit = gWorld->getUnit(friendlyUnits[i]);
            if (pUnit)
            {
               totalAllyHP += pUnit->getHPMax();
               if (totalAllyHP > allyHPthreshold)
               {
                  targetUnits.remove(newClosestTargetID);
                  targetFound = false;
                  break;
               }
            }
         }
         if (!targetFound)
            continue;

      }

      // [6/30/2008 xemu] if this target is just plain too far away from our current position, bail (since it's better to restart the power in that case)
      // [6/30/2008 xemu] note we have to do this check because the initial query is units around the HERO, not around the beam position 
      BVector moveDelta = pNewClosestTargetUnit->getPosition() - currentBeamPos;
      float moveDist2 = moveDelta.lengthSquared();
      if (moveDist2 > maxRetargetDist * maxRetargetDist)
      {
         // [6/30/2008 xemu] if we're hitting this, all subsequent targets will be *further* from the beam, so just cancel the power  
         BPowerInput powerInput;
         powerInput.mType = PowerInputType::cUserCancel;
         pPower->submitInput(powerInput);
         return;
      }

      // [6/30/2008 xemu] now check dot product relative to hero to avoid "crossing over"
      beamDelta.normalize();
      targetDelta.normalize();
      float dotBeam = beamDelta.dot(targetDelta);
      if (dotBeam < 0)
      {
         targetUnits.remove(newClosestTargetID);
         targetFound = false;
         continue;
      }
   }

   if (pNewClosestTargetUnit)
   {
      BVector desiredPos = BSimHelper::clampPosToAnchor(mPreviousPosition, pNewClosestTargetUnit->getPosition(), maxTravelDist);
      gTerrainSimRep.getHeightRaycast(desiredPos, desiredPos.y, true);

      BPowerInput powerInput;
      powerInput.mType = PowerInputType::cPosition;
      powerInput.mVector = desiredPos;
      pPower->submitInput(powerInput);
      mPreviousPosition = desiredPos;
      mTimestampNextInput = currentGameTime + pPower->getCommandInterval();
      mTimeElapsedSinceLastInput = 0.0f;
   }
}


//==============================================================================
//==============================================================================
void BAIPowerMission::updateRage(BPowerRage* pPower, DWORD currentGameTime)
{
   if (!pPower)
      return;
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   BASSERT(pPlayer);
   if (!pPlayer)
      return;
   const BKB* pKB = gWorld->getKB(pPlayer->getTeamID());
   BASSERT(pKB);
   if (!pKB)
      return;
   const BAIMissionTargetWrapper* pWrapper = this->getCurrentTargetWrapper();
   BASSERT(pWrapper);
   if (!pWrapper)
      return;
   const BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(pWrapper->getTargetID());
   BASSERT(pTarget);
   if (!pTarget)
      return;

   mTimeElapsedSinceLastInput += gWorld->getLastUpdateLengthFloat();
   mTimeElapsedSinceLastRageJump += gWorld->getLastUpdateLength();
   mTimeElapsedSinceLastRageMove += gWorld->getLastUpdateLength();

   // MARCHACK - Hard-code max time to 10 seconds.
   if (pPower->getElapsed() >= 25.0)
   {
      BPowerInput powerInput;
      powerInput.mType = PowerInputType::cUserCancel;
      pPower->submitInput(powerInput);
      return;
   }

   // Submit an input this frequently.
   if (currentGameTime < mTimestampNextInput)
      return;

   // Get the hero squad.
   const BSquad* pHeroSquad = gWorld->getSquad(mOwnerID);
   if (!pHeroSquad)
      return;
   BVector heroSquadPos = pHeroSquad->getPosition();

   // See if we're already attacking something.
   bool bIsAttacking = false;
   bool bIsBoardedAirUnit = false;
   bIsAttacking = pPower->isAttackingTarget(bIsBoardedAirUnit);
   bool bIsJumping = pPower->isJumping();

   // If we're on an air unit or currently jumping, don't do anything.
   if (bIsBoardedAirUnit || bIsJumping)
      return;

   // If we're attacking but it hasn't been long, wait.
   const DWORD cMinAttackTimePerTarget = 3000;
   if (bIsAttacking && mTimeElapsedSinceLastRageJump < cMinAttackTimePerTarget)
      return;


   //const BAction* pAction = pHeroSquad->getActionByTypeConst(BAction::cActionTypeSquadAttack);
   //if (pAction && mTimeElapsedSinceLastRageJump < cMinAttackTimePerTarget)
   //   return;

   BVector targetPos = pTarget->getPosition();
   float workDist = pWrapper->getSearchWorkDist();
   //updateCache(currentGameTime);


   if (mTimeElapsedSinceLastRageJump >= cMinAttackTimePerTarget)
   {
      // See if we can jump on something.
      BKBSquadIDArray kbSquadIDs;
      BKBSquadQuery query;
      query.setPointRadius(heroSquadPos, 40.0f);
      query.setPlayerRelation(mPlayerID, cRelationTypeEnemy);
      query.setCurrentlyVisible(true);
      uint numKBSquads = pKB->executeKBSquadQuery(query, kbSquadIDs, BKB::cClearExistingResults);
      if (numKBSquads > 0)
      {
         const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadIDs[getRandRange(cSimRand, 0, numKBSquads-1)]);
         BASSERT(pKBSquad);
         if (pKBSquad)
         {
            BVector dir = pKBSquad->getPosition() - heroSquadPos;
            if (dir.safeNormalize())
            {
               BPowerInput powerInput;
               powerInput.mType = PowerInputType::cDirection;
               powerInput.mVector = dir;
               pPower->submitInput(powerInput);
               mTimeElapsedSinceLastRageJump = 0;
            }
            return;
         }
      }
   }

   if (!bIsAttacking && mTimeElapsedSinceLastRageJump > 1500)
   {
      // Look in a wider area and run towards something.
      const DWORD cMinMoveTimeBetweenAttacks = 1000;
      if (mTimeElapsedSinceLastRageMove < cMinMoveTimeBetweenAttacks)
         return;

      // Move towards an enemy.
      BKBSquadIDArray kbSquadIDs;
      BKBSquadQuery query;
      query.setPointRadius(targetPos, workDist);
      query.setPlayerRelation(mPlayerID, cRelationTypeEnemy);
      query.setCurrentlyVisible(true);
      uint numKBSquads = pKB->executeKBSquadQuery(query, kbSquadIDs, BKB::cClearExistingResults);
      if (numKBSquads > 0)
      {
         const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadIDs[getRandRange(cSimRand, 0, numKBSquads-1)]);
         BASSERT(pKBSquad);
         if (pKBSquad)
         {
            BPowerInput powerInput;
            powerInput.mType = PowerInputType::cPosition;
            powerInput.mVector = pKBSquad->getPosition();
            pPower->submitInput(powerInput);
            mTimeElapsedSinceLastRageMove = 0;
            return;
         }
      }
      else
      {
         BPowerInput powerInput;
         powerInput.mType = PowerInputType::cUserCancel;
         pPower->submitInput(powerInput);
         return;
      }
   }
}


//==============================================================================
//==============================================================================
void BAIPowerMission::updateWave(BPowerWave* pPower, DWORD currentGameTime)
{
   if (!pPower)
      return;

   mTimeElapsedSinceLastInput += gWorld->getLastUpdateLengthFloat();

   // MARCHACK - Hard-code max time to 10 seconds.
   if (pPower->getElapsed() >= 10.0)
   {
      BPowerInput powerInput;
      powerInput.mType = PowerInputType::cUserOK;
      pPower->submitInput(powerInput);
      return;
   }

   BObject* pBallObject = pPower->getRealGravityBall().getBallObject();
   if (!pBallObject)
      return;

   BVector ballObjectPos = pBallObject->getPosition();

   // Look at the beam if we aren't already.
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (pPlayer)
      pPlayer->setLookAtPos(ballObjectPos);

   // Submit an input this frequently.
   if (currentGameTime < mTimestampNextInput)
      return;

   const BSquad* pHeroSquad = gWorld->getSquad(mOwnerID);
   if (!pHeroSquad)
      return;  

   float maxBeamDist = pPower->getMaxBallDistance();
   float minBeamDist = pPower->getMinBallDistance();
   //BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(pPower->getProtoPowerID());
   //BTactic *pTactic = pPower->getBeamUnit()->getTactic();
   //BASSERT(pTactic);
   //BProtoAction *pBeamProtoAction = pTactic->getProtoAction(0, NULL, XMVectorZero(), NULL, XMVectorZero(), cInvalidObjectID, -1, false);
   //BASSERT(pBeamProtoAction);
   //float beamBlastSize = pBeamProtoAction->getAOERadius();
   float beamBlastSize = 8.0f;  // ???
   float beamBlastAvoidance = beamBlastSize * 2.0f;
   //float maxTravelDist = pPower->getMaxBallSSpeed() * mTimeElapsedSinceLastInput;//pPower->getCommandInterval();
   float maxRetargetDist = 25.0f;
   float allyHPthreshold = 10000.0f;

   // Don't bother with KB - only steer towards visible units for now.
   BEntityIDArray targetUnits(0, 64);
   BUnitQuery query(pHeroSquad->getPosition(), maxBeamDist, true);
   query.setRelation(getPlayerID(), cRelationTypeEnemy);
   query.setUnitVisibility(getPlayerID());
   gWorld->getUnitsInArea(&query, &targetUnits);

   bool targetFound = false;

   const BUnit* pNewClosestTargetUnit = NULL;

   while (!targetFound)
   {
      // [6/30/2008 xemu] if we have no valid targets, abort the power
      if (targetUnits.getNumber() == 0)
      {
         BPowerInput powerInput;
         powerInput.mType = PowerInputType::cUserOK;
         pPower->submitInput(powerInput);
         return;
      }

      // Find the closest 
      BEntityID newClosestTargetID = BSimHelper::computeClosestToPoint(ballObjectPos, targetUnits);
      pNewClosestTargetUnit = gWorld->getUnit(newClosestTargetID);

      targetFound = true;

      // [6/30/2008 xemu] filter out targets that are invalid, for a variety of reasons

      // [6/30/2008 xemu] if we're too close to the casting hero, then bail 
      BVector beamDelta = ballObjectPos - pHeroSquad->getPosition();
      BVector targetDelta = pNewClosestTargetUnit->getPosition() - pHeroSquad->getPosition();
      if (targetDelta.lengthSquared() < minBeamDist * minBeamDist)
      {
         targetUnits.remove(newClosestTargetID);
         targetFound = false;
         continue;
      }

      // [6/30/2008 xemu] check an area around the blast size to minimize going too close to friendlies 
      BEntityIDArray friendlyUnits(0, 64);
      BUnitQuery friendlyQuery(pNewClosestTargetUnit->getPosition(), beamBlastAvoidance, true);
      friendlyQuery.setRelation(getPlayerID(), cRelationTypeAlly);
      friendlyQuery.setUnitVisibility(getPlayerID());
      gWorld->getUnitsInArea(&friendlyQuery, &friendlyUnits);

      // [6/30/2008 xemu] friendlies, so try another target.  Really this wants to be sophisticated to not even go through enemies, but hopefully this approximation will do
      if (friendlyUnits.getNumber() > 0)
      {
         float totalAllyHP = 0.0f;
         uint numAllies = friendlyUnits.getSize();
         for (uint i = 0; i < numAllies; i++)
         {
            const BUnit* pUnit = gWorld->getUnit(friendlyUnits[i]);
            if (pUnit)
            {
               totalAllyHP += pUnit->getHPMax();
               if (totalAllyHP > allyHPthreshold)
               {
                  targetUnits.remove(newClosestTargetID);
                  targetFound = false;
                  break;
               }
            }
         }
         if (!targetFound)
            continue;

      }

      // [6/30/2008 xemu] if this target is just plain too far away from our current position, bail (since it's better to restart the power in that case)
      // [6/30/2008 xemu] note we have to do this check because the initial query is units around the HERO, not around the beam position 
      BVector moveDelta = pNewClosestTargetUnit->getPosition() - ballObjectPos;
      float moveDist2 = moveDelta.lengthSquared();
      if (moveDist2 > maxRetargetDist * maxRetargetDist)
      {
         // [6/30/2008 xemu] if we're hitting this, all subsequent targets will be *further* from the beam, so just cancel the power  
         BPowerInput powerInput;
         powerInput.mType = PowerInputType::cUserOK;
         pPower->submitInput(powerInput);
         return;
      }

      // [6/30/2008 xemu] now check dot product relative to hero to avoid "crossing over"
      beamDelta.normalize();
      targetDelta.normalize();
      float dotBeam = beamDelta.dot(targetDelta);
      if (dotBeam < 0)
      {
         targetUnits.remove(newClosestTargetID);
         targetFound = false;
         continue;
      }
   }

   if (pNewClosestTargetUnit)
   {
      BVector desiredPos = pNewClosestTargetUnit->getPosition();//BSimHelper::clampPosToAnchor(mPreviousPosition, pNewClosestTargetUnit->getPosition(), maxTravelDist);
      gTerrainSimRep.getHeightRaycast(desiredPos, desiredPos.y, true);

      BPowerInput powerInput;
      powerInput.mType = PowerInputType::cPosition;
      powerInput.mVector = desiredPos;
      pPower->submitInput(powerInput);
      mPreviousPosition = desiredPos;
      mTimestampNextInput = currentGameTime + pPower->getCommandInterval();
      mTimeElapsedSinceLastInput = 0.0f;
   }
}


//==============================================================================
//==============================================================================
void BAIPowerMission::updateOrbital(BPowerOrbital* pPower, DWORD currentGameTime)
{
   if (!pPower)
      return;
   BAI* pAI = gWorld->getAI(mPlayerID);
   BASSERT(pAI);
   if (!pAI)
      return;
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   BASSERT(pPlayer);
   if (!pPlayer)
      return;
   const BAIMissionTargetWrapper* pCurrentTargetWrapper = getCurrentTargetWrapper();
   if (!pCurrentTargetWrapper)
      return;
   const BAIMissionTarget* pCurrentTarget = gWorld->getAIMissionTarget(pCurrentTargetWrapper->getTargetID());
   if (!pCurrentTarget)
      return;
   
   mTimeElapsedSinceLastInput += gWorld->getLastUpdateLengthFloat();
   mTimeElapsedSinceLastOrbitalBomb += gWorld->getLastUpdateLength();

   // MARCHACK - Hard-code max time to 10 seconds.
   if (pPower->getElapsed() >= 12.0)
   {
      BPowerInput powerInput;
      powerInput.mType = PowerInputType::cUserCancel;
      pPower->submitInput(powerInput);
      return;
   }

   BVector currentTargetPos = pCurrentTarget->getPosition();
   //float currentTargetRad = pCurrentTarget->getRadius();

   // Don't bother with KB - only steer towards visible units for now.
   BEntityIDArray targetUnits(0, 64);
   BUnitQuery query(pCurrentTarget->getPosition(), pCurrentTargetWrapper->getSearchWorkDist(), true);
   query.setRelation(mPlayerID, cRelationTypeEnemy);
   query.setUnitVisibility(mPlayerID);
   gWorld->getUnitsInArea(&query, &targetUnits);

   const float cOrbitalRadius = 50.0f;
   if (pPower->getShotsRemaining() > 0)
   {
      BVector targetLoc;
      if (!recalcBestOrbitalLoc(cOrbitalRadius, targetUnits, targetLoc))
         return;

      // Update AI EYE here.
      pPlayer->setLookAtPos(targetLoc);

      // Steer the beam towards something good.
      if (mTimeElapsedSinceLastInput > 0.2f)
      {
         BPowerInput targetingInput;
         targetingInput.mType = PowerInputType::cPosition;
         targetingInput.mVector = targetLoc;
         pPower->submitInput(targetingInput);
         mTimeElapsedSinceLastInput = 0.0f;
      }

      if (canAct(currentGameTime) && mTimeElapsedSinceLastOrbitalBomb > 2000)
      {
         BPowerOrbital* pPowerOrbital = static_cast<BPowerOrbital*>(pPower);
         BObject* pObject = gWorld->getObject(pPowerOrbital->getRealTargetingLaserID());
         if (!pObject)
            return;

         BVector beamPos = pObject->getPosition();
         if (beamPos.xzDistanceSqr(targetLoc) < 25.0f)
         {
            BPowerInput bombInput;
            bombInput.mType = PowerInputType::cUserOK;
            bombInput.mVector = targetLoc;
            pPower->submitInput(bombInput);
            mTimeElapsedSinceLastOrbitalBomb = 0;
            mTimeElapsedSinceLastInput = 0.0f;
            payTimeCostAct(currentGameTime);
         }
      }
   }
   else
   {
      setState(MissionState::cSuccess);
      mPowerID = cInvalidPowerID;
   }
}


//==============================================================================
//==============================================================================
bool BAIPowerMission::processTerminalConditions(DWORD currentGameTime)
{
   // No AI.
   BAI* pAI = gWorld->getAI(mPlayerID);
   BASSERT(pAI);
   if (!pAI)
   {
      setState(MissionState::cInvalid);
      return (true);
   }

   // No power manager.
   BPowerManager* pPowerManager = gWorld->getPowerManager();
   BASSERT(pPowerManager);
   if (!pPowerManager)
   {
      setState(MissionState::cInvalid);
      pAI->AIQTopicLotto(currentGameTime);
      return (true);
   }

   // If the power no longer exists, we need to make this mission go away.
//-- FIXING PREFIX BUG ID 1202
   const BPower* pPower = pPowerManager->getPowerByID(mPowerID);
//--
   if (!pPower)
   {
      setState(MissionState::cInvalid);
      pAI->AIQTopicLotto(currentGameTime);
      return (true);
   }

   // If this power mission is supposed to have an owner squad, validate some stuff.
   if (mOwnerID.isValid())
   {
      // Does the squad exist?
      const BSquad* pSquad = gWorld->getSquad(mOwnerID);
      if (!pSquad)
      {
         setState(MissionState::cInvalid);
         pAI->AIQTopicLotto(currentGameTime);
         return (true);
      }

      // Squad is not alive.
      if (!pSquad->isAlive())
      {
         setState(MissionState::cInvalid);
         pAI->AIQTopicLotto(currentGameTime);
         return (true);
      }

      // Squad is wrong player.
      if (pSquad->getPlayerID() != getPlayerID())
      {
         setState(MissionState::cInvalid);
         pAI->AIQTopicLotto(currentGameTime);
         return (true);
      }
   }

   // Not terminal, continue...
   return (false);
}

//==============================================================================
// BAIPowerMission::save
//==============================================================================
bool BAIPowerMission::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mPreviousPosition);
   GFWRITEVAR(pStream, DWORD, mTimestampNextInput);
   GFWRITEVAR(pStream, float, mTimeElapsedSinceLastInput);
   GFWRITEVAR(pStream, DWORD, mTimeElapsedSinceLastRageJump);
   GFWRITEVAR(pStream, DWORD, mTimeElapsedSinceLastRageMove);
   GFWRITEVAR(pStream, DWORD, mTimeElapsedSinceLastOrbitalBomb);

   GFWRITEVAR(pStream, BPowerID, mPowerID);
   GFWRITEVAR(pStream, BEntityID, mOwnerID);
   return true;
}

//==============================================================================
// BAIPowerMission::load
//==============================================================================
bool BAIPowerMission::load(BStream* pStream, int saveType)
{  
   GFREADVECTOR(pStream, mPreviousPosition);
   GFREADVAR(pStream, DWORD, mTimestampNextInput);
   GFREADVAR(pStream, float, mTimeElapsedSinceLastInput);
   
   mTimeElapsedSinceLastRageJump = 0;
   mTimeElapsedSinceLastRageMove = 0;
   mTimeElapsedSinceLastOrbitalBomb = 0;
   if (BAIPowerMission::mGameFileVersion >= 2)
   {
      GFREADVAR(pStream, DWORD, mTimeElapsedSinceLastRageJump);
      GFREADVAR(pStream, DWORD, mTimeElapsedSinceLastRageMove);
      if (BAIPowerMission::mGameFileVersion >= 3)
      {
         GFREADVAR(pStream, DWORD, mTimeElapsedSinceLastOrbitalBomb);
      }
   }

   GFREADVAR(pStream, BPowerID, mPowerID);
   GFREADVAR(pStream, BEntityID, mOwnerID);
   return true;
}

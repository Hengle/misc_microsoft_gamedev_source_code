//==============================================================================
// powerhelper.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "powerhelper.h"
#include "TerrainSimRep.h"
#include "world.h"
#include "unitquery.h"
#include "physics.h"
#include "tactic.h"
#include "protosquad.h"
#include "uimanager.h"
#include "decalmanager.h"
#include "squadactiontransport.h"
#include "usermanager.h"
#include "power.h"
#include "user.h"
#include "powerdisruption.h"
#include "powermanager.h"
#include "protopower.h"
#include "simhelper.h"

namespace BPowerHelper
{
   //==============================================================================
   BBomberData::BBomberData()
   {
      clear();
   }

   void BBomberData::clear()
   {
      BomberId = cInvalidObjectID;
      BomberFlyinDistance = 0.0f;
      BomberFlyinHeight = 0.0f;
      BomberBombHeight = 0.0f;
      BomberSpeed = 0.0f;
      BombTime = 0.0f;
      FlyoutTime = 0.0f;
      AdditionalHeight = 0.0f;
   }

   bool BBomberData::save(BStream* pStream, int saveType) const
   {
      GFWRITEVAR(pStream, BEntityID, BomberId);
      GFWRITEVAR(pStream, float, BomberFlyinDistance);
      GFWRITEVAR(pStream, float, BomberFlyinHeight);
      GFWRITEVAR(pStream, float, BomberBombHeight);
      GFWRITEVAR(pStream, float, BomberSpeed);
      GFWRITEVAR(pStream, float, BombTime);
      GFWRITEVAR(pStream, float, FlyoutTime);
      GFWRITEVAR(pStream, float, AdditionalHeight);
      return true;
   };

   bool BBomberData::load(BStream* pStream, int saveType)
   {
      GFREADVAR(pStream, BEntityID, BomberId);
      GFREADVAR(pStream, float, BomberFlyinDistance);
      GFREADVAR(pStream, float, BomberFlyinHeight);
      GFREADVAR(pStream, float, BomberBombHeight);
      GFREADVAR(pStream, float, BomberSpeed);
      GFREADVAR(pStream, float, BombTime);
      GFREADVAR(pStream, float, FlyoutTime);
      if (BPower::mGameFileVersion >= 14)
      {
         GFREADVAR(pStream, float, AdditionalHeight);
      }
      return true;
   }

   //==============================================================================
   BLimitData::BLimitData()
   {
      clear();
   }

   void BLimitData::clear()
   {
      mOTID = cInvalidObjectTypeID;
      mLimit = 0;
   }

   bool BLimitData::save(BStream* pStream, int saveType) const
   {
      GFWRITEVAR(pStream, BObjectTypeID, mOTID);
      GFWRITEVAR(pStream, uint, mLimit);
      return true;
   }

   bool BLimitData::load(BStream* pStream, int saveType)
   {
      GFREADVAR(pStream, BObjectTypeID, mOTID);
      GFREADVAR(pStream, uint, mLimit);
      gSaveGame.remapObjectType(mOTID);
      return true;
   }

   //==============================================================================
   BHUDSounds::BHUDSounds()
   {
      clear();
   }

   bool BHUDSounds::loadFromProtoPower(const BProtoPower* pProtoPower, int level)
   {
      BASSERT(pProtoPower);

      // required
      if(!pProtoPower->getDataSound(level, "HudUpSound", HudUpSound))
         return false;
      if(!pProtoPower->getDataSound(level, "HudAbortSound", HudAbortSound))
         return false;
      if(!pProtoPower->getDataSound(level, "HudFireSound", HudFireSound))
         return false;
      
      // optional
      pProtoPower->getDataSound(level, "HudLastFireSound", HudLastFireSound);
      pProtoPower->getDataSound(level, "HudStartEnvSound", HudStartEnvSound);
      pProtoPower->getDataSound(level, "HudStopEnvSound", HudStopEnvSound);

      return true;
   }

   void BHUDSounds::clear()
   {
      HudUpSound = cInvalidCueIndex;
      HudAbortSound = cInvalidCueIndex;
      HudFireSound = cInvalidCueIndex;
      HudLastFireSound = cInvalidCueIndex;
      HudStartEnvSound = cInvalidCueIndex;
      HudStopEnvSound = cInvalidCueIndex;
   }

   bool BHUDSounds::save(BStream* pStream, int saveType) const
   {
      GFWRITEVAR(pStream, BCueIndex, HudUpSound);
      GFWRITEVAR(pStream, BCueIndex, HudAbortSound);
      GFWRITEVAR(pStream, BCueIndex, HudFireSound);
      GFWRITEVAR(pStream, BCueIndex, HudLastFireSound);
      GFWRITEVAR(pStream, BCueIndex, HudStartEnvSound);
      GFWRITEVAR(pStream, BCueIndex, HudStopEnvSound);
      return true;
   }

   bool BHUDSounds::load(BStream* pStream, int saveType)
   {
      GFREADVAR(pStream, BCueIndex, HudUpSound);
      GFREADVAR(pStream, BCueIndex, HudAbortSound);
      GFREADVAR(pStream, BCueIndex, HudFireSound);
      GFREADVAR(pStream, BCueIndex, HudLastFireSound);
      if (BPower::mGameFileVersion >= 22)
      {
         GFREADVAR(pStream, BCueIndex, HudStartEnvSound);
         GFREADVAR(pStream, BCueIndex, HudStopEnvSound);
      }
      return true;
   }


   //==============================================================================
   BEntityID createBomber(BProtoObjectID bomberProtoId, BBomberData& bomberData, const BVector& startLocation, BVector startDirection, BPlayerID playerId)
   {
      // normalize direction
      startDirection.normalize();

      // calc cross product
      BVector rightVector;
      rightVector.assignCrossProduct(cYAxisVector, startDirection);
      rightVector.normalize();

      // set up bomber
      BVector bomberStart = -startDirection;
      bomberStart.y = 0;
      bomberStart.normalize();
      bomberStart *= bomberData.BomberFlyinDistance;
      bomberStart += startLocation;
      gTerrainSimRep.getFlightHeightRaycast(bomberStart, bomberStart.y, true, true);

      BVector raycastStart = bomberStart;
      raycastStart.y += bomberData.BomberBombHeight;

      // see if we need to adjust the height for air obstructions
      // Make sure there are no "ObstructsAir" obstacles in the way and that the position is within map boundaries before setting the squad position
      // be safe, and check the whole bombing run 
      float radius = bomberData.BomberFlyinDistance;
      BObstructionNodePtrArray obstructionList;
      gObsManager.findObstructions(startLocation.x - radius, startLocation.z - radius, startLocation.x + radius, startLocation.z + radius, 0.0f, BObstructionManager::cIsNewTypeBlockAirMovement, BObstructionManager::cObsNodeTypeAll, playerId, false, obstructionList);
      long numObstructions = obstructionList.getNumber();
      for (long i = 0; i < numObstructions; i++)
      {
         const BOPObstructionNode* pNode = obstructionList.get(i);
         BObject* pObject = (pNode && pNode->mObject) ? pNode->mObject->getObject() : NULL;
         if (pObject)
         {
            const BBoundingBox* pVisualBounds = pObject->getVisualBoundingBox();

            // do a fast and dirty intersect - false positive is better than false neg
            if (pVisualBounds->rayIntersectsSphere(raycastStart, startDirection))
            {
               float flightHeight = 0.0f;
               gTerrainSimRep.getFlightHeightRaycast(pNode->mObject->getPosition(), flightHeight, true, true);

               float maxHeight = pVisualBounds->getCenter().y + pVisualBounds->getExtents()[1];
               static const float cBufferBombHeight = 10.0f;

               float bombHeight = bomberData.BomberBombHeight + flightHeight;
               bomberData.AdditionalHeight = Math::Max(bomberData.AdditionalHeight, (maxHeight + cBufferBombHeight) - bombHeight);
            }
         }
      }

      bomberStart.y += (bomberData.BomberFlyinHeight + bomberData.AdditionalHeight);

      BObjectCreateParms bomberCreate;
      bomberCreate.mPlayerID = playerId;
      bomberCreate.mPosition = bomberStart;
      bomberCreate.mForward = startDirection;
      bomberCreate.mRight = rightVector;
      bomberCreate.mType = BEntity::cClassTypeObject;
      bomberCreate.mProtoObjectID = bomberProtoId;
      BObject* pBomberObject = gWorld->createObject(bomberCreate);
      if (pBomberObject)
         return pBomberObject->getID();

      return cInvalidObjectID;
   }

   //==============================================================================
   bool updateBomberPosition(const BBomberData& bomberData, float totalElapsed, float lastUpdateElapsed)
   {
      // manually move the bomber, no worrying about pathing here 
      BObject* pBomberUnit = gWorld->getObject(bomberData.BomberId);
      if (pBomberUnit)
      {
         // make sure lateral speed is correct
         BVector newPosition = pBomberUnit->getPosition();
         BVector forwardXZ = pBomberUnit->getForward();
         forwardXZ.y = 0.0f;
         forwardXZ.normalize();
         newPosition += (forwardXZ * bomberData.BomberSpeed * lastUpdateElapsed);

         // compute vertical position
         if (totalElapsed < bomberData.BombTime)
         {
            // flying in (basic parabolic algorithm)
            float additionalCurveHeight = (float)((bomberData.BombTime - totalElapsed) / bomberData.BombTime);
            additionalCurveHeight *= additionalCurveHeight;
            additionalCurveHeight *= (bomberData.BomberFlyinHeight - bomberData.BomberBombHeight);
            newPosition.y = bomberData.AdditionalHeight + bomberData.BomberBombHeight + additionalCurveHeight;
         }
         else if (totalElapsed < bomberData.FlyoutTime)
         {
            // bombing
            newPosition.y = bomberData.AdditionalHeight + bomberData.BomberBombHeight;
         }
         else if (totalElapsed < (bomberData.FlyoutTime + bomberData.BombTime))
         {
            // flying out (basic parabolic algorithm)
            float additionalCurveHeight = (float)((totalElapsed - bomberData.FlyoutTime) / bomberData.BombTime);
            additionalCurveHeight *= additionalCurveHeight;
            additionalCurveHeight = min(1.0f, additionalCurveHeight);
            additionalCurveHeight *= (bomberData.BomberFlyinHeight - bomberData.BomberBombHeight);
            newPosition.y = bomberData.AdditionalHeight + bomberData.BomberBombHeight + additionalCurveHeight;
         }
         else
         {
            // we're done - kill it
            pBomberUnit->kill(true);
            return true;
         }

         // bump up the newpos by the flight height
         float flightHeight = 0.0f;
         gTerrainSimRep.getFlightHeightRaycast(newPosition, flightHeight, true, true);
         newPosition.y += flightHeight;

         BVector newForward = (newPosition - pBomberUnit->getPosition());
         newForward.normalize();
         pBomberUnit->setForward(newForward);
         pBomberUnit->setPosition(newPosition);
         return true;
      }

      return false;
   }

   //==============================================================================
   void impulsePhysicsAirUnits(const BVector& location, float radius, int percentChanceToImpulse)
   {
      // physics impulses vs. air units
      BEntityIDArray results(0, 10);       
      BUnitQuery query(location, radius, true);
      gWorld->getSquadsInArea(&query, &results, false);
      uint numSquads = (uint)results.getNumber();
      BSquad* pSquad = NULL;
      for (uint i = 0; i < numSquads; ++i)
      {
         pSquad = gWorld->getSquad(results[i]);
         if (!pSquad)
            continue;

         const BEntityIDArray& children = pSquad->getChildList();
         uint numChildren = children.getSize();
         BUnit* pUnit = NULL;
         for (uint j = 0; j < numChildren; ++j)
         {
            // only do this every so often
            if(getRandRange(cSimRand, 0, 100) > percentChanceToImpulse)
               continue;

            // bomb check
            pUnit = gWorld->getUnit(children[j]);
            if(!pUnit)
               continue;

            impulsePhysicsAirUnit(*pUnit, 1.0f, &location, radius);
         }
      }
   }

   //==============================================================================
   void impulsePhysicsAirUnit(BUnit& unit, float impulseScale, const BVector* impulseTarget, float radius)
   {
      // only air units
      if(!unit.isPhysicsAircraft())
         return;

      // get physics object
      BPhysicsObject* pPO = unit.getPhysicsObject();
      if(!pPO)
         return;

      // calc some basics
      BVector diff, normalizedDiff, linearImpulse, linearPointImpulse, location;
      float impulseMult = 0, healthMult = 0;
      static const float cMaxImpulseMult = 150.0f;
      static const float cMinimumFalloffPercent = 0.75f;
      static const float cHealthImpulseMult = 2.0f;
      static const float cPointImpulseMult = 2.5f;

      if (impulseTarget)
         location = *impulseTarget;
      else
      {
         // pick a random impulse downward
         location.x = getRandRangeFloat(cSimRand, -0.5f, 0.5f) * unit.getObstructionRadiusX();
         location.y = -unit.getObstructionRadius();
         location.z = getRandRangeFloat(cSimRand, -0.5f, 0.5f) * unit.getObstructionRadiusZ();
         location += unit.getPosition();
      }

      diff = location - unit.getPosition();
      diff.y = 0;
      normalizedDiff = diff;
      normalizedDiff.normalize();

      // init linear impulse
      linearImpulse = diff;
      linearImpulse.normalize();

      // add negative y-axis to impulse so that the beam appears to be forcing the aircraft downward
      linearImpulse -= cYAxisVector;

      // calc the health multiplier, which will increase the magnitude of the impulse by how damaged the unit is
      healthMult = Math::Lerp(cHealthImpulseMult, 1.0f, unit.getHitpoints() / unit.getHPMax());

      // calc the overall impulse multiplier, scaling it down the farther the unit is from the beam, then apply to impulse
      //impulseMult = cMaxImpulseMult * healthMult / max(1.0f, diff.length());
      impulseMult = cMaxImpulseMult * healthMult * impulseScale;
      if (radius > cFloatCompareEpsilon)
         impulseMult *= Math::Lerp(cMinimumFalloffPercent, 1.0f, 1.0f - 0.5f * diff.length() / radius);
      linearImpulse *= impulseMult;

      // apply impulse through center of mass
      pPO->applyImpulse(linearImpulse);

      // calculate a point impulse to cause some rotation and apply
      linearPointImpulse = linearImpulse * cPointImpulseMult;
      pPO->applyPointImpulse(linearPointImpulse, unit.getPosition() + normalizedDiff);
   }

   //==============================================================================
   BProtoAction* getFirstProtoAction(BPlayer& player, BProtoObjectID protoObjectId)
   {
      BProtoObject* pProjProtoObject = player.getProtoObject(protoObjectId);
      if (!pProjProtoObject)
         return NULL;

      BTactic* pProjTactic = pProjProtoObject->getTactic();
      if (!pProjTactic)
         return NULL;

      return pProjTactic->getProtoAction(0, NULL, XMVectorZero(), NULL, XMVectorZero(), cInvalidObjectID, -1, false);
   }

   //==============================================================================
   bool checkSquadPlacement(const BPlayer& player, const BVector& location, const BEntityIDArray& squads)
   {
      if(squads.getNumber() == 0)
         return false;

      BSquad* pExitSquad = gWorld->getSquad(squads[0]);
      if(!pExitSquad)
         return false;

      static BDynamicSimVectorArray instantiatePositions;
      instantiatePositions.setNumber(0);
      long closestDesiredPositionIndex;
      bool success = BSimHelper::findInstantiatePositions(1, instantiatePositions, 0.5f, location,
         pExitSquad->getForward(), pExitSquad->getRight(), pExitSquad->getObstructionRadius(), location, closestDesiredPositionIndex, 0, true);

      return success;
   }

   //==============================================================================
   bool getTransportableSquads(BPlayerID playerId, const BVector& location, float radius, BEntityIDArray& outSquads)
   {
      // search for squads in range
      BEntityIDArray squadsInArea(0, 100);       
      BUnitQuery unitQuery(location, radius, true);
      unitQuery.setRelation(playerId, cRelationTypeSelf);
      gWorld->getSquadsInArea(&unitQuery, &squadsInArea, false);

      // populate our list of valid squads
      outSquads.resize(0);
      BSquadActionTransport::filterTransportableSquads(squadsInArea, playerId, true, outSquads);

      return (outSquads.getSize() > 0);
   }

   //==============================================================================
  BPowerUser* getPowerUser(const BPower& power)
  {
     // returns null if the local user isn't our power user
     BPowerUserID powerUserID = power.getPowerUserID();
     BUser* pUser = gUserManager.getUserByPlayerID(powerUserID.getPlayerID());
     if(pUser && pUser->getPowerUser() && pUser->getPowerUser()->getID() == powerUserID)
        return pUser->getPowerUser();

     return NULL;
  }

  //==============================================================================
  //==============================================================================
  bool checkPowerLocation(BPlayerID playerId, const BVector& targetLocation, const BPower* pSourcePower, bool strikeInvalid)
  {
     BPowerDisruption* pDisruptionPowerCause = NULL;
     BVector tempVec;
     if(!gWorld->checkPlacement(-1,  playerId, targetLocation, tempVec, cZAxisVector, BWorld::cCPLOSFullVisible, BWorld::cCPLOSCenterOnly))
        return false;

     if (!gWorld->getPowerManager())
        return false;

     BUser* pUser = gUserManager.getUserByPlayerID(playerId);
     if (!gWorld->getPowerManager()->isValidPowerLocation(targetLocation, pSourcePower, &pDisruptionPowerCause))
     {
        if (pDisruptionPowerCause && strikeInvalid)
           pDisruptionPowerCause->strikeLocation(targetLocation, pUser);
        return false;
     }

     return true;
  }
}
//==============================================================================
// alert.cpp
//
// The alert system.
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
//==============================================================================
#include "common.h"
#include "ai.h"
#include "alert.h"
#include "world.h"
#include "protoobject.h"
#include "minimap.h"
#include "uigame.h"
#include "unitquery.h"
#include "render.h"
#include "usermanager.h"
#include "user.h"
#include "camera.h"
#include "uimanager.h"
#include "worldsoundmanager.h"
#include "config.h"
#include "configsgame.h"
#include "game.h"
#include "UnitActionCollisionAttack.h"


const float BAlertManager::cDistance = 80.0f;
const float BAlertManager::cDistanceSqr = 80.0f * 80.0f;
const float BAlertManager::cMinAlertDistanceSqr = 300.0f * 300.0f;


//==============================================================================
//==============================================================================
BAlert::BAlert(void)
{
   mID.invalidate();
   resetNonIDData();
}


//==============================================================================
//==============================================================================
void BAlert::resetNonIDData()
{
   mLocation = cOriginVector;
   mCreateTime = 0;
   mExpireTime = 0;
   mDeleteTime = 0;
   mType = AlertType::cInvalid;
   mEntityID = cInvalidObjectID;
   mAttackerID = cInvalidObjectID;
   mFlaringPlayerID = cInvalidPlayerID;
   mbQueued = false;
   mbExpired = false;
   mbVisible = true;
   mbPlayAlert = true;
   mbAttackOnBase = false;
}


//==============================================================================
//==============================================================================
BAlertManager::BAlertManager(BPlayer* pPlayer) :
   mNextGotoAlertID(cInvalidAlertID),
   mNextAttackAlertTime(0),
   mLastAllyFlare(cInvalidAlertID),
   mLastAttackAlert(cInvalidAlertID),
   mbSquelchAlerts(false),
   mbNewAlertsAllowed(true)
{
   BASSERT(pPlayer);

   mpPlayer = pPlayer;
}


//==============================================================================
//==============================================================================
BPlayerID BAlertManager::getPlayerID() const
{
   return (mpPlayer->getID());
}


//==============================================================================
//==============================================================================
void BAlertManager::clearAlerts()
{
   mNextAttackAlertTime = 0;
   mNextGotoAlertID = cInvalidAlertID;
   mLastAllyFlare = cInvalidAlertID;
   mLastAttackAlert = cInvalidAlertID;
}

//==============================================================================
//==============================================================================
const BAlert* BAlertManager::peekGotoAlert() const
{
   const BAlert* pNextGotoAlert = getAlert(mNextGotoAlertID);
   return pNextGotoAlert;
}

//==============================================================================
//==============================================================================
const BAlert* BAlertManager::getGotoAlert()
{
   const BAlert* pNextGotoAlert = getAlert(mNextGotoAlertID);
   BAlertID newNextID = getNextGotoAlertID();
   setNextGotoAlertID(newNextID);
   return (pNextGotoAlert);
}

//==============================================================================
//==============================================================================
BAlertID BAlertManager::getNextGotoAlertID()
{
   int index = mAlertIDs.find(mNextGotoAlertID);
   BAlertID newNextID = cInvalidAlertID;
   int numAlerts = static_cast<int>(mAlertIDs.getSize());

   for (int i=(index+1); i<numAlerts; i++)
   {
      const BAlert* pAlert = getAlert(mAlertIDs[i]);
      if (pAlert && !pAlert->getQueued() /*&& !pAlert->getExpired()*/)
      {
         newNextID = mAlertIDs[i];
         break;
      }
   }

   if (newNextID == cInvalidAlertID)
   {
      for (int i=0; i<=index; i++)
      {
         const BAlert* pAlert = getAlert(mAlertIDs[i]);
         if (pAlert && !pAlert->getQueued() /*&& !pAlert->getExpired()*/)
         {
            newNextID = mAlertIDs[i];
            break;
         }
      }
   }

   return (newNextID);
}


//==============================================================================
//==============================================================================
bool BAlertManager::isAlertUIActive(const BAlert* pAlert) const
{
   if (mbSquelchAlerts)
      return (false);
   else
   {
      bool result = (gUserManager.getPrimaryUser()->getPlayerID() == getPlayerID() || (gGame.isSplitScreen() && gUserManager.getSecondaryUser()->getPlayerID() == getPlayerID()));

      //-- Make sure alerts play for attacks on buildings for the co-op player.
      if(pAlert && pAlert->getAttackOnBase())
      {
         BPlayer* pPlayer = gWorld->getPlayer(getPlayerID());
         if(pPlayer && pPlayer->getCoopID() == gUserManager.getPrimaryUser()->getPlayerID())
            result = true;
      }

      return result; 
   }
}


//==============================================================================
//==============================================================================
BAlert* BAlertManager::createAlert()
{
   // Player will disable new alerts once they win or lose the game
   if (!mbNewAlertsAllowed)
      return NULL;

   uint newAlertIndex = BAlertID::cMaxIndex;
   BAlert* pNewAlert = mAlerts.acquire(newAlertIndex, true);  // fatal on alloc failure, so don't check return.
   BASSERT(newAlertIndex < BAlertID::cMaxIndex);

   pNewAlert->resetNonIDData();

   // If the ID isn't invalid, just bump the refcount.
   if (pNewAlert->getID().isValid())
   {
      pNewAlert->getID().bumpRefCount();
   }
   // Otherwise give it a new ID
   else
   {
      BAlertID newAlertID(getPlayerID(), 0, newAlertIndex);
      pNewAlert->setID(newAlertID);
   }

   mAlertIDs.add(pNewAlert->getID());

   return (pNewAlert);
}


//==============================================================================
//==============================================================================
BAlert* BAlertManager::getAlert(BAlertID alertID)
{
   // Do the easy ID & player checks.
   if (!alertID.isValid())
      return (NULL);
   if (alertID.getPlayerID() != getPlayerID())
      return (NULL);

   // Look at the index and make sure that's ok.
   uint index = alertID.getIndex();
   if (index >= mAlerts.getHighWaterMark())
      return (NULL);
   if (!mAlerts.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   BAlert* pAlert = mAlerts.get(index);
   if (!pAlert)
      return (NULL);
   uint idRefCount = alertID.getRefCount();
   uint alertRefCount = pAlert->getID().getRefCount();
   if (idRefCount == alertRefCount)
      return (pAlert);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BAlert* BAlertManager::getAlert(BAlertID alertID) const
{
   // Do the easy ID & player checks.
   if (!alertID.isValid())
      return (NULL);
   if (alertID.getPlayerID() != getPlayerID())
      return (NULL);

   // Look at the index and make sure that's ok.
   uint index = alertID.getIndex();
   if (index >= mAlerts.getHighWaterMark())
      return (NULL);
   if (!mAlerts.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   const BAlert* pAlert = mAlerts.get(index);
   if (!pAlert)
      return (NULL);
   uint idRefCount = alertID.getRefCount();
   uint alertRefCount = pAlert->getID().getRefCount();
   if (idRefCount == alertRefCount)
      return (pAlert);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
void BAlertManager::deleteAlert(BAlertID alertID)
{
   BAlert* pAlert = getAlert(alertID);
   if (pAlert)
   {
      // If this alert was our last flare from an ally, reset that on the AI
      if (getLastAllyFlareAlertID() == alertID)
         setLastAllyFlareAlertID(cInvalidAlertID);

      if (alertID == mNextGotoAlertID)
      {
         BAlertID newNextID = getNextGotoAlertID();
         if (newNextID != mNextGotoAlertID)
            setNextGotoAlertID(newNextID);
         else
            setNextGotoAlertID(cInvalidAlertID);
      }

      BASSERT(mAlertIDs.contains(alertID));
      mAlertIDs.remove(alertID);
      mAlerts.release(pAlert);
   }
   else
   {
      mAlertIDs.remove(alertID);
   }
}


//==============================================================================
//==============================================================================
bool BAlertManager::createBaseDestructionAlert(BVector pos, BEntityID destroyedBaseID)
{
   BAlert* pNewAlert = createAlert();
   if (!pNewAlert)
      return (false);
   const DWORD createTime = gWorld->getGametime();
   const DWORD expireTime = createTime + BAlertManager::cFlareAlertExpireTime;
   const DWORD deleteTime = createTime + BAlertManager::cFlareAlertDeleteTime;

   pNewAlert->setType(AlertType::cBaseDestruction);
   pNewAlert->setLocation(pos);
   pNewAlert->setCreateTime(createTime);
   pNewAlert->setExpireTime(expireTime);
   pNewAlert->setDeleteTime(deleteTime);
   pNewAlert->setEntityID(destroyedBaseID);
   pNewAlert->setQueued(false);

   setNextGotoAlertID(pNewAlert->getID());

   return (true);
}


//==============================================================================
//==============================================================================
bool BAlertManager::createResearchCompleteAlert(BVector pos, BEntityID buildingID)
{
   BAlert* pNewAlert = createAlert();
   if (!pNewAlert)
      return (false);
   const DWORD createTime = gWorld->getGametime();
   const DWORD expireTime = createTime + BAlertManager::cFlareAlertExpireTime;
   const DWORD deleteTime = createTime + BAlertManager::cFlareAlertDeleteTime;

   pNewAlert->setType(AlertType::cResearchComplere);
   pNewAlert->setLocation(pos);
   pNewAlert->setCreateTime(createTime);
   pNewAlert->setExpireTime(expireTime);
   pNewAlert->setDeleteTime(deleteTime);
   pNewAlert->setEntityID(buildingID);
   pNewAlert->setQueued(false);

   setNextGotoAlertID(pNewAlert->getID());

   return (true);
}


//==============================================================================
//==============================================================================
bool BAlertManager::createTrainingCompleteAlert(BVector pos, BEntityID squadID)
{
   BAlert* pNewAlert = createAlert();
   if (!pNewAlert)
      return (false);
   const DWORD createTime = gWorld->getGametime();
   const DWORD expireTime = createTime + BAlertManager::cFlareAlertExpireTime;
   const DWORD deleteTime = createTime + BAlertManager::cFlareAlertDeleteTime;

   pNewAlert->setType(AlertType::cTrainingComplete);
   pNewAlert->setLocation(pos);
   pNewAlert->setCreateTime(createTime);
   pNewAlert->setExpireTime(expireTime);
   pNewAlert->setDeleteTime(deleteTime);
   pNewAlert->setEntityID(squadID);
   pNewAlert->setQueued(false);

   setNextGotoAlertID(pNewAlert->getID());

   return (true);
}

//==============================================================================
//==============================================================================
bool BAlertManager::createTransportCompleteAlert(BVector pos, BEntityID squadID)
{

   //-- Make sure there isn't already an alert hanging around for this one.
   for(uint i=0; i < mAlertIDs.getSize(); i++)
   {
//-- FIXING PREFIX BUG ID 2136
      const BAlert* pAlert = getAlert(mAlertIDs[i]);
//--
      if(!pAlert)
         continue;
      if(pAlert->getType() != AlertType::cTransportComplete)
         continue;

      if(pAlert->getLocation().xzDistanceSqr(pos) <= BAlertManager::cDistanceSqr) //-- Already an alert for this.
         return false; 
   }


   BPlayerID playerID = cInvalidPlayerID;
//-- FIXING PREFIX BUG ID 2137
   const BSquad* pSquad = gWorld->getSquad(squadID);
//--
   if(pSquad)
      playerID = pSquad->getPlayerID();

   if (isAlertUIActive())
   {
      //if (!gConfig.isDefined(cConfigFlashGameUI))
      //   gMiniMap.triggerFlare(pos, playerID, BUIGame::cFlareLook);
      //else
      gUIManager->addMinimapFlare(pos, playerID, BUIGame::cFlareLook);
   }

   BAlert* pNewAlert = createAlert();
   if (!pNewAlert)
      return (false);
   const DWORD createTime = gWorld->getGametime();
   const DWORD expireTime = createTime + BAlertManager::cTransportAlertExpireTime;
   const DWORD deleteTime = createTime + BAlertManager::cTransportAlertDeleteTime;

   pNewAlert->setType(AlertType::cTransportComplete);
   pNewAlert->setLocation(pos);
   pNewAlert->setCreateTime(createTime);
   pNewAlert->setExpireTime(expireTime);
   pNewAlert->setDeleteTime(deleteTime);
   pNewAlert->setEntityID(squadID);
   pNewAlert->setQueued(false);
   pNewAlert->setVisible(true);

   setNextGotoAlertID(pNewAlert->getID());

   return (true);
}


//==============================================================================
//==============================================================================
bool BAlertManager::createAttackAlert(BVector pos, BEntityID attackingUnitID, BEntityID defenderUnitID)
{
   const BUnit *pDefenderUnit = gWorld->getUnit(defenderUnitID);
   if (!pDefenderUnit)
      return (false);
   BASSERT(pDefenderUnit->getPlayerID() == getPlayerID());
   const DWORD createTime = gWorld->getGametime();
   const DWORD expireTime = createTime + BAlertManager::cAttackAlertExpireTime;
   const DWORD deleteTime = createTime + BAlertManager::cAttackAlertDeleteTime;

   // Find out if we have any overlapping attack alerts.
   bool existingAlert = false;
   bool overlapAlert = false;
   uint numAlerts = mAlertIDs.getSize();
   for (uint i=0; i<numAlerts; i++)
   {
      BAlert* pAlert = getAlert(mAlertIDs[i]);
      if (!pAlert || pAlert->getType() != AlertType::cAttack)
         continue;
      BVector location = pAlert->getLocation();
      float distanceSqr = pos.distanceSqr(location);
      if (distanceSqr <= BAlertManager::cDistanceSqr)
      {
         pAlert->setExpireTime(expireTime);
         pAlert->setDeleteTime(deleteTime);
         existingAlert = true;

         setLastAttackAlertID(pAlert->getID());
      }
      //-- This isn't within an existing alert radius, but it overlaps.
      //-- So, Make this the old alert invisible, and set the new alert to 
      //-- not retrigger the audio.
      else if (distanceSqr <= 2*BAlertManager::cDistanceSqr) 
      {
         pAlert->setExpireTime(expireTime);
         pAlert->setDeleteTime(deleteTime);         
         pAlert->setVisible(false);
         overlapAlert = true;
      }
   }

   // Not overlapping an existing attack alert, so queue up the new alert.
   if (!existingAlert)
   {
      //-- We're creating a new alert. Make sure that this is not a stray bullet
      bool strayBullet = false;
      const BUnit *pAttackingUnit = gWorld->getUnit(attackingUnitID);
      if(pAttackingUnit)
      {
         bool warthogRamming = false;
         const BUnitActionCollisionAttack*  pAction = (const BUnitActionCollisionAttack*)pAttackingUnit->getActionByTypeConst(BAction::cActionTypeUnitCollisionAttack);
//-- FIXING PREFIX BUG ID 2139
         const BSquad* pSquad = pAttackingUnit->getParentSquad();
//--
         if (pAction && pSquad && (pSquad->getSquadMode() == BSquadAI::cModeHitAndRun))
            warthogRamming = true;

         if( (pAttackingUnit->getAttackTargetID() != defenderUnitID) && !warthogRamming )
            strayBullet = true;
      }

      if(!strayBullet)
      {
         BAlert* pNewAlert = createAlert();
         if (!pNewAlert)
            return (false);
         pNewAlert->setType(AlertType::cAttack);
         pNewAlert->setLocation(pos);
         pNewAlert->setCreateTime(createTime);
         pNewAlert->setExpireTime(expireTime);
         pNewAlert->setDeleteTime(deleteTime);
         pNewAlert->setEntityID(defenderUnitID);
         pNewAlert->setAttackerID(attackingUnitID);
         pNewAlert->setQueued(true);
         
         if(overlapAlert)
            pNewAlert->setPlayAlert(false);

         //-- Is this an attack on a base?
         bool attackOnBase = isAttackOnBase(pNewAlert);
         pNewAlert->setAttackOnBase(attackOnBase);

         setLastAttackAlertID(pNewAlert->getID());

         // We don't wait for them to become un-queued to notify the music manager about them.
         /*if (gUserManager.getPrimaryUser()->getPlayerID() == getPlayerID() || (gGame.isSplitScreen() && gUserManager.getSecondaryUser()->getPlayerID() == getPlayerID()))
            gWorld->getWorldSoundManager()->getMusicManager()->attackStarted(pos, attackingUnitID, defenderUnitID);*/
      }
   }

   return true;
}


//==============================================================================
//==============================================================================
bool BAlertManager::createFlareAlert(BVector pos, BPlayerID flaringPlayerID)
{
   if (isAlertUIActive())
   {
      //if (!gConfig.isDefined(cConfigFlashGameUI))
      //   gMiniMap.triggerFlare(pos, flaringPlayerID, -1);
      //else
      gUIManager->addMinimapFlare(pos, flaringPlayerID, -1);

      gUI.playRumbleEvent(BRumbleEvent::cTypeFlare);
   }

   BAlert* pNewAlert = createAlert();
   if (!pNewAlert)
      return (false);
   const DWORD createTime = gWorld->getGametime();
   const DWORD expireTime = createTime + BAlertManager::cFlareAlertExpireTime;
   const DWORD deleteTime = createTime + BAlertManager::cFlareAlertDeleteTime;

   pNewAlert->setType(AlertType::cFlare);
   pNewAlert->setLocation(pos);
   pNewAlert->setCreateTime(createTime);
   pNewAlert->setExpireTime(expireTime);
   pNewAlert->setDeleteTime(deleteTime);
   pNewAlert->setFlaringPlayerID(flaringPlayerID);
   pNewAlert->setQueued(false);

   setNextGotoAlertID(pNewAlert->getID());

   // If this is an alert from our ally, notify the AI about it.
//-- FIXING PREFIX BUG ID 2141
   const BPlayer* pPlayer = getPlayer();
//--
   if (pPlayer->isAlly(flaringPlayerID, false))
      setLastAllyFlareAlertID(pNewAlert->getID());

   return (true);
}


//==============================================================================
//==============================================================================
void BAlertManager::update(void)
{
   BStaticSimArray<BAlert*, 32> queuedAttackAlerts;
   const DWORD currentGameTime = gWorld->getGametime();
   bool alertUIActive = isAlertUIActive();

   // Delete expired alerts, ready queued alerts, and display active alerts.
   uint i = 0;
   while (i < mAlertIDs.getSize())
   {
      BAlert* pAlert = getAlert(mAlertIDs[i]);
      if (!pAlert)
      {
         //BASSERTM(pAlert, "Very bad error if this is null!");
         deleteAlert(mAlertIDs[i]);
         continue;
      }
      else if (currentGameTime >= pAlert->getDeleteTime())
      {
         deleteAlert(mAlertIDs[i]);
         continue;
      }
      else if (currentGameTime >= pAlert->getExpireTime())
      {
         pAlert->setExpired(true);
      }
      else if (pAlert->getQueued())
      {
         if (pAlert->getType() == AlertType::cAttack)
            queuedAttackAlerts.add(pAlert);
      }
      else if (alertUIActive)
      {
         if (pAlert->getVisible())
         {
            if (pAlert->getType() == AlertType::cAttack)
            {
               //if (!gConfig.isDefined(cConfigFlashGameUI))
               //   gMiniMap.showAlert(*pAlert);
               //else
               gUIManager->addMinimapAlert(pAlert->getLocation());
            }
         }
      }
      i++;
   }

   // If it has been long enough and we have queued attack alerts.
   if (currentGameTime >= mNextAttackAlertTime && queuedAttackAlerts.getSize() > 0)
   {
      uint numQueuedAttackAlerts = queuedAttackAlerts.getSize();     

      // Unqueue all the queued attack alerts and play their notifications
      BAlertID newNextAlertID = cInvalidAlertID;
      for (uint i=0; i<numQueuedAttackAlerts; i++)
      {
         queuedAttackAlerts[i]->setQueued(false);

         if (newNextAlertID == cInvalidAlertID)
         {
            newNextAlertID = queuedAttackAlerts[i]->getID();
            setNextGotoAlertID(newNextAlertID);
         }

         if (alertUIActive && queuedAttackAlerts[i]->getPlayAlert() == true)
            playSingleAttackAlert(queuedAttackAlerts[i]);
      }

      // Update the cooldown time.
      mNextAttackAlertTime = currentGameTime + BAlertManager::cAttackAlertCooldown;
   }
}


//==============================================================================
//==============================================================================
uint BAlertManager::getFlareAlertIDs(BAlertIDArray& flareAlertIDs)
{
   flareAlertIDs.resize(0);
   uint numAlerts = mAlertIDs.getSize();
   for (uint i=0; i<numAlerts; i++)
   {
      const BAlert* pAlert = getAlert(mAlertIDs[i]);
      if (!pAlert)
         continue;
      if (pAlert->getType() != AlertType::cFlare)
         continue;
      if (pAlert->getQueued())
         continue;
      //if (pAlert->getExpired())
      //   continue;
      flareAlertIDs.add(mAlertIDs[i]);
   }
   return (flareAlertIDs.getSize());
}


//==============================================================================
//==============================================================================
uint BAlertManager::getAttackAlertIDs(BAlertIDArray& attackAlertIDs)
{
   attackAlertIDs.resize(0);
   uint numAlerts = mAlertIDs.getSize();
   for (uint i=0; i<numAlerts; i++)
   {
      const BAlert* pAlert = getAlert(mAlertIDs[i]);
      if (!pAlert)
         continue;
      if (pAlert->getType() != AlertType::cAttack)
         continue;
      if (pAlert->getQueued())
         continue;
      //if (pAlert->getExpired())
      //   continue;
      attackAlertIDs.add(mAlertIDs[i]);
   }
   return (attackAlertIDs.getSize());
}


//==============================================================================
//==============================================================================
const BAlert* BAlertManager::getLastAllyFlareAlert() const
{
   const BPlayer* pPlayer = getPlayer();
   const BAlert* pAlert = getAlert(mLastAllyFlare);
   if (!pAlert)
      return (NULL);
   if (pAlert->getType() != AlertType::cFlare)
      return (NULL);
   BPlayerID flaringPlayerID = pAlert->getFlaringPlayerID();
   if (!pPlayer->isAlly(flaringPlayerID, false))
      return (NULL);
   return (pAlert);
}


//==============================================================================
//==============================================================================
void BAlertManager::setLastAllyFlareAlertID(BAlertID alertID)
{
   const BPlayer* pPlayer = getPlayer();
   const BAlert* pAlert = getAlert(alertID);
   if (!pAlert)
   {
      mLastAllyFlare = cInvalidAlertID;
      return;
   }
   if (pAlert->getType() != AlertType::cFlare)
   {
      mLastAllyFlare = cInvalidAlertID;
      return;
   }
   BPlayerID flaringPlayerID = pAlert->getFlaringPlayerID();
   if (!pPlayer->isAlly(flaringPlayerID, false))
   {
      mLastAllyFlare = cInvalidAlertID;
      return;
   }
   mLastAllyFlare = alertID;
}


//==============================================================================
//==============================================================================
const BAlert* BAlertManager::getLastAttackAlert() const
{
   const BAlert* pAlert = getAlert(mLastAttackAlert);
   if (!pAlert)
      return (NULL);
   if (pAlert->getType() != AlertType::cAttack)
      return (NULL);
   return (pAlert);
}


//==============================================================================
//==============================================================================
void BAlertManager::setLastAttackAlertID(BAlertID alertID)
{
   const BAlert* pAlert = getAlert(alertID);
   if (!pAlert)
   {
      mLastAttackAlert = cInvalidAlertID;
      return;
   }
   if (pAlert->getType() != AlertType::cAttack)
   {
      mLastAttackAlert = cInvalidAlertID;
      return;
   }
   mLastAttackAlert = alertID;
}

//==============================================================================
//==============================================================================
BAttackSeverity BAlertManager::getAttackSeverity(BEntityID defenderID, BEntityID attackerID)
{
   const BUnit *pDefender = gWorld->getUnit(defenderID);
   if (!pDefender)
      return (AttackSeverity::cNone);
   const BUnit *pAttacker = gWorld->getUnit(attackerID);
   if (!pAttacker)
      return (AttackSeverity::cSev1);
   float combatValue = gWorld->getAreaCombatValue(pAttacker->getPosition(), pAttacker->getLOS(), pAttacker->getPlayerID(), cRelationTypeAlly);
   if(combatValue <= gWorld->getWorldSoundManager()->getAttackSev1Value())
      return (AttackSeverity::cSev1);
   else if(combatValue <= gWorld->getWorldSoundManager()->getAttackSev2Value())
      return (AttackSeverity::cSev2);
   else
      return (AttackSeverity::cSev3);
}


//==============================================================================
//==============================================================================
void BAlertManager::playSingleAttackAlert(BAlert *pAlert)
{
   if (!isAlertUIActive(pAlert))
      return;

   // The alert is onscreen, so we don't need to play anything.
   BVector alertLoc = pAlert->getLocation();
   BUser* pUser = mpPlayer->getUser();
   if (!pUser)
      return;
      
   // FIXME: This is gonna be slow. 
   BRenderViewParams view(gRender.getViewParams());
   
   BMatrix mat;
   pUser->getCamera()->getViewMatrix(mat);
   view.setViewMatrix(mat);
   
   if (view.isPointOnScreen(alertLoc))
   {
      //-- If its onscreen but really far away, we'll still play it
      float distance = pUser->getCamera()->getCameraLoc().xzDistanceSqr(alertLoc);
      if (distance < BAlertManager::cMinAlertDistanceSqr)
         return;
   }

   bool attackOnBase = pAlert->getAttackOnBase();

   //-- Determine the correct sound
   //BAttackSeverity attackSev = getAttackSeverity(pAlert->getUnitID(), pAlert->getAttackerID());
   long soundType = -1;
   if(mpPlayer->getCiv()->getID() == gDatabase.getCivID("UNSC"))
   {         
      if(attackOnBase)
         soundType = BSoundManager::cSoundAttackNotificationUNSCBaseSev1;
      else
         soundType = BSoundManager::cSoundAttackNotificationUNSCSev1;             
   }
   else if(mpPlayer->getCiv()->getID() == gDatabase.getCivID("Covenant"))
   {
      if(attackOnBase)
         soundType = BSoundManager::cSoundAttackNotificationCovBaseSev1;
      else
         soundType = BSoundManager::cSoundAttackNotificationCovSev1;
   }

   //-- Play the sound
   if(soundType != -1)
      gUIGame.playSound(soundType);

   // Rumble
   gUI.playRumbleEvent(BRumbleEvent::cTypeAttack);
}

//==============================================================================
//==============================================================================
void BAlertManager::render(void)
{
   if(gConfig.isDefined(cConfigRenderAlerts) && gUserManager.getPrimaryUser()->getPlayerID() == getPlayerID())
   {
      uint numAlerts = mAlerts.getHighWaterMark();
      for(uint i = 0; i < numAlerts; i++)
      {  
         if(mAlerts.isInUse(i) == false)
            continue;

//-- FIXING PREFIX BUG ID 2142
         const BAlert* pAlert = (mAlerts.get(i));
//--
         if(pAlert)
         {         
            if(pAlert->getType() == AlertType::cAttack)
            {
               if(pAlert->getQueued() == false)
                  gTerrainSimRep.addDebugThickCircleOverTerrain(pAlert->getLocation(), BAlertManager::cDistance, 1.0f, cDWORDRed, 1.0f);
               if(pAlert->getQueued() == true)
                  gTerrainSimRep.addDebugThickCircleOverTerrain(pAlert->getLocation(), BAlertManager::cDistance, 1.0f, cDWORDGreen, 1.0f);
            }
            else if(pAlert->getType() == AlertType::cBaseDestruction)
            {
               gTerrainSimRep.addDebugThickCircleOverTerrain(pAlert->getLocation(), BAlertManager::cDistance, 1.0f, cDWORDPurple, 1.0f);
            }
            else if(pAlert->getType() == AlertType::cResearchComplere)
            {
               gTerrainSimRep.addDebugThickCircleOverTerrain(pAlert->getLocation(), BAlertManager::cDistance, 1.0f, cDWORDBlue, 1.0f);
            }
            else if(pAlert->getType() == AlertType::cTrainingComplete)
            {
               gTerrainSimRep.addDebugThickCircleOverTerrain(pAlert->getLocation(), BAlertManager::cDistance, 1.0f, cDWORDYellow, 1.0f);
            }
         }
      }
   }
}


//==============================================================================
//==============================================================================
bool BAlertManager::isAttackOnBase(const BAlert* pAlert)
{
   if(!pAlert)
   {
      BASSERT(0);
      return false;
   }

   //-- Look at the unit being attacked and determine if it is a base unit
//-- FIXING PREFIX BUG ID 2143
   const BUnit* pUnit = gWorld->getUnit(pAlert->getUnitID());
//--
   if(pUnit)
   {
      if(pUnit->getAssociatedBase() != cInvalidObjectID || pUnit->getAssociatedSettlement() != cInvalidObjectID)
      {         
         return true;
      }
   }
   return false;
}





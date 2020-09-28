//==============================================================================
// squad.cpp
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "physics.h"
#include "ai.h"
#include "aimission.h"
#include "squad.h"
#include "world.h"
#include "utilities.h"
#include "action.h"
#include "ActionManager.h"
#include "kb.h"
#include "pather.h"
#include "obstructionmanager.h"
#include "config.h"
#include "configsgame.h"
#include "database.h"
#include "platoon.h" 
#include "protoobject.h"
#include "protosquad.h"
#include "unit.h"
#include "formationmanager.h"
#include "Formation2.h"
#include "command.h"
#include "actionmanager.h"
#include "dopple.h"
#include "tactic.h"
#include "unitquery.h"
#include "civ.h"
#include "alert.h"
#include "SimOrderManager.h"
#include "SquadActionAttack.h"
#include "SquadActionWork.h"
#include "SquadActionMove.h"

#include "squadactioncarpetbomb.h"
#include "squadactiongarrison.h"
#include "squadactionungarrison.h"
#include "squadactionhitch.h"
#include "squadactionunhitch.h"
#include "squadactionchangemode.h"
#include "squadactionrepair.h"
#include "squadactiondetonate.h"
#include "squadactionrepairother.h"
#include "squadactioncloak.h"
#include "unitactiondetonate.h"
#include "unitactionhonk.h"
#include "squadactiondaze.h"
#include "squadactioncryo.h"
#include "squadactionjump.h"
#include "squadactionambientlife.h"
#include "usermanager.h"
#include "user.h"
#include "team.h"
#include "syncmacros.h"
#include "entityScheduler.h"
#include "commands.h"
#include "squadplotter.h"
#include "decalManager.h"
#include "squadactionplayblockinganimation.h"
#include "HPBar.h"
#include "game.h"
#include "squadactiontransport.h"
#include "worldsoundmanager.h"
#include "selectionmanager.h"
#include "ability.h"
#include "render.h"
#include "simhelper.h"
#include "unitactionrevive.h"
#include "scenario.h"
#include "uimanager.h"
#include "squadlosvalidator.h"
#include "econfigenum.h"
#include "Physics/Dynamics/Action/hkpAction.h"
#include "skullmanager.h"
#include "unitactionchargedrangedattack.h"
#include "unitactionsecondaryturretattack.h"
#include "scoremanager.h"

//E3.
#include "physicsinfomanager.h"
#include "physicsinfo.h"

#include "wwise_ids.h"

GFIMPLEMENTVERSION(BSquad, 6);
enum 
{
   cSaveMarkerSquad1=10000,
};


#ifndef _MOVE4
#define _MOVE4
#endif

#define FACING_ANGLE_EPSILON Math::fFastCos(Math::fDegToRad(2.0f))

#ifndef BUILD_FINAL
   BSquadStats BSquad::mStats;
   BEntityID sDebugSquadTempID;

   //#define DEBUG_MOVE4
#endif

#ifdef DEBUG_MOVE4
#define debugMove4 sDebugSquadTempID=mID, dbgSquadInternalTempID
#else
#define debugMove4 __noop
#endif

#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void dbgSquadInternal(BEntityID squadID, const char *format, ...)
{
   // Assemble caller's string.
   char buf[256];        
   va_list arglist;
   va_start(arglist, format);
   StringCchVPrintf(buf, _countof(buf), format, arglist);
   va_end(arglist);
   
   long lSpecificSquad = -1;

   gConfig.get(cConfigDebugSpecificSquad, &lSpecificSquad);
   bool bMatch = false;

   if (lSpecificSquad == -1 || ((lSpecificSquad != -1) && (lSpecificSquad == squadID.asLong())))
      bMatch = true;

   if (!bMatch)
      return;

   // Output.
   gConsole.output(cChannelSim, "SQUAD %d (%d): %s", squadID.asLong(), squadID.getIndex(), buf);
   //syncSquadData("debugMove4 --", buf);
}


//==============================================================================
//==============================================================================
void dbgSquadInternalTempID(const char *format, ...)
{
   // Assemble caller's string.
   char buf[256];        
   va_list arglist;
   va_start(arglist, format);
   StringCchVPrintf(buf, _countof(buf), format, arglist);
   va_end(arglist);

   // Call with preset ID.
   dbgSquadInternal(sDebugSquadTempID, buf);
}
#endif



//==============================================================================
//==============================================================================
//#define _DEBUG_SQUAD_ACTIONS
//#define _DEBUG_FORMATION
#define cDefaultMovementProjectionTime    1.0f
//==============================================================================
//==============================================================================
static const uint  cMinSquadChatterSize = 2;
static const float cMinSquadChatterSizePct = 0.2f;



//==============================================================================
//==============================================================================
BSquad::~BSquad()
{
   mActions.clearActions();
}

//==============================================================================
//==============================================================================
void BSquad::onRelease()
{
   stopExistSound(true);

   // release the decal handle -- kill doesn't get called if a game ends
   if (mSelectionDecal != -1)
      gDecalManager.destroyDecal(mSelectionDecal);
   mSelectionDecal = -1;

   gFormationManager.releaseFormation2(mpFormation);
   mpFormation = NULL;

   uint numChildren = mChildren.size();
   if (numChildren > 0)
   {
      for (uint i=0; i<numChildren; i++)
      {
         BUnit* pUnit = gWorld->getUnit(mChildren[i]);
         if (pUnit)
            pUnit->setParentID(cInvalidObjectID);
      }
   }
   mChildren.setNumber(0);
   mpCachedLeaderUnit = NULL;
   
   mVisibleSquads.resize(0);
   mVisibleEnemySquads.resize(0);
   
   //Remove all of our orders.
   if (!gWorldReset)
      removeAllOrders();
   else
      mOrders.clear();

   if (!gWorldReset)
      removeEffects(false);

   BEntity::onRelease();
}

//==============================================================================
//==============================================================================
void BSquad::init()
{
    // jce [9/29/2008] -- entity's init should be setting this up, but defaulting it to zero here first just to be paranoid
    mLastMoveTime = 0;

    BEntity::init();
    clearFlags();    
    setFlagAlive(true);    
    
    mProtoID = cInvalidProtoID;
    mSubUpdateNumber = 0;
    mChildren.clear();
    mpCachedLeaderUnit = NULL;
    mpFormation=NULL;
    mOldPosition=cOriginVector;
    mOldRight=cOriginVector;
    mOldUp=cOriginVector;
    mOldForward=cOriginVector;
    mLeashPosition=cOriginVector;
    mAnchorPosition=cOriginVector;
    mOrders.clear();
    mLastDamagedTime = 0;
    mLastAttackedTime = 0;    
    mDamageBank = 0.0f;
    mDamageBankTimer = -1.0f;
    mSelectionDecal = -1;    
    mSquadAI.init(this);
    mAIMissionID = cInvalidAIMissionID;
    mXP = 0.0f;
    mXPBank = 0.0f;
    mLevel = 0;
    mLastCommandType = BSimOrder::cTypeNone;
    mLastAbilityAttackTargetID = cInvalidObjectID;
    mChangingToSquadMode = -1;
    mRecoverType = -1;
    mRecoverTime = 0.0f;
    mMovementSoundTimer = -1.0f;
    mMovementSoundOn = false;
    mFlagHasActiveEffect = false;
    mKilledByRecycler = false;
    mStasisSpeedMult = 1.0f;
    mNumStasisEffects = 0;
    mFlagCloaked = false;
    mFlagCloakDetected = false;
    mFlagWantsToCloak = false;
    mMergedTypes = 0;
    mLastAttackTargetID.invalidate();
    mLastAttackPriority = -1;
    mMergeCount = 0;
    mDamageProxy = cInvalidObjectID;
    mLastTrueLOSCheckCRC = 0;
    mFlagHasTrueLOS = true;
    mFlagIsTransporting = false;
    mFlagStopShieldRegen = false;

    const BProtoSquad* pProtoSquad = getProtoSquad();
    if (pProtoSquad)
       mFlagNoPlatoonMerge = pProtoSquad->getFlagNoPlatoonMerge();
    else
       mFlagNoPlatoonMerge = false;

    #ifdef _MOVE4
    // DLM - by default, squads are collidable
    mFlagCollidable = true;
    #endif

    for (uint i = 0; i < cSmartTargetMax; i++)
       mSmartTargetValues[i] = 0;

    for (uint i=0; i<cMaximumSupportedTeams; i++)
       mKBSquadIDs[i].invalidate();


    for (uint i=0; i<cSquadSoundStateMax; i++)
       mChatterTimer[i] = -1.0f;

   mLastKnownLeaderProtoObjectID = cInvalidProtoID;
   mKillerPlayerID = cInvalidPlayerID;
   mKillerProtoID = cInvalidProtoID;
   mKillerProtoSquadID = cInvalidProtoID;

   mVisibleSquads.clear();
   mVisibleEnemySquads.clear();
} 

//==============================================================================
//==============================================================================
bool BSquad::initFromProtoSquad(const BProtoSquad* pProtoSquad, BObjectCreateParms& parms,
   BEntityIDArray* existingUnits)
{
   if (pProtoSquad->getFlagForceToGaiaPlayer())
      setPlayerID(cGaiaPlayer);

   //Misc sets.
   setFlagProtoSquad(true);
   setProtoID(pProtoSquad->getID());

   setFlagHasHPBar(pProtoSquad->getFlagHasHPBar());
   setFlagTiesToGround(!parms.mNoTieToGround && !pProtoSquad->getFlagNoTieToGround());
   setFlagSquadChatter(pProtoSquad->getFlagChatter());

   #ifdef _MOVE4
   // DLM This needs to come from the protodata.  
   mFlagCollidable = true;
   /*
   // DLM - get collidable flags for protoSquads
   setFlagCollidable(pProtoSquad->getFlagCollidable());
   */
   #endif

   //Allocate the formation.
   initFormation();

   //Create the units for this squad (or use existing units if we were given those).
   if (existingUnits)
   {
      uint number=existingUnits->getSize();
      for (uint i=0; i < number; i++)
         addChild((*existingUnits)[i]);
   }
   else
   {
      BObjectCreateParms childParms=parms;
      childParms.mPlayerID = getPlayerID();
      childParms.mLevel = 0;
      long numUnitNodes = pProtoSquad->getNumberUnitNodes();
      for (long i = 0; i < numUnitNodes; i++)
      {
         const BProtoSquadUnitNode& unitNode = pProtoSquad->getUnitNode(i);
         long unitCount = unitNode.mUnitCount;
         for (long j = 0; j < unitCount; j++)
         {
            childParms.mProtoObjectID = unitNode.mUnitType;
            childParms.mPosition = mPosition;
            childParms.mForward = mForward;
            childParms.mRight = mRight;

            BUnit *pObject = gWorld->createUnit(childParms);
            if (pObject && pObject->isAlive())
            {
               addChild(pObject->getID());
               addPersistentSquadActions(pObject);
            }
            // mrh - removed the break after N units since out of memory is a fatal fail
         }
      }
   }

   //Rip through our children.  If any have the 'IgnoreSquadAI' flag on their PO, set that
   //on us.
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BUnit *pUnit=gWorld->getUnit(mChildren[i]);
      if (pUnit && pUnit->getProtoObject() && (pUnit->getProtoObject()->getFlagIgnoreSquadAI()))
      {
         setFlagIgnoreAI(true);
         break;
      }
   }
   
   //Init our position.
   initPosition(false);

   //Make sure everyone is where they're supposed to be.
   settle();

   BPlayer* pPlayer = getPlayer();
   if (getFlagProtoSquad())
   {
      BProtoSquadID protoSquadID = getProtoSquadID();
      pPlayer->addSquadToProtoSquad(this, protoSquadID);
   }
   
   //Add to player SquadAI tracking.
   pPlayer->addSquadAISquad(this);
   refreshVisibleSquads();

   if (gConfig.isDefined(cConfigVeterancy) && gScenario.getFlagAllowVeterancy())
   {
      //Veterancy level
      int level = pProtoSquad->getLevel();
      if (parms.mLevel > level)
         level = parms.mLevel;
      // [10/8/2008 xemu] apply skull bonus levels
      if (!getPlayer()->isHuman())
         level = level + gCollectiblesManager.getBonusSquadLevels();
      if (level > 0)
         upgradeLevel(level, false);
   }

   if (mFlagHasShield)
   {
      BAction* pAction = gActionManager.createAction(BAction::cActionTypeSquadShieldRegen);
      if (pAction)
      {
         addAction(pAction);
         setFlagShieldDamaged(true);
      }
   }

   // Add player pop
   adjustPlayerPop(true);

   // Does anyone want to react to our birth
   createAudioReaction(cSquadSoundChatterReactBirth);

   // notify the world that we have been created.
   gWorld->notify(BEntity::cEventBuilt, mID, mPlayerID, 0);

   mSelectionDecal = gDecalManager.createDecal();
   mAIMissionID = cInvalidAIMissionID;

   // Initialize the turn radius data
   #ifdef NEW_TURNRADIUS
      if (isSquadAPhysicsVehicle() && getProtoObject()->getMovementType() != cMovementTypeAir)
      {
         mFlagUpdateTurnRadius = true;
         setTurnRadiusPos(mPosition);
         setTurnRadiusFwd(mForward);
      }
   #endif

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
// This is a little awkward but only units have tactics files, which means persistent squad actions must come from
// child units.  Perhaps this could be replaced by an action no attached to any order?
void BSquad::addPersistentSquadActions(BUnit* pChild)
{
   BTactic* pTactic = pChild->getTactic();

   if (pTactic)
   {
      for (long i = 0; i < pTactic->getNumberPersistentSquadActions(); i++)
      {
         const BProtoAction* pProtoAction = pTactic->getPersistentSquadAction(i);
         if(!pProtoAction)
            continue; 

         BActionType actionType = pProtoAction->getPersistentActionType();
         if(actionType == BAction::cActionTypeInvalid)
            actionType = pProtoAction->getActionType();

         BAction* pAction = mActions.getActionByType(actionType, true);

         if( !pAction )
         {
            pAction = gActionManager.createAction(actionType);
            if(pAction)
            {
               pAction->setProtoAction(pProtoAction);
               pAction->setFlagPersistent(true);
               pAction->setFlagFromTactic(true);
               addAction(pAction, NULL);
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BSquad::removePersistentTacticActions()
{
   mActions.clearPersistentTacticActions();
}

//==============================================================================
//==============================================================================
void BSquad::initFormation()
{   
   if (mpFormation != NULL)
      gFormationManager.releaseFormation2(mpFormation);
   mpFormation=gFormationManager.createFormation2();
   mpFormation->setOwner(this);

   //Set the formation type.
   const BProtoSquad* pProtoSquad=getProtoSquad();
   if (pProtoSquad)
      mpFormation->setType(pProtoSquad->getFormationType());
}

//=============================================================================
// BSquad::transform
// Helper version of function for transforming to new proto squad type.
//=============================================================================
void BSquad::transform(long newProtoSquadID, bool techUpgrade)
{
   const BProtoSquad *pOldProto = getProtoSquad();
   BProtoSquad *pNewProto = getPlayer()->getProtoSquad(newProtoSquadID);
   if (pOldProto && pNewProto && pOldProto != pNewProto)
      transform(pOldProto, pNewProto, techUpgrade);
}

//=============================================================================
// BSquad::transform
//=============================================================================
void BSquad::transform(const BProtoSquad* pOldProto, const BProtoSquad* pNewProto, bool techUpgrade)
{
   preTransform(pOldProto, pNewProto, techUpgrade);
   postTransform(pOldProto, pNewProto, techUpgrade);
}

//=============================================================================
// BSquad::preTransform
//=============================================================================
void BSquad::preTransform(const BProtoSquad* pOldProto, const BProtoSquad* pNewProto, bool techUpgrade)
{
#ifdef SYNC_Squad
   syncSquadData("BSquad::preTransform mID", mID.asLong());
   syncSquadData("BSquad::preTransform oldProto", pOldProto->getName());
   syncSquadData("BSquad::preTransform newProto", pNewProto->getName());
#endif

   // Add in new units
   int numNodes1=pNewProto->getNumberUnitNodes();
   for (int i=0; i<numNodes1; i++)
   {
      const BProtoSquadUnitNode& node1=pNewProto->getUnitNode(i);
      int newCount = node1.mUnitCount;
      int oldCount = 0;
      int numNodes2=pOldProto->getNumberUnitNodes();
      for (int j=0; j<numNodes2; j++)
      {
         const BProtoSquadUnitNode& node2=pOldProto->getUnitNode(j);
         if (node2.mUnitType == node1.mUnitType)
         {
            oldCount = node2.mUnitCount;
            break;
         }
      }
      if (newCount > oldCount)
      {
         for (int j=oldCount; j<newCount; j++)
         {
            BObjectCreateParms unitParms;
            unitParms.mType = BEntity::cClassTypeUnit;
            unitParms.mPlayerID = mPlayerID;
            unitParms.mProtoObjectID = node1.mUnitType;
            unitParms.mStartBuilt = true;
            unitParms.mPosition = mPosition;
            unitParms.mForward = mForward;
            unitParms.mRight = mRight;
            unitParms.mLevel = mLevel;
            const BUnit* pUnit = gWorld->createUnit(unitParms);
            if (pUnit)
            {
               addChild(pUnit->getID());
               notify(BEntity::cEventSquadUnitAdded, mID, pUnit->getID(), 0);
               if (isGarrisoned())
               {                  
                  const BEntityRef* pEntityRef = getContainingEntityRef();
                  if (pEntityRef)
                  {
                     BUnit* pContainingUnit = gWorld->getUnit(pEntityRef->mID);
                     if (pContainingUnit)
                     {
                        pContainingUnit->containUnit(pUnit->getID());
                        if (pContainingUnit->getSquad())
                           pContainingUnit->getSquad()->setFlagForceUpdateGarrisoned(true);
                     }
                  }
               }
            }
         }
         settle();
         if (gUserManager.getPrimaryUser()->getSelectionManager()->isSquadSelected(mID))
            gUserManager.getPrimaryUser()->getSelectionManager()->selectSquad(mID);
         if (gGame.isSplitScreen() && gUserManager.getSecondaryUser()->getSelectionManager()->isSquadSelected(mID))
            gUserManager.getSecondaryUser()->getSelectionManager()->selectSquad(mID);
      }
   }

   // Adjust pop
   BPlayer* pPlayer = getPlayer();
   if (pPlayer)
   {
      BPopArray oldPops;
      BPopArray newPops;
      pOldProto->getPops(oldPops);
      pNewProto->getPops(newPops);
      for (int i = 0; i < oldPops.getNumber(); i++)
      {
         bool matchFound = false;
         for (int j = 0; i < newPops.getNumber(); j++)
         {
            // Match, set to difference
            if (newPops[j].mID == oldPops[i].mID)
            {
               newPops[j].mCount = (newPops[j].mCount - oldPops[i].mCount);
               matchFound = true;
               break;
            }
         }
         // If no matching type found in newPops, add the negative of the old one
         if (!matchFound)
         {
            BPop pop = oldPops[i];
            pop.mCount = -pop.mCount;
            newPops.add(pop);
         }
      }
      for (int i = 0; i < newPops.getNumber(); i++)
      {
         pPlayer->adjustPopCount(newPops[i].mID, newPops[i].mCount);
      }
   }
}

//=============================================================================
// BSquad::postTransform
//=============================================================================
void BSquad::postTransform(const BProtoSquad* pOldProto, const BProtoSquad* pNewProto, bool techUpgrade)
{
   BProtoSquadID oldProtoID = pOldProto->getID();
   BProtoSquadID newProtoID = pNewProto->getID();

   BPlayer* pPlayer = getPlayer();

   if (!techUpgrade)
   {
      pPlayer->removeSquadFromProtoSquad(this, oldProtoID);
      setProtoID(newProtoID);
      pPlayer->addSquadToProtoSquad(this, newProtoID);
   }
   
   //Note: Transforming doesn't need to update the squadAISquads.
}

//==============================================================================
//==============================================================================
void BSquad::changeOwner(BPlayerID newPlayerID, bool makeUniqueProto, BEntityID sourceID)
{
   long oldPlayerID = getPlayerID();
   if (oldPlayerID == newPlayerID)
      return;

   //-- Wall tower code
   BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedWallTower);
   if(pRef)
   {
      BSquad* pOtherSquad = gWorld->getSquad(pRef->mID);
      if(pOtherSquad)
         pOtherSquad->changeOwner(newPlayerID, makeUniqueProto);
   }

   //DCP 07/24/07: Remove any current or queued orders and actions.
   removeOrders(true, true, true, false);
   removeActions();

   // Remove player pop
   adjustPlayerPop(false);

   // Count as a loss for the original player and a gain for the new player
   BPlayerID killerPlayerID = cInvalidPlayerID;
   long killerProtoID = cInvalidProtoID;
   long killerProtoSquadID = cInvalidProtoID;
   queryEntityID(sourceID, killerPlayerID, killerProtoID, killerProtoSquadID);

   gWorld->notify2(BEntity::cEventKilled, mPlayerID, getProtoObjectID(), getProtoSquadID(), newPlayerID, killerProtoID, killerProtoSquadID, mXP, mLevel);
   gWorld->notify(BEntity::cEventBuilt, mID, newPlayerID, 0);

   // Remove from the proto squad
   const BProtoSquad* pOldPS = getProtoSquad();
   BPlayer* pOldPlayer = gWorld->getPlayer(oldPlayerID);
   if (this->getFlagProtoSquad())
      pOldPlayer->removeSquadFromProtoSquad(this, getProtoSquadID());      
   //Remove from player SquadAI tracking.
   pOldPlayer->removeSquadAISquad(this);

   //DCP 07/24/07: Remove us from our platoon.
   BPlatoon* pPlatoon = gWorld->getPlatoon(mParentID);
   if (pPlatoon)
      pPlatoon->removeChild(this);

   // Alloc unique proto squad if needed
   const BProtoSquad* pUniquePS = NULL;
   if (makeUniqueProto || pOldPS->getFlagUniqueInstance())
   {
      BPlayer* pNewPlayer = gWorld->getPlayer(newPlayerID);
      pUniquePS = pNewPlayer->allocateUniqueProtoSquad(pOldPS, gWorld->getPlayer(oldPlayerID), getID());
      if (pUniquePS)
         setProtoID(pUniquePS->getID());
   }

   // Note:  The child unit's changeOwner sends a notify event that changes the squads mPlayerID before we set it below... weird.
   // Change our children units' owner
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
      {
         // If a unique proto squad was made for this ownership change, get the new proto object
         // for the this unit type to assign to the unit
         BProtoObjectID newPOID = cInvalidProtoObjectID;
         if (pOldPS && pUniquePS)
         {
            for (int j = 0; j < pOldPS->getNumberUnitNodes(); j++)
            {
               const BProtoSquadUnitNode& node = pOldPS->getUnitNode(j);
               if (node.mUnitType == pUnit->getProtoID())
               {
                  newPOID = pUniquePS->getUnitNode(j).mUnitType;
                  break;
               }
            }
         }
         // Change unit owner
         bool zhp = pUnit->getFlagDiesAtZeroHP();
         pUnit->setFlagDiesAtZeroHP(true);
         pUnit->changeOwner(newPlayerID, newPOID);
         pUnit->setFlagDiesAtZeroHP(zhp);
      }
   }

   // Change our owner
   setPlayerID(newPlayerID);

   // Add to the protosquad
   BPlayer* pNewPlayer = gWorld->getPlayer(newPlayerID);
   if (this->getFlagProtoSquad())
      pNewPlayer->addSquadToProtoSquad(this, getProtoSquadID());
   //Add to player SquadAI tracking.
   pNewPlayer->addSquadAISquad(this);

   // Add player pop
   adjustPlayerPop(true);
}

//==============================================================================
//==============================================================================
void BSquad::setProtoID(long protoID)
{
   mProtoID=protoID;
   #ifndef BUILD_FINAL
   const BProtoSquad *pPS=getProtoSquad();
   if (pPS)
      mEntityName=const_cast<BSimString*>(&(pPS->getName()));
   else
   {
      const BProtoObject* pPO=getProtoObject();
      if (pPO)
         mEntityName=const_cast<BSimString*>(&(pPO->getName()));
   }
   #endif
}

//==============================================================================
//==============================================================================
long BSquad::getProtoSquadID() const
{
   return (getFlagProtoSquad() ? mProtoID : cInvalidProtoID);
}

//==============================================================================
//==============================================================================
const BProtoSquad* BSquad::getProtoSquad() const
{
   if(!getFlagProtoSquad())
      return NULL;

   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return NULL;

   return pPlayer->getProtoSquad(mProtoID);
}

//==============================================================================
//==============================================================================
long BSquad::getProtoObjectID() const
{
   // If this changes be sure to look at BEntity::getProtoObjectForSquad too.
   // It basically copies this logic.
   if (getFlagProtoSquad())
   {
      BUnit *pLeaderUnit = getLeaderUnit();
      return (pLeaderUnit ? pLeaderUnit->getProtoID() : cInvalidProtoID);
   }
   else
   {
      return(mProtoID);
   }
}

//=============================================================================
//=============================================================================
bool BSquad::isKBAware() const
{
   // This is derived from the proto squad.
   const BProtoSquad* pProtoSquad = this->getProtoSquad();
   if (pProtoSquad)
      return (pProtoSquad->getFlagKBAware());

   // Catchall
   return (false);
}

//=============================================================================
//=============================================================================
BPlatoon* BSquad::getParentPlatoon() const
{
   return(gWorld->getPlatoon(mParentID));
}

//=============================================================================
//=============================================================================
BArmy* BSquad::getParentArmy() const
{
   BPlatoon *platoon=gWorld->getPlatoon(mParentID);
   if (!platoon)
      return(NULL);
   return(gWorld->getArmy(platoon->getParentID()));
}

//==============================================================================
//==============================================================================
long BSquad::computeObstructionType()
{
   // Squads that aren't collideable shouldn't have obstructions created for them,
   // meaning this shouldn't be called for them
   long obsType = BObstructionManager::cObsTypeUnknown;
   bool isFlyingSquad = isFlyingSquad_4();
   #ifdef _MOVE4
   if (!isMoving())
   {
      if (!getFlagCollidable() || isFlyingSquad)
         obsType = BObstructionManager::cObsTypeNonCollidableSquad;
      else
         obsType = BObstructionManager::cObsTypeCollidableStationarySquad;
      mpObstructionNode->mProperties |=	BObstructionManager::cObsPropertyMovableUnit;
   }
   else
   {
      if (!getFlagCollidable() || isFlyingSquad)
         obsType = BObstructionManager::cObsTypeNonCollidableSquad;
      else
         obsType = BObstructionManager::cObsTypeCollidableMovingSquad;
      mpObstructionNode->mProperties |=	BObstructionManager::cObsPropertyMovableUnit;
   }
   #else
   if (!isMoving())
   {
      obsType = BObstructionManager::cObsTypeCollidableStationarySquad;
      mpObstructionNode->mProperties |=	BObstructionManager::cObsPropertyMovableUnit;
   }
   else
   {
      obsType = BObstructionManager::cObsTypeCollidableMovingSquad;
      mpObstructionNode->mProperties |=	BObstructionManager::cObsPropertyMovableUnit;
   }
   #endif

   return obsType;
}

//==============================================================================
//==============================================================================
bool BSquad::addChild(BEntityID id)
{
   //Get the unit.
   BUnit* pChild=gWorld->getUnit(id);
   if (!pChild)
      return (false);

   //Remove from its old squad, if there is one.
   if (pChild->getParentID() != cInvalidObjectID)
   {
      BSquad* pParent=pChild->getParentSquad();
      if (pParent && !pParent->removeChild(pChild->getID(), true))
         return(false);
   }

   //Add it.
   BASSERTM(pChild->isAlive(), "A dead unit is being added as a child to a squad.");
   if (pChild->isAlive())
   {
      // Mark our old visibility to the teams.
      BBitVector oldSquadTeamVis;
      long numTeams = gWorld->getNumberTeams();
      for (long i=0; i<numTeams; i++)
      {
         if (isVisible(i))
            oldSquadTeamVis.set(i);
      }

      long index=mChildren.uniqueAdd(id);
      if (index == -1)
         return (false);
      pChild->setParentID(mID);
      refreshLeaderUnit();

      // propagate the child's IsDoneBuilding flag to the squad
      setFlagIsDoneBuilding(pChild->getFlagIsDoneBuilding());

      //Add it to our formation.
      mpFormation->addChild(pChild->getID());

      //Flags.
      if (pChild->getFlagHasShield())
         setFlagHasShield(true);
      //mobility
      if (!pChild->isMobile())
         setFlagNonMobile(true);

      #ifdef _MOVE4
      // DLM 6/5/8 flying means noncollidable squad..
      if (pChild->getFlagFlying())
      {
         mFlagCollidable = false;
         updateObstruction();
      }
      #endif

      // if the targetting selection is set, set it on the child
      if (mFlagTargettingSelectionOn)
         pChild->setTargettingSelection(true);

      //If we weren't visible to a team before but are now, do the KB thing for this squad.
      for (long i=0; i<numTeams; i++)
      {
         if (!oldSquadTeamVis.isSet(i) && isVisible(i))
         {
            BKB* pKB = gWorld->getKB(i);
            if (pKB)
            {
               pKB->updateSquad(this);
               pKB->acquireVisToSquad(this);
            }
         }
      }

      return (true);
   }
   else
   {
      return (false);
   }
}

//==============================================================================
//==============================================================================
void BSquad::removeInvalidChildren()
{
   long numChildren = getNumberChildren();
   for (long i = numChildren - 1; i >= 0; i--)
   {
      // If a child doesn't exist, remove it from the list
      BEntityID childID = getChild(i);
      if (!gWorld->getUnit(childID))
         mChildren.removeIndex(i);
   }
   refreshLeaderUnit();
}

//==============================================================================
//==============================================================================
bool BSquad::removeChild(BEntityID id, bool fixup)
{
   BUnit* pChild=gWorld->getUnit(id);
   if (!pChild || (pChild->getParentID() != mID))
      return (false);

   // Mark our old visibility to the teams.
   BBitVector oldSquadTeamVis;
   long numTeams = gWorld->getNumberTeams();
   for (long i=0; i<numTeams; i++)
   {
      if (isVisible(i))
         oldSquadTeamVis.set(i);
   }

   //Remove it from the formation.
   mpFormation->removeChild(pChild->getID());

   // determine what killed our unit and store for future use
   if (mChildren.getSize() == 1)
   {
      mLastKnownLeaderProtoObjectID = getProtoObjectID();
      bool killedByRecyler;
      pChild->determineKiller(mKillerPlayerID, mKillerProtoID, mKillerProtoSquadID, killedByRecyler);
      mKilledByRecycler = killedByRecyler;
   }

   //Remove it from us.
   mChildren.removeValue(id);
   pChild->setParentID(cInvalidObjectID); 
   refreshLeaderUnit();

   // remove any targetting selection
   if (mFlagTargettingSelectionOn)
      pChild->setTargettingSelection(false);

   //If we were visible to a team before but are not now, do the KB thing for this squad.
   for (long i=0; i<numTeams; i++)
   {
      if (oldSquadTeamVis.isSet(i) && !isVisible(i))
      {
         BKB* pKB = gWorld->getKB(i);
         if (pKB)
         {
            pKB->updateSquad(this);
            pKB->loseVisToSquad(this);
         }
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
BEntityID BSquad::getChild(long index) const
{
   if (index < 0 || index >= mChildren.getNumber())
      return (cInvalidObjectID);
   else
      return (mChildren[index]);
}

//==============================================================================
//==============================================================================
bool BSquad::containsChild(BEntityID unitID) const
{
   if(mChildren.find(unitID) != -1)
      return true;
   else
      return false;   
}

//==============================================================================
//==============================================================================
bool BSquad::allChildrenInList(const BEntityIDArray& list, bool mustContainAllListedUnits /*= true*/)
{
   //-- compare a list against the children and make sure all the children exist in that list
   //-- if mustContainAllListedUnits=true, then every unit in the list must be represented in the child array
   //-- otherwise, the each member of the child array just has to appear in the passed in list (the list can contain units
   //-- that are not in this squad.

   //-- walk through the list
   long numberContained = 0;
   long count = list.getNumber();
   for (long i=0; i < count; i++)
   {
      //-- try to find representation for an item on the list
      //-- count the number of times this succeeds
      //-- if we have to succeed every time, then return false when we fail
      if (mChildren.find(list[i]) != -1) 
      {
         numberContained++;
      }
      else if (mustContainAllListedUnits)
         return false;
   }

   if (numberContained == mChildren.getNumber())
      return true;
   
   return false; 
}

//==============================================================================
//==============================================================================
long BSquad::stopAllChildren(DWORD state)
{
   long numStopped = 0;

   long count = mChildren.getNumber();
   for (long i=0; i < count; i++)
   {
      BUnit* pUnit = gWorld->getUnit(mChildren[i]);
      if (pUnit)
      {
         //if (state == BFormationPosition::cStateAny || (mpFormation->getPositionState(pUnit->getFormationIndex()) == state))
         //{
            pUnit->stop();
            numStopped++;
         //}
      }
   }

   return numStopped;
}

//==========================================================================================
//=========================================================================================
bool BSquad::transferChildren(BSquad* pSquadTo)
{
   // This will transfer the children from this squad to a target squad.
   // The function does a blind transfer so it is up to the caller to not break any squad
   // game mechanics.
   if (!pSquadTo)
   {
      return (false);
   }

   uint numFromUnits = getNumberChildren();
   for (int i = (numFromUnits - 1); i >= 0; i--)
   {
      BEntityID fromUnit = getChild(i);
      if (!removeChild(fromUnit, false))
      {
         return (false);
      }
      if (!pSquadTo->addChild(fromUnit))
      {
         return (false);
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::canJump() const
{
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      if (!pUnit->canJump())
         return (false);
   }
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::isChildLeashed(BEntityID id, float fudgeDistance) const
{
   BVector leashLocation;
   return (isChildLeashed(id, fudgeDistance, leashLocation));
}

//==============================================================================
//==============================================================================
bool BSquad::isChildLeashed(BEntityID id, float fudgeDistance, BVector& leashLocation) const
{
   BDEBUG_ASSERT(mpFormation);
   //Ignore if we can't get the desired location.
   if (!getDesiredChildLocation(id, leashLocation))
      return (true);

   //Get the leash limit.
   const BUnit* pUnit=gWorld->getUnit(id);
   BDEBUG_ASSERT(pUnit);
   float leashLimit=gDatabase.getUnitLeashLength()+fudgeDistance;

   //We're leashed if we're within the leash distance of our desired position.
   //DCP 09/12/07: Turning this off for now.
   //float leashDistance=pUnit->calculateXZDistance(leashLocation);
   //if (leashLimit > leashDistance)
   //   return (true);

   //We're leashed if we're inside the radius of the squad plus the leash distance.
   float centerDistance=pUnit->calculateXZDistance(mPosition);
   float centerLeashLimit=mpFormation->getRadius()+leashLimit;
   if (centerDistance < centerLeashLimit)
      return (true);

   //Fail.
   return (false);
}

//==============================================================================
//==============================================================================
bool BSquad::areAllChildrenLeashed(float fudgeDistance) const
{
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      if (!isChildLeashed(mChildren[i], fudgeDistance))
         return (false);
   }
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::isChildInRange(bool meleeRange, BEntityID id, BSimTarget target, float minRange, float maxRange, float childMaxRangeFudgeDistance) const
{
   //Get the child.
   const BUnit* pUnit=gWorld->getUnit(id);
   BDEBUG_ASSERT(pUnit);

   //Get the desired position for the child.
   BVector unitDesiredPosition;
   if (!getDesiredChildLocation(pUnit->getID(), unitDesiredPosition))
      return (false);

   //Get the squad's and unit's distance to the target (at the desired position for the unit).
   //We must have a valid ID (unit or squad) or position or we fail.
   float unitTargetDistance;
   float unitTargetDistanceAtDesiredPosition;
   float squadTargetDistance, squadTargetDistanceToCenter;
   if (target.getID().isValid())
   {
      if (target.getID().getType() == BEntity::cClassTypeUnit)
      {
         const BUnit* pTarget=gWorld->getUnit(target.getID());
         if (!pTarget || !pTarget->getParentSquad())
            return (false);

         //If our target is attached to the child unit, we always return true.
         if (pTarget->getFlagAttached() && pUnit->findAttachment(pTarget->getID()))
            return (true);

         //MeleeRange denotes that we want true unit-to-unit distance (thus it only 
         //applies for unit targets).
         if (meleeRange)
         {
            unitTargetDistance=pUnit->calculateXZDistance(pTarget);
            unitTargetDistanceAtDesiredPosition=pUnit->calculateXZDistance(unitDesiredPosition, pTarget);
         }
         else
         {
            unitTargetDistance=pUnit->calculateXZDistance(pTarget->getParentSquad());
            unitTargetDistanceAtDesiredPosition=pUnit->calculateXZDistance(unitDesiredPosition, pTarget->getParentSquad());
         }
         squadTargetDistance=calculateXZDistance(pTarget->getParentSquad());
         squadTargetDistanceToCenter=calculateXZDistance(pTarget->getParentSquad()->getPosition());
      }
      else if (target.getID().getType() == BEntity::cClassTypeSquad)
      {
         const BSquad* pTarget=gWorld->getSquad(target.getID());
         if (!pTarget)
            return (false);
         unitTargetDistance=pUnit->calculateXZDistance(pTarget);
         unitTargetDistanceAtDesiredPosition=pUnit->calculateXZDistance(unitDesiredPosition, pTarget);
         squadTargetDistance=calculateXZDistance(pTarget);
         squadTargetDistanceToCenter=calculateXZDistance(pTarget->getPosition());
      }
      else
         return (false);
   }
   else if (target.isPositionValid())
   {
      unitTargetDistance=pUnit->calculateXZDistance(target.getPosition());
      unitTargetDistanceAtDesiredPosition=pUnit->calculateXZDistance(unitDesiredPosition, target.getPosition());
      squadTargetDistanceToCenter=squadTargetDistance=calculateXZDistance(target.getPosition());
   }
   else
      return (false);

   //The squad must be within range.
   if ((squadTargetDistanceToCenter < minRange) || (squadTargetDistance > maxRange))
      return (false);

   //Figure out the "range" to use for the unit check.  If we're not doing melee range
   //and the distance from the unit's desired position is more than the squad to squad
   //distance, we use that.  Note that we only check MaxRange on the unit.  We assume
   //the squad takes care of the min range.
   float distanceToUse;
   if (!meleeRange && (unitTargetDistanceAtDesiredPosition > maxRange))
      distanceToUse=unitTargetDistanceAtDesiredPosition;
   else
      distanceToUse=maxRange;
   //Add in the fudge factor.
   distanceToUse+=childMaxRangeFudgeDistance;

   //Now, do the actual check.
   // TRB 3/24/08 - Added leash check to get vulture barrage to work when targeted on position out of LOS (bug #4993).
   // The squad was moving to within range of the target but the unit was slightly outside of range but within leash distance.
   // Since range checking should be squad based, this unit distance check should probably go away.
   if (unitTargetDistance < distanceToUse)
      return (true);
   else if (isChildLeashed(id))
      return (true);
   return (false);
}

//==============================================================================
//==============================================================================
bool BSquad::isSquadAPhysicsVehicle() const
{
   if (getNumberChildren() == 1)
   {
      const BUnit* pUnit = gWorld->getUnit(getChild(0));
      if (pUnit)
      {
         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         if(pProtoObject)
         {
            BPhysicsInfo *pInfo = gPhysicsInfoManager.get(pProtoObject->getPhysicsInfoID(), true);
            if(pInfo && pInfo->isVehicle())
               return (true);
         }
      }
   }
   return (false);
}

//==============================================================================
//==============================================================================
BEntityID BSquad::getHigherMovementPriority(BEntityID child1, BEntityID child2) const
{
   uint child1Priority=mpFormation->getChildPriority(child1);
   uint child2Priority=mpFormation->getChildPriority(child2);
   if (child2Priority < child1Priority)
      return (child2);
   return (child1);
}

//==============================================================================
//==============================================================================
uint BSquad::getMovementPriority(BEntityID childID) const
{
   return (mpFormation->getChildPriority(childID));
}

//==============================================================================
//==============================================================================
bool BSquad::update(float elapsedTime)
{
   // store off values for interpolation/sub-update
   mOldPosition = mPosition;
   mOldForward = mForward;
   mOldRight = mRight;
   mOldUp = mUp;
   mSubUpdateNumber = gWorld->getSubUpdate();

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::updateAsync(float elapsedTime)
{
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::updatePreAsync(float elapsedTime)
{
   #ifdef SYNC_Squad
   syncSquadData("BSquad::update mID", mID.asLong());
   if (getProtoSquad())
   {
      syncSquadData("BSquad::update name", getProtoSquad()->getName());
   }
   else if (getProtoObject())
   {
      syncSquadData("BSquad::update name", getProtoObject()->getName());
   }
   syncSquadData("BSquad::update mPosition", mPosition);
   #endif
   
   // Flip direction if reverse moving
   if (getReverseMove() && !mReverseMoveDone)
   {
      mReverseMoveDone = true;
      setForward(-getForward());
      calcRight();
      calcUp();
      updateObstruction();
   }
   else if (getReverseMove() && mReverseMoveDone && !getFlagMoving())
   {
      mReverseMoveDone = false;
   }

#ifndef BUILD_FINAL
   // See if we need to update the counts
   BTimer localTimer;

   if (gWorld->getUpdateNumber() != mStats.mReferenceFrame)
   {
      if (mStats.mUpdateEntityTime > mStats.mMaxEntityTime)
         mStats.mMaxEntityTime = mStats.mUpdateEntityTime;
      mStats.mAvgEntityTime.addSample(mStats.mUpdateEntityTime);
      mStats.mUpdateEntityTime = 0.0;

      if (mStats.mUpdateFormationTime > mStats.mMaxFormationTime)
         mStats.mMaxFormationTime = mStats.mUpdateFormationTime;
      mStats.mAvgFormationTime.addSample(mStats.mUpdateFormationTime);
      mStats.mUpdateFormationTime = 0.0;

      if (mStats.mUpdateGarrisonedSquadsTime > mStats.mMaxGarrisonedSquadsTime)
         mStats.mMaxGarrisonedSquadsTime = mStats.mUpdateGarrisonedSquadsTime;
      mStats.mAvgGarrisonedSquadsTime.addSample(mStats.mUpdateGarrisonedSquadsTime);
      mStats.mUpdateGarrisonedSquadsTime = 0.0;

      if (mStats.mUpdateHitchedSquadTime > mStats.mMaxHitchedSquadTime)
         mStats.mMaxHitchedSquadTime = mStats.mUpdateHitchedSquadTime;
      mStats.mAvgHitchedSquadTime.addSample(mStats.mUpdateHitchedSquadTime);
      mStats.mUpdateHitchedSquadTime = 0.0;

      if (mStats.mUpdateLeashTime > mStats.mMaxLeashTime)
         mStats.mMaxLeashTime = mStats.mUpdateLeashTime;
      mStats.mAvgLeashTime.addSample(mStats.mUpdateLeashTime);
      mStats.mUpdateLeashTime = 0.0;

      if (mStats.mUpdateOrdersTime > mStats.mMaxOrdersTime)
         mStats.mMaxOrdersTime = mStats.mUpdateOrdersTime;
      mStats.mAvgOrdersTime.addSample(mStats.mUpdateOrdersTime);
      mStats.mUpdateOrdersTime = 0.0;

      if (mStats.mUpdateSquadAITime > mStats.mMaxSquadAITime)
         mStats.mMaxSquadAITime = mStats.mUpdateSquadAITime;
      mStats.mAvgSquadAITime.addSample(mStats.mUpdateSquadAITime);
      mStats.mUpdateSquadAITime = 0.0;

      mStats.mReferenceFrame = gWorld->getUpdateNumber();
   }
#endif

   //Base Entity update/Kill check.
   //DCP 07/18/07: We really shouldn't bother with an entity update (which spawns action
   //updates) if we have no children.  You'll get a lot less asserts that way:)
   uint numChildren = mChildren.getSize();
   if (numChildren == 0)
   {
      setFlagDestroy(true);
      kill(false);
      return (false);
   }

#ifndef BUILD_FINAL
   localTimer.start();
#endif
   #ifdef SYNC_Squad
      syncSquadCode("BSquad::updatePreAsync entering BEntity::update");
   #endif
   bool bUpdate = BEntity::update(elapsedTime);
   #ifdef SYNC_Squad
      syncSquadCode("BSquad::updatePreAsync leaving BEntity::update");
   #endif
#ifndef BUILD_FINAL
   localTimer.stop();
   mStats.mUpdateEntityTime += localTimer.getElapsedSeconds();
#endif

   // Halwes - 10/10/2007 - Entity update (which spawns action updates) may have killed all children so check again.
   numChildren = mChildren.getSize();
   if ((numChildren == 0) || !bUpdate)
   {
      setFlagDestroy(true);
      kill(false);
      return (false);
   }

   #ifndef BUILD_FINAL
   BASSERTM(getProtoSquad(), "Squad does not contain a valid ProtoSquad!");
   for (uint i=0; i<numChildren; i++)
   {
      const BUnit* pChildUnit = gWorld->getUnit(mChildren[i]);
      BASSERTM(pChildUnit, "Squad contains a child unit that is not valid!");
      if (pChildUnit)
      {
         BASSERTM(this->mPlayerID == pChildUnit->getPlayerID(), "Squad contains a child unit that is owned by a different player!");
         if ((mPlayerID != pChildUnit->getPlayerID()) && ((mPlayerID == cGaiaPlayer) || (pChildUnit->getPlayerID() == cGaiaPlayer)))
         {
            BASSERTM(0, "Squad and child unit not both owned by Gaia.  Check ForceToGaiaPlayer flag in objects.xml and squads.xml");
         }
      }
   }
   #endif

   //Update our formation.
   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   updateFormation();
   #ifndef BUILD_FINAL
   localTimer.stop();
   mStats.mUpdateFormationTime += localTimer.getElapsedSeconds();
   #endif

   // Update our garrisoned squads
   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   updateGarrisonedSquads();
   #ifndef BUILD_FINAL
   localTimer.stop();
   mStats.mUpdateGarrisonedSquadsTime += localTimer.getElapsedSeconds();
   #endif

   // Update our hitched squads
   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   updateHitchedSquad();
   #ifndef BUILD_FINAL
   localTimer.stop();
   mStats.mUpdateHitchedSquadTime += localTimer.getElapsedSeconds();
   #endif

   //Update our leash.
   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   updateLeash();
   #ifndef BUILD_FINAL
   localTimer.stop();
   mStats.mUpdateLeashTime += localTimer.getElapsedSeconds();
   #endif

   //Update our orders.
   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   updateOrders();
   #ifndef BUILD_FINAL
   localTimer.stop();
   mStats.mUpdateOrdersTime += localTimer.getElapsedSeconds();
   #endif

   // Update turn radius
   #ifdef NEW_TURNRADIUS
      if (getFlagUpdateTurnRadius())
         updateTurning(elapsedTime);
   #endif


   // MPB [1/24/2008] - Rmove this super hack that takes a squad out of hit and run
   // mode if all it's physics units are inactive.  Eventually the physics units and
   // their squad should be more in sync when complete so the mode could be changed
   // when the squad is done moving
   if (mSquadAI.getMode() == BSquadAI::cModeHitAndRun)
   {
      BSquadActionAttack* pSQA = reinterpret_cast<BSquadActionAttack*>(getActionByType(BAction::cActionTypeSquadAttack));
      if (!(pSQA && pSQA->getOrder() && pSQA->getOrder()->getMode() == BSquadAI::cModeHitAndRun))
      {
         bool allStopped = true;
         for (uint i = 0; i < getNumberChildren(); i++)
         {
            const BEntity* pChild = gWorld->getEntity(getChild(i));
            if (pChild && pChild->getFlagPhysicsControl() && pChild->getPhysicsObject() && pChild->getPhysicsObject()->isActive())
            {
               allStopped = false;
               break;
            }
         }
         if (allStopped)
            mSquadAI.setMode(BSquadAI::cModeNormal);
      }
   }

   //Update our squadAI.
   #ifndef BUILD_FINAL
   localTimer.start();
   #endif

   if (!getFlagIgnoreAI())
      mSquadAI.update();

   #ifndef BUILD_FINAL
   localTimer.stop();
   mStats.mUpdateSquadAITime += localTimer.getElapsedSeconds();
   #endif
   
   //Handle Idle.
   syncSquadData("BSquad::update mOrders.getSize", mOrders.getNumber());
   if ((mOrders.getSize() == 0) &&
      !mActions.hasConflictsWithType(BAction::cActionTypeEntityIdle))
   {
      syncSquadCode("BSquad::update doIdle");
      doIdle();
   }

   //HP bars.
   //showHPBars(wasAttackedRecently() && !gWorld->isPlayingCinematic());
   
   updateChatter(elapsedTime);

   updateDamageBank(elapsedTime);

   updateRecover(elapsedTime);

   updateAbilityDuration(elapsedTime);

   updateEffects();

   updateMovementSound(elapsedTime);

   // [6/27/2008 xemu] minimap marker added for individual squads, but only show if marked in the proto squad
   if (getProtoSquad()->getMinimapScale() > 0)
      gUIManager->addMinimapIcon(this);

   return (true);
}

//==============================================================================
// BSquad::render()
//==============================================================================
void BSquad::render()
{
   showHPBars(wasAttackedRecently() && !gWorld->isPlayingCinematic());

   const BProtoSquad* pProtoSquad = getProtoSquad();
   bool show = wasAttackedRecently() && !gWorld->isPlayingCinematic();

   // Don't show hibernating unit's HPbars
   BUnit* pLeaderUnit = getLeaderUnit();
   if (pLeaderUnit && pLeaderUnit->isHibernating())
      show = false;

   bool bIsVisible = (isVisible(gUserManager.getPrimaryUser()->getTeamID()) || (gGame.isSplitScreen() && isVisible(gUserManager.getSecondaryUser()->getTeamID())));

   //-- if this has a real proto squad then use the proto squad hp data
   if (pProtoSquad && bIsVisible)
   {
      bool bShowHPBar = (getFlagHasHPBar() && (show || getFlagForceDisplayHP()));
      if (bShowHPBar)
      {
         gHPBar.displaySquadHP(this);

         if (hasDamageProxy())
         {
            BSquad* pDmgProxy = gWorld->getSquad(getDamageProxy());
            if (pDmgProxy)
               gHPBar.displaySquadHP(pDmgProxy);
         }
      }

      // show a recharge meter for abilities -- center it if the hp bar is not visible
      gHPBar.displayRechargeProgress(this, !bShowHPBar);

      // show bobble head if I have one
      if ((pProtoSquad->getBobbleHeadID() != -1) && (!pProtoSquad->getFlagOnlyShowBobbleHeadWhenContained()))
      {                  
         gHPBar.displayBobbleHead(this);
      }
      // [7/8/2008 xemu] alternatively, I could have a bobble-head Spartan riding inside
      else if (getFlagSpartanContainer())
      {                  
         // [7/8/2008 xemu] so look up who we have inside and pass it along instead, since it won't get its own render call
         BSquad *pContainedSquad = getGarrisonedSpartanSquad();
         // [8/19/2008 xemu] changing asset to just an if-check since apparently you can get in this situation after resignation 
         //BASSERT(pContainedSquad != NULL);
         if (pContainedSquad != NULL)
            gHPBar.displayBobbleHead(pContainedSquad);
      }
      else
      {                  
         gHPBar.displayVeterancy(this, !bShowHPBar);
      }

      setFlagForceDisplayHP(false);

      if (pProtoSquad->getFlagAlwaysRenderSelectionDecal() && !getFlagContainedSpartan())
      {
         DWORD color=gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextSelection);
         float intensity=6.0f;
	      gConfig.get(cConfigBaseDecalIntensity, &intensity);
         drawSquadSelection(color, true, intensity, false, true);
      }
   }
   
   BEntity::render();
}

//==============================================================================
//==============================================================================
void BSquad::updateGarrisonedFlag(void)
{      
   uint numUnits = mChildren.getSize();
   if (numUnits == 0)
   {
      setFlagGarrisoned(false);
      setFlagPassiveGarrisoned(false);
      setFlagInCover(false);
      return;
   }

   bool mobile = false;
   bool garrisoned = true;
   bool passive = false;
   bool inCover = false;
   for (uint i = 0; i < numUnits; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(mChildren[i]);
      if (pUnit)
      {         
         if (!pUnit->isGarrisoned())
         {
            garrisoned = false;
         }

         if (pUnit->isMobile())
         {
            mobile = true;
         }
         
         if (pUnit->getFlagPassiveGarrisoned())
         {
            passive = true;
         }

         if (pUnit->getFlagInCover())
         {
            inCover = true;
         }
      }
   }

   // If every unit in a squad is garrisoned, then the entire squad is garrisoned
   setFlagGarrisoned(garrisoned);

   if (garrisoned)
   {
      // If the squad is garrisoned it is non-mobile
      setFlagNonMobile(true);
      // If any unit in a garrisoned squad is passive garrisoned the squad is passive garrisoned
      setFlagPassiveGarrisoned(passive);
      // If any unit in a garrisoned squad is in cover then the squad is in cover
      setFlagInCover(inCover);
   }
   else
   {
      // If any unit in a non-garrisoned squad is non-mobile then the squad is non-mobile
      setFlagNonMobile(!mobile);
      // If a squad is not garrisoned then it is not passive garrisoned
      setFlagPassiveGarrisoned(false);
      // If a squad is not garrisoned then it is not in cover
      setFlagInCover(false);

      // ungarrisoned squads need to have platoons for valid movement
      if (!getParentPlatoon())
      {
         BObjectCreateParms platoonObjectParms;
         platoonObjectParms.mPlayerID = getPlayerID();
         BPlatoon* pPlatoon = gWorld->createPlatoon(platoonObjectParms);
         if (pPlatoon)
            pPlatoon->addChild(this);
      }
   }   
}

//==============================================================================
//==============================================================================
void BSquad::updateAttachedFlag()
{
   //-- If every unit in a squad is garrisoned, then the entire squad is garrisoned
   long numUnits = mChildren.getNumber();
   if(numUnits==0)
   {
      setFlagAttached(false);
      return;
   }

   bool mobile = false;
   bool attached = true;
   for(long i = 0; i < numUnits; i++)
   {
      BUnit *pUnit = gWorld->getUnit(mChildren[i]);
      if(pUnit)
      {
         mobile = pUnit->isMobile();
         if(!pUnit->isAttached())
         {
            attached = false;
            break;
         }
      }
   }

   setFlagAttached(attached);

   if(attached)
      setFlagNonMobile(true);
   else
      setFlagNonMobile(!mobile);
}

//==============================================================================
//==============================================================================
void BSquad::updateGarrisonedSquads()
{
   // No children no update
   if (getNumberChildren() == 0)
      return;

   // No position change no update
   if (!mForceUpdateGarrisoned && !getFlagMoving() && !getFlagTeleported())
      return;

   // Find all of our garrisoned squads
   BEntityIDArray squadList = getGarrisonedSquads();   

   // Set their squad and leash positions   
   uint numSquads = squadList.getSize();
   if (mFlagUpdateGarrisonedSquadPositions)
   {
      for (uint i = 0; i < numSquads; i++)
      {
         BSquad* pSquad = gWorld->getSquad(squadList[i]);
         if (pSquad)
         {
            pSquad->setPosition(getPosition());
            pSquad->setLeashPosition(getLeashPosition());
            pSquad->updateObstruction();

            pSquad->mForceUpdateGarrisoned = true;
            pSquad->updateGarrisonedSquads();
         }
      }
   }

   // Update squad mode if necessary
   const BProtoObject* pProtoObject = getProtoObject();
   if ((numSquads != 0) && pProtoObject && (pProtoObject->getGarrisonSquadMode() != -1) && (pProtoObject->getGarrisonSquadMode() != getSquadMode()))
   {
      // Change squad mode to garrison mode
      BSimOrder* pOrder = gSimOrderManager.createOrder();
      BASSERT(pOrder);
      pOrder->setOwnerID(getID());
      pOrder->setPriority(BSimOrder::cPrioritySim);
      pOrder->setMode((int8)pProtoObject->getGarrisonSquadMode());
      doChangeMode(pOrder, NULL);
   }

   if (mForceUpdateGarrisoned)
   {
      mForceUpdateGarrisoned = false;
   }
}

//==============================================================================
// Update the squad that is hitched to this squad
//==============================================================================
void BSquad::updateHitchedSquad()
{
   if (!hasHitchedSquad())
      return;

   BUnit* pTowUnit = getLeaderUnit();
   BASSERT(pTowUnit);   
   if (!pTowUnit)
      return;

   if (!pTowUnit->isMoving() && (getTurnRadiusState() == BAction::cStateDone))
      return;

   // Find the hitched squad
   BSquad* pHitchedSquad = gWorld->getSquad(getHitchedSquad());
   if (!pHitchedSquad)
      return;

   // Calculate follow point for hitched squad
   BVector towUnitPos = pTowUnit->getPosition();    
   BVector towDir = -pTowUnit->getForward();
   float offset = pTowUnit->getObstructionRadiusZ();
   BVector hitchOffset = towDir * offset;
   BVector followPos = towUnitPos + hitchOffset;

   // Calculate direction for hitched squad
   BVector hitchDir = followPos - pHitchedSquad->getPosition();
   hitchDir.y = 0.0f;
   hitchDir.normalize();
   BVector hitchRight;
   hitchRight.assignCrossProduct(cYAxisVector, hitchDir);
   hitchRight.y = 0.0f;
   hitchRight.normalize();

   // Calculate position for hitched squad
   offset = pHitchedSquad->getObstructionRadiusZ() + gDatabase.getHitchOffset();
   BVector hitchPosOffset = -hitchDir * offset;
   BVector hitchPos = followPos + hitchPosOffset;
   
   // Set hitched squad's positions and orientations
   pHitchedSquad->setPosition(hitchPos);
   pHitchedSquad->setLeashPosition(hitchPos);
   pHitchedSquad->setForward(hitchDir);
   pHitchedSquad->setRight(hitchRight);
   pHitchedSquad->calcUp();
   pHitchedSquad->setTurnRadiusPos(hitchPos);
   pHitchedSquad->setTurnRadiusFwd(hitchDir);
   pHitchedSquad->updateObstruction();
}

//==============================================================================
//==============================================================================
void BSquad::updateLeash()
{
   //Early out if we are not leashing (this only happens during wow-moments)
   if(mFlagIgnoreLeash)
      return;

   // If we have no platoon then ignore the leash at the squad level
   // TRB 10/28/08 - This was a pretty scary check that has been in for a year and a half because of the really
   // old way transporting was done.  A side effect was it would allow you to draw guys who didn't have a platoon
   // across the map, which would cause problems like not allowing the guy to auto attack.  So make sure guys
   // follow leashing rules even if they don't have a platoon.
   //if (getParentID() == cInvalidObjectID)
   //   return;

   // Early out if we are garrisoned, attached, hitched, or jumping
   if (getFlagGarrisoned() || getFlagAttached() || getFlagHitched() || getFlagJumping() || getFlagLockedDown())
      return;

   // Early out if we are currently garrisoning, ungarrisoning, hitching, or unhitching
   if (getFlagIsGarrisoning() || getFlagIsUngarrisoning() || getFlagIsHitching() || getFlagIsUnhitching() || (getCurrentOrChangingMode() == BSquadAI::cModeLockdown))
      return;
   
   const BSquadActionMove* pMoveAction=getNonPausedMoveAction_4();
   if (pMoveAction)
   {
      //If we're moving with a platoon move, just ignore this.
      if(pMoveAction->getFlagPlatoonMove())
         return;
      //If we're already heading to our leash, bail.
      if(pMoveAction->getTarget()->getPosition() == mLeashPosition)
         return;
   }

   //NOTE: This assumes things can be as simple as checking the distance to the
   //leash position.
   float distanceFromLeash=calculateXZDistance(mLeashPosition);
   float leashDistance=getLeashDistance();
   if (distanceFromLeash > leashDistance)
   {
      //Remove our current order(s).
      removeOrders(true, true, true, false);
      queueMove(mLeashPosition);
      return;
   }
   else
   {
      float leashDeadzone = getLeashDeadzone();
      DWORD idleDuration = getIdleDuration();
      DWORD leashRecallDelay = getLeashRecallDelay();
      if (distanceFromLeash > leashDeadzone && idleDuration > leashRecallDelay)
      {
         //Remove our current order(s).
         removeOrders(true, true, true, false);
         queueMove(mLeashPosition);
         return;
      }
   }
}

//==============================================================================
//==============================================================================
void BSquad::updateOrders()
{
   //NOTE: You can streamline these loops, but I'm going for clarity right now.

   //Check for executing or paused orders that have gone away on their own.
   BSimOrderEntry* pCurrentOrderEntry=NULL;
   for (uint i=0; i < mOrders.getSize(); i++)
   {
      //Ignore orders that are queued.
      if (mOrders[i].getState() == BSimOrderEntry::cStateQueued)
         continue;

      BSimOrder* pTempOrder=mOrders[i].getOrder();

      // If the order has the remove me flag set, then we 
      // or if the order should be removed when interruptable, 
      // and it is, remove it from our list now.
      bool removeOrder = (mOrders[i].getFlagRemoveMe() || (mOrders[i].getFlagRemoveMeWhenInterruptible() && isOrderInterruptible(&mOrders[i])));
      if(removeOrder)
      {         
         if (mOrders[i].getState() != BSimOrderEntry::cStateQueued)
            removeAllActionsForOrder(pTempOrder);
         if (pTempOrder->getOwnerID() == mID)
            gSimOrderManager.markForDelete(pTempOrder);
         mOrders.removeIndex(i);
         i--;
         continue;
      }

#ifndef BUILD_FINAL
#ifndef BUILD_PROFILE
      {
         uint index = UINT_MAX;
         if (!BSimOrder::mFreeList.getIndex(pTempOrder, index))
         {
            char buffer[1024];
            sprintf_s(buffer, sizeof(buffer), "BSquad::updateOrders - mOrders references a BSimOrder (%s) that has been released. ", mOrders[i].getTypeName());
            BASSERTM(0, buffer);
         }
      }
#endif
#endif

      //If this order has a 0 ref count, we're done with it.  Take it out of our
      //list and mark it for deletion if we own it.
      if (pTempOrder->getRefCount() <= 0)
      {
         mOrders.removeIndex(i);
         i--;
         if (pTempOrder->getOwnerID() == mID)
            gSimOrderManager.markForDelete(pTempOrder);
      }
      //If this is executing, it's our current order.
      else if (!pCurrentOrderEntry && (mOrders[i].getState() == BSimOrderEntry::cStateExecute))
      {
         // Confirm the actions are still executing for this order
         bool haveActions = false;
         for (int actionIndex = 0; actionIndex < getNumberActions(); actionIndex++)
         {
            BAction* pAction = getActionByIndex(actionIndex);
            if (pAction && (pAction->getOrder() == pTempOrder))
            {
               haveActions = true;
               break;
            }
         }
         // No actions so remove order
         if (!haveActions)
         {
            mOrders.removeIndex(i);
            i--;
            if (pTempOrder->getOwnerID() == mID)
               gSimOrderManager.markForDelete(pTempOrder);
            continue;
         }

         pCurrentOrderEntry=&(mOrders[i]);
      }
   }
   
   //If our current order is not interruptable, then return.
   if (pCurrentOrderEntry && !isOrderInterruptible(pCurrentOrderEntry))
      return;
   
   //If the first order in our list isn't executing, execute it/unpause it.
   if ((mOrders.getSize() > 0) && (mOrders[0].getState() != BSimOrderEntry::cStateExecute))
   {
      if (mOrders[0].getState() == BSimOrderEntry::cStatePaused)
         unpauseOrder(&(mOrders[0]));
      else
      {
         //Remove any executing or paused orders (which shouldn't be anything, but let's be safe).
         removeOrders(true, false, false, false);
         //Execute it.
         executeOrder(&(mOrders[0]));
      } 
   }
}

//==============================================================================
//==============================================================================
void BSquad::executeOrder(BSimOrderEntry* pOrderEntry)
{
   #ifndef BUILD_FINAL
   if ((pOrderEntry->getOrder()->getPriority() == BSimOrder::cPriorityUser) && gConfig.isDefined(cConfigRemakeSquadFormations))
   {
      mpFormation->setMakeNeeded(true);
      mpFormation->setReassignNeeded(true);
   }
   #endif

   debugMove4("executeOrder: %s", pOrderEntry->getTypeName());

   //Flip the state bit.
   pOrderEntry->setState(BSimOrderEntry::cStateExecute);

   //Create the right action.
   BSimOrder* pOrder=pOrderEntry->getOrder();
   BAction* pAction=NULL;

   if (pOrder->getPriority() == BSimOrder::cPriorityUser)
   {
      mLastCommandType = pOrderEntry->getType();

      // Remember the last target ID if this is an ability attack order.
      mLastAbilityAttackTargetID = cInvalidObjectID;
      if (pOrderEntry->getType() == BSimOrder::cTypeAttack)
      {
         if (pOrder->getTarget().isAbilityIDValid() && pOrder->getTarget().getAbilityID() != -1)
         {
            // Need to call determineTarget in case target passed in is a unit and needs to be converted to a squad.
            BSimTarget target;
            determineTarget(pOrder, target, true);
            if (target.isIDValid())
               mLastAbilityAttackTargetID = target.getID();
         }
      }
      else
      {
         clearBankXP();
      }
   }

   switch (pOrderEntry->getType())
   {
      case BSimOrder::cTypeAttack:
         doAttack(pOrder);
         return;

      case BSimOrder::cTypeGather:
         doGather(pOrder);
         break;

      case BSimOrder::cTypeCapture:
         doCapture(pOrder);
         return;

      case BSimOrder::cTypeJoin:
         doJoin(pOrder);
         return;

      case BSimOrder::cTypeMines:
         doMines(pOrder);
         return;

      case BSimOrder::cTypePlayBlockingAnimation:
         doPlayBlockingAnimation(pOrder, BObjectAnimationState::cAnimationStateMisc, pOrder->getMode(), true, false, false, cInvalidCueIndex, true);
         return;

      case BSimOrder::cTypeMove:
      {
         if (pOrder && (getParentID() != cInvalidObjectID) && (pOrder->getOwnerID() == getParentID()))
            // DLM 5/13/08 - I think this is just a bug in the mainline code, but nevertheless, I know for SURE in the 
            // the new movement I want to monitorOpps for all platoon moves.
         #ifdef _MOVE4
         {
            debugMove4("executeOrder: calling doMove..");
            doMove(pOrder, NULL, true, true, true);
         }
         #else
            doMove(pOrder, NULL, true, false, true);
         #endif
         else
            doMove(pOrder, NULL, false, false, true);
         return;

      }
      case BSimOrder::cTypeIdle:
      {
         syncSquadCode("BSquad::executeOrder doIdle");
         doIdle();
         return;
      }

      case BSimOrder::cTypeChangeMode:
         doChangeMode(pOrder, NULL);
         return;

      case BSimOrder::cTypeRepair:
         doRepair(pOrder);
         return;

      case BSimOrder::cTypeRepairOther:
         doRepairOther(pOrder);
         return;

      case BSimOrder::cTypeGarrison:
         debugMove4("executeOrder: calling doGarrison..");
         doGarrison(pOrder);
         break;

      case BSimOrder::cTypeUngarrison:
         doUngarrison(pOrder);
         break;

      //DCPTODO: This shouldn't be getting hit, but the AI keeps telling its buildings to move right now:).
      case BSimOrder::cTypeRallyPoint:
         break;

      case BSimOrder::cTypeUnpack:
         doUnpack(pOrder);
         break;

      case BSimOrder::cTypeHitch:
         doHitch(pOrder);
         break;

      case BSimOrder::cTypeUnhitch:
         doUnhitch(pOrder);
         break;

      case BSimOrder::cTypeTransport:
         doTransport(pOrder);
         break;

      case BSimOrder::cTypeDetonate:
         doDetonate(pOrder);
         break;

      case BSimOrder::cTypeCloak:
         doCloak(pOrder);
         break;

      case BSimOrder::cTypeJump:
         doJump(pOrder, BSimOrder::cJumpOrderJump);
         break;

      case BSimOrder::cTypeJumpGather:
         doJump(pOrder, BSimOrder::cJumpOrderJumpGather);
         break;

      case BSimOrder::cTypeJumpGarrison:
         doJump(pOrder, BSimOrder::cJumpOrderJumpGarrison);
         break;

      case BSimOrder::cTypeJumpAttack:
         doJump(pOrder, BSimOrder::cJumpOrderJumpAttack);
         break;

      case BSimOrder::cTypeJumpPull:
         doJump(pOrder, BSimOrder::cJumpOrderJumpPull);
         break;

      case BSimOrder::cTypePointBlankAttack:
         doPointBlankAttack(pOrder);
         break;

      //DCPTODO: Not done yet (and maybe never done).
      case BSimOrder::cTypeWander:
      case BSimOrder::cTypeBuild:
      case BSimOrder::cTypeHonk:
      case BSimOrder::cTypeRequestRepair:
      default:
         #ifndef BUILD_FINAL
            BSimString errorMsg;
            errorMsg.format("Unsupported sim order type %d", pOrderEntry->getType());
            BASSERTM(false, errorMsg);
         #endif
         break;
   }
   if (!pAction)
      return;

   //Fill it out.
   pAction->setTarget(pOrder->getTarget());
   pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);

   //Add it.
   addAction(pAction, pOrder);

#ifndef BUILD_FINAL
   // Trigger timings
   if (pOrder->getPriority() == BSimOrder::cPriorityTrigger)
   {
      setFlagIsTriggered(true);
   }
   else
   {
      setFlagIsTriggered(false);
   }
#endif
}

//==============================================================================
//==============================================================================
void BSquad::removeOrders(bool execute, bool paused, bool queued, bool uninterruptible, bool delayRemoval)
{
   BASSERT(execute || paused || queued);

   for (uint i=0; i < mOrders.getSize(); i++)
   {
      if (!queued && (mOrders[i].getState() == BSimOrderEntry::cStateQueued))
         continue;
      if (!paused && (mOrders[i].getState() == BSimOrderEntry::cStatePaused))
         continue;
      if (!execute && (mOrders[i].getState() == BSimOrderEntry::cStateExecute))
         continue;

      if (!uninterruptible && !isOrderInterruptible(&(mOrders[i])))
      {
         mOrders[i].setFlagRemoveMeWhenInterruptible(true);
         continue;
      }
   
      if(delayRemoval)
      {
         mOrders[i].setFlagRemoveMe(true);
         continue;
      }

      BSimOrder* pOrder=mOrders[i].getOrder();

      //If active, remove any actions that are a result of this order.
      if (mOrders[i].getState() != BSimOrderEntry::cStateQueued)
         removeAllActionsForOrder(pOrder);
      //If we own this order, mark it for deletion.
      if (pOrder->getOwnerID() == mID)
         gSimOrderManager.markForDelete(pOrder);
      //Remove this index.
      mOrders.removeIndex(i);
      i--;
   }
}

//==============================================================================
//==============================================================================
void BSquad::removeAutoGeneratedAttackMoveOrders()
{
   for (uint i = 0; i < mOrders.getSize(); i++)
   {
      BSimOrder* pOrder = mOrders[i].getOrder();
      if (!pOrder)
         continue;

      if (pOrder->getAttackMove() && pOrder->getAutoGeneratedAttackMove())
      {
         // Delay the removal
         mOrders[i].setFlagRemoveMe(true);
      }
   }
}

//==============================================================================
//==============================================================================
void BSquad::removeOrder(BSimOrder* pOrder, bool removeUninterruptible, bool delayRemoval)
{
   for (uint i=0; i < mOrders.getSize(); i++)
   {
      if (mOrders[i].getOrder() == pOrder)
      {
         if (!removeUninterruptible && !isOrderInterruptible(&(mOrders[i])))
         {
            mOrders[i].setFlagRemoveMeWhenInterruptible(true);
            continue;
         }

         if(delayRemoval)
         {
            mOrders[i].setFlagRemoveMe(true);
            continue;
         }

         //If active, remove any actions that are a result of this order.
         if (mOrders[i].getState() != BSimOrderEntry::cStateQueued)
            removeAllActionsForOrder(pOrder);
         //If we own this order, mark it for deletion.
         if (pOrder->getOwnerID() == mID)
            gSimOrderManager.markForDelete(pOrder);
         //Take it out of the list.
         mOrders.removeIndex(i);
         //Done.
         return;
      }
   }
}

//==============================================================================
//==============================================================================
BSimOrderType BSquad::getOrderType(const BCommand* pCommand) const
{
   BSimTarget target(pCommand->getTargetID());
   if (pCommand->getNumberWaypoints() > 0)
      target.setPosition(pCommand->getWaypoint(pCommand->getNumberWaypoints()-1));
   if (pCommand->getAbilityID() >= 0)
      target.setAbilityID(pCommand->getAbilityID());
   return(getOrderType(target, pCommand, NULL));
}

//==============================================================================
//==============================================================================
BSimOrderType BSquad::getOrderType(BSimTarget target, const BCommand* pCommand, const BSimOrder* pOrder) const
{
   //NOTE: This is supposed to use the BSimTarget target as much as possible.
   //It may NOT be the target presented in the command.

   const BProtoAction* pAction=getProtoActionForTarget(target.getID(), target.getPosition(), target.getAbilityID(), false);
   if (!pAction)
   {
      if (pCommand && pCommand->getType()==cCommandGame && ((BGameCommand*)pCommand)->getGameCommandType()==BGameCommand::cTypeRepair)
         return (BSimOrder::cTypeRepair);

      // TOMMYTODO:  This now allows moves when no proto action but a valid target ID is given.  Is this ok?
      //             This was done to prevent asserts from the command center repair trigger script that was
      //             tasking units to move using an entity ID.
      //if (!target.isPositionValid())
      if (!target.isPositionValid() && !target.getID().isValid())
      {
         if ((pCommand && pCommand->getSquadMode()!=-1) || (pOrder && pOrder->getMode()!=-1))
            return (BSimOrder::cTypeChangeMode);
         return(BSimOrder::cTypeNone);
      }
/*
      if (target.isPositionValid() &&
         !isMobile() &&
         getProtoObject() &&
         (getProtoObject()->getRallyPointType() != -1))
         return(BSimOrder::cTypeRallyPoint);
*/
      return(BSimOrder::cTypeMove);
   }

   switch (pAction->getActionType())
   {
      case BAction::cActionTypeUnitRangedAttack:
      case BAction::cActionTypeUnitSecondaryTurretAttack:
         return (BSimOrder::cTypeAttack);
      case BAction::cActionTypeUnitGather:
         return (BSimOrder::cTypeGather);
      case BAction::cActionTypeUnitCapture:
         return (BSimOrder::cTypeCapture);
      case BAction::cActionTypeUnitJoin:
         return (BSimOrder::cTypeJoin);
      case BAction::cActionTypeUnitGarrison:
         return (BSimOrder::cTypeGarrison);
      case BAction::cActionTypeUnitUngarrison:
         return (BSimOrder::cTypeUngarrison);
      case BAction::cActionTypeUnitDetonate:
         return (BSimOrder::cTypeDetonate);
      case BAction::cActionTypeUnitHonk:
         return (BSimOrder::cTypeHonk);
      case BAction::cActionTypeUnitMines:
         return (BSimOrder::cTypeMines);
      case BAction::cActionTypeUnitHitch:
         return (BSimOrder::cTypeHitch);
      case BAction::cActionTypeUnitUnhitch:
         return (BSimOrder::cTypeUnhitch);
      case BAction::cActionTypeUnitMove:
          return(BSimOrder::cTypeMove);
      case BAction::cActionTypeUnitJump:
         return (BSimOrder::cTypeJump);
      case BAction::cActionTypeSquadTransport:
         return(BSimOrder::cTypeTransport);
      case BAction::cActionTypeSquadRepairOther:
         return(BSimOrder::cTypeRepairOther);
      case BAction::cActionTypeSquadDetonate:
         return (BSimOrder::cTypeDetonate);
      case BAction::cActionTypeSquadChangeMode:
         return(BSimOrder::cTypeChangeMode);
      case BAction::cActionTypeSquadWander:
         return(BSimOrder::cTypeWander);
      case BAction::cActionTypeSquadCloak:
         return (BSimOrder::cTypeCloak);
      case BAction::cActionTypeSquadJump:
         return (BSimOrder::cTypeJump);
      case BAction::cActionTypeUnitJumpGather:
         return (BSimOrder::cTypeJumpGather);
      case BAction::cActionTypeUnitJumpGarrison:
         return (BSimOrder::cTypeJumpGarrison);
      case BAction::cActionTypeUnitJumpAttack:
         return (BSimOrder::cTypeJumpAttack);
      case BAction::cActionTypeUnitPointBlankAttack:
         return (BSimOrder::cTypePointBlankAttack);
      case BAction::cActionTypeUnitEnergyShield:
         return (BSimOrder::cTypeEnergyShield);
      case BAction::cActionTypeUnitInfantryEnergyShield:
         return (BSimOrder::cTypeInfantryEnergyShield);
   }
   return (BSimOrder::cTypeNone);
}

//==============================================================================
//==============================================================================
bool BSquad::queueOrder(const BCommand *pCommand, bool prepend)
{
   //gConsole.output(cChannelSim, "BSquad::queueOrderCommand, time=%d.", timeGetTime());
   //debug("BSquad::queueOrderCommand, time=%d.", timeGetTime());
   if (!pCommand)
      return (false);

   //Figure out what to do with this command.  In a few cases, we might just
   //do something with it directly.
   BSimOrderType orderType=getOrderType(pCommand);
   if (orderType == BSimOrder::cTypeRallyPoint)
   {
      for (uint i=0; i < mChildren.getSize(); i++)
      {
         BUnit* pUnit=gWorld->getUnit(mChildren[i]);
         if (pUnit)
            pUnit->setRallyPoint(pCommand->getWaypoint(0), cInvalidObjectID, mPlayerID);
      }
      return (true);
   }

   //Create the order.
   BSimOrder* pOrder=gSimOrderManager.createOrder();
   if (!pOrder)
      return (false);

   //Create the target.
   BSimTarget target(pCommand->getTargetID());

   // Make sure the last waypoint corresponds to the target entity position
   BDynamicVectorArray waypoints;
   waypoints = pCommand->getWaypointList();
   if (target.isIDValid() && (waypoints.getSize() > 0))
   {
      const BEntity* pEntity = gWorld->getEntity(target.getID());
      if (pEntity != NULL)
         waypoints.setAt(waypoints.getSize() - 1, pEntity->getPosition());
   }

   if (waypoints.getSize() > 0)
      target.setPosition(waypoints[waypoints.getSize()-1]);
   if (pCommand->getAbilityID() >= 0)
   {
      target.setAbilityID(pCommand->getAbilityID());
   }

   //Init it.
   pOrder->setOwnerID(mID);
   pOrder->setTarget(target);
   pOrder->setWaypoints(waypoints);
   pOrder->setPriority(BSimOrder::cPriorityUser);
   if (pCommand->getSquadMode() >= 0)
      pOrder->setMode(static_cast<int8>(pCommand->getSquadMode()));
   if ((pCommand->getType() == cCommandWork) && pCommand->getFlag(BWorkCommand::cFlagAttackMove))
      pOrder->setAttackMove(true);

   //Add it.
   if (!queueOrder(pOrder, orderType, prepend))
   {
      gSimOrderManager.markForDelete(pOrder);
      return (false);
   }
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::queueOrder(BSimOrder* pOrder, BSimOrderType orderType, bool prepend)
{
   //gConsole.output(cChannelSim, "BSquad::queueOrderSimOrder, time=%d.", timeGetTime());
   //debug("BSquad::queueOrderSimOrder, time=%d.", timeGetTime());
   //Add this order.  It's already been filled out by the caller (which may or
   //may not be us)
   if (!pOrder)
      return (false);

   //Do dupe checking.  Ignore any order that's a dupe of something already in the list.
   //Also figure out where to insert this in case we're not prepending.
   uint insertIndex=0;
   for (uint i=0; i < mOrders.getSize(); i++)
   {
      BSimOrder *pTempOrder=mOrders[i].getOrder();
      //Equality.
      if ( (pTempOrder == pOrder) || ((*pTempOrder == *pOrder) && !mOrders[i].hasRemovalPending()) )
         return (false);      

      //See if this is where this order would go.  If the temp order is higher or 
      //equal in priority, we want to insert this *AFTER* it.
      if (pOrder->getPriority() <= pTempOrder->getPriority())
         insertIndex=i+1;
   }
   BDEBUG_ASSERT(insertIndex <= mOrders.getSize()+1);

   //If we don't have an order type, figure one out.
   if (orderType == BSimOrder::cTypeNone)
      orderType=getOrderType(pOrder->getTarget(), NULL, pOrder);

   // TRB 7/30/08 - Changing this so X button does nothing when unit locked down.  Ignore
   // move orders in this case.
   if (orderType == BSimOrder::cTypeMove)
   {
      if (!canMoveForOrder(pOrder))
         return false;
   }

   //If we're not prepending or there are no orders, just add it and get out of here:)
   if (!prepend || (mOrders.getSize() == 0))
   {
      BSimOrderEntry soe;
      soe.setOrder(pOrder);
      soe.setType(orderType);
      soe.setState(BSimOrderEntry::cStateQueued);
      mOrders.insertAtIndex(soe, insertIndex);
      //return (true);
   }
   else
   {   
      //Else, if we're prepending, we have potentially more work to do.
      //Go through and pause any active orders with this new order's ID.
      for (uint i=0; i < mOrders.getSize(); i++)
      {
         if (mOrders[i].getState() != BSimOrderEntry::cStateExecute)
            continue;
         pauseOrder(&(mOrders[i]));
      }

      //Create the new order entry.
      BSimOrderEntry soe;
      soe.setOrder(pOrder);
      soe.setType(orderType);
      soe.setState(BSimOrderEntry::cStateQueued);
      //Add it at the front.
      insertIndex = 0;
      mOrders.insertAtIndex(soe, insertIndex);
   }

   //-- See if the newly added order can replace any of the other orders in the list.
   for(uint i=0; i < mOrders.getSize(); i++)
   {      
      if(i == insertIndex)
         continue; 

      bool result = mergeRedundantOrders(&mOrders[i], &mOrders[insertIndex]);      
      if(result)
         break;
   }

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::mergeRedundantOrders(BSimOrderEntry* pOldOrderEntry, BSimOrderEntry* pNewOrderEntry)
{
   if(!pOldOrderEntry || !pNewOrderEntry)
      return false;

   //-- Are these orders the same enough?
   if((pNewOrderEntry->getType() == pOldOrderEntry->getType()) && pNewOrderEntry->getOrder()->isSimilarOrder(pOldOrderEntry->getOrder()))
   {
      // TRB 7/22/08 - If the new order is an ability order and the squad can do its ability,
      // don't merge the orders.  This is necessary in case the old order has switched over
      // to a standard attack and the new order is being commanded now that the ability can
      // be done again.
      if (pNewOrderEntry->getOrder()->getTarget().isAbilityIDValid())
      {
         int abilityID = gDatabase.getSquadAbilityID(this, pNewOrderEntry->getOrder()->getTarget().getAbilityID());
         BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);

         // Check the proto object's ability disabled flag
         bool abilityEnabled = true;
         BUnit *pLeaderUnit = getLeaderUnit();
         if (pLeaderUnit && pLeaderUnit->getProtoObject() && pLeaderUnit->getProtoObject()->getFlagAbilityDisabled())
            abilityEnabled = false;
         if (pAbility && abilityEnabled && (!getFlagRecovering() || (getRecoverType() != pAbility->getRecoverType())))
         {
            // TRB 8/22/08 - Only do this if the action associated with the old order has executed the ability.  Otherwise
            // leave it alone (aka do the merge) to execute the ability.
            for (int actionIndex = 0; actionIndex < mActions.getNumberActions(); actionIndex++)
            {
               BAction *pAction = mActions.getAction(actionIndex);
               if (pAction && (pAction->getOrder() == pOldOrderEntry->getOrder()) && (pAction->getType() == BAction::cActionTypeSquadAttack))
               {
                  // Ability executed so don't want to merge so ability gets executed again
                  BSquadActionAttack *pAttackAction = reinterpret_cast<BSquadActionAttack*>(pAction);
                  if (pAttackAction->getFlagAbilityExecuted() || !pAttackAction->getFlagAbilityCommand())
                     return false;
               }
            }
         }
      }

      // TRB 7/28/08 - If the new order has higher priority it's easier to just blow away the old one
      // and queue the new one.  This has to do with the platoon creating move actions for the squad
      // now and those conflicting with existing move actions for the old order.  The move action
      // contains a notification action and fixing all that up is too tricky so it's better to just
      // blow away the old stuff.
      if (pNewOrderEntry->getOrder()->getPriority() > pOldOrderEntry->getOrder()->getPriority())
      {
         pOldOrderEntry->setFlagRemoveMe(true);
         return true;
      }

      //-- The order we've just added is the same as the current order. 
      //-- Throw out the old order and fixup all things that are referencing it.
      pNewOrderEntry->setState(pOldOrderEntry->getState());
      //-- Run through all our actions and all our childrens actions and if they were referencing the old order
      //-- then tell them to reference the new order.
      changeOrderForAllActions(pOldOrderEntry->getOrder(), pNewOrderEntry->getOrder());
      for(uint i=0; i < mChildren.getSize(); i++)
      {
         BUnit* pChild = gWorld->getUnit(mChildren[i]);
         if(!pChild)
            continue;
         pChild->changeOrderForAllActions(pOldOrderEntry->getOrder(), pNewOrderEntry->getOrder());         
      }

      //-- This order can go away now. 
      pOldOrderEntry->setFlagRemoveMe(true);

      return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
void BSquad::pauseOrder(BSimOrderEntry *pOrderEntry)
{
   BASSERT(pOrderEntry->getOrder());

   //If we can't pause the actions, just bail (which should lazily get the order nuked).
   if (!pauseAllActionsForOrder(pOrderEntry->getOrder()))
      return;
   //Increment the ref count for this order since we still want/need it around
   //but we may no longer have any actions that have refs in it.
   //pOrderEntry->getOrder()->incrementRefCount();
   //Pause our entry for it.
   pOrderEntry->setState(BSimOrderEntry::cStatePaused);
}

//==============================================================================
//==============================================================================
void BSquad::unpauseOrder(BSimOrderEntry *pOrderEntry)
{
   BASSERT(pOrderEntry->getOrder());

   //Decrement the ref count for the order.  See above.
   //pOrderEntry->getOrder()->decrementRefCount();
   //Unpause our entry for the order by setting the state back to execute.
   pOrderEntry->setState(BSimOrderEntry::cStateExecute);
   //Unpause our actions.
   unpauseAllActionsForOrder(pOrderEntry->getOrder());
}

//==============================================================================
//==============================================================================
const BSimOrderEntry* BSquad::getOrderEntry(bool execute, bool paused) const
{
   //Return the first order that matches.
   for (uint i=0; i < mOrders.getSize(); i++)
   {
      if (execute && (mOrders[i].getState() == BSimOrderEntry::cStateExecute))
         return ( &(mOrders[i]));
      if (paused && (mOrders[i].getState() == BSimOrderEntry::cStatePaused))
         return ( &(mOrders[i]));
   }
   
   return (NULL);
}

//==============================================================================
//==============================================================================
BSimOrder* BSquad::getOrderByID(uint orderID) const
{
   for (uint i = 0; i < mOrders.getSize(); i++)
   {
      BSimOrder* pOrder = mOrders[i].getOrder();
      if (pOrder && (pOrder->getID() == orderID))
         return pOrder;
   }
   return NULL;
}

//==============================================================================
//==============================================================================
bool BSquad::queueMove(BSimTarget target)
{
   //Create the order.
   BSimOrder* pOrder=gSimOrderManager.createOrder();
   if (!pOrder)
      return (false);

   //Init it.
   pOrder->setOwnerID(mID);
   pOrder->setTarget(target);

   //Add it.
   if (!queueOrder(pOrder, BSimOrder::cTypeMove))
   {
      gSimOrderManager.markForDelete(pOrder);
      return (false);
   }
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::queueAttack(BSimTarget target, bool attackMove, bool informPlatoon, bool autoGeneratedAttackMove)
{
   //Create the order.
   BSimOrder* pOrder=gSimOrderManager.createOrder();
   if (!pOrder)
      return (false);

   //Init it.
   pOrder->setOwnerID(mID);
   pOrder->setTarget(target);
   //Creepy, but necessary to give AttackMove the right priority.
   if (attackMove)
   {
      pOrder->setPriority(BSimOrder::cPriorityUser);
      pOrder->setAttackMove(true);
      pOrder->setAutoGeneratedAttackMove(autoGeneratedAttackMove);   // Differentiate between main attack move and squad AI generated ones
   }

   //Add it.  If AttackMove is true, then we need to prepend this order, therefore just
   //pass that in as the prepend value.
   if (!queueOrder(pOrder, BSimOrder::cTypeAttack, attackMove))
   {
      gSimOrderManager.markForDelete(pOrder);
      return (false);
   }
   // If we successfully queued the order, and it's an attack move, then inform the platoon.  It can direct other squads in the platoon
   // to attack this target as well, if they're not already engaged. 
   if (attackMove && informPlatoon)
   {
      BPlatoon *pPlatoon = getParentPlatoon();
      if (pPlatoon)
      {
         pPlatoon->attackMoveNotify(mID, target);
      }
   }
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::queueIdle()
{
   //Create the order.
   BSimOrder* pOrder=gSimOrderManager.createOrder();
   if (!pOrder)
      return (false);

   //Init it.
   pOrder->setOwnerID(mID);

   //Add it.
   if (!queueOrder(pOrder, BSimOrder::cTypeIdle))
   {
      gSimOrderManager.markForDelete(pOrder);
      return (false);
   }
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::queueWork(BSimTarget target, uint8 priority)
{
   //Create the order.
   BSimOrder* pOrder=gSimOrderManager.createOrder();
   if (!pOrder)
      return (false);

   BSimOrderType orderType=getOrderType(target, NULL, NULL);
   if (orderType == BSimOrder::cTypeNone)
      return (false);

   //Init it.
   pOrder->setOwnerID(mID);
   pOrder->setTarget(target);
   pOrder->setPriority(priority);

   //Add it.
   if (!queueOrder(pOrder, orderType))
   {
      gSimOrderManager.markForDelete(pOrder);
      return (false);
   }
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::queueUnload(uint8 priority, BEntityIDArray& garrisonedSquads, bool useRallyPoint/*=false*/, BPlayerID thisPlayerOnly /*= cInvalidPlayerID*/)
{
   // Find contained or attached squads
   BEntityIDArray squadList = getGarrisonedSquads();

   if (!garrisonedSquads.isEmpty())
   {
      // Remove any squads not in the passed in list.
      for (int i=squadList.getNumber()-1; i>=0 ; i--)
      {
         BEntityID squadID = squadList[i];
         if (garrisonedSquads.find(squadID) == -1)
            squadList.removeIndex(i);
      }
   }

   // Group squads per player
   BPlayerSpecificEntityIDsArray playerEntityIDs = BSimHelper::groupEntityIDsByPlayer(squadList);
   uint numPlayerEntityIDs = playerEntityIDs.getSize();
   for (uint i = 0; i < numPlayerEntityIDs; i++)
   {
      uint numEntities = playerEntityIDs[i].mEntityIDs.getSize();
      if (numEntities > 0)
      {
         BObjectCreateParms objectParms;
         objectParms.mPlayerID = playerEntityIDs[i].mPlayerID;
         BArmy* pArmy = gWorld->createArmy(objectParms);
         BASSERT(pArmy);
         if (pArmy)
         {
            // Platoon the squads
            if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
               pArmy->addSquads(playerEntityIDs[i].mEntityIDs, false);
            else
               pArmy->addSquads(playerEntityIDs[i].mEntityIDs);
         }               
      }
   }

   // Calculate rally point locations
   BVector rallyPoint = cInvalidVector;
   if (useRallyPoint)
   {
      const BUnit* pUnit = getLeaderUnit();
      if (pUnit && pUnit->haveRallyPoint(thisPlayerOnly))
         rallyPoint = pUnit->getRallyPoint(thisPlayerOnly);
      else if (getPlayer()->haveRallyPoint())
         rallyPoint = getPlayer()->getRallyPoint();
      if (!rallyPoint.almostEqual(cInvalidVector))
      {
         // Tell units to move near the rally point instead of right on top of it.
         if (pUnit)
         {
            BVector dir = rallyPoint - pUnit->getPosition();
            if (dir.length() > 4.0f)
            {
               dir.normalize();
               rallyPoint -= dir * 4.0f;
            }
         }
         BCommand* pCommand = new BCommand(-1, BCommand::cNumberCommandFlags);
         pCommand->setPlayerID(mPlayerID);
         pCommand->addWaypoint(rallyPoint);
         gSquadPlotter.plotSquads(squadList, pCommand);
         delete (pCommand);
      }
   }   
   const BDynamicSimArray<BSquadPlotterResult>& plotterResults = gSquadPlotter.getResults();

   // Give ungarrison actions to all of the contained squads
   uint numGarrisonedSquads = squadList.getSize();
   for (uint i = 0; i < numGarrisonedSquads; i++)
   {      
      BSquad* pGarrisonedSquad = gWorld->getSquad(squadList[i]);
      if (pGarrisonedSquad && ((thisPlayerOnly == cInvalidPlayerID) || (thisPlayerOnly == pGarrisonedSquad->getPlayerID())))
      {
         BSimTarget target(getID());
         BSimOrder* pOrder = gSimOrderManager.createOrder();
         BASSERT(pOrder);
         if (pOrder)  
         {
            pOrder->setPriority(priority);
            pOrder->setTarget(target);
         }
         
         BVector squadRallyPoint = cInvalidVector;
         if (useRallyPoint)
         {
            if (plotterResults.getSize() > i)
               squadRallyPoint = plotterResults[i].getDesiredPosition();
            else
               squadRallyPoint = rallyPoint;
         }

         BActionID childActionID = pGarrisonedSquad->doUngarrison(pOrder, NULL, &target, &target, squadRallyPoint, 0, cInvalidVector, cInvalidVector, true, false, true);
         if (childActionID == cInvalidActionID)
         {
            return (false);
         }                  
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::addOppToChildren(BUnitOpp &opp) const
{
   //We return false if NO CHILD returns true.  If any child returns true, then
   //we do, as well.
   bool rVal = false;
   for (uint i = 0; i < mChildren.getSize(); i++)
   {
      BUnit* pUnit = gWorld->getUnit(mChildren[i]);
      if (pUnit)
      {
         //Make a creepy copy of this so each unit can have their own.
         BUnitOpp* pNewOpp=BUnitOpp::getInstance();
         *pNewOpp=opp;
         if (!pUnit->addOpp(pNewOpp))
            BUnitOpp::releaseInstance(pNewOpp);
         else
            rVal = true;
      }
   }
    
   return (rVal);
}

//==============================================================================
//==============================================================================
bool BSquad::addOppToMaxNumChildren(BUnitOpp &opp, int maxNumChildrenToAddOpp) const
{
   //We return false if NO CHILD returns true.  If any child returns true, then
   //we do, as well.
   bool rVal = false;
   int count = 0;
   for (uint i = 0; i < mChildren.getSize(); i++)
   {
      BUnit* pUnit = gWorld->getUnit(mChildren[i]);
      if (pUnit)
      {
         //Make a creepy copy of this so each unit can have their own.
         BUnitOpp* pNewOpp=BUnitOpp::getInstance();
         *pNewOpp=opp;
         if (!pUnit->addOpp(pNewOpp))
            BUnitOpp::releaseInstance(pNewOpp);
         else
         {
            rVal = true;
            count++;

            if (count >= maxNumChildrenToAddOpp)
               break;
         }
      }
   }
    
   return (rVal);
}

//==============================================================================
//==============================================================================
bool BSquad::addOppToChildren(BUnitOpp &opp, uint8& count) const
{
   //We return false if NO CHILD returns true.  If any child returns true, then
   //we do, as well.
   bool rVal = false;
   count = 0;
   uint numChildren = mChildren.getSize();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(mChildren[i]);
      if (pUnit)
      {
         //Make a creepy copy of this so each unit can have their own.
         BUnitOpp* pNewOpp=BUnitOpp::getInstance();
         *pNewOpp=opp;
         if (!pUnit->addOpp(pNewOpp))
            BUnitOpp::releaseInstance(pNewOpp);
         else
         {
            count++;
            rVal = true;
         }
      }
   }

   return (rVal);
}

//==============================================================================
//==============================================================================
bool BSquad::addOppToChildren(BUnitOpp &opp, BDynamicSimArray<uint>* oppIDs, BEntityIDArray* children/*=NULL*/) const
{
   bool result = false;
   uint count = 0;
   if(children)
      count = children->getSize();
   else
      count = mChildren.getSize();

   for (uint i = 0; i < count; i++)   
   {
      BUnit* pUnit = NULL;
      if(children)
         pUnit = gWorld->getUnit((*children)[i]);
      else
         pUnit = gWorld->getUnit(mChildren[i]);
      
      if (pUnit)
      {
         //Make a creepy copy of this so each unit can have their own.  Each gets
         //a unique ID now, though.
         BUnitOpp* pNewOpp=BUnitOpp::getInstance();
         *pNewOpp=opp;
         pNewOpp->generateID();
         //Add it.
         if (!pUnit->addOpp(pNewOpp))
            BUnitOpp::releaseInstance(pNewOpp);
         else if(oppIDs)
         {
            oppIDs->add(pNewOpp->getID());
            result |= true;
         }
      }
   }
   
   return result;
}

//==============================================================================
//==============================================================================
void BSquad::removeOppFromChildren(BUnitOppID oppID, bool removeActions) const
{
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      if (pUnit)
         pUnit->removeOpp(oppID, removeActions);
   }
}

//==============================================================================
//==============================================================================
void BSquad::removeOppFromChildren(BDynamicSimArray<uint>& oppIDs, bool removeActions) const
{
   if (oppIDs.getSize() == 0)
      return;

   //NOTE: We assume that these oppIDs did originally map 1:1 with the units, so
   //any success on a remove means we can short-circuit the rest of the ID list.
   //But, we cannot assume anything else as the ID list may not match 1:1 anymore.
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      if (pUnit)
      {
         for (uint j=0; j < oppIDs.getSize(); j++)
         {
            if (pUnit->removeOpp(oppIDs[j], removeActions))
               break;
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquad::removeAllActionsForOrder(BSimOrder* pOrder)
{
   //Remove it from ourself.
   mActions.removeAllActionsForOrder(pOrder);

   //Remove it from our children.
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      if (pUnit)
         pUnit->removeAllActionsForOrder(pOrder);
   }
   
   return(true);
}

//==============================================================================
//==============================================================================
bool BSquad::pauseAllActionsForOrder(BSimOrder* pOrder)
{
   return (mActions.pauseAllActionsForOrder(pOrder));
}

//==============================================================================
//==============================================================================
bool BSquad::unpauseAllActionsForOrder(BSimOrder* pOrder)
{
   return (mActions.unpauseAllActionsForOrder(pOrder));
}

//==============================================================================
//==============================================================================
bool BSquad::doPlayBlockingAnimation(BSimOrder* pOrder, long state, long type, bool applyInstantly, bool useSquadMatrix, bool useMaxHeight, BCueIndex soundCue, bool loop, bool disableMotionExtractionCollisions, bool updateSquadPhysicsPosAtEnd, bool clearUnitNoRenderFlag, bool birthAnim)
{
   syncSquadData("BSquad::doPlayBlockingAnimation mID", mID.asLong());
   BSquadActionPlayBlockingAnimation*  pAction = (BSquadActionPlayBlockingAnimation*) gActionManager.createAction(BAction::cActionTypeSquadPlayBlockingAnimation);

   pAction->setAnimationState(state, type, applyInstantly, useSquadMatrix, useMaxHeight, loop);
   pAction->setSoundCue(soundCue);
   pAction->setDisableMotionExtractionCollisions(disableMotionExtractionCollisions);
   pAction->setClearUnitNoRenderFlag(clearUnitNoRenderFlag);
   pAction->setUpdateSquadPhysicsPosAtEnd(updateSquadPhysicsPosAtEnd);
   pAction->setFlagBirthAnim(birthAnim);
   return (addAction(pAction, pOrder));
}

//==============================================================================
//==============================================================================
bool BSquad::doTeleport(BVector location, long searchScale /*= 1*/, bool testObstructions /*= true*/, bool clearActions /*= true*/)
{
   BVector suggestion = location;
   DWORD flags = BWorld::cCPCheckObstructions | BWorld::cCPSetPlacementSuggestion | BWorld::cCPExpandHull | BWorld::cCPIgnoreMoving | BWorld::cCPIgnoreNonSolid;
   if (testObstructions && gWorld->checkPlacement(getProtoObjectID(), getPlayerID(), location, suggestion, getForward(), BWorld::cCPLOSDontCare, flags, searchScale))
   {
      stop();
      stopAllChildren(0);
      //Remove ALL orders.
      if (clearActions)
      {
         removeAllOrders();
         removeActions();
      }
      BPlatoon* pPlatoon = getParentPlatoon();
      if (pPlatoon)
         pPlatoon->setPosition(suggestion);
      setPosition(suggestion);
      setLeashPosition(suggestion);
      noStopSettle();
      updateObstruction();
      dirtyLOS();
      //settle();
      setFlagTeleported(true);
      return (true);
   }
   else if (!testObstructions)
   {
      stop();
      stopAllChildren(0);
      //Remove ALL orders.
      if (clearActions)
      {
         removeAllOrders();
         removeActions();
      }
      BPlatoon* pPlatoon = getParentPlatoon();
      if (pPlatoon)
         pPlatoon->setPosition(suggestion);
      setPosition(suggestion);
      setLeashPosition(suggestion);
      noStopSettle();
      updateObstruction();
      dirtyLOS();
      //settle();
      setFlagTeleported(true);
      return (true);
   }
   return (false);
}

//==============================================================================
//==============================================================================
void BSquad::kill(bool bKillImmediately)
{
   #ifdef SYNC_Squad
   syncSquadData("BSquad::kill mID", mID.asLong());
   #endif

   //Remove us from our player.
   BPlayer* pPlayer=getPlayer();
   BASSERT(pPlayer);
   pPlayer->removeSquadAISquad(this);

   if (!getFlagAlive())
   {
      return;
   }

   const BProtoObject* pProtoObject = getProtoObject();
   if (!bKillImmediately && pProtoObject && pProtoObject->isType(gDatabase.getOTIDHeroDeath()))
   {
      killHero();
      return;
   }

   setFlagAlive(false);

   // Since pUnit->kill() removes the unit from the squad, changing the size of mChildren in the squad, iterate backwards to avoid shenanigans.
   long count = mChildren.getNumber();
   for (long i = count - 1; i >= 0; i--)
   {
      BUnit* pUnit = gWorld->getUnit(mChildren[i]);
      if (pUnit)
      {
         #ifdef SYNC_Squad
         syncSquadData("BSquad::kill unitID", mChildren[i].asLong());
         #endif
         pUnit->kill(bKillImmediately);
      }
   }

   //Notify the score system
   // [11/5/2008 xemu] modified to look at new score-prevention flag (for trigger-killed units) 
   if (!getFlagPreventScoring())
      gScoreManager.reportSquadKilled(mKillerPlayerID, mID, 1);


   revokePower();

   // Pop
   if (!getFlagIgnorePop())
   {
      BPlayer* pPlayer = getPlayer();
      const BProtoSquad* pProtoSquad = getProtoSquad();
      if (pProtoSquad)
      {
         uint unitNodeCount = (uint)pProtoSquad->getNumberUnitNodes();
         // [10/29/2008 xemu] to mitigate some floating point issues, sum the pop contribution for the squad and round before applying 
         float popCost = 0.0f;
         short popID = -1;

         for (uint i = 0; i < unitNodeCount; i++)
         {
            const BProtoSquadUnitNode& unitNode = pProtoSquad->getUnitNode(i);
            const BProtoObject* pProtoObject = pPlayer->getProtoObject(unitNode.mUnitType);
            uint unitPopCount = (uint)pProtoObject->getNumberPops();
            for (uint j = 0; j < unitPopCount; j++)
            {
               BPop pop = pProtoObject->getPop(j);
               popCost = popCost + (pop.mCount * unitNode.mUnitCount);
               BASSERT((popID == -1) || (popID == pop.mID));
               popID = pop.mID;
            }
         }
         popCost = floorf(popCost + 0.5f);
         pPlayer->adjustPopCount(popID, -popCost);
      }
      else
      {
         const BProtoObject* pProtoObject = getProtoObject();
         if (pProtoObject)
         {
            uint unitPopCount = (uint)pProtoObject->getNumberPops();
            for (uint j = 0; j < unitPopCount; j++)
            {
               BPop pop = pProtoObject->getPop(j);
               pPlayer->adjustPopCount(pop.mID, -pop.mCount);
            }
         }
      }
   }

   if (mSelectionDecal != -1)
      gDecalManager.destroyDecal(mSelectionDecal);
   mSelectionDecal = -1;

   if (mAIMissionID != cInvalidAIMissionID)
      removeFromAIMission();

   BProtoSquadID protoSquadID = cInvalidProtoID;

   if (this->getFlagProtoSquad())
   {
      BPlayer* pPlayer = getPlayer();
      protoSquadID = getProtoSquadID();
      pPlayer->removeSquadFromProtoSquad(this, protoSquadID);
      pPlayer->adjustDeadSquadCount(protoSquadID, 1);
   }

   removeEffects(bKillImmediately);
   setTargettingSelection(false);

   gWorld->updateAllKBsForSquad(this);

   BASSERTM(mChildren.getSize() == 0, "Squad has been killed but still has child unit IDs!");

   // [10/27/2008 xemu] don't send this if we are just doing game-over cleanup however
   bool gameOver = gWorld->getFlagGameOver();
   if (!gameOver)
   {
      if (!mKilledByRecycler && getFlagIsDoneBuilding())
         gWorld->notify2(BEntity::cEventKilled, mPlayerID, mLastKnownLeaderProtoObjectID, protoSquadID, mKillerPlayerID, mKillerProtoID, mKillerProtoSquadID, mXP, mLevel);

      // move up into entity?
      gWorld->notify(BEntity::cEventKilled, mID, 0, 0);
   }


   //-- Entity kill
   BEntity::kill(bKillImmediately);
}

//==============================================================================
// Perform a kill on a hero squad
//==============================================================================
void BSquad::killHero()
{
   #ifdef SYNC_Squad
      syncSquadData("BSquad::killHero mID", mID.asLong());
   #endif

   int count = mChildren.getNumber();
   for (int i = (count - 1); i >= 0; i--)
   {
      BUnit* pUnit = gWorld->getUnit(mChildren[i]);
      if (pUnit)
      {
         #ifdef SYNC_Squad
            syncSquadData("BSquad::killHero unitID", mChildren[i].asLong());
         #endif
         pUnit->killHero();
      }
   }

   //Notify the score system
   // [11/5/2008 xemu] modified to look at new score-prevention flag (for trigger-killed units) 
   if (!getFlagPreventScoring())
      gScoreManager.reportSquadKilled(mKillerPlayerID, mID, 1);


   //XXXHalwes - 5/5/2008 - Valid?
   //revokePower();

   //XXXHalwes - 5/5/2008 - Just remove from selection don't destroy decal?
   //if (mSelectionDecal != -1)
   //   gDecalManager.destroyDecal(mSelectionDecal);
   //mSelectionDecal = -1;
   BUser* pUser = gUserManager.getUserByPlayerID(getPlayerID());
   if (pUser)
   {
      BSelectionManager* pSelectionManager = pUser->getSelectionManager();
      if (pSelectionManager)
      {
         pSelectionManager->unselectSquad(mID);
      }
   }

   if (mAIMissionID != cInvalidAIMissionID)
      removeFromAIMission();

   //XXXHalwes - 5/5/2008 - ?
   gWorld->updateAllKBsForSquad(this);

   //XXXHalwes - 5/5/2008 - Need a specific "hero killed" event?
   gWorld->notify2(BEntity::cEventKilled, mPlayerID, mLastKnownLeaderProtoObjectID, getProtoSquadID(), mKillerPlayerID, mKillerProtoID, mKillerProtoSquadID, mXP, mLevel);
   gWorld->notify(BEntity::cEventKilled, mID, 0, 0);
}

//==============================================================================
//==============================================================================
void BSquad::destroy()
{
   #ifdef SYNC_Squad
   syncSquadData("BSquad::destroy mID", mID.asLong());
   #endif

   BASSERTM(getFlagAlive() == false, "Destroy is being called on a squad that has not been killed.  Don't call destroy directly.  Call kill(true) instead!");
   BASSERTM(mChildren.getSize() == 0, "Destroy is being called on a squad that still has child unit IDs!  That means the squad hasn't been killed yet!");

   // Since pUnit->destroy() / pUnit->kill() removes the unit from the squad, changing the size of mChildren in the squad, iterate backwards to avoid shenanigans.
   long count = mChildren.getNumber();
   for (long i=count-1; i>=0; i--)
   {
      BUnit* pUnit = gWorld->getUnit(mChildren[i]);
      if (pUnit)
      {
         #ifdef SYNC_Squad
         syncSquadData("BSquad::destroy unitID", pUnit->getID().asLong())
         #endif
         pUnit->kill(true);
      }
   }

   mChildren.clear();
   mpCachedLeaderUnit = NULL;
   
   //Remove us from our parent, if we have one.
   BPlatoon *platoon=gWorld->getPlatoon(mParentID);
   if (platoon)
      platoon->removeChild(this);


   //if (this->getFlagProtoSquad())
   //{
   //   BPlayer* pPlayer = getPlayer();
   //   BProtoSquadID protoSquadID = getProtoSquadID();
    //  pPlayer->removeSquadFromProtoSquad(this, protoSquadID);
  // }
//
   //gWorld->updateAllKBsForSquad(this);
}

//==============================================================================
//==============================================================================
bool BSquad::isAlive() const
{
   return getFlagAlive();
}

//==============================================================================
//==============================================================================
bool BSquad::isEverMobile() const
{
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      if (pUnit && pUnit->isEverMobile())
         return (true);
   }
   return (false);
}

//==============================================================================
// If any units are down the whole squad is down
//==============================================================================
bool BSquad::isDown() const
{
   bool result = false;
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(mChildren[i]);
      if (pUnit && pUnit->getFlagDown())
      {
         result = true;
         break;
      }
   }

   return (result);
}

//==============================================================================
// If any units are hibernating the whole squad is hibernating
//==============================================================================
bool BSquad::isHibernating() const
{
   bool result = false;
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(mChildren[i]);
      if (pUnit && pUnit->isHibernating())
      {
         result = true;
         break;            
      }
   }

   return (result);
}

//==============================================================================
//==============================================================================
BProtoAction* BSquad::getProtoActionForTarget(BEntityID targetID, BVector targetLoc, long abilityID, bool autoTarget, bool* pInsideMinRange, bool noDiscardAbilities, long* pRuleAbilityID, bool* pTargetsGround) const
{
   // Get our squad leader and tactic.
   BEntityID leaderID = getLeader();
   BUnit *pLeader = gWorld->getUnit(leaderID);
   if (!pLeader)
      return(NULL);
   BTactic *pTactic = pLeader->getTactic();
   if (!pTactic)
      return(NULL);

   // Can't do anything if not alive or not built
   if(!pLeader->isAlive() || !pLeader->getFlagBuilt())
      return(NULL);

   // Get our target if applicable.  This may be NULL (which is OK)
   BObject *pTarget = NULL;
   if(targetID.getType() == BEntity::cClassTypeSquad)
   {
      BSquad *pSquad = gWorld->getSquad(targetID);
      if (pSquad)
         pTarget = pSquad->getLeaderUnit();
   }
   else if(targetID.getType() == BEntity::cClassTypeUnit)
      pTarget = gWorld->getUnit(targetID);
   else if(targetID.getType() == BEntity::cClassTypeDopple)
      pTarget = gWorld->getDopple(targetID);

   // Look up the proto action
   BProtoAction *pAction = pTactic->getProtoAction(pLeader->getTacticState(), pTarget, targetLoc,
      getPlayer(), (pLeader) ? pLeader->getPosition() : XMVectorZero(), leaderID, abilityID, autoTarget, 
      -1, false, false, pInsideMinRange, noDiscardAbilities, pRuleAbilityID, pTargetsGround);
   return pAction;
}

//==============================================================================
//==============================================================================
bool BSquad::canAttackTarget(BEntityID targetUnit) const
{
   BEntity *pEntity = gWorld->getEntity(targetUnit);
   const BProtoAction* pAction = getProtoActionForTarget(targetUnit, pEntity->getPosition(), -1, true);
   if (pAction /*pAction->getActionType() == BAction::cActionTypeSquadAttack*/)
      return true;
   else
      return false;
}

//==============================================================================
//==============================================================================
bool BSquad::isVisible(BTeamID teamID) const
{
   //-- If any unit in a squad is visible, then the entire squad is visible
   long numUnits = mChildren.getNumber();
   for(long i = 0; i < numUnits; i++)
   {
      BUnit *pUnit = gWorld->getUnit(mChildren[i]);
      //BASSERT(pUnit);   // should not be possible to have invalid child unit.
      if(pUnit && pUnit->isVisible(teamID) && !pUnit->isDoppled(teamID))
      {
         //-- Make sure the unit is not way out of formation (broken)
         //const BFormationPosition* pos =  mpFormation->getFormationPosition(pUnit->getFormationIndex());
         //if(pos && pos->mState != BFormationPosition::cStateBroken)
            return true;
      }
   }
   return false;
}

//==============================================================================
//==============================================================================
bool BSquad::isVisibleOnScreen() const
{
   //-- If any unit in a squad is visible, then the entire squad is visible
   long numUnits = mChildren.getNumber();
   const BUnit* pUnit = NULL;
   for(long i = 0; i < numUnits; i++)
   {
      pUnit = gWorld->getUnit(mChildren[i]);
      BASSERT(pUnit);   // should not be possible to have invalid child unit.
      if(pUnit && pUnit->isVisibleOnScreen())
         return true;         
   }
   return false;
}

//==============================================================================
//==============================================================================
bool BSquad::isVisibleOrDoppled(BTeamID teamID) const
{
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      BDEBUG_ASSERT(pUnit);
      if (pUnit && pUnit->isVisibleOrDoppled(teamID))
         return (true);
   }
   return (false);
}

//==============================================================================
//==============================================================================
void BSquad::dirtyLOS()
{
   long numChildren = getNumberChildren();
   for( long i = 0; i < numChildren; i++ )
   {
      BObject* pObject = gWorld->getObject( getChild( i ) );
      if( pObject )
      {
         #ifdef SYNC_Visibility
         syncVisibilityData("BSquad::dirtyLOS id", pObject->getID().asLong());
         #endif
         pObject->setFlagLOSDirty(true);
      }
   }
}

//==============================================================================
//==============================================================================
float BSquad::getLOS() const
{
   //-- If we get mixed squads this might be more confusing.
   if(mChildren.getNumber() > 0)
   {
      BUnit *pUnit = gWorld->getUnit(mChildren[0]);
      if(pUnit)
         return pUnit->getLOS();
   }

   return 0.0f;
}

//==============================================================================
// Helper function that caches true LOS result
//==============================================================================
bool BSquad::validateLOS(const BProtoAction *pSourceSquadAction, const BSquad *targetSquad)
{
   // Calculate CRC that represents update number, proto action, and target squad
   DWORD newCRC = 0;
   Crc32Long(newCRC, static_cast<long>(gWorld->getUpdateNumber()));
   if (pSourceSquadAction)
      Crc32Long(newCRC, pSourceSquadAction->getID());
   if (targetSquad)
      Crc32Long(newCRC, targetSquad->getID().asLong());

   // Return cached result if LOS checked this update
   if (newCRC == mLastTrueLOSCheckCRC)
       return mFlagHasTrueLOS;

   // Calculate LOS
   mFlagHasTrueLOS = gSquadLOSValidator.validateLOS(this, pSourceSquadAction, targetSquad);
   mLastTrueLOSCheckCRC = newCRC;
   return mFlagHasTrueLOS;
}

//==============================================================================
// Check to see if the squad is outside the playable area
//==============================================================================
bool BSquad::isOutsidePlayableBounds(bool forceCheckWorldBoundaries) const
{
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit && pUnit->isOutsidePlayableBounds(forceCheckWorldBoundaries))
      {
         return (true);
      }
   }

   return (BEntity::isOutsidePlayableBounds(forceCheckWorldBoundaries));
}

//==============================================================================
//==============================================================================
float BSquad::getAttackRating() const
{
   return getProtoSquad()->getAttackRating();
}


//==============================================================================
//==============================================================================
float BSquad::getAttackRatingDPS(BDamageTypeID damageType) const
{
   return getProtoSquad()->getAttackRatingDPS(damageType);
}


//==============================================================================
//==============================================================================
float BSquad::getStrength() const
{
   return getProtoSquad()->getStrength();
}

//==============================================================================
//==============================================================================
uint BSquad::getAttackGrade(BDamageTypeID damageType) const
{
   return getProtoSquad()->getAttackGrade(damageType);
}

//==============================================================================
//==============================================================================
float BSquad::getAttackGradeRatio(BDamageTypeID damageType) const
{
   return getProtoSquad()->getAttackGradeRatio(damageType);
}

//==============================================================================
//==============================================================================
BDamageTypeID BSquad::getDamageType() const
{
   return getProtoSquad()->getDamageType();
}

//==============================================================================
//==============================================================================
bool BSquad::isDamageType(BDamageTypeID damageType) const
{
   return getProtoSquad()->isDamageType(damageType);
}

//==============================================================================
//==============================================================================
float BSquad::getCombatValue() const
{
   return getProtoSquad()->getCombatValue();
}

//==============================================================================
//==============================================================================
float BSquad::getCombatValueHP() const
{
   // actual HP + SP
   float actualHPSP = getCurrentHP() + getCurrentSP();
   float maxHPSP = getHPMax() + getSPMax();
   float percentTotal = 0.0f;
   if (maxHPSP > 0.0f)
      percentTotal = actualHPSP / maxHPSP;

   return (getCombatValue() * percentTotal);
}

//==============================================================================
//==============================================================================
float BSquad::getAttackRating(BDamageTypeID damageType) const
{
   return getProtoSquad()->getAttackRating(damageType);
}

//==============================================================================
//==============================================================================
float BSquad::getDefenseRating() const
{
   return getProtoSquad()->getDefenseRating();
}

//==============================================================================
//==============================================================================
BEntityID BSquad::getLeastAttackedUnitID() const
{
   if (mChildren.getSize() == 0)
      return (cInvalidObjectID);

   BEntityID rVal=cInvalidObjectID;
   uint rCount=0;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      uint attackerCount=pUnit->getNumberAttackingUnits();
      if ((rVal == cInvalidObjectID) || (rCount > attackerCount))
      {
         rVal=pUnit->getID();
         rCount=attackerCount;
      }
   }
   
   //If we didn't find anything, take a random unit.
   if (rVal == cInvalidObjectID)
   {
      uint randomIndex=getRand(cSimRand)%mChildren.getSize();
      return(mChildren[randomIndex]);
   }
   return (rVal);
}

//==============================================================================
//==============================================================================
BEntityID BSquad::getRandomUnitID(BUnit* pUnit, float angleLimit) const
{
   BDEBUG_ASSERT(pUnit);
   if (mChildren.getSize() == 0)
      return (cInvalidObjectID);
      
   //If we're happy with anything, just take any random one.
   float angleDiff=(float)fabs(angleLimit-cPi);
   if (angleDiff < cFloatCompareEpsilon)
   {
      uint randomIndex=getRand(cSimRand)%mChildren.getSize();
      return(mChildren[randomIndex]);
   }
   
   //Else, we need to build a list of the valid ones and pick one of those.
   BSmallDynamicSimArray<BEntityID> validTargets;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BUnit* pTargetUnit=gWorld->getUnit(mChildren[i]);
      if(pTargetUnit)
      {
         BVector targetDirection=pTargetUnit->getPosition()-pUnit->getPosition();
         targetDirection.y=0.0f;
         targetDirection.safeNormalize();
         BVector unitForward=pUnit->getForward();
         unitForward.y=0.0f;
         unitForward.safeNormalize();
         float angle=targetDirection.angleBetweenVector(unitForward);
         if (angle <= angleLimit)
            validTargets.add(pTargetUnit->getID());
      }
   }
   if (validTargets.getSize() <= 0)
      return (cInvalidObjectID);
   uint randomIndex=getRand(cSimRand)%validTargets.getSize();
   return(validTargets[randomIndex]);
}

//==============================================================================
//==============================================================================
BEntityID BSquad::getLeader() const
{
   if (mChildren.getNumber() == 0)
      return cInvalidObjectID;
   return mChildren[0];
}

//==============================================================================
//==============================================================================
BUnit* BSquad::getLeaderUnit() const
{
   if (mpCachedLeaderUnit && !mChildren.isEmpty() && mpCachedLeaderUnit->getID() == mChildren[0]) 
      return mpCachedLeaderUnit;
   return NULL;
}

//==============================================================================
//==============================================================================
void BSquad::refreshLeaderUnit()
{
   if (!mChildren.isEmpty())
      mpCachedLeaderUnit = gWorld->getUnit(mChildren[0]);
   else
      mpCachedLeaderUnit = NULL;
}

//==============================================================================
//==============================================================================
long BSquad::getUnitType() const
{
   const BUnit *pLeaderUnit = getLeaderUnit();
   return (pLeaderUnit ? pLeaderUnit->getProtoID() : cInvalidProtoID);
}

//==============================================================================
//==============================================================================
float BSquad::getMaxVelocity() const
{
   const BUnit* pLeaderUnit = getLeaderUnit();
   if (pLeaderUnit)
   {
      return (pLeaderUnit->getMaxVelocity());
   }

   return (0.0f);
}

//==============================================================================
//==============================================================================
float BSquad::getDesiredVelocity() const
{
   // Return squad's leader unit's desired velocity
   const BUnit* pLeaderUnit = getLeaderUnit();
   if (pLeaderUnit)
   {
      return (pLeaderUnit->getDesiredVelocity());
   }

   return (0.0f);
}

//==============================================================================
//==============================================================================
float BSquad::getActionVelocity() const
{
   const BSquadActionMove* pAction=getNonPausedMoveAction_4();
   if (pAction)
      return (pAction->getVelocity());
   return (getDesiredVelocity());
}

//==============================================================================
//==============================================================================
float BSquad::getAcceleration() const
{
   const BUnit *pUnit = getLeaderUnit();
   return (pUnit ? pUnit->getAcceleration() : 0.0f);
}

//==============================================================================
//==============================================================================
bool BSquad::canMove(bool allowAutoUnlock) const
{
   if (!BEntity::canMove(allowAutoUnlock))
      return (false);
   if (getCurrentOrChangingMode() == BSquadAI::cModeLockdown && !allowAutoUnlock && !canAutoUnlock())
      return (false);
   if (mStasisSpeedMult == 0.0f)
      return (false);
   if (mFlagDazeImmobilized)
      return (false);
   if (mFlagCryoFrozen)
      return (false);

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::canMoveForOrder(BSimOrder* pOrder) const
{
   // ajl 4/20/08 - Jira 5650 X/Y button change (line below replaces other allowAutoUnlock code below)
   // TRB 7/30/08 - Changing this back so it only unlocks if pressing the Y button.  X button does nothing.
   //bool allowAutoUnlock = (pOrder && pOrder->getPriority() == BSimOrder::cPriorityUser);
   bool allowAutoUnlock = false;
   if (pOrder && pOrder->getPriority() == BSimOrder::cPriorityUser && pOrder->getTarget().isAbilityIDValid())
   {
      int abilityID = gDatabase.getSquadAbilityID(this, pOrder->getTarget().getAbilityID());
      const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
      if (pAbility && pAbility->getSquadMode() == BSquadAI::cModeLockdown)
         allowAutoUnlock = true;
   }
   return (canMove(allowAutoUnlock));
}

//==============================================================================
//==============================================================================
void BSquad::getWidthAndDepth(float &width, float &depth) const
{
   if (mpFormation)
   {
      width=mpFormation->getRadiusX() * 2.0f;
      depth=mpFormation->getRadiusZ() * 2.0f;
   }
   else
   {
      width=1.0f;
      depth=1.0f;
   }
   return;
}

//==============================================================================
//==============================================================================
float BSquad::getHPMax() const
{
   return (getProtoSquad()->getMaxHP());
}

//==============================================================================
//==============================================================================
float BSquad::getSPMax() const
{
   return (getProtoSquad()->getMaxSP());
}

//==============================================================================
//==============================================================================
float BSquad::getAmmoMax() const
{
   return (getProtoSquad()->getMaxAmmo());
}

//==============================================================================
//==============================================================================
float BSquad::getCurrentHP() const
{
   float currentHP = 0.0f;
   int numChildren = getNumberChildren();
   for (int i=0; i<numChildren; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
         currentHP += pUnit->getHitpoints();
   }
   return (currentHP);
}

//==============================================================================
//==============================================================================
float BSquad::getCurrentSP() const
{
   float currentSP = 0.0f;
   int numChildren = getNumberChildren();
   for (int i=0; i<numChildren; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit && pUnit->getFlagHasShield())
         currentSP += pUnit->getShieldpoints();
   }
   return (currentSP);
}

//==============================================================================
//==============================================================================
float BSquad::getCurrentAmmo() const
{
   float currentAmmo = 0.0f;
   int numChildren = getNumberChildren();
   for (int i=0; i<numChildren; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit && pUnit->getFlagUsesAmmo())
         currentAmmo += pUnit->getAmmunition();
   }
   return (currentAmmo);
}

//==============================================================================
//==============================================================================
float BSquad::getHPPercentage() const
{
   float hpMax = getHPMax();
   float hpCurrent = getCurrentHP();
   if (hpMax >= cFloatCompareEpsilon)
      return (hpCurrent / hpMax);
   else
      return (0.0f);
}

//==============================================================================
//==============================================================================
float BSquad::getSPPercentage() const
{
   float spMax = getSPMax();
   float spCurrent = getCurrentSP();
   if (spMax >= cFloatCompareEpsilon)
      return (spCurrent / spMax);
   else
      return (0.0f);
}

//==============================================================================
//==============================================================================
float BSquad::getAmmoPercentage() const
{
   float ammoMax = getAmmoMax();
   float ammoCurrent = getCurrentAmmo();
   if (ammoMax >= cFloatCompareEpsilon)
      return (ammoCurrent / ammoMax);
   else
      return (0.0f);
}

//==============================================================================
//==============================================================================
void BSquad::adjustAmmunition(float adjust)
{
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
      {
         pUnit->adjustAmmunition(adjust);
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquad::getIgnoreUserInput() const
{
   // If any children units are flagged to ignore then ignore the whole squad
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit && pUnit->getIgnoreUserInput())
      {
         return (true);
      }
   }

   return (false);
}

//==============================================================================
//==============================================================================
void BSquad::setIgnoreUserInput(bool ignore)
{
   // Set children units ignore flag
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
      {
         pUnit->setIgnoreUserInput(ignore);
      }
   }
}

//==============================================================================
// Get the contained or attached squads
//==============================================================================
BEntityIDArray BSquad::getGarrisonedSquads(BPlayerID playerID /*= cInvalidPlayerID*/)
{
   // Find contained or attached squads
   BEntityIDArray squadList;
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
      {
         BEntityIDArray garrisonedUnits = pUnit->getGarrisonedUnits(playerID);
         uint numGarrisonedUnits = garrisonedUnits.getSize();
         for (uint j = 0; j < numGarrisonedUnits; j++)
         {
            BUnit* pGarrisonUnit = gWorld->getUnit(garrisonedUnits[j]);
            if (pGarrisonUnit)
            {
               const BSquad* pGarrisonSquad = pGarrisonUnit->getParentSquad();
               if (pGarrisonSquad && (pGarrisonSquad->getFlagGarrisoned() || pGarrisonSquad->getFlagAttached()))
               {
                  squadList.uniqueAdd(pGarrisonSquad->getID());
               }
            }
         }
      }
   }

   return (squadList);
}

//==============================================================================
// Get the contained or attached squads in cover
//==============================================================================
BEntityIDArray BSquad::getCoverSquads(BPlayerID playerID /*= cInvalidPlayerID*/)
{
   // Find contained or attached squads
   BEntityIDArray squadList;
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
      {
         BEntityIDArray coverUnits = pUnit->getCoverUnits(playerID);
         uint numCoverUnits = coverUnits.getSize();
         for (uint j = 0; j < numCoverUnits; j++)
         {
            BUnit* pCoverUnit = gWorld->getUnit(coverUnits[j]);
            if (pCoverUnit)
            {
               const BSquad* pCoverSquad = pCoverUnit->getParentSquad();
               if (pCoverSquad && pCoverSquad->getFlagInCover())
               {
                  squadList.uniqueAdd(pCoverSquad->getID());
               }
            }
         }
      }
   }

   return (squadList);
}

//==============================================================================
// Get the contained or attached units in cover
//==============================================================================
BEntityIDArray BSquad::getCoverUnits(BPlayerID playerID /*= cInvalidPlayerID*/)
{
   BEntityIDArray unitList;
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
      {
         BEntityIDArray coverUnits = pUnit->getCoverUnits(playerID);
         uint numCoverUnits = coverUnits.getSize();
         for (uint j = 0; j < numCoverUnits; j++)
         {
            const BUnit* pCoverUnit = gWorld->getUnit(coverUnits[j]);
            if (pCoverUnit)
            {
               unitList.uniqueAdd(coverUnits[j]);
            }
         }
      }
   }

   return (unitList);
}

//==============================================================================
// Get the entity ref for the containing unit
//==============================================================================
BEntityRef* BSquad::getContainingEntityRef()
{
   BEntityRef* pResult = NULL;
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
      {
         pResult = pUnit->getFirstEntityRefByType(BEntityRef::cTypeContainingUnit);
         if (pResult)
         {
            break;
         }
      }
   }

   return (pResult);
}

//==============================================================================
// See if this squad has any garrisoned or attached squads
//==============================================================================
bool BSquad::hasGarrisoned()
{
   bool result = false;
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit && (pUnit->getFlagHasGarrisoned() || pUnit->getFlagHasAttached()))
      {
         result = true;
         break;
      }
   }

   return (result);
}

//====================================================================================================
// See if this squad has any garrisoned or attached squads that are enemies of the provided player ID
// Note: This will only return true if the garrisoned units are visible to the player.
//====================================================================================================
bool BSquad::hasGarrisonedEnemies(BPlayerID playerID)
{
   bool result = false;
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
   {
      BTeamID playerTeamID = pPlayer->getTeamID();
      BEntityIDArray garrisonedSquads = getGarrisonedSquads();
      uint numGarrisonedSquads = garrisonedSquads.getSize();
      for (uint i = 0; i < numGarrisonedSquads; i++)
      {
         const BSquad* pSquad = gWorld->getSquad(garrisonedSquads[i]);
         if (pSquad && pPlayer->isEnemy(pSquad->getPlayerID()) && pSquad->isVisibleOrDoppled(playerTeamID))
         {
            result = true;
            break;
         }
      }
   }

   return (result);
}

//==============================================================================
// Get the hitched unit
//==============================================================================
BEntityID BSquad::getHitchedUnit()
{
   BEntityID hitchedUnit = cInvalidObjectID;   

   // I can only have a hitched squad if I am a single unit squad
   if (getNumberChildren() == 1)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(0));
      if (pUnit && pUnit->getFlagHasHitched())
      {
         uint numEntityRefs = pUnit->getNumberEntityRefs();
         for (uint i = 0; i < numEntityRefs; i++)
         {
            const BEntityRef* pRef = pUnit->getEntityRefByIndex(i);
            if (pRef && (pRef->mType == BEntityRef::cTypeHitchUnit))
            {
               const BUnit* pHitchedUnit = gWorld->getUnit(pRef->mID);
               if (pHitchedUnit)
               {
                  hitchedUnit = pRef->mID;
                  break;
               }
            }
         }         
      }      
   }

   return (hitchedUnit);
}

//==============================================================================
// Get the hitched squad
//==============================================================================
BEntityID BSquad::getHitchedSquad()
{
   BEntityID hitchedSquad = cInvalidObjectID;   
   BEntityID hitchedUnit = getHitchedUnit();
   BUnit* pHitchedUnit = gWorld->getUnit(hitchedUnit);
   if (pHitchedUnit)
   {
      const BSquad* pHitchedSquad = pHitchedUnit->getParentSquad();
      if (pHitchedSquad)
      {
         hitchedSquad = pHitchedSquad->getID();
      }
   }
   
   return (hitchedSquad);
}

//==============================================================================
// Get the hitched to unit
//==============================================================================
BEntityID BSquad::getHitchedToUnit()
{
   BEntityID hitchedToUnit = cInvalidObjectID;   

   // I can only have a hitched to unit if I am a single unit squad
   if (getNumberChildren() == 1)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(0));
      if (pUnit && pUnit->isHitched())
      {
         const BEntityRef* pRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeHitchedToUnit);
         if (pRef)
         {
            hitchedToUnit = pRef->mID;
         }
      }
   }

   return (hitchedToUnit);
}

//==============================================================================
// Get the hitched to squad
//==============================================================================
BEntityID BSquad::getHitchedToSquad()
{
   BEntityID hitchedToSquad = cInvalidObjectID;   
   BEntityID hitchedToUnit = getHitchedToUnit();
   BUnit* pHitchedToUnit = gWorld->getUnit(hitchedToUnit);
   if (pHitchedToUnit)
   {
      const BSquad* pHitchedToSquad = pHitchedToUnit->getParentSquad();
      if (pHitchedToSquad)
      {
         hitchedToSquad = pHitchedToSquad->getID();
      }
   }

   return (hitchedToSquad);
}

//==============================================================================
//==============================================================================
bool BSquad::hasHitchedSquad()
{
   return (getHitchedSquad() != cInvalidObjectID);
}

//==============================================================================
//==============================================================================
bool BSquad::getRepairCost(BCost& cost, float& combatValueCost) const
{
   bool retval=false;
   cost.zero();
   combatValueCost = 0.0f;
   const BPlayer* pPlayer=getPlayer();
   const BProtoSquad* pProtoSquad=getProtoSquad();
   int numChildren=mChildren.getNumber();
   int numNodes=pProtoSquad->getNumberUnitNodes();
   for (int i=0; i<numNodes; i++)
   {
      const BProtoSquadUnitNode&node = pProtoSquad->getUnitNode(i);
      const BProtoObject* pProtoObject=pPlayer->getProtoObject(node.mUnitType);
      if (pProtoObject && node.mUnitCount>0)
      {
         int unitCount=0;
         float unitHitpoints=0.0f;
         for (int j=0; j<numChildren; j++)
         {
            const BUnit* pUnit=gWorld->getUnit(mChildren[j]);
            if (pUnit && pUnit->getProtoID()==node.mUnitType)
            {
               unitCount++;
               unitHitpoints+=pUnit->getHitpoints();
            }
         }
         float nodeHitpoints=node.mUnitCount*pProtoObject->getHitpoints();
         if (unitHitpoints < nodeHitpoints)
         {
            float damagePct=(nodeHitpoints-unitHitpoints)/nodeHitpoints;
            float combatValue=pProtoObject->getCombatValue()*node.mUnitCount;
            BCost nodeCost = *(pPlayer->getRepairCost());
            nodeCost *= (combatValue * damagePct);
            cost.add(&nodeCost);
            combatValueCost += (combatValue * damagePct);
            retval=true;
         }
      }
   }
   return retval;
}

//==============================================================================
//==============================================================================
void BSquad::updateLastDamagedTime()
{
   mLastDamagedTime = gWorld->getGametime();
}

//==============================================================================
//==============================================================================
void BSquad::updateLastAttackedTime()
{
   mLastAttackedTime = gWorld->getGametime();
}

//==============================================================================
//==============================================================================
void BSquad::showHPBars(bool show)
{
   const BProtoSquad* pProtoSquad = getProtoSquad();

   if (!pProtoSquad) // dynamically created squad use the unit hp bars
   {
      long unitCount = mChildren.getNumber();
      for(long i = 0; i < unitCount; i++)
      {
         BUnit *pUnit = gWorld->getUnit(mChildren[i]);
         if(!pUnit)
            continue;

         pUnit->setFlagDisplayHP(show);
      }
   }      
}

//==============================================================================
//==============================================================================
bool BSquad::wasAttackedRecently() const
{
   const DWORD lastDamagedTime = getLastDamagedTime();
   if(lastDamagedTime==0)
      return false;

   const DWORD recently = 6000;
   const DWORD gameTime = gWorld->getGametime();
   const DWORD duration = gameTime - lastDamagedTime;
   return (duration <= recently);
}


//==============================================================================
//==============================================================================
void BSquad::hideSelectionDecal()
{
   if (mSelectionDecal == -1)
      return;

   BDecalAttribs* pDecalAttributes = gDecalManager.getDecal(mSelectionDecal);
   if (!pDecalAttributes)
      return;

   pDecalAttributes->setEnabled(false);
}

//==============================================================================
//==============================================================================
BFormationType BSquad::getFormationType() const
{
   if (mpFormation)
      return (mpFormation->getType());
   return (BFormation2::eTypeStandard);
}

//==============================================================================
//==============================================================================
BVector BSquad::getAveragePosition() const
{
   BVector ap=cOriginVector;
   int count=0;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      if (pUnit)
      {
         ap+=pUnit->getPosition();
         count++;
      }
   }
   if (count > 0)
      ap/=(float)count;
   return(ap);
}

//==============================================================================
//==============================================================================
BVector BSquad::getFormationPosition(BEntityID id) const
{
   const BFormationPosition2* pFP=mpFormation->getFormationPosition(id);
   if (!pFP)
      return(cInvalidVector);
   return(pFP->getPosition());
}

//==============================================================================
//==============================================================================
BVector BSquad::getFormationPositionAtTarget(BEntityID id, BVector targetPos)
{
   //Evil.  Way too much recalc.

   if(targetPos == cOriginVector)
   {
      //DCPO 05/16/07: This is a bug, it needs to be fixed, not covered over with
      //default behavior.
      BASSERT(0);
      //targetPos = mTargetLocation;
   }

   BUnit *pUnit = gWorld->getUnit(id);
   BASSERT(pUnit);
   
   BMatrix worldMatrix;      
   worldMatrix.makeOrient(mForward, mUp, mRight);  
   worldMatrix.setTranslation(targetPos);

   //-- Calc Position
   mpFormation->update(worldMatrix);

   //-- Get our position
   const BFormationPosition2* pFP=mpFormation->getFormationPosition(pUnit->getID());
   if (!pFP)
      return(cInvalidVector);
   syncSquadData("BSquad::getFormationPositionAtTarget -- FP offset", pFP->getOffset());
   BVector finalDestination=pFP->getPosition();

   //-- Put our formation back to normal
   getWorldMatrix(worldMatrix);
   mpFormation->update(worldMatrix);
   
   return finalDestination;
}

//==============================================================================
//==============================================================================
bool BSquad::getDesiredChildLocation(BEntityID childID, BVector &position) const
{
   if (mpFormation != NULL)
   {
      const BFormationPosition2* pFP=mpFormation->getFormationPosition(childID);
      if (!pFP)
         return(false);
      position=pFP->getPosition();
      return(true);
   }
   else
   {
      position = getPosition();
      return true;
   }
}

#ifdef _MOVE4
//==============================================================================
// getChildTargetLocation
// easy.  
// Get our move action.  
// Get the interim target for the move action.  
// Transform our formation to that target.  Currently, use the target location
// minus current location as direction.  Update when path is present to use
// last waypoint of path to target.
//==============================================================================
bool BSquad::getChildInterimTarget_4(BEntityID childID, BVector &position) 
{
   if (mpFormation != NULL)
   {
      BVector targetPosition;
      BSquadActionMove *pAction = getNonPausedMoveAction_4();
      if (pAction)
      {
         targetPosition = pAction->getInterimTarget_4();
         BVector direction = targetPosition - getPosition();
         BVector right;
         if (direction.length() < cFloatCompareEpsilon)
         {
            targetPosition = getPosition();
            direction = getForward();
         }
         else
         {
            direction.safeNormalize();
         }
         right.assignCrossProduct(cYAxisVector, direction);
         position = mpFormation->getTransformedFormationPositionFast(childID, targetPosition, direction, right);
      }
      else
      {
         const BFormationPosition2* pFP=mpFormation->getFormationPosition(childID);
         if (!pFP)
            return(false);
         position=pFP->getPosition();
      }
      return(true);
   }
   else
   {
      position = getPosition();
      return true;
   }
}

#endif

//==============================================================================
//==============================================================================
bool BSquad::getFutureDesiredChildLocation(BEntityID childID, float desiredTimeIntoFuture, BVector &position, float &realTimeNeeded)
{
   syncSquadData("BSquad::getFutureDesiredChildLocation mID", mID.asLong());
   BASSERT(gWorld->getUnit(childID));

   //Figure out where we are in the future.
   BVector futurePosition=cOriginVector;
   if (!getFuturePosition(desiredTimeIntoFuture, false, futurePosition, realTimeNeeded))
   {
      syncSquadCode("BSquad::getFutureDesiredChildLocation -- squad getFuturePosition failed");
      return (false);
   }

   //Now figure out where our child is.
   position=getFormationPositionAtTarget(childID, futurePosition);

   syncSquadData("BSquad::getFutureDesiredChildLocation -- position", position);
   return(true);
}

//==============================================================================
//==============================================================================
bool BSquad::getFutureDesiredChildPath(BEntityID childID, float desiredTimeIntoFuture, BDynamicVectorArray &futurePositions, float &realTimeNeeded, float &realDistanceNeeded)
{
   syncSquadData("BSquad::getFutureDesiredChildPath mID", mID.asLong());
   BASSERT(gWorld->getUnit(childID));

   futurePositions.clear();

   // If squad moving get path from squad move action
   BSquadActionMove* pAction = getNonPausedMoveAction_4();
   if (pAction)
   {
      if (!pAction->getFuturePosition2(desiredTimeIntoFuture, false, true, futurePositions, realTimeNeeded, realDistanceNeeded))
      {
         syncSquadCode("BSquad::getFutureDesiredChildPath -- squad getFuturePosition2 failed");
         return false;
      }
   }
   // Use squad position if squad not moving anymore
   else
   {
      futurePositions.add(getPosition());
      realTimeNeeded = 0.0f;
      realDistanceNeeded = 0.0f;
   }

   // Figure out where the child will be at the given squad positions
   BVector prevPos;
   for (uint i = 0; i < futurePositions.getSize(); i++)
   {
      // Calculate forward vector
      BVector forward;
      BVector right;
      if (i == 0)
      {
         forward = getForward();
         right = getRight();
         prevPos = futurePositions[i];
      }
      else
      {
         forward = futurePositions[i] - prevPos;
         forward.normalize();
         right.assignCrossProduct(cYAxisVector, forward);
      }

      futurePositions[i] = mpFormation->getTransformedFormationPositionFast(childID, futurePositions[i], forward, right);
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BSquad::getFuturePosition(float desiredTimeIntoFuture, bool ignoreBlocked, BVector &futurePosition, float &realTimeNeeded)
{
   //If squad moving get position from squad move action
   BSquadActionMove* pAction = getNonPausedMoveAction_4();
   if (pAction)
   {
      // Get future position from squad move action if it's controlling movement
      if (!pAction->getFuturePosition(desiredTimeIntoFuture, ignoreBlocked, futurePosition, realTimeNeeded))
      {
         syncSquadCode("BSquad::getFuturePosition -- squad getFuturePosition failed");
         return (false);
      }
      syncSquadData("BSquad::getFuturePosition SquadActionMove -- futurePosition", futurePosition);
   }
   // Use squad position if squad not moving anymore
   else
   {
      futurePosition = getPosition();
      realTimeNeeded = 0.0f;
      syncSquadData("BSquad::getFuturePosition -- futurePosition", futurePosition);
   }
   
   return (true);
}

//==============================================================================
//==============================================================================
float BSquad::getDefaultMovementProjectionTime() const
{
   const BPlatoon* pPlatoon = getParentPlatoon();
   if (pPlatoon != NULL)
      return (pPlatoon->getDefaultMovementProjectionTime());

   return cDefaultMovementProjectionTime;
}

//==============================================================================
//==============================================================================
void BSquad::updateFormation()
{
   BMatrix worldMatrix;
   getWorldMatrix(worldMatrix);
   mpFormation->update(worldMatrix);
}

//==============================================================================
//==============================================================================
bool BSquad::calculateRange(const BSimTarget* target, float& range, BProtoAction** ppProtoAction, float* pMinRange) const
{
   bool validRange=false;
   range=0.0f;
   if (pMinRange)
      *pMinRange = 0.0f;
   if (!target)
      return(false);

   //Get our target.  If it's NULL, then target position must be valid.
   BObject *pTarget=NULL;
   if (target->getID().isValid())
   {
      if (target->getID().getType() == BEntity::cClassTypeSquad)
      {
         //DCPTODO: This blows.  We should be able to directly target another squad.
         BSquad* pSquad = gWorld->getSquad(target->getID());
         if (pSquad)
            pTarget = pSquad->getLeaderUnit();
      }
      else if (target->getID().getType() == BEntity::cClassTypeUnit)
         pTarget = gWorld->getUnit(target->getID());
      else if (target->getID().getType() == BEntity::cClassTypeDopple)
         pTarget = gWorld->getDopple(target->getID());
      if (!pTarget)
         return(false);
   }
   else if (!target->isPositionValid())
      return(false);

   //Take the smallest, valid range that any of our units have for this target.
   //This isn't the fastest thing in the world, but it's not meant to be called
   //every update, either.
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      if (!pUnit)
         continue;
      BTactic* pTactic=pUnit->getTactic();
      if (!pTactic)
         continue;
      //Ignore invalid things.
      if (!pUnit->isAlive() || !pUnit->getFlagBuilt())
         continue;

      //Look up the proto action.
      //DCPTODO: Fixup how the auto target parm is handled.
      BProtoAction *pPA=pTactic->getProtoAction(pUnit->getTacticState(), pTarget,
         target->getPosition(), getPlayer(), pUnit->getPosition(), pUnit->getID(),
         target->getAbilityID(), false);
      if (!pPA)
         continue;

      if(ppProtoAction != NULL)
         *ppProtoAction = pPA;

      //Get the actual range.   
      float tempRange;
      float tempMinRange;
      switch (pPA->getActionType())
      {
         case BAction::cActionTypeUnitDetonate:
         case BAction::cActionTypeUnitRangedAttack:
         case BAction::cActionTypeUnitSecondaryTurretAttack:
            {
               const BUnitActionChargedRangedAttack *pAttackAction = NULL;

               if (pUnit)
               {
                  pAttackAction = (const BUnitActionChargedRangedAttack*)pUnit->getActionByTypeConst(BAction::cActionTypeUnitChargedRangedAttack);
               }

               if (pAttackAction && pAttackAction->isCharged() && pAttackAction->isValidPullTarget(pTarget, pPA))
                  tempRange=pPA->getMaxRange(pUnit);
               else
                  tempRange=pPA->getMaxRange(pUnit, true, false);

               tempMinRange = pPA->getMinRange();
            }
            break;
         default:
            tempRange=pPA->getWorkRange();
            tempMinRange = pPA->getMinRange();
            break;
      }
      
      //Save that if it's the smallest.
      if (!validRange || (tempRange < range))
      {
         range=tempRange;
         validRange=true;

         if (pMinRange)
            *pMinRange = tempMinRange;
      }
   }

   //Done.
   return(validRange);
}

//==============================================================================
//==============================================================================
bool BSquad::calculateRange(BSimTarget* pTarget, float* pMinRange) const
{
   float range;
   if (pTarget && calculateRange(pTarget, range, NULL, pMinRange))
   {
      pTarget->setRange(range);
      return(true);
   }
   return(false);
}

//==============================================================================
//==============================================================================
float BSquad::getPathingRange() const
{
   float range = 0.0f;
   for (uint i = 0; i < mChildren.getSize(); i++)
   {
      const BUnit* pUnit = gWorld->getUnit(mChildren[i]);
      if (pUnit != NULL)
         range = Math::Max(range, pUnit->getPathingRange());
   }
   return range;
}

//==============================================================================
//==============================================================================
float BSquad::getAttackRange() const
{
   //Get the leader because that's what we use as the sample unit.
   const BUnit* pLeaderUnit=getLeaderUnit();
   if (!pLeaderUnit)
      return (0.0f);
   const BTactic* pTactic=pLeaderUnit->getTactic();
   if (!pTactic)
      return (0.0f);

   //Use overall max range from tactic for the search.
   //DCPTODO: We really need to check the range we can use right now, but whatever.
   float range=pTactic->getOverallRange(pLeaderUnit->getID(), mSquadAI.getMode());
   if (pTactic && pTactic->getNumberWeapons() > 0)
   {
      for (int i=0; i<pTactic->getNumberWeapons(); i++)
      {
         if (pTactic->getWeapon(i)->getFlagUseGroupRange())
         {
            range += pLeaderUnit->getGroupDiagSpan();
            return (range);
         }
      }
   }
   /*float range=0.0f;
   for (long i=0; i < pTactic->getNumberProtoActions(); i++)
   {
      BProtoAction *pa=pTactic->getProtoAction(i);
      if (!pa || pa->getFlagDisabled())
         continue;
      const BWeapon* pWeapon=pTactic->getWeapon(pa->getWeaponID());
      if (!pWeapon)
         continue;
      
      float weaponRange=pWeapon->mMaxRange;
      if (pWeapon->getFlagUseGroupRange())
         weaponRange+=pLeaderUnit->getGroupDiagSpan();
      if (weaponRange > range)
         range=weaponRange;
   }*/

   return (range);
}


//==============================================================================
//==============================================================================
float BSquad::getLeashDistance() const
{
   return (getProtoSquad()->getLeashDistance());
   // mrh 3/13/08 - Leash distance no longer vary with weapons.
   //float attackRange = getAttackRange();
   //float leashDistance = gDatabase.getSquadLeashLength() - attackRange;
   //if (leashDistance < 0.0f)
   //   leashDistance = 0.0f;
   //return (leashDistance);
}


//==============================================================================
//==============================================================================
float BSquad::getLeashDeadzone() const
{
   return (getProtoSquad()->getLeashDeadzone());
}


//==============================================================================
//==============================================================================
DWORD BSquad::getLeashRecallDelay() const
{
   return (getProtoSquad()->getLeashRecallDelay());
}


//==============================================================================
//==============================================================================
float BSquad::getAggroDistance() const
{
   return (getProtoSquad()->getAggroDistance());
}


//==============================================================================
//==============================================================================
//void BSquad::getCombatDistances(float& attackRange, float& leashDistance) const
//{
//   attackRange=getAttackRange();
//   leashDistance=gDatabase.getSquadLeashLength()-attackRange;
//   if (leashDistance < 0.0f)
//      leashDistance=0.0f;
//}

//==============================================================================
//==============================================================================
long BSquad::getSquadMode() const
{
   return mSquadAI.getMode();
}

//==============================================================================
//==============================================================================
long BSquad::getCurrentOrChangingMode() const
{
   if (getFlagChangingMode())
      return mChangingToSquadMode;
   else
      return mSquadAI.getMode();
}

//==============================================================================
// Grant the player any power associated with this squad
//==============================================================================
void BSquad::grantPower()
{
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
      {
         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         if (pProtoObject)
         {
            long protoPowerID = pProtoObject->getProtoPowerID();
            if (protoPowerID != -1)
            {
               const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
               if (pProtoPower)
               {
                  BPlayer* pPlayer = getPlayer();
                  if (pPlayer)
                  {                     
                     // [5/8/2008 xemu] special case hackitude for powers on neutral buildings, they get granted to everyone since anyone could control them
                     if (pPlayer->isGaia())
                     {
                        long numPlayers = gWorld->getNumberPlayers();
                        for (long i=0; i<numPlayers; i++)
                        {
                           BPlayer* pGrantPlayer = gWorld->getPlayer(i);
                           pGrantPlayer->addPowerEntry(protoPowerID, getID(), 1, -1);
                        }
                        
                     }
                     else
                     {
                        // [5/8/2008 xemu] normal case, just give it to the squad's owning player
                        pPlayer->addPowerEntry(protoPowerID, getID(), 1, -1);
                     }
                     break;
                  }
               }
            }
         }
      }
   }
}

//==============================================================================
// Revoke from the player any power associated with this squad
//==============================================================================
void BSquad::revokePower()
{
   BPlayer* pPlayer = getPlayer();
   if (pPlayer)
   {                     
      pPlayer->removePowerEntry(-1, getID());
   }
}

//==============================================================================
//==============================================================================
int BSquad::getVetLevel() const
{
   // Start with vet level
   int returnLevel = mLevel;

   // Add garrisoned spartan levels
   if (getFlagSpartanVeterancy())
   {
      const BSquad* pSpartanSquad = getGarrisonedSpartanSquad();
      if (pSpartanSquad)
      {
         // Add spartan's levels
         returnLevel += pSpartanSquad->getVetLevel();
      }
   }

   return returnLevel;
}

//==============================================================================
//==============================================================================
int BSquad::getTechPlusVetLevel() const
{
   // Start with vet level
   int returnLevel = mLevel;

   // Add tech level
   const BProtoSquad* pPS = getProtoSquad();
   if (pPS)
      returnLevel += pPS->getTechLevel();

   // Add garrisoned spartan levels
   if (getFlagSpartanVeterancy())
   {
      const BSquad* pSpartanSquad = getGarrisonedSpartanSquad();
      if (pSpartanSquad)
      {
         // Add spartan's levels
         returnLevel += pSpartanSquad->getVetLevel();

         // Add levels specific to the join
         BUnit* pSpartanUnit = gWorld->getUnit(pSpartanSquad->getChild(0));
         if (pSpartanUnit)
         {
            const BUnitActionJoin* pJoinAction = reinterpret_cast<BUnitActionJoin*>(pSpartanUnit->getActionByType(BAction::cActionTypeUnitJoin));
            if (pJoinAction)
            {
               const BProtoAction* pPA = pJoinAction->getProtoAction();
               if (pPA)
                  returnLevel += pPA->getJoinLevels();
            }
         }
      }
   }

   return returnLevel;
}

//==============================================================================
//==============================================================================
void BSquad::addBankXP(float xp)
{
   // Skip it if veterancy is disabled
   if (!gConfig.isDefined(cConfigVeterancy) || !gScenario.getFlagAllowVeterancy())
      return;

   // For squads with garrisoned spartan, split the xp between this squad
   // and the spartan squad proportional to their combat value.
   if (getFlagSpartanVeterancy())
   {
      BSquad* pSpartanSquad = getGarrisonedSpartanSquad();
      if (pSpartanSquad)
      {
         float thisCV = getCombatValue();
         float spartanCV = pSpartanSquad->getCombatValue();
         float totalCV = thisCV + spartanCV;
         if (totalCV > 0.0f)
         {
            float thisXP = xp * thisCV / totalCV;
            float spartanXP = xp - thisXP;
            pSpartanSquad->addBankXP(spartanXP);
            xp = thisXP;
         }
      }
   }

   mXPBank += xp;
}

//==============================================================================
//==============================================================================
void BSquad::clearBankXP()
{
   mXPBank = 0;
}

//==============================================================================
//==============================================================================
void BSquad::applyBankXP()
{
   // Skip it if veterancy is disabled
   if (!gConfig.isDefined(cConfigVeterancy) || !gScenario.getFlagAllowVeterancy())
      return;

   mXP += mXPBank;

   const BProtoSquad* pProtoSquad = getProtoSquad();
   int squadLevelCount = pProtoSquad->getNumberLevels();
   for (int i=mLevel; i<squadLevelCount; i++)
   {
      float xpReq = pProtoSquad->getLevelXP(i); // index 0 returns xp required for level 1, 1 returns xp for level 2, etc.
      if (xpReq == 0.0f)
         break;
      if (mXP > xpReq)
         upgradeLevel(mLevel+1, true);
      else
         break;
   }

   clearBankXP();

   // For squads with garrisoned spartan, apply their banked XP too.
   if (getFlagSpartanVeterancy())
   {
      BSquad* pSpartanSquad = getGarrisonedSpartanSquad();
      if (pSpartanSquad)
         pSpartanSquad->applyBankXP();
   }
}

//==============================================================================
//==============================================================================
void BSquad::upgradeLevel(int level, bool doEffects)
{
   // Skip it if veterancy is disabled
   if (!gConfig.isDefined(cConfigVeterancy) || !gScenario.getFlagAllowVeterancy())
      return;

   // Get spartan container/protoObject if we're upgrading a contained spartan squad
   BUnit* pSpartanContainer = NULL;
   const BProtoObject* pSpartanPO = NULL;
   if (getFlagSpartanVeterancy() && getFlagContainedSpartan())
   {
      pSpartanContainer = getSpartanContainerUnit();
      const BUnit* pSpartanUnit = getLeaderUnit();
      if (pSpartanUnit)
         pSpartanPO = pSpartanUnit->getProtoObject();
   }

   while (mLevel < level)
   {
      mLevel++;
      int numChildren = mChildren.getNumber();
      for (int j=0; j<numChildren; j++)
      {
         BUnit* pUnit = gWorld->getUnit(mChildren[j]);
         if (pUnit && pUnit->isAlive())
            pUnit->upgradeLevel(mLevel-1, mLevel, doEffects, NULL);
      }

      // Apply spartan's level upgrades to its container unit if there is one
      if (pSpartanContainer && pSpartanPO)
         pSpartanContainer->upgradeLevel(mLevel - 1, mLevel, true, pSpartanPO);
   }
   if (doEffects)
      gWorld->getWorldSoundManager()->addSound(mPosition, getProtoSquad()->getSound(cSquadSoundChatterLevelUp), true, cInvalidCueIndex, true, true);
}

//==============================================================================
//==============================================================================
void BSquad::formUp()
{
   //DCP 05/16/07: Turn off for now.
   //-- Create a move action with desired position as current position
   /*BAction *pAction = gActionManager.createAction(BAction::cActionTypeSquadMove);
   pAction->setTargetPosition(mPosition);

   // Cloaking is handled in the idle action, so allow the move and idle actions to co-exist...
   const BProtoObject *pProto = getProtoObject();
   if (pProto && pProto->getFlag(BProtoObject::cFlagCloakMove))
      pAction->setFlagConflictsWithIdle(false);

   addAction(pAction);*/
}

//==============================================================================
//==============================================================================
void BSquad::settle() 
{
   stop();
   noStopSettle();
}

//==============================================================================
//==============================================================================
void BSquad::noStopSettle() 
{
   BVector oldPosition = getPosition();
   tieToGround();

   BMatrix worldMatrix;
   getWorldMatrix(worldMatrix);
   mpFormation->update(worldMatrix);
   
   //-- instantly settle units at the final destination
   BUnit *pUnit = NULL;
   for(long i = 0; i < mChildren.getNumber(); i++)
   {
      pUnit = gWorld->getUnit(mChildren[i]);
      if(!pUnit)
         continue;

      const BFormationPosition2* pFP=mpFormation->getFormationPosition(pUnit->getID());
      BVector desiredLocation=pFP->getPosition();
      // Set the position unless this unit is under physics control
      if (!pUnit->getFlagPhysicsControl())
      {
         #ifdef SYNC_Unit
            syncUnitData("BSquad::noStopSettle", desiredLocation);
         #endif
         pUnit->setPosition(desiredLocation);

         if (!pUnit->getFlagUseMaxHeight())
            pUnit->tieToGround();
      }
   }
}

//==============================================================================
//==============================================================================
void  BSquad::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   switch(eventType)
   {
      case BEntity::cEventDamaged:
      {
         BEntityID attackerID = gWorld->getObjectManager()->upconvertEntityID((long)data);

         const BUnit *pDefenderUnit = gWorld->getUnit(senderID);
         if (pDefenderUnit && (pDefenderUnit->getPlayerID() == getPlayerID()) && ((attackerID == cInvalidObjectID) || pDefenderUnit->isBeingAttackedByUnit(attackerID)))
            getPlayer()->getAlertManager()->createAttackAlert(mPosition, attackerID, senderID);

         updateLastDamagedTime();
         setFlagShieldDamaged(true);
         mActions.notify(BEntity::cEventDamaged, senderID, data, data2);
         break;
      }   

      case BEntity::cEventKilledUnit:
      case BEntity::cEventAllyDamaged:
      case BEntity::cEventDetected:
      {
         mActions.notify(BEntity::cEventDetected, senderID, data, data2);
         break;
      }
   }

   //-- normal processing
   BEntity::notify(eventType, senderID, data, data2);
}

//==============================================================================
//==============================================================================
bool BSquad::isIdle() const
{
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      if (pUnit && !pUnit->isIdle())
         return (false);
   }
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::canMerge()
{
   if (getFlagIsHitching() || getFlagIsUnhitching() || isHitched())
   {
      return (false);
   }

   if (mFlagNoPlatoonMerge)
   {
      return (false);
   }

   // Our platoon cannot merge if we have an uninterruptible order
   uint numOrders = mOrders.getSize();
   for (uint i = 0; i < numOrders; i++)
   {
      if (!isOrderInterruptible(&(mOrders[i])))
      {
         return (false);
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
float BSquad::getResourceAmount() const
{
   float rVal=0.0f;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      if (pUnit)
         rVal+=pUnit->getResourceAmount();
   }
   return (rVal);
}

//==============================================================================
//==============================================================================
void BSquad::clearLastAttackTargetData()
{
   mLastAttackTargetID.invalidate();
   mLastAttackPriority = -1;
   mLastAbilityAttackTargetID = cInvalidObjectID;
}

//==============================================================================
//==============================================================================
void BSquad::setRandomTacticStates() const
{
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      if (pUnit && (pUnit->getNumberTacticStates() > 1))
         pUnit->setRandomTacticState();
   }
}

//==============================================================================
//==============================================================================
void BSquad::debugRender()
{

#ifdef _MOVE4
   BMatrix matrix;
   
   // Tie position to terrain.
   BVector pos = mPosition;
   gTerrainSimRep.getHeight(pos, true);
   BSimHelper::calculateDebugRenderMatrix(pos, mForward, mUp, mRight, 1.0f, matrix);

#ifndef BUILD_FINAL
   if (!gConfig.isDefined(cConfigRenderSimDebugNoBoxes))
   {
#endif
      DWORD color = cDWORDOrange;
      float radius = 2.0f;
      // Get our movement action.. adjust the color based on it's state, if it exists.
      const BSquadActionMove* pMoveAction=getNonPausedMoveAction_4();
      if (pMoveAction)
      {
         BActionState state = pMoveAction->getState();
         switch (state)
         {
            case BAction::cStateWait:
               color = cDWORDCyan;
               break;
         }
      }
      gpDebugPrimitives->addDebugBox(matrix, radius, color);
#ifndef BUILD_FINAL
   }
#endif

   //Actions.
   mActions.debugRender();

#else
   BEntity::debugRender();
   BMatrix matrix;
#endif

   getWorldMatrix(matrix);

   //Formation.
   mpFormation->debugRender();

   #ifdef _DEBUG_SQUAD_ACTIONS
   long count = mActions.getNumberActions();
   float angle = 0.0f;
   for (long i=0; i < count; i++)
   {
      const BAction *pAction = mActions.getAction(i);
      BVector face = mForward;
      angle += cPiOver4;
      BMatrix rot;
      rot.makeRotateY(angle);
      rot.transformVector(face, face);
      face.scale(getObstructionRadius());
      BVector pos = mPosition + face;

      DWORD color = cDWORDWhite;
      switch (pAction->getType())
      {
      case BAction::cActionTypeSquadMove: 
         color = cDWORDGreen;
         break;
      case BAction::cActionTypeSquadAttack: 
         color = cDWORDRed;
         break;
      case BAction::cActionTypeEntityIdle: 
         color = cDWORDBlue;
         break;
      default:
         color = cDWORDWhite;
      }
      gpDebugPrimitives->addDebugSphere(pos, 0.25f, color, BDebugPrimitives::cCategoryMovement);
   }
   #endif

   // Turn radius
   #ifdef NEW_TURNRADIUS
      if (getFlagUpdateTurnRadius())
      {
         BVector pos = getTurnRadiusPos();
         BVector fwd = getTurnRadiusFwd();
         gTerrainSimRep.addDebugThickArrowOverTerrain(pos, fwd, 0.1f, cDWORDBlue, 1.2f, 3.0f, BDebugPrimitives::cCategoryPathing, -1.0f);
         gTerrainSimRep.addDebugThickCircleOverTerrain(pos, 1.0f, 0.1f, cDWORDBlue, 1.2f, BDebugPrimitives::cCategoryPathing, -1.0f);
      }
   #endif
}

//==============================================================================
//==============================================================================
void  BSquad::clearFlags()
{
   mFlagProtoSquad = false;
   mFlagAlive = false;
   mFlagHasShield = false;
   mFlagShieldDamaged = false;
   mFlagChangingMode = false;
   mFlagDoNextCommand = false;
   mFlagPlayingBlockingAnimation = false;
   mFlagDontPopCommand = false;
   mFlagHasHPBar = false;
   mFlagForceDisplayHP = false;
   mFlagIgnorePop = false;
   mFlagIgnoreLeash = false;
   mFlagIsUngarrisoning = false;
   mFlagIsGarrisoning = false;
   mFlagIsRepairing = false;
   mFlagSquadExistSoundPlaying = false;
   mFlagSquadChatter = false;
   mFlagIsHitching = false;
   mFlagIsUnhitching = false;
   mReverseMoveDone = false;
   mForceUpdateGarrisoned = false;
   mFlagAttackBlocked = false;
   mFlagUsingTimedAbility = false;
   mFlagUpdateGarrisonedSquadPositions = true;
   mFlagSpartanVeterancy = false;
   mFlagSpartanContainer = false;
   mFlagContainedSpartan = false;
   mFlagDazeImmobilized = false;
   mFlagCryoFrozen = false;
   mFlagUpdateTurnRadius = false;
   mFlagJumping = false;
   mFlagMatchFacing = false;
   mFlagTargettingSelectionOn = false;
   mFlagIsTransporting = false;
   mFlagCloaked = false;
   mFlagCloakDetected = false;
   mFlagWantsToCloak = false;

   const BProtoSquad* pProtoSquad = getProtoSquad();
   if (pProtoSquad)
      mFlagNoPlatoonMerge = pProtoSquad->getFlagNoPlatoonMerge();
   else
      mFlagNoPlatoonMerge = false;

   mFlagIgnoreAI=false;
   mFlagPreventScoring = false;
}

//==============================================================================
//==============================================================================
void BSquad::whackUnitPositions(float elapsedTime)
{
   //Um, HACK.  Don't use this.
   if (!mpFormation)
      return;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BUnit* pUnit=gWorld->getUnit(mChildren[i]);
      if (!pUnit)
         continue;
      const BFormationPosition2* pFP=mpFormation->getFormationPosition(pUnit->getID());
      if (!pFP)
         continue;

      // Calculate SIM velocity
      BVector simTranslation;
      simTranslation.assignDifference(pFP->getPosition(), pUnit->getPosition());
      simTranslation.y = 0.0f;
      BVector simVelocity;
      float oneOverTime = 1.0f / elapsedTime;
      simVelocity.assignProduct(oneOverTime, simTranslation);

      #ifdef SYNC_Unit
         syncUnitData("BSquad::whackUnitPositions", pFP->getPosition());
      #endif
      pUnit->setPosition(pFP->getPosition());
      pUnit->setVelocity(simVelocity);
      pUnit->setForward(getForward());
      pUnit->calcRight();
      pUnit->calcUp();
      pUnit->tieToGround();
   }
}

//==============================================================================
//==============================================================================
void BSquad::initPosition(bool fromSave)
{
   //Now that we've added everyone, initialize our position.
   if (!fromSave)
      setPosition(getAveragePosition());
   mLeashPosition=mPosition;

   mOldPosition=mPosition;
   mOldRight=mRight;
   mOldUp=mUp;
   mOldForward=mForward;

   //Then, transform our formation (which will likely cause a make under the covers).
   BMatrix worldMatrix;      
   getWorldMatrix(worldMatrix);
   BASSERT(mpFormation);
   if (fromSave)
   {
      uint numChildren = mChildren.size();
      for (uint i=0; i<numChildren; i++)
         mpFormation->addChild(mChildren[i]);
   }
   mpFormation->update(worldMatrix);

   //Then, update based on what our formation says.
   mObstructionRadiusX=mpFormation->getRadiusX();
   mObstructionRadiusY=0.0f;
   mObstructionRadiusZ=mpFormation->getRadiusZ();

   // Create the obstruction for multi-unit squads
   bool isPlayerOwned = false;
   const BProtoObject* protoObject = getProtoObject();
   if (protoObject != NULL)
      isPlayerOwned = protoObject->getFlagPlayerOwnsObstruction();
   
   #ifdef _MOVE4
   // jce [5/9/2008] -- in _MOVE4, squads must have an obstruction even if they only have one unit in them.  If it's a single guy
   // make the obstruction rotate if the unit's does.
   if(getNumberChildren() == 1)
   {
      BUnit *child = gWorld->getUnit(mChildren[0]);
      if(child)
         mFlagRotateObstruction = child->getFlagRotateObstruction();
   }
   if (isEverMobile())
   #else
   if (isEverMobile() && !isFlyingSquad && (getNumberChildren() > 1))
   #endif
      createObstruction(isPlayerOwned);
}

//==============================================================================
//==============================================================================
void BSquad::regenShield()
{
   setFlagShieldDamaged(false);

   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit *pChild = gWorld->getUnit(getChild(i));
      if (pChild && pChild->getFlagHasShield() && pChild->isAlive() && !pChild->getFlagDown())
      {
         BAction* pAction = gActionManager.createAction(BAction::cActionTypeUnitShieldRegen);
         pChild->addAction(pAction);
      }
   }
}


//==============================================================================
//==============================================================================
void BSquad::removeFromAIMission()
{
   BAIMission* pAIMission = gWorld->getAIMission(mAIMissionID);
   if (pAIMission)
      pAIMission->removeSquad(mID);
   mAIMissionID = cInvalidAIMissionID;
}


//==============================================================================
//==============================================================================
bool BSquad::doIdle()
{
   syncSquadData("BSquad::doIdle mID", mID.asLong());

   BAction* pAction=gActionManager.createAction(BAction::cActionTypeEntityIdle);
   addAction(pAction);
   return (true);
}

//==============================================================================
//==============================================================================
BActionID BSquad::doMove(BSimOrder* pOrder, BAction* pParentAction, bool platoonMove, bool monitorOpps /*= false*/, bool autoSquadMode /*= false*/, bool forceLeashUpdate /*= false*/)
{
   //If we can't move, don't.
   if (!canMoveForOrder(pOrder))
      return (cInvalidActionID);

   //DCPTODO: Figure out what to do with commands.
   //DCPTODO: Figure out what we're passing in vs. having the squad/squad action look up.
   #ifdef _MOVE4
   // DLM 5/7/8 - So for platoon moves now, we don't have to do a thing.  The platoon is charge of creating and
   // monitoring the move actions for the squad.  In fact, we should already have an action, and we'll
   // return it's value.  However, I'll still need to handle the flags we pass into the action here -- MonitorOpps & autoSquad && ForceLeashUpdate
   if (platoonMove)
   {
      BSquadActionMove *pMoveAction = getNonPausedMoveAction_4();
      if (pMoveAction)
      {
         pMoveAction->setParentAction(pParentAction);
         pMoveAction->setFlagMonitorOpps(monitorOpps);
         pMoveAction->setFlagAutoSquadMode(autoSquadMode);
         pMoveAction->setFlagForceLeashUpdate(forceLeashUpdate);

         // Any hitched squads?
         if (hasHitchedSquad())
         {
            // Ignore it
            pMoveAction->addIgnoreUnit(getHitchedSquad());
         }

         return pMoveAction->getID();
      }
      else
      {
         // DLM 5/30/08 If this was a platoon move.. but the platoon doesn't have a move action for the squad.. chances are the platoon's done,
         // or didn't need to move at all.  Set Platoon Move to false in this case, and let the squad move itself to its destination.
         // This fixes the case where an order is given to garrison at a specific location, but almost assuredly breaks something else.
         platoonMove = false;
         // return cInvalidActionID;
      }
   }
   #endif
   BSquadActionMove* pAction=reinterpret_cast<BSquadActionMove*>(gActionManager.createAction(BAction::cActionTypeSquadMove));
   pAction->setParentAction(pParentAction);
   if (pOrder)
   {
      BSimTarget orderTarget = pOrder->getTarget();
      if (pOrder->getFlagOverridePosition())
      {  
         BVector overridePos = orderTarget.getPosition();
         pAction->setOverridePosition(overridePos);
         pAction->setOverrideRadius(overridePos.w);
         overridePos.w = 1.0f;
         orderTarget.setPosition(overridePos);
      }      
      if (pOrder->getFlagOverrideRange())
      {
         pAction->setOverrideRange(orderTarget.getRange());
         orderTarget.setRange(0.0f);
      }
      pAction->setTarget(orderTarget);
   }
   pAction->setFlagPlatoonMove(platoonMove);
   pAction->setFlagMonitorOpps(monitorOpps);
   pAction->setFlagAutoSquadMode(autoSquadMode);
   pAction->setFlagForceLeashUpdate(forceLeashUpdate);
   pAction->setOrder(pOrder);

   // Any hitched squads?
   if (hasHitchedSquad())
   {
      // Ignore it
      pAction->addIgnoreUnit(getHitchedSquad());
   }

   if (!addAction(pAction, pOrder))
      return (cInvalidActionID);
   
   return (pAction->getID());
}

//==============================================================================
//==============================================================================
BActionID BSquad::doMove(BSimOrder* pOrder, BAction* pParentAction, const BSimTarget* pTarget, bool platoonMove, bool monitorOpps /*= false*/, bool autoSquadMode /*= false*/, bool forceLeashUpdate /*= false*/, BEntityID ignoreUnit /* = cInvalidObjectID */)
{
   // DLM 7/25/08 - added the IgnoreUnit parm.  This allows us to pass down to the moveaction a special unit to ignore for collision checks
   // and pathing.  Currently this is necessary for warthog ramming, to allow the squad to path through the thing being ramed.  
   //If we can't move, don't.
   debugMove4("BSquad::doMove called with Order, and target..");
   if (!canMoveForOrder(pOrder))
      return (cInvalidActionID);

   //DCPTODO: Figure out what to do with commands.
   //DCPTODO: Figure out what we're passing in vs. having the squad/squad action look up.
   #ifdef _MOVE4
   // DLM 5/7/8 - So for platoon moves now, we don't have to do a thing.  The platoon is charget of creating and
   // monitoring the move actions for the squad.  In fact, we should already have an action, and we'll
   // return it's value.  However, I'll still need to handle the flags we pass into the action here -- MonitorOpps & autoSquad && ForceLeashUpdate
   if (platoonMove)
   {
      BSquadActionMove *pMoveAction = getNonPausedMoveAction_4();
      if (pMoveAction)
      {
         pMoveAction->setParentAction(pParentAction);
         pMoveAction->setFlagMonitorOpps(monitorOpps);
         pMoveAction->setFlagAutoSquadMode(autoSquadMode);
         pMoveAction->setFlagForceLeashUpdate(forceLeashUpdate);
         pMoveAction->setOrder(pOrder);
         pMoveAction->addIgnoreUnit(ignoreUnit);

         // Any hitched squads?
         if (hasHitchedSquad())
         {
            // Ignore it
            pMoveAction->addIgnoreUnit(getHitchedSquad());
         }
         return pMoveAction->getID();
      }
      else
      {
         // DLM 5/30/08 If this was a platoon move.. but the platoon doesn't have a move action for the squad.. chances are the platoon's done,
         // or didn't need to move at all.  Set Platoon Move to false in this case, and let the squad move itself to its destination.
         // This fixes the case where an order is given to garrison at a specific location, but almost assuredly breaks something else.
         platoonMove = false;
         // return cInvalidActionID;
      }
   }
   #endif

   BSquadActionMove* pAction=reinterpret_cast<BSquadActionMove*>(gActionManager.createAction(BAction::cActionTypeSquadMove));
   pAction->setParentAction(pParentAction);
   if (pTarget)
      pAction->setTarget(*pTarget);
   else if (pOrder)
   {
      BSimTarget orderTarget = pOrder->getTarget();
      if (pOrder->getFlagOverridePosition())
      {         
         BVector overridePos = orderTarget.getPosition();
         pAction->setOverridePosition(overridePos);
         pAction->setOverrideRadius(overridePos.w);
         overridePos.w = 1.0f;
         orderTarget.setPosition(overridePos);
      }      
      if (pOrder->getFlagOverrideRange())
      {
         pAction->setOverrideRange(orderTarget.getRange());
         orderTarget.setRange(0.0f);
      }
      pAction->setTarget(orderTarget);
   }
   pAction->setFlagPlatoonMove(platoonMove);
   pAction->setFlagMonitorOpps(monitorOpps);
   pAction->setFlagAutoSquadMode(autoSquadMode);
   pAction->setFlagForceLeashUpdate(forceLeashUpdate);
   
   // jce [10/6/2008] -- only add ignore unit if it's actually valid.  Otherwise there was constantly a useless -1 in the list.
   if(ignoreUnit.isValid())
      pAction->addIgnoreUnit(ignoreUnit);

   // Any hitched squads?
   if (hasHitchedSquad())
   {
      // Ignore it
      pAction->addIgnoreUnit(getHitchedSquad());
   }

   if (!addAction(pAction, pOrder))
      return (cInvalidActionID);

   return (pAction->getID());
}

//==============================================================================
//==============================================================================
BActionID BSquad::doGarrison(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget, bool ignoreRange, bool reverseHotDrop)
{
   BSquadActionGarrison* pAction = reinterpret_cast<BSquadActionGarrison*>(gActionManager.createAction(BAction::cActionTypeSquadGarrison));
   pAction->setParentAction(pParentAction);
   if (pTarget)
   {
      pAction->setTarget(*pTarget);
   }
   pAction->setIgnoreRange(ignoreRange);
   pAction->setReverseHotDropGarrison(reverseHotDrop);

   if (!addAction(pAction, pOrder))
   {
      debugMove4("doGarrison: unable to AddAction.");
      return (cInvalidActionID);
   }

   return (pAction->getID());
}

//==============================================================================
//==============================================================================
BActionID BSquad::doUngarrison(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget, BSimTarget* pSource /*= pTarget*/, BVector rallyPoint /*= cInvalidVector*/,
                               uint8 exitDirection /*= 0*/, BVector facing /*= cInvalidVector*/, BVector spawnPoint /*= cInvalidVector*/,
                               bool allowMovingSquadsFromUngarrisonPoint /*= true*/, bool alertWhenComplete/*=false*/, bool ignoreSpawnPoint/*=false*/)
{
   BSquadActionUngarrison* pAction = reinterpret_cast<BSquadActionUngarrison*>(getActionByType(BAction::cActionTypeSquadUngarrison));
   bool createNewAction = false;
   // No current action
   if (!pAction)
   {
      createNewAction = true;      
   }   
   // Current action target does not match this action's target
   else if (pAction->getTarget() && pTarget && (*pAction->getTarget() != *pTarget))
   {
      // delete old action
      removeAction(pAction);
      createNewAction = true;
   }
   // Trying to re-issue the same action so bail
   else
   {
      return (pAction->getID());
   }

   // Create new action
   if (createNewAction)
   {
      pAction = reinterpret_cast<BSquadActionUngarrison*>(gActionManager.createAction(BAction::cActionTypeSquadUngarrison));
   }

   pAction->setParentAction(pParentAction);
   if (pTarget)
   {
      pAction->setTarget(*pTarget);
   }

   if (pSource)
   {
      pAction->setSource(*pSource);
   }
   else if (pTarget)
   {
      pAction->setSource(*pTarget);
   }

   pAction->setExitDirection(exitDirection);
   pAction->setRallyPoint(rallyPoint);
   pAction->setFacing(facing);
   pAction->setSpawnPoint(spawnPoint);
   pAction->setAllowMovingSquadsFromUngarrisonPoint(allowMovingSquadsFromUngarrisonPoint);
   pAction->setFlagAlertWhenComplete(alertWhenComplete);
   pAction->setParentAction(pParentAction);
   pAction->setIgnoreSpawnPoint(ignoreSpawnPoint);

   if (!addAction(pAction, pOrder))
   {
      return (cInvalidActionID);
   }

   return (pAction->getID());
}

//==============================================================================
//==============================================================================
//BActionID BSquad::doHitch(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget)
//{
//   BSquadActionHitch* pAction = reinterpret_cast<BSquadActionHitch*>(gActionManager.createAction(BAction::cActionTypeSquadHitch));
//   pAction->setParentAction(pParentAction);
//   if (pTarget)
//   {
//      pAction->setTarget(*pTarget);
//   }
//
//   if (!addAction(pAction, pOrder))
//   {
//      return (cInvalidActionID);
//   }
//
//   return (pAction->getID());
//}

//==============================================================================
//==============================================================================
//BActionID BSquad::doUnhitch(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget)
//{
//   BSquadActionUnhitch* pAction = reinterpret_cast<BSquadActionUnhitch*>(gActionManager.createAction(BAction::cActionTypeSquadUnhitch));
//   pAction->setParentAction(pParentAction);
//   if (pTarget)
//   {
//      pAction->setTarget(*pTarget);
//   }
//
//   if (!addAction(pAction, pOrder))
//   {
//      return (cInvalidActionID);
//   }
//
//   return (pAction->getID());
//}

//==============================================================================
//==============================================================================
BActionID BSquad::doAttack(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget)
{
   BAbilityID ruleAbilityId = cInvalidAbilityID;
   BProtoAction* pProtoAction = getProtoActionForTarget(pTarget->getID(), pTarget->getPosition(), pTarget->getAbilityID(), false, NULL, false &ruleAbilityId);
   const BTactic* pTactic = NULL;
   BUnit* pLeaderUnit = getLeaderUnit();
   if (pLeaderUnit)
      pTactic = pLeaderUnit->getTactic();
   if (pTactic && pProtoAction)
   {
      const BWeapon* pWeapon = pTactic->getWeapon(pProtoAction->getWeaponID());
      if (pWeapon && pWeapon->getSmartTargetType() != -1)
         chooseSmartTarget(pOrder, pProtoAction, *pTarget);
   }

   // update the target's ability id to the one from the rule
   pTarget->setAbilityID(ruleAbilityId);   

   BSquadActionAttack* pAction = reinterpret_cast<BSquadActionAttack*>(gActionManager.createAction(BAction::cActionTypeSquadAttack));
   pAction->setParentAction(pParentAction);
   pAction->setProtoAction(pProtoAction);
   if (pTarget)
   {
      pAction->setTarget(*pTarget);

      if (mLastAttackTargetID != pTarget->getID())
      {
         clearBankXP();
         mLastAttackTargetID = pTarget->getID();
         mLastAttackPriority = pOrder->getPriority();
      }
   }

   if (!addAction(pAction, pOrder))
   {
      return (cInvalidActionID);
   }

   return (pAction->getID());
}

//==============================================================================
//==============================================================================
bool BSquad::doAttack(BSimOrder* pOrder)
{
   syncSquadData("BSquad::doAttack mID", mID.asLong());

   //Figure out the right target to send down.
   BSimTarget target;
   determineTarget(pOrder, target, true);

   BAbilityID ruleAbilityId = cInvalidAbilityID;
   BProtoAction* pProtoAction = getProtoActionForTarget(target.getID(), target.getPosition(), target.getAbilityID(), false, NULL, false, &ruleAbilityId);
   const BTactic* pTactic = NULL;
   BUnit* pLeaderUnit = getLeaderUnit();
   if (pLeaderUnit)
      pTactic = pLeaderUnit->getTactic();
   if (pTactic && pProtoAction)
   {
      const BWeapon* pWeapon = pTactic->getWeapon(pProtoAction->getWeaponID());
      if (pWeapon && pWeapon->getSmartTargetType() != -1)
         chooseSmartTarget(pOrder, pProtoAction, target);
   }

   // update the target's ability id to the one from the rule
   target.setAbilityID(ruleAbilityId);   

   if (mLastAttackTargetID != target.getID())
   {
      clearBankXP();
      mLastAttackTargetID = target.getID();
      mLastAttackPriority = pOrder->getPriority();
   }

   BSquadActionAttack* pAction = reinterpret_cast<BSquadActionAttack*>(gActionManager.createAction(BAction::cActionTypeSquadAttack));
   pAction->setProtoAction(pProtoAction);
   pAction->setTarget(target);
   pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);
   addAction(pAction, pOrder);
   return (true);
}

//==============================================================================
//==============================================================================
BActionID BSquad::doCarpetBomb(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget)
{
   BSquadActionAttack* pAction = reinterpret_cast<BSquadActionAttack*>(gActionManager.createAction(BAction::cActionTypeSquadAttack));
   pAction->setParentAction(pParentAction);
   if (pTarget)
   {
      pAction->setTarget(*pTarget);
   }

   if (!addAction(pAction, pOrder))
   {
      return (cInvalidActionID);
   }

   return (pAction->getID());
}

//==============================================================================
//==============================================================================
BActionID BSquad::doRepair(BSimOrder* pOrder)
{
   if (getFlagIsRepairing())
      return (cInvalidActionID);

   BSquadActionRepair* pAction = reinterpret_cast<BSquadActionRepair*>(gActionManager.createAction(BAction::cActionTypeSquadRepair));

   if (!addAction(pAction, pOrder))
      return (cInvalidActionID);

   return (pAction->getID());
}

//==============================================================================
//==============================================================================
BActionID BSquad::doRepairOther(BSimOrder* pOrder)
{
   //if (getFlagIsRepairing())
      //return (cInvalidActionID);

   BSquadActionRepairOther* pAction = reinterpret_cast<BSquadActionRepairOther*>(gActionManager.createAction(BAction::cActionTypeSquadRepairOther));

   const BSimTarget &target = pOrder->getTarget();
   const BProtoAction* pProtoAction = getProtoActionForTarget(target.getID(), target.getPosition(), target.getAbilityID(), false);
   //bool allowComplete = pProtoAction ? pProtoAction->getDontLoopAttackAnim() : false;

   // DMG 10/28/2008:  Its possible that what we're repairing has already been repaired (in which case the pProtoAction is NULL).  So go ahead and fail
   // out to avoid triggering a NULL assert later.
   if (!pProtoAction)
      return (cInvalidActionID);

   //pAction->setParentAction(pParentAction);
   pAction->setProtoAction(pProtoAction);

   if (pOrder)
      pAction->setTarget(pOrder->getTarget());

   if (!addAction(pAction, pOrder))
      return (cInvalidActionID);

   return (pAction->getID());
}

//==============================================================================
//==============================================================================
float BSquad::getRepairPercent() const
{
   const BSquadActionRepair* pAction = (BSquadActionRepair*)mActions.getActionByType(BAction::cActionTypeSquadRepair);
   if (pAction)
      return (pAction->getRepairPercent());
   else
      return (0.0f);
}


//==============================================================================
//==============================================================================
void BSquad::repairHitpoints(float repairHP, bool bAllowReinforce, float& excessHP, float damageTakenScalarForNewUnits)
{
   bool isSquadAlive = isAlive();
   uint numChildren = getNumberChildren();
   if (!isSquadAlive || numChildren == 0)
      return;
   
   excessHP = 0.0f;
   float remainingHP = repairHP;
   if (remainingHP <= 0.0f)
      return;
   float maxSquadHP = getHPMax();
   if (maxSquadHP <= 0.0f)
      return;

   const BProtoSquad* pProtoSquad = getProtoSquad();
   BDynamicSimArray<BProtoSquadUnitNode> unitNodes;
   
   if (bAllowReinforce)
   {
      uint totalProtoSquadChildren = 0;
      unitNodes = pProtoSquad->getUnitNodes(); // start with all then remove the alive ones
      for (uint i=0; i<unitNodes.getSize(); i++)
         totalProtoSquadChildren += unitNodes[i].mUnitCount;
      if (totalProtoSquadChildren <= 1)
         bAllowReinforce = false;
   }

   // Restore hit points for existing units.
   for (uint i=0; i<numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(mChildren[i]);
      if (!pUnit || !pUnit->isAlive())
         continue;

      if (bAllowReinforce)
      {
         // This unit is alive so remove from the dead unit list.
         for (uint j=0; j<unitNodes.getSize(); j++)
         {
            if (unitNodes[j].mUnitType == pUnit->getProtoID())
            {
               unitNodes[j].mUnitCount--;
               break;
            }
         }
      }

      // Heal the HP on the unit.
      float maxUnitHP = pUnit->getProtoObject()->getHitpoints();
      float currentUnitHP = pUnit->getHitpoints();
      // [5/8/2008 xemu] don't heal dead-but-not-quite-yet things
      if (currentUnitHP <= 0.0f)
         return;

      if (currentUnitHP < maxUnitHP)
      {
         float deltaHP = Math::Min(maxUnitHP-currentUnitHP, remainingHP);
         pUnit->adjustHitpoints(deltaHP);
         remainingHP -= deltaHP;
         if (remainingHP <= 0.0f)
            return;
      }
   }

   if (bAllowReinforce)
   {
      // Make a simple list of the unit types to replace because they are dead.
      BStaticArray<BProtoObjectID> deadUnits;
      for (uint i=0; i<unitNodes.getSize(); i++)
      {
         for (int j=0; j<unitNodes[i].mUnitCount; j++)
            deadUnits.add(unitNodes[i].mUnitType);
      }

      // Reinforce the squad.
      bool squadReinforced = false;
      while ((remainingHP > 0.0f) && (deadUnits.getSize() > 0))
      {
         BProtoObjectID replaceUnitProtoID = deadUnits[0];
         deadUnits.removeIndex(0);

         // Spawn a new unit.
         bool validProtoID = true;
         // Get base object type if this is a unique
         if (!gDatabase.isValidProtoObject(replaceUnitProtoID))
         {
            validProtoID = false;
            const BProtoObject* pPO = getPlayer()->getProtoObject(replaceUnitProtoID);
            if (pPO)
               replaceUnitProtoID = pPO->getBaseType();
         }

         if (validProtoID || gDatabase.isValidProtoObject(replaceUnitProtoID))
         {
            BObjectCreateParms unitParms;
            unitParms.mType = BEntity::cClassTypeUnit;
            unitParms.mPlayerID = mPlayerID;
            unitParms.mProtoObjectID = replaceUnitProtoID;
            unitParms.mStartBuilt = true;
            unitParms.mPosition = mPosition;
            unitParms.mForward = mForward;
            unitParms.mRight = mRight;
            unitParms.mLevel = mLevel;

            BUnit* pUnit = gWorld->createUnit(unitParms);
            if (pUnit)
            {
               float deltaHP = Math::Min(pUnit->getHPMax(), remainingHP);
               pUnit->setHitpoints(deltaHP);
               pUnit->adjustDamageTakenScalar(damageTakenScalarForNewUnits);
               remainingHP -= deltaHP;
               addChild(pUnit->getID());
               squadReinforced = true;

               // Notify the squad that a unit was added - this is for the squad actions to add
               // unit opps to the new unit.
               notify(BEntity::cEventSquadUnitAdded, mID, pUnit->getID(), 0);

               // Copied this code from preTransform. If this squad is garrisoned, then we need to notify our parent container to include the new unit
               if (isGarrisoned())
               {                  
                  BEntityRef* pEntityRef = getContainingEntityRef();
                  if (pEntityRef)
                  {
                     BUnit* pContainingUnit = gWorld->getUnit(pEntityRef->mID);
                     if (pContainingUnit)
                     {
                        pContainingUnit->containUnit(pUnit->getID());
                        if (pContainingUnit->getSquad())
                           pContainingUnit->getSquad()->setFlagForceUpdateGarrisoned(true);
                     }
                  }
               }
            }
         }
      }

      // If the squad was reinforced, settle and fix up selection stuff.
      if (squadReinforced)
      {
         settle();
         if (gUserManager.getPrimaryUser()->getSelectionManager()->isSquadSelected(getID()))
            gUserManager.getPrimaryUser()->getSelectionManager()->selectSquad(getID());
         if (gGame.isSplitScreen() && gUserManager.getSecondaryUser()->getSelectionManager()->isSquadSelected(getID()))
            gUserManager.getSecondaryUser()->getSelectionManager()->selectSquad(getID());
      }
   }

   // Finally make sure we write out the excess.
   excessHP = remainingHP;
}


//==============================================================================
//==============================================================================
void BSquad::repairCombatValue(float repairCV, bool bAllowReinforce, float& excessCV, float damageTakenScalarForNewUnits)
{
   // Initialize the excess and handle early bails
   excessCV = 0.0f;
   if (repairCV <= 0.0f)
      return;
   float squadCV = getCombatValue();
   if (squadCV <= 0.0f)
      return;
   float maxSquadHP = getHPMax();
   if (maxSquadHP <= 0.0f)
      return;

   // Convert to HP
   float totalHPToAdd = maxSquadHP * (repairCV / squadCV);
   float excessHP = totalHPToAdd;

   // RepairHitpoints on the squad.  Return the remainingHP as the excess
   repairHitpoints(totalHPToAdd, true, excessHP, damageTakenScalarForNewUnits);

   // Return excess
   excessCV = (excessHP / maxSquadHP) * squadCV;
}

//==============================================================================
//==============================================================================
bool BSquad::doGather(BSimOrder* pOrder)
{
   syncSquadData("BSquad::doGather mID", mID.asLong());

   //Figure out the right target to send down.
   BSimTarget target;
   determineTarget(pOrder, target);

   //Determine the ProtoObject to search on.
   BEntity *pEntity=gWorld->getEntity(target.getID());
   if (!pEntity)
      return (false);
   const BProtoObject* pProtoObject=pEntity->getProtoObject();
   if (!pProtoObject)
      return (false);

   BSquadActionWork* pAction=reinterpret_cast<BSquadActionWork*>(gActionManager.createAction(BAction::cActionTypeSquadWork));
   pAction->setUnitOppType(BUnitOpp::cTypeGather);
   pAction->setSearchType(pProtoObject->getID());
   pAction->setTarget(target);
   pAction->setFlagSearchForPotentialTargets(true);
   pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);

   addAction(pAction, pOrder);
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::doCapture(BSimOrder* pOrder)
{
   syncSquadData("BSquad::doCapture mID", mID.asLong());

   //Figure out the right target to send down.
   BSimTarget target;
   determineTarget(pOrder, target);

   BSquadActionWork* pAction=reinterpret_cast<BSquadActionWork*>(gActionManager.createAction(BAction::cActionTypeSquadWork));
   pAction->setTarget(target);
   pAction->setUnitOppType(BUnitOpp::cTypeCapture);
   pAction->setFlagDoneOnOppComplete(true);
   pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);

   addAction(pAction, pOrder);
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::doJoin(BSimOrder* pOrder)
{
   syncSquadData("BSquad::doJoin mID", mID.asLong());

   //Figure out the right target to send down.
   BSimTarget target;
   determineTarget(pOrder, target);

   BSquadActionWork* pAction=reinterpret_cast<BSquadActionWork*>(gActionManager.createAction(BAction::cActionTypeSquadWork));
   pAction->setTarget(target);
   pAction->setUnitOppType(BUnitOpp::cTypeJoin);
   //pAction->setFlagDoneOnOppComplete(true); // [8-14-2008 CJS] This was causing protectors to stop following a vehicle if they were attached to a spartan that jacked the vehicle
   pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);

   addAction(pAction, pOrder);
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::doMines(BSimOrder* pOrder)
{
   syncSquadData("BSquad::doMines mID", mID.asLong());

   //Figure out the right target to send down.
   BSimTarget target;
   determineTarget(pOrder, target);

   BSquadActionWork* pAction=reinterpret_cast<BSquadActionWork*>(gActionManager.createAction(BAction::cActionTypeSquadWork));
   pAction->setTarget(target);
   pAction->setUnitOppType(BUnitOpp::cTypeMines);
   pAction->setFlagDoneOnOppComplete(true);
   pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);

   addAction(pAction, pOrder);
   return (true);
}

//==============================================================================
//==============================================================================
BActionID BSquad::doChangeMode(BSimOrder* pOrder, BAction* pParentAction)
{
   BASSERT(pOrder);
   int8 mode=pOrder->getMode();

   // Change mode orders will usually have the mode specified in the order, including for ability / move driven ones.
   // The only exception is if the change mode action is specified in a tactics file.  In this case get the new mode from
   // the proto action.
   if (mode == -1)
   {
      BSimTarget target;
      determineTarget(pOrder, target, false);

      BProtoAction* pProtoAction = getProtoActionForTarget(target.getID(), target.getPosition(), target.getAbilityID(), false);
      if (pProtoAction && (pProtoAction->getActionType() == BAction::cActionTypeSquadChangeMode))
         mode = pProtoAction->getSquadMode();
   }

   if (mode == -1)
      return (cInvalidActionID);

   //-- Wall tower code
   BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedWallTower);
   if(pRef)
   {
      BSquad* pOtherSquad = gWorld->getSquad(pRef->mID);
      if(pOtherSquad)
         pOtherSquad->doChangeMode(pOrder, pParentAction);
   }

   if (getFlagChangingMode())
   {
      // The change mode stuff currently only supports one change mode request at a time.
      //BASSERT(0);
      return (cInvalidActionID);
   }

   if (mode==getSquadMode())
      return (cInvalidActionID);

   syncSquadData("BSquad::doChangeMode mID", mID.asLong());
   syncSquadData("BSquad::doChangeMode mode", mode);

   //DCPTODO.
   //if (mode==BSquadAI::cModeStandGround || mode==BSquadAI::cModeLockdown || mSquadAI.getMode()==BSquadAI::cModeStandGround || mSquadAI.getMode()==BSquadAI::cModeLockdown)
   //   doTotalStop();

   BSquadActionChangeMode* pAction=(BSquadActionChangeMode*)gActionManager.createAction(BAction::cActionTypeSquadChangeMode);
   if (!pAction)
      return (cInvalidActionID);
   pAction->setParentAction(pParentAction);
   pAction->setSquadMode(mode);
   if (pOrder)
      pAction->setTarget(pOrder->getTarget());
   if (!addAction(pAction, pOrder))
      return (cInvalidActionID);
   return (pAction->getID());
}

//==============================================================================
//==============================================================================
bool BSquad::doGarrison(BSimOrder* pOrder)
{
   BASSERT(pOrder);
   syncSquadData("BSquad::doGarrison mID", mID.asLong());

   BSimTarget target;
   determineTarget(pOrder, target, false);

   BProtoAction* pProtoAction = getProtoActionForTarget(target.getID(), target.getPosition(), target.getAbilityID(), false);

   BSquadActionGarrison* pAction = (BSquadActionGarrison*)gActionManager.createAction(BAction::cActionTypeSquadGarrison);
   if (!pAction)
   {
      return (false);
   }

   pAction->setProtoAction(pProtoAction);

   pAction->setTarget(pOrder->getTarget());
   addAction(pAction, pOrder);
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::doUngarrison(BSimOrder* pOrder)
{
   BASSERT(pOrder);
   syncSquadData("BSquad::doUngarrison mID", mID.asLong());   

   BSquadActionUngarrison* pAction = (BSquadActionUngarrison*)gActionManager.createAction(BAction::cActionTypeSquadUngarrison);
   if (!pAction)
   {
      return (false);
   }

   pAction->setTarget(pOrder->getTarget());
   addAction(pAction, pOrder);
   return (true);
}

//==============================================================================
// Instantiate and add the squad hitch action
//==============================================================================
bool BSquad::doHitch(BSimOrder* pOrder)
{
   BASSERT(pOrder);
   syncSquadData("BSquad::doHitch mID", mID.asLong());

   BSquadActionHitch* pAction = (BSquadActionHitch*)gActionManager.createAction(BAction::cActionTypeSquadHitch);
   if (!pAction)
   {
      return (false);
   }

   pAction->setTarget(pOrder->getTarget());
   addAction(pAction, pOrder);
   return (true);
}


//==============================================================================
// Instantiate and add the squad cloak action
//==============================================================================
bool BSquad::doCloak(BSimOrder* pOrder)
{
   BASSERT(pOrder);
   syncSquadData("BSquad::doCloak mID", mID.asLong());

   if (!getFlagRecovering() && !getFlagCloaked())
   {
      BSimTarget tempTarget = pOrder->getTarget();

      determineTarget(pOrder, tempTarget, true);
      if (tempTarget.isIDValid())
         mLastAbilityAttackTargetID = tempTarget.getID();

      if (pOrder && (getParentID() != cInvalidObjectID) && (pOrder->getOwnerID() == getParentID()) && !tempTarget.isIDValid())
         doMove(pOrder, NULL, true, false, true);
      else if (pOrder && tempTarget.isIDValid())
         queueAttack(tempTarget, false);
      else
         doMove(pOrder, NULL, false, false, true);

      setFlagWantsToCloak(true);
   }

   return (true);
}

//==============================================================================
// Instantiate and add the squad jump action
//==============================================================================
bool BSquad::doJump(BSimOrder* pOrder, BJumpOrderType jumpType)
{
   BASSERT(pOrder);
   syncSquadData("BSquad::doJump mID", mID.asLong());

   //Figure out the right target to send down.
   BSimTarget target;
   determineTarget(pOrder, target, false);

   const BProtoAction* pProtoAction = getProtoActionForTarget(target.getID(), target.getPosition(), target.getAbilityID(), false);

   BSquadActionJump* pAction = reinterpret_cast<BSquadActionJump*>(gActionManager.createAction(BAction::cActionTypeSquadJump));
   pAction->setTarget(target);
   pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);
   pAction->setProtoAction(pProtoAction);

   pAction->setJumpType(jumpType);
   addAction(pAction, pOrder);

   return (true);
}

//==============================================================================
// Instantiate and add the squad point blank attack action
//==============================================================================
bool BSquad::doPointBlankAttack(BSimOrder* pOrder)
{
   BASSERT(pOrder);
   syncSquadData("BSquad::doPointBlankAttack mID", mID.asLong());

   BSimTarget target;
   determineTarget(pOrder, target, false);

   BUnitOpp opp;
   opp.init();

   opp.setType(BUnitOpp::cTypePointBlankAttack);
   opp.setTarget(target);
   opp.setSource(getID());
   opp.setPriority(BUnitOpp::cPriorityCommand);
   opp.generateID();

   addOppToChildren(opp);

   return (true);
}

//==============================================================================
// Instantiate and add the squad unhitch action
//==============================================================================
bool BSquad::doUnhitch(BSimOrder* pOrder)
{
   BASSERT(pOrder);
   syncSquadData("BSquad::doUnhitch mID", mID.asLong());

   BSquadActionUnhitch* pAction = (BSquadActionUnhitch*)gActionManager.createAction(BAction::cActionTypeSquadUnhitch);
   if (!pAction)
   {
      return (false);
   }

   pAction->setTarget(pOrder->getTarget());
   addAction(pAction, pOrder);
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::doUnpack(BSimOrder* pOrder)
{
   syncSquadData("BSquad::doUnpack mID", mID.asLong());

   //Figure out the right target to send down.
   BSimTarget target;
   determineTarget(pOrder, target);

   BSquadActionWork* pAction=reinterpret_cast<BSquadActionWork*>(gActionManager.createAction(BAction::cActionTypeSquadWork));
   pAction->setTarget(target);
   pAction->setUnitOppType(BUnitOpp::cTypeUnpack);
   pAction->setFlagDoneOnOppComplete(true);
   pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);

   addAction(pAction, pOrder);
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::doTransport(BSimOrder* pOrder)
{
   syncSquadData("BSquad::doTransport mID", mID.asLong());

   //Figure out the right target to send down.
   BSimTarget target;
   determineTarget(pOrder, target);

   BSquadActionTransport* pAction=reinterpret_cast<BSquadActionTransport*>(gActionManager.createAction(BAction::cActionTypeSquadTransport));
   pAction->setTarget(target);
   pAction->setControllableTransport(true);
   pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);

   addAction(pAction, pOrder);
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquad::doDetonate(BSimOrder* pOrder)
{
   syncSquadData("BSquad::doDetonate mID", mID.asLong());

   //Figure out the right target to send down.
   BSimTarget target;
   determineTarget(pOrder, target, false);

   const BProtoAction* pProtoAction = getProtoActionForTarget(target.getID(), target.getPosition(), target.getAbilityID(), false);
   if (!pProtoAction)
      return false;

   BSquadActionDetonate* pAction = reinterpret_cast<BSquadActionDetonate*>(gActionManager.createAction(BAction::cActionTypeSquadDetonate));
   pAction->setTarget(target);
   pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);
   pAction->setProtoAction(pProtoAction);
   addAction(pAction, pOrder);

   return (true);
}

//==============================================================================
// Check to see if all the squad's units are set to reverse move
//==============================================================================
bool BSquad::getReverseMove()
{
   bool reverse = false;
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
      {
         if (pUnit->getFlagReverseMove())
         {
            reverse = true;
         }
         else
         {
            reverse = false;
            break;
         }
      }
   }

   return (reverse);
}

//==============================================================================
//==============================================================================
void BSquad::determineTarget(BSimOrder* pOrder, BSimTarget& target, bool attack /*= false*/)
{
   //TargetID.
   target.setID(pOrder->getTarget().getID());
   //If the target is valid, make sure it's something we can use.
   if (target.getID() != cInvalidObjectID)
   {
      switch (target.getID().getType())
      {
         case BEntity::cClassTypeDopple:
            {
               const BDopple* pDopple=gWorld->getDopple(target.getID());
               if (pDopple && (pDopple->getParentID() != cInvalidObjectID))
                  target.setID(pDopple->getParentID());               
            }
            break;
         
         case BEntity::cClassTypeUnit:
            {
               BUnit* pUnit = gWorld->getUnit(target.getID());
               if (pUnit && (pUnit->getParentID() != cInvalidObjectID))
               {
                  const BProtoObject *protoObj = getLeaderUnit() ? getLeaderUnit()->getProtoObject() : false;
                  bool meleeOnly = protoObj ? protoObj->getFlagRegularAttacksMeleeOnly() : false;

                  bool boarded = (pUnit && pUnit->getFlagBeingBoarded());

                  // If this target has garrisoned enemies in cover then send that as the target
                  if (attack && pUnit->hasGarrisonedEnemies(getPlayerID()) && !meleeOnly && !boarded)
                  {
                     BEntityIDArray coverUnits = pUnit->getCoverUnits();
                     BEntityID targetID = pUnit->getParentID();
                     uint numUnits = coverUnits.getSize();
                     for (uint i = 0; i < numUnits; i++)
                     {
                        const BUnit* pCoverUnit = gWorld->getUnit(coverUnits[i]);
                        if (pCoverUnit && (pCoverUnit->getParentID() != cInvalidObjectID))
                        {
                           targetID = pCoverUnit->getParentID();
                           break;
                        }
                     }
                     target.setID(targetID);
                  }
                  // [11-12-08 CJS] If acting on a unit being jacked, target the jacking unit
                  else if (boarded)
                  {
                     // Find boarding ref
                     const BEntityRef *pRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeBoardingUnit);
                     if (pRef)
                     {
                        BEntityID attackerID = pRef->mID;
                        BUnit* pAttacker = gWorld->getUnit(attackerID);
                        if (pAttacker && pAttacker->getParentID() != cInvalidObjectID)
                        {
                           attackerID = pAttacker->getParentID();
                           target.setID(attackerID);
                        }
                        else
                           target.setID(pUnit->getParentID());
                     }
                     else
                        target.setID(pUnit->getParentID());
                  }
                  else
                  {
                     target.setID(pUnit->getParentID());
                  }
               }
            }
            break;

         case BEntity::cClassTypeSquad:
            {
               // If this target has garrisoned enemies in cover then send that as the target (unless we are a melee only unit
               const BProtoObject *protoObj = getLeaderUnit() ? getLeaderUnit()->getProtoObject() : false;
               bool meleeOnly = protoObj ? protoObj->getFlagRegularAttacksMeleeOnly() : false;
               BSquad* pSquad = gWorld->getSquad(target.getID());

               bool boarded = (pSquad && pSquad->getLeaderUnit() && pSquad->getLeaderUnit()->getFlagBeingBoarded());

               if (attack && !meleeOnly && !boarded)
               {
                  if (pSquad && pSquad->hasGarrisonedEnemies(getPlayerID()))
                  {
                     BEntityIDArray coverSquads = pSquad->getCoverSquads();
                     uint numCoverSquads = coverSquads.getSize();
                     for (uint i = 0; i < numCoverSquads; i++)
                     {
                        if (coverSquads[i] != cInvalidObjectID)
                        {
                           target.setID(coverSquads[i]);
                           break;
                        }
                     }
                  }
               }
               // [11-12-08 CJS] If acting on a unit being jacked, target the jacking unit
               else if (boarded)
               {
                  // Find boarding ref
                  const BEntityRef *pRef = pSquad->getLeaderUnit()->getFirstEntityRefByType(BEntityRef::cTypeBoardingUnit);
                  if (pRef)
                  {
                     BEntityID attackerID = pRef->mID;
                     BUnit* pAttacker = gWorld->getUnit(attackerID);
                     if (pAttacker && pAttacker->getParentID() != cInvalidObjectID)
                     {
                        attackerID = pAttacker->getParentID();
                        target.setID(attackerID);
                     }
                  }
               }
            }
            break;

         default:
            break;
      }
   }      

   //Position.
   if (pOrder->getTarget().isPositionValid())
      target.setPosition(pOrder->getTarget().getPosition());
   //Range.
   if (pOrder->getTarget().isRangeValid())
      target.setRange(pOrder->getTarget().getRange());
   //Ability.
   if (pOrder->getTarget().isAbilityIDValid())
      target.setAbilityID(pOrder->getTarget().getAbilityID());
}

//==============================================================================
//==============================================================================
bool BSquad::isOrderInterruptible(BSimOrderEntry* pOrderEntry)
{
   // Don't interrupt orders that will leave units or squads in a bad state
   if (pOrderEntry->getState() != BSimOrderEntry::cStateQueued)
   {
      int actionIndex;
      for (actionIndex = 0; actionIndex < mActions.getNumberActions(); actionIndex++)
      {
         BAction* pAction = mActions.getAction(actionIndex);
         if (pAction && (pAction->getOrder() == pOrderEntry->getOrder()) && !pAction->isInterruptible())
            return false;
      }
   }
   return true;
}

//==============================================================================
//==============================================================================
void BSquad::adjustPlayerPop(bool add)
{
   float multiplier = (add ? 1.0f : -1.0f);
   BPlayer* pPlayer=getPlayer();
   const BProtoSquad* pProtoSquad=getProtoSquad();
   if (pProtoSquad)
   {
      long unitNodeCount=pProtoSquad->getNumberUnitNodes();
      // [10/29/2008 xemu] to mitigate some floating point issues, sum the pop contribution for the squad and round before applying 
      float popCost = 0.0f;
      short popID = -1;

      for (long i=0; i<unitNodeCount; i++)
      {
         const BProtoSquadUnitNode& unitNode = pProtoSquad->getUnitNode(i);
         const BProtoObject* pProtoObject = pPlayer->getProtoObject(unitNode.mUnitType);
         long unitPopCount=pProtoObject->getNumberPops();
         for (long j=0; j<unitPopCount; j++)
         {
            uint unitPopCount = (uint)pProtoObject->getNumberPops();
            for (uint j = 0; j < unitPopCount; j++)
            {
               BPop pop = pProtoObject->getPop(j);
               popCost = popCost + (pop.mCount * unitNode.mUnitCount);
               BASSERT((popID == -1) || (popID == pop.mID));
               popID = pop.mID;
            }
         }
      }
      // [10/29/2008 xemu] round it off 
      popCost = floorf(popCost + 0.5f);
      pPlayer->adjustPopCount(popID, popCost * multiplier);
   }
   else
   {
      const BProtoObject* pProtoObject=getProtoObject();
      if (pProtoObject)
      {
         long unitPopCount=pProtoObject->getNumberPops();
         for (long j=0; j<unitPopCount; j++)
         {
            uint unitPopCount = (uint)pProtoObject->getNumberPops();
            for (uint j = 0; j < unitPopCount; j++)
            {
               BPop pop = pProtoObject->getPop(j);
               pPlayer->adjustPopCount(pop.mID, pop.mCount * multiplier);
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
uint BSquad::getNumVisibleChildren(BTeamID teamID) const
{
   uint numVis = 0;

   long numUnits = mChildren.getNumber();
   for(long i = 0; i < numUnits; i++)
   {
      BUnit *pUnit = gWorld->getUnit(mChildren[i]);
      if(pUnit && pUnit->isVisible(teamID) && !pUnit->isDoppled(teamID))
      {         
         numVis++;
      }
   }
   return numVis;
}

//==============================================================================
//==============================================================================
void BSquad::playBirthAnimation(int id)
{
   const BProtoSquad* pProtoSquad = getProtoSquad();
   if (pProtoSquad)
   {
      bool success = doPlayBlockingAnimation(NULL, BObjectAnimationState::cAnimationStateMisc, pProtoSquad->getBirthAnim(id), true, false, false, cInvalidCueIndex, false, true, true, true, true);

      // Set the units' no render flag so they don't render until the birth anim starts playing.  The units' play anim action will clear this flag.
      if (success)
      {
         uint numUnits = mChildren.getSize();
         for (uint i = 0; i < numUnits; i++)
         {
            BUnit *pUnit = gWorld->getUnit(mChildren[i]);
            if (pUnit)
               pUnit->setFlagNoRender(true);
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquad::hasCompletedBirth() const
{
   // if we are passive garrisoned, then we are still inside of a base
   if (getFlagPassiveGarrisoned())
      return false;

   // else check the animation
   const BSquadActionPlayBlockingAnimation* pPlayBlockingAnimation = reinterpret_cast<const BSquadActionPlayBlockingAnimation*>(mActions.getActionByType(BAction::cActionTypeSquadPlayBlockingAnimation));
   const BProtoSquad* pProtoSquad = getProtoSquad();
   if (pPlayBlockingAnimation && pProtoSquad && 
       pPlayBlockingAnimation->getAnimationState() == BObjectAnimationState::cAnimationStateMisc && 
       (pPlayBlockingAnimation->getAnimationType() == pProtoSquad->getBirthAnim(0) ||
       pPlayBlockingAnimation->getAnimationType() == pProtoSquad->getBirthAnim(1) ||
       pPlayBlockingAnimation->getAnimationType() == pProtoSquad->getBirthAnim(2) ||
       pPlayBlockingAnimation->getAnimationType() == pProtoSquad->getBirthAnim(3)))
       return false;

   return true;
}

//==============================================================================
//==============================================================================
bool BSquad::addCryo(float cryoAmount, float* freezingThawTime, float* frozenThawTime)
{
   BSquadActionCryo* pCryoAction = reinterpret_cast<BSquadActionCryo*>(mActions.getActionByType(BAction::cActionTypeSquadCryo));

   if (!pCryoAction)
   {
      pCryoAction = (BSquadActionCryo*)gActionManager.createAction(BAction::cActionTypeSquadCryo);
      if (!addAction(pCryoAction, NULL))
         return false;
   }

   BASSERT(pCryoAction);
   if (freezingThawTime)
      pCryoAction->updateFreezingThawTime(*freezingThawTime);
   if (frozenThawTime)
      pCryoAction->updateFrozenThawTime(*frozenThawTime);
   pCryoAction->addCryo(cryoAmount);

   return true;
}

//==============================================================================
//==============================================================================
void BSquad::forceCryoFrozenKill(BPlayer *pPowerPlayer, BEntityID killerID)
{
   BSquadActionCryo* pCryoAction = reinterpret_cast<BSquadActionCryo*>(mActions.getActionByType(BAction::cActionTypeSquadCryo));

   if (!pCryoAction)
   {
      pCryoAction = (BSquadActionCryo*)gActionManager.createAction(BAction::cActionTypeSquadCryo);
      addAction(pCryoAction, NULL);
   }

   BASSERT(pCryoAction);
   pCryoAction->forceCryoFrozen();
   pCryoAction->cryoKillSquad(pPowerPlayer, killerID);
}

//==============================================================================
//==============================================================================
void BSquad::chooseSmartTarget(BSimOrder* pOrder, BProtoAction* pProtoAction, BSimTarget& target)
{
   if (!pProtoAction)
      return;

   if ((target.getID() == cInvalidObjectID) && !pProtoAction->targetsAir())
      return; 

   // DMG NOTE: Does this still work when the leader is something else? (Like if a Spartan joins?)
   BUnit* pLeaderUnit = getLeaderUnit();
   if (!pLeaderUnit)
      return;

   const BTactic* pTactic = pLeaderUnit->getTactic();
   if (!pTactic)
      return;

   const BWeapon* pWeapon = pTactic->getWeapon(pProtoAction->getWeaponID());

   if ( pWeapon && pWeapon->getSmartTargetType() != -1)
   {
      BSquad* pTargetSquad = gWorld->getSquad(target.getID());

      if (pTargetSquad || pProtoAction->targetsAir())
      {
         bool bRedundantTarget = false;

         if (pTargetSquad)
            pLeaderUnit = pTargetSquad->getLeaderUnit();

         if (pLeaderUnit || pProtoAction->targetsAir())
         {
            const BProtoObject* pObject = NULL;
            if (pLeaderUnit)
               pObject = pLeaderUnit->getProtoObject();
            BAbilityID abilityID = gDatabase.getSquadAbilityID(this, gDatabase.getAIDCommand());
            
            BAbility * pAbility = gDatabase.getAbilityFromID(abilityID);

            float smartTargetRange = 0.0f;

            if (pAbility)
               smartTargetRange = pAbility->getSmartTargetRange();

            if ((pTargetSquad && !pTargetSquad->incrementSmartTargetReference(pWeapon->getSmartTargetType())) || (!pTargetSquad && pProtoAction->targetsAir()))
            {
               // Need a new target...
               BUnitQuery query(target.getPosition(), smartTargetRange, false);            
               query.setFlagIgnoreDead(true);
               query.setUnitVisibility(getPlayerID());
               query.setRelation(getPlayerID(), cRelationTypeEnemy);

               if (pTargetSquad && pObject)
               {
                  for( long j = 0; j < gDatabase.getNumberObjectTypes(); j++)
                  {
                     if (pObject->isType(j))
                     {
                        query.addObjectTypeFilter(j);
                     }
                  }
               }
               else if (pProtoAction->targetsAir())
                  query.addObjectTypeFilter(gDatabase.getObjectType("Flying"));

               BEntityIDArray results(0, 100);
               long numResults = gWorld->getSquadsInArea(&query, &results, false);

               for (long i = 0; i < numResults; ++i)
               {
                  pTargetSquad = gWorld->getSquad(results[i]);

                  if (pTargetSquad->incrementSmartTargetReference(pWeapon->getSmartTargetType()))
                  {
                     bRedundantTarget = false;
                     break;
                  }
                  else
                     bRedundantTarget = true;
               }
            }
         }

         if (pTargetSquad)
         {
            target.setID(pTargetSquad->getID());
            target.setPosition(pTargetSquad->getPosition());

            if (bRedundantTarget)
               target.invalidateAbilityID(); // Revert to normal attack
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquad::incrementSmartTargetReference(int8 type)
{
   if ( type > -1 && type < cSmartTargetMax)
   {
      int maxAttackers = 1;
      switch (type)
      {
      case cSmartTargetStasis:
         {
            const BUnit* pLeaderUnit = getLeaderUnit();
            if (pLeaderUnit)
               maxAttackers = pLeaderUnit->getProtoObject()->getNumStasisFieldsToStop();
         }
         break;
      }

      if (mSmartTargetValues[type] >= maxAttackers )
         return false;

      mSmartTargetValues[type]++;
      return true;

   }

   return false;
}

//==============================================================================
//==============================================================================
void BSquad::addDaze(float dazeDuration, float movementModifier, bool smartTarget)
{
   if (!isDazed())
   {
      // Take any daze resists into account
      if (getProtoSquad())
         dazeDuration *= getProtoSquad()->getDazeResist();

      if (dazeDuration > 0.0f)
      {
         BSquadActionDaze* pDazeAction = (BSquadActionDaze*)gActionManager.createAction(BAction::cActionTypeSquadDaze);

         pDazeAction->setupDazeAttributes(dazeDuration, movementModifier, smartTarget);

         addAction(pDazeAction, NULL);
      }
   }
}

//==============================================================================
//==============================================================================
void BSquad::resetDaze(float dazeDuration)
{
   BAction* pAction = mActions.getActionByType(BAction::cActionTypeSquadDaze);

   if (pAction)
   {
      BSquadActionDaze* pDazeAction = (BSquadActionDaze*)pAction;

      if (pDazeAction)
      {
         // Take any daze resists into account
         if (getProtoSquad())
            dazeDuration *= getProtoSquad()->getDazeResist();

         pDazeAction->resetDaze(dazeDuration);
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquad::isDazed() const
{
   const BAction* pAction = mActions.getActionByType(BAction::cActionTypeSquadDaze);

   if (pAction)
      return true;

   return false;
}

//==============================================================================
//==============================================================================
bool BSquad::decrementSmartTargetReference(int8 type)
{
   if ( type > -1 && type < cSmartTargetMax)
   {
      if (mSmartTargetValues[type] != 0)
      {
         mSmartTargetValues[type]--;
         return true;
      }
   }

   return false;
}


//==============================================================================
//==============================================================================
void BSquad::startExistSound()
{
   if(gConfig.isDefined(cConfigNoExistSound))
      return;

   if(getFlagSquadExistSoundPlaying())
      return;

   const BProtoSquad* pProtoSquad=getProtoSquad();
   if (pProtoSquad)
   {
      BCueIndex startExist = pProtoSquad->getSound(cSquadSoundExist);
      if(startExist != cInvalidCueIndex)
      {
         uint numVis = getNumVisibleChildren(gUserManager.getPrimaryUser()->getTeamID());
         float minSquadSize = (float)pProtoSquad->getSquadSize() * cMinSquadChatterSizePct;
         if(numVis >= cMinSquadChatterSize && numVis >= minSquadSize) //-- Only play exist sounds with 2 or more units and more than 20%
         {     
            BRTPCInitArray rtpc;
            rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::FOW_DISTANCE, 0.0f) );
            gWorld->getWorldSoundManager()->addSound(this, -1, startExist, false, cInvalidCueIndex, false, false, &rtpc);
            setFlagSquadExistSoundPlaying(true);
         }
      }
      else
         setFlagSquadExistSoundPlaying(true); //-- Set this flag anyway, so we will be sure to call stop exist
   }
}

//==============================================================================
//==============================================================================
void BSquad::stopExistSound(bool force)
{
   if(!getFlagSquadExistSoundPlaying())
      return;

   const BProtoSquad* pProtoSquad=getProtoSquad();
   if (pProtoSquad)
   {
      BCueIndex stopExist = pProtoSquad->getSound(cSquadSoundStopExist);
      if(stopExist != cInvalidCueIndex)
      {
         //-- Unless the squad is going away (force == true), then we don't stop the exist sound until
         //-- the squad size is small enough relative to the 
         uint numVis = getNumVisibleChildren(gUserManager.getPrimaryUser()->getTeamID());
         float minSquadSize = (float)pProtoSquad->getSquadSize() * cMinSquadChatterSizePct;
         if(numVis < cMinSquadChatterSize || numVis <= minSquadSize || force)
         {
            BRTPCInitArray rtpc;
            rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::FOW_DISTANCE, 0.0f) );
            gWorld->getWorldSoundManager()->addSound(this, -1, stopExist, false, cInvalidCueIndex, false, false, &rtpc);
            setFlagSquadExistSoundPlaying(false);
         }
      }
      else
         setFlagSquadExistSoundPlaying(false);
   }      
}

//==============================================================================
//==============================================================================
void BSquad::updateChatter(float elapsed)
{
   if(getFlagSquadChatter() == false)
      return; 

   //-- Get our current sound state and target proto
   long targetProtoSquad = cInvalidProtoID;
   BSquadSoundState soundState = cSquadSoundStateInvalid;
   getCurrentSoundState(soundState, targetProtoSquad);
   if(soundState == cSquadSoundStateInvalid)
      return;

   bool playSound = (mChatterTimer[soundState] == 0); 

   //-- Check our timer
   if(mChatterTimer[soundState] == -1.0f || playSound)
   {              
      //-- Reset the timer
      mChatterTimer[soundState] = getRandRangeFloat(cUnsyncedRand, gWorld->getWorldSoundManager()->getMinChatterTime(soundState), gWorld->getWorldSoundManager()->getMaxChatterTime(soundState));
   }
   else
   {
      //-- Decrement the timer            
      if(mChatterTimer[soundState] <= elapsed)
         mChatterTimer[soundState] = 0.0f;
      else
         mChatterTimer[soundState] -= elapsed;
   }

   if(playSound)
   {
      //-- Is the global timer set? If so, we don't chat.
      if(gWorld->getWorldSoundManager()->getGlobalChatterTimer(soundState) > 0.0f)
         return;

      //-- Is the unit not visible to the primary player
      if(isVisible(gUserManager.getPrimaryUser()->getPlayer()->getTeamID()) == false)
         return;
      
      const BProtoSquad* pProto = getProtoSquad();
      if(!pProto)
         return;

      if(pProto->getSquadSize() <= 1)
      {
         //-- See if there are other squads within our LOS         
         bool foundOtherSquad = false;
         const BEntityIDArray& results = getVisibleSquads();
         long numResults = results.getNumber();
         for(long i=0; i < numResults; i++)
         {
            BSquad* pSquad= gWorld->getSquad(results[i]);
            if(pSquad)
            {
               if(getID() == pSquad->getID())
                  continue;
               
               if(pSquad->getPlayer() && (pSquad->getPlayer()->isAlly(pSquad->getPlayerID()) == false))
                  continue;         

               if(pSquad->getProtoID() == getProtoID())
               {
                  foundOtherSquad = true;
                  break;
               }
            }
         }
         if(foundOtherSquad == false)
         {
            //-- Single unit squad with no other squads nearby, don't chat
            return;
         }
      }
      else if(mChildren.getSize() < cMinSquadChatterSize)
         return;
      
      //-- Play the sound, ignore the size since we've already special checked it.
      playSquadStateChatter(soundState, targetProtoSquad);
   }
}

//==============================================================================
//==============================================================================
void BSquad::setRecover(int type, float time, int abilityID)
{ 
   if (type == -1)
   {
      mRecoverType=-1; 
      mRecoverTime=0.0f; 
      setFlagRecovering(false);
   }
   else
   {
      mRecoverType=type; 
      mRecoverTime=time; 
      setFlagRecovering(true);
   }
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
         pUnit->notify(cEventRecoverSet, mID, abilityID, type);
   }
   activateEffect(cEffectRecovering, (type!=-1));

   if (type == cRecoverAbility && abilityID >= 0)
      gWorld->notify(BEntity::cEventSquadAbility, mID, mPlayerID, abilityID);
}

//==============================================================================
//==============================================================================
void BSquad::updateRecover(float elapsed)
{
   if (mRecoverType != -1)
   {
      if (gWorld->getFlagQuickBuild())
         elapsed *= 30.0f;
      mRecoverTime -= elapsed;
      if (mRecoverTime <= 0.0f)
         setRecover(-1, 0.0f, -1);
   }
}

//==============================================================================
//==============================================================================
float BSquad::getRecoverPercent() const
{
   // compute recover percent
   const BUnit* pLeader = getLeaderUnit();
   if (!pLeader)
      return 0.0f;

   const BProtoObject* pProtoObject = pLeader->getProtoObject();
   if (!pProtoObject)
      return 0.0f;

   int abilityID = pProtoObject->getAbilityCommand();
   if (abilityID < 0)
      return 0.0f;

   const BPlayer* pPlayer = getPlayer();
   if (!pPlayer)
      return 0.0f;

   float duration = pPlayer->getAbilityRecoverTime(abilityID);
   if (duration <= cFloatCompareEpsilon)
      return 0.0f;

   float curRecoverTime = getRecoverTime();   
   float alpha = 1.0f - (curRecoverTime / duration);
   alpha = Math::Clamp(alpha, 0.0f, 1.0f);

   return alpha;
}

//==============================================================================
//==============================================================================
void BSquad::setAbilityTimer(int type, float time, int abilityID)
{
   if (type == -1)
   {
      mAbilityTime=0.0f;
      setFlagUsingTimedAbility(false);
      notify(cEventSquadTimedAbilityOver, mID, abilityID, type);  
   }
   else
   { 
      mAbilityTime=time; 
      setFlagUsingTimedAbility(true);  
   }
}

//==============================================================================
//==============================================================================
void BSquad::updateAbilityDuration(float elapsed)
{
   if (mFlagUsingTimedAbility)
   {
      mAbilityTime -= elapsed;
      if (mAbilityTime <= 0.0f)
         setAbilityTimer(-1, 0.0f, -1);
   }
}

//==============================================================================
//==============================================================================
void BSquad::updateEffects()
{
   if (!mFlagHasActiveEffect || !mpEntityRefs)
      return;

   bool havePos = false;
   BVector pos;
   BVector pos2;

   int numEffects = 0;

   int numRefs = mpEntityRefs->getNumber();
   for (int i=numRefs-1; i>=0; i--)
   {
      BEntityRef ref = (*mpEntityRefs)[i];
      if (ref.mType == BEntityRef::cTypeEffect)
      {
         BObject* pObject = gWorld->getObject(ref.mID);
         if (!pObject)
         {
            removeEntityRef(i);
            continue;
         }
         numEffects++;
         if (!havePos)
         {
            pos = mPosition;
            gHPBar.getSquadHPBarPosition(this, pos);
            havePos = true;
         }
         pos2 = pos;
         // ajl 3/21/08 - temp hard code lowering effect position so that it will be closer to the squad (under the HP bar)
         if (ref.mData2 == cEffectRecovering)
         {
            float yOffset = 0.0f;
            if (gConfig.get(cConfigRecoverEffectOffset, &yOffset))
               pos2.y += yOffset;

            // Do a simple billboarding of the recover effect so it can be seen
            if (gConfig.isDefined(cConfigBillboardRecoverEffect))
            {
               BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
               if (pUser && pUser->getCamera())
               {
                  const BCamera* pCamera = pUser->getCamera();
                  BVector dir = -pCamera->getCameraDir();
                  dir.y = 0.0f;
                  if (!dir.safeNormalize())
                     dir = pObject->getForward();
                  BVector right = cYAxisVector.cross(dir);
                  BMatrix rotation;
                  rotation.makeOrient(dir, cYAxisVector, right);
                  pObject->setRotation(rotation, false);
               }
            }
         }
         pObject->setPosition(pos2);
      }
   }

   if (numEffects == 0)
      mFlagHasActiveEffect = false;
}

//==============================================================================
//==============================================================================
void BSquad::removeEffects(bool bKillImmediately)
{
   if (!mFlagHasActiveEffect || !mpEntityRefs)
      return;
   int numRefs = mpEntityRefs->getNumber();
   for (int i=numRefs-1; i>=0; i--)
   {
      BEntityRef ref = (*mpEntityRefs)[i];
      if (ref.mType == BEntityRef::cTypeEffect)
      {
         BObject* pObject = gWorld->getObject(ref.mID);
         if (pObject)
            pObject->kill(bKillImmediately);
      }
   }
}

//==============================================================================
//==============================================================================
void BSquad::getCurrentSoundState(BSquadSoundState& soundState, long& protoID)
{
   bool isMoving=false;
   bool isAttacking=false;

   BAction *pAttackAction = NULL;
   uint numActions = (uint)mActions.getNumberActions();
   for(uint i = 0; i < numActions; i++)
   {
      BAction *pAction = mActions.getAction(i);
      if(!pAction)
         continue;

      if(pAction->getType() == BAction::cActionTypeSquadMove)
         isMoving = true;
      else if(pAction->getType() == BAction::cActionTypeSquadAttack)
      {
         pAttackAction = pAction;
         isAttacking = true;
      }
   }

   isMoving = isMoving && getFlagMoving();

   if(isMoving && !isAttacking)
      soundState = cSquadSoundStateMove;
   else if(isAttacking)
   {
      protoID = cInvalidProtoID;
      if(pAttackAction)
      {
         BSquad *pTarget = gWorld->getSquad(reinterpret_cast<BSquadActionAttack*>(pAttackAction)->getTarget()->getID());
         if(pTarget)
            protoID = pTarget->getProtoSquadID();
      }

      if(isMoving)
         soundState = cSquadSoundStateMoveAttack;
      else
         soundState = cSquadSoundStateAttack;
   }
   else 
   {
      //-- make sure we havent attacked for a little while
      if( (gWorld->getGametime() - mLastAttackedTime) > gWorld->getWorldSoundManager()->getTimeToIdleChat())
      {
         soundState = cSquadSoundStateIdle;
      }      
      else
         soundState = cSquadSoundStateInvalid;
   }
}

//==============================================================================
//==============================================================================
void BSquad::createAudioReaction(BSquadSoundType soundType)
{
   //-- Find units in area that want to react to whatever we just did. 
   const BProtoSquad* pProtoSquad = getProtoSquad();
   if (pProtoSquad && pProtoSquad->getFlagCreateAudioReactions())
   {
      const BUnit* pLeader = gWorld->getUnit(getChild(0));      
      if(pLeader)
      {
         const BProtoObject* pProtoObject = pLeader->getProtoObject();
         if(pProtoObject)
         {
            //-- Area Search
            const BEntityIDArray& results = getVisibleSquads();
            long numResults = results.getNumber();
            for(long i=0; i < numResults; i++)
            {
               BSquad* pSquad= gWorld->getSquad(results[i]);
               if(pSquad)
               {
                  if(getID() == pSquad->getID())
                     continue;

                  if(pSquad->getPlayer() && (pSquad->getPlayer()->isAlly(getPlayerID()) == false))
                     continue;         

                  pSquad->playChatterSound(soundType, pProtoSquad->getID());                  
               }
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BSquad::playSquadStateChatter(BSquadSoundState state, int32 targetProtoSquad)
{  
   const BProtoSquad* pProtoSquad = getProtoSquad();
   if(!pProtoSquad)
      return;

   BSquadSoundType squadSoundType = cSquadSoundNone;      
   switch(state)
   {
      case cSquadSoundStateMove:
         squadSoundType = cSquadSoundChatterMove;
         break;
      case cSquadSoundStateAttack:
         squadSoundType = cSquadSoundChatterAttack;                  
         break;
      case cSquadSoundStateMoveAttack:
         squadSoundType = cSquadSoundChatterMoveAttack;
         break;
      case cSquadSoundStateIdle:
         squadSoundType = cSquadSoundChatterIdle;
         break;
   }

   if(squadSoundType == cSquadSoundNone)
      return;
   
   bool result = playChatterSound(squadSoundType, targetProtoSquad, false);
   if(result)
      gWorld->getWorldSoundManager()->setGlobalChatterTimer(state);
}

//==============================================================================
//==============================================================================
bool BSquad::playChatterSound(BSquadSoundType squadSoundType, int32 targetOrEnemyProtoSquadID, bool checkSquadSize, BEntityID castingSquadID)
{
   const BProtoSquad* pProtoSquad = getProtoSquad();
   if(!pProtoSquad)
      return false;

   if(getFlagSquadChatter() == false)
      return false;

   //-- Is the unit on screen?   
   if(gRender.getViewParams().isPointOnScreen(getPosition()) == false)
      return false;

   //-- See if there is chance not to play at all
   float totalChance = gWorld->getWorldSoundManager()->getChanceToPlayChatter(squadSoundType);
   if(totalChance != 1.0f)
   {
      bool playChatter = (getRandRangeFloat(cUnsyncedRand, 0.0f, 1.0f) < totalChance);
      if(!playChatter)
         return false;
   }

   if(checkSquadSize == false || mChildren.getSize() >= cMinSquadChatterSize || (pProtoSquad->getSquadSize() == 1))
   {
      // If we're playing a "killed", "taunt" or "waskilled" VO while targetting specific groups, remap to generic protosquads that all sound events targetting these groups use.
      if ((targetOrEnemyProtoSquadID != -1) &&
          ((squadSoundType == cSquadSoundChatterAttack) ||
           (squadSoundType == cSquadSoundChatterAllyKilled) ||
           (squadSoundType == cSquadSoundChatterKilledEnemy)))
      {
         const BPlayer* pPlayer = getPlayer();
         if (pPlayer)
         {
            const BProtoSquad* pTargetOrEnemyProtoSquad = pPlayer->getProtoSquad(targetOrEnemyProtoSquadID);
            if (pTargetOrEnemyProtoSquad)
            {
               if (pTargetOrEnemyProtoSquad->getFlagFlyingFlood())
               {
                  targetOrEnemyProtoSquadID = gDatabase.getProtoSquad("FlyingFlood");
               }
               else if (pTargetOrEnemyProtoSquad->getFlagFlood())
               {
                  targetOrEnemyProtoSquadID = gDatabase.getProtoSquad("Flood");
               }
               else if (pTargetOrEnemyProtoSquad->getFlagForerunner())
               {
                  targetOrEnemyProtoSquadID = gDatabase.getProtoSquad("Forerunner");
               }
               else if (pTargetOrEnemyProtoSquad->getFlagRebel())
               {
                  targetOrEnemyProtoSquadID = gDatabase.getProtoSquad("Rebel");
               }
            }
         }
      }

      //-- Get the generic Sound Cue
      BCueIndex genericSoundCue = pProtoSquad->getSound(squadSoundType, -1, cWorldNone, getID(), castingSquadID);

      //-- Get the specific Sound Cue
      BCueIndex specificSoundCue = pProtoSquad->getSound(squadSoundType, targetOrEnemyProtoSquadID, gWorld->getWorldID(), getID(), castingSquadID);      

      BCueIndex soundCue = cInvalidCueIndex;

      //-- See if we want to do specific or generic chatter      
      float chanceToUseSpecific = gWorld->getWorldSoundManager()->getChanceToPlaySpecificChatter(squadSoundType);
      bool useTargetSpecific = (getRandRangeFloat(cUnsyncedRand, 0.0f, 1.0f) < chanceToUseSpecific);

      //-- Force specific one if the generic one isn't specified and vice versa.
      if(genericSoundCue == cInvalidCueIndex)
         useTargetSpecific = true;
      if(specificSoundCue == cInvalidCueIndex)
         useTargetSpecific = false;

      if(useTargetSpecific)
         soundCue = specificSoundCue;
      else
         soundCue = genericSoundCue;      
      
      if(soundCue != cInvalidCueIndex)
      {
         gWorld->getWorldSoundManager()->addSound(this, -1, soundCue, true, cInvalidCueIndex, true, true);
         return true;
      }      
   }

   return false;
}


//==============================================================================
//==============================================================================
float BSquad::getPathingRadius() const
{
   // Get squad obstruction radius
   float pathingRadius = this->getObstructionRadius();

   // Fallback to the radius of the largest unit
   if (pathingRadius < cFloatCompareEpsilon)
   {
      uint unitIndex;
      for (unitIndex=0; unitIndex<this->getNumberChildren(); unitIndex++)
      {
         const BUnit* pUnit = gWorld->getUnit(this->getChild(unitIndex));
         if (pUnit)
            pathingRadius = Math::Max(pathingRadius, pUnit->getObstructionRadius());
      }
   }

   return (pathingRadius);
}

//==============================================================================
float BSquad::getLargestUnitRadius() const
{
   float pathingRadius = 0;

   uint unitIndex;
   for (unitIndex=0; unitIndex<this->getNumberChildren(); unitIndex++)
   {
      const BUnit* pUnit = gWorld->getUnit(this->getChild(unitIndex));
      if (pUnit)
         pathingRadius = Math::Max(pathingRadius, pUnit->getObstructionRadius());
   }

   return (pathingRadius);
}

//==============================================================================
//==============================================================================
void BSquad::refreshVisibleSquads()
{
   //Get our player.
   BPlayer* pPlayer=getPlayer();
   BASSERT(pPlayer);

   //Zero the old lists and save the time.
   mVisibleSquads.resize(0);
   mVisibleEnemySquads.resize(0);
   
   //Get the ranges.
   float searchRange=0.0f;
   //New search.
   float los=getLOS();
   float attackRange=getAttackRange();
   float leashDistance=getLeashDistance();
   float obsX=getObstructionRadiusX();
   float obsZ=getObstructionRadiusZ();
   float obsDiag=Math::fSqrt(obsX*obsX + obsZ*obsZ);
   if (attackRange+leashDistance > los)
      searchRange=attackRange+leashDistance+obsDiag;
   else
      searchRange=los+obsDiag;

   //Do the query for anything we might find.
   static BEntityIDArray results(0, 512);
   results.resize(0);
   BUnitQuery query(getPosition(), searchRange, false);
   query.setUnitVisibility(pPlayer->getID());
   query.setRelation(pPlayer->getID(), cRelationTypeAny);
   query.setFlagIgnoreDead(true);
   gWorld->getSquadsInArea(&query, &results);
   if (results.getNumber() <= 0)
      return;

   //Go through the results.
   for (uint i=0; i < results.getSize(); i++)
   {
      BSquad* pTargetSquad=gWorld->getSquad(results[i]);
      BASSERT(pTargetSquad);

      //Add it as a visible squad.
      mVisibleSquads.add(pTargetSquad->getID());
      //If we're enemy to it, add it to the enemy list.
      if (pPlayer->isEnemy(pTargetSquad->getPlayerID()))
         mVisibleEnemySquads.add(pTargetSquad->getID());
   }
}

//==============================================================================
//==============================================================================
void BSquad::adjustDamageBank(float val)
{ 
   mDamageBank += val; 

   //-- Reset the damage bank timer
   mDamageBankTimer = gDatabase.getDamageBankTimer();
}

//==============================================================================
//==============================================================================
void BSquad::updateDamageBank(float elapsed)
{
   if(mDamageBankTimer == -1.0f)
      return;

   if(mDamageBankTimer <= elapsed)
   {
      mDamageBank = 0.0f;
      mDamageBankTimer = -1.0f;
   }
   else
      mDamageBankTimer -= elapsed;
}

//==============================================================================
//==============================================================================
void BSquad::setFlagLockedDown(bool v)
{
   BEntity::setFlagLockedDown(v);
   for (uint i=0; i<getNumberChildren(); i++)
   {
      BUnit* pUnit=gWorld->getUnit(getChild(i));
      if (pUnit)
         pUnit->setFlagLockedDown(v);
   }
}
//==============================================================================
//==============================================================================
void BSquad::setFlagInSniper(bool v)
{
   BEntity::setFlagInSniper(v);
   for (uint i=0; i<getNumberChildren(); i++)
   {
      BUnit* pUnit=gWorld->getUnit(getChild(i));
      if (pUnit)
         pUnit->setFlagInSniper(v);
   }
}

//==============================================================================
//==============================================================================
void BSquad::setFlagSprinting(bool v)
{
   BEntity::setFlagSprinting(v);
   for (uint i=0; i<getNumberChildren(); i++)
   {
      BUnit* pUnit=gWorld->getUnit(getChild(i));
      if (pUnit)
      {
         pUnit->setFlagSprinting(v);
         pUnit->computeVisual();
      }
   }
}

//==============================================================================
//==============================================================================
void BSquad::setFlagRecovering(bool v)
{
   BEntity::setFlagRecovering(v);
   for (uint i=0; i<getNumberChildren(); i++)
   {
      BUnit* pUnit=gWorld->getUnit(getChild(i));
      if (pUnit)
      {
         pUnit->setFlagRecovering(v);
         pUnit->computeVisual();
      }
   }
}

//==============================================================================
//==============================================================================
void BSquad::setFlagChangingMode(bool v) 
{ 
   mFlagChangingMode = v; 
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
      {
         pUnit->setFlagParentSquadChangingMode(v);
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquad::getFlagForceDisplayHP() const
{
   bool result = mFlagForceDisplayHP;
   const BProtoSquad* pProtoSquad = getProtoSquad();
   if (pProtoSquad)
   {
      result = (mFlagForceDisplayHP || pProtoSquad->getFlagAlwaysShowHPBar());
   }

   return (result);
}

//==============================================================================
//==============================================================================
void BSquad::setFlagForceDisplayHP(bool v)
{
   mFlagForceDisplayHP=v;

   if (hasDamageProxy())
   {
      BSquad* pDmgProxy = gWorld->getSquad(getDamageProxy());
      if (pDmgProxy && v)
         pDmgProxy->setFlagForceDisplayHP(v);
   }
}

//==============================================================================
//==============================================================================
void BSquad::activateEffect(int effectType, bool on)
{
#if 0 // disble the recovering particle effect -- we now have a new recharge meter
   if (mFlagHasActiveEffect)
   {
      uint numRefs = mpEntityRefs->getSize();
      for (uint i=0; i<numRefs; i++)
      {
         BEntityRef ref = (*mpEntityRefs)[i];
         if (ref.mType == BEntityRef::cTypeEffect && ref.mData2 == effectType)
         {
            BObject* pObject = gWorld->getObject(ref.mID);
            if (pObject && !pObject->getFlagDestroy())
               pObject->kill(false);
         }
      }
   }
   
   if (!on)
      return;

   BProtoObjectID protoID = -1;
   switch(effectType)
   {
      case cEffectRecovering:
         protoID = getProtoSquad()->getRecoveringEffect();
         break;
   }
   if (protoID == cInvalidProtoID)
      return;

   BEntityID effectObject = gWorld->createEntity(protoID, false, mPlayerID, mPosition, mForward, mRight, true, false, true);
   if (effectObject == cInvalidObjectID)
      return;

   if (addEntityRef(BEntityRef::cTypeEffect, effectObject, 0, effectType) == NULL)
   {
      const BEntity* pEntity = gWorld->getEntity(effectObject);
      if (pEntity)
         pEntity->kill(false);
   }
   else
   {

      if (getFlagRecovering() && mRecoverTime > 0.0f)
      {
         BObject* pObject = gWorld->getObject(effectObject);
         if (pObject)
         {
            pObject->computeAnimation();
            float len = pObject->getAnimationDuration(cActionAnimationTrack);
            pObject->setAnimationRate(len / mRecoverTime);
            pObject->computeAnimation();
         }
      }
      mFlagHasActiveEffect=true;
   }
#endif
}

//==============================================================================
//==============================================================================
int BSquad::getSelectType(BTeamID teamId) const
{
   if (!isSelectable(teamId))
      return (cSelectTypeNone);

   const BUnit* pUnit = getLeaderUnit();
   if (pUnit)
   {
      if (pUnit->getFlagSelectTypeTarget())
         return cSelectTypeTarget;

      const BProtoObject* pProtoObject = pUnit->getProtoObject();
      if (pProtoObject && pProtoObject->getFlagLockdownMenu())
      {
         if (getFlagChangingMode())
            return cSelectTypeNone;
         else if (getSquadMode() == BSquadAI::cModeLockdown)
            return cSelectTypeCommand;
      }
   }

   const BProtoObject* pProtoObject = getProtoObject();
   if (pProtoObject)
      return pProtoObject->getSelectType();
   else
      return cSelectTypeNone;
}

//==============================================================================
//==============================================================================
int BSquad::getGotoType() const
{
   if (getSquadMode()==BSquadAI::cModeLockdown)
   {
      const BUnit* pUnit = getLeaderUnit();
      if (pUnit && pUnit->getProtoObject()->getFlagLockdownMenu())
         return cGotoTypeBase;
   }
   const BProtoObject* pProtoObject = getProtoObject();
   if (pProtoObject)
      return pProtoObject->getGotoType();
   else
      return cGotoTypeNone;
}

//==============================================================================
//==============================================================================
void BSquad::updateMovementSound(float elapsedTime)
{
   if(mMovementSoundTimer != -1.0f)
   {
      mMovementSoundTimer -= elapsedTime;
      if(mMovementSoundTimer <= 0.0f)
      {
         mMovementSoundTimer = 0.0f;
         bool moving = isMoving();        
         playMovementSound(moving);
      }
   }
}

//==============================================================================
//==============================================================================
void  BSquad::playMovementSound(bool startMove, bool force)
{
   if(mMovementSoundOn == startMove)
   {
      mMovementSoundTimer = -1.0f; //-- We're in the state we want to be, turn off the timer.
      return;
   }

   if(mMovementSoundTimer == -1.0f && !force) //-- This is the first call, set timer and return
   {
      mMovementSoundTimer = 0.01f; //-- Delay 10ms
      return;
   }
   else if(mMovementSoundTimer > 0.0f && !force) //-- The timer is still running, return
      return;
   else
      mMovementSoundTimer = -1.0f; //-- The timer is done, reset it and continue

   //-- Track whether we're playing or not.
   mMovementSoundOn = startMove;

   //-- start/stop movement sound on all children   
   for(uint i=0; i < getNumberChildren(); i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if(!pUnit)
         continue;

      BCueIndex cueIndex = cInvalidCueIndex;
      if(startMove)
         cueIndex = pUnit->getProtoObject()->getSound(cObjectSoundStartMove);
      else
         cueIndex = pUnit->getProtoObject()->getSound(cObjectSoundStopMove);

      BRTPCInitArray rtpc;
      rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::FOW_DISTANCE, 0.0f) );
      rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::ENGINE_VELOCITY, 0.0f) );
      gWorld->getWorldSoundManager()->addSound(pUnit, -1, cueIndex, false, cInvalidCueIndex, false, false, &rtpc);   
   }

   //-- If there is a squad movement sound then we should play that one now.
   BCueIndex cueIndex = cInvalidCueIndex;
   const BProtoSquad* pProtoSquad = getProtoSquad();
   if(pProtoSquad)
   {
      if(startMove)
         cueIndex = pProtoSquad->getSound(cSquadSoundStartMove);
      else
         cueIndex = pProtoSquad->getSound(cSquadSoundStopMove);
   }
   if(cueIndex != cInvalidCueIndex)
   {
      BRTPCInitArray rtpc;
      rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::FOW_DISTANCE, 0.0f) );
      rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::ENGINE_VELOCITY, 0.0f) );
      gWorld->getWorldSoundManager()->addSound(this, -1, cueIndex, false, cInvalidCueIndex, false, false, &rtpc);   
   }
}

//==============================================================================
//==============================================================================
void BSquad::setAbilityMovementSpeedModifier(int modifierType, float speedModifier, bool reset)
{
   if (speedModifier == 0.0f)
      return;

   setAbilityMovementSpeedModifier(speedModifier, reset);

   if (modifierType == BAbility::cMovementModifierMode)
   {
      if (reset)
         mModeMovementModifier = 0.0f;
      else
         mModeMovementModifier = speedModifier;
   }
}

//==============================================================================
//==============================================================================
void BSquad::setAbilityMovementSpeedModifier(float speedModifier, bool reset)
{
   syncSquadData("setAbilityMovementSpeedModifier, modifier", speedModifier);
   syncSquadData("setAbilityMovementSpeedModifier, reset", reset);
   
   if (speedModifier == 0.0f)
      return;

   if (reset)
      speedModifier = 1.0f / speedModifier;

   int numChildren = getNumberChildren();
   for (int i=0; i<numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
         pUnit->adjustVelocityScalar(speedModifier);
   }
}

//==============================================================================
//==============================================================================
void BSquad::setAbilityAccuracyModifier(float accuracyModifier, bool reset)
{
   if (accuracyModifier == 0.0f)
      return;

   if (reset)
      accuracyModifier = 1.0f / accuracyModifier;

   int numChildren = getNumberChildren();
   for (int i=0; i<numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
         pUnit->adjustAccuracyScalar(accuracyModifier);
   }
}

//==============================================================================
//==============================================================================
void BSquad::setAbilityDodgeModifier(float dodgeModifier, bool reset)
{
   if (dodgeModifier == 0.0f)
      return;

   // DMG NOTE: May need to reverse this so that its 1.0 normally and not when resetting
   if (reset)
      dodgeModifier = 1.0f / dodgeModifier;

   int numChildren = getNumberChildren();
   for (int i=0; i<numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
         pUnit->adjustDodgeScalar(dodgeModifier);
   }
}

//==============================================================================
//==============================================================================
void BSquad::setAbilityDamageModifier(float damageModifier, bool reset)
{
   if (damageModifier == 0.0f)
      return;

   if (reset)
      damageModifier = 1.0f / damageModifier;

   int numChildren = getNumberChildren();
   for (int i=0; i<numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
         pUnit->adjustDamageModifier(damageModifier);
   }
}

//==============================================================================
//==============================================================================
void BSquad::setAbilityDamageTakenModifier(float damageTakenModifier, bool reset)
{
   if (damageTakenModifier == 0.0f)
      return;

   if (reset)
      damageTakenModifier = 1.0f / damageTakenModifier;

   int numChildren = getNumberChildren();
   for (int i=0; i<numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
         pUnit->adjustDamageTakenScalar(damageTakenModifier);
   }
}

//==============================================================================
//==============================================================================
BSquad* BSquad::getGarrisonedSpartanSquad() const
{
   if (!getFlagSpartanContainer())
      return NULL;

   // This is a horribly non-robust way of getting the spartan.  It assumes
   // that it is the first garrisoned unit in the leader unit of this squad.
   BUnit* pLeaderUnit = getLeaderUnit();
   if (pLeaderUnit && pLeaderUnit->getFlagHasGarrisoned())
   {
      const BEntityRef* pRef = pLeaderUnit->getFirstEntityRefByType(BEntityRef::cTypeContainUnit);
      if (pRef)
      {
         BUnit* pUnit = gWorld->getUnit(pRef->mID);
         if (pUnit)
         {
            BSquad* pParentSquad = pUnit->getParentSquad();
            if (pParentSquad)
               return pParentSquad;
         }
      }
   }

   return NULL;
}

//==============================================================================
//==============================================================================
BUnit* BSquad::getSpartanContainerUnit() const
{
   if (!getFlagContainedSpartan())
      return NULL;

   // This assumes that the contained spartan is the leader of this squad
   BUnit* pLeaderUnit = getLeaderUnit();
   if (pLeaderUnit && pLeaderUnit->getFlagGarrisoned())
   {
      const BEntityRef* pRef = pLeaderUnit->getFirstEntityRefByType(BEntityRef::cTypeContainingUnit);
      if (pRef)
      {
         BUnit* pUnit = gWorld->getUnit(pRef->mID);
         if (pUnit)
            return pUnit;
      }
   }

   return NULL;
}

//==============================================================================
//==============================================================================
void BSquad::fleeMap(BSquad* pSquad)
{
   BAction* pAction = mActions.getActionByType(BAction::cActionTypeSquadAmbientLife);

   if (pAction)
   {
      BSquadActionAmbientLife* pAmbientLifeAction = (BSquadActionAmbientLife*)pAction;

      if (pAmbientLifeAction)
      {
         pAmbientLifeAction->fleeMap(pSquad);
      }
   }
}

#ifdef _MOVE4
//==============================================================================
// doMove_4 - Currently, it's just the same as the old doMove, cept there will
// not be an order associated here.  
BActionID BSquad::doMove_4(BSimOrder *pOrder, BAction* pParentAction, BSimTarget target, BVector interimTarget, bool platoonMove, bool monitorOpps /*= false*/, bool autoSquadMode /*= false*/, bool forceLeashUpdate /*= false*/)
{
   //If we can't move, don't.
   if (!canMoveForOrder(pOrder))
      return (cInvalidActionID);

   BSquadActionMove* pAction=reinterpret_cast<BSquadActionMove*>(gActionManager.createAction(BAction::cActionTypeSquadMove));
   pAction->setParentAction(pParentAction);
   pAction->setTarget(target);
   pAction->setInterimTarget_4(interimTarget);
   pAction->setFlagPlatoonMove(platoonMove);
   pAction->setFlagMonitorOpps(monitorOpps);
   pAction->setFlagAutoSquadMode(autoSquadMode);
   pAction->setFlagForceLeashUpdate(forceLeashUpdate);
   if (!addAction(pAction, pOrder))
      return (cInvalidActionID);

   return (pAction->getID());
}

//==============================================================================
// hasCompletedMovement_4 - Query from children, requesting to see if its
// okay for them to complete their actions.  
bool BSquad::hasCompletedMovement_4() const
{
   // First, do we have a moveaction?  If not, we've definitely completed movement.
   const BSquadActionMove* pMoveAction = getNonPausedMoveAction_4();
   if (!pMoveAction)
      return true;
   // if the Move action is in WaitOnOpps, then it's done.
   if (pMoveAction->getState() == BAction::cStateWaitOnChildren)
      return true;

   // jce [10/8/2008] -- Also consider "wait" to be completed since we are completed for now and
   // don't want units to run in place while we're paused.      
   if (pMoveAction->getState() == BAction::cStateWait)
      return true;
      
   // Otherwise, we're still busy
   return false;
}

//==============================================================================
// pauseMovement_4
// This routine is called by the platoon on a squad to pause it.  the selfImposed
// flag is generated by the action that requested the pause, and indicates whether
// the action was pausing it's own squad, or asking the platoon to pause someone
// else's squad.
void BSquad::pauseMovement_4(DWORD pauseTime)
{
   BSquadActionMove* pMoveAction = NULL;
   for (int i = 0; i < getNumberActions(); i++)
   {
      BAction* pAction = mActions.getAction(i);
      if (pAction && (pAction->getType() == BAction::cActionTypeSquadMove) && (pAction->getState() != BAction::cStatePaused))
      {
         pMoveAction = (BSquadActionMove *)pAction;
         break;
      }
   }
   if (!pMoveAction)
      return;

   pMoveAction->pauseMovement_4(pauseTime);
}

//==============================================================================
bool BSquad::isMovementPaused_4(DWORD *remainingTime) const
{
   // Get move action if we have one.
   const BSquadActionMove* pMoveAction = getNonPausedMoveAction_4();
   
   // No move action == not paused in movement.
   if (!pMoveAction)
   {
      // Give back time remaining, if requested.
      if(remainingTime)
         *remainingTime = 0;
         
      return(false);
   }
      
   // If the move action is in the wait state, we're moving but currently paused.
   if(pMoveAction->getState() == BAction::cStateWait)
   {
      // Give back time remaining, if requested.
      if(remainingTime)
         *remainingTime = pMoveAction->getPauseTimeRemaining_4();

      return(true);
   }
      
   // Any other state == not paused.
   // Give back time remaining, if requested.
   if(remainingTime)
      *remainingTime = 0;
   return(false);
}

//==============================================================================
bool BSquad::isFlyingSquad_4() const
{ 
   return (getLeaderUnit())?getLeaderUnit()->getFlagFlying():false; 
}

#ifdef NEW_TURNRADIUS
   //==============================================================================
   //==============================================================================
   BActionState BSquad::getTurnRadiusState() const
   {
      if (!mFlagUpdateTurnRadius)
         return BAction::cStateDone;

      BVector direction = getPosition() - mTurnRadiusPos;
      direction.y = 0;
      float distanceRemaining = direction.length();

      // If in range, done
      const float cActionCompleteEpsilon = 0.1f;
      if (distanceRemaining < cActionCompleteEpsilon)
      {
         // If we want to match the squad's facing direction
         if (mFlagMatchFacing && (mTurnRadiusFwd.dot(getForward()) < FACING_ANGLE_EPSILON))
            return (BAction::cStateWorking);
         // If we caught up to the squad, see if it's done.  If so, then we can be done.
         // if not, go into pathing state (which is, in this case, really just a "retry" state)
         else if (hasCompletedMovement_4())
            return BAction::cStateDone;
         else
            return BAction::cStatePathing;
      }

      return BAction::cStateWorking;
   }

   //==============================================================================
   // Update the mTurnRadiusPos/Fwd.  This is based on the turn code in
   // unitActionMove.
   //==============================================================================
   BActionState BSquad::updateTurning(float elapsed)
   {
      if (!mFlagUpdateTurnRadius)
         return BAction::cStateDone;

      // Desired new position
      BVector newPosition = getPosition();

      // Direction / distance to new position from current (mTurnRadiusPos)
      BVector direction = newPosition - mTurnRadiusPos;
      direction.y = 0;
      float distanceRemaining = direction.length();
      direction.safeNormalize();
      BVector newFwd = direction;

      // If in range, done
      const float cActionCompleteEpsilon = 0.1f;
      if (distanceRemaining < cActionCompleteEpsilon)
      {
         // If we want to match the squad's facing direction
         BVector forward = getForward();
         if (mFlagMatchFacing && (mTurnRadiusFwd.dot(forward) < FACING_ANGLE_EPSILON))
         {
            // Reset direction to squad's facing direction
            newFwd = forward;
         }
         // If we caught up to the squad, see if it's done.  If so, then we can be done.
         // if not, go into pathing state (which is, in this case, really just a "retry" state)
         else if (hasCompletedMovement_4())
         {
            // Hack to "endMove" on non-sliding physics vehicle when the turn radius
            // movement is done.  This is so the units go immediately to idle once they reach
            // the destination
            if (isSquadAPhysicsVehicle())
            {
               BUnit* pUnit = getLeaderUnit();
               if (pUnit && pUnit->isMoving())
               {
                  BPhysicsObject* pPO = pUnit->getPhysicsObject();
                  if (pPO && pPO->getNumActions() > 0)
                  {
                     hkpAction* pHKAction = pPO->getAction(0);
                     if (pHKAction && (pHKAction->getUserData() == BPhysicsInfo::cGround) && pPO->isDeactivationEnabled())
                     {
                        pUnit->endMove();
                     }
                  }
               }
            }

            return BAction::cStateDone;
         }
         else
            return BAction::cStatePathing;
      }

      // jce [8/28/2008] -- Instead of using action velocity, use a velocity based on how far out of position we are.
      // Since the unit might be far behind the squad, we don't necessarily want to slow down when the squad slows down/stops.
      const float cMaxSpeedupFactor = 2.0f;
      const float cCatchUpFastestDistance = 10.0f;
      float speedupFactor = Math::fSmoothStep(0.0f, 1.0f, distanceRemaining/cCatchUpFastestDistance);
      float baseVelocity = getDesiredVelocity();
      float fastestVelocity = cMaxSpeedupFactor*baseVelocity;
      float velocity = Math::Lerp(baseVelocity, fastestVelocity, speedupFactor);

      // Clamp distance
      float positionChangeScalar = elapsed * velocity;
      // Any time the positionChangeScalar is greater than the distance remaining, snap to the target position.
      if (positionChangeScalar > distanceRemaining)
      {
         positionChangeScalar = distanceRemaining;
      }

      // New position / direction to update
      BVector newPos = mTurnRadiusPos + (direction * positionChangeScalar);

      // Adjust the position and direction based on the turn rate / radius
      calcTurning(elapsed, positionChangeScalar, distanceRemaining, mTurnRadiusPos, mTurnRadiusFwd, newPos, newFwd);

      // Update mTurnRadiusPos/Fwd
      mTurnRadiusPos = newPos;
      mTurnRadiusPos.y = 0.0f;
      mTurnRadiusFwd = newFwd;

      return BAction::cStateWorking;
   }

   //==============================================================================
   // This is lifted directly from the same routine that used to exist at the 
   // unit level.
   //==============================================================================
   bool BSquad::calcTurning(float elapsedTime, float distanceChange, 
      float distanceRemainingToTarget, BVector currentPos, BVector currentFwd,
      BVector& newPosition, BVector& newForward)
   {
      // newPosition and newForward are already set to desired values.
      // This function adjusts them if the turn is too great.

      // Smoothly rotate towards the requested angle
      float unitAngle = currentFwd.getAngleAroundY();
      float reqAngle = newForward.getAngleAroundY();
      float angleDiff = reqAngle - unitAngle;
      //pSquad->setFlagTurning((angleDiff >= XMConvertToRadians(5.0f)));
      if (angleDiff >= -cFloatCompareEpsilon && angleDiff <= cFloatCompareEpsilon)
      {
         // If the difference between the angles is almost zero, don't need to adjust forward vector.
         return false;
      }

      // Get turn rate
      // We might have to get the turn rate for the leader unit in the squad.
      BUnit *pLeaderUnit = getLeaderUnit();
      if (!pLeaderUnit)
         return false;
      float turnSpeed = pLeaderUnit->getProtoObject()->getTurnRate() * cRadiansPerDegree;
      if (turnSpeed < cFloatCompareEpsilon)
         return false;

      if (angleDiff > cPi)
         angleDiff = -(cPi + cPi - angleDiff);
      else if (angleDiff < -cPi)
         angleDiff = cPi + cPi + angleDiff;

      // ajl 1/9/02 - Yaw at a constant speed. The old way would yaw much 
      // slower as the angle difference got smaller. I tried to keep some
      // of the nice smoothness that the old way had by slowing the turn
      // speed down a little when the angle difference is less than 45 degrees.
      float yawAmount;
      float absDiff = (float) fabs(angleDiff);

      // jce [10/2/2008] -- Speedup based on how far from correct we are.
      const float cMaxSpeedupFactor = 2.0f;
      const float cCatchUpFastestAngle = cPiOver2;
      float speedupFactor = Math::fSmoothStep(0.0f, 1.0f, absDiff/cCatchUpFastestAngle);
      float fastestTurnSpeed = cMaxSpeedupFactor*turnSpeed;
      turnSpeed = Math::Lerp(turnSpeed, fastestTurnSpeed, speedupFactor);

      // MPB - Remove this for now because it makes some jerky motion at the end of turns
      /*
      if (absDiff < cPi * 0.03125)
         turnSpeed *= 0.125f;
      else if (absDiff < cPi * 0.0625)
         turnSpeed *= 0.25f;
      else if (absDiff < cPiOver8)
         turnSpeed *= 0.50f;
      else if (absDiff < cPiOver4)
         turnSpeed *= 0.75f;
      */

      float maxTurn = turnSpeed * elapsedTime;
      if (absDiff > maxTurn)
      {
         if (angleDiff < 0.0f)
            yawAmount = -maxTurn;
         else
            yawAmount = maxTurn;
      }
      else
      {
         //yawAmount = angleDiff;
         // Turn isn't too large so don't need to adjust forward vector.
         return false;
      }

      // New forward
      BMatrix tempMatrix;
      tempMatrix.makeRotateArbitrary(yawAmount, getUp());
      tempMatrix.transformVector(currentFwd, newForward);
      newForward.normalize();

      // Limit distanceChange based on turn radius
      const BProtoSquad* pPS = getProtoSquad();
      if (pPS && pPS->hasTurnRadius())
      {
         float minTurnRadius = pPS->getTurnRadiusMin();
         //float maxTurnRadius = pPS->getTurnRadiusMax();
         float minVel = minTurnRadius * turnSpeed;
         float maxVel = (elapsedTime > cFloatCompareEpsilon) ? (distanceChange / elapsedTime) : 100.0f;
         const float cutoffAngle = cPiOver2;
         float interpFactor = Math::Min(absDiff / cutoffAngle, 1.0f);
         float newVel = Math::Lerp(maxVel, minVel, interpFactor);
         distanceChange = newVel * elapsedTime;
      }

      // Scale distance to move by how long it will take to complete the turn
      if ((distanceRemainingToTarget > cFloatCompareEpsilon) && (absDiff > maxTurn))
      {
         // Decrease clamp distance by some factor since moving forward while turning may increase
         // total turn than if rotating in place
         float clampDist = distanceRemainingToTarget * maxTurn / absDiff * 0.5f;
         if (clampDist < distanceChange)
            distanceChange = clampDist;
      }

      // Determine position in new direction
      newPosition = currentPos;
      newPosition += (newForward * distanceChange);

      /*
      // If pathing around obstruction then see if tighter turning needed
      if ((mPathMoveData != NULL) && (mPathMoveData->mPathLevel != BPathMoveData::cPathLevelUser))
      {
         static BEntityIDArray ignoreUnits;
         static BEntityIDArray collisions;
         BVector intersectionPoint;
         buildIgnoreList(ignoreUnits, false);

         BSimCollisionResult collisionResult = pUnit->checkForCollisions(ignoreUnits, pUnit->getPosition(), newPosition, mPathMoveData->mPathTime, true, collisions, &intersectionPoint);
         if (collisionResult != cSimCollisionNone)
         {
            newPosition = intersectionPoint;
         }
      }
      */

      return true;
   }
#endif // NEW_TURNRADIUS


//==============================================================================
const BSquadActionMove* BSquad::getNonPausedMoveAction_4() const
{
   // Get the move action.  Make sure it isn't a paused move action, which can happen if attack move is happening.
   for (int i = 0; i < getNumberActions(); i++)
   {
      const BAction* pAction = getActionByIndexConst(i);
      if (pAction && (pAction->getType() == BAction::cActionTypeSquadMove) && (pAction->getState() != BAction::cStatePaused))
         return (reinterpret_cast<const BSquadActionMove*>(pAction));
   }
   return NULL;
}

//==============================================================================
BSquadActionMove* BSquad::getNonPausedMoveAction_4()
{
   // Get the move action.  Make sure it isn't a paused move action, which can happen if attack move is happening.
   for (int i = 0; i < getNumberActions(); i++)
   {
      BAction* pAction = getActionByIndex(i);
      if (pAction && (pAction->getType() == BAction::cActionTypeSquadMove) && (pAction->getState() != BAction::cStatePaused))
         return (reinterpret_cast<BSquadActionMove*>(pAction));
   }
   return NULL;
}


//==============================================================================
BSquadActionMove* BSquad::getMoveAction_4()
{
   // Get the move action.
   for (int i = 0; i < getNumberActions(); i++)
   {
      BAction* pAction = getActionByIndex(i);
      if (pAction && (pAction->getType() == BAction::cActionTypeSquadMove))
         return (reinterpret_cast<BSquadActionMove*>(pAction));
   }
   return NULL;
}


//==============================================================================
int BSquad::getMoveActionPath_4(BVector start, BVector end, long &findPathResult, BPath *pOutPath)
{
   // First, do we have a moveaction?  If not, no path
   BSquadActionMove* pMoveAction = getNonPausedMoveAction_4();
   if (!pMoveAction)
      return BSquadActionMove::cFindPathFailedNoAction;
   int result = pMoveAction->findLowLevelPath_4(start, end, findPathResult, false, pOutPath);

   // End obMgr..
   if (gObsManager.inUse())
      gObsManager.end();

   return result;
}

//==============================================================================
void BSquad::setMoveActionPath_4(BPath &newPath)
{
   // First, do we have a moveaction?  If not, no path
   BSquadActionMove* pMoveAction = getNonPausedMoveAction_4();
   if (!pMoveAction)
      return;

   pMoveAction->setMoveActionPath_4(newPath);
   return;

}


#endif // _MOVE4

//==============================================================================
const BVector BSquad::getInterpolatedPosition() const
{
   if (!gEnableSubUpdating)
      return getPosition();

   float interpolation = gWorld->getUpdateCompletion(mSubUpdateNumber); 
   if (interpolation == 1.0f)
      return mPosition;

   BVector position;
   position.lerpPosition(interpolation, mOldPosition, mPosition);
   return position;
}

//==============================================================================
const BVector BSquad::getInterpolatedForward() const
{
   if (!gEnableSubUpdating)
      return getForward();

   float interpolation = gWorld->getUpdateCompletion(mSubUpdateNumber); 
   if (interpolation == 1.0f)
      return mForward;

   BQuaternion quat1(mOldForward, mOldUp, mOldRight);
   BQuaternion quat2(mForward, mUp, mRight);
   BQuaternion quat3;
   quat1.slerp(quat2, interpolation, quat3);
   BMatrix interpolatedMatrix;
   quat3.toMatrix(interpolatedMatrix);
   BVector v;
   interpolatedMatrix.getForward(v);
   return v;
}

//==============================================================================
const BVector BSquad::getInterpolatedUp() const
{
   if (!gEnableSubUpdating)
      return getUp();

   float interpolation = gWorld->getUpdateCompletion(mSubUpdateNumber);
   if (interpolation == 1.0f)
      return mUp;

   BQuaternion quat1(mOldForward, mOldUp, mOldRight);
   BQuaternion quat2(mForward, mUp, mRight);
   BQuaternion quat3;
   quat1.slerp(quat2, interpolation, quat3);
   BMatrix interpolatedMatrix;
   quat3.toMatrix(interpolatedMatrix);
   BVector v;
   interpolatedMatrix.getUp(v);
   return v;
}

//==============================================================================
const BVector BSquad::getInterpolatedRight() const
{
   if (!gEnableSubUpdating)
      return getRight();

   float interpolation = gWorld->getUpdateCompletion(mSubUpdateNumber);
   if (interpolation == 1.0f)
      return mRight;

   BQuaternion quat1(mOldForward, mOldUp, mOldRight);
   BQuaternion quat2(mForward, mUp, mRight);
   BQuaternion quat3;
   quat1.slerp(quat2, interpolation, quat3);
   BMatrix interpolatedMatrix;
   quat3.toMatrix(interpolatedMatrix);
   BVector v;
   interpolatedMatrix.getRight(v);
   return v;
}

//==============================================================================
//==============================================================================
void BSquad::resetSubUpdating()
{
   mOldPosition = mPosition;
   mOldRight = mRight;
   mOldForward = mForward;
   mOldUp = mUp;
   mSubUpdateNumber = gWorld->getSubUpdate();
}

//==============================================================================
//==============================================================================
bool BSquad::save(BStream* pStream, int saveType) const
{
   if (!BEntity::save(pStream, saveType))
      return false;

   GFWRITEVAR(pStream, long, mProtoID);
   GFWRITECLASS(pStream, saveType, mSquadAI);
   GFWRITEVECTOR(pStream, mOldPosition);
   GFWRITEVECTOR(pStream, mOldRight);
   GFWRITEVECTOR(pStream, mOldUp);
   GFWRITEVECTOR(pStream, mOldForward);
   GFWRITEVECTOR(pStream, mLeashPosition);
   GFWRITEVECTOR(pStream, mAnchorPosition);
   #ifdef NEW_TURNRADIUS
      GFWRITEVECTOR(pStream, mTurnRadiusPos);
      GFWRITEVECTOR(pStream, mTurnRadiusFwd);
   #else
      GFWRITEVECTOR(pStream, cOriginVector);
      GFWRITEVECTOR(pStream, cOriginVector);
   #endif
   GFWRITEARRAY(pStream, BEntityID, mChildren, uint8, 50);
   GFWRITECLASSARRAY(pStream, saveType, mOrders, uint8, 200);
   GFWRITEVAR(pStream, float, mDamageBank);
   GFWRITEVAR(pStream, float, mDamageBankTimer);
   GFWRITECLASSPTR(pStream, saveType, mpFormation);
   GFWRITEVAR(pStream, DWORD, mSubUpdateNumber);
   GFWRITEVAR(pStream, DWORD, mLastDamagedTime);
   GFWRITEVAR(pStream, DWORD, mLastAttackedTime);
   GFWRITEVAR(pStream, DWORD, mLastMoveTime);
   //int mSelectionDecal;
   GFWRITEVAR(pStream, BAIMissionID, mAIMissionID);
   GFWRITEVAR(pStream, float, mXP);
   GFWRITEVAR(pStream, float, mXPBank);
   GFWRITEVAR(pStream, int, mLevel);
   GFWRITEVAR(pStream, int, mLastCommandType);
   GFWRITEVAR(pStream, BEntityID, mLastAbilityAttackTargetID);
   GFWRITEVAR(pStream, int, mChangingToSquadMode);
   GFWRITEVAR(pStream, float, mRecoverTime);
   GFWRITEVAR(pStream, int, mRecoverType);
   GFWRITEVAR(pStream, float, mMovementSoundTimer);
   GFWRITEVAR(pStream, float, mModeMovementModifier);
   GFWRITEVAR(pStream, float, mStasisSpeedMult);
   GFWRITEVAR(pStream, int8, mNumStasisEffects);
   GFWRITEVAR(pStream, float, mAbilityTime);
   GFWRITEVAR(pStream, BEntityID, mLastAttackTargetID);
   GFWRITEVAR(pStream, int8, mLastAttackPriority);
   GFWRITEVAR(pStream, int8, mMergeCount);
   GFWRITEPTR(pStream, sizeof(float)*cSquadSoundStateMax, mChatterTimer);
   GFWRITEPTR(pStream, sizeof(int8)*cSmartTargetMax, mSmartTargetValues);
   GFWRITEVAR(pStream, long, mLastKnownLeaderProtoObjectID);
   GFWRITEVAR(pStream, BPlayerID, mKillerPlayerID);
   GFWRITEVAR(pStream, long, mKillerProtoID);
   GFWRITEVAR(pStream, long, mKillerProtoSquadID);
   GFWRITEPTR(pStream, sizeof(BKBSquadID)*cMaximumSupportedTeams, mKBSquadIDs);
   GFWRITEVAR(pStream, BEntityID, mDamageProxy);
   GFWRITEVAR(pStream, DWORD, mLastTrueLOSCheckCRC);
   GFWRITEVAR(pStream, long, mMergedTypes);

   GFWRITEBITBOOL(pStream, mFlagProtoSquad);
   GFWRITEBITBOOL(pStream, mFlagAlive);
   GFWRITEBITBOOL(pStream, mFlagHasShield);
   GFWRITEBITBOOL(pStream, mFlagShieldDamaged);
   GFWRITEBITBOOL(pStream, mFlagChangingMode);
   GFWRITEBITBOOL(pStream, mFlagDoNextCommand);
   GFWRITEBITBOOL(pStream, mFlagPlayingBlockingAnimation);
   GFWRITEBITBOOL(pStream, mFlagDontPopCommand);
   GFWRITEBITBOOL(pStream, mFlagHasHPBar);
   GFWRITEBITBOOL(pStream, mFlagForceDisplayHP);
   GFWRITEBITBOOL(pStream, mFlagIgnorePop);
   GFWRITEBITBOOL(pStream, mFlagIgnoreLeash);
   GFWRITEBITBOOL(pStream, mFlagVisibleLastUpdate);
   GFWRITEBITBOOL(pStream, mFlagIsUngarrisoning);
   GFWRITEBITBOOL(pStream, mFlagIsGarrisoning);
   GFWRITEBITBOOL(pStream, mFlagIsRepairing);
   GFWRITEBITBOOL(pStream, mFlagSquadExistSoundPlaying);
   GFWRITEBITBOOL(pStream, mFlagSquadChatter);
   GFWRITEBITBOOL(pStream, mFlagIsHitching);
   GFWRITEBITBOOL(pStream, mFlagIsUnhitching);
   GFWRITEBITBOOL(pStream, mReverseMoveDone);
   GFWRITEBITBOOL(pStream, mForceUpdateGarrisoned);
   GFWRITEBITBOOL(pStream, mMovementSoundOn);
   GFWRITEBITBOOL(pStream, mFlagHasActiveEffect);
   GFWRITEBITBOOL(pStream, mKilledByRecycler);
   GFWRITEBITBOOL(pStream, mFlagAttackBlocked);
   GFWRITEBITBOOL(pStream, mFlagUsingTimedAbility);
   GFWRITEBITBOOL(pStream, mFlagCloaked);
   GFWRITEBITBOOL(pStream, mFlagCloakDetected);
   GFWRITEBITBOOL(pStream, mFlagWantsToCloak);
   GFWRITEBITBOOL(pStream, mFlagUpdateGarrisonedSquadPositions);
   GFWRITEBITBOOL(pStream, mFlagSpartanVeterancy);
   GFWRITEBITBOOL(pStream, mFlagSpartanContainer);
   GFWRITEBITBOOL(pStream, mFlagContainedSpartan);
   GFWRITEBITBOOL(pStream, mFlagDazeImmobilized);
   GFWRITEBITBOOL(pStream, mFlagCryoFrozen);
   GFWRITEBITBOOL(pStream, mFlagHasTrueLOS);
   GFWRITEBITBOOL(pStream, mFlagUpdateTurnRadius);
   GFWRITEBITBOOL(pStream, mFlagJumping);
   GFWRITEBITBOOL(pStream, mFlagMatchFacing);
   GFWRITEBITBOOL(pStream, mFlagTargettingSelectionOn);
   GFWRITEBITBOOL(pStream, mFlagIsTransporting);
   GFWRITEBITBOOL(pStream, mFlagNoPlatoonMerge);
   GFWRITEBITBOOL(pStream, mFlagIgnoreAI);
   // [11/5/2008 xemu] not saving the mFlagPreventScoring flag, since it is entirely intra-frame transitory 

   GFWRITEVAL(pStream, bool, (mpObstructionNode != NULL));

   GFWRITEBITBOOL(pStream, mFlagStopShieldRegen);

   GFWRITEMARKER(pStream, cSaveMarkerSquad1);

   return true;
}

//==============================================================================
//==============================================================================
bool BSquad::load(BStream* pStream, int saveType)
{
   if (!BEntity::load(pStream, saveType))
      return false;

   long protoID;
   GFREADVAR(pStream, long, protoID);
   gSaveGame.remapProtoSquadID(protoID);
   mFlagProtoSquad = true;
   setProtoID(protoID);

   GFREADCLASS(pStream, saveType, mSquadAI);
   GFREADVECTOR(pStream, mOldPosition);
   GFREADVECTOR(pStream, mOldRight);
   GFREADVECTOR(pStream, mOldUp);
   GFREADVECTOR(pStream, mOldForward);
   GFREADVECTOR(pStream, mLeashPosition);
   GFREADVECTOR(pStream, mAnchorPosition);
   #ifdef NEW_TURNRADIUS
      GFREADVECTOR(pStream, mTurnRadiusPos);
      GFREADVECTOR(pStream, mTurnRadiusFwd);
   #else
      BVector tempVector;
      GFREADVECTOR(pStream, tempVector);
      GFREADVECTOR(pStream, tempVector);
   #endif
   GFREADARRAY(pStream, BEntityID, mChildren, uint8, 50);
   GFREADCLASSARRAY(pStream, saveType, mOrders, uint8, 200);
   GFREADVAR(pStream, float, mDamageBank);
   GFREADVAR(pStream, float, mDamageBankTimer);

   initFormation();
   GFREADCLASSPTR(pStream, saveType, mpFormation);

   GFREADVAR(pStream, DWORD, mSubUpdateNumber);
   GFREADVAR(pStream, DWORD, mLastDamagedTime);
   GFREADVAR(pStream, DWORD, mLastAttackedTime);
   GFREADVAR(pStream, DWORD, mLastMoveTime);
   //int mSelectionDecal;
   GFREADVAR(pStream, BAIMissionID, mAIMissionID);
   GFREADVAR(pStream, float, mXP);
   GFREADVAR(pStream, float, mXPBank);
   GFREADVAR(pStream, int, mLevel);
   GFREADVAR(pStream, int, mLastCommandType);
   GFREADVAR(pStream, BEntityID, mLastAbilityAttackTargetID);
   GFREADVAR(pStream, int, mChangingToSquadMode);
   GFREADVAR(pStream, float, mRecoverTime);
   GFREADVAR(pStream, int, mRecoverType);
   GFREADVAR(pStream, float, mMovementSoundTimer);
   GFREADVAR(pStream, float, mModeMovementModifier);
   GFREADVAR(pStream, float, mStasisSpeedMult);
   GFREADVAR(pStream, int8, mNumStasisEffects);
   GFREADVAR(pStream, float, mAbilityTime);
   GFREADVAR(pStream, BEntityID, mLastAttackTargetID);
   GFREADVAR(pStream, int8, mLastAttackPriority);
   GFREADVAR(pStream, int8, mMergeCount);
   GFREADPTR(pStream, sizeof(float)*cSquadSoundStateMax, mChatterTimer);
   GFREADPTR(pStream, sizeof(int8)*cSmartTargetMax, mSmartTargetValues);
   GFREADVAR(pStream, long, mLastKnownLeaderProtoObjectID);
   GFREADVAR(pStream, BPlayerID, mKillerPlayerID);
   GFREADVAR(pStream, long, mKillerProtoID);
   GFREADVAR(pStream, long, mKillerProtoSquadID);
   GFREADPTR(pStream, sizeof(BKBSquadID)*cMaximumSupportedTeams, mKBSquadIDs);
   GFREADVAR(pStream, BEntityID, mDamageProxy);
   GFREADVAR(pStream, DWORD, mLastTrueLOSCheckCRC);
   GFREADVAR(pStream, long, mMergedTypes);

   GFREADBITBOOL(pStream, mFlagProtoSquad);
   GFREADBITBOOL(pStream, mFlagAlive);
   GFREADBITBOOL(pStream, mFlagHasShield);
   GFREADBITBOOL(pStream, mFlagShieldDamaged);
   GFREADBITBOOL(pStream, mFlagChangingMode);
   GFREADBITBOOL(pStream, mFlagDoNextCommand);
   GFREADBITBOOL(pStream, mFlagPlayingBlockingAnimation);
   GFREADBITBOOL(pStream, mFlagDontPopCommand);
   GFREADBITBOOL(pStream, mFlagHasHPBar);
   GFREADBITBOOL(pStream, mFlagForceDisplayHP);
   GFREADBITBOOL(pStream, mFlagIgnorePop);
   GFREADBITBOOL(pStream, mFlagIgnoreLeash);
   GFREADBITBOOL(pStream, mFlagVisibleLastUpdate);
   GFREADBITBOOL(pStream, mFlagIsUngarrisoning);
   GFREADBITBOOL(pStream, mFlagIsGarrisoning);
   GFREADBITBOOL(pStream, mFlagIsRepairing);
   GFREADBITBOOL(pStream, mFlagSquadExistSoundPlaying);
   GFREADBITBOOL(pStream, mFlagSquadChatter);
   GFREADBITBOOL(pStream, mFlagIsHitching);
   GFREADBITBOOL(pStream, mFlagIsUnhitching);
   GFREADBITBOOL(pStream, mReverseMoveDone);
   GFREADBITBOOL(pStream, mForceUpdateGarrisoned);
   GFREADBITBOOL(pStream, mMovementSoundOn);
   GFREADBITBOOL(pStream, mFlagHasActiveEffect);
   GFREADBITBOOL(pStream, mKilledByRecycler);
   GFREADBITBOOL(pStream, mFlagAttackBlocked);
   GFREADBITBOOL(pStream, mFlagUsingTimedAbility);
   GFREADBITBOOL(pStream, mFlagCloaked);
   GFREADBITBOOL(pStream, mFlagCloakDetected);
   GFREADBITBOOL(pStream, mFlagWantsToCloak);
   GFREADBITBOOL(pStream, mFlagUpdateGarrisonedSquadPositions);
   GFREADBITBOOL(pStream, mFlagSpartanVeterancy);
   GFREADBITBOOL(pStream, mFlagSpartanContainer);
   GFREADBITBOOL(pStream, mFlagContainedSpartan);
   GFREADBITBOOL(pStream, mFlagDazeImmobilized);
   GFREADBITBOOL(pStream, mFlagCryoFrozen);
   GFREADBITBOOL(pStream, mFlagHasTrueLOS);
   GFREADBITBOOL(pStream, mFlagUpdateTurnRadius);
   GFREADBITBOOL(pStream, mFlagJumping);
   if (mGameFileVersion <= 2)
   {
      bool bTemp;
      GFREADBITBOOL(pStream, bTemp);
   }
   GFREADBITBOOL(pStream, mFlagMatchFacing);
   GFREADBITBOOL(pStream, mFlagTargettingSelectionOn);
   if (mGameFileVersion >= 2)
      GFREADBITBOOL(pStream, mFlagIsTransporting);
   if (mGameFileVersion >= 4)
      GFREADBITBOOL(pStream, mFlagNoPlatoonMerge);
   if (mGameFileVersion >= 6)
      GFREADBITBOOL(pStream, mFlagIgnoreAI);
   mFlagPreventScoring = false; // [11/5/2008 xemu] not entirely sure if this needs to be here, but better safe than sorry 

   bool bObstruction;
   GFREADVAR(pStream, bool, bObstruction);

   if (mGameFileVersion >= 5)
      GFREADBITBOOL(pStream, mFlagStopShieldRegen);

   GFREADMARKER(pStream, cSaveMarkerSquad1);

   // AJL 7/29/08 - Reset for now since the AI isn't saved
   mAIMissionID = cInvalidAIMissionID;

   if (bObstruction)
   {
      bool isPlayerOwned = false;
      const BProtoObject* protoObject = getProtoObject();
      if (protoObject != NULL)
         isPlayerOwned = protoObject->getFlagPlayerOwnsObstruction();
      createObstruction(isPlayerOwned);
   }

   // Remove order entries with NULL orders.  This probably points to an issue where the order
   // wasn't cleaned up properly before saving, but this will at least prevent future crashes.
   for (int i = (mOrders.getNumber() - 1); i >= 0; i--)
   {
      BSimOrder *pTempOrder = mOrders[i].getOrder();
      if (pTempOrder == NULL)
         mOrders.removeIndex(i);
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BSquad::postLoad(int saveType)
{
   if (!BEntity::postLoad(saveType))
      return false;

   BPlayer* pPlayer = getPlayer();
   if (!pPlayer)
      return false;

   // we need to make sure we refresh our leader unit 
   refreshLeaderUnit();

   //initFormation();
   //initPosition(true);

   pPlayer->addSquadToProtoSquad(this, mProtoID);
   //Add to search squads (we do this instead of saving the list off in player).
   pPlayer->addSquadAISquad(this);

   //if (mFlagHasShield)
   //   addAction(gActionManager.createAction(BAction::cActionTypeSquadShieldRegen));

   mSelectionDecal = gDecalManager.createDecal();

   return true;
}

//==============================================================================
// BUser::drawSquadSelection
//==============================================================================
void BSquad::drawSquadSelection(DWORD color, bool bRenderOneFrameOnly, float intensity, bool bIsHover, bool bUseStaticDecal)
{
   if (!gUIManager)
      return;
        
   int decalHandle = getSelectionDecal();
   if (decalHandle == -1)
      return;

   BDecalAttribs* pDecalAttributes = gDecalManager.getDecal(decalHandle);
   if (!pDecalAttributes)
      return;

   if (!isVisibleOnScreen() && !getFlagJumping())
   {
      // Fixes Bug PHX-5257
      pDecalAttributes->setEnabled(false);
      return;  
   }   

   BVector pos = getInterpolatedPosition();
   BVector forward = getInterpolatedForward();
   BVector right = getInterpolatedRight();  
   BVector up = getInterpolatedUp();
   
   XMMATRIX mtx = XMMatrixIdentity();   
   mtx.r[0] = right;
   mtx.r[1] = up;
   mtx.r[2] = forward;

   //-- if the squad has only one child then use the units position because its more
   //-- accurate.  This is a hack because the squad position doesn't really reflect 
   //-- the centroid of all of the units.  If the squad position is fixed 
   //-- we can take this code back out
   if (getNumberChildren() == 1)
   {
      const BUnit* pChild = gWorld->getUnit(getChild(0));
      if (pChild)
      {
         pos = pChild->getInterpolatedPosition();         
         forward = pChild->getInterpolatedForward();
      }
   }
   else if (getNumberChildren() > 1)
   {
      XMVECTOR avgPos = XMVectorZero();
      uint count = getNumberChildren();
      for (uint i = 0; i < count; ++i)
      {
         const BUnit* pUnit = gWorld->getUnit(getChild(i));
         if (!pUnit)
            continue;

         avgPos += pUnit->getInterpolatedPosition(); 
      }
      
      avgPos /= (float) count;
      avgPos.w = 1.0f;

      pos = avgPos;
   }

   const BProtoSquad* pProtoSquad = getProtoSquad();
   const BProtoObject* pProtoObject = getProtoObject();
                        
   // multiple children use the squads position
   // turned of for the spartans.  If we need to turn orientation on a per squad basis again
   // add a flag to the proto squads to toggle this on / off
   bool bAllowOrientation = true;
   if (pProtoSquad)
      bAllowOrientation=pProtoSquad->getSelectionAllowOrientation();
         
   BVec3 decalForward(0.0f, 0.0f, 1.0f);
   if (bAllowOrientation)
      decalForward.set(forward.x,forward.y,forward.z);
      
   pDecalAttributes->setForward(decalForward);      
   pDecalAttributes->setBlendMode(BDecalAttribs::cBlendOver);
   pDecalAttributes->setEnabled(true);
   pDecalAttributes->setIntensity(intensity);
   pDecalAttributes->setRenderOneFrame(bRenderOneFrameOnly);   
   pDecalAttributes->setColor(color);
   
   float sizeX   = 1.0f;
   float sizeZ   = 1.0f;
   float yOffset = 0.0f;
   float selectionIntensity    = 1.0f;
   float hoverIntensity        = 1.0f;
   float subSelectionIntensity = 1.0f;
   BVector offset;

   if (pProtoSquad)   
   {
      const BFlashProtoIcon* pProtoIcon = gUIManager->getDecalProtoIcon(gUIManager->getDecalProtoIconIndex(pProtoSquad->getBaseType()));
      if (pProtoIcon)
      {
         offset = XMLoadHalf4(&pProtoIcon->mOffset);    
         sizeX = XMConvertHalfToFloat(pProtoIcon->mSize.x);
         yOffset = XMConvertHalfToFloat(pProtoIcon->mSize.y);
         sizeZ = XMConvertHalfToFloat(pProtoIcon->mSize.z);
               
         pos = XMVectorAdd(pos, XMVector3Transform(offset, mtx));         

         selectionIntensity    = XMConvertHalfToFloat( pProtoIcon->mIntensity.x);
         hoverIntensity        = XMConvertHalfToFloat( pProtoIcon->mIntensity.y);
         subSelectionIntensity = XMConvertHalfToFloat( pProtoIcon->mIntensity.z);

         intensity = selectionIntensity;
         intensity = (bIsHover) ? hoverIntensity : intensity;         
      }
      else
      {
         sizeX = pProtoSquad->getSelectionRadiusX();
         sizeZ = pProtoSquad->getSelectionRadiusZ();
         yOffset = pProtoSquad->getSelectionYOffset();
      }
            
      pDecalAttributes->setSizeX(sizeX);
      pDecalAttributes->setSizeZ(sizeZ);
      pDecalAttributes->setYOffset(yOffset);

      int decalID = pProtoSquad->getSelectionDecalID();
      if (decalID == -1)
         decalID = gUIGame.getDefaultDecalHandle();

      if (decalID == -1)
         return;

      int flashMovieIndex = gUIManager->getDecalFlashMovieIndex(decalID);
      if (flashMovieIndex == -1)
         return;

      pDecalAttributes->setFlashMovieIndex(flashMovieIndex);
      pDecalAttributes->setConformToTerrain(pProtoSquad->getSelectionConformToTerrain());

      //-- offset the decal in the zDirection if so desired
      float zOffsetScalar = pProtoSquad->getSelectionZOffset();
      if (zOffsetScalar > 0.0f)
      {
         BVector zOffset = forward;
         zOffset.normalize();
         zOffset *= zOffsetScalar;
         pos += zOffset;
      }        

      if (bUseStaticDecal)
      {                  
         BManagedTextureHandle colorTexture = gUIManager->getDecalTextureHandle(pProtoSquad->getStaticDecalID());         
         if (colorTexture != cInvalidManagedTextureHandle)
         {
            float maxSize = Math::Max(sizeX,sizeZ);
            pDecalAttributes->setSizeX(maxSize);
            pDecalAttributes->setSizeZ(maxSize);
            pDecalAttributes->setBlendMode(BDecalAttribs::cBlendOver);
            pDecalAttributes->setFlashMovieIndex(-1);
            pDecalAttributes->setTextureHandle(colorTexture);            
         }
      }
      else if (!bIsHover)
      {                  
         BManagedTextureHandle colorTexture = gUIManager->getDecalTextureHandle(pProtoSquad->getClumpSelectionDecalID());
         BManagedTextureHandle maskTexture = gUIManager->getDecalTextureHandle(pProtoSquad->getClumpSelectionDecalMaskID());

         if ((colorTexture != cInvalidManagedTextureHandle) && (maskTexture != cInvalidManagedTextureHandle))
         {
            float maxSize = Math::Max(sizeX,sizeZ);
            pDecalAttributes->setSizeX(maxSize);
            pDecalAttributes->setSizeZ(maxSize);
            pDecalAttributes->setBlendMode(BDecalAttribs::cBlendStencil);
            pDecalAttributes->setFlashMovieIndex(-1);
            pDecalAttributes->setTextureHandle(colorTexture);
            pDecalAttributes->setStencilTextureHandle(maskTexture);
         }
      }      
   }
   else if (pProtoObject)
   {
      const BFlashProtoIcon* pProtoIcon = gUIManager->getDecalProtoIcon(gUIManager->getDecalProtoIconIndex(pProtoObject->getBaseType()));
      if (pProtoIcon)
      {
         offset = XMLoadHalf4(&pProtoIcon->mOffset);    
         sizeX = XMConvertHalfToFloat(pProtoIcon->mSize.x);
         yOffset = XMConvertHalfToFloat(pProtoIcon->mSize.y);
         sizeZ = XMConvertHalfToFloat(pProtoIcon->mSize.z);
         
         pos = XMVectorAdd(pos, XMVector3Transform(offset, mtx));

         selectionIntensity    = XMConvertHalfToFloat( pProtoIcon->mIntensity.x);
         hoverIntensity        = XMConvertHalfToFloat( pProtoIcon->mIntensity.y);
         subSelectionIntensity = XMConvertHalfToFloat( pProtoIcon->mIntensity.z);

         intensity = selectionIntensity;
         intensity = (bIsHover) ? hoverIntensity : intensity;                  
      }
      else
      {
         sizeX = pProtoObject->getSelectedRadiusX();
         sizeZ = pProtoObject->getSelectedRadiusZ();
         yOffset = 0.0f;
      }
      
      pDecalAttributes->setSizeX(sizeX);
      pDecalAttributes->setSizeZ(sizeZ);
      pDecalAttributes->setYOffset(yOffset);
         
      int decalID = pProtoObject->getSelectionDecalID();
      if (decalID == -1)
         decalID = gUIGame.getDefaultDecalHandle();

      if (decalID == -1)
         return;

      int flashmovieIndex = gUIManager->getDecalFlashMovieIndex(decalID);
      if (flashmovieIndex == -1)
         return;
      
      pDecalAttributes->setFlashMovieIndex(flashmovieIndex);
      pDecalAttributes->setConformToTerrain(true);
   }

   pDecalAttributes->setPos(BVec3(pos.x, pos.y, pos.z));
}

//==============================================================================
//==============================================================================
bool BSquad::pullSquad(BEntityID attackerID, int32 attackerProtoActionID)
{
   const BUnit* pThrowerUnit = gWorld->getUnit(attackerID);
   if (!pThrowerUnit)
      return false;

   BSimOrder* pOrder = gSimOrderManager.createOrder();
   BASSERT(pOrder);
   if (!pOrder)
      return (false);

   float dist = pThrowerUnit->getObstructionRadius() + getObstructionRadius();
   BVector targetPos = (getAveragePosition() - pThrowerUnit->getPosition());

   // Add the attack range in so it tries to find a position just inside attack range
   BTactic* pTactic = pThrowerUnit->getTactic();
   if (pTactic)
   {
      BProtoAction *pThrowerProtoAction = pTactic->getProtoAction(attackerProtoActionID);
      if (pThrowerProtoAction)
      {
         float range = pThrowerProtoAction->getMaxRange(NULL, false, false);
         dist += range;
         if (range > 1.0f)
            dist -= 0.5f;
      }
   }

   if (targetPos.length() <= dist * 1.1f)
      return false;

   targetPos.normalize();

   targetPos *= dist;
   targetPos += pThrowerUnit->getPosition();

   BSimTarget target;
   target.setID(pThrowerUnit->getID());
   target.setPosition(targetPos);

   BDynamicVectorArray waypoints;            
   waypoints.add(getAveragePosition());

   target.setAbilityID(1);
   pOrder->setTarget(target);
   pOrder->setOwnerID(getID());
   pOrder->setPriority(BSimOrder::cPriorityUser);
   pOrder->setWaypoints(waypoints);

   syncSquadData("BSquad::pullSquad attackerID", attackerID.asLong());

   //Figure out the right target to send down.
   //BProtoAction* pProtoAction = pThrowerUnit->getTactic()->getProtoAction(attackerProtoActionID);

   BSquadActionJump* pAction = reinterpret_cast<BSquadActionJump*>(gActionManager.createAction(BAction::cActionTypeSquadJump));
   
   BASSERT(pAction);
      
   pAction->setTarget(target);
   pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);
   pAction->setThrowerProtoActionID(attackerProtoActionID);
   //pAction->setProtoAction(pProtoAction);

   pAction->setJumpType(BSimOrder::cJumpOrderJumpPull);
   addAction(pAction, pOrder);

   return true;
}

//==============================================================================
//==============================================================================
void BSquad::setTargettingSelection(bool on)
{
   if (mFlagTargettingSelectionOn == on)
      return;

   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit)
         pUnit->setTargettingSelection(on);
   }
   
   mFlagTargettingSelectionOn = on;
}

//==============================================================================
//==============================================================================
bool BSquad::isUnderCinematicControl() const
{
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit && pUnit->getFlagIsUnderCinematicControl())
         return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
void BSquad::setPosition(const BVector pos, bool overridePhysics)
{
   #ifdef SYNC_Squad
      syncSquadData("BSquad::setPosition pos", pos);
   #endif

   BEntity::setPosition(pos, overridePhysics);
}

//==============================================================================
//==============================================================================
BPlayerID BSquad::getColorPlayerID() const
{
   const BUnit* pUnit = getLeaderUnit();
   if (pUnit)
      return pUnit->getColorPlayerID();
   else
      return mPlayerID;
}

//==============================================================================
//==============================================================================
void BSquad::setSecondaryTurretScanTokens(bool v)
{
   //gConsole.output(cChannelSim, "giving tokens to squad %d on update %d", getID(), gWorld->getUpdateNumber());
   for(uint i = 0; i < getNumberChildren(); i++)
   {
      BUnit* pUnit = gWorld->getUnit(getChild(i));
      if (pUnit && pUnit->getProtoObject())
      {
         for(uint j = 0; j < pUnit->getProtoObject()->getNumberHardpoints(); j++)
         {
            //gConsole.output(cChannelSim, "  setting token on unit %d hardpoint %d", pUnit->getID(), j);
            pUnit->setFlagSecondaryTurretScanToken(j, true);
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BSquad::determineKillerForDelayedBaseDestruction()
{
   if(getNumberChildren() == 0)
      return;

   bool killedByRecyler = false;
   if(getLeaderUnit())
      getLeaderUnit()->determineKiller(mKillerPlayerID, mKillerProtoID, mKillerProtoSquadID, killedByRecyler);
   mKilledByRecycler = killedByRecyler;
}
